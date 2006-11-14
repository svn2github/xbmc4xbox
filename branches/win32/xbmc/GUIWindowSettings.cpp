#include "stdafx.h"
#include "GUIWindowSettings.h"
#ifdef HAS_CREDITS
#include "Credits.h"
#endif

#define CONTROL_CREDITS 12

CGUIWindowSettings::CGUIWindowSettings(void)
    : CGUIWindow(WINDOW_SETTINGS_MENU, "Settings.xml")
{
}

CGUIWindowSettings::~CGUIWindowSettings(void)
{
}

bool CGUIWindowSettings::OnAction(const CAction &action)
{
  if (action.wID == ACTION_PREVIOUS_MENU)
  {
    m_gWindowManager.PreviousWindow();
    return true;
  }

  return CGUIWindow::OnAction(action);
}

bool CGUIWindowSettings::OnMessage(CGUIMessage& message)
{
  switch ( message.GetMessage() )
  {
  case GUI_MSG_CLICKED:
    {
      int iControl = message.GetSenderId();
      if (iControl == CONTROL_CREDITS)
      {
#ifdef HAS_CREDITS
        RunCredits();
#endif
        return true;
      }
    }
    break;
  }

  return CGUIWindow::OnMessage(message);
}
