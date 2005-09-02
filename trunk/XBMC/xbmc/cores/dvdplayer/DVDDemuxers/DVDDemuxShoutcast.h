
#pragma once

#include "DVDDemux.h"

class CDemuxStreamAudioShoutcast : public CDemuxStreamAudio
{
public:
  virtual void GetStreamInfo(std::string& strInfo);
};

#define SHOUTCAST_BUFFER_SIZE 1024 * 32

class CDVDDemuxShoutcast : public CDVDDemux
{
public:
  CDVDDemuxShoutcast();
  ~CDVDDemuxShoutcast();

  bool Open(CDVDInputStream* pInput);
  void Dispose();
  void Reset();
  void Flush();

  CDVDDemux::DemuxPacket* Read();

  bool Seek(int iTime);
  int GetStreamLenght();
  CDemuxStream* GetStream(int iStreamId);
  int GetNrOfStreams();

protected:

  CDemuxStreamAudioShoutcast* m_pDemuxStream;

  int m_iMetaStreamInterval;
};
