
#include "guiwindowhome.h"
#include "localizestrings.h"
#include "texturemanager.h"
#include "xbox/iosupport.h"
#include "xbox/undocumented.h"
#include "xbox/xkutils.h"
#include "settings.h"
#include "sectionloader.h"
#include "util.h"
#include "application.h"

CGUIWindowHome::CGUIWindowHome(void)
:CGUIWindow(0)
{
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
      // make controls 101-110 invisible...
      for (int iControl=102; iControl < 110; iControl++)
      {
        CGUIMessage msg(GUI_MSG_HIDDEN,GetID(), iControl);
        g_graphicsContext.SendMessage(msg);
      }
    }

    case GUI_MSG_SETFOCUS:
    {
      int iControl=message.GetControlId();
      if (iControl>=2 && iControl <=8)
      {
        // make controls 101-110 invisible...
        for (int i=102; i < 110; i++)
        {
          CGUIMessage msg(GUI_MSG_HIDDEN,GetID(), i);
          g_graphicsContext.SendMessage(msg);
        }
        CGUIMessage msg(GUI_MSG_VISIBLE,GetID(), iControl+100);
        g_graphicsContext.SendMessage(msg);
      }
    }
    break;

    case GUI_MSG_CLICKED:
    {
      int iControl=message.GetSenderId();
      
      switch (iControl)
      {

        case 10: // powerdown
        {
					g_application.Stop();
          XKUtils::XBOXPowerOff();
        }
        break;

        case 11: // execute dashboard
        {
          char* szBackslash = strrchr(g_stSettings.szDashboard,'\\');
		      *szBackslash=0x00;
		      char* szXbe = &szBackslash[1];

		      char* szColon = strrchr(g_stSettings.szDashboard,':');
		      *szColon=0x00;
		      char* szDrive = g_stSettings.szDashboard;
		      char* szDirectory = &szColon[1];
          
          char szDevicePath[1024];
          char szXbePath[1024];
          CIoSupport helper;
		      helper.GetPartition( (LPCSTR) szDrive, szDevicePath);

		      strcat(szDevicePath,szDirectory);
		      wsprintf(szXbePath,"d:\\%s",szXbe);

					g_application.Stop();

          CUtil::LaunchXbe(szDevicePath,szXbePath,NULL);
        }
        break;
      
				case 12:
				{
					g_application.Stop();
					XKUtils::XBOXPowerCycle();
				}
      }
    }
    break;
  }

  return CGUIWindow::OnMessage(message);
}

void CGUIWindowHome::Render()
{
  WCHAR szText[128];

  SYSTEMTIME time;
	GetLocalTime(&time);

  GetDate(szText,&time);
	
  CGUIMessage msg(GUI_MSG_LABEL_SET,GetID(),200,0,0,(void*)szText);
  g_graphicsContext.SendMessage(msg);

	GetTime(szText,&time);
	CGUIMessage msg2(GUI_MSG_LABEL_SET,GetID(),201,0,0,(void*)szText);
  g_graphicsContext.SendMessage(msg2);

  CGUIWindow::Render();
}


VOID CGUIWindowHome::GetDate(WCHAR* wszDate, LPSYSTEMTIME pTime)
{
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

	swprintf(wszDate,L"%s, %s %d", day, month, pTime->wDay);
}

VOID CGUIWindowHome::GetTime(WCHAR* szTime, LPSYSTEMTIME pTime)
{
	INT iHour = pTime->wHour;
	swprintf(szTime,L"%02d:%02d", iHour, pTime->wMinute);
}