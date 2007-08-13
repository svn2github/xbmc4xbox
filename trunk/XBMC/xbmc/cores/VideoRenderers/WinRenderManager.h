#pragma once

#include "WinRenderer.h"

class CWinRenderManager
{
public:
  CWinRenderManager();
  ~CWinRenderManager();

  // Functions called from the GUI
  void GetVideoRect(RECT &rs, RECT &rd) { if (m_pRenderer) m_pRenderer->GetVideoRect(rs, rd); }
  float GetAspectRatio() { if (m_pRenderer) return m_pRenderer->GetAspectRatio(); else return 1.0f; }
  void AutoCrop(bool bCrop = true) { if (m_pRenderer) m_pRenderer->AutoCrop(bCrop); }
  void Update(bool bPauseDrawing);
  void RenderUpdate(bool clear, DWORD flags = 0, DWORD alpha = 255);
  void SetupScreenshot();
  void CreateThumbnail(LPDIRECT3DSURFACE8 surface, unsigned int width, unsigned int height);
  void SetViewMode(int iViewMode) { if (m_pRenderer) m_pRenderer->SetViewMode(iViewMode); }

  // Functions called from mplayer
  bool Configure(unsigned int width, unsigned int height, unsigned int d_width, unsigned int d_height, float fps, unsigned flags);

  // a call to GetImage must be followed by a call to releaseimage if getimage was successfull
  // failure to do so will result in deadlock
  inline int GetImage(YV12Image *image, int source = AUTOSOURCE, bool readonly = false)
  {
    if (m_pRenderer)
      return m_pRenderer->GetImage(image, source, readonly);
    return -1;
  }
  inline void ReleaseImage(int source = AUTOSOURCE, bool preserve = false)
  {
    if (m_pRenderer)
      m_pRenderer->ReleaseImage(source, preserve);
  }
  inline unsigned int DrawSlice(unsigned char *src[], int stride[], int w, int h, int x, int y)
  {
    if (m_pRenderer)
      return m_pRenderer->DrawSlice(src, stride, w, h, x, y);
    return 0;
  }

  void FlipPage(DWORD timestamp = 0L, int source = -1, EFIELDSYNC sync = FS_NONE);
  unsigned int PreInit();
  void UnInit();

  inline void DrawAlpha(int x0, int y0, int w, int h, unsigned char *src, unsigned char *srca, int stride)
  {
    if (m_pRenderer)
      m_pRenderer->DrawAlpha(x0, y0, w, h, src, srca, stride);
  }
  inline void Reset()
  {
    if (m_pRenderer)
      m_pRenderer->Reset();
  }
  inline RESOLUTION GetResolution()
  {
    if (m_pRenderer)
      return m_pRenderer->GetResolution();
    else
      return INVALID;
  }

  inline DWORD GetPresentDelay() { return 0;  }
  inline bool Paused() { return m_bPauseDrawing; };
  inline bool IsStarted() { return m_bIsStarted;}

  CWinRenderer *m_pRenderer;
protected:
  bool m_bPauseDrawing;   // true if we should pause rendering
  bool m_bIsStarted;
};

extern CWinRenderManager g_renderManager;
