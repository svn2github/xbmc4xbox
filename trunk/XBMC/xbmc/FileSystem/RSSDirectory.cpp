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
#include "RSSDirectory.h"
#include "utils/RssFeed.h"
#include "Util.h"
#include "FileItem.h"
#include "FileCurl.h"

using namespace XFILE;
using namespace DIRECTORY;

CRSSDirectory::CRSSDirectory()
{
  SetCacheDirectory(DIR_CACHE_ONCE);
}

CRSSDirectory::~CRSSDirectory()
{
}

bool CRSSDirectory::GetDirectory(const CStdString& strPath, CFileItemList &items)
{
  CStdString path(strPath);
  CUtil::RemoveSlashAtEnd(path);
  
  CURL url(path);
  url.SetProtocol("http");
  url.GetURL(path);

  CRssFeed feed;
  feed.Init(path);
  feed.ReadFeed();
  
  feed.GetItemList(items);
  if (items.Size() == 0)
    return false;

  return true;
}
