/*!
\file GUIRSSControl.h
\brief 
*/

#ifndef GUILIB_GUIRSSControl_H
#define GUILIB_GUIRSSControl_H

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

#include "GUIControl.h"
#include "utils/RssReader.h"

/*!
\ingroup controls
\brief 
*/
class CGUIRSSControl : public CGUIControl, public IRssObserver
{
public:
  CGUIRSSControl(DWORD dwParentID, DWORD dwControlId, float posX, float posY, float width, float height, const CLabelInfo& labelInfo, const CGUIInfoColor &channelColor, const CGUIInfoColor &headlineColor, CStdString& strRSSTags, int scrollSpeed);
  CGUIRSSControl(const CGUIRSSControl &from);
  virtual ~CGUIRSSControl(void);
  virtual CGUIRSSControl *Clone() const { return new CGUIRSSControl(*this); };

  virtual void Render();
  virtual void OnFeedUpdate(const std::vector<DWORD> &feed);
  virtual void OnFeedRelease();
  virtual bool CanFocus() const { return false; };

  void SetIntervals(const std::vector<int>& vecIntervals);
  void SetUrls(const std::vector<std::string>& vecUrl);

protected:
  virtual void UpdateColors();

  CCriticalSection m_criticalSection;

  CRssReader* m_pReader;
  std::vector<DWORD> m_feed;

  CStdString m_strRSSTags;

  CLabelInfo m_label;
  CGUIInfoColor m_channelColor;
  CGUIInfoColor m_headlineColor;

  std::vector<std::string> m_vecUrls;
  std::vector<int> m_vecIntervals;
  CScrollInfo m_scrollInfo;
};
#endif
