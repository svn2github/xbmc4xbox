/*!
\file guiImage.h
\brief 
*/

#ifndef GUILIB_GUIIMAGECONTROL_H
#define GUILIB_GUIIMAGECONTROL_H

#pragma once

#include "GUIControl.h"

/*!
 \ingroup controls
 \brief 
 */
class CGUIImage : public CGUIControl
{
public:
  enum GUIIMAGE_ASPECT_RATIO { ASPECT_RATIO_STRETCH = 0, ASPECT_RATIO_SCALE, ASPECT_RATIO_KEEP };

  CGUIImage(DWORD dwParentID, DWORD dwControlId, int iPosX, int iPosY, DWORD dwWidth, DWORD dwHeight, const CStdString& strTexture, DWORD dwColorKey = 0);
  CGUIImage(const CGUIImage &left);
  virtual ~CGUIImage(void);

  virtual void Render();
  virtual void Render(int iPosX, int iPosY, DWORD dwWidth, DWORD dwHeight);
  virtual bool OnAction(const CAction &action) ;
  virtual bool OnMessage(CGUIMessage& message);
  virtual void PreAllocResources();
  virtual void AllocResources();
  virtual void FreeResources();
  virtual void DynamicResourceAlloc(bool bOnOff);
  virtual bool IsDynamicallyAllocated() { return m_bDynamicResourceAlloc; };
  virtual bool CanFocus() const;
  virtual bool IsAllocated() const;

  void Select(int iBitmap);
  void SetItems(int iItems);
  void SetTextureWidth(int iWidth);
  void SetTextureHeight(int iHeight);
  int GetTextureWidth() const;
  int GetTextureHeight() const;
  void GetBottomRight(float &x, float &y) const;

  void SetFileName(const CStdString& strFileName);
  const CStdString& GetFileName() const { return m_strFileName;};
  DWORD GetColorKey() const { return m_dwColorKey;};
  void SetAspectRatio(GUIIMAGE_ASPECT_RATIO ratio);
  GUIIMAGE_ASPECT_RATIO GetAspectRatio() const;
  int GetRenderWidth() const;
  int GetRenderHeight() const;
  void SetCornerAlpha(DWORD dwLeftTop, DWORD dwRightTop, DWORD dwLeftBottom, DWORD dwRightBottom);

  void SetInfo(int info) { m_Info = info; };
  int GetInfo() const { return m_Info; };

  void CalculateSize();
protected:
  void FreeTextures();
  void Process();
  static const DWORD FVF_VERTEX = D3DFVF_XYZRHW | D3DFVF_DIFFUSE | D3DFVF_TEX0;

  DWORD m_dwColorKey;
  CStdString m_strFileName;
  CStdString m_textureFileName;
  int m_iTextureWidth;
  int m_iTextureHeight;
  int m_iImageWidth;
  int m_iImageHeight;
  int m_iBitmap;
  DWORD m_dwItems;
  int m_iCurrentLoop;
  int m_iCurrentImage;
  DWORD m_dwFrameCounter;
  GUIIMAGE_ASPECT_RATIO m_aspectRatio;
  vector <LPDIRECT3DTEXTURE8> m_vecTextures;
  LPDIRECT3DPALETTE8 m_pPalette;
  int m_iRenderWidth;
  int m_iRenderHeight;
  bool m_bWasVisible;
  DWORD m_cornerAlpha[4];
  bool m_bDynamicResourceAlloc;

  // for when we are changing textures
  bool m_texturesAllocated;

  //vertex values
  float m_fX;
  float m_fY;
  float m_fUOffs;
  float m_fU;
  float m_fV;
  float m_fNW;
  float m_fNH;
  bool m_linearTexture; // true if it's a linear 32bit texture

  // conditional info
  int m_Info;
};
#endif
