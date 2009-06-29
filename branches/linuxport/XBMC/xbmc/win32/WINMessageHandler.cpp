/*
 *      Copyright (C) 2005-2009 Team XBMC
 *      http://www.xbmc.org
 *
 *  This Program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your option)
 *  any later version.
 *
 *  This Program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with XBMC; see the file COPYING.  If not, write to
 *  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 *  http://www.gnu.org/copyleft/gpl.html
 *
 */

#include "stdafx.h"
#include "WINMessageHandler.h"
#include "WIN32Util.h"
#include <dbt.h>
#include "GUIWindowManager.h"
#include "Settings.h"
#include "VideoReferenceClock.h"
#include "Application.h"

extern HWND g_hWnd;
WNDPROC g_lpOriginalWndProc=NULL;
UINT g_uQueryCancelAutoPlay = 0;
LRESULT CALLBACK WndProc (HWND hWnd, UINT Message, WPARAM wParam, LPARAM lParam);

CWINMessageHandler::CWINMessageHandler()
{
  m_bHooked = false;
}

CWINMessageHandler::~CWINMessageHandler()
{
}

bool CWINMessageHandler::Initialize()
{
  if(m_bHooked)
    return true;

  g_lpOriginalWndProc = (WNDPROC) SetWindowLongPtr(g_hWnd, GWL_WNDPROC, (LONG_PTR) WndProc);
  
  if(g_lpOriginalWndProc == NULL)
    return false;

  g_uQueryCancelAutoPlay = RegisterWindowMessage(TEXT("QueryCancelAutoPlay"));

  return m_bHooked = true;
}

LRESULT CALLBACK WndProc (HWND hWnd, UINT Message, WPARAM wParam, LPARAM lParam)
{
  PDEV_BROADCAST_HDR lpdb = (PDEV_BROADCAST_HDR)lParam;
  char szMsg[80];

  switch(Message)
  {
  case WM_DEVICECHANGE:
    switch(wParam)
    {
      case DBT_DEVICEARRIVAL:
         if (lpdb -> dbch_devicetype == DBT_DEVTYP_VOLUME)
         {
            PDEV_BROADCAST_VOLUME lpdbv = (PDEV_BROADCAST_VOLUME)lpdb;

            // Check whether a CD or DVD was inserted into a drive.
            if (lpdbv -> dbcv_flags & DBTF_MEDIA)
            {
               sprintf (szMsg, "Drive %c: Media has arrived.\n", CWIN32Util::FirstDriveFromMask(lpdbv ->dbcv_unitmask));
               OutputDebugString(szMsg);
            }
            else
            {
              // USB drive inserted
              CMediaSource share;
              CStdString strDrive;
              strDrive.Format("%c:",CWIN32Util::FirstDriveFromMask(lpdbv ->dbcv_unitmask));
              share.strName.Format("%s (%s)", g_localizeStrings.Get(437), strDrive);
              share.strPath = strDrive;
              share.m_ignore = true;
              share.m_iDriveType = CMediaSource::SOURCE_TYPE_REMOVABLE;
              g_settings.AddShare("files",share);
              g_settings.AddShare("video",share);
              g_settings.AddShare("pictures",share);
              g_settings.AddShare("music",share);
              g_settings.AddShare("programs",share);
              CGUIMessage msg(GUI_MSG_NOTIFY_ALL, 0, 0, GUI_MSG_UPDATE_SOURCES);
              m_gWindowManager.SendThreadMessage( msg );
            }
         }
         break;

      case DBT_DEVICEREMOVECOMPLETE:
         if (lpdb -> dbch_devicetype == DBT_DEVTYP_VOLUME)
         {
            PDEV_BROADCAST_VOLUME lpdbv = (PDEV_BROADCAST_VOLUME)lpdb;
        
            // Check whether a CD or DVD was removed from a drive.
            if (lpdbv -> dbcv_flags & DBTF_MEDIA)
            {
               sprintf (szMsg, "Drive %c: Media was removed.\n", CWIN32Util::FirstDriveFromMask(lpdbv ->dbcv_unitmask));
               OutputDebugString(szMsg);
            }
            else
            {
              // USB drive was removed
              CStdString strDrive;
              CStdString strName;
              strDrive.Format("%c:",CWIN32Util::FirstDriveFromMask(lpdbv ->dbcv_unitmask));
              strName.Format("%s (%s)", g_localizeStrings.Get(437), strDrive);
              g_settings.DeleteSource("files",strName,strDrive);
              g_settings.DeleteSource("video",strName,strDrive);
              g_settings.DeleteSource("pictures",strName,strDrive);
              g_settings.DeleteSource("music",strName,strDrive);
              g_settings.DeleteSource("programs",strName,strDrive);
              CGUIMessage msg(GUI_MSG_NOTIFY_ALL, 0, 0, GUI_MSG_UPDATE_SOURCES);
              m_gWindowManager.SendThreadMessage( msg );
            }
         }
         break;
    }
  case WM_POWERBROADCAST:
    if(wParam == PBT_APMRESUMESUSPEND || wParam == PBT_APMRESUMEAUTOMATIC)
    {
      // TODO: reconnect shares/network, etc
      CLog::Log(LOGINFO, "Resuming from suspend" );
      g_application.ResetScreenSaver();
      if(g_advancedSettings.m_fullScreen)
      {
        ShowWindow(g_hWnd,SW_RESTORE);
        SetForegroundWindow(g_hWnd);
        LockSetForegroundWindow(LSFW_LOCK);
      }
    }
    break;
  case WM_MOVE:
    {
      RECT rBounds;
      HMONITOR hMonitor;
      MONITORINFOEX mi;
      
      //get the monitor the main window is on
      GetWindowRect(g_hWnd, &rBounds);
      hMonitor = MonitorFromRect(&rBounds, MONITOR_DEFAULTTONEAREST);
      mi.cbSize = sizeof(mi);
      GetMonitorInfo(hMonitor, &mi);

      g_VideoReferenceClock.SetMonitor(mi); //let the videoreferenceclock know which monitor we're on
    }
    break;
  }

  // disable windows autoplay
  if(g_uQueryCancelAutoPlay != 0 && Message == g_uQueryCancelAutoPlay)
    return 1;


  return CallWindowProc(g_lpOriginalWndProc, hWnd, Message, wParam, lParam);
}