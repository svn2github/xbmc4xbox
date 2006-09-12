
#include "../../../stdafx.h"
#include "DVDDemuxUtils.h"

#define INPUT_BUFFER_PADDING_SIZE 8

void CDVDDemuxUtils::FreeDemuxPacket(CDVDDemux::DemuxPacket* pPacket)
{
  if (pPacket)
  {
    try {
      if (pPacket->pData) _aligned_free(pPacket->pData);    
      delete pPacket;
    }
    catch(...) {
      CLog::Log(LOGERROR, __FUNCTION__" - Exception thrown while freeing packet");
    }
  }
}

CDVDDemux::DemuxPacket* CDVDDemuxUtils::AllocateDemuxPacket(int iDataSize)
{
  CDVDDemux::DemuxPacket* pPacket = new CDVDDemux::DemuxPacket;
  if (!pPacket) return NULL;

  try 
  {    
    memset(pPacket, 0, sizeof(CDVDDemux::DemuxPacket));
    
    if (iDataSize > 0)
    {
      // need to allocate a few bytes more.
      // From avcodec.h (ffmpeg)
      /**
        * Required number of additionally allocated bytes at the end of the input bitstream for decoding.
        * this is mainly needed because some optimized bitstream readers read 
        * 32 or 64 bit at once and could read over the end<br>
        * Note, if the first 23 bits of the additional bytes are not 0 then damaged
        * MPEG bitstreams could cause overread and segfault
        */ 
      pPacket->pData =(BYTE*)_aligned_malloc(iDataSize + INPUT_BUFFER_PADDING_SIZE, 16);    
      if (!pPacket->pData)
      {
        FreeDemuxPacket(pPacket);
        return NULL;
      }
      
      // reset the last 8 bytes to 0;
      memset(pPacket->pData + iDataSize, 0, INPUT_BUFFER_PADDING_SIZE);
    }        
  }
  catch(...)
  {
    CLog::Log(LOGERROR, __FUNCTION__" - Exception thrown");
    FreeDemuxPacket(pPacket);
    pPacket = NULL;
  }  
  return pPacket;
}
