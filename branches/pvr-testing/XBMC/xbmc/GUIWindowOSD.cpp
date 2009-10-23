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

#include "GUIWindowOSD.h"
#include "Application.h"
#include "GUIUserMessages.h"
#include "GUIWindowManager.h"
#include "MouseStat.h"

CGUIWindowOSD::CGUIWindowOSD(void)
    : CGUIDialog(WINDOW_OSD, "VideoOSD.xml")
{
}

CGUIWindowOSD::~CGUIWindowOSD(void)
{
}

void CGUIWindowOSD::OnWindowLoaded()
{
  CGUIDialog::OnWindowLoaded();
  m_bRelativeCoords = true;
}

void CGUIWindowOSD::Render()
{
  if (m_autoClosing)
  {
    // check for movement of mouse or a submenu open
    if (g_Mouse.HasMoved() || g_windowManager.IsWindowActive(WINDOW_DIALOG_AUDIO_OSD_SETTINGS)
                           || g_windowManager.IsWindowActive(WINDOW_DIALOG_VIDEO_OSD_SETTINGS)
                           || g_windowManager.IsWindowActive(WINDOW_DIALOG_VIDEO_BOOKMARKS)
                           || g_windowManager.IsWindowActive(WINDOW_DIALOG_TV_OSD_CHANNELS)
                           || g_windowManager.IsWindowActive(WINDOW_DIALOG_TV_OSD_GUIDE)
                           || g_windowManager.IsWindowActive(WINDOW_DIALOG_TV_OSD_DIRECTOR)
                           || g_windowManager.IsWindowActive(WINDOW_DIALOG_OSD_TELETEXT))
      SetAutoClose(3000);
  }
  CGUIDialog::Render();
}

bool CGUIWindowOSD::OnAction(const CAction &action)
{
  // keyboard or controller movement should prevent autoclosing
  if (action.id != ACTION_MOUSE && m_autoClosing)
    SetAutoClose(3000);

  if (action.id == ACTION_NEXT_ITEM || action.id == ACTION_PREV_ITEM)
  {
    // these could indicate next chapter if video supports it
    if (g_application.m_pPlayer != NULL && g_application.m_pPlayer->OnAction(action))
      return true;
  }

  return CGUIDialog::OnAction(action);
}

bool CGUIWindowOSD::OnMouse(const CPoint &point)
{
  if (g_Mouse.GetWheel())
  { // Mouse wheel
    int wheel = abs(g_Mouse.GetWheel());
    CAction action;
    action.amount1 = 0.5f * (float)wheel;
    action.id = g_Mouse.GetWheel() > 0 ? ACTION_ANALOG_SEEK_FORWARD : ACTION_ANALOG_SEEK_BACK;
    return g_application.OnAction(action);
  }
  if (g_Mouse.bClick[MOUSE_LEFT_BUTTON])
  { // pause
    CAction action;
    action.id = ACTION_PAUSE;
    return g_application.OnAction(action);
  }

  return CGUIDialog::OnMouse(point);
}

bool CGUIWindowOSD::OnMessage(CGUIMessage& message)
{
  switch ( message.GetMessage() )
  {
  case GUI_MSG_VIDEO_MENU_STARTED:
    {
      // We have gone to the DVD menu, so close the OSD.
      Close();
    }
  case GUI_MSG_WINDOW_DEINIT:  // fired when OSD is hidden
    {
      // Remove our subdialogs if visible
      CGUIDialog *pDialog = (CGUIDialog *)g_windowManager.GetWindow(WINDOW_DIALOG_VIDEO_OSD_SETTINGS);
      if (pDialog && pDialog->IsDialogRunning())
        pDialog->Close(true);
      pDialog = (CGUIDialog *)g_windowManager.GetWindow(WINDOW_DIALOG_AUDIO_OSD_SETTINGS);
      if (pDialog && pDialog->IsDialogRunning()) pDialog->Close(true);
      pDialog = (CGUIDialog *)g_windowManager.GetWindow(WINDOW_DIALOG_VIDEO_BOOKMARKS);
      if (pDialog && pDialog->IsDialogRunning()) pDialog->Close(true);
      pDialog = (CGUIDialog *)g_windowManager.GetWindow(WINDOW_DIALOG_TV_OSD_CHANNELS);
      if (pDialog && pDialog->IsDialogRunning()) pDialog->Close(true);
      pDialog = (CGUIDialog *)g_windowManager.GetWindow(WINDOW_DIALOG_TV_OSD_GUIDE);
      if (pDialog && pDialog->IsDialogRunning()) pDialog->Close(true);
      pDialog = (CGUIDialog *)g_windowManager.GetWindow(WINDOW_DIALOG_TV_OSD_DIRECTOR);
      if (pDialog && pDialog->IsDialogRunning()) pDialog->Close(true);
      pDialog = (CGUIDialog *)g_windowManager.GetWindow(WINDOW_DIALOG_OSD_TELETEXT);
      if (pDialog && pDialog->IsDialogRunning()) pDialog->Close(true);
    }
    break;
  }
  return CGUIDialog::OnMessage(message);
}

