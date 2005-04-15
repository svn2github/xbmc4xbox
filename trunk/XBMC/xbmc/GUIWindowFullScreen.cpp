#include "stdafx.h"
#include "GUIWindowFullScreen.h"
#include "Application.h"
#include "Util.h"
#include "cores/mplayer/mplayer.h"
#include "cores/mplayer.h"
#include "VideoDatabase.h"
#include "cores/mplayer/ASyncDirectSound.h"
#include "PlayListPlayer.h"
#include "utils/GUIInfoManager.h"
#include "cores/mplayer/xbox_video.h"
#include "../guilib/GUIProgressControl.h"
#include "GUIAudioManager.h"

#include <stdio.h>


#define BLUE_BAR    0
#define LABEL_ROW1 10
#define LABEL_ROW2 11
#define LABEL_ROW3 12

#define BTN_OSD_VIDEO 13
#define BTN_OSD_AUDIO 14
#define BTN_OSD_SUBTITLE 15

#define MENU_ACTION_AVDELAY       1
#define MENU_ACTION_SEEK          2
#define MENU_ACTION_SUBTITLEDELAY 3
#define MENU_ACTION_SUBTITLEONOFF 4
#define MENU_ACTION_SUBTITLELANGUAGE 5
#define MENU_ACTION_INTERLEAVED 6
#define MENU_ACTION_FRAMERATECONVERSIONS 7
#define MENU_ACTION_AUDIO_STREAM 8

#define MENU_ACTION_NEW_BOOKMARK  9
#define MENU_ACTION_NEXT_BOOKMARK  10
#define MENU_ACTION_CLEAR_BOOKMARK  11

#define MENU_ACTION_NOCACHE 12

#define IMG_PAUSE     16
#define IMG_2X        17
#define IMG_4X        18
#define IMG_8X      19
#define IMG_16X       20
#define IMG_32X       21

#define IMG_2Xr       117
#define IMG_4Xr     118
#define IMG_8Xr     119
#define IMG_16Xr    120
#define IMG_32Xr      121

//Displays current position, visible after seek or when forced
//Alt, use conditional visibility Player.DisplayAfterSeek
#define LABEL_CURRENT_TIME 22

//Displays when video is rebuffering
//Alt, use conditional visibility Player.IsCaching
#define LABEL_BUFFERING 24

//Progressbar used for buffering status and after seeking
#define CONTROL_PROGRESS 23


extern IDirectSoundRenderer* m_pAudioDecoder;

static DWORD color[6] = { 0xFFFF00, 0xFFFFFF, 0x0099FF, 0x00FF00, 0xCCFF00, 0x00FFFF };

CGUIWindowFullScreen::CGUIWindowFullScreen(void)
    : CGUIWindow(0)
{
  m_strTimeStamp[0] = 0;
  m_iTimeCodePosition = 0;
  m_bShowTime = false;
  m_bShowCodecInfo = false;
  m_bShowViewModeInfo = false;
  m_dwShowViewModeTimeout = 0;
  m_bShowCurrentTime = false;
  m_dwTimeCodeTimeout = 0;
  m_fFPS = 0;
  m_fFrameCounter = 0.0f;
  m_dwFPSTime = timeGetTime();
  m_subtitleFont = NULL;
  // enable relative coordinates
  m_bNeedsScaling = true;

  // audio
  //  - language
  //  - volume
  //  - stream

  // video
  //  - Create Bookmark (294)
  //  - Cycle bookmarks (295)
  //  - Clear bookmarks (296)
  //  - jump to specific time
  //  - slider
  //  - av delay

  // subtitles
  //  - delay
  //  - language

}

CGUIWindowFullScreen::~CGUIWindowFullScreen(void)
{}

void CGUIWindowFullScreen::AllocResources()
{
  CGUIWindow::AllocResources();
  g_application.m_guiWindowOSD.AllocResources();
}

void CGUIWindowFullScreen::FreeResources()
{
  g_settings.Save();
  g_application.m_guiWindowOSD.FreeResources();
  CGUIWindow::FreeResources();
}

void CGUIWindowFullScreen::OnAction(const CAction &action)
{

  if (m_bOSDVisible)
  {
    if (action.wID == ACTION_SHOW_OSD && !g_application.m_guiWindowOSD.SubMenuVisible())  // hide the OSD
    {
      CSingleLock lock (m_section);
      OutputDebugString("CGUIWindowFullScreen::HIDEOSD\n");
      CGUIMessage msg(GUI_MSG_WINDOW_DEINIT, 0, 0, 0, 0, NULL);
      g_application.m_guiWindowOSD.OnMessage(msg);  // Send a de-init msg to the OSD
      m_bOSDVisible = false;

    }
    else
    {
      OutputDebugString("CGUIWindowFullScreen::OnAction() reset timeout\n");
      m_dwOSDTimeOut = timeGetTime();
      g_application.m_guiWindowOSD.OnAction(action);  // route keys to OSD window
    }
    return ;
  }

  // if the player has handled the message, just return
  if (g_application.m_pPlayer != NULL && g_application.m_pPlayer->OnAction(action)) return ;

  switch (action.wID)
  {

  case ACTION_SHOW_GUI:
    {
      // switch back to the menu
      OutputDebugString("Switching to GUI\n");
      m_gWindowManager.PreviousWindow();
      OutputDebugString("Now in GUI\n");

      return ;
    }
    break;

  case ACTION_STEP_BACK:
    Seek(false, false);
    break;

  case ACTION_STEP_FORWARD:
    Seek(true, false);
    break;

  case ACTION_BIG_STEP_BACK:
    Seek(false, true);
    break;

  case ACTION_BIG_STEP_FORWARD:
    Seek(true, true);
    break;

  case ACTION_SHOW_MPLAYER_OSD:
    g_application.m_pPlayer->ToggleOSD();
    break;

  case ACTION_SHOW_OSD_TIME:
    if( !HasProgressDisplay() ) g_application.m_pPlayer->ToggleOSD();
    m_bShowCurrentTime = !m_bShowCurrentTime;
    if(!m_bShowCurrentTime)
      g_infoManager.SetDisplayAfterSeek(0); //Force display off
    break;

  case ACTION_SHOW_OSD:  // Show the OSD
    {
      //CSingleLock lock(m_section);
      OutputDebugString("CGUIWindowFullScreen:SHOWOSD\n");
      m_dwOSDTimeOut = timeGetTime();

      CGUIMessage msg(GUI_MSG_WINDOW_INIT, 0, 0, 0, 0, NULL);
      g_application.m_guiWindowOSD.OnMessage(msg);  // Send an init msg to the OSD
      m_bOSDVisible = true;

    }
    break;

  case ACTION_SHOW_SUBTITLES:
    {
      g_application.m_pPlayer->ToggleSubtitles();
    }
    break;

  case ACTION_SHOW_CODEC:
    {
      m_bShowCodecInfo = !m_bShowCodecInfo;
    }
    break;

  case ACTION_NEXT_SUBTITLE:
    {
      g_application.m_pPlayer->SwitchToNextLanguage();
    }
    break;

  case ACTION_SUBTITLE_DELAY_MIN:
    g_application.m_pPlayer->SubtitleOffset(false);
    break;
  case ACTION_SUBTITLE_DELAY_PLUS:
    g_application.m_pPlayer->SubtitleOffset(true);
    break;
  case ACTION_AUDIO_DELAY_MIN:
    g_application.m_pPlayer->AudioOffset(false);
    break;
  case ACTION_AUDIO_DELAY_PLUS:
    g_application.m_pPlayer->AudioOffset(true);
    break;
  case ACTION_AUDIO_NEXT_LANGUAGE:
    //g_application.m_pPlayer->AudioOffset(false);
    break;
  case REMOTE_0:
    ChangetheTimeCode(REMOTE_0);
    break;
  case REMOTE_1:
    ChangetheTimeCode(REMOTE_1);
    break;
  case REMOTE_2:
    ChangetheTimeCode(REMOTE_2);
    break;
  case REMOTE_3:
    ChangetheTimeCode(REMOTE_3);
    break;
  case REMOTE_4:
    ChangetheTimeCode(REMOTE_4);
    break;
  case REMOTE_5:
    ChangetheTimeCode(REMOTE_5);
    break;
  case REMOTE_6:
    ChangetheTimeCode(REMOTE_6);
    break;
  case REMOTE_7:
    ChangetheTimeCode(REMOTE_7);
    break;
  case REMOTE_8:
    ChangetheTimeCode(REMOTE_8);
    break;
  case REMOTE_9:
    ChangetheTimeCode(REMOTE_9);
    break;

  case ACTION_ASPECT_RATIO:
    { // toggle the aspect ratio mode (only if the info is onscreen)
      if (m_bShowViewModeInfo)
      {
        g_renderManager.SetViewMode(++g_stSettings.m_currentVideoSettings.m_ViewMode);
      }
      m_bShowViewModeInfo = true;
      m_dwShowViewModeTimeout = timeGetTime();
    }
    break;
  case ACTION_SMALL_STEP_BACK:
    {
      // unpause the player so that it seeks nicely
      bool bNeedPause(false);
      if (g_application.m_pPlayer->IsPaused())
      {
        g_application.m_pPlayer->Pause();
        bNeedPause = true;
      }
      int orgpos = (int)(g_application.m_pPlayer->GetTime() / 1000);
      int triesleft = g_stSettings.m_iSmallStepBackTries;
      int jumpsize = g_stSettings.m_iSmallStepBackSeconds; // secs
      int setpos = (orgpos > jumpsize) ? orgpos - jumpsize : 0; // First jump = 2*jumpsize
      int newpos;
      do
      {
        setpos = (setpos > jumpsize) ? setpos - jumpsize : 0;
        g_application.m_pPlayer->SeekTime(setpos*1000);
        Sleep(g_stSettings.m_iSmallStepBackDelay); // delay to let mplayer finish its seek (in ms)
        newpos = (int)(g_application.m_pPlayer->GetTime() / 1000);
      }
      while ( (newpos > orgpos - jumpsize) && (setpos > 0) && (--triesleft > 0));
      // repause player if needed
      if (bNeedPause) g_application.m_pPlayer->Pause();
    }
    break;
  }
  CGUIWindow::OnAction(action);
}

void CGUIWindowFullScreen::OnWindowLoaded()
{
  CGUIWindow::OnWindowLoaded();
  //  Do not free resources of invisible controls
  //  or hdd will spin up when fast forwarding etc.
  DynamicResourceAlloc(false);
#ifndef SKIN_VERSION_1_3
  CGUIImage *pImage = (CGUIImage *)GetControl(IMG_PAUSE);
  if (pImage && !pImage->GetVisibleCondition()) pImage->SetVisibleCondition(5);
  // make the ff and rw images conditionally visible
  pImage = (CGUIImage *)GetControl(IMG_2X);
  if (pImage && !pImage->GetVisibleCondition()) pImage->SetVisibleCondition(13);
  pImage = (CGUIImage *)GetControl(IMG_2Xr);
  if (pImage && !pImage->GetVisibleCondition()) pImage->SetVisibleCondition(7);
  pImage = (CGUIImage *)GetControl(IMG_4X);
  if (pImage && !pImage->GetVisibleCondition()) pImage->SetVisibleCondition(14);
  pImage = (CGUIImage *)GetControl(IMG_4Xr);
  if (pImage && !pImage->GetVisibleCondition()) pImage->SetVisibleCondition(8);
  pImage = (CGUIImage *)GetControl(IMG_8X);
  if (pImage && !pImage->GetVisibleCondition()) pImage->SetVisibleCondition(15);
  pImage = (CGUIImage *)GetControl(IMG_8Xr);
  if (pImage && !pImage->GetVisibleCondition()) pImage->SetVisibleCondition(9);
  pImage = (CGUIImage *)GetControl(IMG_16X);
  if (pImage && !pImage->GetVisibleCondition()) pImage->SetVisibleCondition(16);
  pImage = (CGUIImage *)GetControl(IMG_16Xr);
  if (pImage && !pImage->GetVisibleCondition()) pImage->SetVisibleCondition(10);
  pImage = (CGUIImage *)GetControl(IMG_32X);
  if (pImage && !pImage->GetVisibleCondition()) pImage->SetVisibleCondition(17);
  pImage = (CGUIImage *)GetControl(IMG_32Xr);
  if (pImage && !pImage->GetVisibleCondition()) pImage->SetVisibleCondition(11);
#endif
}

bool CGUIWindowFullScreen::OnMessage(CGUIMessage& message)
{

  if (m_bOSDVisible)
  {
    //if (timeGetTime()-m_dwOSDTimeOut > 5000)
    if (g_guiSettings.GetInt("MyVideos.OSDTimeout"))
    {
      if ( (timeGetTime() - m_dwOSDTimeOut) > (DWORD)(g_guiSettings.GetInt("MyVideos.OSDTimeout") * 1000))
      {
        CSingleLock lock (m_section);
        CGUIMessage msg(GUI_MSG_WINDOW_DEINIT, 0, 0, 0, 0, NULL);
        g_application.m_guiWindowOSD.OnMessage(msg);  // Send a de-init msg to the OSD
        m_bOSDVisible = false;
        return g_application.m_guiWindowOSD.OnMessage(message); // route messages to OSD window
      }
    }

    switch (message.GetMessage())
    {
    case GUI_MSG_SETFOCUS:
    case GUI_MSG_LOSTFOCUS:
    case GUI_MSG_CLICKED:
    case GUI_MSG_WINDOW_INIT:
    case GUI_MSG_WINDOW_DEINIT:
      OutputDebugString("CGUIWindowFullScreen::OnMessage() reset timeout\n");
      m_dwOSDTimeOut = timeGetTime();
      break;
    }
    return g_application.m_guiWindowOSD.OnMessage(message); // route messages to OSD window
  }

  switch (message.GetMessage())
  {
  case GUI_MSG_WINDOW_INIT:
    {
      // check whether we've come back here from a window during which time we've actually
      // stopped playing videos
      if (message.GetParam1() == WINDOW_INVALID && !g_application.IsPlayingVideo())
      { // why are we here if nothing is playing???
        m_gWindowManager.PreviousWindow();
        return true;
      }
      m_bLastRender = false;
      m_bOSDVisible = false;
      m_bShowCurrentTime = false;

      //  Disable nav sounds if spindown is active as they are loaded
      //  from HDD all the time.
      const CIMDBMovie& movie=g_infoManager.GetCurrentMovie();
      CFileItem movieItem(movie.m_strPath, false);
      if ((movieItem.IsRemote() || movieItem.IsDVD() || movieItem.IsISO9660()) 
        && g_guiSettings.GetInt("System.RemotePlayHDSpinDown")!=SPIN_DOWN_NONE)
        g_audioManager.Enable(false);

      CGUIWindow::OnMessage(message);

      CUtil::SetBrightnessContrastGammaPercent(g_stSettings.m_currentVideoSettings.m_Brightness, g_stSettings.m_currentVideoSettings.m_Contrast, g_stSettings.m_currentVideoSettings.m_Gamma, false);

      g_graphicsContext.Lock();
      g_graphicsContext.SetFullScreenVideo( true );
      g_graphicsContext.Unlock();

      if (g_application.m_pPlayer)
        g_application.m_pPlayer->Update();

      HideOSD();

      m_iCurrentBookmark = 0;
      m_bShowCodecInfo = false;
      m_bShowViewModeInfo = false;

      // set the correct view mode
      g_renderManager.SetViewMode(g_stSettings.m_currentVideoSettings.m_ViewMode);

      if (CUtil::IsUsingTTFSubtitles())
      {
        CSingleLock lock (m_fontLock);

        if (m_subtitleFont)
        {
          delete m_subtitleFont;
          m_subtitleFont = NULL;
        }

        m_subtitleFont = new CGUIFontTTF("__subtitle__");
        CStdString fontPath = "Q:\\Media\\Fonts\\";
        fontPath += g_guiSettings.GetString("Subtitles.Font");
        if (!m_subtitleFont->Load(fontPath, g_guiSettings.GetInt("Subtitles.Height"), g_guiSettings.GetInt("Subtitles.Style")))
        {
          delete m_subtitleFont;
          m_subtitleFont = NULL;
        }
      }
      else
        m_subtitleFont = NULL;

      return true;
    }
  case GUI_MSG_WINDOW_DEINIT:
    {
      // Pause player before lock or the app will deadlock
      if (g_application.m_pPlayer)
        g_application.m_pPlayer->Update(true);
      // Pause so that we make sure that our fullscreen renderer has finished...
      Sleep(100);

      CSingleLock lock (m_section);

      if (m_bOSDVisible)
      {
        CGUIMessage msg(GUI_MSG_WINDOW_DEINIT, 0, 0, 0, 0, NULL);
        g_application.m_guiWindowOSD.OnMessage(msg);  // Send a de-init msg to the OSD
      }

      m_bOSDVisible = false;

      CGUIWindow::OnMessage(message);

      CUtil::RestoreBrightnessContrastGamma();

      g_graphicsContext.Lock();
      g_graphicsContext.SetFullScreenVideo(false);
      g_graphicsContext.SetGUIResolution(g_guiSettings.m_LookAndFeelResolution);
      g_graphicsContext.Unlock();

      m_iCurrentBookmark = 0;

      HideOSD();

      CSingleLock lockFont(m_fontLock);
      if (m_subtitleFont)
      {
        delete m_subtitleFont;
        m_subtitleFont = NULL;
      }

      if (g_application.m_pPlayer)
        g_application.m_pPlayer->Update();

      g_audioManager.Enable(true);
      return true;
    }
  case GUI_MSG_SETFOCUS:
  case GUI_MSG_LOSTFOCUS:
    if (m_bOSDVisible) return true;
    if (message.GetSenderId() != WINDOW_FULLSCREEN_VIDEO) return true;
    break;
  }

  return CGUIWindow::OnMessage(message);
}

void CGUIWindowFullScreen::OnMouse()
{
  if (g_Mouse.bClick[MOUSE_RIGHT_BUTTON])
  { // no control found to absorb this click - go back to GUI
    CAction action;
    action.wID = ACTION_SHOW_GUI;
    OnAction(action);
    return ;
  }
  if (g_Mouse.bClick[MOUSE_LEFT_BUTTON])
  { // no control found to absorb this click - toggle the OSD
    CAction action;
    action.wID = ACTION_SHOW_OSD;
    OnAction(action);
  }
}

// Dummy override of Render() - RenderFullScreen() is where the action takes place
// this is called via mplayer when the video window is flipped (indicating a frame
// change) so that we get smooth video playback
void CGUIWindowFullScreen::Render()
{
  return ;
}

bool CGUIWindowFullScreen::HasProgressDisplay()
{
  return GetControl(CONTROL_PROGRESS) != NULL;
}

bool CGUIWindowFullScreen::NeedRenderFullScreen()
{
  CSingleLock lock (m_section);
  if (g_application.m_pPlayer)
  {
    if (g_application.m_pPlayer->IsPaused() ) return true;
    if (g_application.m_pPlayer->IsCaching() ) return true;
  }

  if (g_application.GetPlaySpeed() != 1) return true;
  if (m_bShowTime) return true;
  if (m_bShowCodecInfo) return true;
  if (m_bShowViewModeInfo) return true;
  if (m_bShowCurrentTime) return true;
  if (g_infoManager.GetDisplayAfterSeek()) return true;
  if (m_gWindowManager.IsRouted()) return true;
  if (m_gWindowManager.IsModelessAvailable()) return true;
  if (m_bOSDVisible) return true;
  if (g_Mouse.IsActive()) return true;
  if (CUtil::IsUsingTTFSubtitles() && g_application.m_pPlayer->GetSubtitleVisible() && m_subtitleFont)
    return true;
  if (m_bLastRender)
  {
    m_bLastRender = false;
  }

  return false;
}

void CGUIWindowFullScreen::RenderFullScreen()
{
  if (g_application.GetPlaySpeed() != 1)
    g_infoManager.SetDisplayAfterSeek();
  if (m_bShowCurrentTime)
    g_infoManager.SetDisplayAfterSeek();

  m_bLastRender = true;
  m_fFrameCounter += 1.0f;
  FLOAT fTimeSpan = (float)(timeGetTime() - m_dwFPSTime);
  if (fTimeSpan >= 1000.0f)
  {
    fTimeSpan /= 1000.0f;
    m_fFPS = (m_fFrameCounter / fTimeSpan);
    m_dwFPSTime = timeGetTime();
    m_fFrameCounter = 0;
  }
  if (!g_application.m_pPlayer) return ;

  bool bRenderGUI = g_application.m_pPlayer->IsPaused();

  //Display current progress
  if( g_infoManager.GetDisplayAfterSeek() )
  {
    CGUIProgressControl* pControl = (CGUIProgressControl*)GetControl(CONTROL_PROGRESS);
    if (pControl) 
    {
      pControl->SetPercentage(g_application.m_pPlayer->GetPercentage());
      pControl->SetVisible(true);
    }
    bRenderGUI = true;
  }

  if( g_application.m_pPlayer->IsCaching() )
  {
    CGUIProgressControl* pControl = (CGUIProgressControl*)GetControl(CONTROL_PROGRESS);
    if (pControl) 
    {
      pControl->SetPercentage(g_application.m_pPlayer->GetCacheLevel());
      pControl->SetVisible(true);
    }
    g_infoManager.SetDisplayAfterSeek(0); //Make sure these stuff aren't visible now
    bRenderGUI = true;
    SET_CONTROL_VISIBLE(LABEL_BUFFERING);
  }
  else
    SET_CONTROL_HIDDEN(LABEL_BUFFERING);

  if( !g_infoManager.GetDisplayAfterSeek() && !g_application.m_pPlayer->IsCaching() )
  {
    SET_CONTROL_HIDDEN(CONTROL_PROGRESS);    
  }

  //------------------------
  if (m_bShowCodecInfo)
  {
    bRenderGUI = true;
    // show audio codec info
    CStdString strAudio, strVideo, strGeneral;
    g_application.m_pPlayer->GetAudioInfo(strAudio);
    {
      CGUIMessage msg(GUI_MSG_LABEL_SET, GetID(), LABEL_ROW1);
      msg.SetLabel(strAudio);
      OnMessage(msg);
    }
    // show video codec info
    g_application.m_pPlayer->GetVideoInfo(strVideo);
    {
      CGUIMessage msg(GUI_MSG_LABEL_SET, GetID(), LABEL_ROW2);
      msg.SetLabel(strVideo);
      OnMessage(msg);
    }
    // show general info
    g_application.m_pPlayer->GetGeneralInfo(strGeneral);
    {
      CStdString strGeneralFPS;
      strGeneralFPS.Format("fps:%02.2f %s", m_fFPS, strGeneral.c_str() );
      CGUIMessage msg(GUI_MSG_LABEL_SET, GetID(), LABEL_ROW3);
      msg.SetLabel(strGeneralFPS);
      OnMessage(msg);
    }
  }
  //----------------------
  // ViewMode Information
  //----------------------
  if (m_bShowViewModeInfo && timeGetTime() - m_dwShowViewModeTimeout > 2500)
  {
    m_bShowViewModeInfo = false;
  }
  if (m_bShowViewModeInfo)
  {
    bRenderGUI = true;
    {
      // get the "View Mode" string
      CStdString strTitle = g_localizeStrings.Get(629);
      CStdString strMode = g_localizeStrings.Get(630 + g_stSettings.m_currentVideoSettings.m_ViewMode);
      CStdString strInfo;
      strInfo.Format("%s : %s", strTitle.c_str(), strMode.c_str());
      CGUIMessage msg(GUI_MSG_LABEL_SET, GetID(), LABEL_ROW1);
      msg.SetLabel(strInfo);
      OnMessage(msg);
    }
    // show sizing information
    RECT SrcRect, DestRect;
    float fAR;
    g_application.m_pPlayer->GetVideoRect(SrcRect, DestRect);
    g_application.m_pPlayer->GetVideoAspectRatio(fAR);
    {
      CStdString strSizing;
      strSizing.Format("Sizing: (%i,%i)->(%i,%i) (Zoom x%2.2f) AR:%2.2f:1 (Pixels: %2.2f:1)",
                       SrcRect.right - SrcRect.left, SrcRect.bottom - SrcRect.top,
                       DestRect.right - DestRect.left, DestRect.bottom - DestRect.top, g_stSettings.m_fZoomAmount, fAR*g_stSettings.m_fPixelRatio, g_stSettings.m_fPixelRatio);
      CGUIMessage msg(GUI_MSG_LABEL_SET, GetID(), LABEL_ROW2);
      msg.SetLabel(strSizing);
      OnMessage(msg);
    }
    // show resolution information
    int iResolution = g_graphicsContext.GetVideoResolution();
    {
      CStdString strStatus;
      strStatus.Format("%ix%i %s", g_settings.m_ResInfo[iResolution].iWidth, g_settings.m_ResInfo[iResolution].iHeight, g_settings.m_ResInfo[iResolution].strMode);
      if (g_guiSettings.GetBool("Filters.Soften"))
        strStatus += "  |  Soften";
      else
        strStatus += "  |  No Soften";

      CStdString strFilter;
      strFilter.Format("  |  Flicker Filter: %i", g_guiSettings.GetInt("Filters.Flicker"));
      strStatus += strFilter;
      CGUIMessage msg(GUI_MSG_LABEL_SET, GetID(), LABEL_ROW3);
      msg.SetLabel(strStatus);
      OnMessage(msg);
    }
  }

  if (m_gWindowManager.IsRouted() || m_gWindowManager.IsModelessAvailable())
    m_gWindowManager.RenderDialogs();

  if (m_bOSDVisible)
  {
    // tell the OSD window to draw itself
    CSingleLock lock (m_section);
    g_application.m_guiWindowOSD.Render();
    // Render the mouse pointer, if visible...
    if (g_Mouse.IsActive()) g_application.m_guiPointer.Render();
    return ;
  }

  RenderTTFSubtitles();

  if (m_bShowTime && m_iTimeCodePosition != 0)
  {
    if ( (timeGetTime() - m_dwTimeCodeTimeout) >= 2500)
    {
      m_bShowTime = false;
      m_iTimeCodePosition = 0;
      return ;
    }
    bRenderGUI = true;
    CStdString strDispTime = "??:??";

    CGUIMessage msg(GUI_MSG_LABEL_SET, GetID(), LABEL_ROW1);
    for (int count = 0; count < m_iTimeCodePosition; count++)
    {
      if (m_strTimeStamp[count] == -1)
        strDispTime[count] = ':';
      else
        strDispTime[count] = (char)m_strTimeStamp[count] + 48;
    }
    strDispTime += "/" + g_infoManager.GetVideoLabel(257) + " [" + g_infoManager.GetVideoLabel(254) + "]"; // duration [ time ]
    msg.SetLabel(strDispTime);
    OnMessage(msg);
  }

  int iSpeed = g_application.GetPlaySpeed();
  if (iSpeed != 1)
    bRenderGUI = true;

  // Render current time if requested
  if (g_infoManager.GetDisplayAfterSeek())
  {
    CStdString strTime;
    bRenderGUI = true;
    CStdString strLabel;
    strLabel = g_infoManager.GetLabel(254);   // time
    strLabel += " / ";
    strLabel += g_infoManager.GetLabel(257);  // duration
    SET_CONTROL_LABEL(LABEL_CURRENT_TIME, strLabel);
    SET_CONTROL_VISIBLE(LABEL_CURRENT_TIME);
  }
  else
  {
    SET_CONTROL_HIDDEN(LABEL_CURRENT_TIME);
  }

  if ( bRenderGUI)
  {
    if (g_application.m_pPlayer->IsPaused() || iSpeed != 1)
    {
      SET_CONTROL_HIDDEN(LABEL_ROW1);
      SET_CONTROL_HIDDEN(LABEL_ROW2);
      SET_CONTROL_HIDDEN(LABEL_ROW3);
      SET_CONTROL_HIDDEN(BLUE_BAR);
    }
    else if (m_bShowCodecInfo || m_bShowViewModeInfo)
    {
      SET_CONTROL_VISIBLE(LABEL_ROW1);
      SET_CONTROL_VISIBLE(LABEL_ROW2);
      SET_CONTROL_VISIBLE(LABEL_ROW3);
      SET_CONTROL_VISIBLE(BLUE_BAR);
    }
    else if (m_bShowTime)
    {
      SET_CONTROL_VISIBLE(LABEL_ROW1);
      SET_CONTROL_HIDDEN(LABEL_ROW2);
      SET_CONTROL_HIDDEN(LABEL_ROW3);
      SET_CONTROL_VISIBLE(BLUE_BAR);
    }
    else
    {
      SET_CONTROL_HIDDEN(LABEL_ROW1);
      SET_CONTROL_HIDDEN(LABEL_ROW2);
      SET_CONTROL_HIDDEN(LABEL_ROW3);
      SET_CONTROL_HIDDEN(BLUE_BAR);
    }
    CGUIWindow::Render();
  }
  // and lastly render the mouse pointer...
  if (g_Mouse.IsActive()) g_application.m_guiPointer.Render();
}

void CGUIWindowFullScreen::RenderTTFSubtitles()
{
  if (CUtil::IsUsingTTFSubtitles() && g_application.m_pPlayer->GetSubtitleVisible() && m_subtitleFont)
  {
    CSingleLock lock (m_fontLock);

    subtitle* sub = mplayer_GetCurrentSubtitle();

    if (sub != NULL)
    {
      CStdStringW subtitleText = L"";

      for (int i = 0; i < sub->lines; i++)
      {
        if (i != 0)
        {
          subtitleText += L"\n";
        }

        CStdStringA S = sub->text[i];
        CStdStringW W;
        g_charsetConverter.subtitleCharsetToFontCharset(S, W);
        subtitleText += W;
      }

      int m_iResolution = g_graphicsContext.GetVideoResolution();

      float w;
      float h;
      m_subtitleFont->GetTextExtent(subtitleText.c_str(), &w, &h);

      float x = (float) (g_settings.m_ResInfo[m_iResolution].iWidth) / 2;
      float y = (float) g_settings.m_ResInfo[m_iResolution].iSubtitles - h;

      float outlinewidth = (float)g_guiSettings.GetInt("Subtitles.Height") / 8;

      m_subtitleFont->DrawText(x - outlinewidth, y, 0, subtitleText.c_str(), XBFONT_CENTER_X);
      m_subtitleFont->DrawText(x + outlinewidth, y, 0, subtitleText.c_str(), XBFONT_CENTER_X);
      m_subtitleFont->DrawText(x, y + outlinewidth, 0, subtitleText.c_str(), XBFONT_CENTER_X);
      m_subtitleFont->DrawText(x, y - outlinewidth, 0, subtitleText.c_str(), XBFONT_CENTER_X);
      m_subtitleFont->DrawText(x, y, color[g_guiSettings.GetInt("Subtitles.Color")], subtitleText.c_str(), XBFONT_CENTER_X);
    }
  }
}

void CGUIWindowFullScreen::HideOSD()
{
  CSingleLock lock (m_section);
  m_osdMenu.Clear();
  SET_CONTROL_HIDDEN(BTN_OSD_VIDEO);
  SET_CONTROL_HIDDEN(BTN_OSD_AUDIO);
  SET_CONTROL_HIDDEN(BTN_OSD_SUBTITLE);

  SET_CONTROL_VISIBLE(LABEL_ROW1);
  SET_CONTROL_VISIBLE(LABEL_ROW2);
  SET_CONTROL_VISIBLE(LABEL_ROW3);
  SET_CONTROL_VISIBLE(BLUE_BAR);
}

void CGUIWindowFullScreen::ShowOSD()
{
}

bool CGUIWindowFullScreen::OSDVisible() const
{
  return m_bOSDVisible;
}

void CGUIWindowFullScreen::OnExecute(int iAction, const IOSDOption* option)
{
}
void CGUIWindowFullScreen::ChangetheTimeCode(DWORD remote)
{
  if (remote >= 58 && remote <= 67) //Make sure it's only for the remote
  {
    m_bShowTime = true;
    m_dwTimeCodeTimeout = timeGetTime();
    int itime = remote - 58;
    if (m_iTimeCodePosition <= 4 && m_iTimeCodePosition != 2)
    {
      m_strTimeStamp[m_iTimeCodePosition++] = itime;
      if (m_iTimeCodePosition == 2)
        m_strTimeStamp[m_iTimeCodePosition++] = -1;
    }
    if (m_iTimeCodePosition > 4)
    {
      long itotal, ih, im, is = 0;
      ih = (m_strTimeStamp[0] - 0) * 10;
      ih += (m_strTimeStamp[1] - 0);
      im = (m_strTimeStamp[3] - 0) * 10;
      im += (m_strTimeStamp[4] - 0);
      im *= 60;
      ih *= 3600;
      itotal = ih + im + is;
      bool bNeedsPause(false);
      if (g_application.m_pPlayer->IsPaused())
      {
        bNeedsPause = true;
        g_application.m_pPlayer->Pause();
      }
      if (itotal < g_application.m_pPlayer->GetTotalTime())
        g_application.m_pPlayer->SeekTime(itotal*1000);
      if (bNeedsPause)
      {
        Sleep(g_stSettings.m_iSmallStepBackDelay);  // allow mplayer to finish it's seek (nasty hack)
        g_application.m_pPlayer->Pause();
      }
      m_iTimeCodePosition = 0;
      m_bShowTime = false;
    }
  }
}
void CGUIWindowFullScreen::ChangetheSpeed(DWORD action)
{
  int iSpeed = g_application.GetPlaySpeed();
  if (action == ACTION_REWIND && iSpeed == 1) // Enables Rewinding
    iSpeed *= -2;
  else if (action == ACTION_REWIND && iSpeed > 1) //goes down a notch if you're FFing
    iSpeed /= 2;
  else if (action == ACTION_FORWARD && iSpeed < 1) //goes up a notch if you're RWing
    iSpeed /= 2;
  else
    iSpeed *= 2;

  if (action == ACTION_FORWARD && iSpeed == -1) //sets iSpeed back to 1 if -1 (didn't plan for a -1)
    iSpeed = 1;
  if (iSpeed > 32 || iSpeed < -32)
    iSpeed = 1;
  g_application.SetPlaySpeed(iSpeed);
}

void CGUIWindowFullScreen::Update()
{
}

void CGUIWindowFullScreen::Seek(bool bPlus, bool bLargeStep)
{
  // Unpause mplayer if necessary
  bool bNeedsPause(false);
  if (g_application.m_pPlayer->IsPaused())
  {
    g_application.m_pPlayer->Pause();
    bNeedsPause = true;
  }
  g_application.m_pPlayer->Seek(bPlus, bLargeStep);
  
  //Make sure gui items are visible
  g_infoManager.SetDisplayAfterSeek();

  // And repause it
  if (bNeedsPause)
  {
    Sleep(g_stSettings.m_iSmallStepBackDelay);  // allow mplayer to finish it's seek (nasty hack)
    g_application.m_pPlayer->Pause();
  }

}
