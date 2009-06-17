#pragma once
#ifndef ADDON_DLL_H
#define ADDON_DLL_H

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
 
#include "Addon.h"
#include "DllAddon.h"

namespace ADDON
{
  class CAddonDll : public CAddon
  {
  public:
    CAddonDll();
    virtual ~CAddonDll() {}

    virtual void Remove();
    virtual bool HasSettings();
    virtual bool GetSettings();
    virtual ADDON_STATUS SetSetting(const char *settingName, const void *settingValue);
    virtual ADDON_STATUS GetStatus();

  private:
    friend CAddonManager;
    DllAddon* m_pDll;
  };

};

#endif /* ADDON_DLL_H */
