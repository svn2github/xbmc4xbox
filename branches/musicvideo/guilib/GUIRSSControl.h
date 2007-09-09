/*!
\file GUIRSSControl.h
\brief 
*/

#ifndef GUILIB_GUIRSSControl_H
#define GUILIB_GUIRSSControl_H

#pragma once

#include "GUIControl.h"
#include "../xbmc/utils/RssReader.h"

/*!
\ingroup controls
\brief 
*/
class CGUIRSSControl : public CGUIControl, public IRssObserver
{
public:
  CGUIRSSControl(DWORD dwParentID, DWORD dwControlId, float posX, float posY, float width, float height, const CLabelInfo& labelInfo, D3DCOLOR dwChannelColor, D3DCOLOR dwHeadlineColor, CStdString& strRSSTags);
  virtual ~CGUIRSSControl(void);

  virtual void Render();
  virtual void OnFeedUpdate(CStdStringW& aFeed, LPBYTE aColorArray);
  virtual bool CanFocus() const { return false; };

  void SetIntervals(const std::vector<int>& vecIntervals);
  void SetUrls(const std::vector<string>& vecUrl);

protected:

  void RenderText();

  CCriticalSection m_criticalSection;

  CRssReader* m_pReader;
  WCHAR* m_pwzText;
  LPBYTE m_pbColors;

  CStdString m_strRSSTags;
  D3DCOLOR m_dwChannelColor;
  D3DCOLOR m_dwHeadlineColor;

  CLabelInfo m_label;

  std::vector<string> m_vecUrls;
  std::vector<int> m_vecIntervals;
  CScrollInfo m_scrollInfo;
};
#endif
