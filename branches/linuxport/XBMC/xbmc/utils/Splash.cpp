
#include "stdafx.h"
#include "Splash.h"
#include "guiImage.h"
#include "../Util.h"

using namespace XFILE;

CSplash::CSplash(const CStdString& imageName)
{
  m_ImageName = imageName;
}


CSplash::~CSplash()
{
  Stop();
}

void CSplash::OnStartup()
{}

void CSplash::OnExit()
{}

void CSplash::Process()
{
#ifndef HAS_SDL
  D3DGAMMARAMP newRamp;
  D3DGAMMARAMP oldRamp;  
#else
  Uint16 newRampRed[256];
  Uint16 newRampGreen[256];
  Uint16 newRampBlue[256];  
  Uint16 oldRampRed[256];
  Uint16 oldRampGreen[256];
  Uint16 oldRampBlue[256];  
#endif

  g_graphicsContext.Lock();
#ifndef HAS_SDL
  g_graphicsContext.Get3DDevice()->Clear(0, NULL, D3DCLEAR_TARGET, 0, 0, 0);
#endif
  
  float w = g_graphicsContext.GetWidth() * 0.5f;
  float h = g_graphicsContext.GetHeight() * 0.5f;  
  CGUIImage* image = new CGUIImage(0, 0, w*0.5f, h*0.5f, w, h, m_ImageName);
  image->SetAspectRatio(CGUIImage::ASPECT_RATIO_KEEP);
  image->AllocResources();

#ifndef HAS_SDL
  // Store the old gamma ramp
  g_graphicsContext.Get3DDevice()->GetGammaRamp(&oldRamp);
  float fade = 0.5f;
  for (int i = 0; i < 256; i++)
  {
    newRamp.red[i] = (int)((float)oldRamp.red[i] * fade);
    newRamp.green[i] = (int)((float)oldRamp.red[i] * fade);
    newRamp.blue[i] = (int)((float)oldRamp.red[i] * fade);
  }
  g_graphicsContext.Get3DDevice()->SetGammaRamp(GAMMA_RAMP_FLAG, &newRamp);
#else
  SDL_GetGammaRamp(oldRampRed, oldRampGreen, oldRampBlue);
  float fade = 0.5f;
  for (int i = 0; i < 256; i++)
  {
    newRampRed[i] = (int)((float)oldRampRed[i] * fade);
    newRampGreen[i] = (int)((float)oldRampRed[i] * fade);
    newRampBlue[i] = (int)((float)oldRampRed[i] * fade);
  }
  SDL_SetGammaRamp(newRampRed, newRampGreen, newRampBlue);
#endif 
  
  //render splash image
#if !defined(HAS_XBOX_D3D) && !defined(HAS_SDL)
  g_graphicsContext.Get3DDevice()->BeginScene();
#endif

  image->Render();
  image->FreeResources();
  delete image;

  //show it on screen
#ifndef HAS_SDL
#ifdef HAS_XBOX_D3D
  g_graphicsContext.Get3DDevice()->BlockUntilVerticalBlank();
#else
  g_graphicsContext.Get3DDevice()->EndScene();
#endif
  g_graphicsContext.Get3DDevice()->Present( NULL, NULL, NULL, NULL );
#elif defined(HAS_SDL_2D) 
  SDL_Flip(g_graphicsContext.getScreenSurface());
#elif defined(HAS_SDL_OPENGL)
  SDL_GL_SwapBuffers();
#endif
  g_graphicsContext.Unlock();


  //fade in and wait untill the thread is stopped
  while (!m_bStop)
  {
    if (fade <= 1.f)
    {
      Sleep(10);
#ifndef HAS_SDL      
      for (int i = 0; i < 256; i++)
      {
        newRamp.red[i] = (int)((float)oldRamp.red[i] * fade);
        newRamp.green[i] = (int)((float)oldRamp.green[i] * fade);
        newRamp.blue[i] = (int)((float)oldRamp.blue[i] * fade);
      }
      g_graphicsContext.Lock();
      g_graphicsContext.Get3DDevice()->SetGammaRamp(GAMMA_RAMP_FLAG, &newRamp);
      g_graphicsContext.Unlock();
#else
      for (int i = 0; i < 256; i++)
      {
        newRampRed[i] = (int)((float)oldRampRed[i] * fade);
        newRampGreen[i] = (int)((float)oldRampGreen[i] * fade);
        newRampBlue[i] = (int)((float)oldRampBlue[i] * fade);
      }
      SDL_SetGammaRamp(newRampRed, newRampGreen, newRampBlue);
#endif      
      fade += 0.01f;
    }
    else
    {
      Sleep(10);
    }
  }

  g_graphicsContext.Lock();
  // fade out
  for (float fadeout = fade - 0.01f; fadeout >= 0.f; fadeout -= 0.01f)
  {
#ifndef HAS_SDL  
    for (int i = 0; i < 256; i++)
    {
      newRamp.red[i] = (int)((float)oldRamp.red[i] * fadeout);
      newRamp.green[i] = (int)((float)oldRamp.green[i] * fadeout);
      newRamp.blue[i] = (int)((float)oldRamp.blue[i] * fadeout);
    }
    Sleep(1);
    g_graphicsContext.Get3DDevice()->SetGammaRamp(GAMMA_RAMP_FLAG, &newRamp);
#else
	 for (int i = 0; i < 256; i++)
    {
      newRampRed[i] = (int)((float)oldRampRed[i] * fade);
      newRampGreen[i] = (int)((float)oldRampGreen[i] * fade);
      newRampBlue[i] = (int)((float)oldRampBlue[i] * fade);
    }
    Sleep(1);
    SDL_SetGammaRamp(newRampRed, newRampGreen, newRampBlue);
#endif
  }
  
  //restore original gamma ramp
#ifndef HAS_SDL
  g_graphicsContext.Get3DDevice()->Clear(0, NULL, D3DCLEAR_TARGET, 0, 0, 0);
  g_graphicsContext.Get3DDevice()->SetGammaRamp(0, &oldRamp);
  g_graphicsContext.Get3DDevice()->Present( NULL, NULL, NULL, NULL );
#elif defined(HAS_SDL_2D) 
  SDL_SetGammaRamp(oldRampRed, oldRampGreen, oldRampBlue);
  SDL_Flip(g_graphicsContext.getScreenSurface());
#elif defined(HAS_SDL_OPENGL)
  SDL_SetGammaRamp(oldRampRed, oldRampGreen, oldRampBlue);
  SDL_GL_SwapBuffers();
#endif
  g_graphicsContext.Unlock();
}

bool CSplash::Start()
{
  if (m_ImageName.IsEmpty() || !CFile::Exists(m_ImageName))
  {
    CLog::Log(LOGDEBUG, "Splash image %s not found", m_ImageName.c_str());
    return false;
  }
  Create();
  return true;
}

void CSplash::Stop()
{
  StopThread();
}

bool CSplash::IsRunning()
{
  return (m_ThreadHandle != NULL);
}
