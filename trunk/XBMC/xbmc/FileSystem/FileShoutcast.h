// FileShoutcast.h: interface for the CFileShoutcast class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_FILESHOUTCAST_H__6B6082E6_547E_44C4_8801_9890781659C0__INCLUDED_)
#define AFX_FILESHOUTCAST_H__6B6082E6_547E_44C4_8801_9890781659C0__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "IFile.h"
#include "../lib/libid3/id3.h"
#include "../lib/libid3/misc_support.h"
using namespace XFILE;
namespace XFILE
{


typedef struct FileStateSt
{
  bool bBuffering;
  bool bRipDone;
  bool bRipStarted;
  bool bRipError;
}
FileState;
class CFileShoutcast : public IFile
{
public:
  CFileShoutcast();
  virtual ~CFileShoutcast();
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
  virtual bool CanRecord();
  virtual bool Record();
  virtual void StopRecording();
  virtual bool IsRecording();
  virtual bool GetID3TagInfo(ID3_Tag& tag);
protected:
  void outputTimeoutMessage(const char* message);
  DWORD m_dwLastTime;
};
};
#endif // !defined(AFX_FILESHOUTCAST_H__6B6082E6_547E_44C4_8801_9890781659C0__INCLUDED_)
