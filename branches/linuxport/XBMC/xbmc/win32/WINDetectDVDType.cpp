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
#include "WINDetectDVDType.h"
#ifdef _WIN32PC
#include "WIN32Util.h"

using namespace MEDIA_DETECT;

CWINDetectDVDMedia* CWINDetectDVDMedia::m_instance = NULL;


CWINDetectDVDMedia::CWINDetectDVDMedia()
{
}

CWINDetectDVDMedia::~CWINDetectDVDMedia()
{
  RemoveAllMedia();
}
 
CWINDetectDVDMedia* CWINDetectDVDMedia::GetInstance()
{
  if( !m_instance )
    m_instance = new CWINDetectDVDMedia();
  return m_instance;
}

void CWINDetectDVDMedia::Destroy()
{
  if (m_instance)
  {
    delete m_instance;
    m_instance = NULL;
  }
}

CStdString CWINDetectDVDMedia::GetDrive(CStdString strDrive)
{
  CStdString strPath;
  if(!strDrive.empty())
    strPath = strDrive;
  else
    strPath = GetDevice(strDrive).c_str()+4;

  strPath.MakeLower();

  return strPath;
}

CStdString CWINDetectDVDMedia::GetDevice(CStdString strDrive)
{
  CStdString strDevice;
  if(!strDrive.empty())
    strDevice.Format("\\\\.\\%c:", strDrive[0]);
  else
    strDevice = CLibcdio::GetInstance()->GetDeviceFileName();

  strDevice.MakeLower();

  return strDevice;
}

void CWINDetectDVDMedia::AddMedia(CStdString& strDrive)
{
  CSingleLock waitLock(m_critsec);
  CStdString strPath = GetDrive(strDrive);
  std::map<char,CCdInfo*>::iterator it;
  it = m_mapCdInfo.find(strPath[0]);
  if(it == m_mapCdInfo.end())
  {
    CStdString strNewUrl;
    CCdIoSupport cdio;
    CLog::Log(LOGINFO, "Detecting DVD-ROM media filesystem...");
    CCdInfo* pCdInfo = cdio.GetCdInfo((char*)strPath.c_str());
    if (pCdInfo == NULL)
    {
      CLog::Log(LOGERROR, "Detection of DVD-ROM media failed.");
      return ;
    }
    else
      m_mapCdInfo.insert(std::pair<char,CCdInfo*>(strPath[0],pCdInfo));

    CLog::Log(LOGINFO, "Tracks overall:%i; Audio tracks:%i; Data tracks:%i",
              pCdInfo->GetTrackCount(),
              pCdInfo->GetAudioTrackCount(),
              pCdInfo->GetDataTrackCount() );

    if(pCdInfo->IsAudio(1))
      strNewUrl = "cdda://local/"; // we need to extend the cdda: protocol to support other drives (dvd:// as well?)
    else
      strNewUrl = strPath;

    CLog::Log(LOGINFO, "Using protocol %s", strNewUrl.c_str());

    if (pCdInfo->IsValidFs())
    {
      if (!pCdInfo->IsAudio(1))
        CLog::Log(LOGINFO, "Disc label: %s", pCdInfo->GetDiscLabel().c_str());
    }
    else
    {
      CLog::Log(LOGWARNING, "Filesystem is not supported");
    }

  }
}

void CWINDetectDVDMedia::RemoveMedia(CStdString& strDrive)
{
  CSingleLock waitLock(m_critsec);
  CStdString strPath = GetDrive(strDrive);
  std::map<char,CCdInfo*>::iterator it;
  it = m_mapCdInfo.find(strPath[0]);
  if(it != m_mapCdInfo.end())
  {
    if(it->second != NULL)
      delete it->second;

    m_mapCdInfo.erase(it);
  }
}

void CWINDetectDVDMedia::RemoveAllMedia()
{
  std::map<char,CCdInfo*>::iterator it;
  for (it=m_mapCdInfo.begin();it!=m_mapCdInfo.end();++it)
  {
    if(it->second != NULL)
      delete it->second;
  }
  m_mapCdInfo.clear();
}

CCdInfo* CWINDetectDVDMedia::GetCdInfo(CStdString& strDrive)
{
  CSingleLock waitLock(m_critsec);
  CStdString strPath = GetDrive(strDrive);
  std::map<char,CCdInfo*>::iterator it;
  it = m_mapCdInfo.find(strPath[0]);
  if(it != m_mapCdInfo.end())
    return it->second;
  else
    return NULL;
}

bool CWINDetectDVDMedia::IsAudio(CStdString strDrive)
{
  CSingleLock waitLock(m_critsec);
  CCdInfo* pCdInfo = GetCdInfo(strDrive);
  if(pCdInfo != NULL)
    return pCdInfo->IsAudio(1);
  else
    return false;
}

bool CWINDetectDVDMedia::IsDiscInDrive(CStdString strDrive)
{
  return GetTrayState(strDrive) == DRIVE_CLOSED_MEDIA_PRESENT;
}

void CWINDetectDVDMedia::WaitMediaReady()
{
  CSingleLock waitLock(m_critsec);
}

CStdString CWINDetectDVDMedia::GetDVDLabel(CStdString strDrive)
{
  CSingleLock waitLock(m_critsec);
  CStdString strPath = GetDrive(strDrive);
  std::map<char,CCdInfo*>::iterator it;
  it = m_mapCdInfo.find(strPath[0]);
  if(it == m_mapCdInfo.end())
    return "";
  
  CStdString strLabel;
  if(it->second->IsAudio(1))
    strLabel = "Audio-CD";
  else
  {
    strLabel = it->second->GetDiscLabel();
    strLabel.TrimRight(" ");
  }

  return strLabel;
}

CStdString CWINDetectDVDMedia::GetDVDPath(CStdString strDrive)
{
  CSingleLock waitLock(m_critsec);
  CStdString strPath = GetDrive(strDrive);
  std::map<char,CCdInfo*>::iterator it;
  it = m_mapCdInfo.find(strPath[0]);
  if(it != m_mapCdInfo.end())
  {
    if(it->second->IsAudio(1))
      strPath = "cdda://local/";
  }
  return strPath;
}

DWORD CWINDetectDVDMedia::GetTrayState(CStdString strDrive)
{
  CSingleLock waitLock(m_critsec);
  
  DWORD dwDriveState=1;
  int status = CWIN32Util::GetDriveStatus(GetDevice(strDrive));
  switch(status)
  {
  case -1: // error
    dwDriveState = DRIVE_NOT_READY;
    break;
  case 0: // no media
    dwDriveState = DRIVE_CLOSED_NO_MEDIA;
    break;
  case 1: // tray open
    dwDriveState = DRIVE_OPEN;      
    break;
  case 2: // media accessible
    dwDriveState = DRIVE_CLOSED_MEDIA_PRESENT;
    break;
  default:
    dwDriveState = DRIVE_NOT_READY;
  }

#ifdef HAS_DVD_DRIVE
  return dwDriveState;
#else
  return DRIVE_READY;
#endif
}


#endif
