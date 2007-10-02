//
// C++ Implementation: CacheStrategy
//
// Description: 
//
//
// Author: Team XBMC <>, (C) 2007
//
// Copyright: See COPYING file that comes with this distribution
//
//
#include "stdafx.h"
#include "CacheStrategy.h"
#ifdef _LINUX
#include "PlatformInclude.h"
#endif
#include "Util.h"
#include "../utils/log.h"
#include "../utils/SingleLock.h"

namespace XFILE {

CCacheStrategy::CCacheStrategy() : m_bEndOfInput(false)
{
}


CCacheStrategy::~CCacheStrategy()
{
}

void CCacheStrategy::EndOfInput() {
	m_bEndOfInput = true;
}

bool CCacheStrategy::IsEndOfInput()
{
  return m_bEndOfInput;
}

void CCacheStrategy::ClearEndOfInput()
{
  m_bEndOfInput = false;
}

CSimpleFileCache::CSimpleFileCache() : m_hCacheFileRead(NULL), m_hCacheFileWrite(NULL), m_hDataAvailEvent(NULL), m_nStartPosition(0) {
}

CSimpleFileCache::~CSimpleFileCache() {
	Close();
}

int CSimpleFileCache::Open() {
	Close();

	m_hDataAvailEvent = CreateEvent(NULL, TRUE, FALSE, NULL);

	CStdString fileName = CUtil::GetNextFilename(_P("Z:\\filecache%03d.cache"), 999);
	if(fileName.empty())
	{
		CLog::Log(LOGERROR, CStdString(__FUNCTION__) + " - Unable to generate a new filename");
		Close();
		return CACHE_RC_ERROR;
	}
	
	m_hCacheFileWrite = CreateFile(fileName.c_str()
						, GENERIC_WRITE, FILE_SHARE_READ
						, NULL
						, CREATE_ALWAYS
						, FILE_ATTRIBUTE_NORMAL | FILE_FLAG_NO_BUFFERING
						,NULL);

	if(m_hCacheFileWrite == INVALID_HANDLE_VALUE)
	{
		CLog::Log(LOGERROR, CStdString(__FUNCTION__)+" - failed to create file %s with error code %d", fileName.c_str(), GetLastError());
		Close();
		return CACHE_RC_ERROR;
	}

	m_hCacheFileRead = CreateFile(fileName.c_str()
						, GENERIC_READ, 0
						, NULL
						, OPEN_EXISTING
						, FILE_ATTRIBUTE_NORMAL | FILE_FLAG_DELETE_ON_CLOSE 
						, NULL);

	if(m_hCacheFileRead == INVALID_HANDLE_VALUE)
	{
		CLog::Log(LOGERROR, CStdString(__FUNCTION__)+" - failed to open file %s with error code %d", fileName.c_str(), GetLastError());
		Close();
		return CACHE_RC_ERROR;
	}

	return CACHE_RC_OK;
}

int CSimpleFileCache::Close() 
{
	CSingleLock lock(m_sync);

	if (m_hDataAvailEvent)
		CloseHandle(m_hDataAvailEvent);

	m_hDataAvailEvent = NULL;

	if (m_hCacheFileWrite)
		CloseHandle(m_hCacheFileWrite);

	m_hCacheFileWrite = NULL;

	if (m_hCacheFileRead)
		CloseHandle(m_hCacheFileRead);

	m_hCacheFileRead = NULL;

	return CACHE_RC_OK;
}

int CSimpleFileCache::WriteToCache(const char *pBuffer, size_t iSize) {
	DWORD iWritten=0;
	if (!WriteFile(m_hCacheFileWrite, pBuffer, iSize, &iWritten, NULL)) {
		CLog::Log(LOGERROR, "%s - failed to write to file. err: %d", __FUNCTION__, GetLastError());
		return CACHE_RC_ERROR;
	}
	
	//CLog::Log(LOGDEBUG,"CSimpleFileCache::WriteToCache. wrote %d bytes", iWritten);

	// when reader waits for data it will wait on the event.
	// it will reset it prior to the wait.
	// there is a possible race condition here - but the reader will worse case wait 1 second, 
	// so rather not lock (performance)
	SetEvent(m_hDataAvailEvent);

	return iWritten;
}

__int64 CSimpleFileCache::GetAvailableRead() {
	LARGE_INTEGER posRead = {}, posWrite = {};
	
	if(!SetFilePointerEx(m_hCacheFileRead, posRead, &posRead, FILE_CURRENT)) {
		return CACHE_RC_ERROR;
	}
	
	if(!SetFilePointerEx(m_hCacheFileWrite, posWrite, &posWrite, FILE_CURRENT)) {
		return CACHE_RC_ERROR;
	}
	
	return posWrite.QuadPart - posRead.QuadPart;
}

int CSimpleFileCache::ReadFromCache(char *pBuffer, size_t iMaxSize) {
	__int64 iAvailable = GetAvailableRead(); 
	if ( iAvailable <= 0 ) {
		return m_bEndOfInput?CACHE_RC_EOF : CACHE_RC_WOULD_BLOCK;
	}

	if (iMaxSize > iAvailable)
		iMaxSize = iAvailable;

	DWORD iRead = 0;
	if (!ReadFile(m_hCacheFileRead, pBuffer, iMaxSize, &iRead, NULL)) {
		CLog::Log(LOGERROR,"CSimpleFileCache::ReadFromCache - failed to read %d bytes.", iMaxSize);
		return CACHE_RC_ERROR;
	}

	//CLog::Log(LOGDEBUG,"CSimpleFileCache::ReadFromCache. read %d bytes", iRead);
	return iRead;
}

__int64 CSimpleFileCache::WaitForData(unsigned int iMinAvail, unsigned int iMillis) {
	
	DWORD tmStart = timeGetTime();
	__int64 iAvail = 0;

	while ( iAvail < iMinAvail && timeGetTime() - tmStart < iMillis) {

		// possible race condition here:
		// if writer will set the event between the availability check and reset - 
		// we will hang. but only for 1 second, so better not take the performance hit
		if ( (iAvail = GetAvailableRead()) < iMinAvail )
			ResetEvent(m_hDataAvailEvent);
		else {
			return iAvail;
		}

		// busy look (sleep max 1 sec each round)
		DWORD dwRc = WaitForSingleObject(m_hDataAvailEvent, 1000);
		
		if (iAvail >= iMinAvail)
			return iAvail;

		if (dwRc == WAIT_FAILED)
			return CACHE_RC_ERROR;
	}

	return CACHE_RC_TIMEOUT;
}

__int64 CSimpleFileCache::Seek(__int64 iFilePosition, int iWhence) {

	CLog::Log(LOGDEBUG,"CSimpleFileCache::Seek, seeking to %lld", iFilePosition);

	if (iFilePosition < m_nStartPosition)
	{
		CLog::Log(LOGDEBUG,"CSimpleFileCache::Seek, request seek before start of cache.");
		return CACHE_RC_ERROR;
	}

	// we cant seek to a location not read yet
	// in this case we will return error and reset the read pointer
	LARGE_INTEGER posWrite = {}, posRead = {};	
	if(!SetFilePointerEx(m_hCacheFileWrite, posWrite, &posWrite, FILE_CURRENT))
		return CACHE_RC_ERROR;
	if(!SetFilePointerEx(m_hCacheFileRead, posRead, &posRead, FILE_CURRENT))
		return CACHE_RC_ERROR;

	__int64 iTarget = iFilePosition - m_nStartPosition;
	if (SEEK_END == iWhence) 
	{
		CLog::Log(LOGERROR,"%s, cant seek relative to end", __FUNCTION__);
		return CACHE_RC_ERROR;
	}
	else if (SEEK_CUR == iWhence)
		iTarget = iFilePosition + posRead.QuadPart;

	__int64 nDiff = iTarget - posWrite.QuadPart;
	if ( nDiff > 500000 || (nDiff > 0 && WaitForData(nDiff, 5000) == CACHE_RC_TIMEOUT)  ) {		
		CLog::Log(LOGWARNING,"CSimpleFileCache::Seek - attempt to seek pass read data (seek to %lld. max: %lld. reset read pointer. (%lld:%d)", iTarget, posWrite.QuadPart, iFilePosition, iWhence);

		// roll back file pointer
		SetFilePointerEx(m_hCacheFileRead, posRead, NULL, FILE_BEGIN);
		return  CACHE_RC_ERROR;
	}

	LARGE_INTEGER pos;
	pos.QuadPart = iTarget;
	 
	if(!SetFilePointerEx(m_hCacheFileRead, pos, &pos, FILE_BEGIN))
		return CACHE_RC_ERROR;
	
	return pos.QuadPart;
}

void CSimpleFileCache::Reset(__int64 iSourcePosition)
{
	CSingleLock lock(m_sync);

	LARGE_INTEGER pos;
	pos.QuadPart = 0;

	SetFilePointerEx(m_hCacheFileWrite, pos, NULL, FILE_BEGIN);
	SetFilePointerEx(m_hCacheFileRead, pos, NULL, FILE_BEGIN);
	m_nStartPosition = iSourcePosition;
}

}
