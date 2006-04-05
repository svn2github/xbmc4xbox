#pragma once

#include "IFile.h"
#include "FileCurl.h"
#include "ringbuffer.h"
#include "..\cores\paplayer\RingHoldBuffer.h"


class CFileLastFM : public IFile, public CThread
{
public:
  CFileLastFM();
  virtual ~CFileLastFM();
  virtual __int64 GetPosition();
  virtual __int64 GetLength();
  virtual bool Open(const CURL& url, bool bBinary = true);
  virtual bool Exists(const CURL& url) { return true;};
  virtual int Stat(const CURL& url, struct __stat64* buffer) { errno = ENOENT; return -1; };
  virtual unsigned int Read(void* lpBuf, __int64 uiBufSize);
  virtual bool ReadString(char *szLine, int iLineLength);
  virtual __int64 Seek(__int64 iFilePosition, int iWhence = SEEK_SET);
  virtual void Close();
  virtual bool CanSeek();
  virtual int  GetChunkSize() { return 1024; }
  virtual bool SkipNext();
protected:
  bool HandShake();
  bool ChangeStation(const CURL& url);
  bool RetreiveMetaData();
  bool DoSkipNext();
  bool OpenStream();
  void Parameter(const CStdString& key, const CStdString& data, CStdString& value);
  bool RecordToProfile(bool enabled);
  int  SyncReceived(const char* data, unsigned int size);
  
  virtual void Process();

  CFileCurl* m_pFile;
  char* m_pSyncBuffer;
  HANDLE m_hWorkerEvent;

  void DownloadThread();

  bool m_bOpened;
  bool m_bDirectSkip;

  CStdString m_Url;
  CStdString m_Session;
  CStdString m_StreamUrl;
  CStdString m_BaseUrl;
  CStdString m_BasePath;
  CStdString m_Subscriber;
  CStdString m_Banned;

};
