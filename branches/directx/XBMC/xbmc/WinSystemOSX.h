/*!
\file Surface.h
\brief
*/

#ifndef WINDOW_SYSTEM_OSX_H
#define WINDOW_SYSTEM_OSX_H

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
#include "WinSystem.h"
#include <SDL/SDL_video.h>

class CWinSystemOSX : public CWinSystemBase
{
public:
  CWinSystemOSX();
  virtual ~CWinSystemOSX();

  // CWinSystemBase
  virtual bool InitWindowSystem();
  virtual bool DestroyWindowSystem();
  virtual bool CreateNewWindow(CStdString name, int width, int height, bool fullScreen, PHANDLE_EVENT_FUNC userFunction);
  virtual bool DestroyWindow();
  virtual bool ResizeWindow(int newWidth, int newHeight, int newLeft, int newTop);
  virtual bool SetFullScreen(bool fullScreen, int width, int height);
  virtual void UpdateResolutions();

protected:
  void GetDesktopRes(RESOLUTION_INFO& desktopRes);
  virtual bool Resize();
  
  void* m_glContext;
  SDL_Surface* m_SDLSurface;
};

#endif // WINDOW_SYSTEM_H

