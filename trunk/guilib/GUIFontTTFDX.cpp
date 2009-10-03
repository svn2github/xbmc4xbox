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

#include "GUIFont.h"
#include "GUIFontTTFDX.h"
#include "GUIFontManager.h"
#include "Texture.h"
#include "gui3d.h"
#include "WindowingFactory.h"

// stuff for freetype
#include "ft2build.h"

#include FT_FREETYPE_H
#include FT_GLYPH_H
#include FT_OUTLINE_H

using namespace std;

#ifdef HAS_DX

struct CUSTOMVERTEX 
{
  FLOAT x, y, z;
  DWORD color;
  FLOAT tu, tv;   // Texture coordinates
};


CGUIFontTTFDX::CGUIFontTTFDX(const CStdString& strFileName)
: CGUIFontTTFBase(strFileName)
{
 
}

CGUIFontTTFDX::~CGUIFontTTFDX(void)
{
  
}

void CGUIFontTTFDX::RenderInternal(SVertex* v)
{
  CUSTOMVERTEX verts[4] =  {
  { v[0].x-0.5f, v[0].y-0.5f, v[0].z, m_color, v[0].u, v[0].v},
  { v[1].x-0.5f, v[1].y-0.5f, v[1].z, m_color, v[1].u, v[1].v},
  { v[2].x-0.5f, v[2].y-0.5f, v[2].z, m_color, v[2].u, v[2].v},
  { v[3].x-0.5f, v[3].y-0.5f, v[3].z, m_color, v[3].u, v[3].v}
  };

  g_Windowing.Get3DDevice()->DrawPrimitiveUP(D3DPT_TRIANGLEFAN, 2, verts, sizeof(CUSTOMVERTEX));
}

void CGUIFontTTFDX::Begin()
{
  LPDIRECT3DDEVICE9 pD3DDevice = g_Windowing.Get3DDevice();

  if (m_nestedBeginCount == 0)
  {
    // just have to blit from our texture.
    pD3DDevice->SetTexture( 0, m_texture->GetTextureObject() );

    pD3DDevice->SetSamplerState( 0, D3DSAMP_ADDRESSU, D3DTADDRESS_CLAMP );
    pD3DDevice->SetSamplerState( 0, D3DSAMP_ADDRESSV, D3DTADDRESS_CLAMP );
    pD3DDevice->SetSamplerState( 0, D3DSAMP_MAGFILTER, D3DTEXF_LINEAR );
    pD3DDevice->SetSamplerState( 0, D3DSAMP_MINFILTER, D3DTEXF_LINEAR );
    pD3DDevice->SetTextureStageState( 0, D3DTSS_COLOROP, D3DTOP_SELECTARG1 ); // only use diffuse
    pD3DDevice->SetTextureStageState( 0, D3DTSS_COLORARG1, D3DTA_DIFFUSE);
    pD3DDevice->SetTextureStageState( 0, D3DTSS_ALPHAOP, D3DTOP_MODULATE );
    pD3DDevice->SetTextureStageState( 0, D3DTSS_ALPHAARG1, D3DTA_TEXTURE);
    pD3DDevice->SetTextureStageState( 0, D3DTSS_ALPHAARG2, D3DTA_DIFFUSE);

    // no other texture stages needed
    pD3DDevice->SetTextureStageState( 1, D3DTSS_COLOROP, D3DTOP_DISABLE);

    pD3DDevice->SetRenderState( D3DRS_ZENABLE, FALSE );
    pD3DDevice->SetRenderState( D3DRS_FOGENABLE, FALSE );
    pD3DDevice->SetRenderState( D3DRS_FILLMODE, D3DFILL_SOLID );
    pD3DDevice->SetRenderState( D3DRS_CULLMODE, D3DCULL_NONE );
    pD3DDevice->SetRenderState( D3DRS_ALPHABLENDENABLE, TRUE );
    pD3DDevice->SetRenderState( D3DRS_SRCBLEND, D3DBLEND_SRCALPHA );
    pD3DDevice->SetRenderState( D3DRS_DESTBLEND, D3DBLEND_INVSRCALPHA );
    pD3DDevice->SetRenderState( D3DRS_LIGHTING, FALSE);

    pD3DDevice->SetFVF(D3DFVF_XYZ | D3DFVF_DIFFUSE | D3DFVF_TEX1);
  }
  // Keep track of the nested begin/end calls.
  m_vertex_count = 0;
  m_nestedBeginCount++;
}

void CGUIFontTTFDX::End()
{
  LPDIRECT3DDEVICE9 pD3DDevice = g_Windowing.Get3DDevice();

  if (m_nestedBeginCount == 0)
    return;

  if (--m_nestedBeginCount > 0)
    return;

  pD3DDevice->SetTexture(0, NULL);
  pD3DDevice->SetTextureStageState( 0, D3DTSS_COLOROP, D3DTOP_MODULATE );
}

CBaseTexture* CGUIFontTTFDX::ReallocTexture(unsigned int& newHeight)
{
  CBaseTexture* pNewTexture = new CTexture(m_textureWidth, newHeight, XB_FMT_A8);

  if (pNewTexture == NULL)
    return NULL;

  pNewTexture->CreateTextureObject();

  LPDIRECT3DTEXTURE9 newTexture = pNewTexture->GetTextureObject();

  if (newTexture == NULL)
    return NULL;

  // correct texture sizes
  D3DSURFACE_DESC desc;
  newTexture->GetLevelDesc(0, &desc);
  m_textureHeight = desc.Height;
  m_textureWidth = desc.Width;

  D3DLOCKED_RECT rect;
  newTexture->LockRect(0, &rect, NULL, 0);
  memset(rect.pBits, 0, rect.Pitch * m_textureHeight);
  newTexture->UnlockRect(0);

  if (m_texture)
  { // copy across from our current one using gpu
    LPDIRECT3DSURFACE9 pTarget, pSource;
    newTexture->GetSurfaceLevel(0, &pTarget);
    m_texture->GetTextureObject()->GetSurfaceLevel(0, &pSource);

    // TODO:DIRECTX - this is probably really slow, but UpdateSurface() doesn't like rendering from non-system textures
    D3DXLoadSurfaceFromSurface(pTarget, NULL, NULL, pSource, NULL, NULL, D3DX_FILTER_NONE, 0);

    SAFE_RELEASE(pTarget);
    SAFE_RELEASE(pSource);
    delete m_texture;
  }

  return pNewTexture;
}

bool CGUIFontTTFDX::CopyCharToTexture(FT_BitmapGlyph bitGlyph, Character* ch)
{
  FT_Bitmap bitmap = bitGlyph->bitmap;

  // render this onto our normal texture using gpu
  LPDIRECT3DSURFACE9 target;
  m_texture->GetTextureObject()->GetSurfaceLevel(0, &target);

  RECT sourcerect = { 0, 0, bitmap.width, bitmap.rows };
  RECT targetrect;
  targetrect.top = m_posY + ch->offsetY;
  targetrect.left = m_posX + bitGlyph->left;
  targetrect.bottom = targetrect.top + bitmap.rows;
  targetrect.right = targetrect.left + bitmap.width;

  D3DXLoadSurfaceFromMemory( target, NULL, &targetrect,
    bitmap.buffer, D3DFMT_LIN_A8, bitmap.pitch, NULL, &sourcerect,
    D3DX_FILTER_NONE, 0x00000000);

  SAFE_RELEASE(target);

  return TRUE;
}


void CGUIFontTTFDX::DeleteHardwareTexture()
{
  
}


#endif
