/*
 *      Copyright (C) 2005-2008 Team XBMC
 *      http://www.xbmc.org
 *
 *  This Program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your option)
 *  any later version.
 *
 *  This Program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with XBMC; see the file COPYING.  If not, write to
 *  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 *  http://www.gnu.org/copyleft/gpl.html
 *
 */

#include "stdafx.h"
#include "FileItem.h"
#include "DVDInputStreamMMS.h"
#include "VideoInfoTag.h"
#include "FileSystem/IFile.h"

#include "Application.h"

#include <libmms/mmsio.h> // FIXME: remove this header once the ubuntu headers is fixed (variable named this)

using namespace XFILE;

CDVDInputStreamMMS::CDVDInputStreamMMS() : CDVDInputStream(DVDSTREAM_TYPE_MMS)
{
  m_mms = NULL;
}

CDVDInputStreamMMS::~CDVDInputStreamMMS()
{
  Close();
}

bool CDVDInputStreamMMS::IsEOF()
{
  return false;
}

bool CDVDInputStreamMMS::Open(const char* strFile, const std::string& content)
{
  return (m_mms=mmsx_connect((mms_io_t*)mms_get_default_io_impl(),NULL,strFile,2000*1000)); // TODO: what to do with bandwidth?
}

// close file and reset everyting
void CDVDInputStreamMMS::Close()
{
  CDVDInputStream::Close();
  if (m_mms)
    mmsx_close(m_mms);
}

int CDVDInputStreamMMS::Read(BYTE* buf, int buf_size)
{
  return mmsx_read((mms_io_t*)mms_get_default_io_impl(),m_mms,(char*)buf,buf_size);
}

__int64 CDVDInputStreamMMS::Seek(__int64 offset, int whence)
{
  if(whence == SEEK_POSSIBLE)
    return 0;
  else
    return -1; // TODO: implement offset based seeks
}

bool CDVDInputStreamMMS::SeekTime(int iTimeInMsec)
{
  if (mmsx_get_seekable(m_mms))
    return mmsx_time_seek(NULL,m_mms,double(iTimeInMsec)/1000);
  
  return false;
}

__int64 CDVDInputStreamMMS::GetLength()
{
  return mmsx_get_time_length(m_mms);
}

bool CDVDInputStreamMMS::NextStream()
{
  return false;
}

