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
#include "ThumbLoader.h"
#include "Util.h"
#include "URL.h"
#include "Picture.h"
#include "FileSystem/File.h"
#include "FileItem.h"
#include "TextureManager.h"
#include "VideoInfoTag.h"

using namespace XFILE;

CThumbLoader::CThumbLoader()
{
}

CThumbLoader::~CThumbLoader()
{
}

bool CThumbLoader::LoadRemoteThumb(CFileItem *pItem)
{
  // look for remote thumbs
  CStdString thumb(pItem->GetThumbnailImage());
  if (!g_TextureManager.CanLoad(thumb))
  {
    CStdString cachedThumb(pItem->GetCachedVideoThumb());
    if (CFile::Exists(cachedThumb))
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
  return pItem->HasThumbnail();
}

CVideoThumbLoader::CVideoThumbLoader()
{
}

CVideoThumbLoader::~CVideoThumbLoader()
{
  StopThread();
}

bool CVideoThumbLoader::LoadItem(CFileItem* pItem)
{
  if (pItem->m_bIsShareOrDrive) return true;
  if (pItem->IsVideoDb() && pItem->HasVideoInfoTag() && !pItem->HasThumbnail())
  {
    if (pItem->m_bIsFolder && pItem->GetVideoInfoTag()->m_iSeason > -1)
      return false;
    CFileItem item(*pItem->GetVideoInfoTag());
    bool bResult = LoadItem(&item);
    if (bResult)
    {
      pItem->SetProperty("HasAutoThumb",item.GetProperty("HasAutoThumb"));
      pItem->SetProperty("AutoThumbImage",item.GetProperty("AutoThumbImage"));
      pItem->SetThumbnailImage(item.GetThumbnailImage());
    }
    return bResult;
  }
  CStdString cachedThumb(pItem->GetCachedVideoThumb());

  if (!pItem->HasThumbnail())
    pItem->SetUserVideoThumb();
  else
    LoadRemoteThumb(pItem);

  if (!pItem->HasProperty("fanart_image"))
  {
    pItem->CacheFanart();
    if (CFile::Exists(pItem->GetCachedFanart()))
      pItem->SetProperty("fanart_image",pItem->GetCachedFanart());
  }                          
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
  else
    LoadRemoteThumb(pItem);
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
    LoadRemoteThumb(pItem);
  return true;
}

