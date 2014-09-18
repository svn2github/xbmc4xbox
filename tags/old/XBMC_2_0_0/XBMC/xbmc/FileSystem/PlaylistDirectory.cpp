#include "../stdafx.h"
#include "playlistdirectory.h"
#include "../playlistplayer.h"


CPlaylistDirectory::CPlaylistDirectory()
{

}

CPlaylistDirectory::~CPlaylistDirectory()
{

}

bool CPlaylistDirectory::GetDirectory(const CStdString& strPath, CFileItemList &items)
{
  CURL url(strPath);

  int playlistTyp=PLAYLIST_NONE;
  if (url.GetProtocol()=="playlistmusic")
    playlistTyp=PLAYLIST_MUSIC;
  else if (url.GetProtocol()=="playlistvideo")
    playlistTyp=PLAYLIST_VIDEO;

  if (playlistTyp==PLAYLIST_NONE)
    return false;

  CPlayList& playlist = g_playlistPlayer.GetPlaylist(playlistTyp);
  items.Reserve(playlist.size());

  for (int i = 0; i < playlist.size(); ++i)
  {
    CFileItem* item = new CFileItem(playlist[i]);
    items.Add(item);
  }

  return true;
}
