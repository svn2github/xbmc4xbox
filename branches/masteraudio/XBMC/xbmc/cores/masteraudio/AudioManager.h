/*
 *      Copyright (C) 2009 Team XBMC
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

#ifndef __AUDIO_MANAGER_H__
#define __AUDIO_MANAGER_H__

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "MasterAudioCore.h"
#include <vector>


typedef void* MA_STREAM_ID;

#define MA_MIXER_HARDWARE 1
#define MA_MIXER_SOFTWARE 2

struct audio_profile
{
  CStreamDescriptor output_descriptor;
};

class CAudioStream
{
public:
  CAudioStream();
  virtual ~CAudioStream();
  bool Initialize(CStreamInput* pInput, CDSPChain* pDSPChain, int mixerChannel, IAudioSink* pMixerSink);
  CStreamInput* GetInput();
  CDSPChain* GetDSPChain();
  int GetMixerChannel();
  float GetMaxLatency();
  void Close();
  void Flush();
  bool Drain(unsigned int timeout);
  bool ProcessStream();
private:
  CStreamInput* m_pInput;
  CDSPChain* m_pDSPChain;
  int m_MixerChannel;
  IAudioSink* m_pMixerSink;
  CAudioDataInterconnect m_InputConnection;
  CAudioDataInterconnect m_OutputConnection;
  lap_timer m_ProcessTimer;
  lap_timer m_IntervalTimer;
  bool m_Open;
};

typedef std::map<MA_STREAM_ID,CAudioStream*>::iterator StreamIterator;

class CAudioManager
{
public:
  CAudioManager();
  virtual ~CAudioManager();
  MA_STREAM_ID OpenStream(CStreamDescriptor* pDesc);
  void CloseStream(MA_STREAM_ID streamId);
  size_t AddDataToStream(MA_STREAM_ID streamId, void* pData, size_t len);
  bool ControlStream(MA_STREAM_ID streamId, int controlCode);
  bool SetStreamVolume(MA_STREAM_ID streamId, long vol);
  bool SetStreamProp(MA_STREAM_ID streamId, int propId, const void* pVal);
  bool GetStreamProp(MA_STREAM_ID streamId, int propId, void* pVal);
  float GetMaxStreamLatency(MA_STREAM_ID streamId);
  bool DrainStream(MA_STREAM_ID streamId, unsigned int timeout); // Play all slices that have been received but not rendered yet (finish or give-up within timeout ms)
  void FlushStream(MA_STREAM_ID streamId);  // Drop any slices that have been received but not rendered yet
  bool SetMixerType(int mixerType);
  
protected:
  std::map<MA_STREAM_ID,CAudioStream*> m_StreamList;
  IAudioMixer* m_pMixer;
  unsigned int m_MaxStreams;

  CAudioStream* GetInputStream(MA_STREAM_ID streamId);
  MA_STREAM_ID GetStreamId(CAudioStream* pStream);
  int GetOpenStreamCount();
  void CleanupStreamResources(CAudioStream* pStream);
  audio_profile* GetProfile(CStreamDescriptor* pInputDesc);
};

extern CAudioManager g_AudioLibManager;

#endif // __AUDIO_MANAGER_H__