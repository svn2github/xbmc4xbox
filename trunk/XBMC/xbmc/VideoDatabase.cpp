//********************************************************************************************************************************
//**
//** see docs/videodatabase.png for a diagram of the database
//**
//********************************************************************************************************************************

#include "stdafx.h"
#include ".\videodatabase.h"
#include "settings.h"
#include "utils/fstrcmp.h"
#include "utils/log.h"
#include "util.h"

//********************************************************************************************************************************
CVideoDatabase::CVideoDatabase(void)
{
}

//********************************************************************************************************************************
CVideoDatabase::~CVideoDatabase(void)
{
}

//********************************************************************************************************************************
void CVideoDatabase::Split(const CStdString& strFileNameAndPath, CStdString& strPath, CStdString& strFileName)
{
	strFileName="";
	strPath="";
	int i=strFileNameAndPath.size()-1;
	while (i > 0)
	{
		char ch=strFileNameAndPath[i];
		if (ch==':' || ch=='/' || ch=='\\') break;
		else i--;
	}
	strPath     = strFileNameAndPath.Left(i);
	strFileName = strFileNameAndPath.Right(strFileNameAndPath.size() - i);
}

//********************************************************************************************************************************
void CVideoDatabase::RemoveInvalidChars(CStdString& strTxt)
{
	CStdString strReturn="";
	for (int i=0; i < (int)strTxt.size(); ++i)
	{
		byte k=strTxt[i];
		if (k==0x27) 
		{
			strReturn += k;
		}
		strReturn += k;
	}
	if (strReturn=="") 
		strReturn="unknown";
	strTxt=strReturn;
}



//********************************************************************************************************************************
bool CVideoDatabase::Open()
{
	CStdString videoDatabase=g_stSettings.m_szAlbumDirectory;
	videoDatabase+="\\MyVideos31.db";

	Close();

	// test id dbs already exists, if not we need 2 create the tables
	bool bDatabaseExists=false;
	FILE* fd= fopen(videoDatabase.c_str(),"rb");
	if (fd)
	{
		bDatabaseExists=true;
		fclose(fd);
	}

	m_pDB.reset(new SqliteDatabase() ) ;
  m_pDB->setDatabase(videoDatabase.c_str());
	
  m_pDS.reset(m_pDB->CreateDataset());
	if ( m_pDB->connect() != DB_CONNECTION_OK) 
	{
    CLog::Log(LOGERROR, "videodatabase::unable to open %s",videoDatabase.c_str());
		Close();
    ::DeleteFile(videoDatabase.c_str());
		return false;
	}

	if (!bDatabaseExists) 
	{
		if (!CreateTables()) 
		{
      CLog::Log(LOGERROR, "videodatabase::unable to create %s",videoDatabase.c_str());
			Close();
      ::DeleteFile(videoDatabase.c_str());
			return false;
		}
	}

	m_pDS->exec("PRAGMA cache_size=8192\n");
	m_pDS->exec("PRAGMA synchronous='OFF'\n");
	m_pDS->exec("PRAGMA count_changes='OFF'\n");
//	m_pDS->exec("PRAGMA temp_store='MEMORY'\n");
	return true;
}


//********************************************************************************************************************************
void CVideoDatabase::Close()
{
	if (NULL==m_pDB.get() ) return;
  if (NULL!=m_pDS.get()) m_pDS->close();
	m_pDB->disconnect();
	m_pDB.reset();
}

//********************************************************************************************************************************
bool CVideoDatabase::CreateTables()
{

  try 
	{
    CLog::Log(LOGINFO, "create bookmark table");
    m_pDS->exec("CREATE TABLE bookmark ( idBookmark integer primary key, idFile integer, fPercentage text)\n");
		
    CLog::Log(LOGINFO, "create genre table");
    m_pDS->exec("CREATE TABLE genre ( idGenre integer primary key, strGenre text)\n");
    
    CLog::Log(LOGINFO, "create genrelinkmovie table");
    m_pDS->exec("CREATE TABLE genrelinkmovie ( idGenre integer, idMovie integer)\n");
    
    CLog::Log(LOGINFO, "create movie table");
    m_pDS->exec("CREATE TABLE movie ( idMovie integer primary key, idPath integer, hasSubtitles integer, cdlabel text)\n");
    
    CLog::Log(LOGINFO, "create movieinfo table");
    m_pDS->exec("CREATE TABLE movieinfo ( idMovie integer, idDirector integer, strPlotOutline text, strPlot text, strTagLine text, strVotes text, strRuntime text, fRating text, strCast text,strCredits text, iYear integer, strGenre text, strPictureURL text, strTitle text, IMDBID text)\n");
    
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
    CLog::Log(LOGERROR, "videodatabase::unable to create tables:%i",GetLastError());
		return false;
	}

	return true;
}

//********************************************************************************************************************************
long CVideoDatabase::AddFile(long lMovieId, long lPathId, const CStdString& strFileName)
{
	CStdString strSQL="";
  try
  {
    long lFileId;
    if (NULL==m_pDB.get()) return -1;
	  if (NULL==m_pDS.get()) return -1;

    strSQL.Format("select * from files where idmovie=%i and idpath=%i and strFileName like '%s'",lMovieId,lPathId,strFileName.c_str());
    m_pDS->query(strSQL.c_str());
    if (m_pDS->num_rows() > 0) 
    {
      lFileId=m_pDS->fv("idFile").get_asLong() ;
      m_pDS->close();
      return lFileId;
    }
    m_pDS->close();
	  strSQL.Format ("insert into files (idFile, idMovie,idPath, strFileName) values(NULL, %i,%i,'%s')", lMovieId,lPathId, strFileName.c_str() );
	  m_pDS->exec(strSQL.c_str());
    lFileId=(long)sqlite3_last_insert_rowid( m_pDB->getHandle() );
    return lFileId;
  }
  catch(...)
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
    lPathId=-1;
    lMovieId=-1;
    if (NULL==m_pDB.get()) return -1;
	  if (NULL==m_pDS.get()) return -1;
	  CStdString strPath, strFileName ;
	  Split(strFilenameAndPath, strPath, strFileName); 
    RemoveInvalidChars(strPath);
    
    lPathId = GetPath(strPath);
    if (lPathId < 0) return -1;

	  CStdString strSQL;
    strSQL.Format("select * from files where idpath=%i",lPathId);
    m_pDS->query(strSQL.c_str());
    if (m_pDS->num_rows() > 0) 
    {
      while (!m_pDS->eof()) 
      {
        CStdString strFname= m_pDS->fv("strFilename").get_asString() ;
        if (bExact)
        {
          if (strFname==strFileName)
		      {
		        // was just returning 'true' here, but this caused problems with
			      // the bookmarks as these are stored by fileid. forza.
			      long lFileId=m_pDS->fv("idFile").get_asLong() ;
			      lMovieId=m_pDS->fv("idMovie").get_asLong() ;
            m_pDS->close();
            return lFileId;
  			    //return true;
		      }
        }
        else
        {
          double fPercentage=fstrcmp(strFname.c_str(),strFileName.c_str(), COMPARE_PERCENTAGE_MIN );
          if ( fPercentage >=COMPARE_PERCENTAGE)
          {
            long lFileId=m_pDS->fv("idFile").get_asLong() ;
            lMovieId=m_pDS->fv("idMovie").get_asLong() ;
            m_pDS->close();
            return lFileId;
          }
        }
        m_pDS->next();
      }
      m_pDS->close();
    }
  }
  catch(...)
  {
    CLog::Log(LOGERROR, "CVideoDatabase::GetFile(%s) failed",strFilenameAndPath.c_str());
  }
  return -1;
}


//********************************************************************************************************************************
long CVideoDatabase::AddPath(const CStdString& strPath)
{
  try
  {
    if (NULL==m_pDB.get()) return -1;
	  if (NULL==m_pDS.get()) return -1;
	  CStdString strSQL;
    strSQL.Format("select * from path where strPath like '%s'",strPath.c_str());
	  m_pDS->query(strSQL.c_str());
	  if (m_pDS->num_rows() == 0) 
	  {
      m_pDS->close();
		  // doesnt exists, add it
		  strSQL.Format("insert into Path (idPath, strPath) values( NULL, '%s')",
                        strPath.c_str());
		  m_pDS->exec(strSQL.c_str());
		  long lPathId=(long)sqlite3_last_insert_rowid(m_pDB->getHandle());
		  return lPathId;
	  }
	  else
	  {
		  const field_value value = m_pDS->fv("idPath");
		  long lPathId=value.get_asLong() ;
      m_pDS->close();
		  return lPathId;
	  }

  }
  catch(...)
  {
    CLog::Log(LOGERROR, "CVideoDatabase::AddPath(%s) failed",strPath.c_str());
  }
	return -1;
}

//********************************************************************************************************************************
long CVideoDatabase::GetPath(const CStdString& strPath)
{
  try
  {
    if (NULL==m_pDB.get()) return -1;
	  if (NULL==m_pDS.get()) return -1;
	  CStdString strSQL;
    strSQL.Format("select * from path where strPath like '%s' ",strPath.c_str());
	  m_pDS->query(strSQL.c_str());
	  if (m_pDS->num_rows() > 0) 
	  {
      long lPathId = m_pDS->fv("idPath").get_asLong();
      m_pDS->close();
		  return lPathId;
	  }
    m_pDS->close();
  }
  catch(...)
  {
    CLog::Log(LOGERROR, "CVideoDatabase::GetPath(%s) failed",strPath.c_str());
  }
	return -1;
}

//********************************************************************************************************************************
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

int CVideoDatabase::GetRecentMovies(long* pMovieIdArray, int nSize)
{
	int count = 0;

	try
	{
		if (NULL==m_pDB.get())
		{
			return -1;
		}

		if (NULL==m_pDS.get())
		{
			return -1;
		}
  
		CStdString strSQL;
		strSQL.Format("select * from movie order by idMovie desc limit %d",nSize); 
		if (m_pDS->query(strSQL.c_str()))
		{
			while ((!m_pDS->eof()) && (count<nSize))
			{
				pMovieIdArray[count++] = m_pDS->fv("idMovie").get_asLong() ;			
				m_pDS->next();
			}
      m_pDS->close();
		}
	}
	catch(...)
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
    if (NULL==m_pDB.get()) return -1;
	  if (NULL==m_pDS.get()) return -1;
	  CStdString strPath, strFileName, strCDLabel=strcdLabel;
	  Split(strFilenameAndPath, strPath, strFileName); 
    RemoveInvalidChars(strPath);
    RemoveInvalidChars(strFileName);
    RemoveInvalidChars(strCDLabel);
    
    long lMovieId = GetMovie(strFilenameAndPath);
    if (lMovieId < 0)
    {
	    CStdString strSQL;

      long lPathId = AddPath(strPath);
      if (lPathId < 0) return -1;
      strSQL.Format("insert into movie (idMovie, idPath, hasSubtitles, cdlabel) values( NULL, %i, %i, '%s')",
                    lPathId,bHassubtitles,strCDLabel.c_str());
	    m_pDS->exec(strSQL.c_str());
      lMovieId=(long)sqlite3_last_insert_rowid(m_pDB->getHandle());
      AddFile(lMovieId,lPathId,strFileName);
    }
    else
    {
      long lPathId = GetPath(strPath);
      if (lPathId < 0) return -1;
      AddFile(lMovieId,lPathId,strFileName);
    }
    return lMovieId;
  }
  catch(...)
  {
    CLog::Log(LOGERROR, "CVideoDatabase::AddMovie(%s,%s) failed",strFilenameAndPath.c_str() , strcdLabel.c_str() );
  }
  return -1;
}


//********************************************************************************************************************************
long CVideoDatabase::AddGenre(const CStdString& strGenre1)
{
  try
  {
	  CStdString strGenre=strGenre1;
	  RemoveInvalidChars(strGenre);

	  if (NULL==m_pDB.get()) return -1;
	  if (NULL==m_pDS.get()) return -1;
	  CStdString strSQL="select * from genre where strGenre like '";
	  strSQL += strGenre;
	  strSQL += "'";
	  m_pDS->query(strSQL.c_str());
	  if (m_pDS->num_rows() == 0) 
	  {
      m_pDS->close();
		  // doesnt exists, add it
		  strSQL = "insert into genre (idGenre, strGenre) values( NULL, '" ;
		  strSQL += strGenre;
		  strSQL += "')";
		  m_pDS->exec(strSQL.c_str());
		  long lGenreId=(long)sqlite3_last_insert_rowid(m_pDB->getHandle());
		  return lGenreId;
	  }
	  else
	  {
		  const field_value value = m_pDS->fv("idGenre");
		  long lGenreId=value.get_asLong() ;
      m_pDS->close();
		  return lGenreId;
	  }
  }
  catch(...)
  {
    CLog::Log(LOGERROR, "CVideoDatabase::AddGenre(%s) failed",strGenre1.c_str() );
  }

	return -1;
}


//********************************************************************************************************************************
long CVideoDatabase::AddActor(const CStdString& strActor1)
{
  try
  {
	  CStdString strActor=strActor1;
	  RemoveInvalidChars(strActor);

	  if (NULL==m_pDB.get()) return -1;
	  if (NULL==m_pDS.get()) return -1;
	  CStdString strSQL="select * from Actors where strActor like '";
	  strSQL += strActor;
	  strSQL += "'";
	  m_pDS->query(strSQL.c_str());
	  if (m_pDS->num_rows() == 0) 
	  {
      m_pDS->close();
		  // doesnt exists, add it
		  strSQL = "insert into Actors (idActor, strActor) values( NULL, '" ;
		  strSQL += strActor;
		  strSQL += "')";
		  m_pDS->exec(strSQL.c_str());
		  long lActorId=(long)sqlite3_last_insert_rowid(m_pDB->getHandle());
		  return lActorId;
	  }
	  else
	  {
		  const field_value value = m_pDS->fv("idActor");
		  long lActorId=value.get_asLong() ;
      m_pDS->close();
		  return lActorId;
	  }

  }
  catch(...)
  {
    CLog::Log(LOGERROR, "CVideoDatabase::AddActor(%s) failed",strActor1.c_str() );
  }
	return -1;
}
//********************************************************************************************************************************
void CVideoDatabase::AddGenreToMovie(long lMovieId, long lGenreId)
{
  try
  {
	  if (NULL==m_pDB.get()) return ;
	  if (NULL==m_pDS.get()) return ;
	  CStdString strSQL;
    strSQL.Format("select * from genrelinkmovie where idGenre=%i and idMovie=%i",lGenreId,lMovieId);
	  m_pDS->query(strSQL.c_str());
	  if (m_pDS->num_rows() == 0) 
	  {
		  // doesnt exists, add it
		  strSQL.Format ("insert into genrelinkmovie (idGenre, idMovie) values( %i,%i)",lGenreId,lMovieId);
		  m_pDS->exec(strSQL.c_str());
	  }
    m_pDS->close();
  }
  catch(...)
  {
    CLog::Log(LOGERROR, "CVideoDatabase::AddGenreToMovie() failed");
  }
}

//********************************************************************************************************************************
void CVideoDatabase::AddActorToMovie(long lMovieId, long lActorId)
{
  try
  {
	  if (NULL==m_pDB.get()) return ;
	  if (NULL==m_pDS.get()) return ;
	  CStdString strSQL;
    strSQL.Format("select * from actorlinkmovie where idActor=%i and idMovie=%i",lActorId,lMovieId);
	  m_pDS->query(strSQL.c_str());
	  if (m_pDS->num_rows() == 0) 
	  {
		  // doesnt exists, add it
		  strSQL.Format ("insert into actorlinkmovie (idActor, idMovie) values( %i,%i)",lActorId,lMovieId);
		  m_pDS->exec(strSQL.c_str());
	  }
    m_pDS->close();
  }
  catch(...)
  {
    CLog::Log(LOGERROR, "CVideoDatabase::AddActorToMovie() failed");
  }
}

//********************************************************************************************************************************
void CVideoDatabase::GetGenres(VECMOVIEGENRES& genres)
{
  try
  {
    genres.erase(genres.begin(),genres.end());
	  if (NULL==m_pDB.get()) return ;
	  if (NULL==m_pDS.get()) return ;
    m_pDS->query("select * from genre order by strGenre");
    while (!m_pDS->eof()) 
    {
      genres.push_back( m_pDS->fv("strGenre").get_asString() );
      m_pDS->next();
    }
    m_pDS->close();
  }
  catch(...)
  {
    CLog::Log(LOGERROR, "CVideoDatabase::GetGenres() failed");
  }
}

//********************************************************************************************************************************
void CVideoDatabase::GetActors(VECMOVIEACTORS& actors)
{
  try
  {
    actors.erase(actors.begin(),actors.end());
	  if (NULL==m_pDB.get()) return ;
	  if (NULL==m_pDS.get()) return ;
    m_pDS->query("select * from actors");
    while (!m_pDS->eof()) 
    {
      actors.push_back( m_pDS->fv("strActor").get_asString() );
      m_pDS->next();
    }
    m_pDS->close();
  }
  catch(...)
  {
    CLog::Log(LOGERROR, "CVideoDatabase::GetActors() failed");
  }
}

//********************************************************************************************************************************
bool CVideoDatabase::HasMovieInfo(const CStdString& strFilenameAndPath)
{
  try
  {
    if (NULL==m_pDB.get()) return false;
	  if (NULL==m_pDS.get()) return false;
    long lMovieId=GetMovie(strFilenameAndPath);
    if ( lMovieId < 0) return false;
    CStdString strSQL;
    strSQL.Format("select * from movieinfo where movieinfo.idmovie=%i",lMovieId);
	  m_pDS->query(strSQL.c_str());
	  if (m_pDS->num_rows() == 0) 
    {
      m_pDS->close();
      return false;
    }
    m_pDS->close();
    return true;
  }
  catch(...)
  {
    CLog::Log(LOGERROR, "CVideoDatabase::HasMovieInfo(%s) failed",strFilenameAndPath.c_str());
  }
  return false;

}

//********************************************************************************************************************************
bool CVideoDatabase::HasSubtitle(const CStdString& strFilenameAndPath)
{
  try
  {
    if (NULL==m_pDB.get()) return false;
	  if (NULL==m_pDS.get()) return false;
   
    long lMovieId=GetMovie(strFilenameAndPath);
    if ( lMovieId < 0) return false;

	  CStdString strSQL;
    strSQL.Format("select * from movie where movie.idMovie=%i",lMovieId);
	  m_pDS->query(strSQL.c_str());
    long lHasSubs = 0;
	  if (m_pDS->num_rows() > 0) 
    {
	    lHasSubs = m_pDS->fv("hasSubtitles").get_asLong() ;
    }
    m_pDS->close();
    return (lHasSubs!=0);
  }
  catch(...)
  {
    CLog::Log(LOGERROR, "CVideoDatabase::HasSubtitle(%s) failed",strFilenameAndPath.c_str());
  }
  return false;
}

//********************************************************************************************************************************
void CVideoDatabase::DeleteMovieInfo(const CStdString& strFileNameAndPath)
{
  try
  {
    if (NULL==m_pDB.get()) return ;
	  if (NULL==m_pDS.get()) return ;
    
    long lMovieId=GetMovie(strFileNameAndPath);
    if ( lMovieId < 0) return ;
    
    CStdString strSQL;
    strSQL.Format("delete from genrelinkmovie where idmovie=%i", lMovieId);
    m_pDS->exec(strSQL.c_str());

    strSQL.Format("delete from actorlinkmovie where idmovie=%i", lMovieId);
    m_pDS->exec(strSQL.c_str());

    strSQL.Format("delete from movieinfo where idmovie=%i", lMovieId);
    m_pDS->exec(strSQL.c_str());  
  }
  catch(...)
  {
    CLog::Log(LOGERROR, "CVideoDatabase::DeleteMovieInfo(%s) failed",strFileNameAndPath.c_str());
  }
}



//********************************************************************************************************************************
void CVideoDatabase::GetMoviesByGenre(CStdString& strGenre1, VECMOVIES& movies)
{
  try
  {
 	  CStdString strGenre=strGenre1;
	  RemoveInvalidChars(strGenre);

    movies.erase(movies.begin(),movies.end());
	  if (NULL==m_pDB.get()) return ;
	  if (NULL==m_pDS.get()) return ;
    CStdString strSQL;
    strSQL.Format("select * from genrelinkmovie,genre,movie,movieinfo,actors,path where path.idpath=movie.idpath and genrelinkmovie.idGenre=genre.idGenre and genrelinkmovie.idmovie=movie.idmovie and movieinfo.idmovie=movie.idmovie and genre.strGenre='%s' and movieinfo.iddirector=actors.idActor", strGenre.c_str());

    m_pDS->query( strSQL.c_str() );
    while (!m_pDS->eof()) 
    {
      movies.push_back(GetDetailsFromDataset(m_pDS));
      m_pDS->next();
    }
    m_pDS->close();
  }
  catch(...)
  {
    CLog::Log(LOGERROR, "CVideoDatabase::GetMoviesByGenre(%s) failed",strGenre1.c_str());
  }
}

//********************************************************************************************************************************
void CVideoDatabase::GetMoviesByActor(CStdString& strActor1, VECMOVIES& movies)
{
  try
  {
    CStdString strActor=strActor1;
	  RemoveInvalidChars(strActor);

    movies.erase(movies.begin(),movies.end());
	  if (NULL==m_pDB.get()) return ;
	  if (NULL==m_pDS.get()) return ;
    CStdString strSQL;
    strSQL.Format("select * from actorlinkmovie,actors,movie,movieinfo,path where path.idpath=movie.idpath and actors.idActor=actorlinkmovie.idActor and actorlinkmovie.idmovie=movie.idmovie and movieinfo.idmovie=movie.idmovie and actors.stractor='%s'", strActor.c_str());
    m_pDS->query( strSQL.c_str() );
    while (!m_pDS->eof()) 
    {
      movies.push_back(GetDetailsFromDataset(m_pDS));
      m_pDS->next();
    }
    m_pDS->close();
  }
  catch(...)
  {
    CLog::Log(LOGERROR, "CVideoDatabase::GetMoviesByActor(%s) failed",strActor1.c_str());
  }
}


//********************************************************************************************************************************
void CVideoDatabase::GetMovieInfo(const CStdString& strFilenameAndPath,CIMDBMovie& details)
{
  try
  {
    long lMovieId=GetMovie(strFilenameAndPath);
    if (lMovieId<0) return;

	  CStdString strSQL;
    strSQL.Format("select * from movieinfo,actors,movie,path where path.idpath=movie.idpath and movie.idMovie=movieinfo.idMovie and movieinfo.idmovie=%i and idDirector=idActor", lMovieId);
    
	  m_pDS->query(strSQL.c_str());
    if (m_pDS->num_rows() > 0) 
    {
			details = GetDetailsFromDataset(m_pDS);
    }
    m_pDS->close();
  }
  catch(...)
  {
    CLog::Log(LOGERROR, "CVideoDatabase::GetMovieInfo(%s) failed",strFilenameAndPath.c_str());
  }
}

//********************************************************************************************************************************
void CVideoDatabase::SetMovieInfo(const CStdString& strFilenameAndPath, CIMDBMovie& details)
{
  try
  {
    long lMovieId=GetMovie(strFilenameAndPath);
    if (lMovieId< 0) return;
    details.m_strSearchString.Format("%i", lMovieId);

	  CStdString strPath, strFileName ;
	  Split(strFilenameAndPath, strPath, strFileName); 
    details.m_strPath=strPath;
    details.m_strFile=strFileName;

    CIMDBMovie details1=details;
    RemoveInvalidChars(details1.m_strCast);
    RemoveInvalidChars(details1.m_strDirector);  
    RemoveInvalidChars(details1.m_strPlot);  
    RemoveInvalidChars(details1.m_strPlotOutline);  
    RemoveInvalidChars(details1.m_strTagLine);  
    RemoveInvalidChars(details1.m_strPictureURL);  
    RemoveInvalidChars(details1.m_strSearchString);  
    RemoveInvalidChars(details1.m_strTitle);  
    RemoveInvalidChars(details1.m_strVotes);  
    RemoveInvalidChars(details1.m_strWritingCredits);  
    RemoveInvalidChars(details1.m_strGenre);  
    RemoveInvalidChars(details1.m_strIMDBNumber);  

    // add director
    long lDirector=AddActor(details.m_strDirector);
    
    // add all genres
    char szGenres[1024];
    strcpy(szGenres,details.m_strGenre.c_str());
    vector<long> vecGenres;
    if (strstr(szGenres,"/"))
    {
      char *pToken=strtok(szGenres,"/");
      while( pToken != NULL )
      {
        CStdString strGenre=pToken; 
        strGenre.Trim();
        long lGenreId=AddGenre(strGenre);
        vecGenres.push_back(lGenreId);
        pToken = strtok( NULL, "/" );
      }
    }
    else
    {
      CStdString strGenre=details.m_strGenre; 
      strGenre.Trim();
      long lGenreId=AddGenre(strGenre);
      vecGenres.push_back(lGenreId);
    }

    // add cast...
    vector<long> vecActors;
    CStdString strCast;
    int ipos=0;
    strCast=details.m_strCast.c_str();
    ipos=strCast.Find(" as ");
    while (ipos > 0)
    {
      CStdString strActor;
      int x=ipos;
      while (x > 0)
      {
        if (strCast[x] != '\r'&& strCast[x]!='\n') x--;
        else 
        {
          x++;
          break;
        }
      }
      strActor=strCast.Mid(x,ipos-x);          
      long lActorId=AddActor(strActor);
      vecActors.push_back(lActorId);
      ipos=strCast.Find(" as ",ipos+3);
    }

    for (int i=0; i < (int)vecGenres.size(); ++i)
    {
      AddGenreToMovie(lMovieId,vecGenres[i]);
    }
    
    for (i=0; i < (int)vecActors.size(); ++i)
    {
      AddActorToMovie(lMovieId,vecActors[i]);
    }

	  CStdString strSQL;
    CStdString strRating;
    strRating.Format("%3.3f", details1.m_fRating);
    if (strRating=="") strRating="0.0";
    strSQL.Format("select * from movieinfo where idmovie=%i", lMovieId);
	  m_pDS->query(strSQL.c_str());
	  if (m_pDS->num_rows() == 0) 
    {
      m_pDS->close();
      CStdString strTmp="";
      strSQL.Format("insert into movieinfo ( idMovie,idDirector,strPlotOutline,strPlot,strTagLine,strVotes,strRuntime,fRating,strCast,strCredits , iYear  ,strGenre, strPictureURL, strTitle,IMDBID) values(%i,%i,'%s','%s','%s','%s','%s','%s','%s','%s',%i,'%s','%s','%s','%s')",
              lMovieId,lDirector, details1.m_strPlotOutline.c_str(),
              details1.m_strPlot.c_str(),details1.m_strTagLine.c_str(),
              details1.m_strVotes.c_str(),details1.m_strRuntime.c_str(), strRating.c_str(),
              details1.m_strCast.c_str(),details1.m_strWritingCredits.c_str(),
              
              details1.m_iYear, details1.m_strGenre.c_str(),
              details1.m_strPictureURL.c_str(),details1.m_strTitle.c_str(),
              details1.m_strIMDBNumber.c_str() );

	    m_pDS->exec(strSQL.c_str());
                
    }
    else
    {
      m_pDS->close();
      strSQL.Format("update movieinfo set idDirector=%i, strPlotOutline='%s', strPlot='%s', strTagLine='%s', strVotes='%s', strRuntime='%s', fRating='%s', strCast='%s',strCredits='%s', iYear=%i, strGenre='%s' strPictureURL='%s', strTitle='%s' IMDBID='%s' where idMovie=%i",
              lDirector,details1.m_strPlotOutline.c_str(),
              details1.m_strPlot.c_str(),details1.m_strTagLine.c_str(),
              details1.m_strVotes.c_str(),details1.m_strRuntime.c_str(),strRating.c_str(),
              details1.m_strCast.c_str(),details1.m_strWritingCredits.c_str(),
              details1.m_iYear,details1.m_strGenre.c_str(),
              details1.m_strPictureURL.c_str(),details1.m_strTitle.c_str(),
              details1.m_strIMDBNumber.c_str(),
              lMovieId);
	    m_pDS->exec(strSQL.c_str());
    }
  }
  catch(...)
  {
    CLog::Log(LOGERROR, "CVideoDatabase::SetMovieInfo(%s) failed",strFilenameAndPath.c_str());
  }
}


//********************************************************************************************************************************
void CVideoDatabase::GetMoviesByPath(CStdString& strPath1, VECMOVIES& movies)
{
  try
  {
 	  CStdString strPath=strPath1;
    if (CUtil::HasSlashAtEnd(strPath)) strPath = strPath.Left(strPath.size()-1);

	  RemoveInvalidChars(strPath);

    movies.erase(movies.begin(),movies.end());
	  if (NULL==m_pDB.get()) return ;
	  if (NULL==m_pDS.get()) return ;
    long lPathId = GetPath(strPath);
    if (lPathId< 0) return;

    CStdString strSQL;
    strSQL.Format("select * from files,movieinfo where files.idpath=%i and files.idMovie=movieinfo.idMovie", lPathId);

    m_pDS->query( strSQL.c_str() );
    while (!m_pDS->eof()) 
    {
      CIMDBMovie details;
      long lMovieId=m_pDS->fv("files.idMovie").get_asLong();
      details.m_strSearchString.Format("%i", lMovieId);
      details.m_strIMDBNumber=m_pDS->fv("movieinfo.IMDBID").get_asString();
      details.m_strFile=m_pDS->fv("files.strFilename").get_asString();
      movies.push_back(details);
      m_pDS->next();
    }
    m_pDS->close();
  }
  catch(...)
  {
    CLog::Log(LOGERROR, "CVideoDatabase::GetMoviesByPath(%s) failed",strPath1.c_str());
  }
}

//********************************************************************************************************************************
void CVideoDatabase::GetFiles(long lMovieId, VECMOVIESFILES& movies)
{
  try
  {
    movies.erase(movies.begin(),movies.end());
	  if (NULL==m_pDB.get()) return ;
	  if (NULL==m_pDS.get()) return ;

	  //long lMovieId=GetMovie(strFile);
	  if (lMovieId < 0) return;
  	
	  CStdString strSQL;
    strSQL.Format("select * from path,files where path.idPath=files.idPath and files.idmovie=%i order by strFilename", lMovieId );
    m_pDS->query( strSQL.c_str() );
    while (!m_pDS->eof()) 
    {
      CStdString strPath,strFile;
      strFile=m_pDS->fv("files.strFilename").get_asString();
      strPath=m_pDS->fv("path.strPath").get_asString();
      strFile=strPath+strFile;
      movies.push_back(strFile);
      m_pDS->next();
    }
    m_pDS->close();
  }
  catch(...)
  {
    CLog::Log(LOGERROR, "CVideoDatabase::GetFiles() failed");
  }
}

//********************************************************************************************************************************
void CVideoDatabase::GetYears(VECMOVIEYEARS& years)
{
  try
  {
    years.erase(years.begin(),years.end());
	  if (NULL==m_pDB.get()) return ;
	  if (NULL==m_pDS.get()) return ;
    m_pDS->query("select * from movieinfo");
    while (!m_pDS->eof()) 
    {
      CStdString strYear=m_pDS->fv("iYear").get_asString();
      bool bAdd=true;
      for (int i=0; i < (int)years.size();++i)
      {
        if (strYear == years[i]) bAdd=false;
      }
      if (bAdd) years.push_back( strYear);
      m_pDS->next();
    }
    m_pDS->close();

  }
  catch(...)
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
    movies.erase(movies.begin(),movies.end());
	  if (NULL==m_pDB.get()) return ;
	  if (NULL==m_pDS.get()) return ;
    CStdString strSQL;
    strSQL.Format("select * from movie,movieinfo,actors,path where path.idpath=movie.idpath and movieinfo.idmovie=movie.idmovie and movieinfo.iddirector=actors.idActor and movieinfo.iYear=%i",iYear);

    m_pDS->query( strSQL.c_str() );
    while (!m_pDS->eof()) 
    {
      movies.push_back(GetDetailsFromDataset(m_pDS));
      m_pDS->next();
    }
    m_pDS->close();
  }
  catch(...)
  {
    CLog::Log(LOGERROR, "CVideoDatabase::GetMoviesByYear(%s) failed",strYear.c_str());
  }
}

//********************************************************************************************************************************
void CVideoDatabase::GetBookMarksForMovie(const CStdString& strFilenameAndPath, VECBOOKMARKS& bookmarks)
{
  try
  {
    long lPathId, lMovieId;
    long lFileId=GetFile(strFilenameAndPath, lPathId, lMovieId, true);
    if (lFileId < 0) return;
    bookmarks.erase(bookmarks.begin(),bookmarks.end());
	  if (NULL==m_pDB.get()) return ;
	  if (NULL==m_pDS.get()) return ;

    CStdString strSQL;
    strSQL.Format("select * from bookmark where idFile=%i order by fPercentage", lFileId);
    m_pDS->query( strSQL.c_str() );
    while (!m_pDS->eof()) 
    {
      float fPercentage=m_pDS->fv("fPercentage").get_asFloat();
      bookmarks.push_back(fPercentage);
      m_pDS->next();
    }
    m_pDS->close();
  }
  catch(...)
  {
    CLog::Log(LOGERROR, "CVideoDatabase::GetBookMarksForMovie(%s) failed",strFilenameAndPath.c_str());
  }
}

//********************************************************************************************************************************
void CVideoDatabase::AddBookMarkToMovie(const CStdString& strFilenameAndPath, float fPercentage)
{
  try
  {
    long lPathId, lMovieId;
    long lFileId=GetFile(strFilenameAndPath, lPathId, lMovieId, true);
    if (lFileId < 0) return;
	  if (NULL==m_pDB.get()) return ;
	  if (NULL==m_pDS.get()) return ;
    CStdString strSQL;
    strSQL.Format("select * from bookmark where idFile=%i and fPercentage=%03.3f",lFileId,fPercentage);
    m_pDS->query( strSQL.c_str() );
    if (m_pDS->num_rows() != 0)
    {
      m_pDS->close();
      return;
    }
    m_pDS->close();
	  strSQL.Format ("insert into bookmark (idBookmark, idFile, fPercentage) values(NULL,%i,%03.3f)", lFileId,fPercentage);
	  m_pDS->exec(strSQL.c_str());
  }
  catch(...)
  {
    CLog::Log(LOGERROR, "CVideoDatabase::AddBookMarkToMovie(%s) failed",strFilenameAndPath.c_str());
  }
}

//********************************************************************************************************************************
void CVideoDatabase::ClearBookMarksOfMovie(const CStdString& strFilenameAndPath)
{
  try
  {
    long lPathId, lMovieId;
    long lFileId=GetFile(strFilenameAndPath, lPathId, lMovieId, true);
    if (lFileId < 0) return;
	  if (NULL==m_pDB.get()) return ;
	  if (NULL==m_pDS.get()) return ;
    CStdString strSQL;
    strSQL.Format("delete from bookmark where idFile=%i",lFileId);
	  m_pDS->exec(strSQL.c_str());
  }
  catch(...)
  {
    CLog::Log(LOGERROR, "CVideoDatabase::ClearBookMarksOfMovie(%s) failed",strFilenameAndPath.c_str());
  }
}

//********************************************************************************************************************************
void  CVideoDatabase::GetMovies(VECMOVIES& movies)
{	
  try
  {
    movies.erase(movies.begin(),movies.end());
	  if (NULL==m_pDB.get()) return ;
	  if (NULL==m_pDS.get()) return ;
    CStdString strSQL;
    strSQL.Format("select * from movie,movieinfo,actors,path where movieinfo.idmovie=movie.idmovie and movieinfo.iddirector=actors.idActor and movie.idpath=path.idpath");

    m_pDS->query( strSQL.c_str() );
    while (!m_pDS->eof()) 
    {
      movies.push_back(GetDetailsFromDataset(m_pDS));
      m_pDS->next();
    }
    m_pDS->close();
  }
  catch(...)
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
 	  if (NULL==m_pDB.get()) return ;
	  if (NULL==m_pDS.get()) return ;
    if (GetFile(strFilenameAndPath, lPathId, lMovieId) < 0)
    {
      return ;
    }

    ClearBookMarksOfMovie(strFilenameAndPath);

    CStdString strSQL;
    strSQL.Format("delete from files where idmovie=%i", lMovieId);
    m_pDS->exec(strSQL.c_str());

    strSQL.Format("delete from genrelinkmovie where idmovie=%i", lMovieId);
    m_pDS->exec(strSQL.c_str());

    strSQL.Format("delete from actorlinkmovie where idmovie=%i", lMovieId);
    m_pDS->exec(strSQL.c_str());

    strSQL.Format("delete from movieinfo where idmovie=%i", lMovieId);
    m_pDS->exec(strSQL.c_str());

    strSQL.Format("delete from movie where idmovie=%i", lMovieId);
    m_pDS->exec(strSQL.c_str());
  }
  catch(...)
  {
    CLog::Log(LOGERROR, "CVideoDatabase::DeleteMovie() failed");
  }
}

//********************************************************************************************************************************
void CVideoDatabase::SetDVDLabel(long lMovieId, const CStdString& strDVDLabel1)
{
  CStdString strDVDLabel=strDVDLabel1;
	RemoveInvalidChars(strDVDLabel);
  try
  {
    if (NULL==m_pDB.get()) return ;
	  if (NULL==m_pDS.get()) return ;
    CLog::Log(LOGERROR, "setdvdlabel:id:%i label:%s",lMovieId,strDVDLabel1.c_str());

	  CStdString strSQL;

    strSQL.Format("update movie set cdlabel='%s' where idMovie=%i", strDVDLabel1.c_str(), lMovieId );
    m_pDS->exec(strSQL.c_str());
  }
  catch(...)
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
    if (NULL==m_pDB.get()) return;
	  if (NULL==m_pDS.get()) return;

	  CStdString strSQL;

    strSQL.Format("select cdlabel from movie where idMovie=%i", lMovieId);
    m_pDS->query(strSQL.c_str());
    if (m_pDS->num_rows() == 1) {
      strDVDLabel=m_pDS->fv("cdlabel").get_asString();
    }
    m_pDS->close();
  }
  catch(...)
  {
    CLog::Log(LOGERROR, "CVideoDatabase::GetDVDLabel() failed");
  }
}

CIMDBMovie CVideoDatabase::GetDetailsFromDataset(auto_ptr<Dataset> pDS)
{
	CIMDBMovie details;
  details.m_fRating=(float)atof(pDS->fv("movieinfo.fRating").get_asString().c_str()) ;
	details.m_strDirector=pDS->fv("actors.strActor").get_asString();
	details.m_strWritingCredits=pDS->fv("movieinfo.strCredits").get_asString();
	details.m_strTagLine=pDS->fv("movieinfo.strTagLine").get_asString();
	details.m_strPlotOutline=pDS->fv("movieinfo.strPlotOutline").get_asString();
	details.m_strPlot=pDS->fv("movieinfo.strPlot").get_asString();
	details.m_strVotes=pDS->fv("movieinfo.strVotes").get_asString();
	details.m_strRuntime=pDS->fv("movieinfo.strRuntime").get_asString();
	details.m_strCast=pDS->fv("movieinfo.strCast").get_asString();
	details.m_iYear=pDS->fv("movieinfo.iYear").get_asLong();
  details.m_strGenre=pDS->fv("movieinfo.strGenre").get_asString();
  details.m_strPictureURL=pDS->fv("movieinfo.strPictureURL").get_asString();
  details.m_strTitle=pDS->fv("movieinfo.strTitle").get_asString();
  details.m_strPath=pDS->fv("path.strPath").get_asString();
  details.m_strDVDLabel=pDS->fv("movie.cdlabel").get_asString();
  details.m_strIMDBNumber=pDS->fv("movieinfo.IMDBID").get_asString();

  long lMovieId=pDS->fv("movieinfo.idMovie").get_asLong();
  details.m_strSearchString.Format("%i", lMovieId);
	return details;
}