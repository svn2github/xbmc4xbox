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
#ifdef HAS_PULSEAUDIO
#include "PulseAudioDirectSound.h"
#include "AudioContext.h"
#include "Util.h"


static const char *ContextStateToString(pa_context_state s)
{
    if (s == PA_CONTEXT_UNCONNECTED)
      return "unconnected";
    if (s == PA_CONTEXT_CONNECTING)
      return "connecting";
    if (s == PA_CONTEXT_AUTHORIZING)
      return "authorizing";
    if (s == PA_CONTEXT_SETTING_NAME)
      return "setting name";
    if (s == PA_CONTEXT_READY)
      return "ready";
    if (s == PA_CONTEXT_FAILED)
      return "failed";
    if (s == PA_CONTEXT_TERMINATED)
      return "terminated";

    return "none";
}

static const char *StreamStateToString(pa_stream_state s)
{
    if (s == PA_STREAM_UNCONNECTED)
      return "unconnected";
    if (s == PA_STREAM_CREATING)
      return "creating";
    if (s == PA_STREAM_READY)
      return "ready";
    if (s == PA_STREAM_FAILED)
      return "failed";
    if (s == PA_STREAM_TERMINATED)
      return "terminated";

    return "none";
}

/* Static callback functions */

static void ContextStateCallback(pa_context *c, void *userdata)
{
  pa_threaded_mainloop *m = (pa_threaded_mainloop *)userdata;
  switch (pa_context_get_state(c))
  {
    case PA_CONTEXT_READY:
    case PA_CONTEXT_TERMINATED:
    case PA_CONTEXT_UNCONNECTED:
    case PA_CONTEXT_CONNECTING:
    case PA_CONTEXT_AUTHORIZING:
    case PA_CONTEXT_SETTING_NAME:
    case PA_CONTEXT_FAILED:
      pa_threaded_mainloop_signal(m, 0);
      break;
  }
}

static void StreamStateCallback(pa_stream *s, void *userdata)
{
  pa_threaded_mainloop *m = (pa_threaded_mainloop *)userdata;
  switch (pa_stream_get_state(s))
  {
    case PA_STREAM_UNCONNECTED:
    case PA_STREAM_CREATING:
    case PA_STREAM_READY:
    case PA_STREAM_FAILED:
    case PA_STREAM_TERMINATED:
      pa_threaded_mainloop_signal(m, 0);
      break;
  }
}

static void StreamRequestCallback(pa_stream *s, size_t length, void *userdata)
{
  pa_threaded_mainloop *m = (pa_threaded_mainloop *)userdata;
  pa_threaded_mainloop_signal(m, 0);
}

static void StreamLatencyUpdateCallback(pa_stream *s, void *userdata)
{
  pa_threaded_mainloop *m = (pa_threaded_mainloop *)userdata;
  pa_threaded_mainloop_signal(m, 0);
}


/* PulseAudio class memberfunctions*/

CPulseAudioDirectSound::CPulseAudioDirectSound()
{
}

bool CPulseAudioDirectSound::Initialize(IAudioCallback* pCallback, int iChannels, unsigned int uiSamplesPerSec, unsigned int uiBitsPerSample, bool bResample, const char* strAudioCodec, bool bIsMusic, bool bPassthrough)
{
  CLog::Log(LOGDEBUG,"PulseAudio: Opening Channels: %i - SampleRate: %i - SampleBit: %i - Resample %s - Codec %s - IsMusic %s - IsPassthrough %s - audioHost: %s - audioDevice: %s", iChannels, uiSamplesPerSec, uiBitsPerSample, bResample ? "true" : "false", strAudioCodec, bIsMusic ? "true" : "false", bPassthrough ? "true" : "false", g_advancedSettings.m_audioHost.c_str(), g_guiSettings.GetString("audiooutput.audiodevice").c_str());
  if (iChannels == 0)
    iChannels = 2;

  bool bAudioOnAllSpeakers(false);
  g_audioContext.SetupSpeakerConfig(iChannels, bAudioOnAllSpeakers, bIsMusic);
  g_audioContext.SetActiveDevice(CAudioContext::DIRECTSOUND_DEVICE);

  m_Context = NULL;
  m_Stream = NULL;
  m_MainLoop = NULL;
  m_bPause = false;
  m_bIsAllocated = false;
  m_uiChannels = iChannels;
  m_uiSamplesPerSec = uiSamplesPerSec;
  m_uiBufferSize = 0;
  m_uiBitsPerSample = uiBitsPerSample;
  m_bPassthrough = bPassthrough;
  m_uiBytesPerSecond = uiSamplesPerSec * (uiBitsPerSample / 8) * iChannels;

  m_nCurrentVolume = g_stSettings.m_nVolumeLevel;

  m_dwPacketSize = iChannels*(uiBitsPerSample/8)*512;
  m_dwNumPackets = 16;

  /* Open the device */
  CStdString device, deviceuse;
  if (!m_bPassthrough)
    device = g_guiSettings.GetString("audiooutput.audiodevice");
  else
    device = g_guiSettings.GetString("audiooutput.passthroughdevice");

  if (m_bPassthrough)
  {
    CLog::Log(LOGERROR, "PulseAudio: Passthrough not possible");
    return false;
  }

  const char *host = NULL;
  if (strcmp(g_advancedSettings.m_audioHost, "default") != 0)
    host = g_advancedSettings.m_audioHost.c_str();
  const char *sink = NULL;
  if (strcmp(device, "default") != 0)
    sink = device.c_str();

  struct pa_channel_map map;

  m_SampleSpec.channels = iChannels;
  m_SampleSpec.rate = uiSamplesPerSec;
  m_SampleSpec.format = PA_SAMPLE_S16LE;  

  if (!pa_sample_spec_valid(&m_SampleSpec)) 
  {
    CLog::Log(LOGERROR, "PulseAudio: Invalid sample spec");
    Deinitialize();
    return false;
  }

  if (strstr(strAudioCodec, "DMO") || strstr(strAudioCodec, "FLAC") || strstr(strAudioCodec, "PCM"))
    pa_channel_map_init_auto(&map, m_SampleSpec.channels, PA_CHANNEL_MAP_WAVEEX);
  else
    pa_channel_map_init_auto(&map, m_SampleSpec.channels, PA_CHANNEL_MAP_ALSA);

  pa_cvolume_reset(&m_Volume, m_SampleSpec.channels);

  if ((m_MainLoop = pa_threaded_mainloop_new()) == NULL)
  {
    CLog::Log(LOGERROR, "PulseAudio: Failed to allocate main loop");
    Deinitialize();
    return false;
  }

  if ((m_Context = pa_context_new(pa_threaded_mainloop_get_api(m_MainLoop), "XBMC")) == NULL)
  {
    CLog::Log(LOGERROR, "PulseAudio: Failed to allocate context");
    Deinitialize();
    return false;
  }

  pa_context_set_state_callback(m_Context, ContextStateCallback, m_MainLoop);

  if (pa_context_connect(m_Context, host, (pa_context_flags_t)0, NULL) < 0)
  {
    CLog::Log(LOGERROR, "PulseAudio: Failed to connect context");
    Deinitialize();
    return false;
  }
  pa_threaded_mainloop_lock(m_MainLoop);

  if (pa_threaded_mainloop_start(m_MainLoop) < 0)
  {
    CLog::Log(LOGERROR, "PulseAudio: Failed to start MainLoop");
    if (m_MainLoop)
      pa_threaded_mainloop_unlock(m_MainLoop);
    Deinitialize();
    return false;
  }

  /* Wait until the context is ready */
  do
  {  
    pa_threaded_mainloop_wait(m_MainLoop);
    CLog::Log(LOGDEBUG, "PulseAudio: Context %s", ContextStateToString(pa_context_get_state(m_Context)));
  }
  while (pa_context_get_state(m_Context) != PA_CONTEXT_READY && pa_context_get_state(m_Context) != PA_CONTEXT_FAILED);

  if (pa_context_get_state(m_Context) == PA_CONTEXT_FAILED)
  {
    CLog::Log(LOGERROR, "PulseAudio: Waited for the Context but it failed");
    if (m_MainLoop)
      pa_threaded_mainloop_unlock(m_MainLoop);
    Deinitialize();
    return false;
  }

  if ((m_Stream = pa_stream_new(m_Context, "audio stream", &m_SampleSpec, &map)) == NULL)
  {
    CLog::Log(LOGERROR, "PulseAudio: Could not create a stream");
    if (m_MainLoop)
      pa_threaded_mainloop_unlock(m_MainLoop);
    Deinitialize();
    return false;
  }

  pa_stream_set_state_callback(m_Stream, StreamStateCallback, m_MainLoop);
  pa_stream_set_write_callback(m_Stream, StreamRequestCallback, m_MainLoop);
  pa_stream_set_latency_update_callback(m_Stream, StreamLatencyUpdateCallback, m_MainLoop);

  if (pa_stream_connect_playback(m_Stream, sink, NULL, ((pa_stream_flags)(PA_STREAM_INTERPOLATE_TIMING | PA_STREAM_AUTO_TIMING_UPDATE)), &m_Volume, NULL) < 0)
  {
    CLog::Log(LOGERROR, "PulseAudio: Failed to connect stream to output");
    if (m_MainLoop)
      pa_threaded_mainloop_unlock(m_MainLoop);
    Deinitialize();
    return false;
  }

  /* Wait until the stream is ready */
  do
  {
    pa_threaded_mainloop_wait(m_MainLoop);
    CLog::Log(LOGDEBUG, "PulseAudio: Stream %s", StreamStateToString(pa_stream_get_state(m_Stream)));
  }
  while (pa_stream_get_state(m_Stream) != PA_STREAM_READY && pa_stream_get_state(m_Stream) != PA_STREAM_FAILED);

  if (pa_stream_get_state(m_Stream) == PA_STREAM_FAILED)
  {
    CLog::Log(LOGERROR, "PulseAudio: Waited for the stream but it failed");
    if (m_MainLoop)
      pa_threaded_mainloop_unlock(m_MainLoop);
    Deinitialize();
    return false;
  }

  const pa_buffer_attr *a;

  if (!(a = pa_stream_get_buffer_attr(m_Stream)))
      CLog::Log(LOGERROR, "PulseAudio: %s", pa_strerror(pa_context_errno(m_Context)));
  else
  {
    m_dwPacketSize = a->minreq;
  CLog::Log(LOGDEBUG, "PulseAudio: Default buffer attributes, maxlength=%u, tlength=%u, prebuf=%u, minreq=%u", a->maxlength, a->tlength, a->prebuf, a->minreq);
    pa_buffer_attr b;
    b.prebuf = a->minreq * 10;
    b.minreq = a->minreq;
    b.tlength = m_uiBufferSize = a->tlength;
    b.maxlength = a->maxlength;
    b.fragsize = a->fragsize;

    WaitForOperation(pa_stream_set_buffer_attr(m_Stream, &b, NULL, NULL), "SetBuffer");


    if (!(a = pa_stream_get_buffer_attr(m_Stream)))
        CLog::Log(LOGERROR, "PulseAudio: %s", pa_strerror(pa_context_errno(m_Context)));
    else
    {
      m_dwPacketSize = a->minreq;
      m_uiBufferSize = a->tlength;
      CLog::Log(LOGDEBUG, "PulseAudio: Choosen buffer attributes, maxlength=%u, tlength=%u, prebuf=%u, minreq=%u", a->maxlength, a->tlength, a->prebuf, a->minreq);
    }
  }
  
  pa_threaded_mainloop_unlock(m_MainLoop);

  m_bIsAllocated = true;

  SetCurrentVolume(m_nCurrentVolume);
  m_bPause = Cork(false);
  return true;
}

CPulseAudioDirectSound::~CPulseAudioDirectSound()
{
  Deinitialize();
}

bool CPulseAudioDirectSound::Deinitialize()
{
  m_bIsAllocated = false;
  if (m_Context)
  {
    if (m_Stream)
      WaitCompletion();

    if (m_MainLoop)
        pa_threaded_mainloop_stop(m_MainLoop);

    if (m_Stream)
    {
        pa_stream_disconnect(m_Stream);
        pa_stream_unref(m_Stream);
        m_Stream = NULL;
    }

    if (m_Context)
    {
      pa_context_disconnect(m_Context);
      pa_context_unref(m_Context);
      m_Context = NULL;
    }

    if (m_MainLoop)
    {
      pa_threaded_mainloop_free(m_MainLoop);
      m_MainLoop = NULL;
    }
  }

  g_audioContext.SetActiveDevice(CAudioContext::DEFAULT_DEVICE);
  return true;
}

inline bool CPulseAudioDirectSound::WaitForOperation(pa_operation *op, const char *LogEntry = "")
{
  if (op == NULL)
    return false;

  bool sucess = true;

  while (pa_operation_get_state(op) == PA_OPERATION_RUNNING)
    pa_threaded_mainloop_wait(m_MainLoop);

  if (pa_operation_get_state(op) != PA_OPERATION_DONE)
  {
    CLog::Log(LOGERROR, "PulseAudio: %s Operation failed", LogEntry);
    sucess = false;
  }

  pa_operation_unref(op);
  return sucess;
}

void CPulseAudioDirectSound::Flush()
{
  if (!m_bIsAllocated)
     return;

  pa_threaded_mainloop_lock(m_MainLoop);
  WaitForOperation(pa_stream_flush(m_Stream, NULL, NULL), "Flush");
  pa_threaded_mainloop_unlock(m_MainLoop);
}

bool CPulseAudioDirectSound::Cork(bool cork)
{
  pa_threaded_mainloop_lock(m_MainLoop);

  if (!WaitForOperation(pa_stream_cork(m_Stream, cork ? 1 : 0, NULL, NULL), cork ? "Pause" : "Resume"))
    cork = !cork;

  pa_threaded_mainloop_unlock(m_MainLoop);

  return cork;
}

bool CPulseAudioDirectSound::Pause()
{
  if (!m_bIsAllocated)
    return -1;

  if (m_bPause) 
    return true;

  m_bPause = Cork(true);

  return m_bPause;
}

bool CPulseAudioDirectSound::Resume()
{
  if (!m_bIsAllocated)
     return -1;
  if(m_bPause)
    m_bPause = Cork(false);

  return !m_bPause;
}

bool CPulseAudioDirectSound::Stop()
{
  if (!m_bIsAllocated)
    return -1;

  Flush();

  m_bPause = false;

  return true;
}

long CPulseAudioDirectSound::GetCurrentVolume() const
{
  return m_nCurrentVolume;
}

void CPulseAudioDirectSound::Mute(bool bMute)
{
  if (!m_bIsAllocated) 
    return;

  if (bMute)
    SetCurrentVolume(VOLUME_MINIMUM);
  else
    SetCurrentVolume(m_nCurrentVolume);
}

bool CPulseAudioDirectSound::SetCurrentVolume(long nVolume)
{
  if (!m_bIsAllocated || m_bPassthrough)
    return -1;

  pa_threaded_mainloop_lock(m_MainLoop);
  pa_volume_t volume = pa_sw_volume_from_dB((float)nVolume / 200.0f);
  /*if ( nVolume <= GetMinimumVolume())
    pa_cvolume_mute(&m_Volume, m_SampleSpec.channels);
  else */
    pa_cvolume_set(&m_Volume, m_SampleSpec.channels, volume);
  pa_operation *op = pa_context_set_sink_input_volume(m_Context, pa_stream_get_index(m_Stream), &m_Volume, NULL, NULL);
  if (op == NULL)
    CLog::Log(LOGERROR, "PulseAudio: Failed to set volume");
  else
    pa_operation_unref(op);

  pa_threaded_mainloop_unlock(m_MainLoop);

  return true;
}

unsigned int CPulseAudioDirectSound::GetSpace()
{
  if (!m_bIsAllocated)
    return 0;

  size_t l;
  pa_threaded_mainloop_lock(m_MainLoop);
  l = pa_stream_writable_size(m_Stream);
  pa_threaded_mainloop_unlock(m_MainLoop);
  return l;
}

unsigned int CPulseAudioDirectSound::AddPackets(const void* data, unsigned int len)
{
  if (!m_bIsAllocated)
    return len;

  pa_threaded_mainloop_lock(m_MainLoop);
  int length = std::min((int)GetSpace(), (int)len);

  int rtn = pa_stream_write(m_Stream, data, length, NULL, 0, PA_SEEK_RELATIVE);

  pa_threaded_mainloop_unlock(m_MainLoop);
  return length - rtn;
}

float CPulseAudioDirectSound::GetCacheTime()
{
  return (float)(m_uiBufferSize - GetSpace()) / (float)m_uiBytesPerSecond;
}

float CPulseAudioDirectSound::GetDelay()
{
  if (!m_bIsAllocated)
    return 0;

  pa_usec_t latency = (pa_usec_t) -1;
  pa_threaded_mainloop_lock(m_MainLoop);
  while (pa_stream_get_latency(m_Stream, &latency, NULL) < 0)
  {
    if (pa_context_errno(m_Context) != PA_ERR_NODATA)
    {
      CLog::Log(LOGERROR, "PulseAudio: pa_stream_get_latency() failed");
      break;
    }
    /* Wait until latency data is available again */
    pa_threaded_mainloop_wait(m_MainLoop);
  }
  pa_threaded_mainloop_unlock(m_MainLoop);
  return latency / 1000000.0;
}

unsigned int CPulseAudioDirectSound::GetChunkLen()
{
  return m_dwPacketSize;
}

int CPulseAudioDirectSound::SetPlaySpeed(int iSpeed)
{
  return 0;
}

void CPulseAudioDirectSound::RegisterAudioCallback(IAudioCallback *pCallback)
{
  m_pCallback = pCallback;
}

void CPulseAudioDirectSound::UnRegisterAudioCallback()
{
  m_pCallback = NULL;
}

void CPulseAudioDirectSound::WaitCompletion()
{
  if (!m_bIsAllocated)
    return;

  pa_threaded_mainloop_lock(m_MainLoop);
  WaitForOperation(pa_stream_drain(m_Stream, NULL, NULL), "Drain");
  pa_threaded_mainloop_unlock(m_MainLoop);
}

void CPulseAudioDirectSound::SwitchChannels(int iAudioStream, bool bAudioOnAllSpeakers)
{
}
#endif

