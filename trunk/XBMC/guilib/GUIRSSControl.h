/*!
\file GUIRSSControl.h
\brief 
*/

#ifndef GUILIB_GUIRSSControl_H
#define GUILIB_GUIRSSControl_H

#pragma once

#include "GUIControl.h"
#include "..\XBMC\Utils\RssReader.h"

/*!
 \ingroup controls
 \brief 
 */
class CGUIRSSControl : public CGUIControl, public IRssObserver
{
public:
  CGUIRSSControl(DWORD dwParentID, DWORD dwControlId, int iPosX, int iPosY, DWORD dwWidth, DWORD dwHeight, const CStdString& strFontName, D3DCOLOR dwChannelColor, D3DCOLOR dwHeadlineColor, D3DCOLOR dwNormalColor, CStdString& strUrl, CStdString& strRSSTags);
  virtual ~CGUIRSSControl(void);

  virtual void Render();
  virtual void OnFeedUpdate(CStdString& aFeed, LPBYTE aColorArray);

  DWORD GetChannelTextColor() const { return m_dwChannelColor;};
  DWORD GetHeadlineTextColor() const { return m_dwHeadlineColor;};
  DWORD GetNormalTextColor() const { return m_dwTextColor;};
  const char *GetFontName() const { return m_pFont ? m_pFont->GetFontName().c_str() : ""; };
  const CStdString& GetUrl() const { return m_strUrl; };
  const CStdString& GetTags() const { return m_strRSSTags; };

protected:

  void RenderText();

  CRssReader* m_pReader;
  WCHAR* m_pwzText;
  WCHAR* m_pwzBuffer;
  LPBYTE m_pbColors;
  LPBYTE m_pbBuffer;
  LPDWORD m_pdwPalette;

  CStdString m_strUrl;
  CStdString m_strRSSTags;
  D3DCOLOR m_dwChannelColor;
  D3DCOLOR m_dwHeadlineColor;
  D3DCOLOR m_dwTextColor;
  CGUIFont* m_pFont;

  int m_iCharIndex;
  int m_iStartFrame;
  int m_iScrollX;
  int m_iLeadingSpaces;
  int m_iTextLenght;
  float m_fTextHeight;
  float m_fTextWidth;
};
#endif
