
#include "../../../../stdafx.h"
#include "DVDAudioCodecLibFaad.h"

CDVDAudioCodecLibFaad::CDVDAudioCodecLibFaad() : CDVDAudioCodec()
{
  m_bInitializedDecoder = false;
  
  m_pHandle = NULL;
}

CDVDAudioCodecLibFaad::~CDVDAudioCodecLibFaad()
{
  Dispose();
}

bool CDVDAudioCodecLibFaad::Open(CodecID codecID, int iChannels, int iSampleRate, int iBits, void* ExtraData, unsigned int ExtraSize)
{
  // for safety
  if (m_pHandle) Dispose();
  
  if (!m_dll.Load())
    return false;
  
  memset(&m_frameInfo, 0, sizeof(m_frameInfo));
  
  if (!OpenDecoder() ) 
    return false;

  if( ExtraSize )
  {
    unsigned long samplerate;
    unsigned char channels;

    int res = m_dll.faacDecInit2(m_pHandle, (unsigned char*)ExtraData, ExtraSize, &samplerate, &channels);
    if (res >= 0)
    {
      m_iSourceSampleRate = samplerate;
      m_iSourceChannels = channels;
      m_iSourceBitrate = 0;
  
      m_bInitializedDecoder = true;
    }
  }

  return true;
}

void CDVDAudioCodecLibFaad::Dispose()
{
  CloseDecoder();
}

bool CDVDAudioCodecLibFaad::SyncStream()
{
  BYTE* p = m_inputBuffer;
  
  while (m_iInputBufferSize > 4)
  {
    // Check if an ADIF or ADTS header is present
    if (((p[0] == 'A') && (p[1] == 'D') && (p[2] == 'I') && (p[3] == 'F')) ||
        ((p[1] | p[0] << 8) & 0xfff0) == 0xfff0)
    {
      // sync found, update our buffer if needed
      if (p != m_inputBuffer)
      {
        CLog::Log(LOGINFO, "CDVDAudioCodecLibFaad::SyncStream(), stream synced");
        memmove(m_inputBuffer, p, m_iInputBufferSize);
      }
      return true;
    }
    p++;
    m_iInputBufferSize--;
  }
  
  // no sync found
  CLog::Log(LOGWARNING, "CDVDAudioCodecLibFaad::SyncStream(), no sync found (ADIF or ADTS header) in stream");
  return false;
}

int CDVDAudioCodecLibFaad::Decode(BYTE* pData, int iSize)
{
  m_iDecodedDataSize = 0;
  
  if (m_pHandle)
  {
    int iBytesFree = LIBFAAD_INPUT_SIZE - m_iInputBufferSize;
    int iBytesToCopy = iSize;
    
    if (iBytesToCopy > iBytesFree) iBytesToCopy = iBytesFree;
    
    fast_memcpy(m_inputBuffer + m_iInputBufferSize, pData, iBytesToCopy);
    m_iInputBufferSize += iBytesToCopy;
    
    // if the caller does not supply enough data, return
    if (m_iInputBufferSize < FAAD_MIN_STREAMSIZE)
    {
      return iBytesToCopy;
    }
    
    if (SyncStream())
    {
      // initialize decoder if needed
      if (!m_bInitializedDecoder)
      {
        unsigned long samplerate;
        unsigned char channels;
        
        int res = m_dll.faacDecInit(m_pHandle, m_inputBuffer, m_iInputBufferSize, &samplerate, &channels);
        if (res >= 0)
        {
          m_iSourceSampleRate = samplerate;
          m_iSourceChannels = channels;
          m_iSourceBitrate = 0;
      
          m_bInitializedDecoder = true;
        }
      }
      
      if (m_bInitializedDecoder)
      {
        BYTE* pInputData = m_inputBuffer;
        bool bFullDecodedBuffer = false;
        
        // decode as long there is enough source data available, and
        // the output buffer has enough free bytes to save the decoded data
        while (m_iInputBufferSize >= FAAD_MIN_STREAMSIZE && !bFullDecodedBuffer)
        {
          void* pSamples = m_dll.faacDecDecode(m_pHandle, &m_frameInfo, pInputData, m_iInputBufferSize);
          if (!m_frameInfo.error && pSamples)
          {
            // we set this info again, it could be this info changed 
            m_iSourceSampleRate = m_frameInfo.samplerate;
            m_iSourceChannels = m_frameInfo.channels;
            m_iSourceBitrate = 0;
            
            // copy the data
            if (m_frameInfo.samples > 0)
            {
              int iSize = m_frameInfo.samples * sizeof(short);
              fast_memcpy(m_decodedData + m_iDecodedDataSize, pSamples, iSize);
              m_iDecodedDataSize += iSize;
              
              // check for the next run if can can save all decoded data
              if ((m_iDecodedDataSize + iSize) > LIBFAAD_DECODED_SIZE)
              {
                bFullDecodedBuffer = true;
              }
            }
            
            m_iInputBufferSize -= m_frameInfo.bytesconsumed;
            pInputData += m_frameInfo.bytesconsumed;
          }
          else
          {
            m_iInputBufferSize = 0;
            pInputData = NULL; // this pointer should not be used anymore
            
            if (m_frameInfo.error)
            {
              char* strError = m_dll.faacDecGetErrorMessage(m_frameInfo.error);
              CLog::Log(LOGERROR, "CDVDAudioCodecLibFaad() : %s", strError);
            }
          }
        }
        
        // move remaining data to start of buffer
        if (m_iInputBufferSize > 0)
        {
          memmove(m_inputBuffer, pInputData, m_iInputBufferSize);
        }
            
        return iBytesToCopy;
      }
    }
  }
  
  // undefined, return -1
  return -1;
}

int CDVDAudioCodecLibFaad::GetData(BYTE** dst)
{
  *dst = m_decodedData;
  return m_iDecodedDataSize;
}

void CDVDAudioCodecLibFaad::Reset()
{
  CloseDecoder();
  OpenDecoder();
}

void CDVDAudioCodecLibFaad::CloseDecoder()
{
  if (m_pHandle)
  {
    m_dll.faacDecClose(m_pHandle);
    m_pHandle = NULL;
  }
}

bool CDVDAudioCodecLibFaad::OpenDecoder()
{
  if (m_pHandle)
  {
    CLog::Log(LOGWARNING, "CDVDAudioCodecLibFaad : Decoder already opened");
    return false;
  }
  
  m_bInitializedDecoder = false;
  
  m_iInputBufferSize = 0;
  m_iDecodedDataSize = 0;
  
  m_iSourceSampleRate = 0;
  m_iSourceChannels = 0;
  m_iSourceBitrate = 0;
  
  m_pHandle = m_dll.faacDecOpen();
  
  if (m_pHandle)
  {
    faacDecConfigurationPtr pConfiguration;
    pConfiguration = m_dll.faacDecGetCurrentConfiguration(m_pHandle);
    
    // modify some stuff here
    pConfiguration->outputFormat = FAAD_FMT_16BIT; // already default

    m_dll.faacDecSetConfiguration(m_pHandle, pConfiguration);
    
    return true;
  }
  
  return false;
}
