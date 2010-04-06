/*
 *      Copyright (C) 2005-2010 Team XBMC
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

#include "MusicLibrary.h"
#include "../MusicDatabase.h"
#include "../FileItem.h"
#include "../Util.h"
#include "../MusicInfoTag.h"
#include "../Song.h"
#include "Application.h"

using namespace MUSIC_INFO;
using namespace Json;
using namespace JSONRPC;

JSON_STATUS CMusicLibrary::GetArtists(const CStdString &method, ITransportLayer *transport, IClient *client, const Value &parameterObject, Value &result)
{
  if (!(parameterObject.isObject() || parameterObject.isNull()))
    return InvalidParams;

  CMusicDatabase musicdatabase;
  if (!musicdatabase.Open())
    return InternalError;

  const Value param = parameterObject.isObject() ? parameterObject : Value(objectValue);
  int genreID = param.get("genreid", -1).asInt();

  CFileItemList items;
  if (musicdatabase.GetArtistsNav("", items, genreID, false))
    HandleFileItemList("artistid", "artists", items, param, result);

  musicdatabase.Close();
  return OK;
}

JSON_STATUS CMusicLibrary::GetAlbums(const CStdString &method, ITransportLayer *transport, IClient *client, const Value &parameterObject, Value &result)
{
  if (!(parameterObject.isObject() || parameterObject.isNull()))
    return InvalidParams;

  CMusicDatabase musicdatabase;
  if (!musicdatabase.Open())
    return InternalError;

  const Value param = parameterObject.isObject() ? parameterObject : Value(objectValue);
  int artistID = param.get("artistid", -1).asInt();
  int genreID = param.get("genreid", -1).asInt();

  CFileItemList items;
  if (musicdatabase.GetAlbumsNav("", items, genreID, artistID))
    HandleFileItemList("albumid", "albums", items, param, result);

  musicdatabase.Close();
  return OK;
}

JSON_STATUS CMusicLibrary::GetSongs(const CStdString &method, ITransportLayer *transport, IClient *client, const Value &parameterObject, Value &result)
{
  if (!(parameterObject.isObject() || parameterObject.isNull()))
    return InvalidParams;

  CMusicDatabase musicdatabase;
  if (!musicdatabase.Open())
    return InternalError;

  const Value param = parameterObject.isObject() ? parameterObject : Value(objectValue);
  int artistID = param.get("artistid", -1).asInt();
  int albumID = param.get("albumid", -1).asInt();
  int genreID = param.get("genreid", -1).asInt();

  CFileItemList items;
  if (musicdatabase.GetSongsNav("", items, genreID, artistID, albumID))
    HandleFileItemList("songid", "songs", items, param, result);

  musicdatabase.Close();
  return OK;
}

JSON_STATUS CMusicLibrary::GetSongInfo(const CStdString &method, ITransportLayer *transport, IClient *client, const Value &parameterObject, Value &result)
{
  if (!parameterObject.isObject())
    return InvalidParams;
  int songID = parameterObject.get("songid", -1).asInt();

  if (songID < 0)
    return InvalidParams;

  CMusicDatabase musicdatabase;
  if (!musicdatabase.Open())
    return InternalError;

  CSong songInfo;
  if (musicdatabase.GetSongById(songID, songInfo))
  {
    if (!songInfo.strTitle.IsEmpty())
      result["title"] = songInfo.strTitle.c_str();
    if (!songInfo.strArtist.IsEmpty())
      result["artist"] = songInfo.strArtist.c_str();
    if (!songInfo.strAlbum.IsEmpty())
      result["album"] = songInfo.strAlbum.c_str();
    if (!songInfo.strAlbumArtist.IsEmpty())
      result["albumartist"] = songInfo.strAlbumArtist.c_str();
    if (!songInfo.strGenre.IsEmpty())
      result["genre"] = songInfo.strGenre.c_str();
    if (!songInfo.strThumb.IsEmpty())
      result["thumbnail"] = songInfo.strThumb.c_str();
    if (!songInfo.strComment.IsEmpty())
      result["comment"] = songInfo.strComment.c_str();

    if (songInfo.iTrack > 0)
      result["track"] = songInfo.iTrack;
    if (songInfo.iDuration > 0)
      result["duration"] = songInfo.iDuration;

    result["rating"]      = songInfo.rating;
    result["year"]        = songInfo.iYear;
    result["timesplayed"] = songInfo.iTimesPlayed;
  }

  musicdatabase.Close();
  return OK;
}

JSON_STATUS CMusicLibrary::ScanForContent(const CStdString &method, ITransportLayer *transport, IClient *client, const Value &parameterObject, Value &result)
{
  g_application.getApplicationMessenger().ExecBuiltIn("updatelibrary(music)");
  return ACK;
}
