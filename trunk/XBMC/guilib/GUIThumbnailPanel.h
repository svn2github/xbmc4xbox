/*!
	\file GUIThumbnailPanel.h
	\brief 
	*/

#ifndef GUILIB_GUITHUMBNAILCONTROL_H
#define GUILIB_GUITHUMBNAILCONTROL_H

#pragma once
#include "gui3d.h"
#include "guicontrol.h"
#include "guimessage.h"
#include "guifont.h"
#include "guiimage.h"
#include "guispincontrol.h"
#include "guiButtonControl.h"
#include "guiListItem.h"
#include <vector>
#include "stdstring.h"
using namespace std;

/*!
	\ingroup controls
	\brief 
	*/
class CGUIThumbnailPanel :
  public CGUIControl
{
public:
  CGUIThumbnailPanel(DWORD dwParentID, DWORD dwControlId, int iPosX, int iPosY, DWORD dwWidth, DWORD dwHeight, 
                    const CStdString& strFontName, 
                    const CStdString& strImageIcon,
                    const CStdString& strImageIconFocus,
                    DWORD dwitemWidth, DWORD dwitemHeight,
                    DWORD dwSpinWidth,DWORD dwSpinHeight,
                    const CStdString& strUp, const CStdString& strDown, 
                    const CStdString& strUpFocus, const CStdString& strDownFocus, 
                    DWORD dwSpinColor,DWORD dwSpinX, DWORD dwSpinY,
                    const CStdString& strFont, DWORD dwTextColor,DWORD dwSelectedColor);
  virtual ~CGUIThumbnailPanel(void);
  virtual void Render();
  virtual void OnAction(const CAction &action) ;
  virtual bool OnMessage(CGUIMessage& message);

	virtual void PreAllocResources();
  virtual void AllocResources() ;
  virtual void FreeResources() ;
  void         SetScrollySuffix(CStdString wstrSuffix);
	void				 SetTextureDimensions(int iWidth, int iHeight);
  void         SetThumbDimensions(int iXpos, int iYpos,int iWidth, int iHeight);
  void         GetThumbDimensions(int& iXpos, int& iYpos,int& iWidth, int& iHeight);
  void         GetThumbDimensionsBig(int& iXpos, int& iYpos,int& iWidth, int& iHeight);
  void         GetThumbDimensionsLow(int& iXpos, int& iYpos,int& iWidth, int& iHeight);
  int                   GetSelectedItem(CStdString& strLabel);
	DWORD									GetTextColor() const { return m_dwTextColor;};
	DWORD									GetSelectedColor() const { return m_dwSelectedColor;};
	const char*						GetFontName() const { return m_pFont ? m_pFont->GetFontName().c_str() : ""; };
	DWORD									GetSpinWidth() const { return m_upDown.GetWidth()/2; };
	DWORD									GetSpinHeight() const { return m_upDown.GetHeight(); };
	const	CStdString&			GetTexutureUpName() const { return m_upDown.GetTexutureUpName(); };
	const	CStdString&			GetTexutureDownName() const { return m_upDown.GetTexutureDownName(); };
	const	CStdString&			GetTexutureUpFocusName() const { return m_upDown.GetTexutureUpFocusName(); };
	const	CStdString&			GetTexutureDownFocusName() const { return m_upDown.GetTexutureDownFocusName(); };
	DWORD									GetSpinTextColor() const { return m_upDown.GetTextColor();};
	int										GetSpinX() const { return m_upDown.GetXPosition();};
	int										GetSpinY() const { return m_upDown.GetYPosition();};
	DWORD									GetTextureWidth() const { return m_iTextureWidth;};
	DWORD									GetTextureHeight() const { return m_iTextureHeight;};
	const CStdString			GetFocusName() const { return m_imgFolderFocus.GetFileName();};
	const CStdString			GetNoFocusName() const { return m_imgFolder.GetFileName();};
	DWORD									GetItemWidth() const { return m_iItemWidth;};
	DWORD									GetItemHeight() const { return m_iItemHeight;};
	void                  SetItemWidth(DWORD dwWidth);
	void                  SetItemHeight(DWORD dwHeight);
  bool                  IsTextureShown() const { return m_bShowTexture;};
  void                  ShowTexture(bool bOnoff);
  void                  SetTextureWidthBig(int textureWidthBig) { m_iTextureWidthBig=textureWidthBig;};
  void                  SetTextureHeightBig(int textureHeightBig){ m_iTextureHeightBig=textureHeightBig;};
  void                  SetItemWidthBig(int itemWidthBig){ m_iItemWidthBig=itemWidthBig;};
  void                  SetItemHeightBig(int itemHeightBig) { m_iItemHeightBig=itemHeightBig;};
  
  void                  SetTextureWidthLow(int textureWidthLow) { m_iTextureWidthLow=textureWidthLow;};
  void                  SetTextureHeightLow(int textureHeightLow){ m_iTextureHeightLow=textureHeightLow;};
  void                  SetItemWidthLow(int itemWidthLow){ m_iItemWidthLow=itemWidthLow;};
  void                  SetItemHeightLow(int itemHeightLow) { m_iItemHeightLow=itemHeightLow;};

  int                   GetTextureWidthBig() { return m_iTextureWidthBig;};
  int                   GetTextureHeightBig(){ return m_iTextureHeightBig;};
  int                   GetItemWidthBig(){ return m_iItemWidthBig;};
  int                   GetItemHeightBig() { return m_iItemHeightBig;};
  
  int                   GetTextureWidthLow() { return m_iTextureWidthLow;};
  int                   GetTextureHeightLow(){ return m_iTextureHeightLow;};
  int                   GetItemWidthLow(){ return m_iItemWidthLow;};
  int                   GetItemHeightLow() { return m_iItemHeightLow;};
  void                  ShowBigIcons(bool bOnOff);
  const wstring&				GetSuffix() const { return m_strSuffix;};
  void                  SetThumbDimensionsLow(int iXpos, int iYpos,int iWidth, int iHeight) { m_iThumbXPosLow=iXpos;m_iThumbYPosLow=iYpos;m_iThumbWidthLow=iWidth;m_iThumbHeightLow=iHeight;};
  void                  SetThumbDimensionsBig(int iXpos, int iYpos,int iWidth, int iHeight) { m_iThumbXPosBig=iXpos;m_iThumbYPosBig=iYpos;m_iThumbWidthBig=iWidth;m_iThumbHeightBig=iHeight;};
  virtual void 					OnMouseOver();
  virtual void 					OnMouseClick(DWORD dwButton);
  virtual void 					OnMouseDoubleClick(DWORD dwButton);
  virtual void					OnMouseWheel();
  void							ScrollDown();
  void							ScrollUp();
  virtual bool					HitTest(int iPosX, int iPosY) const;
  bool							SelectItemFromPoint(int iPosX, int iPosY);
  void							GetOffsetFromPage();

protected:
  void                  Calculate();
	void				          RenderItem(bool bFocus,int iPosX, int iPosY, CGUIListItem* pItem);
  void                  RenderText(float fPosX, float fPosY, DWORD dwTextColor, WCHAR* wszText,bool bScroll );
  virtual void          OnRight();
  virtual void          OnLeft();
  virtual void          OnDown();
  virtual void          OnUp();
  void                  OnPageUp();
  void                  OnPageDown();
  bool                  ValidItem(int iX, int iY);
  
  int                   m_iItemHeightLow;
  int                   m_iItemWidthLow;
  int                   m_iTextureHeightLow;
  int                   m_iTextureWidthLow;

  int                   m_iItemHeightBig;
  int                   m_iItemWidthBig;
  int                   m_iTextureHeightBig;
  int                   m_iTextureWidthBig;

  bool                  m_bShowTexture;
  int                   m_iOffset;
  int                   m_iItemHeight;
  int                   m_iItemWidth;
  int                   m_iSelect;
	int										m_iCursorX;
	int										m_iCursorY;
  int                   m_iRows;
  int                   m_iColumns;
	bool									m_bScrollUp;
	bool									m_bScrollDown;
	int										m_iScrollCounter;
  wstring               m_strSuffix;
  DWORD                 m_dwTextColor;
  DWORD                 m_dwSelectedColor;
	int										m_iLastItem;
	int										m_iTextureWidth;
	int										m_iTextureHeight;
  int                   m_iThumbXPos;
  int                   m_iThumbYPos;
  int                   m_iThumbWidth;
  int                   m_iThumbHeight;
  
  int                   m_iThumbXPosLow;
  int                   m_iThumbYPosLow;
  int                   m_iThumbWidthLow;
  int                   m_iThumbHeightLow;
  
  int                   m_iThumbXPosBig;
  int                   m_iThumbYPosBig;
  int                   m_iThumbWidthBig;
  int                   m_iThumbHeightBig;
  CGUIFont*             m_pFont;
  CGUISpinControl       m_upDown;
  CGUIImage             m_imgFolder;
  CGUIImage             m_imgFolderFocus;
  vector<CGUIListItem*> m_vecItems;
  typedef vector<CGUIListItem*> ::iterator ivecItems;
};
#endif
