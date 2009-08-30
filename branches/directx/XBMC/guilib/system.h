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

/*****************
 * All platforms
 *****************/

#define HAS_SDL

#define HAS_DVD_DRIVE
#define HAS_DVD_SWSCALE
#define HAS_DVDPLAYER
#define HAS_EVENT_SERVER
#define HAS_KARAOKE
#define HAS_RAR
#define HAS_SCREENSAVER
#define HAS_PYTHON
#define HAS_SHOUTCAST
#define HAS_SYSINFO
#define HAS_UPNP
#define HAS_VIDEO_PLAYBACK
#define HAS_VISUALISATION
#define HAS_WEB_SERVER

#define HAS_AC3_CODEC
#define HAS_DTS_CODEC
#define HAS_CDDA_RIPPER

#define HAS_FILESYSTEM
#define HAS_FILESYSTEM_SMB
#define HAS_FILESYSTEM_CDDA
#define HAS_FILESYSTEM_RTV
#define HAS_FILESYSTEM_DAAP
#define HAS_FILESYSTEM_SAP
#define HAS_FILESYSTEM_VTP
#define HAS_FILESYSTEM_HTSP
#define HAS_FILESYSTEM_MMS
#define HAS_CCXSTREAM

/*****************
 * Win32 Specific
 *****************/

#ifdef _WIN32
#define _WIN32PC
#define HAS_SDL_JOYSTICK
#define HAS_WIN32_NETWORK
#define HAS_LIRC
#define HAS_IRSERVERSUITE // depends on HAS_LIRC define
#define HAS_AUDIO
#endif

/*****************
 * Mac Specific
 *****************/

#ifdef __APPLE__
#define HAS_ZEROCONF
#define HAS_GL
#define HAS_LINUX_NETWORK
#define HAS_SDL_AUDIO
#define HAS_SDL_OPENGL
#define HAS_SDL_WIN_EVENTS
#endif

/*****************
 * Linux Specific
 *****************/

#if defined(_LINUX) && !defined(__APPLE__)
#ifdef HAS_AVAHI
#define HAS_ZEROCONF
#endif
#define HAS_LCD
#define HAS_HAL
#define HAS_DBUS
#define HAS_DBUS_SERVER
#define HAS_GL
#define HAS_GLX
#define HAS_LINUX_NETWORK
#define HAS_SDL_AUDIO
#define HAS_LIRC
#define HAS_SDL_OPENGL
#define HAS_SDL_WIN_EVENTS
#endif

/*****************
 * SVN revision
 *****************/

#ifdef __APPLE__
#include "../svn_revision.h"
#endif

#ifndef SVN_REV
#define SVN_REV "Unknown"
#endif

#if defined(_LINUX) && !defined(__APPLE__)
#include "../config.h"
#endif

/****************************************
 * Additional platform specific includes
 ****************************************/

#ifdef _WIN32
#if !(defined(_WINSOCKAPI_) || defined(_WINSOCK_H))
#include <winsock2.h>
#endif
#include <windows.h>
#define DIRECTINPUT_VERSION 0x0800
#include "DInput.h"
#include "DSound.h"
#define DSSPEAKER_USE_DEFAULT DSSPEAKER_STEREO
#define LPDIRECTSOUND8 LPDIRECTSOUND
#undef GetFreeSpace
#include "../xbmc/win32/PlatformInclude.h"
#include "D3D9.h"   // On Win32, we're always using DirectX for something, whether it be the actual rendering
#include "D3DX9.h"  // or the reference video clock.
#ifdef HAS_SDL
#include "SDL\SDL.h"
#endif
#endif

#ifdef _LINUX
#include <unistd.h>
#include <time.h>
#include <sys/time.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <errno.h>
#include "PlatformInclude.h"
#endif

#ifdef HAS_GL
#ifdef _WIN32
#include "GL/glew.h"
#include <GL/gl.h>
#include <GL/glu.h>
#include <GL/wglext.h>
#elif defined(__APPLE__)
#include <GL/glew.h>
#include <OpenGL/gl.h>
#elif defined(_LINUX)
#include <GL/glew.h>
#include <GL/gl.h>
#endif
#endif
