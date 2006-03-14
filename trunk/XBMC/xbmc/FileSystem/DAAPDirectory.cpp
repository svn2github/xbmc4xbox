/*
* DAAP Support for XBox Media Center
* Copyright (c) 2004 Forza (Chris Barnett)
* Portions Copyright (c) by the authors of libOpenDAAP
*
* This program is free software; you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation; either version 2 of the License, or
* (at your option) any later version.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with this program; if not, write to the Free Software
* Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
*/

#include "../stdafx.h"
#include "FileDAAP.h"
#include "DAAPDirectory.h"
#include "../util.h"
#include "directorycache.h"
// using the smb library for UTF-8 conversion
#include "../lib/libsmb/xbLibSmb.h"


CDAAPDirectory::CDAAPDirectory(void)
{
  CSectionLoader::Load("LIBXDAAP");
  // m_currLevel holds where we are in the playlist/artist/album/songs hierarchy (0,1,2,3)
  m_currLevel = -1;
  m_thisHost = NULL;
  m_artisthead = NULL;
  m_currentSongItems = NULL;
  m_currentSongItemCount = 0;
}

CDAAPDirectory::~CDAAPDirectory(void)
{  
  //if (m_thisClient) DAAP_Client_Release(m_thisClient);
  free_artists();

  m_thisHost = NULL;
  m_artisthead = NULL;

  m_currentSongItems = NULL;
  CSectionLoader::Unload("LIBXDAAP");
}

bool CDAAPDirectory::GetDirectory(const CStdString& strPath, CFileItemList &items)
{
  int c;
  //wchar_t wStrFile[1024]; // buffer for converting strings

  CURL url(strPath);

  CStdString strRoot = strPath;
  if (!CUtil::HasSlashAtEnd(strPath)) strRoot += "/";

  CFileItemList vecCacheItems;
  // Clear out any cached entries for this path
  g_directoryCache.ClearDirectory(strPath);


  m_thisHost = g_DaapClient.GetHost(url.GetHostName());
  if (!m_thisHost)  
    return false;
  

  // if we have at least one database we should show it's contents
  if (m_thisHost->nDatabases)
  {
    CLog::Log(LOGDEBUG, "Have %i databases", m_thisHost->nDatabases);
    //Store the first database
    g_DaapClient.m_iDatabase = m_thisHost->databases[0].id;

    // Get the songs from the database if we haven't already
    if (!m_artisthead)
    {
      m_currentSongItems = m_thisHost->dbitems[0].items;
      m_currentSongItemCount = m_thisHost->dbitems[0].nItems;

      CLog::Log(LOGDEBUG, "Getting songs from the database.  Have %i", m_currentSongItemCount);
      // Add each artist and album to an array
      int c;
      for (c = 0; c < m_currentSongItemCount; c++)
      {
        AddToArtistAlbum(m_currentSongItems[c].songartist, m_currentSongItems[c].songalbum);
      }
    }

    // find out where we are in the folder hierarchy
    m_currLevel = GetCurrLevel(strRoot);

    CLog::Log(LOGDEBUG, "DAAPDirectory: Current Level is %i", m_currLevel);
    if (m_currLevel < 0) // root, so show playlists
    {
      for (c = 0; c < m_thisHost->dbplaylists->nPlaylists; c++)
      {
        CStdString strFile;
        //size_t strLen;

        // we use UTF-8 internally, so no need to convert
        strFile = m_thisHost->dbplaylists->playlists[c].itemname;

        // Add item to directory list
        CLog::Log(LOGDEBUG, "DAAPDirectory: Adding item %s", strFile.c_str());
        CFileItem* pItem = new CFileItem(strFile);
        pItem->m_strPath = strRoot + m_thisHost->dbplaylists->playlists[c].itemname;
        pItem->m_bIsFolder = true;
        items.Add(new CFileItem(*pItem));
        vecCacheItems.Add(pItem);
      }
    }
    else if (m_currLevel == 0) // playlists, so show albums
    {
      CStdString strFolder;
      bool bFoundMatch;

      // find the playlist
      bFoundMatch = false;
      for (c = 0; c < m_thisHost->dbplaylists->nPlaylists; c++)
      {
        if (m_thisHost->dbplaylists->playlists[c].itemname == m_selectedPlaylist)
        {
          bFoundMatch = true;
          break;
        }
      }

      // if we found it show the songs contained ...
      if (bFoundMatch)
      {

        // if selected playlist name == d/b name then show in artist/album/song formation
        if (strcmp(m_thisHost->databases[0].name, m_thisHost->dbplaylists->playlists[c].itemname) == 0)
        {
          CStdString strBuffer;
          artistPTR *cur = m_artisthead;
          while (cur)
          {            
            strBuffer = cur->artist;
            CLog::Log(LOGDEBUG, "DAAPDirectory: Adding item %s", strBuffer.c_str());
            CFileItem* pItem = new CFileItem(strBuffer);
            pItem->m_strPath = strRoot + cur->artist;
            pItem->m_bIsFolder = true;
            items.Add(new CFileItem(*pItem));
            vecCacheItems.Add(pItem);
            cur = cur->next;
          }
        }
        else
        {
          // no support for playlist items in libOpenDAAP yet? - there is now :)

          int j;
          for (j = 0; j < m_thisHost->dbplaylists->playlists[c].count; j++)
          {
            // the playlist id is the song id, these do not directly match the array
            // position so we've no choice but to cycle through all the songs until
            // we find the one we want.
            int i, idx;
            idx = -1;
            for (i = 0; i < m_currentSongItemCount; i ++)
            {
              if (m_currentSongItems[i].id == m_thisHost->dbplaylists->playlists[c].items[j].songid)
              {
                idx = i;
                break;
              }
            }

            if (idx > -1)
            {              
              CLog::Log(LOGDEBUG, "DAAPDirectory: Adding item %s", m_currentSongItems[idx].itemname);
              CFileItem* pItem = new CFileItem(m_currentSongItems[idx].itemname);
              pItem->m_strPath.Format("daap://%s/%d.%s", m_thisHost->host, m_currentSongItems[idx].id,
                                      m_currentSongItems[idx].songformat);
              pItem->m_bIsFolder = false;
              pItem->m_dwSize = m_currentSongItems[idx].songsize;

              pItem->m_musicInfoTag.SetURL(pItem->m_strPath);

              
              pItem->m_musicInfoTag.SetTitle(m_currentSongItems[idx].itemname);
              pItem->m_musicInfoTag.SetArtist(m_currentSongItems[idx].songartist);
              pItem->m_musicInfoTag.SetAlbum(m_currentSongItems[idx].songalbum);

              //pItem->m_musicInfoTag.SetTrackNumber(m_currentSongItems[idx].songtracknumber);
              pItem->m_musicInfoTag.SetTrackNumber(m_thisHost->dbplaylists->playlists[c].items[j].songid);
              //pItem->m_musicInfoTag.SetTrackNumber(j+1);
              //pItem->m_musicInfoTag.SetPartOfSet(m_currentSongItems[idx].songdiscnumber);
              pItem->m_musicInfoTag.SetDuration((int) (m_currentSongItems[idx].songtime / 1000));
              pItem->m_musicInfoTag.SetLoaded(true);

              items.Add(new CFileItem(*pItem));
              vecCacheItems.Add(pItem);
            }
          }
        }
      }
    }
    else if (m_currLevel == 1) // artists, so show albums
    {
      // Find the artist ...
      artistPTR *cur = m_artisthead;
      while (cur)
      {
        if (cur->artist == m_selectedArtist) break;
        cur = cur->next;
      }

      // if we find it, then show albums for this artist
      if (cur)
      {
        albumPTR *curAlbum = cur->albumhead;
        while (curAlbum)
        {
          CLog::Log(LOGDEBUG, "DAAPDirectory: Adding item %s", curAlbum->album);
          CFileItem* pItem = new CFileItem(curAlbum->album);

          pItem->m_strPath = strRoot + curAlbum->album;
          pItem->m_bIsFolder = true;
          items.Add(new CFileItem(*pItem));
          vecCacheItems.Add(pItem);
          curAlbum = curAlbum->next;
        }
      }
    }
    else if (m_currLevel == 2) // albums, so show songs
    {
      int c;
      for (c = 0; c < m_currentSongItemCount; c++)
      {
        // if this song is for the current artist & album add it to the file list
        if (m_currentSongItems[c].songartist == m_selectedArtist &&
            m_currentSongItems[c].songalbum == m_selectedAlbum)
        {
          CLog::Log(LOGDEBUG, "DAAPDirectory: Adding item %s", m_currentSongItems[c].itemname);
          CFileItem* pItem = new CFileItem(m_currentSongItems[c].itemname);
          pItem->m_strPath.Format("daap://%s/%d.%s", m_thisHost->host, m_currentSongItems[c].id,
                                  m_currentSongItems[c].songformat);
          pItem->m_bIsFolder = false;
          pItem->m_dwSize = m_currentSongItems[c].songsize;

          pItem->m_musicInfoTag.SetURL(pItem->m_strPath);

          pItem->m_musicInfoTag.SetTitle(m_currentSongItems[c].itemname);
          pItem->m_musicInfoTag.SetArtist(m_selectedArtist);
          pItem->m_musicInfoTag.SetAlbum(m_selectedAlbum);

          pItem->m_musicInfoTag.SetTrackNumber(m_currentSongItems[c].songtracknumber);
          pItem->m_musicInfoTag.SetPartOfSet(m_currentSongItems[c].songdiscnumber);
          pItem->m_musicInfoTag.SetDuration((int) (m_currentSongItems[c].songtime / 1000));
          pItem->m_musicInfoTag.SetLoaded(true);

          items.Add(new CFileItem(*pItem));
          vecCacheItems.Add(pItem);
        }
      }
    }
  }

  g_directoryCache.SetDirectory(strPath, vecCacheItems);
  return true;
}

void CDAAPDirectory::free_albums(albumPTR *alb)
{
  albumPTR *cur = alb;
  while (cur)
  {
    albumPTR *next = cur->next;
    if (cur->album) free(cur->album);
    free(cur);
    cur = next;
  }
}

void CDAAPDirectory::free_artists()
{
  artistPTR *cur = m_artisthead;
  while (cur)
  {
    artistPTR *next = cur->next;
    if (cur->artist) free(cur->artist);
    if (cur->albumhead) free_albums(cur->albumhead);
    free(cur);
    cur = next;
  }
  m_artisthead = NULL;
}

void CDAAPDirectory::AddToArtistAlbum(char *artist_s, char *album_s)
{
  if (artist_s && album_s)
    CLog::Log(LOGDEBUG, "DAAP::AddToArtistAlbum(%s, %s)", artist_s, album_s);
  else if (artist_s)
    CLog::Log(LOGDEBUG, "DAAP::AddToArtistAlbum called with NULL album_s");
  else if (album_s)
    CLog::Log(LOGDEBUG, "DAAP::AddToArtistAlbum called with NULL artist_s");
  else
    CLog::Log(LOGDEBUG, "DAAP::AddToArtistAlbum called with NULL artist_s and NULL album_s");
  artistPTR *cur_artist = m_artisthead;
  albumPTR *cur_album = NULL;
  while (cur_artist)
  {
    if (strcmp(cur_artist->artist, artist_s) == 0)
      break;
    cur_artist = cur_artist->next;
  }
  if (!cur_artist)
  {
    artistPTR *newartist = (artistPTR *) malloc(sizeof(artistPTR));

    newartist->artist = (char *) malloc(strlen(artist_s) + 1);
    strcpy(newartist->artist, artist_s);

    newartist->albumhead = NULL;

    newartist->next = m_artisthead;
    m_artisthead = newartist;

    cur_artist = newartist;
  }
  cur_album = cur_artist->albumhead;
  while (cur_album)
  {
    if (strcmp(cur_album->album, album_s) == 0)
      break;
    cur_album = cur_album->next;
  }
  if (!cur_album)
  {
    albumPTR *newalbum = (albumPTR *) malloc(sizeof(albumPTR));

    newalbum->album = (char *) malloc(strlen(album_s) + 1);
    strcpy(newalbum->album, album_s);

    newalbum->next = cur_artist->albumhead;
    cur_artist->albumhead = newalbum;

    cur_album = newalbum;
  }
}

int CDAAPDirectory::GetCurrLevel(CStdString strPath)
{
  int intSPos;
  int intEPos;
  int intLevel;
  int intCnt;
  CStdString strJustPath;

  intSPos = strPath.Find("://");
  if (intSPos > -1)
    strJustPath = strPath.Right(strPath.size() - (intSPos + 3));
  else
    strJustPath = strPath;

  if (CUtil::HasSlashAtEnd(strJustPath))
    strJustPath = strJustPath.Left(strJustPath.size() - 1);

  intLevel = -1;
  intSPos = strPath.length();
  while (intSPos > -1)
  {
    intSPos = strJustPath.ReverseFind("/", intSPos);
    if (intSPos > -1) intLevel ++;
    intSPos -= 2;
  }

  m_selectedPlaylist = "";
  m_selectedArtist = "";
  m_selectedAlbum = "";
  intCnt = intLevel;
  intEPos = (strJustPath.length() - 1);
  while (intCnt >= 0)
  {
    intSPos = strJustPath.ReverseFind("/", intEPos);
    if (intSPos > -1)
    {
      if (intCnt == 2)  // album
      {
        m_selectedAlbum = strJustPath.substr(intSPos + 1, (intEPos - intSPos));
      }
      else if (intCnt == 1) // artist
      {
        m_selectedArtist = strJustPath.substr(intSPos + 1, (intEPos - intSPos));
      }
      else if (intCnt == 0) // playlist
      {
        m_selectedPlaylist = strJustPath.substr(intSPos + 1, (intEPos - intSPos));
      }

      intEPos = (intSPos - 1);
      intCnt --;
    }
  }

  return intLevel;
}
