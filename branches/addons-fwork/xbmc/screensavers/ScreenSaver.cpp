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
#include "AddonManager.h"

void CScreenSaver::Start()
{
  // notify screen saver that they should start
  if (m_initialized) m_pScr->Start();
}

void CScreenSaver::Render()
{
  // ask screensaver to render itself
  if (m_initialized) m_pScr->Render();
}

void CScreenSaver::Stop()
{
  // ask screensaver to cleanup
  if (m_initialized) m_pScr->Stop();
}


void CScreenSaver::GetInfo(SCR_INFO *info)
{
  // get info from screensaver
  if (m_initialized) m_pScr->GetInfo(info);
}
