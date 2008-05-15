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
#include "SmartPlaylist.h"
#include "utils/log.h"
#include "StringUtils.h"
#include "FileSystem/SmartPlaylistDirectory.h"
#include "utils/CharsetConverter.h"
#include "XMLUtils.h"
#include "Database.h"
#include "VideoDatabase.h"
#include "Util.h"
#include "DateTime.h"

using namespace std;
using namespace DIRECTORY;

typedef struct
{
  char string[13];
  CSmartPlaylistRule::DATABASE_FIELD field;
  CSmartPlaylistRule::FIELD_TYPE type;
  int localizedString;
} translateField;

static const translateField fields[] = { { "none", CSmartPlaylistRule::FIELD_NONE, CSmartPlaylistRule::TEXT_FIELD, 231 }, 
                                         { "genre", CSmartPlaylistRule::FIELD_GENRE, CSmartPlaylistRule::BROWSEABLE_FIELD, 515 },
                                         { "album", CSmartPlaylistRule::SONG_ALBUM, CSmartPlaylistRule::BROWSEABLE_FIELD, 558 },
                                         { "albumartist", CSmartPlaylistRule::SONG_ALBUM_ARTIST, CSmartPlaylistRule::BROWSEABLE_FIELD, 566 },
                                         { "artist", CSmartPlaylistRule::SONG_ARTIST, CSmartPlaylistRule::BROWSEABLE_FIELD, 557 },
                                         { "title", CSmartPlaylistRule::FIELD_TITLE, CSmartPlaylistRule::TEXT_FIELD, 556 },
                                         { "year", CSmartPlaylistRule::FIELD_YEAR, CSmartPlaylistRule::NUMERIC_FIELD, 562 },
                                         { "time", CSmartPlaylistRule::FIELD_TIME, CSmartPlaylistRule::SECONDS_FIELD, 180 },
                                         { "tracknumber", CSmartPlaylistRule::SONG_TRACKNUMBER, CSmartPlaylistRule::NUMERIC_FIELD, 554 },
                                         { "filename", CSmartPlaylistRule::SONG_FILENAME, CSmartPlaylistRule::TEXT_FIELD, 561 },
                                         { "playcount", CSmartPlaylistRule::FIELD_PLAYCOUNT, CSmartPlaylistRule::NUMERIC_FIELD, 567 },
                                         { "lastplayed", CSmartPlaylistRule::SONG_LASTPLAYED, CSmartPlaylistRule::DATE_FIELD, 568 },
                                         { "rating", CSmartPlaylistRule::FIELD_RATING, CSmartPlaylistRule::NUMERIC_FIELD, 563 },
                                         { "comment", CSmartPlaylistRule::SONG_COMMENT, CSmartPlaylistRule::TEXT_FIELD, 569 },
                                         { "dateadded", CSmartPlaylistRule::FIELD_DATEADDED, CSmartPlaylistRule::DATE_FIELD, 570 },
                                         { "plot", CSmartPlaylistRule::VIDEO_PLOT, CSmartPlaylistRule::TEXT_FIELD, 207 },
                                         { "plotoutline", CSmartPlaylistRule::VIDEO_PLOTOUTLINE, CSmartPlaylistRule::TEXT_FIELD, 203 },
                                         { "tagline", CSmartPlaylistRule::VIDEO_TAGLINE, CSmartPlaylistRule::TEXT_FIELD, 202 },
                                         { "mpaarating", CSmartPlaylistRule::VIDEO_MPAA, CSmartPlaylistRule::TEXT_FIELD, 20074 },
                                         { "top250", CSmartPlaylistRule::VIDEO_TOP250, CSmartPlaylistRule::NUMERIC_FIELD, 13409 },
                                         { "status", CSmartPlaylistRule::TVSHOW_STATUS, CSmartPlaylistRule::TEXT_FIELD, 126 },
                                         { "votes", CSmartPlaylistRule::VIDEO_VOTES, CSmartPlaylistRule::TEXT_FIELD, 205 },
                                         { "director", CSmartPlaylistRule::VIDEO_DIRECTOR, CSmartPlaylistRule::BROWSEABLE_FIELD, 20339 },
                                         { "actor", CSmartPlaylistRule::VIDEO_ACTOR, CSmartPlaylistRule::BROWSEABLE_FIELD, 20337 },
                                         { "studio", CSmartPlaylistRule::VIDEO_STUDIO, CSmartPlaylistRule::BROWSEABLE_FIELD, 572 },
                                         { "numepisodes", CSmartPlaylistRule::TVSHOW_NUMEPISODES, CSmartPlaylistRule::NUMERIC_FIELD, 20360 },
                                         { "numwatched", CSmartPlaylistRule::TVSHOW_NUMWATCHED, CSmartPlaylistRule::NUMERIC_FIELD, 21441 },
                                         { "writers", CSmartPlaylistRule::VIDEO_WRITER, CSmartPlaylistRule::BROWSEABLE_FIELD, 20417 },
                                         { "airdate", CSmartPlaylistRule::EPISODE_AIRDATE, CSmartPlaylistRule::DATE_FIELD, 20416 },
                                         { "episode", CSmartPlaylistRule::EPISODE_EPISODE, CSmartPlaylistRule::NUMERIC_FIELD, 20359 },
                                         { "season", CSmartPlaylistRule::EPISODE_SEASON, CSmartPlaylistRule::NUMERIC_FIELD, 20373 },
                                         { "tvshow", CSmartPlaylistRule::FIELD_TVSHOWTITLE, CSmartPlaylistRule::BROWSEABLE_FIELD, 20364 },
                                         { "episodetitle", CSmartPlaylistRule::FIELD_EPISODETITLE, CSmartPlaylistRule::TEXT_FIELD, 21442 },
                                         { "random", CSmartPlaylistRule::FIELD_RANDOM, CSmartPlaylistRule::TEXT_FIELD, 590 },
                                         { "playlist", CSmartPlaylistRule::FIELD_PLAYLIST, CSmartPlaylistRule::PLAYLIST_FIELD, 559 }
                                       };

#define NUM_FIELDS sizeof(fields) / sizeof(translateField)

typedef struct
{
  char string[15];
  CSmartPlaylistRule::SEARCH_OPERATOR op;
  int localizedString;
} operatorField;

static const operatorField operators[] = { { "contains", CSmartPlaylistRule::OPERATOR_CONTAINS, 21400 },
                                           { "doesnotcontain", CSmartPlaylistRule::OPERATOR_DOES_NOT_CONTAIN, 21401 },
                                           { "is", CSmartPlaylistRule::OPERATOR_EQUALS, 21402 },
                                           { "isnot", CSmartPlaylistRule::OPERATOR_DOES_NOT_EQUAL, 21403 },
                                           { "startswith", CSmartPlaylistRule::OPERATOR_STARTS_WITH, 21404 },
                                           { "endswith", CSmartPlaylistRule::OPERATOR_ENDS_WITH, 21405 },
                                           { "greaterthan", CSmartPlaylistRule::OPERATOR_GREATER_THAN, 21406 },
                                           { "lessthan", CSmartPlaylistRule::OPERATOR_LESS_THAN, 21407 },
                                           { "after", CSmartPlaylistRule::OPERATOR_AFTER, 21408 },
                                           { "before", CSmartPlaylistRule::OPERATOR_BEFORE, 21409 },
                                           { "inthelast", CSmartPlaylistRule::OPERATOR_IN_THE_LAST, 21410 },
                                           { "notinthelast", CSmartPlaylistRule::OPERATOR_NOT_IN_THE_LAST, 21411 }
                                         };

#define NUM_OPERATORS sizeof(operators) / sizeof(operatorField)

CSmartPlaylistRule::CSmartPlaylistRule()
{
  m_field = FIELD_NONE;
  m_operator = OPERATOR_CONTAINS;
  m_parameter = "";
}

void CSmartPlaylistRule::TranslateStrings(const char *field, const char *oper, const char *parameter)
{
  m_field = TranslateField(field);
  m_operator = TranslateOperator(oper);
  m_parameter = parameter;
}

TiXmlElement CSmartPlaylistRule::GetAsElement()
{
  TiXmlElement rule("rule");
  TiXmlText parameter(m_parameter.c_str());
  rule.InsertEndChild(parameter);
  rule.SetAttribute("field", TranslateField(m_field).c_str());
  rule.SetAttribute("operator", TranslateOperator(m_operator).c_str());
  return rule;
}

CSmartPlaylistRule::DATABASE_FIELD CSmartPlaylistRule::TranslateField(const char *field)
{
  for (unsigned int i = 0; i < NUM_FIELDS; i++)
    if (strcmpi(field, fields[i].string) == 0) return fields[i].field;
  return FIELD_NONE;
}

CStdString CSmartPlaylistRule::TranslateField(DATABASE_FIELD field)
{
  for (unsigned int i = 0; i < NUM_FIELDS; i++)
    if (field == fields[i].field) return fields[i].string;
  return "none";
}

CSmartPlaylistRule::SEARCH_OPERATOR CSmartPlaylistRule::TranslateOperator(const char *oper)
{
  for (unsigned int i = 0; i < NUM_OPERATORS; i++)
    if (strcmpi(oper, operators[i].string) == 0) return operators[i].op;
  return OPERATOR_CONTAINS;
}

CStdString CSmartPlaylistRule::TranslateOperator(SEARCH_OPERATOR oper)
{
  for (unsigned int i = 0; i < NUM_OPERATORS; i++)
    if (oper == operators[i].op) return operators[i].string;
  return "contains";
}

CStdString CSmartPlaylistRule::GetLocalizedField(DATABASE_FIELD field)
{
  for (unsigned int i = 0; i < NUM_FIELDS; i++)
    if (field == fields[i].field) return g_localizeStrings.Get(fields[i].localizedString);
  return g_localizeStrings.Get(16018);
}

CSmartPlaylistRule::FIELD_TYPE CSmartPlaylistRule::GetFieldType(DATABASE_FIELD field)
{
  for (unsigned int i = 0; i < NUM_FIELDS; i++)
    if (field == fields[i].field) return fields[i].type;
  return TEXT_FIELD;
}

vector<CSmartPlaylistRule::DATABASE_FIELD> CSmartPlaylistRule::GetFields(const CStdString &type)
{
  vector<DATABASE_FIELD> fields;
  if (type == "music")
  {
    fields.push_back(FIELD_GENRE);
    fields.push_back(SONG_ALBUM);
    fields.push_back(SONG_ARTIST);
    fields.push_back(SONG_ALBUM_ARTIST);
    fields.push_back(FIELD_TITLE);
    fields.push_back(FIELD_YEAR);
    fields.push_back(FIELD_TIME);
    fields.push_back(SONG_TRACKNUMBER);
    fields.push_back(SONG_FILENAME);
    fields.push_back(FIELD_PLAYCOUNT);
    fields.push_back(SONG_LASTPLAYED);
    fields.push_back(FIELD_RATING);
    fields.push_back(SONG_COMMENT);
//    fields.push_back(FIELD_DATEADDED);  // no date added yet in db
  }
  else if (type == "tvshows")
  {
    fields.push_back(FIELD_TITLE);
    fields.push_back(VIDEO_PLOT);
    fields.push_back(TVSHOW_STATUS);
    fields.push_back(VIDEO_VOTES);
    fields.push_back(FIELD_RATING);
    fields.push_back(FIELD_YEAR);
    fields.push_back(FIELD_GENRE);
    fields.push_back(VIDEO_DIRECTOR);
    fields.push_back(VIDEO_ACTOR);
    fields.push_back(TVSHOW_NUMEPISODES);
    fields.push_back(TVSHOW_NUMWATCHED);
    fields.push_back(FIELD_PLAYCOUNT);
//    fields.push_back(FIELD_DATEADDED);  // no date added yet in db
  }
  else if (type == "episodes")
  {
    fields.push_back(FIELD_EPISODETITLE);
    fields.push_back(FIELD_TVSHOWTITLE);
    fields.push_back(VIDEO_PLOT);
    fields.push_back(VIDEO_VOTES);
    fields.push_back(FIELD_RATING);
    fields.push_back(FIELD_TIME);
    fields.push_back(VIDEO_WRITER);
    fields.push_back(EPISODE_AIRDATE);
    fields.push_back(FIELD_PLAYCOUNT);
    fields.push_back(FIELD_GENRE);
    fields.push_back(FIELD_YEAR); // premiered
    fields.push_back(VIDEO_DIRECTOR);
    fields.push_back(VIDEO_ACTOR);
    fields.push_back(EPISODE_EPISODE);
    fields.push_back(EPISODE_SEASON);
//    fields.push_back(FIELD_DATEADDED);  // no date added yet in db
  }
  else if (type == "movies")
  {
    fields.push_back(FIELD_TITLE);
    fields.push_back(VIDEO_PLOT);
    fields.push_back(VIDEO_PLOTOUTLINE);
    fields.push_back(VIDEO_TAGLINE);
    fields.push_back(VIDEO_VOTES);
    fields.push_back(FIELD_RATING);
    fields.push_back(FIELD_TIME);
    fields.push_back(VIDEO_WRITER);
    fields.push_back(FIELD_PLAYCOUNT);
    fields.push_back(FIELD_GENRE);
    fields.push_back(FIELD_YEAR); // premiered
    fields.push_back(VIDEO_DIRECTOR);
    fields.push_back(VIDEO_ACTOR);
    fields.push_back(VIDEO_MPAA);
    fields.push_back(VIDEO_TOP250);
    fields.push_back(VIDEO_STUDIO);
//    fields.push_back(FIELD_DATEADDED);  // no date added yet in db
  }
  else if (type == "musicvideo")
  {
    fields.push_back(FIELD_TITLE);
    fields.push_back(FIELD_GENRE);
    fields.push_back(SONG_ALBUM);
    fields.push_back(FIELD_YEAR);
    fields.push_back(SONG_ARTIST);
    fields.push_back(SONG_FILENAME);
    fields.push_back(FIELD_PLAYCOUNT);
    fields.push_back(FIELD_TIME);
    fields.push_back(VIDEO_DIRECTOR);
    fields.push_back(VIDEO_STUDIO);
    fields.push_back(VIDEO_PLOT);
//    fields.push_back(FIELD_DATEADDED);  // no date added yet in db
  }
  fields.push_back(FIELD_PLAYLIST);
  return fields;
}

CStdString CSmartPlaylistRule::GetLocalizedOperator(SEARCH_OPERATOR oper)
{
  for (unsigned int i = 0; i < NUM_OPERATORS; i++)
    if (oper == operators[i].op) return g_localizeStrings.Get(operators[i].localizedString);
  return g_localizeStrings.Get(16018);
}

CStdString CSmartPlaylistRule::GetLocalizedRule()
{
  CStdString rule;
  rule.Format("%s %s %s", GetLocalizedField(m_field).c_str(), GetLocalizedOperator(m_operator).c_str(), m_parameter.c_str());
  return rule;
}

CStdString CSmartPlaylistRule::GetWhereClause(const CStdString& strType)
{
  SEARCH_OPERATOR op = m_operator;
  if (strType == "tvshows" || strType == "episodes" && m_field == FIELD_YEAR)
  { // special case for premiered which is a date rather than a year
    // TODO: SMARTPLAYLISTS do we really need this, or should we just make this field the premiered date and request a date?
    if (op == OPERATOR_EQUALS)
      op = OPERATOR_CONTAINS;
    else if (op == OPERATOR_DOES_NOT_EQUAL)
      op = OPERATOR_DOES_NOT_CONTAIN;
  }
  // the comparison piece
  CStdString operatorString;
  switch (op)
  {
  case OPERATOR_CONTAINS:
    operatorString = " LIKE '%%%s%%'"; break;
  case OPERATOR_DOES_NOT_CONTAIN:
    operatorString = " NOT LIKE '%%%s%%'"; break;
  case OPERATOR_EQUALS:
    operatorString = " LIKE '%s'"; break;
  case OPERATOR_DOES_NOT_EQUAL:
    operatorString = " NOT LIKE '%s'"; break;
  case OPERATOR_STARTS_WITH:
    operatorString = " LIKE '%s%%'"; break;
  case OPERATOR_ENDS_WITH:
    operatorString = " LIKE '%%%s'"; break;
  case OPERATOR_AFTER:
  case OPERATOR_GREATER_THAN:
  case OPERATOR_IN_THE_LAST:
    operatorString = " > '%s'"; break;
  case OPERATOR_BEFORE:
  case OPERATOR_LESS_THAN:
  case OPERATOR_NOT_IN_THE_LAST:
    operatorString = " < '%s'"; break;
  default:
    break;
  }
  CStdString parameter = CDatabase::FormatSQL(operatorString.c_str(), m_parameter.c_str());
  if (m_field == SONG_LASTPLAYED)
  {
    if (m_operator == OPERATOR_IN_THE_LAST || m_operator == OPERATOR_NOT_IN_THE_LAST)
    { // translate time period
      CDateTime date=CDateTime::GetCurrentDateTime();
      CDateTimeSpan span;
      span.SetFromPeriod(m_parameter);
      date-=span;
      parameter = CDatabase::FormatSQL(operatorString.c_str(), date.GetAsDBDate().c_str());
    }
  }
  else if (m_field == FIELD_TIME)
  { // translate time to seconds
    CStdString seconds; seconds.Format("%i", StringUtils::TimeStringToSeconds(m_parameter));
    parameter = CDatabase::FormatSQL(operatorString.c_str(), seconds.c_str());
  }

  // now the query parameter
  CStdString query;
  if (strType == "music")
  {
    if (m_field == FIELD_GENRE)
      query = "(strGenre" + parameter + ") or (idsong IN (select idsong from genre,exgenresong where exgenresong.idgenre = genre.idgenre and genre.strGenre" + parameter + "))";
    else if (m_field == SONG_ARTIST)
      query = "(strArtist" + parameter + ") or (idsong IN (select idsong from artist,exartistsong where exartistsong.idartist = artist.idartist and artist.strArtist" + parameter + "))";
    else if (m_field == SONG_ALBUM_ARTIST)
      query = "idalbum in (select idalbum from artist,album where album.idartist=artist.idartist and artist.strArtist" + parameter + ") or idalbum in (select idalbum from artist,exartistalbum where exartistalbum.idartist = artist.idartist and artist.strArtist" + parameter + ")";
    else if (m_field == SONG_LASTPLAYED && (m_operator == OPERATOR_LESS_THAN || m_operator == OPERATOR_BEFORE || m_operator == OPERATOR_NOT_IN_THE_LAST))
      query = "lastPlayed is NULL or lastPlayed" + parameter;
  }
  else if (strType == "movie")
  {
    if (m_field == FIELD_GENRE)
      query = "idmovie in (select idmovie from genrelinkmovie join genre on genre.idgenre=genrelinkmovie.idgenre where genre.strGenre" + parameter + ")";
    else if (m_field == VIDEO_DIRECTOR)
      query = "idmovie in (select idmovie from directorlinkmovie join actors on actors.idactor=directorlinkmovie.iddirector where actors.strActor" + parameter + ")";
    else if (m_field == VIDEO_ACTOR)
      query = "idmovie in (select idmovie from actorlinkmovie join actors on actors.idactor=actorlinkmovie.idactor where actors.strActor" + parameter + ")";
    else if (m_field == VIDEO_WRITER)
      query = "idmovie in (select idmovie from writerlinkmovie join actors on actors.idactor=writerlinkmovie.idactor where actors.strActor" + parameter + ")";
    else if (m_field == VIDEO_STUDIO)
      query = "idmovie in (select idmovie from studiolinkmovie join studio on studio.idstudio=studiolinkmovie.idstudio where studio.strStudio" + parameter + ")";
  }
  else if (strType == "musicvideos")
  {
    if (m_field == FIELD_GENRE)
      query = "idmvideo in (select idmvideo from genrelinkmusicvideo join genre on genre.idgenre=genrelinkmusicvideo.idgenre where genre.strGenre" + parameter + ")";
    else if (m_field == SONG_ARTIST)
      query = "idmvideo in (select idmvideo from artistlinkmusicvideo join actor on actor.idactor=artistlinkmusicvideo.idactor where actor.strActor" + parameter + ")";
    else if (m_field == VIDEO_STUDIO)
      query = "idmvideo in (select idmvideo from studiolinkmusicvideo join studio on studio.idstudio=studiolinkmusicvideo.idstudio where studio.strStudio" + parameter + ")";
    else if (m_field == VIDEO_DIRECTOR)
      query = "idmvideo in (select idmvideo from directorlinkmusicvideo join actors on actors.idactor=directorlinkmusicvideo.iddirector where actors.strActor" + parameter + ")";
  }
  else if (strType == "tvshows")
  {
    if (m_field == FIELD_GENRE)
      query = "idshow in (select idshow from genrelinktvshow join genre on genre.idgenre=genrelinktvshow.idgenre where genre.strGenre" + parameter + ")";
    else if (m_field == VIDEO_DIRECTOR)
      query = "idshow in (select idshow from directorlinktvshow join actors on actors.idactor=directorlinktvshow.iddirector where actors.strActor" + parameter + ")";
    else if (m_field == VIDEO_ACTOR)
      query = "idshow in (select idshow from actorlinktvshow join actors on actors.idactor=actorlinktvshow.idactor where actors.strActor" + parameter + ")";
  }
  else if (strType == "episodes")
  {
    if (m_field == FIELD_GENRE)
      query = "idshow in (select idshow from genrelinktvshow join genre on genre.idgenre=genrelinktvshow.idgenre where genre.strGenre" + parameter + ")";
    else if (m_field == VIDEO_DIRECTOR)
      query = "idepisode in (select idepisode from directorlinkepisode join actors on actors.idactor=directorlinkepisode.iddirector where actors.strActor" + parameter + ")";
    else if (m_field == VIDEO_ACTOR)
      query = "idepisode in (select idepisode from actorlinkepisode join actors on actors.idactor=actorlinkepisode.idactor where actors.strActor" + parameter + ")";
    else if (m_field == VIDEO_WRITER)
      query = "idepisode in (select idepisode from writerlinkepisode join actors on actors.idactor=writerlinkepisode.idactor where actors.strActor" + parameter + ")";
  }
  if (m_field == FIELD_PLAYLIST)
  { // playlist field - grab our playlist and add to our where clause
    CStdString playlistFile = CSmartPlaylistDirectory::GetPlaylistByName(m_parameter);
    if (!playlistFile.IsEmpty())
    {
      CSmartPlaylist playlist;
      playlist.Load(playlistFile);
      CStdString playlistQuery = playlist.GetWhereClause(false);
      if (m_operator == OPERATOR_DOES_NOT_EQUAL)
        query.Format("NOT (%s)", playlistQuery.c_str());
      else if (m_operator == OPERATOR_EQUALS)
        query = playlistQuery;
    }
  }
  if (m_field == FIELD_PLAYCOUNT && strType != "music")
  { // playcount is stored as NULL or number
    if ((m_operator == OPERATOR_EQUALS && m_parameter == "0") ||
        (m_operator == OPERATOR_DOES_NOT_EQUAL && m_parameter != "0") ||
        (m_operator == OPERATOR_LESS_THAN))
    {
      CStdString field = GetDatabaseField(FIELD_PLAYCOUNT, strType);
      query = field + " is NULL or " + field + parameter;
    }
  }
  else if (query.IsEmpty() && m_field != FIELD_NONE)
    query = GetDatabaseField(m_field,strType) + parameter;
  return query;
}

CStdString CSmartPlaylistRule::GetDatabaseField(DATABASE_FIELD field, const CStdString& type)
{
  if (type == "music")
  {
    if (field == FIELD_TITLE) return "strTitle";
    else if (field == FIELD_GENRE) return "strGenre";
    else if (field == SONG_ALBUM) return "strAlbum";
    else if (field == FIELD_YEAR) return "iYear";
    else if (field == SONG_ARTIST || field == SONG_ALBUM_ARTIST) return "strArtist";
    else if (field == FIELD_TIME) return "iDuration";
    else if (field == FIELD_PLAYCOUNT) return "iTimesPlayed";
    else if (field == SONG_FILENAME) return "strFilename";
    else if (field == SONG_TRACKNUMBER) return "iTrack";
    else if (field == SONG_LASTPLAYED) return "lastplayed";
    else if (field == FIELD_RATING) return "rating";
    else if (field == SONG_COMMENT) return "comment";
    else if (field == FIELD_RANDOM) return "random()";      // only used for order clauses
    else if (field == FIELD_DATEADDED) return "idsong";     // only used for order clauses
  }
  else if (type == "movies")
  {
    CStdString result;
    if (field == FIELD_TITLE) result.Format("c%02d", VIDEODB_ID_TITLE);
    else if (field == VIDEO_PLOT) result.Format("c%02d", VIDEODB_ID_PLOT);
    else if (field == VIDEO_PLOTOUTLINE) result.Format("c%02d", VIDEODB_ID_PLOTOUTLINE);
    else if (field == VIDEO_TAGLINE) result.Format("c%02d", VIDEODB_ID_TAGLINE);
    else if (field == VIDEO_VOTES) result.Format("c%02d", VIDEODB_ID_VOTES);
    else if (field == FIELD_RATING) result.Format("c%02d", VIDEODB_ID_RATING);
    else if (field == FIELD_TIME) result.Format("c%02d", VIDEODB_ID_RUNTIME);
    else if (field == VIDEO_WRITER) result = "never_use_this";   // join required
    else if (field == FIELD_PLAYCOUNT) result.Format("c%02d", VIDEODB_ID_PLAYCOUNT);
    else if (field == FIELD_GENRE) result = "never_use_this";    // join required
    else if (field == FIELD_YEAR) result.Format("c%02d", VIDEODB_ID_YEAR);
    else if (field == VIDEO_DIRECTOR) result = "never_use_this"; // join required
    else if (field == VIDEO_ACTOR) result = "never_use_this";    // join required
    else if (field == VIDEO_MPAA) result.Format("c%02d", VIDEODB_ID_MPAA);
    else if (field == VIDEO_TOP250) result.Format("c%02d", VIDEODB_ID_TOP250);
    else if (field == VIDEO_STUDIO) result = "never_use_this";   // join required
    else if (field == FIELD_RANDOM) result = "random()";      // only used for order clauses
    else if (field == FIELD_DATEADDED) result = "idshow";       // only used for order clauses
  }
  else if (type == "musicvideos")
  {
    CStdString result;
    if (field == FIELD_TITLE) result.Format("c%02d",VIDEODB_ID_MUSICVIDEO_TITLE);
    else if (field == FIELD_GENRE) result = "never_use_this";  // join required
    else if (field == SONG_ALBUM) result.Format("c%02d",VIDEODB_ID_MUSICVIDEO_ALBUM);
    else if (field == FIELD_YEAR) result.Format("c%02d",VIDEODB_ID_MUSICVIDEO_YEAR);
    else if (field == SONG_ARTIST) result = "never_use_this";  // join required;
    else if (field == SONG_FILENAME) result = "strFilename";
    else if (field == FIELD_PLAYCOUNT) result.Format("c%02d", VIDEODB_ID_MUSICVIDEO_PLAYCOUNT);
    else if (field == FIELD_TIME) result.Format("c%02d", VIDEODB_ID_MUSICVIDEO_RUNTIME);
    else if (field == VIDEO_DIRECTOR) result = "never_use_this";   // join required
    else if (field == VIDEO_STUDIO) result = "never_use_this";     // join required
    else if (field == VIDEO_PLOT) result.Format("c%02d", VIDEODB_ID_MUSICVIDEO_PLOT);
    else if (field == FIELD_RANDOM) result = "random()";      // only used for order clauses
    else if (field == FIELD_DATEADDED) result = "idmvideo";        // only used for order clauses
    return result;
  }
  if (type == "tvshows")
  {
    CStdString result;
    if (field == FIELD_TITLE) result.Format("c%02d", VIDEODB_ID_TV_TITLE);
    else if (field == VIDEO_PLOT) result.Format("c%02d", VIDEODB_ID_TV_PLOT);
    else if (field == TVSHOW_STATUS) result.Format("c%02d", VIDEODB_ID_TV_STATUS);
    else if (field == VIDEO_VOTES) result.Format("c%02d", VIDEODB_ID_TV_VOTES);
    else if (field == FIELD_RATING) result.Format("c%02d", VIDEODB_ID_TV_RATING);
    else if (field == FIELD_YEAR) result.Format("c%02d", VIDEODB_ID_TV_PREMIERED);
    else if (field == FIELD_GENRE) result.Format("c%02d", VIDEODB_ID_TV_GENRE);
    else if (field == VIDEO_DIRECTOR) result = "never_use_this"; // join required
    else if (field == VIDEO_ACTOR) result = "never_use_this";    // join required
    else if (field == TVSHOW_NUMEPISODES) result = "totalcount";
    else if (field == TVSHOW_NUMWATCHED) result = "watchedcount";
    else if (field == FIELD_PLAYCOUNT) result = "watched";
    else if (field == FIELD_RANDOM) result = "random()";      // only used for order clauses
    else if (field == FIELD_DATEADDED) result = "idshow";       // only used for order clauses
    return result;
  }
  if (type == "episodes")
  {
    CStdString result;
    if (field == FIELD_EPISODETITLE) result.Format("c%02d", VIDEODB_ID_EPISODE_TITLE);
    else if (field == FIELD_TVSHOWTITLE) result = "strTitle";
    else if (field == VIDEO_PLOT) result.Format("c%02d", VIDEODB_ID_EPISODE_PLOT);
    else if (field == VIDEO_VOTES) result.Format("c%02d", VIDEODB_ID_EPISODE_VOTES);
    else if (field == FIELD_RATING) result.Format("c%02d", VIDEODB_ID_EPISODE_RATING);
    else if (field == FIELD_TIME) result.Format("c%02d", VIDEODB_ID_EPISODE_RUNTIME);
    else if (field == VIDEO_WRITER) result = "never_use_this";   // join required
    else if (field == EPISODE_AIRDATE) result.Format("c%02d", VIDEODB_ID_EPISODE_AIRED);
    else if (field == FIELD_PLAYCOUNT) result.Format("c%02d", VIDEODB_ID_EPISODE_PLAYCOUNT);
    else if (field == FIELD_GENRE) result = "never_use_this";    // join required
    else if (field == FIELD_YEAR) result = "premiered";
    else if (field == VIDEO_DIRECTOR) result = "never_use_this"; // join required
    else if (field == VIDEO_ACTOR) result = "never_use_this";    // join required
    else if (field == EPISODE_EPISODE) result.Format("c%02d", VIDEODB_ID_EPISODE_EPISODE);
    else if (field == EPISODE_SEASON) result.Format("c%02d", VIDEODB_ID_EPISODE_SEASON);
    else if (field == FIELD_RANDOM) result = "random()";      // only used for order clauses
    else if (field == FIELD_DATEADDED) result = "idshow";       // only used for order clauses
    return result;
  }
  
  return "";
}

CSmartPlaylist::CSmartPlaylist()
{
  m_matchAllRules = true;
  m_limit = 0;
  m_orderField = CSmartPlaylistRule::FIELD_NONE;
  m_orderAscending = true;
  m_playlistType = "music"; // sane default
}

TiXmlElement *CSmartPlaylist::OpenAndReadName(const CStdString &path)
{
  if (!m_xmlDoc.LoadFile(path))
  {
    CLog::Log(LOGERROR, "Error loading Smart playlist %s", path.c_str());
    return NULL;
  }

  TiXmlElement *root = m_xmlDoc.RootElement();
  if (!root || strcmpi(root->Value(),"smartplaylist") != 0)
  {
    CLog::Log(LOGERROR, "Error loading Smart playlist %s", path.c_str());
    return NULL;
  }
  // load the playlist type
  const char* type = root->Attribute("type");
  if (type)
    m_playlistType = type;
  // load the playlist name
  TiXmlHandle name = ((TiXmlHandle)root->FirstChild("name")).FirstChild();
  if (name.Node())
    m_playlistName = name.Node()->Value();
  else
  {
    m_playlistName = CUtil::GetTitleFromPath(path);
    if (CUtil::GetExtension(m_playlistName) == ".xsp")
      CUtil::RemoveExtension(m_playlistName);
  }
  return root;
}

bool CSmartPlaylist::Load(const CStdString &path)
{
  TiXmlElement *root = OpenAndReadName(path);
  if (!root)
    return false;

  // encoding:
  CStdString encoding;
  XMLUtils::GetEncoding(&m_xmlDoc, encoding);

  TiXmlHandle match = ((TiXmlHandle)root->FirstChild("match")).FirstChild();
  if (match.Node())
    m_matchAllRules = strcmpi(match.Node()->Value(), "all") == 0;
  // now the rules
  TiXmlElement *rule = root->FirstChildElement("rule");
  while (rule)
  {
    // format is:
    // <rule field="Genre" operator="contains">parameter</rule>
    const char *field = rule->Attribute("field");
    const char *oper = rule->Attribute("operator");
    TiXmlNode *parameter = rule->FirstChild();
    if (field && oper && parameter)
    { // valid rule
      CStdString utf8Parameter;
      if (encoding.IsEmpty()) // utf8
        utf8Parameter = parameter->Value();
      else
        g_charsetConverter.stringCharsetToUtf8(encoding, parameter->Value(), utf8Parameter);
      CSmartPlaylistRule rule;
      rule.TranslateStrings(field, oper, utf8Parameter.c_str());
      m_playlistRules.push_back(rule);
    }
    rule = rule->NextSiblingElement("rule");
  }
  // now any limits
  // format is <limit>25</limit>
  TiXmlHandle limit = ((TiXmlHandle)root->FirstChild("limit")).FirstChild();
  if (limit.Node())
    m_limit = atoi(limit.Node()->Value());
  // and order
  // format is <order direction="ascending">field</order>
  TiXmlElement *order = root->FirstChildElement("order");
  if (order && order->FirstChild())
  {
    const char *direction = order->Attribute("direction");
    if (direction)
      m_orderAscending = strcmpi(direction, "ascending") == 0;
    m_orderField = CSmartPlaylistRule::TranslateField(order->FirstChild()->Value());
  }
  return true;
}

bool CSmartPlaylist::Save(const CStdString &path)
{
  TiXmlDocument doc;
  TiXmlDeclaration decl("1.0", "UTF-8", "yes");
  doc.InsertEndChild(decl);

  TiXmlElement xmlRootElement("smartplaylist");
  xmlRootElement.SetAttribute("type",m_playlistType.c_str());
  TiXmlNode *pRoot = doc.InsertEndChild(xmlRootElement);
  if (!pRoot) return false;
  // add the <name> tag
  TiXmlText name(m_playlistName.c_str());
  TiXmlElement nodeName("name");
  nodeName.InsertEndChild(name);
  pRoot->InsertEndChild(nodeName);
  // add the <match> tag
  TiXmlText match(m_matchAllRules ? "all" : "one");
  TiXmlElement nodeMatch("match");
  nodeMatch.InsertEndChild(match);
  pRoot->InsertEndChild(nodeMatch);
  // add <rule> tags
  for (vector<CSmartPlaylistRule>::iterator it = m_playlistRules.begin(); it != m_playlistRules.end(); ++it)
  {
    pRoot->InsertEndChild((*it).GetAsElement());
  }
  // add <limit> tag
  if (m_limit)
  {
    CStdString limitFormat;
    limitFormat.Format("%i", m_limit);
    TiXmlText limit(limitFormat);
    TiXmlElement nodeLimit("limit");
    nodeLimit.InsertEndChild(limit);
    pRoot->InsertEndChild(nodeLimit);
  }
  // add <order> tag
  if (m_orderField != CSmartPlaylistRule::FIELD_NONE)
  {
    TiXmlText order(CSmartPlaylistRule::TranslateField(m_orderField).c_str());
    TiXmlElement nodeOrder("order");
    nodeOrder.SetAttribute("direction", m_orderAscending ? "ascending" : "descending");
    nodeOrder.InsertEndChild(order);
    pRoot->InsertEndChild(nodeOrder);
  }
  return doc.SaveFile(path);
}

void CSmartPlaylist::SetName(const CStdString &name)
{
  m_playlistName = name;
}

void CSmartPlaylist::SetType(const CStdString &type)
{
  m_playlistType = type;
}

void CSmartPlaylist::AddRule(const CSmartPlaylistRule &rule)
{
  m_playlistRules.push_back(rule);
}

CStdString CSmartPlaylist::GetWhereClause(bool needWhere /* = true */)
{
  CStdString rule;
  for (vector<CSmartPlaylistRule>::iterator it = m_playlistRules.begin(); it != m_playlistRules.end(); ++it)
  {
    if (it != m_playlistRules.begin())
      rule += m_matchAllRules ? " AND " : " OR ";
    else if (needWhere)
      rule += "WHERE ";
    rule += "(";
    rule += (*it).GetWhereClause(GetType());
    rule += ")";
  }
  return rule;
}

CStdString CSmartPlaylist::GetOrderClause()
{
  CStdString order;
  if (m_orderField != CSmartPlaylistRule::FIELD_NONE)
    order.Format("ORDER BY %s%s", CSmartPlaylistRule::GetDatabaseField(m_orderField,GetType()), m_orderAscending ? "" : " DESC");
  if (m_limit)
  {
    CStdString limit;
    limit.Format(" LIMIT %i", m_limit);
    order += limit;
  }
  return order;
}

const vector<CSmartPlaylistRule> &CSmartPlaylist::GetRules() const
{
  return m_playlistRules;
}

CStdString CSmartPlaylist::GetSaveLocation() const
{
  if (m_playlistType == "music" || m_playlistType == "mixed")
    return m_playlistType;
  // all others are video
  return "video";
}
