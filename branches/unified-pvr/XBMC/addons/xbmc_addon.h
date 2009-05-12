#ifndef __XBMC_ADDON_H__
#define __XBMC_ADDON_H__

#ifdef HAS_XBOX_HARDWARE
#include <xtl.h>
#else
#ifndef _LINUX
#include <windows.h>
#else
#undef __cdecl
#define __cdecl
#undef __declspec
#define __declspec(x)
#include <time.h>
#endif
#endif

#include "DllAddonTypes.h"

extern "C"
{
  ADDON_STATUS __declspec(dllexport) GetStatus();
  bool __declspec(dllexport) HasSettings();
  ADDON_STATUS __declspec(dllexport) SetSetting(const char *settingName, const void *settingValue);
  void __declspec(dllexport) Remove();
};

#endif
