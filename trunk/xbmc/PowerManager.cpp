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

#include "system.h"
#include "PowerManager.h"
#include "Application.h"
#include "GUIWindowManager.h"
#include "GUIDialogVideoScan.h"
#include "GUIDialogMusicScan.h"
#include "KeyboardStat.h"
#include "MouseStat.h"
#include "GUISettings.h"
#include "WindowingFactory.h"

#ifdef HAS_LCD
#include "utils/LCDFactory.h"
#endif

#ifdef __APPLE__
#include "osx/CocoaPowerSyscall.h"
#elif defined(_LINUX) && defined(HAS_DBUS)
#include "linux/DBusPowerSyscall.h"
#elif defined(_WIN32)
#include "win32/Win32PowerSyscall.h"
#endif

#ifdef HAS_LIRC
#include "common/LIRC.h"
#endif

CPowerManager g_powerManager;

CPowerManager::CPowerManager()
{
  m_instance = NULL;

#ifdef __APPLE__
  m_instance = new CCocoaPowerSyscall();
#elif defined(_LINUX) && defined(HAS_DBUS)
  m_instance = new CDBusPowerSyscall();
#elif defined(_WIN32)
  m_instance = new CWin32PowerSyscall();
#endif

  if (m_instance == NULL)
    m_instance = new CNullPowerSyscall();
}

CPowerManager::~CPowerManager()
{
  delete m_instance;
}

void CPowerManager::Initialize()
{
  int defaultShutdown = g_guiSettings.GetInt("system.shutdownstate");

  switch (defaultShutdown)
  {
    case POWERSTATE_QUIT:
    case POWERSTATE_MINIMIZE:
      // assume we can shutdown if --standalone is passed
      if (g_application.IsStandAlone())
        defaultShutdown = POWERSTATE_SHUTDOWN;
    break;
    case POWERSTATE_HIBERNATE:
      if (!g_powerManager.CanHibernate())
      {
        if (g_powerManager.CanSuspend())
          defaultShutdown = POWERSTATE_SUSPEND;
        else
          defaultShutdown = g_powerManager.CanPowerdown() ? POWERSTATE_SHUTDOWN : POWERSTATE_QUIT;
      }
    break;
    case POWERSTATE_SUSPEND:
      if (!g_powerManager.CanSuspend())
      {
        if (g_powerManager.CanHibernate())
          defaultShutdown = POWERSTATE_HIBERNATE;
        else
          defaultShutdown = g_powerManager.CanPowerdown() ? POWERSTATE_SHUTDOWN : POWERSTATE_QUIT;
      }
    break;
    case POWERSTATE_SHUTDOWN:
      if (!g_powerManager.CanPowerdown())
      {
        if (g_powerManager.CanSuspend())
          defaultShutdown = POWERSTATE_SUSPEND;
        else
          defaultShutdown = g_powerManager.CanHibernate() ? POWERSTATE_HIBERNATE : POWERSTATE_QUIT;
      }
    break;
  }

  g_guiSettings.SetInt("system.shutdownstate", defaultShutdown);
}
  
bool CPowerManager::Powerdown()
{
  return CanPowerdown() ? m_instance->Powerdown() : false;
}
bool CPowerManager::Suspend()
{
  if (CanSuspend())
  {
    g_application.m_bRunResumeJobs = true;
#ifdef HAS_LCD
    g_lcd->SetBackLight(0);
#endif
    g_Keyboard.ResetState();
    return m_instance->Suspend();
  }
  
  return false;
}
bool CPowerManager::Hibernate()
{
  if (CanHibernate())
  {
    g_application.m_bRunResumeJobs = true;
    g_Keyboard.ResetState();
    return m_instance->Hibernate();
  }

  return false;
}
bool CPowerManager::Reboot()
{
  return CanReboot() ? m_instance->Reboot() : false;
}

void CPowerManager::Resume()
{
  CLog::Log(LOGNOTICE, "%s: Running resume jobs", __FUNCTION__);

#ifdef HAS_SDL
  // Hack to reclaim focus, thus rehiding system mouse pointer.
  // Surely there's a better way?
  if (g_Windowing.IsFullScreen())
  {
    g_graphicsContext.ToggleFullScreenRoot();
    g_graphicsContext.ToggleFullScreenRoot();
  }
  g_application.ResetScreenSaver();
#endif

  // restart lirc
#ifdef HAS_LIRC
  CLog::Log(LOGNOTICE, "%s: Restarting lirc", __FUNCTION__);
  g_RemoteControl.Disconnect();
  g_RemoteControl.Initialize();
#endif

  // restart and undim lcd
#ifdef HAS_LCD
  CLog::Log(LOGNOTICE, "%s: Restarting lcd", __FUNCTION__);
#ifdef _LINUX
  g_lcd->SetBackLight(1);
#else
  g_lcd->SetBackLight(g_guiSettings.GetInt("lcd.backlight"));
#endif
  g_lcd->Stop();
  g_lcd->Initialize();
#endif

  g_application.UpdateLibraries();

  // reset
  g_application.m_bRunResumeJobs = false;
}

bool CPowerManager::CanPowerdown()
{
  return m_instance->CanPowerdown();
}
bool CPowerManager::CanSuspend()
{
  return m_instance->CanSuspend();
}
bool CPowerManager::CanHibernate()
{
  return m_instance->CanHibernate();
}
bool CPowerManager::CanReboot()
{
  return m_instance->CanReboot();
}
