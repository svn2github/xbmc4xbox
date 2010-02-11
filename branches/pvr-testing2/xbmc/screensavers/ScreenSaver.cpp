/*
 *      Copyright (C) 2005-2009 Team XBMC
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
#include "ScreenSaver.h"
#include "Settings.h"
#include "WindowingFactory.h"

using namespace ADDON;

bool CScreenSaver::CreateScreenSaver()
{
 // pass it the screen width,height
 // and the name of the screensaver
  int iWidth = g_graphicsContext.GetWidth();
  int iHeight = g_graphicsContext.GetHeight();

  m_pInfo = new SCR_PROPS;
#ifdef HAS_DX
  m_pInfo->device     = g_Windowing.Get3DDevice();
#else
  m_pInfo->device     = NULL;
#endif
  m_pInfo->x          = 0;
  m_pInfo->y          = 0;
  m_pInfo->width      = iWidth;
  m_pInfo->height     = iHeight;
  m_pInfo->pixelRatio = g_settings.m_ResInfo[g_graphicsContext.GetVideoResolution()].fPixelRatio;
  m_pInfo->name       = Name().c_str();

  if (CAddonDll<DllScreenSaver, ScreenSaver, SCR_PROPS>::Create())
  {
    m_initialized = true;
    return true;
  }
  return false;
}

void CScreenSaver::Start()
{
  // notify screen saver that they should start
  if (m_initialized) m_pStruct->Start();
}

void CScreenSaver::Render()
{
  // ask screensaver to render itself
  if (m_initialized) m_pStruct->Render();
}

void CScreenSaver::Stop()
{
  // ask screensaver to cleanup
  if (m_initialized) m_pStruct->Stop();
}
