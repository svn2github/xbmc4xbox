#pragma once

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

#include "GUIMediaWindow.h"
#include "GUIEPGGridContainer.h"

enum TVWindow
{
  TV_WINDOW_UNKNOWN         = 0,
  TV_WINDOW_TV_PROGRAM      = 1,
  TV_WINDOW_CHANNELS_TV     = 2,
  TV_WINDOW_CHANNELS_RADIO  = 3,
  TV_WINDOW_RECORDINGS      = 4,
  TV_WINDOW_TIMERS          = 5,
  TV_WINDOW_SETTINGS        = 6
};

class CGUIWindowTV : public CGUIMediaWindow
{
public:
  CGUIWindowTV(void);
  virtual ~CGUIWindowTV(void);
  virtual bool OnMessage(CGUIMessage& message);
  virtual bool OnAction(const CAction &action);

  void UpdateData(TVWindow update);

protected:
  virtual bool OnPopupMenu(int iItem);
  virtual void GetContextButtons(int itemNumber, CContextButtons &buttons);
  virtual bool OnContextButton(int itemNumber, CONTEXT_BUTTON button);
  virtual void UpdateButtons();

private:
  TVWindow m_iCurrSubTVWindow;    /* Active subwindow */
  TVWindow m_iSavedSubTVWindow;   /* Last subwindow, required if main window is shown again */
  bool m_bShowHiddenChannels;

  /* Selected item in associated list, required for subwindow change */
  int m_iSelected_GUIDE;
  int m_iSelected_CHANNELS_TV;
  int m_iSelected_CHANNELS_RADIO;
  int m_iSelected_RECORDINGS;
  int m_iSelected_TIMERS;

  int m_iGuideView;
  int m_iCurrentTVGroup;
  int m_iCurrentRadioGroup;

  void ShowEPGInfo(CFileItem *item);
  void ShowRecordingInfo(CFileItem *item);
  bool ShowTimerSettings(CFileItem *item);
  void ShowGroupManager(bool IsRadio);

  void UpdateGuide();
  void UpdateChannelsTV();
  void UpdateChannelsRadio();
  void UpdateRecordings();
  void UpdateTimers();

  CGUIEPGGridContainer   *m_guideGrid;
};
