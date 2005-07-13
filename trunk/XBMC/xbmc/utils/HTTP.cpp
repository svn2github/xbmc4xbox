// HTTP.cpp: implementation of the CHTTP class.
//
//////////////////////////////////////////////////////////////////////


#include "../stdafx.h"
#include "HTTP.h"
#ifdef _XBOX
#include "../dnsnamecache.h"
#endif

#include "../util.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

/* 
 * HTTP Helper
 * by Bertrand Baudet <bertrand_baudet@yahoo.com>
 * (C) 2001, MPlayer team.
 *
 * Only base64_encode copied
 */

int base64_encode(const void *enc, int encLen, char *out, int outMax) {
	static const char	b64[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789-_";

	unsigned char		*encBuf;
	int			outLen;
	unsigned int		bits;
	unsigned int		shift;

	encBuf = (unsigned char*)enc;
	outLen = 0;
	bits = 0;
	shift = 0;

	while( outLen<outMax ) {
		if( encLen>0 ) {
			// Shift in byte
			bits <<= 8;
			bits |= *encBuf;
			shift += 8;
			// Next byte
			encBuf++;
			encLen--;
		} else if( shift>0 ) {
			// Pad last bits to 6 bits - will end next loop
			bits <<= 6 - shift;
			shift = 6;
		} else {
			// Terminate with Mime style '='
			*out = '=';
			outLen++;

			return outLen;
		}

		// Encode 6 bit segments
		while( shift>=6 ) {
			shift -= 6;
			*out = b64[ (bits >> shift) & 0x3F ];
			out++;
			outLen++;
		}
	}

	// Output overflow
	return -1;
}

CHTTP::CHTTP(const string& strProxyServer, int iProxyPort)
    : m_strProxyServer(strProxyServer)
    , m_iProxyPort(iProxyPort)
    , m_socket(INVALID_SOCKET)
{
  m_strCookie = "";
  hEvent = WSA_INVALID_EVENT;
  m_RecvBytes = 0;
  m_RecvBuffer = 0;
  m_redirectedURL = "";
}


CHTTP::CHTTP()
    : m_socket(INVALID_SOCKET)
{
  m_strProxyServer = "";
  m_bProxyEnabled = g_guiSettings.GetBool("Network.UseHTTPProxy");
  if (m_bProxyEnabled)
  {
    m_strProxyServer = g_guiSettings.GetString("Network.HTTPProxyServer").c_str();
    m_iProxyPort = atoi(g_guiSettings.GetString("Network.HTTPProxyPort").c_str());
  }
  m_strCookie = "";
  hEvent = WSA_INVALID_EVENT;
  m_RecvBytes = 0;
  m_RecvBuffer = 0;
  m_redirectedURL = "";
}


CHTTP::~CHTTP()
{
  Close();
}

//------------------------------------------------------------------------------------------------------------------

bool CHTTP::ReadData(string& strData)
{
  strData.clear();
  string::size_type n = m_strHeaders.find("Transfer-Encoding: chunked");
  if (n != string::npos)
  {
    char* p = m_RecvBuffer;
    for (;;)
    {
      // chunked transfer
      char* num = p;
      for (;;)
      {
        while (*p && *p != '\n')
          ++p;
        if (*p == '\n')
          break;

        memmove(m_RecvBuffer, num, m_RecvBytes = (m_RecvBuffer + m_RecvBytes) - num);
        if (!Recv( -1))
        {
          CLog::Log(LOGERROR, "Recv failed: %d", WSAGetLastError());
          Close();
          return false;
        }
        num = p = m_RecvBuffer;
        if (!*p)
        {
          CLog::Log(LOGERROR, "Invalid reply from server");
          Close();
          return false;
        }
      }
      ++p;

      int len = strtol(num, NULL, 16);
      if (!len)
        break; // end of data

      while (len > 0)
      {
        int size = m_RecvBytes - (p - m_RecvBuffer);
        strData.append(p, p + (len > size ? size : len));
        p += (len > size ? size : len);
        len -= size;
        if (len > 0)
        {
          m_RecvBytes = 0;
          if (!Recv( -1))
          {
            strData.clear();
            CLog::Log(LOGERROR, "Recv failed: %d", WSAGetLastError());
            Close();
            return false;
          }
          p = m_RecvBuffer;
        }
      }
      if (p + 2 > m_RecvBuffer + m_RecvBytes)
      {
        if (p != m_RecvBuffer + m_RecvBytes)
        {
          m_RecvBuffer[0] = *p;
          m_RecvBytes = 1;
        }
        else
          m_RecvBytes = 0;
        if (!Recv( -1))
        {
          strData.clear();
          CLog::Log(LOGERROR, "Recv failed: %d", WSAGetLastError());
          Close();
          return false;
        }
        p = m_RecvBuffer;
      }
      p += 2; // strip footer
    }
  }
  else
  {
    // normal transfer
    n = m_strHeaders.find("Content-Length:");
    if (n == string::npos)  // no chunked transfer or content-length.  We should read until recvbytes is 0
    {
      do
      {
        if (m_RecvBytes > 0)
        {
          strData.append(m_RecvBuffer, m_RecvBuffer + m_RecvBytes);
          m_RecvBytes = 0;
        }

        if (!Recv( -1))
        {
          strData.clear();
          CLog::Log(LOGERROR, "Invalid reply from server");
          Close();
          return false;
        }
      }
      while (m_RecvBytes > 0);
    }

    else
    {
      int len = atoi(m_strHeaders.c_str() + n + 16);
      while (len > 0)
      {
        strData.append(m_RecvBuffer, m_RecvBuffer + (len > m_RecvBytes ? m_RecvBytes : len));
        len -= m_RecvBytes;
        if (len > 0)
        {
          m_RecvBytes = 0;
          if (!Recv(len))
          {
            strData.clear();
            CLog::Log(LOGERROR, "Recv failed: %d", WSAGetLastError());
            Close();
            return false;
          }
        }
      }
    }
  }
  return true;
}

bool CHTTP::GetHeader(CStdString strName, CStdString& strValue) const
{
  map<CStdString, CStdString>::const_iterator iter = m_mapHeaders.find(strName.ToUpper());
  if (iter == m_mapHeaders.end())
  {
    return false;
  }
  strValue = iter->second;
  return true;
}

//------------------------------------------------------------------------------------------------------------------

bool CHTTP::Get(string& strURL, string& strHTML)
{
  CLog::Log(LOGINFO, "Get URL: %s", strURL.c_str());

  int status = Open(strURL, "GET", NULL);

  if (status != 200)
  {
    Close();
    return false;
  }

  if (!ReadData(strHTML))
    return false;

  Close();
  return true;
}

//GeminiServer Detect if we online or not! Very Simple and Dirty!
bool CHTTP::IsInternet()
  {
	CStdString strURL="http://www.google.com";
	CStdString strURL2="http://www.yahoo.com";

	CLog::Log(LOGDEBUG, "Connecting to the Internet:!");
		CLog::Log(LOGDEBUG, "	- Sending Ping to: http://www.google.com");
	int status = Open(strURL, "HEAD", NULL);
		CLog::Log(LOGDEBUG, "	- Sending Ping to: http://www.yahoo.com");
	int status2 = Open(strURL2, "HEAD", NULL);
	Close();
	//if ( (status != 302) || (status2 != 200) )	//Status=302 Status2=200
	if (status != 302 )
	{
		CLog::Log(LOGDEBUG, "ERROR Connecting to the Internet Failed!");
		return false;
	}
	else
	{
		CLog::Log(LOGDEBUG, "Connecting to the Internet Succesfull!");
		return true; 
	}
	return false;
}

bool CHTTP::Head(string& strURL)
{
  CLog::Log(LOGINFO, "Get Header URL: %s", strURL.c_str());

  int status = Open(strURL, "HEAD", NULL);
  Close();

  if (status != 200)
  {
    return false;
  }

  return true;
}

//------------------------------------------------------------------------------------------------------------------

bool CHTTP::Post(const string &strURL, const string &strPostData, string &strHTML)
{
  CLog::Log(LOGINFO, "Post URL:%s", strURL.c_str());

  int status = Open(strURL, "POST", strPostData.c_str());

  if (status != 200)
  {
    Close();
    return false;
  }

  if (!ReadData(strHTML))
    return false;

  Close();
  return true;
}

//------------------------------------------------------------------------------------------------------------------
bool CHTTP::Download(const string &strURL, const string &strFileName, LPDWORD pdwSize)
{
  CLog::Log(LOGINFO, "Download: %s->%s", strURL.c_str(), strFileName.c_str());

  int status = Open(strURL, "GET", NULL);

  if (status != 200)
  {
    Close();
    return false;
  }

  string strData;
  if (!ReadData(strData))
    return false;

  Close();

  HANDLE hFile = CreateFile(strFileName.c_str(), GENERIC_WRITE, FILE_SHARE_READ, 0, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, 0);
  if (hFile == INVALID_HANDLE_VALUE)
  {
    CLog::Log(LOGERROR, "Unable to open file %s: %d", strFileName.c_str(), GetLastError());
    return false;
  }
  if (strData.size())
  {
    SetFilePointer(hFile, strData.size(), 0, FILE_BEGIN);
    SetEndOfFile(hFile);
    SetFilePointer(hFile, 0, 0, FILE_BEGIN);
    DWORD n;
    WriteFile(hFile, strData.data(), strData.size(), &n, 0);
  }
  CloseHandle(hFile);

  if (pdwSize != NULL)
  {
    *pdwSize = strData.size();
  }

  return true;
}

//------------------------------------------------------------------------------------------------------------------
bool CHTTP::Connect()
{
  if (!CUtil::IsNetworkUp())
    return false;

  sockaddr_in service;
  service.sin_family = AF_INET;
  memset(service.sin_zero, 0, sizeof(service.sin_zero));

  if (m_bProxyEnabled)
  {
    // connect to proxy server
    service.sin_addr.s_addr = inet_addr(m_strProxyServer.c_str());
    service.sin_port = htons(m_iProxyPort);
  }
  else
  {
    // connect to site directly
    service.sin_addr.s_addr = inet_addr(m_strHostName.c_str());
    if (service.sin_addr.s_addr == INADDR_NONE)
    {
      CStdString strIpAddress = "";
      CDNSNameCache::Lookup(m_strHostName, strIpAddress);
      service.sin_addr.s_addr = inet_addr(strIpAddress.c_str());
      if (service.sin_addr.s_addr == INADDR_NONE || strIpAddress == "")
      {
        CLog::Log(LOGWARNING, "ERROR: Problem accessing the DNS. Defaulting to using hard-coded IP address for known Hosts.");
        //    if (strcmp(m_strHostName.c_str(),"ia.imdb.com")==0)
        //     service.sin_addr.s_addr = inet_addr("193.108.152.15");
        if (strcmp(m_strHostName.c_str(), "us.imdb.com") == 0)
          service.sin_addr.s_addr = inet_addr("207.171.166.140");
        else if (strcmp(m_strHostName.c_str(), "www.imdb.com") == 0)
          service.sin_addr.s_addr = inet_addr("207.171.166.140");
        else if (strcmp(m_strHostName.c_str(), "www.allmusic.com") == 0)
          service.sin_addr.s_addr = inet_addr("64.152.71.2");
        else if (strcmp(m_strHostName.c_str(), "image.allmusic.com") == 0)
          service.sin_addr.s_addr = inet_addr("64.152.70.67");
        else if (strcmp(m_strHostName.c_str(), "xoap.weather.com") == 0)
          service.sin_addr.s_addr = inet_addr("63.111.24.34");
        else
        {
          WSASetLastError(WSAHOST_NOT_FOUND);
          return false;
        }
      }
    }
    service.sin_port = htons(m_iPort);
  }
  m_socket.attach(socket(AF_INET, SOCK_STREAM, IPPROTO_TCP));

  // attempt to connection
  int nTries = 0;
  while (connect((SOCKET)m_socket, (sockaddr*) &service, sizeof(struct sockaddr)) == SOCKET_ERROR)
  {
    int e = WSAGetLastError();
    if ((e != WSAETIMEDOUT && e != WSAEADDRINUSE) || ++nTries > 3) // retry up to 3 times on timeout / addr in use
    {
      if (e == WSAECANCELLED)
        CLog::Log(LOGNOTICE, "HTTP: User canceled");

      Close();
      return false;
    }
    m_socket.attach(socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)); // new socket
  }
  hEvent = WSACreateEvent();
  return true;
}


//------------------------------------------------------------------------------------------------------------------
bool CHTTP::BreakURL(const string &strURL, string &strHostName, string &strUsername, string &strPassword, int& iPort, string &strFile)
{
  int pos1, pos2;
  char *ptr1, *ptr2, *ptr3;
  char *url = (char*)strURL.c_str();
  char szProtocol[128];
  // extract the protocol
  strFile = "";
  iPort = 80;
  strHostName = "";

  ptr1 = strstr(url, "://");
  if ( ptr1 == NULL )
  {
    return false;
  }

  pos1 = ptr1 - url;
  strncpy(szProtocol, url, pos1);
  szProtocol[pos1] = '\0';

  // jump the "://"
  ptr1 += 3;
  pos1 += 3;

  // look for username and pass
  ptr2 = strchr(ptr1, '@');
  // if @ is after first slash it isn't an username delimiter
  ptr3 = strchr(ptr1, '/');
  if( ptr2 && ptr3 && ptr3 < ptr2 )
    ptr2 = NULL;

  if( ptr2 )
  {
    char *szBuff = new char[ptr2 - ptr1 + 2];

    //If the colon is before the @, it's a username:password setting
    ptr3 = strchr(ptr1, ':');
    if(ptr3 && ptr3 < ptr2)
    {
      // URL like http://username:password@host.....

      strncpy(szBuff, ptr1, ptr3 - ptr1);
      szBuff[ptr3 - ptr1] = '\0';
      strUsername = szBuff;

      strncpy(szBuff, ptr3+1, ptr2 - ptr3 - 1);
      szBuff[ptr2 - ptr3 - 1] = '\0';
      strPassword = szBuff;
    }
    else
    {
      // URL like http://username@host.....
      strncpy(szBuff, ptr1, ptr2 - ptr1);
      szBuff[ptr2 - ptr1] = '\0';
      strUsername = szBuff;
      strPassword = "";
    }
    delete[] szBuff;

    //Skip ahead as auth has now been read
    pos1 += ptr2 - ptr1 + 1;
    ptr1 += ptr2 - ptr1 + 1;
  }
  else
  {
    strUsername = "";
    strPassword = "";
  }

  // look if the port is given
  ptr2 = strchr(ptr1, ':');
  // If the : is after the first / it isn't the port
  ptr3 = strchr(ptr1, '/');
  if (ptr3 && ptr3 - ptr2 < 0)
    ptr2 = NULL;
  if ( ptr2 == NULL )
  {
    // No port is given
    // Look if a path is given
    iPort = 80;
    ptr2 = strchr(ptr1, '/');
    if ( ptr2 == NULL )
    {
      // No path/filename
      // So we have an URL like http://www.hostname.com
      pos2 = strlen(url);
    }
    else
    {
      // We have an URL like http://www.hostname.com/file.txt
      pos2 = ptr2 - url;
    }
  }
  else
  {
    // We have an URL beginning like http://www.hostname.com:1212
    // Get the port number
    iPort = atoi(ptr2 + 1);
    pos2 = ptr2 - url;
  }
  char szHostName[128];
  strncpy(szHostName, ptr1, pos2 - pos1);
  szHostName[pos2 - pos1] = '\0';
  strHostName = szHostName;

  // Look if a path is given
  ptr2 = strchr(ptr1, '/');
  if ( ptr2 != NULL )
  {
    // A path/filename is given
    // check if it's not a trailing '/'
    if ( strlen(ptr2) > 1 )
    {
      // copy the path/filename in the URL container
      strFile = ptr2;
    }
  }
  // Check if a filenme was given or set, else set it with '/'
  if ( !strFile.size() )
  {
    strFile = "/";
  }
  return true;
}

//*********************************************************************************************

#define TIMEOUT (30*1000)
#define BUFSIZE (32768)

bool CHTTP::Send(char* pBuffer, int iLen)
{
  int iPos = 0;
  WSABUF buf;
  WSAOVERLAPPED ovl;
  DWORD n, flags;

  while (iLen > 0)
  {
    buf.buf = &pBuffer[iPos];
    buf.len = iLen;
    ovl.hEvent = hEvent;
    WSAResetEvent(hEvent);
    int ret = WSASend(m_socket, &buf, 1, &n, 0, &ovl, NULL);
    if (ret == SOCKET_ERROR)
    {
      if (WSAGetLastError() == WSA_IO_PENDING)
      {
        if (WSAWaitForMultipleEvents(1, &hEvent, FALSE, TIMEOUT, FALSE) == WSA_WAIT_EVENT_0)
        {
          if (!WSAGetOverlappedResult(m_socket, &ovl, &n, FALSE, &flags))
            return false;
        }
        else
        {
          WSACancelOverlappedIO(m_socket);
          WSASetLastError(WSAETIMEDOUT);
          return false;
        }
      }
      else if (WSAGetLastError() == WSAECANCELLED)
      {
        CLog::Log(LOGNOTICE, "HTTP: User canceled");
        return false;
      }
      else
        return false;
    }
    iPos += n;
    iLen -= n;
  }
  return true;
}

//*********************************************************************************************
bool CHTTP::Recv(int iLen)
{
  WSABUF buf;
  WSAOVERLAPPED ovl;
  DWORD n, flags;
  bool bUnknown = (iLen < 0);

  if (iLen > (BUFSIZE - m_RecvBytes) || bUnknown)
    iLen = (BUFSIZE - m_RecvBytes);

  if (!m_RecvBuffer)
    m_RecvBuffer = new char[BUFSIZE + 1];

  while (iLen > 0)
  {
    buf.buf = &m_RecvBuffer[m_RecvBytes];
    buf.len = iLen;
    flags = 0;
    ovl.hEvent = hEvent;
    WSAResetEvent(hEvent);
    int ret = WSARecv(m_socket, &buf, 1, &n, &flags, &ovl, NULL);
    if (ret == SOCKET_ERROR)
    {
      if (WSAGetLastError() == WSA_IO_PENDING)
      {
        if (WSAWaitForMultipleEvents(1, &hEvent, FALSE, TIMEOUT, FALSE) == WSA_WAIT_EVENT_0)
        {
          if (!WSAGetOverlappedResult(m_socket, &ovl, &n, FALSE, &flags))
          {
            return false;
          }
        }
        else
        {
          WSACancelOverlappedIO(m_socket);
          WSASetLastError(WSAETIMEDOUT);
          return false;
        }
      }
      else if (WSAGetLastError() == WSAECANCELLED)
      {
        CLog::Log(LOGNOTICE, "HTTP: User canceled");
        return false;
      }
      else
      {
        return false;
      }
    }
    if (!n)
    {
      shutdown(m_socket, SD_BOTH);
      m_RecvBuffer[m_RecvBytes] = 0;
      WSASetLastError(0);
      return bUnknown; // graceful close
    }
    m_RecvBytes += n;
    iLen -= n;

    if (bUnknown)
    {
      m_RecvBuffer[m_RecvBytes] = 0;
      return true; // got some data, don't get any more
    }
  }
  m_RecvBuffer[m_RecvBytes] = 0;
  return true;
}

//------------------------------------------------------------------------------------------------------------------

void CHTTP::SetCookie(const string &strCookie)
{
  m_strCookie = strCookie;
}

//------------------------------------------------------------------------------------------------------------------

int CHTTP::Open(const string& strURL, const char* verb, const char* pData)
{
  string strFile = "";
  m_strHostName = "";
  m_redirectedURL = strURL;
  if (!BreakURL(strURL, m_strHostName, m_strUsername, m_strPassword, m_iPort, strFile))
  {
    CLog::Log(LOGERROR, "Invalid url: %s", strURL.c_str());
    return 0;
  }

  if (!Connect())
  {
    CLog::Log(LOGERROR, "Unable to connect to %s: %d", m_strHostName.c_str(), WSAGetLastError());
    return 0;
  }

  // send request...
  char* szHTTPHEADER = (char*)_alloca(350 + m_strHostName.size() + m_strCookie.size() + (pData ? strlen(pData) : 0));
  if (stricmp(verb, "POST")==0)
  {
    strcpy(szHTTPHEADER, "Content-Type: application/x-www-form-urlencoded\r\n");
  }
  else
  {
    strcpy(szHTTPHEADER, "Connection: close\r\n"
          "Accept: image/gif, image/x-xbitmap, image/jpeg, image/pjpeg, application/msword, */*\r\n"
          "Accept-Language: en-us\r\n");
  }
  strcat(szHTTPHEADER, "Host:");
  strcat(szHTTPHEADER, m_strHostName.c_str());
  strcat(szHTTPHEADER, "\r\n"
         "User-Agent: Mozilla/4.0 (compatible; MSIE 6.0; Windows NT 5.1)\r\n");
  if (m_strCookie.size())
  {
    // is this even valid in http?
    strcat(szHTTPHEADER, "Cookie: ");
    strcat(szHTTPHEADER, m_strCookie.c_str());
    strcat(szHTTPHEADER, "\r\n");
  }
  if (pData)
  {
    sprintf(szHTTPHEADER + strlen(szHTTPHEADER), "Content-Length: %d\r\n\r\n", strlen(pData));
    strcat(szHTTPHEADER, pData);
  }

  if( m_strUsername.size())
  {
    //Basic authentication
    char *szBuff, *szEncode;
    szBuff = (char*)_alloca(m_strUsername.size() + m_strPassword.size() + 2);
    sprintf(szBuff, "%s:%s", m_strUsername.c_str(), m_strPassword.c_str());
   
    int len = strlen(szBuff);
    szEncode = (char*)_alloca(len*2); 
    
    len = base64_encode(szBuff, len, szEncode, len*2);
    if( len < 0 )
    {
      CLog::Log(LOGERROR, "Base64 Encode Overflow");
      Close();
      return 0;
    }
    szEncode[len] = '\0';
    strcat(szHTTPHEADER, "Authorization: Basic ");
    strcat(szHTTPHEADER, szEncode);
    strcat(szHTTPHEADER, "\r\n");
  }

  char* szGet;
  if (m_strProxyServer.size())
  {
    szGet = (char*)_alloca(strlen(szHTTPHEADER) + strURL.size() + 20);
    sprintf(szGet, "%s %s HTTP/1.1\r\n%s\r\n", verb, strURL.c_str(), szHTTPHEADER);
  }
  else
  {
    szGet = (char*)_alloca(strlen(szHTTPHEADER) + strFile.size() + 20);
    sprintf(szGet, "%s %s HTTP/1.1\r\n%s\r\n", verb, strFile.c_str(), szHTTPHEADER);
  }


  //CLog::Log(LOGINFO, "Opening page with request:\n%s", szGet);

  if (!Send(szGet, strlen(szGet)))
  {
    CLog::Log(LOGERROR, "Send failed: %d", WSAGetLastError());
    Close();
    return 0;
  }

  m_RecvBytes = 0;
  char* HeaderEnd;
  do
  {
    if (m_RecvBytes >= BUFSIZE)
    {
      CLog::Log(LOGERROR, "Invalid reply from server");
      Close();
      return 0;
    }
    if (!Recv( -1))
    {
      CLog::Log(LOGERROR, "Recv failed: %d", WSAGetLastError());
      Close();
      return 0;
    }
  }
  while ((HeaderEnd = strstr(m_RecvBuffer, "\r\n\r\n")) == NULL);
  HeaderEnd += 4;

  //CLog::Log(LOGINFO, "Retrieved:\n%s", m_RecvBuffer);
  // Parse the return value...first line is:
  // PROTOCOL STATUS MESSAGE
  // Currently assuming 1 space between PROTOCOL/STATUS and STATUS/MESSAGE
  CStdString strProtocol, strStatus, strReason;
  char * pszStart = m_RecvBuffer;
  int nWhich = 0;
  char *pszCurrent;
  for (pszCurrent = pszStart; *pszCurrent != '\r'; pszCurrent++)
  {
    if (*pszCurrent == ' ')
    {
      switch (nWhich)
      {
      case 0:
        strProtocol.assign(pszStart, pszCurrent - pszStart);
        pszStart = pszCurrent + 1;
        nWhich = 1;
        break;

      case 1:
        strStatus.assign(pszStart, pszCurrent - pszStart);
        pszStart = pszCurrent + 1;
        nWhich = 2;
        break;
      }
    }
  }
  strReason.assign(pszStart, pszCurrent - pszStart);
  char *end = pszCurrent;

  // expected return:
  // HTTP-Version SP Status-Code SP Reason-Phrase CRLF
  //if (strnicmp(m_RecvBuffer, "HTTP/", 5) || m_RecvBuffer[5] != '1' || m_RecvBuffer[6] != '.' || m_RecvBuffer[7] < '0' || m_RecvBuffer[7] > '1')
  //{
  // CLog::Log(LOGERROR, "Invalid reply from server");
  // Close();
  // return 0; // malformed reply
  //}
  //int status = atoi(m_RecvBuffer + 9);
  //char* end = strchr(m_RecvBuffer + 13, '\r');
  //string strReason(m_RecvBuffer + 13, end);
  m_strHeaders.assign(end + 2, HeaderEnd);

  // Keep the headers string for compatability for now, but also parse
  // headers into a map
  ParseHeaders();

  // Check protocol for any type other than HEAD
  if (stricmp(verb, "HEAD"))
  {
    if (strProtocol != "HTTP/1.0" && strProtocol != "HTTP/1.1")
    {
      CLog::Log(LOGERROR, "Invalid reply from server");
      Close();
      return 0; // malformed reply
    }
  }

  // Return value is the status of the request
  int status = atoi(strStatus.c_str());

  memmove(m_RecvBuffer, HeaderEnd, 1 + (m_RecvBytes -= (HeaderEnd - m_RecvBuffer)));

  if (status < 100)
  {
    CLog::Log(LOGERROR, "Invalid reply from server");
    Close();
    return 0; // malformed reply
  }
  else if (status < 300)
  {
    return status; // successful
  }
  else if (status < 400)
  {
    // redirect
    bool CanHandle = false;
    switch (status)
    {
    case 302:
      // 302 Found - auto redirect if this is a GET
      CanHandle = !stricmp(verb, "GET");
      break;
    case 303:
      // 303 See Other - perform GET on the new resource
      verb = "GET";
      CanHandle = true;
      break;
    case 307:
      // Temporary Redirect - auto redirect if NOT a GET
      CanHandle = !!stricmp(verb, "GET");
      break;
    }

    Close();

    if (!CanHandle)
    {
      CLog::Log(LOGERROR, "Server returned: %d %s", status, strReason.c_str());
      return status; // unhandlable
    }

    string::size_type n = m_strHeaders.find("Location:");
    if (n != string::npos)
    {
      n += 10;
      string strURL(m_strHeaders.begin() + n, m_strHeaders.begin() + m_strHeaders.find('\r', n));
      if (strnicmp(strURL.c_str(), "http:", 5))
      {
        char portstr[8];
        sprintf(portstr, ":%d", m_iPort);
        strURL.insert(0, portstr);
        strURL.insert(0, m_strHostName.c_str());
        strURL.insert(0, "http://");
      }
      CLog::Log(LOGINFO, "%d Redirected: %s", status, strURL.c_str());
      m_RecvBytes = 0;
      m_redirectedURL = strURL;
      return Open(strURL, verb, pData);
    }
    else
    {
      CLog::Log(LOGERROR, "Invalid reply from server");
      Close();
      return 0; // malformed reply
    }
  }
  else
  {
    CLog::Log(LOGERROR, "Server returned: %d %s", status, strReason.c_str());
    Close();
    return status; // error
  }
}

//------------------------------------------------------------------------------------------------------------------

void CHTTP::ParseHeaders()
{
  string::size_type nStart, nColon, nEnd;
  string::size_type i;

  nStart = nColon = nEnd = 0;
  for (i = 0; i < m_strHeaders.length(); i++)
  {
    // I am assuming here that all lines between headers are '\r\n' terminated.
    // I don't know if that is a safe assumption.  In this code, '\r' terminated will
    // work fine, but '\n' terminators will fail (headers will not be parsed)
    switch (m_strHeaders[i])
    {
    case ':':
      nColon = i;
      break;

    case '\r':
      // End of the line...parse the header
      nEnd = i;
      ParseHeader(nStart, nColon, nEnd);
      nStart = i + 1;

      // Clear previous data
      nColon = nEnd = 0;
      break;

    case '\n':
      // Extra whitespace...advance the pointer
      nStart = i + 1;
      break;
    }
  }

  // Call it again for the last entry
  ParseHeader(nStart, nColon, i);
}

//------------------------------------------------------------------------------------------------------------------

void CHTTP::ParseHeader(string::size_type start, string::size_type colon, string::size_type end)
{
  // If colon or end are 0, bail
  if (colon == 0 || end == 0)
  {
    return ;
  }

  CStdString strName, strValue;

  strName.assign(m_strHeaders.substr(start, colon - start));
  strValue.assign(m_strHeaders.substr(colon + 2, end - colon - 2));

  // Convert name to upper-case for case insensitive lookups
  strName.ToUpper();
  m_mapHeaders[strName] = strValue;
}

//------------------------------------------------------------------------------------------------------------------

void CHTTP::Close()
{
  int e = WSAGetLastError(); // make sure it's preserved
  m_socket.reset();
  if (hEvent != WSA_INVALID_EVENT)
    WSACloseEvent(hEvent);
  hEvent = WSA_INVALID_EVENT;
  m_RecvBytes = 0;
  if (m_RecvBuffer)
    delete [] m_RecvBuffer;
  m_RecvBuffer = 0;
  WSASetLastError(e);
}

//------------------------------------------------------------------------------------------------------------------

void CHTTP::Cancel()
{
  WSACancelOverlappedIO((SOCKET)m_socket);
}
