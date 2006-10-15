/*!
\file GUIConsoleControl.h
\brief 
*/

#ifndef GUILIB_GUIConsoleControl_H
#define GUILIB_GUIConsoleControl_H

#pragma once

#include "GUIControl.h"

/*!
 \ingroup controls
 \brief 
 */
class CGUIConsoleControl : public CGUIControl
{
public:

  CGUIConsoleControl(DWORD dwParentID, DWORD dwControlId,
                     int iPosX, int iPosY, DWORD dwWidth, DWORD dwHeight,
                     const CLabelInfo &labelInfo,
                     D3DCOLOR dwPenColor1, D3DCOLOR dwPenColor2, D3DCOLOR dwPenColor3, D3DCOLOR dwPenColor4);

  virtual ~CGUIConsoleControl(void);

  virtual void Render();

  virtual void PreAllocResources();
  virtual void AllocResources();
  virtual void FreeResources();

  DWORD GetPenColor(INT nPaletteIndex)
  {
    return nPaletteIndex < (INT)m_palette.size() ? m_palette[nPaletteIndex] : 0xFF808080;
  };

  void Clear();
  void Write(CStdString& aString, INT nPaletteIndex = 0);
  const CLabelInfo& GetLabelInfo() const { return m_label; };

protected:

  void AddLine(CStdString& aString, DWORD aColour);
  void WriteString(CStdString& aString, DWORD aColour);

protected:

  struct Line
  {
    CStdString text;
    DWORD colour;
  };

  typedef vector<Line> LINEVECTOR;
  LINEVECTOR m_lines;
  LINEVECTOR m_queuedLines;

  typedef vector<D3DCOLOR> PALETTE;
  PALETTE m_palette;

  INT m_nMaxLines;
  DWORD m_dwLineCounter;
  DWORD m_dwFrameCounter;

  FLOAT m_fFontHeight;
  CLabelInfo m_label;
};
#endif
