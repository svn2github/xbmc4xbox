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

#ifdef HAS_EVENT_SERVER

#include "EventServer.h"
#include "EventPacket.h"
#include "EventClient.h"
#include "Socket.h"
#include "CriticalSection.h"
#include "SingleLock.h"
#include <map>
#include <queue>

using namespace EVENTSERVER;
using namespace EVENTPACKET;
using namespace EVENTCLIENT;
using namespace SOCKETS;
using namespace std;

/************************************************************************/
/* CEventServer                                                         */
/************************************************************************/
CEventServer* CEventServer::m_pInstance = NULL;
CEventServer::CEventServer()
{
  m_pSocket       = NULL;
  m_pPacketBuffer = NULL;
  m_bStop         = false;
  m_pThread       = NULL;
  m_bRunning      = false;
  m_bRefreshSettings = false;

  // default timeout in ms for receiving a single packet
  m_iListenTimeout = 1000;
}

void CEventServer::RemoveInstance()
{
  if (m_pInstance)
  {
    delete m_pInstance;
    m_pInstance=NULL;
  }
}

CEventServer* CEventServer::GetInstance()
{
  if (!m_pInstance)
  {
    m_pInstance = new CEventServer();
  }
  return m_pInstance;
}

void CEventServer::StartServer()
{
  if (m_pThread)
    return;

  // set default port
  string port = (const char*)g_guiSettings.GetString("remoteevents.port");
  if (port.length() == 0)
  {
    m_iPort = 9777;
  }
  else
  {
    m_iPort = atoi(port.c_str());
  }
  if (m_iPort > 65535 || m_iPort < 1)
  {
    CLog::Log(LOGERROR, "ES: Invalid port specified %d, defaulting to 9777", m_iPort);
    m_iPort = 9777;
  }

  // max clients
  m_iMaxClients = g_guiSettings.GetInt("remoteevents.maxclients");
  if (m_iMaxClients < 0)
  {
    CLog::Log(LOGERROR, "ES: Invalid maximum number of clients specified %d", m_iMaxClients);
    m_iMaxClients = 20;
  }

  m_bStop = false;
  m_pThread = new CThread(this);
  m_pThread->Create();
  m_pThread->SetName("EventServer");
}

void CEventServer::StopServer()
{
  m_bStop = true;
  if (m_pThread)
  {
    m_pThread->WaitForThreadExit(2000);
    delete m_pThread;
  }
  m_pThread = NULL;
}

void CEventServer::Cleanup()
{
  if (m_pSocket)
  {
    m_pSocket->Close();
    delete m_pSocket;
    m_pSocket = NULL;
  }

  if (m_pPacketBuffer)
  {
    free(m_pPacketBuffer);
    m_pPacketBuffer = NULL;
  }
  CSingleLock lock(m_critSection);

  map<unsigned long, CEventClient*>::iterator iter = m_clients.begin();
  while (iter != m_clients.end())
  {
    if (iter->second)
    {
      delete iter->second;
    }
    m_clients.erase(iter);
    iter =  m_clients.begin();
  }
}

void CEventServer::Run()
{
  CAddress any_addr;
  CSocketListener listener;
  int packetSize = 0;

#ifndef _XBOX
  if (!g_guiSettings.GetBool("remoteevents.allinterfaces"))
    any_addr.SetAddress ("127.0.0.1");  // only listen on localhost
#endif

  CLog::Log(LOGNOTICE, "ES: Starting UDP Event server on %s:%d", any_addr.Address(), m_iPort);

  Cleanup();

  // create socket and initialize buffer
  m_pSocket = CSocketFactory::CreateUDPSocket();
  if (!m_pSocket)
  {
    CLog::Log(LOGERROR, "ES: Could not create socket, aborting!");
    return;
  }
  m_pPacketBuffer = (unsigned char *)malloc(PACKET_SIZE);

  if (!m_pPacketBuffer)
  {
    CLog::Log(LOGERROR, "ES: Out of memory, could not allocate packet buffer");
    return;
  }

  // bind to IP and start listening on port
  int port_range = g_guiSettings.GetInt("remoteevents.portrange");
  if (port_range < 1 || port_range > 100)
  {
    CLog::Log(LOGERROR, "ES: Invalid port range specified %d, defaulting to 10", port_range);
    port_range = 10;
  }
  if (!m_pSocket->Bind(any_addr, m_iPort, port_range))
  {
    CLog::Log(LOGERROR, "ES: Could not listen on port %d", m_iPort);
    return;
  }

  // add our socket to the 'select' listener
  listener.AddSocket(m_pSocket);

  m_bRunning = true;

  while (!m_bStop)
  {
    // start listening until we timeout
    if (listener.Listen(m_iListenTimeout))
    {
      CAddress addr;
      if ((packetSize = m_pSocket->Read(addr, PACKET_SIZE, (void *)m_pPacketBuffer)) > -1)
      {
        ProcessPacket(addr, packetSize);
      }
    }

    // execute events
    ExecuteEvents();

    // refresh client list
    RefreshClients();

    // broadcast
    // BroadcastBeacon();
  }

  CLog::Log(LOGNOTICE, "ES: UDP Event server stopped");
  m_bRunning = false;
  Cleanup();
}

void CEventServer::ProcessPacket(CAddress& addr, int pSize)
{
  // check packet validity
  CEventPacket* packet = new CEventPacket(pSize, m_pPacketBuffer);
  if(packet == NULL)
  {
    CLog::Log(LOGERROR, "ES: Out of memory, cannot accept packet");
    return;
  }

  unsigned int clientToken;

  if (!packet->IsValid())
  {
    CLog::Log(LOGDEBUG, "ES: Received invalid packet");
    delete packet;
    return;
  }

  clientToken = packet->ClientToken();
  if (!clientToken)
    clientToken = addr.ULong(); // use IP if packet doesn't have a token

  CSingleLock lock(m_critSection);

  // first check if we have a client for this address
  map<unsigned long, CEventClient*>::iterator iter = m_clients.find(clientToken);

  if ( iter == m_clients.end() )
  {
    if ( m_clients.size() >= (unsigned int)m_iMaxClients)
    {
      CLog::Log(LOGWARNING, "ES: Cannot accept any more clients, maximum client count reached");
      delete packet;
      return;
    }

    // new client
    CEventClient* client = new CEventClient ( addr );
    if (client==NULL)
    {
      CLog::Log(LOGERROR, "ES: Out of memory, cannot accept new client connection");
      delete packet;
      return;
    }

    m_clients[clientToken] = client;
  }
  m_clients[clientToken]->AddPacket(packet);
}

void CEventServer::RefreshClients()
{
  CSingleLock lock(m_critSection);
  map<unsigned long, CEventClient*>::iterator iter = m_clients.begin();

  while ( iter != m_clients.end() )
  {
    if (! (iter->second->Alive()))
    {
      CLog::Log(LOGNOTICE, "ES: Client %s from %s timed out", iter->second->Name().c_str(),
                iter->second->Address().Address());
      delete iter->second;
      m_clients.erase(iter);
      iter = m_clients.begin();
    }
    else
    {
      if (m_bRefreshSettings)
      {
        iter->second->RefreshSettings();
      }
      iter++;
    }
  }
  m_bRefreshSettings = false;
}

void CEventServer::ExecuteEvents()
{
  CSingleLock lock(m_critSection);
  map<unsigned long, CEventClient*>::iterator iter = m_clients.begin();

  while (iter != m_clients.end())
  {
    iter->second->ExecuteEvents();
    iter++;
  }
}

unsigned short CEventServer::GetButtonCode(std::string& strMapName, bool& isAxis, float& fAmount)
{
  CSingleLock lock(m_critSection);
  map<unsigned long, CEventClient*>::iterator iter = m_clients.begin();
  unsigned short bcode = 0;

  while (iter != m_clients.end())
  {
    bcode = iter->second->GetButtonCode(strMapName, isAxis, fAmount);
    if (bcode)
      return bcode;
    iter++;
  }
  return bcode;
}

bool CEventServer::GetMousePos(float &x, float &y)
{
  CSingleLock lock(m_critSection);
  map<unsigned long, CEventClient*>::iterator iter = m_clients.begin();

  while (iter != m_clients.end())
  {
    if (iter->second->GetMousePos(x, y))
      return true;
    iter++;
  }
  return false;
}

#endif // HAS_EVENT_SERVER
