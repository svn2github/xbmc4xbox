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

#include "AudioRendererFactory.h"
#include "NullDirectSound.h"

#ifdef HAS_PULSEAUDIO
#include "PulseAudioDirectSound.h"
#endif

#ifdef _WIN32
#include "../mplayer/Win32DirectSound.h"
#endif
#ifdef __APPLE__
#include "PortaudioDirectSound.h"
#elif defined(_LINUX)
#include "ALSADirectSound.h"
#endif


IDirectSoundRenderer* CAudioRendererFactory::Create(IAudioCallback* pCallback, int iChannels, unsigned int uiSamplesPerSec, unsigned int uiBitsPerSample, bool bResample, const char* strAudioCodec, bool bIsMusic, bool bPassthrough)
{
  IDirectSoundRenderer *audioSink = CreateAudioRenderer(pCallback, iChannels, uiSamplesPerSec, uiBitsPerSample, bResample, strAudioCodec, bIsMusic, bPassthrough);

  return audioSink;
}

IDirectSoundRenderer* CAudioRendererFactory::CreateAudioRenderer(IAudioCallback* pCallback, int iChannels, unsigned int uiSamplesPerSec, unsigned int uiBitsPerSample, bool bResample, const char* strAudioCodec, bool bIsMusic, bool bPassthrough)
{
  IDirectSoundRenderer* audioSink = NULL;

/* First pass creation */
#ifdef HAS_PULSEAUDIO
  audioSink = new CPulseAudioDirectSound();
  ReturnOnValidInitialize();
#endif

/* incase none in the first pass was able to be created, fall back to os specific */

#ifdef WIN32
  audioSink = new CWin32DirectSound();
  ReturnOnValidInitialize();
#endif
#ifdef __APPLE__
  audioSink = new PortAudioDirectSound();
  ReturnOnValidInitialize();
#elif defined(_LINUX)
  audioSink = new CALSADirectSound();
  ReturnOnValidInitialize();
#endif

  audioSink = new CNullDirectSound();
  audioSink->Initialize(pCallback, iChannels, uiSamplesPerSec, uiBitsPerSample, bResample, strAudioCodec, bIsMusic, bPassthrough);
  return audioSink;
}
