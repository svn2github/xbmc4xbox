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

#include "VideoLibrary.h"
#include "../VideoDatabase.h"
#include "../Util.h"
#include "Application.h"

using namespace Json;
using namespace JSONRPC;

JSON_STATUS CVideoLibrary::GetMovies(const CStdString &method, ITransportLayer *transport, IClient *client, const Value &parameterObject, Value &result)
{
  if (!(parameterObject.isObject() || parameterObject.isNull()))
    return InvalidParams;

  CVideoDatabase videodatabase;
  if (!videodatabase.Open())
    return InternalError;

//  int genreID = parameterObject.get("genreid", -1).asInt();

  CFileItemList items;
  if (videodatabase.GetMoviesNav("", items))
    HandleFileItemList("movieid", "movies", items, parameterObject, result);

  videodatabase.Close();
  return OK;
}

JSON_STATUS CVideoLibrary::GetTVShows(const CStdString &method, ITransportLayer *transport, IClient *client, const Value &parameterObject, Value &result)
{
  if (!(parameterObject.isObject() || parameterObject.isNull()))
    return InvalidParams;

  CVideoDatabase videodatabase;
  if (!videodatabase.Open())
    return InternalError;

//  int genreID = parameterObject.get("genreid", -1).asInt();

  CFileItemList items;
  if (videodatabase.GetTvShowsNav("", items))
    HandleFileItemList("tvshowid", "tvshows", items, parameterObject, result);

  videodatabase.Close();
  return OK;
}

JSON_STATUS CVideoLibrary::GetSeasons(const CStdString &method, ITransportLayer *transport, IClient *client, const Value &parameterObject, Value &result)
{
  if (!(parameterObject.isObject() || parameterObject.isNull()))
    return InvalidParams;

  CVideoDatabase videodatabase;
  if (!videodatabase.Open())
    return InternalError;

  const Value param = parameterObject.isObject() ? parameterObject : Value(objectValue);
  int tvshowID = param.get("tvshowid", -1).asInt();

  CFileItemList items;
  if (videodatabase.GetSeasonsNav("", items, -1, -1, -1, -1, tvshowID))
    HandleFileItemList(NULL, "seasons", items, param, result);

  videodatabase.Close();
  return OK;
}

JSON_STATUS CVideoLibrary::GetEpisodes(const CStdString &method, ITransportLayer *transport, IClient *client, const Value &parameterObject, Value &result)
{
  if (!(parameterObject.isObject() || parameterObject.isNull()))
    return InvalidParams;

  CVideoDatabase videodatabase;
  if (!videodatabase.Open())
    return InternalError;

  const Value param = parameterObject.isObject() ? parameterObject : Value(objectValue);
  int tvshowID = param.get("tvshowid", -1).asInt();
  int season = param.get("season", -1).asInt();

  CFileItemList items;
  if (videodatabase.GetEpisodesNav("", items, -1, -1, -1, -1, tvshowID, season))
    HandleFileItemList("episodeid", "episodes", items, param, result);

  videodatabase.Close();
  return OK;
}

JSON_STATUS CVideoLibrary::GetMusicVideoAlbums(const CStdString &method, ITransportLayer *transport, IClient *client, const Value &parameterObject, Value &result)
{
/*  CVideoDatabase videodatabase;
  if (!videodatabase.Open())
    return InternalError;

  int artistID = parameterObject.get("artistid", -1).asInt();
//  bool GetMusicVideosNav(const CStdString& strBaseDir, CFileItemList& items, int idGenre=-1, int idYear=-1, int idArtist=-1, int idDirector=-1, int idStudio=-1, int idAlbum=-1);
  CFileItemList items;
//  bool GetMusicVideosNav(const CStdString& strBaseDir, CFileItemList& items, int idGenre=-1, int idYear=-1, int idArtist=-1, int idDirector=-1, int idStudio=-1, int idAlbum=-1);
  if (videodatabase.GetMusicVideoAlbumsNav("", items, -1, -1, artistID, -1, -1, -1, -1))
    HandleFileItemList("albumid", "albums", items, parameterObject, result);

  videodatabase.Close();*/
  return OK;
}

JSON_STATUS CVideoLibrary::GetMusicVideos(const CStdString &method, ITransportLayer *transport, IClient *client, const Value &parameterObject, Value &result)
{
  if (!(parameterObject.isObject() || parameterObject.isNull()))
    return InvalidParams;

  CVideoDatabase videodatabase;
  if (!videodatabase.Open())
    return InternalError;

  const Value param = parameterObject.isObject() ? parameterObject : Value(objectValue);
  int artistID = param.get("artistid", -1).asInt();
  int albumID  = param.get("albumid",  -1).asInt();

  CFileItemList items;
  if (videodatabase.GetMusicVideosNav("", items, -1, -1, artistID, -1, -1, albumID))
    HandleFileItemList("musicvideoid", "musicvideos", items, param, result);

  videodatabase.Close();
  return OK;
}

JSON_STATUS CVideoLibrary::ScanForContent(const CStdString &method, ITransportLayer *transport, IClient *client, const Value &parameterObject, Value &result)
{
  g_application.getApplicationMessenger().ExecBuiltIn("updatelibrary(video)");
  return ACK;
}
