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
#include "PartyModeManager.h"
#include "Application.h"
#include "playlistplayer.h"
#include "MusicDatabase.h"
#include "Util.h"
#include "FileItem.h"
#include "GUIWindowMusicPlayList.h"
#include "SmartPlaylist.h"
#include "GUIDialogProgress.h"

using namespace PLAYLIST;

#define QUEUE_DEPTH       10

CPartyModeManager g_partyModeManager;

CPartyModeManager::CPartyModeManager(void)
{
  m_bEnabled = false;
  m_strCurrentFilter = "";
  ClearState();
}

CPartyModeManager::~CPartyModeManager(void)
{
}
//#define NEW_PARTY_MODE_METHOD 1
bool CPartyModeManager::Enable()
{
  // Filter using our PartyMode xml file
  CSmartPlaylist playlist;
  CStdString partyModePath;

  CGUIDialogProgress* pDialog = (CGUIDialogProgress*)m_gWindowManager.GetWindow(WINDOW_DIALOG_PROGRESS);
  pDialog->SetHeading(20121);
  pDialog->SetLine(0,20123);
  pDialog->SetLine(1, "");
  pDialog->SetLine(2, "");
  pDialog->StartModal();
  partyModePath = g_settings.GetUserDataItem("partymode.xml");
  if (playlist.Load(partyModePath))
    m_strCurrentFilter = playlist.GetWhereClause();
  else
    m_strCurrentFilter.Empty();
  CLog::Log(LOGINFO, "PARTY MODE MANAGER: Registering filter:[%s]", m_strCurrentFilter.c_str());

  ClearState();
  CMusicDatabase musicdatabase;
  DWORD time = timeGetTime();
  vector<long> songIDs;
  if (musicdatabase.Open())
  {
    m_iMatchingSongs = (int)musicdatabase.GetSongIDs(m_strCurrentFilter, songIDs);
    if (m_iMatchingSongs < 1)
    {
      pDialog->Close();
      musicdatabase.Close();
      OnError(16031, (CStdString)"Party mode found no matching songs. Aborting.");
      return false;
    }
  }
  else
  {
    pDialog->Close();
    OnError(16033, (CStdString)"Party mode could not open database. Aborting.");
    return false;
  }
  musicdatabase.Close();

  // calculate history size
  if (m_iMatchingSongs < 50)
    m_songsInHistory = 0;
  else
    m_songsInHistory = (int)(m_iMatchingSongs/2);
  if (m_songsInHistory > 200)
    m_songsInHistory = 200;

  CLog::Log(LOGINFO,"PARTY MODE MANAGER: Matching songs = %i, History size = %i", m_iMatchingSongs, m_songsInHistory);
  CLog::Log(LOGINFO,"PARTY MODE MANAGER: Party mode enabled!");

  // setup the playlist
  g_playlistPlayer.ClearPlaylist(PLAYLIST_MUSIC);
  g_playlistPlayer.SetShuffle(PLAYLIST_MUSIC, false);
  g_playlistPlayer.SetRepeat(PLAYLIST_MUSIC, PLAYLIST::REPEAT_NONE);

  pDialog->SetLine(0,20124);
  pDialog->Progress();
  // add initial songs
  if (!AddInitialSongs(songIDs))
  {
    pDialog->Close();
    return false;
  }
  CLog::Log(LOGDEBUG, __FUNCTION__" time for song fetch: %i", timeGetTime() - time);
  // start playing
  g_playlistPlayer.SetCurrentPlaylist(PLAYLIST_MUSIC);
  Play(0);

  pDialog->Close();
  // open now playing window
  if (m_gWindowManager.GetActiveWindow() != WINDOW_MUSIC_PLAYLIST)
    m_gWindowManager.ActivateWindow(WINDOW_MUSIC_PLAYLIST);

  // done
  m_bEnabled = true;
  return true;
}

void CPartyModeManager::Disable()
{
  if (!IsEnabled())
    return;
  m_bEnabled = false;
  CLog::Log(LOGINFO,"PARTY MODE MANAGER: Party mode disabled.");
}

void CPartyModeManager::OnSongChange(bool bUpdatePlayed /* = false */)
{
  if (!IsEnabled())
    return;
  Process();
  if (bUpdatePlayed)
    m_iSongsPlayed++;
}

void CPartyModeManager::AddUserSongs(CPlayList& tempList, bool bPlay /* = false */)
{
  if (!IsEnabled())
    return;

  // where do we add?
  int iAddAt = -1;
  if (m_iLastUserSong < 0 || bPlay)
    iAddAt = 1; // under the currently playing song
  else
    iAddAt = m_iLastUserSong + 1; // under the last user added song

  int iNewUserSongs = tempList.size();
  CLog::Log(LOGINFO,"PARTY MODE MANAGER: Adding %i user selected songs at %i", iNewUserSongs, iAddAt);

  g_playlistPlayer.GetPlaylist(PLAYLIST_MUSIC).Insert(tempList, iAddAt);

  // update last user added song location
  if (m_iLastUserSong < 0)
    m_iLastUserSong = 0;
  m_iLastUserSong += iNewUserSongs;

  if (bPlay)
    Play(1);
}

void CPartyModeManager::AddUserSongs(CFileItemList& tempList, bool bPlay /* = false */)
{
  if (!IsEnabled())
    return;

  // where do we add?
  int iAddAt = -1;
  if (m_iLastUserSong < 0 || bPlay)
    iAddAt = 1; // under the currently playing song
  else
    iAddAt = m_iLastUserSong + 1; // under the last user added song

  int iNewUserSongs = tempList.Size();
  CLog::Log(LOGINFO,"PARTY MODE MANAGER: Adding %i user selected songs at %i", iNewUserSongs, iAddAt);

  g_playlistPlayer.GetPlaylist(PLAYLIST_MUSIC).Insert(tempList, iAddAt);

  // update last user added song location
  if (m_iLastUserSong < 0)
    m_iLastUserSong = 0;
  m_iLastUserSong += iNewUserSongs;

  if (bPlay)
    Play(1);
}

void CPartyModeManager::Process()
{
  ReapSongs();
  MovePlaying();
  AddRandomSongs();
  UpdateStats();
  SendUpdateMessage();
}

bool CPartyModeManager::AddRandomSongs(int iSongs /* = 0 */)
{
  CPlayList& playlist = g_playlistPlayer.GetPlaylist(PLAYLIST_MUSIC);
  int iMissingSongs = QUEUE_DEPTH - playlist.size();
  if (iSongs <= 0)
    iSongs = iMissingSongs;
  if (iSongs > 0)
  {
    // add songs to fill queue
    CMusicDatabase musicdatabase;
    if (musicdatabase.Open())
    {
      // Method:
      // 1. Grab a random entry from the database using a where clause
      // 2. Iterate on iSongs.

      // Note: At present, this method is faster than the alternative, which is to grab
      // all valid songids, then select a random number of them (as done in AddInitialSongs()).
      // The reason for this is simply the number of songs we are requesting - we generally
      // only want one here.  Any more than about 3 songs and it is more efficient
      // to use the technique in AddInitialSongs.  As it's unlikely that we'll require
      // more than 1 song at a time here, this method is faster.
      bool error(false);
      musicdatabase.BeginTransaction();
      for (int i = 0; i < iSongs; i++)
      {
        CStdString whereClause = GetWhereClauseWithHistory();
        CFileItem *item = new CFileItem;
        long songID;
        if (musicdatabase.GetRandomSong(item, songID, GetWhereClauseWithHistory()))
        { // success
          Add(item);
          AddToHistory(songID);
        }
        else
        {
          delete item;
          error = true;
          break;
        }
      }
      musicdatabase.CommitTransaction();

      if (error)
      {
        musicdatabase.Close();
        OnError(16034, (CStdString)"Cannot get songs from database. Aborting.");
        return false;
      }
    }
    else
    {
      OnError(16033, (CStdString)"Party mode could not open database. Aborting.");
      return false;
    }
    musicdatabase.Close();
  }
  return true;
}

void CPartyModeManager::Add(CFileItem *pItem)
{
  CPlayList::CPlayListItem playlistItem;
  CUtil::ConvertFileItemToPlayListItem(pItem, playlistItem);
  CPlayList& playlist = g_playlistPlayer.GetPlaylist(PLAYLIST_MUSIC);
  playlist.Add(playlistItem);
  CLog::Log(LOGINFO,"PARTY MODE MANAGER: Adding randomly selected song at %i:[%s]", playlist.size() - 1, pItem->m_strPath.c_str());
  m_iMatchingSongsPicked++;
}

bool CPartyModeManager::ReapSongs()
{
  CPlayList& playlist = g_playlistPlayer.GetPlaylist(PLAYLIST_MUSIC);

  // reap any played songs
  int iCurrentSong = g_playlistPlayer.GetCurrentSong();
  int i=0;
  while (i < g_playlistPlayer.GetPlaylist(PLAYLIST_MUSIC).size())
  {
    if (i < iCurrentSong)
    {
      g_playlistPlayer.GetPlaylist(PLAYLIST_MUSIC).Remove(i);
      iCurrentSong--;
      if (i <= m_iLastUserSong)
        m_iLastUserSong--;
    }
    else
      i++;
  }

  g_playlistPlayer.SetCurrentSong(iCurrentSong);
  return true;
}

bool CPartyModeManager::MovePlaying()
{
  // move current song to the top if its not there
  int iCurrentSong = g_playlistPlayer.GetCurrentSong();
  if (iCurrentSong > 0)
  {
    CLog::Log(LOGINFO,"PARTY MODE MANAGER: Moving currently playing song from %i to 0", iCurrentSong);
    CPlayList &playlist = g_playlistPlayer.GetPlaylist(PLAYLIST_MUSIC);
    CPlayList playlistTemp;
    playlistTemp.Add(playlist[iCurrentSong]);
    playlist.Remove(iCurrentSong);
    for (int i=0; i<playlist.size(); i++)
      playlistTemp.Add(playlist[i]);
    playlist.Clear();
    for (int i=0; i<playlistTemp.size(); i++)
      playlist.Add(playlistTemp[i]);
  }
  g_playlistPlayer.SetCurrentSong(0);
  return true;
}

void CPartyModeManager::SendUpdateMessage()
{
  CGUIMessage msg(GUI_MSG_PLAYLIST_CHANGED, 0, 0, 0, 0, NULL);
  m_gWindowManager.SendThreadMessage(msg);
}

void CPartyModeManager::Play(int iPos)
{
  // move current song to the top if its not there
  g_playlistPlayer.Play(iPos);
  CLog::Log(LOGINFO,"PARTY MODE MANAGER: Playing song at %i", iPos);
  Process();
}

void CPartyModeManager::OnError(int iError, CStdString& strLogMessage)
{
  // open error dialog
  CGUIDialogOK::ShowAndGetInput(257, 16030, iError, 0);
  CLog::Log(LOGERROR, "PARTY MODE MANAGER: %s", strLogMessage.c_str());
  m_bEnabled = false;
  SendUpdateMessage();
}

int CPartyModeManager::GetSongsPlayed()
{
  if (!IsEnabled())
    return -1;
  return m_iSongsPlayed;
}

int CPartyModeManager::GetMatchingSongs()
{
  if (!IsEnabled())
    return -1;
  return m_iMatchingSongs;
}

int CPartyModeManager::GetMatchingSongsPicked()
{
  if (!IsEnabled())
    return -1;
  return m_iMatchingSongsPicked;
}

int CPartyModeManager::GetMatchingSongsLeft()
{
  if (!IsEnabled())
    return -1;
  return m_iMatchingSongsLeft;
}

int CPartyModeManager::GetRelaxedSongs()
{
  if (!IsEnabled())
    return -1;
  return m_iRelaxedSongs;
}

int CPartyModeManager::GetRandomSongs()
{
  if (!IsEnabled())
    return -1;
  return m_iRandomSongs;
}

void CPartyModeManager::ClearState()
{
  m_iLastUserSong = -1;
  m_iSongsPlayed = 0;
  m_iMatchingSongs = 0;
  m_iMatchingSongsPicked = 0;
  m_iMatchingSongsLeft = 0;
  m_iRelaxedSongs = 0;
  m_iRandomSongs = 0;

  m_songsInHistory = 0;
  m_history.clear();
}

void CPartyModeManager::UpdateStats()
{
  m_iMatchingSongsLeft = m_iMatchingSongs - m_iMatchingSongsPicked;
  m_iRandomSongs = m_iMatchingSongsPicked;
  m_iRelaxedSongs = 0;  // unsupported at this stage
}

bool CPartyModeManager::AddInitialSongs(vector<long> &songIDs)
{
  CPlayList& playlist = g_playlistPlayer.GetPlaylist(PLAYLIST_MUSIC);
  int iMissingSongs = QUEUE_DEPTH - playlist.size();
  if (iMissingSongs > 0)
  {
    // generate iMissingSongs random ids from songIDs
    if (iMissingSongs > (int)songIDs.size())
      return false; // can't do it if we have less songs than we need

    vector<long> chosenSongIDs;
    GetRandomSelection(songIDs, iMissingSongs, chosenSongIDs);
    // construct the where clause
    CStdString sqlWhere = "where songview.idsong in (";
    for (vector<long>::iterator it = chosenSongIDs.begin(); it != chosenSongIDs.end(); it++)
    {
      CStdString song;
      song.Format("%i,", *it);
      sqlWhere += song;
    }
    // replace the last comma with closing bracket
    sqlWhere[sqlWhere.size() - 1] = ')';

    // add songs to fill queue
    CMusicDatabase musicdatabase;
    if (musicdatabase.Open())
    {
      CFileItemList items;
      //musicdatabase.BeginTransaction();
      if (musicdatabase.GetSongsByWhere("", sqlWhere, items))
      {
        m_history = chosenSongIDs;
    		items.Randomize(); //randomizing the initial list or they will be in database order
        for (int i = 0; i < items.Size(); i++)
        {
          Add(items[i]);
          // TODO: Allow "relaxed restrictions" later?
        }
      }
      else
      {
        //musicdatabase.CommitTransaction();
        musicdatabase.Close();
        OnError(16034, (CStdString)"Cannot get songs from database. Aborting.");
        return false;
      }
      //musicdatabase.CommitTransaction();
    }
    else
    {
      OnError(16033, (CStdString)"Party mode could not open database. Aborting.");
      return false;
    }
    musicdatabase.Close();
  }
  return true;
}

CStdString CPartyModeManager::GetWhereClauseWithHistory() const
{
  CStdString historyWhere;
  // now add this on to the normal where clause
  if (m_history.size())
  {
    if (m_strCurrentFilter.IsEmpty())
      historyWhere = "where songview.idsong not in (";
    else
      historyWhere = m_strCurrentFilter + " and songview.idsong not in (";
    for (unsigned int i = 0; i < m_history.size(); i++)
    {
      CStdString number;
      number.Format("%i,", m_history[i]);
      historyWhere += number;
    }
    historyWhere.TrimRight(",");
    historyWhere += ")";
  }
  return historyWhere;
}

void CPartyModeManager::AddToHistory(long songID)
{
  while (m_history.size() >= m_songsInHistory)
    m_history.erase(m_history.begin());
  m_history.push_back(songID);
}

void CPartyModeManager::GetRandomSelection(vector<long> &in, unsigned int number, vector<long> &out)
{
  // only works if we have < 32768 in the in vector
  srand(timeGetTime());
  for (unsigned int i = 0; i < number; i++)
  {
    int num = rand() % in.size();
    out.push_back(in[num]);
    in.erase(in.begin() + num);
  }
}