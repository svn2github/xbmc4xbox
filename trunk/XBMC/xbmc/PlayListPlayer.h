#pragma once

#include "playlist.h"
#include "IMsgTargetCallback.h"

#define PLAYLIST_NONE    -1
#define PLAYLIST_MUSIC   0
#define PLAYLIST_MUSIC_TEMP 1
#define PLAYLIST_VIDEO   2
#define PLAYLIST_VIDEO_TEMP 3

using namespace PLAYLIST;

namespace PLAYLIST
{
/*!
 \ingroup windows 
 \brief Manages playlist playing.
 */
class CPlayListPlayer : public IMsgTargetCallback
{
public:
  CPlayListPlayer(void);
  virtual ~CPlayListPlayer(void);
  virtual bool OnMessage(CGUIMessage &message);
  void PlayNext(bool bAutoPlay = false);
  void PlayPrevious();
  void Play(int iSong, bool bAutoPlay = false);
  int GetCurrentSong() const;
  int GetNextSong();
  void SetCurrentSong(int iSong);
  bool HasChanged();
  void SetCurrentPlaylist( int iPlayList );
  int GetCurrentPlaylist();
  CPlayList& GetPlaylist( int nPlayList);
  int RemoveDVDItems();
  void Reset();
  void Repeat(int iPlaylist, bool bYesNo);
  bool Repeated(int iPlaylist);
  void RepeatOne(int iPlaylist, bool bYesNo);
  bool RepeatedOne(int iPlaylist);
  void ShufflePlay(int iPlaylist, bool bYesNo);
  bool ShuffledPlay(int iPlaylist);
  bool HasPlayedFirstFile();
protected:
  int NextShuffleItem();
  bool m_bChanged;
  bool m_bPlayedFirstFile;
  int m_iCurrentSong;
  int m_iCurrentPlayList;
  CPlayList m_PlaylistMusic;
  CPlayList m_PlaylistMusicTemp;
  CPlayList m_PlaylistVideo;
  CPlayList m_PlaylistVideoTemp;
  CPlayList m_PlaylistEmpty;
  int m_iOptions;
};

};

/*!
 \ingroup windows 
 \brief Global instance of playlist player
 */
extern CPlayListPlayer g_playlistPlayer;

