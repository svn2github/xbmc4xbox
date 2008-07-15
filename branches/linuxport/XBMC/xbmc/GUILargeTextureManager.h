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

#include "utils/Thread.h"
#include "utils/CriticalSection.h"
#ifdef HAS_SDL
#include "SDL/SDL.h"
#include "TextureManager.h"
#endif

#include <assert.h>

class CGUILargeTextureManager : public CThread
{
public:
  CGUILargeTextureManager();
  virtual ~CGUILargeTextureManager();

  virtual void Process();

#ifdef HAS_SDL_2D
  SDL_Surface * GetImage(const CStdString &path, int &width, int &height, int &orientation, bool firstRequest);
#else
  CGLTexture  * GetImage(const CStdString &path, int &width, int &height, int &orientation, bool firstRequest);
#endif
  void ReleaseImage(const CStdString &path);

  void CleanupUnusedImages();

protected:
  class CLargeTexture
  {
  public:
    CLargeTexture(const CStdString &path)
    {
      m_path = path;
      m_width = 0;
      m_height = 0;
      m_orientation = 0;
      m_texture = NULL;
      m_refCount = 1;
      m_timeToDelete = 0;
    };

    virtual ~CLargeTexture()
    {
      assert(m_refCount == 0);
      if (m_texture)
        SDL_FreeSurface(m_texture);
      m_texture = NULL;
    };

    void AddRef() { m_refCount++; };
    bool DecrRef(bool deleteImmediately)
    {
      assert(m_refCount);
      m_refCount--;
      if (m_refCount == 0)
      {
        if (deleteImmediately)
          delete this;
        else
          m_timeToDelete = timeGetTime() + TIME_TO_DELETE;
        return true;
      }
      return false;
    };

    bool DeleteIfRequired()
    {
      if (m_refCount == 0 && m_timeToDelete < timeGetTime())
      {
        delete this;
        return true;
      }
      return false;
    };

    void SetTexture(SDL_Surface * texture, int width, int height, int orientation)
    {
      assert(m_texture == NULL);
      m_texture = texture;
      m_width = width;
      m_height = height;
      m_orientation = orientation;
    };

    SDL_Surface * GetTexture() const { return m_texture; };
    int GetWidth() const { return m_width; };
    int GetHeight() const { return m_height; };
    int GetOrientation() const { return m_orientation; };
    const CStdString &GetPath() const { return m_path; };

  private:
    static const unsigned int TIME_TO_DELETE = 2000;

    unsigned int m_refCount;
    CStdString m_path;
    SDL_Surface * m_texture;
    int m_width;
    int m_height;
    int m_orientation;
    unsigned int m_timeToDelete;
  };

  void QueueImage(CLargeTexture *image);

private:
  std::vector<CLargeTexture *> m_queued;
  std::vector<CLargeTexture *> m_allocated;
  typedef std::vector<CLargeTexture *>::iterator listIterator;

  CCriticalSection m_listSection;
  bool m_running;
};

extern CGUILargeTextureManager g_largeTextureManager;

