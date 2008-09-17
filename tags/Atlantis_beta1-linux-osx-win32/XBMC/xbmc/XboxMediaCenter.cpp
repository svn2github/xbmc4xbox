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


// XBMC
//
// libraries:
//   - CDRipX   : doesnt support section loading yet
//   - xbfilezilla : doesnt support section loading yet
//

#include "stdafx.h"
#include "Application.h"
#include "FileItem.h"
#include "PlayListPlayer.h"
#ifdef _LINUX
#include <sys/resource.h>
#endif


CApplication g_application;

int main(int argc, char* argv[])
{
  CFileItemList playlist;
#if defined(_LINUX) && defined(DEBUG)
  struct rlimit rlim;
  rlim.rlim_cur = rlim.rlim_max = RLIM_INFINITY;
  setrlimit(RLIMIT_CORE, &rlim);
#endif
  if (argc > 1)
  {
    for (int i = 1; i < argc; i++)
    {
      if (strnicmp(argv[i], "-fs", 3) == 0)
      {
        g_advancedSettings.m_fullScreen = true;
      }
      else if (strnicmp(argv[i], "-h", 2) == 0 || strnicmp(argv[i], "--help", 6) == 0)
      {
        printf("Usage: %s [OPTION]... [FILE]...\n\n", argv[0]);
        printf("Arguments:\n");
        printf("  -fs\t\t\tRuns XBMC in full screen\n");
        printf("  --standalone\t\tXBMC runs in a stand alone environment without a window \n");
        printf("\t\t\tmanager and supporting applications. For example, that\n");
        printf("\t\t\tenables network settings.\n");
        printf("  --legacy-res\t\tEnables screen resolutions such as PAL, NTSC, etc.\n");
        exit(0);
      }
      else if (strnicmp(argv[i], "--standalone", 12) == 0)
      {
        g_application.SetStandAlone(true);
      }
      else if (strnicmp(argv[i], "--legacy-res", 12) == 0)
      {
        g_application.SetEnableLegacyRes(true);
      }
      else if (argv[i][0] != '-')
      {
        CFileItemPtr pItem(new CFileItem(argv[i]));
        pItem->m_strPath = argv[i];
        playlist.Add(pItem);
      }
    }
  }

  // if we're on a Mac or if XBMC_PLATFORM_MODE is set, enable platform
  // specific directories.
#ifdef __APPLE__
  if (1)
#else
  if (getenv("XBMC_PLATFORM_MODE"))
#endif
  {
    g_application.EnablePlatformDirectories();
  }

  // if XBMC_DEFAULT_MODE is set, disable platform specific directories
  if (getenv("XBMC_DEFAULT_MODE"))
  {
    g_application.EnablePlatformDirectories(false);
  }

  g_application.Create(NULL);
  if (playlist.Size() > 0)
  {
    g_playlistPlayer.Add(0,playlist);
    g_playlistPlayer.SetCurrentPlaylist(0);
  }

  ThreadMessage tMsg = {TMSG_PLAYLISTPLAYER_PLAY, (DWORD) -1};
  g_application.getApplicationMessenger().SendMessage(tMsg, false);

  try
  {
    while (1)
    {
      g_application.Run();
    }
  }
  catch(...)
  {
    printf("********ERROR- exception caught on main loop. exiting");
	return -1;
  }

  return 0;
}

extern "C"
{

  void mp_msg( int x, int lev, const char *format, ... )
  {
    va_list va;
    static char tmp[2048];
    va_start(va, format);
#ifndef _LINUX
    _vsnprintf(tmp, 2048, format, va);
#else
    vsnprintf(tmp, 2048, format, va);
#endif
    va_end(va);
    tmp[2048 - 1] = 0;

    OutputDebugString(tmp);
  }
}
