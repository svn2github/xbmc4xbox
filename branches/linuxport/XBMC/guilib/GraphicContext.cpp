#include "include.h"
#include "GraphicContext.h"
#include "GUIMessage.h"
#include "IMsgSenderCallback.h"
#include "../xbmc/Settings.h"
#include "../xbmc/XBVideoConfig.h"
#ifdef HAS_XBOX_D3D
 #include "xgraphics.h"
 #define D3D_CLEAR_STENCIL D3DCLEAR_STENCIL
#else
 #define D3D_CLEAR_STENCIL 0x0l
#endif
#ifdef HAS_SDL_OPENGL
#define GLVALIDATE CLockMe locker(this);ValidateSurface()
#endif
#include "Surface.h"
#include "SkinInfo.h"

using namespace Surface;

CGraphicContext g_graphicsContext;
extern bool g_fullScreen;

/* quick access to a skin setting, fine unless we starts clearing video settings */
static CSettingInt* g_guiSkinzoom = NULL;

CGraphicContext::CGraphicContext(void)
{
  m_iScreenWidth = 720;
  m_iScreenHeight = 576;
#ifndef HAS_SDL
  m_pd3dDevice = NULL;
  m_pd3dParams = NULL;
  m_stateBlock = 0xffffffff;
#endif
  m_dwID = 0;
  m_strMediaDir = "D:\\media";
  m_bShowPreviewWindow = false;
  m_bCalibrating = false;
  m_Resolution = INVALID;
  m_pCallback = NULL;
  m_windowScaleX = m_windowScaleY = 1.0f;
  m_windowResolution = INVALID;
  MergeAlpha(10); // this just here so the inline function is included (why doesn't it include it normally??)
  float x=0,y=0;
  ScaleFinalCoords(x, y);
}

CGraphicContext::~CGraphicContext(void)
{
#ifndef HAS_SDL
  if (m_stateBlock != 0xffffffff)
  {
    Get3DDevice()->DeleteStateBlock(m_stateBlock);
  }
#endif

  while (m_viewStack.size())
  {
#ifndef HAS_SDL
    D3DVIEWPORT8 *viewport = m_viewStack.top();
#elif defined(HAS_SDL_2D)
    SDL_Rect *viewport = m_viewStack.top();
#elif defined(HAS_SDL_OPENGL)
    GLint* viewport = m_viewStack.top();
#endif
    m_viewStack.pop();
    if (viewport) delete [] viewport;
  }
}

#ifndef HAS_SDL
void CGraphicContext::SetD3DDevice(LPDIRECT3DDEVICE8 p3dDevice)
{
  m_pd3dDevice = p3dDevice;
}

void CGraphicContext::SetD3DParameters(D3DPRESENT_PARAMETERS *p3dParams)
{
  m_pd3dParams = p3dParams;
}
#endif

bool CGraphicContext::SendMessage(CGUIMessage& message)
{
  if (!m_pCallback) return false;
  return m_pCallback->SendMessage(message);
}

void CGraphicContext::setMessageSender(IMsgSenderCallback* pCallback)
{
  m_pCallback = pCallback;
}

DWORD CGraphicContext::GetNewID()
{
  m_dwID++;
  return m_dwID;
}

bool CGraphicContext::SetViewPort(float fx, float fy , float fwidth, float fheight, bool intersectPrevious /* = false */)
{
#ifndef HAS_SDL
  D3DVIEWPORT8 newviewport;
  D3DVIEWPORT8 *oldviewport = new D3DVIEWPORT8;
  Get3DDevice()->GetViewport(oldviewport);
#elif defined(HAS_SDL_2D)
  SDL_Rect newviewport;
  SDL_Rect *oldviewport = new SDL_Rect;
  SDL_GetClipRect(m_screenSurface->SDL(), oldviewport);
#elif defined(HAS_SDL_OPENGL)
  GLVALIDATE;
  GLint newviewport[4];
  GLint* oldviewport = new GLint[4];
  glGetIntegerv(GL_SCISSOR_BOX, oldviewport);	
#endif
  
  // transform coordinates - we may have a rotation which changes the positioning of the
  // minimal and maximal viewport extents.  We currently go to the maximal extent.
  float x[4], y[4];
  x[0] = x[3] = fx;
  x[1] = x[2] = fx + fwidth;
  y[0] = y[1] = fy;
  y[2] = y[3] = fy + fheight;
  float minX = (float)m_iScreenWidth;
  float maxX = 0;
  float minY = (float)m_iScreenHeight;
  float maxY = 0;
  for (int i = 0; i < 4; i++)
  {
    ScaleFinalCoords(x[i], y[i]);
    if (x[i] < minX) minX = x[i];
    if (x[i] > maxX) maxX = x[i];
    if (y[i] < minY) minY = y[i];
    if (y[i] > maxY) maxY = y[i];
  }

  int newLeft = (int)(minX + 0.5f);
  int newTop = (int)(minY + 0.5f);
  int newRight = (int)(maxX + 0.5f);
  int newBottom = (int)(maxY + 0.5f);
  if (intersectPrevious)
  {
    // do the intersection
#ifndef HAS_SDL    
    int oldLeft = (int)oldviewport->X;
    int oldTop = (int)oldviewport->Y;
    int oldRight = (int)oldviewport->X + oldviewport->Width;
    int oldBottom = (int)oldviewport->Y + oldviewport->Height;
#elif defined(HAS_SDL_2D)
    int oldLeft = (int)oldviewport->x;
    int oldTop = (int)oldviewport->y;
    int oldRight = (int)oldviewport->x + oldviewport->w;
    int oldBottom = (int)oldviewport->y + oldviewport->h;
#elif defined(HAS_SDL_OPENGL)
    int oldLeft = (int)oldviewport[0];
    int oldBottom = m_iScreenHeight - oldviewport[1];       // opengl uses bottomleft as origin
    int oldTop = oldBottom - oldviewport[3];
    int oldRight = (int)oldviewport[0] + oldviewport[2];
#endif    
    if (newLeft >= oldRight || newTop >= oldBottom || newRight <= oldLeft || newBottom <= oldTop)
    { // empty intersection - return false to indicate no rendering should occur
      delete [] oldviewport;
      return false;
    }
    // ok, they intersect, do the intersection
    if (newLeft < oldLeft) newLeft = oldLeft;
    if (newTop < oldTop) newTop = oldTop;
    if (newRight > oldRight) newRight = oldRight;
    if (newBottom > oldBottom) newBottom = oldBottom;
  }
  // check range against screen size
  if (newRight <= 0 || newBottom <= 0 ||
      newTop >= m_iScreenHeight || newLeft >= m_iScreenWidth ||
      newLeft >= newRight || newTop >= newBottom)
  { // no intersection with the screen
    delete [] oldviewport;
    return false;
  }
  // intersection with the screen
  if (newLeft < 0) newLeft = 0;
  if (newTop < 0) newTop = 0;
  if (newRight > m_iScreenWidth) newRight = m_iScreenWidth;
  if (newBottom > m_iScreenHeight) newBottom = m_iScreenHeight;

  ASSERT(newLeft < newRight);
  ASSERT(newTop < newBottom);

#ifndef HAS_SDL
  newviewport.MinZ = 0.0f;
  newviewport.MaxZ = 1.0f;
  newviewport.X = newLeft;
  newviewport.Y = newTop;
  newviewport.Width = newRight - newLeft;
  newviewport.Height = newBottom - newTop;
  m_pd3dDevice->SetViewport(&newviewport);
#elif defined(HAS_SDL_2D)
  newviewport.x = newLeft;
  newviewport.y = newTop;
  newviewport.w = newRight - newLeft;
  newviewport.h = newBottom - newTop;
  SDL_SetClipRect(m_screenSurface->SDL(), &newviewport);
#elif defined(HAS_SDL_OPENGL)
  newviewport[0] = newLeft;
  newviewport[1] = m_iScreenHeight - newBottom; // opengl uses bottomleft as origin
  newviewport[2] = newRight - newLeft;
  newviewport[3] = newBottom - newTop;
  glScissor(newviewport[0], newviewport[1], newviewport[2], newviewport[3]);
  VerifyGLState();
#endif

  m_viewStack.push(oldviewport);

  return true;
}

void CGraphicContext::RestoreViewPort()
{
  if (!m_viewStack.size()) return;
#ifndef HAS_SDL
  D3DVIEWPORT8 *oldviewport = (D3DVIEWPORT8*)m_viewStack.top();
  Get3DDevice()->SetViewport(oldviewport);
#elif defined(HAS_SDL_2D)
  SDL_Rect *oldviewport = (SDL_Rect*)m_viewStack.top();
  SDL_SetClipRect(m_screenSurface->SDL(), oldviewport);
#elif defined(HAS_SDL_OPENGL)
  GLVALIDATE;
  GLint* oldviewport = (GLint*)m_viewStack.top();
  glScissor(oldviewport[0], oldviewport[1], oldviewport[2], oldviewport[3]);
  VerifyGLState();
#endif  

  m_viewStack.pop();

  if (oldviewport) delete [] oldviewport;
}

const RECT& CGraphicContext::GetViewWindow() const
{
  return m_videoRect;
}
void CGraphicContext::SetViewWindow(float left, float top, float right, float bottom)
{
  if (m_bCalibrating)
  {
    SetFullScreenViewWindow(m_Resolution);
  }
  else
  {
    m_videoRect.left = (long)(ScaleFinalXCoord(left, top) + 0.5f);
    m_videoRect.top = (long)(ScaleFinalYCoord(left, top) + 0.5f);
    m_videoRect.right = (long)(ScaleFinalXCoord(right, bottom) + 0.5f);
    m_videoRect.bottom = (long)(ScaleFinalYCoord(right, bottom) + 0.5f);
    if (m_bShowPreviewWindow && !m_bFullScreenVideo)
    {
#ifndef HAS_SDL    
      D3DRECT d3dRC;
      d3dRC.x1 = m_videoRect.left;
      d3dRC.x2 = m_videoRect.right;
      d3dRC.y1 = m_videoRect.top;
      d3dRC.y2 = m_videoRect.bottom;
      Get3DDevice()->Clear( 1, &d3dRC, D3DCLEAR_TARGET, 0x00010001, 1.0f, 0L );
#elif defined(HAS_SDL_2D)
      SDL_Rect r;
      r.x = (Sint16)m_videoRect.left;
      r.y = (Sint16)m_videoRect.top;
      r.w = (Sint16)(m_videoRect.right - m_videoRect.left + 1);
      r.h = (Sint16)(m_videoRect.bottom - m_videoRect.top +1);
      SDL_FillRect(m_screenSurface->SDL(), &r, 0x00010001);    
#endif		  
    }
  }
}

void CGraphicContext::ClipToViewWindow()
{
#ifndef HAS_SDL
  D3DRECT clip = { m_videoRect.left, m_videoRect.top, m_videoRect.right, m_videoRect.bottom };
  if (m_videoRect.left < 0) clip.x1 = 0;
  if (m_videoRect.top < 0) clip.y1 = 0;
  if (m_videoRect.left > m_iScreenWidth - 1) clip.x1 = m_iScreenWidth - 1;
  if (m_videoRect.top > m_iScreenHeight - 1) clip.y1 = m_iScreenHeight - 1;
  if (m_videoRect.right > m_iScreenWidth) clip.x2 = m_iScreenWidth;
  if (m_videoRect.bottom > m_iScreenHeight) clip.y2 = m_iScreenHeight;
  if (clip.x2 < clip.x1) clip.x2 = clip.x1 + 1;
  if (clip.y2 < clip.y1) clip.y2 = clip.y1 + 1;
#ifdef HAS_XBOX_D3D
  m_pd3dDevice->SetScissors(1, FALSE, &clip);
#endif
#else
#ifdef  __GNUC__
#warning CGraphicContext::ClipToViewWindow not implemented
#endif
#endif
}

void CGraphicContext::SetFullScreenViewWindow(RESOLUTION &res)
{
  m_videoRect.left = g_settings.m_ResInfo[res].Overscan.left;
  m_videoRect.top = g_settings.m_ResInfo[res].Overscan.top;
  m_videoRect.right = g_settings.m_ResInfo[res].Overscan.right;
  m_videoRect.bottom = g_settings.m_ResInfo[res].Overscan.bottom;
}

void CGraphicContext::SetFullScreenVideo(bool bOnOff)
{
  Lock();
  m_bFullScreenVideo = bOnOff;
  SetFullScreenViewWindow(m_Resolution);
  Unlock();
}

bool CGraphicContext::IsFullScreenVideo() const
{
  return m_bFullScreenVideo;
}

void CGraphicContext::EnablePreviewWindow(bool bEnable)
{
  m_bShowPreviewWindow = bEnable;
}


bool CGraphicContext::IsCalibrating() const
{
  return m_bCalibrating;
}

void CGraphicContext::SetCalibrating(bool bOnOff)
{
  m_bCalibrating = bOnOff;
}

bool CGraphicContext::IsValidResolution(RESOLUTION res)
{
  return g_videoConfig.IsValidResolution(res);
}

void CGraphicContext::GetAllowedResolutions(vector<RESOLUTION> &res, bool bAllowPAL60)
{
  bool bCanDoWidescreen = g_videoConfig.HasWidescreen();
  res.clear();  
  if (g_videoConfig.HasPAL())
  {
    res.push_back(PAL_4x3);
    if (bCanDoWidescreen) res.push_back(PAL_16x9);
    if (bAllowPAL60 && g_videoConfig.HasPAL60())
    {
      res.push_back(PAL60_4x3);
      if (bCanDoWidescreen) res.push_back(PAL60_16x9);
    }
  }
  if (g_videoConfig.HasNTSC())
  {
    res.push_back(NTSC_4x3);
    if (bCanDoWidescreen) res.push_back(NTSC_16x9);
    if (g_videoConfig.Has480p())
    {
      res.push_back(HDTV_480p_4x3);
      if (bCanDoWidescreen) res.push_back(HDTV_480p_16x9);
    }
    if (g_videoConfig.Has720p())
      res.push_back(HDTV_720p);
    if (g_videoConfig.Has1080i())
      res.push_back(HDTV_1080i);
  }
}

#ifndef HAS_SDL
void CGraphicContext::SetVideoResolution(RESOLUTION &res, BOOL NeedZ, bool forceClear /* = false */)
{
  if (res == AUTORES)
  {
    res = g_videoConfig.GetBestMode();
  }
  if (!IsValidResolution(res))
  { // Choose a failsafe resolution that we can actually display
    CLog::Log(LOGERROR, "The screen resolution requested is not valid, resetting to a valid mode");
    res = g_videoConfig.GetSafeMode();
  }
  
  if (!m_pd3dParams)
  {
    m_Resolution = res;
    return ;
  }
  bool NeedReset = false;

  UINT interval = D3DPRESENT_INTERVAL_ONE;  
  //if( m_bFullScreenVideo )
  //  interval = D3DPRESENT_INTERVAL_IMMEDIATE;

#ifdef PROFILE
  interval = D3DPRESENT_INTERVAL_IMMEDIATE;
#endif

#ifndef HAS_XBOX_D3D
  interval = 0;
#endif

  if (interval != m_pd3dParams->FullScreen_PresentationInterval)
  {
    m_pd3dParams->FullScreen_PresentationInterval = interval;
    NeedReset = true;
  }


  if (NeedZ != m_pd3dParams->EnableAutoDepthStencil)
  {
    m_pd3dParams->EnableAutoDepthStencil = NeedZ;
    NeedReset = true;
  }
  if (m_Resolution != res)
  {
    NeedReset = true;
    m_pd3dParams->BackBufferWidth = g_settings.m_ResInfo[res].iWidth;
    m_pd3dParams->BackBufferHeight = g_settings.m_ResInfo[res].iHeight;
    m_pd3dParams->Flags = g_settings.m_ResInfo[res].dwFlags;
    m_pd3dParams->Flags |= D3DPRESENTFLAG_LOCKABLE_BACKBUFFER;

    if (res == HDTV_1080i || res == HDTV_720p || m_bFullScreenVideo)
      m_pd3dParams->BackBufferCount = 1;
    else
      m_pd3dParams->BackBufferCount = 2;

    if (res == PAL60_4x3 || res == PAL60_16x9)
    {
      if (m_pd3dParams->BackBufferWidth <= 720 && m_pd3dParams->BackBufferHeight <= 480)
      {
        m_pd3dParams->FullScreen_RefreshRateInHz = 60;
      }
      else
      {
        m_pd3dParams->FullScreen_RefreshRateInHz = 0;
      }
    }
    else
    {
      m_pd3dParams->FullScreen_RefreshRateInHz = 0;
    }
  }
  Lock();
  if (m_pd3dDevice)
  {
    if (NeedReset)
    {
      CLog::Log(LOGDEBUG, "Setting resolution %i", res);
      m_pd3dDevice->Reset(m_pd3dParams);
    }

    /* need to clear and preset, otherwise flicker filters won't take effect */
    if (NeedReset || forceClear)
    {
      m_pd3dDevice->Clear( 0L, NULL, D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER | D3D_CLEAR_STENCIL, 0x00010001, 1.0f, 0L );
      m_pd3dDevice->Present( NULL, NULL, NULL, NULL );
    }

    m_iScreenWidth = m_pd3dParams->BackBufferWidth;
    m_iScreenHeight = m_pd3dParams->BackBufferHeight;
    m_bWidescreen = (m_pd3dParams->Flags & D3DPRESENTFLAG_WIDESCREEN) != 0;
  }
  if ((g_settings.m_ResInfo[m_Resolution].iWidth != g_settings.m_ResInfo[res].iWidth) || (g_settings.m_ResInfo[m_Resolution].iHeight != g_settings.m_ResInfo[res].iHeight))
  { // set the mouse resolution
    g_Mouse.SetResolution(g_settings.m_ResInfo[res].iWidth, g_settings.m_ResInfo[res].iHeight, 1, 1);
  }

  SetFullScreenViewWindow(res);
  SetScreenFilters(m_bFullScreenVideo);
  
  m_Resolution = res;
  if(NeedReset)
    CLog::Log(LOGDEBUG, "We set resolution %i", m_Resolution);

  Unlock();  
}

#else
#ifdef __GNUC__
#warning Need to implement GraphicContext::SetVideoResolution
#endif
void CGraphicContext::SetVideoResolution(RESOLUTION &res, BOOL NeedZ, bool forceClear /* = false */)
{
  if (res == AUTORES)
  {
    res = g_videoConfig.GetBestMode();
  }
  if (!IsValidResolution(res))
  { // Choose a failsafe resolution that we can actually display
    CLog::Log(LOGERROR, "The screen resolution requested is not valid, resetting to a valid mode");
    res = g_videoConfig.GetSafeMode();
  }

  if (m_Resolution != res)
  {
    Lock();
    m_iScreenWidth = g_settings.m_ResInfo[res].iWidth;
    m_iScreenHeight = g_settings.m_ResInfo[res].iHeight;

#ifdef HAS_SDL_2D
    int options = SDL_HWSURFACE | SDL_DOUBLEBUF;
    if (g_advancedSettings.m_fullScreen) options |= SDL_FULLSCREEN;
    m_screenSurface = new CSurface(m_iScreenWidth, m_iScreenHeight, true, 0, 0, 0, (bool)g_advancedSettings.m_fullScreen);
    //m_screenSurface = SDL_SetVideoMode(m_iScreenWidth, m_iScreenHeight, 32, options);
#elif defined(HAS_SDL_OPENGL)
    int options = 0;
    if (g_advancedSettings.m_fullScreen) options |= SDL_FULLSCREEN;

    // Create a bare root window so that SDL Input handling works
#ifdef HAS_GLX
    static SDL_Surface* rootWindow = NULL;
    if (!rootWindow) 
    {
      rootWindow = SDL_SetVideoMode(m_iScreenWidth, m_iScreenHeight, 0,  options);
      // attach a GLX surface to the root window
      m_screenSurface = new CSurface(m_iScreenWidth, m_iScreenHeight, true, 0, 0, rootWindow);
      SDL_WM_SetCaption("XBox Media Center", NULL);
    } else {
      // FIXME: this doesn't work :(      
      //m_screenSurface->ReleaseContext();
      rootWindow = SDL_SetVideoMode(m_iScreenWidth, m_iScreenHeight, 0,  options);
      m_screenSurface->ResizeSurface(m_iScreenWidth, m_iScreenHeight);
      //m_screenSurface->MakeCurrent();
      
    }

#else
    m_screenSurface = new CSurface(m_iScreenWidth, m_iScreenHeight, true, 0, 0, 0);
#endif
    m_surfaces[SDL_ThreadID()] = m_screenSurface;

    glClearColor( 0.0f, 0.0f, 0.0f, 0.0f );

    glViewport(0, 0, m_iScreenWidth, m_iScreenHeight);
    glScissor(0, 0, m_iScreenWidth, m_iScreenHeight);
    glEnable(GL_TEXTURE_2D); 
    glEnable(GL_SCISSOR_TEST); 

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
 
    glOrtho(0.0f, m_iScreenWidth, m_iScreenHeight, 0.0f, -1.0f, 1.0f);

    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity(); 
    
    glBlendFunc(GL_SRC_ALPHA, GL_ONE);
    glEnable(GL_BLEND);          // Turn Blending On
    glDisable(GL_DEPTH_TEST);
    VerifyGLState();
#endif

    m_bWidescreen = (res == HDTV_1080i || res == HDTV_720p || res == PAL60_16x9 || 
                  	res == PAL_16x9 || res == NTSC_16x9);
  
    if ((g_settings.m_ResInfo[m_Resolution].iWidth != g_settings.m_ResInfo[res].iWidth) || (g_settings.m_ResInfo[m_Resolution].iHeight != g_settings.m_ResInfo[res].iHeight))
    { // set the mouse resolution
      g_Mouse.SetResolution(g_settings.m_ResInfo[res].iWidth, g_settings.m_ResInfo[res].iHeight, 1, 1);
    }
   
    SetFullScreenViewWindow(res);

    m_Resolution = res;
    Unlock();  
  }
}

#endif


RESOLUTION CGraphicContext::GetVideoResolution() const
{
  return m_Resolution;
}

void CGraphicContext::SetScreenFilters(bool useFullScreenFilters)
{
#ifdef HAS_XBOX_D3D
  Lock();
  if (m_pd3dDevice)
  {
    // These are only valid here and nowhere else
    // set soften on/off
    m_pd3dDevice->SetSoftDisplayFilter(useFullScreenFilters ? g_guiSettings.GetBool("videoplayer.soften") : g_guiSettings.GetBool("videoscreen.soften"));
    m_pd3dDevice->SetFlickerFilter(useFullScreenFilters ? g_guiSettings.GetInt("videoplayer.flicker") : g_guiSettings.GetInt("videoscreen.flickerfilter"));
  }
  Unlock();
#endif
}

void CGraphicContext::ResetOverscan(RESOLUTION res, OVERSCAN &overscan)
{
  overscan.left = 0;
  overscan.top = 0;
  switch (res)
  {
  case HDTV_1080i:
    overscan.right = 1920;
    overscan.bottom = 1080;
    break;
  case HDTV_720p:
    overscan.right = 1280;
    overscan.bottom = 720;
    break;
  case HDTV_480p_16x9:
  case HDTV_480p_4x3:
  case NTSC_16x9:
  case NTSC_4x3:
  case PAL60_16x9:
  case PAL60_4x3:
    overscan.right = 720;
    overscan.bottom = 480;
    break;
  case PAL_16x9:
  case PAL_4x3:
    overscan.right = 720;
    overscan.bottom = 576;
    break;
  }
}

void CGraphicContext::ResetScreenParameters(RESOLUTION res)
{
  ResetOverscan(res, g_settings.m_ResInfo[res].Overscan);
  g_settings.m_ResInfo[res].fPixelRatio = GetPixelRatio(res);
  // 1080i
  switch (res)
  {
  case HDTV_1080i:
    g_settings.m_ResInfo[res].iSubtitles = (int)(0.965 * 1080);
    g_settings.m_ResInfo[res].iWidth = 1920;
    g_settings.m_ResInfo[res].iHeight = 1080;
    g_settings.m_ResInfo[res].dwFlags = D3DPRESENTFLAG_INTERLACED | D3DPRESENTFLAG_WIDESCREEN;
    strcpy(g_settings.m_ResInfo[res].strMode, "1080i 16:9");
    break;
  case HDTV_720p:
    g_settings.m_ResInfo[res].iSubtitles = (int)(0.965 * 720);
    g_settings.m_ResInfo[res].iWidth = 1280;
    g_settings.m_ResInfo[res].iHeight = 720;
    g_settings.m_ResInfo[res].dwFlags = D3DPRESENTFLAG_PROGRESSIVE | D3DPRESENTFLAG_WIDESCREEN;
    strcpy(g_settings.m_ResInfo[res].strMode, "720p 16:9");
    break;
  case HDTV_480p_4x3:
    g_settings.m_ResInfo[res].iSubtitles = (int)(0.9 * 480);
    g_settings.m_ResInfo[res].iWidth = 720;
    g_settings.m_ResInfo[res].iHeight = 480;
    g_settings.m_ResInfo[res].dwFlags = D3DPRESENTFLAG_PROGRESSIVE;
    strcpy(g_settings.m_ResInfo[res].strMode, "480p 4:3");
    break;
  case HDTV_480p_16x9:
    g_settings.m_ResInfo[res].iSubtitles = (int)(0.965 * 480);
    g_settings.m_ResInfo[res].iWidth = 720;
    g_settings.m_ResInfo[res].iHeight = 480;
    g_settings.m_ResInfo[res].dwFlags = D3DPRESENTFLAG_PROGRESSIVE | D3DPRESENTFLAG_WIDESCREEN;
    strcpy(g_settings.m_ResInfo[res].strMode, "480p 16:9");
    break;
  case NTSC_4x3:
    g_settings.m_ResInfo[res].iSubtitles = (int)(0.9 * 480);
    g_settings.m_ResInfo[res].iWidth = 720;
    g_settings.m_ResInfo[res].iHeight = 480;
    g_settings.m_ResInfo[res].dwFlags = D3DPRESENTFLAG_INTERLACED;
    strcpy(g_settings.m_ResInfo[res].strMode, "NTSC 4:3");
    break;
  case NTSC_16x9:
    g_settings.m_ResInfo[res].iSubtitles = (int)(0.965 * 480);
    g_settings.m_ResInfo[res].iWidth = 720;
    g_settings.m_ResInfo[res].iHeight = 480;
    g_settings.m_ResInfo[res].dwFlags = D3DPRESENTFLAG_INTERLACED | D3DPRESENTFLAG_WIDESCREEN;
    strcpy(g_settings.m_ResInfo[res].strMode, "NTSC 16:9");
    break;
  case PAL_4x3:
    g_settings.m_ResInfo[res].iSubtitles = (int)(0.9 * 576);
    g_settings.m_ResInfo[res].iWidth = 720;
    g_settings.m_ResInfo[res].iHeight = 576;
    g_settings.m_ResInfo[res].dwFlags = D3DPRESENTFLAG_INTERLACED;
    strcpy(g_settings.m_ResInfo[res].strMode, "PAL 4:3");
    break;
  case PAL_16x9:
    g_settings.m_ResInfo[res].iSubtitles = (int)(0.965 * 576);
    g_settings.m_ResInfo[res].iWidth = 720;
    g_settings.m_ResInfo[res].iHeight = 576;
    g_settings.m_ResInfo[res].dwFlags = D3DPRESENTFLAG_INTERLACED | D3DPRESENTFLAG_WIDESCREEN;
    strcpy(g_settings.m_ResInfo[res].strMode, "PAL 16:9");
    break;
  case PAL60_4x3:
    g_settings.m_ResInfo[res].iSubtitles = (int)(0.9 * 480);
    g_settings.m_ResInfo[res].iWidth = 720;
    g_settings.m_ResInfo[res].iHeight = 480;
    g_settings.m_ResInfo[res].dwFlags = D3DPRESENTFLAG_INTERLACED;
    strcpy(g_settings.m_ResInfo[res].strMode, "PAL60 4:3");
    break;
  case PAL60_16x9:
    g_settings.m_ResInfo[res].iSubtitles = (int)(0.965 * 480);
    g_settings.m_ResInfo[res].iWidth = 720;
    g_settings.m_ResInfo[res].iHeight = 480;
    g_settings.m_ResInfo[res].dwFlags = D3DPRESENTFLAG_INTERLACED | D3DPRESENTFLAG_WIDESCREEN;
    strcpy(g_settings.m_ResInfo[res].strMode, "PAL60 16:9");
    break;
  }
}

float CGraphicContext::GetPixelRatio(RESOLUTION iRes) const
{
  if (iRes == HDTV_1080i || iRes == HDTV_720p) return 1.0f;
  if (iRes == HDTV_480p_4x3 || iRes == NTSC_4x3 || iRes == PAL60_4x3) return 4320.0f / 4739.0f;
  if (iRes == HDTV_480p_16x9 || iRes == NTSC_16x9 || iRes == PAL60_16x9) return 4320.0f / 4739.0f*4.0f / 3.0f;
  if (iRes == PAL_16x9) return 128.0f / 117.0f*4.0f / 3.0f;
  return 128.0f / 117.0f;
}

void CGraphicContext::Clear()
{
#ifndef HAS_SDL
  if (!m_pd3dDevice) return;
  //Not trying to clear the zbuffer when there is none is 7 fps faster (pal resolution)
  if ((!m_pd3dParams) || (m_pd3dParams->EnableAutoDepthStencil == TRUE))
    m_pd3dDevice->Clear( 0L, NULL, D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER | D3D_CLEAR_STENCIL, 0x00010001, 1.0f, 0L );
  else
    m_pd3dDevice->Clear( 0L, NULL, D3DCLEAR_TARGET, 0x00010001, 1.0f, 0L );
#elif defined(HAS_SDL_2D)
  SDL_FillRect(m_screenSurface->SDL(), NULL, 0x00010001);
#elif defined(HAS_SDL_OPENGL)
  GLVALIDATE;
  glClear(GL_COLOR_BUFFER_BIT); 
#endif    
}

void CGraphicContext::CaptureStateBlock()
{
#ifndef HAS_SDL
  if (m_stateBlock != 0xffffffff)
  {
    Get3DDevice()->DeleteStateBlock(m_stateBlock);
  }

  if (D3D_OK != Get3DDevice()->CreateStateBlock(D3DSBT_ALL, &m_stateBlock))
  {
    // Creation failure
    m_stateBlock = 0xffffffff;
  }
#endif  
}

void CGraphicContext::ApplyStateBlock()
{
#ifndef HAS_SDL
  if (m_stateBlock != 0xffffffff)
  {
    Get3DDevice()->ApplyStateBlock(m_stateBlock);
  }
#endif
}

void CGraphicContext::SetScalingResolution(RESOLUTION res, float posX, float posY, bool needsScaling)
{
  m_windowResolution = res;
  if (needsScaling)
  {
    // calculate necessary scalings
    float fFromWidth = (float)g_settings.m_ResInfo[res].iWidth;
    float fFromHeight = (float)g_settings.m_ResInfo[res].iHeight;
    float fToPosX = (float)g_settings.m_ResInfo[m_Resolution].Overscan.left;
    float fToPosY = (float)g_settings.m_ResInfo[m_Resolution].Overscan.top;
    float fToWidth = (float)g_settings.m_ResInfo[m_Resolution].Overscan.right - fToPosX;
    float fToHeight = (float)g_settings.m_ResInfo[m_Resolution].Overscan.bottom - fToPosY;

    // add additional zoom to compensate for any overskan built in skin
    float fZoom = g_SkinInfo.GetSkinZoom();

    if(!g_guiSkinzoom) // lookup gui setting if we didn't have it already
      g_guiSkinzoom = (CSettingInt*)g_guiSettings.GetSetting("lookandfeel.skinzoom");

    if(g_guiSkinzoom)
      fZoom *= (100 + g_guiSkinzoom->GetData()) * 0.01f;

    fZoom -= 1.0f;
    fToPosX -= fToWidth * fZoom * 0.5f;
    fToWidth *= fZoom + 1.0f;

    /* adjust for aspect ratio as zoom is given in the vertical direction and we don't /*
    /* do aspect ratio corrections in the gui code */
    fZoom = fZoom / g_settings.m_ResInfo[m_Resolution].fPixelRatio;
    fToPosY -= fToHeight * fZoom * 0.5f;
    fToHeight *= fZoom + 1.0f;
    
    m_windowScaleX = fToWidth / fFromWidth;
    m_windowScaleY = fToHeight / fFromHeight;
    TransformMatrix windowOffset = TransformMatrix::CreateTranslation(posX, posY);
    TransformMatrix guiScaler = TransformMatrix::CreateScaler(fToWidth / fFromWidth, fToHeight / fFromHeight);
    TransformMatrix guiOffset = TransformMatrix::CreateTranslation(fToPosX, fToPosY);
    m_guiTransform = guiOffset * guiScaler * windowOffset;
  }
  else
  {
    m_guiTransform = TransformMatrix::CreateTranslation(posX, posY);
    m_windowScaleX = 1.0f;
    m_windowScaleY = 1.0f;
  }
  // reset the final transform and window/group transforms
  while (m_groupTransform.size())
    m_groupTransform.pop();
  m_groupTransform.push(m_guiTransform);
  m_finalTransform = m_guiTransform;
}

void CGraphicContext::InvertFinalCoords(float &x, float &y) const
{
  m_finalTransform.InverseTransformPosition(x, y);
}

float CGraphicContext::GetScalingPixelRatio() const
{
  if (m_Resolution == m_windowResolution)
    return GetPixelRatio(m_windowResolution);

  RESOLUTION checkRes = m_windowResolution;
  if (checkRes == INVALID)
    checkRes = m_Resolution;
  // resolutions are different - we want to return the aspect ratio of the video resolution
  // but only once it's been corrected for the skin -> screen coordinates scaling
  float winWidth = (float)g_settings.m_ResInfo[checkRes].iWidth;
  float winHeight = (float)g_settings.m_ResInfo[checkRes].iHeight;
  float outWidth = (float)g_settings.m_ResInfo[m_Resolution].iWidth;
  float outHeight = (float)g_settings.m_ResInfo[m_Resolution].iHeight;
  float outPR = GetPixelRatio(m_Resolution);

  return outPR * (outWidth / outHeight) / (winWidth / winHeight);
}

int CGraphicContext::GetFPS() const
{
  if (m_Resolution == PAL_4x3 || m_Resolution == PAL_16x9)
    return 50;
  else if (m_Resolution == HDTV_1080i)
    return 30;
  return 60;
}

#ifdef HAS_SDL_2D
int CGraphicContext::BlitToScreen(SDL_Surface *src, SDL_Rect *srcrect, SDL_Rect *dstrect)
{
  return SDL_BlitSurface(src, srcrect, m_screenSurface->SDL(), dstrect);
}
#endif

#ifdef HAS_SDL_OPENGL
#ifdef  __GNUC__
#warning TODO CGraphicContext needs to cleanup unused surfaces
#endif
bool CGraphicContext::ValidateSurface(CSurface* dest)
{
  // FIXME: routine cleanup of unused surfaces
  map<Uint32, CSurface*>::iterator iter;
  Uint32 tid = SDL_ThreadID();
  iter = m_surfaces.find(tid);
  if (iter==m_surfaces.end()) {
#ifdef HAS_GLX
    if (dest==NULL)
    {
      CLog::Log(LOGDEBUG, "GL: Sharing screen surface for thread %ul", tid);
      CSurface* surface = new CSurface(m_screenSurface);
      if (!surface->MakeCurrent())
      {
	CLog::Log(LOGERROR, "GL: Error making context current");
	delete surface;
	return false;
      }
      m_surfaces[tid] = surface;
      return true;
    } else {
      m_surfaces[tid] = dest;
      dest->MakeCurrent();
    }
#else
    CLog::Log(LOGDEBUG, "Creating surface for thread %ul", tid);
    CSurface* surface = InitializeSurface();
    if (surface) 
    {
      m_surfaces[tid] = surface;
      return true;
    } else {
      CLog::Log(LOGERROR, "Did not get surface for thread %ul", tid);
      return false;
    }
#endif
  } else {
    (iter->second)->MakeCurrent();
  }
  return true;
}

CSurface* CGraphicContext::InitializeSurface()
{
  CSurface* screenSurface = NULL;
  Lock();

  screenSurface = new CSurface(m_iScreenWidth, m_iScreenHeight, true, m_screenSurface, m_screenSurface);
  if (!screenSurface || !screenSurface->IsValid()) 
  {
    CLog::Log(LOGERROR, "Surface creation error");
    if (screenSurface) 
    {
      delete screenSurface;
    }
    Unlock();
    return NULL;
  }
  glClearColor( 0.0f, 0.0f, 0.0f, 0.0f );
  
  glViewport(0, 0, m_iScreenWidth, m_iScreenHeight);
  glScissor(0, 0, m_iScreenWidth, m_iScreenHeight);
  glEnable(GL_TEXTURE_2D); 
  glEnable(GL_SCISSOR_TEST); 
  
  glMatrixMode(GL_PROJECTION);
  glLoadIdentity();
 
  glOrtho(0.0f, m_iScreenWidth, m_iScreenHeight, 0.0f, -1.0f, 1.0f);

  glMatrixMode(GL_MODELVIEW);
  glLoadIdentity(); 
  
  glBlendFunc(GL_SRC_ALPHA, GL_ONE);
  glEnable(GL_BLEND);          // Turn Blending On
  glDisable(GL_DEPTH_TEST);

  Unlock();
  return screenSurface;
}

#endif

void CGraphicContext::ReleaseCurrentContext(Surface::CSurface* ctx)
{
#ifdef HAS_SDL_OPENGL
  if (ctx)
  {
    Lock();
    ctx->ReleaseContext();
    Unlock();
    return;
  }
  Lock();
  map<Uint32, CSurface*>::iterator iter;
  Uint32 tid = SDL_ThreadID();
  iter = m_surfaces.find(tid);
  if (iter==m_surfaces.end()) 
  {
    m_screenSurface->ReleaseContext();
    Unlock();
    return;
  }
  (iter->second)->ReleaseContext();
  Unlock();
#endif
}

void CGraphicContext::AcquireCurrentContext(Surface::CSurface* ctx)
{
#ifdef HAS_SDL_OPENGL
  if (ctx)
  {
    Lock();
    if (!ctx->MakeCurrent())
    {
      CLog::Log(LOGERROR, "Error making context current");
    }
    Unlock();
    return;
  }
  Lock();
  map<Uint32, CSurface*>::iterator iter;
  Uint32 tid = SDL_ThreadID();
  iter = m_surfaces.find(tid);
  if (iter==m_surfaces.end()) 
  {
    Unlock();
    return;
  }
  if (!(iter->second)->MakeCurrent())
  {
    CLog::Log(LOGERROR, "Error making context current");
  }
  Unlock();
#endif
}


void CGraphicContext::BeginPaint(CSurface *dest)
{
#ifdef HAS_SDL_OPENGL
  Lock();
  ValidateSurface(dest);
  VerifyGLState();
#endif
}

void CGraphicContext::EndPaint(CSurface *dest)
{
#ifdef HAS_SDL_OPENGL
  Unlock();
  VerifyGLState();
#endif
}


