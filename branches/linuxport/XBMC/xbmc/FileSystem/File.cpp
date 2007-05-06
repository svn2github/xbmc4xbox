/*
* XBoxMediaPlayer
* Copyright (c) 2002 Frodo
* Portions Copyright (c) by the authors of ffmpeg and xvid
*
* This program is free software; you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation; either version 2 of the License, or
* (at your option) any later version.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with this program; if not, write to the Free Software
* Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
*/

#include "stdafx.h"
#include "File.h"
#include "FileFactory.h"
#include "../Application.h"
#include "../Util.h"
#include "DirectoryCache.h"
#ifndef _LINUX
#include "../utils/Win32Exception.h"
#endif

using namespace XFILE;
using namespace DIRECTORY;

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////
#pragma warning (disable:4244)

class CAsyncFileCallback
  : public CThread
{
public:
  ~CAsyncFileCallback()
  {
    StopThread();
  }

  CAsyncFileCallback(IFileCallback* callback, void* context)
  {
    m_callback = callback;
    m_context = context;
    m_percent = 0;
    m_speed = 0.0f;
    m_cancel = false;
    Create();
  }

  virtual void Process()
  {
    while(!m_bStop)
    {
      m_event.WaitMSec(1000/30);
      if (m_callback)
        if(!m_callback->OnFileCallback(m_context, m_percent, m_speed))
          if(!m_cancel)
            m_cancel = true;
    }
  }

  void SetStatus(int percent, float speed)
  {
    m_percent = percent;
    m_speed = speed;
    m_event.Set();
  }

  bool IsCanceled()
  {
    return m_cancel;
  }

private:
  IFileCallback* m_callback;
  void* m_context;
  int   m_percent;
  float m_speed;
  CEvent m_event;
  bool m_cancel;
};


//*********************************************************************************************
CFile::CFile()
{
  m_pFile = NULL;
}

//*********************************************************************************************
CFile::~CFile()
{
  if (m_pFile) delete m_pFile;
  m_pFile = NULL;
}

//*********************************************************************************************

class CAutoBuffer
{
  char* p;
public:
  explicit CAutoBuffer(size_t s) { p = (char*)malloc(s); }
  ~CAutoBuffer() { if (p) free(p); }
char* get() { return p; }
};

bool CFile::Cache(const CStdString& strFileName, const CStdString& strDest, XFILE::IFileCallback* pCallback, void* pContext)
{
  CFile file;
  CAsyncFileCallback* helper = NULL;

  if (file.Open(strFileName, true))
  {
    if (file.GetLength() <= 0)
    {
      CLog::Log(LOGWARNING, "FILE::cache: the file %s has a length of 0 bytes", strFileName.c_str());
      file.Close();
      // no need to return false here.  Technically, we should create the new file and leave it at that
//      return false;
    }

    CFile newFile;
    if (CUtil::IsHD(strDest)) // create possible missing dirs
    {
      vector<CStdString> tokens;
      CStdString strDirectory;
      CUtil::GetDirectory(strDest,strDirectory);
      CUtil::RemoveSlashAtEnd(strDirectory);  // for the test below
      if (!(strDirectory.size() == 2 && strDirectory[1] == ':'))
      {
        CUtil::Tokenize(strDirectory,tokens,"\\");
        CStdString strCurrPath = tokens[0]+"\\";
        for (vector<CStdString>::iterator iter=tokens.begin()+1;iter!=tokens.end();++iter)
        {
          strCurrPath += *iter+"\\";
          CDirectory::Create(strCurrPath);
        }
      }
    }
    CFile::Delete(strDest);
    if (!newFile.OpenForWrite(strDest, true, true))  // overwrite always
    {
      file.Close();
      return false;
    }

    /* larger then 1 meg, let's do rendering async */
    if( file.GetLength() > 1024*1024 )
      helper = new CAsyncFileCallback(pCallback, pContext);

    // 128k is optimal for xbox
    int iBufferSize = 128 * 1024;

    CAutoBuffer buffer(iBufferSize);
    int iRead, iWrite;

    UINT64 llFileSize = file.GetLength();
    UINT64 llFileSizeOrg = llFileSize;
    UINT64 llPos = 0;
    int ipercent = 0;

    CStopWatch timer;
    timer.StartZero();
    float start = 0.0f;
    float callback = 0.0f;
    while (llFileSize > 0)
    {
      g_application.ResetScreenSaver();
      int iBytesToRead = iBufferSize;
      
      /* make sure we don't try to read more than filesize*/
      if (iBytesToRead > llFileSize) iBytesToRead = llFileSize;

      iRead = file.Read(buffer.get(), iBytesToRead, READ_TRUNCATED);
      if (iRead == 0) break;
      else if (iRead < 0) 
      {
        CLog::Log(LOGERROR, "%s - Failed read from file %s", __FUNCTION__, strFileName.c_str());
        break;
      }

      /* write data and make sure we managed to write it all */
      iWrite = 0;
      while(iWrite < iRead)
      {
        int iWrite2 = newFile.Write(buffer.get()+iWrite, iRead-iWrite);
        if(iWrite2 <=0)          
          break;
        iWrite+=iWrite2;
      }

      if (iWrite != iRead)
      {
        CLog::Log(LOGERROR, "%s - Failed write to file %s", __FUNCTION__, strDest.c_str());
        break;
      }
      
      llFileSize -= iRead;
      llPos += iRead;

      // calculate the current and average speeds
      float end = timer.GetElapsedSeconds();
      float averageSpeed = llPos / end;
      start = end;

      float fPercent = 100.0f * (float)llPos / (float)llFileSizeOrg;

      if ((int)fPercent != ipercent)
      {
        if( helper )
        {
          helper->SetStatus((int)fPercent, averageSpeed);
          if(helper->IsCanceled())
            break;
        }
        else if( pCallback )
        {
          if (!pCallback->OnFileCallback(pContext, ipercent, averageSpeed)) 
            break;
        }

        ipercent = (int)fPercent;
      }
    }

    /* close both files */
    newFile.Close();
    file.Close();
    
    if(helper)
      delete helper;

    /* verify that we managed to completed the file */
    if (llPos != llFileSizeOrg)
    {      
      CFile::Delete(strDest);
      return false;
    }
    return true;
  }
  return false;
}

//*********************************************************************************************
bool CFile::Open(const CStdString& strFileName, bool bBinary)
{
  try 
  {
    bool bPathInCache;
    if (!g_directoryCache.FileExists(strFileName, bPathInCache) )
    {
      if (bPathInCache)
        return false;
    }
    CURL url(strFileName);

    m_pFile = CFileFactory::CreateLoader(url);
    if (m_pFile)
      return m_pFile->Open(url, bBinary);
  }
#ifndef _LINUX
  catch (const win32_exception &e) 
  {
    e.writelog(__FUNCTION__);
  }
#endif
  catch(...)
  {
    CLog::Log(LOGERROR, "%s - Unhandled exception", __FUNCTION__);
  }
  CLog::Log(LOGERROR, "%s - Error opening %s", __FUNCTION__, strFileName.c_str());    
  return false;
}

bool CFile::OpenForWrite(const CStdString& strFileName, bool bBinary, bool bOverWrite)
{
  try 
  {
    CURL url(strFileName);

    m_pFile = CFileFactory::CreateLoader(url);
    if (m_pFile)
      return m_pFile->OpenForWrite(url, bBinary, bOverWrite);
  }
#ifndef _LINUX
  catch (const win32_exception &e) 
  {
    e.writelog(__FUNCTION__);
  }
#endif
  catch(...)
  {
    CLog::Log(LOGERROR, "%s - Unhandled exception opening %s", __FUNCTION__, strFileName.c_str());    
  }
  CLog::Log(LOGERROR, "%s - Error opening %s", __FUNCTION__, strFileName.c_str());
  return false;
}

bool CFile::Exists(const CStdString& strFileName)
{
  try
  {
    if (strFileName.IsEmpty()) return false;

    bool bPathInCache;
    if (g_directoryCache.FileExists(strFileName, bPathInCache) )
      return true;
    if (bPathInCache)
      return false;

    CURL url(strFileName);

    auto_ptr<IFile> pFile(CFileFactory::CreateLoader(url));
    if (!pFile.get()) return false;

    return pFile->Exists(url);
  }
#ifndef _LINUX
  catch (const win32_exception &e) 
  {
    e.writelog(__FUNCTION__);
  }
#endif
  catch(...)
  {
    CLog::Log(LOGERROR, "%s - Unhandled exception", __FUNCTION__);    
  }
  CLog::Log(LOGERROR, "%s - Error checking for %s", __FUNCTION__, strFileName.c_str());    
  return false;
}

int CFile::Stat(const CStdString& strFileName, struct __stat64* buffer)
{
  try
  {
    CURL url(strFileName);

    auto_ptr<IFile> pFile(CFileFactory::CreateLoader(url));
    if (!pFile.get()) return false;

    return pFile->Stat(url, buffer);
  }
#ifndef _LINUX
  catch (const win32_exception &e) 
  {
    e.writelog(__FUNCTION__);
  }
#endif
  catch(...)
  {
    CLog::Log(LOGERROR, "%s - Unhandled exception", __FUNCTION__);    
  }
  CLog::Log(LOGERROR, "%s - Error statting %s", __FUNCTION__, strFileName.c_str());
  return -1;
}

//*********************************************************************************************
unsigned int CFile::Read(void *lpBuf, __int64 uiBufSize)
{
  return Read(lpBuf, (unsigned int)uiBufSize, 0);
}

unsigned int CFile::Read(void *lpBuf, unsigned int uiBufSize, unsigned flags)
{
  try
  {
    if (m_pFile) 
    {
      if(flags & READ_TRUNCATED)
        return m_pFile->Read(lpBuf, uiBufSize);
      else
      {        
        unsigned int done = 0;
        while((uiBufSize-done) > 0)
        {
          unsigned int curr = m_pFile->Read((char*)lpBuf+done, uiBufSize-done);
          if(curr==0)
            break;

          done+=curr;
        }
        return done;
      }        
    }
    return 0;
  }
#ifndef _LINUX
  catch (const win32_exception &e) 
  {
    e.writelog(__FUNCTION__);
  }
#endif
  catch(...)
  {
    CLog::Log(LOGERROR, "%s - Unhandled exception", __FUNCTION__);    
  }
  return 0;
}

//*********************************************************************************************
void CFile::Close()
{
  try
  {
    if (m_pFile)
    {
      m_pFile->Close();
      delete m_pFile;
      m_pFile = NULL;
    }
  }
#ifndef _LINUX
  catch (const win32_exception &e) 
  {
    e.writelog(__FUNCTION__);
  }
#endif
  catch(...)
  {
    CLog::Log(LOGERROR, "%s - Unhandled exception", __FUNCTION__);    
  }
  return;
}

void CFile::Flush()
{
  try
  {
    if (m_pFile) m_pFile->Flush();
  }
#ifndef _LINUX
  catch (const win32_exception &e) 
  {
    e.writelog(__FUNCTION__);
  }
#endif
  catch(...)
  {
    CLog::Log(LOGERROR, "%s - Unhandled exception", __FUNCTION__);    
  }
  return;
}

//*********************************************************************************************
__int64 CFile::Seek(__int64 iFilePosition, int iWhence)
{
  try
  {
    if (m_pFile) return m_pFile->Seek(iFilePosition, iWhence);
    return 0;
  }
#ifndef _LINUX
  catch (const win32_exception &e) 
  {
    e.writelog(__FUNCTION__);
  }
#endif
  catch(...)
  {
    CLog::Log(LOGERROR, "%s - Unhandled exception", __FUNCTION__);    
  }
  return 0;
}

//*********************************************************************************************
__int64 CFile::GetLength()
{
  try
  {
    if (m_pFile) return m_pFile->GetLength();
    return 0;
  }
#ifndef _LINUX
  catch (const win32_exception &e) 
  {
    e.writelog(__FUNCTION__);
  }
#endif
  catch(...)
  {
    CLog::Log(LOGERROR, "%s - Unhandled exception", __FUNCTION__);    
  }
  return 0;
}

//*********************************************************************************************
__int64 CFile::GetPosition()
{
  try
  {
    if (m_pFile) return m_pFile->GetPosition();
    return -1;
  }
#ifndef _LINUX
  catch (const win32_exception &e) 
  {
    e.writelog(__FUNCTION__);
  }
#endif
  catch(...)
  {
    CLog::Log(LOGERROR, "%s - Unhandled exception", __FUNCTION__);    
  }
  return -1;
}


//*********************************************************************************************
bool CFile::ReadString(char *szLine, int iLineLength)
{
  try
  {
    return m_pFile->ReadString(szLine, iLineLength);
  }
#ifndef _LINUX
  catch (const win32_exception &e) 
  {
    e.writelog(__FUNCTION__);
  }
#endif
  catch(...)
  {
    CLog::Log(LOGERROR, "%s - Unhandled exception", __FUNCTION__);    
  }
  return false;
}

int CFile::Write(const void* lpBuf, __int64 uiBufSize)
{
  try
  {
    return m_pFile->Write(lpBuf, uiBufSize);
  }
#ifndef _LINUX
  catch (const win32_exception &e) 
  {
    e.writelog(__FUNCTION__);
  }
#endif
  catch(...)
  {
    CLog::Log(LOGERROR, "%s - Unhandled exception", __FUNCTION__);
  }
  return -1;
}

bool CFile::Delete(const CStdString& strFileName)
{
  try
  {
    CURL url(strFileName);

    auto_ptr<IFile> pFile(CFileFactory::CreateLoader(url));
    if (!pFile.get()) return false;

    if(pFile->Delete(url))
      return true;
  }
#ifndef _LINUX
  catch (const access_violation &e) 
  {
    e.writelog(__FUNCTION__);
  }
  catch (const win32_exception &e) 
  {
    e.writelog(__FUNCTION__);
  }
#endif
  catch(...)
  {
    CLog::Log(LOGERROR, "%s - Unhandled exception", __FUNCTION__);    
  }
  CLog::Log(LOGERROR, "%s - Error deleting file %s", __FUNCTION__, strFileName.c_str());
  return false;
}

bool CFile::Rename(const CStdString& strFileName, const CStdString& strNewFileName)
{
  try
  {
    CURL url(strFileName);
    CURL urlnew(strNewFileName);

    auto_ptr<IFile> pFile(CFileFactory::CreateLoader(url));
    if (!pFile.get()) return false;

    if(pFile->Rename(url, urlnew))
      return true;
  }
#ifndef _LINUX
  catch (const win32_exception &e) 
  {
    e.writelog(__FUNCTION__);
  }
#endif
  catch(...)
  {
    CLog::Log(LOGERROR, "%s - Unhandled exception ", __FUNCTION__);    
  }
  CLog::Log(LOGERROR, "%s - Error renaming file %s", __FUNCTION__, strFileName.c_str());    
  return false;
}

//*********************************************************************************************
//*************** Stream IO for CFile objects *************************************************
//*********************************************************************************************
CFileStreamBuffer::~CFileStreamBuffer()
{
  sync();
  Detach();
}

CFileStreamBuffer::CFileStreamBuffer(int backsize)
  : std::streambuf()
  , m_file(NULL)
  , m_buffer(NULL)
  , m_frontsize(0)
  , m_backsize(backsize)
{}

void CFileStreamBuffer::Attach(IFile *file)
{
  m_file = file;

  m_frontsize = m_file->GetChunkSize();
  if(!m_frontsize)
    m_frontsize = 1024;

  m_buffer = new char[m_frontsize+m_backsize];
  setg(0,0,0);
  setp(0,0);
}

void CFileStreamBuffer::Detach()
{
  setg(0,0,0);
  setp(0,0);
  if(m_buffer)
    SAFE_DELETE(m_buffer);
}

CFileStreamBuffer::int_type CFileStreamBuffer::underflow()
{
  if(gptr() < egptr())
    return traits_type::to_int_type(*gptr());

  if(!m_file)
    return traits_type::eof();
  
  int backsize = 0;
  if(m_backsize)
  {
    backsize = min(m_backsize, egptr()-eback());
    memmove(m_buffer, egptr()-backsize, backsize);
  }

  unsigned int size = 0;
#ifndef _LINUX
  try
  {
#endif
    size = m_file->Read(m_buffer+backsize, m_frontsize);
#ifndef _LINUX
  }
  catch (const win32_exception &e) 
  {
    e.writelog(__FUNCTION__);
  }  
#endif

  if(size == 0)
    return traits_type::eof();

  setg(m_buffer, m_buffer+backsize, m_buffer+backsize+size);
  return traits_type::to_int_type(*gptr());
}

CFileStreamBuffer::pos_type CFileStreamBuffer::seekoff(
  off_type offset, 
  ios_base::seekdir way, 
  ios_base::openmode mode)
{  
  if(way == ios_base::cur)
  {
    // try to seek within buffer
    if(gptr()+offset >= eback() && gptr()+offset < egptr())
    {
      gbump(offset);
      return m_file->GetPosition() - (eback() - gptr());
    }
  }

  // reset our buffer pointer, will
  // start buffering on next read
  setg(0,0,0);
  setp(0,0);

  __int64 position = -1;
#ifndef _LINUX
  try
  {
#endif
    if(way == ios_base::cur)
      position = m_file->Seek(offset, SEEK_CUR);
    else if(way == ios_base::end)
      position = m_file->Seek(offset, SEEK_END);
    else
      position = m_file->Seek(offset, SEEK_SET);
#ifndef _LINUX
  }
  catch (const win32_exception &e) 
  {
    e.writelog(__FUNCTION__);
    return (streampos(-1));
  }
#endif

  if(position<0)
    return (streampos(-1));

  return position;
}

CFileStreamBuffer::pos_type CFileStreamBuffer::seekpos(
  pos_type pos, 
  ios_base::openmode mode)
{
  // TODO check if seek can be done in buffer
  setg(0,0,0);
  setp(0,0);

#ifndef _LINUX
  try
  {
#endif
    pos = m_file->Seek(pos, SEEK_SET);
#ifndef _LINUX
  }
  catch (const win32_exception &e) 
  {
    e.writelog(__FUNCTION__);
    return (streampos(-1));
  }  
#endif

  if(pos<0)
    return (streampos(-1));

  return pos;
}


CFileStream::CFileStream(int backsize /*= 0*/)
    : m_buffer(backsize)
    , m_file(NULL)
    , std::istream(&m_buffer)
{
}

CFileStream::~CFileStream()
{
  Close();
}


bool CFileStream::Open(const CURL& filename)
{
  Close();

  m_file = CFileFactory::CreateLoader(filename);
  if(m_file && m_file->Open(filename, true))
  {
    m_buffer.Attach(m_file);
    return true;
  }

  setstate(failbit);
  return false;
}

__int64 CFileStream::GetLength()
{
  return m_file->GetLength();
}

void CFileStream::Close()
{
  if(!m_file)
    return;

  m_buffer.Detach();
  SAFE_DELETE(m_file);
}
