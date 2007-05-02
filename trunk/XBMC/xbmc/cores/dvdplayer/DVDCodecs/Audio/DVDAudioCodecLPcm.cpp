
#include "stdafx.h"
#include "DVDAudioCodecLPcm.h"

CDVDAudioCodecLPcm::CDVDAudioCodecLPcm() : CDVDAudioCodecPcm()
{
  m_codecID = CODEC_ID_NONE;
  m_bufferSize = LPCM_BUFFER_SIZE;
}

bool CDVDAudioCodecLPcm::Open(CodecID codecID, int iChannels, int iSampleRate, int iBits, void* ExtraData, unsigned int ExtraSize)
{
  m_codecID = codecID;

  CodecID pcm_id = CODEC_ID_NONE;
#if 0
  if (codecID = CODEC_ID_LPCM_S24BE) pcm_id = CODEC_ID_PCM_S24BE;
#endif
  if (pcm_id != CODEC_ID_NONE)
  {
    return CDVDAudioCodecPcm::Open(pcm_id, iChannels, iSampleRate, iBits, ExtraData, ExtraSize);
  }
  
  return false;
}

int CDVDAudioCodecLPcm::Decode(BYTE* pData, int iSize)
{
  BYTE* d = m_buffer;
  BYTE* s = pData;
  int iDecoded = 0;

  if (iSize > m_bufferSize) iSize = m_bufferSize;
  
  if (iSize >= 12)
  {
#if 0
    if (m_codecID == CODEC_ID_LPCM_S24BE)
#endif
    {
      for (iDecoded = 0; iDecoded <= (iSize - 12); iDecoded += 12)
      {
        // first sample
        d[0] = s[0];
        d[1] = s[1];
        d[2] = s[8];
        // second sample
        d[3] = s[2];
        d[4] = s[3];
        d[5] = s[9];
        // third sample
        d[6] = s[4];
        d[7] = s[5];
        d[8] = s[10];
        // fourth sample
        d[9] = s[6];
        d[10] = s[7];
        d[11] = s[11];
        
        s += 12;
        d += 12;
      }
    }
    
    return CDVDAudioCodecPcm::Decode(m_buffer, iDecoded);
  }
  
  return iSize;
}
