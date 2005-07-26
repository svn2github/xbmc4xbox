#include "stdafx.h"
#include ".\programdatabase.h"
#include "utils/fstrcmp.h"
#include "util.h"

#define PROGRAM_DATABASE_VERSION 0.5f

//********************************************************************************************************************************
CProgramDatabase::CProgramDatabase(void)
{
  m_fVersion=PROGRAM_DATABASE_VERSION;
  m_strDatabaseFile=PROGRAM_DATABASE_NAME;
}

//********************************************************************************************************************************
CProgramDatabase::~CProgramDatabase(void)
{

}

//********************************************************************************************************************************
bool CProgramDatabase::CreateTables()
{

  try
  {
    CDatabase::CreateTables();

    CLog::Log(LOGINFO, "create program table");
    m_pDS->exec("CREATE TABLE program ( idProgram integer primary key, idPath integer, idBookmark integer)\n");
    CLog::Log(LOGINFO, "create bookmark table");
    m_pDS->exec("CREATE TABLE bookmark (idBookmark integer primary key, bookmarkName text)\n");
    CLog::Log(LOGINFO, "create path table");
    m_pDS->exec("CREATE TABLE path ( idPath integer primary key, strPath text)\n");
    CLog::Log(LOGINFO, "create files table");
    m_pDS->exec("CREATE TABLE files ( idFile integer primary key, idPath integer, strFilename text, titleId text, xbedescription text, iTimesPlayed integer, lastAccessed integer)\n");
    CLog::Log(LOGINFO, "create bookmark index");
    m_pDS->exec("CREATE INDEX idxBookMark ON bookmark(bookmarkName)");
    CLog::Log(LOGINFO, "create path index");
    m_pDS->exec("CREATE INDEX idxPath ON path(strPath)");
    CLog::Log(LOGINFO, "create files index");
    m_pDS->exec("CREATE INDEX idxFiles ON files(strFilename)");
    CLog::Log(LOGINFO, "create files - titleid index");
    m_pDS->exec("CREATE INDEX idxTitleIdFiles ON files(titleId)");
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "programdatabase::unable to create tables:%i", GetLastError());
    return false;
  }

  return true;
}

bool CProgramDatabase::UpdateOldVersion(float fVersion)
{
  if (NULL == m_pDB.get()) return false;
  if (NULL == m_pDS.get()) return false;
  if (NULL == m_pDS2.get()) return false;

  try
  {
    if (fVersion < 0.5f)
    { // Version 0 to 0.5 upgrade - we need to add the version table
      CLog::Log(LOGINFO, "creating versions table");
      m_pDS->exec("CREATE TABLE version (idVersion float)\n");
    }
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "Error attempting to update the database version!");
    return false;
  }
  return true;
}

//********************************************************************************************************************************
long CProgramDatabase::GetFile(const CStdString& strFilenameAndPath, CFileItemList& programs)
{
  try
  {
    FILETIME localTime;
    if (NULL == m_pDB.get()) return -1;
    if (NULL == m_pDS.get()) return -1;
    CStdString strPath, strFileName;
    Split(strFilenameAndPath, strPath, strFileName);
    strPath.Replace("\\", "/");
    strFileName.Replace("\\", "/");
    CStdString strSQL=FormatSQL("select * from path,files where path.idPath=files.idPath and path.strPath like '%s' and files.strFileName like '%s'", strPath.c_str(), strFileName.c_str());
    m_pDS->query(strSQL.c_str());
    if (m_pDS->num_rows() > 0)
    {
      WIN32_FILE_ATTRIBUTE_DATA FileAttributeData;
      CStdString strPath, strFile, strPathandFile;
      strPath = m_pDS->fv("path.strPath").get_asString();
      strFile = m_pDS->fv("files.strFilename").get_asString();
      long lFileId = m_pDS->fv("files.idFile").get_asLong();
      strPathandFile = strPath + strFile;
      strPathandFile.Replace("/", "\\");
      if (CFile::Exists(strPathandFile))
      {
        CFileItem *pItem = new CFileItem(m_pDS->fv("files.xbedescription").get_asString());
        pItem->m_iprogramCount = m_pDS->fv("files.iTimesPlayed").get_asLong();
        pItem->m_strPath = strPathandFile;
        pItem->m_bIsFolder = false;
        GetFileAttributesEx(pItem->m_strPath, GetFileExInfoStandard, &FileAttributeData);
        FileTimeToLocalFileTime(&FileAttributeData.ftLastWriteTime, &localTime);
        FileTimeToSystemTime(&localTime, &pItem->m_stTime);
        pItem->m_dwSize = FileAttributeData.nFileSizeLow;
        programs.Add(pItem);
        m_pDS->close();
        return lFileId;
      }
      else
      {
        m_pDS->close();
        DeleteFile(lFileId);
        return -1;
      }
      m_pDS->close();
    }
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "CProgramDatabase::GetFile(%s)", strFilenameAndPath.c_str());
  }
  return -1;
}

bool CProgramDatabase::GetXBEPathByTitleId(const DWORD titleId, CStdString& strPathAndFilename)
{
  try
  {
    if (NULL == m_pDB.get()) return false;
    if (NULL == m_pDS.get()) return false;

    CStdString strSQL=FormatSQL("select path.strPath, files.strFilename from files join path on files.idPath=path.idPath where files.titleId='%x'", titleId);
    m_pDS->query(strSQL.c_str());
    if (m_pDS->num_rows() > 0)
    {
      strPathAndFilename = m_pDS->fv("path.strPath").get_asString();
      strPathAndFilename += m_pDS->fv("files.strFilename").get_asString();
      strPathAndFilename.Replace('/', '\\');
      m_pDS->close();
      return true;
    }
    m_pDS->close();
    return false;
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "CProgramDatabase::GetXBEPathByTitleId(%i) failed", titleId);
  }
  return false;
}

long CProgramDatabase::AddFile(long lPathId, const CStdString& strFileName , DWORD titleId, const CStdString& strDescription)
{
  CStdString strSQL = "";
  try
  {
    long lFileId;
    if (NULL == m_pDB.get()) return -1;
    if (NULL == m_pDS.get()) return -1;
    strSQL=FormatSQL("select * from files where idPath=%i and strFileName like '%s'", lPathId, strFileName.c_str());
    m_pDS->query(strSQL.c_str());
    if (m_pDS->num_rows() > 0)
    {
      lFileId = m_pDS->fv("idFile").get_asLong() ;
      m_pDS->close();
      return lFileId;
    }
    m_pDS->close();
    SYSTEMTIME localTime;
    GetLocalTime(&localTime);
    unsigned __int64 lastAccessed = LocalTimeToTimeStamp(localTime);
    strSQL=FormatSQL("insert into files (idFile, idPath, strFileName, titleId, xbedescription, iTimesPlayed, lastAccessed) values(NULL, %i, '%s', '%x','%s',%i,%I64u)", lPathId, strFileName.c_str(), titleId, strDescription.c_str(), 0, lastAccessed);
    m_pDS->exec(strSQL.c_str());
    lFileId = (long)sqlite3_last_insert_rowid( m_pDB->getHandle() );
    return lFileId;
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "programdatabase:unable to addfile (%s)", strSQL.c_str());
  }
  return -1;
}

//********************************************************************************************************************************
long CProgramDatabase::AddBookMark(const CStdString& strBookmark)
{
  try
  {
    if (NULL == m_pDB.get()) return -1;
    if (NULL == m_pDS.get()) return -1;

    CStdString strSQL=FormatSQL("select * from bookmark where bookmarkName='%s'", strBookmark.c_str());
    m_pDS->query(strSQL.c_str());
    if (m_pDS->num_rows() == 0)
    {
      m_pDS->close();
      strSQL=FormatSQL("insert into bookmark (idBookmark, bookMarkname) values (NULL, '%s')", strBookmark.c_str());
      m_pDS->exec(strSQL.c_str());
      long lBookmarkId = (long)sqlite3_last_insert_rowid(m_pDB->getHandle());
      return lBookmarkId;
    }
    else
    {
      const field_value value = m_pDS->fv("idBookmark");
      long lBookmarkId = value.get_asLong();
      m_pDS->close();
      return lBookmarkId;
    }
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "CProgramDatabase::AddBookMark(%s) failed", strBookmark.c_str());
  }
  return -1;
}

//********************************************************************************************************************************
long CProgramDatabase::AddPath(const CStdString& strPath)
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
    CLog::Log(LOGERROR, "CProgramDatabase::AddPath(%s) failed", strPath.c_str());
  }
  return -1;
}

//********************************************************************************************************************************
bool CProgramDatabase::EntryExists(const CStdString& strPath1, const CStdString& strBookmark)
{
  try
  {
    CStdString strPath = strPath1;
    strPath.Replace("\\", "/");
    if (NULL == m_pDB.get()) return false;
    if (NULL == m_pDS.get()) return false;
    long lPathId = GetPath(strPath);
    long lBookmarkId = AddBookMark(strBookmark);
    CStdString strSQL=FormatSQL("select * from program where idPath=%i and idBookmark=%i", lPathId, lBookmarkId);
    m_pDS->query(strSQL.c_str());
    if (m_pDS->num_rows() > 0)
    {
      m_pDS->close();
      return true;
    }
    m_pDS->close();
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "CProgramDatabase::EntryExists(%s,%s) failed", strPath1.c_str(), strBookmark.c_str());
  }
  return false;
}

void CProgramDatabase::GetPathsByBookmark(const CStdString& strBookmark, vector <CStdString>& vecPaths)
{
  try
  {
    if (NULL == m_pDB.get()) return;
    if (NULL == m_pDS.get()) return;
    long lBookMarkId = AddBookMark(strBookmark);
    CStdString strSQL=FormatSQL("select strPath from path,program where program.idBookmark=%i and program.idPath=path.idPath", lBookMarkId);
    m_pDS->query(strSQL.c_str());
    while (!m_pDS->eof())
    {
      CStdString strPath = m_pDS->fv("strPath").get_asString();
      strPath.Replace("/", "\\");
      vecPaths.push_back(strPath);
      m_pDS->next();
    }
    m_pDS->close();
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "CProgramDatabase::GetPathsByBookmark(%s) failed", strBookmark.c_str());
  }
}

long CProgramDatabase::GetPath(const CStdString& strPath)
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
    CLog::Log(LOGERROR, "CProgramDatabase::GetPath(%s) failed", strPath.c_str());
  }
  return -1;
}

//********************************************************************************************************************************
long CProgramDatabase::GetProgram(long lPathId)
{
  try
  {
    if (NULL == m_pDB.get()) return -1;
    if (NULL == m_pDS.get()) return -1;

    CStdString strSQL=FormatSQL("select idProgram from program where idPath=%i", lPathId);
    m_pDS->query(strSQL.c_str());
    if (m_pDS->num_rows() > 0)
    {
      long lProgramId = m_pDS->fv("idProgram").get_asLong();
      m_pDS->close();
      return lProgramId;
    }
    m_pDS->close();
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "CProgramDatabase::GetProgram(%i) failed", lPathId);
  }
  return -1;
}

//********************************************************************************************************************************
long CProgramDatabase::AddProgram(const CStdString& strFilenameAndPath, DWORD titleId, const CStdString& strDescription, const CStdString& strBookmark)
{
  try
  {
    if (NULL == m_pDB.get()) return -1;
    if (NULL == m_pDS.get()) return -1;
    CStdString strPath, strFileName;
    Split(strFilenameAndPath, strPath, strFileName);
    strPath.Replace("\\", "/");
    strFileName.Replace("\\", "/");

    long lPathId = GetPath(strPath);

    if (!EntryExists(strPath, strBookmark))
    {
      lPathId = AddPath(strPath);
      if (lPathId < 0) return -1;
      long lBookMarkId = AddBookMark(strBookmark);
      if (lBookMarkId < 0) return -1;
      CStdString strSQL=FormatSQL("insert into program (idProgram, idPath, idBookmark) values( NULL, %i, %i)",
                    lPathId, lBookMarkId);
      m_pDS->exec(strSQL.c_str());
      long lProgramId = (long)sqlite3_last_insert_rowid(m_pDB->getHandle());
      AddFile(lPathId, strFileName, titleId, strDescription);
    }
    else
    {
      long lProgramId = GetProgram(lPathId);
      AddFile(lPathId, strFileName, titleId, strDescription);
      return lProgramId;
    }

  }
  catch (...)
  {
    CLog::Log(LOGERROR, "CProgramDatabase::AddProgram(%s,%s) failed", strFilenameAndPath.c_str() , strDescription.c_str() );
  }
  return -1;
}

unsigned __int64 CProgramDatabase::LocalTimeToTimeStamp( const SYSTEMTIME & localTime )
{

  unsigned __int64 localFileTime;
  unsigned __int64 systemFileTime;

  ::SystemTimeToFileTime( &localTime, (FILETIME*)&localFileTime );
  ::LocalFileTimeToFileTime( (const FILETIME*)&localFileTime,(FILETIME*)&systemFileTime );

  systemFileTime += Date_1601;
  return systemFileTime;
}

SYSTEMTIME CProgramDatabase::TimeStampToLocalTime( unsigned __int64 timeStamp )
{
  SYSTEMTIME localTime;
  unsigned __int64 fileTime;

  timeStamp -= Date_1601;
  ::FileTimeToLocalFileTime( (const FILETIME *)&timeStamp, (FILETIME*)&fileTime);
  ::FileTimeToSystemTime( (const FILETIME *)&fileTime, &localTime );
  return localTime;
} 

//********************************************************************************************************************************
void CProgramDatabase::GetProgramsByBookmark(CStdString& strBookmark, CFileItemList& programs, int iDepth, bool bOnlyDefaultXBE)
{
  try
  {
    VECPROGRAMPATHS todelete;
    if (NULL == m_pDB.get()) return ;
    if (NULL == m_pDS.get()) return ;
    long lBookmarkId = AddBookMark(strBookmark);
    CStdString strSQL;
    if (bOnlyDefaultXBE)
      strSQL=FormatSQL("select strPath,strFilename,xbedescription,iTimesPlayed,lastAccessed from files,path,program where program.idBookmark=%i and program.idPath=path.idPath and files.idPath=program.idPath and files.strFilename like '/default.xbe'", lBookmarkId);
    else
      strSQL=FormatSQL("select strPath,strFilename,xbedescription,iTimesPlayed,lastAccessed from files,path,program where program.idBookmark=%i and program.idPath=path.idPath and files.idPath=program.idPath", lBookmarkId);
    m_pDS->query(strSQL.c_str());
    while (!m_pDS->eof())
    {
      CStdString strPath, strFile, strPathandFile;
      strPath = m_pDS->fv("strPath").get_asString();
      strFile = m_pDS->fv("strFilename").get_asString();
      strPathandFile = strPath + strFile;
      strPathandFile.Replace("/", "\\");
      CFileItem *pItem = new CFileItem(m_pDS->fv("xbedescription").get_asString());
      pItem->m_iprogramCount = m_pDS->fv("iTimesPlayed").get_asLong();
      pItem->m_strPath = strPathandFile;
      pItem->m_bIsFolder = false;
      CStdString timestampstr = m_pDS->fv("lastAccessed").get_asString();
      unsigned __int64 timestamp = _atoi64(timestampstr.c_str());
      pItem->m_stTime=TimeStampToLocalTime(timestamp);
      programs.Add(pItem);
      m_pDS->next();
    }
    m_pDS->close();
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "CProgramDatabase::GetProgamsByBookmark() failed");
  }
}

void CProgramDatabase::GetProgramsByPath(const CStdString& strPath1, CFileItemList& programs, int iDepth, bool bOnlyDefaultXBE)
{
  try
  {
    VECPROGRAMPATHS todelete;
    if (NULL == m_pDB.get()) return ;
    if (NULL == m_pDS.get()) return ;
    CStdString strSQL;
    CStdString strPath = strPath1;
    strPath.Replace("\\", "/");
    CStdString strShortCutsDir = g_stSettings.m_szShortcutDirectory;
    strShortCutsDir.Replace("\\", "/");
    if (bOnlyDefaultXBE)
    {
      strSQL=FormatSQL("select strPath,strFilename,xbedescription,iTimesPlayed,lastAccessed from files,path where files.idPath=path.idPath and path.strPath like '%s/%%' and files.strFilename like '/default.xbe'", strPath.c_str());
    }
    else
    {
      if (strPath.c_str() == strShortCutsDir)
        strSQL=FormatSQL("select strPath,strFilename,xbedescription,iTimesPlayed,lastAccessed from files,path where files.idPath=path.idPath and path.strPath like '%s'", strPath.c_str());
      else
        strSQL=FormatSQL("select strPath,strFilename,xbedescription,iTimesPlayed,lastAccessed from files,path where files.idPath=path.idPath and path.strPath like '%s/%%'", strPath.c_str());
    }
    m_pDS->query(strSQL.c_str());
    while (!m_pDS->eof())
    {
      CStdString strPath, strFile, strPathandFile;
      strPath = m_pDS->fv("strPath").get_asString();
      strFile = m_pDS->fv("strFilename").get_asString();
      int depth = StringUtils::FindNumber(strPath, "/") - StringUtils::FindNumber(strPath1, "/");
      strPathandFile = strPath + strFile;
      strPathandFile.Replace("/", "\\");
      if (depth <= iDepth)
      {
        CFileItem *pItem = new CFileItem(m_pDS->fv("xbedescription").get_asString());
        pItem->m_iprogramCount = m_pDS->fv("iTimesPlayed").get_asLong();
        pItem->m_strPath = strPathandFile;
        pItem->m_bIsFolder = false;
        CStdString timestampstr = m_pDS->fv("lastAccessed").get_asString();
        unsigned __int64 timestamp = _atoi64(timestampstr.c_str());
        pItem->m_stTime=TimeStampToLocalTime(timestamp);
        programs.Add(pItem);
      }
      m_pDS->next();
    }
    m_pDS->close();
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "CProgramDatabase::GetProgramsByPath() failed");
  }
}

//********************************************************************************************************************************
void CProgramDatabase::DeleteFile(long lFileId)
{
  try
  {
    if (NULL == m_pDB.get()) return ;
    if (NULL == m_pDS.get()) return ;

    CStdString strSQL=FormatSQL("delete from files where idFile=%i", lFileId);
    m_pDS->exec(strSQL.c_str());
  }

  catch (...)
  {
    CLog::Log(LOGERROR, "CProgramDatabase::DeleteFile() failed");
  }
}

void CProgramDatabase::DeleteProgram(const CStdString& strPath)
{
  try
  {
    if (NULL == m_pDB.get()) return ;
    if (NULL == m_pDS.get()) return ;

    long lPathId = GetPath(strPath);
    if (lPathId < 0)
    {
      return ;
    }


    CStdString strSQL=FormatSQL("delete from files where idpath=%i", lPathId);
    m_pDS->exec(strSQL.c_str());

    strSQL=FormatSQL("delete from program where idpath=%i", lPathId);
    m_pDS->exec(strSQL.c_str());

    strSQL=FormatSQL("delete from path where idpath=%i", lPathId);
    m_pDS->exec(strSQL.c_str());
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "CProgramDatabase::DeleteProgram() failed");
  }
}

bool CProgramDatabase::IncTimesPlayed(const CStdString& strFileName)
{
  try
  {
    CStdString strPath;
    CUtil::GetDirectory(strFileName, strPath);
    strPath.Replace("\\", "/");

    if (NULL == m_pDB.get()) return false;
    if (NULL == m_pDS.get()) return false;

    CStdString strSQL=FormatSQL("select * from files,path where files.idPath=path.idPath and path.strPath='%s'", strPath.c_str());
    if (!m_pDS->query(strSQL.c_str())) return false;
    int iRowsFound = m_pDS->num_rows();
    if (iRowsFound == 0)
    {
      m_pDS->close();
      return false;
    }
    int idFile = m_pDS->fv("files.idFile").get_asLong();
    int iTimesPlayed = m_pDS->fv("files.iTimesPlayed").get_asLong();
    m_pDS->close();

    CLog::Log(LOGDEBUG, "CProgramDatabase::IncTimesPlayed(%s), idFile=%i, iTimesPlayed=%i",
              strFileName.c_str(), idFile, iTimesPlayed);

    strSQL=FormatSQL("update files set iTimesPlayed=%i where idFile=%i",
                  ++iTimesPlayed, idFile);
    m_pDS->exec(strSQL.c_str());
    return true;
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "CProgramDatabase:IncTimesPlayed(%s) failed", strFileName.c_str());
  }

  return false;
}
