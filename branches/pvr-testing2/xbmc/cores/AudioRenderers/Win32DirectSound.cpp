/*
* XBMC Media Center
* Copyright (c) 2002 d7o3g4q and RUNTiME
* Portions Copyright (c) by the authors of ffmpeg and xvid
*
* This program is free software; you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation; either version 2 of the License, or
* (at your option) any later version.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with this program; if not, write to the Free Software
* Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
*/

#include "system.h" // WIN32INCLUDES needed for the directsound stuff below
#include "Win32DirectSound.h"
#include "AudioContext.h"
#include "Settings.h"
#include <initguid.h>
#include <Mmreg.h>
#include "SingleLock.h"
#include "utils/log.h"
#include "utils/TimeUtils.h"
#include "CharsetConverter.h"

#pragma comment(lib, "dxguid.lib")

DEFINE_GUID( _KSDATAFORMAT_SUBTYPE_IEEE_FLOAT, WAVE_FORMAT_IEEE_FLOAT, 0x0000, 0x0010, 0x80, 0x00, 0x00, 0xaa, 0x00, 0x38, 0x9b, 0x71 );
DEFINE_GUID( _KSDATAFORMAT_SUBTYPE_PCM, WAVE_FORMAT_PCM, 0x0000, 0x0010, 0x80, 0x00, 0x00, 0xaa, 0x00, 0x38, 0x9b, 0x71 );
DEFINE_GUID( _KSDATAFORMAT_SUBTYPE_DOLBY_AC3_SPDIF, WAVE_FORMAT_DOLBY_AC3_SPDIF, 0x0000, 0x0010, 0x80, 0x00, 0x00, 0xaa, 0x00, 0x38, 0x9b, 0x71 );

const int dsound_channel_mask[] = 
{
  SPEAKER_FRONT_CENTER,
  SPEAKER_FRONT_LEFT   | SPEAKER_FRONT_RIGHT,
  SPEAKER_FRONT_LEFT   | SPEAKER_FRONT_RIGHT  | SPEAKER_LOW_FREQUENCY,
  SPEAKER_FRONT_LEFT   | SPEAKER_FRONT_RIGHT  | SPEAKER_BACK_LEFT    | SPEAKER_BACK_RIGHT,
  SPEAKER_FRONT_LEFT   | SPEAKER_FRONT_RIGHT  | SPEAKER_BACK_LEFT    | SPEAKER_BACK_RIGHT   | SPEAKER_LOW_FREQUENCY,
  SPEAKER_FRONT_LEFT   | SPEAKER_FRONT_CENTER | SPEAKER_FRONT_RIGHT  | SPEAKER_BACK_LEFT    | SPEAKER_BACK_RIGHT     | SPEAKER_LOW_FREQUENCY,
  SPEAKER_FRONT_LEFT   | SPEAKER_FRONT_RIGHT  | SPEAKER_FRONT_CENTER | SPEAKER_BACK_LEFT    | SPEAKER_BACK_RIGHT     | SPEAKER_BACK_CENTER | SPEAKER_LOW_FREQUENCY,
  SPEAKER_FRONT_LEFT   | SPEAKER_FRONT_RIGHT  | SPEAKER_FRONT_CENTER | SPEAKER_BACK_LEFT    | SPEAKER_BACK_RIGHT     | SPEAKER_SIDE_LEFT | SPEAKER_SIDE_RIGHT | SPEAKER_LOW_FREQUENCY
};

#define DSOUND_CHANNEL_MASK_COUNT 8

static BOOL CALLBACK DSEnumCallback(LPGUID lpGuid, LPCTSTR lpcstrDescription, LPCTSTR lpcstrModule, LPVOID lpContext)
{
  AudioSinkList& enumerator = *static_cast<AudioSinkList*>(lpContext);
  
  CStdString device(lpcstrDescription);
  g_charsetConverter.unknownToUTF8(device);
  
  enumerator.push_back(AudioSink(CStdString("DirectSound: ").append(device), CStdString("directsound:").append(device)));

  return TRUE;
}

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////
//***********************************************************************************************
CWin32DirectSound::CWin32DirectSound() :
  m_Passthrough(false),
  m_AvgBytesPerSec(0),
  m_CacheLen(0),
  m_dwChunkSize(0),
  m_dwBufferLen(0),
  m_PreCacheSize(0),
  m_LastCacheCheck(0),
  m_pChannelMap(NULL)
{
}

bool CWin32DirectSound::Initialize(IAudioCallback* pCallback, const CStdString& device, int iChannels, unsigned int uiSamplesPerSec, unsigned int uiBitsPerSample, bool bResample, const char* strAudioCodec, bool bIsMusic, bool bAudioPassthrough)
{
  if(iChannels > DSOUND_CHANNEL_MASK_COUNT)
  {
    CLog::Log(LOGERROR, __FUNCTION__": Unsupported number of channels. %i specified while max is %i", iChannels, DSOUND_CHANNEL_MASK_COUNT);
    return false;
  }

  bool bAudioOnAllSpeakers(false);
  g_audioContext.SetupSpeakerConfig(iChannels, bAudioOnAllSpeakers, bIsMusic);
  if(bAudioPassthrough)
    g_audioContext.SetActiveDevice(CAudioContext::DIRECTSOUND_DEVICE_DIGITAL);
  else
    g_audioContext.SetActiveDevice(CAudioContext::DIRECTSOUND_DEVICE);
  m_pDSound=g_audioContext.GetDirectSoundDevice();

  m_bPause = false;
  m_bIsAllocated = false;
  m_pBuffer = NULL;
  m_uiChannels = iChannels;
  m_uiSamplesPerSec = uiSamplesPerSec;
  m_uiBitsPerSample = uiBitsPerSample;
  m_Passthrough = bAudioPassthrough;

  m_nCurrentVolume = g_settings.m_nVolumeLevel;
  
  WAVEFORMATEXTENSIBLE wfxex = {0};

  //fill waveformatex
  ZeroMemory(&wfxex, sizeof(WAVEFORMATEXTENSIBLE));
  wfxex.Format.cbSize          =  sizeof(WAVEFORMATEXTENSIBLE)-sizeof(WAVEFORMATEX);
  wfxex.Format.nChannels       = iChannels;
  wfxex.Format.nSamplesPerSec  = uiSamplesPerSec;
  if (bAudioPassthrough == true) 
  {
    wfxex.dwChannelMask          = SPEAKER_FRONT_LEFT | SPEAKER_FRONT_RIGHT;
    wfxex.Format.wFormatTag      = WAVE_FORMAT_DOLBY_AC3_SPDIF;
    wfxex.SubFormat              = _KSDATAFORMAT_SUBTYPE_DOLBY_AC3_SPDIF;
    wfxex.Format.wBitsPerSample  = 16;
    wfxex.Format.nChannels       = 2;
  } 
  else
  {
    wfxex.dwChannelMask          = dsound_channel_mask[iChannels - 1];

    if (iChannels > 2)
      wfxex.Format.wFormatTag    = WAVE_FORMAT_EXTENSIBLE;
    else
      wfxex.Format.wFormatTag    = WAVE_FORMAT_PCM;

    wfxex.SubFormat              = _KSDATAFORMAT_SUBTYPE_PCM;
    wfxex.Format.wBitsPerSample  = uiBitsPerSample;
  }

  wfxex.Samples.wValidBitsPerSample = wfxex.Format.wBitsPerSample;
  wfxex.Format.nBlockAlign       = wfxex.Format.nChannels * (wfxex.Format.wBitsPerSample >> 3);
  wfxex.Format.nAvgBytesPerSec   = wfxex.Format.nSamplesPerSec * wfxex.Format.nBlockAlign;

  m_AvgBytesPerSec = wfxex.Format.nAvgBytesPerSec;

  // unsure if these are the right values
  m_dwChunkSize = wfxex.Format.nBlockAlign * 3096;
  m_PreCacheSize = m_dwChunkSize;
  m_dwBufferLen = m_dwChunkSize * 16;

  CLog::Log(LOGDEBUG, __FUNCTION__": Packet Size = %d. Avg Bytes Per Second = %d.", m_dwChunkSize, m_AvgBytesPerSec);

  // fill in the secondary sound buffer descriptor
  DSBUFFERDESC dsbdesc;
  memset(&dsbdesc, 0, sizeof(DSBUFFERDESC));
  dsbdesc.dwSize = sizeof(DSBUFFERDESC);
  dsbdesc.dwFlags = DSBCAPS_GETCURRENTPOSITION2 /** Better position accuracy */
                  | DSBCAPS_GLOBALFOCUS         /** Allows background playing */
                  | DSBCAPS_CTRLVOLUME          /** volume control enabled */
                  | DSBCAPS_LOCHARDWARE;         /** Needed for 5.1 on emu101k  */

  dsbdesc.dwBufferBytes = m_dwBufferLen;
  dsbdesc.lpwfxFormat = (WAVEFORMATEX *)&wfxex;

  // now create the stream buffer
  HRESULT res = IDirectSound_CreateSoundBuffer(m_pDSound, &dsbdesc, &m_pBuffer, NULL);
  if (res != DS_OK) 
  {
    if (dsbdesc.dwFlags & DSBCAPS_LOCHARDWARE) // DSBCAPS_LOCHARDWARE Always fails on Vista, by design
    {
      SAFE_RELEASE(m_pBuffer);
      CLog::Log(LOGDEBUG, __FUNCTION__": Couldn't create secondary buffer (%s). Trying without LOCHARDWARE.", dserr2str(res));
      // Try without DSBCAPS_LOCHARDWARE
      dsbdesc.dwFlags &= ~DSBCAPS_LOCHARDWARE;
      res = IDirectSound_CreateSoundBuffer(m_pDSound, &dsbdesc, &m_pBuffer, NULL);
    }
    if (res != DS_OK && dsbdesc.dwFlags & DSBCAPS_CTRLVOLUME) 
    {
      SAFE_RELEASE(m_pBuffer);
      CLog::Log(LOGDEBUG, __FUNCTION__": Couldn't create secondary buffer (%s). Trying without CTRLVOLUME.", dserr2str(res));
      // Try without DSBCAPS_CTRLVOLUME
      dsbdesc.dwFlags &= ~DSBCAPS_CTRLVOLUME;
      res = IDirectSound_CreateSoundBuffer(m_pDSound, &dsbdesc, &m_pBuffer, NULL);
    }
    if (res != DS_OK) 
    {
      SAFE_RELEASE(m_pBuffer);
      CLog::Log(LOGERROR, __FUNCTION__": cannot create secondary buffer (%s)", dserr2str(res));
      return false;
    }
  }
  CLog::Log(LOGDEBUG, __FUNCTION__": secondary buffer created");

  // Set up channel mapping
  SetChannelMap(iChannels, uiBitsPerSample, bAudioPassthrough, strAudioCodec);

  m_pBuffer->Stop();
  
  if (DSERR_CONTROLUNAVAIL == m_pBuffer->SetVolume(g_settings.m_nVolumeLevel))
    CLog::Log(LOGINFO, __FUNCTION__": Volume control is unavailable in the current configuration");

  m_bIsAllocated = true;
  m_BufferOffset = 0;
  m_CacheLen = 0;
  m_LastCacheCheck = CTimeUtils::GetTimeMS();
  
  return m_bIsAllocated;
}

//***********************************************************************************************
CWin32DirectSound::~CWin32DirectSound()
{
  Deinitialize();
}

//***********************************************************************************************
bool CWin32DirectSound::Deinitialize()
{
  if (m_bIsAllocated)
  {
    CLog::Log(LOGDEBUG, __FUNCTION__": Cleaning up");
    m_bIsAllocated = false;
    if (m_pBuffer)
    {
      m_pBuffer->Stop();
      SAFE_RELEASE(m_pBuffer);
    }

    m_pBuffer = NULL;
    m_pDSound = NULL;  
    m_BufferOffset = 0;
    m_CacheLen = 0;
    m_dwChunkSize = 0;
    m_dwBufferLen = 0;

    g_audioContext.SetActiveDevice(CAudioContext::DEFAULT_DEVICE);
  }
  return true;
}

//***********************************************************************************************
bool CWin32DirectSound::Pause()
{
  CSingleLock lock (m_critSection);
  if (m_bPause) // Already paused
    return true;
  m_bPause = true;
  m_pBuffer->Stop();

  return true;
}

//***********************************************************************************************
bool CWin32DirectSound::Resume()
{
  CSingleLock lock (m_critSection);
  if (!m_bPause) // Already playing
    return true;
  m_bPause = false;
  if (m_CacheLen > m_PreCacheSize) // Make sure we have some data to play (if not, playback will start when we add some)
    m_pBuffer->Play(0, 0, DSBPLAY_LOOPING);

  return true;
}

//***********************************************************************************************
bool CWin32DirectSound::Stop()
{
  CSingleLock lock (m_critSection);
  // Stop and reset DirectSound buffer
  m_pBuffer->Stop();
  m_pBuffer->SetCurrentPosition(0);

  // Reset buffer management members
  m_BufferOffset = 0;
  m_CacheLen = 0;
  m_bPause = false;

  return true;
}

//***********************************************************************************************
long CWin32DirectSound::GetCurrentVolume() const
{
  return m_nCurrentVolume;
}

//***********************************************************************************************
void CWin32DirectSound::Mute(bool bMute)
{
  CSingleLock lock (m_critSection);
  if (!m_bIsAllocated) return;
  if (bMute)
    m_pBuffer->SetVolume(VOLUME_MINIMUM);
  else
    m_pBuffer->SetVolume(m_nCurrentVolume);
}

//***********************************************************************************************
bool CWin32DirectSound::SetCurrentVolume(long nVolume)
{
  CSingleLock lock (m_critSection);
  if (!m_bIsAllocated) return false;
  m_nCurrentVolume = nVolume;
  return m_pBuffer->SetVolume( m_nCurrentVolume ) == S_OK;
}

//***********************************************************************************************
unsigned int CWin32DirectSound::AddPackets(const void* data, unsigned int len)
{
  CSingleLock lock (m_critSection);
  DWORD total = len;
  unsigned char* pBuffer = (unsigned char*)data;

  DWORD bufferStatus = 0;
  m_pBuffer->GetStatus(&bufferStatus);
  if (bufferStatus & DSBSTATUS_BUFFERLOST)
  {
    CLog::Log(LOGDEBUG, __FUNCTION__ ": Buffer allocation was lost. Restoring buffer.");
    m_pBuffer->Restore();
  }

  while (len >= m_dwChunkSize && GetSpace() >= m_dwChunkSize) // We want to write at least one chunk at a time
  {
    LPVOID start = NULL, startWrap = NULL;
    DWORD size = 0, sizeWrap = 0;
    if (m_BufferOffset >= m_dwBufferLen) // Wrap-around manually
      m_BufferOffset = 0;
    HRESULT res = m_pBuffer->Lock(m_BufferOffset, m_dwChunkSize, &start, &size, &startWrap, &sizeWrap, 0);
    if (DS_OK != res)
    { 
      CLog::Log(LOGERROR, __FUNCTION__ ": Unable to lock buffer at offset %u. HRESULT: 0x%08x", m_BufferOffset, res);
      break;
    }

    // Write data into the buffer
    MapDataIntoBuffer(pBuffer, size, (unsigned char*)start);
    m_BufferOffset += size;
    if (startWrap) // Write-region wraps to beginning of buffer
    {
      MapDataIntoBuffer(pBuffer + size, sizeWrap, (unsigned char*)startWrap);
      m_BufferOffset = sizeWrap;
    }
    
    size_t bytes = size + sizeWrap;
    m_CacheLen += bytes; // This data is now in the cache
    pBuffer += bytes; // Update buffer pointer
    len -= bytes; // Update remaining data len

    m_pBuffer->Unlock(start, size, startWrap, sizeWrap);
  }

  CheckPlayStatus();

  return total - len; // Bytes used
}

void CWin32DirectSound::UpdateCacheStatus()
{
  CSingleLock lock (m_critSection);
  // TODO: Check to see if we may have cycled around since last time
  unsigned int time = CTimeUtils::GetTimeMS();
  if (time == m_LastCacheCheck)
    return; // Don't recalc more frequently than once/ms (that is our max resolution anyway)

  DWORD playCursor = 0, writeCursor = 0;
  HRESULT res = m_pBuffer->GetCurrentPosition(&playCursor, &writeCursor); // Get the current playback and safe write positions
  if (DS_OK != res)
  {
    CLog::Log(LOGERROR,__FUNCTION__ ": GetCurrentPosition failed. Unable to determine buffer status. HRESULT = 0x%08x", res);
    return;
  }

  m_LastCacheCheck = time;
  // Check the state of the ring buffer (P->O->W == underrun)
  // These are the logical situations that can occur
  // O: CurrentOffset  W: WriteCursor  P: PlayCursor
  // | | | | | | | | | |
  // ***O----W----P***** < underrun   P > W && O < W (1)
  // | | | | | | | | | |
  // ---P****O----W----- < underrun   O > P && O < W (2)
  // | | | | | | | | | |
  // ---W----P****O----- < underrun   P > W && P < O (3)
  // | | | | | | | | | |
  // ***W****O----P*****              P > W && P > O (4)
  // | | | | | | | | | |
  // ---P****W****O-----              P < W && O > W (5)
  // | | | | | | | | | |
  // ***O----P****W*****              P < W && O < P (6)

  // Check for underruns
  if ((playCursor > writeCursor && m_BufferOffset < writeCursor) ||    // (1)
      (playCursor < m_BufferOffset && m_BufferOffset < writeCursor) || // (2)
      (playCursor > writeCursor && playCursor <  m_BufferOffset))      // (3)
  { 
    CLog::Log(LOGWARNING, "CWin32DirectSound::GetSpace - buffer underrun - W:%u, P:%u, O:%u.", writeCursor, playCursor, m_BufferOffset);
    m_BufferOffset = writeCursor; // Catch up
    m_pBuffer->Stop(); // Wait until someone gives us some data to restart playback (prevents glitches)
  }

  // Calculate available space in the ring buffer
  if (playCursor == m_BufferOffset && m_BufferOffset ==  writeCursor) // Playback is stopped and we are all at the same place
    m_CacheLen = 0;
  else if (m_BufferOffset > playCursor)
    m_CacheLen = m_BufferOffset - playCursor;
  else
    m_CacheLen = m_dwBufferLen - (playCursor - m_BufferOffset);
}

void CWin32DirectSound::CheckPlayStatus()
{
  DWORD status = 0;
  m_pBuffer->GetStatus(&status);

  if(!m_bPause && !(status & DSBSTATUS_PLAYING) && m_CacheLen > m_PreCacheSize) // If we have some data, see if we can start playback
  {
    m_pBuffer->Play(0, 0, DSBPLAY_LOOPING);
    CLog::Log(LOGDEBUG,__FUNCTION__ ": Resuming Playback");
  }
}

unsigned int CWin32DirectSound::GetSpace()
{
  CSingleLock lock (m_critSection);
  UpdateCacheStatus();

  return m_dwBufferLen - m_CacheLen;
}

//***********************************************************************************************
float CWin32DirectSound::GetDelay()
{
  // Make sure we know how much data is in the cache
  UpdateCacheStatus();

  CSingleLock lock (m_critSection);
  float delay  = 0.008f; // WTF?
  delay += (float)m_CacheLen / (float)m_AvgBytesPerSec;
  return delay;
}

//***********************************************************************************************
float CWin32DirectSound::GetCacheTime()
{
  CSingleLock lock (m_critSection);
  // Make sure we know how much data is in the cache
  UpdateCacheStatus();

  return (float)m_CacheLen / (float)m_AvgBytesPerSec;
}

float CWin32DirectSound::GetCacheTotal()
{
  CSingleLock lock (m_critSection);
  return (float)m_dwBufferLen / (float)m_AvgBytesPerSec;
}

//***********************************************************************************************
unsigned int CWin32DirectSound::GetChunkLen()
{
  return m_dwChunkSize;
}

//***********************************************************************************************
int CWin32DirectSound::SetPlaySpeed(int iSpeed)
{
  return 0;
}

//***********************************************************************************************
void CWin32DirectSound::RegisterAudioCallback(IAudioCallback *pCallback)
{
  m_pCallback = pCallback;
}

//***********************************************************************************************
void CWin32DirectSound::UnRegisterAudioCallback()
{
  m_pCallback = NULL;
}

//***********************************************************************************************
void CWin32DirectSound::WaitCompletion()
{
  CSingleLock lock (m_critSection);
  DWORD status, timeout;
  unsigned char* silence;

  if (!m_pBuffer)
    return ;

  if(FAILED(m_pBuffer->GetStatus(&status)) || (status & DSBSTATUS_PLAYING) == 0)
    return; // We weren't playing anyway

  // The drain should complete in the time occupied by the cache
  timeout  = (DWORD)(1000 * GetDelay());
  timeout += CTimeUtils::GetTimeMS();
  silence  = (unsigned char*)calloc(1,m_dwChunkSize); // Initialize 'silence' to zero...

  while(AddPackets(silence, m_dwChunkSize) == 0)
  {
    if(FAILED(m_pBuffer->GetStatus(&status)) || (status & DSBSTATUS_PLAYING) == 0)
      break;

    if(timeout < CTimeUtils::GetTimeMS())
    {
      CLog::Log(LOGWARNING, __FUNCTION__ ": timeout adding silence to buffer");
      break;
    }
  }
  free(silence);

  while(m_CacheLen)
  {
    if(FAILED(m_pBuffer->GetStatus(&status)) || (status & DSBSTATUS_PLAYING) == 0)
      break;

    if(timeout < CTimeUtils::GetTimeMS())
    {
      CLog::Log(LOGDEBUG, "CWin32DirectSound::WaitCompletion - timeout waiting for silence");
      break;
    }
    else
      Sleep(1);
    GetSpace();
  }

  m_pBuffer->Stop();
}

//***********************************************************************************************
void CWin32DirectSound::SwitchChannels(int iAudioStream, bool bAudioOnAllSpeakers)
{
  return;
}

//***********************************************************************************************

void CWin32DirectSound::EnumerateAudioSinks(AudioSinkList &vAudioSinks, bool passthrough)
{
  if (FAILED(DirectSoundEnumerate(DSEnumCallback, &vAudioSinks)))
    CLog::Log(LOGERROR, "%s - failed to enumerate output devices", __FUNCTION__);
}

//***********************************************************************************************
char * CWin32DirectSound::dserr2str(int err)
{
  switch (err) 
  {
    case DS_OK: return "DS_OK";
    case DS_NO_VIRTUALIZATION: return "DS_NO_VIRTUALIZATION";
    case DSERR_ALLOCATED: return "DS_NO_VIRTUALIZATION";
    case DSERR_CONTROLUNAVAIL: return "DSERR_CONTROLUNAVAIL";
    case DSERR_INVALIDPARAM: return "DSERR_INVALIDPARAM";
    case DSERR_INVALIDCALL: return "DSERR_INVALIDCALL";
    case DSERR_GENERIC: return "DSERR_GENERIC";
    case DSERR_PRIOLEVELNEEDED: return "DSERR_PRIOLEVELNEEDED";
    case DSERR_OUTOFMEMORY: return "DSERR_OUTOFMEMORY";
    case DSERR_BADFORMAT: return "DSERR_BADFORMAT";
    case DSERR_UNSUPPORTED: return "DSERR_UNSUPPORTED";
    case DSERR_NODRIVER: return "DSERR_NODRIVER";
    case DSERR_ALREADYINITIALIZED: return "DSERR_ALREADYINITIALIZED";
    case DSERR_NOAGGREGATION: return "DSERR_NOAGGREGATION";
    case DSERR_BUFFERLOST: return "DSERR_BUFFERLOST";
    case DSERR_OTHERAPPHASPRIO: return "DSERR_OTHERAPPHASPRIO";
    case DSERR_UNINITIALIZED: return "DSERR_UNINITIALIZED";
    case DSERR_NOINTERFACE: return "DSERR_NOINTERFACE";
    case DSERR_ACCESSDENIED: return "DSERR_ACCESSDENIED";
    default: return "unknown";
  }
}
