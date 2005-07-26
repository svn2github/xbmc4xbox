/*!
\file GUIFont.h
\brief 
*/

#ifndef CGUILIB_GUIFONT_H
#define CGUILIB_GUIFONT_H
#pragma once

#include "common/xbfont.h"

class CScrollInfo
{
public:
  CScrollInfo(unsigned int wait = 50) { initialWait = wait; Reset(); };
  void Reset()
  {
    waitTime = initialWait;
    pixelPos = characterPos = 0;
  }
  unsigned int waitTime;
  unsigned int pixelPos;
  unsigned int characterPos;
  unsigned int initialWait;
};

/*!
 \ingroup textures
 \brief 
 */
class CGUIFont
{
public:
  CGUIFont(const CStdString& strFontName);
  virtual ~CGUIFont();

  CStdString& GetFontName();

  void DrawShadowText( FLOAT fOriginX, FLOAT fOriginY, DWORD dwColor,
                       const WCHAR* strText, DWORD dwFlags = 0,
                       FLOAT fMaxPixelWidth = 0.0,
                       int iShadowWidth = 5,
                       int iShadowHeight = 5,
                       DWORD dwShadowColor = 0xff000000);

  void DrawTextWidth(FLOAT fOriginX, FLOAT fOriginY, DWORD dwColor,
                     const WCHAR* strText, float fMaxWidth);

  void DrawColourTextWidth(FLOAT fOriginX, FLOAT fOriginY, DWORD* pdw256ColorPalette,
                           const WCHAR* strText, BYTE* pbColours, float fMaxWidth);

  void DrawText( FLOAT sx, FLOAT sy, DWORD dwColor,
                 const WCHAR* strText, DWORD dwFlags = 0L,
                 FLOAT fMaxPixelWidth = 0.0f );
  void DrawScrollingText(float x, float y, DWORD* color, const CStdStringW &text, float w, CScrollInfo &scrollInfo, BYTE *pPalette = NULL);

  inline void GetTextExtent( const WCHAR* strText, FLOAT* pWidth, FLOAT* pHeight, BOOL bFirstLineOnly = FALSE);

  FLOAT GetTextWidth( const WCHAR* strText );
  virtual void Begin() {};
  virtual void End() {};

protected:
  virtual void GetTextExtentInternal( const WCHAR* strText, FLOAT* pWidth, FLOAT* pHeight, BOOL bFirstLineOnly = FALSE) = 0;

  void DrawTextWidthInternal(FLOAT fOriginX, FLOAT fOriginY, DWORD dwColor,
                             const WCHAR* strText, float fMaxWidth);

  virtual void DrawTextImpl(FLOAT fOriginX, FLOAT fOriginY, DWORD dwColor,
                            const WCHAR* strText, DWORD cchText, DWORD dwFlags = 0,
                            FLOAT fMaxPixelWidth = 0.0f) = 0;

  virtual void DrawColourTextImpl(FLOAT fOriginX, FLOAT fOriginY, DWORD* pdw256ColorPalette,
                                  const WCHAR* strText, BYTE* pbColours, DWORD cchText, DWORD dwFlags,
                                  FLOAT fMaxPixelWidth) = 0;

  float m_iMaxCharWidth;
  CStdString m_strFontName;
};

#endif
