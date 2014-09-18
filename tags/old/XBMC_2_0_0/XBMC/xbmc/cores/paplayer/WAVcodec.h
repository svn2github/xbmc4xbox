#pragma once
#include "ICodec.h"
#include "FileReader.h"

class WAVCodec : public ICodec
{
public:
  WAVCodec();
  virtual ~WAVCodec();

  virtual bool Init(const CStdString &strFile, unsigned int filecache);
  virtual void DeInit();
  virtual __int64 Seek(__int64 iSeekTime);
  virtual int ReadPCM(BYTE *pBuffer, int size, int *actualsize);
  virtual bool CanInit();

private:
  CFileReader m_file;
  long m_iDataStart;
  long m_iDataLen;
};
