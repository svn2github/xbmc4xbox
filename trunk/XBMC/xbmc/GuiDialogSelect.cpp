
#include "GUIDialogSelect.h"
#include "localizestrings.h"
#include "application.h"

#define CONTROL_HEADING	      4
#define CONTROL_LIST          3
#define CONTROL_NUMBEROFFILES 2
#define CONTROL_BUTTON        5

CGUIDialogSelect::CGUIDialogSelect(void)
:CGUIDialog(0)
{
  m_bButtonEnabled=false;
}

CGUIDialogSelect::~CGUIDialogSelect(void)
{
}


void CGUIDialogSelect::OnAction(const CAction &action)
{
	if (action.wID == ACTION_CLOSE_DIALOG || action.wID == ACTION_PREVIOUS_MENU)
  {
		Close();
		return;
  }
	CGUIWindow::OnAction(action);
}

bool CGUIDialogSelect::OnMessage(CGUIMessage& message)
{
  switch ( message.GetMessage() )
  {
		case GUI_MSG_WINDOW_DEINIT:
    {
			Reset();
			g_application.EnableOverlay();			
		}
		break;

    case GUI_MSG_WINDOW_INIT:
    {
      m_bButtonPressed=false;
			CGUIDialog::OnMessage(message);
			g_application.DisableOverlay();
			m_iSelected=-1;
			CGUIMessage msg(GUI_MSG_LABEL_RESET,GetID(),CONTROL_LIST,0,0,NULL);
			g_graphicsContext.SendMessage(msg);         

			for (int i=0; i < (int)m_vecList.size(); i++)
			{
				CGUIListItem* pItem=m_vecList[i];
				CGUIMessage msg(GUI_MSG_LABEL_ADD,GetID(),CONTROL_LIST,0,0,(void*)pItem);
				g_graphicsContext.SendMessage(msg);          
			}
			WCHAR wszText[20];
			const WCHAR* szText=g_localizeStrings.Get(127).c_str();
			swprintf(wszText,L"%i %s", m_vecList.size(),szText);

			SET_CONTROL_LABEL(GetID(), CONTROL_NUMBEROFFILES,wszText);

      if (m_bButtonEnabled)
      {
        CGUIMessage msg2(GUI_MSG_ENABLED,GetID(),CONTROL_BUTTON,0,0,NULL);
			  g_graphicsContext.SendMessage(msg2);  
      }
      else
      {
        CGUIMessage msg2(GUI_MSG_DISABLED,GetID(),CONTROL_BUTTON,0,0,NULL);
			  g_graphicsContext.SendMessage(msg2);  
      }
			return true;
		}
		break;


    case GUI_MSG_CLICKED:
    {
			
      int iControl=message.GetSenderId();
			if (CONTROL_LIST==iControl)
			{
				int iAction=message.GetParam1();
				if (ACTION_SELECT_ITEM==iAction)
				{
					CGUIMessage msg(GUI_MSG_ITEM_SELECTED,GetID(),iControl,0,0,NULL);
					g_graphicsContext.SendMessage(msg);         
					m_iSelected=msg.GetParam1();
					Close();
				}
			}
      if (CONTROL_BUTTON==iControl)
      {
        m_iSelected=-1;
        m_bButtonPressed=true;
        Close();
      }
    }
    break;
  }

  return CGUIWindow::OnMessage(message);
}


void CGUIDialogSelect::Reset()
{
	for (int i=0; i < (int)m_vecList.size(); ++i)
	{
		CGUIListItem* pItem=m_vecList[i];
		delete pItem;
	}
	m_vecList.erase(m_vecList.begin(),m_vecList.end());
  m_bButtonEnabled=false;

}

void CGUIDialogSelect::Add(const CStdString& strLabel)
{
	CGUIListItem* pItem = new CGUIListItem(strLabel);
	m_vecList.push_back(pItem);
}
int CGUIDialogSelect::GetSelectedLabel() const
{
	return m_iSelected;
}
void  CGUIDialogSelect::SetHeading(const wstring& strLine)
{
	CGUIMessage msg(GUI_MSG_LABEL_SET,GetID(),CONTROL_HEADING);
	msg.SetLabel(strLine);
	OnMessage(msg);
}

void  CGUIDialogSelect::SetHeading(const string& strLine)
{
	CGUIMessage msg(GUI_MSG_LABEL_SET,GetID(),CONTROL_HEADING);
	msg.SetLabel(strLine);
	OnMessage(msg);
}

void CGUIDialogSelect::SetHeading(int iString)
{
	CGUIMessage msg(GUI_MSG_LABEL_SET,GetID(),CONTROL_HEADING);
	msg.SetLabel(iString);
	OnMessage(msg);
}


void CGUIDialogSelect::EnableButton(bool bOnOff)
{
  m_bButtonEnabled=bOnOff;
}

void CGUIDialogSelect::SetButtonLabel(int iString)
{
	CGUIMessage msg(GUI_MSG_LABEL_SET,GetID(),CONTROL_BUTTON);
	msg.SetLabel(iString);
	OnMessage(msg);
}

bool CGUIDialogSelect::IsButtonPressed()
{
  return m_bButtonPressed;
}