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
#include "Splash.h"
#include "GUIImage.h"
#include "FileSystem/File.h"
#include "WindowingFactory.h"
#include "RenderSystem.h"
#include "log.h"

using namespace XFILE;

CSplash::CSplash(const CStdString& imageName)
{
  m_ImageName = imageName;
  fade = 0.5;
}


CSplash::~CSplash()
{
  Stop();
}

void CSplash::OnStartup()
{}

void CSplash::OnExit()
{}

void CSplash::Show()
{
  g_graphicsContext.Lock();
  g_graphicsContext.Clear();

  g_graphicsContext.SetCameraPosition(CPoint(0, 0));
  float w = g_graphicsContext.GetWidth() * 0.5f;
  float h = g_graphicsContext.GetHeight() * 0.5f;
  CGUIImage* image = new CGUIImage(0, 0, w*0.5f, h*0.5f, w, h, m_ImageName);
  image->SetAspectRatio(CAspectRatio::AR_KEEP);
  image->AllocResources();

#ifdef HAS_DX
  // Store the old gamma ramp
  g_Windowing.Get3DDevice()->GetGammaRamp(0, &oldRamp);
  fade = 0.5f;
  for (int i = 0; i < 256; i++)
  {
    newRamp.red[i] = (int)((float)oldRamp.red[i] * fade);
    newRamp.green[i] = (int)((float)oldRamp.red[i] * fade);
    newRamp.blue[i] = (int)((float)oldRamp.red[i] * fade);
  }
  g_Windowing.Get3DDevice()->SetGammaRamp(0, GAMMA_RAMP_FLAG, &newRamp);
#endif

  //render splash image
#if !defined(HAS_XBOX_D3D) && !defined(HAS_GL) && !defined(HAS_GLES)
  g_Windowing.BeginRender();
#endif

  image->Render();
  image->FreeResources();
  delete image;

  //show it on screen
#ifdef HAS_DX
#ifdef HAS_XBOX_D3D
  g_Windowing.Get3DDevice()->BlockUntilVerticalBlank();
#else
  g_Windowing.EndRender();
#endif
  g_Windowing.Get3DDevice()->Present( NULL, NULL, NULL, NULL );
#elif defined(HAS_SDL_2D)
  XBMC_Flip(g_graphicsContext.getScreenSurface()->SDL());
#elif defined(HAS_GL) || defined(HAS_GLES)
  g_graphicsContext.Flip();
#endif
  g_graphicsContext.Unlock();
}

void CSplash::Hide()
{
  g_graphicsContext.Lock();

  // fade out
  for (float fadeout = fade - 0.01f; fadeout >= 0.f; fadeout -= 0.01f)
  {
#ifdef HAS_DX
    for (int i = 0; i < 256; i++)
    {
      newRamp.red[i] = (int)((float)oldRamp.red[i] * fadeout);
      newRamp.green[i] = (int)((float)oldRamp.green[i] * fadeout);
      newRamp.blue[i] = (int)((float)oldRamp.blue[i] * fadeout);
    }
    Sleep(1);
    g_Windowing.Get3DDevice()->SetGammaRamp(0, GAMMA_RAMP_FLAG, &newRamp);
#endif
  }

  //restore original gamma ramp
#ifdef HAS_DX
  g_Windowing.Get3DDevice()->Clear(0, NULL, D3DCLEAR_TARGET, 0, 0, 0);
  g_Windowing.Get3DDevice()->SetGammaRamp(0, 0, &oldRamp);
  g_Windowing.Get3DDevice()->Present( NULL, NULL, NULL, NULL );
#endif
  g_graphicsContext.Unlock();
}

void CSplash::Process()
{
  Show();

  //fade in and wait untill the thread is stopped
  while (!m_bStop)
  {
    if (fade <= 1.f)
    {
      Sleep(10);
#ifdef HAS_DX
      for (int i = 0; i < 256; i++)
      {
        newRamp.red[i] = (int)((float)oldRamp.red[i] * fade);
        newRamp.green[i] = (int)((float)oldRamp.green[i] * fade);
        newRamp.blue[i] = (int)((float)oldRamp.blue[i] * fade);
      }
      g_graphicsContext.Lock();
      g_Windowing.Get3DDevice()->SetGammaRamp(0, GAMMA_RAMP_FLAG, &newRamp);
      g_graphicsContext.Unlock();
#endif
      fade += 0.01f;
    }
    else
    {
      Sleep(10);
    }
  }
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
