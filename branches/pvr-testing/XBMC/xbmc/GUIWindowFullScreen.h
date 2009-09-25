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

#include "GUIWindow.h"
#include "utils/CriticalSection.h"
#include "GUIDialogSlider.h"

class CGUITextLayout; // forward

class CGUIWindowFullScreen :
      public CGUIWindow, public ISliderCallback
{
public:
  CGUIWindowFullScreen(void);
  virtual ~CGUIWindowFullScreen(void);
  virtual void AllocResources(bool forceLoad = false);
  virtual void FreeResources(bool forceUnLoad = false);
  virtual bool OnMessage(CGUIMessage& message);
  virtual bool OnAction(const CAction &action);
  virtual bool OnMouse(const CPoint &point);
  virtual void Render();
  virtual void OnWindowLoaded();
  void RenderFullScreen();
  bool NeedRenderFullScreen();
  void ChangetheTimeCode(int remote);

  virtual void OnSliderChange(void *data, CGUISliderControl *slider);
protected:
  virtual void OnDeinitWindow(int nextWindow) {}; // no out window animation for fullscreen video

private:
  void RenderTTFSubtitles();
  void Seek(bool bPlus, bool bLargeStep);
  void SeekChapter(int iChapter);
  void PreloadDialog(unsigned int windowID);
  void UnloadDialog(unsigned int windowID);

  bool m_bShowViewModeInfo;
  DWORD m_dwShowViewModeTimeout;

  bool m_bShowCurrentTime;
  bool m_bLastRender;

  bool m_timeCodeShow;
  DWORD m_timeCodeTimeout;
  int m_timeCodeStamp[5];
  int m_timeCodePosition;

  CCriticalSection m_fontLock;
  CGUITextLayout* m_subsLayout;
};
