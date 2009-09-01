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
#include "WinSystemWin32DX.h"

#ifdef HAS_DX

CWinSystemWin32DX g_Windowing;

CWinSystemWin32DX::CWinSystemWin32DX()
: CRenderSystemDX()
{

}

CWinSystemWin32DX::~CWinSystemWin32DX()
{

}

bool CWinSystemWin32DX::CreateNewWindow(CStdString name, int width, int height, bool fullScreen, PHANDLE_EVENT_FUNC userFunction)
{
  CWinSystemWin32::CreateNewWindow(name, width, height, fullScreen, userFunction);

  if(m_hWnd == NULL)
    return false;

  SetFocusWnd(m_hWnd);
  SetDeviceWnd(m_hWnd);

  SetBackBufferSize(m_nWidth, m_nHeight);
  SetDeviceFullScreen(fullScreen);

  return true;
}

#endif