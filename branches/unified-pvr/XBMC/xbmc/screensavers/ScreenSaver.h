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
// Screensaver.h: interface for the CScreensaver class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_ScreenSaver_H__99B9A52D_ED09_4540_A887_162A68217A31__INCLUDED_)
#define AFX_ScreenSaver_H__99B9A52D_ED09_4540_A887_162A68217A31__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "DllScreenSaver.h"
#include "../utils/Addon.h"

#ifdef __cplusplus
extern "C"
{
}
#endif

class CScreenSaver : public ADDON::CAddon
{
public:
  CScreenSaver(struct ScreenSaver* pScr, DllScreensaver* pDll, const ADDON::CAddon& addon);
  ~CScreenSaver();

  // Things that MUST be supplied by the child classes
  void Create();
  void Destroy();
  void Start();
  void Render();
  void Stop();
  void GetInfo(SCR_INFO *info);

protected:
  std::auto_ptr<struct ScreenSaver> m_pScr;
  std::auto_ptr<DllScreensaver> m_pDll;
  ScreensaverCallbacks *m_callbacks;
  bool m_ReadyToUse;

private:
  static void AddOnStatusCallback(void *userData, const ADDON_STATUS status, const char* msg);
  static void AddOnLogCallback(void *userData, const ADDON_LOG loglevel, const char *format, ... );
  static bool AddOnGetSetting(void *userData, const char *settingName, void *settingValue);
  static void AddOnOpenSettings(const char *url, bool bReload);
  static void AddOnOpenOwnSettings(void *userData, bool bReload);
  static const char* AddOnGetLocalizedString(void *userData, long dwCode);
};


#endif // !defined(AFX_ScreenSaver_H__99B9A52D_ED09_4540_A887_162A68217A31__INCLUDED_)

