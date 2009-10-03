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


#include "system.h"

#ifdef HAS_GL

#include "GraphicContext.h"
#include "AdvancedSettings.h"
#include "RenderSystemGL.h"
#include "utils/log.h"


CRenderSystemGL::CRenderSystemGL() : CRenderSystemBase()
{
  m_enumRenderingSystem = RENDERING_SYSTEM_OPENGL;
}

CRenderSystemGL::~CRenderSystemGL()
{
  DestroyRenderSystem();
}

bool CRenderSystemGL::InitRenderSystem()
{
  m_bVSync = false;
  m_iVSyncMode = 0;
  m_iSwapStamp = 0;
  m_iSwapTime = 0;
  m_iSwapRate = 0;
  m_bVsyncInit = false;
  m_maxTextureSize = 2048;
  
  // init glew library
  GLenum err = glewInit();
  if (GLEW_OK != err)
  {
    // Problem: glewInit failed, something is seriously wrong
    return false;
  }
 
  // Get the GL version number 
  m_RenderVerdenVersionMajor = 0;
  m_RenderVerdenVersionMinor = 0;

  const char* ver = (const char*)glGetString(GL_VERSION);
  if (ver != 0)
    sscanf(ver, "%d.%d", &m_RenderVerdenVersionMajor, &m_RenderVerdenVersionMinor);
  
  // Check if we need DPOT  
  m_NeedPower2Texture = true;
  if (GLEW_ARB_texture_non_power_of_two)
    m_NeedPower2Texture = false;
  
  // Get our driver vendor and renderer
  m_RenderVendor = (const char*) glGetString(GL_VENDOR);
  m_RenderRenderer = (const char*) glGetString(GL_RENDERER);
  
  LogGraphicsInfo();
  
  m_bRenderCreated = true;
  
  return true;
}

bool CRenderSystemGL::ResetRenderSystem(int width, int height)
{
  m_width = width;
  m_height = height;
  
  glClearColor( 0.0f, 0.0f, 0.0f, 0.0f );
  
  CalculateMaxTexturesize();
  
  glViewport(0, 0, width, height);
  glScissor(0, 0, width, height);

  glEnable(GL_TEXTURE_2D); 
  glEnable(GL_SCISSOR_TEST); 

  glMatrixMode(GL_PROJECTION);
  glLoadIdentity();

  glOrtho(0.0f, width-1, height-1, 0.0f, -1.0f, 1.0f);

  glMatrixMode(GL_MODELVIEW);
  glLoadIdentity(); 
  
  glBlendFunc(GL_SRC_ALPHA, GL_ONE);
  glEnable(GL_BLEND);          // Turn Blending On
  glDisable(GL_DEPTH_TEST);  
    
  return true;
}

bool CRenderSystemGL::DestroyRenderSystem()
{
  m_bRenderCreated = false;

  return true;
}

bool CRenderSystemGL::BeginRender()
{
  if (!m_bRenderCreated)
    return false;

  return true;
}

bool CRenderSystemGL::EndRender()
{
  if (!m_bRenderCreated)
    return false;

  return true;
}

bool CRenderSystemGL::ClearBuffers(color_t color)
{
  if (!m_bRenderCreated)
    return false;

  return true;
}

bool CRenderSystemGL::ClearBuffers(float r, float g, float b, float a)
{
  if (!m_bRenderCreated)
    return false;
  
  glClearColor(r, g, b, a);

  GLbitfield flags = GL_COLOR_BUFFER_BIT;
  glClear(flags);

  return true;
}

bool CRenderSystemGL::IsExtSupported(const char* extension)
{
  return false;
}

static int64_t abs(int64_t a)
{
  if(a < 0)
    return -a;
  return a;
}

bool CRenderSystemGL::PresentRender()
{
  if (!m_bRenderCreated)
    return false;

  if (m_iVSyncMode != 0 && m_iSwapRate != 0) 
  {
    int64_t curr, diff, freq;
    QueryPerformanceCounter((LARGE_INTEGER*)&curr);
    QueryPerformanceFrequency((LARGE_INTEGER*)&freq);

    if(m_iSwapStamp == 0)
      m_iSwapStamp = curr;

    /* calculate our next swap timestamp */
    diff = curr - m_iSwapStamp;
    diff = m_iSwapRate - diff % m_iSwapRate;
    m_iSwapStamp = curr + diff;

    /* sleep as close as we can before, assume 1ms precision of sleep *
     * this should always awake so that we are guaranteed the given   *
     * m_iSwapTime to do our swap                                     */
    diff = (diff - m_iSwapTime) * 1000 / freq;
    if (diff > 0)
      Sleep((DWORD)diff);
  }
  
  bool result = PresentRenderImpl();
  
  if (m_iVSyncMode && m_iSwapRate != 0)
  {
    int64_t curr, diff;
    QueryPerformanceCounter((LARGE_INTEGER*)&curr);

    diff = curr - m_iSwapStamp;
    m_iSwapStamp = curr;

    if (abs(diff - m_iSwapRate) < abs(diff))
      CLog::Log(LOGDEBUG, "%s - missed requested swap",__FUNCTION__);
  }
  
  return result;
}

void CRenderSystemGL::SetVSync(bool enable)
{
  if (m_bVSync==enable && m_bVsyncInit == true)
    return;

  if (!m_bRenderCreated)
    return;
  
  if (enable)
    CLog::Log(LOGINFO, "GL: Enabling VSYNC");
  else
    CLog::Log(LOGINFO, "GL: Disabling VSYNC");

  m_iVSyncMode   = 0;
  m_iVSyncErrors = 0;
  m_iSwapRate    = 0;
  m_bVSync       = enable;
  m_bVsyncInit   = true;

  SetVSyncImpl(enable);
  
  if (!enable)
    return;

  if (g_advancedSettings.m_ForcedSwapTime != 0.0)
  {
    /* some hardware busy wait on swap/glfinish, so we must manually sleep to avoid 100% cpu */
    double rate = g_graphicsContext.GetFPS();
    if (rate <= 0.0 || rate > 1000.0)
    {
      CLog::Log(LOGWARNING, "Unable to determine a valid horizontal refresh rate, vsync workaround disabled %.2g", rate);
      m_iSwapRate = 0;
    }
    else
    {
      int64_t freq;
      QueryPerformanceFrequency((LARGE_INTEGER*)&freq);
      m_iSwapRate   = (int64_t)((double)freq / rate);
      m_iSwapTime   = (int64_t)(0.001 * g_advancedSettings.m_ForcedSwapTime * freq);
      m_iSwapStamp  = 0;
      CLog::Log(LOGINFO, "GL: Using artificial vsync sleep with rate %f", rate);
      if(!m_iVSyncMode)
        m_iVSyncMode = 1;
    }
  }
    
  if (!m_iVSyncMode)
    CLog::Log(LOGERROR, "GL: Vertical Blank Syncing unsupported");
  else
    CLog::Log(LOGINFO, "GL: Selected vsync mode %d", m_iVSyncMode);  
}

void CRenderSystemGL::CaptureStateBlock()
{
  if (!m_bRenderCreated)
    return;
  
  glMatrixMode(GL_PROJECTION);
  glPushMatrix();
  glMatrixMode(GL_TEXTURE);
  glPushMatrix();
  glMatrixMode(GL_MODELVIEW);
  glPushMatrix();
  glDisable(GL_SCISSOR_TEST); // fixes FBO corruption on Macs
  if (glActiveTextureARB)
    glActiveTextureARB(GL_TEXTURE0_ARB);
  glDisable(GL_TEXTURE_2D);
  glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
  glColor3f(1.0, 1.0, 1.0);  
}

void CRenderSystemGL::ApplyStateBlock()
{
  if (!m_bRenderCreated)
    return;
  
  glMatrixMode(GL_PROJECTION);
  glPopMatrix();
  glMatrixMode(GL_TEXTURE);
  glPopMatrix();
  glMatrixMode(GL_MODELVIEW);
  glPopMatrix();
  if (glActiveTextureARB)
    glActiveTextureARB(GL_TEXTURE0_ARB);
  glEnable(GL_TEXTURE_2D);
  glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
  glEnable(GL_BLEND);
  glEnable(GL_SCISSOR_TEST);  
}

void CRenderSystemGL::SetCameraPosition(const CPoint &camera, int screenWidth, int screenHeight)
{ 
  if (!m_bRenderCreated)
    return;
  
  g_graphicsContext.BeginPaint();
  
  CPoint offset = camera - CPoint(screenWidth*0.5f, screenHeight*0.5f);
  
  GLint viewport[4];
  glGetIntegerv(GL_VIEWPORT, viewport);

  float w = (float)viewport[2]*0.5f;
  float h = (float)viewport[3]*0.5f;

  glMatrixMode(GL_MODELVIEW);
  glLoadIdentity();
  glTranslatef(-(viewport[0] + w + offset.x), +(viewport[1] + h + offset.y), 0);
  gluLookAt(0.0, 0.0, -2.0*h, 0.0, 0.0, 0.0, 0.0, -1.0, 0.0);
  glMatrixMode(GL_PROJECTION);
  glLoadIdentity();
  glFrustum( (-w - offset.x)*0.5f, (w - offset.x)*0.5f, (-h + offset.y)*0.5f, (h + offset.y)*0.5f, h, 100*h);
  glMatrixMode(GL_MODELVIEW);
  
  g_graphicsContext.EndPaint();
}

bool CRenderSystemGL::TestRender()
{
  static float theta = 0.0;

  //RESOLUTION_INFO resInfo = g_settings.m_ResInfo[g_guiSettings.m_LookAndFeelResolution];
  //glViewport(0, 0, resInfo.iWidth, resInfo.iHeight);

  glPushMatrix();
  glRotatef( theta, 0.0f, 0.0f, 1.0f );
  glBegin( GL_TRIANGLES );
  glColor3f( 1.0f, 0.0f, 0.0f ); glVertex2f( 0.0f, 1.0f );
  glColor3f( 0.0f, 1.0f, 0.0f ); glVertex2f( 0.87f, -0.5f );
  glColor3f( 0.0f, 0.0f, 1.0f ); glVertex2f( -0.87f, -0.5f );
  glEnd();
  glPopMatrix();

  theta += 1.0f;

  return true;
}

void CRenderSystemGL::ApplyHardwareTransform(const TransformMatrix &finalMatrix)
{ 
  if (!m_bRenderCreated)
    return;
  
  glMatrixMode(GL_MODELVIEW);
  glPushMatrix();
  GLfloat matrix[4][4];

  for(int i=0;i<3;i++)
    for(int j=0;j<4;j++)
      matrix[j][i] = finalMatrix.m[i][j];

  matrix[0][3] = 0.0f;
  matrix[1][3] = 0.0f;
  matrix[2][3] = 0.0f;
  matrix[3][3] = 1.0f;

  glMultMatrixf(&matrix[0][0]);
}

void CRenderSystemGL::RestoreHardwareTransform()
{
  if (!m_bRenderCreated)
    return;
  
  glMatrixMode(GL_MODELVIEW);
  glPopMatrix();  
}

void CRenderSystemGL::CalculateMaxTexturesize()
{
  GLint width = 256;
  glGetError(); // reset any previous GL errors

  // max out at 2^(8+8)
  for (int i = 0 ; i<8 ; i++) 
  {
    glTexImage2D(GL_PROXY_TEXTURE_2D, 0, 4, width, width, 0, GL_BGRA,
                 GL_UNSIGNED_BYTE, NULL);
    glGetTexLevelParameteriv(GL_PROXY_TEXTURE_2D, 0, GL_TEXTURE_WIDTH,
                             &width);

    // GMA950 on OS X sets error instead
    if (width == 0 || (glGetError() != GL_NO_ERROR) )
      break;
    
    m_maxTextureSize = width;
    width *= 2;
    if (width > 65536) // have an upper limit in case driver acts stupid
    {
      CLog::Log(LOGERROR, "GL: Could not determine maximum texture width, falling back to 2048");
      m_maxTextureSize = 2048;
      break;
    }
  }

  CLog::Log(LOGINFO, "GL: Maximum texture width: %u", m_maxTextureSize);
}

void CRenderSystemGL::GetViewPort(CRect& viewPort)
{
  if (!m_bRenderCreated)
    return;
  
  GLint glvp[4];
  glGetIntegerv(GL_SCISSOR_BOX, glvp);
  
  viewPort.x1 = glvp[0];
  viewPort.y1 = m_height - glvp[1] - glvp[3];
  viewPort.x2 = glvp[0] + glvp[2];
  viewPort.y2 = viewPort.y1 + glvp[3];
}

void CRenderSystemGL::SetViewPort(CRect& viewPort)
{
  if (!m_bRenderCreated)
    return;
  
  glScissor((GLint) viewPort.x1, (GLint) (m_height - viewPort.y1 - viewPort.Height()), (GLsizei) viewPort.Width(), (GLsizei) viewPort.Height());
  glViewport((GLint) viewPort.x1, (GLint) (m_height - viewPort.y1 - viewPort.Height()), (GLsizei) viewPort.Width(), (GLsizei) viewPort.Height());
}

bool CRenderSystemGL::SupportsCompressedTextures()
{
  return glewIsSupported("GL_EXT_texture_compression_s3tc");
}

#endif
