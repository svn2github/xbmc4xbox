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

#if defined (HAS_GL)
  #include "LinuxRendererGL.h"
#elif defined(HAS_DX)
  #include "WinRenderer.h"
  #include "WinDsRenderer.h"
#elif defined(HAS_SDL)
  #include "LinuxRenderer.h"
#endif

#include "utils/SharedSection.h"
#include "utils/Thread.h"
#include "settings/VideoSettings.h"
#include "OverlayRenderer.h"

class CXBMCRenderManager
{
public:
  CXBMCRenderManager();
  ~CXBMCRenderManager();

  // Functions called from the GUI
  void GetVideoRect(CRect &source, CRect &dest) { CSharedLock lock(m_sharedSection); if (m_pRenderer) m_pRenderer->GetVideoRect(source, dest); };
  float GetAspectRatio() { CSharedLock lock(m_sharedSection); if (m_pRenderer) return m_pRenderer->GetAspectRatio(); else return 1.0f; };
  void Update(bool bPauseDrawing);
  void RenderUpdate(bool clear, DWORD flags = 0, DWORD alpha = 255);
  void SetupScreenshot();

  void CreateThumbnail(CBaseTexture *texture, unsigned int width, unsigned int height);

  void SetViewMode(int iViewMode) { CSharedLock lock(m_sharedSection); if (m_pRenderer) m_pRenderer->SetViewMode(iViewMode); };

  // Functions called from mplayer
  bool Configure(unsigned int width, unsigned int height, unsigned int d_width, unsigned int d_height, float fps, unsigned flags);
  bool IsConfigured();

  // a call to GetImage must be followed by a call to releaseimage if getimage was successfull
  // failure to do so will result in deadlock
  inline int GetImage(YV12Image *image, int source = AUTOSOURCE, bool readonly = false)
  {
    CSharedLock lock(m_sharedSection);
    if (m_pRenderer)
      return m_pRenderer->GetImage(image, source, readonly);
    return -1;
  }

  inline void ReleaseImage(int source = AUTOSOURCE, bool preserve = false)
  {
    CSharedLock lock(m_sharedSection);
    if (m_pRenderer)
      m_pRenderer->ReleaseImage(source, preserve);
  }
#if defined(HAS_DX)
  inline void PaintVideoTexture(CD3DTexture* videoTexture,IDirect3DSurface9* videoSurface)
  {
    CSharedLock lock(m_sharedSection);
    if (m_pRenderer)
      m_pRenderer->PaintVideoTexture(videoTexture,videoSurface);
  }
#endif

  inline unsigned int DrawSlice(unsigned char *src[], int stride[], int w, int h, int x, int y)
  {
    CSharedLock lock(m_sharedSection);
    if (m_pRenderer)
      return m_pRenderer->DrawSlice(src, stride, w, h, x, y);
    return 0;
  }

  void FlipPage(volatile bool& bStop, double timestamp = 0.0, int source = -1, EFIELDSYNC sync = FS_NONE);
  unsigned int PreInit(bool dshow = false);
  void UnInit();

  void AddOverlay(CDVDOverlay* o, double pts)
  {
    CSharedLock lock(m_sharedSection);
    m_overlays.AddOverlay(o, pts);
  }

  void AddCleanup(OVERLAY::COverlay* o)
  {
    CSharedLock lock(m_sharedSection);
    m_overlays.AddCleanup(o);
  }

  inline void Reset()
  {
    CSharedLock lock(m_sharedSection);
    if (m_pRenderer)
      m_pRenderer->Reset();
  }
  RESOLUTION GetResolution()
  {
    CSharedLock lock(m_sharedSection);
    if (m_pRenderer)
      return m_pRenderer->GetResolution();
    else
      return RES_INVALID;
  }

  RENDERERTYPE GetRendererType() { return m_pRendererType; }
  float GetMaximumFPS();
  inline bool Paused() { return m_bPauseDrawing; }
  inline bool IsStarted() { return m_bIsStarted; }
  bool SupportsBrightness();
  bool SupportsContrast();
  bool SupportsGamma();

  bool Supports(EINTERLACEMETHOD method)
  {
    CSharedLock lock(m_sharedSection);
    if (m_pRenderer)
      return m_pRenderer->Supports(method);
    else
      return false;
  }

  bool Supports(ESCALINGMETHOD method)
  {
    CSharedLock lock(m_sharedSection);
    if (m_pRenderer)
      return m_pRenderer->Supports(method);
    else
      return false;
  }

  double GetPresentTime();
  void  WaitPresentTime(double presenttime);

  CStdString GetVSyncState();
  
  void UpdateResolution();

#ifdef HAS_GL
  CLinuxRendererGL *m_pRenderer;
#elif defined(HAS_DX)
  CWinBaseRenderer *m_pRenderer;
#elif defined(HAS_SDL)
  CLinuxRenderer *m_pRenderer;
#elif defined(HAS_XBOX_D3D)
  CXBoxRenderer *m_pRenderer;
  
#endif

  void Present();
  void Recover(); // called after resolution switch if something special is needed

  CSharedSection& GetSection() { return m_sharedSection; };
protected:

  void PresentSingle();
  void PresentWeave();
  void PresentBob();
  void PresentBlend();

  bool m_bPauseDrawing;   // true if we should pause rendering
  bool m_bIsStarted;
  CSharedSection m_sharedSection;

  bool m_bReconfigured;
  
  int m_rendermethod;

  RENDERERTYPE m_pRendererType;

  enum EPRESENTSTEP
  {
    PRESENT_IDLE     = 0
  , PRESENT_FLIP
  , PRESENT_FRAME
  , PRESENT_FRAME2
  };

  double     m_presenttime;
  double     m_presentcorr;
  double     m_presenterr;
  EFIELDSYNC m_presentfield;
  EINTERLACEMETHOD m_presentmethod;
  EPRESENTSTEP     m_presentstep;
  int        m_presentsource;
  CEvent     m_presentevent;


  OVERLAY::CRenderer m_overlays;
};

extern CXBMCRenderManager g_renderManager;



