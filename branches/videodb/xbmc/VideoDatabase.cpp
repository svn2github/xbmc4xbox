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
#include "videodatabase.h"
#include "GUIWindowVideoBase.h"
#include "utils/fstrcmp.h"
#include "utils/RegExp.h"
#include "util.h"
#include "GUIPassword.h"
#include "filesystem/VirtualPathDirectory.h"
#include "filesystem/StackDirectory.h"

using namespace XFILE;
using namespace DIRECTORY;

#define VIDEO_DATABASE_VERSION 3
#define VIDEO_DATABASE_OLD_VERSION 3.f
#define VIDEO_DATABASE_NAME "MyVideos34.db"

CBookmark::CBookmark()
{
  timeInSeconds = 0.0f;
  type = STANDARD;
}

//********************************************************************************************************************************
CVideoDatabase::CVideoDatabase(void)
{
  m_preV2version=VIDEO_DATABASE_OLD_VERSION;
  m_version = VIDEO_DATABASE_VERSION;
  m_strDatabaseFile=VIDEO_DATABASE_NAME;
}

//********************************************************************************************************************************
CVideoDatabase::~CVideoDatabase(void)
{}

//********************************************************************************************************************************
bool CVideoDatabase::CreateTables()
{
  /* indexes should be added on any columns that are used in in  */ 
  /* a where or a join. primary key on a column is the same as a */ 
  /* unique index on that column, so there is no need to add any */
  /* index if no other columns are refered                       */

  /* order of indexes are important, for an index to be considered all  */
  /* columns up to the column in question have to have been specified   */
  /* select * from actorlinkmovie where idMovie = 1, can not take       */
  /* advantage of a index that has been created on ( idGenre, idMovie ) */
  /*, hower on on ( idMovie, idGenre ) will be considered for use       */

  try
  {
    CDatabase::CreateTables();

    CLog::Log(LOGINFO, "create bookmark table");
    m_pDS->exec("CREATE TABLE bookmark ( idBookmark integer primary key, idFile integer, timeInSeconds double, thumbNailImage text, player text, playerState text, type integer)\n");

    CLog::Log(LOGINFO, "create settings table");
    m_pDS->exec("CREATE TABLE settings ( idFile integer, Interleaved bool, NoCache bool, Deinterlace bool, FilmGrain integer,"
                "ViewMode integer,ZoomAmount float, PixelRatio float, AudioStream integer, SubtitleStream integer,"
                "SubtitleDelay float, SubtitlesOn bool, Brightness integer, Contrast integer, Gamma integer,"
                "VolumeAmplification float, AudioDelay float, OutputToAllSpeakers bool, ResumeTime integer, Crop bool, CropLeft integer,"
                "CropRight integer, CropTop integer, CropBottom integer)\n");
    m_pDS->exec("CREATE UNIQUE INDEX ix_settings ON settings ( idFile )\n");

    CLog::Log(LOGINFO, "create stacktimes table");
    m_pDS->exec("CREATE TABLE stacktimes (idFile integer, times text)\n");
    m_pDS->exec("CREATE UNIQUE INDEX ix_stacktimes ON stacktimes ( idFile )\n");

    CLog::Log(LOGINFO, "create genre table");
    m_pDS->exec("CREATE TABLE genre ( idGenre integer primary key, strGenre text)\n");

    CLog::Log(LOGINFO, "create genrelinkmovie table");
    m_pDS->exec("CREATE TABLE genrelinkmovie ( idGenre integer, idMovie integer)\n");
    m_pDS->exec("CREATE UNIQUE INDEX ix_genrelinkmovie_1 ON genrelinkmovie ( idGenre, idMovie)\n");
    m_pDS->exec("CREATE UNIQUE INDEX ix_genrelinkmovie_2 ON genrelinkmovie ( idMovie, idGenre)\n");

    CLog::Log(LOGINFO, "create movie table");
    CStdString columns = "CREATE TABLE movie ( idMovie integer primary key";
    for (int i = 0; i < VIDEODB_MAX_COLUMNS; i++)
    {
      CStdString column;
      column.Format(",c%02d text", i);
      columns += column;
    }
    columns += ")";
    m_pDS->exec(columns.c_str());

    CLog::Log(LOGINFO, "create actorlinkmovie table");
    m_pDS->exec("CREATE TABLE actorlinkmovie ( idActor integer, idMovie integer, strRole text)\n");
    m_pDS->exec("CREATE UNIQUE INDEX ix_actorlinkmovie_1 ON actorlinkmovie ( idActor, idMovie )\n");
    m_pDS->exec("CREATE UNIQUE INDEX ix_actorlinkmovie_2 ON actorlinkmovie ( idMovie, idActor )\n");

    CLog::Log(LOGINFO, "create directorlinkmovie table");
    m_pDS->exec("CREATE TABLE directorlinkmovie ( idDirector integer, idMovie integer)\n");
    m_pDS->exec("CREATE UNIQUE INDEX ix_directorlinkmovie_1 ON directorlinkmovie ( idDirector, idMovie )\n");
    m_pDS->exec("CREATE UNIQUE INDEX ix_directorlinkmovie_2 ON directorlinkmovie ( idMovie, idDirector )\n");

    CLog::Log(LOGINFO, "create actors table");
    m_pDS->exec("CREATE TABLE actors ( idActor integer primary key, strActor text )\n");

    CLog::Log(LOGINFO, "create movielinkfile table");
    m_pDS->exec("CREATE TABLE movielinkfile (idMovie integer, idFile integer)\n");
    m_pDS->exec("CREATE UNIQUE INDEX ix_movielinkfile_1 ON movielinkfile ( idMovie, idFile )\n");
    m_pDS->exec("CREATE UNIQUE INDEX ix_movielinkfile_2 ON movielinkfile ( idFile, idMovie )\n");

    CLog::Log(LOGINFO, "create path table");
    m_pDS->exec("CREATE TABLE path ( idPath integer primary key, strPath text, strContent text, strScraper text)\n");
    m_pDS->exec("CREATE UNIQUE INDEX ix_path ON path ( strPath )\n");

    CLog::Log(LOGINFO, "create files table");
    m_pDS->exec("CREATE TABLE files ( idFile integer primary key, idPath integer, strFilename text, idMovie integer, idEpisode integer)\n");
    m_pDS->exec("CREATE UNIQUE INDEX ix_files ON files ( idPath, strFilename )\n");

    CLog::Log(LOGINFO, "create tvshow table");
    columns = "CREATE TABLE tvshow ( idShow integer";
    for (int i = 0; i < VIDEODB_MAX_COLUMNS; i++)
    {
      CStdString column;
      column.Format(",c%02d text", i);
      columns += column;
    }
    columns += ")";
    m_pDS->exec(columns.c_str());

    CLog::Log(LOGINFO, "create directorlinktvshow table");
    m_pDS->exec("CREATE TABLE directorlinktvshow ( idDirector integer, idShow integer)\n");
    m_pDS->exec("CREATE UNIQUE INDEX ix_directorlinktvshow_1 ON directorlinktvshow ( idDirector, idShow )\n");
    m_pDS->exec("CREATE UNIQUE INDEX ix_directorlinktvshow_2 ON directorlinktvshow ( idShow, idDirector )\n");

    CLog::Log(LOGINFO, "create actorlinktvshow table");
    m_pDS->exec("CREATE TABLE actorlinktvshow ( idActor integer, idShow integer, strRole text)\n");
    m_pDS->exec("CREATE UNIQUE INDEX ix_actorlinktvshow_1 ON actorlinktvshow ( idActor, idShow )\n");
    m_pDS->exec("CREATE UNIQUE INDEX ix_actorlinktvshow_2 ON actorlinktvshow ( idShow, idActor )\n");
  
    CLog::Log(LOGINFO, "create episode table");
    columns = "CREATE TABLE episode ( idEpisode integer";
    for (int i = 0; i < VIDEODB_MAX_COLUMNS; i++)
    {
      CStdString column;
      column.Format(",c%02d text", i);
      columns += column;
    }
    columns += ")";
    m_pDS->exec(columns.c_str());

    CLog::Log(LOGINFO, "create tvshowlinkepisode table");
    m_pDS->exec("CREATE TABLE tvshowlinkepisode ( idShow integer, idEpisode integer)\n");

    CLog::Log(LOGINFO, "create tvshowlinkpath table");
    m_pDS->exec("CREATE TABLE tvshowlinkpath (idShow integer, idPath integer)\n");
    m_pDS->exec("CREATE UNIQUE INDEX ix_tvshowlinkpath_1 ON tvshowlinkpath ( idShow, idPath )\n");
    m_pDS->exec("CREATE UNIQUE INDEX ix_tvshowlinkpath_2 ON tvshowlinkpath ( idPath, idShow )\n");

    CLog::Log(LOGINFO, "create actorlinkepisode table");
    m_pDS->exec("CREATE TABLE actorlinkepisode ( idActor integer, idEpisode integer, strRole text)\n");
    m_pDS->exec("CREATE UNIQUE INDEX ix_actorlinkepisode_1 ON actorlinkepisode ( idActor, idEpisode )\n");
    m_pDS->exec("CREATE UNIQUE INDEX ix_actorlinkepisode_2 ON actorlinkepisode ( idEpisode, idActor )\n");

    CLog::Log(LOGINFO, "create directorlinkepisode table");
    m_pDS->exec("CREATE TABLE directorlinkepisode ( idDirector integer, idEpisode integer, strRole text)\n");
    m_pDS->exec("CREATE UNIQUE INDEX ix_directorlinkepisode_1 ON directorlinkepisode ( idDirector, idEpisode )\n");
    m_pDS->exec("CREATE UNIQUE INDEX ix_directorlinkepisode_2 ON directorlinkepisode ( idEpisode, idDirector )\n");

    CLog::Log(LOGINFO, "create genrelinktvshow table");
    m_pDS->exec("CREATE TABLE genrelinktvshow ( idGenre integer, idShow integer)\n");
    m_pDS->exec("CREATE UNIQUE INDEX ix_genrelinktvshow_1 ON genrelinktvshow ( idGenre, idShow)\n");
    m_pDS->exec("CREATE UNIQUE INDEX ix_genrelinktvshow_2 ON genrelinktvshow ( idShow, idGenre)\n");

    CLog::Log(LOGINFO, "create genrelinkepisode table");
    m_pDS->exec("CREATE TABLE genrelinkepisode ( idGenre integer, idEpisode integer)\n");
    m_pDS->exec("CREATE UNIQUE INDEX ix_genrelinkepisode_1 ON genrelinkepisode ( idGenre, idEpisode)\n");
    m_pDS->exec("CREATE UNIQUE INDEX ix_genrelinkepisode_2 ON genrelinkepisode ( idEpisode, idGenre)\n");
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "videodatabase::unable to create tables:%i", GetLastError());
    return false;
  }

  return true;
}

//********************************************************************************************************************************
long CVideoDatabase::GetPath(const CStdString& strPath)
{
  CStdString strSQL;
  try
  {
    long lPathId=-1;
    if (NULL == m_pDB.get()) return -1;
    if (NULL == m_pDS.get()) return -1;
    
    CStdString strPath1(strPath);
    CUtil::RemoveSlashAtEnd(strPath1);

    strSQL=FormatSQL("select idPath from path where strPath like '%s'",strPath1.c_str());
    m_pDS->query(strSQL.c_str());
    if (!m_pDS->eof())
      lPathId = m_pDS->fv("path.idPath").get_asLong();

    return lPathId;
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "videodatabase:unable to addpath (%s)", strSQL.c_str());
  }
  return -1;
}

long CVideoDatabase::AddPath(const CStdString& strPath)
{
  CStdString strSQL;
  try
  {
    long lPathId;
    if (NULL == m_pDB.get()) return -1;
    if (NULL == m_pDS.get()) return -1;

    CStdString strPath1(strPath);
    CUtil::RemoveSlashAtEnd(strPath1);

    strSQL=FormatSQL("insert into path (idPath, strPath, strContent, strScraper) values (NULL,'%s','','')", strPath1.c_str());
    m_pDS->exec(strSQL.c_str());
    lPathId = (long)sqlite3_last_insert_rowid( m_pDB->getHandle() );
    return lPathId;
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "videodatabase:unable to addpath (%s)", strSQL.c_str());
  }
  return -1;
}


//********************************************************************************************************************************
long CVideoDatabase::AddFile(const CStdString& strFileNameAndPath)
{
  CStdString strSQL = "";
  try
  {
    long lFileId;
    if (NULL == m_pDB.get()) return -1;
    if (NULL == m_pDS.get()) return -1;

    CStdString strFileName, strPath;
    CUtil::Split(strFileNameAndPath,strPath, strFileName);
    long lPathId=GetPath(strPath);
    if (lPathId < 0)
      lPathId = AddPath(strPath);

    if (lPathId < 0)
      return -1;

    CStdString strSQL=FormatSQL("select idFile from files where strFileName like '%s' and idPath=%u", strFileName.c_str(),lPathId);

    m_pDS->query(strSQL.c_str());
    if (m_pDS->num_rows() > 0)
    {
      lFileId = m_pDS->fv("idFile").get_asLong() ;
      m_pDS->close();
      return lFileId;
    }
    m_pDS->close();
    strSQL=FormatSQL("insert into files (idFile,idPath,strFileName,idMovie,idEpisode) values(NULL, %u, '%s',-1,-1)", lPathId,strFileName.c_str());
    m_pDS->exec(strSQL.c_str());
    lFileId = (long)sqlite3_last_insert_rowid( m_pDB->getHandle() );
    return lFileId;
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "videodatabase:unable to addfile (%s)", strSQL.c_str());
  }
  return -1;
}


//********************************************************************************************************************************
long CVideoDatabase::GetFile(const CStdString& strFilenameAndPath, long& lMovieId, long& lEpisodeId, bool bExact)
{
  try
  {
    if (NULL == m_pDB.get()) return -1;
    if (NULL == m_pDS.get()) return -1;
    CStdString strPath, strFileName ;
    CUtil::Split(strFilenameAndPath, strPath, strFileName);
    long lPathId = GetPath(strPath);
    if (lPathId < 0)
      return -1;

    CStdString strSQL;
    strSQL=FormatSQL("select idFile,idMovie,idEpisode from files where strFileName like '%s' and idPath=%u", strFileName.c_str(),lPathId);
    m_pDS->query(strSQL.c_str());
    if (m_pDS->num_rows() > 0)
    {
      long lFileId = m_pDS->fv("files.idFile").get_asLong();
      lMovieId = m_pDS->fv("files.idMovie").get_asLong();
      lEpisodeId = m_pDS->fv("files.idEpisode").get_asLong();
      m_pDS->close();
      return lFileId;
    }
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "CVideoDatabase::GetFile(%s) failed", strFilenameAndPath.c_str());
  }
  return -1;
}

//********************************************************************************************************************************

long CVideoDatabase::GetMovie(const CStdString& strFilenameAndPath)
{
  long lMovieId, lEpisodeId;
  if (GetFile(strFilenameAndPath, lMovieId, lEpisodeId) < 0)
  {
    return -1;
  }
  return lMovieId;
}

long CVideoDatabase::GetTvShow(const CStdString& strFilenameAndPath)
{
  return GetTvShowInfo(strFilenameAndPath);
}

long CVideoDatabase::GetEpisode(const CStdString& strFilenameAndPath)
{
  long lMovieId, lEpisodeId;
  if (GetFile(strFilenameAndPath, lMovieId, lEpisodeId) < 0)
  {
    return -1;
  }
  return lEpisodeId;
}

long CVideoDatabase::GetMovieInfo(const CStdString& strFilenameAndPath)
{
  try
  {
    if (NULL == m_pDB.get()) return -1;
    if (NULL == m_pDS.get()) return -1;
    long lMovieId = -1;

    // needed for query parameters
    CStdString strPath, strFile;
    CUtil::Split(strFilenameAndPath, strPath, strFile);

    // have to join movieinfo table for correct results
    long lPathId = GetPath(strPath);
    if (lPathId < 0 && strPath != strFilenameAndPath)
      return -1;

    CStdString strSQL;
    if (strPath == strFilenameAndPath) // i.e. we where handed a path, we may have rarred items in it
    {
      if (lPathId == -1)
      {
        strSQL=FormatSQL("select files.idMovie from files, path where files.idpath = path.idpath and path.strPath like '%%%s%%'",strPath.c_str());
        m_pDS->query(strSQL.c_str());
        if (m_pDS->eof())
        {
          CUtil::URLEncode(strPath);
          strSQL=FormatSQL("select files.idMovie from files, path where files.idpath = path.idpath and path.strPath like '%%%s%%'",strPath.c_str());
        }
      }
      else
      {
        strSQL=FormatSQL("select idMovie from files where files.idpath = %u",lPathId);
        m_pDS->query(strSQL.c_str());
        if (m_pDS->num_rows() > 0)
          lMovieId = m_pDS->fv("files.idMovie").get_asLong();  
        if (m_pDS->eof() || lMovieId == -1)
        {
          strSQL=FormatSQL("select files.idMovie from files, path where files.idpath = path.idpath and path.strPath like '%%%s%%'",strPath.c_str());
          m_pDS->query(strSQL.c_str());
          if (m_pDS->eof())
          {
            CUtil::URLEncode(strPath);
            strSQL=FormatSQL("select files.idMovie from files, path where files.idpath = path.idpath and path.strPath like '%%%s%%'",strPath.c_str());
          }
        }
      }
    }
    else
      strSQL=FormatSQL("select idMovie from files where strFileName like '%s' and idPath=%i", strFile.c_str(),lPathId);
    
    CLog::Log(LOGDEBUG,"CVideoDatabase::GetMovieInfo(%s), query = %s", strFilenameAndPath.c_str(), strSQL.c_str());
    m_pDS->query(strSQL.c_str());
    if (m_pDS->num_rows() > 0)
      lMovieId = m_pDS->fv("files.idMovie").get_asLong();  
    m_pDS->close();

    return lMovieId;
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "CVideoDatabase::GetMovieInfo(%s) failed", strFilenameAndPath.c_str());
  }
  return -1;
}

long CVideoDatabase::GetTvShowInfo(const CStdString& strFilenameAndPath)
{
  try
  {
    if (NULL == m_pDB.get()) return -1;
    if (NULL == m_pDS.get()) return -1;
    long lTvShowId = -1;

    // needed for query parameters
    CStdString strPath, strFile;
    CUtil::Split(strFilenameAndPath, strPath, strFile);

    // have to join movieinfo table for correct results
    long lPathId = GetPath(strPath);
    if (lPathId < 0 && strPath != strFilenameAndPath)
      return -1;

    CStdString strSQL;
    if (strPath == strFilenameAndPath) // i.e. we where handed a path, we may have rarred items in it
    {
      CStdString strPath1=strPath;
      CStdString strParent;
      int iFound=0;

      if (lPathId != -1)
      {
        strSQL=FormatSQL("select tvshow.idshow from tvshow,tvshowlinkpath where tvshowlinkpath.idPath=%u and tvshow.idshow=tvshowlinkpath.idshow",lPathId);
        m_pDS->query(strSQL);
        if (!m_pDS->eof())
          iFound = 2;
      }
      while (iFound == 0 && CUtil::GetParentPath(strPath1, strParent))
      {
        CUtil::RemoveSlashAtEnd(strParent);
        strSQL=FormatSQL("select tvshowlinkpath.idShow from path,tvshowlinkpath where tvshowlinkpath.idpath = path.idpath and strPath like '%s'",strParent.c_str());
        m_pDS->query(strSQL.c_str());
        if (!m_pDS->eof())
        {
          long idShow = m_pDS->fv("tvshowlinkpath.idshow").get_asLong();
          if (idShow != -1)
          {
            strSQL=FormatSQL("select tvshow.idshow from tvshow where idShow=%i",idShow);
            iFound = 2;
          }
        }
        strPath1 = strParent;
      }
    }
    else
      strSQL=FormatSQL("select tvshow.idshow from files,tvshow,tvshowlinkepisode where files.idepisode = tvshowlinkepisode.idepisode and tvshowlinkepisode.idshow = tvshow.idshow and files.strFileName like '%s' and files.idPath=%i", strFile.c_str(),lPathId);
    
    CLog::Log(LOGDEBUG,"CVideoDatabase::GetTvShowInfo(%s), query = %s", strFilenameAndPath.c_str(), strSQL.c_str());
    m_pDS->query(strSQL.c_str());
    if (m_pDS->num_rows() > 0)
      lTvShowId = m_pDS->fv("tvshow.idShow").get_asLong();  
    m_pDS->close();

    return lTvShowId;
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "CVideoDatabase::GetTvShowInfo(%s) failed", strFilenameAndPath.c_str());
  }
  return -1;
}

long CVideoDatabase::GetEpisodeInfo(const CStdString& strFilenameAndPath)
{
  try
  {
    if (NULL == m_pDB.get()) return -1;
    if (NULL == m_pDS.get()) return -1;
    long lEpisodeId = -1;

    // needed for query parameters
    CStdString strPath, strFile;
    CUtil::Split(strFilenameAndPath, strPath, strFile);

    // have to join movieinfo table for correct results
    long lPathId = GetPath(strPath);
    if (lPathId < 0 && strPath != strFilenameAndPath)
      return -1;

    CStdString strSQL;
    if (strPath == strFilenameAndPath) // i.e. we where handed a path, we may have rarred items in it
    {
      if (lPathId == -1)
      {
        strSQL=FormatSQL("select files.idEpisode from files, path where files.idpath = path.idpath and path.strPath like '%%%s%%'",strPath.c_str());
        m_pDS->query(strSQL.c_str());
        if (m_pDS->eof())
        {
          CUtil::URLEncode(strPath);
          strSQL=FormatSQL("select files.idEpisode from files, path where files.idpath = path.idpath and path.strPath like '%%%s%%'",strPath.c_str());
        }
      }
      else
      {
        strSQL=FormatSQL("select idEpisode from files where files.idpath = %u",lPathId);
        m_pDS->query(strSQL.c_str());
        if (m_pDS->num_rows() > 0)
          lEpisodeId = m_pDS->fv("files.idEpisode").get_asLong();  
        if (m_pDS->eof() || lEpisodeId == -1)
        {
          strSQL=FormatSQL("select files.idEpisode from files, path where files.idpath = path.idpath and path.strPath like '%%%s%%'",strPath.c_str());
          m_pDS->query(strSQL.c_str());
          if (m_pDS->eof())
          {
            CUtil::URLEncode(strPath);
            strSQL=FormatSQL("select files.idEpisode from files, path where files.idpath = path.idpath and path.strPath like '%%%s%%'",strPath.c_str());
          }
        }
      }
    }
    else
      strSQL=FormatSQL("select idEpisode from files where strFileName like '%s' and idPath=%i", strFile.c_str(),lPathId);
    
    CLog::Log(LOGDEBUG,"CVideoDatabase::GetEpisodeInfo(%s), query = %s", strFilenameAndPath.c_str(), strSQL.c_str());
    m_pDS->query(strSQL.c_str());
    if (m_pDS->num_rows() > 0)
      lEpisodeId = m_pDS->fv("files.idEpisode").get_asLong();  
    m_pDS->close();

    return lEpisodeId;
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "CVideoDatabase::GetEpisodeInfo(%s) failed", strFilenameAndPath.c_str());
  }
  return -1;
}

int CVideoDatabase::GetRecentMovies(long* pMovieIdArray, int nSize)
{
  int count = 0;

  try
  {
    if (NULL == m_pDB.get())
      return -1;

    if (NULL == m_pDS.get())
      return -1;

    CStdString strSQL=FormatSQL("select idMovie from movie order by idMovie desc limit %d", nSize);
    if (m_pDS->query(strSQL.c_str()))
    {
      while ((!m_pDS->eof()) && (count < nSize))
      {
        pMovieIdArray[count++] = m_pDS->fv("idMovie").get_asLong() ;
        m_pDS->next();
      }
      m_pDS->close();
    }
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "CVideoDatabase::GetRecentMovies failed.");
  }

  return count;
}

//********************************************************************************************************************************
long CVideoDatabase::AddMovie(const CStdString& strFilenameAndPath)
{
  try
  {
    if (NULL == m_pDB.get()) return -1;
    if (NULL == m_pDS.get()) return -1;
    
    long lFileId, lMovieId=-1,lEpisodeId=-1;
    lFileId = GetFile(strFilenameAndPath,lMovieId,lEpisodeId);
    if (lFileId < 0)
      lFileId = AddFile(strFilenameAndPath);
    if (lMovieId < 0)
    {
      CStdString strSQL=FormatSQL("insert into movie (idMovie) values (NULL)");
      m_pDS->exec(strSQL.c_str());
      lMovieId = (long)sqlite3_last_insert_rowid(m_pDB->getHandle());
      
      // update file info to reflect it points to this movie
      CStdString strPath, strFileName;
      CUtil::Split(strFilenameAndPath,strPath,strFileName);
      long lPathId = GetPath(strPath);
      if (lPathId < 0)
        lPathId = AddPath(strPath);
      strSQL=FormatSQL("update files set idMovie=%i,idEpisode=-1 where strFilename like '%s' and idPath=%u",lMovieId,strFileName.c_str(),lPathId);
      m_pDS->exec(strSQL.c_str());
      CommitTransaction();
    }
    
    return lMovieId;
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "CVideoDatabase::AddMovie(%s) failed", strFilenameAndPath.c_str());
  }
  return -1;
}

long CVideoDatabase::AddTvShow(const CStdString& strPath)
{
  try
  {
    if (NULL == m_pDB.get()) return -1;
    if (NULL == m_pDS.get()) return -1;
    
    CStdString strSQL=FormatSQL("select tvshowlinkpath.idShow from path,tvshowlinkpath where path.strPath like '%s' and path.idPath = tvshowlinkpath.idPath",strPath.c_str());
    m_pDS->query(strSQL.c_str());
    if (m_pDS->num_rows() != 0)
      return m_pDS->fv("tvshowlinkpath.idShow").get_asLong();

    strSQL=FormatSQL("insert into tvshow (idShow) values (0)");
    m_pDS->exec(strSQL.c_str());
    long lTvShow = (long)sqlite3_last_insert_rowid(m_pDB->getHandle());
    strSQL=FormatSQL("update tvshow set idShow=%i where idShow=0",lTvShow);
    m_pDS->exec(strSQL.c_str());
    
    long lPathId = GetPath(strPath);
    if (lPathId < 0)
      lPathId = AddPath(strPath);
    strSQL=FormatSQL("insert into tvshowlinkpath values (%i,%i)",lTvShow,lPathId);
    m_pDS->exec(strSQL.c_str());

    CommitTransaction();
    
    return lTvShow;
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "CVideoDatabase::AddTvShow(%s) failed", strPath.c_str());
  }
  return -1;
}

//********************************************************************************************************************************
long CVideoDatabase::AddEpisode(long idShow, const CStdString& strFilenameAndPath)
{
  try
  {
    if (NULL == m_pDB.get()) return -1;
    if (NULL == m_pDS.get()) return -1;
    
    long lFileId, lMovieId=-1,lEpisodeId=-1;
    lFileId = GetFile(strFilenameAndPath,lMovieId,lEpisodeId);
    if (lFileId < 0)
      lFileId = AddFile(strFilenameAndPath);
    if (lEpisodeId < 0)
    {
      CStdString strSQL=FormatSQL("insert into episode (idEpisode) values (0)");
      m_pDS->exec(strSQL.c_str());
      lEpisodeId = (long)sqlite3_last_insert_rowid(m_pDB->getHandle());
      strSQL=FormatSQL("update episode set idEpisode=%i where idEpisode=0",lEpisodeId);
      m_pDS->exec(strSQL.c_str());
      
      // update file info to reflect it points to this episode
      CStdString strPath, strFileName;
      CUtil::Split(strFilenameAndPath,strPath,strFileName);
      long lPathId = GetPath(strPath);
      if (lPathId < 0)
        lPathId = AddPath(strPath);
      strSQL=FormatSQL("update files set idMovie=-1,idEpisode=%i where strFilename like '%s' and idPath=%u",lEpisodeId,strFileName.c_str(),lPathId);
      m_pDS->exec(strSQL.c_str());
      strSQL=FormatSQL("insert into tvshowlinkepisode (idShow,idEpisode) values (%i,%i)",idShow,lEpisodeId);
      m_pDS->exec(strSQL.c_str());
      // and update the show
      strSQL=FormatSQL("update tvshow set c%02d=c%02d+1 where idshow=%u",VIDEODB_ID_TV_EPISODES,VIDEODB_ID_TV_EPISODES,idShow);
      m_pDS->exec(strSQL.c_str());
      CommitTransaction();
    }
    
    return lEpisodeId;
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "CVideoDatabase::AddEpisode(%s) failed", strFilenameAndPath.c_str());
  }
  return -1;
}

//********************************************************************************************************************************
long CVideoDatabase::AddGenre(const CStdString& strGenre)
{
  try
  {
    if (NULL == m_pDB.get()) return -1;
    if (NULL == m_pDS.get()) return -1;

    CStdString strSQL=FormatSQL("select idGenre from genre where strGenre like '%s'", strGenre.c_str());
    m_pDS->query(strSQL.c_str());
    if (m_pDS->num_rows() == 0)
    {
      m_pDS->close();
      // doesnt exists, add it
      strSQL=FormatSQL("insert into genre (idGenre, strGenre) values( NULL, '%s')", strGenre.c_str());
      m_pDS->exec(strSQL.c_str());
      long lGenreId = (long)sqlite3_last_insert_rowid(m_pDB->getHandle());
      return lGenreId;
    }
    else
    {
      const field_value value = m_pDS->fv("idGenre");
      long lGenreId = value.get_asLong() ;
      m_pDS->close();
      return lGenreId;
    }
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "CVideoDatabase::AddGenre(%s) failed", strGenre.c_str() );
  }

  return -1;
}


//********************************************************************************************************************************
long CVideoDatabase::AddActor(const CStdString& strActor)
{
  try
  {
    if (NULL == m_pDB.get()) return -1;
    if (NULL == m_pDS.get()) return -1;
    CStdString strSQL=FormatSQL("select idActor from Actors where strActor like '%s'", strActor.c_str());
    m_pDS->query(strSQL.c_str());
    if (m_pDS->num_rows() == 0)
    {
      m_pDS->close();
      // doesnt exists, add it
      strSQL=FormatSQL("insert into Actors (idActor, strActor) values( NULL, '%s')", strActor.c_str());
      m_pDS->exec(strSQL.c_str());
      long lActorId = (long)sqlite3_last_insert_rowid(m_pDB->getHandle());
      return lActorId;
    }
    else
    {
      const field_value value = m_pDS->fv("idActor");
      long lActorId = value.get_asLong() ;
      m_pDS->close();
      return lActorId;
    }

  }
  catch (...)
  {
    CLog::Log(LOGERROR, "CVideoDatabase::AddActor(%s) failed", strActor.c_str() );
  }
  return -1;
}
//********************************************************************************************************************************
void CVideoDatabase::AddGenreToMovie(long lMovieId, long lGenreId)
{
  try
  {
    if (NULL == m_pDB.get()) return ;
    if (NULL == m_pDS.get()) return ;

    CStdString strSQL=FormatSQL("select * from genrelinkmovie where idGenre=%i and idMovie=%i", lGenreId, lMovieId);
    m_pDS->query(strSQL.c_str());
    if (m_pDS->num_rows() == 0)
    {
      // doesnt exists, add it
      strSQL=FormatSQL("insert into genrelinkmovie (idGenre, idMovie) values( %i,%i)", lGenreId, lMovieId);
      m_pDS->exec(strSQL.c_str());
    }
    m_pDS->close();
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "CVideoDatabase::AddGenreToMovie() failed");
  }
}

void CVideoDatabase::AddGenreToTvShow(long lTvShowId, long lGenreId)
{
  try
  {
    if (NULL == m_pDB.get()) return ;
    if (NULL == m_pDS.get()) return ;

    CStdString strSQL=FormatSQL("select * from genrelinktvshow where idGenre=%i and idShow=%i", lGenreId, lTvShowId);
    m_pDS->query(strSQL.c_str());
    if (m_pDS->num_rows() == 0)
    {
      // doesnt exists, add it
      strSQL=FormatSQL("insert into genrelinktvshow (idGenre, idShow) values( %i,%i)", lGenreId, lTvShowId);
      m_pDS->exec(strSQL.c_str());
    }
    m_pDS->close();
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "CVideoDatabase::AddGenreToTvShow() failed");
  }
}

void CVideoDatabase::AddGenreToEpisode(long lEpisodeId, long lGenreId)
{
  try
  {
    if (NULL == m_pDB.get()) return ;
    if (NULL == m_pDS.get()) return ;

    CStdString strSQL=FormatSQL("select * from genrelinkepisode where idGenre=%i and idEpisode=%i", lGenreId, lEpisodeId);
    m_pDS->query(strSQL.c_str());
    if (m_pDS->num_rows() == 0)
    {
      // doesnt exists, add it
      strSQL=FormatSQL("insert into genrelinkepisode (idGenre, idEpisode) values( %i,%i)", lGenreId, lEpisodeId);
      m_pDS->exec(strSQL.c_str());
    }
    m_pDS->close();
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "CVideoDatabase::AddGenreToEpisode() failed");
  }
}

//********************************************************************************************************************************
void CVideoDatabase::AddActorToMovie(long lMovieId, long lActorId, const CStdString& strRole)
{
  try
  {
    if (NULL == m_pDB.get()) return ;
    if (NULL == m_pDS.get()) return ;

    CStdString strSQL=FormatSQL("select * from actorlinkmovie where idActor=%u and idMovie=%u", lActorId, lMovieId);
    m_pDS->query(strSQL.c_str());
    if (m_pDS->num_rows() == 0)
    {
      // doesnt exists, add it
      strSQL=FormatSQL("insert into actorlinkmovie (idActor, idMovie, strRole) values( %i,%i,'%s')", lActorId, lMovieId, strRole.c_str());
      m_pDS->exec(strSQL.c_str());
    }
    m_pDS->close();
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "CVideoDatabase::AddActorToMovie() failed");
  }
}

void CVideoDatabase::AddActorToTvShow(long lTvShowId, long lActorId, const CStdString& strRole)
{
  try
  {
    if (NULL == m_pDB.get()) return ;
    if (NULL == m_pDS.get()) return ;

    CStdString strSQL=FormatSQL("select * from actorlinktvshow where idActor=%u and idShow=%u", lActorId, lTvShowId);
    m_pDS->query(strSQL.c_str());
    if (m_pDS->num_rows() == 0)
    {
      // doesnt exists, add it
      strSQL=FormatSQL("insert into actorlinktvshow (idActor, idShow, strRole) values( %i,%i,'%s')", lActorId, lTvShowId, strRole.c_str());
      m_pDS->exec(strSQL.c_str());
    }
    m_pDS->close();
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "CVideoDatabase::AddActorToTvShow() failed");
  }
}

void CVideoDatabase::AddActorToEpisode(long lEpisodeId, long lActorId, const CStdString& strRole)
{
  try
  {
    if (NULL == m_pDB.get()) return ;
    if (NULL == m_pDS.get()) return ;

    CStdString strSQL=FormatSQL("select * from actorlinkepisode where idActor=%u and idEpisode=%u", lActorId, lEpisodeId);
    m_pDS->query(strSQL.c_str());
    if (m_pDS->num_rows() == 0)
    {
      // doesnt exists, add it
      strSQL=FormatSQL("insert into actorlinkepisode (idActor, idEpisode, strRole) values( %i,%i,'%s')", lActorId, lEpisodeId, strRole.c_str());
      m_pDS->exec(strSQL.c_str());
    }
    m_pDS->close();
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "CVideoDatabase::AddActorToEpisode() failed");
  }
}

//********************************************************************************************************************************
void CVideoDatabase::AddDirectorToMovie(long lMovieId, long lDirectorId)
{
  try
  {
    if (NULL == m_pDB.get()) return ;
    if (NULL == m_pDS.get()) return ;

    CStdString strSQL=FormatSQL("select * from directorlinkmovie where idDirector=%u and idMovie=%u", lDirectorId, lMovieId);
    m_pDS->query(strSQL.c_str());
    if (m_pDS->num_rows() == 0)
    {
      // doesnt exists, add it
      strSQL=FormatSQL("insert into directorlinkmovie (idDirector, idMovie) values( %i,%i)", lDirectorId, lMovieId);
      m_pDS->exec(strSQL.c_str());
    }
    m_pDS->close();
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "CVideoDatabase::AddDirectorToMovie() failed");
  }
}

void CVideoDatabase::AddDirectorToTvShow(long lTvShowId, long lDirectorId)
{
  try
  {
    if (NULL == m_pDB.get()) return ;
    if (NULL == m_pDS.get()) return ;

    CStdString strSQL=FormatSQL("select * from directorlinktvshow where idDirector=%u and idShow=%u", lDirectorId, lTvShowId);
    m_pDS->query(strSQL.c_str());
    if (m_pDS->num_rows() == 0)
    {
      // doesnt exists, add it
      strSQL=FormatSQL("insert into directorlinktvshow (idDirector, idShow) values( %i,%i)", lDirectorId, lTvShowId);
      m_pDS->exec(strSQL.c_str());
    }
    m_pDS->close();
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "CVideoDatabase::AddDirectorToTvShow() failed");
  }
}

void CVideoDatabase::AddDirectorToEpisode(long lEpisodeId, long lDirectorId)
{
  try
  {
    if (NULL == m_pDB.get()) return ;
    if (NULL == m_pDS.get()) return ;

    CStdString strSQL=FormatSQL("select * from directorlinkepisode where idDirector=%u and idEpisode=%u", lDirectorId, lEpisodeId);
    m_pDS->query(strSQL.c_str());
    if (m_pDS->num_rows() == 0)
    {
      // doesnt exists, add it
      strSQL=FormatSQL("insert into directorlinkepisode (idDirector, idEpisode) values( %i,%i)", lDirectorId, lEpisodeId);
      m_pDS->exec(strSQL.c_str());
    }
    m_pDS->close();
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "CVideoDatabase::AddDirectorToEpisode() failed");
  }
}

//********************************************************************************************************************************
bool CVideoDatabase::HasMovieInfo(const CStdString& strFilenameAndPath)
{
  try
  {
    if (NULL == m_pDB.get()) return false;
    if (NULL == m_pDS.get()) return false;
    long lMovieId = GetMovieInfo(strFilenameAndPath);
    if ( lMovieId < 0) return false;

    CStdString strSQL=FormatSQL("select idMovie from movie where movie.idmovie=%i", lMovieId);
    m_pDS->query(strSQL.c_str());
    if (m_pDS->num_rows() == 0)
    {
      m_pDS->close();
      return false;
    }
    m_pDS->close();
    return true;
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "CVideoDatabase::HasMovieInfo(%s) failed", strFilenameAndPath.c_str());
  }

  return false;
}

bool CVideoDatabase::HasTvShowInfo(const CStdString& strFilenameAndPath)
{
  try
  {
    if (NULL == m_pDB.get()) return false;
    if (NULL == m_pDS.get()) return false;
    long lTvShowId = GetTvShowInfo(strFilenameAndPath);
    if ( lTvShowId < 0) return false;
    return true;
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "CVideoDatabase::HasTvShowInfo(%s) failed", strFilenameAndPath.c_str());
  }

  return false;
}

bool CVideoDatabase::HasEpisodeInfo(const CStdString& strFilenameAndPath)
{
  try
  {
    if (NULL == m_pDB.get()) return false;
    if (NULL == m_pDS.get()) return false;
    long lEpisodeId = GetEpisode(strFilenameAndPath);
    if ( lEpisodeId < 0) return false;
    return true;
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "CVideoDatabase::HasEpisodeInfo(%s) failed", strFilenameAndPath.c_str());
  }

  return false;
}

//********************************************************************************************************************************
void CVideoDatabase::DeleteDetailsForMovie(const CStdString& strFileNameAndPath)
{
  try
  {
    if (NULL == m_pDB.get()) return ;
    if (NULL == m_pDS.get()) return ;

    long lMovieId = GetMovie(strFileNameAndPath);
    if ( lMovieId < 0) return ;

    CStdString strSQL;
    strSQL=FormatSQL("delete from genrelinkmovie where idmovie=%i", lMovieId);
    m_pDS->exec(strSQL.c_str());

    strSQL=FormatSQL("delete from actorlinkmovie where idmovie=%i", lMovieId);
    m_pDS->exec(strSQL.c_str());

    strSQL=FormatSQL("delete from directorlinkmovie where idmovie=%i", lMovieId);
    m_pDS->exec(strSQL.c_str());

    // remove all info other than the id
    // we do this due to the way we have the link between the file + movie tables.

    // IMO it may be better to link from the movie->file table instead of vice-versa
    // (ie have movie.idFile and movie.idPath instead of file.idMovie)
    // then when we remove from the movie table, we don't care if it's still in the file table
    // Alternatively, we could remove from the file table as well (I don't see any point in keeping it there, unless
    // we're planning on having stuff linked to the file table for settings or whatever).  In fact,
    // the file table is pretty useless as is, except for the link to the path table.  Shouldn't the movie
    // table be the base, as movie->file is a 1:1 mapping?  In this case, there's no reason why the filename can be
    // directly in the movie table, and idPath in the movie table as well.
    strSQL = "update movie set ";
    for (int iType = VIDEODB_ID_MIN + 1; iType < VIDEODB_ID_MAX; iType++)
    {
      CStdString column;
      column.Format("c%02d=NULL,", iType);
      strSQL += column;
    }
    strSQL = strSQL.Mid(0, strSQL.size() - 1) + FormatSQL(" where idMovie=%i", lMovieId);
    m_pDS->exec(strSQL.c_str());
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "CVideoDatabase::DeleteDetailsForMovie(%s) failed", strFileNameAndPath.c_str());
  }
}

void CVideoDatabase::DeleteDetailsForTvShow(const CStdString& strPath)
{
  try
  {
    if (NULL == m_pDB.get()) return ;
    if (NULL == m_pDS.get()) return ;

    long lTvShowId = GetTvShowInfo(strPath);
    if ( lTvShowId < 0) return ;

    CStdString strSQL;
    strSQL=FormatSQL("delete from genrelinktvshow where idshow=%i", lTvShowId);
    m_pDS->exec(strSQL.c_str());

    strSQL=FormatSQL("delete from actorlinktvshow where idshow=%i", lTvShowId);
    m_pDS->exec(strSQL.c_str());

    strSQL=FormatSQL("delete from directorlinktvshow where idshow=%i", lTvShowId);
    m_pDS->exec(strSQL.c_str());

    // remove all info other than the id
    // we do this due to the way we have the link between the file + movie tables.

    strSQL = "update tvshow set ";
    for (int iType = VIDEODB_ID_TV_MIN + 1; iType < VIDEODB_ID_TV_MAX; iType++)
    {
      CStdString column;
      column.Format("c%02d=NULL,", iType);
      strSQL += column;
    }
    strSQL = strSQL.Mid(0, strSQL.size() - 1) + FormatSQL(" where idShow=%i", lTvShowId);
    m_pDS->exec(strSQL.c_str());
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "CVideoDatabase::DeleteDetailsForTvShow(%s) failed", strPath.c_str());
  }
}

void CVideoDatabase::DeleteDetailsForEpisode(const CStdString& strFilenameAndPath)
{
  try
  {
    if (NULL == m_pDB.get()) return ;
    if (NULL == m_pDS.get()) return ;

    long lEpisodeId = GetEpisode(strFilenameAndPath);
    if ( lEpisodeId < 0) return ;

    CStdString strSQL;
    strSQL=FormatSQL("delete from genrelinkepisode where idepisode=%i", lEpisodeId);
    m_pDS->exec(strSQL.c_str());

    strSQL=FormatSQL("delete from actorlinkepisode where idepisode=%i", lEpisodeId);
    m_pDS->exec(strSQL.c_str());

    strSQL=FormatSQL("delete from directorlinkepisode where idepisode=%i", lEpisodeId);
    m_pDS->exec(strSQL.c_str());

    // remove all info other than the id
    // we do this due to the way we have the link between the file + movie tables.

    strSQL = "update episode set ";
    for (int iType = VIDEODB_ID_EPISODE_MIN + 1; iType < VIDEODB_ID_EPISODE_MAX; iType++)
    {
      CStdString column;
      column.Format("c%02d=NULL,", iType);
      strSQL += column;
    }
    strSQL = strSQL.Mid(0, strSQL.size() - 1) + FormatSQL(" where idEpisode=%i", lEpisodeId);
    m_pDS->exec(strSQL.c_str());
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "CVideoDatabase::DeleteDetailsForEpisode(%s) failed", strFilenameAndPath.c_str());
  }
}

//********************************************************************************************************************************
void CVideoDatabase::GetMoviesByActor(const CStdString& strActor, VECMOVIES& movies)
{
  try
  {
    movies.erase(movies.begin(), movies.end());
    if (NULL == m_pDB.get()) return ;
    if (NULL == m_pDS.get()) return ;

    CStdString strSQL=FormatSQL("select movie.*,files.strFileName,path.strPath from movie join files on files.idMovie=movie.idMovie join path on files.idPath=path.idPath join actorlinkmovie on actorlinkmovie.idmovie=movie.idmovie join actors on actors.idActor=actorlinkmovie.idActor where actors.stractor='%s'", strActor.c_str());
    m_pDS->query( strSQL.c_str() );

    long lLastPathId = -1;
    while (!m_pDS->eof())
    {
      movies.push_back(GetDetailsForMovie(m_pDS));
      m_pDS->next();
    }
    m_pDS->close();
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "CVideoDatabase::GetMoviesByActor(%s) failed", strActor.c_str());
  }
}

void CVideoDatabase::GetTvShowsByActor(const CStdString& strActor, VECMOVIES& movies)
{
  try
  {
    movies.erase(movies.begin(), movies.end());
    if (NULL == m_pDB.get()) return ;
    if (NULL == m_pDS.get()) return ;

    CStdString strSQL=FormatSQL("select tvshow.*,path.strPath,path.strPath from tvshow join path on tvshowlinkpath.idpath = path.idpath join tvshowlinkpath on tvshowlinkpath.idshow=tvshow.idshow join actorlinktvshow on actorlinktvshow.idshow=tvshow.idshow join actors on actors.idActor=actorlinktvshow.idActor where actors.stractor='%s'", strActor.c_str());
    m_pDS->query( strSQL.c_str() );

    long lLastPathId = -1;
    while (!m_pDS->eof())
    {
      movies.push_back(GetDetailsForTvShow(m_pDS));
      m_pDS->next();
    }
    m_pDS->close();
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "CVideoDatabase::GetTvShowsByActor(%s) failed", strActor.c_str());
  }
}

void CVideoDatabase::GetEpisodesByActor(const CStdString& strActor, VECMOVIES& movies)
{
  try
  {
    movies.erase(movies.begin(), movies.end());
    if (NULL == m_pDB.get()) return ;
    if (NULL == m_pDS.get()) return ;

    CStdString strSQL=FormatSQL("select episode.*,files.strFileName,path.strPath from episode join files on files.idEpisode=episode.idEpisode join path on files.idPath=path.idPath join actorlinkepisode on actorlinkepisode.idepisode=episode.idepisode join actors on actors.idActor=actorlinkepisode.idActor where actors.stractor='%s'", strActor.c_str());
    m_pDS->query( strSQL.c_str() );

    long lLastPathId = -1;
    while (!m_pDS->eof())
    {
      movies.push_back(GetDetailsForEpisode(m_pDS));
      m_pDS->next();
    }
    m_pDS->close();
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "CVideoDatabase::GetEpisodesByActor(%s) failed", strActor.c_str());
  }
}

//********************************************************************************************************************************
void CVideoDatabase::GetMovieInfo(const CStdString& strFilenameAndPath, CIMDBMovie& details, long lMovieId /* = -1 */)
{
  try
  {
    // TODO: Optimize this - no need for all the queries!
    if (lMovieId < 0)
      lMovieId = GetMovieInfo(strFilenameAndPath);
    if (lMovieId < 0) return ;

    CStdString sql = FormatSQL("select movie.*,files.strFileName,path.strPath from movie join files on files.idMovie=movie.idMovie join path on files.idPath=path.idPath where movie.idMovie=%i", lMovieId);
    if (!m_pDS->query(sql.c_str()))
      return;
    details = GetDetailsForMovie(m_pDS, true);
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "CVideoDatabase::GetMovieInfo(%s) failed", strFilenameAndPath.c_str());
  }
}

//********************************************************************************************************************************
void CVideoDatabase::GetTvShowInfo(const CStdString& strFilenameAndPath, CIMDBMovie& details, long lTvShowId /* = -1 */)
{
  try
  {
    // TODO: Optimize this - no need for all the queries!
    if (lTvShowId < 0)
      lTvShowId = GetTvShowInfo(strFilenameAndPath);
    if (lTvShowId < 0) return ;

//    CStdString sql = FormatSQL("select tvshow.*,files.strFileName,path.strPath from tvshow join files on files.idEpisode=tvshowlinkepisode.idEpisode join path on files.idPath=path.idPath where tvshow.idshow=%i", lTvShowId);
    CStdString sql = FormatSQL("select tvshow.*,tvshowlinkpath.idpath,tvshowlinkpath.idpath from tvshow,tvshowlinkpath where tvshow.idshow=%i and tvshowlinkpath.idshow=tvshow.idshow", lTvShowId);
    if (!m_pDS->query(sql.c_str()))
      return;
    details = GetDetailsForTvShow(m_pDS, true);
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "CVideoDatabase::GetTvShowInfo(%s) failed", strFilenameAndPath.c_str());
  }
}

bool CVideoDatabase::GetEpisodeInfo(const CStdString& strFilenameAndPath, CIMDBMovie& details, long lEpisodeId /* = -1 */)
{
  try
  {
    // TODO: Optimize this - no need for all the queries!
    if (lEpisodeId < 0)
      lEpisodeId = GetEpisodeInfo(strFilenameAndPath);
    if (lEpisodeId < 0) return false;

    CStdString sql = FormatSQL("select episode.*,files.strFileName,path.strPath from episode join files on files.idEpisode=episode.idEpisode join path on files.idPath=path.idPath where episode.idEpisode=%i", lEpisodeId);
    if (!m_pDS->query(sql.c_str()))
      return false;
    details = GetDetailsForEpisode(m_pDS, true);
    return true;
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "CVideoDatabase::GetEpisodeInfo(%s) failed", strFilenameAndPath.c_str());
  }
  return false;
}

void CVideoDatabase::AddGenreAndDirectors(const CIMDBMovie& details, vector<long>& vecDirectors, vector<long>& vecGenres)
{
  // add all directors
  char szDirector[1024];
  strcpy(szDirector, details.m_strDirector.c_str());
  if (strstr(szDirector, "/"))
  {
    char *pToken = strtok(szDirector, "/");
    while ( pToken != NULL )
    {
      CStdString strDirector = pToken;
      strDirector.Trim();
      long lDirectorId = AddActor(strDirector);
      vecDirectors.push_back(lDirectorId);
      pToken = strtok( NULL, "/" );
    }
  }
  else if (!details.m_strDirector.IsEmpty())
  {
    CStdString strDirector = details.m_strDirector;
    strDirector.Trim();
    long lDirectorId = AddActor(strDirector);
    vecDirectors.push_back(lDirectorId);
  }

  // add all genres
  char szGenres[1024];
  strcpy(szGenres, details.m_strGenre.c_str());
  if (strstr(szGenres, "/"))
  {
    char *pToken = strtok(szGenres, "/");
    while ( pToken != NULL )
    {
      CStdString strGenre = pToken;
      strGenre.Trim();
      long lGenreId = AddGenre(strGenre);
      vecGenres.push_back(lGenreId);
      pToken = strtok( NULL, "/" );
    }
  }
  else if (!details.m_strGenre.IsEmpty())
  {
    CStdString strGenre = details.m_strGenre;
    strGenre.Trim();
    long lGenreId = AddGenre(strGenre);
    vecGenres.push_back(lGenreId);
  }
}

//********************************************************************************************************************************
void CVideoDatabase::SetDetailsForMovie(const CStdString& strFilenameAndPath, const CIMDBMovie& details)
{
  try
  {
    long lMovieId = GetMovie(strFilenameAndPath);
    if (lMovieId < 0)
      lMovieId = AddMovie(strFilenameAndPath);

    BeginTransaction();

    vector<long> vecDirectors;
    vector<long> vecGenres;
    AddGenreAndDirectors(details,vecDirectors,vecGenres);
    
    // add cast...
    for (CIMDBMovie::iCast it = details.m_cast.begin(); it != details.m_cast.end(); ++it)
    {
      long lActor = AddActor(it->first);
      AddActorToMovie(lMovieId, lActor, it->second);
    }
    
    for (int i = 0; i < (int)vecGenres.size(); ++i)
    {
      AddGenreToMovie(lMovieId, vecGenres[i]);
    }

    for (i = 0; i < (int)vecDirectors.size(); ++i)
    {
      AddDirectorToMovie(lMovieId, vecDirectors[i]);
    }

    // delete the current row for this movie
    CStdString sql = FormatSQL("delete from movie where movie.idMovie=%i", lMovieId);
    m_pDS->exec(sql.c_str());

    // and insert the new row
    CStdString sqlColumns = "insert into movie (idMovie";
    CStdString strValues = FormatSQL(") values (%i", lMovieId);
    for (int iType=VIDEODB_ID_MIN+1;iType<VIDEODB_ID_MAX;++iType)
    {
      CStdString strValue;
      switch (DbMovieOffsets[iType].type)
      {
      case VIDEODB_TYPE_STRING:
        strValue = *(CStdString*)(((char*)&details)+DbMovieOffsets[iType].offset);
        break;
      case VIDEODB_TYPE_INT:
        strValue.Format("%i",*(int*)(((char*)&details)+DbMovieOffsets[iType].offset));
        break;
      case VIDEODB_TYPE_BOOL:
        strValue = *(bool*)(((char*)&details)+DbMovieOffsets[iType].offset)?"true":"false";
        break;
      case VIDEODB_TYPE_FLOAT:
        strValue.Format("%f",*(float*)(((char*)&details)+DbMovieOffsets[iType].offset));
        break;
      }
      sqlColumns += FormatSQL(",c%02d", iType);
      strValues += FormatSQL(",'%s'", strValue.c_str());
    }
    CStdString strSQL = sqlColumns + strValues + ")";
    m_pDS->exec(strSQL.c_str());
    CommitTransaction();
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "CVideoDatabase::SetDetailsForMovie(%s) failed", strFilenameAndPath.c_str());
  }
}

long CVideoDatabase::SetDetailsForTvShow(const CStdString& strPath, const CIMDBMovie& details)
{
  try
  {
    long lTvShowId = GetTvShow(strPath);
    if (lTvShowId < 0)
      lTvShowId = AddTvShow(strPath);

    BeginTransaction();

    vector<long> vecDirectors;
    vector<long> vecGenres;
    AddGenreAndDirectors(details,vecDirectors,vecGenres);
  
    // add cast...
    for (CIMDBMovie::iCast it = details.m_cast.begin(); it != details.m_cast.end(); ++it)
    {
      long lActor = AddActor(it->first);
      AddActorToTvShow(lTvShowId, lActor, it->second);
    }
    
    for (int i = 0; i < (int)vecGenres.size(); ++i)
    {
      AddGenreToTvShow(lTvShowId, vecGenres[i]);
    }

    for (i = 0; i < (int)vecDirectors.size(); ++i)
    {
      AddDirectorToTvShow(lTvShowId, vecDirectors[i]);
    }

    // delete the current row for this movie
    CStdString sql = FormatSQL("delete from tvshow where tvshow.idShow=%i", lTvShowId);
    m_pDS->exec(sql.c_str());

    // and insert the new row
    CStdString sqlColumns = "insert into tvshow (idShow";
    CStdString strValues = FormatSQL(") values (%i", lTvShowId);
    for (int iType=VIDEODB_ID_MIN+1;iType<VIDEODB_ID_MAX;++iType)
    {
      CStdString strValue;
      switch (DbTvShowOffsets[iType].type)
      {
      case VIDEODB_TYPE_STRING:
        strValue = *(CStdString*)(((char*)&details)+DbTvShowOffsets[iType].offset);
        break;
      case VIDEODB_TYPE_INT:
        strValue.Format("%i",*(int*)(((char*)&details)+DbTvShowOffsets[iType].offset));
        break;
      case VIDEODB_TYPE_BOOL:
        strValue = *(bool*)(((char*)&details)+DbTvShowOffsets[iType].offset)?"true":"false";
        break;
      case VIDEODB_TYPE_FLOAT:
        strValue.Format("%f",*(float*)(((char*)&details)+DbTvShowOffsets[iType].offset));
        break;
      }
      sqlColumns += FormatSQL(",c%02d", iType);
      strValues += FormatSQL(",'%s'", strValue.c_str());
    }
    CStdString strSQL = sqlColumns + strValues + ")";
    m_pDS->exec(strSQL.c_str());
    // update tvshowlinkpath info to reflect it points to this tvshow
    long lPathId = GetPath(strPath);
    if (lPathId < 0)
      lPathId = AddPath(strPath);
    CommitTransaction();
    return lTvShowId;
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "CVideoDatabase::SetDetailsForTvShow(%s) failed", strPath.c_str());
  }

  return -1;
}

void CVideoDatabase::SetDetailsForEpisode(const CStdString& strFilenameAndPath, const CIMDBMovie& details, long idShow)
{
  try
  {
    long lEpisodeId = GetEpisode(strFilenameAndPath);
    if (lEpisodeId < 0)
      lEpisodeId = AddEpisode(idShow,strFilenameAndPath);
    if (lEpisodeId < 0)
      return;

    vector<long> vecDirectors;
    vector<long> vecGenres;
    AddGenreAndDirectors(details,vecDirectors,vecGenres);
    
    // add cast...
    for (CIMDBMovie::iCast it = details.m_cast.begin(); it != details.m_cast.end(); ++it)
    {
      long lActor = AddActor(it->first);
      AddActorToEpisode(lEpisodeId, lActor, it->second);
    }
    
    for (int i = 0; i < (int)vecGenres.size(); ++i)
    {
      AddGenreToEpisode(lEpisodeId, vecGenres[i]);
    }

    for (i = 0; i < (int)vecDirectors.size(); ++i)
    {
      AddDirectorToEpisode(lEpisodeId, vecDirectors[i]);
    }

    // delete the current row for this movie
    CStdString sql = FormatSQL("delete from episode where episode.idEpisode=%i", lEpisodeId);
    m_pDS->exec(sql.c_str());

    // and insert the new row
    CStdString sqlColumns = "insert into episode (idEpisode";
    CStdString strValues = FormatSQL(") values (%i", lEpisodeId);
    for (int iType=VIDEODB_ID_EPISODE_MIN+1;iType<VIDEODB_ID_EPISODE_MAX;++iType)
    {
      CStdString strValue;
      switch (DbEpisodeOffsets[iType].type)
      {
      case VIDEODB_TYPE_STRING:
        strValue = *(CStdString*)(((char*)&details)+DbEpisodeOffsets[iType].offset);
        break;
      case VIDEODB_TYPE_INT:
        strValue.Format("%i",*(int*)(((char*)&details)+DbEpisodeOffsets[iType].offset));
        break;
      case VIDEODB_TYPE_BOOL:
        strValue = *(bool*)(((char*)&details)+DbEpisodeOffsets[iType].offset)?"true":"false";
        break;
      case VIDEODB_TYPE_FLOAT:
        strValue.Format("%f",*(float*)(((char*)&details)+DbEpisodeOffsets[iType].offset));
        break;
      }
      sqlColumns += FormatSQL(",c%02d", iType);
      strValues += FormatSQL(",'%s'", strValue.c_str());
    }
    CStdString strSQL = sqlColumns + strValues + ")";
    m_pDS->exec(strSQL.c_str());
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "CVideoDatabase::SetDetailsForEpisode(%s) failed", strFilenameAndPath.c_str());
  }
}

//********************************************************************************************************************************
void CVideoDatabase::GetMoviesByPath(const CStdString& strPath1, VECMOVIES& movies)
{
  try
  {
    if (strPath1.size() == 0) return;
    movies.erase(movies.begin(), movies.end());

    vector<CStdString> vecPaths;
    CStdString strPath = strPath1;
    if (strPath.Left(14).Equals("virtualpath://"))
    {
      CVirtualPathDirectory vpath;
      if (!vpath.GetPathes(strPath, vecPaths))
        return;
    }
    else
      vecPaths.push_back(strPath);

    // query for each patg in the vector!
    for (int i = 0; i < (int)vecPaths.size(); ++i)
    {
      strPath = vecPaths[i];
      if (CUtil::HasSlashAtEnd(strPath)) strPath = strPath.Left(strPath.size() - 1);
      CStdString strStackPath = "stack://" + strPath;

      if (NULL == m_pDB.get()) return ;
      if (NULL == m_pDS.get()) return ;
      CStdString strSQL=FormatSQL("select * from path join files on path.idPath = files.idPath join movieinfo on files.idMovie = movieinfo.idMovie where path.strPath like '%%%s' or path.strPath like '%%%s'", strPath.c_str(), strStackPath.c_str());
      CLog::Log(LOGDEBUG,"CVideoDatabase::GetMoviesByPath query = %s", strSQL.c_str());

      m_pDS->query( strSQL.c_str() );
      while (!m_pDS->eof())
      {
        CIMDBMovie details;
        long lMovieId = m_pDS->fv("files.idMovie").get_asLong();
        details.m_strSearchString.Format("%i", lMovieId);
        details.m_strIMDBNumber = m_pDS->fv("movieinfo.IMDBID").get_asString();
        details.m_strFile = m_pDS->fv("files.strFilename").get_asString();
        details.m_strPath = m_pDS->fv("path.strPath").get_asString();
        //CLog::Log(LOGDEBUG,"  movie [%s%s]", details.m_strPath.c_str(), details.m_strFile.c_str());
        movies.push_back(details);
        m_pDS->next();
      }
      m_pDS->close();
    }
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "CVideoDatabase::GetMoviesByPath(%s) failed", strPath1.c_str());
  }
}

//********************************************************************************************************************************
void CVideoDatabase::GetFilePath(long lMovieId, CStdString &filePath, int iType)
{
  try
  {
    if (NULL == m_pDB.get()) return ;
    if (NULL == m_pDS.get()) return ;

    if (lMovieId < 0) return ;
    
    CStdString strSQL;
    if (iType == 0)
      strSQL=FormatSQL("select path.strPath,files.strFileName from path, files where path.idPath=files.idPath and files.idmovie=%i order by strFilename", lMovieId );
    if (iType == 1)
      strSQL=FormatSQL("select path.strPath,files.strFileName from path, files where path.idPath=files.idPath and files.idepisode=%i order by strFilename", lMovieId );
    if (iType == 2)
      strSQL=FormatSQL("select path.strPath from path,tvshowlinkpath where path.idpath=tvshowlinkpath.idpath and tvshowlinkpath.idshow=%i", lMovieId );

    m_pDS->query( strSQL.c_str() );
    if (!m_pDS->eof())
    {
      if (iType != 2)
        CUtil::AddFileToFolder(m_pDS->fv("path.strPath").get_asString(),m_pDS->fv("files.strFilename").get_asString(),filePath);
      else
        filePath = m_pDS->fv("path.strPath").get_asString();
    }
    m_pDS->close();
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "CVideoDatabase::GetFilePath() failed");
  }
}

//********************************************************************************************************************************
void CVideoDatabase::GetBookMarksForFile(const CStdString& strFilenameAndPath, VECBOOKMARKS& bookmarks, CBookmark::EType type /*= CBookmark::STANDARD*/)
{
  try
  {
    long lMovieId, lEpisodeId;
    long lFileId = GetFile(strFilenameAndPath, lMovieId, lEpisodeId, true);
    if (lFileId < 0) return ;
    bookmarks.erase(bookmarks.begin(), bookmarks.end());
    if (NULL == m_pDB.get()) return ;
    if (NULL == m_pDS.get()) return ;

    CStdString strSQL=FormatSQL("select * from bookmark where idFile=%i and type=%i order by timeInSeconds", lFileId, (int)type);
    m_pDS->query( strSQL.c_str() );
    while (!m_pDS->eof())
    {
      CBookmark bookmark;
      bookmark.timeInSeconds = m_pDS->fv("timeInSeconds").get_asDouble();
      bookmark.thumbNailImage = m_pDS->fv("thumbNailImage").get_asString();
      bookmark.playerState = m_pDS->fv("playerState").get_asString();
      bookmark.player = m_pDS->fv("player").get_asString();
      bookmark.type = type;
      bookmarks.push_back(bookmark);
      m_pDS->next();
    }
    //sort(bookmarks.begin(), bookmarks.end(), SortBookmarks);
    m_pDS->close();
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "CVideoDatabase::GetBookMarksForMovie(%s) failed", strFilenameAndPath.c_str());
  }
}

bool CVideoDatabase::GetResumeBookMark(const CStdString& strFilenameAndPath, CBookmark &bookmark)
{
  VECBOOKMARKS bookmarks;
  GetBookMarksForFile(strFilenameAndPath, bookmarks, CBookmark::RESUME);
  if(bookmarks.size()>0)
  {
    bookmark = bookmarks[0];
    return true;
  }
  return false;
}

//********************************************************************************************************************************
void CVideoDatabase::AddBookMarkToFile(const CStdString& strFilenameAndPath, const CBookmark &bookmark, CBookmark::EType type /*= CBookmark::STANDARD*/)
{
  try
  {
    long lMovieId, lEpisodeId;
    long lFileId = GetFile(strFilenameAndPath, lMovieId, lEpisodeId, true);
    if (lFileId < 0)
    {
      // Doesn't exist in the database yet - add it.
      // TODO: It doesn't appear to me that the CDLabel parameter or the subtitles
      // parameter is anywhere in use in XBMC.
      lFileId = AddFile(strFilenameAndPath);
      if (lFileId < 0)
        return ;
    }
    if (NULL == m_pDB.get()) return ;
    if (NULL == m_pDS.get()) return ;

    CStdString strSQL;
    int idBookmark=-1;
    if( type == CBookmark::RESUME ) // get the same resume mark bookmark each time type
    {
      strSQL=FormatSQL("select idBookmark from bookmark where idFile=%i and type=1", lFileId);   
    }
    else // get the same bookmark again, and update. not sure here as a dvd can have same time in multiple places, state will differ thou
    {
      /* get a bookmark within the same time as previous */
      double mintime = bookmark.timeInSeconds - 0.5f;
      double maxtime = bookmark.timeInSeconds + 0.5f;
      strSQL=FormatSQL("select idBookmark from bookmark where idFile=%i and type=%i and (timeInSeconds between %f and %f) and playerState='%s'", lFileId, (int)type, mintime, maxtime, bookmark.playerState.c_str());
    }

    // get current id
    m_pDS->query( strSQL.c_str() );
    if (m_pDS->num_rows() != 0)
      idBookmark = m_pDS->get_field_value("idBookmark").get_asInteger();            
    m_pDS->close();

    // update or insert depending if it existed before
    if( idBookmark >= 0 )
      strSQL=FormatSQL("update bookmark set timeInSeconds = %f, thumbNailImage = '%s', player = '%s', playerState = '%s' where idBookmark = %i", bookmark.timeInSeconds, bookmark.thumbNailImage.c_str(), bookmark.player.c_str(), bookmark.playerState.c_str(), idBookmark);
    else
      strSQL=FormatSQL("insert into bookmark (idBookmark, idFile, timeInSeconds, thumbNailImage, player, playerState, type) values(NULL,%i,%f,'%s','%s','%s', %i)", lFileId, bookmark.timeInSeconds, bookmark.thumbNailImage.c_str(), bookmark.player.c_str(), bookmark.playerState.c_str(), (int)type);

    m_pDS->exec(strSQL.c_str());
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "CVideoDatabase::AddBookMarkToMovie(%s) failed", strFilenameAndPath.c_str());
  }
}

void CVideoDatabase::ClearBookMarkOfFile(const CStdString& strFilenameAndPath, CBookmark& bookmark, CBookmark::EType type /*= CBookmark::STANDARD*/)
{
  try
  {
    long lMovieId, lEpisodeId;
    long lFileId = GetFile(strFilenameAndPath, lMovieId, lEpisodeId, true);
    if (lFileId < 0) return ;
    if (NULL == m_pDB.get()) return ;
    if (NULL == m_pDS.get()) return ;

    /* a litle bit uggly, we clear first bookmark that is within one second of given */
    /* should be no problem since we never add bookmarks that are closer than that   */
    double mintime = bookmark.timeInSeconds - 0.5f;
    double maxtime = bookmark.timeInSeconds + 0.5f;
    CStdString strSQL=FormatSQL("select idBookmark from bookmark where idFile=%i and type=%i and (timeInSeconds between %f and %f)", lFileId, type, mintime, maxtime);
    m_pDS->query( strSQL.c_str() );
    if (m_pDS->num_rows() != 0)
    {
      int idBookmark = m_pDS->get_field_value("idBookmark").get_asInteger();
      strSQL=FormatSQL("delete from bookmark where idBookmark=%i",idBookmark);
      m_pDS->exec(strSQL.c_str());
    }

    m_pDS->close();
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "CVideoDatabase::ClearBookMarkOfFile(%s) failed", strFilenameAndPath.c_str());
  }
}

//********************************************************************************************************************************
void CVideoDatabase::ClearBookMarksOfFile(const CStdString& strFilenameAndPath, CBookmark::EType type /*= CBookmark::STANDARD*/)
{
  try
  {
    long lMovieId, lEpisodeId;
    long lFileId = GetFile(strFilenameAndPath, lMovieId, lEpisodeId, true);
    if (lFileId < 0) return ;
    if (NULL == m_pDB.get()) return ;
    if (NULL == m_pDS.get()) return ;

    CStdString strSQL=FormatSQL("delete from bookmark where idFile=%i and type=%i", lFileId, (int)type);
    m_pDS->exec(strSQL.c_str());
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "CVideoDatabase::ClearBookMarksOfMovie(%s) failed", strFilenameAndPath.c_str());
  }
}

//********************************************************************************************************************************
void CVideoDatabase::DeleteMovie(const CStdString& strFilenameAndPath)
{
  try
  {
    long lMovieId, lEpisodeId;
    if (NULL == m_pDB.get()) return ;
    if (NULL == m_pDS.get()) return ;
    if (GetFile(strFilenameAndPath, lMovieId, lEpisodeId) < 0)
    {
      return ;
    }

    ClearBookMarksOfFile(strFilenameAndPath);

    CStdString strSQL;
    strSQL=FormatSQL("update files set idMovie=-1 where idmovie=%i",lMovieId);
    m_pDS->exec(strSQL.c_str());

    strSQL=FormatSQL("delete from genrelinkmovie where idmovie=%i", lMovieId);
    m_pDS->exec(strSQL.c_str());

    strSQL=FormatSQL("delete from actorlinkmovie where idmovie=%i", lMovieId);
    m_pDS->exec(strSQL.c_str());

    strSQL=FormatSQL("delete from directorlinkmovie where idmovie=%i", lMovieId);
    m_pDS->exec(strSQL.c_str());

    strSQL=FormatSQL("delete from movie where idmovie=%i", lMovieId);
    m_pDS->exec(strSQL.c_str());
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "CVideoDatabase::DeleteMovie() failed");
  }
}

void CVideoDatabase::DeleteTvShow(const CStdString& strPath)
{
  try
  {
    long lTvShowId=-1;
    if (NULL == m_pDB.get()) return ;
    if (NULL == m_pDS.get()) return ;
    lTvShowId = GetTvShowInfo(strPath);
    if (lTvShowId < 0)
    {
      return ;
    }

    BeginTransaction();
    CStdString strSQL=FormatSQL("select idepisode from tvshowlinkepisode where tvshowlinkepisode.idshow=%u",lTvShowId);
    m_pDS2->query(strSQL.c_str());
    while (!m_pDS2->eof())
    {
      DeleteEpisode("",m_pDS2->fv(0).get_asLong());
      m_pDS2->next();
    }

    strSQL=FormatSQL("delete from genrelinktvshow where idshow=%i", lTvShowId);
    m_pDS->exec(strSQL.c_str());

    strSQL=FormatSQL("delete from actorlinktvshow where idshow=%i", lTvShowId);
    m_pDS->exec(strSQL.c_str());

    strSQL=FormatSQL("delete from directorlinktvshow where idshow=%i", lTvShowId);
    m_pDS->exec(strSQL.c_str());

    strSQL=FormatSQL("delete from tvshow where idshow=%i", lTvShowId);
    m_pDS->exec(strSQL.c_str());

    CommitTransaction();
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "CVideoDatabase::DeleteTvShow() failed");
  }
}

void CVideoDatabase::DeleteEpisode(const CStdString& strFilenameAndPath, long lEpisodeId)
{
  try
  {
    long lMovieId;
    if (NULL == m_pDB.get()) return ;
    if (NULL == m_pDS.get()) return ;
    if (lEpisodeId < 0)
    {
      if (GetFile(strFilenameAndPath, lMovieId, lEpisodeId) < 0)
      {
        return ;
      }
    }

    ClearBookMarksOfFile(strFilenameAndPath);

    CStdString strSQL;
    strSQL=FormatSQL("update files set idEpisode=-1 where idepisode=%i",lEpisodeId);
    m_pDS->exec(strSQL.c_str());

    strSQL=FormatSQL("delete from genrelinkepisode where idepisode=%i", lEpisodeId);
    m_pDS->exec(strSQL.c_str());

    strSQL=FormatSQL("delete from actorlinkepisode where idepisode=%i", lEpisodeId);
    m_pDS->exec(strSQL.c_str());

    strSQL=FormatSQL("delete from directorlinkepisode where idepisode=%i", lEpisodeId);
    m_pDS->exec(strSQL.c_str());

    strSQL=FormatSQL("select tvshowlinkepisode.idshow from tvshowlinkepisode where idepisode=%u",lEpisodeId);
    m_pDS->query(strSQL.c_str());

    strSQL=FormatSQL("update tvshow set c%02d=c%02d-1 where idshow=%u",VIDEODB_ID_TV_EPISODES,VIDEODB_ID_TV_EPISODES,m_pDS->fv(0).get_asString());
    m_pDS->exec(strSQL);

    strSQL=FormatSQL("delete from tvshowlinkepisode where idepisode=%i", lEpisodeId);
    m_pDS->exec(strSQL.c_str());

    strSQL=FormatSQL("delete from episode where idepisode=%i", lEpisodeId);
    m_pDS->exec(strSQL.c_str());
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "CVideoDatabase::DeleteEpisode() failed");
  }
}

DWORD movieTime = 0;
DWORD castTime = 0;

CIMDBMovie CVideoDatabase::GetDetailsForMovie(auto_ptr<Dataset> &pDS, bool needsCast /* = false */)
{
  CIMDBMovie details;
  details.Reset();

  DWORD time = timeGetTime();
  long lMovieId = pDS->fv(0).get_asLong();

  for (int iType = VIDEODB_ID_MIN + 1; iType < VIDEODB_ID_MAX; iType++)
  {
    switch (DbMovieOffsets[iType].type)
    {
    case VIDEODB_TYPE_STRING:
      *(CStdString*)(((char*)&details)+DbMovieOffsets[iType].offset) = pDS->fv(iType+1).get_asString();
      break;
    case VIDEODB_TYPE_INT:
      *(int*)(((char*)&details)+DbMovieOffsets[iType].offset) = pDS->fv(iType+1).get_asInteger();
      break;
    case VIDEODB_TYPE_BOOL:
      *(bool*)(((char*)&details)+DbMovieOffsets[iType].offset) = pDS->fv(iType+1).get_asBool();
      break;
    case VIDEODB_TYPE_FLOAT:
      *(float*)(((char*)&details)+DbMovieOffsets[iType].offset) = pDS->fv(iType+1).get_asFloat();
      break;
    }
  }
  details.m_strSearchString.Format("%i", lMovieId);

  details.m_strPath = m_pDS->fv(VIDEODB_DETAILS_PATH).get_asString();
  CUtil::AddFileToFolder(details.m_strPath, m_pDS->fv(VIDEODB_DETAILS_FILE).get_asString(),details.m_strFileNameAndPath);
  movieTime += timeGetTime() - time; time = timeGetTime();

  if (needsCast)
  {
    // create cast string
    CStdString strSQL = FormatSQL("select actors.strActor,actorlinkmovie.strRole from actorlinkmovie,actors where actorlinkmovie.idMovie=%u and actorlinkmovie.idActor = actors.idActor",lMovieId);
    m_pDS2->query(strSQL.c_str());
    while (!m_pDS2->eof())
    {
      details.m_cast.push_back(make_pair(m_pDS2->fv("actors.strActor").get_asString(), m_pDS2->fv("actorlinkmovie.strRole").get_asString()));
      m_pDS2->next();
    }
    castTime += timeGetTime() - time; time = timeGetTime();
  }
  return details;
}

CIMDBMovie CVideoDatabase::GetDetailsForTvShow(auto_ptr<Dataset> &pDS, bool needsCast /* = false */, const CStdString &baseDir)
{
  CIMDBMovie details;
  details.Reset();

  DWORD time = timeGetTime();
  long lTvShowId = pDS->fv(0).get_asLong();

  for (int iType = VIDEODB_ID_TV_MIN + 1; iType < VIDEODB_ID_TV_MAX; iType++)
  {
    switch (DbTvShowOffsets[iType].type)
    {
    case VIDEODB_TYPE_STRING:
      *(CStdString*)(((char*)&details)+DbTvShowOffsets[iType].offset) = pDS->fv(iType+1).get_asString();
      break;
    case VIDEODB_TYPE_INT:
      *(int*)(((char*)&details)+DbTvShowOffsets[iType].offset) = pDS->fv(iType+1).get_asInteger();
      break;
    case VIDEODB_TYPE_BOOL:
      *(bool*)(((char*)&details)+DbTvShowOffsets[iType].offset) = pDS->fv(iType+1).get_asBool();
      break;
    case VIDEODB_TYPE_FLOAT:
      *(float*)(((char*)&details)+DbTvShowOffsets[iType].offset) = pDS->fv(iType+1).get_asFloat();
      break;
    }
  }
  details.m_strSearchString.Format("%i", lTvShowId);
  details.m_strPath = m_pDS->fv(VIDEODB_DETAILS_PATH).get_asString();
  if (baseDir.IsEmpty())
    details.m_strFileNameAndPath = details.m_strPath;
  else
    details.m_strFileNameAndPath.Format("%s%i/", baseDir.c_str(), lTvShowId);

  movieTime += timeGetTime() - time; time = timeGetTime();

  if (needsCast)
  {
    // create cast string
    CStdString strSQL = FormatSQL("select actors.strActor,actorlinktvshow.strRole from actorlinktvshow,actors where actorlinktvshow.idShow=%u and actorlinktvshow.idActor = actors.idActor",lTvShowId);
    m_pDS2->query(strSQL.c_str());
    while (!m_pDS2->eof())
    {
      details.m_cast.push_back(make_pair(m_pDS2->fv("actors.strActor").get_asString(), m_pDS2->fv("actorlinktvshow.strRole").get_asString()));
      m_pDS2->next();
    }
    castTime += timeGetTime() - time; time = timeGetTime();
  }
  return details;
}

CIMDBMovie CVideoDatabase::GetDetailsForEpisode(auto_ptr<Dataset> &pDS, bool needsCast /* = false */)
{
  CIMDBMovie details;
  details.Reset();

  DWORD time = timeGetTime();
  long lEpisodeId = pDS->fv(0).get_asLong();

  for (int iType = VIDEODB_ID_EPISODE_MIN + 1; iType < VIDEODB_ID_EPISODE_MAX; iType++)
  {
    switch (DbEpisodeOffsets[iType].type)
    {
    case VIDEODB_TYPE_STRING:
      *(CStdString*)(((char*)&details)+DbEpisodeOffsets[iType].offset) = pDS->fv(iType+1).get_asString();
      break;
    case VIDEODB_TYPE_INT:
      *(int*)(((char*)&details)+DbEpisodeOffsets[iType].offset) = pDS->fv(iType+1).get_asInteger();
      break;
    case VIDEODB_TYPE_BOOL:
      *(bool*)(((char*)&details)+DbEpisodeOffsets[iType].offset) = pDS->fv(iType+1).get_asBool();
      break;
    case VIDEODB_TYPE_FLOAT:
      *(float*)(((char*)&details)+DbEpisodeOffsets[iType].offset) = pDS->fv(iType+1).get_asFloat();
      break;
    }
  }
  details.m_strSearchString.Format("%i", lEpisodeId);

  details.m_strPath = m_pDS->fv(VIDEODB_DETAILS_PATH).get_asString();
  CUtil::AddFileToFolder(details.m_strPath, m_pDS->fv(VIDEODB_DETAILS_FILE).get_asString(),details.m_strFileNameAndPath);
  movieTime += timeGetTime() - time; time = timeGetTime();

  if (needsCast)
  {
    // create cast string
    CStdString strSQL = FormatSQL("select actors.strActor,actorlinkepisode.strRole from actorlinkepisode,actors where actorlinkepisode.idEpisode=%u and actorlinkepisode.idActor = actors.idActor",lEpisodeId);
    m_pDS2->query(strSQL.c_str());
    while (!m_pDS2->eof())
    {
      details.m_cast.push_back(make_pair(m_pDS2->fv("actors.strActor").get_asString(), m_pDS2->fv("actorlinkepisode.strRole").get_asString()));
      m_pDS2->next();
    }
    castTime += timeGetTime() - time; time = timeGetTime();
  }
  return details;
}

/// \brief GetVideoSettings() obtains any saved video settings for the current file.
/// \retval Returns true if the settings exist, false otherwise.
bool CVideoDatabase::GetVideoSettings(const CStdString &strFilenameAndPath, CVideoSettings &settings)
{
  try
  {
    // obtain the FileID (if it exists)
#ifdef NEW_VIDEODB_METHODS
    if (NULL == m_pDB.get()) return false;
    if (NULL == m_pDS.get()) return false;
    CStdString strPath, strFileName;
    CUtil::Split(strFilenameAndPath, strPath, strFileName);
    CStdString strSQL=FormatSQL("select * from settings, files, path where settings.idfile=files.idfile and path.idpath=files.idpath and path.strPath like '%s' and files.strFileName like '%s'", strPath.c_str() , strFileName.c_str());
#else
    long lPathId, lMovieId;
    long lFileId = GetFile(strFilenameAndPath, lPathId, lMovieId, true);
    if (lFileId < 0) return false;
    if (NULL == m_pDB.get()) return false;
    if (NULL == m_pDS.get()) return false;
    // ok, now obtain the settings for this file
    CStdString strSQL=FormatSQL("select * from settings where settings.idFile = '%i'", lFileId);
#endif
    m_pDS->query( strSQL.c_str() );
    if (m_pDS->num_rows() > 0)
    { // get the video settings info
      settings.m_AudioDelay = m_pDS->fv("AudioDelay").get_asFloat();
      settings.m_AudioStream = m_pDS->fv("AudioStream").get_asInteger();
      settings.m_Brightness = m_pDS->fv("Brightness").get_asInteger();
      settings.m_Contrast = m_pDS->fv("Contrast").get_asInteger();
      settings.m_CustomPixelRatio = m_pDS->fv("PixelRatio").get_asFloat();
      settings.m_CustomZoomAmount = m_pDS->fv("ZoomAmount").get_asFloat();
      settings.m_Gamma = m_pDS->fv("Gamma").get_asInteger();
      settings.m_NonInterleaved = m_pDS->fv("Interleaved").get_asBool();
      settings.m_NoCache = m_pDS->fv("NoCache").get_asBool();
      settings.m_SubtitleDelay = m_pDS->fv("SubtitleDelay").get_asFloat();
      settings.m_SubtitleOn = m_pDS->fv("SubtitlesOn").get_asBool();
      settings.m_SubtitleStream = m_pDS->fv("SubtitleStream").get_asInteger();
      settings.m_ViewMode = m_pDS->fv("ViewMode").get_asInteger();
      settings.m_ResumeTime = m_pDS->fv("ResumeTime").get_asInteger();
      settings.m_Crop = m_pDS->fv("Crop").get_asBool();
      settings.m_CropLeft = m_pDS->fv("CropLeft").get_asInteger();
      settings.m_CropRight = m_pDS->fv("CropRight").get_asInteger();
      settings.m_CropTop = m_pDS->fv("CropTop").get_asInteger();
      settings.m_CropBottom = m_pDS->fv("CropBottom").get_asInteger();
      settings.m_InterlaceMethod = (EINTERLACEMETHOD)m_pDS->fv("Deinterlace").get_asInteger();
      settings.m_VolumeAmplification = m_pDS->fv("VolumeAmplification").get_asFloat();
      settings.m_OutputToAllSpeakers = m_pDS->fv("OutputToAllSpeakers").get_asBool();
      settings.m_SubtitleCached = false;
      m_pDS->close();
      return true;
    }
    m_pDS->close();
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "CVideoDatabase::GetVideoSettings() failed");
  }
  return false;
}

/// \brief Sets the settings for a particular video file
void CVideoDatabase::SetVideoSettings(const CStdString& strFilenameAndPath, const CVideoSettings &setting)
{
  try
  {
    long lMovieId, lEpisodeId;
    if (NULL == m_pDB.get()) return ;
    if (NULL == m_pDS.get()) return ;
    long lFileId = GetFile(strFilenameAndPath, lMovieId, lEpisodeId, true);
    if (lFileId < 0)
    { // no files found - we have to add one
      lFileId = AddFile(strFilenameAndPath);
      if (lFileId < 0) return ;
    }
    CStdString strSQL;
    strSQL.Format("select * from settings where idFile=%i", lFileId);
    m_pDS->query( strSQL.c_str() );
    if (m_pDS->num_rows() > 0)
    {
      m_pDS->close();
      // update the item
      strSQL=FormatSQL("update settings set Interleaved=%i,NoCache=%i,Deinterlace=%i,FilmGrain=%i,ViewMode=%i,ZoomAmount=%f,PixelRatio=%f,"
                       "AudioStream=%i,SubtitleStream=%i,SubtitleDelay=%f,SubtitlesOn=%i,Brightness=%i,Contrast=%i,Gamma=%i,"
                       "VolumeAmplification=%f,AudioDelay=%f,OutputToAllSpeakers=%i,",
                       setting.m_NonInterleaved, setting.m_NoCache, setting.m_InterlaceMethod, setting.m_FilmGrain, setting.m_ViewMode, setting.m_CustomZoomAmount, setting.m_CustomPixelRatio,
                       setting.m_AudioStream, setting.m_SubtitleStream, setting.m_SubtitleDelay, setting.m_SubtitleOn,
                       setting.m_Brightness, setting.m_Contrast, setting.m_Gamma, setting.m_VolumeAmplification, setting.m_AudioDelay,
                       setting.m_OutputToAllSpeakers);
      CStdString strSQL2;
      strSQL2=FormatSQL("ResumeTime=%i,Crop=%i,CropLeft=%i,CropRight=%i,CropTop=%i,CropBottom=%i where idFile=%i\n", setting.m_ResumeTime, setting.m_Crop, setting.m_CropLeft, setting.m_CropRight, setting.m_CropTop, setting.m_CropBottom, lFileId);
      strSQL += strSQL2;
      m_pDS->exec(strSQL.c_str());
      return ;
    }
    else
    { // add the items
      m_pDS->close();
      strSQL=FormatSQL("insert into settings ( idFile,Interleaved,NoCache,Deinterlace,FilmGrain,ViewMode,ZoomAmount,PixelRatio,"
                       "AudioStream,SubtitleStream,SubtitleDelay,SubtitlesOn,Brightness,Contrast,Gamma,"
                       "VolumeAmplification,AudioDelay,OutputToAllSpeakers,ResumeTime,Crop,CropLeft,CropRight,CropTop,CropBottom)"
                       " values (%i,%i,%i,%i,%i,%i,%f,%f,%i,%i,%f,%i,%i,%i,%i,%f,%f,",
                       lFileId, setting.m_NonInterleaved, setting.m_NoCache, setting.m_InterlaceMethod, setting.m_FilmGrain, setting.m_ViewMode, setting.m_CustomZoomAmount, setting.m_CustomPixelRatio,
                       setting.m_AudioStream, setting.m_SubtitleStream, setting.m_SubtitleDelay, setting.m_SubtitleOn,
                       setting.m_Brightness, setting.m_Contrast, setting.m_Gamma, setting.m_VolumeAmplification, setting.m_AudioDelay);
      CStdString strSQL2;
      strSQL2=FormatSQL("%i,%i,%i,%i,%i,%i,%i)\n", setting.m_OutputToAllSpeakers, setting.m_ResumeTime, setting.m_Crop, setting.m_CropLeft, setting.m_CropRight,
                    setting.m_CropTop, setting.m_CropBottom);
      strSQL += strSQL2;
      m_pDS->exec(strSQL.c_str());
    }
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "CVideoDatabase::SetVideoSettings(%s) failed", strFilenameAndPath.c_str());
  }
}

/// \brief GetStackTimes() obtains any saved video times for the stacked file
/// \retval Returns true if the stack times exist, false otherwise.
bool CVideoDatabase::GetStackTimes(const CStdString &filePath, vector<long> &times)
{
  try
  {
    // obtain the FileID (if it exists)
    long lMovieId, lEpisodeId;
    long lFileId = GetFile(filePath, lMovieId, lEpisodeId, true);
    if (lFileId < 0) return false;
    if (NULL == m_pDB.get()) return false;
    if (NULL == m_pDS.get()) return false;
    // ok, now obtain the settings for this file
    CStdString strSQL=FormatSQL("select times from stacktimes where idFile=%i\n", lFileId);
    m_pDS->query( strSQL.c_str() );
    if (m_pDS->num_rows() > 0)
    { // get the video settings info
      CStdStringArray timeString;
      long timeTotal = 0;
      StringUtils::SplitString(m_pDS->fv("times").get_asString(), ",", timeString);
      times.clear();
      for (unsigned int i = 0; i < timeString.size(); i++)
      {
        times.push_back(atoi(timeString[i].c_str()));
        timeTotal += atoi(timeString[i].c_str());
      }
      m_pDS->close();
      return (timeTotal > 0);
    }
    m_pDS->close();
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "CVideoDatabase::GetStackTimes() failed");
  }
  return false;
}

/// \brief Sets the stack times for a particular video file
void CVideoDatabase::SetStackTimes(const CStdString& filePath, vector<long> &times)
{
  try
  {
    long lMovieId, lEpisodeId;
    if (NULL == m_pDB.get()) return ;
    if (NULL == m_pDS.get()) return ;
    long lFileId = GetFile(filePath, lMovieId, lEpisodeId, true);
    if (lFileId < 0)
    { // no files found - we have to add one
      lMovieId = AddMovie(filePath);
      lFileId = GetFile(filePath, lMovieId, lEpisodeId, true);
      if (lFileId < 0) return ;
    }

    // delete any existing items
    m_pDS->exec( FormatSQL("delete from stacktimes where idFile=%i", lFileId) );

    // add the items
    CStdString timeString;
    timeString.Format("%i", times[0]);
    for (unsigned int i = 1; i < times.size(); i++)
    {
      CStdString time;
      time.Format(",%i", times[i]);
      timeString += time;
    }
    m_pDS->exec( FormatSQL("insert into stacktimes (idFile,times) values (%i,'%s')\n", lFileId, timeString.c_str()) );
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "CVideoDatabase::SetStackTimes(%s) failed", filePath.c_str());
  }
}

void CVideoDatabase::RemoveContentForPath(const CStdString& strPath)
{
  try
  {
    if (NULL == m_pDB.get()) return ;
    if (NULL == m_pDS.get()) return ;

    std::auto_ptr<Dataset> pDS(m_pDB->CreateDataset());
    CStdString strPath1(strPath);
    CUtil::RemoveSlashAtEnd(strPath1);

    
    CStdString strSQL = FormatSQL("select idPath,strContent,strPath from path where strPath like '%%%s%%'",strPath1.c_str());
    pDS->query(strSQL.c_str());
    bool bEncodedChecked=false;
    while (!pDS->eof())
    {
      long lPathId = pDS->fv("path.idPath").get_asLong();
      CStdString strCurrPath = pDS->fv("path.strPath").get_asString();
//      if (pDS->fv("path.strContent").get_asString() == "movies")
      {
        strSQL=FormatSQL("select strFilename from files where files.idPath=%u and NOT (files.idMovie=-1)",lPathId);
        m_pDS2->query(strSQL.c_str());
        while (!m_pDS2->eof())
        {
          CStdString strMoviePath;
          CUtil::AddFileToFolder(strCurrPath,m_pDS2->fv("files.strFilename").get_asString(),strMoviePath);
          DeleteMovie(strMoviePath);
          m_pDS2->next();
        }
      }
      pDS->next();
      if (pDS->eof() && !bEncodedChecked) // rarred titles needs this
      {
        CStdString strEncoded(strPath);
        CUtil::URLEncode(strEncoded);
        CStdString strSQL = FormatSQL("select idPath,strContent,strPath from path where strPath like '%%%s%%'",strEncoded.c_str());
        pDS->query(strSQL.c_str());
        bEncodedChecked = true;
      }
    }
    strSQL = FormatSQL("update path set strContent = '', strScraper='' where strPath like '%%%s%%'",strPath1.c_str());
    pDS->exec(strSQL);

    CStdString strEncoded(strPath);
    CUtil::URLEncode(strEncoded);
    strSQL = FormatSQL("update path set strContent = '', strScraper='' where strPath like '%%%s%%'",strEncoded.c_str());
    pDS->exec(strSQL);
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "CVideoDatabase::RemoveContentFromPath(%s) failed", strPath.c_str());
  }
}

void CVideoDatabase::SetScraperForPath(const CStdString& filePath, const CStdString& strScraper, const CStdString& strContent)
{
  try
  {
    if (NULL == m_pDB.get()) return ;
    if (NULL == m_pDS.get()) return ;
    long lPathId = GetPath(filePath);
    if (lPathId < 0)
    { // no path found - we have to add one
      lPathId = AddPath(filePath);
      if (lPathId < 0) return ;
    }

    // Update
    CStdString strSQL =FormatSQL("update path set strContent='%s',strScraper='%s' where idPath=%u", strContent.c_str(), strScraper.c_str(), lPathId);
    m_pDS->exec(strSQL.c_str());
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "CVideoDatabase::SetScraperForPath(%s) failed", filePath.c_str());
  }
}

bool CVideoDatabase::UpdateOldVersion(int iVersion)
{
  return true;
}

void CVideoDatabase::MarkAsWatched(const CFileItem &item)
{
  // find the movie in the db
  long movieID = -1;
  if (item.m_musicInfoTag.GetURL().IsEmpty())
    movieID = GetMovieInfo(item.m_strPath);
  else
    movieID = atol(item.m_musicInfoTag.GetURL().c_str());
  bool bEpisode=false;
  if (movieID < 0)
  {
    bEpisode = true;
    movieID = GetEpisodeInfo(item.m_strPath);
    if (movieID < 0)
      return;    
  }
  // and mark as watched
  MarkAsWatched(movieID,bEpisode);
}

void CVideoDatabase::MarkAsWatched(long lMovieId, bool bEpisode /* = false */)
{
  try
  {
    if (NULL == m_pDB.get()) return ;
    if (NULL == m_pDS.get()) return ;
    CStdString strSQL;
    if (bEpisode)
    {
      CLog::Log(LOGINFO, "Updating Episode:%i as Watched", lMovieId);
      strSQL.Format("UPDATE episode set c%02d='true' WHERE idEpisode=%u", VIDEODB_ID_EPISODE_WATCHED, lMovieId);
    }
    else
    {
      CLog::Log(LOGINFO, "Updating Movie:%i as Watched", lMovieId);
      strSQL.Format("UPDATE movie set c%02d='true' WHERE idMovie=%u", VIDEODB_ID_WATCHED, lMovieId);
    }

    m_pDS->exec(strSQL.c_str());
  }
  catch (...)
  {
	  CLog::Log(LOGERROR, "CVideoDatabase::MarkAsWatched(long lMovieId) failed on MovieID:%i", lMovieId);
  }
}

void CVideoDatabase::MarkAsUnWatched(long lMovieId, bool bEpisode /* = false */)
{
  try
  {
    if (NULL == m_pDB.get()) return ;
    if (NULL == m_pDS.get()) return ;
    CStdString strSQL;
    if (bEpisode)
    {
      CLog::Log(LOGINFO, "Updating Episode:%i as UnWatched", lMovieId);
      strSQL.Format("UPDATE episode set c%02d='false' WHERE idEpisode=%u", VIDEODB_ID_EPISODE_WATCHED, lMovieId);
    }
    else
    {
      CLog::Log(LOGINFO, "Updating Movie:%i as UnWatched", lMovieId);
      strSQL.Format("UPDATE movie set c%02d='false' WHERE idMovie=%u", VIDEODB_ID_WATCHED, lMovieId);
    }
    m_pDS->exec(strSQL.c_str());
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "CVideoDatabase::MarkAsUnWatched(long lMovieId) failed on MovieID:%i", lMovieId);
  }
}

void CVideoDatabase::UpdateMovieTitle(long lMovieId, const CStdString& strNewMovieTitle, int iType)
{
  try
  {
    if (NULL == m_pDB.get()) return ;
    if (NULL == m_pDS.get()) return ;
    CStdString strSQL;
    if (iType == 2)
    {
      CLog::Log(LOGINFO, "Changing TvShow:id:%i New Title:%s", lMovieId, strNewMovieTitle.c_str());
      strSQL.Format("UPDATE tvshow SET c%02d='%s' WHERE idShow=%i", VIDEODB_ID_TV_TITLE, strNewMovieTitle.c_str(), lMovieId );
    }
    else if (iType == 1)
    {
      CLog::Log(LOGINFO, "Changing Episode:id:%i New Title:%s", lMovieId, strNewMovieTitle.c_str());
      strSQL.Format("UPDATE episode SET c%02d='%s' WHERE idEpisode=%i", VIDEODB_ID_EPISODE_TITLE, strNewMovieTitle.c_str(), lMovieId );
    }
    else
    {
      CLog::Log(LOGINFO, "Changing Movie:id:%i New Title:%s", lMovieId, strNewMovieTitle.c_str());
      strSQL.Format("UPDATE movie SET c%02d='%s' WHERE idMovie=%i", VIDEODB_ID_TITLE, strNewMovieTitle.c_str(), lMovieId );
    }
    m_pDS->exec(strSQL.c_str());
  }
  catch (...)
  {
	  CLog::Log(LOGERROR, "CVideoDatabase::UpdateMovieTitle(long lMovieId, const CStdString& strNewMovieTitle) failed on MovieID:%i and Title:%s", lMovieId, strNewMovieTitle);
  }
}

/// \brief EraseVideoSettings() Erases the videoSettings table and reconstructs it
void CVideoDatabase::EraseVideoSettings()
{
  try
  {
    CLog::Log(LOGINFO, "Deleting settings information for all movies");
    m_pDS->exec("delete from settings");
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "CVideoDatabase::EraseVideoSettings() failed");
  }
}

bool CVideoDatabase::GetGenresNav(const CStdString& strBaseDir, CFileItemList& items, long idContent)
{
  try
  {
    if (NULL == m_pDB.get()) return false;
    if (NULL == m_pDS.get()) return false;

//    DWORD time = timeGetTime();
    // get primary genres for movies
    CStdString strSQL;
    if (g_settings.m_vecProfiles[0].getLockMode() != LOCK_MODE_EVERYONE && !g_passwordManager.bMasterUser)
    {
      if (idContent == VIDEODB_CONTENT_MOVIES)
      {
        strSQL=FormatSQL("select genre.idgenre,genre.strgenre,path.strPath from genre,genrelinkmovie,movie,path,files where genre.idGenre=genrelinkMovie.idGenre and genrelinkMovie.idMovie = movie.idMovie and files.idMovie=movie.idMovie and path.idPath = files.idPath");
        if (g_stSettings.m_iMyVideoWatchMode == 1)
          strSQL += FormatSQL(" and movie.c%02d='false'", VIDEODB_ID_WATCHED);

        if (g_stSettings.m_iMyVideoWatchMode == 2)
          strSQL += FormatSQL(" and movie.c%02d='true'", VIDEODB_ID_WATCHED);
      }
      else if (idContent == VIDEODB_CONTENT_TVSHOWS)
      {
        strSQL=FormatSQL("select genre.idgenre,genre.strgenre,path.strPath from genre,genrelinktvshow,tvshow,tvshowlinkpath,files,path where genre.idGenre=genrelinkTvShow.idGenre and genrelinkTvShow.idShow = tvshow.idshow and files.idPath=tvshowlinkpath.idPath and tvshowlinkpath.idShow=tvshow.idShow and path.idPath = files.idPath");
      }
    }
    else
    {
      if (idContent == VIDEODB_CONTENT_MOVIES)
      {
        strSQL=FormatSQL("select distinct genre.idgenre,genre.strgenre from genre,genrelinkmovie,movie where genre.idGenre=genrelinkMovie.idGenre and genrelinkMovie.idMovie = movie.idMovie");
        if (g_stSettings.m_iMyVideoWatchMode == 1)
          strSQL += FormatSQL(" and movie.c%02d='false'", VIDEODB_ID_WATCHED);

        if (g_stSettings.m_iMyVideoWatchMode == 2)
          strSQL += FormatSQL(" and movie.c%02d='true'", VIDEODB_ID_WATCHED);
      }
      else if (idContent == VIDEODB_CONTENT_TVSHOWS)
      {
        strSQL=FormatSQL("select distinct genre.idgenre,genre.strgenre from genre,genrelinktvshow,tvshow where genre.idGenre=genrelinkTvShow.idGenre and genrelinkTvShow.idShow = tvshow.idshow");
      }
    }

    // run query
    CLog::Log(LOGDEBUG, "CVideoDatabase::GetGenresNav() query: %s", strSQL.c_str());
    if (!m_pDS->query(strSQL.c_str())) return false;
    int iRowsFound = m_pDS->num_rows();
    if (iRowsFound == 0)
    {
      m_pDS->close();
      return true;
    }

    if (g_settings.m_vecProfiles[0].getLockMode() != LOCK_MODE_EVERYONE && !g_passwordManager.bMasterUser)
    {
      map<long, CStdString> mapGenres;
      map<long, CStdString>::iterator it;
      while (!m_pDS->eof())
      {
        long lGenreId = m_pDS->fv("genre.idgenre").get_asLong();
        CStdString strGenre = m_pDS->fv("genre.strgenre").get_asString();
        it = mapGenres.find(lGenreId);
        // was this genre already found?
        if (it == mapGenres.end())
        {
          // check path
          CStdString strPath;
          if (g_passwordManager.IsDatabasePathUnlocked(CStdString(m_pDS->fv("path.strPath").get_asString()),g_settings.m_vecMyVideoShares))
            mapGenres.insert(pair<long, CStdString>(lGenreId, strGenre));
        }
        m_pDS->next();
      }
      m_pDS->close();

      for (it=mapGenres.begin();it != mapGenres.end();++it)
      {
        CFileItem* pItem=new CFileItem(it->second);
        CStdString strDir;
        strDir.Format("%ld/", it->first);
        pItem->m_strPath=strBaseDir + strDir;
        pItem->m_bIsFolder=true;
        pItem->SetLabelPreformated(true);
        items.Add(pItem);
      }
    }
    else
    {
      while (!m_pDS->eof())
      {
        CFileItem* pItem=new CFileItem(m_pDS->fv("genre.strgenre").get_asString());
        CStdString strDir;
        strDir.Format("%ld/", m_pDS->fv("genre.idgenre").get_asLong());
        pItem->m_strPath=strBaseDir + strDir;
        pItem->m_bIsFolder=true;
        pItem->SetLabelPreformated(true);
        items.Add(pItem);
        m_pDS->next();
      }
      m_pDS->close();
    }

//    CLog::Log(LOGDEBUG, __FUNCTION__" Time: %d ms", timeGetTime() - time);
    return true;
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "CVideoDatabase:GetGenresNav() failed");
  }
  return false;
}

bool CVideoDatabase::GetDirectorsNav(const CStdString& strBaseDir, CFileItemList& items, long idContent)
{
  try
  {
    if (NULL == m_pDB.get()) return false;
    if (NULL == m_pDS.get()) return false;

    // get primary genres for movies
    CStdString strSQL;
    if (g_settings.m_vecProfiles[0].getLockMode() != LOCK_MODE_EVERYONE && !g_passwordManager.bMasterUser)
    {
      if (idContent == VIDEODB_CONTENT_MOVIES)
      {
        strSQL=FormatSQL("select actors.idActor,actors.strActor,path.strPath from actors,directorlinkmovie,movie,path,files where actor.idActor=directorlinkMovie.idDirector and directorlinkMovie.idMovie = movie.idMovie and files.idMovie=movie.idMovie and path.idPath = files.idPath");
        if (g_stSettings.m_iMyVideoWatchMode == 1)
          strSQL += FormatSQL(" and movie.c%02d='true')", VIDEODB_ID_WATCHED);
        if (g_stSettings.m_iMyVideoWatchMode == 2)
          strSQL += FormatSQL(" and movie.c%02d='true'", VIDEODB_ID_WATCHED);
      }
      else if (idContent == VIDEODB_CONTENT_TVSHOWS)
      {
        strSQL=FormatSQL("select actors.idActor,actors.strActor,path.strPath from actors,directorlinktvshow,tvshow,path,files,tvshowlinkepisode where actor.idActor=directorlinktvshow.idDirector and directorlinktvshow.idShow = tvshow.idShow and files.idEpisode=episode.idEpisode and tvshowlinkepisode.idShow=tvshow.idShow and episode.idEpisode=tvshowlinkepisode.idEpisode and path.idPath = files.idPath");
      }
      else
      { // TODO: JOINED QUERY IN LOCKED STATE
      }
    }
    else
    {
      if (idContent == VIDEODB_CONTENT_MOVIES)
      {
        strSQL=FormatSQL("select distinct actors.idActor,actors.strActor from actors,directorlinkmovie,movie where actors.idActor=directorlinkMovie.idDirector and directorlinkMovie.idMovie = movie.idMovie");
        if (g_stSettings.m_iMyVideoWatchMode == 1)
          strSQL += FormatSQL(" and movie.c%02d='true')", VIDEODB_ID_WATCHED);

        if (g_stSettings.m_iMyVideoWatchMode == 2)
          strSQL += FormatSQL(" and movie.c%02d='true'", VIDEODB_ID_WATCHED);
      }
      else if (idContent == VIDEODB_CONTENT_TVSHOWS)
      {
        strSQL=FormatSQL("select distinct actors.idActor,actors.strActor from actors,directorlinktvshow,tvshow where actors.idActor=directorlinktvshow.idDirector and directorlinktvshow.idShow = tvshow.idShow");
      }
      else
      {
        if (g_stSettings.m_iMyVideoWatchMode == 1)
          strSQL=FormatSQL("select distinct actors.idActor,actors.strActor from actors,directorlinkmovie,movie where actors.idActor=directorlinkMovie.idDirector and directorlinkMovie.idMovie = movie.idMovie and movie.c%02d='false' union select distinct actors.idActor,actors.strActor from actors,directorlinktvshow,tvshow where actors.idActor=directorlinktvshow.idDirector and directorlinktvshow.idShow = tvshow.idShow",VIDEODB_ID_WATCHED); 
        else if (g_stSettings.m_iMyVideoWatchMode == 2)
          strSQL=FormatSQL("select distinct actors.idActor,actors.strActor from actors,directorlinkmovie,movie where actors.idActor=directorlinkMovie.idDirector and directorlinkMovie.idMovie = movie.idMovie and movie.c%02d='true' union select distinct actors.idActor,actors.strActor from actors,directorlinktvshow,tvshow where actors.idActor=directorlinktvshow.idDirector and directorlinktvshow.idShow = tvshow.idShow",VIDEODB_ID_WATCHED); 
        else
          strSQL=FormatSQL("select distinct actors.idActor,actors.strActor from actors,directorlinkmovie,movie where actors.idActor=directorlinkMovie.idDirector and directorlinkMovie.idMovie = movie.idMovie union select distinct actors.idActor,actors.strActor from actors,directorlinktvshow,tvshow where actors.idActor=directorlinktvshow.idDirector and directorlinktvshow.idShow = tvshow.idShow"); 
      }
    }

    // run query
    CLog::Log(LOGDEBUG, "CVideoDatabase::GetDirectorsNav() query: %s", strSQL.c_str());
    if (!m_pDS->query(strSQL.c_str())) return false;
    int iRowsFound = m_pDS->num_rows();
    if (iRowsFound == 0)
    {
      m_pDS->close();
      return true;
    }

    if (g_settings.m_vecProfiles[0].getLockMode() != LOCK_MODE_EVERYONE && !g_passwordManager.bMasterUser)
    {
      map<long, CStdString> mapDirector;
      map<long, CStdString>::iterator it;
      while (!m_pDS->eof())
      {
        long lDirectorId = m_pDS->fv("actors.idactor").get_asLong();
        CStdString strDirector = m_pDS->fv("actors.strActor").get_asString();
        it = mapDirector.find(lDirectorId);
        // was this genre already found?
        if (it == mapDirector.end())
        {
          // check path
          CStdString strPath;
          if (g_passwordManager.IsDatabasePathUnlocked(CStdString(m_pDS->fv("path.strPath").get_asString()),g_settings.m_vecMyVideoShares))
            mapDirector.insert(pair<long, CStdString>(lDirectorId, strDirector));
        }
        m_pDS->next();
      }
      m_pDS->close();

      for (it=mapDirector.begin();it != mapDirector.end();++it)
      {
        CFileItem* pItem=new CFileItem(it->second);
        CStdString strDir;
        strDir.Format("%ld/", it->first);
        pItem->m_strPath=strBaseDir + strDir;
        pItem->m_bIsFolder=true;
        pItem->SetLabelPreformated(true);
        items.Add(pItem);
      }
    }
    else
    {
      while (!m_pDS->eof())
      {
        CFileItem* pItem=new CFileItem(m_pDS->fv("actors.strActor").get_asString());
        CStdString strDir;
        strDir.Format("%ld/", m_pDS->fv("actors.idactor").get_asLong());
        pItem->m_strPath=strBaseDir + strDir;
        pItem->m_bIsFolder=true;
        pItem->SetLabelPreformated(true);
        items.Add(pItem);
        m_pDS->next();
      }
      m_pDS->close();
    }
      
    return true;
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "CVideoDatabase:GetDirectorsNav() failed");
  }
  return false;
}

bool CVideoDatabase::GetActorsNav(const CStdString& strBaseDir, CFileItemList& items, long idContent)
{
  try
  {
    if (NULL == m_pDB.get()) return false;
    if (NULL == m_pDS.get()) return false;

//    DWORD time = timeGetTime();
    CStdString strSQL;
    
    if (g_settings.m_vecProfiles[0].getLockMode() != LOCK_MODE_EVERYONE && !g_passwordManager.bMasterUser)
    {
      if (idContent == VIDEODB_CONTENT_MOVIES)
      {
        strSQL=FormatSQL("select actors.idactor,actors.strActor,path.strPath from actorlinkmovie,actors,movie,files,path where actors.idActor=actorlinkmovie.idActor and actorlinkmovie.idmovie=movie.idmovie and files.idMovie = movie.idMovie and files.idPath = path.idPath");
        if (g_stSettings.m_iMyVideoWatchMode == 1)
          strSQL += FormatSQL(" and movie.c%02d='false'", VIDEODB_ID_WATCHED);
        if (g_stSettings.m_iMyVideoWatchMode == 2)
          strSQL += FormatSQL(" and movie.c%02d='true'", VIDEODB_ID_WATCHED);
      }
      if (idContent == VIDEODB_CONTENT_TVSHOWS)
      {
        strSQL=FormatSQL("select actors.idactor,actors.strActor,path.strPath from actorlinkepisode,actors,episode,files,path where actors.idActor=actorlinkepisode.idActor and actorlinkepisode.idepisode=episode.idepisode and files.idEpisode = episode.idEpisode and files.idPath = path.idPath");
        if (g_stSettings.m_iMyVideoWatchMode == 1)
          strSQL += FormatSQL(" and episode.c%02d='false'", VIDEODB_ID_WATCHED);
        if (g_stSettings.m_iMyVideoWatchMode == 2)
          strSQL += FormatSQL(" and episode.c%02d='true'", VIDEODB_ID_WATCHED);
      }
    }
    else
    {
      if (idContent == VIDEODB_CONTENT_MOVIES)
      {
        strSQL=FormatSQL("select distinct actors.idactor,actors.strActor from actorlinkmovie,actors,movie where actors.idActor=actorlinkmovie.idActor and actorlinkmovie.idmovie=movie.idmovie");
        if (g_stSettings.m_iMyVideoWatchMode == 1)
          strSQL += FormatSQL(" and movie.c%02d='false'", VIDEODB_ID_WATCHED);
        if (g_stSettings.m_iMyVideoWatchMode == 2)
          strSQL += FormatSQL(" and movie.c%02d='true'", VIDEODB_ID_WATCHED);
      }
      else if (idContent == VIDEODB_CONTENT_TVSHOWS)
      {
        strSQL=FormatSQL("select distinct actors.idactor,actors.strActor from actorlinktvshow,actors,tvshow where actors.idActor=actorlinktvshow.idActor and actorlinktvshow.idshow=tvshow.idshow");
      }
    }

    if (!m_pDS->query(strSQL.c_str())) return false;
    int iRowsFound = m_pDS->num_rows();
    if (iRowsFound == 0)
    {
      m_pDS->close();
      return true;
    }

    if (g_settings.m_vecProfiles[0].getLockMode() != LOCK_MODE_EVERYONE && !g_passwordManager.bMasterUser)
    {
      map<long, CStdString> mapActors;
      map<long, CStdString>::iterator it;
      long lLastPathId = -1;

      while (!m_pDS->eof())
      {
        long lActorId = m_pDS->fv("actors.idactor").get_asLong();
        CStdString strActor = m_pDS->fv("actors.strActor").get_asString();
        it = mapActors.find(lActorId);
        // is this actor already known?
        if (it == mapActors.end())
        {
          // check path
          if (g_passwordManager.IsDatabasePathUnlocked(CStdString(m_pDS->fv("path.strPath").get_asString()),g_settings.m_vecMyVideoShares))
            mapActors.insert(pair<long, CStdString>(lActorId, strActor));
        }
        m_pDS->next();
      }
      m_pDS->close();

      for (it=mapActors.begin();it != mapActors.end();++it)
      {
        CFileItem* pItem=new CFileItem(it->second);
        CStdString strDir;
        strDir.Format("%ld/", it->first);
        pItem->m_strPath=strBaseDir + strDir;
        pItem->m_bIsFolder=true;
        items.Add(pItem);
      }
    }
    else
    {
      while (!m_pDS->eof())
      {
        CFileItem* pItem=new CFileItem(m_pDS->fv("actors.strActor").get_asString());
        CStdString strDir;
        strDir.Format("%ld/", m_pDS->fv("actors.idactor").get_asLong());
        pItem->m_strPath=strBaseDir + strDir;
        pItem->m_bIsFolder=true;
        items.Add(pItem);
        m_pDS->next();
      }
      m_pDS->close();
    }

//    CLog::Log(LOGDEBUG, __FUNCTION__" Time: %d ms", timeGetTime() - time);
    return true;
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "CVideoDatabase:GetActorsNav() failed");
  }
  return false;
}

bool CVideoDatabase::GetYearsNav(const CStdString& strBaseDir, CFileItemList& items, long idContent)
{
  try
  {
    if (NULL == m_pDB.get()) return false;
    if (NULL == m_pDS.get()) return false;

    CStdString strSQL;
    CStdString addParam;
    if (idContent == VIDEODB_CONTENT_MOVIES)
    {
      if (g_stSettings.m_iMyVideoWatchMode == 1)
        addParam = FormatSQL("movie.c%02d='false'", VIDEODB_ID_WATCHED);

      if (g_stSettings.m_iMyVideoWatchMode == 2)
        addParam = FormatSQL("movie.c%02d='true'", VIDEODB_ID_WATCHED);

      if (g_settings.m_vecProfiles[0].getLockMode() != LOCK_MODE_EVERYONE && !g_passwordManager.bMasterUser)
      {
        if (!addParam.IsEmpty())
          addParam = " and "+addParam;
        strSQL = FormatSQL("select movie.c%02d,path.strPath from movie,files,path where movie.idMovie = files.idMovie and files.idPath = path.idPath", VIDEODB_ID_YEAR);
      }
      else
      {
        if (!addParam.IsEmpty())
          addParam = " where "+addParam;
        strSQL = FormatSQL("select distinct movie.c%02d from movie", VIDEODB_ID_YEAR);
      }
    }
    else if (idContent == VIDEODB_CONTENT_TVSHOWS)
    {
      if (g_settings.m_vecProfiles[0].getLockMode() != LOCK_MODE_EVERYONE && !g_passwordManager.bMasterUser)
        strSQL = FormatSQL("select tvshow.c%02d,path.strPath from tvshow,files,path,tvshowlinkepisode where tvshow.idShow = tvshowlinkepisode.idShow and files.idEpisode = tvshowlinkepisode.idEpisode and files.idPath = path.idPath", VIDEODB_ID_YEAR);
      else
        strSQL = FormatSQL("select distinct tvshow.c%02d from tvshow", VIDEODB_ID_YEAR);
    }     

    strSQL += addParam;

    // run query
    CLog::Log(LOGDEBUG, "CVideoDatabase::GetYearsNav() query: %s", strSQL.c_str());
    if (!m_pDS->query(strSQL.c_str())) return false;
    int iRowsFound = m_pDS->num_rows();
    if (iRowsFound == 0)
    {
      m_pDS->close();
      return true;
    }

    if (g_settings.m_vecProfiles[0].getLockMode() != LOCK_MODE_EVERYONE && !g_passwordManager.bMasterUser)
    {
      map<long, CStdString> mapYears;
      map<long, CStdString>::iterator it;
      long lLastPathId = -1;
      while (!m_pDS->eof())
      {
        long lYear = m_pDS->fv(0).get_asLong();
        it = mapYears.find(lYear);
        if (it == mapYears.end())
        {
          // check path
          if (g_passwordManager.IsDatabasePathUnlocked(CStdString(m_pDS->fv("path.strPath").get_asString()),g_settings.m_vecMyVideoShares))
          {
            CStdString year;
            year.Format("%d", lYear);
            mapYears.insert(pair<long, CStdString>(lYear, year));
          }
        }
        m_pDS->next();
      }
      m_pDS->close();
    
      for (it=mapYears.begin();it != mapYears.end();++it)
      {
        CFileItem* pItem=new CFileItem(it->second);
        CStdString strDir;
        strDir.Format("%ld/", it->first);
        pItem->m_strPath=strBaseDir + strDir;
        pItem->m_bIsFolder=true;
        items.Add(pItem);
      }
    }
    else
    {
      while (!m_pDS->eof())
      {
        long lYear = m_pDS->fv(0).get_asLong();
        CFileItem* pItem=new CFileItem(m_pDS->fv(0).get_asString());
        CStdString strDir;
        strDir.Format("%ld/", lYear);
        pItem->m_strPath=strBaseDir + strDir;
        pItem->m_bIsFolder=true;
        items.Add(pItem);
        m_pDS->next();
      }
      m_pDS->close();
    }

    return true;
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "CVideoDatabase:GetYearsNav() failed");
  }
  return false;
}

bool CVideoDatabase::GetSeasonsNav(const CStdString& strBaseDir, CFileItemList& items, long idActor, long idDirector, long idGenre, long idYear, long idShow)
{
  try
  {
    if (NULL == m_pDB.get()) return false;
    if (NULL == m_pDS.get()) return false;

    CStdString strSQL;
    CStdString addParam;
  
    strSQL = FormatSQL("select distinct episode.c%02d from episode join tvshow on tvshow.idshow=tvshowlinkepisode.idshow join tvshowlinkepisode on tvshowlinkepisode.idepisode=episode.idepisode where tvshow.idshow=%u", VIDEODB_ID_EPISODE_SEASON,idShow);

    if (idYear != -1)
      strSQL += FormatSQL(" and tvshow.c%02d=%u",VIDEODB_ID_TV_YEAR,idYear);

    if (idActor != -1)
      strSQL = FormatSQL("select distinct episode.c%02d from episode join tvshow on tvshow.idshow=tvshowlinkepisode.idshow join tvshowlinkepisode on tvshowlinkepisode.idepisode = episode.idepisode join actorlinktvshow on actorlinktvshow.idshow=tvshow.idshow where tvshow.idshow=%u and actorlinktvshow.idActor=%u",VIDEODB_ID_EPISODE_SEASON,idShow,idActor);

    if (idDirector != -1)
      strSQL = FormatSQL("select distinct episode.c%02d from episode join tvshow on tvshow.idshow=tvshowlinkepisode.idshow join directorlinktvshow on directorlinktvshow.idshow=tvshow.idshow where tvshow.idshow=%u and directorlinktvshow.idDirector=%u",VIDEODB_ID_EPISODE_SEASON,idShow,idDirector);

    if (idGenre != -1)
      strSQL = FormatSQL("select distinct episode.c%02d from episode join tvshow on tvshow.idshow=tvshowlinkepisode.idshow join tvshowlinkepisode on tvshowlinkepisode.idepisode = episode.idepisode join genrelinktvshow on genrelinktvshow.idshow=tvshow.idshow where tvshow.idshow=%u and genrelinktvshow.idGenre=%u",VIDEODB_ID_EPISODE_SEASON,idShow,idGenre);

    // run query
    CLog::Log(LOGDEBUG, __FUNCTION__" query: %s", strSQL.c_str());
    if (!m_pDS->query(strSQL.c_str())) return false;
    int iRowsFound = m_pDS->num_rows();
    if (iRowsFound == 0)
    {
      m_pDS->close();
      return true;
    }

    if (g_settings.m_vecProfiles[0].getLockMode() != LOCK_MODE_EVERYONE && !g_passwordManager.bMasterUser)
    {
      map<long, CStdString> mapYears;
      map<long, CStdString>::iterator it;
      long lLastPathId = -1;
      while (!m_pDS->eof())
      {
        long lYear = m_pDS->fv(0).get_asLong();
        it = mapYears.find(lYear);
        if (it == mapYears.end())
        {
          // check path
          if (g_passwordManager.IsDatabasePathUnlocked(CStdString(m_pDS->fv("path.strPath").get_asString()),g_settings.m_vecMyVideoShares))
          {
            CStdString year;
            year.Format("%d", lYear);
            mapYears.insert(pair<long, CStdString>(lYear, year));
          }
        }
        m_pDS->next();
      }
      m_pDS->close();
    
      for (it=mapYears.begin();it != mapYears.end();++it)
      {
        long lSeason = m_pDS->fv(0).get_asLong();
        CStdString strLabel;
        strLabel.Format(g_localizeStrings.Get(20358),it->second);
        CFileItem* pItem=new CFileItem(strLabel);
        CStdString strDir;
        strDir.Format("%ld/", it->first);
        pItem->m_strPath=strBaseDir + strDir;
        pItem->m_bIsFolder=true;
        items.Add(pItem);
      }
    }
    else
    {
      while (!m_pDS->eof())
      {
        long lSeason = m_pDS->fv(0).get_asLong();
        CStdString strLabel;
        strLabel.Format(g_localizeStrings.Get(20358),lSeason);
        CFileItem* pItem=new CFileItem(strLabel);
        CStdString strDir;
        strDir.Format("%ld/", lSeason);
        pItem->m_strPath=strBaseDir + strDir;
        pItem->m_bIsFolder=true;
        items.Add(pItem);
        m_pDS->next();
      }
      m_pDS->close();
    }

    return true;
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "CVideoDatabase:GetSeasonsNav() failed");
  }
  return false;
}

bool CVideoDatabase::GetTitlesNav(const CStdString& strBaseDir, CFileItemList& items, long idGenre, long idYear, long idActor, long idDirector)
{
  try
  {
    DWORD time = timeGetTime();
	  movieTime = 0;
	  castTime = 0;
	
    if (NULL == m_pDB.get()) return false;
    if (NULL == m_pDS.get()) return false;

    CStdString strSQL = "select movie.*,files.strFileName,path.strPath from movie join files on files.idMovie=movie.idMovie join path on files.idPath=path.idPath";

    if (idGenre != -1)
    {
      strSQL=FormatSQL("select movie.*,files.strFileName,path.strPath from movie join files on files.idMovie=movie.idMovie join path on files.idPath=path.idPath join genrelinkmovie on genrelinkmovie.idmovie=movie.idmovie where genrelinkmovie.idGenre=%u", idGenre);
    }

    if (idDirector != -1)
    {
      strSQL=FormatSQL("select movie.*,files.strFileName,path.strPath from movie join files on files.idMovie=movie.idMovie join path on files.idPath=path.idPath join directorlinkmovie on directorlinkmovie.idmovie=movie.idmovie where directorlinkmovie.idDirector=%u", idDirector);
    }

    if (idYear !=-1)
    {
      strSQL=FormatSQL("select movie.*,files.strFileName,path.strPath from movie join files on files.idMovie=movie.idMovie join path on files.idPath=path.idPath where c%02d='%i'",VIDEODB_ID_YEAR,idYear);
    }

    if (idActor != -1)
    {
      strSQL=FormatSQL("select movie.*,files.strFileName,path.strPath from movie join files on files.idMovie=movie.idMovie join path on files.idPath=path.idPath join actorlinkmovie on actorlinkmovie.idmovie=movie.idmovie join actors on actors.idActor=actorlinkmovie.idActor where actors.idActor=%u",idActor);
    }

    // get all songs out of the database in fixed size chunks
    // dont reserve the items ahead of time just in case it fails part way though
    VECMOVIES movies;
    if (idGenre == -1 && idYear == -1 && idActor == -1 && idDirector == -1)
    {
      int iLIMIT = 5000;    // chunk size
      int iSONGS = 0;       // number of movies added to items
      int iITERATIONS = 0;  // number of iterations
      
      for (int i=0;;i+=iLIMIT)
      {
        CStdString strSQL2=strSQL+FormatSQL(" limit %i offset %i", iLIMIT, i);
        CLog::Log(LOGDEBUG, "CVideoDatabase::GetTitlesNav() query: %s", strSQL2.c_str());
        try
        {
          if (!m_pDS->query(strSQL2.c_str()))
            return false;

          // keep going until no rows are left!
          int iRowsFound = m_pDS->num_rows();
          if (iRowsFound == 0)
          {
            m_pDS->close();
            if (iITERATIONS == 0)
              return true; // failed on first iteration, so there's probably no songs in the db
            else
            {
              CGUIWindowVideoBase::SetDatabaseDirectory(movies,items);
              return true; // there no more songs left to process (aborts the unbounded for loop)
            }
          }

          CLog::DebugLog("Time for actual SQL query = %d", timeGetTime() - time); time = timeGetTime();
          // get movies from returned subtable
          while (!m_pDS->eof())
          {
            long lMovieId = m_pDS->fv("movie.idMovie").get_asLong();
            
            CIMDBMovie movie = GetDetailsForMovie(m_pDS);
            movies.push_back(movie);
            iSONGS++;
            m_pDS->next();
          }
          CLog::DebugLog("Time to retrieve movies from dataset = %d", timeGetTime() - time);
    		  CLog::Log(LOGDEBUG, __FUNCTION__" times: Info %d, Cast %d", movieTime, castTime);
        }
        catch (...)
        {
          CLog::Log(LOGERROR, "CVideoDatabase::GetTitlesNav() failed at iteration %i, num songs %i", iITERATIONS, iSONGS);

          if (iSONGS > 0)
          {
            CGUIWindowVideoBase::SetDatabaseDirectory(movies,items);
            return true; // keep whatever songs we may have gotten before the failure
          }
          else
            return true; // no songs, return false
        }
        // next iteration
        iITERATIONS++;
        m_pDS->close();
      }
      CGUIWindowVideoBase::SetDatabaseDirectory(movies,items);
      return true;
    }

    // run query
    CLog::Log(LOGDEBUG, "CVideoDatabase::GetTitlesNav() query: %s", strSQL.c_str());
    if (!m_pDS->query(strSQL.c_str())) return false;
    int iRowsFound = m_pDS->num_rows();
    if (iRowsFound == 0)
    {
      m_pDS->close();
      return true;
    }

    CLog::DebugLog("Time for actual SQL query = %d", timeGetTime() - time); time = timeGetTime();

    // get data from returned rows
    items.Reserve(iRowsFound);

    while (!m_pDS->eof())
    {
      CIMDBMovie movie = GetDetailsForMovie(m_pDS);
      movies.push_back(movie);

      m_pDS->next();
    }

    CLog::DebugLog("Time to retrieve movies from dataset = %d", timeGetTime() - time);

    // cleanup
    m_pDS->close();
    CGUIWindowVideoBase::SetDatabaseDirectory(movies,items);
    return true;
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "CVideoDatabase::GetTitlesNav() failed");
  }
  return false;
}

bool CVideoDatabase::GetTvShowsNav(const CStdString& strBaseDir, CFileItemList& items, long idGenre, long idYear, long idActor, long idDirector)
{
  try
  {
    DWORD time = timeGetTime();
	  movieTime = 0;
	  castTime = 0;
	
    if (NULL == m_pDB.get()) return false;
    if (NULL == m_pDS.get()) return false;

    CStdString strSQL = FormatSQL("select tvshow.*,path.strPath,path.strPath from tvshow join tvshowlinkpath on tvshow.idShow=tvshowlinkpath.idShow join path on path.idpath=tvshowlinkpath.idPath");

    if (idGenre != -1)
    {
      strSQL=FormatSQL("select tvshow.*,path.strPath,path.strPath from tvshow join genrelinktvshow on genrelinktvshow.idshow=tvshow.idshow join tvshowlinkpath on tvshowlinkpath.idshow = tvshow.idshow join path on path.idpath=tvshowlinkpath.idpath where genrelinktvshow.idGenre=%u ", idGenre);
    }

    if (idDirector != -1)
    {
       strSQL=FormatSQL("select tvshow.*,path.strPath,path.strPath from tvshow join directorlinktvshow on directorlinktvshow.idshow=tvshow.idshow join tvshowlinkpath on tvshowlinkpath.idshow = tvshow.idshow join path on path.idpath=tvshowlinkpath.idpath where directorlinktvshow.idDirector=%u", idDirector);
    }

    if (idYear != -1)
    {
      strSQL=FormatSQL("select tvshow.*,path.strPath,path.strPath from tvshow,path,tvshowlinkpath where tvshow.c%02d=%i and path.idPath=tvshowlinkpath.idpath and tvshowlinkpath.idshow=tvshow.idshow",VIDEODB_ID_TV_YEAR,idYear);
    }

    if (idActor != -1)
    {
      strSQL=FormatSQL("select tvshow.*,path.strPath,path.strPath from tvshow join actorlinktvshow on actorlinktvshow.idshow=tvshow.idshow join actors on actors.idActor=actorlinktvshow.idActor join tvshowlinkpath on tvshowlinkpath.idshow = tvshow.idshow join path on tvshowlinkpath.idpath = path.idpath where actors.idActor=%u",idActor);
    }

    // get all songs out of the database in fixed size chunks
    // dont reserve the items ahead of time just in case it fails part way though
    VECMOVIES movies;
    if (idGenre == -1 && idYear == -1 && idActor == -1 && idDirector == -1)
    {
      int iLIMIT = 5000;    // chunk size
      int iSONGS = 0;       // number of movies added to items
      int iITERATIONS = 0;  // number of iterations
      
      for (int i=0;;i+=iLIMIT)
      {
        CStdString strSQL2=strSQL+FormatSQL(" limit %i offset %i", iLIMIT, i);
        CLog::Log(LOGDEBUG, "CVideoDatabase::GetTvShowsNav() query: %s", strSQL2.c_str());
        try
        {
          if (!m_pDS->query(strSQL2.c_str()))
            return false;

          // keep going until no rows are left!
          int iRowsFound = m_pDS->num_rows();
          if (iRowsFound == 0)
          {
//            m_pDS->close();
            if (iITERATIONS == 0)
              return true; // failed on first iteration, so there's probably no songs in the db
            else
            {
              CGUIWindowVideoBase::SetDatabaseDirectory(movies,items,true);
              return true; // there no more songs left to process (aborts the unbounded for loop)
            }
          }

          CLog::DebugLog("Time for actual SQL query = %d", timeGetTime() - time); time = timeGetTime();
          // get movies from returned subtable
          while (!m_pDS->eof())
          {
            CIMDBMovie movie = GetDetailsForTvShow(m_pDS, false, strBaseDir);
            CUtil::RemoveSlashAtEnd(movie.m_strPath);
            movies.push_back(movie);
            iSONGS++;
            m_pDS->next();
          }
          CLog::DebugLog("Time to retrieve movies from dataset = %d", timeGetTime() - time);
    		  CLog::Log(LOGDEBUG, __FUNCTION__" times: Info %d, Cast %d", movieTime, castTime);
        }
        catch (...)
        {
          CLog::Log(LOGERROR, "CVideoDatabase::GetTvShowNav() failed at iteration %i, num songs %i", iITERATIONS, iSONGS);

          if (iSONGS > 0)
          {
            CGUIWindowVideoBase::SetDatabaseDirectory(movies,items, true);
            return true; // keep whatever songs we may have gotten before the failure
          }
          else
            return true; // no songs, return false
        }
        // next iteration
        iITERATIONS++;
        m_pDS->close();
      }
      CGUIWindowVideoBase::SetDatabaseDirectory(movies,items, true);
      return true;
    }

    // run query
    CLog::Log(LOGDEBUG, "CVideoDatabase::GetTvShowsNav() query: %s", strSQL.c_str());
    if (!m_pDS->query(strSQL.c_str())) return false;
    int iRowsFound = m_pDS->num_rows();
    if (iRowsFound == 0)
    {
      m_pDS->close();
      return true;
    }

    CLog::DebugLog("Time for actual SQL query = %d", timeGetTime() - time); time = timeGetTime();

    // get data from returned rows
    items.Reserve(iRowsFound);

    while (!m_pDS->eof())
    {
      CIMDBMovie movie = GetDetailsForTvShow(m_pDS, false, strBaseDir);
      movies.push_back(movie);

      m_pDS->next();
    }

    CLog::DebugLog("Time to retrieve movies from dataset = %d", timeGetTime() - time);

    // cleanup
    m_pDS->close();
    CGUIWindowVideoBase::SetDatabaseDirectory(movies,items,true);
    return true;
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "CVideoDatabase::GetTvShowsNav() failed");
  }
  return false;
}

bool CVideoDatabase::GetEpisodesNav(const CStdString& strBaseDir, CFileItemList& items, long idGenre, long idYear, long idActor, long idDirector, long idShow, long idSeason)
{
  try
  {
    DWORD time = timeGetTime();
	  movieTime = 0;
	  castTime = 0;
	
    if (NULL == m_pDB.get()) return false;
    if (NULL == m_pDS.get()) return false;

    CStdString strSQL = FormatSQL("select episode.*,files.strFileName,path.strPath from episode join files on files.idEpisode=tvshowlinkepisode.idepisode join tvshowlinkepisode on episode.idepisode=tvshowlinkepisode.idepisode join path on files.idPath=path.idPath where tvshowlinkepisode.idShow=%u",idShow);

    if (idGenre != -1)
    {
      strSQL = FormatSQL("select episode.*,files.strFileName,path.strPath from episode join files on files.idEpisode=tvshowlinkepisode.idepisode join tvshowlinkepisode on tvshow.idshow=tvshowlinkepisode.idshow join path on files.idPath=path.idPath join genrelinktvshow on genrelinktvshow.idshow = tvshow.idshow join tvshow on tvshowlinkepisode.idepisode=episode.idepisode where tvshowlinkepisode.idShow=%u and genrelinktvshow.idgenre=%u",idShow,idGenre);
    }

    if (idDirector != -1)
    {
      strSQL = FormatSQL("select episode.*,files.strFileName,path.strPath from episode join files on files.idEpisode=tvshowlinkepisode.idepisode join tvshowlinkepisode on tvshow.idshow=tvshowlinkepisode.idshow join path on files.idPath=path.idPath join directorlinktvshow on directorlinktvshow.idshow = tvshow.idshow where tvshow.idShow=%u and directorlinktvshow.iddirector=%u",idShow,idDirector);
    }

    if (idYear !=-1)
    {
      strSQL=FormatSQL("select episode.*,files.strFileName,path.strPath from episode join files on files.idEpisode=tvshowlinkepisode.idepisode join tvshowlinkepisode on episode.idepisode=tvshowlinkepisode.idepisode join path on files.idPath=path.idPath join tvshow on tvshowlinkepisode.idshow=tvshow.idshow where tvshowlinkepisode.idShow=%u and tvshow.c%02d=%u",idShow,VIDEODB_ID_TV_YEAR,idYear);
    }

    if (idActor != -1)
    {
      strSQL = FormatSQL("select episode.*,files.strFileName,path.strPath from episode join files on files.idEpisode=tvshowlinkepisode.idepisode join tvshowlinkepisode on episode.idepisode=tvshowlinkepisode.idepisode join tvshow on tvshowlinkepisode.idshow=tvshow.idshow join path on files.idPath=path.idPath join actorlinktvshow on actorlinktvshow.idshow = tvshow.idshow where tvshow.idShow=%u and actorlinktvshow.idactor=%u",idShow,idActor);
    }

    if (idSeason != -1)
      strSQL += FormatSQL(" and episode.c%02d=%u",VIDEODB_ID_EPISODE_SEASON,idSeason);

    // get all songs out of the database in fixed size chunks
    // dont reserve the items ahead of time just in case it fails part way though
    VECMOVIES movies;
    if (idGenre == -1 && idYear == -1 && idActor == -1 && idDirector == -1)
    {
      int iLIMIT = 5000;    // chunk size
      int iSONGS = 0;       // number of movies added to items
      int iITERATIONS = 0;  // number of iterations
      
      for (int i=0;;i+=iLIMIT)
      {
        CStdString strSQL2=strSQL+FormatSQL(" limit %i offset %i", iLIMIT, i);
        CLog::Log(LOGDEBUG, "CVideoDatabase::GetEpisodesNav() query: %s", strSQL2.c_str());
        try
        {
          if (!m_pDS->query(strSQL2.c_str()))
            return false;

          // keep going until no rows are left!
          int iRowsFound = m_pDS->num_rows();
          if (iRowsFound == 0)
          {
            m_pDS->close();
            if (iITERATIONS == 0)
              return true; // failed on first iteration, so there's probably no songs in the db
            else
            {
              CGUIWindowVideoBase::SetDatabaseDirectory(movies,items);
              return true; // there no more songs left to process (aborts the unbounded for loop)
            }
          }

          CLog::DebugLog("Time for actual SQL query = %d", timeGetTime() - time); time = timeGetTime();
          // get movies from returned subtable
          while (!m_pDS->eof())
          {
            long lEpisodeId = m_pDS->fv("episode.idepisode").get_asLong();
            
            CIMDBMovie movie = GetDetailsForEpisode(m_pDS);
            movies.push_back(movie);
            iSONGS++;
            m_pDS->next();
          }
          CLog::DebugLog("Time to retrieve movies from dataset = %d", timeGetTime() - time);
    		  CLog::Log(LOGDEBUG, __FUNCTION__" times: Info %d, Cast %d", movieTime, castTime);
        }
        catch (...)
        {
          CLog::Log(LOGERROR, "CVideoDatabase::GetEpisodesNav() failed at iteration %i, num songs %i", iITERATIONS, iSONGS);

          if (iSONGS > 0)
          {
            CGUIWindowVideoBase::SetDatabaseDirectory(movies,items);
            return true; // keep whatever songs we may have gotten before the failure
          }
          else
            return true; // no songs, return false
        }
        // next iteration
        iITERATIONS++;
        m_pDS->close();
      }
      CGUIWindowVideoBase::SetDatabaseDirectory(movies,items);
      return true;
    }

    // run query
    CLog::Log(LOGDEBUG, "CVideoDatabase::GetEpisodesNav() query: %s", strSQL.c_str());
    if (!m_pDS->query(strSQL.c_str())) return false;
    int iRowsFound = m_pDS->num_rows();
    if (iRowsFound == 0)
    {
      m_pDS->close();
      return true;
    }

    CLog::DebugLog("Time for actual SQL query = %d", timeGetTime() - time); time = timeGetTime();

    // get data from returned rows
    items.Reserve(iRowsFound);

    while (!m_pDS->eof())
    {
      CIMDBMovie movie = GetDetailsForEpisode(m_pDS);
      movies.push_back(movie);

      m_pDS->next();
    }

    CLog::DebugLog("Time to retrieve movies from dataset = %d", timeGetTime() - time);

    // cleanup
    m_pDS->close();
    CGUIWindowVideoBase::SetDatabaseDirectory(movies,items);
    return true;
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "CVideoDatabase::GetEpisodesNav() failed");
  }
  return false;
}

bool CVideoDatabase::GetGenreById(long lIdGenre, CStdString& strGenre)
{
  try
  {
    if (NULL == m_pDB.get()) return false;
    if (NULL == m_pDS.get()) return false;

    CStdString strSQL=FormatSQL("select genre.strGenre from genre where genre.idGenre=%u", lIdGenre);
    m_pDS->query( strSQL.c_str() );

    bool bResult = false;
    if (!m_pDS->eof())
    {
      strGenre  = m_pDS->fv("genre.strGenre").get_asString();
      bResult = true;
    }
    m_pDS->close();
    return bResult;
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "CVideoDatabase::GetMoviesByGenre(%s) failed", strGenre.c_str());
  }
  return false;
}

int CVideoDatabase::GetMovieCount()
{
  try
  {
    if (NULL == m_pDB.get()) return 0;
    if (NULL == m_pDS.get()) return 0;

    CStdString strSQL=FormatSQL("select count (idMovie) as nummovies from movie");
    m_pDS->query( strSQL.c_str() );

    int iResult = 0;
    if (!m_pDS->eof())
    {
      iResult = m_pDS->fv("nummovies").get_asInteger();
    }
    m_pDS->close();
    return iResult;
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "CVideoDatabase::GetMovieCount() failed");
  }
  return 0;
}

int CVideoDatabase::GetTvShowCount()
{
  try
  {
    if (NULL == m_pDB.get()) return 0;
    if (NULL == m_pDS.get()) return 0;

    CStdString strSQL=FormatSQL("select count (idShow) as numshows from tvshow");
    m_pDS->query( strSQL.c_str() );

    int iResult = 0;
    if (!m_pDS->eof())
    {
      iResult = m_pDS->fv("numshows").get_asInteger();
    }
    m_pDS->close();
    return iResult;
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "CVideoDatabase::GetTvShowCount() failed");
  }
  return 0;
}


bool CVideoDatabase::GetScraperForPath(const CStdString& strPath, CStdString& strScraper, CStdString& strContent)
{
  int iDummy;
  return GetScraperForPath(strPath,strScraper,strContent,iDummy);
}

bool CVideoDatabase::GetScraperForPath(const CStdString& strPath, CStdString& strScraper, CStdString& strContent, int& iFound)
{
  try
  {
    if (strPath.IsEmpty()) return false;
    if (NULL == m_pDB.get()) return false;
    if (NULL == m_pDS.get()) return false;

    CStdString strPath1(strPath);
    CUtil::RemoveSlashAtEnd(strPath1);
    CStdString strSQL=FormatSQL("select path.strContent,path.strScraper from path where strPath like '%s'",strPath1.c_str());
    m_pDS->query( strSQL.c_str() );

    iFound = 0;
    if (!m_pDS->eof())
    {
      strContent = m_pDS->fv("path.strContent").get_asString();
      strScraper = m_pDS->fv("path.strScraper").get_asString();
      if (!strScraper.IsEmpty())
        iFound = 1;
    }
    if (iFound == 0)
    {
      CStdString strParent;
      bool bIsBookMark=false;
      int iBookMark=-2;
      while (iFound == 0 && CUtil::GetParentPath(strPath1, strParent))
      {
        CUtil::RemoveSlashAtEnd(strParent);
        strSQL=FormatSQL("select path.strContent,path.strScraper from path where strPath like '%s'",strParent.c_str());
        m_pDS->query(strSQL.c_str());
        if (m_pDS->eof())
        {
          if (iBookMark == -2)
          {
            iBookMark = CUtil::GetMatchingShare(strParent,g_settings.m_vecMyVideoShares,bIsBookMark);
            bIsBookMark = g_settings.m_vecMyVideoShares[iBookMark].vecPaths.size() > 1;
          }
          if (iBookMark > -1 && bIsBookMark)
          {
            for (unsigned int i=0;i<g_settings.m_vecMyVideoShares[iBookMark].vecPaths.size();++i)
            {
              if (g_settings.m_vecMyVideoShares[iBookMark].vecPaths[i].Equals(strParent))
              {
                strSQL=strSQL=FormatSQL("select path.strContent,path.strScraper from path where strPath like '%s'",g_settings.m_vecMyVideoShares[iBookMark].strPath.c_str());
                m_pDS->query(strSQL.c_str());
                break;
              }
            }
          }
        }
        if (!m_pDS->eof())
        {
          strContent = m_pDS->fv("path.strContent").get_asString();
          strScraper = m_pDS->fv("path.strScraper").get_asString();
          if (!strScraper.IsEmpty())
            iFound = 2;
        }

        strPath1 = strParent;
      }
    }
    m_pDS->close();
    return iFound != 0;
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "CVideoDatabase::GetScraperForPath() failed");
  }
  return false;
}

void CVideoDatabase::GetMovieGenresByName(const CStdString& strSearch, CFileItemList& items)
{
  CStdString strSQL;

  try
  {
    if (NULL == m_pDB.get()) return;
    if (NULL == m_pDS.get()) return;

    if (g_settings.m_vecProfiles[0].getLockMode() != LOCK_MODE_EVERYONE && !g_passwordManager.bMasterUser)
      strSQL=FormatSQL("select genre.idgenre,genre.strgenre,path.strPath from genre,genrelinkmovie,movie,path,files where genre.idGenre=genrelinkMovie.idGenre and genrelinkMovie.idMovie = movie.idMovie and files.idMovie=movie.idMovie and path.idPath = files.idPath and genre.strGenre like '%%%s%%'",strSearch.c_str());
    else
      strSQL=FormatSQL("select genre.idgenre,genre.strgenre from genre,genrelinkmovie where genrelinkmovie.idgenre=genre.idgenre and strGenre like '%%%s%%'", strSearch.c_str());
    m_pDS->query( strSQL.c_str() );

    while (!m_pDS->eof())
    {
      if (g_settings.m_vecProfiles[0].getLockMode() != LOCK_MODE_EVERYONE && !g_passwordManager.bMasterUser)
        if (!g_passwordManager.IsDatabasePathUnlocked(CStdString(m_pDS->fv("path.strPath").get_asString()),g_settings.m_vecMyVideoShares))
        {
          m_pDS->next();
          continue;
        }

      CFileItem* pItem=new CFileItem(m_pDS->fv("genre.strGenre").get_asString());
      CStdString strDir;
      strDir.Format("%ld/", m_pDS->fv("genre.idGenre").get_asLong());
      pItem->m_strPath="videodb://1/1/"+ strDir;
      pItem->m_bIsFolder=true;
      items.Add(pItem);
      m_pDS->next();
    }
    m_pDS->close();
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "CVideoDatabase::GetMovieGenresByName(%s) failed",strSQL.c_str());
  }
}

void CVideoDatabase::GetTvShowGenresByName(const CStdString& strSearch, CFileItemList& items)
{
  CStdString strSQL;

  try
  {
    if (NULL == m_pDB.get()) return;
    if (NULL == m_pDS.get()) return;

    if (g_settings.m_vecProfiles[0].getLockMode() != LOCK_MODE_EVERYONE && !g_passwordManager.bMasterUser)
      strSQL=FormatSQL("select genre.idgenre,genre.strgenre,path.strPath from genre,genrelinktvshow,tvshow,path,tvshowlinkpath where genre.idGenre=genrelinktvshow.idGenre and genrelinktvshow.idshow = tvshow.idshow and path.idPath = tvshowlinkpath.idPath and tvshowlinkpath.idshow = tvshow.idshow and genre.strGenre like '%%%s%%'",strSearch.c_str());
    else
      strSQL=FormatSQL("select genre.idgenre,genre.strgenre from genre,genrelinktvshow where genrelinktvshow.idgenre=genre.idgenre and strGenre like '%%%s%%'", strSearch.c_str());
    m_pDS->query( strSQL.c_str() );

    while (!m_pDS->eof())
    {
      if (g_settings.m_vecProfiles[0].getLockMode() != LOCK_MODE_EVERYONE && !g_passwordManager.bMasterUser)
        if (!g_passwordManager.IsDatabasePathUnlocked(CStdString(m_pDS->fv("path.strPath").get_asString()),g_settings.m_vecMyVideoShares))
        {
          m_pDS->next();
          continue;
        }

      CFileItem* pItem=new CFileItem(m_pDS->fv("genre.strGenre").get_asString());
      CStdString strDir;
      strDir.Format("%ld/", m_pDS->fv("genre.idGenre").get_asLong());
      pItem->m_strPath="videodb://1/1/"+ strDir;
      pItem->m_bIsFolder=true;
      items.Add(pItem);
      m_pDS->next();
    }
    m_pDS->close();
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "CVideoDatabase::GetMovieGenresByName(%s) failed",strSQL.c_str());
  }
}

void CVideoDatabase::GetMovieActorsByName(const CStdString& strSearch, CFileItemList& items)
{
  CStdString strSQL;

  try
  {
    if (NULL == m_pDB.get()) return;
    if (NULL == m_pDS.get()) return;

    if (g_settings.m_vecProfiles[0].getLockMode() != LOCK_MODE_EVERYONE && !g_passwordManager.bMasterUser)
      strSQL=FormatSQL("select actors.idactor,actors.strActor,path.strPath from actorlinkmovie,actors,movie,files,path where actors.idActor=actorlinkmovie.idActor and actorlinkmovie.idmovie=movie.idmovie and files.idMovie = movie.idMovie and files.idPath = path.idPath and actors.strActor like '%%%s%%'",strSearch.c_str());
    else
      strSQL=FormatSQL("select actors.idactor,actors.strActor from actorlinkmovie,actors,movie where actors.idActor=actorlinkmovie.idActor and actorlinkmovie.idmovie=movie.idmovie and actors.strActor like '%%%s%%'",strSearch.c_str());
    m_pDS->query( strSQL.c_str() );

    while (!m_pDS->eof())
    {
      if (g_settings.m_vecProfiles[0].getLockMode() != LOCK_MODE_EVERYONE && !g_passwordManager.bMasterUser)
        if (!g_passwordManager.IsDatabasePathUnlocked(CStdString(m_pDS->fv("path.strPath").get_asString()),g_settings.m_vecMyVideoShares))
        {
          m_pDS->next();
          continue;
        }

      CFileItem* pItem=new CFileItem(m_pDS->fv("actors.strActor").get_asString());
      CStdString strDir;
      strDir.Format("%ld/", m_pDS->fv("actors.idActor").get_asLong());
      pItem->m_strPath="videodb://1/4/"+ strDir;
      pItem->m_bIsFolder=true;
      items.Add(pItem);
      m_pDS->next();
    }
    m_pDS->close();
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "CVideoDatabase::GetGenresByName(%s) failed",strSQL.c_str());
  }
}

void CVideoDatabase::GetTvShowsActorsByName(const CStdString& strSearch, CFileItemList& items)
{
  CStdString strSQL;

  try
  {
    if (NULL == m_pDB.get()) return;
    if (NULL == m_pDS.get()) return;

    if (g_settings.m_vecProfiles[0].getLockMode() != LOCK_MODE_EVERYONE && !g_passwordManager.bMasterUser)
      strSQL=FormatSQL("select actors.idactor,actors.strActor,path.strPath from actorlinktvshow,actors,tvshow,path,tvshowlinkpath where actors.idActor=actorlinktvshow.idActor and actorlinktvshow.idshow=tvshow.idshow and tvshowlinkpath.idpath=tvshow.idshow and tvshowlinkpath.idpath=path.idpath and actors.strActor like '%%%s%%'",strSearch.c_str());
    else
      strSQL=FormatSQL("select actors.idactor,actors.strActor from actorlinktvshow,actors,tvshow where actors.idActor=actorlinktvshow.idActor and actorlinktvshow.idshow=tvshow.idshow and actors.strActor like '%%%s%%'",strSearch.c_str());
    m_pDS->query( strSQL.c_str() );

    while (!m_pDS->eof())
    {
      if (g_settings.m_vecProfiles[0].getLockMode() != LOCK_MODE_EVERYONE && !g_passwordManager.bMasterUser)
        if (!g_passwordManager.IsDatabasePathUnlocked(CStdString(m_pDS->fv("path.strPath").get_asString()),g_settings.m_vecMyVideoShares))
        {
          m_pDS->next();
          continue;
        }

      CFileItem* pItem=new CFileItem(m_pDS->fv("actors.strActor").get_asString());
      CStdString strDir;
      strDir.Format("%ld/", m_pDS->fv("actors.idActor").get_asLong());
      pItem->m_strPath="videodb://2/4/"+ strDir;
      pItem->m_bIsFolder=true;
      items.Add(pItem);
      m_pDS->next();
    }
    m_pDS->close();
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "CVideoDatabase::GetTvShowsGenresByName(%s) failed",strSQL.c_str());
  }
}

void CVideoDatabase::GetMoviesByName(const CStdString& strSearch, CFileItemList& items)
{
  CStdString strSQL;

  try
  {
    if (NULL == m_pDB.get()) return;
    if (NULL == m_pDS.get()) return;

    if (g_settings.m_vecProfiles[0].getLockMode() != LOCK_MODE_EVERYONE && !g_passwordManager.bMasterUser)
      strSQL = FormatSQL("select movie.idmovie,movie.c%02d,path.strPath from movie,file,path where file.idMovie=movie.idMovie and file.idPath=path.idPath and movie.strValue like '%%%s%%'",VIDEODB_ID_TITLE,strSearch.c_str());
    else
      strSQL = FormatSQL("select movie.idmovie,movie.c%02d from movie where movie.c%02d like '%%%s%%'",VIDEODB_ID_TITLE,VIDEODB_ID_TITLE,strSearch.c_str());
    m_pDS->query( strSQL.c_str() );

    while (!m_pDS->eof())
    {
      if (g_settings.m_vecProfiles[0].getLockMode() != LOCK_MODE_EVERYONE && !g_passwordManager.bMasterUser)
        if (!g_passwordManager.IsDatabasePathUnlocked(CStdString(m_pDS->fv("path.strPath").get_asString()),g_settings.m_vecMyVideoShares))
        {
          m_pDS->next();
          continue;
        }

      CFileItem* pItem=new CFileItem(m_pDS->fv(1).get_asString());
      CStdString strDir;
      strSQL = FormatSQL("select genrelinkmovie.idgenre from genrelinkmovie where genrelinkmovie.idMovie=%u",m_pDS->fv("movie.idMovie").get_asLong());
      m_pDS2->query(strSQL);
      strDir.Format("1/1/%ld/%ld", m_pDS2->fv("genrelinkmovie.idGenre").get_asLong(),m_pDS->fv("movie.idMovie").get_asLong());
      
      pItem->m_strPath="videodb://"+ strDir;
      pItem->m_bIsFolder=true;
      items.Add(pItem);
      m_pDS->next();
    }
    m_pDS->close();
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "CVideoDatabase::GetTitlesByName(%s) failed",strSQL.c_str());
  }
}

void CVideoDatabase::GetTvShowsByName(const CStdString& strSearch, CFileItemList& items)
{
  CStdString strSQL;

  try
  {
    if (NULL == m_pDB.get()) return;
    if (NULL == m_pDS.get()) return;

    if (g_settings.m_vecProfiles[0].getLockMode() != LOCK_MODE_EVERYONE && !g_passwordManager.bMasterUser)
      strSQL = FormatSQL("select movie.idmovie,movie.c%02d,path.strPath from movie,file,path where file.idMovie=movie.idMovie and file.idPath=path.idPath and movie.strValue like '%%%s%%'",VIDEODB_ID_TITLE,strSearch.c_str());
    else
      strSQL = FormatSQL("select movie.idmovie,movie.c%02d from movie where movie.strValue like '%%%s%%'",VIDEODB_ID_TITLE,strSearch.c_str());
    m_pDS->query( strSQL.c_str() );

    while (!m_pDS->eof())
    {
      if (g_settings.m_vecProfiles[0].getLockMode() != LOCK_MODE_EVERYONE && !g_passwordManager.bMasterUser)
        if (!g_passwordManager.IsDatabasePathUnlocked(CStdString(m_pDS->fv("path.strPath").get_asString()),g_settings.m_vecMyVideoShares))
        {
          m_pDS->next();
          continue;
        }

      CFileItem* pItem=new CFileItem(m_pDS->fv(2).get_asString());
      CStdString strDir;
      strSQL = FormatSQL("select * from genrelinkmovie where genrelinkmovie.idMovie=%u",m_pDS->fv("movie.idMovie").get_asLong());
      m_pDS2->query(strSQL);
      strDir.Format("2/1/%ld/%ld", m_pDS2->fv("genre.idGenre").get_asLong(),m_pDS->fv("movie.idMovie").get_asLong());
      
      pItem->m_strPath="videodb://"+ strDir;
      pItem->m_bIsFolder=true;
      items.Add(pItem);
      m_pDS->next();
    }
    m_pDS->close();
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "CVideoDatabase::GetTitlesByName(%s) failed",strSQL.c_str());
  }
}

void CVideoDatabase::GetEpisodesByName(const CStdString& strSearch, CFileItemList& items)
{
  CStdString strSQL;

  try
  {
    if (NULL == m_pDB.get()) return;
    if (NULL == m_pDS.get()) return;

    if (g_settings.m_vecProfiles[0].getLockMode() != LOCK_MODE_EVERYONE && !g_passwordManager.bMasterUser)
      strSQL = FormatSQL("select episode.idepisode,episode.c%02d,episode,c%02d,path.strPath,tvshowlinkepisode.idshow from episode,file,path,tvshowlinkepisode where file.idepisode=episode.idepisode and tvshowlinkepisode.idepisode=episode.idepisode and file.idPath=path.idPath and episode.c%02d like '%%%s%%'",VIDEODB_ID_EPISODE_TITLE,VIDEODB_ID_EPISODE_SEASON,VIDEODB_ID_EPISODE_TITLE,strSearch.c_str());
    else
      strSQL = FormatSQL("select episode.idepisode,episode.c%02d,episode.c%02d,tvshowlinkepisode.idshow from episode,tvshowlinkepisode where tvshowlinkepisode.idepisode=episode.idepisode and episode.c%02d like '%%%s%%'",VIDEODB_ID_EPISODE_TITLE,VIDEODB_ID_EPISODE_SEASON,VIDEODB_ID_EPISODE_TITLE,strSearch.c_str());
    m_pDS->query( strSQL.c_str() );

    while (!m_pDS->eof())
    {
      if (g_settings.m_vecProfiles[0].getLockMode() != LOCK_MODE_EVERYONE && !g_passwordManager.bMasterUser)
        if (!g_passwordManager.IsDatabasePathUnlocked(CStdString(m_pDS->fv("path.strPath").get_asString()),g_settings.m_vecMyVideoShares))
        {
          m_pDS->next();
          continue;
        }

      CFileItem* pItem=new CFileItem(m_pDS->fv(1).get_asString());
      pItem->m_strPath.Format("videodb://2/2/%ld/%ld/%ld",m_pDS->fv("tvshowlinkepisode.idshow").get_asLong(),m_pDS->fv(2).get_asLong(),m_pDS->fv(0).get_asLong());
      pItem->m_bIsFolder=false;
      items.Add(pItem);
      m_pDS->next();
    }
    m_pDS->close();
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "CVideoDatabase::GetEpisodesByName(%s) failed",strSQL.c_str());
  }
}

void CVideoDatabase::GetMovieDirectorsByName(const CStdString& strSearch, CFileItemList& items)
{
  CStdString strSQL;

  try
  {
    if (NULL == m_pDB.get()) return;
    if (NULL == m_pDS.get()) return;

    if (g_settings.m_vecProfiles[0].getLockMode() != LOCK_MODE_EVERYONE && !g_passwordManager.bMasterUser)
      strSQL = FormatSQL("select directorlinkmovie.idDirector,actors.strActor,path.strPath from movie,file,path,actors,directorlinkmovie where file.idMovie=movie.idMovie and file.idPath=path.idPath and directorlinkmovie.idmovie = movie.idMovie and directorlinkmovie.idDirector = actors.idActor and actors.strActor like '%%%s%%'",strSearch.c_str());
    else
      strSQL = FormatSQL("select directorlinkmovie.idDirector,actors.strActor from movie,actors,directorlinkmovie where directorlinkmovie.idmovie = movie.idMovie and directorlinkmovie.idDirector = actors.idActor and actors.strActor like '%%%s%%'",strSearch.c_str());

    m_pDS->query( strSQL.c_str() );

    while (!m_pDS->eof())
    {
      if (g_settings.m_vecProfiles[0].getLockMode() != LOCK_MODE_EVERYONE && !g_passwordManager.bMasterUser)
        if (!g_passwordManager.IsDatabasePathUnlocked(CStdString(m_pDS->fv("path.strPath").get_asString()),g_settings.m_vecMyVideoShares))
        {
          m_pDS->next();
          continue;
        }

      CStdString strDir;
      strDir.Format("%ld/", m_pDS->fv("directorlinkmovie.idDirector").get_asLong());
      CFileItem* pItem=new CFileItem(m_pDS->fv("actors.strActor").get_asString());
      
      pItem->m_strPath="videodb://1/5/"+ strDir;
      pItem->m_bIsFolder=true;
      items.Add(pItem);
      m_pDS->next();
    }
    m_pDS->close();
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "CVideoDatabase::GetGenresByName(%s) failed",strSQL.c_str());
  }
}

void CVideoDatabase::GetTvShowsDirectorsByName(const CStdString& strSearch, CFileItemList& items)
{
  CStdString strSQL;

  try
  {
    if (NULL == m_pDB.get()) return;
    if (NULL == m_pDS.get()) return;

    if (g_settings.m_vecProfiles[0].getLockMode() != LOCK_MODE_EVERYONE && !g_passwordManager.bMasterUser)
      strSQL = FormatSQL("select directorlinktvshow.idDirector,actors.strActor,path.strPath from tvshow,path,actors,directorlinktvshow,tvshowlinkpath where tvshowlinkpath.idPath=path.idPath and tvshowlinkpath.idshow=tvshow.idshow and directorlinktvshow.idshow = tvshow.idshow and directorlinktvshow.idDirector = actors.idActor and actors.strActor like '%%%s%%'",strSearch.c_str());
    else
      strSQL = FormatSQL("select directorlinktvshow.idDirector,actors.strActor from tvshow,actors,directorlinktvshow where directorlinktvshow.idshow = tvshow.idshow and directorlinktvshow.idDirector = actors.idActor and actors.strActor like '%%%s%%'",strSearch.c_str());

    m_pDS->query( strSQL.c_str() );

    while (!m_pDS->eof())
    {
      if (g_settings.m_vecProfiles[0].getLockMode() != LOCK_MODE_EVERYONE && !g_passwordManager.bMasterUser)
        if (!g_passwordManager.IsDatabasePathUnlocked(CStdString(m_pDS->fv("path.strPath").get_asString()),g_settings.m_vecMyVideoShares))
        {
          m_pDS->next();
          continue;
        }

      CStdString strDir;
      strDir.Format("%ld/", m_pDS->fv("directorlinktvshow.idDirector").get_asLong());
      CFileItem* pItem=new CFileItem(m_pDS->fv("actors.strActor").get_asString());
      
      pItem->m_strPath="videodb://2/5/"+ strDir;
      pItem->m_bIsFolder=true;
      items.Add(pItem);
      m_pDS->next();
    }
    m_pDS->close();
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "CVideoDatabase::GetGenresByName(%s) failed",strSQL.c_str());
  }
}

void CVideoDatabase::CleanDatabase()
{
  try
  {
    BeginTransaction();
    if (NULL == m_pDB.get()) return;
    if (NULL == m_pDS.get()) return;

    // find all the files
    CStdString sql = "select * from files, path where files.idPath = path.idPath";
 
    m_pDS->query(sql.c_str());
    if (m_pDS->num_rows() == 0) return;

    CGUIDialogProgress *progress = (CGUIDialogProgress *)m_gWindowManager.GetWindow(WINDOW_DIALOG_PROGRESS);
    if (progress)
    {
      progress->SetHeading(700);
      progress->SetLine(0, "");
      progress->SetLine(1, 313);
      progress->SetLine(2, 330);
      progress->SetPercentage(0);
      progress->StartModal();
      progress->ShowProgressBar(true);
    }

    CStdString filesToDelete = "(";
    CStdString moviesToDelete = "(";
    int total = m_pDS->num_rows();
    int current = 0;
    while (!m_pDS->eof())
    {
      CStdString path = m_pDS->fv("path.strPath").get_asString();
      CStdString fileName = m_pDS->fv("files.strFileName").get_asString();
      CStdString fullPath;
      CUtil::AddFileToFolder(path, fileName, fullPath);

      if (CUtil::IsStack(fullPath))
      { // do something?
        CStackDirectory dir;
        CFileItemList items;
        if (dir.GetDirectory(fullPath, items) && items.Size())
          fullPath = items[0]->m_strPath; // just test the first path
      }

      // delete all removable media + ftp/http streams
      CURL url(fullPath);
      if (CUtil::IsOnDVD(fullPath) ||
          CUtil::IsMemCard(fullPath) ||
          url.GetProtocol() == "http" ||
          url.GetProtocol() == "ftp" ||
          !CFile::Exists(fullPath))
      { // mark for deletion
        filesToDelete += m_pDS->fv("files.idFile").get_asString() + ",";
        moviesToDelete += m_pDS->fv("files.idMovie").get_asString() + ",";
      }
      if ((current % 50) == 0 && progress)
      {
        progress->SetPercentage(current * 100 / total);
        progress->Progress();
        if (progress->IsCanceled())
        {
          progress->Close();
          m_pDS->close();
          return;
        }
      }
      m_pDS->next();
      current++;
    }
    m_pDS->close();
    filesToDelete.TrimRight(",");
    filesToDelete += ")";
    moviesToDelete.TrimRight(",");
    moviesToDelete += ")";

    if (progress)
    {
      progress->SetPercentage(100);
      progress->Progress();
    }

    CLog::Log(LOGDEBUG, __FUNCTION__" Cleaning files table");
    sql = "delete from files where idFile in " + filesToDelete;
    m_pDS->exec(sql.c_str());

    CLog::Log(LOGDEBUG, __FUNCTION__" Cleaning bookmark table");
    sql = "delete from bookmark where idFile in " + filesToDelete;
    m_pDS->exec(sql.c_str());

    CLog::Log(LOGDEBUG, __FUNCTION__" Cleaning settings table");
    sql = "delete from settings where idFile in " + filesToDelete;
    m_pDS->exec(sql.c_str());

    CLog::Log(LOGDEBUG, __FUNCTION__" Cleaning stacktimes table");
    sql = "delete from stacktimes where idFile in " + filesToDelete;
    m_pDS->exec(sql.c_str());

    CLog::Log(LOGDEBUG, __FUNCTION__" Cleaning path table");
    sql = "delete from path where idPath not in (select distinct idPath from files)";
    m_pDS->exec(sql.c_str());

    CLog::Log(LOGDEBUG, __FUNCTION__" Cleaning movie table");
    sql = "delete from movie where idMovie in " + moviesToDelete;
    m_pDS->exec(sql.c_str());

    CLog::Log(LOGDEBUG, __FUNCTION__" Cleaning actorlinkmovie table");
    sql = "delete from actorlinkmovie where idMovie in " + moviesToDelete;
    m_pDS->exec(sql.c_str());

    CLog::Log(LOGDEBUG, __FUNCTION__" Cleaning genrelinkmovie table");
    sql = "delete from directorlinkmovie where idMovie in " + moviesToDelete;
    m_pDS->exec(sql.c_str());

    CLog::Log(LOGDEBUG, __FUNCTION__" Cleaning actor table of actors and directors");
    sql = "delete from actors where idActor not in (select distinct idActor from actorlinkmovie) and idActor not in (select distinct idDirector from directorlinkmovie)";
    m_pDS->exec(sql.c_str());

    CLog::Log(LOGDEBUG, __FUNCTION__" Cleaning genrelinkmovie table");
    sql = "delete from genrelinkmovie where idMovie in " + moviesToDelete;
    m_pDS->exec(sql.c_str());

    CLog::Log(LOGDEBUG, __FUNCTION__" Cleaning genre table");
    sql = "delete from genre where idGenre not in (select distinct idGenre from genrelinkmovie)";
    m_pDS->exec(sql.c_str());
 
    CommitTransaction();

    Compress();

    CUtil::DeleteVideoDatabaseDirectoryCache();

    if (progress)
      progress->Close(); 
 }
  catch (...)
  {
    CLog::Log(LOGERROR, "CVideoDatabase::CleanDatabase() failed");
  }
}

void CVideoDatabase::ExportToXML(const CStdString &xmlFile)
{
  try
  {
    if (NULL == m_pDB.get()) return;
    if (NULL == m_pDS.get()) return;

    // find all movies
    CStdString sql = "select movie.*,files.strFileName,path.strPath from movie join files on files.idMovie=movie.idMovie join path on files.idPath=path.idPath";
 
    m_pDS->query(sql.c_str());
    if (m_pDS->num_rows() == 0) return;

    CGUIDialogProgress *progress = (CGUIDialogProgress *)m_gWindowManager.GetWindow(WINDOW_DIALOG_PROGRESS);
    if (progress)
    {
      progress->SetHeading(647);
      progress->SetLine(0, 650);
      progress->SetLine(1, "");
      progress->SetLine(2, "");
      progress->SetPercentage(0);
      progress->StartModal();
      progress->ShowProgressBar(true);
    }

    int total = m_pDS->num_rows();
    int current = 0;

    // create our xml document
    TiXmlDocument xmlDoc;
    TiXmlElement xmlMainElement("videodb");
    TiXmlNode *pMain = xmlDoc.InsertEndChild(xmlMainElement);
    while (!m_pDS->eof())
    {
      CIMDBMovie movie = GetDetailsForMovie(m_pDS);
      movie.Save(pMain);
      if ((current % 50) == 0 && progress)
      {
        progress->SetLine(1, movie.m_strTitle);
        progress->SetPercentage(current * 100 / total);
        progress->Progress();
        if (progress->IsCanceled())
        {
          progress->Close();
          m_pDS->close();
          return;
        }
      }
      m_pDS->next();
      current++;
    }
    m_pDS->close();

    if (progress)
      progress->Close();

    xmlDoc.SaveFile(xmlFile.c_str());
  }
  catch (...)
  {
    CLog::Log(LOGERROR, __FUNCTION__" failed");
  }
}

void CVideoDatabase::ImportFromXML(const CStdString &xmlFile)
{
  try
  {
    if (NULL == m_pDB.get()) return;
    if (NULL == m_pDS.get()) return;

    TiXmlDocument xmlDoc;
    if (!xmlDoc.LoadFile(xmlFile))
      return;

    TiXmlElement *root = xmlDoc.RootElement();
    if (!root) return;

    CGUIDialogProgress *progress = (CGUIDialogProgress *)m_gWindowManager.GetWindow(WINDOW_DIALOG_PROGRESS);
    if (progress)
    {
      progress->SetHeading(648);
      progress->SetLine(0, 649);
      progress->SetLine(1, 330);
      progress->SetLine(2, "");
      progress->SetPercentage(0);
      progress->StartModal();
      progress->ShowProgressBar(true);
    }

    BeginTransaction();
    TiXmlElement *movie = root->FirstChildElement("movie");
    int current = 0;
    int total = 0;
    // first count the number of items...
    while (movie)
    {
      total++;
      movie = movie->NextSiblingElement("movie");
    }
    movie = root->FirstChildElement("movie");
    while (movie)
    {
      CIMDBMovie info;
      info.Load(movie);
      DeleteDetailsForMovie(info.m_strFileNameAndPath);
      SetDetailsForMovie(info.m_strFileNameAndPath, info);
      movie = movie->NextSiblingElement("movie");
      if (progress)
      {
        progress->SetPercentage(current * 100 / total);
        progress->SetLine(2, info.m_strTitle);
        progress->Progress();
        if (progress->IsCanceled())
        {
          progress->Close();
          RollbackTransaction();
          return;
        }
      }
      current++;
    }
    CommitTransaction();

    if (progress)
      progress->Close();
  }
  catch (...)
  {
    CLog::Log(LOGERROR, __FUNCTION__" failed");
  }
}

bool CVideoDatabase::GetArbitraryQuery(const CStdString& strQuery, const CStdString& strOpenRecordSet, const CStdString& strCloseRecordSet,
	const CStdString& strOpenRecord, const CStdString& strCloseRecord, const CStdString& strOpenField, const CStdString& strCloseField, CStdString& strResult)
{
  try
  {
    strResult = "";
	if (NULL == m_pDB.get()) return false;
	if (NULL == m_pDS.get()) return false;
	CStdString strSQL=FormatSQL(strQuery);
	if (!m_pDS->query(strSQL.c_str()))
	{
	  strResult = m_pDB->getErrorMsg();
	  return false;
	}
	int iRowsFound = m_pDS->num_rows();
	strResult=strOpenRecordSet;
	while (!m_pDS->eof())
	{
	  strResult += strOpenRecord;
	  for (int i=0; i<m_pDS->fieldCount(); i++)
	  {
	    strResult += strOpenField + m_pDS->fv(i).get_asString() + strCloseField;
	  }
	  strResult += strCloseRecord;
	  m_pDS->next();
	}
	strResult += strCloseRecordSet;
	m_pDS->close();
	return true;
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "CVideoDatabase:GetArbitraryQuery() failed");
  }
  try
  {
	if (NULL == m_pDB.get()) return false;
	strResult = m_pDB->getErrorMsg();
  }
  catch (...)
  {

  }

  return false;
}