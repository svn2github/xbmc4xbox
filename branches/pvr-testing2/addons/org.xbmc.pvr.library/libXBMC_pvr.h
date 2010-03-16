#pragma once
/*
 *      Copyright (C) 2005-2010 Team XBMC
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

#include <string>
#include <vector>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <dlfcn.h>
#include "xbmc_pvr_types.h"

#ifndef _LINUX
#define PVR_HELPER_DLL "\org.xbmc.pvr.library\libXBMC_pvr.dll"
#else
#if defined(__APPLE__)
#if defined(__POWERPC__)
#define PVR_HELPER_DLL "/org.xbmc.pvr.library/libXBMC_pvr-powerpc-osx.so"
#else
#define PVR_HELPER_DLL "/org.xbmc.pvr.library/libXBMC_pvr-x86-osx.so"
#endif
#elif defined(__x86_64__)
#define PVR_HELPER_DLL "/org.xbmc.pvr.library/libXBMC_pvr-x86_64-linux.so"
#elif defined(_POWERPC)
#define PVR_HELPER_DLL "/org.xbmc.pvr.library/libXBMC_pvr-powerpc-linux.so"
#elif defined(_POWERPC64)
#define PVR_HELPER_DLL "/org.xbmc.pvr.library/libXBMC_pvr-powerpc64-linux.so"
#else /* !__x86_64__ && !__powerpc__ */
#define PVR_HELPER_DLL "/org.xbmc.pvr.library/libXBMC_pvr-i486-linux.so"
#endif /* __x86_64__ */
#endif /* _LINUX */

class cHelper_libXBMC_pvr
{
public:
  cHelper_libXBMC_pvr()
  {
    m_libXBMC_pvr = NULL;
    m_Handle      = NULL;
  }

  ~cHelper_libXBMC_pvr()
  {
    if (m_libXBMC_pvr)
    {
      PVR_unregister_me();
      dlclose(m_libXBMC_pvr);
    }
  }

  bool RegisterMe(void *Handle)
  {
    m_Handle = Handle;

    std::string libBasePath;
    libBasePath  = ((cb_array*)m_Handle)->libPath;
    libBasePath += PVR_HELPER_DLL;

    m_libXBMC_pvr = dlopen(libBasePath.c_str(), RTLD_LAZY);
    if (m_libXBMC_pvr == NULL)
    {
      fprintf(stderr, "Unable to load %s\n", dlerror());
      return false;
    }

    PVR_register_me         = (int (*)(void *HANDLE))
      dlsym(m_libXBMC_pvr, "PVR_register_me");
    if (PVR_register_me == NULL)      { fprintf(stderr, "Unable to assign function %s\n", dlerror()); return false; }

    PVR_unregister_me       = (void (*)())
      dlsym(m_libXBMC_pvr, "PVR_unregister_me");
    if (PVR_unregister_me == NULL)    { fprintf(stderr, "Unable to assign function %s\n", dlerror()); return false; }

    TransferEpgEntry        = (void (*)(const PVRHANDLE handle, const PVR_PROGINFO *epgentry))
      dlsym(m_libXBMC_pvr, "PVR_transfer_epg_entry");
    if (TransferEpgEntry == NULL)       { fprintf(stderr, "Unable to assign function %s\n", dlerror()); return false; }

    TransferChannelEntry    = (void (*)(const PVRHANDLE handle, const PVR_CHANNEL *chan))
      dlsym(m_libXBMC_pvr, "PVR_transfer_channel_entry");
    if (TransferChannelEntry == NULL)   { fprintf(stderr, "Unable to assign function %s\n", dlerror()); return false; }

    TransferTimerEntry      = (void (*)(const PVRHANDLE handle, const PVR_TIMERINFO *timer))
      dlsym(m_libXBMC_pvr, "PVR_transfer_timer_entry");
    if (TransferTimerEntry == NULL)     { fprintf(stderr, "Unable to assign function %s\n", dlerror()); return false; }

    TransferRecordingEntry  = (void (*)(const PVRHANDLE handle, const PVR_RECORDINGINFO *recording))
      dlsym(m_libXBMC_pvr, "PVR_transfer_recording_entry");
    if (TransferRecordingEntry == NULL) { fprintf(stderr, "Unable to assign function %s\n", dlerror()); return false; }

    AddMenuHook             = (void (*)(PVR_MENUHOOK *hook))
      dlsym(m_libXBMC_pvr, "PVR_add_menu_hook");
    if (AddMenuHook == NULL)            { fprintf(stderr, "Unable to assign function %s\n", dlerror()); return false; }

    return PVR_register_me(m_Handle) > 0;
  }

  void (*TransferEpgEntry)(const PVRHANDLE handle, const PVR_PROGINFO *epgentry);
  void (*TransferChannelEntry)(const PVRHANDLE handle, const PVR_CHANNEL *chan);
  void (*TransferTimerEntry)(const PVRHANDLE handle, const PVR_TIMERINFO *timer);
  void (*TransferRecordingEntry)(const PVRHANDLE handle, const PVR_RECORDINGINFO *recording);
  void (*AddMenuHook)(PVR_MENUHOOK *hook);

protected:
  int (*PVR_register_me)(void *HANDLE);
  void (*PVR_unregister_me)();

private:
  void *m_libXBMC_pvr;
  void *m_Handle;
  struct cb_array
  {
    const char* libPath;
  };
};
