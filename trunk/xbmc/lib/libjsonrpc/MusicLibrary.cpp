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

  const Value param = ForceObject(parameterObject);
  if (!ParameterIntOrNull(param, "genreid"))
    return InvalidParams;

  CMusicDatabase musicdatabase;
  if (!musicdatabase.Open())
    return InternalError;

  int genreID = ParameterAsInt(param, -1, "genreid");

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

  const Value param = ForceObject(parameterObject);
  if (!(ParameterIntOrNull(param, "artistid") || ParameterIntOrNull(param, "albumid")))
    return InvalidParams;

  CMusicDatabase musicdatabase;
  if (!musicdatabase.Open())
    return InternalError;

  int artistID = ParameterAsInt(param, -1, "artistid");
  int genreID  = ParameterAsInt(param, -1, "genreid");

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

  const Value param = ForceObject(parameterObject);
  if (!(ParameterIntOrNull(param, "artistid") || ParameterIntOrNull(param, "albumid") || ParameterIntOrNull(param, "genreid")))
    return InvalidParams;

  CMusicDatabase musicdatabase;
  if (!musicdatabase.Open())
    return InternalError;

  int artistID = ParameterAsInt(param, -1, "artistid");
  int albumID  = ParameterAsInt(param, -1, "albumid");
  int genreID  = ParameterAsInt(param, -1, "genreid");

  CFileItemList items;
  if (musicdatabase.GetSongsNav("", items, genreID, artistID, albumID))
    HandleFileItemList("songid", "songs", items, param, result);

  musicdatabase.Close();
  return OK;
}

JSON_STATUS CMusicLibrary::ScanForContent(const CStdString &method, ITransportLayer *transport, IClient *client, const Value &parameterObject, Value &result)
{
  g_application.getApplicationMessenger().ExecBuiltIn("updatelibrary(music)");
  return ACK;
}
