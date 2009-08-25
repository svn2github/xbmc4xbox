/*
 *      Copyright (C) 2005-2008 Team XBMC
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
#include "GUIWindowScreensaver.h"
#include "utils/AddonManager.h"
#include "Application.h"
#include "GUIPassword.h"
#include "GUISettings.h"
#include "GUIWindowManager.h"

using namespace ADDON;

CGUIWindowScreensaver::CGUIWindowScreensaver(void)
    : CGUIWindow(WINDOW_SCREENSAVER, "")
{
#ifdef HAS_SCREENSAVER
  m_pScreenSaver = NULL;
  m_addon.reset();
#endif
}

CGUIWindowScreensaver::~CGUIWindowScreensaver(void)
{
}

void CGUIWindowScreensaver::Render()
{
  CSingleLock lock (m_critSection);

#ifdef HAS_SCREENSAVER
  if (m_pScreenSaver)
  {
    if (m_bInitialized)
    {
      try
      {
        //some screensavers seem to be depending on xbmc clearing the screen
        //       g_graphicsContext.Get3DDevice()->Clear( 0L, NULL, D3DCLEAR_TARGET|D3DCLEAR_ZBUFFER, 0x00010001, 1.0f, 0L );
        g_graphicsContext.CaptureStateBlock();
        m_pScreenSaver->Render();
        g_graphicsContext.ApplyStateBlock();
      }
      catch (...)
      {
        CLog::Log(LOGERROR, "SCREENSAVER: - Exception in Render() - %s", m_addon->Name().c_str());
      }
      return ;
    }
    else
    {
      try
      {
        m_pScreenSaver->Start();
        m_bInitialized = true;
      }
      catch (...)
      {
        CLog::Log(LOGERROR, "SCREENSAVER: - Exception in Start() - %s", m_addon->Name().c_str());
      }
      return ;
    }
  }
#endif
  CGUIWindow::Render();
}

bool CGUIWindowScreensaver::OnAction(const CAction &action)
{
  // We're just a screen saver, nothing to do here
  return false;
}

// called when the mouse is moved/clicked etc. etc.
bool CGUIWindowScreensaver::OnMouse(const CPoint &point)
{
  m_gWindowManager.PreviousWindow();
  return true;
}

bool CGUIWindowScreensaver::OnMessage(CGUIMessage& message)
{
  switch ( message.GetMessage() )
  {
  case GUI_MSG_WINDOW_DEINIT:
    {
      CSingleLock lock (m_critSection);
#ifdef HAS_SCREENSAVER
      if (m_pScreenSaver)
      {
        CLog::Log(LOGDEBUG, "SCREENSAVER: - Stopping - %s", m_addon->Name().c_str());
        m_pScreenSaver->Stop();
        delete m_pScreenSaver;

        g_graphicsContext.ApplyStateBlock();
      }
      m_pScreenSaver = NULL;
#endif
      m_bInitialized = false;

      // remove z-buffer
//      RESOLUTION res = g_graphicsContext.GetVideoResolution();
 //     g_graphicsContext.SetVideoResolution(res, FALSE);

      // enable the overlay
      m_gWindowManager.ShowOverlay(OVERLAY_STATE_SHOWN);
    }
    break;

  case GUI_MSG_WINDOW_INIT:
    {
      CGUIWindow::OnMessage(message);
      CSingleLock lock (m_critSection);

#ifdef HAS_SCREENSAVER
      if (m_pScreenSaver)
      {
        m_pScreenSaver->Stop();
        delete m_pScreenSaver;
        g_graphicsContext.ApplyStateBlock();
      }
      m_bInitialized = false;
      
      m_addon.reset();
      // Setup new screensaver instance
      AddonPtr addon;
      if (!CAddonMgr::Get()->GetAddon(ADDON_SCREENSAVER, g_guiSettings.GetString("screensaver.mode"), addon))
        return false;
      m_addon = boost::dynamic_pointer_cast<CScreenSaver>(addon);

      if (!m_addon)
        return false;

      m_pScreenSaver = NULL; //TODO addonmanager loader /*factory.LoadScreenSaver(m_addon);*/
      if (m_pScreenSaver)
      {
        g_graphicsContext.CaptureStateBlock();
        m_addon->Create();
      }
#endif
      // setup a z-buffer
//      RESOLUTION res = g_graphicsContext.GetVideoResolution();
//      g_graphicsContext.SetVideoResolution(res, TRUE);how was 

      // disable the overlay
      m_gWindowManager.ShowOverlay(OVERLAY_STATE_HIDDEN);
      return true;
    }
  case GUI_MSG_CHECK_LOCK:
    if (!g_passwordManager.IsProfileLockUnlocked())
    {
      g_application.m_iScreenSaveLock = -1;
      return false;
    }
    g_application.m_iScreenSaveLock = 1;
    return true;
  }
  return CGUIWindow::OnMessage(message);
}
