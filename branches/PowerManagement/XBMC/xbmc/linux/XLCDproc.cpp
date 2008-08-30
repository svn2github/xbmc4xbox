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
#include "PlatformInclude.h"
#include "XLCDproc.h"
#include "../utils/log.h"
#include "Settings.h"

#define SCROLL_SPEED_IN_MSEC 250


XLCDproc::XLCDproc()
{
  m_iActualpos   = 0;
  m_iRows        = 4;
  m_iColumns     = 16;
  m_iBackLight   = 32;
  m_iLCDContrast = 50;
  sockfd         = -1;
}

XLCDproc::~XLCDproc()
{
}

void XLCDproc::Initialize()
{
  if (g_guiSettings.GetInt("lcd.type") == LCD_TYPE_NONE)
    return ;//nothing to do

  ILCD::Initialize();

  struct hostent *server;
  server = gethostbyname("localhost");
  if (server == NULL)
  {
     CLog::Log(LOGERROR, "LCDproc:Initialize: Unable to resolve LCDd host.");
     return;
  }

  struct sockaddr_in serv_addr;

  sockfd = socket(AF_INET, SOCK_STREAM, 0);
  bzero((char *) &serv_addr, sizeof(serv_addr));
  serv_addr.sin_family = AF_INET;

  bcopy((char *)server->h_addr_list[0],
        (char *)&serv_addr.sin_addr,
        server->h_length);

  //Connect to default LCDd port, hard coded for now.

  serv_addr.sin_port = htons(13666);
  if (connect(sockfd,(struct sockaddr*)&serv_addr,sizeof(serv_addr)) < 0)
  {
    CLog::Log(LOGERROR, "LCDproc:Initialize: Unable to connect to host.");
    return;
  }

  //Build command to setup screen
  CStdString cmd;
  cmd = "hello\nscreen_add xbmc\n";
  if (!g_advancedSettings.m_lcdHeartbeat)
    cmd.append("screen_set xbmc -heartbeat off\n");
  cmd.append("widget_add xbmc line1 scroller\n");
  cmd.append("widget_add xbmc line2 scroller\n");
  cmd.append("widget_add xbmc line3 scroller\n");
  cmd.append("widget_add xbmc line4 scroller\n");

  //Send to server
  if (write(sockfd,cmd.c_str(),cmd.size()) < 0)
    CLog::Log(LOGERROR, "LCDproc:Initialize: Unable to write to socket");
  m_bStop = false;
}
void XLCDproc::SetBackLight(int iLight)
{
  //TODO:
}
void XLCDproc::SetContrast(int iContrast)
{
  //TODO: Not sure if you can control contrast from client
}

void XLCDproc::Stop()
{
  if (!m_bStop)
  {
    //Close connection
    if (sockfd >= 0)
      shutdown(sockfd, SHUT_RDWR);
    m_bStop = true;
  }
}

void XLCDproc::SetLine(int iLine, const CStdString& strLine)
{
  if (m_bStop)
    return;

  if (iLine < 0 || iLine >= (int)m_iRows)
    return;

  char cmd[1024];
  CStdString strLineLong = strLine;
  strLineLong.Trim();
  StringToLCDCharSet(strLineLong);

  while (strLineLong.size() < m_iColumns)
    strLineLong += " ";

  if (strLineLong != m_strLine[iLine])
  {
    int ln = iLine + 1;
    sprintf(cmd, "widget_set xbmc line%i 1 %i 16 %i m 1 \"%s\"\n", ln, ln, ln, strLineLong.c_str());
    //CLog::Log(LOGINFO, "LCDproc sending command: %s",cmd);
    if (write(sockfd, cmd, strlen(cmd)) < 0)
    {
        m_bStop = true;
        CLog::Log(LOGERROR, "XLCDproc::SetLine() Unable to write to socket, LCDd not running?");
        return;
    }
    m_bUpdate[iLine] = true;
    m_strLine[iLine] = strLineLong;
    m_event.Set();
  }
}

void XLCDproc::Process()
{
}
