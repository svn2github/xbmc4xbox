
#include "stdafx.h"
#include "PlayListPlayer.h"
#include "application.h"
#include "util.h"
#include "PartyModeManager.h"

using namespace PLAYLIST;

CPlayListPlayer g_playlistPlayer;

CPlayListPlayer::CPlayListPlayer(void)
{
  m_iCurrentSong = -1;
  m_bChanged = false;
  m_bPlayedFirstFile = false;
  m_iCurrentPlayList = PLAYLIST_NONE;
  for (int i = 0; i < 4; i++)
  {
    m_repeatState[i] = REPEAT_NONE;
    m_shuffleState[i] = false;
  }
  m_iFailedSongs = 0;
}

CPlayListPlayer::~CPlayListPlayer(void)
{
  m_PlaylistMusic.Clear();
  m_PlaylistMusicTemp.Clear();
  m_PlaylistVideo.Clear();
  m_PlaylistVideoTemp.Clear();
  m_PlaylistEmpty.Clear();
}

bool CPlayListPlayer::OnMessage(CGUIMessage &message)
{
  switch (message.GetMessage())
  {
  case GUI_MSG_PLAYBACK_STOPPED:
    {
      if (m_iCurrentPlayList != PLAYLIST_NONE)
      {
        CGUIMessage msg(GUI_MSG_PLAYLISTPLAYER_STOPPED, 0, 0, m_iCurrentPlayList, m_iCurrentSong);
        m_gWindowManager.SendThreadMessage(msg);
        Reset();
        m_iCurrentPlayList = PLAYLIST_NONE;
        return true;
      }
    }
    break;
  }

  return false;
}

int CPlayListPlayer::GetNextSong()
{
  if (m_iCurrentPlayList == PLAYLIST_NONE)
    return -1;
  CPlayList& playlist = GetPlaylist(m_iCurrentPlayList);
  if (playlist.size() <= 0)
    return -1;
  int iSong = m_iCurrentSong;

  // party mode
  if (g_partyModeManager.IsEnabled() && GetCurrentPlaylist() == PLAYLIST_MUSIC)
  {
    // if we skipped ahead, go back to the top of the list
    if (iSong != 0)
      return 0;
    else
      return 1;
  }

  // if repeat one, keep playing the current song if its valid
  if (RepeatedOne(m_iCurrentPlayList))
  {
    // otherwise immediately abort playback
    if (playlist[m_iCurrentSong].IsUnPlayable())
    {
      CLog::Log(LOGERROR,"Playlist Player: RepeatOne stuck on unplayable item: %i, path [%s]", m_iCurrentSong, playlist[m_iCurrentSong].m_strPath.c_str());
      CGUIMessage msg(GUI_MSG_PLAYLISTPLAYER_STOPPED, 0, 0, m_iCurrentPlayList, m_iCurrentSong);
      m_gWindowManager.SendThreadMessage(msg);
      Reset();
      m_iCurrentPlayList = PLAYLIST_NONE;
      return -1;
    }
    return iSong;
  }

  // if random, get the next random song
  if (IsShuffled(m_iCurrentPlayList) && playlist.size() > 1)
  {
    while (iSong == m_iCurrentSong)
      iSong = NextShuffleItem();
  }
  // if not random, increment the song
  else
  {
    iSong++;

    // if we've gone beyond the playlist and repeat all is enabled,
    // then we clear played status and wrap around
    if (iSong >= playlist.size() && Repeated(m_iCurrentPlayList))
    {
      playlist.ClearPlayed();
      iSong = 0;
    }
  }

  return iSong;
}

/// \brief Play next entry in current playlist
void CPlayListPlayer::PlayNext(bool bAutoPlay)
{
  int iSong = GetNextSong();
  CPlayList& playlist = GetPlaylist(m_iCurrentPlayList);

  // stop playing
  if ((iSong < 0) || (iSong >= playlist.size()) || (playlist.GetPlayable() <= 0))
  {
    CGUIMessage msg(GUI_MSG_PLAYLISTPLAYER_STOPPED, 0, 0, m_iCurrentPlayList, m_iCurrentSong);
    m_gWindowManager.SendThreadMessage(msg);
    Reset();
    m_iCurrentPlayList = PLAYLIST_NONE;
    return;
  }

  if (bAutoPlay)
  {
    const CPlayList::CPlayListItem& item = playlist[iSong];
    if ( item.IsShoutCast() )
    {
      return ;
    }
  }
  Play(iSong, bAutoPlay);
  //g_partyModeManager.OnSongChange();
}

/// \brief Play previous entry in current playlist
void CPlayListPlayer::PlayPrevious()
{
  if (m_iCurrentPlayList == PLAYLIST_NONE)
    return ;

  CPlayList& playlist = GetPlaylist(m_iCurrentPlayList);
  if (playlist.size() <= 0) return ;
  int iSong = m_iCurrentSong;

  if (IsShuffled(m_iCurrentPlayList))
  {
    while (iSong == m_iCurrentSong)
      iSong = PreviousShuffleItem();
  }
  else
  {
    if (!RepeatedOne(m_iCurrentPlayList))
      iSong--;
  }

  if (iSong < 0)
    iSong = playlist.size() - 1;

  Play(iSong, false, true);

}

void CPlayListPlayer::Play()
{
  if (m_iCurrentPlayList == PLAYLIST_NONE)
    return;

  CPlayList& playlist = GetPlaylist(m_iCurrentPlayList);
  if (playlist.size() <= 0) return;

  int iSong = 0;
  if (IsShuffled(m_iCurrentPlayList))
    iSong = NextShuffleItem();

  Play(iSong);
}

/// \brief Start playing entry \e iSong in current playlist
/// \param iSong Song in playlist
void CPlayListPlayer::Play(int iSong, bool bAutoPlay /* = false */, bool bPlayPrevious /* = false */)
{
  if (m_iCurrentPlayList == PLAYLIST_NONE)
    return ;

  CPlayList& playlist = GetPlaylist(m_iCurrentPlayList);
  if (playlist.size() <= 0) return ;
  if (iSong < 0) iSong = 0;
  if (iSong >= playlist.size()) iSong = playlist.size() - 1;

  m_bChanged = true;
  int iPreviousSong = m_iCurrentSong;
  m_iCurrentSong = iSong;
  const CPlayList::CPlayListItem& item = playlist[m_iCurrentSong];
  playlist.SetPlayed(m_iCurrentSong);

  if (!g_application.PlayFile(item, bAutoPlay))
  {
    CLog::Log(LOGERROR,"Playlist Player: skipping unplayable item: %i, path [%s]", m_iCurrentSong, item.m_strPath.c_str());
    playlist.SetUnPlayable(m_iCurrentSong);

    // abort on 100 failed CONSECTUTIVE songs
    m_iFailedSongs++;
    if (m_iFailedSongs >= 100)
    {
      CLog::Log(LOGDEBUG,"Playlist Player: too many consecutive failures... aborting playback");

      // open error dialog
      CGUIDialogOK::ShowAndGetInput(257, 16026, 16027, 0);

      CGUIMessage msg(GUI_MSG_PLAYLISTPLAYER_STOPPED, 0, 0, m_iCurrentPlayList, m_iCurrentSong);
      m_gWindowManager.SendThreadMessage(msg);
      Reset();
      GetPlaylist(m_iCurrentPlayList).Clear();
      m_iCurrentPlayList = PLAYLIST_NONE;
      m_iFailedSongs = 0;
      return;
    }

    // how many playable items are in the playlist?
    if (playlist.GetPlayable() > 0)
    {
      if (bPlayPrevious)
        PlayPrevious();
      else
        PlayNext();
      return;
    }
    // none? then abort playback
    else
    {
      CLog::Log(LOGDEBUG,"Playlist Player: no more playable items... aborting playback");
      CGUIMessage msg(GUI_MSG_PLAYLISTPLAYER_STOPPED, 0, 0, m_iCurrentPlayList, m_iCurrentSong);
      m_gWindowManager.SendThreadMessage(msg);
      Reset();
      m_iCurrentPlayList = PLAYLIST_NONE;
      return;
    }
  }

  m_bPlayedFirstFile = true;

  // consecutive error counter so reset if the current item is playing
  m_iFailedSongs = 0;

  if (!item.IsShoutCast())
  {
    if (iPreviousSong < 0)
    {
      CGUIMessage msg(GUI_MSG_PLAYLISTPLAYER_STARTED, 0, 0, m_iCurrentPlayList, m_iCurrentSong, (LPVOID)&item);
      m_gWindowManager.SendThreadMessage( msg );
    }
    else
    {
      CGUIMessage msg(GUI_MSG_PLAYLISTPLAYER_CHANGED, 0, 0, m_iCurrentPlayList, MAKELONG(m_iCurrentSong, iPreviousSong), (LPVOID)&item);
      m_gWindowManager.SendThreadMessage(msg);
    }
  }
}

/// \brief Change the current song in playlistplayer.
/// \param iSong Song in playlist
void CPlayListPlayer::SetCurrentSong(int iSong)
{
  if (iSong >= -1 && iSong < GetPlaylist(m_iCurrentPlayList).size())
    m_iCurrentSong = iSong;
}

/// \brief Returns to current song in active playlist.
/// \return Current song
int CPlayListPlayer::GetCurrentSong() const
{
  return m_iCurrentSong;
}

bool CPlayListPlayer::HasChanged()
{
  bool bResult = m_bChanged;
  m_bChanged = false;
  return bResult;
}

/// \brief Returns the active playlist.
/// \return Active playlist \n
/// Return values can be: \n
/// - PLAYLIST_NONE \n No playlist active
/// - PLAYLIST_MUSIC \n Playlist from music playlist window
/// - PLAYLIST_MUSIC_TEMP \n Playlist started in a normal music window
/// - PLAYLIST_VIDEO \n Playlist from music playlist window
/// - PLAYLIST_VIDEO_TEMP \n Playlist started in a normal video window
int CPlayListPlayer::GetCurrentPlaylist()
{
  return m_iCurrentPlayList;
}

/// \brief Set active playlist.
/// \param iPlayList Playlist to set active \n
/// Values can be: \n
/// - PLAYLIST_NONE \n No playlist active
/// - PLAYLIST_MUSIC \n Playlist from music playlist window
/// - PLAYLIST_MUSIC_TEMP \n Playlist started in a normal music window
/// - PLAYLIST_VIDEO \n Playlist from music playlist window
/// - PLAYLIST_VIDEO_TEMP \n Playlist started in a normal video window
void CPlayListPlayer::SetCurrentPlaylist(int iPlayList)
{
  if (iPlayList == m_iCurrentPlayList)
    return;

  // changing the current playlist while party mode is on
  // disables party mode
  if (g_partyModeManager.IsEnabled())
    g_partyModeManager.Disable();

  m_iCurrentPlayList = iPlayList;
  m_bPlayedFirstFile = false;
  m_bChanged = true;

  // reset the played items
  GetPlaylist(m_iCurrentPlayList).ClearPlayed();
}

void CPlayListPlayer::ClearPlaylist(int iPlayList)
{
  // clear our applications playlist file
  g_application.m_strPlayListFile.Empty();

  CPlayList& playlist = GetPlaylist(iPlayList);
  playlist.Clear();

  // if clearing temp playlists, then reset options
  if (iPlayList == PLAYLIST_MUSIC_TEMP || iPlayList == PLAYLIST_VIDEO_TEMP)
  {
    m_shuffleState[iPlayList] = false;
    m_repeatState[iPlayList] = REPEAT_NONE;
    
    // restore repeat for music temp
    if (iPlayList == PLAYLIST_MUSIC_TEMP && g_guiSettings.GetBool("musicfiles.repeat"))
      m_repeatState[iPlayList] = REPEAT_ALL;
  }

  // its likely that the playlist changed
  CGUIMessage msg(GUI_MSG_PLAYLIST_CHANGED, 0, 0, 0, 0, NULL);
  m_gWindowManager.SendMessage(msg);
}

/// \brief Get the playlist object specified in \e nPlayList
/// \param nPlayList Values can be: \n
/// - PLAYLIST_MUSIC \n Playlist from music playlist window
/// - PLAYLIST_MUSIC_TEMP \n Playlist started in a normal music window
/// - PLAYLIST_VIDEO \n Playlist from music playlist window
/// - PLAYLIST_VIDEO_TEMP \n Playlist started in a normal video window
/// \return A reference to the CPlayList object.
CPlayList& CPlayListPlayer::GetPlaylist(int nPlayList)
{
  switch ( nPlayList )
  {
  case PLAYLIST_MUSIC:
    return m_PlaylistMusic;
    break;
  case PLAYLIST_MUSIC_TEMP:
    return m_PlaylistMusicTemp;
    break;
  case PLAYLIST_VIDEO:
    return m_PlaylistVideo;
    break;
  case PLAYLIST_VIDEO_TEMP:
    return m_PlaylistVideoTemp;
    break;
  default:
    m_PlaylistEmpty.Clear();
    return m_PlaylistEmpty;
    break;
  }
}

/// \brief Removes any item from all playlists located on a removable share
/// \return Number of items removed from PLAYLIST_MUSIC and PLAYLIST_VIDEO
int CPlayListPlayer::RemoveDVDItems()
{
  int nRemovedM = m_PlaylistMusic.RemoveDVDItems();
  m_PlaylistMusicTemp.RemoveDVDItems();
  int nRemovedV = m_PlaylistVideo.RemoveDVDItems();
  m_PlaylistVideoTemp.RemoveDVDItems();

  return nRemovedM + nRemovedV;
}

/// \brief Resets the playlistplayer, but the active playlist stays the same.
void CPlayListPlayer::Reset()
{
  m_iCurrentSong = -1;
  m_bPlayedFirstFile = false;

  // reset the played items
  GetPlaylist(m_iCurrentPlayList).ClearPlayed();

  // its likely that the playlist changed
  CGUIMessage msg(GUI_MSG_PLAYLIST_CHANGED, 0, 0, 0, 0, NULL);
  m_gWindowManager.SendMessage(msg);
}

/// \brief Whether or not something has been played yet or not from the current playlist.
bool CPlayListPlayer::HasPlayedFirstFile()
{
  return m_bPlayedFirstFile;
}

/// \brief Returns \e true if iPlaylist is repeated
/// \param iPlaylist Playlist to be asked
bool CPlayListPlayer::Repeated(int iPlaylist)
{
  if (iPlaylist >= PLAYLIST_MUSIC && iPlaylist <= PLAYLIST_VIDEO_TEMP)
    return (m_repeatState[iPlaylist] == REPEAT_ALL);
  return false;
}

/// \brief Returns \e true if iPlaylist repeats one song
/// \param iPlaylist Playlist to be asked
bool CPlayListPlayer::RepeatedOne(int iPlaylist)
{
  if (iPlaylist >= PLAYLIST_MUSIC && iPlaylist <= PLAYLIST_VIDEO_TEMP)
    return (m_repeatState[iPlaylist] == REPEAT_ONE);
  return false;
}

/// \brief Shuffle play the current playlist
/// \param bYesNo To Enable shuffle play, set to \e true
void CPlayListPlayer::SetShuffle(int iPlaylist, bool bYesNo)
{
  if (iPlaylist < PLAYLIST_MUSIC || iPlaylist > PLAYLIST_VIDEO_TEMP)
    return;

  // disable shuffle in party mode
  if (g_partyModeManager.IsEnabled() && iPlaylist == PLAYLIST_MUSIC)
  {
    m_shuffleState[iPlaylist] = false;
    return;
  }

  bool bTest = IsShuffled(iPlaylist);

  m_shuffleState[iPlaylist] = bYesNo;

  if (bTest != bYesNo)
    GetPlaylist(iPlaylist).ClearPlayed();
}

bool CPlayListPlayer::IsShuffled(int iPlaylist)
{
  if (iPlaylist >= PLAYLIST_MUSIC && iPlaylist <= PLAYLIST_VIDEO_TEMP)
    return m_shuffleState[iPlaylist];
  return false;
}

/*
// this is a true randomly picked up unplayed song, 
// but it must always traverse the entire list
// and use memory to build the vector of unplayed items
int CPlayListPlayer::NextShuffleItem()
{
  srand( timeGetTime() );

  CPlayList& playlist = GetPlaylist(m_iCurrentPlayList);

  // if theres no more unplayed songs, but the playlist is in repeat mode,
  // clear the played status
  if ((playlist.GetUnplayed() <= 0) && (Repeated(m_iCurrentPlayList)))
    playlist.ClearPlayed();
  
  if (playlist.GetUnplayed() <= 0)
    return -1;

  vector <int> vecUnplayedItems;
  CStdString strList = "(";
  for (int i = 0; i < playlist.size(); i++)
  {
    if (!playlist[i].WasPlayed())
    {
      vecUnplayedItems.push_back(i);
      CStdString strTemp;
      strTemp.Format("%i,",i);
      strList += strTemp;
    }
  }
  strList.TrimRight(",");
  strList += ")";
  CLog::Log(LOGDEBUG,"CPlayListPlayer::NextShuffleItem(), unplayed list = %s",strList.c_str());

  int nItemCount = vecUnplayedItems.size();
  int iItem = vecUnplayedItems[rand() % nItemCount];
  CLog::Log(LOGDEBUG,"CPlayListPlayer::NextShuffleItem(), next song = %i",iItem);
  return iItem;
}
*/

// this works reasonably well and uses less processing and memory
int CPlayListPlayer::NextShuffleItem()
{
  srand( timeGetTime() );

  CPlayList& playlist = GetPlaylist(m_iCurrentPlayList);

  // if theres no more unplayed songs, but the playlist is in repeat mode,
  // clear the played status
  if ((playlist.GetUnplayed() <= 0) && (Repeated(m_iCurrentPlayList)))
    playlist.ClearPlayed();
  
  if ((playlist.GetUnplayed() <= 0) || (playlist.GetPlayable() <= 0))
    return -1;

  // pick random number
  int iNumIterations = 0;
  int iRandom = rand() % playlist.GetUnplayed();
  while (playlist[iRandom].WasPlayed() || playlist[iRandom].IsUnPlayable())
  {
    // sanity check
    if (iNumIterations > playlist.size())
      return -1;

    //CLog::Log(LOGDEBUG,"CPlayListPlayer::NextShuffleItem(), skipping song at %i. WasPlayed: %s. IsUnPlayable: %s", iRandom, playlist[iRandom].WasPlayed() ? "true" : "false", playlist[iRandom].IsUnPlayable() ? "true" : "false");
    // increment by one until an unplayed song is found
    iRandom++;

    // circle around if past the last song
    if (iRandom >= playlist.size())
      iRandom = 0;

    iNumIterations++;
  }
  CLog::Log(LOGDEBUG,"CPlayListPlayer::NextShuffleItem(), return = %i",iRandom);

  return iRandom;
}

int CPlayListPlayer::PreviousShuffleItem()
{
  srand( timeGetTime() );

  CPlayList& playlist = GetPlaylist(m_iCurrentPlayList);

  // need more than one played song for previous to work!
  int iPlayed = playlist.size() - playlist.GetUnplayed();
  if (iPlayed <= 1)
    return -1;
  
  // pick random number
  int iNumIterations = 0;
  int iRandom = rand() % iPlayed;
  while (iRandom == m_iCurrentSong || !playlist[iRandom].WasPlayed())
  {
    // sanity check
    if (iNumIterations > playlist.size())
      return -1;

    //CLog::Log(LOGDEBUG,"CPlayListPlayer::PreviousShuffleItem(), skipping unplayed song at %i",iRandom);
    // increment by one until a played song is found
    iRandom++;

    // circle around if past the last song
    if (iRandom >= playlist.size())
      iRandom = 0;

    iNumIterations++;
  }
  //CLog::Log(LOGDEBUG,"CPlayListPlayer::PreviousShuffleItem(), return = %i",iRandom);

  return iRandom;
}

void CPlayListPlayer::SetRepeat(int iPlaylist, REPEAT_STATE state)
{
  if (iPlaylist < PLAYLIST_MUSIC || iPlaylist > PLAYLIST_VIDEO_TEMP)
    return;

  // disable repeat in party mode
  if (g_partyModeManager.IsEnabled() && iPlaylist == PLAYLIST_MUSIC)
    state = REPEAT_NONE;

  m_repeatState[iPlaylist] = state;
}

REPEAT_STATE CPlayListPlayer::GetRepeat(int iPlaylist)
{
  if (iPlaylist >= PLAYLIST_MUSIC && iPlaylist <= PLAYLIST_VIDEO_TEMP)
    return m_repeatState[iPlaylist];
  return REPEAT_NONE;
}