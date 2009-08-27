
#include "stdafx.h" 
/*
 * XBMC Media Center
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
#include "WINFileSMB.h"
#include "URL.h"
#include "GUISettings.h"

#include <sys/stat.h>
#include <io.h>

using namespace XFILE;

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

//*********************************************************************************************
CWINFileSMB::CWINFileSMB()
    : m_hFile(INVALID_HANDLE_VALUE)
{}

//*********************************************************************************************
CWINFileSMB::~CWINFileSMB()
{
  if (m_hFile != INVALID_HANDLE_VALUE) Close();
}
//*********************************************************************************************
CStdString CWINFileSMB::GetLocal(const CURL &url)
{
  CStdString path( url.GetFileName() );

  if( url.GetProtocol().Equals("smb", false) )
  {
    CStdString host( url.GetHostName() );

    if(host.size() > 0)
    {
      path = "//" + host + "/" + path;
    }
  }

  path.Replace('/', '\\');

  return path;
}

//*********************************************************************************************
bool CWINFileSMB::Open(const CURL& url)
{
  CStdString strFile = GetLocal(url);

  CStdStringW strWFile;
  g_charsetConverter.utf8ToW(strFile, strWFile, false);
  m_hFile.attach(CreateFileW(strWFile.c_str(), GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, NULL));

  if (!m_hFile.isValid())
  {
    CLog::Log(LOGERROR,"CWINFileSMB: Unable to open file '%s' Error '%d%",strWFile.c_str(), GetLastError());
    return false;
  }

  m_i64FilePos = 0;
  Seek(0, SEEK_SET);

  return true;
}

bool CWINFileSMB::Exists(const CURL& url)
{
  struct __stat64 buffer;
  if(url.GetFileName() == url.GetShareName())
    return false;
  CStdString strFile = GetLocal(url);
  CStdStringW strWFile;
  g_charsetConverter.utf8ToW(strFile, strWFile, false);
  return (_wstat64(strWFile.c_str(), &buffer)==0);
}

int CWINFileSMB::Stat(struct __stat64* buffer)
{
  int fd;

  fd = _open_osfhandle((intptr_t)((HANDLE)m_hFile), 0);
  if (fd == -1)
    CLog::Log(LOGERROR, "CWINFileSMB Stat: fd == -1");

  return _fstat64(fd, buffer);
}

int CWINFileSMB::Stat(const CURL& url, struct __stat64* buffer)
{
  CStdString strFile = GetLocal(url);

  CStdStringW strWFile;
  g_charsetConverter.utf8ToW(strFile, strWFile, false);
  return _wstat64(strWFile.c_str(), buffer);
}


//*********************************************************************************************
bool CWINFileSMB::OpenForWrite(const CURL& url, bool bOverWrite)
{
  CStdString strPath = GetLocal(url);

  CStdStringW strWPath;
  g_charsetConverter.utf8ToW(strPath, strWPath, false);
  m_hFile.attach(CreateFileW(strWPath.c_str(), GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ, NULL, bOverWrite ? CREATE_ALWAYS : OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL));

  if (!m_hFile.isValid())
  {
    CLog::Log(LOGERROR,"CWINFileSMB: Unable to open file for writing '%s' Error '%d%",strWPath.c_str(), GetLastError());
    return false;
  }

  m_i64FilePos = 0;
  Seek(0, SEEK_SET);

  return true;
}

//*********************************************************************************************
unsigned int CWINFileSMB::Read(void *lpBuf, __int64 uiBufSize)
{
  if (!m_hFile.isValid()) return 0;
  DWORD nBytesRead;
  if ( ReadFile((HANDLE)m_hFile, lpBuf, (DWORD)uiBufSize, &nBytesRead, NULL) )
  {
    m_i64FilePos += nBytesRead;
    return nBytesRead;
  }
  return 0;
}

//*********************************************************************************************
int CWINFileSMB::Write(const void *lpBuf, __int64 uiBufSize)
{
  if (!m_hFile.isValid())
    return 0;
  
  DWORD nBytesWriten;
  if ( WriteFile((HANDLE)m_hFile, (void*) lpBuf, (DWORD)uiBufSize, &nBytesWriten, NULL) )
    return nBytesWriten;
  
  return 0;
}

//*********************************************************************************************
void CWINFileSMB::Close()
{
  m_hFile.reset();
}

//*********************************************************************************************
__int64 CWINFileSMB::Seek(__int64 iFilePosition, int iWhence)
{
  LARGE_INTEGER lPos, lNewPos;
  lPos.QuadPart = iFilePosition;
  int bSuccess;

  __int64 length = GetLength();

  switch (iWhence)
  {
  case SEEK_SET:
    if (iFilePosition <= length || length == 0)
      bSuccess = SetFilePointerEx((HANDLE)m_hFile, lPos, &lNewPos, FILE_BEGIN);
    else
      bSuccess = false;
    break;

  case SEEK_CUR:
    if ((GetPosition()+iFilePosition) <= length || length == 0)
      bSuccess = SetFilePointerEx((HANDLE)m_hFile, lPos, &lNewPos, FILE_CURRENT);
    else
      bSuccess = false;
    break;

  case SEEK_END:
    bSuccess = SetFilePointerEx((HANDLE)m_hFile, lPos, &lNewPos, FILE_END);
    break;

  default:
    return -1;
  }
  if (bSuccess)
  {
    m_i64FilePos = lNewPos.QuadPart;
    return m_i64FilePos;
  }
  else
    return -1;
}

//*********************************************************************************************
__int64 CWINFileSMB::GetLength()
{
  LARGE_INTEGER i64Size;
  GetFileSizeEx((HANDLE)m_hFile, &i64Size);
  return i64Size.QuadPart;
}

//*********************************************************************************************
__int64 CWINFileSMB::GetPosition()
{
  return m_i64FilePos;
}

bool CWINFileSMB::Delete(const CURL& url)
{
  CStdString strFile=GetLocal(url);

  CStdStringW strWFile;
  g_charsetConverter.utf8ToW(strFile, strWFile, false);
  return ::DeleteFileW(strWFile.c_str()) ? true : false;
}

bool CWINFileSMB::Rename(const CURL& url, const CURL& urlnew)
{
  CStdString strFile=GetLocal(url);
  CStdString strNewFile=GetLocal(urlnew);

  CStdStringW strWFile;
  CStdStringW strWNewFile;
  g_charsetConverter.utf8ToW(strFile, strWFile, false);
  g_charsetConverter.utf8ToW(strNewFile, strWNewFile, false);
  return ::MoveFileW(strWFile.c_str(), strWNewFile.c_str()) ? true : false;
}

void CWINFileSMB::Flush()
{
  ::FlushFileBuffers(m_hFile);
}

int CWINFileSMB::IoControl(int request, void* param)
{ 
  return -1;
}
