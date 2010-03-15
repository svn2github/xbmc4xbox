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

#include "GUIWindowVisualisation.h"
#include "GUIVisualisationControl.h"
#include "visualizations/Visualisation.h"
#include "Application.h"
#include "GUIDialogMusicOSD.h"
#include "GUIUserMessages.h"
#include "utils/GUIInfoManager.h"
#include "ButtonTranslator.h"
#include "GUIDialogVisualisationPresetList.h"
#include "GUIWindowManager.h"
#include "Settings.h"
#include "AdvancedSettings.h"

using namespace MUSIC_INFO;
using namespace ADDON;

#define TRANSISTION_COUNT   50  // 1 second
#define TRANSISTION_LENGTH 200  // 4 seconds
#define START_FADE_LENGTH  100  // 2 seconds on startup

#define CONTROL_VIS          2

CGUIWindowVisualisation::CGUIWindowVisualisation(void)
    : CGUIWindow(WINDOW_VISUALISATION, "MusicVisualisation.xml")
{
  m_initTimer = 0;
  m_lockedTimer = 0;
  m_bShowPreset = false;
}

bool CGUIWindowVisualisation::OnAction(const CAction &action)
{
  VIS_ACTION visAction = VIS_ACTION_NONE;
  switch (action.GetID())
  {
  case ACTION_VIS_PRESET_NEXT:
    visAction = VIS_ACTION_NEXT_PRESET; break;
  case ACTION_VIS_PRESET_PREV:
    visAction = VIS_ACTION_PREV_PRESET; break;
  case ACTION_VIS_PRESET_RANDOM:
    visAction = VIS_ACTION_RANDOM_PRESET; break;
  case ACTION_VIS_RATE_PRESET_PLUS:
    visAction = VIS_ACTION_RATE_PRESET_PLUS; break;
  case ACTION_VIS_RATE_PRESET_MINUS:
    visAction = VIS_ACTION_RATE_PRESET_MINUS; break;
  case ACTION_SHOW_INFO:
    {
      if (!m_initTimer || g_settings.m_bMyMusicSongThumbInVis)
        g_settings.m_bMyMusicSongThumbInVis = !g_settings.m_bMyMusicSongThumbInVis;
      g_infoManager.SetShowInfo(g_settings.m_bMyMusicSongThumbInVis);
      return true;
    }
    break;

  case ACTION_SHOW_GUI:
    // save the settings
    g_settings.Save();
    g_windowManager.PreviousWindow();
    return true;
    break;

  case ACTION_VIS_PRESET_LOCK:
    { // show the locked icon + fall through so that the vis handles the locking
      if (!m_bShowPreset)
      {
        m_lockedTimer = START_FADE_LENGTH;
        g_infoManager.SetShowCodec(true);
      }
    }
    break;
  case ACTION_VIS_PRESET_SHOW:
    {
      if (!m_lockedTimer || m_bShowPreset)
        m_bShowPreset = !m_bShowPreset;
      g_infoManager.SetShowCodec(m_bShowPreset);
      return true;
    }
    break;

  case ACTION_DECREASE_RATING:
  case ACTION_INCREASE_RATING:
    {
      // actual action is taken care of in CApplication::OnAction()
      m_initTimer = g_advancedSettings.m_songInfoDuration * 50;
      g_infoManager.SetShowInfo(true);
    }
    break;
    // TODO: These should be mapped to it's own function - at the moment it's overriding
    // the global action of fastforward/rewind and OSD.
/*  case KEY_BUTTON_Y:
    g_application.m_CdgParser.Pause();
    return true;
    break;

    case ACTION_ANALOG_FORWARD:
    // calculate the speed based on the amount the button is held down
    if (action.GetAmount())
    {
      float AVDelay = g_application.m_CdgParser.GetAVDelay();
      g_application.m_CdgParser.SetAVDelay(AVDelay - action.GetAmount() / 4.0f);
      return true;
    }
    break;*/
  }

  if (visAction != VIS_ACTION_NONE && m_addon)
    return m_addon->OnAction(visAction);

  return CGUIWindow::OnAction(action);
}

bool CGUIWindowVisualisation::OnMessage(CGUIMessage& message)
{
  switch ( message.GetMessage() )
  {
  case GUI_MSG_GET_VISUALISATION:
    {
      if (m_addon)
        message.SetPointer(m_addon.get());
      return m_addon;
    }
    break;
  case GUI_MSG_VISUALISATION_ACTION:
  {
    CAction action(message.GetParam1());
    return OnAction(action);
  }
  case GUI_MSG_PLAYBACK_STARTED:
  {
    if (IsActive() && m_addon)
    {
      m_addon->UpdateTrack();
    }
    break;
  }
  case GUI_MSG_WINDOW_DEINIT:
    {
      if (IsActive()) // save any changed settings from the OSD
        g_settings.Save();
      // check and close any OSD windows
      CGUIDialogMusicOSD *pOSD = (CGUIDialogMusicOSD *)g_windowManager.GetWindow(WINDOW_DIALOG_MUSIC_OSD);
      if (pOSD && pOSD->IsDialogRunning()) pOSD->Close(true);
      CGUIDialogVisualisationPresetList *pList = (CGUIDialogVisualisationPresetList *)g_windowManager.GetWindow(WINDOW_DIALOG_VIS_PRESET_LIST);
      if (pList && pList->IsDialogRunning()) pList->Close(true);
    }
    break;
  case GUI_MSG_WINDOW_INIT:
    {
      // check whether we've come back here from a window during which time we've actually
      // stopped playing music
      if (message.GetParam1() == WINDOW_INVALID && !g_application.IsPlayingAudio())
      { // why are we here if nothing is playing???
        g_windowManager.PreviousWindow();
        return true;
      }

      AddonPtr viz;
      CAddonMgr::Get()->GetDefault(ADDON_VIZ, viz);
      m_addon = boost::dynamic_pointer_cast<CVisualisation>(viz);
      if (!m_addon)
        return false;

      // hide or show the preset button(s)
      g_infoManager.SetShowCodec(m_bShowPreset);
      g_infoManager.SetShowInfo(true);  // always show the info initially.
      CGUIWindow::OnMessage(message);
      if (g_infoManager.GetCurrentSongTag())
        m_tag = *g_infoManager.GetCurrentSongTag();

      if (g_settings.m_bMyMusicSongThumbInVis)
      { // always on
        m_initTimer = 0;
      }
      else
      {
        // start display init timer (fade out after 3 secs...)
        m_initTimer = g_advancedSettings.m_songInfoDuration * 50;
      }
      return true;
    }
  }
  return CGUIWindow::OnMessage(message);
}

void CGUIWindowVisualisation::OnWindowLoaded()
{
  if (m_addon)
  {
    CGUIVisualisationControl *pVisControl = (CGUIVisualisationControl *)GetControl(CONTROL_VIS);
    if (pVisControl)
      pVisControl->LoadAddon(m_addon);
  }
}

bool CGUIWindowVisualisation::UpdateTrack()
{
  if (m_addon)
  {
    return m_addon->UpdateTrack();
  }
  return false;
}

bool CGUIWindowVisualisation::OnMouseEvent(const CPoint &point, const CMouseEvent &event)
{
  if (event.m_id == ACTION_MOUSE_RIGHT_CLICK)
  { // no control found to absorb this click - go back to GUI
    OnAction(CAction(ACTION_SHOW_GUI));
    return true;
  }
  if (event.m_id == ACTION_MOUSE_LEFT_CLICK)
  { // no control found to absorb this click - toggle the track INFO
    return g_application.OnAction(CAction(ACTION_PAUSE));
  }
  if (event.m_id != ACTION_MOUSE_MOVE || event.m_offsetX || event.m_offsetY)
  { // some other mouse action has occurred - bring up the OSD
    CGUIDialog *pOSD = (CGUIDialog *)g_windowManager.GetWindow(WINDOW_DIALOG_MUSIC_OSD);
    if (pOSD)
    {
      pOSD->SetAutoClose(3000);
      pOSD->DoModal();
    }
    return true;
  }
  return false;
}

void CGUIWindowVisualisation::FrameMove()
{
  g_application.ResetScreenSaver(); //why here?
  // check for a tag change
  const CMusicInfoTag* tag = g_infoManager.GetCurrentSongTag();
  if (tag && *tag != m_tag)
  { // need to fade in then out again
    m_tag = *tag;
    // fade in
    m_initTimer = g_advancedSettings.m_songInfoDuration * 50;
    g_infoManager.SetShowInfo(true);
  }
  if (m_initTimer)
  {
    m_initTimer--;
    if (!m_initTimer && !g_settings.m_bMyMusicSongThumbInVis)
    { // reached end of fade in, fade out again
      g_infoManager.SetShowInfo(false);
    }
  }
  // show or hide the locked texture
  if (m_lockedTimer)
  {
    m_lockedTimer--;
    if (!m_lockedTimer && !m_bShowPreset)
      g_infoManager.SetShowCodec(false);
  }
  CGUIWindow::FrameMove();
}
