/*!
\file GUIListControlEx.h
\brief 
*/

#ifndef GUILIB_GUIListControlEx_H
#define GUILIB_GUIListControlEx_H

#pragma once
#include "GUISpinControl.h"
#include "GUIButtonControl.h"
#include "GUIListExItem.h"
#include "GUIList.h"

/*!
 \ingroup controls
 \brief 
 */
class CGUIListControlEx : public CGUIControl
{
public:
  CGUIListControlEx(DWORD dwParentID, DWORD dwControlId, int iPosX, int iPosY, DWORD dwWidth, DWORD dwHeight,
                    const CStdString& strFontName,
                    DWORD dwSpinWidth, DWORD dwSpinHeight,
                    const CStdString& strUp, const CStdString& strDown,
                    const CStdString& strUpFocus, const CStdString& strDownFocus,
                    DWORD dwSpinColor, DWORD dwSpinX, DWORD dwSpinY,
                    const CStdString& strFont, DWORD dwTextColor, DWORD dwSelectedColor,
                    const CStdString& strButton, const CStdString& strButtonFocus,
                    DWORD dwItemTextOffsetX, DWORD dwItemTextOffsetY, DWORD dwTextAlign);
  virtual ~CGUIListControlEx(void);
  virtual void Render();
  virtual bool OnAction(const CAction &action);
  virtual void OnMouseOver();
  virtual void OnMouseClick(DWORD dwButton);
  virtual bool OnMessage(CGUIMessage& message);

  virtual bool CanFocus() const;

  virtual void PreAllocResources();
  virtual void AllocResources() ;
  virtual void FreeResources() ;
  virtual void DynamicResourceAlloc(bool bOnOff);
  virtual void SetPosition(int iPosX, int iPosY);
  virtual void SetWidth(int iWidth);
  virtual void SetHeight(int iHeight);
  void SetScrollySuffix(const CStdString& wstrSuffix);
  void SetImageDimensions(int iWidth, int iHeight);
  void SetItemHeight(int iHeight);
  void SetSpace(int iHeight);
  void SetFont2(const CStdString& strFont);
  void SetColors2(DWORD dwTextColor, DWORD dwSelectedColor);
  void SetPageControlVisible(bool bVisible);
  int GetSelectedItem(CStdString& strLabel);
  DWORD GetTextColor() const { return m_dwTextColor;};
  DWORD GetTextColor2() const { return m_dwTextColor2;};
  DWORD GetSelectedColor() const { return m_dwSelectedColor;};
  DWORD GetSelectedColor2() const { return m_dwSelectedColor2;};
  const char* GetFontName() const { return m_pFont ? m_pFont->GetFontName().c_str() : ""; };
  const char* GetFontName2() const { if (!m_pFont2) return ""; else return m_pFont2->GetFontName().c_str(); };
DWORD GetSpinWidth() const { return m_upDown.GetWidth() / 2; };
  DWORD GetSpinHeight() const { return m_upDown.GetHeight(); };
  const CStdString& GetTextureUpName() const { return m_upDown.GetTextureUpName(); };
  const CStdString& GetTextureDownName() const { return m_upDown.GetTextureDownName(); };
  const CStdString& GetTextureUpFocusName() const { return m_upDown.GetTextureUpFocusName(); };
  const CStdString& GetTextureDownFocusName() const { return m_upDown.GetTextureDownFocusName(); };
  DWORD GetSpinTextColor() const { return m_upDown.GetTextColor();};
  int GetSpinX() const { return m_upDown.GetXPosition();};
  int GetSpinY() const { return m_upDown.GetYPosition();};
  DWORD GetSpace() const { return m_iSpaceBetweenItems;};
  DWORD GetItemHeight() const { return m_iItemHeight; };
  DWORD GetImageWidth() const { return m_iImageWidth;};
  DWORD GetImageHeight() const { return m_iImageHeight;};
  DWORD GetTextOffsetX() const { return m_dwTextOffsetX;};
  DWORD GetTextOffsetY() const { return m_dwTextOffsetY;};
  DWORD GetAlignmentY() const { return m_imgButton.GetTextAlign() & 0x00000004; };

  const wstring& GetSuffix() const { return m_strSuffix;};
  const CStdString GetButtonFocusName() const { return m_imgButton.GetTextureFocusName();};
  const CStdString GetButtonNoFocusName() const { return m_imgButton.GetTextureNoFocusName();};
protected:

  virtual void OnRight();
  virtual void OnLeft();
  virtual void OnDown();
  virtual void OnUp();
  void OnPageUp();
  void OnPageDown();
  int m_iSpaceBetweenItems;
  int m_iOffset;
  float m_fSmoothScrollOffset;
  int m_iItemsPerPage;
  int m_iItemHeight;
  int m_iSelect;
  int m_iCursorY;
  int m_iImageWidth;
  int m_iImageHeight;
  bool m_bUpDownVisible;
  DWORD m_dwTextColor, m_dwTextColor2;
  DWORD m_dwSelectedColor, m_dwSelectedColor2;
  DWORD m_dwTextOffsetX;
  DWORD m_dwTextOffsetY;

  CGUIFont* m_pFont;
  CGUIFont* m_pFont2;
  CGUISpinControl m_upDown;
  CGUIButtonControl m_imgButton;
  wstring m_strSuffix;
  CGUIList* m_pList;
};
#endif
