#pragma once
#include "ICodec.h"
#include "FileReader.h"
#include "aac/aaccodec.h"

class AACCodec : public ICodec
{
  struct AACdll
  {
    AACHandle (__cdecl *AACOpen)(const char *fn, AACIOCallbacks callbacks);
    int (__cdecl *AACRead)(AACHandle handle, BYTE* pBuffer, int iSize);
    int (__cdecl *AACSeek)(AACHandle handle, int iTimeMs);
    void (__cdecl *AACClose)(AACHandle handle);
    const char* (__cdecl *AACGetErrorMessage)();
    int (__cdecl *AACGetInfo)(AACHandle handle, AACInfo* info);
  };

public:
  AACCodec();
  virtual ~AACCodec();

  virtual bool Init(const CStdString &strFile, unsigned int filecache);
  virtual void DeInit();
  virtual __int64 Seek(__int64 iSeekTime);
  virtual int ReadPCM(BYTE *pBuffer, int size, int *actualsize);
  virtual bool CanInit();
private:
  static unsigned __int32 AACOpenCallback(const char *pName, const char *mode, void *userData);
  static void AACCloseCallback(void *userData);
  static unsigned __int32 AACReadCallback(void *userData, void *pBuffer, unsigned long nBytesToRead);
  static __int32 AACSeekCallback(void *userData, unsigned __int64 pos);
  static __int64 AACFilesizeCallback(void *userData);

  AACHandle m_Handle;
  BYTE*     m_Buffer;
  int       m_BufferSize; 
  int       m_BufferPos;

  CFileReader m_file;
  // Our dll
  bool LoadDLL();
  bool m_bDllLoaded;
  AACdll m_dll;
};
