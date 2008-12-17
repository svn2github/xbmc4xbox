#pragma once
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

#include "IFile.h"
#include "ILiveTV.h"
#include "VideoInfoTag.h"


class CVTPSession;

namespace XFILE {

class CVTPFile 
  : public  IFile
  ,         ILiveTVInterface
{
public:
  CVTPFile();
  virtual ~CVTPFile();
  virtual bool          Open(const CURL& url, bool binary = true);
  virtual __int64       Seek(__int64 pos, int whence=SEEK_SET)         { return -1; }
  virtual __int64       GetPosition()                                  { return -1; }
  virtual __int64       GetLength()                                    { return -1; }
  virtual int           Stat(const CURL& url, struct __stat64* buffer) { return -1; }
  virtual void          Close();
  virtual unsigned int  Read(void* buffer, __int64 size);
  virtual CStdString    GetContent()                                   { return ""; }
  virtual bool          SkipNext()                                     { return false; }

  virtual bool          Delete(const CURL& url)                        { return false; }
  virtual bool          Exists(const CURL& url)                        { return false; }

  virtual ILiveTVInterface* GetLiveTV() {return (ILiveTVInterface*)this;}

  virtual bool           NextChannel()               { return false; }
  virtual bool           PrevChannel()               { return false; }

  virtual int            GetTotalTime()              { return 0; }
  virtual int            GetStartTime()              { return 0; }
  virtual bool           UpdateItem(CFileItem& item) { return false; }


protected:
  CVTPSession* m_session;
  SOCKET       m_socket;
};

}

