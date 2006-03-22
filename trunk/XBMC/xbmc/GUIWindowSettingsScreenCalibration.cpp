
#include "stdafx.h"
#include "GUIWindowSettingsScreenCalibration.h"
#include "GUIMoverControl.h"
#include "GUIResizeControl.h"
#include "Application.h"
#include "Util.h"


#define CONTROL_LABEL_ROW1  2
#define CONTROL_LABEL_ROW2  3
#define CONTROL_TOP_LEFT  8
#define CONTROL_BOTTOM_RIGHT 9
#define CONTROL_SUBTITLES  10
#define CONTROL_PIXEL_RATIO  11
#define CONTROL_OSD    12
#define CONTROL_VIDEO   20
#define CONTROL_NONE   0

CGUIWindowSettingsScreenCalibration::CGUIWindowSettingsScreenCalibration(void)
    : CGUIWindow(WINDOW_MOVIE_CALIBRATION, "SettingsScreenCalibration.xml")
{
  m_needsScaling = false;         // we handle all the scaling
}

CGUIWindowSettingsScreenCalibration::~CGUIWindowSettingsScreenCalibration(void)
{}


bool CGUIWindowSettingsScreenCalibration::OnAction(const CAction &action)
{
  switch (action.wID)
  {
  case ACTION_PREVIOUS_MENU:
    {
      m_gWindowManager.PreviousWindow();
      return true;
    }
    break;

  case ACTION_CALIBRATE_SWAP_ARROWS:
    {
      NextControl();
      return true;
    }
    break;

  case ACTION_CALIBRATE_RESET:
    {
      g_graphicsContext.ResetScreenParameters(m_Res[m_iCurRes]);
      ResetControls();
      CGUIWindow *pOSD = m_gWindowManager.GetWindow(WINDOW_OSD);
      if (pOSD) pOSD->SetPosition(0, g_settings.m_ResInfo[m_Res[m_iCurRes]].iOSDYOffset);
      return true;
    }
    break;

  case ACTION_CHANGE_RESOLUTION:
    // choose the next resolution in our list
    {
      m_iCurRes++;
      if (m_iCurRes == m_Res.size())
        m_iCurRes = 0;
      Sleep(1000);
      g_graphicsContext.SetGUIResolution(m_Res[m_iCurRes]);
      ResetControls();
      CGUIWindow *pOSD = m_gWindowManager.GetWindow(WINDOW_OSD);
      if (pOSD) pOSD->SetPosition(0, g_settings.m_ResInfo[m_Res[m_iCurRes]].iOSDYOffset);
      return true;
    }
    break;
  }
  return CGUIWindow::OnAction(action); // base class to handle basic movement etc.
}

void CGUIWindowSettingsScreenCalibration::AllocResources(bool forceLoad)
{
  CGUIWindow::AllocResources(forceLoad);
  CGUIWindow *pWindow = m_gWindowManager.GetWindow(WINDOW_OSD);
  if (pWindow) pWindow->AllocResources(true);
}

void CGUIWindowSettingsScreenCalibration::FreeResources(bool forceUnload)
{
  CGUIWindow *pWindow = m_gWindowManager.GetWindow(WINDOW_OSD);
  if (pWindow) pWindow->FreeResources(true);
  CGUIWindow::FreeResources(forceUnload);
}


bool CGUIWindowSettingsScreenCalibration::OnMessage(CGUIMessage& message)
{
  switch ( message.GetMessage() )
  {
  case GUI_MSG_WINDOW_DEINIT:
    {
      CGUIMessage msg(GUI_MSG_WINDOW_DEINIT, 0, 0, 0, 0, NULL);
      g_settings.Save();
      g_graphicsContext.SetCalibrating(false);
      m_gWindowManager.ShowOverlay(true);
      // reset our screen resolution to what it was initially
      g_graphicsContext.SetGUIResolution(g_guiSettings.m_LookAndFeelResolution);
      // Inform the player so we can update the resolution
      if (g_application.m_pPlayer)
        g_application.m_pPlayer->Update();
    }
    break;

  case GUI_MSG_WINDOW_INIT:
    {
      CGUIWindow::OnMessage(message);
      m_gWindowManager.ShowOverlay(false);
      g_graphicsContext.SetCalibrating(true);
      // Inform the player so we can update the resolution
      if (g_application.IsPlayingVideo())
        g_application.m_pPlayer->Update();
      // Get the allowable resolutions that we can calibrate...
      m_Res.clear();
      if (g_application.IsPlayingVideo())
      { // don't allow resolution switching if we are playing a video
        m_iCurRes = 0;
        m_Res.push_back(g_graphicsContext.GetVideoResolution());
        SET_CONTROL_VISIBLE(CONTROL_VIDEO);
      }
      else
      {
        SET_CONTROL_HIDDEN(CONTROL_VIDEO);
        g_graphicsContext.GetAllowedResolutions(m_Res, true);
        // find our starting resolution
        for (UINT i = 0; i < m_Res.size(); i++)
        {
          if (m_Res[i] == g_graphicsContext.GetVideoResolution())
            m_iCurRes = i;
        }
      }
      // Setup the first control
      m_iControl = CONTROL_TOP_LEFT;
      ResetControls();
      return true;
    }
    break;
  case GUI_MSG_CLICKED:
    {
      DWORD test = message.GetSenderId();
      // clicked - change the control...
      NextControl();
    }
    break;
  }
  return CGUIWindow::OnMessage(message);
}

void CGUIWindowSettingsScreenCalibration::NextControl()
{ // set the old control invisible and not focused, and choose the next control
  CGUIControl *pControl = (CGUIControl *)GetControl(m_iControl);
  if (pControl)
  {
    pControl->SetVisible(false);
    pControl->SetFocus(false);
  }
  // switch to the next control
  m_iControl++;
  if (m_iControl > CONTROL_OSD)
    m_iControl = CONTROL_TOP_LEFT;
  // enable the new control
  EnableControl(m_iControl);
}

void CGUIWindowSettingsScreenCalibration::EnableControl(int iControl)
{
  // get the current control
  if (iControl == CONTROL_OSD)
  {
    SET_CONTROL_HIDDEN(CONTROL_TOP_LEFT);
    SET_CONTROL_HIDDEN(CONTROL_BOTTOM_RIGHT);
    SET_CONTROL_HIDDEN(CONTROL_SUBTITLES);
    SET_CONTROL_HIDDEN(CONTROL_PIXEL_RATIO);
    SET_CONTROL_VISIBLE(CONTROL_OSD);
    SET_CONTROL_FOCUS(CONTROL_OSD, 0);
    // show the OSD dialog is handled in Render()
  }
  else
  {
    SET_CONTROL_HIDDEN(CONTROL_OSD);
    SET_CONTROL_VISIBLE(CONTROL_TOP_LEFT);
    SET_CONTROL_VISIBLE(CONTROL_BOTTOM_RIGHT);
    SET_CONTROL_VISIBLE(CONTROL_SUBTITLES);
    SET_CONTROL_VISIBLE(CONTROL_PIXEL_RATIO);
    SET_CONTROL_FOCUS(iControl, 0);
    // hide the OSD dialog
    CGUIDialog *pOSD = (CGUIDialog *)m_gWindowManager.GetWindow(WINDOW_OSD);
    if (pOSD) pOSD->Close(true);
  }
}

void CGUIWindowSettingsScreenCalibration::ResetControls()
{
  // disable the video control, so that our other controls take mouse clicks etc.
  CONTROL_DISABLE(CONTROL_VIDEO);
  // disable the UI calibration for our controls
  // and set their limits
  // also, set them to invisible if they don't have focus
  CGUIMoverControl *pControl = (CGUIMoverControl*)GetControl(CONTROL_TOP_LEFT);
  if (pControl)
  {
    pControl->SetLimits( -g_settings.m_ResInfo[m_Res[m_iCurRes]].iWidth / 4,
                         -g_settings.m_ResInfo[m_Res[m_iCurRes]].iHeight / 4,
                         g_settings.m_ResInfo[m_Res[m_iCurRes]].iWidth / 4,
                         g_settings.m_ResInfo[m_Res[m_iCurRes]].iHeight / 4);
    pControl->SetPosition(g_settings.m_ResInfo[m_Res[m_iCurRes]].Overscan.left,
                          g_settings.m_ResInfo[m_Res[m_iCurRes]].Overscan.top);
    pControl->SetLocation(g_settings.m_ResInfo[m_Res[m_iCurRes]].Overscan.left,
                          g_settings.m_ResInfo[m_Res[m_iCurRes]].Overscan.top, false);
  }
  pControl = (CGUIMoverControl*)GetControl(CONTROL_BOTTOM_RIGHT);
  if (pControl)
  {
    pControl->SetLimits(g_settings.m_ResInfo[m_Res[m_iCurRes]].iWidth*3 / 4,
                        g_settings.m_ResInfo[m_Res[m_iCurRes]].iHeight*3 / 4,
                        g_settings.m_ResInfo[m_Res[m_iCurRes]].iWidth*5 / 4,
                        g_settings.m_ResInfo[m_Res[m_iCurRes]].iHeight*5 / 4);
    pControl->SetPosition(g_settings.m_ResInfo[m_Res[m_iCurRes]].Overscan.right - (int)pControl->GetWidth(),
                          g_settings.m_ResInfo[m_Res[m_iCurRes]].Overscan.bottom - (int)pControl->GetHeight());
    pControl->SetLocation(g_settings.m_ResInfo[m_Res[m_iCurRes]].Overscan.right,
                          g_settings.m_ResInfo[m_Res[m_iCurRes]].Overscan.bottom, false);
  }
  // Subtitles and OSD controls can only move up and down
  pControl = (CGUIMoverControl*)GetControl(CONTROL_SUBTITLES);
  if (pControl)
  {
    pControl->SetLimits(0, g_settings.m_ResInfo[m_Res[m_iCurRes]].iHeight*3 / 4,
                        0, g_settings.m_ResInfo[m_Res[m_iCurRes]].iHeight*5 / 4);
    pControl->SetPosition((g_settings.m_ResInfo[m_Res[m_iCurRes]].iWidth - pControl->GetWidth()) / 2,
                          g_settings.m_ResInfo[m_Res[m_iCurRes]].iSubtitles - (int)pControl->GetHeight());
    pControl->SetLocation(0, g_settings.m_ResInfo[m_Res[m_iCurRes]].iSubtitles, false);
  }
  pControl = (CGUIMoverControl*)GetControl(CONTROL_OSD);
  if (pControl)
  {
    pControl->SetLimits(0, g_settings.m_ResInfo[m_Res[m_iCurRes]].iHeight / 2,
                        0, g_settings.m_ResInfo[m_Res[m_iCurRes]].iHeight*5 / 4);
    pControl->SetPosition((g_settings.m_ResInfo[m_Res[m_iCurRes]].iWidth - pControl->GetWidth()) / 2,
                          g_settings.m_ResInfo[m_Res[m_iCurRes]].iHeight + g_settings.m_ResInfo[m_Res[m_iCurRes]].iOSDYOffset - (int)pControl->GetHeight());
    pControl->SetLocation(0, g_settings.m_ResInfo[m_Res[m_iCurRes]].iHeight + g_settings.m_ResInfo[m_Res[m_iCurRes]].iOSDYOffset, false);
    pControl->SetVisible(false);
  }
  // lastly the pixel ratio control...
  CGUIResizeControl *pResize = (CGUIResizeControl*)GetControl(CONTROL_PIXEL_RATIO);
  if (pResize)
  {
    pResize->SetLimits(g_settings.m_ResInfo[m_Res[m_iCurRes]].iWidth / 4, g_settings.m_ResInfo[m_Res[m_iCurRes]].iHeight / 2,
                       g_settings.m_ResInfo[m_Res[m_iCurRes]].iWidth*3 / 4, g_settings.m_ResInfo[m_Res[m_iCurRes]].iHeight / 2);
    pResize->SetHeight(g_settings.m_ResInfo[m_Res[m_iCurRes]].iHeight / 2);
    pResize->SetWidth((DWORD)(pResize->GetHeight() / g_settings.m_ResInfo[m_Res[m_iCurRes]].fPixelRatio));
    pResize->SetPosition((g_settings.m_ResInfo[m_Res[m_iCurRes]].iWidth - (int)pResize->GetWidth()) / 2,
                         (g_settings.m_ResInfo[m_Res[m_iCurRes]].iHeight - (int)pResize->GetHeight()) / 2);
  }
  // Enable the default control
  EnableControl(m_iControl);
}

void CGUIWindowSettingsScreenCalibration::UpdateFromControl(int iControl)
{
  CStdString strStatus;
  if (iControl == CONTROL_PIXEL_RATIO)
  {
    CGUIResizeControl *pControl = (CGUIResizeControl*)GetControl(CONTROL_PIXEL_RATIO);
    if (pControl)
    {
      float fWidth = (float)pControl->GetWidth();
      float fHeight = (float)pControl->GetHeight();
      g_settings.m_ResInfo[m_Res[m_iCurRes]].fPixelRatio = fHeight / fWidth;
      // recenter our control...
      pControl->SetPosition((g_settings.m_ResInfo[m_Res[m_iCurRes]].iWidth - pControl->GetWidth()) / 2,
                            (g_settings.m_ResInfo[m_Res[m_iCurRes]].iHeight - pControl->GetHeight()) / 2);
      strStatus.Format("%s (%5.3f)", g_localizeStrings.Get(275).c_str(), g_settings.m_ResInfo[m_Res[m_iCurRes]].fPixelRatio);
      SET_CONTROL_LABEL(CONTROL_LABEL_ROW2, 278);
    }
  }
  else
  {
    CGUIMoverControl *pControl = (CGUIMoverControl*)GetControl(iControl);
    if (pControl)
    {
      switch (iControl)
      {
      case CONTROL_TOP_LEFT:
        {
          g_settings.m_ResInfo[m_Res[m_iCurRes]].Overscan.left = pControl->GetXLocation();
          g_settings.m_ResInfo[m_Res[m_iCurRes]].Overscan.top = pControl->GetYLocation();
          strStatus.Format("%s (%i,%i)", g_localizeStrings.Get(272).c_str(), pControl->GetXLocation(), pControl->GetYLocation());
          SET_CONTROL_LABEL(CONTROL_LABEL_ROW2, 276);
        }
        break;

      case CONTROL_BOTTOM_RIGHT:
        {
          g_settings.m_ResInfo[m_Res[m_iCurRes]].Overscan.right = pControl->GetXLocation();
          g_settings.m_ResInfo[m_Res[m_iCurRes]].Overscan.bottom = pControl->GetYLocation();
          int iXOff1 = g_settings.m_ResInfo[m_Res[m_iCurRes]].iWidth - pControl->GetXLocation();
          int iYOff1 = g_settings.m_ResInfo[m_Res[m_iCurRes]].iHeight - pControl->GetYLocation();
          strStatus.Format("%s (%i,%i)", g_localizeStrings.Get(273).c_str(), iXOff1, iYOff1);
          SET_CONTROL_LABEL(CONTROL_LABEL_ROW2, 276);
        }
        break;

      case CONTROL_SUBTITLES:
        {
          g_settings.m_ResInfo[m_Res[m_iCurRes]].iSubtitles = pControl->GetYLocation();
          strStatus.Format("%s (%i)", g_localizeStrings.Get(274).c_str(), pControl->GetYLocation());
          SET_CONTROL_LABEL(CONTROL_LABEL_ROW2, 277);
        }
        break;

      case CONTROL_OSD:
        {
          g_settings.m_ResInfo[m_Res[m_iCurRes]].iOSDYOffset = pControl->GetYLocation() - g_settings.m_ResInfo[m_Res[m_iCurRes]].iHeight;
          strStatus.Format("%s (%i, Offset=%i)", g_localizeStrings.Get(469).c_str(), pControl->GetYLocation(), g_settings.m_ResInfo[m_Res[m_iCurRes]].iOSDYOffset);
          SET_CONTROL_LABEL(CONTROL_LABEL_ROW2, 468);
          CGUIWindow *pOSD = m_gWindowManager.GetWindow(WINDOW_OSD);
          if (pOSD) pOSD->SetPosition(0, g_settings.m_ResInfo[m_Res[m_iCurRes]].iOSDYOffset);
        }
        break;
      }
    }
  }
  // set the label control correctly
  CStdString strText;
  strText.Format("%s | %s", g_settings.m_ResInfo[m_Res[m_iCurRes]].strMode, strStatus.c_str());
  SET_CONTROL_LABEL(CONTROL_LABEL_ROW1, strText);
}

void CGUIWindowSettingsScreenCalibration::Render()
{
  //  g_graphicsContext.Get3DDevice()->Clear(0, NULL, D3DCLEAR_TARGET, 0, 0, 0);
  m_iControl = GetFocusedControl();
  if (m_iControl >= 0)
  {
    UpdateFromControl(m_iControl);
  }
  else
  {
    SET_CONTROL_LABEL(CONTROL_LABEL_ROW1, "");
    SET_CONTROL_LABEL(CONTROL_LABEL_ROW2, "");
  }
  // show the OSD if necessary (need to do this every frame, as otherwise
  // it will time out.
  if (m_iControl == CONTROL_OSD)
  {
    CGUIDialog *pOSD = (CGUIDialog *)m_gWindowManager.GetWindow(WINDOW_OSD);
    if (pOSD) pOSD->Show(GetID());
  }
  CGUIWindow::Render();

  // render the subtitles
  if (g_application.m_pPlayer)
  {
    g_application.m_pPlayer->UpdateSubtitlePosition();
    g_application.m_pPlayer->RenderSubtitles();
  }

}
