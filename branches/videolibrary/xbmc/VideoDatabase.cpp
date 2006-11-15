//********************************************************************************************************************************
//**
//** see docs/videodatabase.png for a diagram of the database
//**
//********************************************************************************************************************************

#include "stdafx.h"
#include "videodatabase.h"
#include "GUIWindowVideoBase.h"
#include "utils/fstrcmp.h"
#include "util.h"
#include "GUIPassword.h"
#include "filesystem/VirtualPathDirectory.h"
#include "filesystem/StackDirectory.h"

#define VIDEO_DATABASE_VERSION 2.0f
#define VIDEO_DATABASE_NAME "MyVideos31.db"

CBookmark::CBookmark()
{
  timeInSeconds = 0.0f;
  type = STANDARD;
}

//********************************************************************************************************************************
CVideoDatabase::CVideoDatabase(void)
{
  m_fVersion=VIDEO_DATABASE_VERSION;
  m_strDatabaseFile=VIDEO_DATABASE_NAME;
}

//********************************************************************************************************************************
CVideoDatabase::~CVideoDatabase(void)
{}

//********************************************************************************************************************************
bool CVideoDatabase::CreateTables()
{

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

    CLog::Log(LOGINFO, "create stacktimes table");
    m_pDS->exec("CREATE TABLE stacktimes (idFile integer, usingConversions bool, times text)\n");

    CLog::Log(LOGINFO, "create genre table");
    m_pDS->exec("CREATE TABLE genre ( idGenre integer primary key, strGenre text)\n");

    CLog::Log(LOGINFO, "create genrelinkmovie table");
    m_pDS->exec("CREATE TABLE genrelinkmovie ( idGenre integer, idMovie integer)\n");

    CLog::Log(LOGINFO, "create movie table");
    m_pDS->exec("CREATE TABLE movie ( idMovie integer primary key, idPath integer, hasSubtitles integer, cdlabel text)\n");

    CLog::Log(LOGINFO, "create movieinfo table");
    //m_pDS->exec("CREATE TABLE movieinfo ( idMovie integer, idDirector integer, strPlotOutline text, strPlot text, strTagLine text, strVotes text, strRuntime text, fRating text, strCast text,strCredits text, iYear integer, strGenre text, strPictureURL text, strTitle text, IMDBID text)\n");
	  m_pDS->exec("CREATE TABLE movieinfo ( idMovie integer, idDirector integer, strPlotOutline text, strPlot text, strTagLine text, strVotes text, strRuntime text, fRating text, strCast text,strCredits text, iYear integer, strGenre text, strPictureURL text, strTitle text, IMDBID text, bWatched bool)\n");

    CLog::Log(LOGINFO, "create actorlinkmovie table");
    m_pDS->exec("CREATE TABLE actorlinkmovie ( idActor integer, idMovie integer )\n");

    CLog::Log(LOGINFO, "create actors table");
    m_pDS->exec("CREATE TABLE actors ( idActor integer primary key, strActor text )\n");

    CLog::Log(LOGINFO, "create path table");
    m_pDS->exec("CREATE TABLE path ( idPath integer primary key, strPath text)\n");

    CLog::Log(LOGINFO, "create files table");
    m_pDS->exec("CREATE TABLE files ( idFile integer primary key, idPath integer, idMovie integer,strFilename text)\n");
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "videodatabase::unable to create tables:%i", GetLastError());
    return false;
  }

  return true;
}

//********************************************************************************************************************************
long CVideoDatabase::AddFile(long lMovieId, long lPathId, const CStdString& strFileName)
{
  CStdString strSQL = "";
  try
  {
    long lFileId;
    if (NULL == m_pDB.get()) return -1;
    if (NULL == m_pDS.get()) return -1;

    strSQL=FormatSQL("select * from files where idmovie=%i and idpath=%i and strFileName like '%s'", lMovieId, lPathId, strFileName.c_str());
    m_pDS->query(strSQL.c_str());
    if (m_pDS->num_rows() > 0)
    {
      lFileId = m_pDS->fv("idFile").get_asLong() ;
      m_pDS->close();
      return lFileId;
    }
    m_pDS->close();
    strSQL=FormatSQL("insert into files (idFile, idMovie,idPath, strFileName) values(NULL, %i,%i,'%s')", lMovieId, lPathId, strFileName.c_str() );
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
long CVideoDatabase::GetFile(const CStdString& strFilenameAndPath, long &lPathId, long& lMovieId, bool bExact)
{
  try
  {
    lPathId = -1;
    lMovieId = -1;
    if (NULL == m_pDB.get()) return -1;
    if (NULL == m_pDS.get()) return -1;
    CStdString strPath, strFileName ;
    Split(strFilenameAndPath, strPath, strFileName);

#define NEW_VIDEO_DB_METHODS 1

#ifdef NEW_VIDEO_DB_METHODS
    CStdString strSQL;
    strSQL=FormatSQL("select * from files, path where path.idpath=files.idpath and path.strPath like '%s' and files.strFileName like '%s'", strPath.c_str(), strFileName.c_str());
    m_pDS->query(strSQL.c_str());
    if (m_pDS->num_rows() > 0)
    {
      long lFileId = m_pDS->fv("files.idFile").get_asLong();
      lMovieId = m_pDS->fv("files.idMovie").get_asLong();
      lPathId = m_pDS->fv("files.idPath").get_asLong();
      m_pDS->close();
      return lFileId;
    }
#else

    lPathId = GetPath(strPath);
    if (lPathId < 0) return -1;

    CStdString strSQL;
    strSQL=FormatSQL("select * from files where idpath=%i", lPathId);
    m_pDS->query(strSQL.c_str());
    if (m_pDS->num_rows() > 0)
    {
      while (!m_pDS->eof())
      {
        CStdString strFname = m_pDS->fv("strFilename").get_asString() ;
        if (strFname == strFileName)
        {
          // was just returning 'true' here, but this caused problems with
          // the bookmarks as these are stored by fileid. forza.
          long lFileId = m_pDS->fv("idFile").get_asLong() ;
          lMovieId = m_pDS->fv("idMovie").get_asLong() ;
          m_pDS->close();
          return lFileId;
          //return true;
        }
        m_pDS->next();
      }
      m_pDS->close();
    }
#endif
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "CVideoDatabase::GetFile(%s) failed", strFilenameAndPath.c_str());
  }
  return -1;
}


//********************************************************************************************************************************
long CVideoDatabase::AddPath(const CStdString& strPath)
{
  try
  {
    if (NULL == m_pDB.get()) return -1;
    if (NULL == m_pDS.get()) return -1;

    CStdString strSQL=FormatSQL("select * from path where strPath like '%s'", strPath.c_str());
    m_pDS->query(strSQL.c_str());
    if (m_pDS->num_rows() == 0)
    {
      m_pDS->close();
      // doesnt exists, add it
      strSQL=FormatSQL("insert into Path (idPath, strPath) values( NULL, '%s')",
                    strPath.c_str());
      m_pDS->exec(strSQL.c_str());
      long lPathId = (long)sqlite3_last_insert_rowid(m_pDB->getHandle());
      return lPathId;
    }
    else
    {
      const field_value value = m_pDS->fv("idPath");
      long lPathId = value.get_asLong() ;
      m_pDS->close();
      return lPathId;
    }

  }
  catch (...)
  {
    CLog::Log(LOGERROR, "CVideoDatabase::AddPath(%s) failed", strPath.c_str());
  }
  return -1;
}

//********************************************************************************************************************************
long CVideoDatabase::GetPath(const CStdString& strPath)
{
  try
  {
    if (NULL == m_pDB.get()) return -1;
    if (NULL == m_pDS.get()) return -1;

    CStdString strSQL=FormatSQL("select * from path where strPath like '%s' ", strPath.c_str());
    m_pDS->query(strSQL.c_str());
    if (m_pDS->num_rows() > 0)
    {
      long lPathId = m_pDS->fv("idPath").get_asLong();
      m_pDS->close();
      return lPathId;
    }
    m_pDS->close();
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "CVideoDatabase::GetPath(%s) failed", strPath.c_str());
  }
  return -1;
}


long CVideoDatabase::GetMovie(const CStdString& strFilenameAndPath)
{
  long lPathId;
  long lMovieId;
  if (GetFile(strFilenameAndPath, lPathId, lMovieId) < 0)
  {
    return -1;
  }
  return lMovieId;
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
    Split(strFilenameAndPath, strPath, strFile);

    // have to join movieinfo table for correct results
    CStdString strSQL=FormatSQL("select * from movie join path on movie.idPath = path.idPath join movieinfo on movie.idMovie = movieinfo.idMovie join files on movie.idMovie = files.idMovie where (path.strPath like '%s' and files.strFileName like '%s') or path.strPath like 'stack://%s'", strPath.c_str(), strFile.c_str(), strPath.c_str());
    CLog::Log(LOGDEBUG,"CVideoDatabase::GetMovieInfo(%s), query = %s", strFilenameAndPath.c_str(), strSQL.c_str());
    m_pDS->query(strSQL.c_str());
    if (m_pDS->num_rows() > 0)
    {
      CLog::Log(LOGDEBUG,"Looking for movies by filename [%s]", strFilenameAndPath.c_str());
      while (!m_pDS->eof())
      {
        CStdString strTest = m_pDS->fv("path.strPath").get_asString() + m_pDS->fv("files.strFilename").get_asString();
        CLog::Log(LOGDEBUG,"  Testing [%s]", strTest.c_str());

        // exact file match
        if (strFilenameAndPath.CompareNoCase(strTest) == 0)
          lMovieId = m_pDS->fv("movieinfo.idMovie").get_asLong();

        // file may be part of a stacked path
        else if (strTest.Left(8).Equals("stack://"))
        {
          CStackDirectory dir;
          CFileItemList items;
          dir.GetDirectory(strTest, items);
          for (int i = 0; i < items.Size(); ++i)
          {
            if (strFilenameAndPath.CompareNoCase(items[i]->m_strPath) == 0)
            {
              lMovieId = m_pDS->fv("movieinfo.idMovie").get_asLong();
              break;
            }
          }
        }

        // did we find anything?
        if (lMovieId > -1)
        {
          // abort out of while loop if we have a match
          CLog::Log(LOGDEBUG,"  Found match! [%s]", strTest.c_str());
          break;
        }
        // no, try next db result
        m_pDS->next();
      }
    }
    m_pDS->close();

    return lMovieId;
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "CVideoDatabase::GetMovieInfo(%s) failed", strFilenameAndPath.c_str());
  }
  return -1;
}

int CVideoDatabase::GetRecentMovies(long* pMovieIdArray, int nSize)
{
  int count = 0;

  try
  {
    if (NULL == m_pDB.get())
    {
      return -1;
    }

    if (NULL == m_pDS.get())
    {
      return -1;
    }

    CStdString strSQL=FormatSQL("select * from movie order by idMovie desc limit %d", nSize);
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
long CVideoDatabase::AddMovie(const CStdString& strFilenameAndPath, const CStdString& strcdLabel, bool bHassubtitles)
{
  try
  {
    if (NULL == m_pDB.get()) return -1;
    if (NULL == m_pDS.get()) return -1;
    CStdString strPath, strFileName;
    Split(strFilenameAndPath, strPath, strFileName);

    long lMovieId = GetMovie(strFilenameAndPath);
    if (lMovieId < 0)
    {
      long lPathId = AddPath(strPath);
      if (lPathId < 0) return -1;
      CStdString strSQL=FormatSQL("insert into movie (idMovie, idPath, hasSubtitles, cdlabel) values( NULL, %i, %i, '%s')",
                    lPathId, bHassubtitles, strcdLabel.c_str());
      m_pDS->exec(strSQL.c_str());
      lMovieId = (long)sqlite3_last_insert_rowid(m_pDB->getHandle());
      AddFile(lMovieId, lPathId, strFileName);
    }
    else
    {
      long lPathId = GetPath(strPath);
      if (lPathId < 0) return -1;
      AddFile(lMovieId, lPathId, strFileName);
    }
    return lMovieId;
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "CVideoDatabase::AddMovie(%s,%s) failed", strFilenameAndPath.c_str() , strcdLabel.c_str() );
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

    CStdString strSQL=FormatSQL("select * from genre where strGenre like '%s'", strGenre.c_str());
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
    CStdString strSQL=FormatSQL("select * from Actors where strActor like '%s'", strActor.c_str());
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

//********************************************************************************************************************************
void CVideoDatabase::AddActorToMovie(long lMovieId, long lActorId)
{
  try
  {
    if (NULL == m_pDB.get()) return ;
    if (NULL == m_pDS.get()) return ;

    CStdString strSQL=FormatSQL("select * from actorlinkmovie where idActor=%i and idMovie=%i", lActorId, lMovieId);
    m_pDS->query(strSQL.c_str());
    if (m_pDS->num_rows() == 0)
    {
      // doesnt exists, add it
      strSQL=FormatSQL("insert into actorlinkmovie (idActor, idMovie) values( %i,%i)", lActorId, lMovieId);
      m_pDS->exec(strSQL.c_str());
    }
    m_pDS->close();
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "CVideoDatabase::AddActorToMovie() failed");
  }
}

//********************************************************************************************************************************
void CVideoDatabase::GetGenres(VECMOVIEGENRES& genres, int iShowMode /* = VIDEO_SHOW_ALL */)
{
  try
  {
    genres.erase(genres.begin(), genres.end());
    map<long, CStdString> mapGenres;
    map<long, CStdString>::iterator it;

    if (NULL == m_pDB.get()) return ;
    if (NULL == m_pDS.get()) return ;

    CStdString strSQL;
	  if (iShowMode == VIDEO_SHOW_WATCHED)
	  {
		  strSQL=FormatSQL("select * from genrelinkmovie,genre,movie,movieinfo,actors,path where path.idpath=movie.idpath and genrelinkmovie.idGenre=genre.idGenre and genrelinkmovie.idmovie=movie.idmovie and movieinfo.idmovie=movie.idmovie and movieinfo.bWatched='true' and movieinfo.iddirector=actors.idActor order by path.idpath");
	  }
	  else if (iShowMode == VIDEO_SHOW_UNWATCHED)
	  {
		  strSQL=FormatSQL("select * from genrelinkmovie,genre,movie,movieinfo,actors,path where path.idpath=movie.idpath and genrelinkmovie.idGenre=genre.idGenre and genrelinkmovie.idmovie=movie.idmovie and movieinfo.idmovie=movie.idmovie and NOT(movieinfo.bWatched='true') and movieinfo.iddirector=actors.idActor order by path.idpath");
	  }
	  else // VIDEO_SHOW_ALL
	  {
		  strSQL=FormatSQL("select * from genrelinkmovie,genre,movie,movieinfo,actors,path where path.idpath=movie.idpath and genrelinkmovie.idGenre=genre.idGenre and genrelinkmovie.idmovie=movie.idmovie and movieinfo.idmovie=movie.idmovie and movieinfo.iddirector=actors.idActor order by path.idpath");
	  }
    m_pDS->query(strSQL.c_str());

    long lLastPathId = -1;
    while (!m_pDS->eof())
    {
      long lGenreId = m_pDS->fv("genre.idgenre").get_asLong();
      CStdString strGenre = m_pDS->fv("genre.strgenre").get_asString();
      it = mapGenres.find(lGenreId);
      // was this genre already found?
      if (it == mapGenres.end())
      {
        // is the current path the same as the previous path?
        long lPathId = m_pDS->fv("path.idpath").get_asLong();
        if (lPathId == lLastPathId)
          mapGenres.insert(pair<long, CStdString>(lGenreId, strGenre));
        // test path
        else
        {
          CStdString strPath = m_pDS->fv("path.strPath").get_asString();
          if (g_passwordManager.IsDatabasePathUnlocked(strPath, g_settings.m_vecMyVideoShares))
          {
            // the path is unlocked so set last path to current path
            lLastPathId = lPathId;
            mapGenres.insert(pair<long, CStdString>(lGenreId, strGenre));
          }
        }
      }
      m_pDS->next();
    }
    m_pDS->close();

    // convert map back to vector
    for (it = mapGenres.begin(); it != mapGenres.end(); it++)
      genres.push_back(it->second);
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "CVideoDatabase::GetGenres() failed");
  }
}

//********************************************************************************************************************************
void CVideoDatabase::GetActors(VECMOVIEACTORS& actors, int iShowMode /* = VIDEO_SHOW_ALL */)
{
  try
  {
    actors.erase(actors.begin(), actors.end());
    map<long, CStdString> mapActors;
    map<long, CStdString>::iterator it;

    if (NULL == m_pDB.get()) return ;
    if (NULL == m_pDS.get()) return ;

    CStdString strSQL;
    if (iShowMode == VIDEO_SHOW_WATCHED)
	  {
		  strSQL=FormatSQL("select * from actorlinkmovie,actors,movie,movieinfo,path where path.idpath=movie.idpath and actors.idActor=actorlinkmovie.idActor and actorlinkmovie.idmovie=movie.idmovie and movieinfo.bWatched='true' and movieinfo.idmovie=movie.idmovie order by path.idpath");
	  }
	  else if (iShowMode == VIDEO_SHOW_UNWATCHED)
	  {
		  strSQL=FormatSQL("select * from actorlinkmovie,actors,movie,movieinfo,path where path.idpath=movie.idpath and actors.idActor=actorlinkmovie.idActor and actorlinkmovie.idmovie=movie.idmovie and NOT(movieinfo.bWatched='true') and movieinfo.idmovie=movie.idmovie order by path.idpath");
	  }
	  else // VIDEO_SHOW_ALL
	  {
		  strSQL=FormatSQL("select * from actorlinkmovie,actors,movie,movieinfo,path where path.idpath=movie.idpath and actors.idActor=actorlinkmovie.idActor and actorlinkmovie.idmovie=movie.idmovie and movieinfo.idmovie=movie.idmovie order by path.idpath");
    }
    m_pDS->query(strSQL.c_str());

    long lLastPathId = -1;
    while (!m_pDS->eof())
    {
      long lActorId = m_pDS->fv("actors.idactor").get_asLong();
      CStdString strActor = m_pDS->fv("actors.strActor").get_asString();
      it = mapActors.find(lActorId);
      // is this actor already known?
      if (it == mapActors.end())
      {
        // is this the same path as previous path?
        long lPathId = m_pDS->fv("path.idpath").get_asLong();
        if (lPathId == lLastPathId)
          mapActors.insert(pair<long, CStdString>(lActorId, strActor));
        // test path
        else
        {
          CStdString strPath = m_pDS->fv("path.strPath").get_asString();
          if (g_passwordManager.IsDatabasePathUnlocked(strPath, g_settings.m_vecMyVideoShares))
          {
            // the path is unlocked so set last path to current path
            lLastPathId = lPathId;
            mapActors.insert(pair<long, CStdString>(lActorId, strActor));
          }
        }
      }
      m_pDS->next();
    }
    m_pDS->close();

    // convert map back to vector
    for (it = mapActors.begin(); it != mapActors.end(); it++)
      actors.push_back(it->second);
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "CVideoDatabase::GetActors() failed");
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

    CStdString strSQL=FormatSQL("select * from movieinfo where movieinfo.idmovie=%i", lMovieId);
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

//********************************************************************************************************************************
bool CVideoDatabase::HasSubtitle(const CStdString& strFilenameAndPath)
{
  try
  {
    if (NULL == m_pDB.get()) return false;
    if (NULL == m_pDS.get()) return false;

    long lMovieId = GetMovie(strFilenameAndPath);
    if ( lMovieId < 0) return false;

    CStdString strSQL=FormatSQL("select * from movie where movie.idMovie=%i", lMovieId);
    m_pDS->query(strSQL.c_str());
    long lHasSubs = 0;
    if (m_pDS->num_rows() > 0)
    {
      lHasSubs = m_pDS->fv("hasSubtitles").get_asLong() ;
    }
    m_pDS->close();
    return (lHasSubs != 0);
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "CVideoDatabase::HasSubtitle(%s) failed", strFilenameAndPath.c_str());
  }
  return false;
}

//********************************************************************************************************************************
void CVideoDatabase::DeleteMovieInfo(const CStdString& strFileNameAndPath)
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

    strSQL=FormatSQL("delete from movieinfo where idmovie=%i", lMovieId);
    m_pDS->exec(strSQL.c_str());
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "CVideoDatabase::DeleteMovieInfo(%s) failed", strFileNameAndPath.c_str());
  }
}

//********************************************************************************************************************************
void CVideoDatabase::GetMoviesByGenre(CStdString& strGenre, VECMOVIES& movies)
{
  try
  {
    movies.erase(movies.begin(), movies.end());
    if (NULL == m_pDB.get()) return ;
    if (NULL == m_pDS.get()) return ;

    CStdString strSQL=FormatSQL("select * from genrelinkmovie,genre,movie,movieinfo,actors,path,files where path.idpath=movie.idpath and genrelinkmovie.idGenre=genre.idGenre and genrelinkmovie.idmovie=movie.idmovie and movieinfo.idmovie=movie.idmovie and genre.strGenre='%s' and movieinfo.iddirector=actors.idActor and files.idmovie = movie.idmovie", strGenre.c_str());
    m_pDS->query( strSQL.c_str() );

    long lLastPathId = -1;
    while (!m_pDS->eof())
    {
      // if the current path is the same as the last path tested and unlocked,
      // just add the movie without retesting
      long lPathId = m_pDS->fv("path.idPath").get_asLong();
      if (lPathId == lLastPathId)
        movies.push_back(GetDetailsFromDataset(m_pDS));
      // we have a new path so test it.
      else
      {
        CStdString strPath = m_pDS->fv("path.strPath").get_asString();
        if (g_passwordManager.IsDatabasePathUnlocked(strPath, g_settings.m_vecMyVideoShares))
        {
          // the path is unlocked so set last path to current path
          lLastPathId = lPathId;
          movies.push_back(GetDetailsFromDataset(m_pDS));
        }
      }
      m_pDS->next();
    }
    m_pDS->close();
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "CVideoDatabase::GetMoviesByGenre(%s) failed", strGenre.c_str());
  }
}

//********************************************************************************************************************************
void CVideoDatabase::GetMoviesByActor(CStdString& strActor, VECMOVIES& movies)
{
  try
  {
    movies.erase(movies.begin(), movies.end());
    if (NULL == m_pDB.get()) return ;
    if (NULL == m_pDS.get()) return ;

    CStdString strSQL=FormatSQL("select * from actorlinkmovie,actors,movie,movieinfo,path,files where path.idpath=movie.idpath and actors.idActor=actorlinkmovie.idActor and actorlinkmovie.idmovie=movie.idmovie and movieinfo.idmovie=movie.idmovie and actors.stractor='%s' and files.idmovie = movie.idmovie", strActor.c_str());
    m_pDS->query( strSQL.c_str() );

    long lLastPathId = -1;
    while (!m_pDS->eof())
    {
      // if the current path is the same as the last path tested and unlocked,
      // just add the movie without retesting
      long lPathId = m_pDS->fv("path.idPath").get_asLong();
      if (lPathId == lLastPathId)
        movies.push_back(GetDetailsFromDataset(m_pDS));
      // we have a new path so test it.
      else
      {
        CStdString strPath = m_pDS->fv("path.strPath").get_asString();
        if (g_passwordManager.IsDatabasePathUnlocked(strPath, g_settings.m_vecMyVideoShares))
        {
          // the path is unlocked so set last path to current path
          lLastPathId = lPathId;
          movies.push_back(GetDetailsFromDataset(m_pDS));
        }
      }
      m_pDS->next();
    }
    m_pDS->close();
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "CVideoDatabase::GetMoviesByActor(%s) failed", strActor.c_str());
  }
}


//********************************************************************************************************************************
void CVideoDatabase::GetMovieInfo(const CStdString& strFilenameAndPath, CIMDBMovie& details, long lMovieId /* = -1 */)
{
  try
  {
    if (lMovieId < 0)
      lMovieId = GetMovieInfo(strFilenameAndPath);
    if (lMovieId < 0) return ;

    CStdString strSQL=FormatSQL("select * from movieinfo,actors,movie,path,files where path.idpath=movie.idpath and movie.idMovie=movieinfo.idMovie and movieinfo.idmovie=%i and idDirector=idActor and files.idmovie = movie.idmovie", lMovieId);

    m_pDS->query(strSQL.c_str());
    if (m_pDS->num_rows() > 0)
    {
      details = GetDetailsFromDataset(m_pDS);
      if (details.m_strWritingCredits.IsEmpty())
      { // try loading off disk
        CIMDB imdb;
        imdb.LoadDetails(details.m_strIMDBNumber, details);
      }
    }
    m_pDS->close();
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "CVideoDatabase::GetMovieInfo(%s) failed", strFilenameAndPath.c_str());
  }
}

//********************************************************************************************************************************
void CVideoDatabase::SetMovieInfo(const CStdString& strFilenameAndPath, CIMDBMovie& details)
{
  try
  {
    long lMovieId = GetMovie(strFilenameAndPath);
    if (lMovieId < 0) return ;
    details.m_strSearchString.Format("%i", lMovieId);

    CStdString strPath, strFileName ;
    Split(strFilenameAndPath, strPath, strFileName);
    details.m_strPath = strPath;
    details.m_strFile = strFileName;
    details.m_strFileNameAndPath = strFilenameAndPath;

    // add director
    long lDirector = AddActor(details.m_strDirector);

    // add all genres
    char szGenres[1024];
    strcpy(szGenres, details.m_strGenre.c_str());
    vector<long> vecGenres;
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
    else
    {
      CStdString strGenre = details.m_strGenre;
      strGenre.Trim();
      long lGenreId = AddGenre(strGenre);
      vecGenres.push_back(lGenreId);
    }

    // add cast...
    vector<long> vecActors;
    CStdString strCast;
    int ipos = 0;
    strCast = details.m_strCast.c_str();
    ipos = strCast.Find(" as ");
    while (ipos > 0)
    {
      CStdString strActor;
      int x = ipos;
      while (x > 0)
      {
        if (strCast[x] != '\r' && strCast[x] != '\n') x--;
        else
        {
          x++;
          break;
        }
      }
      strActor = strCast.Mid(x, ipos - x);
      long lActorId = AddActor(strActor);
      vecActors.push_back(lActorId);
      ipos = strCast.Find(" as ", ipos + 3);
    }

    for (int i = 0; i < (int)vecGenres.size(); ++i)
    {
      AddGenreToMovie(lMovieId, vecGenres[i]);
    }

    for (i = 0; i < (int)vecActors.size(); ++i)
    {
      AddActorToMovie(lMovieId, vecActors[i]);
    }

    CStdString strSQL;
    CStdString strRating;
    strRating.Format("%3.3f", details.m_fRating);
    if (strRating == "") strRating = "0.0";
    strSQL=FormatSQL("select * from movieinfo where idmovie=%i", lMovieId);
    m_pDS->query(strSQL.c_str());
    if (m_pDS->num_rows() == 0)
    {
      m_pDS->close();
      CStdString strTmp = "";
      strSQL=FormatSQL("insert into movieinfo ( idMovie,idDirector,strRuntime,fRating,iYear,strTitle,strGenre,IMDBID,bWatched) values(%i,%i,'%s','%s',%i,'%s','%s','%s','%s')",
                    lMovieId, lDirector, details.m_strRuntime.c_str(), strRating.c_str(),
                    details.m_iYear, details.m_strTitle.c_str(), details.m_strGenre.c_str(),
                    details.m_strIMDBNumber.c_str(), "false" );

      m_pDS->exec(strSQL.c_str());

    }
    else
    {
      m_pDS->close();
      strSQL=FormatSQL("update movieinfo set idDirector=%i, strRuntime='%s', fRating='%s', iYear=%i, strTitle='%s', strGenre='%s', IMDBID='%s', bWatched='false' where idMovie=%i",
                    lDirector, details.m_strRuntime.c_str(), strRating.c_str(),
                    details.m_iYear, details.m_strTitle.c_str(), details.m_strGenre.c_str(),
                    details.m_strIMDBNumber.c_str(),
                    lMovieId);
      m_pDS->exec(strSQL.c_str());
    }
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "CVideoDatabase::SetMovieInfo(%s) failed", strFilenameAndPath.c_str());
  }
}


//********************************************************************************************************************************
void CVideoDatabase::GetMoviesByPath(CStdString& strPath1, VECMOVIES& movies)
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
void CVideoDatabase::GetFilePath(long lMovieId, CStdString &filePath)
{
  try
  {
    if (NULL == m_pDB.get()) return ;
    if (NULL == m_pDS.get()) return ;

    //long lMovieId=GetMovie(strFile);
    if (lMovieId < 0) return ;

    CStdString strSQL=FormatSQL("select * from path,files where path.idPath=files.idPath and files.idmovie=%i order by strFilename", lMovieId );
    m_pDS->query( strSQL.c_str() );
    if (!m_pDS->eof())
    {
      filePath = m_pDS->fv("path.strPath").get_asString();
      filePath += m_pDS->fv("files.strFilename").get_asString();
    }
    m_pDS->close();
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "CVideoDatabase::GetFilePath() failed");
  }
}

//********************************************************************************************************************************
void CVideoDatabase::GetYears(VECMOVIEYEARS& years, int iShowMode /* = VIDEO_SHOW_ALL */)
{
  try
  {
    years.erase(years.begin(), years.end());
    map<long, CStdString> mapYears;
    map<long, CStdString>::iterator it;

    if (NULL == m_pDB.get()) return ;
    if (NULL == m_pDS.get()) return ;
    
	  CStdString strSQL;
	  if (iShowMode == VIDEO_SHOW_WATCHED)
	  {
		  strSQL=FormatSQL("select * from movie,movieinfo,actors,path where path.idpath=movie.idpath and movieinfo.idmovie=movie.idmovie and movieinfo.iddirector=actors.idActor and movieinfo.bWatched='true' order by path.idpath");
	  }
	  else if (iShowMode == VIDEO_SHOW_UNWATCHED)
	  {
		  strSQL=FormatSQL("select * from movie,movieinfo,actors,path where path.idpath=movie.idpath and movieinfo.idmovie=movie.idmovie and movieinfo.iddirector=actors.idActor and NOT(movieinfo.bWatched='true') order by path.idpath");
	  }
	  else // VIDEO_SHOW_ALL
	  {
		  strSQL=FormatSQL("select * from movie,movieinfo,actors,path where path.idpath=movie.idpath and movieinfo.idmovie=movie.idmovie and movieinfo.iddirector=actors.idActor order by path.idpath");
	  }
    m_pDS->query(strSQL.c_str());

    long lLastPathId = -1;
    while (!m_pDS->eof())
    {
      CStdString strYear = m_pDS->fv("movieinfo.iYear").get_asString();
      long lYear = atol(strYear.c_str());
      it = mapYears.find(lYear);
      if (it == mapYears.end())
      {
        // is the path the same as the previous path?
        long lPathId = m_pDS->fv("path.idpath").get_asLong();
        if (lPathId == lLastPathId)
          mapYears.insert(pair<long, CStdString>(lYear, strYear));
        // test path
        else
        {
          CStdString strPath = m_pDS->fv("path.strPath").get_asString();
          if (g_passwordManager.IsDatabasePathUnlocked(strPath, g_settings.m_vecMyVideoShares))
          {
            // the path is unlocked so set last path to current path
            lLastPathId = lPathId;
            mapYears.insert(pair<long, CStdString>(lYear, strYear));
          }
        }
      }
      m_pDS->next();
    }
    m_pDS->close();

    // convert map back to vector
    for (it = mapYears.begin(); it != mapYears.end(); it++)
      years.push_back(it->second);
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "CVideoDatabase::GetYears() failed");
  }
}

//********************************************************************************************************************************
void CVideoDatabase::GetMoviesByYear(CStdString& strYear, VECMOVIES& movies)
{
  try
  {
    int iYear = atoi(strYear.c_str());
    movies.erase(movies.begin(), movies.end());
    if (NULL == m_pDB.get()) return ;
    if (NULL == m_pDS.get()) return ;

    CStdString strSQL=FormatSQL("select * from movie,movieinfo,actors,path,files where path.idpath=movie.idpath and movieinfo.idmovie=movie.idmovie and movieinfo.iddirector=actors.idActor and movieinfo.iYear=%i and files.idmovie = movie.idmovie", iYear);
    m_pDS->query( strSQL.c_str() );

    long lLastPathId = -1;
    while (!m_pDS->eof())
    {
      // if the current path is the same as the last path tested and unlocked,
      // just add the movie without retesting
      long lPathId = m_pDS->fv("path.idPath").get_asLong();
      if (lPathId == lLastPathId)
        movies.push_back(GetDetailsFromDataset(m_pDS));
      // we have a new path so test it.
      else
      {
        CStdString strPath = m_pDS->fv("path.strPath").get_asString();
        if (g_passwordManager.IsDatabasePathUnlocked(strPath, g_settings.m_vecMyVideoShares))
        {
          // the path is unlocked so set last path to current path
          lLastPathId = lPathId;
          movies.push_back(GetDetailsFromDataset(m_pDS));
        }
      }
      m_pDS->next();
    }
    m_pDS->close();
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "CVideoDatabase::GetMoviesByYear(%s) failed", strYear.c_str());
  }
}

bool SortBookmarks(const CBookmark &left, const CBookmark &right)
{
  return left.timeInSeconds < right.timeInSeconds;
}

//********************************************************************************************************************************
void CVideoDatabase::GetBookMarksForMovie(const CStdString& strFilenameAndPath, VECBOOKMARKS& bookmarks, CBookmark::EType type /*= CBookmark::STANDARD*/)
{
  try
  {
    long lPathId, lMovieId;
    long lFileId = GetFile(strFilenameAndPath, lPathId, lMovieId, true);
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
    sort(bookmarks.begin(), bookmarks.end(), SortBookmarks);
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
  GetBookMarksForMovie(strFilenameAndPath, bookmarks, CBookmark::RESUME);
  if(bookmarks.size()>0)
  {
    bookmark = bookmarks[0];
    return true;
  }
  return false;
}

//********************************************************************************************************************************
void CVideoDatabase::AddBookMarkToMovie(const CStdString& strFilenameAndPath, const CBookmark &bookmark, CBookmark::EType type /*= CBookmark::STANDARD*/)
{
  try
  {
    long lPathId, lMovieId;
    long lFileId = GetFile(strFilenameAndPath, lPathId, lMovieId, true);
    if (lFileId < 0)
    {
      // Doesn't exist in the database yet - add it.
      // TODO: It doesn't appear to me that the CDLabel parameter or the subtitles
      // parameter is anywhere in use in XBMC.
      AddMovie(strFilenameAndPath, "", false);
      lFileId = GetFile(strFilenameAndPath, lPathId, lMovieId, true);
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

void CVideoDatabase::ClearBookMarkOfVideo(const CStdString& strFilenameAndPath, CBookmark& bookmark, CBookmark::EType type /*= CBookmark::STANDARD*/)
{
  try
  {
    long lPathId, lMovieId;
    long lFileId = GetFile(strFilenameAndPath, lPathId, lMovieId, true);
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
    CLog::Log(LOGERROR, "CVideoDatabase::ClearBookMarkOfMovie(%s) failed", strFilenameAndPath.c_str());
  }
}

//********************************************************************************************************************************
void CVideoDatabase::ClearBookMarksOfMovie(const CStdString& strFilenameAndPath, CBookmark::EType type /*= CBookmark::STANDARD*/)
{
  try
  {
    long lPathId, lMovieId;
    long lFileId = GetFile(strFilenameAndPath, lPathId, lMovieId, true);
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
void CVideoDatabase::GetMovies(VECMOVIES& movies)
{
  try
  {
    movies.erase(movies.begin(), movies.end());
    if (NULL == m_pDB.get()) return ;
    if (NULL == m_pDS.get()) return ;

    DWORD time = timeGetTime();
    CStdString strSQL="select * from movie,movieinfo,actors,path,files where movieinfo.idmovie=movie.idmovie and movieinfo.iddirector=actors.idActor and movie.idpath=path.idpath and files.idmovie = movie.idmovie order by path.idpath";
    m_pDS->query( strSQL.c_str() );

    CLog::Log(LOGDEBUG, "GetMovies query took %i ms", timeGetTime() - time);
    time = timeGetTime();
    long lLastPathId = -1;
    while (!m_pDS->eof())
    {
      // if the current path is the same as the last path tested and unlocked,
      // just add the movie without retesting
      long lPathId = m_pDS->fv("path.idPath").get_asLong();
      if (lPathId == lLastPathId)
        movies.push_back(GetDetailsFromDataset(m_pDS));
      // we have a new path so test it.
      else
      {
        CStdString strPath = m_pDS->fv("path.strPath").get_asString();
        if (g_passwordManager.IsDatabasePathUnlocked(strPath, g_settings.m_vecMyVideoShares))
        {
          // the path is unlocked so set last path to current path
          lLastPathId = lPathId;
          movies.push_back(GetDetailsFromDataset(m_pDS));
        }
      }
      m_pDS->next();
    }
    m_pDS->close();
    CLog::Log(LOGDEBUG, "Time taken for GetMovies(): %i", timeGetTime() - time);
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "CVideoDatabase::GetMovies() failed");
  }
}

//********************************************************************************************************************************
void CVideoDatabase::DeleteMovie(const CStdString& strFilenameAndPath)
{
  try
  {
    long lPathId;
    long lMovieId;
    if (NULL == m_pDB.get()) return ;
    if (NULL == m_pDS.get()) return ;
    if (GetFile(strFilenameAndPath, lPathId, lMovieId) < 0)
    {
      return ;
    }

    ClearBookMarksOfMovie(strFilenameAndPath);

    CStdString strSQL;
    strSQL=FormatSQL("delete from files where idmovie=%i", lMovieId);
    m_pDS->exec(strSQL.c_str());

    strSQL=FormatSQL("delete from genrelinkmovie where idmovie=%i", lMovieId);
    m_pDS->exec(strSQL.c_str());

    strSQL=FormatSQL("delete from actorlinkmovie where idmovie=%i", lMovieId);
    m_pDS->exec(strSQL.c_str());

    strSQL=FormatSQL("delete from movieinfo where idmovie=%i", lMovieId);
    m_pDS->exec(strSQL.c_str());

    strSQL=FormatSQL("delete from movie where idmovie=%i", lMovieId);
    m_pDS->exec(strSQL.c_str());
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "CVideoDatabase::DeleteMovie() failed");
  }
}

//********************************************************************************************************************************
void CVideoDatabase::SetDVDLabel(long lMovieId, const CStdString& strDVDLabel)
{
  try
  {
    if (NULL == m_pDB.get()) return ;
    if (NULL == m_pDS.get()) return ;
    CLog::Log(LOGERROR, "setdvdlabel:id:%i label:%s", lMovieId, strDVDLabel.c_str());

    CStdString strSQL;

    strSQL.Format("update movie set cdlabel='%s' where idMovie=%i", strDVDLabel.c_str(), lMovieId );
    m_pDS->exec(strSQL.c_str());
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "CVideoDatabase::SetDVDLabel() failed");
  }
}

//********************************************************************************************************************************
void CVideoDatabase::GetDVDLabel(long lMovieId, CStdString& strDVDLabel)
{
  strDVDLabel = "";
  try
  {
    if (NULL == m_pDB.get()) return ;
    if (NULL == m_pDS.get()) return ;

    CStdString strSQL=FormatSQL("select cdlabel from movie where idMovie=%i", lMovieId);
    m_pDS->query(strSQL.c_str());
    if (m_pDS->num_rows() == 1)
    {
      strDVDLabel = m_pDS->fv("cdlabel").get_asString();
    }
    m_pDS->close();
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "CVideoDatabase::GetDVDLabel() failed");
  }
}

CIMDBMovie CVideoDatabase::GetDetailsFromDataset(auto_ptr<Dataset> &pDS)
{
  CIMDBMovie details;
  details.m_fRating = (float)atof(pDS->fv("movieinfo.fRating").get_asString().c_str()) ;
  details.m_strDirector = pDS->fv("actors.strActor").get_asString();
  details.m_strWritingCredits = pDS->fv("movieinfo.strCredits").get_asString();
  details.m_strTagLine = pDS->fv("movieinfo.strTagLine").get_asString();
  details.m_strPlotOutline = pDS->fv("movieinfo.strPlotOutline").get_asString();
  details.m_strPlot = pDS->fv("movieinfo.strPlot").get_asString();
  details.m_strVotes = pDS->fv("movieinfo.strVotes").get_asString();
  details.m_strRuntime = pDS->fv("movieinfo.strRuntime").get_asString();
  details.m_strCast = pDS->fv("movieinfo.strCast").get_asString();
  details.m_iYear = pDS->fv("movieinfo.iYear").get_asLong();
  long lMovieId = pDS->fv("movieinfo.idMovie").get_asLong();
  details.m_strGenre = pDS->fv("movieinfo.strGenre").get_asString();
  details.m_strPictureURL = pDS->fv("movieinfo.strPictureURL").get_asString();
  details.m_strTitle = pDS->fv("movieinfo.strTitle").get_asString();
  details.m_strPath = pDS->fv("path.strPath").get_asString();
  details.m_strDVDLabel = pDS->fv("movie.cdlabel").get_asString();
  details.m_strIMDBNumber = pDS->fv("movieinfo.IMDBID").get_asString();
  details.m_bWatched = pDS->fv("movieinfo.bWatched").get_asBool();
  details.m_strFileNameAndPath = details.m_strPath + pDS->fv("files.strFileName").get_asString();

  details.m_strSearchString.Format("%i", lMovieId);
  return details;
}

/// \brief GetVideoSettings() obtains any saved video settings for the current file.
/// \retval Returns true if the settings exist, false otherwise.
bool CVideoDatabase::GetVideoSettings(const CStdString &strFilenameAndPath, CVideoSettings &settings)
{
  try
  {
    // obtain the FileID (if it exists)
#ifdef NEW_VIDEO_DB_METHODS
    if (NULL == m_pDB.get()) return false;
    if (NULL == m_pDS.get()) return false;
    CStdString strPath, strFileName;
    Split(strFilenameAndPath, strPath, strFileName);
    CStdString strSQL=FormatSQL("select * from settings, files, path where settings.idfile=files.idfile and path.idpath=files.idpath and path.strPath like '%s' and files.strFileName like '%s'", strPath.c_str() , strFileName.c_str());
#else
    long lPathId, lMovieId;
    long lFileId = GetFile(strFilenameAndPath, lPathId, lMovieId, true);
    if (lFileId < 0) return false;
    if (NULL == m_pDB.get()) return false;
    if (NULL == m_pDS.get()) return false;
    // ok, now obtain the settings for this file
    CStdString strSQL=FormatSQL("select settings.* settings where settings.idFile = '%i'", lFileId);
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
    long lPathId, lMovieId;
    if (NULL == m_pDB.get()) return ;
    if (NULL == m_pDS.get()) return ;
    long lFileId = GetFile(strFilenameAndPath, lPathId, lMovieId, true);
    if (lFileId < 0)
    { // no files found - we have to add one
      lMovieId = AddMovie(strFilenameAndPath, "", false);
      lFileId = GetFile(strFilenameAndPath, lPathId, lMovieId, true);
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
    long lPathId, lMovieId;
    long lFileId = GetFile(filePath, lPathId, lMovieId, true);
    if (lFileId < 0) return false;
    if (NULL == m_pDB.get()) return false;
    if (NULL == m_pDS.get()) return false;
    // ok, now obtain the settings for this file
    CStdString strSQL=FormatSQL("select times from stacktimes where idFile='%i'\n", lFileId);
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
    long lPathId, lMovieId;
    if (NULL == m_pDB.get()) return ;
    if (NULL == m_pDS.get()) return ;
    long lFileId = GetFile(filePath, lPathId, lMovieId, true);
    if (lFileId < 0)
    { // no files found - we have to add one
      lMovieId = AddMovie(filePath, "", false);
      lFileId = GetFile(filePath, lPathId, lMovieId, true);
      if (lFileId < 0) return ;
    }

    // delete any existing items
    CStdString strSQL;
    strSQL.Format("delete from stacktimes where idFile=%i", lFileId);
    m_pDS->exec( strSQL.c_str() );

    // add the items
    CStdString timeString;
    timeString.Format("%i", times[0]);
    for (unsigned int i = 1; i < times.size(); i++)
    {
      CStdString time;
      time.Format(",%i", times[i]);
      timeString += time;
    }
    strSQL.Format("insert into stacktimes (idFile,usingConversions,times) values (%i,%i,'%s')\n", lFileId, false, timeString.c_str());
    m_pDS->exec(strSQL.c_str());
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "CVideoDatabase::SetStackTimes(%s) failed", filePath.c_str());
  }
}

bool CVideoDatabase::UpdateOldVersion(float fVersion)
{
  if (fVersion < 0.5f)
  { // Version 0 to 0.5 upgrade - we need to add the version table and the settings table
    CLog::Log(LOGINFO, "creating versions table");
    m_pDS->exec("CREATE TABLE version (idVersion float)\n");

    CLog::Log(LOGINFO, "create settings table");
    m_pDS->exec("CREATE TABLE settings ( idFile integer, Interleaved bool, NoCache bool, Deinterlace bool, FilmGrain integer,"
                "ViewMode integer,ZoomAmount float, PixelRatio float, AudioStream integer, SubtitleStream integer,"
                "SubtitleDelay float, SubtitlesOn bool, Brightness integer, Contrast integer, Gamma integer,"
                "AdjustFrameRate integer, AudioDelay float)\n");
  }
  // REMOVED: v 1.0 -> 1.2 updates - they just delete the settings table in order
  // to add new settings - mayaswell just delete all the time.
  if (fVersion < 1.3f)
  { // v 1.0 -> 1.3 (new crop settings to video settings table)
    // Just delete and recreate the settings table is the easiest thing to do
    // all it means is per-video settings need recreating on playback - not a big deal
    // and it means the code can be kept reasonably simple.
    CLog::Log(LOGINFO, "Deleting old settings table");
    m_pDS->exec("DROP TABLE settings");
    CLog::Log(LOGINFO, "Creating new settings table");
    m_pDS->exec("CREATE TABLE settings ( idFile integer, Interleaved bool, NoCache bool, Deinterlace bool, FilmGrain integer,"
                "ViewMode integer,ZoomAmount float, PixelRatio float, AudioStream integer, SubtitleStream integer,"
                "SubtitleDelay float, SubtitlesOn bool, Brightness integer, Contrast integer, Gamma integer,"
                "AdjustFrameRate integer, AudioDelay float, ResumeTime integer, Crop bool, CropLeft integer,"
                "CropRight integer, CropTop integer, CropBottom integer)\n");
  }
  if (fVersion < 1.4f)
  { // v 1.3 -> 1.4 (new layout for bookmarks table)
    // Just delete the old bookmarks table and create it fresh
    CLog::Log(LOGINFO, "Deleting old bookmarks table");
    m_pDS->exec("DROP TABLE bookmark");
    CLog::Log(LOGINFO, "Creating new bookmarks table");
    m_pDS->exec("CREATE TABLE bookmark ( idBookmark integer primary key, idFile integer, timeInSeconds integer, thumbNailImage text)\n");
  }
//*************
// 2005-10-08
// MercuryInc
  if (fVersion < 1.5f)
  {
    // v 1.4 -> 1.5 (new layout for Watched tag)
    // Add watched column to movieinfo
    try
    {
      CLog::Log(LOGINFO, "Adding column to movieinfo");
      m_pDS->exec("CREATE TABLE TMPmovieinfo ( idMovie integer, idDirector integer, strPlotOutline text, strPlot text, strTagLine text, strVotes text, strRuntime text, fRating text, strCast text,strCredits text, iYear integer, strGenre text, strPictureURL text, strTitle text, IMDBID text, bWatched bool)\n");
      m_pDS->exec("INSERT INTO TMPmovieinfo SELECT *, 'false' FROM movieinfo\n");
      m_pDS->exec("DROP TABLE movieinfo\n");
      m_pDS->exec("CREATE TABLE movieinfo ( idMovie integer, idDirector integer, strPlotOutline text, strPlot text, strTagLine text, strVotes text, strRuntime text, fRating text, strCast text,strCredits text, iYear integer, strGenre text, strPictureURL text, strTitle text, IMDBID text, bWatched bool)\n");
      m_pDS->exec("INSERT INTO movieinfo SELECT * FROM TMPmovieinfo\n");
      m_pDS->exec("DROP TABLE TMPmovieinfo\n");      
    }
    catch (...)
    {
      CLog::Log(LOGERROR, "Failed to add bWatched to movieinfo");
      return false;
    }
    
    //vacuum movieinfo so the db is readable again
    try
    {
      CLog::Log(LOGINFO, "Vacuuming movieinfo");
      m_pDS->exec("VACUUM movieinfo\n");      
    }
    catch (...)
    {
      CLog::Log(LOGERROR, "Failed to vacuum movieinfo");
      return false;
    }
  }
//*************
  if (fVersion < 1.6f)
  {
    // dropping of AdjustFrameRate setting, and addition of VolumeAmplification setting
    CLog::Log(LOGINFO, "Deleting old settings table");
    m_pDS->exec("DROP TABLE settings");
    CLog::Log(LOGINFO, "Creating new settings table");
    m_pDS->exec("CREATE TABLE settings ( idFile integer, Interleaved bool, NoCache bool, Deinterlace bool, FilmGrain integer,"
                "ViewMode integer,ZoomAmount float, PixelRatio float, AudioStream integer, SubtitleStream integer,"
                "SubtitleDelay float, SubtitlesOn bool, Brightness integer, Contrast integer, Gamma integer,"
                "VolumeAmplification float, AudioDelay float, ResumeTime integer, Crop bool, CropLeft integer,"
                "CropRight integer, CropTop integer, CropBottom integer)\n");
  }

//*************
// 2006-02-20
// dvdplayer state
  if (fVersion < 1.7f)
  {
    try
    {
      CLog::Log(LOGINFO, "Adding playerState and type to bookmark");
      m_pDS->exec("CREATE TABLE TMPbookmark ( idBookmark integer primary key, idFile integer, timeInSeconds integer, thumbNailImage text, playerState text, type integer)\n");
      m_pDS->exec("INSERT INTO TMPbookmark SELECT idBookmark, idFile, timeInSeconds, thumbNailImage, '', 0 FROM bookmark");
      m_pDS->exec("DROP TABLE bookmark");
      m_pDS->exec("CREATE TABLE bookmark ( idBookmark integer primary key, idFile integer, timeInSeconds integer, thumbNailImage text, playerState text, type integer)\n");
      m_pDS->exec("INSERT INTO bookmark SELECT * FROM TMPbookmark");
      m_pDS->exec("DROP TABLE TMPbookmark");
    }
    catch(...)
    {
      CLog::Log(LOGERROR, "Failed to add playerState and type to bookmark");
      return false;
    }
  }
  if (fVersion < 1.8f)
  {
    // Adding of OutputToAllSpeakers setting
    CLog::Log(LOGINFO, "Deleting old settings table");
    m_pDS->exec("DROP TABLE settings");
    CLog::Log(LOGINFO, "Creating new settings table");
    m_pDS->exec("CREATE TABLE settings ( idFile integer, Interleaved bool, NoCache bool, Deinterlace bool, FilmGrain integer,"
                "ViewMode integer,ZoomAmount float, PixelRatio float, AudioStream integer, SubtitleStream integer,"
                "SubtitleDelay float, SubtitlesOn bool, Brightness integer, Contrast integer, Gamma integer,"
                "VolumeAmplification float, AudioDelay float, OutputToAllSpeakers bool, ResumeTime integer, Crop bool, CropLeft integer,"
                "CropRight integer, CropTop integer, CropBottom integer)\n");
  }
  if (fVersion < 1.9f)
  { // Add the stacktimes table
    try
    {
    CLog::Log(LOGINFO, "Adding stacktimes table");
    m_pDS->exec("CREATE TABLE stacktimes (idFile integer, usingConversions bool, times text)\n");
    }
    catch (...)
    {
      CLog::Log(LOGERROR, "Error adding stacktimes table to the database");
    }
  }


//*************
// 2006-02-20
// dvdplayer state
  if (fVersion < 2.0f)
  {
    try
    {
      CLog::Log(LOGINFO, "Adding player and changing timeInSeconds to float in bookmark table");
      m_pDS->exec("CREATE TABLE TMPbookmark ( idBookmark integer primary key, idFile integer, timeInSeconds integer, thumbNailImage text, playerState text, type integer)\n");
      m_pDS->exec("INSERT INTO TMPbookmark SELECT idBookmark, idFile, timeInSeconds, thumbNailImage, playerState, type FROM bookmark");
      m_pDS->exec("DROP TABLE bookmark");
      m_pDS->exec("CREATE TABLE bookmark ( idBookmark integer primary key, idFile integer, timeInSeconds double, thumbNailImage text, player text, playerState text, type integer)\n");
      m_pDS->exec("INSERT INTO bookmark SELECT idBookmark, idFile, timeInSeconds, thumbNailImage, '', playerState, type FROM TMPbookmark");
      m_pDS->exec("DROP TABLE TMPbookmark");
    }
    catch(...)
    {
      CLog::Log(LOGERROR, "Failed to upgrade bookmarks");
      return false;
    }
    
  }


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
  if (movieID < 0)
    return;
  // and mark as watched
  MarkAsWatched(movieID);
}

void CVideoDatabase::MarkAsWatched(long lMovieId)
{
  try
  {
    if (NULL == m_pDB.get()) return ;
    if (NULL == m_pDS.get()) return ;
    CLog::Log(LOGINFO, "Updating Movie:%i as Watched", lMovieId);
    CStdString strSQL;
    strSQL.Format("UPDATE movieinfo set bWatched='true' WHERE idMovie=%i", lMovieId );
    m_pDS->exec(strSQL.c_str());
  }
  catch (...)
  {
	  CLog::Log(LOGERROR, "CVideoDatabase::MarkAsWatched(long lMovieId) failed on MovieID:%i", lMovieId);
  }
}

void CVideoDatabase::MarkAsUnWatched(long lMovieId)
{
  try
  {
    if (NULL == m_pDB.get()) return ;
    if (NULL == m_pDS.get()) return ;
    CLog::Log(LOGINFO, "Updating Movie:%i as UnWatched", lMovieId);
    CStdString strSQL;
    strSQL.Format("UPDATE movieinfo set bWatched='false' WHERE idMovie=%i", lMovieId );
    m_pDS->exec(strSQL.c_str());
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "CVideoDatabase::MarkAsUnWatched(long lMovieId) failed on MovieID:%i", lMovieId);
  }
}

void CVideoDatabase::UpdateMovieTitle(long lMovieId, const CStdString& strNewMovieTitle)
{
  try
  {
    if (NULL == m_pDB.get()) return ;
    if (NULL == m_pDS.get()) return ;
    CLog::Log(LOGINFO, "Changing Movie:id:%i New Title:%s", lMovieId, strNewMovieTitle.c_str());
    CStdString strSQL;
    strSQL.Format("UPDATE movieinfo SET strTitle='%s' WHERE idMovie=%i", strNewMovieTitle.c_str(), lMovieId );
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

bool CVideoDatabase::GetGenresNav(const CStdString& strBaseDir, CFileItemList& items, long idYear)
{
  try
  {
    if (NULL == m_pDB.get()) return false;
    if (NULL == m_pDS.get()) return false;

    // get primary genres for movies
    CStdString strSQL="select * from genrelinkmovie,genre,movie,movieinfo,actors,path where path.idpath=movie.idpath and genrelinkmovie.idGenre=genre.idGenre and genrelinkmovie.idmovie=movie.idmovie and movieinfo.idmovie=movie.idmovie and movieinfo.iddirector=actors.idActor";

    if (idYear != -1)
      strSQL += FormatSQL(" and movieinfo.iYear=%i",idYear);

    // run query
    CLog::Log(LOGDEBUG, "CVideoDatabase::GetGenresNav() query: %s", strSQL.c_str());
    if (!m_pDS->query(strSQL.c_str())) return false;
    int iRowsFound = m_pDS->num_rows();
    if (iRowsFound == 0)
    {
      m_pDS->close();
      return false;
    }

    map<long, CStdString> mapGenres;
    map<long, CStdString>::iterator it;
    long lLastPathId = -1;
    while (!m_pDS->eof())
    {
      long lGenreId = m_pDS->fv("genre.idgenre").get_asLong();
      CStdString strGenre = m_pDS->fv("genre.strgenre").get_asString();
      it = mapGenres.find(lGenreId);
      // was this genre already found?
      if (it == mapGenres.end())
      {
        // is the current path the same as the previous path?
        long lPathId = m_pDS->fv("path.idpath").get_asLong();
        if (lPathId == lLastPathId)
          mapGenres.insert(pair<long, CStdString>(lGenreId, strGenre));
        // test path
        else
        {
          CStdString strPath = m_pDS->fv("path.strPath").get_asString();
          if (g_passwordManager.IsDatabasePathUnlocked(strPath, g_settings.m_vecMyVideoShares))
          {
            // the path is unlocked so set last path to current path
            lLastPathId = lPathId;
            mapGenres.insert(pair<long, CStdString>(lGenreId, strGenre));
          }
        }
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
      items.Add(pItem);
    }

    return true;
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "CVideoDatabase:GetGenresNav() failed");
  }
  return false;
}

bool CVideoDatabase::GetActorsNav(const CStdString& strBaseDir, CFileItemList& items)
{
  try
  {
    if (NULL == m_pDB.get()) return false;
    if (NULL == m_pDS.get()) return false;

		CStdString strSQL=FormatSQL("select * from actorlinkmovie,actors,movie,movieinfo,path where path.idpath=movie.idpath and actors.idActor=actorlinkmovie.idActor and actorlinkmovie.idmovie=movie.idmovie and movieinfo.idmovie=movie.idmovie order by path.idpath");

    if (!m_pDS->query(strSQL.c_str())) return false;
    int iRowsFound = m_pDS->num_rows();
    if (iRowsFound == 0)
    {
      m_pDS->close();
      return false;
    }

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
        // is this the same path as previous path?
        long lPathId = m_pDS->fv("path.idpath").get_asLong();
        if (lPathId == lLastPathId)
          mapActors.insert(pair<long, CStdString>(lActorId, strActor));
        // test path
        else
        {
          CStdString strPath = m_pDS->fv("path.strPath").get_asString();
          if (g_passwordManager.IsDatabasePathUnlocked(strPath, g_settings.m_vecMyVideoShares))
          {
            // the path is unlocked so set last path to current path
            lLastPathId = lPathId;
            mapActors.insert(pair<long, CStdString>(lActorId, strActor));
          }
        }
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

    return true;
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "CVideoDatabase:GetActorsNav() failed");
  }
  return false;
}

bool CVideoDatabase::GetYearsNav(const CStdString& strBaseDir, CFileItemList& items)
{
  try
  {
    if (NULL == m_pDB.get()) return false;
    if (NULL == m_pDS.get()) return false;

    // get primary genres for movies
    //CStdString strSQL="select * from genrelinkmovie,genre,movie,movieinfo,actors,path where path.idpath=movie.idpath and genrelinkmovie.idGenre=genre.idGenre and genrelinkmovie.idmovie=movie.idmovie and movieinfo.idmovie=movie.idmovie and movieinfo.iddirector=actors.idActor order by path.idpath";
    CStdString strSQL=FormatSQL("select * from movie,movieinfo,actors,path where path.idpath=movie.idpath and movieinfo.idmovie=movie.idmovie and movieinfo.iddirector=actors.idActor and NOT(movieinfo.bWatched='true') order by path.idpath");

    // run query
    CLog::Log(LOGDEBUG, "CVideoDatabase::GetGenresNav() query: %s", strSQL.c_str());
    if (!m_pDS->query(strSQL.c_str())) return false;
    int iRowsFound = m_pDS->num_rows();
    if (iRowsFound == 0)
    {
      m_pDS->close();
      return false;
    }

    map<long, CStdString> mapYears;
    map<long, CStdString>::iterator it;
    long lLastPathId = -1;
    while (!m_pDS->eof())
    {
      CStdString strYear = m_pDS->fv("movieinfo.iYear").get_asString();
      long lYear = atol(strYear.c_str());
      it = mapYears.find(lYear);
      if (it == mapYears.end())
      {
        // is the path the same as the previous path?
        long lPathId = m_pDS->fv("path.idpath").get_asLong();
        if (lPathId == lLastPathId)
          mapYears.insert(pair<long, CStdString>(lYear, strYear));
        // test path
        else
        {
          CStdString strPath = m_pDS->fv("path.strPath").get_asString();
          if (g_passwordManager.IsDatabasePathUnlocked(strPath, g_settings.m_vecMyVideoShares))
          {
            // the path is unlocked so set last path to current path
            lLastPathId = lPathId;
            mapYears.insert(pair<long, CStdString>(lYear, strYear));
          }
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

    return true;
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "CVideoDatabase:GetYearsNav() failed");
  }
  return false;
}

bool CVideoDatabase::GetTitlesNav(const CStdString& strBaseDir, CFileItemList& items, long idGenre, long idYear, long idActor)
{
  try
  {
    DWORD time = timeGetTime();

    if (NULL == m_pDB.get()) return false;
    if (NULL == m_pDS.get()) return false;
    //CStdString strSQL="select * from movie,movieinfo,actors,path,files where movieinfo.idmovie=movie.idmovie and movieinfo.iddirector=actors.idActor and movie.idpath=path.idpath and files.idmovie = movie.idmovie";

    CStdString strWhere;
    //CStdString strSQL=FormatSQL("select * from movie,movieinfo,actors,path,files where path.idpath=movie.idpath and movieinfo.idmovie=movie.idmovie and movieinfo.iddirector=actors.idActor and movieinfo.iYear=%i and files.idmovie = movie.idmovie", iYear);
    CStdString strSQL = FormatSQL("select * from movie,movieinfo,actors,path,files where movieinfo.idmovie=movie.idmovie and movieinfo.iddirector=actors.idActor and movie.idpath=path.idpath and files.idmovie = movie.idmovie");

    if (idGenre != -1)
    {
      strSQL=FormatSQL("select * from movie,genre,genrelinkmovie,movieinfo,actors,path,files where movieinfo.idmovie=movie.idmovie and movieinfo.iddirector=actors.idActor and movie.idpath=path.idpath and files.idmovie = movie.idmovie and genrelinkmovie.idGenre=genre.idGenre and genrelinkmovie.idmovie=movie.idmovie and genre.idGenre=%u", idGenre);
    }

    if (idYear !=-1)
    {
      strWhere += FormatSQL(" and movieinfo.iYear=%i", idYear);
    }

    if (idActor != -1)
    {
        strSQL =  FormatSQL("select * from movie,actorlinkmovie,movieinfo,actors,path,files where movieinfo.idmovie=movie.idmovie and movie.idpath=path.idpath and files.idmovie = movie.idmovie and actors.idactor=actorlinkmovie.idActor and actorlinkMovie.idmovie = movie.idmovie and actors.idactor=%i",idActor);
    }

    if (g_stSettings.m_iMyVideoWatchMode == 1)
      strSQL += FormatSQL(" and NOT (movieinfo.bWatched='true')");

    if (g_stSettings.m_iMyVideoWatchMode == 2)
      strSQL += FormatSQL(" and movieinfo.bWatched='true'");

    strSQL += strWhere;

    // get all songs out of the database in fixed size chunks
    // dont reserve the items ahead of time just in case it fails part way though
    VECMOVIES movies;
    if (idGenre == -1 && idYear == -1 && idActor == -1)
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
              return false; // failed on first iteration, so there's probably no songs in the db
            else
            {
              CGUIWindowVideoBase::SetDatabaseDirectory(movies,items,true);
              return true; // there no more songs left to process (aborts the unbounded for loop)
            }
          }

          // get movies from returned subtable
          while (!m_pDS->eof())
          {
            CIMDBMovie movie = GetDetailsFromDataset(m_pDS);
            movies.push_back(movie);
            iSONGS++;
            m_pDS->next();
          }
        }
        catch (...)
        {
          CLog::Log(LOGERROR, "CVideoDatabase::GetTitlesNav() failed at iteration %i, num songs %i", iITERATIONS, iSONGS);

          if (iSONGS > 0)
          {
            CGUIWindowVideoBase::SetDatabaseDirectory(movies,items,true);
            return true; // keep whatever songs we may have gotten before the failure
          }
          else
            return false; // no songs, return false
        }
        // next iteration
        iITERATIONS++;
        m_pDS->close();
      }
      CGUIWindowVideoBase::SetDatabaseDirectory(movies,items,true);
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
      CIMDBMovie movie = GetDetailsFromDataset(m_pDS);
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
    CLog::Log(LOGERROR, "CVideoDatabase::GetTitlesNav() failed");
  }
  return false;
}

bool CVideoDatabase::GetGenreById(long lIdGenre, CStdString& strGenre)
{
  try
  {
    if (NULL == m_pDB.get()) return false;
    if (NULL == m_pDS.get()) return false;

    CStdString strSQL=FormatSQL("select * from genre where genre.idGenre=%u", lIdGenre);
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

    CLog::Log(LOGDEBUG, __FUNCTION__" Cleaning movieinfo table");
    sql = "delete from movieinfo where idMovie in " + moviesToDelete;
    m_pDS->exec(sql.c_str());

    CLog::Log(LOGDEBUG, __FUNCTION__" Cleaning actorlinkmovie table");
    sql = "delete from actorlinkmovie where idMovie in " + moviesToDelete;
    m_pDS->exec(sql.c_str());

    CLog::Log(LOGDEBUG, __FUNCTION__" Cleaning actor table of actors and directors");
    sql = "delete from actors where idActor not in (select distinct idActor from actorlinkmovie) and idActor not in (select distinct idDirector from movieinfo)";
    m_pDS->exec(sql.c_str());

    CLog::Log(LOGDEBUG, __FUNCTION__" Cleaning genrelinkmovie table");
    sql = "delete from genrelinkmovie where idMovie in " + moviesToDelete;
    m_pDS->exec(sql.c_str());

    CLog::Log(LOGDEBUG, __FUNCTION__" Cleaning genre table");
    sql = "delete from genre where idGenre not in (select distinct idGenre from genrelinkmovie)";
    m_pDS->exec(sql.c_str());
 
    CommitTransaction();

    Compress();

    if (progress)
      progress->Close();
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "CVideoDatabase::CleanDatabase() failed");
  }
}