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
#include "SmartPlaylistDirectory.h"
#include "utils/log.h"
#include "SmartPlaylist.h"
#include "MusicDatabase.h"
#include "VideoDatabase.h"
#include "FileSystem/Directory.h"
#include "FileSystem/File.h"
#include "FileItem.h"

namespace DIRECTORY
{
  CSmartPlaylistDirectory::CSmartPlaylistDirectory()
  {
  }

  CSmartPlaylistDirectory::~CSmartPlaylistDirectory()
  {
  }

  bool CSmartPlaylistDirectory::GetDirectory(const CStdString& strPath, CFileItemList& items)
  {
    // Load in the SmartPlaylist and get the WHERE query
    CSmartPlaylist playlist;
    if (!playlist.Load(strPath))
      return false;
    bool success = false, success2 = false;
    if (playlist.GetType().Equals("music") || playlist.GetType().Equals("mixed") || playlist.GetType().IsEmpty())
    {
      CMusicDatabase db;
      db.Open();
      CStdString type=playlist.GetType();
      if (type.IsEmpty())
        type = "music";
      if (playlist.GetType().Equals("mixed"))
        playlist.SetType("music");

      CStdString whereOrder = playlist.GetWhereClause() + " " + playlist.GetOrderClause();
      success = db.GetSongsByWhere("", whereOrder, items);
      db.Close();
      playlist.SetType(type);
    }
    if (playlist.GetType().Equals("video") || playlist.GetType().Equals("mixed"))
    {
      CVideoDatabase db;
      db.Open();
      CStdString type=playlist.GetType();
      if (playlist.GetType().Equals("mixed"))
        playlist.SetType("video");
      CStdString whereOrder = playlist.GetWhereClause() + " " + playlist.GetOrderClause();
      CFileItemList items2;
      success2 = db.GetMusicVideosByWhere("videodb://3/2/", whereOrder, items2);
      db.Close();
      items.Append(items2);
      playlist.SetType(type);
    }
    if (playlist.GetType().Equals("mixed"))
      return success || success2;
    else if (playlist.GetType().Equals("video"))
      return success2;
    else
      return success;
  }

  bool CSmartPlaylistDirectory::ContainsFiles(const CStdString& strPath)
  {
    // smart playlists always have files??
    return true;
  }

  CStdString CSmartPlaylistDirectory::GetPlaylistByName(const CStdString& name)
  {
    CFileItemList list;
    if (CDirectory::GetDirectory("special://musicplaylists/", list, "*.xsp"))
    {
      for (int i = 0; i < list.Size(); i++)
      {
        CFileItem *item = list[i];
        if (item->GetLabel().CompareNoCase(name) == 0)
        { // found :)
          return item->m_strPath;
        } 
      }
    }
    return "";
  }

  bool CSmartPlaylistDirectory::Remove(const char *strPath)
  { 
    return XFILE::CFile::Delete(strPath); 
  }
}


