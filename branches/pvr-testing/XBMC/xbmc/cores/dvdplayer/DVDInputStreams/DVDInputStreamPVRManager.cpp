/*
 *      Copyright (C) 2005-2009 Team XBMC
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
#include "DVDFactoryInputStream.h"
#include "DVDInputStreamPVRManager.h"
#include "FileSystem/PVRFile.h"
#include "URL.h"

using namespace XFILE;

/************************************************************************
 * Description: Class constructor, initialize member variables
 *              public class is CDVDInputStream
 */
CDVDInputStreamPVRManager::CDVDInputStreamPVRManager(IDVDPlayer* pPlayer) : CDVDInputStream(DVDSTREAM_TYPE_PVRMANAGER)
{
  m_pPlayer         = pPlayer;
  m_pFile           = NULL;
  m_pRecordable     = NULL;
  m_pLiveTV         = NULL;
  m_pTimeshift      = NULL;
  m_pOtherStream    = NULL;
  m_eof             = true;
  m_bPaused         = false;
}

/************************************************************************
 * Description: Class destructor
 */
CDVDInputStreamPVRManager::~CDVDInputStreamPVRManager()
{
  Close();
}

bool CDVDInputStreamPVRManager::IsEOF()
{
  if (m_pOtherStream)
    return m_pOtherStream->IsEOF();
  else
    return !m_pFile || m_eof;
}

bool CDVDInputStreamPVRManager::Open(const char* strFile, const std::string& content)
{
  /* Open PVR File for both cases, to have access to ILiveTVInterface and
   * IRecordable
   */
  m_pFile       = new CPVRFile();
  m_pLiveTV     = ((CPVRFile*)m_pFile)->GetLiveTV();
  m_pRecordable = ((CPVRFile*)m_pFile)->GetRecordable();
  m_pTimeshift  = ((CPVRFile*)m_pFile)->GetTimeshiftTV();

  /*
   * Translate the "pvr://....." entry.
   * The PVR Client can use http or whatever else is supported by DVDPlayer.
   * to access streams.
   * If after translation the file protocol is still "pvr://" use this class
   * to read the stream data over the CPVRFile class and the PVR Library itself.
   * Otherwise call CreateInputStream again with the translated filename and looks again
   * for the right protocol stream handler and swap every call to this input stream
   * handler.
   */
  std::string transFile = XFILE::CPVRFile::TranslatePVRFilename(strFile);
  if(transFile.substr(0, 6) != "pvr://")
  {
    m_pOtherStream = CDVDFactoryInputStream::CreateInputStream(m_pPlayer, transFile, content);
    if (!m_pOtherStream->Open(transFile.c_str(), content))
    {
      delete m_pFile;
      m_pFile = NULL;
      delete m_pOtherStream;
      m_pOtherStream = NULL;
      return false;
    }
  }
  else
  {
    if (!CDVDInputStream::Open(transFile.c_str(), content)) return false;

    CURL url(transFile);
    if (!m_pFile->Open(url))
    {
      delete m_pFile;
      m_pFile = NULL;
      return false;
    }
    m_eof = false;
  }
  return true;
}

// close file and reset everyting
void CDVDInputStreamPVRManager::Close()
{
  if (m_pFile)
  {
    m_pFile->Close();
    delete m_pFile;
  }

  if (m_pOtherStream)
  {
    m_pOtherStream->Close();
    delete m_pOtherStream;
  }
  else
  {
    CDVDInputStream::Close();
  }

  m_pPlayer         = NULL;
  m_pFile           = NULL;
  m_pLiveTV         = NULL;
  m_pRecordable     = NULL;
  m_pTimeshift      = NULL;
  m_pOtherStream    = NULL;
  m_eof             = true;
}

int CDVDInputStreamPVRManager::Read(BYTE* buf, int buf_size)
{
  if(!m_pFile) return -1;

  if (m_pOtherStream)
  {
    return m_pOtherStream->Read(buf, buf_size);
  }
  else
  {
    unsigned int ret = m_pFile->Read(buf, buf_size);

    /* we currently don't support non completing reads */
    if( ret <= 0 ) m_eof = true;

    return (int)(ret & 0xFFFFFFFF);
  }
}

__int64 CDVDInputStreamPVRManager::Seek(__int64 offset, int whence)
{
  if(!m_pFile) return -1;

  if (m_pOtherStream)
  {
    return m_pOtherStream->Seek(offset, whence);
  }
  else
  {
    __int64 ret = m_pFile->Seek(offset, whence);

    /* if we succeed, we are not eof anymore */
    if( ret >= 0 ) m_eof = false;

    return ret;
  }
}

__int64 CDVDInputStreamPVRManager::GetLength()
{
  if(!m_pFile) return -1;

  if (m_pOtherStream)
    return m_pOtherStream->GetLength();
  else
    return m_pFile->GetLength();
}

int CDVDInputStreamPVRManager::GetTotalTime()
{
  if (m_pLiveTV)
    return m_pLiveTV->GetTotalTime();
  return false;
}

int CDVDInputStreamPVRManager::GetStartTime()
{
  if (m_pLiveTV)
    return m_pLiveTV->GetStartTime();
  return false;
}

bool CDVDInputStreamPVRManager::NextChannel()
{
  if (m_pLiveTV)
    return m_pLiveTV->NextChannel();
  return false;
}

bool CDVDInputStreamPVRManager::PrevChannel()
{
  if (m_pLiveTV)
    return m_pLiveTV->PrevChannel();
  return false;
}

bool CDVDInputStreamPVRManager::SelectChannel(unsigned int channel)
{
  if (m_pLiveTV)
    return m_pLiveTV->SelectChannel(channel);
  return false;
}

bool CDVDInputStreamPVRManager::UpdateItem(CFileItem& item)
{
  if (m_pLiveTV)
    m_pLiveTV->UpdateItem(item);
  return false;
}

bool CDVDInputStreamPVRManager::SeekTime(int iTimeInMsec)
{
  fprintf(stderr, "SeekTime >>>>>>>>>> %i\n", iTimeInMsec);
  return false;
}

bool CDVDInputStreamPVRManager::Pause(double dTime)
{
  if (!m_pTimeshift)
    return false;

  if (m_bPaused)
  {
    m_bPaused = false;
    m_pTimeshift->SendPause(m_bPaused, dTime);
  }
  else
  {
    m_bPaused = true;
    m_pTimeshift->SendPause(m_bPaused, dTime);
  }

  return true;
};

bool CDVDInputStreamPVRManager::NextStream()
{
  if(!m_pFile) return -1;

  if (m_pOtherStream)
    return m_pOtherStream->NextStream();
  else
  {
    if(m_pFile->SkipNext())
    {
      m_eof = false;
      return true;
    }
  }
  return false;
}

bool CDVDInputStreamPVRManager::CanRecord()
{
  if (m_pRecordable)
    return m_pRecordable->CanRecord();
  return false;
}

bool CDVDInputStreamPVRManager::IsRecording()
{
  if (m_pRecordable)
    return m_pRecordable->IsRecording();
  return false;
}

bool CDVDInputStreamPVRManager::Record(bool bOnOff)
{
  if (m_pRecordable)
    return m_pRecordable->Record(bOnOff);
  return false;
}
