#include "../stdafx.h"
#include "FileCurl.h"
#include "CurlInterface.h"
#include "../Util.h"
#include <sys/Stat.h>

extern "C" int __stdcall dllselect(int ntfs, fd_set *readfds, fd_set *writefds, fd_set *errorfds, const timeval *timeout);

// curl calls this routine to debug
extern "C" int debug_callback(CURL_HANDLE *handle, curl_infotype info, char *output, size_t size, void *data)
{
  if (info == CURLINFO_DATA_IN || info == CURLINFO_DATA_OUT)
    return 0;
  char *pOut = new char[size + 1];
  strncpy(pOut, output, size);
  pOut[size] = '\0';
  CStdString strOut = pOut;
  delete[] pOut;
  strOut.TrimRight("\r\n");
  CLog::Log(LOGDEBUG, "Curl:: Debug %s", strOut.c_str());
  return 0;
}

/* curl calls this routine to get more data */
extern "C" size_t write_callback(char *buffer,
               size_t size,
               size_t nitems,
               void *userp)
{
  CFileCurl *file = (CFileCurl *)userp;
  return file->WriteCallback(buffer, size, nitems);
}

extern "C" size_t header_callback(void *ptr, size_t size, size_t nmemb, void *stream)
{
  CFileCurl *file = (CFileCurl *)stream;
  return file->HeaderCallback(ptr, size, nmemb);
}

size_t CFileCurl::HeaderCallback(void *ptr, size_t size, size_t nmemb)
{
  // libcurl doc says that this info is not always \0 terminated
  char* strData = (char*)ptr;
  int iSize = size * nmemb;
  
  if (strData[iSize] != 0)
  {
    strData = (char*)malloc(iSize + 1);
    strncpy(strData, (char*)ptr, iSize);
    strData[iSize] = 0;
  }
  else strData = strdup((char*)ptr);
  
  if (m_pHeaderCallback) m_pHeaderCallback->ParseHeaderData(strData);
  
  free(strData);
  
  return iSize;
}

size_t CFileCurl::WriteCallback(char *buffer, size_t size, size_t nitems)
{
  unsigned int amount = size * nitems;

//  CLog::Log(LOGDEBUG, "CFileCurl::WriteCallback (%p) with %i bytes, readsize = %i, writesize = %i", this, amount, m_buffer.GetMaxReadSize(), m_buffer.GetMaxWriteSize() - m_overflowSize);
  if (m_overflowSize)
  {
    // we have our overflow buffer - first get rid of as much as we can
    unsigned int maxWriteable = min(m_buffer.GetMaxWriteSize(), m_overflowSize);
    if (maxWriteable)
    {
      if (!m_buffer.WriteBinary(m_overflowBuffer, maxWriteable))
        CLog::Log(LOGERROR, "Unable to write to buffer - what's up?");
      if (m_overflowSize > maxWriteable)
      { // still have some more - copy it down
        memmove(m_overflowBuffer, m_overflowBuffer + maxWriteable, m_overflowSize - maxWriteable);
      }
      m_overflowSize -= maxWriteable;
    }
  }
  // ok, now copy the data into our ring buffer
  unsigned int maxWriteable = min(m_buffer.GetMaxWriteSize(), amount);
  if (maxWriteable)
  {
    if (!m_buffer.WriteBinary(buffer, maxWriteable))
      CLog::Log(LOGERROR, "Unable to write to buffer - what's up?");
    amount -= maxWriteable;
    buffer += maxWriteable;
  }
  if (amount)
  {
    CLog::Log(LOGDEBUG, "CFileCurl::WriteCallback(%p) not enough free space for %i bytes", this,  amount);
    // don't have enough room in our buffer - need to copy into our temp buffer
    char *newbuffer = (char *)malloc(amount + m_overflowSize);
    if (m_overflowBuffer)
    {
      memcpy(newbuffer, m_overflowBuffer, m_overflowSize);
      free(m_overflowBuffer);
    }
    memcpy(newbuffer + m_overflowSize, buffer, amount);
    m_overflowSize += amount;
    m_overflowBuffer = newbuffer;
  }
  return size * nitems;
}

CFileCurl::~CFileCurl()
{ 
  Close(); 
}

CFileCurl::CFileCurl()
{
  g_curlInterface.Create(); // loads the curl dll and resolves exports etc.
	m_filePos = 0;
	m_fileSize = 0;
  m_easyHandle = NULL;
  m_multiHandle = NULL;
  m_curlAliasList = NULL;
  m_curlHeaderList = NULL;
  m_opened = false;
  m_useOldHttpVersion = false;
  m_overflowBuffer = NULL;
  m_overflowSize = 0;
  m_pHeaderCallback = NULL;
}

void CFileCurl::Close()
{
  CLog::Log(LOGDEBUG, "FileCurl::Close(%p) %s", this, m_url.c_str());
	m_filePos = 0;
	m_fileSize = 0;
  m_opened = false;

  /* make sure the easy handle is not in the multi handle anymore */
  g_curlInterface.multi_remove_handle(m_multiHandle, m_easyHandle);

  /* set time out to 1 second to make sure we kill the stream quickly
   * on ftp transfers (filezilla doesn't like just a QUIT) */
  if (CUtil::IsFTP(m_url))
    g_curlInterface.easy_setopt(m_easyHandle, CURLOPT_TIMEOUT, 1);

  m_url.Empty();
  
  /* cleanup */
  g_curlInterface.easy_cleanup(m_easyHandle);
  g_curlInterface.slist_free_all(m_curlAliasList);
  g_curlInterface.slist_free_all(m_curlHeaderList);
  
  m_multiHandle = NULL;
  m_easyHandle = NULL;
  m_curlAliasList = NULL;
  m_curlHeaderList = NULL;
  
  m_buffer.Destroy();
  if (m_overflowBuffer)
    free(m_overflowBuffer);
  m_overflowBuffer = NULL;
  m_overflowSize = 0;
}

#define BUFFER_SIZE 32768

bool CFileCurl::Open(const CURL& url, bool bBinary)
{
  m_easyHandle = g_curlInterface.easy_init();

  m_buffer.Create(BUFFER_SIZE * 3, BUFFER_SIZE);  // 3 times our buffer size (2 in front, 1 behind)
  m_overflowBuffer = 0;
  m_overflowSize = 0;

  url.GetURL(m_url);
  CLog::Log(LOGDEBUG, "FileCurl::Open(%p) %s", this, m_url.c_str());
  g_curlInterface.easy_setopt(m_easyHandle, CURLOPT_URL, m_url.c_str());
  
  g_curlInterface.easy_setopt(m_easyHandle, CURLOPT_DEBUGFUNCTION, debug_callback);
  g_curlInterface.easy_setopt(m_easyHandle, CURLOPT_VERBOSE, TRUE);
  
  g_curlInterface.easy_setopt(m_easyHandle, CURLOPT_WRITEDATA, this);
  g_curlInterface.easy_setopt(m_easyHandle, CURLOPT_WRITEFUNCTION, write_callback);
  
  // make sure headers are seperated from the data stream
  g_curlInterface.easy_setopt(m_easyHandle, CURLOPT_WRITEHEADER, this);
  g_curlInterface.easy_setopt(m_easyHandle, CURLOPT_HEADERFUNCTION, header_callback);
  g_curlInterface.easy_setopt(m_easyHandle, CURLOPT_HEADER, FALSE);
  
  g_curlInterface.easy_setopt(m_easyHandle, CURLOPT_FTP_USE_EPSV, 0); // turn off epsv
  g_curlInterface.easy_setopt(m_easyHandle, CURLOPT_FOLLOWLOCATION, TRUE);
  if (!bBinary) g_curlInterface.easy_setopt(m_easyHandle, CURLOPT_TRANSFERTEXT, TRUE);

  // When using multiple threads you should set the CURLOPT_NOSIGNAL option to
  // TRUE for all handles. Everything will work fine except that timeouts are not
  // honored during the DNS lookup - which you can work around by building libcurl
  // with c-ares support. c-ares is a library that provides asynchronous name
  // resolves. Unfortunately, c-ares does not yet support IPv6.
  g_curlInterface.easy_setopt(m_easyHandle, CURLOPT_NOSIGNAL, TRUE);

  // enable support for icecast / shoutcast streams
  m_curlAliasList = g_curlInterface.slist_append(m_curlAliasList, "ICY 200 OK"); 
  g_curlInterface.easy_setopt(m_easyHandle, CURLOPT_HTTP200ALIASES, m_curlAliasList); 

  // add user defined headers
  if (m_curlHeaderList)
    g_curlInterface.easy_setopt(m_easyHandle, CURLOPT_HTTPHEADER, m_curlHeaderList); 
  
  if (m_userAgent.length() > 0)
    g_curlInterface.easy_setopt(m_easyHandle, CURLOPT_USERAGENT, m_userAgent.c_str());
  
  if (m_useOldHttpVersion)
    g_curlInterface.easy_setopt(m_easyHandle, CURLOPT_HTTP_VERSION, CURL_HTTP_VERSION_1_0);

  
  m_multiHandle = g_curlInterface.multi_init();

  g_curlInterface.multi_add_handle(m_multiHandle, m_easyHandle);

  // start the transfer
  while (g_curlInterface.multi_perform(m_multiHandle, &m_stillRunning) == CURLM_CALL_MULTI_PERFORM )
        ;

  // read some data in to try and obtain the length
  // maybe there's a better way to get this info??
  FillBuffer(BUFFER_SIZE, 10);           // completely fill our buffer

  if (m_buffer.GetMaxReadSize() == 0 && !m_stillRunning)
  {
    // if still_running is 0 now, we should return NULL
    Close();
    return false;
  }
  
  double length;
  if (CURLE_OK == g_curlInterface.easy_getinfo(m_easyHandle, CURLINFO_CONTENT_LENGTH_DOWNLOAD, &length))
    m_fileSize = (__int64)length;
  
  m_opened = true;
  return true;
}

bool CFileCurl::ReadString(char *szLine, int iLineLength)
{
  // unimplemented
  return false;
}

bool CFileCurl::Exists(const CURL& url)
{
	return Open(url);
}

__int64 CFileCurl::Seek(__int64 iFilePosition, int iWhence)
{
  __int64 nextPos = m_filePos;
	switch(iWhence) 
	{
		case SEEK_SET:
			nextPos = iFilePosition;
			break;
		case SEEK_CUR:
			nextPos += iFilePosition;
			break;
		case SEEK_END:
			if (m_fileSize)
        nextPos = m_fileSize + iFilePosition;
      else
        nextPos = 0;
			break;
	}
//  CLog::Log(LOGDEBUG, "FileCurl::Seek(%p) - current pos %i, new pos %i", this, (unsigned int)m_filePos, (unsigned int)nextPos);
  // see if nextPos is within our buffer
  if (!m_buffer.SkipBytes((int)(nextPos - m_filePos)))
  {
    // bummer - seeking outside our buffers

    /* halt transaction */
    g_curlInterface.multi_remove_handle(m_multiHandle, m_easyHandle);

    /* set offset */
    CURLcode ret = g_curlInterface.easy_setopt(m_easyHandle, CURLOPT_RESUME_FROM_LARGE, nextPos);
//    if (CURLE_OK == ret)
//      CLog::Log(LOGDEBUG, "FileCurl::Seek(%p) - resetting file fetch to %i (successful)", this, nextPos);
//    else
//      CLog::Log(LOGDEBUG, "FileCurl::Seek(%p) - resetting file fetch to %i (failed, code %i)", this, nextPos, ret);

    /* restart */
    g_curlInterface.multi_add_handle(m_multiHandle, m_easyHandle);

    /* ditch buffer - write will recreate - resets stream pos*/
    m_buffer.Clear();
    if (m_overflowBuffer)
      free(m_overflowBuffer);
    m_overflowBuffer=NULL;
    m_overflowSize = 0;
  }
  m_filePos = nextPos;
  return m_filePos;
}

__int64 CFileCurl::GetLength()
{
	if (!m_opened) return 0;
	return m_fileSize;
}

__int64 CFileCurl::GetPosition()
{
	if (!m_opened) return 0;
	return m_filePos;
}

int CFileCurl::Stat(const CURL& url, struct __stat64* buffer)
{
	if (Open(url, true))
	{
		buffer->st_size = GetLength();
		buffer->st_mode = _S_IFREG;
		Close();
		return 0;
	}
	errno = ENOENT;
	return -1;
}

unsigned int CFileCurl::Read(void *lpBuf, __int64 uiBufSize)
{
//  CLog::Log(LOGDEBUG, "FileCurl::Read(%p) %i bytes", this, uiBufSize);
  unsigned int want = (unsigned int)uiBufSize;

  FillBuffer(want,1);

  if (!m_stillRunning && !m_buffer.GetMaxReadSize() && m_filePos != m_fileSize)
  {
    // means we've finished our transfer
    return 0;
  }

  /* ensure only available data is considered */
  if(m_buffer.GetMaxReadSize() < want)
    want = m_buffer.GetMaxReadSize();

  /* xfer data to caller */
  if (m_buffer.ReadBinary((char *)lpBuf, want))
  {
    m_filePos += want;
//    CLog::Log(LOGDEBUG, "FileCurl::Read(%p) return %d bytes %d left", this, want,m_buffer.GetMaxReadSize());
    return want;
  }
  CLog::Log(LOGDEBUG, "FileCurl::Read(%p) failed %d bytes %d left", this, want,m_buffer.GetMaxReadSize());
  return 0;
}

/* use to attempt to fill the read buffer up to requested number of bytes */
int CFileCurl::FillBuffer(unsigned int want, int waittime)
{
  int maxfd;
  fd_set fdread;
  fd_set fdwrite;
  fd_set fdexcep;

  FD_ZERO(&fdread);
  FD_ZERO(&fdwrite);
  FD_ZERO(&fdexcep);

  // 500 msec timeout
  struct timeval timeout;
  timeout.tv_sec = 0;
  timeout.tv_usec = 500 * 1000;
    
  // only attempt to fill buffer if transactions still running and buffer
  // doesnt exceed required size already
  while (m_stillRunning && m_buffer.GetMaxReadSize() < want + BUFFER_SIZE && m_buffer.GetMaxWriteSize() > 0)
  {
    // get file descriptors from the transfers
    g_curlInterface.multi_fdset(m_multiHandle, &fdread, &fdwrite, &fdexcep, &maxfd);

    // wait until data is avialable or a timeout occours
    int rc = dllselect(maxfd + 1, &fdread, &fdwrite, &fdexcep, &timeout);
    
    // fill buffer
    while(g_curlInterface.multi_perform(m_multiHandle, &m_stillRunning) == CURLM_CALL_MULTI_PERFORM)
    {
      int test = 1;
    }
  }
  return 1;
}

void CFileCurl::AddHeaderParam(const char* sParam)
{
  // libcurl is already initialized in the constructor
  m_curlHeaderList = g_curlInterface.slist_append(m_curlHeaderList, sParam); 
}