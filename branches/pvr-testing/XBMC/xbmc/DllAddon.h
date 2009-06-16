#pragma once
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

#include "DynamicDll.h"
#include "addons/include/xbmc_addon_types.h"

template <typename T>
class DllAddonInterface
{
public:
  virtual void GetAddon(T* pAddon) =0;
  virtual ADDON_STATUS GetStatus() =0;  /* For "ADDON_STATUS" see header "xbmc_addon_types.h" */
  virtual bool HasSettings() =0;
  virtual ADDON_STATUS SetSetting(const char *settingName, const void *settingValue) =0;
  virtual void Remove() =0;
};

template <typename T>
class DllAddon : public DllDynamic, public DllAddonInterface<T>
{
public:
  DECLARE_DLL_WRAPPER_TEMPLATE(DllAddon)
  DEFINE_METHOD0(ADDON_STATUS, GetStatus)
  DEFINE_METHOD0(bool, HasSettings)
  DEFINE_METHOD2(ADDON_STATUS, SetSetting, (const char *p1, const void *p2))
  DEFINE_METHOD0(void, Remove)
  DEFINE_METHOD1(void, GetAddon, (T* p1))
  BEGIN_METHOD_RESOLVE()
    RESOLVE_METHOD_RENAME(get_addon,GetAddon)
    RESOLVE_METHOD(GetStatus)
    RESOLVE_METHOD(SetSetting)
    RESOLVE_METHOD(HasSettings)
    RESOLVE_METHOD(Remove)
  END_METHOD_RESOLVE()
};

