/*!
\file guiImage.h
\brief 
*/

#ifndef GUILIB_GUIIMAGECONTROL_H
#define GUILIB_GUIIMAGECONTROL_H

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
#include "TextureManager.h"
#include "GUILabelControl.h"  // for CLabelInfo

struct FRECT
{
  float left;
  float top;
  float right;
  float bottom;
};

// image alignment for <aspect>keep</aspect>, <aspect>scale</aspect> or <aspect>center</aspect>
#define ASPECT_ALIGN_CENTER  0
#define ASPECT_ALIGN_LEFT    1
#define ASPECT_ALIGN_RIGHT   2
#define ASPECT_ALIGNY_CENTER 0
#define ASPECT_ALIGNY_TOP    4
#define ASPECT_ALIGNY_BOTTOM 8
#define ASPECT_ALIGN_MASK    3
#define ASPECT_ALIGNY_MASK  ~3

class CImage
{
public:
  CImage(const CStdString &fileName) : file(fileName, "")
  {
    memset(&border, 0, sizeof(FRECT));
    orientation = 0;
  };

  CImage()
  {
    memset(&border, 0, sizeof(FRECT));
    orientation = 0;
  };

  void operator=(const CImage &left)
  {
    file = left.file;
    memcpy(&border, &left.border, sizeof(FRECT));
    orientation = left.orientation;
    diffuse = left.diffuse;
  };
  CGUIInfoLabel file;
  FRECT      border;  // scaled  - unneeded if we get rid of scale on load
  int        orientation; // orientation of the texture (0 - 7 == EXIForientation - 1)
  CStdString diffuse; // diffuse overlay texture (unimplemented)
};

#ifdef HAS_SDL
class CCachedTexture
{
  public:
    CCachedTexture() { surface = NULL; width = height = 0; diffuseColor = 0xffffffff; };
    
    SDL_Surface *surface;
    int          width;
    int          height;
    DWORD        diffuseColor;
};
#endif
/*!
 \ingroup controls
 \brief 
 */

class CGUIImage : public CGUIControl
{
public:
  class CAspectRatio
  {
  public:
    enum ASPECT_RATIO { AR_STRETCH = 0, AR_SCALE, AR_KEEP, AR_CENTER };
    CAspectRatio(ASPECT_RATIO aspect = AR_STRETCH)
    {
      ratio = aspect;
      align = ASPECT_ALIGN_CENTER | ASPECT_ALIGNY_CENTER;
      scaleDiffuse = true;
    };
    bool operator!=(const CAspectRatio &right) const
    {
      if (ratio != right.ratio) return true;
      if (align != right.align) return true;
      if (scaleDiffuse != right.scaleDiffuse) return true;
      return false;
    };

    ASPECT_RATIO ratio;
    DWORD        align;
    bool         scaleDiffuse;
  };

  CGUIImage(DWORD dwParentID, DWORD dwControlId, float posX, float posY, float width, float height, const CImage& texture, DWORD dwColorKey = 0);
  CGUIImage(const CGUIImage &left);
  virtual ~CGUIImage(void);
  virtual CGUIImage *Clone() const { return new CGUIImage(*this); };

  virtual void Render();
  virtual void UpdateVisibility(const CGUIListItem *item = NULL);
  virtual bool OnAction(const CAction &action) ;
  virtual bool OnMessage(CGUIMessage& message);
  virtual void PreAllocResources();
  virtual void AllocResources();
  virtual void FreeResources();
  virtual void DynamicResourceAlloc(bool bOnOff);
  virtual bool IsDynamicallyAllocated() { return m_bDynamicResourceAlloc; };
  virtual bool CanFocus() const;
  virtual bool IsAllocated() const;
  virtual void UpdateInfo(const CGUIListItem *item = NULL);

  void PythonSetColorKey(DWORD dwColorKey);
  virtual void SetFileName(const CStdString& strFileName, bool setConstant = false);
  virtual void SetAspectRatio(const CAspectRatio &aspect);
  void SetAspectRatio(CAspectRatio::ASPECT_RATIO ratio) { CAspectRatio aspect(ratio); SetAspectRatio(aspect); };
  void SetAlpha(unsigned char alpha);
  void SetAlpha(unsigned char a0, unsigned char a1, unsigned char a2, unsigned char a3);
  virtual void SetWidth(float width);
  virtual void SetHeight(float height);

  const CStdString& GetFileName() const { return m_strFileName;};
  int GetTextureWidth() const;
  int GetTextureHeight() const;

  void CalculateSize();
#ifdef _DEBUG
  virtual void DumpTextureUse();
#endif
protected:
  void LoadDiffuseImage();
  virtual void AllocateOnDemand();
  virtual void FreeTextures();
  void Process();
  void Render(float left, float top, float bottom, float right, float u1, float v1, float u2, float v2);
  virtual int GetOrientation() const { return m_image.orientation; };
  void OrientateTexture(CRect &rect, int orientation);

  DWORD m_dwColorKey;
  unsigned char m_alpha[4];
  CStdString m_strFileName;
  int m_iTextureWidth;
  int m_iTextureHeight;
  int m_iImageWidth;
  int m_iImageHeight;
  int m_iCurrentLoop;
  int m_iCurrentImage;
  DWORD m_dwFrameCounter;
  CAspectRatio m_aspect;
#ifndef HAS_SDL
  std::vector <LPDIRECT3DTEXTURE8> m_vecTextures;
  LPDIRECT3DTEXTURE8 m_diffuseTexture;
  LPDIRECT3DPALETTE8 m_diffusePalette;
  LPDIRECT3DPALETTE8 m_pPalette;
  float m_diffuseScaleU, m_diffuseScaleV;
#elif defined(HAS_SDL_2D)
  void CalcBoundingBox(float *x, float *y, int n, int *b);
  void GetTexel(float u, float v, SDL_Surface *src, BYTE *texel);
  void RenderWithEffects(SDL_Surface *src, float *x, float *y, float *u, float *v, DWORD *c, SDL_Surface *diffuse, float diffuseScaleU, float diffuseScaleV, CCachedTexture &dst);
  std::vector <SDL_Surface*> m_vecTextures;
  std::vector <CCachedTexture> m_vecCachedTextures;
  SDL_Surface* m_diffuseTexture;
  SDL_Palette* m_diffusePalette;
  SDL_Palette* m_pPalette;
#elif defined(HAS_SDL_OPENGL)
  CGLTexture* m_diffuseTexture;
  SDL_Palette* m_diffusePalette;
  SDL_Palette* m_pPalette;
  std::vector <CGLTexture*> m_vecTextures;
#endif  
  float m_diffuseScaleU, m_diffuseScaleV;
  CPoint m_diffuseOffset;
  bool m_bDynamicResourceAlloc;

  // for when we are changing textures
  bool m_texturesAllocated;

  //vertex values
  float m_fX;
  float m_fY;
  float m_fU;
  float m_fV;
  float m_fNW;
  float m_fNH;
  bool m_linearTexture; // true if it's a linear 32bit texture

  // border + conditional info
  CImage m_image;
};
#endif
