#ifndef _ENCODERVORBIS_H
#define _ENCODERVORBIS_H

#include "Encoder.h"
#include "oggvorbis\vorbisenc.h"

class DllLoader;

class CEncoderVorbis : public CEncoder
{
public:
  CEncoderVorbis();
  ~CEncoderVorbis();
  bool Init(const char* strFile, int iInChannels, int iInRate, int iInBits);
  int Encode(int nNumBytesRead, BYTE* pbtStream);
  bool Close();
  void AddTag(int key, const char* value);

protected:
  DllLoader* LoadDLL(const char* strFile);

  vorbis_info m_sVorbisInfo; /* struct that stores all the static vorbis bitstream settings */
  vorbis_dsp_state m_sVorbisDspState; /* central working state for the packet->PCM decoder */
  vorbis_block m_sVorbisBlock; /* local working space for packet->PCM decode */
  vorbis_comment m_sVorbisComment;

  ogg_stream_state m_sOggStreamState; /* take physical pages, weld into a logical stream of packets */
  ogg_page m_sOggPage; /* one Ogg bitstream page.  Vorbis packets are inside */
  ogg_packet m_sOggPacket; /* one raw packet of data for decode */

  BYTE* m_pBuffer;

  DllLoader* m_pDLLOgg;
  DllLoader* m_pDLLVorbis;
};

#endif // _ENCODERVORBIS_H
