#include "../stdafx.h"
#include "FileCurl.h"
#include "../Util.h"
#include <sys/Stat.h>

#include "DllLibCurl.h"

using namespace XFILE;
using namespace XCURL;

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
  if(userp == NULL) return 0;
 
  CFileCurl *file = (CFileCurl *)userp;
  return file->WriteCallback(buffer, size, nitems);
}

extern "C" size_t header_callback(void *ptr, size_t size, size_t nmemb, void *stream)
{
  CFileCurl *file = (CFileCurl *)stream;
  return file->HeaderCallback(ptr, size, nmemb);
}

/* fix for silly behavior of realloc */
static inline void* realloc_simple(void *ptr, size_t size)
{
  void *ptr2 = realloc(ptr, size);
  if(ptr && !ptr2 && size > 0)
  {
    free(ptr);
    return NULL;
  }
  else
    return ptr2;
}


/* small dummy class to be able to get headers simply */
class CDummyHeaders : public IHttpHeaderCallback
{
public:
  CDummyHeaders(CHttpHeader *headers)
  {
    m_headers = headers;
  }

  virtual void ParseHeaderData(CStdString strData)
  {
    m_headers->Parse(strData);
  }
  CHttpHeader *m_headers;
};

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
  
  m_httpheader.Parse(strData);

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
    
    m_overflowBuffer = (char*)realloc_simple(m_overflowBuffer, amount + m_overflowSize);
    if(m_overflowBuffer == NULL)
    {
      CLog::Log(LOGDEBUG, __FUNCTION__" - Failed to grow overflow buffer");
      return 0;
    }
    memcpy(m_overflowBuffer + m_overflowSize, buffer, amount);
    m_overflowSize += amount;
  }
  return size * nitems;
}

CFileCurl::~CFileCurl()
{ 
  Close();

  /* release this session so it can be reused */
  if( m_easyHandle )
  {
    g_curlInterface.easy_release(m_easyHandle);
    m_easyHandle = NULL;
  }

  g_curlInterface.Unload();
}

#define BUFFER_SIZE 32768

CFileCurl::CFileCurl()
{
  g_curlInterface.Load(); // loads the curl dll and resolves exports etc.
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
  m_bufferSize = BUFFER_SIZE;
  m_threadid = 0;
}

//Has to be called before Open()
void CFileCurl::SetBufferSize(unsigned int size)
{
  m_bufferSize = size;
}

void CFileCurl::Close()
{
  CLog::Log(LOGDEBUG, "FileCurl::Close(%p) %s", this, m_url.c_str());
	m_filePos = 0;
	m_fileSize = 0;
  m_opened = false;

  if( m_easyHandle )
  {
    if( m_multiHandle )
    {
      /* make sure the easy handle is not in the multi handle anymore */
      g_curlInterface.multi_remove_handle(m_multiHandle, m_easyHandle);
      g_curlInterface.multi_cleanup(m_multiHandle);
      m_multiHandle = NULL;
    }

    /* set time out to 1 second to make sure we kill the stream quickly
    * on ftp transfers (filezilla doesn't like just a QUIT) */
    if (CUtil::IsFTP(m_url))
      g_curlInterface.easy_setopt(m_easyHandle, CURLOPT_TIMEOUT, 1);
  }

  m_url.Empty();
  
  /* cleanup */
  if( m_curlAliasList )
    g_curlInterface.slist_free_all(m_curlAliasList);
  if( m_curlHeaderList )
    g_curlInterface.slist_free_all(m_curlHeaderList);
  
  m_multiHandle = NULL;
  m_curlAliasList = NULL;
  m_curlHeaderList = NULL;
  
  m_buffer.Destroy();
  if (m_overflowBuffer)
    free(m_overflowBuffer);
  m_overflowBuffer = NULL;
  m_overflowSize = 0;
}

void CFileCurl::SetCommonOptions()
{
  g_curlInterface.easy_reset(m_easyHandle);
  
  g_curlInterface.easy_setopt(m_easyHandle, CURLOPT_DEBUGFUNCTION, debug_callback);

  if( g_advancedSettings.m_logLevel >= LOG_LEVEL_DEBUG )
    g_curlInterface.easy_setopt(m_easyHandle, CURLOPT_VERBOSE, TRUE);
  else
    g_curlInterface.easy_setopt(m_easyHandle, CURLOPT_VERBOSE, FALSE);
  
  g_curlInterface.easy_setopt(m_easyHandle, CURLOPT_WRITEDATA, this);
  g_curlInterface.easy_setopt(m_easyHandle, CURLOPT_WRITEFUNCTION, write_callback);
  
  // make sure headers are seperated from the data stream
  g_curlInterface.easy_setopt(m_easyHandle, CURLOPT_WRITEHEADER, this);
  g_curlInterface.easy_setopt(m_easyHandle, CURLOPT_HEADERFUNCTION, header_callback);
  g_curlInterface.easy_setopt(m_easyHandle, CURLOPT_HEADER, FALSE);
  
  g_curlInterface.easy_setopt(m_easyHandle, CURLOPT_FTP_USE_EPSV, 0); // turn off epsv
  
  // Allow us to follow two redirects
  g_curlInterface.easy_setopt(m_easyHandle, CURLOPT_FOLLOWLOCATION, TRUE);
  g_curlInterface.easy_setopt(m_easyHandle, CURLOPT_MAXREDIRS, 2);

  // When using multiple threads you should set the CURLOPT_NOSIGNAL option to
  // TRUE for all handles. Everything will work fine except that timeouts are not
  // honored during the DNS lookup - which you can work around by building libcurl
  // with c-ares support. c-ares is a library that provides asynchronous name
  // resolves. Unfortunately, c-ares does not yet support IPv6.
  g_curlInterface.easy_setopt(m_easyHandle, CURLOPT_NOSIGNAL, TRUE);

  // not interested in failed requests
  g_curlInterface.easy_setopt(m_easyHandle, CURLOPT_FAILONERROR, 1);

  // enable support for icecast / shoutcast streams
  m_curlAliasList = g_curlInterface.slist_append(m_curlAliasList, "ICY 200 OK"); 
  g_curlInterface.easy_setopt(m_easyHandle, CURLOPT_HTTP200ALIASES, m_curlAliasList); 

  // never verify peer, we don't have any certificates to do this
  g_curlInterface.easy_setopt(m_easyHandle, CURLOPT_SSL_VERIFYPEER, 0);

  // always allow gzip compression
  if( m_contentencoding.length() > 0 )
    g_curlInterface.easy_setopt(m_easyHandle, CURLOPT_ENCODING, m_contentencoding.c_str());
  
  if (m_userAgent.length() > 0)
    g_curlInterface.easy_setopt(m_easyHandle, CURLOPT_USERAGENT, m_userAgent.c_str());
  else /* set some default agent as shoutcast doesn't return proper stuff otherwise */
    g_curlInterface.easy_setopt(m_easyHandle, CURLOPT_USERAGENT, "XBMC/pre-2.1 (compatible; MSIE 6.0; Windows NT 5.1; WinampMPEG/5.09)");
  
  if (m_useOldHttpVersion)
    g_curlInterface.easy_setopt(m_easyHandle, CURLOPT_HTTP_VERSION, CURL_HTTP_VERSION_1_0);

  if (m_proxy.length() > 0)
    g_curlInterface.easy_setopt(m_easyHandle, CURLOPT_PROXY, m_proxy.c_str());

  if (m_customrequest.length() > 0)
    g_curlInterface.easy_setopt(m_easyHandle, CURLOPT_CUSTOMREQUEST, m_customrequest.c_str());
  
  SetRequestHeaders();
}

void CFileCurl::SetRequestHeaders()
{
  if(m_curlHeaderList) 
  {
    g_curlInterface.slist_free_all(m_curlHeaderList);
    m_curlHeaderList = NULL;
  }

  MAPHTTPHEADERS::iterator it;
  for(it = m_requestheaders.begin(); it != m_requestheaders.end(); it++)
  {
    CStdString buffer = it->first + ": " + it->second;
    m_curlHeaderList = g_curlInterface.slist_append(m_curlHeaderList, buffer.c_str()); 
  }

  // add user defined headers
  if (m_curlHeaderList && m_easyHandle)
    g_curlInterface.easy_setopt(m_easyHandle, CURLOPT_HTTPHEADER, m_curlHeaderList); 

}
bool CFileCurl::Open(const CURL& url, bool bBinary)
{
  return Open(url, bBinary , 10);
}
bool CFileCurl::Open(const CURL& url, bool bBinary, int iTimeOut)
{
  if (iTimeOut <=0)
    iTimeOut = 10; //default!
  m_buffer.Create(m_bufferSize * 3, m_bufferSize);  // 3 times our buffer size (2 in front, 1 behind)
  m_overflowBuffer = 0;
  m_overflowSize = 0;

  CURL url2(url);
  if( url2.GetProtocol().Equals("ftpx") )
    url2.SetProtocol("ftp");
  else if (url2.GetProtocol().Equals("shout") || url2.GetProtocol().Equals("daap") || url2.GetProtocol().Equals("upnp") || url2.GetProtocol().Equals("tuxbox"))
    url2.SetProtocol("http");    

  if( url2.GetProtocol().Equals("ftp") )
  {
    /* this is uggly, depending on from where   */
    /* we get the link it may or may not be     */
    /* url encoded. if handed from ftpdirectory */
    /* it won't be so let's handle that case    */
    
    CStdString partial, filename(url2.GetFileName());
    CStdStringArray array;

    /* our current client doesn't support utf8 */
    g_charsetConverter.utf8ToStringCharset(filename);

    /* TODO: create a tokenizer that doesn't skip empty's */
    CUtil::Tokenize(filename, array, "/");
    filename.Empty();
    for(CStdStringArray::iterator it = array.begin(); it != array.end(); it++)
    {
      if(it != array.begin())
        filename += "/";

      partial = *it;      
      CUtil::URLEncode(partial);      
      filename += partial;
    }

    /* make sure we keep slashes */    
    if(url2.GetFileName().Right(1) == "/")
      filename += "/";

    url2.SetFileName(filename);
  }

  url2.GetURL(m_url);

  CLog::Log(LOGDEBUG, "FileCurl::Open(%p) %s", this, m_url.c_str());  
  
  if( m_easyHandle == NULL )
    m_easyHandle = g_curlInterface.easy_aquire(url2.GetProtocol(), url2.GetHostName());


  if( m_contentencoding.length() == 0 && bBinary)
    m_contentencoding = "gzip";

  // setup common curl options
  SetCommonOptions();

  g_curlInterface.easy_setopt(m_easyHandle, CURLOPT_URL, m_url.c_str());
  if (!bBinary) g_curlInterface.easy_setopt(m_easyHandle, CURLOPT_TRANSFERTEXT, TRUE);  

  m_multiHandle = g_curlInterface.multi_init();

  m_threadid = GetCurrentThreadId();
  g_curlInterface.multi_add_handle(m_multiHandle, m_easyHandle);

  m_stillRunning = 1;
  m_opened = true;

  // read some data in to try and obtain the length
  // maybe there's a better way to get this info??
  if (!FillBuffer(m_bufferSize, iTimeOut))
  {
    CLog::Log(LOGERROR, "CFileCurl:Open, didn't get any data from stream.");    
    Close();
    return false;
  }
  
  long response;
  if (CURLE_OK == g_curlInterface.easy_getinfo(m_easyHandle, CURLINFO_RESPONSE_CODE, &response))
  {
    if( url2.GetProtocol().Equals("http") || url2.GetProtocol().Equals("https") )
    {
      if( response >= 400 )
      {
        CLog::Log(LOGERROR, "CFileCurl:Open, Error code from server %l", response);
        Close();
        return false;
      }
    }
  }

  double length;
  if (CURLE_OK == g_curlInterface.easy_getinfo(m_easyHandle, CURLINFO_CONTENT_LENGTH_DOWNLOAD, &length))
    m_fileSize = (__int64)length;

  /* workaround for shoutcast server wich doesn't set content type on standard mp3 */
  if( m_httpheader.GetContentType().IsEmpty() )
  {
    if( !m_httpheader.GetValue("icy-notice1").IsEmpty()
     || !m_httpheader.GetValue("icy-name").IsEmpty()
     || !m_httpheader.GetValue("icy-br").IsEmpty() )
     m_httpheader.Parse("Content-Type: audio/mpeg\r\n");
  }

  m_seekable = false;
  if(m_fileSize > 0)
  {
    if(url2.GetProtocol().Equals("http") || url2.GetProtocol().Equals("https"))
    {
      // only assume server is seekable if it provides Accept-Ranges
      if(!m_httpheader.GetValue("Accept-Ranges").IsEmpty())
        m_seekable = true;
    }
    else
      m_seekable = true;
  }

  return true;
}
bool CFileCurl::ReadString(char *szLine, int iLineLength)
{
  return ReadString(szLine, iLineLength, 10);
}
bool CFileCurl::ReadString(char *szLine, int iLineLength, int iTimeOut)
{
  if (iTimeOut <=0)
    iTimeOut = 10; //default!

  unsigned int want = (unsigned int)iLineLength;

  if(!FillBuffer(want,iTimeOut)) //GeminiServer
    return false;

  if (!m_stillRunning && !m_buffer.GetMaxReadSize() && m_filePos != m_fileSize)
  {
    // means we've finished our transfer
    return false;
  }

  // ensure only available data is considered 
  want = min(m_buffer.GetMaxReadSize(), want);

  char* pLine = szLine;
  do
  {
    if (!m_buffer.ReadBinary(pLine, 1))
    {
      break;
    }
    pLine++;
  } while (((pLine - 1)[0] != '\n') && ((unsigned int)(pLine - szLine) < want));
  pLine[0] = 0;
  m_filePos += (pLine - szLine);
  return (bool)((pLine - szLine) > 0);
}

bool CFileCurl::Exists(const CURL& url)
{
  return Stat(url, NULL) == 0;
}

__int64 CFileCurl::Seek(__int64 iFilePosition, int iWhence)
{
  /* if we don't have a filesize this is most likely unseekable */
  if( !m_seekable ) return -1;

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

    /* caller might have changed some headers (needed for daap)*/
    SetRequestHeaders();

    /* set offset */
    CURLcode ret = g_curlInterface.easy_setopt(m_easyHandle, CURLOPT_RESUME_FROM_LARGE, nextPos);
//    if (CURLE_OK == ret)
//      CLog::Log(LOGDEBUG, "FileCurl::Seek(%p) - resetting file fetch to %i (successful)", this, nextPos);
//    else
//      CLog::Log(LOGDEBUG, "FileCurl::Seek(%p) - resetting file fetch to %i (failed, code %i)", this, nextPos, ret);


    /* restart */
    g_curlInterface.multi_add_handle(m_multiHandle, m_easyHandle);

    /* reset stillrunning as we now are going to reget data */
    m_stillRunning = 1;

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
  // if file is already running, get infor from it
  if( m_opened )
  {
    CLog::Log(LOGWARNING, __FUNCTION__" - Stat called on open file");
    buffer->st_size = GetLength();
		buffer->st_mode = _S_IFREG;
    return 0;
  }
  
  CURL url2(url);
  if( url2.GetProtocol().Equals("ftpx") )
    url2.SetProtocol("ftp");
  else if (url2.GetProtocol().Equals("shout") || url2.GetProtocol().Equals("daap") || url2.GetProtocol().Equals("upnp"))
    url2.SetProtocol("http");
  
  /* ditch options as it's not supported on ftp */
  if( url2.GetProtocol().Equals("ftp") )
    url2.SetOptions("");

  url2.GetURL(m_url);

  if( m_easyHandle == NULL )
    m_easyHandle = g_curlInterface.easy_aquire(url2.GetProtocol(), url2.GetHostName());

  SetCommonOptions(); 
  g_curlInterface.easy_setopt(m_easyHandle, CURLOPT_TIMEOUT, 5);
  g_curlInterface.easy_setopt(m_easyHandle, CURLOPT_URL, m_url.c_str());
  g_curlInterface.easy_setopt(m_easyHandle, CURLOPT_NOBODY, 1);
  g_curlInterface.easy_setopt(m_easyHandle, CURLOPT_WRITEDATA, NULL); /* will cause write failure*/

  CURLcode result = g_curlInterface.easy_perform(m_easyHandle);

  
  if (result == CURLE_GOT_NOTHING)
  {
    /* some http servers and shoutcast servers don't give us any data on a head request */
    /* request normal and just fail out, it's their loss */
    /* somehow curl doesn't reset CURLOPT_NOBODY properly so reset everything */    
    SetCommonOptions(); 
    g_curlInterface.easy_setopt(m_easyHandle, CURLOPT_TIMEOUT, 5);
    g_curlInterface.easy_setopt(m_easyHandle, CURLOPT_URL, m_url.c_str());
    g_curlInterface.easy_setopt(m_easyHandle, CURLOPT_RANGE, "0-0");
    g_curlInterface.easy_setopt(m_easyHandle, CURLOPT_WRITEDATA, NULL); /* will cause write failure*/    
    result = g_curlInterface.easy_perform(m_easyHandle);
  }

  if(result == CURLE_HTTP_RANGE_ERROR )
  {
    /* crap can't use the range option, disable it and try again */
    g_curlInterface.easy_setopt(m_easyHandle, CURLOPT_RANGE, NULL);
    result = g_curlInterface.easy_perform(m_easyHandle);
  }

  if( result == CURLE_WRITE_ERROR || result == CURLE_OK )
  {

    /* workaround for shoutcast server wich doesn't set content type on standard mp3 */
    if( m_httpheader.GetContentType().IsEmpty() )
    {
      if( !m_httpheader.GetValue("icy-notice1").IsEmpty()
      || !m_httpheader.GetValue("icy-name").IsEmpty()
      || !m_httpheader.GetValue("icy-br").IsEmpty() )
      m_httpheader.Parse("Content-Type: audio/mpeg\r\n");
    }

    if( !buffer ) return 0;

    double length;
    char content[255];
    if (CURLE_OK == g_curlInterface.easy_getinfo(m_easyHandle, CURLINFO_CONTENT_LENGTH_DOWNLOAD, &length)
     && CURLE_OK == g_curlInterface.easy_getinfo(m_easyHandle, CURLINFO_CONTENT_TYPE, content))
    {
		  buffer->st_size = (__int64)length;
      if(strstr(content, "text/html")) //consider html files directories
        buffer->st_mode = _S_IFDIR;
      else
        buffer->st_mode = _S_IFREG;

      return 0;
    }    
  }

  errno = ENOENT;
	return -1;
}

unsigned int CFileCurl::Read(void *lpBuf, __int64 uiBufSize)
{
  return Read(lpBuf, uiBufSize, 10);
}
unsigned int CFileCurl::Read(void* lpBuf, __int64 uiBufSize, int iTimeOut)
{
  if (iTimeOut <=0)
    iTimeOut = 10; //default!

  /* only allow caller to read buffersize at a time */
  unsigned int want;
  if(uiBufSize > m_bufferSize)
    want = m_bufferSize;
  else
    want = (unsigned int)uiBufSize;

  if(!FillBuffer(want,iTimeOut)) //GeminiServer
    return -1;

  if (!m_stillRunning && !m_buffer.GetMaxReadSize() && m_filePos != m_fileSize)
  {
    // means we've finished our transfer
    return 0;
  }

  /* ensure only available data is considered */
  want = min(m_buffer.GetMaxReadSize(), want);

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
bool CFileCurl::FillBuffer(unsigned int want, int waittime)
{  
  int maxfd;  
  fd_set fdread;
  fd_set fdwrite;
  fd_set fdexcep;

  FD_ZERO(&fdread);
  FD_ZERO(&fdwrite);
  FD_ZERO(&fdexcep);

  // waittime timeout
  struct timeval timeout;
  timeout.tv_sec = 0;
  timeout.tv_usec = 20000; //timeout between calls to perform, doesn't timeout FillBuffer
  
  // fill timeout
  DWORD timestamp = GetTickCount() + waittime * 1000;

  // only attempt to fill buffer if transactions still running and buffer
  // doesnt exceed required size already
  while (m_buffer.GetMaxReadSize() < want && m_buffer.GetMaxWriteSize() > 0 )
  {
    /* if there is data in overflow buffer, try to use that first */
    if(m_overflowSize)
    {
      unsigned amount = min(m_buffer.GetMaxWriteSize(), m_overflowSize);
      m_buffer.WriteBinary(m_overflowBuffer, amount);

      if(amount < m_overflowSize)
        memcpy(m_overflowBuffer, m_overflowBuffer+amount,m_overflowSize-amount);

      m_overflowSize -= amount;
      m_overflowBuffer = (char*)realloc_simple(m_overflowBuffer, m_overflowSize);
      continue;
    }

    if( GetTickCount() > timestamp )
    {
      CLog::Log(LOGWARNING, __FUNCTION__" - Timeout waiting for data");
      return false;
    }

    CURLMcode result = g_curlInterface.multi_perform(m_multiHandle, &m_stillRunning);
    if( !m_stillRunning ) 
      return (CURLM_OK == result);

    switch(result)
    {
      case CURLM_OK:
      {
        // hack for broken curl, that thinks there is data all the time
        // happens especially on ftp during initial connection
        SwitchToThread();

        // get file descriptors from the transfers
        if( CURLM_OK != g_curlInterface.multi_fdset(m_multiHandle, &fdread, &fdwrite, &fdexcep, &maxfd) )
          return false;
        
        if( maxfd < 0 ) // hack for broken curl
          maxfd = fdread.fd_count + fdwrite.fd_count + fdexcep.fd_count - 1;

        if( maxfd >= 0  )
        {
          // wait until data is avialable or a timeout occours
          if( SOCKET_ERROR == dllselect(maxfd + 1, &fdread, &fdwrite, &fdexcep, &timeout) )
            return false;
        }
        else
          Sleep(timeout.tv_sec * 1000 + timeout.tv_usec / 1000);

      }
      break;
      case CURLM_CALL_MULTI_PERFORM:
      {
        // we don't keep calling here as that can easily overwrite our buffer wich we want to avoid
        // docs says we should call it soon after, but aslong as we are reading data somewhere
        // this aught to be soon enough. should stay in socket otherwise
        continue;
      }
      break;
      default:
      {
        CLog::Log(LOGERROR, __FUNCTION__" - curl multi perform failed with code %d, aborting", result);
        return false;
      }
      break;
    }
  }
  return true;
}

void CFileCurl::ClearRequestHeaders()
{
  m_requestheaders.clear();
}

void CFileCurl::SetRequestHeader(CStdString header, CStdString value)
{
  m_requestheaders[header] = value;
}

void CFileCurl::SetRequestHeader(CStdString header, long value)
{
  CStdString buffer;
  buffer.Format("%ld", value);
  m_requestheaders[header] = buffer;
}

/* STATIC FUNCTIONS */
bool CFileCurl::GetHttpHeader(const CURL &url, CHttpHeader &headers)
{
  try
  {
    CFileCurl file;
    CDummyHeaders callback(&headers);

    file.SetHttpHeaderCallback(&callback);
    
    /* calling stat should give us all needed data */
    return file.Stat(url, NULL) == 0;
  }
  catch(...)
  {
    CStdString path;
    url.GetURL(path);
    CLog::Log(LOGERROR, __FUNCTION__" - Exception thrown while trying to retrieve header url: %s", path.c_str());
    return false;
  }
}

bool CFileCurl::GetContent(const CURL &url, CStdString &content)
{
   CFileCurl file;
   if( file.Stat(url, NULL) == 0 )
   {
     content = file.GetContent();
     return true;
   }
   
   content = "";
   return false;
}