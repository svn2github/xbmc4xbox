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

#pragma once
#include "utils/Thread.h"

#include "DVDAudio.h"
#include "DVDClock.h"
#include "DVDMessageQueue.h"
#include "DVDDemuxers/DVDDemuxUtils.h"
#include "DVDStreamInfo.h"
#include "BitstreamStats.h"
#include "DVDPlayerAudioResampler.h"
#include "DVDAudioTypes.h"
#include "utils/PCMSampleConverter.h"

#include <list>
#include <queue>

class CDVDPlayer;
class CDVDAudioCodec;
class IAudioCallback;
class CDVDAudioCodec;

enum CodecID;

#define DECODE_FLAG_DROP    1
#define DECODE_FLAG_RESYNC  2
#define DECODE_FLAG_ERROR   4
#define DECODE_FLAG_ABORT   8
#define DECODE_FLAG_TIMEOUT 16

class CPTSOutputQueue
{
private:
  typedef struct {double pts; double timestamp; double duration;} TPTSItem;
  TPTSItem m_current;
  std::queue<TPTSItem> m_queue;
  CCriticalSection m_sync;

public:
  CPTSOutputQueue();
  void Add(double pts, double delay, double duration);
  void Flush();
  double Current();
};

class CPTSInputQueue
{
private:
  typedef std::list<std::pair<__int64, double> >::iterator IT;
  std::list<std::pair<__int64, double> > m_list;
  CCriticalSection m_sync;
public:
  void   Add(__int64 bytes, double pts);
  double Get(__int64 bytes, bool consume);
  void   Flush();
};

/////////////////////////
// Temprorary wrapper classes to standardize the interface to decoders/parsers
// TODO: Convert decoders
class CDVDAudioPacketHandler
{
public:
  CDVDAudioPacketHandler(CDVDStreamInfo& hints);
  
  virtual bool Init(void* pData, unsigned int size) {return false;}
  virtual unsigned int AddPacket(void* pData, unsigned int size) {return 0;}
  virtual bool GetFrame(DVDAudioFrame& frame) {return false;}
  virtual void Close() {}
  virtual void Reset() {}
  virtual unsigned int GetCacheSize() {return 0;}
  
  CDVDStreamInfo& GetStreamInfo() {return m_StreamInfo;}
  DVDAudioFormat& GetOutputFormat(){return m_OutputFormat;}
protected:
  CDVDStreamInfo m_StreamInfo;
  DVDAudioFormat m_OutputFormat;
};

// Wraps a CDVDAudioCodec with standardized handler interface
class CDVDAudioDecodePacketHandler : public CDVDAudioPacketHandler
{
public:
  CDVDAudioDecodePacketHandler(CDVDStreamInfo& hints, CDVDAudioCodec* pDecoder);
  ~CDVDAudioDecodePacketHandler();
  bool Init(void* pData, unsigned int size);
  unsigned int AddPacket(void* pData, unsigned int size);
  bool GetFrame(DVDAudioFrame& frame);
  void Close();
  void Reset();
  unsigned int GetCacheSize();
protected:
  CDVDAudioCodec* m_pDecoder;
};

// Parses incoming packets, but does not modify them
class CDVDAudioPassthroughParser : public CDVDAudioPacketHandler
{
public:
  CDVDAudioPassthroughParser(CDVDStreamInfo& hints);
  ~CDVDAudioPassthroughParser();
  bool Init(void* pData, unsigned int size);
  unsigned int AddPacket(void* pData, unsigned int size);
  bool GetFrame(DVDAudioFrame& frame);
  void Close();
protected:
  void* m_pCurrentFrame;
  unsigned int m_CurrentFrameSize;
};

/////////////////////////////////

class CDVDAudioPostProc
{
public:
  CDVDAudioPostProc(DVDAudioFormat inFormat, DVDAudioFormat outFormat);
  virtual bool Init() {return false;}
  virtual bool AddFrame(DVDAudioFrame& frame) {return false;}
  virtual bool GetFrame(DVDAudioFrame& frame) {return false;}
  virtual void Close() {}
  virtual void Reset() {}
protected:
  DVDAudioFormat m_InputFormat;
  DVDAudioFormat m_OutputFormat;
};

///////////////////////////////

class CDVDAudioPacketizerSPDIF : public CDVDAudioPostProc
{
public:
  CDVDAudioPacketizerSPDIF(DVDAudioFormat inFormat, DVDAudioFormat outFormat);
  ~CDVDAudioPacketizerSPDIF();
  bool Init();
  bool AddFrame(DVDAudioFrame& frame);
  bool GetFrame(DVDAudioFrame& frame);
  void Close();
protected:
};

/////////////////////////

class CDVDPlayerAudio : public CThread
{
public:
  CDVDPlayerAudio(CDVDClock* pClock, CDVDMessageQueue& parent);
  virtual ~CDVDPlayerAudio();

  void RegisterAudioCallback(IAudioCallback* pCallback) { m_dvdAudio.RegisterAudioCallback(pCallback); }
  void UnRegisterAudioCallback()                        { m_dvdAudio.UnRegisterAudioCallback(); }

  bool OpenStream(CDVDStreamInfo &hints);
  void CloseStream(bool bWaitForBuffers);

  void SetSpeed(int speed);
  void Flush();

  // waits until all available data has been rendered
  void WaitForBuffers();
  bool AcceptsData()                                    { return !m_messageQueue.IsFull(); }
  void SendMessage(CDVDMsg* pMsg)                       { m_messageQueue.Put(pMsg); }

  void SetVolume(long nVolume)                          { m_dvdAudio.SetVolume(nVolume); }
  void SetDynamicRangeCompression(long drc)             { m_dvdAudio.SetDynamicRangeCompression(drc); }

  std::string GetPlayerInfo();
  int GetAudioBitrate();

  // holds stream information for current playing stream
  CDVDStreamInfo m_nextstream;

  CDVDMessageQueue m_messageQueue;
  CDVDMessageQueue& m_messageParent;
  CPTSOutputQueue m_ptsOutput;
  CPTSInputQueue  m_ptsInput;

  double GetCurrentPts()                            { return m_ptsOutput.Current(); }

  bool IsStalled()                                  { return m_stalled;  }
  bool IsPassthrough() const;
protected:

  virtual void OnStartup();
  virtual void OnExit();
  virtual void Process();

  int DecodeFrame(DVDAudioFrame &audioframe, bool bDropPacket);

  bool OpenAudioPath(CDVDStreamInfo &hint, BYTE* pData = NULL, unsigned int size = 0);
  void CloseAudioPath();
  void ResetAudioPath();

  double m_audioClock;

  // data for audio decoding
  struct
  {
    CDVDMsgDemuxerPacket*  msg;
    BYTE*                  data;
    int                    size;
    double                 dts;

    void Attach(CDVDMsgDemuxerPacket* msg2)
    {
      msg = msg2;
      msg->Acquire();
      DemuxPacket* p = msg->GetPacket();
      data = p->pData;
      size = p->iSize;
      dts = p->dts;

    }
    void Release()
    {
      if(msg) msg->Release();
      msg  = NULL;
      data = NULL;
      size = 0;
      dts  = DVD_NOPTS_VALUE;
    }
  } m_decode;

  CDVDAudio m_dvdAudio; // audio output device
  CDVDClock* m_pClock; // dvd master clock
  BitstreamStats m_audioStats;

  // TODO: Use structure for this?
  CDVDAudioPacketHandler* m_pInputHandler;
  CDVDAudioPostProc* m_pPostProc;
  CPCMSampleConverter* m_pConverter;

  DVDAudioFormat m_InputFormat;
  DVDAudioFormat m_OutputFormat;
  
  int     m_speed;
  double  m_droptime;
  bool    m_stalled;
  bool    m_started;
  double  m_duration; // last packets duration

  CDVDPlayerResampler m_resampler;

  // TODO: Find a better way to handle duplicated frames
  bool OutputFrame(DVDAudioFrame &audioframe, int dupCount = 0);

  //SYNC_DISCON, SYNC_SKIPDUP, SYNC_RESAMPLE
  int    m_synctype;
  int    m_setsynctype;
  int    m_prevsynctype; //so we can print to the log

  double m_error;    //last average error

  int64_t m_errortime; //timestamp of last time we measured
  int64_t m_freq;

  void   SetSyncType(bool passthrough);
  void   HandleSyncError(double duration);
  double m_errorbuff; //place to store average errors
  int    m_errorcount;//number of errors stored
  bool   m_syncclock;

  double m_integral; //integral correction for resampler
  int    m_skipdupcount; //counter for skip/duplicate synctype
  bool   m_prevskipped;
  double m_maxspeedadjust;
};

