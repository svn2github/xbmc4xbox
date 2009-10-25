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
#endif

#if defined(HAVE_LIBCRYSTALHD)
#include "DVDClock.h"
#include "DVDStreamInfo.h"
#include "DVDVideoCodecCrystalHD.h"
#include "utils/log.h"
#include "utils/TimeUtils.h"

#define __MODULE_NAME__ "DVDVideoCodecCrystalHD"

class CExecTimer
{
public:
  CExecTimer() :
    m_StartTime(0),
    m_PunchInTime(0),
    m_PunchOutTime(0),
    m_LastCallInterval(0)
  {
    m_CounterFreq = CurrentHostFrequency() / 1000; // Scale to ms
  }
  
  void Start()
  {
    m_StartTime = CurrentHostCounter();
  }
  
  void PunchIn()
  {
    m_PunchInTime = CurrentHostCounter();
    if (m_PunchOutTime)
      m_LastCallInterval = m_PunchInTime - m_PunchOutTime;
    else
      m_LastCallInterval = 0;
    m_PunchOutTime = 0;
  }
  
  void PunchOut()
  {
    if (m_PunchInTime)
      m_PunchOutTime = CurrentHostCounter();
  }
  
  void Reset()
  {
    m_StartTime = 0;
    m_PunchInTime = 0;
    m_PunchOutTime = 0;
    m_LastCallInterval = 0;
  }

  uint64_t GetTimeSincePunchIn()
  {
    if (m_PunchInTime)
    {
      return (CurrentHostCounter() - m_PunchInTime)/m_CounterFreq;  
    }
    else
      return 0;
  }

  uint64_t GetElapsedTime()
  {
    if (m_StartTime)
    {
      return (CurrentHostCounter() - m_StartTime)/m_CounterFreq;  
    }
    else
      return 0;
  }
  
  uint64_t GetExecTime()
  {
    if (m_PunchOutTime && m_PunchInTime)
      return (m_PunchOutTime - m_PunchInTime)/m_CounterFreq;  
    else
      return 0;
  }
  
  uint64_t GetIntervalTime()
  {
    return m_LastCallInterval/m_CounterFreq;
  }
  
protected:
  uint64_t m_StartTime;
  uint64_t m_PunchInTime;
  uint64_t m_PunchOutTime;
  uint64_t m_LastCallInterval;
  uint64_t m_CounterFreq;
};

CExecTimer g_InputTimer;
CExecTimer g_OutputTimer;
CExecTimer g_ClientTimer;

CDVDVideoCodecCrystalHD::CDVDVideoCodecCrystalHD() :
  m_Device(NULL),
  m_DropPictures(false),
  m_pFormatName("")
{
}

CDVDVideoCodecCrystalHD::~CDVDVideoCodecCrystalHD()
{
  Dispose();
}

bool CDVDVideoCodecCrystalHD::Open(CDVDStreamInfo &hints, CDVDCodecOptions &options)
{
  BCM_CODEC_TYPE codec_type;
  BCM_STREAM_TYPE stream_type;
  
  codec_type = hints.codec;
  stream_type = BC_STREAM_TYPE_ES;
  
  //m_insert_sps_pps = init_h264_mp4toannexb_filter(hints);

  m_Device = new CCrystalHD();
  if (!m_Device)
  {
    CLog::Log(LOGERROR, "%s: Failed to open Broadcom Crystal HD", __MODULE_NAME__);
    return false;
  }
  
  if (!m_Device->Open(stream_type, codec_type))
  {
    CLog::Log(LOGERROR, "%s: Failed to open Broadcom Crystal HD", __MODULE_NAME__);
    return false;
  }

  CLog::Log(LOGDEBUG, "%s: Opened Broadcom Crystal HD", __MODULE_NAME__);
  return true;
}

void CDVDVideoCodecCrystalHD::Dispose()
{
  if (m_Device)
  {
    m_Device->Close();
    delete m_Device;
    m_Device = NULL;
  }
}

int CDVDVideoCodecCrystalHD::Decode(BYTE* pData, int iSize, double pts)
{
  int ret = 0;

  int maxWait = 40;
  unsigned int lastTime = CTimeUtils::GetTimeMS();
  unsigned int maxTime = lastTime + maxWait;
  while ((lastTime = CTimeUtils::GetTimeMS()) < maxTime)
  {
    // Handle Input
    if (pData)
    {
      if (m_Device->AddInput(pData, iSize, pts))
        pData = NULL;
      else
        CLog::Log(LOGDEBUG, "%s: m_pInputThread->AddInput full", __MODULE_NAME__);
    }
    
    // Handle Output
    if (m_Device->GetReadyCount())
      ret |= VC_PICTURE;
    
    if (m_Device->GetInputCount() < 25)
      ret |= VC_BUFFER;

    if (!pData && (ret & VC_PICTURE))
      break;
  }

  if (lastTime >= maxTime)
    CLog::Log(LOGDEBUG, "%s: Timeout in CDVDVideoCodecCrystalHD::Decode. ret: 0x%08x pData: %p", __MODULE_NAME__, ret, pData);

  if (!ret)
    ret = VC_ERROR;
    
  return ret;
}

void CDVDVideoCodecCrystalHD::Reset()
{
  m_Device->Flush();
}

bool CDVDVideoCodecCrystalHD::GetPicture(DVDVideoPicture* pDvdVideoPicture)
{
  m_Device->GetPicture(pDvdVideoPicture);
  //CLog::Log(LOGDEBUG, "%s: Fetching next decoded picture", __MODULE_NAME__);   

  return true;
}

void CDVDVideoCodecCrystalHD::SetDropState(bool bDrop)
{
  m_Device->SetDropState(bDrop);
}

bool CDVDVideoCodecCrystalHD::init_h264_mp4toannexb_filter(CDVDStreamInfo &hints)
{
  // based on h264_mp4toannexb_bsf.c (ffmpeg)
  // which is Copyright (c) 2007 Benoit Fouet <benoit.fouet@free.fr>
  // and Licensed GPL 2.1 or greater
  
  m_sps_pps_data = NULL;
  m_sps_pps_size = 0;

  // nothing to filter
  if (!hints.extradata || hints.extrasize < 6)
    return false;

  uint16_t unit_size;
  uint32_t total_size = 0;
  uint8_t *out = NULL, unit_nb, sps_done = 0;
  const uint8_t *extradata = (uint8_t*)hints.extradata + 4;
  static const uint8_t nalu_header[4] = {0, 0, 0, 1};

  // retrieve length coded size
  m_sps_pps_context.length_size = (*extradata++ & 0x3) + 1;
  if (m_sps_pps_context.length_size == 3)
    return false;

  // retrieve sps and pps unit(s)
  unit_nb = *extradata++ & 0x1f;  // number of sps unit(s)
  if (!unit_nb)
  {
    unit_nb = *extradata++;       // number of pps unit(s)
    sps_done++;
  }
  while (unit_nb--)
  {
    unit_size = extradata[0] << 8 | extradata[1];
    total_size += unit_size + 4;
    if ( (extradata + 2 + unit_size) > ((uint8_t*)hints.extradata + hints.extrasize) )
    {
      free(out);
      return false;
    }
    out = (uint8_t*)realloc(out, total_size);
    if (!out)
      return false;

    memcpy(out + total_size - unit_size - 4, nalu_header, 4);
    memcpy(out + total_size - unit_size, extradata + 2, unit_size);
    extradata += 2 + unit_size;

    if (!unit_nb && !sps_done++)
      unit_nb = *extradata++;     // number of pps unit(s)
  }

  m_sps_pps_context.sps_pps_data = out;
  m_sps_pps_context.size = total_size;
  m_sps_pps_context.first_idr = 1;

  return true;
}

void CDVDVideoCodecCrystalHD::alloc_and_copy(uint8_t **poutbuf,     int *poutbuf_size,
                                  const uint8_t *sps_pps, uint32_t sps_pps_size,
                                  const uint8_t *in,      uint32_t in_size)
{
  // based on h264_mp4toannexb_bsf.c (ffmpeg)
  // which is Copyright (c) 2007 Benoit Fouet <benoit.fouet@free.fr>
  // and Licensed GPL 2.1 or greater
  
  #define CHD_WB32(p, d) { \
    ((uint8_t*)(p))[3] = (d); \
    ((uint8_t*)(p))[2] = (d) >> 8; \
    ((uint8_t*)(p))[1] = (d) >> 16; \
    ((uint8_t*)(p))[0] = (d) >> 24; }

  uint32_t offset = *poutbuf_size;
  uint8_t nal_header_size = offset ? 3 : 4;

  *poutbuf_size += sps_pps_size + in_size + nal_header_size;
  *poutbuf = (uint8_t*)realloc(*poutbuf, *poutbuf_size);
  if (sps_pps)
  {
    memcpy(*poutbuf + offset, sps_pps, sps_pps_size);
  }
  memcpy(*poutbuf + sps_pps_size + nal_header_size + offset, in, in_size);
  if (!offset)
  {
    CHD_WB32(*poutbuf + sps_pps_size, 1);
  }
  else
  {
    (*poutbuf + offset + sps_pps_size)[0] = 0;
    (*poutbuf + offset + sps_pps_size)[1] = 0;
    (*poutbuf + offset + sps_pps_size)[2] = 1;
  }
}

bool CDVDVideoCodecCrystalHD::prepend_sps_pps_data(BYTE* pData, int iSize)
{
  // based on h264_mp4toannexb_bsf.c (ffmpeg)
  // which is Copyright (c) 2007 Benoit Fouet <benoit.fouet@free.fr>
  // and Licensed GPL 2.1 or greater
  
  uint8_t   *buf = pData;
  int       buf_size = iSize;
  uint8_t   *poutbuf = NULL;
  int       poutbuf_size = 0;
  uint8_t   unit_type;
  uint32_t  nal_size, cumul_size = 0;

  do
  {
    if (m_sps_pps_context.length_size == 1)
      nal_size = buf[0];
    else if (m_sps_pps_context.length_size == 2)
      nal_size = buf[0] << 8 | buf[1];
    else
      nal_size = buf[0] << 24 | buf[1] << 16 | buf[2] << 8 | buf[3];

    buf += m_sps_pps_context.length_size;
    unit_type = *buf & 0x1f;

    // prepend only to the first type 5 NAL unit of an IDR picture
    if (m_sps_pps_context.first_idr && unit_type == 5)
    {
      alloc_and_copy(&poutbuf, &poutbuf_size, 
        m_sps_pps_context.sps_pps_data, m_sps_pps_context.size, buf, nal_size);
      m_sps_pps_context.first_idr = 0;
    }
    else
    {
      alloc_and_copy(&poutbuf, &poutbuf_size, NULL, 0, buf, nal_size);
      if (!m_sps_pps_context.first_idr && unit_type == 1)
          m_sps_pps_context.first_idr = 1;
    }

    buf += nal_size;
    cumul_size += nal_size + m_sps_pps_context.length_size;
  } while (cumul_size < buf_size);

  return true;
}

/////////////////////////////////////////////////////////////////////


#endif
