/*
 *      Copyright (C) 2010 Alwin Esch (Team XBMC)
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

#pragma once
#include <deque>
#include "pvrclient-vdrVNSI_os.h"
#include "StdString.h"

#include <algorithm>
#include <vector>
#include <map>

class cResponsePacket;
class cRequestPacket;

class cVNSISession
{
public:
  cVNSISession();
  ~cVNSISession();

  bool              Open(CStdString hostname, int port, long timeout, CStdString name = "");
  void              Close();
  void              Abort();

  cResponsePacket*  ReadMessage(int timeout = 10);
  bool              SendMessage(cRequestPacket* vrp);
  int               sendData(void* bufR, size_t count);
  int               readData(uint8_t* buffer, int totalBytes, int TimeOut = 2);

  cResponsePacket*  ReadResult(cRequestPacket* vrp, bool sequence = true);
  bool              ReadSuccess(cRequestPacket* m, bool sequence = true, std::string action = "");
  int               GetProtocol()   { return m_protocol; }
  CStdString        GetServerName() { return m_server; }
  CStdString        GetVersion()    { return m_version; }

private:
  SOCKET      m_fd;
  int         m_protocol;
  CStdString  m_server;
  CStdString  m_version;

  std::deque<cResponsePacket*> m_queue;
  const unsigned int    m_queue_size;
};
