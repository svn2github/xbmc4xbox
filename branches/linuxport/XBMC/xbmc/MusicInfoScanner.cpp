/*
 *      Copyright (C) 2005-2007 Team XboxMediaCenter
 *      http://www.xboxmediacenter.com
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
 *  along with GNU Make; see the file COPYING.  If not, write to
 *  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 *  http://www.gnu.org/copyleft/gpl.html
 *
 */

#include "stdafx.h"
#include "MusicInfoScanner.h"
#include "MusicDatabase.h"
#include "musicInfoTagLoaderFactory.h"
#include "utils/MusicAlbumInfo.h"
#include "utils/MusicInfoScraper.h"
#include "FileSystem/DirectoryCache.h"
#include "FileSystem/MusicDatabaseDirectory.h"
#include "FileSystem/MusicDatabaseDirectory/DirectoryNode.h"
#include "Util.h"
#include "utils/md5.h"
#include "xbox/XKGeneral.h"
#include "NfoFile.h"
#include "MusicInfoTag.h"
#include "GUIWindowManager.h"
#include "GUIDialogProgress.h"
#include "GUIDialogSelect.h"
#include "GUIDialogKeyboard.h"
#include "FileSystem/File.h"
#include "Settings.h"
#include "FileItem.h"

#include <algorithm>

using namespace std;
using namespace MUSIC_INFO;
using namespace DIRECTORY;
using namespace MUSIC_GRABBER;

CMusicInfoScanner::CMusicInfoScanner()
{
  m_bRunning = false;
  m_pObserver = NULL;
  m_bCanInterrupt = false;
  m_currentItem=0;
  m_itemCount=0;
}

CMusicInfoScanner::~CMusicInfoScanner()
{
}

void CMusicInfoScanner::Process()
{
  try
  {
    DWORD dwTick = timeGetTime();

    m_musicDatabase.Open();

    if (m_pObserver)
      m_pObserver->OnStateChanged(PREPARING);

    m_bCanInterrupt = true;

    CUtil::ThumbCacheClear();
    g_directoryCache.ClearMusicThumbCache();

    if (m_scanType == 0) // load info from files
    {
      CLog::Log(LOGDEBUG, "%s - Starting scan", __FUNCTION__);
      m_musicDatabase.BeginTransaction();
      CLog::Log(LOGDEBUG, "%s - Starting scan", __FUNCTION__);
      m_musicDatabase.BeginTransaction();

      if (m_pObserver)
        m_pObserver->OnStateChanged(READING_MUSIC_INFO);

      // Reset progress vars
      m_currentItem=0;
      m_itemCount=-1;

      // Create the thread to count all files to be scanned
      SetPriority(THREAD_PRIORITY_IDLE);
      CThread fileCountReader(this);
      if (m_pObserver)
        fileCountReader.Create();

      // Database operations should not be canceled
      // using Interupt() while scanning as it could
      // result in unexpected behaviour.
      m_bCanInterrupt = false;
      m_needsCleanup = false;

      bool commit = false;
      bool cancelled = false;
      while (!cancelled && m_pathsToScan.size())
      {
        if (!DoScan(*m_pathsToScan.begin()))
          cancelled = true;
        commit = !cancelled;
      }

      if (commit)
      {
        m_musicDatabase.CommitTransaction();

        if (m_needsCleanup)
        {
          if (m_pObserver)
            m_pObserver->OnStateChanged(CLEANING_UP_DATABASE);

          m_musicDatabase.CleanupOrphanedItems();

          if (m_pObserver)
            m_pObserver->OnStateChanged(COMPRESSING_DATABASE);

          m_musicDatabase.Compress();
        }
      }
      else
        m_musicDatabase.RollbackTransaction();

      fileCountReader.StopThread();

      m_musicDatabase.EmptyCache();

      CUtil::ThumbCacheClear();
      g_directoryCache.ClearMusicThumbCache();

      m_musicDatabase.Close();
      CLog::Log(LOGDEBUG, "%s - Finished scan", __FUNCTION__);

      dwTick = timeGetTime() - dwTick;
      CStdString strTmp, strTmp1;
      StringUtils::SecondsToTimeString(dwTick / 1000, strTmp1);
      strTmp.Format("My Music: Scanning for music info using worker thread, operation took %s", strTmp1);
      CLog::Log(LOGNOTICE, strTmp.c_str());
    }
    if (m_scanType == 1) // load album info
    {
      if (m_pObserver)
        m_pObserver->OnStateChanged(DOWNLOADING_ALBUM_INFO);

      int iCurrentItem = 1;
      for (set<CAlbum>::iterator it=m_albumsToScan.begin();it != m_albumsToScan.end();++it)
      {
        if (m_pObserver)
        {
          m_pObserver->OnDirectoryChanged(it->strArtist+" - "+it->strAlbum);
          m_pObserver->OnSetProgress(iCurrentItem++, m_albumsToScan.size());
        }
        
        DownloadAlbumInfo(it->strGenre,it->strArtist,it->strAlbum); // genre field holds path - see fetchalbuminfo() 

        if (m_bStop)
          break;
      }
    }
    if (m_scanType == 2) // load artist info
    {
      if (m_pObserver)
        m_pObserver->OnStateChanged(DOWNLOADING_ARTIST_INFO);

      int iCurrentItem=1;
      for (set<CArtist>::iterator it=m_artistsToScan.begin();it != m_artistsToScan.end();++it)
      {
        if (m_pObserver)
        {
          m_pObserver->OnDirectoryChanged(it->strArtist);
          m_pObserver->OnSetProgress(iCurrentItem++, m_artistsToScan.size());
        }

        DownloadArtistInfo(it->strGenre,it->strArtist); // genre field holds path - see fetchartistinfo()

        if (m_bStop)
          break;
      }
    }

    m_bRunning = false;
    if (m_pObserver)
      m_pObserver->OnFinished();
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "MusicInfoScanner: Exception while scanning.");
  }
}

void CMusicInfoScanner::Start(const CStdString& strDirectory)
{
  m_pathsToScan.clear();
  m_albumsScanned.clear();
  m_artistsScanned.clear();

  if (strDirectory.IsEmpty())
  { // scan all paths in the database.  We do this by scanning all paths in the db, and crossing them off the list as
    // we go.
    m_musicDatabase.Open();
    m_musicDatabase.GetPaths(m_pathsToScan);
    m_musicDatabase.Close();
  }
  else
    m_pathsToScan.insert(strDirectory);
  m_pathsToCount = m_pathsToScan;
  m_scanType = 0;
  StopThread();
  Create();
  m_bRunning = true;
}

void CMusicInfoScanner::FetchAlbumInfo(const CStdString& strDirectory)
{
  m_albumsToScan.clear();
  m_albumsScanned.clear();

  CFileItemList items;
  if (strDirectory.IsEmpty())
  { 
    m_musicDatabase.Open();
    m_musicDatabase.GetAlbumsNav("musicdb://3/",items,-1,-1);
    m_musicDatabase.Close();
  }
  else
  {
    if (CUtil::HasSlashAtEnd(strDirectory)) // directory
      CDirectory::GetDirectory(strDirectory,items);
    else
      items.Add(new CFileItem(strDirectory,false));
  }
  
  for (int i=0;i<items.Size();++i)
  {
    if (CMusicDatabaseDirectory::IsAllItem(items[i]->m_strPath) || items[i]->IsParentFolder())
      continue;

    CAlbum album;
    album.strAlbum = items[i]->GetMusicInfoTag()->GetAlbum();
    album.strArtist = items[i]->GetMusicInfoTag()->GetArtist();
    album.strGenre = items[i]->m_strPath; // a bit hacky use of field
    m_albumsToScan.insert(album);
  }

  m_scanType = 1;
  StopThread();
  Create();
  m_bRunning = true;
}

void CMusicInfoScanner::FetchArtistInfo(const CStdString& strDirectory)
{
  m_artistsToScan.clear();
  m_artistsScanned.clear();
  CFileItemList items;

  if (strDirectory.IsEmpty())
  { 
    m_musicDatabase.Open();
    m_musicDatabase.GetArtistsNav("musicdb://2/",items,-1,false);
    m_musicDatabase.Close();
  }
  else
  {
    if (CUtil::HasSlashAtEnd(strDirectory)) // directory
      CDirectory::GetDirectory(strDirectory,items);
    else
      items.Add(new CFileItem(strDirectory,false));
  }

  for (int i=0;i<items.Size();++i)
  {
    if (CMusicDatabaseDirectory::IsAllItem(items[i]->m_strPath) || items[i]->IsParentFolder())
      continue;

    CArtist artist;
    artist.strArtist = items[i]->GetMusicInfoTag()->GetArtist();
    artist.strGenre = items[i]->m_strPath; // a bit hacky use of field
    m_artistsToScan.insert(artist);
  }

  m_scanType = 2;
  StopThread();
  Create();
  m_bRunning = true;
}

bool CMusicInfoScanner::IsScanning()
{
  return m_bRunning;
}

void CMusicInfoScanner::Stop()
{
  if (m_bCanInterrupt)
    m_musicDatabase.Interupt();

  StopThread();
}

void CMusicInfoScanner::SetObserver(IMusicInfoScannerObserver* pObserver)
{
  m_pObserver = pObserver;
}

bool CMusicInfoScanner::DoScan(const CStdString& strDirectory)
{
  if (m_pObserver)
    m_pObserver->OnDirectoryChanged(strDirectory);

  // load subfolder
  CFileItemList items;
  CDirectory::GetDirectory(strDirectory, items, g_stSettings.m_musicExtensions + "|.jpg|.tbn");

  // sort and get the path hash.  Note that we don't filter .cue sheet items here as we want
  // to detect changes in the .cue sheet as well.  The .cue sheet items only need filtering
  // if we have a changed hash.
  items.Sort(SORT_METHOD_LABEL, SORT_ORDER_ASC);
  CStdString hash;
  GetPathHash(items, hash);

  // get the folder's thumb (this will cache the album thumb).
  items.SetMusicThumb(true); // true forces it to get a remote thumb

  // check whether we need to rescan or not
  CStdString dbHash;
  if (!m_musicDatabase.GetPathHash(strDirectory, dbHash) || dbHash != hash)
  { // path has changed - rescan
    if (dbHash.IsEmpty())
      CLog::Log(LOGDEBUG, "%s Scanning dir '%s' as not in the database", __FUNCTION__, strDirectory.c_str());
    else
      CLog::Log(LOGDEBUG, "%s Rescanning dir '%s' due to change", __FUNCTION__, strDirectory.c_str());

    // filter items in the sub dir (for .cue sheet support)
    items.FilterCueItems();
    items.Sort(SORT_METHOD_LABEL, SORT_ORDER_ASC);

    // and then scan in the new information
    if (RetrieveMusicInfo(items, strDirectory) > 0)
    {
      if (m_pObserver)
        m_pObserver->OnDirectoryScanned(strDirectory);
    }

    // save information about this folder
    m_musicDatabase.SetPathHash(strDirectory, hash);
  }
  else
  { // path is the same - no need to rescan
    CLog::Log(LOGDEBUG, "%s Skipping dir '%s' due to no change", __FUNCTION__, strDirectory.c_str());
    m_currentItem += CountFiles(items, false);  // false for non-recursive

    // notify our observer of our progress
    if (m_pObserver)
    {
      if (m_itemCount>0)
        m_pObserver->OnSetProgress(m_currentItem, m_itemCount);
      m_pObserver->OnDirectoryScanned(strDirectory);
    }
  }

  // remove this path from the list we're processing
  set<CStdString>::iterator it = m_pathsToScan.find(strDirectory);
  if (it != m_pathsToScan.end())
    m_pathsToScan.erase(it);

  // now scan the subfolders
  for (int i = 0; i < items.Size(); ++i)
  {
    CFileItem *pItem = items[i];

    if (m_bStop)
      break;
    // if we have a directory item (non-playlist) we then recurse into that folder
    if (pItem->m_bIsFolder && !pItem->IsParentFolder() && !pItem->IsPlayList())
    {
      CStdString strPath=pItem->m_strPath;
      if (!DoScan(strPath))
      {
        m_bStop = true;
      }
    }
  }

  return !m_bStop;
}

int CMusicInfoScanner::RetrieveMusicInfo(CFileItemList& items, const CStdString& strDirectory)
{
  CSongMap songsMap;

  // get all information for all files in current directory from database, and remove them
  if (m_musicDatabase.RemoveSongsFromPath(strDirectory, songsMap))
    m_needsCleanup = true;

  VECSONGS songsToAdd;
  // for every file found, but skip folder
  for (int i = 0; i < items.Size(); ++i)
  {
    CFileItem* pItem = items[i];
    CStdString strExtension;
    CUtil::GetExtension(pItem->m_strPath, strExtension);

    if (m_bStop)
      return 0;

    // dont try reading id3tags for folders, playlists or shoutcast streams
    if (!pItem->m_bIsFolder && !pItem->IsPlayList() && !pItem->IsShoutCast() && !pItem->IsPicture())
    {
      m_currentItem++;
//      CLog::Log(LOGDEBUG, "%s - Reading tag for: %s", __FUNCTION__, pItem->m_strPath.c_str());

      // grab info from the song
      CSong *dbSong = songsMap.Find(pItem->m_strPath);

      CMusicInfoTag& tag = *pItem->GetMusicInfoTag();
      if (!tag.Loaded() )
      { // read the tag from a file
        auto_ptr<IMusicInfoTagLoader> pLoader (CMusicInfoTagLoaderFactory::CreateLoader(pItem->m_strPath));
        if (NULL != pLoader.get())
          pLoader->Load(pItem->m_strPath, tag);
      }

      // if we have the itemcount, notify our
      // observer with the progress we made
      if (m_pObserver && m_itemCount>0)
        m_pObserver->OnSetProgress(m_currentItem, m_itemCount);

      if (tag.Loaded())
      {
        CSong song(tag);
        song.iStartOffset = pItem->m_lStartOffset;
        song.iEndOffset = pItem->m_lEndOffset;
        if (dbSong)
        { // keep the db-only fields intact on rescan...
          song.iTimesPlayed = dbSong->iTimesPlayed;
          song.lastPlayed = dbSong->lastPlayed;
          if (song.rating == '0') song.rating = dbSong->rating;
        }
        pItem->SetMusicThumb();
        song.strThumb = pItem->GetThumbnailImage();
        songsToAdd.push_back(song);
//        CLog::Log(LOGDEBUG, "%s - Tag loaded for: %s", __FUNCTION__, spItem->m_strPath.c_str());
      }
      else
        CLog::Log(LOGDEBUG, "%s - No tag found for: %s", __FUNCTION__, pItem->m_strPath.c_str());
    }
  }

  CheckForVariousArtists(songsToAdd);
  if (!items.HasThumbnail())
    UpdateFolderThumb(songsToAdd, items.m_strPath);

  // finally, add these to the database
  for (unsigned int i = 0; i < songsToAdd.size(); ++i)
  {
    if (m_bStop) return i;
    CSong &song = songsToAdd[i];
    m_musicDatabase.AddSong(song, false);
    if (!m_bStop && g_guiSettings.GetBool("musiclibrary.autoartistinfo"))
    {
      long iArtist = m_musicDatabase.GetArtistByName(song.strArtist);
      CStdString strPath;
      strPath.Format("musicdb://2/%u/",iArtist);
      if (find(m_artistsScanned.begin(),m_artistsScanned.end(),iArtist) == m_artistsScanned.end())
        if (DownloadArtistInfo(strPath,song.strArtist))
          m_artistsScanned.push_back(iArtist);

      if (m_pObserver)
        m_pObserver->OnStateChanged(READING_MUSIC_INFO);
    }
    if (!m_bStop && g_guiSettings.GetBool("musiclibrary.autoalbuminfo"))
    {
      long iAlbum = m_musicDatabase.GetAlbumByName(song.strAlbum,song.strArtist);
      CStdString strPath;
      strPath.Format("musicdb://3/%u/",iAlbum);
      if (find(m_albumsScanned.begin(),m_albumsScanned.end(),iAlbum) == m_albumsScanned.end())
        if (DownloadAlbumInfo(strPath,song.strArtist,song.strAlbum))
          m_albumsScanned.push_back(iAlbum);
  
      if (m_pObserver)
        m_pObserver->OnStateChanged(READING_MUSIC_INFO);
    }
  }
  return songsToAdd.size();
}

static bool SortSongsByTrack(CSong *song, CSong *song2)
{
  return song->iTrack < song2->iTrack;
}

void CMusicInfoScanner::CheckForVariousArtists(VECSONGS &songsToCheck)
{
  // first, find all the album names for these songs
  map<CStdString, vector<CSong *> > albumsToAdd;
  map<CStdString, vector<CSong *> >::iterator it;
  for (unsigned int i = 0; i < songsToCheck.size(); ++i)
  {
    CSong &song = songsToCheck[i];
    if (!song.strAlbumArtist.IsEmpty()) // albumartist specified, so assume the user knows what they're doing
      continue;
    it = albumsToAdd.find(song.strAlbum);
    if (it == albumsToAdd.end())
    {
      vector<CSong *> songs;
      songs.push_back(&song);
      albumsToAdd.insert(make_pair(song.strAlbum, songs));
    }
    else
      it->second.push_back(&song);
  }
  // as an additional check for those that have multiple albums in the same folder, ignore albums
  // that have overlapping track numbers
  for (it = albumsToAdd.begin(); it != albumsToAdd.end();)
  {
    vector<CSong *> &songs = it->second;
    bool overlappingTrackNumbers(false);
    if (songs.size() > 1)
    {
      sort(songs.begin(), songs.end(), SortSongsByTrack);
      for (unsigned int i = 0; i < songs.size() - 1; i++)
      {
        CSong *song = songs[i];
        CSong *song2 = songs[i+1];
        if (song->iTrack == song2->iTrack)
        {
          overlappingTrackNumbers = true;
          break;
        }
      }
    }
    if (overlappingTrackNumbers)
    { // remove this album
      albumsToAdd.erase(it++);
    }
    else
      it++;
  }

  // ok, now run through these albums, and check whether they qualify as a "various artist" album
  // an album is considered a various artists album if the songs' primary artist differs
  // it qualifies as a "single artist with featured artists" album if the primary artist is the same, but secondary artists differ
  for (it = albumsToAdd.begin(); it != albumsToAdd.end(); it++)
  {
    const CStdString &album = it->first;
    vector<CSong *> &songs = it->second;
    if (!album.IsEmpty() && songs.size() > 1)
    {
      bool variousArtists(false);
      bool singleArtistWithFeaturedArtists(false);
      for (unsigned int i = 0; i < songs.size() - 1; i++)
      {
        CSong *song1 = songs[i];
        CSong *song2 = songs[i+1];
        CStdStringArray vecArtists1, vecArtists2;
        StringUtils::SplitString(song1->strArtist, g_advancedSettings.m_musicItemSeparator, vecArtists1);
        StringUtils::SplitString(song2->strArtist, g_advancedSettings.m_musicItemSeparator, vecArtists2);
        CStdString primaryArtist1 = vecArtists1[0]; primaryArtist1.TrimRight();
        CStdString primaryArtist2 = vecArtists2[0]; primaryArtist2.TrimRight();
        if (primaryArtist1 != primaryArtist2)
        { // primary artist differs -> a various artists album
          variousArtists = true;
          break;
        }
        else if (song1->strArtist != song2->strArtist)
        { // have more than one artist, the first artist(s) agree, but the full artist name doesn't
          // so this is likely a single-artist compilation (ie with other artists featured on some tracks) album
          singleArtistWithFeaturedArtists = true;
        }
      }
      if (variousArtists)
      { // have a various artists album - update all songs to be the various artist
        for (unsigned int i = 0; i < songs.size(); i++)
        {
          CSong *song = songs[i];
          song->strAlbumArtist = g_localizeStrings.Get(340); // Various Artists
        }
      }
      else if (singleArtistWithFeaturedArtists)
      { // have an album where all the first artists agree - make this the album artist
        CStdStringArray vecArtists;
        StringUtils::SplitString(songs[0]->strArtist, g_advancedSettings.m_musicItemSeparator, vecArtists);
        CStdString albumArtist(vecArtists[0]);
        for (unsigned int i = 0; i < songs.size(); i++)
        {
          CSong *song = songs[i];
          song->strAlbumArtist = albumArtist; // first artist of all tracks
        }
      }
    }
  }
}

bool CMusicInfoScanner::HasSingleAlbum(const VECSONGS &songs, CStdString &album, CStdString &artist)
{
  // check how many unique albums are in this path, and if there's only one, and it has a thumb
  // then cache the thumb as the folder thumb
  for (unsigned int i = 0; i < songs.size(); i++)
  {
    const CSong &song = songs[i];
    // don't bother with empty album tags - they're unlikely to have an embedded thumb anyway,
    // and if one is not tagged correctly, how can we know whether there is only one album?
    if (song.strAlbum.IsEmpty())
      return false;

    CStdString albumArtist = song.strAlbumArtist.IsEmpty() ? song.strArtist : song.strAlbumArtist;

    if (!album.IsEmpty() && (album != song.strAlbum || artist != albumArtist))
      return false; // have more than one album

    album = song.strAlbum;
    artist = albumArtist;
  }
  return !album.IsEmpty();
}

void CMusicInfoScanner::UpdateFolderThumb(const VECSONGS &songs, const CStdString &folderPath)
{
  CStdString album, artist;
  if (!HasSingleAlbum(songs, album, artist)) return;
  // Was the album art of this album read during scan?
  CStdString albumCoverArt(CUtil::GetCachedAlbumThumb(album, artist));
  if (CUtil::ThumbExists(albumCoverArt))
  {
    CStdString folderPath1(folderPath);
    // Folder art is cached without the slash at end
    CUtil::RemoveSlashAtEnd(folderPath1);
    CStdString folderCoverArt(CUtil::GetCachedMusicThumb(folderPath1));
    // copy as directory thumb as well
    if (::CopyFile(albumCoverArt, folderCoverArt, false))
      CUtil::ThumbCacheAdd(folderCoverArt, true);
  }
}

// This function is run by another thread
void CMusicInfoScanner::Run()
{
  int count = 0;
  while (!m_bStop && m_pathsToCount.size())
    count+=CountFilesRecursively(*m_pathsToCount.begin());
  m_itemCount = count;
}

// Recurse through all folders we scan and count files
int CMusicInfoScanner::CountFilesRecursively(const CStdString& strPath)
{
  // load subfolder
  CFileItemList items;
//  CLog::Log(LOGDEBUG, __FUNCTION__" - processing dir: %s", strPath.c_str());
  CDirectory::GetDirectory(strPath, items, g_stSettings.m_musicExtensions, false);

  if (m_bStop)
    return 0;

  // true for recursive counting
  int count = CountFiles(items, true);

  // remove this path from the list we're processing
  set<CStdString>::iterator it = m_pathsToCount.find(strPath);
  if (it != m_pathsToCount.end())
    m_pathsToCount.erase(it);

//  CLog::Log(LOGDEBUG, __FUNCTION__" - finished processing dir: %s", strPath.c_str());
  return count;
}

int CMusicInfoScanner::CountFiles(const CFileItemList &items, bool recursive)
{
  int count = 0;
  for (int i=0; i<items.Size(); ++i)
  {
    const CFileItem* pItem=items[i];

    if (recursive && pItem->m_bIsFolder)
      count+=CountFilesRecursively(pItem->m_strPath);
    else if (pItem->IsAudio() && !pItem->IsPlayList() && !pItem->IsNFO())
      count++;
  }
  return count;
}

int CMusicInfoScanner::GetPathHash(const CFileItemList &items, CStdString &hash)
{
  // Create a hash based on the filenames, filesize and filedate.  Also count the number of files
  if (0 == items.Size()) return 0;
  MD5_CTX md5state;
  unsigned char md5hash[16];
  char md5HexString[33];
  MD5Init(&md5state);
  int count = 0;
  for (int i = 0; i < items.Size(); ++i)
  {
    const CFileItem *pItem = items[i];
    MD5Update(&md5state, (unsigned char *)pItem->m_strPath.c_str(), (int)pItem->m_strPath.size());
    MD5Update(&md5state, (unsigned char *)&pItem->m_dwSize, sizeof(pItem->m_dwSize));
    FILETIME time = pItem->m_dateTime;
    MD5Update(&md5state, (unsigned char *)&time, sizeof(FILETIME));
    if (pItem->IsAudio() && !pItem->IsPlayList() && !pItem->IsNFO())
      count++;
  }
  MD5Final(md5hash, &md5state);
  XKGeneral::BytesToHexStr(md5hash, 16, md5HexString);
  hash = md5HexString;
  return count;
}

#define THRESHOLD .95f

bool CMusicInfoScanner::DownloadAlbumInfo(const CStdString& strPath, const CStdString& strArtist, const CStdString& strAlbum, CGUIDialogProgress* pDialog)
{
  CAlbum album;
  VECSONGS songs;
  DIRECTORY::MUSICDATABASEDIRECTORY::CQueryParams params;
  DIRECTORY::MUSICDATABASEDIRECTORY::CDirectoryNode::GetDatabaseInfo(strPath, params);

  m_musicDatabase.Open();
  if (m_musicDatabase.GetAlbumInfo(params.GetAlbumId(),album,&songs) && !album.strAlbum.IsEmpty())
    return true;

  // find album info
  SScraperInfo info;
  if (!m_musicDatabase.GetScraperForPath(strPath,info))
  {
    m_musicDatabase.Close();
    return false;
  }

  if (m_pObserver)
  {
    m_pObserver->OnStateChanged(DOWNLOADING_ALBUM_INFO);
    m_pObserver->OnDirectoryChanged(strAlbum);
  }

  CMusicInfoScraper scraper(info);

  // handle nfo files
  CStdString strAlbumPath, strNfo;
  m_musicDatabase.GetAlbumPath(params.GetAlbumId(),strAlbumPath);
  CUtil::AddFileToFolder(strAlbumPath,"album.nfo",strNfo);
  if (XFILE::CFile::Exists(strNfo))
  {
    CLog::Log(LOGDEBUG,"Found matching nfo file: %s", strNfo.c_str());
    CNfoFile nfoReader("albums");
    if (nfoReader.Create(strNfo) == S_OK)
    {
      if (nfoReader.m_strScraper == "NFO")
      {
        CLog::Log(LOGDEBUG, "%s Got details from nfo", __FUNCTION__);
        CAlbum album;
        VECSONGS songs;
        nfoReader.GetDetails(album);
        m_musicDatabase.SetAlbumInfo(params.GetAlbumId(), album, songs);
        m_musicDatabase.Close();
        return true;
      }
      else
      {
        CScraperUrl scrUrl(nfoReader.m_strImDbUrl); 
        CMusicAlbumInfo album("nfo",scrUrl);
        scraper.GetAlbums().push_back(album);
      }
    }
    else
      CLog::Log(LOGERROR,"Unable to find an url in nfo file: %s", strNfo.c_str());
  }

  if (!scraper.GetAlbumCount())
    scraper.FindAlbuminfo(strAlbum, strArtist);

  while (!scraper.Completed())
  {
    if (m_bStop)
    {
      scraper.Cancel();
      break;
    }
    Sleep(1);
  }

  CGUIDialogSelect *pDlg=NULL;
  int iSelectedAlbum=0;
  if (scraper.Successfull() && scraper.GetAlbumCount() >= 1)
  {
    int bestMatch = 0;
    double bestRelevance = 0;
    double minRelevance = THRESHOLD;
    if (scraper.GetAlbumCount() > 1) // score the matches
    {
      //show dialog with all albums found
      if (pDialog)
      {
        pDlg = (CGUIDialogSelect*)m_gWindowManager.GetWindow(WINDOW_DIALOG_SELECT);
        pDlg->SetHeading(g_localizeStrings.Get(181).c_str());
        pDlg->Reset();
        pDlg->EnableButton(true);
        pDlg->SetButtonLabel(413); // manual
      }

      double secondBestRelevance = 0;
      for (int i = 0; i < scraper.GetAlbumCount(); ++i)
      {
        CMusicAlbumInfo& info = scraper.GetAlbum(i);
        double relevance = CUtil::AlbumRelevance(info.GetAlbum().strAlbum, strAlbum, info.GetAlbum().strArtist, strArtist);

        // if we're doing auto-selection (ie querying all albums at once, then allow 95->100% for perfect matches)
        // otherwise, perfect matches only
        if (relevance >= max(minRelevance, bestRelevance))
        { // we auto-select the best of these
          secondBestRelevance = bestRelevance;
          bestRelevance = relevance;
          bestMatch = i;
        }
        if (pDialog)
        {
          // set the label to [relevance]  album - artist
          CStdString strTemp;
          strTemp.Format("[%0.2f]  %s", relevance, info.GetTitle2());
          CFileItem item(strTemp);
          item.m_idepth = i; // use this to hold the index of the album in the scraper
          pDlg->Add(&item);
        }
      }
    }
    else
    {
      CMusicAlbumInfo& info = scraper.GetAlbum(0);
      double relevance = CUtil::AlbumRelevance(info.GetAlbum().strAlbum, strAlbum, info.GetAlbum().strArtist, strArtist);
      if (relevance < THRESHOLD)
      {
        m_musicDatabase.Close();
        return false; 
      }
   }

    iSelectedAlbum = bestMatch;
    if (pDialog)
    {
      pDlg->Sort(false);
      pDlg->DoModal();

      // and wait till user selects one
      if (pDlg->GetSelectedLabel() < 0) 
      { // none chosen
        if (!pDlg->IsButtonPressed()) return false;
        // manual button pressed
        CStdString strNewAlbum = strAlbum;
        if (!CGUIDialogKeyboard::ShowAndGetInput(strNewAlbum, g_localizeStrings.Get(16011), false)) return false;
        if (strNewAlbum == "") return false;

        CStdString strNewArtist = strArtist;
        if (!CGUIDialogKeyboard::ShowAndGetInput(strNewArtist, g_localizeStrings.Get(16025), false)) return false;

        pDialog->SetLine(0, strNewAlbum);
        pDialog->SetLine(1, strNewArtist);
        pDialog->Progress();

        m_musicDatabase.Close();
        return DownloadAlbumInfo(strPath,strArtist,strAlbum,pDialog);
      }
      iSelectedAlbum = pDlg->GetSelectedItem().m_idepth;
    }
  }
  else
  {
    m_musicDatabase.Close();
    return false;
  }

  scraper.LoadAlbuminfo(iSelectedAlbum);
  while (!scraper.Completed())
  {
    if (m_bStop)
    {
      scraper.Cancel();
      break;
    }
    Sleep(1);
  }

  if (scraper.Successfull())
  {
    album = scraper.GetAlbum(iSelectedAlbum).GetAlbum();
    m_musicDatabase.SetAlbumInfo(params.GetAlbumId(), album, scraper.GetAlbum(iSelectedAlbum).GetSongs(),false);
  }
  else
  {
    m_musicDatabase.Close();
    return false;
  }

  // check thumb stuff
  if (album.thumbURL.m_url.size())
  {
    CStdString thumb;
    if (!m_musicDatabase.GetAlbumThumb(params.GetAlbumId(),thumb) || thumb.IsEmpty() || !XFILE::CFile::Exists(thumb))
    {
      thumb = CUtil::GetCachedAlbumThumb(album.strAlbum,album.strArtist);
      CScraperUrl::DownloadThumbnail(thumb,album.thumbURL.m_url[0]);
      m_musicDatabase.SaveAlbumThumb(params.GetAlbumId(), thumb);
    }
  }

  m_musicDatabase.Close();
  return true;
}

bool CMusicInfoScanner::DownloadArtistInfo(const CStdString& strPath, const CStdString& strArtist, CGUIDialogProgress* pDialog)
{
  DIRECTORY::MUSICDATABASEDIRECTORY::CQueryParams params;
  DIRECTORY::MUSICDATABASEDIRECTORY::CDirectoryNode::GetDatabaseInfo(strPath, params);
  CArtist artist;
  m_musicDatabase.Open();
  if (m_musicDatabase.GetArtistInfo(params.GetArtistId(),artist)) // already got the info
    return true;

  // find artist info
  SScraperInfo info;
  if (!m_musicDatabase.GetScraperForPath(strPath,info))
  {
    m_musicDatabase.Close();
    return false;
  }

  if (m_pObserver)
  {
    m_pObserver->OnStateChanged(DOWNLOADING_ARTIST_INFO);
    m_pObserver->OnDirectoryChanged(strArtist);  
  }

  CMusicInfoScraper scraper(info);
  // handle nfo files
  CStdString strArtistPath, strNfo;
  m_musicDatabase.GetArtistPath(params.GetArtistId(),strArtistPath);
  CUtil::AddFileToFolder(strArtistPath,"artist.nfo",strNfo);
  if (XFILE::CFile::Exists(strNfo))
  {
    CLog::Log(LOGDEBUG,"Found matching nfo file: %s", strNfo.c_str());
    CNfoFile nfoReader("artists");
    if (nfoReader.Create(strNfo) == S_OK)
    {
      if (nfoReader.m_strScraper == "NFO")
      {
        CLog::Log(LOGDEBUG, "%s Got details from nfo", __FUNCTION__);
        CArtist artist;
        nfoReader.GetDetails(artist);
        m_musicDatabase.SetArtistInfo(params.GetArtistId(), artist);
        m_musicDatabase.Close();
        return true;
      }
      else
      {
        CScraperUrl scrUrl(nfoReader.m_strImDbUrl); 
        CMusicArtistInfo artist("nfo",scrUrl);
        scraper.GetArtists().push_back(artist);
        info.strPath = nfoReader.m_strScraper;
        scraper.SetScraperInfo(info);
      }
    }
    else
      CLog::Log(LOGERROR,"Unable to find an url in nfo file: %s", strNfo.c_str());
  }

  if (!scraper.GetArtistCount())
    scraper.FindArtistinfo(strArtist);

  while (!scraper.Completed())
  {
    if (m_bStop)
    {
      scraper.Cancel();
      break;
    }
    Sleep(1);
  }

  if (scraper.GetArtistCount() >= 1)
  {
    int iSelectedArtist = 0;
    // now load the first match
    if (pDialog && scraper.GetArtistCount() > 1)
    {
      // if we found more then 1 album, let user choose one
      CGUIDialogSelect *pDlg = (CGUIDialogSelect*)m_gWindowManager.GetWindow(WINDOW_DIALOG_SELECT);
      if (pDlg)
      {
        pDlg->SetHeading(g_localizeStrings.Get(21890));
        pDlg->Reset();
        pDlg->EnableButton(true);
        pDlg->SetButtonLabel(413); // manual

        for (int i = 0; i < scraper.GetArtistCount(); ++i)
        {
          // set the label to artist
          CFileItem item(scraper.GetArtist(i).GetArtist());
          CStdString strTemp=scraper.GetArtist(i).GetArtist().strArtist;
          if (!scraper.GetArtist(i).GetArtist().strBorn.IsEmpty())
            strTemp += " ("+scraper.GetArtist(i).GetArtist().strBorn+")";
          if (!scraper.GetArtist(i).GetArtist().strGenre.IsEmpty())
            strTemp.Format("[%s] %s",scraper.GetArtist(i).GetArtist().strGenre.c_str(),strTemp.c_str());
          item.SetLabel(strTemp);
          item.m_idepth = i; // use this to hold the index of the album in the scraper
          pDlg->Add(&item);
        }
        pDlg->DoModal();

        // and wait till user selects one
        if (pDlg->GetSelectedLabel() < 0) 
        { // none chosen
          if (!pDlg->IsButtonPressed()) return false;
          // manual button pressed
          CStdString strNewArtist = strArtist;
          if (!CGUIDialogKeyboard::ShowAndGetInput(strNewArtist, g_localizeStrings.Get(16025), false)) return false;

          if (pDialog)
          {
            pDialog->SetLine(0, strNewArtist);
            pDialog->Progress();
          }
          m_musicDatabase.Close();
          return DownloadArtistInfo(strPath,strArtist,pDialog);
        }
        iSelectedArtist = pDlg->GetSelectedItem().m_idepth;
      }
    }
    
    scraper.LoadArtistinfo(iSelectedArtist);

    while (!scraper.Completed())
    {
      if (m_bStop)
      {
        scraper.Cancel();
        break;
      }
      Sleep(1);
    }

    if (scraper.Successfull())
    {
      artist = scraper.GetArtist(0).GetArtist();
      m_musicDatabase.SetArtistInfo(params.GetArtistId(), artist);
    }
  }
  else
  {
    m_musicDatabase.Close();
    return false;
  }

  // check thumb stuff
  if (artist.thumbURL.m_url.size())
  {
    CFileItem item(strArtist);
    CStdString thumb = item.GetCachedArtistThumb();
    if (!XFILE::CFile::Exists(thumb))
      CScraperUrl::DownloadThumbnail(thumb,artist.thumbURL.m_url[0]);
  }
   
  m_musicDatabase.Close();
  return true;
}
