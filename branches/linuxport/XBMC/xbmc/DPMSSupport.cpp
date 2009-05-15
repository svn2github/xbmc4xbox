/*
 *      Copyright (C) 2009 Team XBMC
 *      http://xbmc.org
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
#include <assert.h>
#include <string>
#include "DPMSSupport.h"
#include "system.h"
#include "utils/log.h"
#include "Surface.h"

//////// Generic, non-platform-specific code

static const char* const MODE_NAMES[DPMSSupport::NUM_MODES] =
  { "STANDBY", "SUSPEND", "OFF" };

bool DPMSSupport::CheckValidMode(PowerSavingMode mode)
{
  if (mode < 0 || mode > NUM_MODES)
  {
    CLog::Log(LOGERROR, "Invalid power-saving mode %d", mode);
    return false;
  }
  return true;
}

const char* DPMSSupport::GetModeName(PowerSavingMode mode)
{
  if (!CheckValidMode(mode)) return NULL;
  return MODE_NAMES[mode];
}

DPMSSupport::DPMSSupport(Surface::CSurface* surface)
{
  assert(surface != NULL);
  m_surface = surface;
  PlatformSpecificInit();

  if (!m_supportedModes.empty())
  {
    std::string modes_message;
    for (size_t i = 0; i < m_supportedModes.size(); i++)
    {
      assert(CheckValidMode(m_supportedModes[i]));
      modes_message += " ";
      modes_message += MODE_NAMES[m_supportedModes[i]];
    }
    CLog::Log(LOGDEBUG, "DPMS: supported power-saving modes:%s",
              modes_message.c_str());
  }
}

bool DPMSSupport::IsModeSupported(PowerSavingMode mode) const
{
  if (!CheckValidMode(mode)) return false;
  for (size_t i = 0; i < m_supportedModes.size(); i++)
  {
    if (m_supportedModes[i] == mode) return true;
  }
  return false;
}

bool DPMSSupport::EnablePowerSaving(PowerSavingMode mode)
{
  if (!CheckValidMode(mode)) return false;
  if (!IsModeSupported(mode))
  {
    CLog::Log(LOGERROR, "DPMS: power-saving mode %s is not supported",
              MODE_NAMES[mode]);
    return false;
  }

  if (!PlatformSpecificEnablePowerSaving(mode)) return false;

  CLog::Log(LOGINFO, "DPMS: enabled power-saving mode %s",
            GetModeName(mode));
  return true;
}

bool DPMSSupport::DisablePowerSaving()
{
  if (!PlatformSpecificDisablePowerSaving()) return false;
  CLog::Log(LOGINFO, "DPMS: disabled power-saving");
  return true;
}

///////// Platform-specific support

#if defined(HAS_GLX)
//// X Windows

// Here's a sad story: our Windows-inspired BOOL type from linux/PlatformDefs.h
// is incompatible with the BOOL in X11's Xmd.h (int vs. unsigned char).
// This is not a good idea for a X11 app and it should
// probably be fixed. Meanwhile, we can work around it rather cleanly with
// the preprocessor (which is partly to blame for this needless conflict
// anyway). BOOL is not used in the DPMS APIs that we need. Try not to use
// BOOL in the remaining X11-specific code in this file, since X might
// someday use a #define instead of a typedef.
// Addendum: INT64 apparently hhas the same problem on x86_64. Oh well.
// Once again, INT64 is not used in the APIs we need, so we can #ifdef it away.
#define BOOL __X11_SPECIFIC_BOOL
#define INT64 __X11_SPECIFIC_INT64
#include <X11/Xlib.h>
#include <X11/extensions/dpms.h>
#undef INT64
#undef BOOL

// Mapping of PowerSavingMode to X11's mode constants.
static const CARD16 X_DPMS_MODES[] =
  { DPMSModeStandby, DPMSModeSuspend, DPMSModeOff };

void DPMSSupport::PlatformSpecificInit()
{
  Display* dpy = m_surface->GetDisplay();
  if (dpy == NULL) return;

  int event_base, error_base;   // we ignore these
  if (!DPMSQueryExtension(dpy, &event_base, &error_base)) {
    CLog::Log(LOGINFO, "DPMS: X11 extension not present, power-saving"
              " will not be available");
    return;
  }

  if (!DPMSCapable(dpy)) {
    CLog::Log(LOGINFO, "DPMS: display does not support power-saving");
    return;
  }

  m_supportedModes.push_back(SUSPEND); // best compromise
  m_supportedModes.push_back(OFF);     // next best
  m_supportedModes.push_back(STANDBY); // rather lame, < 80% power according to
                                       // DPMS spec
}

bool DPMSSupport::PlatformSpecificEnablePowerSaving(PowerSavingMode mode)
{
  Display* dpy = m_surface->GetDisplay();
  if (dpy == NULL) return false;

  // This is not needed on my ATI Radeon, but the docs say that DPMSForceLevel
  // after a DPMSDisable (from SDL) should not normally work.
  DPMSEnable(dpy);
  DPMSForceLevel(dpy, X_DPMS_MODES[mode]);
  // There shouldn't be any errors if we called DPMSEnable; if they do happen,
  // they're asynchronous and messy to detect.
  XFlush(dpy);
  return true;
}

bool DPMSSupport::PlatformSpecificDisablePowerSaving()
{
  Display* dpy = m_surface->GetDisplay();
  if (dpy == NULL) return false;

  DPMSForceLevel(dpy, DPMSModeOn);
  DPMSDisable(dpy);
  XFlush(dpy);
  // On my ATI, the full-screen window stays blank after waking up from
  // DPMS, presumably due to being OpenGL. There is something magical about
  // window expose events (involving the window manager) that solves this
  // without fail.
  XUnmapWindow(dpy, m_surface->GetWindow());
  XMapWindow(dpy, m_surface->GetWindow());
  return true;
}

/////  Add other platforms here.
#else
// Not implemented on this platform
void DPMSSupport::PlatformSpecificInit()
{
  CLog::Log(LOGINFO, "DPMS: not supported on this platform");
}

bool DPMSSupport::PlatformSpecificEnablePowerSaving(PowerSavingMode mode)
{
  return false;
}

bool DPMSSupport::PlatformSpecificDisablePowerSaving()
{
  return false;
}

#endif  // platform ifdefs

