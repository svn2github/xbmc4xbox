#include "guistandardwindow.h"

CGUIStandardWindow::CGUIStandardWindow(void) : CGUIWindow(0)
{
  m_iLastControl=-1;
}

CGUIStandardWindow::~CGUIStandardWindow(void)
{
}

void CGUIStandardWindow::OnAction(const CAction &action)
{
	if (action.wID == ACTION_PREVIOUS_MENU)
	{
		m_iLastControl=-1;
		m_gWindowManager.PreviousWindow();
		return;
	}

	CGUIWindow::OnAction(action);
}

bool CGUIStandardWindow::OnMessage(CGUIMessage& message)
{
	switch ( message.GetMessage() )
	{
		case GUI_MSG_WINDOW_DEINIT:
		{

		}
		break;

		case GUI_MSG_WINDOW_INIT:
		{
			int iFocusControl=m_iLastControl;

			CGUIWindow::OnMessage(message);

			if (iFocusControl>-1)
			{
				SET_CONTROL_FOCUS(GetID(), iFocusControl, 0);
			}

			return true;
		}
		break;

		case GUI_MSG_SETFOCUS:
		{
			m_iLastControl=message.GetControlId();
		}
		break;

		case GUI_MSG_CLICKED:
		{
			// Default control definitions could be placed here in the future
		}
		break;
	}

	return CGUIWindow::OnMessage(message);
}
