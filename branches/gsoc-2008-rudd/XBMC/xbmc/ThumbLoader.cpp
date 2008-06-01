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
#ifndef __STDC_CONSTANT_MACROS
#define __STDC_CONSTANT_MACROS
#endif
#ifdef _LINUX
#include "stdint.h"
#else
#define INT64_C __int64
#endif

#include "FileSystem/StackDirectory.h"
#include "ThumbLoader.h"
#include "Util.h"
#include "URL.h"
#include "Picture.h"
#include "FileSystem/File.h"
#include "FileItem.h"
#include "Settings.h"

#include "cores/ffmpeg/DllAvFormat.h"
#include "cores/ffmpeg/DllAvCodec.h"
#include "cores/ffmpeg/DllSwScale.h"

#include "cores/dvdplayer/DVDPlayer.h"

using namespace XFILE;
using namespace DIRECTORY;

CVideoThumbLoader::CVideoThumbLoader() 
{  
}

CVideoThumbLoader::~CVideoThumbLoader()
{
  StopThread();
}

void CVideoThumbLoader::OnLoaderStart() 
{
  // load ffmpeg libs in start - just a small optimization so that when the frame extractor will run ffmpeg will
  // already be in memory. not a requirement.
  if (!m_dllAvUtil.Load() || !m_dllAvCodec.Load() || !m_dllAvFormat.Load() || !m_dllSwScale.Load())  
  {
    CLog::Log(LOGERROR,"%s - failed to load ffmpeg lib", __FUNCTION__);
    return;
  }

  m_dllAvFormat.av_register_all();
  m_dllSwScale.sws_rgb2rgb_init(SWS_CPU_CAPS_MMX2);
}

void CVideoThumbLoader::OnLoaderFinish() 
{
  m_dllAvFormat.Unload();
  m_dllAvCodec.Unload();
  m_dllAvUtil.Unload();
  m_dllSwScale.Unload();
}

bool CVideoThumbLoader::ExtractThumb(const CStdString &strPath, const CStdString &strTarget)
{
  if (!g_guiSettings.GetBool("myvideos.autothumb"))
    return false;

  if (CUtil::IsMythTV(strPath)
  ||  CUtil::IsUPnP(strPath)
  ||  CUtil::IsTuxBox(strPath)
  ||  CUtil::IsDAAP(strPath))
    return false;

  if (CUtil::IsRemote(strPath) && !CUtil::IsOnLAN(strPath))
    return false;

  CLog::Log(LOGDEBUG,"%s - trying to extract thumb from video file %s", __FUNCTION__, strPath.c_str());
  return CDVDPlayer::ExtractThumb(strPath, strTarget);
}

bool CVideoThumbLoader::LoadItem(CFileItem* pItem)
{
  if (pItem->m_bIsShareOrDrive) return true;
  CStdString cachedThumb(pItem->GetCachedVideoThumb());

  if (!pItem->HasThumbnail())
  {
    pItem->SetUserVideoThumb();
    if (!CFile::Exists(cachedThumb))
    {
      CStdString strPath, strFileName;
      CUtil::Split(cachedThumb, strPath, strFileName);
       
      // create unique thumb for auto generated thumbs
      cachedThumb = strPath + "auto-" + strFileName;
      if (pItem->IsVideo() && !pItem->IsInternetStream() && !pItem->IsPlayList() && !CFile::Exists(cachedThumb))
      {
        if (pItem->IsStack())
        {
          CStackDirectory stack;
          CVideoThumbLoader::ExtractThumb(stack.GetFirstStackedFile(pItem->m_strPath), cachedThumb);
        }
        else
        {
          CVideoThumbLoader::ExtractThumb(pItem->m_strPath, cachedThumb);
        }
      }
  
      if (CFile::Exists(cachedThumb))
      {
        pItem->SetProperty("HasAutoThumb", "1");
        pItem->SetProperty("AutoThumbImage", cachedThumb);
        pItem->SetThumbnailImage(cachedThumb);
      }
    }
  }
  else
  {
    // look for remote thumbs
    CStdString thumb(pItem->GetThumbnailImage());
    if (!CURL::IsFileOnly(thumb) && !CUtil::IsHD(thumb))
    {      
      if(CFile::Exists(cachedThumb))
          pItem->SetThumbnailImage(cachedThumb);
      else
      {
        CPicture pic;
        if(pic.DoCreateThumbnail(thumb, cachedThumb))
          pItem->SetThumbnailImage(cachedThumb);
        else
          pItem->SetThumbnailImage("");
      }
    }  
  }

//  if (pItem->IsVideo() && !pItem->IsInternetStream())
//    CDVDPlayer::GetFileMetaData(pItem->m_strPath, pItem);

  return true;
}

CProgramThumbLoader::CProgramThumbLoader()
{
}

CProgramThumbLoader::~CProgramThumbLoader()
{
}

bool CProgramThumbLoader::LoadItem(CFileItem *pItem)
{
  if (pItem->m_bIsShareOrDrive) return true;
  if (!pItem->HasThumbnail())
    pItem->SetUserProgramThumb();
  return true;
}

CMusicThumbLoader::CMusicThumbLoader()
{
}

CMusicThumbLoader::~CMusicThumbLoader()
{
}

bool CMusicThumbLoader::LoadItem(CFileItem* pItem)
{
  if (pItem->m_bIsShareOrDrive) return true;
  if (!pItem->HasThumbnail())
    pItem->SetUserMusicThumb();
  else
  {
    // look for remote thumbs
    CStdString thumb(pItem->GetThumbnailImage());
    if (!CURL::IsFileOnly(thumb) && !CUtil::IsHD(thumb))
    {
      CStdString cachedThumb(pItem->GetCachedVideoThumb());
      if(CFile::Exists(cachedThumb))
        pItem->SetThumbnailImage(cachedThumb);
      else
      {
        CPicture pic;
        if(pic.DoCreateThumbnail(thumb, cachedThumb))
          pItem->SetThumbnailImage(cachedThumb);
        else
          pItem->SetThumbnailImage("");
      }
    }  
  }
  return true;
}

