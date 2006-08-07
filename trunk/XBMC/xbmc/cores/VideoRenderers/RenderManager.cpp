/*
* XBoxMediaCenter
* Copyright (c) 2003 Frodo/jcmarshall
* Portions Copyright (c) by the authors of ffmpeg / xvid /mplayer
*
* This program is free software; you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation; either version 2 of the License, or
* (at your option) any later version.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with this program; if not, write to the Free Software
* Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
*/
#include "../../stdafx.h"
#include "RenderManager.h"

#include "PixelShaderRenderer.h"
#include "ComboRenderer.h"
#include "RGBRenderer.h"
#include "../../xbox/Undocumented.h"

CXBoxRenderManager g_renderManager;


#define MAXPRESENTDELAY 500

/* at any point we want an exclusive lock on rendermanager */
/* we must make sure we don't have a graphiccontext lock */
/* these two functions allow us to step out from that lock */
/* and reaquire it after having the exclusive lock */

//VBlank information
HANDLE g_eventVBlank=NULL;
void VBlankCallback(D3DVBLANKDATA *pData)
{
  PulseEvent(g_eventVBlank);
}


CXBoxRenderManager::CXBoxRenderManager()
{
  m_pRenderer = NULL;
  m_bPauseDrawing = false;

  m_presentdelay = 5; //Just a guess to what delay we have
  m_presentfield = FS_NONE;
  m_presenttime = 0L;
}

CXBoxRenderManager::~CXBoxRenderManager()
{
  DWORD locks = ExitCriticalSection(g_graphicsContext);
  CExclusiveLock lock(m_sharedSection);
  RestoreCriticalSection(g_graphicsContext, locks);

  if (m_pRenderer)
    delete m_pRenderer;
  m_pRenderer = NULL;
}

unsigned int CXBoxRenderManager::Configure(unsigned int width, unsigned int height, unsigned int d_width, unsigned int d_height, float fps)
{
  DWORD locks = ExitCriticalSection(g_graphicsContext);
  CExclusiveLock lock(m_sharedSection);
  RestoreCriticalSection(g_graphicsContext, locks);

  m_iSourceWidth = width;
  m_iSourceHeight = height;
  m_presentdelay = 5;
  unsigned int result = 0;
  if (m_pRenderer)
  {
    result = m_pRenderer->Configure(width, height, d_width, d_height, fps);
    Update(false);
    m_bIsStarted = true;
  }
  return result;
}

void CXBoxRenderManager::Update(bool bPauseDrawing)
{
  DWORD locks = ExitCriticalSection(g_graphicsContext);
  CExclusiveLock lock(m_sharedSection);
  RestoreCriticalSection(g_graphicsContext, locks);

  m_bPauseDrawing = bPauseDrawing;
  if (m_pRenderer)
  {
    m_pRenderer->Update(bPauseDrawing);
  }
}

unsigned int CXBoxRenderManager::PreInit()
{
  DWORD locks = ExitCriticalSection(g_graphicsContext);
  CExclusiveLock lock(m_sharedSection);
  RestoreCriticalSection(g_graphicsContext, locks);

  if(!g_eventVBlank)
  {
    //Only do this on first run
    g_eventVBlank = CreateEvent(NULL,FALSE,FALSE,NULL);
    D3D__pDevice->SetVerticalBlankCallback((D3DVBLANKCALLBACK)VBlankCallback);
  }

  /* no pedning present */
  m_eventPresented.Set();

  m_bIsStarted = false;
  m_bPauseDrawing = false;
  if (!m_pRenderer)
  { // no renderer
    if (g_guiSettings.GetInt("videoplayer.rendermethod") == RENDER_OVERLAYS)
    {
      CLog::Log(LOGDEBUG, __FUNCTION__" - Selected Overlay-Renderer");
      m_pRenderer = new CComboRenderer(g_graphicsContext.Get3DDevice());
    }
    else if (g_guiSettings.GetInt("videoplayer.rendermethod") == RENDER_HQ_RGB_SHADER)
    {
      CLog::Log(LOGDEBUG, __FUNCTION__" - Selected RGB-Renderer");
      m_pRenderer = new CRGBRenderer(g_graphicsContext.Get3DDevice());
    }
    else // if (g_guiSettings.GetInt("videoplayer.rendermethod") == RENDER_LQ_RGB_SHADER)
    {
      CLog::Log(LOGDEBUG, __FUNCTION__" - Selected LQShader-Renderer");
      m_pRenderer = new CPixelShaderRenderer(g_graphicsContext.Get3DDevice());
    }
  }

  return m_pRenderer->PreInit();
}

void CXBoxRenderManager::UnInit()
{
  m_bStop = true;
  m_eventFrame.Set();

  StopThread();

  DWORD locks = ExitCriticalSection(g_graphicsContext);
  CExclusiveLock lock(m_sharedSection);
  RestoreCriticalSection(g_graphicsContext, locks);

  if (m_pRenderer)
  {
    m_pRenderer->UnInit();
    delete m_pRenderer;
    m_pRenderer = NULL;
  }
}

void CXBoxRenderManager::SetupScreenshot()
{
  CSharedLock lock(m_sharedSection);
  if (m_pRenderer)
    m_pRenderer->SetupScreenshot();
}

void CXBoxRenderManager::CreateThumbnail(LPDIRECT3DSURFACE8 surface, unsigned int width, unsigned int height)
{
  DWORD locks = ExitCriticalSection(g_graphicsContext);
  CExclusiveLock lock(m_sharedSection);
  RestoreCriticalSection(g_graphicsContext, locks);

  if (m_pRenderer)
    m_pRenderer->CreateThumbnail(surface, width, height);
}


void CXBoxRenderManager::FlipPage(DWORD delay /* = 0LL*/, int source /*= -1*/, EFIELDSYNC sync /*= FS_NONE*/)
{
  DWORD timestamp = 0;

  if(delay > MAXPRESENTDELAY) delay = MAXPRESENTDELAY;
  if(delay > 0)
    timestamp = GetTickCount() + delay;

  CSharedLock lock(m_sharedSection);
  if(!m_pRenderer) return;

  /* make sure any previous frame was presented */
  if( !m_eventPresented.WaitMSec(MAXPRESENTDELAY*2) )
    CLog::Log(LOGERROR, " - Timeout waiting for previous frame to be presented");

  m_presenttime = timestamp;
  m_presentfield = sync;

  CSingleLock lock2(g_graphicsContext);
  if( g_graphicsContext.IsFullScreenVideo() )
  {
    lock2.Leave();

    m_pRenderer->FlipPage(source);
    if( timestamp )
    {
      if( CThread::ThreadHandle() == NULL ) CThread::Create();
      m_eventFrame.Set();
    }
    else
    {
      Present();
      m_eventPresented.Set();
    }
  }
  else
  {
    lock2.Leave();

    /* if we are not in fullscreen, we don't control when we render */
    /* so we must await the time and flip then */
    while( timestamp > GetTickCount() && !CThread::m_bStop) Sleep(1);
    m_pRenderer->FlipPage(source);
    m_eventPresented.Set();
  }
}

void CXBoxRenderManager::Present()
{
  EINTERLACEMETHOD mInt = g_stSettings.m_currentVideoSettings.m_InterlaceMethod;
  
  /* check for forced fields */
  if( mInt == VS_INTERLACEMETHOD_AUTO && m_presentfield != FS_NONE )
  {
    /* this is uggly to do on each frame, should only need be done once */
    int mResolution = g_graphicsContext.GetVideoResolution();
    if( mResolution == HDTV_480p_16x9 || mResolution == HDTV_480p_4x3 || mResolution == HDTV_720p || mResolution == HDTV_1080i)
      mInt = VS_INTERLACEMETHOD_RENDER_BLEND;
    else
      mInt = VS_INTERLACEMETHOD_RENDER_BOB;
  }
  else if( mInt == VS_INTERLACEMETHOD_RENDER_BOB_INVERTED || mInt == VS_INTERLACEMETHOD_RENDER_WEAVE_INVERTED )
  {
    /* all methods should default to odd if nothing is specified */
    if( m_presentfield == FS_EVEN )
      m_presentfield = FS_ODD;
    else
      m_presentfield = FS_EVEN;
  }

  /* if we have a field, we will have a presentation delay */
  if( m_presentfield == FS_NONE )
    m_presentdelay = 20;
  else
    m_presentdelay = 40;
  
  if( m_presenttime >= m_presentdelay )
    m_presenttime -=  m_presentdelay;
  else
    m_presenttime = 0;

  if( mInt == VS_INTERLACEMETHOD_RENDER_BOB || mInt == VS_INTERLACEMETHOD_RENDER_BOB_INVERTED)
    PresentBob();
  else if( mInt == VS_INTERLACEMETHOD_RENDER_WEAVE || mInt == VS_INTERLACEMETHOD_RENDER_WEAVE_INVERTED)
    PresentWeave(); 
  else if( mInt == VS_INTERLACEMETHOD_RENDER_BLEND )
    PresentBlend();
  else
    PresentSingle();
}

/* simple present method */
void CXBoxRenderManager::PresentSingle()
{
  CSingleLock lock(g_graphicsContext);

  m_pRenderer->RenderUpdate(true, 0, 255);

  while( m_presenttime > GetTickCount() && !CThread::m_bStop ) Sleep(1);

  D3D__pDevice->Present( NULL, NULL, NULL, NULL );
}

/* new simpler method of handling interlaced material, *
 * we just render the two fields right after eachother */
void CXBoxRenderManager::PresentBob()
{
  CSingleLock lock(g_graphicsContext);

  if( m_presentfield == FS_EVEN )
    m_pRenderer->RenderUpdate(true, RENDER_FLAG_EVEN | RENDER_FLAG_NOUNLOCK , 255);
  else
    m_pRenderer->RenderUpdate(true, RENDER_FLAG_ODD | RENDER_FLAG_NOUNLOCK, 255);

  /* wait for timestamp */
  while( m_presenttime > GetTickCount() && !CThread::m_bStop ) Sleep(1);

  D3D__pDevice->Present( NULL, NULL, NULL, NULL );

  /* render second field */
  if( m_presentfield == FS_EVEN )
    m_pRenderer->RenderUpdate(true, RENDER_FLAG_ODD | RENDER_FLAG_NOLOCK, 255);
  else
    m_pRenderer->RenderUpdate(true, RENDER_FLAG_EVEN | RENDER_FLAG_NOLOCK, 255);
  
  D3D__pDevice->Present( NULL, NULL, NULL, NULL );
}

void CXBoxRenderManager::PresentBlend()
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
  

  /* wait for timestamp */
  while( m_presenttime > GetTickCount() && !CThread::m_bStop ) Sleep(1);

  D3D__pDevice->Present( NULL, NULL, NULL, NULL );
}

/* renders the two fields as one, but doing fieldbased *
 * scaling then reinterlaceing resulting image         */
void CXBoxRenderManager::PresentWeave()
{
  CSingleLock lock(g_graphicsContext);

  m_pRenderer->RenderUpdate(true, RENDER_FLAG_BOTH, 255);

  /* wait for timestamp */
  while( m_presenttime > GetTickCount() && !CThread::m_bStop ) Sleep(1);

  //If we have interlaced video, we have to sync to only render on even fields
  D3DFIELD_STATUS mStatus;
  D3D__pDevice->GetDisplayFieldStatus(&mStatus);

#ifdef PROFILE
  m_presentfield = FS_NONE;
#endif

  //If this was not the correct field. we have to wait for the next one.. damn
  if( (mStatus.Field == D3DFIELD_EVEN && m_presentfield == FS_EVEN) ||
      (mStatus.Field == D3DFIELD_ODD && m_presentfield == FS_ODD) )
  {
    if( WaitForSingleObject(g_eventVBlank, 500) == WAIT_TIMEOUT )
      CLog::Log(LOGERROR, __FUNCTION__" - Waiting for vertical-blank timed out");
  }

  D3D__pDevice->Present( NULL, NULL, NULL, NULL );
}

void CXBoxRenderManager::Process()
{
  float actualdelay = (float)m_presentdelay;

  SetPriority(THREAD_PRIORITY_TIME_CRITICAL);
  SetName("AsyncRenderer");
  while( !m_bStop )
  {
    //Wait for new frame or an stop event
    m_eventFrame.Wait();
    if( m_bStop ) return;

    DWORD dwTimeStamp = GetTickCount();
    try
    {
      CSharedLock lock(m_sharedSection);
      CSingleLock lock2(g_graphicsContext);

      if( m_pRenderer && g_graphicsContext.IsFullScreenVideo() )
        Present();
    }
    catch(...)
    {
      CLog::Log(LOGERROR, "CXBoxRenderer::Process() - Exception thrown in flippage");
    }
    m_eventPresented.Set();

    const int TC = 100; /* time (frame) constant for convergence */
    actualdelay = ( actualdelay * (TC-1) + (GetTickCount() - dwTimeStamp) ) / TC;
  }
}
