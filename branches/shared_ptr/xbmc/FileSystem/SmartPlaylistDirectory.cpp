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
#include "Playlist.h"
#include "FileSystem/Directory.h"
#include "FileSystem/File.h"

using namespace PLAYLIST;

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
    if (playlist.GetType().Equals("tvshows"))
    {
      CVideoDatabase db;
      db.Open();
      CStdString whereOrder = playlist.GetWhereClause() + " " + playlist.GetOrderClause();
      success = db.GetTvShowsByWhere("videodb://2/2/", whereOrder, items);
      items.SetContent("tvshows");
      db.Close();
      return success;
    }
    else if (playlist.GetType().Equals("episodes"))
    {
      CVideoDatabase db;
      db.Open();
      CStdString whereOrder = playlist.GetWhereClause() + " " + playlist.GetOrderClause();
      success = db.GetEpisodesByWhere("videodb://2/2/", whereOrder, items);
      items.SetContent("episodes");
      db.Close();
      return success;
    }
    else if (playlist.GetType().Equals("movies"))
    {
      CVideoDatabase db;
      db.Open();
      CStdString whereOrder = playlist.GetWhereClause() + " " + playlist.GetOrderClause();
      success = db.GetMoviesByWhere("videodb://1/2/", whereOrder, items);
      items.SetContent("movies");
      db.Close();
      return success;
    }
    else if (playlist.GetType().Equals("albums"))
    {
      CMusicDatabase db;
      db.Open();
      CStdString whereOrder = playlist.GetWhereClause() + " " + playlist.GetOrderClause();
      success = db.GetAlbumsByWhere("musicdb://3/", whereOrder, items);
      items.SetContent("albums");
      db.Close();
      return success;
    }
    if (playlist.GetType().Equals("songs") || playlist.GetType().Equals("mixed") || playlist.GetType().IsEmpty())
    {
      CMusicDatabase db;
      db.Open();
      CStdString type=playlist.GetType();
      if (type.IsEmpty())
        type = "songs";
      if (playlist.GetType().Equals("mixed"))
        playlist.SetType("songs");

      CStdString whereOrder = playlist.GetWhereClause() + " " + playlist.GetOrderClause();
      success = db.GetSongsByWhere("", whereOrder, items);
      items.SetContent("songs");
      db.Close();
      playlist.SetType(type);
    }
    if (playlist.GetType().Equals("musicvideos") || playlist.GetType().Equals("mixed"))
    {
      CVideoDatabase db;
      db.Open();
      CStdString type=playlist.GetType();
      if (playlist.GetType().Equals("mixed"))
        playlist.SetType("musicvideos");
      CStdString whereOrder = playlist.GetWhereClause() + " " + playlist.GetOrderClause();
      CFileItemList items2;
      success2 = db.GetMusicVideosByWhere("videodb://3/2/", whereOrder, items2, false); // TODO: SMARTPLAYLISTS Don't check locks???
      db.Close();
      items.Append(items2);
      items.SetContent("musicvideos");
      playlist.SetType(type);
    }
    if (playlist.GetType().Equals("mixed"))
      return success || success2;
    else if (playlist.GetType().Equals("musicvideos"))
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
        CFileItemPtr item = list[i];
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


