#include "stdafx.h"
#include "guiwindowhome.h"
#include "localizestrings.h"
#include "texturemanager.h"
#include "xbox/iosupport.h"
#include "xbox/undocumented.h"
#include "xbox/xkutils.h"
#include "settings.h"
#include "sectionloader.h"
#include "util.h"
#include "utils/KaiClient.h"
#include "application.h"
#include "Credits.h"
#include "GUIButtonScroller.h"
#include "GUIConditionalButtonControl.h"
//#include "GUIDialogContextMenu.h"

#define CONTROL_BTN_SHUTDOWN		10
#define CONTROL_BTN_DASHBOARD		11
#define CONTROL_BTN_REBOOT			12
#define CONTROL_BTN_CREDITS			13
#define CONTROL_BTN_ONLINE			14
#define CONTROL_BTN_XLINK_KAI		99
#define CONTROL_BTN_SCROLLER		300

CGUIWindowHome::CGUIWindowHome(void) : CGUIWindow(0)
{
	m_iLastControl=-1;
	m_iLastMenuOption=-1;

	ON_CLICK_MESSAGE(CONTROL_BTN_SHUTDOWN,	CGUIWindowHome, OnClickShutdown);	
	ON_CLICK_MESSAGE(CONTROL_BTN_DASHBOARD,	CGUIWindowHome, OnClickDashboard);	
	ON_CLICK_MESSAGE(CONTROL_BTN_REBOOT,	CGUIWindowHome, OnClickReboot);	
	ON_CLICK_MESSAGE(CONTROL_BTN_CREDITS,	CGUIWindowHome, OnClickCredits);	
	ON_CLICK_MESSAGE(CONTROL_BTN_ONLINE,	CGUIWindowHome, OnClickOnlineGaming);
}

CGUIWindowHome::~CGUIWindowHome(void)
{
}


bool CGUIWindowHome::OnMessage(CGUIMessage& message)
{
	switch ( message.GetMessage() )
	{
		case GUI_MSG_WINDOW_INIT:
		{
			int iFocusControl=m_iLastControl;
			CGUIWindow::OnMessage(message);
      
			// make controls 101-120 invisible...
			for (int iControl=102; iControl < 120; iControl++)
			{
				SET_CONTROL_HIDDEN(GetID(), iControl);
			}

			if (m_iLastMenuOption>0)
			{
				SET_CONTROL_VISIBLE(GetID(), m_iLastMenuOption+100);
			}

			if (iFocusControl<0)
			{
				iFocusControl=GetFocusedControl();
				m_iLastControl=iFocusControl;
			}

			UpdateButtonScroller();

			ON_POLL_BUTTON_CONDITION(CONTROL_BTN_XLINK_KAI, CGUIWindowHome, OnPollXLinkClient, 50);

			SET_CONTROL_FOCUS(GetID(), iFocusControl, 0);
			return true;
		}

		case GUI_MSG_SETFOCUS:
		{
			int iControl = message.GetControlId();
			m_iLastControl=iControl;
			if (iControl>=2 && iControl <=9)
			{
				m_iLastMenuOption = m_iLastControl;

				// make controls 101-120 invisible...
				for (int i=101; i < 120; i++)
				{
						SET_CONTROL_HIDDEN(GetID(), i);
				}
		
				SET_CONTROL_VISIBLE(GetID(), iControl+100);
			    break;
			}
			if (iControl == CONTROL_BTN_SCROLLER)
			{
				// the button scroller has changed focus...
				int iIconType = message.GetParam1();
				if (iIconType>=101 && iIconType<=120)
				{
					// make controls 101-120 invisible...
					for (int i=101; i < 120; i++)
					{
							SET_CONTROL_HIDDEN(GetID(), i);
					}
					SET_CONTROL_VISIBLE(GetID(), iIconType);
				}
				break;
			}
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
			m_iLastControl = message.GetSenderId();
			break;
		}
	}
	return CGUIWindow::OnMessage(message);
}

void CGUIWindowHome::OnClickShutdown(CGUIMessage& aMessage)
{
	g_applicationMessenger.Shutdown();
}

void CGUIWindowHome::OnClickDashboard(CGUIMessage& aMessage)
{
	CUtil::RunXBE(g_stSettings.szDashboard);
}

void CGUIWindowHome::OnClickReboot(CGUIMessage& aMessage)
{
	CGUIDialogYesNo* dlgYesNo = (CGUIDialogYesNo*)m_gWindowManager.GetWindow(WINDOW_DIALOG_YES_NO);
	if (dlgYesNo)
	{
		dlgYesNo->SetHeading(13313);
		dlgYesNo->SetLine(0, 13308);
		dlgYesNo->SetLine(1, 13309);
		dlgYesNo->SetLine(2, "");
		dlgYesNo->DoModal(GetID());

    if(dlgYesNo->IsConfirmed())
      g_applicationMessenger.Restart();
    else
      g_applicationMessenger.RestartApp();
  }
}

void CGUIWindowHome::OnClickCredits(CGUIMessage& aMessage)
{
	RunCredits();
}

void CGUIWindowHome::OnClickOnlineGaming(CGUIMessage& aMessage)
{
	m_gWindowManager.ActivateWindow( WINDOW_BUDDIES );
}

void CGUIWindowHome::OnAction(const CAction &action)
{
	if (action.wID == ACTION_CONTEXT_MENU)
	{
	}
	CGUIWindow::OnAction(action);
}

void CGUIWindowHome::Render()
{
  WCHAR szText[128];

  SYSTEMTIME time;
	GetLocalTime(&time);

  GetDate(szText,&time);
	
	SET_CONTROL_LABEL(GetID(), 200,szText);
	SET_CONTROL_LABEL(GetID(), 201,szText);

	GetTime(szText,&time);

	SET_CONTROL_LABEL(GetID(), 201,szText);

  CGUIWindow::Render();
}


VOID CGUIWindowHome::GetDate(WCHAR* wszDate, LPSYSTEMTIME pTime)
{
	if (!pTime) return;
	if (!wszDate) return;
	const WCHAR* day;
	switch (pTime->wDayOfWeek)
	{
    case 1 :	day = g_localizeStrings.Get(11).c_str();	break;
		case 2 :	day = g_localizeStrings.Get(12).c_str();	break;
		case 3 :	day = g_localizeStrings.Get(13).c_str();	break;
		case 4 :	day = g_localizeStrings.Get(14).c_str();	break;
		case 5 :	day = g_localizeStrings.Get(15).c_str();	break;
		case 6 :	day = g_localizeStrings.Get(16).c_str();	break;
		default:	day = g_localizeStrings.Get(17).c_str();	break;
	}

	const WCHAR* month;
	switch (pTime->wMonth)
	{
		case 1 :	month= g_localizeStrings.Get(21).c_str();	break;
		case 2 :	month= g_localizeStrings.Get(22).c_str();	break;
		case 3 :	month= g_localizeStrings.Get(23).c_str();	break;
		case 4 :	month= g_localizeStrings.Get(24).c_str();	break;
		case 5 :	month= g_localizeStrings.Get(25).c_str();	break;
		case 6 :	month= g_localizeStrings.Get(26).c_str();	break;
		case 7 :	month= g_localizeStrings.Get(27).c_str();	break;
		case 8 :	month= g_localizeStrings.Get(28).c_str();	break;
		case 9 :	month= g_localizeStrings.Get(29).c_str();	break;
		case 10:	month= g_localizeStrings.Get(30).c_str();	break;
		case 11:	month= g_localizeStrings.Get(31).c_str();	break;
		default:	month= g_localizeStrings.Get(32).c_str();	break;
	}

	if (day && month)
		swprintf(wszDate,L"%s, %s %d", day, month, pTime->wDay);
	else
		swprintf(wszDate,L"no date");
}

VOID CGUIWindowHome::GetTime(WCHAR* szTime, LPSYSTEMTIME pTime)
{
	if (!szTime) return;
	if (!pTime) return;
	INT iHour = pTime->wHour;
	swprintf(szTime,L"%02d:%02d", iHour, pTime->wMinute);
}

void CGUIWindowHome::UpdateButtonScroller()
{
	CGUIButtonScroller *pScroller = (CGUIButtonScroller *)GetControl(CONTROL_BTN_SCROLLER);
	if (pScroller)
	{
		int iButton = pScroller->GetActiveButton();
		pScroller->ClearButtons();
		for (int i=0; i<(int)g_settings.m_buttonSettings.m_vecButtons.size(); i++)
		{
			if (g_settings.m_buttonSettings.m_vecButtons[i]->m_dwLabel != -1)
			{	// grab the required label from our strings
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
		if (iButton>0)
			pScroller->SetActiveButton(iButton);
		else
			pScroller->SetActiveButton(g_settings.m_buttonSettings.m_iDefaultButton);
	}
}

void CGUIWindowHome::OnPopupContextMenu()
{
	CGUIButtonScroller *pScroller = (CGUIButtonScroller *)GetControl(CONTROL_BTN_SCROLLER);
	if (!pScroller) return;
	int iButton = pScroller->GetActiveButton();
	CGUIDialogContextMenu *pMenu = (CGUIDialogContextMenu *)m_gWindowManager.GetWindow(WINDOW_DIALOG_CONTEXT_MENU);
	if (!pMenu) return;
	// popup the context menu
	pMenu->ClearButtons();
	pMenu->AddButton(13332);	// Move Up
	pMenu->AddButton(13333);	// Move Down
	pMenu->AddButton(13334);	// Edit Label
	pMenu->AddButton(13335);	// Make Default
	pMenu->AddButton(13336);	// Remove Button
	// calculate our position
	int iPos[4];
	iPos[0] = pScroller->GetXPosition();
	iPos[1] = pScroller->GetYPosition();
	iPos[2] = g_graphicsContext.GetWidth() - (int)pScroller->GetWidth() - iPos[0];
	iPos[3] = g_graphicsContext.GetHeight() - (int)pScroller->GetHeight() - iPos[1];
	int iMax = 0;
	for (int i=1; i<4; i++)
		if (iPos[i] > iPos[iMax])
			iMax = i;
	int iPosX, iPosY;
	if (iMax == 0)
	{	// left
		iPosX = iPos[0] - (int)pMenu->GetWidth();
		iPosY = iPos[1] + ((int)pScroller->GetHeight() - (int)pMenu->GetHeight())/2;
	}
	else if (iMax == 1)
	{	// top
		iPosX = iPos[0] + ((int)pScroller->GetWidth() - (int)pMenu->GetWidth())/2;
		iPosY = iPos[1] - (int)pMenu->GetHeight();
	}
	else if (iMax == 2)
	{	// right
		iPosX = g_graphicsContext.GetWidth() - iPos[2];
		iPosY = iPos[1] + ((int)pScroller->GetHeight() - (int)pMenu->GetHeight())/2;
	}
	else // if (iMax == 3)
	{	// bottom
		iPosX = iPos[0] + ((int)pScroller->GetWidth() - (int)pMenu->GetWidth())/2;
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
		{	// Move up
			CButtonScrollerSettings::CButton *pButton = g_settings.m_buttonSettings.m_vecButtons[iButton];
			int iNewButton = iButton-1;
			if (iNewButton<0)
			{
				iNewButton = (int)g_settings.m_buttonSettings.m_vecButtons.size()-1;
				CButtonScrollerSettings::CButton *pButton2 = g_settings.m_buttonSettings.m_vecButtons[iNewButton];
				g_settings.m_buttonSettings.m_vecButtons.erase(g_settings.m_buttonSettings.m_vecButtons.begin());
				g_settings.m_buttonSettings.m_vecButtons.erase(g_settings.m_buttonSettings.m_vecButtons.end()-1);
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
		{	// Move down
			int iNewButton = iButton+1;
			if (iNewButton > (int)g_settings.m_buttonSettings.m_vecButtons.size()-1)
			{	// swap first and last items...
				iNewButton = 0;
				CButtonScrollerSettings::CButton *pButton2 = g_settings.m_buttonSettings.m_vecButtons[iButton];
				CButtonScrollerSettings::CButton *pButton = g_settings.m_buttonSettings.m_vecButtons[iNewButton];
				g_settings.m_buttonSettings.m_vecButtons.erase(g_settings.m_buttonSettings.m_vecButtons.begin());
				g_settings.m_buttonSettings.m_vecButtons.erase(g_settings.m_buttonSettings.m_vecButtons.end()-1);
				g_settings.m_buttonSettings.m_vecButtons.insert(g_settings.m_buttonSettings.m_vecButtons.begin(), pButton2);
				g_settings.m_buttonSettings.m_vecButtons.insert(g_settings.m_buttonSettings.m_vecButtons.end(), pButton);
			}
			else
			{	// swap this and the next item
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
		{	// Edit Label
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
		{	// Make default
			g_settings.m_buttonSettings.m_iDefaultButton = iButton;
			g_settings.SaveHomeButtons();
		}
		break;
	case 5:
		{	// Remove Button
			// check we have at least 2 buttons present
			if (g_settings.m_buttonSettings.m_vecButtons.size() < 2)
				return;
			CButtonScrollerSettings::CButton *pButton = g_settings.m_buttonSettings.m_vecButtons[iButton];
			if (pButton) delete pButton;
			g_settings.m_buttonSettings.m_vecButtons.erase(g_settings.m_buttonSettings.m_vecButtons.begin() + iButton);
			if (g_settings.m_buttonSettings.m_iDefaultButton == iButton)
				g_settings.m_buttonSettings.m_iDefaultButton = (iButton == 0) ? g_settings.m_buttonSettings.m_vecButtons.size()-1 : iButton-1;
			g_settings.SaveHomeButtons();
			UpdateButtonScroller();
		}
		break;
	}
}

bool CGUIWindowHome::OnPollXLinkClient(CGUIConditionalButtonControl* pButton)
{
	return CKaiClient::GetInstance()->IsEngineConnected();
}