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
#include "HDDirectory.h"
#include "Util.h"
#include "xbox/IoSupport.h"
#include "DirectoryCache.h"
#include "iso9660.h"
#include "URL.h"
#include "GUISettings.h"
#include "FileItem.h"

#ifndef INVALID_FILE_ATTRIBUTES
#define INVALID_FILE_ATTRIBUTES ((DWORD) -1)
#endif

#ifdef _WIN32PC
typedef WIN32_FIND_DATAW LOCAL_WIN32_FIND_DATA;
#define LocalFindFirstFile FindFirstFileW
#define LocalFindNextFile FindNextFileW
#else
typedef WIN32_FIND_DATA LOCAL_WIN32_FIND_DATA;
#define LocalFindFirstFile FindFirstFile
#define LocalFindNextFile FindNextFile
#endif 

using namespace AUTOPTR;
using namespace DIRECTORY;

CHDDirectory::CHDDirectory(void)
{}

CHDDirectory::~CHDDirectory(void)
{}

bool CHDDirectory::GetDirectory(const CStdString& strPath1, CFileItemList &items)
{
  LOCAL_WIN32_FIND_DATA wfd;

  CStdString strPath=strPath1;

  CFileItemList vecCacheItems;
  g_directoryCache.ClearDirectory(strPath1);


  CStdString strRoot = strPath;
  CURL url(strPath);

  memset(&wfd, 0, sizeof(wfd));
  if (!CUtil::HasSlashAtEnd(strPath) )
#ifndef _LINUX  
    strRoot += "\\";
  strRoot.Replace("/", "\\");
#else
    strRoot += "/";
#endif
  if (CUtil::IsDVD(strRoot) && m_isoReader.IsScanned())
  {
    // Reset iso reader and remount or
    // we can't access the dvd-rom
    m_isoReader.Reset();

    CIoSupport::Dismount("Cdrom0");
    CIoSupport::RemapDriveLetter('D', "Cdrom0");
  }

#ifndef _LINUX
  CStdStringW strSearchMask;
  g_charsetConverter.utf8ToW(strRoot,strSearchMask); 
  strSearchMask += "*.*";
#else
  CStdString strSearchMask = strRoot;
  strSearchMask += "*";
#endif

  FILETIME localTime;
  CAutoPtrFind hFind ( LocalFindFirstFile(strSearchMask.c_str(), &wfd));
  
  // on error, check if path exists at all, this will return true if empty folder
  if (!hFind.isValid()) 
      return Exists(strPath1);

  if (hFind.isValid())
  {
    do
    {
      if (wfd.cFileName[0] != 0)
      {
        CStdString strLabel;
#ifndef _LINUX
        g_charsetConverter.wToUTF8(wfd.cFileName,strLabel);
#else
        strLabel = wfd.cFileName;
#endif
        if ( (wfd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) )
        {
          if (strLabel != "." && strLabel != "..")
          {
            CFileItemPtr pItem(new CFileItem(strLabel));
            pItem->m_strPath = strRoot;
            pItem->m_strPath += strLabel;
            pItem->m_bIsFolder = true;
            CUtil::AddSlashAtEnd(pItem->m_strPath);
            FileTimeToLocalFileTime(&wfd.ftLastWriteTime, &localTime);
            pItem->m_dateTime=localTime;

            vecCacheItems.Add(pItem);

            /* Checks if the file is hidden. If it is then we don't really need to add it */
            if (!(wfd.dwFileAttributes & FILE_ATTRIBUTE_HIDDEN) || g_guiSettings.GetBool("filelists.showhidden"))
              items.Add(pItem);
          }
        }
        else
        {
          CFileItemPtr pItem(new CFileItem(strLabel));
          pItem->m_strPath = strRoot;
          pItem->m_strPath += strLabel;
          pItem->m_bIsFolder = false;
          pItem->m_dwSize = CUtil::ToInt64(wfd.nFileSizeHigh, wfd.nFileSizeLow);
          FileTimeToLocalFileTime(&wfd.ftLastWriteTime, &localTime);
          pItem->m_dateTime=localTime;

          /* Checks if the file is hidden. If it is then we don't really need to add it */
          if ((!(wfd.dwFileAttributes & FILE_ATTRIBUTE_HIDDEN) || g_guiSettings.GetBool("filelists.showhidden")) && IsAllowed(strLabel))
          {
            vecCacheItems.Add(pItem);
            items.Add(pItem);
          }
          else
            vecCacheItems.Add(pItem);
        }
      }
    }
    while (LocalFindNextFile((HANDLE)hFind, &wfd));
  }
  if (m_cacheDirectory)
    g_directoryCache.SetDirectory(strPath1, vecCacheItems);
  return true;
}

bool CHDDirectory::Create(const char* strPath)
{
  CStdString strPath1 = strPath;
  if (!CUtil::HasSlashAtEnd(strPath1))
#ifndef _LINUX  
    strPath1 += '\\';
#else
    strPath1 += '/';
#endif

#ifdef HAS_FTP_SERVER
  // okey this is really evil, since the create will succed
  // caller have no idea that a different directory was created
  if (g_guiSettings.GetBool("servers.ftpautofatx"))
  {
    CStdString strPath2(strPath1);
    CUtil::GetFatXQualifiedPath(strPath1);
    if(strPath2 != strPath1)
      CLog::Log(LOGNOTICE,"fatxq: %s -> %s",strPath2.c_str(), strPath1.c_str());
  }
#endif

#ifndef _LINUX
  CStdStringW strWPath1;
  g_charsetConverter.unknownToUTF8(strPath1);
  g_charsetConverter.utf8ToW(strPath1, strWPath1);
  if(::CreateDirectoryW(strWPath1, NULL))
#else
  if(::CreateDirectory(strPath1.c_str(), NULL))
#endif
    return true;
  else if(GetLastError() == ERROR_ALREADY_EXISTS)
    return true;

  return false;
}

bool CHDDirectory::Remove(const char* strPath)
{
#ifndef _LINUX
  CStdStringW strWPath;
  g_charsetConverter.utf8ToW(strPath, strWPath);
  return ::RemoveDirectoryW(strWPath) ? true : false;
#else
  return ::RemoveDirectory(strPath) ? true : false;
#endif
}

bool CHDDirectory::Exists(const char* strPath)
{
  CStdString strReplaced=strPath;
#ifndef _LINUX
  CStdStringW strWReplaced;
  strReplaced.Replace("/","\\");
  if (!CUtil::HasSlashAtEnd(strReplaced))
    strReplaced += '\\';
  g_charsetConverter.utf8ToW(strReplaced, strWReplaced);
  DWORD attributes = GetFileAttributesW(strWReplaced);
#else    
  DWORD attributes = GetFileAttributes(strReplaced.c_str());
#endif
  if(attributes == INVALID_FILE_ATTRIBUTES)
    return false;
  if (FILE_ATTRIBUTE_DIRECTORY & attributes) return true;
  return false;
}
