#pragma once
#include "../../utils/thread.h"

#include "DVDAudio.h"
#include "DVDClock.h"
#include "DVDMessageQueue.h"
#include "DVDDemuxers\DVDDemuxUtils.h"

class CDVDPlayer;
class CDVDAudioCodec;
class IAudioCallback;
enum CodecID;

#define DECODE_FLAG_DROP    1
#define DECODE_FLAG_RESYNC  2
#define DECODE_FLAG_ERROR   4
#define DECODE_FLAG_ABORT   8

typedef struct stDVDAudioFrame
{
  BYTE* data;
  __int64 pts;
  unsigned int duration;  
  unsigned int size;
} DVDAudioFrame;

class CDVDPlayerAudio : public CThread
{
public:
  CDVDPlayerAudio(CDVDClock* pClock);
  virtual ~CDVDPlayerAudio();

  void RegisterAudioCallback(IAudioCallback* pCallback) { m_dvdAudio.RegisterAudioCallback(pCallback); }
  void UnRegisterAudioCallback()                        { m_dvdAudio.UnRegisterAudioCallback(); }

  bool OpenStream(CDemuxStreamAudio *pDemuxStream);
  void CloseStream(bool bWaitForBuffers);
  
  void SetSpeed(int speed);
  void Flush();

  // waits untill all available data has been rendered  
  void WaitForBuffers();
  bool AcceptsData()                                    { return !m_packetQueue.IsFull(); }

  void DoWork()                                         { m_dvdAudio.DoWork(); }

  void SetVolume(long nVolume)                          { m_dvdAudio.SetVolume(nVolume); }
  int GetVolume()                                       { return m_dvdAudio.GetVolume(); }

  CodecID m_codec;    // codec id of the current active stream
  int m_iSourceChannels; // number of audio channels for the current active stream
  
  CDVDMessageQueue m_packetQueue;
  
  __int64 GetCurrentPts();

protected:

  virtual void OnStartup();
  virtual void OnExit();
  virtual void Process();

  bool InitializeOutputDevice();
  int DecodeFrame(DVDAudioFrame &audioframe, bool bDropPacket);

  bool m_bInitializedOutputDevice;
  __int64 m_audioClock;
  
  // for audio decoding
  CDVDDemux::DemuxPacket* pAudioPacket;
  BYTE* audio_pkt_data; // current audio packet
  int audio_pkt_size; // and current audio packet size
  
  CDVDAudio m_dvdAudio; // audio output device
  CDVDClock* m_pClock; // dvd master clock
  CDVDAudioCodec* m_pAudioCodec; // audio codec

  int m_speed;  // wanted playback speed. if playback speed!=DVD_PLAYSPEED_NORMAL, don't sync clock as it will loose track of position after seek

  typedef struct {__int64 pts; __int64 timestamp;} TPTSItem;
  TPTSItem m_currentPTSItem;
  std::queue<TPTSItem> m_quePTSQueue;

  void AddPTSQueue(__int64 pts, __int64 delay);
  void FlushPTSQueue();


  CRITICAL_SECTION m_critCodecSection;
};
