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
 
#include "stdafx.h"
#include "RenderManager.h"
#include "utils/CriticalSection.h"
#include "VideoReferenceClock.h"
#include "MathUtils.h"

#include "Application.h"
#include "Settings.h"
#include "GUISettings.h"
#include "WindowingFactory.h"

#ifdef _LINUX
#include "PlatformInclude.h"
#endif

#if defined(HAS_GL)
  #include "LinuxRendererGL.h"
#elif defined(HAS_DX)
  #include "WinRenderer.h"
#elif defined(HAS_SDL)
  #include "LinuxRenderer.h"
#endif

#ifdef HAVE_LIBVDPAU
#include "cores/dvdplayer/DVDCodecs/Video/VDPAU.h"
#endif

/* to use the same as player */
#include "../dvdplayer/DVDClock.h"

CXBMCRenderManager g_renderManager;

#define MAXPRESENTDELAY 0.500

/* at any point we want an exclusive lock on rendermanager */
/* we must make sure we don't have a graphiccontext lock */
/* these two functions allow us to step out from that lock */
/* and reaquire it after having the exclusive lock */

template<class T>
class CRetakeLock
{
public:
  CRetakeLock(CSharedSection &section, bool immidiate = true, CCriticalSection &owned = g_graphicsContext)
    : m_owned(owned)
  {
    m_count = ExitCriticalSection(m_owned);
    m_lock  = new T(section);
    if(immidiate)
    {
      RestoreCriticalSection(m_owned, m_count);
      m_count = 0;
    }
  }
  ~CRetakeLock()
  {
    delete m_lock;
    RestoreCriticalSection(m_owned, m_count);
  }
  void Leave() { m_lock->Leave(); }
  void Enter() { m_lock->Enter(); }

private:
  T*                m_lock;
  CCriticalSection &m_owned;
  DWORD             m_count;
};

CXBMCRenderManager::CXBMCRenderManager()
{
  m_pRenderer = NULL;
  m_bPauseDrawing = false;
  m_bIsStarted = false;

  m_presentfield = FS_NONE;
  m_presenttime = 0;
  m_presentstep = 0;
  m_rendermethod = 0;
  m_presentmethod = VS_INTERLACEMETHOD_NONE;
}

CXBMCRenderManager::~CXBMCRenderManager()
{
  delete m_pRenderer;
  m_pRenderer = NULL;
}

/* These is based on QueryPerformanceCounter */
double CXBMCRenderManager::GetPresentTime()
{
  return CDVDClock::GetAbsoluteClock() / DVD_TIME_BASE;
}

void CXBMCRenderManager::WaitPresentTime(double presenttime)
{
  int fps = g_VideoReferenceClock.GetRefreshRate();
  if(fps <= 0)
  {
    /* smooth video not enabled */
    CDVDClock::WaitAbsoluteClock(presenttime * DVD_TIME_BASE);
    return;
  }

  double frametime = g_VideoReferenceClock.GetSpeed() / fps;

  presenttime     += m_presentcorr * frametime;

  double clock     = CDVDClock::WaitAbsoluteClock(presenttime * DVD_TIME_BASE) / DVD_TIME_BASE;
  double target    = 0.5;
  double error     = ( clock - presenttime ) / frametime - target;

  m_presenterr   = error;

  // correct error so it targets the closest vblank
  while(error >   target) error -= 1.0;
  while(error < - target) error += 1.0;

  // scale the error used for correction, 
  // based on how much buffer we have on
  // that side of the target
  if(error > 0)
    error /= 2.0 * (1.0 - target);
  if(error < 0)
    error /= 2.0 * (0.0 + target);

  m_presentcorr += error * 0.02;

  // make sure we wrap correction properly
  while(m_presentcorr > target)       m_presentcorr -= 1.0;
  while(m_presentcorr < target - 1.0) m_presentcorr += 1.0;

  //printf("%f %f % 2.0f%% % f % f\n", presenttime, clock, m_presentcorr * 100, error, error_org);
}

CStdString CXBMCRenderManager::GetVSyncState()
{
  CStdString state;
  state.Format("sync:%+3d%% error:%2d%%"
              ,     MathUtils::round_int(m_presentcorr * 100)
              , abs(MathUtils::round_int(m_presenterr  * 100)));
  return state;
}

bool CXBMCRenderManager::Configure(unsigned int width, unsigned int height, unsigned int d_width, unsigned int d_height, float fps, unsigned flags)
{
  /* all frames before this should be rendered */
  WaitPresentTime(m_presenttime);

  CRetakeLock<CExclusiveLock> lock(m_sharedSection, false);

  if(!m_pRenderer) 
  {
    CLog::Log(LOGERROR, "%s called without a valid Renderer object", __FUNCTION__);
    return false;
  }

  bool result = m_pRenderer->Configure(width, height, d_width, d_height, fps, flags);
  if(result)
  {
    if( flags & CONF_FLAGS_FULLSCREEN )
    {
      lock.Leave();
      g_application.getApplicationMessenger().SwitchToFullscreen();
      lock.Enter();
    }
    m_pRenderer->Update(false);
    m_bIsStarted = true;
  }
  
  return result;
}

bool CXBMCRenderManager::IsConfigured()
{
  if (!m_pRenderer)
    return false;
  return m_pRenderer->IsConfigured();
}

void CXBMCRenderManager::Update(bool bPauseDrawing)
{
  CRetakeLock<CExclusiveLock> lock(m_sharedSection);

  m_bPauseDrawing = bPauseDrawing;
  if (m_pRenderer)
  {
    m_pRenderer->Update(bPauseDrawing);
  }
}

void CXBMCRenderManager::RenderUpdate(bool clear, DWORD flags, DWORD alpha)
{
  CSharedLock lock(m_sharedSection);
  if (!m_pRenderer)
    return;


  if( m_presentmethod == VS_INTERLACEMETHOD_RENDER_WEAVE
   || m_presentmethod == VS_INTERLACEMETHOD_RENDER_WEAVE_INVERTED)
    m_pRenderer->RenderUpdate(clear, flags | RENDER_FLAG_BOTH, alpha);
  else
    m_pRenderer->RenderUpdate(clear, flags | RENDER_FLAG_LAST, alpha);


  m_overlays.Render();
}

unsigned int CXBMCRenderManager::PreInit()
{
  CRetakeLock<CExclusiveLock> lock(m_sharedSection);

  m_presentcorr = 0.0;
  m_presenterr  = 0.0;

  m_bIsStarted = false;
  m_bPauseDrawing = false;
  if (!m_pRenderer)
  { 
#if defined(HAS_GL)
    m_pRenderer = new CLinuxRendererGL();
#elif defined(HAS_DX)
    m_pRenderer = new CPixelShaderRenderer(g_Windowing.Get3DDevice());
#elif defined(HAS_SDL)
    m_pRenderer = new CLinuxRenderer();
#endif
  }

  return m_pRenderer->PreInit();
}

void CXBMCRenderManager::UnInit()
{
  CRetakeLock<CExclusiveLock> lock(m_sharedSection);

  m_bIsStarted = false;

  m_overlays.Flush();

  // free renderer resources.
  // TODO: we may also want to release the renderer here.
  if (m_pRenderer)
    m_pRenderer->UnInit();
}

void CXBMCRenderManager::SetupScreenshot()
{
  CSharedLock lock(m_sharedSection);
  if (m_pRenderer)
    m_pRenderer->SetupScreenshot();
}

void CXBMCRenderManager::CreateThumbnail(CBaseTexture *texture, unsigned int width, unsigned int height)
{  
  CSharedLock lock(m_sharedSection);
  if (m_pRenderer)
    m_pRenderer->CreateThumbnail(texture, width, height);
}

void CXBMCRenderManager::FlipPage(volatile bool& bStop, double timestamp /* = 0LL*/, int source /*= -1*/, EFIELDSYNC sync /*= FS_NONE*/)
{
  if(timestamp - GetPresentTime() > MAXPRESENTDELAY)
    timestamp =  GetPresentTime() + MAXPRESENTDELAY;

  /* make sure any queued frame was presented */
  if(g_graphicsContext.IsFullScreenVideo())
    while(!g_application.WaitFrame(100) && !bStop) {}
  else
    WaitPresentTime(timestamp);

  if(bStop)
    return;

  { CRetakeLock<CExclusiveLock> lock(m_sharedSection);
    if(!m_pRenderer) return;

    m_presenttime  = timestamp;
    m_presentfield = sync;
    m_presentstep  = 0;
    m_presentmethod = g_stSettings.m_currentVideoSettings.m_InterlaceMethod;

    /* select render method for auto */
    if(m_presentmethod == VS_INTERLACEMETHOD_AUTO)
    {
      if(m_presentfield == FS_NONE)
        m_presentmethod = VS_INTERLACEMETHOD_NONE;
      else
        m_presentmethod = VS_INTERLACEMETHOD_RENDER_BOB;
    }

    /* default to odd field if we want to deinterlace and don't know better */
    if(m_presentfield == FS_NONE && m_presentmethod != VS_INTERLACEMETHOD_NONE)
      m_presentfield = FS_ODD;

    /* invert present field if we have one of those methods */
    if( m_presentmethod == VS_INTERLACEMETHOD_RENDER_BOB_INVERTED 
     || m_presentmethod == VS_INTERLACEMETHOD_RENDER_WEAVE_INVERTED )
    {
      if( m_presentfield == FS_EVEN )
        m_presentfield = FS_ODD;
      else
        m_presentfield = FS_EVEN;
    }

    m_overlays.Flip();
    m_pRenderer->FlipPage(source);
  }

  m_presentevent.Reset();
  g_application.NewFrame();
  m_presentevent.WaitMSec(1); // we give the application thread 1ms to present
}

float CXBMCRenderManager::GetMaximumFPS()
{
  float fps;

  if (g_guiSettings.GetInt("videoscreen.vsync") != VSYNC_DISABLED)
  {
    fps = (float)g_VideoReferenceClock.GetRefreshRate();
    if (fps <= 0) fps = g_graphicsContext.GetFPS();
  }
  else
    fps = 1000.0f;
  
  return fps;
}

bool CXBMCRenderManager::SupportsBrightness()
{
  CSharedLock lock(m_sharedSection);
  if (m_pRenderer)
    return m_pRenderer->SupportsBrightness();
  return false;
}

bool CXBMCRenderManager::SupportsContrast()
{
  CSharedLock lock(m_sharedSection);
  if (m_pRenderer)
    return m_pRenderer->SupportsContrast();
  return false;
}

bool CXBMCRenderManager::SupportsGamma()
{
  CSharedLock lock(m_sharedSection);
  if (m_pRenderer)
    return m_pRenderer->SupportsGamma();
  return false;
}

void CXBMCRenderManager::Present()
{
  CSharedLock lock(m_sharedSection);

#ifdef HAVE_LIBVDPAU
  /* wait for this present to be valid */
  if(g_graphicsContext.IsFullScreenVideo() && g_VDPAU)
    WaitPresentTime(m_presenttime);
#endif

  if (!m_pRenderer)
  {
    CLog::Log(LOGERROR, "%s called without valid Renderer object", __FUNCTION__);
    return;
  }

  if     ( m_presentmethod == VS_INTERLACEMETHOD_RENDER_BOB
        || m_presentmethod == VS_INTERLACEMETHOD_RENDER_BOB_INVERTED)
    PresentBob();
  else if( m_presentmethod == VS_INTERLACEMETHOD_RENDER_WEAVE
        || m_presentmethod == VS_INTERLACEMETHOD_RENDER_WEAVE_INVERTED)
    PresentWeave();
  else if( m_presentmethod == VS_INTERLACEMETHOD_RENDER_BLEND )
    PresentBlend();
  else
    PresentSingle();

  m_overlays.Render();

  /* wait for this present to be valid */
  if(g_graphicsContext.IsFullScreenVideo())
  {
    /* we are not done until present step has reverted to 0 */
    if(m_presentstep == 0)
      m_presentevent.Set();
#ifdef HAVE_LIBVDPAU
    if (!g_VDPAU)
#endif
    {
      WaitPresentTime(m_presenttime);
    }
  }
}

/* simple present method */
void CXBMCRenderManager::PresentSingle()
{
  CSingleLock lock(g_graphicsContext);

  m_pRenderer->RenderUpdate(true, 0, 255);
}

/* new simpler method of handling interlaced material, *
 * we just render the two fields right after eachother */
void CXBMCRenderManager::PresentBob()
{
  CSingleLock lock(g_graphicsContext);

  if(m_presentstep == 0)
  {
    if( m_presentfield == FS_EVEN)
      m_pRenderer->RenderUpdate(true, RENDER_FLAG_EVEN, 255);
    else
      m_pRenderer->RenderUpdate(true, RENDER_FLAG_ODD, 255);
    m_presentstep = 1;
    g_application.NewFrame();
  }
  else
  {
    if( m_presentfield == FS_ODD)
      m_pRenderer->RenderUpdate(true, RENDER_FLAG_EVEN, 255);
    else
      m_pRenderer->RenderUpdate(true, RENDER_FLAG_ODD, 255);
    m_presentstep = 0;
  }
}

void CXBMCRenderManager::PresentBlend()
{
  CSingleLock lock(g_graphicsContext);

  if( m_presentfield == FS_EVEN )
  {
    m_pRenderer->RenderUpdate(true, RENDER_FLAG_EVEN | RENDER_FLAG_NOOSD, 255);
    m_pRenderer->RenderUpdate(false, RENDER_FLAG_ODD, 128);
  }
  else
  {
    m_pRenderer->RenderUpdate(true, RENDER_FLAG_ODD | RENDER_FLAG_NOOSD, 255);
    m_pRenderer->RenderUpdate(false, RENDER_FLAG_EVEN, 128);
  }
}

/* renders the two fields as one, but doing fieldbased *
 * scaling then reinterlaceing resulting image         */
void CXBMCRenderManager::PresentWeave()
{
  CSingleLock lock(g_graphicsContext);

  m_pRenderer->RenderUpdate(true, RENDER_FLAG_BOTH, 255);
}

void CXBMCRenderManager::Recover()
{
#ifdef HAVE_LIBVDPAU
  CRetakeLock<CExclusiveLock> lock(m_sharedSection);
  if (g_VDPAU)
  {
    glFlush(); // attempt to have gpu done with pixmap
    g_VDPAU->CheckRecover(true);
  }
#endif
}
