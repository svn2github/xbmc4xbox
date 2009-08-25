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

#include "include.h"
#include "GraphicContext.h"
#include "GUIFontManager.h"
#include "GUIMessage.h"
#include "IMsgSenderCallback.h"
#include "Settings.h"
#include "GUISettings.h"
#include "XBVideoConfig.h"
#include "TextureManager.h"
#include "../xbmc/utils/SingleLock.h"
#include "../xbmc/Application.h"
#include "cores/VideoRenderers/RenderManager.h"
#include "GUIAudioManager.h"
#include "WindowingFactory.h"

#include "SkinInfo.h"
#ifdef HAS_GLX
#include <X11/extensions/Xinerama.h>
#elif defined (__APPLE__)
#include "CocoaInterface.h"
#endif
#ifdef HAS_XRANDR
#include "XRandR.h"
#endif

using namespace std;

CGraphicContext g_graphicsContext;
extern bool g_fullScreen;

/* quick access to a skin setting, fine unless we starts clearing video settings */
static CSettingInt* g_guiSkinzoom = NULL;

CGraphicContext::CGraphicContext(void)
{
  m_iScreenWidth = 720;
  m_iScreenHeight = 576;
#ifdef HAS_GL
  m_maxTextureSize = 2048;
#endif
  m_dwID = 0;
  m_strMediaDir = "";
  m_bCalibrating = false;
  m_Resolution = INVALID;
  m_pCallback = NULL;
  m_guiScaleX = m_guiScaleY = 1.0f;
  m_windowResolution = INVALID;
  m_bFullScreenRoot = false;
}

CGraphicContext::~CGraphicContext(void)
{
}

bool CGraphicContext::SendMessage(DWORD message, DWORD senderID, DWORD destID, DWORD param1, DWORD param2)
{
  if (!m_pCallback) return false;
  CGUIMessage msg(message, senderID, destID, param1, param2);
  return m_pCallback->SendMessage(msg);
}

bool CGraphicContext::SendMessage(CGUIMessage& message)
{
  if (!m_pCallback) return false;
  return m_pCallback->SendMessage(message);
}

void CGraphicContext::setMessageSender(IMsgSenderCallback* pCallback)
{
  m_pCallback = pCallback;
}

void CGraphicContext::SetOrigin(float x, float y)
{
  if (m_origins.size())
    m_origins.push(CPoint(x,y) + m_origins.top());
  else
    m_origins.push(CPoint(x,y));
  
  AddTransform(TransformMatrix::CreateTranslation(x, y));
}

void CGraphicContext::RestoreOrigin()
{
  m_origins.pop();
  RemoveTransform();
}

// add a new clip region, intersecting with the previous clip region.
bool CGraphicContext::SetClipRegion(float x, float y, float w, float h)
{ // transform from our origin
  CPoint origin;
  if (m_origins.size())
    origin = m_origins.top();
  
  // ok, now intersect with our old clip region
  CRect rect(x, y, x + w, y + h);
  rect += origin;
  if (m_clipRegions.size())
  { 
    // intersect with original clip region
    rect.Intersect(m_clipRegions.top());
  }
  
  if (rect.IsEmpty())
    return false;
  
  m_clipRegions.push(rect);

  // here we could set the hardware clipping, if applicable
  return true;
}

void CGraphicContext::RestoreClipRegion()
{
  if (m_clipRegions.size())
    m_clipRegions.pop();

  // here we could reset the hardware clipping, if applicable
}

void CGraphicContext::ClipRect(CRect &vertex, CRect &texture, CRect *texture2)
{
  // this is the software clipping routine.  If the graphics hardware is set to do the clipping
  // (eg via SetClipPlane in D3D for instance) then this routine is unneeded.
  if (m_clipRegions.size())
  {
    // take a copy of the vertex rectangle and intersect
    // it with our clip region (moved to the same coordinate system)
    CRect clipRegion(m_clipRegions.top());
    if (m_origins.size())
      clipRegion -= m_origins.top();
    CRect original(vertex);
    vertex.Intersect(clipRegion);
    // and use the original to compute the texture coordinates
    if (original != vertex)
    {
      float scaleX = texture.Width() / original.Width();
      float scaleY = texture.Height() / original.Height();
      texture.x1 += (vertex.x1 - original.x1) * scaleX;
      texture.y1 += (vertex.y1 - original.y1) * scaleY;
      texture.x2 += (vertex.x2 - original.x2) * scaleX;
      texture.y2 += (vertex.y2 - original.y2) * scaleY;
      if (texture2)
      {
        scaleX = texture2->Width() / original.Width();
        scaleY = texture2->Height() / original.Height();
        texture2->x1 += (vertex.x1 - original.x1) * scaleX;
        texture2->y1 += (vertex.y1 - original.y1) * scaleY;
        texture2->x2 += (vertex.x2 - original.x2) * scaleX;
        texture2->y2 += (vertex.y2 - original.y2) * scaleY;
      }
    }
  }
}

bool CGraphicContext::SetViewPort(float fx, float fy, float fwidth, float fheight, bool intersectPrevious /* = false */)
{
#ifdef HAS_DX
  D3DVIEWPORT9 newviewport;
  D3DVIEWPORT9 *oldviewport = new D3DVIEWPORT9;
  Get3DDevice()->GetViewport(oldviewport);
#elif defined(HAS_GL)
  //GLVALIDATE;
  GLint newviewport[4];
  GLint oldviewport[4];
  glGetIntegerv(GL_SCISSOR_BOX, &oldviewport[0]);
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
    float z = 0;
    ScaleFinalCoords(x[i], y[i], z);
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
#ifdef HAS_DX
    int oldLeft = (int)oldviewport->X;
    int oldTop = (int)oldviewport->Y;
    int oldRight = (int)oldviewport->X + oldviewport->Width;
    int oldBottom = (int)oldviewport->Y + oldviewport->Height;
#elif defined(HAS_SDL_2D)
    int oldLeft = (int)oldviewport->x;
    int oldTop = (int)oldviewport->y;
    int oldRight = (int)oldviewport->x + oldviewport->w;
    int oldBottom = (int)oldviewport->y + oldviewport->h;
#elif defined(HAS_GL)
    int oldLeft = (int)oldviewport[0];
    int oldBottom = m_iScreenHeight - oldviewport[1];       // opengl uses bottomleft as origin
    int oldTop = oldBottom - oldviewport[3];
    int oldRight = (int)oldviewport[0] + oldviewport[2];
#endif
    if (newLeft >= oldRight || newTop >= oldBottom || newRight <= oldLeft || newBottom <= oldTop)
    { // empty intersection - return false to indicate no rendering should occur
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
    return false;
  }
  // intersection with the screen
  if (newLeft < 0) newLeft = 0;
  if (newTop < 0) newTop = 0;
  if (newRight > m_iScreenWidth) newRight = m_iScreenWidth;
  if (newBottom > m_iScreenHeight) newBottom = m_iScreenHeight;

  ASSERT(newLeft < newRight);
  ASSERT(newTop < newBottom);

#ifdef HAS_DX
  newviewport.MinZ = 0.0f;
  newviewport.MaxZ = 1.0f;
  newviewport.X = newLeft;
  newviewport.Y = newTop;
  newviewport.Width = newRight - newLeft;
  newviewport.Height = newBottom - newTop;
  m_pd3dDevice->SetViewport(&newviewport);
#elif defined(HAS_GL)
  newviewport[0] = newLeft;
  newviewport[1] = m_iScreenHeight - newBottom; // opengl uses bottomleft as origin
  newviewport[2] = newRight - newLeft;
  newviewport[3] = newBottom - newTop;
  glScissor(newviewport[0], newviewport[1], newviewport[2], newviewport[3]);
  glViewport(newviewport[0], newviewport[1], newviewport[2], newviewport[3]);
  VerifyGLState();
#endif

  m_viewStack.push(oldviewport);

  UpdateCameraPosition(m_cameras.top());
  return true;
}

void CGraphicContext::RestoreViewPort()
{
  if (!m_viewStack.size()) return;
#ifdef HAS_DX
  D3DVIEWPORT9 *oldviewport = (D3DVIEWPORT9*)m_viewStack.top();
  Get3DDevice()->SetViewport(oldviewport);
#elif defined(HAS_GL)
  //GLVALIDATE;
  GLint* oldviewport = (GLint*)m_viewStack.top();
  glScissor(oldviewport[0], oldviewport[1], oldviewport[2], oldviewport[3]);
  glViewport(oldviewport[0], oldviewport[1], oldviewport[2], oldviewport[3]);
  VerifyGLState();
#endif

  m_viewStack.pop();

#if defined(HAS_GL)
  delete [] oldviewport;
#else
  delete oldviewport;
#endif

  UpdateCameraPosition(m_cameras.top());
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
  }
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
  res.clear();
  if (g_videoConfig.GetNumberOfResolutions())
  {
    res.push_back(CUSTOM);
  }
  res.push_back(DESKTOP);
}

void CGraphicContext::SetVideoResolution(RESOLUTION &res, BOOL NeedZ, bool forceClear /* = false */)
{
  RESOLUTION lastRes = m_Resolution;
  
  // If the user asked us to guess, go with desktop
  if (res == AUTORES || !IsValidResolution(res))
  {
    res = DESKTOP;
  }

  // If we are switching to the same resolution and same window/full-screen, no need to do anything
  if (res == lastRes && m_bFullScreenRoot == g_advancedSettings.m_fullScreen)
  {
    return;
  }
  
  if (res >= DESKTOP || g_advancedSettings.m_startFullScreen)
  {
    g_advancedSettings.m_fullScreen = true;
    m_bFullScreenRoot = true;
  }
  else
  {
    g_advancedSettings.m_fullScreen = false;
    m_bFullScreenRoot = false;
  }

  Lock();
  
  m_iScreenWidth  = g_settings.m_ResInfo[res].iWidth;
  m_iScreenHeight = g_settings.m_ResInfo[res].iHeight;
  m_Resolution    = res;
  
  if (g_advancedSettings.m_fullScreen)
  {
    SetFullScreenRoot(true);
  }
  else if (lastRes >= DESKTOP )
  {
    SetFullScreenRoot(false);
  }
  else
  {
    g_Windowing.ResizeWindow(m_iScreenWidth, m_iScreenHeight, -1, -1);
  }

  // set the mouse resolution
  g_Mouse.SetResolution(g_settings.m_ResInfo[res].iWidth, g_settings.m_ResInfo[res].iHeight, 1, 1);
  g_fontManager.ReloadTTFFonts();

  // elis
  // restore vsync mode
  g_videoConfig.SetVSyncMode((VSYNC)g_guiSettings.GetInt("videoscreen.vsync"));

  SetFullScreenViewWindow(res);
  m_Resolution = res;

  Unlock();
}

RESOLUTION CGraphicContext::GetVideoResolution() const
{
  return m_Resolution;
}

void CGraphicContext::ResetOverscan(RESOLUTION_INFO &res)
{
  res.Overscan.left = 0;
  res.Overscan.top = 0;
  res.Overscan.right = res.iWidth;
  res.Overscan.bottom = res.iHeight;
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
  default:
    overscan.right = g_settings.m_ResInfo[res].iWidth;
    overscan.bottom = g_settings.m_ResInfo[res].iHeight;
    break;
  }
}

void CGraphicContext::ResetScreenParameters(RESOLUTION res)
{
  // For now these are all on the first screen.
  g_settings.m_ResInfo[res].iScreen = PRIMARY_MONITOR;
  static const float fOptimalSwitchPoint = 8.0f / (3.0f*sqrt(3.0f)); // see XboxRenderer.cpp

  // 1080i
  switch (res)
  {
  case HDTV_1080i:
    g_settings.m_ResInfo[res].iSubtitles = (int)(0.965 * 1080);
    g_settings.m_ResInfo[res].iWidth = 1920;
    g_settings.m_ResInfo[res].iHeight = 1080;
    g_settings.m_ResInfo[res].dwFlags = D3DPRESENTFLAG_INTERLACED | D3DPRESENTFLAG_WIDESCREEN;
    g_settings.m_ResInfo[res].fPixelRatio = 1.0f;
    strcpy(g_settings.m_ResInfo[res].strMode, "1080i 16:9");
    break;
  case HDTV_720p:
    g_settings.m_ResInfo[res].iSubtitles = (int)(0.965 * 720);
    g_settings.m_ResInfo[res].iWidth = 1280;
    g_settings.m_ResInfo[res].iHeight = 720;
    g_settings.m_ResInfo[res].dwFlags = D3DPRESENTFLAG_PROGRESSIVE | D3DPRESENTFLAG_WIDESCREEN;
    g_settings.m_ResInfo[res].fPixelRatio = 1.0f;
    strcpy(g_settings.m_ResInfo[res].strMode, "720p 16:9");
    break;
  case HDTV_480p_4x3:
    g_settings.m_ResInfo[res].iSubtitles = (int)(0.9 * 480);
    g_settings.m_ResInfo[res].iWidth = 720;
    g_settings.m_ResInfo[res].iHeight = 480;
    g_settings.m_ResInfo[res].dwFlags = D3DPRESENTFLAG_PROGRESSIVE;
    g_settings.m_ResInfo[res].fPixelRatio = 4320.0f / 4739.0f;
    strcpy(g_settings.m_ResInfo[res].strMode, "480p 4:3");
    break;
  case HDTV_480p_16x9:
    g_settings.m_ResInfo[res].iSubtitles = (int)(0.965 * 480);
    g_settings.m_ResInfo[res].iWidth = 720;
    g_settings.m_ResInfo[res].iHeight = 480;
    g_settings.m_ResInfo[res].dwFlags = D3DPRESENTFLAG_PROGRESSIVE | D3DPRESENTFLAG_WIDESCREEN;
    g_settings.m_ResInfo[res].fPixelRatio = 4320.0f / 4739.0f*4.0f / 3.0f;
    strcpy(g_settings.m_ResInfo[res].strMode, "480p 16:9");
    break;
  case NTSC_4x3:
    g_settings.m_ResInfo[res].iSubtitles = (int)(0.9 * 480);
    g_settings.m_ResInfo[res].iWidth = 720;
    g_settings.m_ResInfo[res].iHeight = 480;
    g_settings.m_ResInfo[res].dwFlags = D3DPRESENTFLAG_INTERLACED;
    g_settings.m_ResInfo[res].fPixelRatio = 4320.0f / 4739.0f;
    strcpy(g_settings.m_ResInfo[res].strMode, "NTSC 4:3");
    break;
  case NTSC_16x9:
    g_settings.m_ResInfo[res].iSubtitles = (int)(0.965 * 480);
    g_settings.m_ResInfo[res].iWidth = 720;
    g_settings.m_ResInfo[res].iHeight = 480;
    g_settings.m_ResInfo[res].dwFlags = D3DPRESENTFLAG_INTERLACED | D3DPRESENTFLAG_WIDESCREEN;
    g_settings.m_ResInfo[res].fPixelRatio = 4320.0f / 4739.0f*4.0f / 3.0f;
    strcpy(g_settings.m_ResInfo[res].strMode, "NTSC 16:9");
    break;
  case PAL_4x3:
    g_settings.m_ResInfo[res].iSubtitles = (int)(0.9 * 576);
    g_settings.m_ResInfo[res].iWidth = 720;
    g_settings.m_ResInfo[res].iHeight = 576;
    g_settings.m_ResInfo[res].dwFlags = D3DPRESENTFLAG_INTERLACED;
    g_settings.m_ResInfo[res].fPixelRatio = 128.0f / 117.0f;
    strcpy(g_settings.m_ResInfo[res].strMode, "PAL 4:3");
    break;
  case PAL_16x9:
    g_settings.m_ResInfo[res].iSubtitles = (int)(0.965 * 576);
    g_settings.m_ResInfo[res].iWidth = 720;
    g_settings.m_ResInfo[res].iHeight = 576;
    g_settings.m_ResInfo[res].dwFlags = D3DPRESENTFLAG_INTERLACED | D3DPRESENTFLAG_WIDESCREEN;
    g_settings.m_ResInfo[res].fPixelRatio = 128.0f / 117.0f*4.0f / 3.0f;
    strcpy(g_settings.m_ResInfo[res].strMode, "PAL 16:9");
    break;
  case PAL60_4x3:
    g_settings.m_ResInfo[res].iSubtitles = (int)(0.9 * 480);
    g_settings.m_ResInfo[res].iWidth = 720;
    g_settings.m_ResInfo[res].iHeight = 480;
    g_settings.m_ResInfo[res].dwFlags = D3DPRESENTFLAG_INTERLACED;
    g_settings.m_ResInfo[res].fPixelRatio = 4320.0f / 4739.0f;
    strcpy(g_settings.m_ResInfo[res].strMode, "PAL60 4:3");
    break;
  case PAL60_16x9:
    g_settings.m_ResInfo[res].iSubtitles = (int)(0.965 * 480);
    g_settings.m_ResInfo[res].iWidth = 720;
    g_settings.m_ResInfo[res].iHeight = 480;
    g_settings.m_ResInfo[res].dwFlags = D3DPRESENTFLAG_INTERLACED | D3DPRESENTFLAG_WIDESCREEN;
    g_settings.m_ResInfo[res].fPixelRatio = 4320.0f / 4739.0f*4.0f / 3.0f;
    strcpy(g_settings.m_ResInfo[res].strMode, "PAL60 16:9");
    break;
  case DESKTOP:
    g_videoConfig.GetCurrentResolution(g_settings.m_ResInfo[res]);
    g_settings.m_ResInfo[res].iSubtitles = (int)(0.965 * g_settings.m_ResInfo[res].iHeight);
    if(g_settings.m_ResInfo[res].fRefreshRate)
      snprintf(g_settings.m_ResInfo[res].strMode
            , sizeof(g_settings.m_ResInfo[res].strMode)
            , "%dx%d @ %.2fHz (Full Screen)"
            , g_settings.m_ResInfo[res].iWidth
            , g_settings.m_ResInfo[res].iHeight
            , g_settings.m_ResInfo[res].fRefreshRate);
    else
      snprintf(g_settings.m_ResInfo[res].strMode
            , sizeof(g_settings.m_ResInfo[res].strMode)
            , "%dx%d (Full Screen)"
            , g_settings.m_ResInfo[res].iWidth
            , g_settings.m_ResInfo[res].iHeight);
    if ((float)g_settings.m_ResInfo[res].iWidth/(float)g_settings.m_ResInfo[res].iHeight >= fOptimalSwitchPoint)
      g_settings.m_ResInfo[res].dwFlags = D3DPRESENTFLAG_WIDESCREEN;
    g_settings.m_ResInfo[res].fPixelRatio = 1.0f;
    break;
  case WINDOW:
    {
      RESOLUTION_INFO info = {};
      g_videoConfig.GetCurrentResolution(info);
      g_settings.m_ResInfo[res] = g_settings.m_ResInfo[PAL60_4x3];
      g_settings.m_ResInfo[res].fPixelRatio = 1.0f;
      g_settings.m_ResInfo[res].fRefreshRate = info.fRefreshRate;
    }
    break;
  default:
    break;
  }
  ResetOverscan(res, g_settings.m_ResInfo[res].Overscan);
}

// TODO: Why do we set a pixel ratio of 1.0 if the
//       user is not running fullscreen?
float CGraphicContext::GetPixelRatio(RESOLUTION iRes) const
{
  if (!m_bFullScreenRoot)
    return 1.0f;

  return g_settings.m_ResInfo[iRes].fPixelRatio;
}

void CGraphicContext::Clear()
{
  g_Windowing.ClearBuffers(0, 0, 0, 0);
}

void CGraphicContext::CaptureStateBlock()
{
  g_Windowing.CaptureStateBlock();
}

void CGraphicContext::ApplyStateBlock()
{
  g_Windowing.ApplyStateBlock();
}

void CGraphicContext::SetScalingResolution(RESOLUTION res, float posX, float posY, bool needsScaling)
{
  Lock();
  m_windowResolution = res;
  if (needsScaling && m_Resolution != INVALID)
  {
    // calculate necessary scalings
    float fFromWidth;
    float fFromHeight;
    float fToPosX;
    float fToPosY;
    float fToWidth;
    float fToHeight;

    {
      fFromWidth = (float)g_settings.m_ResInfo[res].iWidth;
      fFromHeight = (float)g_settings.m_ResInfo[res].iHeight;
      fToPosX = (float)g_settings.m_ResInfo[m_Resolution].Overscan.left;
      fToPosY = (float)g_settings.m_ResInfo[m_Resolution].Overscan.top;
      fToWidth = (float)g_settings.m_ResInfo[m_Resolution].Overscan.right - fToPosX;
      fToHeight = (float)g_settings.m_ResInfo[m_Resolution].Overscan.bottom - fToPosY;
    }

    // add additional zoom to compensate for any overskan built in skin
    float fZoom = g_SkinInfo.GetSkinZoom();

    if(!g_guiSkinzoom) // lookup gui setting if we didn't have it already
      g_guiSkinzoom = (CSettingInt*)g_guiSettings.GetSetting("lookandfeel.skinzoom");

    if(g_guiSkinzoom)
      fZoom *= (100 + g_guiSkinzoom->GetData()) * 0.01f;

    fZoom -= 1.0f;
    fToPosX -= fToWidth * fZoom * 0.5f;
    fToWidth *= fZoom + 1.0f;

    // adjust for aspect ratio as zoom is given in the vertical direction and we don't
    // do aspect ratio corrections in the gui code
    fZoom = fZoom / g_settings.m_ResInfo[m_Resolution].fPixelRatio;
    fToPosY -= fToHeight * fZoom * 0.5f;
    fToHeight *= fZoom + 1.0f;

    m_guiScaleX = fFromWidth / fToWidth;
    m_guiScaleY = fFromHeight / fToHeight;
    TransformMatrix windowOffset = TransformMatrix::CreateTranslation(posX, posY);
    TransformMatrix guiScaler = TransformMatrix::CreateScaler(fToWidth / fFromWidth, fToHeight / fFromHeight, fToHeight / fFromHeight);
    TransformMatrix guiOffset = TransformMatrix::CreateTranslation(fToPosX, fToPosY);
    m_guiTransform = guiOffset * guiScaler * windowOffset;
  }
  else
  {
    m_guiTransform = TransformMatrix::CreateTranslation(posX, posY);
    m_guiScaleX = 1.0f;
    m_guiScaleY = 1.0f;
  }
  // reset our origin and camera
  while (m_origins.size())
    m_origins.pop();
  m_origins.push(CPoint(posX, posY));
  while (m_cameras.size())
    m_cameras.pop();
  m_cameras.push(CPoint(0.5f*m_iScreenWidth, 0.5f*m_iScreenHeight));

  // and reset the final transform
  UpdateFinalTransform(m_guiTransform);
  Unlock();
}

void CGraphicContext::SetRenderingResolution(RESOLUTION res, float posX, float posY, bool needsScaling)
{
  Lock();
  SetScalingResolution(res, posX, posY, needsScaling);
  UpdateCameraPosition(m_cameras.top());
  Unlock();
}

void CGraphicContext::UpdateFinalTransform(const TransformMatrix &matrix)
{
  m_finalTransform = matrix;
  // We could set the world transform here to GPU-ize the animation system.
  // trouble is that we require the resulting x,y coords to be rounded to
  // the nearest pixel (vertex shader perhaps?)
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

void CGraphicContext::SetCameraPosition(const CPoint &camera)
{
  // offset the camera from our current location (this is in XML coordinates) and scale it up to
  // the screen resolution
  CPoint cam(camera);
  if (m_origins.size())
    cam += m_origins.top();

  RESOLUTION windowRes = (m_windowResolution == INVALID) ? m_Resolution : m_windowResolution;
  cam.x *= (float)m_iScreenWidth / g_settings.m_ResInfo[windowRes].iWidth;
  cam.y *= (float)m_iScreenHeight / g_settings.m_ResInfo[windowRes].iHeight;

  m_cameras.push(cam);
  UpdateCameraPosition(m_cameras.top());
}

void CGraphicContext::RestoreCameraPosition()
{ // remove the top camera from the stack
  ASSERT(m_cameras.size());
  m_cameras.pop();
  UpdateCameraPosition(m_cameras.top());
}

// NOTE: This routine is currently called (twice) every time there is a <camera>
//       tag in the skin.  It actually only has to be called before we render
//       something, so another option is to just save the camera coordinates
//       and then have a routine called before every draw that checks whether
//       the camera has changed, and if so, changes it.  Similarly, it could set
//       the world transform at that point as well (or even combine world + view
//       to cut down on one setting)
void CGraphicContext::UpdateCameraPosition(const CPoint &camera)
{
  g_Windowing.SetCameraPosition(camera, m_iScreenWidth, m_iScreenHeight);
}

bool CGraphicContext::RectIsAngled(float x1, float y1, float x2, float y2) const
{ // need only test 3 points, as they must be co-planer
  if (m_finalTransform.TransformZCoord(x1, y1, 0)) return true;
  if (m_finalTransform.TransformZCoord(x2, y2, 0)) return true;
  if (m_finalTransform.TransformZCoord(x1, y2, 0)) return true;
  return false;
}

float CGraphicContext::GetFPS() const
{
  if (g_settings.m_ResInfo[m_Resolution].fRefreshRate > 0)
    return g_settings.m_ResInfo[m_Resolution].fRefreshRate;
  if (m_Resolution == PAL_4x3 || m_Resolution == PAL_16x9)
    return 50.0f;
  if (m_Resolution == HDTV_1080i)
    return 30.0f;
  return 60.0f;
}

void CGraphicContext::BeginPaint(bool lock)
{
  if (lock) Lock();
}

void CGraphicContext::EndPaint(bool lock)
{
  if (lock) Unlock();
}

bool CGraphicContext::IsFullScreenRoot () const
{
  return m_bFullScreenRoot;
}

bool CGraphicContext::ToggleFullScreenRoot ()
{
  static RESOLUTION desktopres = DESKTOP;
  static RESOLUTION windowres = WINDOW;
  static RESOLUTION lastres = INVALID;

  if (m_bFullScreenRoot)
  {
    lastres = GetVideoResolution();
    SetVideoResolution(windowres);
  }
  else
  {
    if (lastres != INVALID)
    {
      SetVideoResolution(lastres);
    }
    else
    {
      SetVideoResolution(desktopres);
    }
  }

  return  m_bFullScreenRoot;
}

void CGraphicContext::SetFullScreenRoot(bool fs)
{
  int blanking = g_guiSettings.GetInt("videoscreen.displayblanking");
  bool blankOtherDisplays = (blanking == BLANKING_ALL_DISPLAYS);

  if (fs)
  {
    m_iFullScreenWidth = m_iScreenWidth;
    m_iFullScreenHeight = m_iScreenHeight;

    g_Windowing.SetFullScreen(true, 0, m_iFullScreenWidth, m_iFullScreenHeight, blankOtherDisplays, g_advancedSettings.m_alwaysOnTop);
    
    g_fontManager.ReloadTTFFonts();
    g_Mouse.SetResolution(m_iFullScreenWidth, m_iFullScreenHeight, 1, 1);
  }
  else
  {
    g_Windowing.SetFullScreen(false, 0, m_iFullScreenWidth, m_iFullScreenHeight, blankOtherDisplays, g_advancedSettings.m_alwaysOnTop);
    g_fontManager.ReloadTTFFonts();
    g_Mouse.SetResolution(g_settings.m_ResInfo[m_Resolution].iWidth, g_settings.m_ResInfo[m_Resolution].iHeight, 1, 1);
  }

  g_renderManager.Recover();

  m_bFullScreenRoot = fs;
  g_advancedSettings.m_fullScreen = fs;
  SetFullScreenViewWindow(m_Resolution);
}

void CGraphicContext::SetMediaDir(const CStdString &strMediaDir)
{
  g_TextureManager.SetTexturePath(strMediaDir);
  m_strMediaDir = strMediaDir;
}

void CGraphicContext::Flip()
{
  g_Windowing.PresentRender();
}

void CGraphicContext::ApplyHardwareTransform()
{
  g_Windowing.ApplyHardwareTransform(m_finalTransform);
}

void CGraphicContext::RestoreHardwareTransform()
{
  g_Windowing.RestoreHardwareTransform();
}

void CGraphicContext::NotifyAppFocusChange(bool bGaining)
{
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
#else
#ifdef  __GNUC__
// TODO: CGraphicContext::ClipToViewWindow not implemented
#endif
#endif
}
