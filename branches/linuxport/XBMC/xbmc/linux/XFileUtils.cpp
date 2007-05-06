
#include "XFileUtils.h"
#include "XTimeUtils.h"

#ifdef _LINUX
#include <sys/types.h>
#include <sys/stat.h>
#include <regex.h>
#include <dirent.h>

HANDLE FindFirstFile(LPCSTR szPath,LPWIN32_FIND_DATA lpFindData) {
	if (lpFindData == NULL || szPath == NULL)
		return NULL;

	CStdString strPath(szPath);

	if (strPath.empty())
		return INVALID_HANDLE_VALUE;

	// if the file name is a directory then we add a * to look for all files in this directory
	DIR *testDir = opendir(szPath);
	if (testDir) {
		strPath += "/*";
		closedir(testDir);
	}

	int nFilePos = strPath.ReverseFind(XBMC_FILE_SEP);
	
	CStdString strDir = ".";
	CStdString strFiles = strPath;

	if (nFilePos > 0) {
		strDir = strPath.substr(0,nFilePos);
		strFiles = strPath.substr(nFilePos + 1);
	}

	strFiles = CStdString("^") + strFiles + "$";
	strFiles.Replace(".","\\.");
	strFiles.Replace("*",".*");
	strFiles.Replace("?",".");

	int status;
	regex_t re;
	if (regcomp(&re, strFiles, REG_EXTENDED|REG_NOSUB) != 0) {
		return(INVALID_HANDLE_VALUE);      
	}

	CXHandle *pHandle = new CXHandle(CXHandle::HND_FIND_FILE);

	struct dirent **namelist = NULL;
	int n = scandir(strDir, &namelist, 0, alphasort);
	while (n-- > 0) {
          	status = regexec(&re, namelist[n]->d_name, (size_t) 0, NULL, 0);
		if (status == 0) {
			pHandle->m_FindFileResults.push_back(strDir + CStdString("/") + namelist[n]->d_name);
		}
		free(namelist[n]);
	}
	
	if (namelist)
		free(namelist);

	regfree(&re);

	FindNextFile(pHandle, lpFindData);

	return pHandle;
}

BOOL   FindNextFile(HANDLE hHandle, LPWIN32_FIND_DATA lpFindData) {
	if (lpFindData == NULL || hHandle == NULL || hHandle->m_type != CXHandle::HND_FIND_FILE)
		return FALSE;

	if (hHandle->m_nFindFileIterator >= hHandle->m_FindFileResults.size())
		return FALSE;

	CStdString strFileName = hHandle->m_FindFileResults[hHandle->m_nFindFileIterator++];
	
	struct stat fileStat;
	if (stat(strFileName, &fileStat) != 0)
		return FALSE;

	bool bIsDir = false;
	DIR *testDir = opendir(strFileName);
	if (testDir) {
		bIsDir = true;
		closedir(testDir);
	}

	memset(lpFindData,0,sizeof(WIN32_FIND_DATA));
	
	lpFindData->dwFileAttributes = FILE_ATTRIBUTE_NORMAL;
	strcpy(lpFindData->cFileName, strFileName.c_str());

	if (bIsDir)
		lpFindData->dwFileAttributes |= FILE_ATTRIBUTE_DIRECTORY;

	if (strFileName[0] == '.')
		lpFindData->dwFileAttributes |= FILE_ATTRIBUTE_HIDDEN;

	TimeTToFileTime(&fileStat.st_ctime, &lpFindData->ftCreationTime);
	TimeTToFileTime(&fileStat.st_atime, &lpFindData->ftLastAccessTime);
	TimeTToFileTime(&fileStat.st_mtime, &lpFindData->ftLastWriteTime);
	
	lpFindData->nFileSizeHigh = 0;
	lpFindData->nFileSizeLow =  fileStat.st_size;

	return TRUE;
}

BOOL   FindClose(HANDLE hFindFile) {
	return CloseHandle(hFindFile);
}

HANDLE CreateFile(LPCTSTR lpFileName, DWORD dwDesiredAccess,
  DWORD dwShareMode, LPSECURITY_ATTRIBUTES lpSecurityAttributes, DWORD dwCreationDisposition,
  DWORD dwFlagsAndAttributes, HANDLE hTemplateFile)
{
  // Fail on unsupported items
  if (lpSecurityAttributes != NULL )
  {
    XXLog(ERROR, "CreateFile does not support security attributes");
    return 0;
  } 
  
  if (hTemplateFile != (HANDLE) 0)
  {
    XXLog(ERROR, "CreateFile does not support template file");
    return 0;
  }
  
  int flags;
  if (dwDesiredAccess & (FILE_READ_DATA | FILE_WRITE_DATA) == (FILE_READ_DATA | FILE_WRITE_DATA)) 
    flags = O_RDWR;
  else if (dwDesiredAccess & FILE_READ_DATA == FILE_READ_DATA) 
    flags = O_RDONLY;
  else if (dwDesiredAccess & FILE_WRITE_DATA == FILE_WRITE_DATA) 
    flags = O_WRONLY;
  else
  {
    XXLog(ERROR, "CreateFile does not desired access other than read and/or write");
    return 0;
  }

  switch (dwCreationDisposition)
  {
    case OPEN_ALWAYS:
      flags |= O_CREAT;
      break;
    case TRUNCATE_EXISTING:
      flags |= O_TRUNC;
      break;
    case CREATE_ALWAYS:
      flags |= O_CREAT|O_TRUNC;
      break;
    case CREATE_NEW:
      flags |= O_CREAT|O_TRUNC|O_EXCL;
      break;
    case OPEN_EXISTING:
      break;
  }
  
  int fd = open(lpFileName, flags);
  if (fd == -1)
    return 0;
    
  HANDLE result = new CXHandle(CXHandle::HND_FILE);
  result->fd = fd;
  
  return result;
}

BOOL DeleteFile(LPCTSTR lpFileName)
{
  if (unlink(lpFileName) == 0)
    return 1;
  else 
    return 0;
}

BOOL MoveFile(LPCTSTR lpExistingFileName, LPCTSTR lpNewFileName) 
{
  if (rename(lpExistingFileName, lpNewFileName) == 0)
    return 1;
  else
    return 0;
}

BOOL CopyFile(LPCTSTR lpExistingFileName, LPCTSTR lpNewFileName, BOOL bFailIfExists)
{
  // If the destination file exists and we should fail...guess what? we fail!
  struct stat destStat;
  bool isDestExists = (stat(lpNewFileName, &destStat) == 0);
  if (isDestExists && bFailIfExists)
    return 0;
  
  // Open the files
  int sf = open(lpExistingFileName, O_RDONLY);
  if (sf == -1)
    return 0;
    
  int df = open(lpNewFileName, O_CREAT|O_WRONLY|O_TRUNC);
  if (df == -1)
  {
    close(sf);
    return 0;
  }
  
  // Read and write chunks of 16K
  char buf[16384];
  size_t bytesRead = 1;
  size_t bytesWritten = 1;
  
  while (bytesRead > 0 && bytesWritten > 0)
  {
    bytesRead = read(sf, buf, sizeof(buf));
    if (bytesRead > 0)
      bytesWritten = write(df, buf, bytesRead);       
  }
  
  // Done
  close(sf);
  close(df);
  
  if (bytesRead == -1 || bytesWritten == -1)
    return 0;
    
  return 1;    
}
		
BOOL ReadFile(HANDLE hFile, LPVOID lpBuffer, DWORD nNumberOfBytesToRead,
  LPDWORD lpNumberOfBytesRead, LPVOID lpOverlapped)
{
  if (lpOverlapped)
  {
    XXLog(ERROR, "ReadFile does not support overlapped I/O");
    return 0;
 }
  
  size_t bytesRead = read(hFile->fd, lpBuffer, nNumberOfBytesToRead);
  if (bytesRead == -1)
    return 0;
    
  *lpNumberOfBytesRead = bytesRead;
}

BOOL WriteFile(HANDLE hFile, LPVOID lpBuffer, DWORD nNumberOfBytesToWrite,
  LPDWORD lpNumberOfBytesWritten, LPVOID lpOverlapped)
{
  if (lpOverlapped)
  {
    XXLog(ERROR, "ReadFile does not support overlapped I/O");
    return 0;
 }
  
  size_t bytesWritten = write(hFile->fd, lpBuffer, nNumberOfBytesToWrite);
  if (bytesWritten == -1)
    return 0;
    
  *lpNumberOfBytesWritten = bytesWritten;
}

BOOL   CreateDirectory(LPCTSTR lpPathName, LPSECURITY_ATTRIBUTES lpSecurityAttributes) 
{
  if (mkdir(lpPathName, 0755) == 0)
    return 1;
  else
    return 0;
}

BOOL   RemoveDirectory(LPCTSTR lpPathName) 
{
  if (rmdir(lpPathName) == 0)
    return 1;
  else
    return 0;
}

DWORD  SetFilePointer(HANDLE hFile, LONG lDistanceToMove, PLONG lpDistanceToMoveHigh, DWORD dwMoveMethod) {
#warning need to complete function SetFilePointer
	return true;
}

// uses statfs
BOOL GetDiskFreeSpaceEx(
  LPCTSTR lpDirectoryName,
  PULARGE_INTEGER lpFreeBytesAvailable,
  PULARGE_INTEGER lpTotalNumberOfBytes,
  PULARGE_INTEGER lpTotalNumberOfFreeBytes
  ) 

{
#warning need to complete function GetDiskFreeSpaceEx
	return true;
}

DWORD GetTimeZoneInformation( LPTIME_ZONE_INFORMATION lpTimeZoneInformation ) {
#warning need to complete function GetTimeZoneInformation
	return 0;
}

BOOL SetEndOfFile(HANDLE hFile) {
#warning need to complete function SetEndOfFile
	return 0;
}

DWORD SleepEx( DWORD dwMilliseconds,  BOOL bAlertable) {
#warning need to complete function SleepEx
	SDL_Delay(dwMilliseconds);
	return 0;
}

BOOL SetFilePointerEx(  HANDLE hFile,
						LARGE_INTEGER liDistanceToMove,
						PLARGE_INTEGER lpNewFilePointer,
						DWORD dwMoveMethod ) {
#warning need to complete function SetFilePointerEx
	return true;
}

BOOL GetFileSizeEx( HANDLE hFile, PLARGE_INTEGER lpFileSize) {
#warning need to complete function GetFileSizeEx
	return true;
}

BOOL FlushFileBuffers( HANDLE hFile ) {
#warning need to complete function FlushFileBuffers
	return true;
}

int _stat64(   const char *path,   struct __stat64 *buffer ) {
#warning need to complete function _stat64
	struct stat buf;
	return stat(path, &buf);
}

DWORD  GetFileSize(HANDLE hFile, LPDWORD lpFileSizeHigh)
{
#warning need to complete function GetFileSize
  return 0;
}

DWORD  GetFileAttributes(LPCTSTR lpFileName)
{
#warning need to complete function GetFileAttributes
  return 0;
}

DWORD  GetCurrentDirectory(DWORD nBufferLength, LPSTR lpBuffer)
{
#warning need to complete function GetCurrentDirectory
  return 0;
}
#endif
