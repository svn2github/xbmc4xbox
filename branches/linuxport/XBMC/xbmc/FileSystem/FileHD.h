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

// FileHD.h: interface for the CFileHD class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_FILEHD_H__DD2B0A9E_4971_4A29_B525_78CEFCDAF4A1__INCLUDED_)
#define AFX_FILEHD_H__DD2B0A9E_4971_4A29_B525_78CEFCDAF4A1__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "IFile.h"

namespace XFILE
{
class CFileHD : public IFile
{
public:
  CFileHD();
  virtual ~CFileHD();
  virtual __int64 GetPosition();
  virtual __int64 GetLength();
  virtual bool Open(const CURL& url, bool bBinary = true);
  virtual bool Exists(const CURL& url);
  virtual int Stat(const CURL& url, struct __stat64* buffer);
#ifdef _WIN32PC
  virtual int Stat(struct __stat64* buffer);
#else
  virtual int Stat(struct stat64* buffer);
#endif
  virtual unsigned int Read(void* lpBuf, __int64 uiBufSize);
  virtual int Write(const void* lpBuf, __int64 uiBufSize);
  virtual __int64 Seek(__int64 iFilePosition, int iWhence = SEEK_SET);
  virtual void Close();
  virtual void Flush();

  virtual bool OpenForWrite(const CURL& url, bool bBinary = true, bool bOverWrite = false);

  virtual bool Delete(const CURL& url);
  virtual bool Rename(const CURL& url, const CURL& urlnew);
  virtual int IoControl(int request, void* param);
protected:
  CStdString GetLocal(const CURL &url); /* crate a properly format path from an url */
  AUTOPTR::CAutoPtrHandle m_hFile;
  __int64 m_i64FilePos;
};

}
#endif // !defined(AFX_FILEHD_H__DD2B0A9E_4971_4A29_B525_78CEFCDAF4A1__INCLUDED_)
