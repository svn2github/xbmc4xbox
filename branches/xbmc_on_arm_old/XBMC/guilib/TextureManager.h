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

/*!
\file TextureManager.h
\brief
*/

#ifndef GUILIB_TEXTUREMANAGER_H
#define GUILIB_TEXTUREMANAGER_H

#include "TextureBundle.h"
#include <vector>

#pragma once

#ifdef HAS_SDL_OPENGL

#if !defined(HAS_SDL_GLES2) && !defined(HAS_SDL_GLES1)
#include <GL/glew.h>
#else
#include "gl2es.h"
#endif

class CGLTexture
{
public:
  CGLTexture(SDL_Surface* surface, bool loadToGPU = true, bool freeSurface = false);
  ~CGLTexture();

  void LoadToGPU();
  void Update(int w, int h, int pitch, const unsigned char *pixels, bool loadToGPU);
  void Update(SDL_Surface *surface, bool loadToGPU, bool freeSurface);

  int imageWidth;
  int imageHeight;
  int textureWidth;
  int textureHeight;
  GLuint id;
private:
  unsigned char* m_pixels;
  bool m_loadedToGPU;
};
#endif

// currently just used as a transport from texture manager to rest of app
class CTexture
{
public:
  CTexture()
  {
    Reset();
  };
  void Reset()
  {
    m_textures.clear();
    m_delays.clear();
    m_palette = NULL;
    m_width = 0;
    m_height = 0;
    m_loops = 0;
    m_texWidth = 0;
    m_texHeight = 0;
    m_texCoordsArePixels = false;
  };
  CTexture(int width, int height, int loops,  XBMC::PalettePtr palette = NULL, bool texCoordsArePixels = false);
#ifdef HAS_SDL_OPENGL
  void Add(CGLTexture *texture, int delay);
  void Set(CGLTexture *texture, int width, int height);
#else
  void Add(XBMC::TexturePtr texture, int delay);
  void Set(XBMC::TexturePtr texture, int width, int height);
#endif
  void Free();
  unsigned int size() const;

#ifdef HAS_SDL_OPENGL
  std::vector<CGLTexture*> m_textures;
#else
  std::vector<XBMC::TexturePtr> m_textures;
#endif
   XBMC::PalettePtr m_palette;
  std::vector<int> m_delays;
  int m_width;
  int m_height;
  int m_loops;
  int m_texWidth;
  int m_texHeight;
  bool m_texCoordsArePixels;
};

/*!
 \ingroup textures
 \brief
 */
class CTextureMap
{
public:
  CTextureMap();
  virtual ~CTextureMap();

  CTextureMap(const CStdString& textureName, int width, int height, int loops,  XBMC::PalettePtr palette);
  void Add(XBMC::TexturePtr pTexture, int delay);
  bool Release();

  const CStdString& GetName() const;
  const CTexture &GetTexture();
  void Dump() const;
  DWORD GetMemoryUsage() const;
  void Flush();
  bool IsEmpty() const;
protected:
  void FreeTexture();

  CStdString m_textureName;
  CTexture m_texture;
  unsigned int m_referenceCount;
  DWORD m_memUsage;
};

/*!
 \ingroup textures
 \brief
 */
class CGUITextureManager
{
public:
  CGUITextureManager(void);
  virtual ~CGUITextureManager(void);

  bool HasTexture(const CStdString &textureName, CStdString *path = NULL, int *bundle = NULL, int *size = NULL);
  bool CanLoad(const CStdString &texturePath) const; ///< Returns true if the texture manager can load this texture
  int Load(const CStdString& strTextureName, bool checkBundleOnly = false);
  const CTexture &GetTexture(const CStdString& strTextureName);
  void ReleaseTexture(const CStdString& strTextureName);
  void Cleanup();
  void Dump() const;
  DWORD GetMemoryUsage() const;
  void Flush();
  CStdString GetTexturePath(const CStdString& textureName, bool directory = false);
  void GetBundledTexturesFromPath(const CStdString& texturePath, std::vector<CStdString> &items);

  void AddTexturePath(const CStdString &texturePath);    ///< Add a new path to the paths to check when loading media
  void SetTexturePath(const CStdString &texturePath);    ///< Set a single path as the path to check when loading media (clear then add)
  void RemoveTexturePath(const CStdString &texturePath); ///< Remove a path from the paths to check when loading media

protected:
  std::vector<CTextureMap*> m_vecTextures;
  typedef std::vector<CTextureMap*>::iterator ivecTextures;
  // we have 2 texture bundles (one for the base textures, one for the theme)
  CTextureBundle m_TexBundle[2];

  std::vector<CStdString> m_texturePaths;
};

/*!
 \ingroup textures
 \brief
 */
extern CGUITextureManager g_TextureManager;
#endif
