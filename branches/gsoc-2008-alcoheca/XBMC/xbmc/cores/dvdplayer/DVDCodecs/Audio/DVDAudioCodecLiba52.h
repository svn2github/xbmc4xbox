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

#include "DVDAudioCodec.h"
#include "DllLiba52.h"

class CDVDAudioCodecLiba52 : public CDVDAudioCodec
{
public:
  CDVDAudioCodecLiba52();
  virtual ~CDVDAudioCodecLiba52();
  virtual bool Open(CDVDStreamInfo &hints, CDVDCodecOptions &options);
  virtual void Dispose();
  virtual int Decode(BYTE* pData, int iSize);
  virtual int GetData(BYTE** dst);
  virtual void Reset();
  virtual int GetChannels();
  virtual int GetSampleRate();
  virtual int GetBitsPerSample();
  virtual const char* GetName() { return "liba52"; }

protected:
  virtual void SetDefault();

  void SetupChannels();

  a52_state_t* m_pState;

  BYTE m_inputBuffer[4096];
  BYTE* m_pInputBuffer;

  BYTE m_decodedData[131072]; // could be a bit to big
  int m_decodedDataSize;

  int m_iFrameSize;
  float* m_fSamples;

  int m_iSourceSampleRate;
  int m_iSourceFlags;
  int m_iSourceBitrate;

  int m_iOutputChannels;
  unsigned int m_iOutputMapping;
  DllLiba52 m_dll;
};
