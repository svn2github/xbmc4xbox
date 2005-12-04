#include "stdafx.h"
#include "GUIWindowHome.h"
#include "Util.h"
#include "Credits.h"
#include "GUIButtonScroller.h"
#include "GUIConditionalButtonControl.h"
#include "utils/GUIInfoManager.h"
#include "GUIDialogContextMenu.h"
#include "utils/KaiClient.h"

#include "application.h"

#define MENU_BUTTON_START 2    // normal buttons
#define MENU_BUTTON_END   20

#define MENU_BUTTON_IMAGE_BACKGROUND_START   101  // background images
#define MENU_BUTTON_IMAGE_BACKGROUND_END     120

#define MENU_BUTTON_IMAGE_NOFOCUS_START      121  // button images for button scroller
#define MENU_BUTTON_IMAGE_NOFOCUS_END        140
#define MENU_BUTTON_IMAGE_FOCUS_START        141  // focused button images for button scroller
#define MENU_BUTTON_IMAGE_FOCUS_END          160

#define CONTROL_BTN_XLINK_KAI  99
#define CONTROL_BTN_SCROLLER  300
#define CONTROL_DVDDRIVE_START  400


#define HOME_HANDLES_FADES 0
#define HOME_FADE_TIME 300

CGUIWindowHome::CGUIWindowHome(void) : CGUIWindow(WINDOW_HOME, "Home.xml")
{
  m_iLastControl = -1;
  m_iLastMenuOption = -1;
  m_iSelectedItem = -1;
}

CGUIWindowHome::~CGUIWindowHome(void)
{}


bool CGUIWindowHome::OnMessage(CGUIMessage& message)
{
  switch ( message.GetMessage() )
  {
  case GUI_MSG_WINDOW_INIT:
    {
      int iFocusControl = m_iLastControl;

      CGUIWindow::OnMessage(message);

#ifdef HOME_HANDLES_FADES
      // make controls 102-120 invisible...
      for (int iControl = MENU_BUTTON_IMAGE_BACKGROUND_START; iControl < MENU_BUTTON_IMAGE_BACKGROUND_END; iControl++)
      {
        SET_CONTROL_HIDDEN(iControl);
      }

      if (m_iLastMenuOption > 0)
      {
        SET_CONTROL_VISIBLE(m_iLastMenuOption + 100);
      }
#endif

      if (iFocusControl < 0)
      {
        iFocusControl = GetFocusedControl();
        m_iLastControl = iFocusControl;
      }

      UpdateButtonScroller();

      if (m_iSelectedItem >= 0)
      {
        CGUIButtonScroller *pScroller = (CGUIButtonScroller *)GetControl(CONTROL_BTN_SCROLLER);
        if (pScroller) pScroller->SetActiveButton(m_iSelectedItem);
      }

      ON_POLL_BUTTON_CONDITION(CONTROL_BTN_XLINK_KAI, CGUIWindowHome, OnPollXLinkClient, 50);

      SET_CONTROL_FOCUS(iFocusControl, 0);
#ifndef HOME_HANDLES_FADES
      SetControlVisibility();
#endif
      return true;
    }
    break;

  case GUI_MSG_WINDOW_DEINIT:
    {
      CGUIButtonScroller *pScroller = (CGUIButtonScroller *)GetControl(CONTROL_BTN_SCROLLER);
      if (pScroller)
        m_iSelectedItem = pScroller->GetActiveButton();
    }
    break;

  case GUI_MSG_SETFOCUS:
    {
      int iControl = message.GetControlId();
      m_iLastControl = iControl;
#ifdef HOME_HANDLES_FADES
      FadeBackgroundImages(message.GetControlId(), message.GetSenderId(), message.GetParam1());
#endif
      break;
    }

  case GUI_MSG_CLICKED:
    {
      int iControl = message.GetSenderId();
      if (iControl == CONTROL_BTN_SCROLLER)
      {
        int iAction = message.GetParam1();
        if (iAction == ACTION_CONTEXT_MENU || iAction == ACTION_MOUSE_RIGHT_CLICK)
        {
          OnPopupContextMenu();
        }
      }
      else if (iControl == CONTROL_DVDDRIVE_START)
      {
        //GeminiServer: to play a inserted media dvd
        CAutorun::PlayDisc();
      }
      m_iLastControl = message.GetSenderId();
      break;
    }
  }
  return CGUIWindow::OnMessage(message);
}

void CGUIWindowHome::OnClickOnlineGaming(CGUIMessage& aMessage)
{
  m_gWindowManager.ActivateWindow( WINDOW_BUDDIES );
}

bool CGUIWindowHome::OnAction(const CAction &action)
{
  return CGUIWindow::OnAction(action);
}

void CGUIWindowHome::Render()
{
  // set controls 121->160 invisible (these are the focus + nofocus buttons for the button scroller)
  for (int i = MENU_BUTTON_IMAGE_NOFOCUS_START; i < MENU_BUTTON_IMAGE_FOCUS_END; i++)
  {
    CGUIControl *pControl = (CGUIControl *)GetControl(i);
    if (pControl)
    {
      pControl->SetVisible(false);  // make invisible
      pControl->DynamicResourceAlloc(false);  // turn of dynamic allocation, as we render these in the
                                              // button scroller class
    }
  }
#ifdef HOME_HANDLES_FADES
  // Hack for when music/video stops while on homepage (images will all display
  // in this case, whereas normally only 2 are allowed at the same time (during
  // fading)  We check for more than 5 images as a safety check from very fast
  // scrolling.  Note that this may well destroy skins designed purely using
  // the new visibility flags.  This needs removing once agreement is reached
  // that doing the fading both here and in the skin is not good.
  int numBackgroundsVisible = 0;
  for (int i = MENU_BUTTON_IMAGE_BACKGROUND_START; i <= MENU_BUTTON_IMAGE_BACKGROUND_END; ++i)
  {
    const CGUIControl *pControl = GetControl(i);
    if (pControl && pControl->IsVisible())
      numBackgroundsVisible++;
  }
  if (numBackgroundsVisible > 5)
    FadeBackgroundImages(GetFocusedControl(), GetFocusedControl(), 0);
#endif
  CGUIWindow::Render();
}

void CGUIWindowHome::UpdateButtonScroller()
{
  CGUIButtonScroller *pScroller = (CGUIButtonScroller *)GetControl(CONTROL_BTN_SCROLLER);
  if (pScroller)
  {
    int iButton = pScroller->GetActiveButton();
    pScroller->ClearButtons();
    for (int i = 0; i < (int)g_settings.m_buttonSettings.m_vecButtons.size(); i++)
    {
      if (g_settings.m_buttonSettings.m_vecButtons[i]->m_dwLabel != -1)
      { // grab the required label from our strings
        pScroller->AddButton(g_localizeStrings.Get(g_settings.m_buttonSettings.m_vecButtons[i]->m_dwLabel),
                             g_settings.m_buttonSettings.m_vecButtons[i]->m_strExecute,
                             g_settings.m_buttonSettings.m_vecButtons[i]->m_iIcon);
      }
      else
      {
        pScroller->AddButton(g_settings.m_buttonSettings.m_vecButtons[i]->m_strLabel,
                             g_settings.m_buttonSettings.m_vecButtons[i]->m_strExecute,
                             g_settings.m_buttonSettings.m_vecButtons[i]->m_iIcon);
      }
    }
    if (iButton > 0)
      pScroller->SetActiveButton(iButton);
    else
      pScroller->SetActiveButton(g_settings.m_buttonSettings.m_iDefaultButton);
  }
}

void CGUIWindowHome::OnPopupContextMenu()
{
  CGUIButtonScroller *pScroller = (CGUIButtonScroller *)GetControl(CONTROL_BTN_SCROLLER);
  if (!pScroller) return ;
  int iButton = pScroller->GetActiveButton();
  CGUIDialogContextMenu *pMenu = (CGUIDialogContextMenu *)m_gWindowManager.GetWindow(WINDOW_DIALOG_CONTEXT_MENU);
  if (!pMenu) return ;
  // load our menu
  pMenu->Initialize();
  pMenu->AddButton(13332); // Move Up
  pMenu->AddButton(13333); // Move Down
  pMenu->AddButton(13334); // Edit Label
  pMenu->AddButton(13335); // Make Default
  pMenu->AddButton(13336); // Remove Button
  // calculate our position
  int iPos[4];
  iPos[0] = pScroller->GetXPosition();
  iPos[1] = pScroller->GetYPosition();
  iPos[2] = g_graphicsContext.GetWidth() - (int)pScroller->GetWidth() - iPos[0];
  iPos[3] = g_graphicsContext.GetHeight() - (int)pScroller->GetHeight() - iPos[1];
  int iMax = 0;
  for (int i = 1; i < 4; i++)
    if (iPos[i] > iPos[iMax])
      iMax = i;
  int iPosX, iPosY;
  if (iMax == 0)
  { // left
    iPosX = iPos[0] - (int)pMenu->GetWidth();
    iPosY = iPos[1] + ((int)pScroller->GetHeight() - (int)pMenu->GetHeight()) / 2;
  }
  else if (iMax == 1)
  { // top
    iPosX = iPos[0] + ((int)pScroller->GetWidth() - (int)pMenu->GetWidth()) / 2;
    iPosY = iPos[1] - (int)pMenu->GetHeight();
  }
  else if (iMax == 2)
  { // right
    iPosX = g_graphicsContext.GetWidth() - iPos[2];
    iPosY = iPos[1] + ((int)pScroller->GetHeight() - (int)pMenu->GetHeight()) / 2;
  }
  else // if (iMax == 3)
  { // bottom
    iPosX = iPos[0] + ((int)pScroller->GetWidth() - (int)pMenu->GetWidth()) / 2;
    iPosY = g_graphicsContext.GetHeight() - iPos[3];
  }
  // check to make sure we're not going offscreen, and compensate if we are...
  if (iPosX < 0) iPosX = 0;
  if (iPosY < 0) iPosY = 0;
  if (iPosX > g_graphicsContext.GetWidth() - (int)pMenu->GetWidth()) iPosX = g_graphicsContext.GetWidth() - (int)pMenu->GetWidth();
  if (iPosY > g_graphicsContext.GetHeight() - (int)pMenu->GetHeight()) iPosY = g_graphicsContext.GetHeight() - (int)pMenu->GetHeight();
  pMenu->SetPosition(iPosX, iPosY);
  pMenu->DoModal(GetID());
  switch (pMenu->GetButton())
  {
  case 1:
    { // Move up
      CButtonScrollerSettings::CButton *pButton = g_settings.m_buttonSettings.m_vecButtons[iButton];
      int iNewButton = iButton - 1;
      if (iNewButton < 0)
      {
        iNewButton = (int)g_settings.m_buttonSettings.m_vecButtons.size() - 1;
        CButtonScrollerSettings::CButton *pButton2 = g_settings.m_buttonSettings.m_vecButtons[iNewButton];
        g_settings.m_buttonSettings.m_vecButtons.erase(g_settings.m_buttonSettings.m_vecButtons.begin());
        g_settings.m_buttonSettings.m_vecButtons.erase(g_settings.m_buttonSettings.m_vecButtons.end() - 1);
        g_settings.m_buttonSettings.m_vecButtons.insert(g_settings.m_buttonSettings.m_vecButtons.begin(), pButton2);
        g_settings.m_buttonSettings.m_vecButtons.insert(g_settings.m_buttonSettings.m_vecButtons.end(), pButton);
      }
      else
      {
        g_settings.m_buttonSettings.m_vecButtons.erase(g_settings.m_buttonSettings.m_vecButtons.begin() + iButton);
        g_settings.m_buttonSettings.m_vecButtons.insert(g_settings.m_buttonSettings.m_vecButtons.begin() + iNewButton, pButton);
      }
      // check if we need to exchange the default button...
      if (g_settings.m_buttonSettings.m_iDefaultButton == iButton)
        g_settings.m_buttonSettings.m_iDefaultButton = iNewButton;
      else if (g_settings.m_buttonSettings.m_iDefaultButton == iNewButton)
        g_settings.m_buttonSettings.m_iDefaultButton = iButton;
      g_settings.SaveHomeButtons();
      UpdateButtonScroller();
    }
    break;
  case 2:
    { // Move down
      int iNewButton = iButton + 1;
      if (iNewButton > (int)g_settings.m_buttonSettings.m_vecButtons.size() - 1)
      { // swap first and last items...
        iNewButton = 0;
        CButtonScrollerSettings::CButton *pButton2 = g_settings.m_buttonSettings.m_vecButtons[iButton];
        CButtonScrollerSettings::CButton *pButton = g_settings.m_buttonSettings.m_vecButtons[iNewButton];
        g_settings.m_buttonSettings.m_vecButtons.erase(g_settings.m_buttonSettings.m_vecButtons.begin());
        g_settings.m_buttonSettings.m_vecButtons.erase(g_settings.m_buttonSettings.m_vecButtons.end() - 1);
        g_settings.m_buttonSettings.m_vecButtons.insert(g_settings.m_buttonSettings.m_vecButtons.begin(), pButton2);
        g_settings.m_buttonSettings.m_vecButtons.insert(g_settings.m_buttonSettings.m_vecButtons.end(), pButton);
      }
      else
      { // swap this and the next item
        CButtonScrollerSettings::CButton *pButton = g_settings.m_buttonSettings.m_vecButtons[iButton];
        g_settings.m_buttonSettings.m_vecButtons.erase(g_settings.m_buttonSettings.m_vecButtons.begin() + iButton);
        g_settings.m_buttonSettings.m_vecButtons.insert(g_settings.m_buttonSettings.m_vecButtons.begin() + iNewButton, pButton);
      }
      // check if we need to exchange the default button...
      if (g_settings.m_buttonSettings.m_iDefaultButton == iButton)
        g_settings.m_buttonSettings.m_iDefaultButton = iNewButton;
      else if (g_settings.m_buttonSettings.m_iDefaultButton == iNewButton)
        g_settings.m_buttonSettings.m_iDefaultButton = iButton;
      g_settings.SaveHomeButtons();
      UpdateButtonScroller();
    }
    break;
  case 3:
    { // Edit Label
      CButtonScrollerSettings::CButton *pButton = g_settings.m_buttonSettings.m_vecButtons[iButton];
      CStdString strLabel;
      if (pButton->m_dwLabel != -1)
        strLabel = g_localizeStrings.Get(pButton->m_dwLabel);
      else
        strLabel = pButton->m_strLabel;
      CStdString strHeading = g_localizeStrings.Get(13334);
      CStdString strNewLabel = strLabel;
      if (CGUIDialogKeyboard::ShowAndGetInput(strNewLabel, strHeading, false) && strNewLabel != strLabel)
      {
        WCHAR wszLabel[1024];
        swprintf(wszLabel, L"%S", strNewLabel.c_str());
        pButton->m_strLabel = wszLabel;
        pButton->m_dwLabel = -1;
        g_settings.SaveHomeButtons();
        UpdateButtonScroller();
      }
    }
    break;
  case 4:
    { // Make default
      g_settings.m_buttonSettings.m_iDefaultButton = iButton;
      g_settings.SaveHomeButtons();
    }
    break;
  case 5:
    { // Remove Button
      // check we have at least 2 buttons present
      if (g_settings.m_buttonSettings.m_vecButtons.size() < 2)
        return ;
      CButtonScrollerSettings::CButton *pButton = g_settings.m_buttonSettings.m_vecButtons[iButton];
      if (pButton) delete pButton;
      g_settings.m_buttonSettings.m_vecButtons.erase(g_settings.m_buttonSettings.m_vecButtons.begin() + iButton);
      if (g_settings.m_buttonSettings.m_iDefaultButton == iButton)
        g_settings.m_buttonSettings.m_iDefaultButton = (iButton == 0) ? g_settings.m_buttonSettings.m_vecButtons.size() - 1 : iButton - 1;
      g_settings.SaveHomeButtons();
      UpdateButtonScroller();
    }
    break;
  }
}

bool CGUIWindowHome::OnPollXLinkClient(CGUIConditionalButtonControl* pButton)
{
  if (g_guiSettings.GetBool("XLinkKai.Enabled"))
    return CKaiClient::GetInstance()->IsEngineConnected();
  return false;
}

void CGUIWindowHome::FadeBackgroundImages(int focusedControl, int messageSender, int associatedImage)
{
  const CGUIControl *pControl = GetControl(focusedControl);
  if (focusedControl >= MENU_BUTTON_START && focusedControl <= MENU_BUTTON_END && pControl && pControl->CanFocus())
  {
    m_iLastMenuOption = m_iLastControl;
    bool fade = ((messageSender == CONTROL_BTN_SCROLLER) || (messageSender >= MENU_BUTTON_START && messageSender < MENU_BUTTON_END)) && (messageSender != focusedControl);

    // make controls 102-120 invisible...
    for (int i = MENU_BUTTON_IMAGE_BACKGROUND_START; i < MENU_BUTTON_IMAGE_BACKGROUND_END; i++)
    {
      if (fade)
      {
        SET_CONTROL_FADE_OUT(i, HOME_FADE_TIME);
      }
      else
      {
        SET_CONTROL_HIDDEN(i);
      }
    }

    if (fade)
    {
      SET_CONTROL_FADE_IN(focusedControl + 100, HOME_FADE_TIME);
    }
    else
    {
      SET_CONTROL_VISIBLE(focusedControl + 100);
    }
  }
  else if (focusedControl == CONTROL_BTN_SCROLLER)
  {
    // the button scroller has changed focus...
    if (messageSender != CONTROL_BTN_SCROLLER && messageSender != GetID())
    {
      // button scroller is focused from some other control
      // we need to fire off a message to update the info
      CGUIMessage msg(GUI_MSG_SETFOCUS, GetID(), CONTROL_BTN_SCROLLER);
      OnMessage(msg);
      return;
    }
    bool fade = (messageSender == CONTROL_BTN_SCROLLER) || (messageSender >= MENU_BUTTON_START && messageSender < MENU_BUTTON_END);
    if (associatedImage >= MENU_BUTTON_IMAGE_BACKGROUND_START && associatedImage <= MENU_BUTTON_IMAGE_BACKGROUND_END)
    {
      // make controls 101-120 invisible...
      for (int i = MENU_BUTTON_IMAGE_BACKGROUND_START; i < MENU_BUTTON_IMAGE_BACKGROUND_END; i++)
      {
        if (fade)
        {
          SET_CONTROL_FADE_OUT(i, HOME_FADE_TIME);
        }
        else
        {
          SET_CONTROL_HIDDEN(i);
        }
      }

      if (fade)
      {
        SET_CONTROL_FADE_IN(associatedImage, HOME_FADE_TIME);
      }
      else
      {
        SET_CONTROL_VISIBLE(associatedImage);
      }
    }
  }
}
