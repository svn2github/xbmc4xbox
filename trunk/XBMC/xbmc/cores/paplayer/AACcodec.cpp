#include "../../stdafx.h"
#include "AACCodec.h"

AACCodec::AACCodec()
{
  m_SampleRate = 0;
  m_Channels = 0;
  m_BitsPerSample = 0;
  m_TotalTime = 0;
  m_Bitrate = 0;
  m_CodecName = L"AAC";

  m_Handle=AAC_INVALID_HANDLE;

  m_BufferSize=0;
  m_BufferPos = 0;
  m_Buffer=NULL;

  // dll stuff
  m_bDllLoaded = false;
  ZeroMemory(&m_dll, sizeof(AACdll));
}

AACCodec::~AACCodec()
{
  DeInit();
  if (m_bDllLoaded)
    CSectionLoader::UnloadDLL(AAC_DLL);
}

bool AACCodec::Init(const CStdString &strFile, unsigned int filecache)
{
  m_file.Initialize(filecache);

  if (!LoadDLL())
    return false;

  // setup our callbacks
  AACIOCallbacks callbacks;
  callbacks.userData=this;
  callbacks.Open=OpenCallback;
  callbacks.Read=ReadCallback;
  callbacks.Write=WriteCallback;
  callbacks.Close=CloseCallback;
  callbacks.Filesize=FilesizeCallback;
  callbacks.Getpos=GetposCallback;
  callbacks.Setpos=SetposCallback;

  m_Handle=m_dll.AACOpen(strFile.c_str(), callbacks);
  if (m_Handle==AAC_INVALID_HANDLE)
  {
    CLog::Log(LOGERROR,"AACCodec: Unable to open file %s (%s)", strFile.c_str(), m_dll.AACGetErrorMessage());
    return false;
  }

	AACInfo info;
	if (m_dll.AACGetInfo(m_Handle, &info))
	{
		m_Channels = info.channels;
		m_SampleRate = info.samplerate;
		m_BitsPerSample = info.bitspersample;
    m_TotalTime = info.totaltime;
	  m_Bitrate = info.bitrate;

    m_Buffer = new BYTE[2048*m_Channels*sizeof(short)*2];

	}
	else
	{
    CLog::Log(LOGERROR,"AACCodec: No stream info found in file %s (%s)", strFile.c_str(), m_dll.AACGetErrorMessage());
    return false;
	}
  return true;
}

void AACCodec::DeInit()
{
  if (m_Handle!=AAC_INVALID_HANDLE)
    m_dll.AACClose(m_Handle);
  m_file.Close();

  if (m_Buffer)
    delete[] m_Buffer;
  m_Buffer=NULL;
}

__int64 AACCodec::Seek(__int64 iSeekTime)
{
  if (m_Handle==AAC_INVALID_HANDLE)
    return -1;

  if (!m_dll.AACSeek(m_Handle, (int)iSeekTime))
    return -1;

  m_BufferSize=m_BufferPos=0;

  return iSeekTime;
}

int AACCodec::ReadPCM(BYTE *pBuffer, int size, int *actualsize)
{
  *actualsize=0;

  bool bEof=false;

  //  Do we have to refill our audio buffer?
  if (m_BufferSize-m_BufferPos<size)
  {
    //  Move the remaining audio data to the beginning of the buffer
    memmove(m_Buffer, m_Buffer + m_BufferPos, m_BufferSize-m_BufferPos);
    m_BufferSize=m_BufferSize-m_BufferPos;
    m_BufferPos = 0;

    //  Fill our buffer with a chunk of audio data
    int iAmountRead=m_dll.AACRead(m_Handle, m_Buffer+m_BufferSize, 2048*m_Channels*sizeof(short));
    if (iAmountRead==AAC_READ_EOF)
      bEof=true;
    else if (iAmountRead<=AAC_READ_ERROR)
    {
      CLog::Log(LOGERROR, "AACCodec: Error, %s", m_dll.AACGetErrorMessage());
      return READ_ERROR;
    } 
    else
      m_BufferSize+=iAmountRead;
  }

  //  Our buffer is empty and no data left to read from the cd
  if (m_BufferSize-m_BufferPos==0 && bEof)
    return READ_EOF;

  //  Try to give the player the amount of audio data he wants
  if (m_BufferSize-m_BufferPos>=size)
  { //  we have enough data in our buffer
    memcpy(pBuffer, m_Buffer + m_BufferPos, size);
    m_BufferPos+=size;
    *actualsize=size;
  }
  else
  { //  Only a smaller amount of data left as the player wants
    memcpy(pBuffer, m_Buffer + m_BufferPos, m_BufferSize-m_BufferPos);
    *actualsize=m_BufferSize-m_BufferPos;
    m_BufferPos+=m_BufferSize-m_BufferPos;
  }

  return READ_SUCCESS;
}

bool AACCodec::HandlesType(const char *type)
{
  return ( strcmp(type, "m4a") == 0 || strcmp(type, "aac") == 0);
}

bool AACCodec::LoadDLL()
{
  if (m_bDllLoaded)
    return true;
  DllLoader* pDll = CSectionLoader::LoadDLL(AAC_DLL);
  if (!pDll)
  {
    CLog::Log(LOGERROR, "AACCodec: Unable to load dll %s", AAC_DLL);
    return false;
  }

  // get handle to the functions in the dll
  pDll->ResolveExport("AACOpen", (void **)&m_dll.AACOpen);
  pDll->ResolveExport("AACClose", (void **)&m_dll.AACClose);
  pDll->ResolveExport("AACGetInfo", (void **)&m_dll.AACGetInfo);
  pDll->ResolveExport("AACGetErrorMessage", (void **)&m_dll.AACGetErrorMessage);
  pDll->ResolveExport("AACRead", (void **)&m_dll.AACRead);
  pDll->ResolveExport("AACSeek", (void **)&m_dll.AACSeek);

  // Check resolves + version number
  if (!m_dll.AACOpen || !m_dll.AACClose || !m_dll.AACGetInfo ||
      !m_dll.AACGetErrorMessage || !m_dll.AACRead || !m_dll.AACSeek)
  {
    CLog::Log(LOGERROR, "AACCodec: Unable to resolve exports from %s", AAC_DLL);
    CSectionLoader::UnloadDLL(AAC_DLL);
    return false;
  }

  m_bDllLoaded = true;
  return true;
}

unsigned __int32 AACCodec::OpenCallback(const char *pName, const char *mode, void *userData)
{
  AACCodec* codec=(AACCodec*) userData;

  if (!codec)
    return 0;

  return codec->m_file.Open(pName);
}

void AACCodec::CloseCallback(void *userData)
{
  AACCodec* codec=(AACCodec*) userData;

  if (!codec)
    return;

  codec->m_file.Close();
}

unsigned __int32 AACCodec::ReadCallback(void *pBuffer, unsigned int nBytesToRead, void *userData)
{
  AACCodec* codec=(AACCodec*) userData;

  if (!codec)
    return 0;

  return codec->m_file.Read(pBuffer, nBytesToRead);
}

unsigned __int32 AACCodec::WriteCallback(void *pBuffer, unsigned int nBytesToWrite, void *userData)
{
  return 0;
}

__int32 AACCodec::SetposCallback(unsigned __int32 pos, void *userData)
{
  AACCodec* codec=(AACCodec*) userData;

  if (!codec)
    return -1;

  codec->m_file.Seek(pos);

  return 0;
}

__int64 AACCodec::GetposCallback(void *userData)
{
  AACCodec* codec=(AACCodec*) userData;

  if (!codec)
    return -1;

  return codec->m_file.GetPosition();
}

__int64 AACCodec::FilesizeCallback(void *userData)
{
  AACCodec* codec=(AACCodec*) userData;

  if (!codec)
    return 0;

  return codec->m_file.GetLength();
}

