/*
 *      Copyright (C) 2005-2008 Team XBMC
 *      http://www.xbmc.org
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
 *  along with XBMC; see the file COPYING.  If not, write to
 *  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 *  http://www.gnu.org/copyleft/gpl.html
 *
 */

#include "stdafx.h"
#include "DNSNameCache.h"
#include "Settings.h"
#include "GUISettings.h"
#include "utils/SingleLock.h"
#include "utils/log.h"

CDNSNameCache g_DNSCache;

CCriticalSection CDNSNameCache::m_critical;

CDNSNameCache::CDNSNameCache(void)
{}

CDNSNameCache::~CDNSNameCache(void)
{}

bool CDNSNameCache::Lookup(const CStdString& strHostName, CStdString& strIpAdres)
{
  // first see if this is already an ip address
  unsigned long ulHostIp = inet_addr( strHostName.c_str() );

  if ( ulHostIp != 0xFFFFFFFF )
  {
    // yes it is, just return it
    strIpAdres.Format("%d.%d.%d.%d", (ulHostIp & 0xFF), (ulHostIp & 0xFF00) >> 8, (ulHostIp & 0xFF0000) >> 16, (ulHostIp & 0xFF000000) >> 24 );
    return true;
  }

  // nop this is a hostname
  // check if we already cached the hostname <-> ip address
  if(g_DNSCache.GetCached(strHostName, strIpAdres))
  {
    // it was already cached
    return true;
  }

  // hostname not found in cache.
  // do a DNS lookup
  {
    SOCKET sd;          /* Socket descriptor */
    struct sockaddr_in socket_address;
    struct hostent *host;

#ifndef _LINUX
    /* Open a windows connection */
    WSADATA wsaData;    /* Used to open Windows connection */
    if (WSAStartup(0x0101, &wsaData) != 0)
    {
      OutputDebugString("Could not open Windows connection\n");
      return false;
    }
#endif

    /* Open up a socket */
    sd = socket(AF_INET, SOCK_STREAM, 0);

    /* Make sure the socket was opened */
    if (sd == SOCKET_ERROR)
    {
      OutputDebugString("Could not open socket.\n");
#ifndef _LINUX
      WSACleanup();
#endif
      return false;
    }

    /* Bind with IP address */
    memset(&socket_address, '\0', sizeof(struct sockaddr_in));
    socket_address.sin_family = AF_INET;
    socket_address.sin_port = htons((short)4000);

    /* Get host by name */
    host = gethostbyname(strHostName.c_str());

    /* Print error message if could not find host */
    if (host == NULL || host->h_addr_list[0] == NULL)
    {
      CLog::Log(LOGERROR, "Unable to find host: %s", strHostName.c_str());
      closesocket(sd);
#ifndef _LINUX
      WSACleanup();
#endif
      return false;
    }

    /* Print out host name */
    CLog::Log(LOGDEBUG, "host name = %s\n", host->h_name);

    /* Loop through and print out any aliases */
#ifndef _LINUX
    // this segfaults due to host->h_aliases[1] being invalid on
    // the weather.com lookup
    int count = 0;
    while(1)
    {
      if(host->h_aliases[count] == NULL)
      {
        break;
      }
      CLog::Log(LOGDEBUG, "alias %d: %s\n", count + 1, host->h_aliases[count]);
      ++count;
    }
#endif

    /* Print out all IP addresses of name */
    if (host->h_addr_list[0])
    {
      strIpAdres.Format("%d.%d.%d.%d", (unsigned char)host->h_addr_list[0][0], (unsigned char)host->h_addr_list[0][1], (unsigned char)host->h_addr_list[0][2], (unsigned char)host->h_addr_list[0][3]);
      g_DNSCache.Add(strHostName, strIpAdres);
    }

    closesocket(sd);
#ifndef _LINUX
    WSACleanup();
#endif

    return true;
  }
  return false;
}

bool CDNSNameCache::GetCached(const CStdString& strHostName, CStdString& strIpAdres)
{
  CSingleLock lock(m_critical);

  // loop through all DNSname entries and see if strHostName is cached
  for (int i = 0; i < (int)g_DNSCache.m_vecDNSNames.size(); ++i)
  {
    CDNSName& DNSname = g_DNSCache.m_vecDNSNames[i];
    if ( DNSname.m_strHostName == strHostName )
    {
      strIpAdres = DNSname.m_strIpAdres;
      return true;
    }
  }

  // not cached
  return false;
}

void CDNSNameCache::Add(const CStdString &strHostName, const CStdString &strIpAddress)
{
  CDNSName dnsName;

  dnsName.m_strHostName = strHostName;
  dnsName.m_strIpAdres  = strIpAddress;

  CSingleLock lock(m_critical);
  g_DNSCache.m_vecDNSNames.push_back(dnsName);
}

