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

#if (defined HAVE_CONFIG_H) && (!defined WIN32)
  #include "config.h"
#elif defined(_WIN32)
#include "system.h"
#include "libavcodec/avcodec.h"
#endif

#if defined(HAVE_LIBCRYSTALHD)
#include "GUISettings.h"
#include "DVDClock.h"
#include "DVDStreamInfo.h"
#include "DVDVideoCodecCrystalHD.h"
#include "utils/log.h"
#include "utils/TimeUtils.h"

#define __MODULE_NAME__ "DVDVideoCodecCrystalHD"

CDVDVideoCodecCrystalHD::CDVDVideoCodecCrystalHD() :
  m_Device(NULL),
  m_decode_started(false),
  m_DropPictures(false),
  m_Duration(0.0),
  m_pFormatName("")
{
}

CDVDVideoCodecCrystalHD::~CDVDVideoCodecCrystalHD()
{
  Dispose();
}

bool CDVDVideoCodecCrystalHD::Open(CDVDStreamInfo &hints, CDVDCodecOptions &options)
{
  int requestedMethod = g_guiSettings.GetInt("videoplayer.rendermethod");

  if ((requestedMethod == RENDER_METHOD_AUTO ||
       requestedMethod == RENDER_METHOD_CRYSTALHD) && !hints.software)
  {
    CRYSTALHD_CODEC_TYPE codec_type;

    switch (hints.codec)
    {
      case CODEC_ID_MPEG2VIDEO:
        codec_type = CRYSTALHD_CODEC_ID_MPEG2;
        m_pFormatName = "bcm-mpeg2";
      break;
      case CODEC_ID_H264:
        codec_type = CRYSTALHD_CODEC_ID_H264;
        m_pFormatName = "bcm-h264";
      break;
      case CODEC_ID_VC1:
        codec_type = CRYSTALHD_CODEC_ID_VC1;
        m_pFormatName = "bcm-vc1";
      break;
      case CODEC_ID_WMV3:
        codec_type = CRYSTALHD_CODEC_ID_WMV3;
        m_pFormatName = "bcm-wmv3";
      break;
      default:
        return false;
      break;
    }

    m_Device = CCrystalHD::GetInstance();
    if (!m_Device)
    {
      CLog::Log(LOGERROR, "%s: Failed to open Broadcom Crystal HD Codec", __MODULE_NAME__);
      return false;
    }

    if (m_Device && !m_Device->OpenDecoder(codec_type, hints.extrasize, hints.extradata))
    {
      CLog::Log(LOGERROR, "%s: Failed to open Broadcom Crystal HD Codec", __MODULE_NAME__);
      return false;
    }

    m_DropPictures = false;

    CLog::Log(LOGINFO, "%s: Opened Broadcom Crystal HD Codec", __MODULE_NAME__);
    return true;
  }

  return false;
}

void CDVDVideoCodecCrystalHD::Dispose(void)
{
  if (m_Device)
  {
    m_Device->CloseDecoder();
    m_Device = NULL;
  }
}

int CDVDVideoCodecCrystalHD::Decode(BYTE *pData, int iSize, double pts)
{
  int ret = 0;
  bool annexbfiltered = false;

  m_Device->BusyListPop();

  // in NULL is passed, DVDPlayer wants us to flush any internal picture frame.
  // we don't have internal picture frames so just return.
  if (!pData)
    return VC_BUFFER;

  // Handle Input
  if (pData)
  {
    if ( m_Device->AddInput(pData, iSize, pts) )
    {
      if (annexbfiltered)
        free(pData);
      pData = NULL;
    }
    else
    {
      CLog::Log(LOGDEBUG, "%s: m_pInputThread->AddInput full.", __MODULE_NAME__);
      Sleep(10);
    }
  }
  if (m_Device->GetInputCount() < 2)
    ret |= VC_BUFFER;

  // Fake a decoding delay of 1/2 the frame duration
  if (!m_DropPictures)
  {
    if (m_Duration > 0.0)
      Sleep(m_Duration/2000.0);
    else
      Sleep(20);
  }

  // Handle Output
  if (m_decode_started && m_Device->GetReadyCount())
      ret |= VC_PICTURE;
  else
  {
    if (m_Device->GetReadyCount() > 4)
    {
      m_decode_started = true;
      ret |= VC_PICTURE;
    }
  }

  return ret;
}

void CDVDVideoCodecCrystalHD::Reset(void)
{
  m_decode_started = false;
  m_Device->Reset();
}

bool CDVDVideoCodecCrystalHD::GetPicture(DVDVideoPicture* pDvdVideoPicture)
{
  bool  ret;
  
  ret = m_Device->GetPicture(pDvdVideoPicture);
  m_Duration = pDvdVideoPicture->iDuration;
  return ret;
}

void CDVDVideoCodecCrystalHD::SetDropState(bool bDrop)
{
  m_DropPictures = bDrop;
  m_Device->SetDropState(m_DropPictures);
}

#endif
