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
#include "XBAudioConfig.h"
#ifdef HAS_XBOX_HARDWARE
#include "xbox/Undocumented.h"
#endif

XBAudioConfig g_audioConfig;

XBAudioConfig::XBAudioConfig()
{
#ifdef HAS_XBOX_AUDIO
  m_dwAudioFlags = XGetAudioFlags();
#else
  m_dwAudioFlags = 0;
#endif
}

XBAudioConfig::~XBAudioConfig()
{
}

bool XBAudioConfig::HasDigitalOutput()
{
#ifdef HAS_XBOX_AUDIO
  DWORD dwAVPack = XGetAVPack();
  if (dwAVPack == XC_AV_PACK_SCART ||
      dwAVPack == XC_AV_PACK_HDTV ||
      dwAVPack == XC_AV_PACK_VGA ||
      dwAVPack == XC_AV_PACK_SVIDEO)
    return true;
#endif
  return false;
}

void XBAudioConfig::SetAC3Enabled(bool bEnable)
{
#ifdef HAS_XBOX_AUDIO
  if (bEnable)
    m_dwAudioFlags |= XC_AUDIO_FLAGS_ENABLE_AC3;
  else
    m_dwAudioFlags &= ~XC_AUDIO_FLAGS_ENABLE_AC3;
#endif
}

bool XBAudioConfig::GetAC3Enabled()
{
  if (!HasDigitalOutput()) return false;
#ifdef HAS_XBOX_AUDIO
  return (XC_AUDIO_FLAGS_ENCODED(XGetAudioFlags()) & XC_AUDIO_FLAGS_ENABLE_AC3) != 0;
#else
  return false;
#endif
}

void XBAudioConfig::SetDTSEnabled(bool bEnable)
{
#ifdef HAS_XBOX_AUDIO
  if (bEnable)
    m_dwAudioFlags |= XC_AUDIO_FLAGS_ENABLE_DTS;
  else
    m_dwAudioFlags &= ~XC_AUDIO_FLAGS_ENABLE_DTS;
#endif
}

bool XBAudioConfig::GetDTSEnabled()
{
  if (!HasDigitalOutput()) return false;
#ifdef HAS_XBOX_AUDIO
  return (XC_AUDIO_FLAGS_ENCODED(XGetAudioFlags()) & XC_AUDIO_FLAGS_ENABLE_DTS) != 0;
#else
  return false;
#endif
}

bool XBAudioConfig::NeedsSave()
{
  if (!HasDigitalOutput()) return false;
#ifdef HAS_XBOX_AUDIO
  return m_dwAudioFlags != XGetAudioFlags();
#else
  return false;
#endif
}

// USE VERY CAREFULLY!!
void XBAudioConfig::Save()
{
  if (!NeedsSave()) return ;
#ifdef HAS_XBOX_AUDIO
  // update the EEPROM settings
  DWORD type = REG_BINARY;
  ExSaveNonVolatileSetting(XC_AUDIO_FLAGS, &type, (PULONG)&m_dwAudioFlags, 4);
  // check that we updated correctly
  if (m_dwAudioFlags != XGetAudioFlags())
  {
    CLog::Log(LOGNOTICE, "Failed to save audio config!");
  }
#endif
}
