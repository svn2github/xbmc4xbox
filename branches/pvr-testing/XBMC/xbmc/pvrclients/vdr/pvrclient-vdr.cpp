/*
 *      Copyright (C) 2005-2009 Team XBMC
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

#include "client.h"
#include "pvrclient-vdr.h"
#include "pvrclient-vdr_os.h"

#ifdef _LINUX
#define SD_BOTH SHUT_RDWR
#endif

#define SEEK_POSSIBLE 0x10 // flag used to check if protocol allows seeks

using namespace std;

#ifndef _LINUX
extern "C" long long atoll(const char *ca)
{
  long long ig=0;
  int       sign=1;
  /* test for prefixing white space */
  while (*ca == ' ' || *ca == '\t' )
    ca++;
  /* Check sign entered or no */
  if ( *ca == '-' )
    sign = -1;
  /* convert string to int */
  while (*ca != '\0')
    if (*ca >= '0' && *ca <= '9')
      ig = ig * 10LL + *ca++ - '0';
    else
      ca++;
  return (ig*(long long)sign);
}
#endif

bool PVRClientVDR::m_bStop						= true;
SOCKET PVRClientVDR::m_socket_data				= INVALID_SOCKET;
SOCKET PVRClientVDR::m_socket_video				= INVALID_SOCKET;
CVTPTransceiver *PVRClientVDR::m_transceiver    = NULL;
bool PVRClientVDR::m_bConnected					= false;

/************************************************************/
/** Class interface */

PVRClientVDR::PVRClientVDR()
{
  m_iCurrentChannel   = 1;
  m_transceiver       = new CVTPTransceiver();
  m_bConnected        = false;
  m_socket_video      = INVALID_SOCKET;
  m_socket_data       = INVALID_SOCKET;
  m_bStop             = true;

  pthread_mutex_init(&m_critSection, NULL);
}

PVRClientVDR::~PVRClientVDR()
{
  Disconnect();
}


/************************************************************/
/** Server handling */

PVR_ERROR PVRClientVDR::GetProperties(PVR_SERVERPROPS *props)
{
  props->SupportChannelLogo        = false;
  props->SupportTimeShift          = false;
  props->SupportEPG                = true;
  props->SupportRecordings         = true;
  props->SupportTimers             = true;
  props->SupportRadio              = true;
  props->SupportChannelSettings    = true;
  props->SupportTeletext           = false;
  props->SupportDirector           = false;
  props->SupportBouquets           = false;

  return PVR_ERROR_NO_ERROR;
}

bool PVRClientVDR::Connect()
{
  /* Open Streamdev-Server VTP-Connection to VDR Backend Server */
  if (!m_transceiver->Open(m_sHostname, m_iPort))
    return false;

  /* Check VDR streamdev is patched by calling a newly added command */
  if (GetNumChannels() == -1)
  {
    XBMC_log(LOG_ERROR, "PCRClient-vdr: Detected unsupported Streamdev-Version");
    return false;
  }

  /* Get Data socket from VDR Backend */
  m_socket_data = m_transceiver->GetStreamData();

  /* If received socket is invalid, return */
  if (m_socket_data == INVALID_SOCKET)
  {
    XBMC_log(LOG_ERROR, "PCRClient-vdr: Couldn't get socket for data response");
    return false;
  }

  /* Start VTP Listening Thread */
//  m_bStop = false;
//  if (pthread_create(&m_thread, NULL, &Process, (void *)"PVRClientVDR VTP-Listener") != 0) {
//    return false;
//  }

  m_bConnected = true;
  return true;
}

void PVRClientVDR::Disconnect()
{
  m_bStop = true;
//  pthread_join(m_thread, NULL);

  m_bConnected = false;

  /* Check if  stream sockets are open, if yes close them */
  if (m_socket_data != INVALID_SOCKET)
  {
    m_transceiver->AbortStreamData();
    closesocket(m_socket_data);
    m_socket_data = INVALID_SOCKET;
  }

  if (m_socket_video != INVALID_SOCKET)
  {
    m_transceiver->AbortStreamLive();
    closesocket(m_socket_video);
    m_socket_video = INVALID_SOCKET;
  }

  /* Close Streamdev-Server VTP Backend Connection */
  m_transceiver->Close();
}

bool PVRClientVDR::IsUp()
{
  if (m_bConnected || m_transceiver->IsOpen())
  {
    return true;
  }
  return false;
}

void* PVRClientVDR::Process(void*)
{
  char   		 data[1024];
  fd_set         set_r, set_e;
  struct timeval tv;
  int            res;

  while (!m_bStop)
  {
	if ((!m_transceiver->IsOpen()) || (m_socket_data == INVALID_SOCKET))
	{
	  XBMC_log(LOG_ERROR, "PVRClientVDR::Process - Loosed connectio to VDR");
	  m_bConnected = false;
	  return NULL;
	}

	tv.tv_sec = 5;
	tv.tv_usec = 0;

	FD_ZERO(&set_r);
	FD_ZERO(&set_e);
	FD_SET(m_socket_data, &set_r);
	FD_SET(m_socket_data, &set_e);
	res = select(FD_SETSIZE, &set_r, NULL, &set_e, &tv);
	if (res < 0)
	{
	  XBMC_log(LOG_ERROR, "PVRClientVDR::Process - select failed");
	  continue;
	}

    if (res == 0)
      continue;

	res = recv(m_socket_data, (char*)data, sizeof(data), 0);
	if (res < 0)
	{
	  XBMC_log(LOG_ERROR, "PVRClientVDR::Process - failed");
	  continue;
	}

	if (res == 0)
	   continue;

	CStdString respStr = data;
	if (respStr.find("MODT", 0) == 0)
	{
	  PVR_event_callback(PVR_EVENT_TIMERS_CHANGE, "");
	}
	else if (respStr.find("DELT", 0) == 0)
	{
	  PVR_event_callback(PVR_EVENT_TIMERS_CHANGE, "");
	}
	else if (respStr.find("ADDT", 0) == 0)
	{
	  PVR_event_callback(PVR_EVENT_TIMERS_CHANGE, "");
	}
	else if (respStr.find("MODC", 0) == 0)
	{
	  PVR_event_callback(PVR_EVENT_CHANNELS_CHANGE, "");
	}
	else if (respStr.find("DELC", 0) == 0)
	{
	  PVR_event_callback(PVR_EVENT_CHANNELS_CHANGE, "");
	}
	else if (respStr.find("ADDC", 0) == 0)
	{
	  PVR_event_callback(PVR_EVENT_CHANNELS_CHANGE, "");
	}
	else
	{
	  XBMC_log(LOG_ERROR, "PVRClientVDR::Process - Unkown respond command %s", respStr.c_str());
	}
  }
  return NULL;
}


/************************************************************/
/** General handling */

const char* PVRClientVDR::GetBackendName()
{
  if (!m_transceiver->IsOpen())
    return "";

  pthread_mutex_lock(&m_critSection);

  vector<string>  lines;
  int             code;

  if (!m_transceiver->SendCommand("STAT name", code, lines))
  {
    pthread_mutex_unlock(&m_critSection);
    return "";
  }

  vector<string>::iterator it = lines.begin();
  string& data(*it);

  pthread_mutex_unlock(&m_critSection);
  return data.c_str();
}

const char* PVRClientVDR::GetBackendVersion()
{
  if (!m_transceiver->IsOpen())
    return "";

  pthread_mutex_lock(&m_critSection);

  vector<string>  lines;
  int             code;

  if (!m_transceiver->SendCommand("STAT version", code, lines))
  {
    pthread_mutex_unlock(&m_critSection);
    return "";
  }

  vector<string>::iterator it = lines.begin();

  string& data(*it);

  pthread_mutex_unlock(&m_critSection);
  return data.c_str();
}

const char* PVRClientVDR::GetConnectionString()
{
  char buffer[1024];
  sprintf(buffer, "%s:%i", m_sHostname.c_str(), m_iPort);
  string s_tmp = buffer;
  return s_tmp.c_str();
}

PVR_ERROR PVRClientVDR::GetDriveSpace(long long *total, long long *used)
{
  if (!m_transceiver->IsOpen())
    return PVR_ERROR_SERVER_ERROR;

  pthread_mutex_lock(&m_critSection);

  vector<string>  lines;
  int             code;

  if (!m_transceiver->SendCommand("STAT disk", code, lines))
  {
    pthread_mutex_unlock(&m_critSection);
    return PVR_ERROR_SERVER_ERROR;
  }

  vector<string>::iterator it = lines.begin();
  string& data(*it);
  size_t found = data.find("MB");

  if (found != CStdString::npos)
  {
    *total = atol(data.c_str()) * 1024;
    data.erase(0, found + 3);
    *used = atol(data.c_str()) * 1024;
  }

  pthread_mutex_unlock(&m_critSection);

  return PVR_ERROR_NO_ERROR;
}


/************************************************************/
/** EPG handling */

PVR_ERROR PVRClientVDR::GetEPGForChannel(unsigned int number, EPG_DATA &epg, time_t start, time_t end)
{

  vector<string> lines;
  int            code;
  char           buffer[1024];
  int            found;
  TVEPGData      broadcast;

  if (!m_transceiver->IsOpen())
    return PVR_ERROR_SERVER_ERROR;

  pthread_mutex_lock(&m_critSection);

  if (start != 0)
    sprintf(buffer, "LSTE %d from %lu to %lu", number, (long)start, (long)end);
  else
    sprintf(buffer, "LSTE %d", number);
  while (!m_transceiver->SendCommand(buffer, code, lines))
  {
    if (code != 451)
    {
      pthread_mutex_unlock(&m_critSection);
      return PVR_ERROR_SERVER_ERROR;
    }
    Sleep(750);
  }

  for (vector<string>::iterator it = lines.begin(); it != lines.end(); it++)
  {
    string& data(*it);
    CStdString str_result = data;

    if (m_bCharsetConv)
      XBMC_unknown_to_utf8(str_result);

    /** Get Channelname **/
    found = str_result.find("C", 0);
    if (found == 0)
    {
      str_result.erase(0, 2);
      found = str_result.find(" ", 0);
      str_result.erase(0, found + 1);
//    = str_result.c_str();
      continue;
    }

    /** Get Title **/
    found = str_result.find("T", 0);
    if (found == 0)
    {
      str_result.erase(0, 2);
      broadcast.m_strTitle = str_result.c_str();
      continue;
    }

    /** Get short description **/
    found = str_result.find("S", 0);
    if (found == 0)
    {
      str_result.erase(0, 2);
      broadcast.m_strPlotOutline = str_result.c_str();
      continue;
    }

    /** Get description **/
    found = str_result.find("D", 0);
    if (found == 0)
    {
      str_result.erase(0, 2);

      int pos = 0;

      while (1)
      {
        pos = str_result.find("|", pos);

        if (pos < 0)
          break;

        str_result.replace(pos, 1, 1, '\n');
      }
      broadcast.m_strPlot = str_result.c_str();
      continue;
    }

    /** Get Genre **/
    found = str_result.find("G", 0);
    if (found == 0)
    {
      str_result.erase(0, 2);
      broadcast.m_GenreType = atol(str_result.c_str());
      found = str_result.find(" ", 0);
      str_result.erase(0, found + 1);
      broadcast.m_GenreSubType = atol(str_result.c_str());
      found = str_result.find(" ", 0);
      str_result.erase(0, found + 1);
      broadcast.m_strGenre = str_result.c_str();
      continue;
    }

    /** Get ID, date and length**/
    found = str_result.find("E ", 0);
    if (found == 0)
    {
      time_t rec_time;
      int duration;
      str_result.erase(0, 2);
//    = atol(str_result.c_str());

      found = str_result.find(" ", 0);
      str_result.erase(0, found + 1);

      rec_time = atol(str_result.c_str());
      found = str_result.find(" ", 0);
      str_result.erase(0, found + 1);
      duration = atol(str_result.c_str());

      broadcast.m_startTime = CDateTime((time_t)rec_time);
      broadcast.m_endTime = CDateTime((time_t)rec_time + duration);
      broadcast.m_duration = CDateTimeSpan(0, 0, duration / 60, duration % 60);
      continue;
    }

    /** end tag **/
    found = str_result.find("e", 0);
    if (found == 0)
    {
      epg.push_back(broadcast);

      broadcast.m_strTitle = "";
      broadcast.m_strPlotOutline = "";
      broadcast.m_strPlot = "";
      broadcast.m_startTime = NULL;
      broadcast.m_endTime = NULL;
      broadcast.m_strGenre = "";
      broadcast.m_GenreType = NULL;
      broadcast.m_GenreSubType = NULL;
    }
  }

  pthread_mutex_unlock(&m_critSection);

  return PVR_ERROR_NO_ERROR;
}

PVR_ERROR PVRClientVDR::GetEPGNowInfo(unsigned int number, CTVEPGInfoTag *result)
{

  vector<string> lines;
  int            code;
  char           buffer[1024];
  int            found;

  if (!m_transceiver->IsOpen())
    return PVR_ERROR_SERVER_ERROR;

  pthread_mutex_lock(&m_critSection);

  sprintf(buffer, "LSTE %d NOW", number);
  while (!m_transceiver->SendCommand(buffer, code, lines))
  {
    if (code != 451)
    {
      pthread_mutex_unlock(&m_critSection);
      return PVR_ERROR_SERVER_ERROR;
    }
    Sleep(750);
  }

  for (vector<string>::iterator it = lines.begin(); it != lines.end(); it++)
  {
    string& data(*it);
    CStdString str_result = data;

    if (m_bCharsetConv)
      XBMC_unknown_to_utf8(str_result);

    /** Get Channelname **/
    found = str_result.find("C", 0);
    if (found == 0)
    {
      str_result.erase(0, 2);
      found = str_result.find(" ", 0);
      str_result.erase(0, found + 1);
      result->m_strChannel = str_result.c_str();
      continue;
    }

    /** Get Title **/
    found = str_result.find("T", 0);
    if (found == 0)
    {
      str_result.erase(0, 2);
      result->m_strTitle = str_result.c_str();
      continue;
    }

    /** Get short description **/
    found = str_result.find("S", 0);
    if (found == 0)
    {
      str_result.erase(0, 2);
      result->m_strPlotOutline = str_result.c_str();
      continue;
    }

    /** Get description **/
    found = str_result.find("D", 0);
    if (found == 0)
    {
      str_result.erase(0, 2);
      int pos = 0;

      while (1)
      {
        pos = str_result.find("|", pos);

        if (pos < 0)
          break;

        str_result.replace(pos, 1, 1, '\n');
      }

      result->m_strPlot = str_result.c_str();
      continue;
    }

    /** Get Genre **/
    found = str_result.find("G", 0);
    if (found == 0)
    {
      str_result.erase(0, 2);
      result->m_GenreType = atol(str_result.c_str());
      found = str_result.find(" ", 0);
      str_result.erase(0, found + 1);
      result->m_GenreSubType = atol(str_result.c_str());
      found = str_result.find(" ", 0);
      str_result.erase(0, found + 1);
      result->m_strGenre = str_result.c_str();
      continue;
    }

    /** Get ID, date and length**/
    found = str_result.find("E ", 0);
    if (found == 0)
    {
      time_t rec_time;
      int duration;
      str_result.erase(0, 2);
//                broadcast.m_bouquetNum = atol(str_result.c_str());

      found = str_result.find(" ", 0);
      str_result.erase(0, found + 1);

      rec_time = atol(str_result.c_str());
      found = str_result.find(" ", 0);
      str_result.erase(0, found + 1);
      duration = atol(str_result.c_str());

      result->m_startTime = CDateTime((time_t)rec_time);
      result->m_endTime = CDateTime((time_t)rec_time + duration);
      result->m_duration = CDateTimeSpan(0, 0, duration / 60, duration % 60);
      continue;
    }
  }

  pthread_mutex_unlock(&m_critSection);

  return PVR_ERROR_NO_ERROR;
}

PVR_ERROR PVRClientVDR::GetEPGNextInfo(unsigned int number, CTVEPGInfoTag *result)
{
  vector<string> lines;
  int            code;
  char           buffer[1024];
  int            found;

  if (!m_transceiver->IsOpen())
    return PVR_ERROR_SERVER_ERROR;

  pthread_mutex_lock(&m_critSection);

  sprintf(buffer, "LSTE %d NEXT", number);
  while (!m_transceiver->SendCommand(buffer, code, lines))
  {
    if (code != 451)
    {
      pthread_mutex_unlock(&m_critSection);
      return PVR_ERROR_SERVER_ERROR;
    }
    Sleep(750);
  }

  for (vector<string>::iterator it = lines.begin(); it != lines.end(); it++)
  {
    string& data(*it);
    CStdString str_result = data;

    if (m_bCharsetConv)
      XBMC_unknown_to_utf8(str_result);

    /** Get Channelname **/
    found = str_result.find("C", 0);
    if (found == 0)
    {
      str_result.erase(0, 2);
      found = str_result.find(" ", 0);
      str_result.erase(0, found + 1);
      result->m_strChannel = str_result.c_str();
      continue;
    }

    /** Get Title **/
    found = str_result.find("T", 0);
    if (found == 0)
    {
      str_result.erase(0, 2);
      result->m_strTitle = str_result.c_str();
      continue;
    }

    /** Get short description **/
    found = str_result.find("S", 0);
    if (found == 0)
    {
      str_result.erase(0, 2);
      result->m_strPlotOutline = str_result.c_str();
      continue;
    }

    /** Get description **/
    found = str_result.find("D", 0);
    if (found == 0)
    {
      str_result.erase(0, 2);

      int pos = 0;

      while (1)
      {
        pos = str_result.find("|", pos);

        if (pos < 0)
          break;

        str_result.replace(pos, 1, 1, '\n');
      }

      result->m_strPlot = str_result.c_str();
      continue;
    }

    /** Get Genre **/
    found = str_result.find("G", 0);
    if (found == 0)
    {
      str_result.erase(0, 2);
      result->m_GenreType = atol(str_result.c_str());
      found = str_result.find(" ", 0);
      str_result.erase(0, found + 1);
      result->m_GenreSubType = atol(str_result.c_str());
      found = str_result.find(" ", 0);
      str_result.erase(0, found + 1);
      result->m_strGenre = str_result.c_str();
      continue;
    }

    /** Get ID, date and length**/
    found = str_result.find("E ", 0);
    if (found == 0)
    {
      time_t rec_time;
      int duration;
      str_result.erase(0, 2);
//                broadcast.m_bouquetNum = atol(str_result.c_str());

      found = str_result.find(" ", 0);
      str_result.erase(0, found + 1);

      rec_time = atol(str_result.c_str());
      found = str_result.find(" ", 0);
      str_result.erase(0, found + 1);
      duration = atol(str_result.c_str());

      result->m_startTime = CDateTime((time_t)rec_time);
      result->m_endTime = CDateTime((time_t)rec_time + duration);
      result->m_duration = CDateTimeSpan(0, 0, duration / 60, duration % 60);
      continue;
    }
  }

  pthread_mutex_unlock(&m_critSection);

  return PVR_ERROR_NO_ERROR;
}


/************************************************************/
/** Channel handling */

int PVRClientVDR::GetNumChannels()
{
  vector<string>  lines;
  int             code;

  if (!m_transceiver->IsOpen())
    return -1;

  pthread_mutex_lock(&m_critSection);

  if (!m_transceiver->SendCommand("STAT channels", code, lines))
  {
    pthread_mutex_unlock(&m_critSection);
    return -1;
  }

  vector<string>::iterator it = lines.begin();

  string& data(*it);
  pthread_mutex_unlock(&m_critSection);
  return atol(data.c_str());
}

PVR_ERROR PVRClientVDR::GetChannelList(VECCHANNELS *channels, bool radio)
{
  vector<string> lines;
  int            code;
  unsigned int   number = 1;

  if (!m_transceiver->IsOpen())
    return PVR_ERROR_SERVER_ERROR;

  pthread_mutex_lock(&m_critSection);

  channels->erase(channels->begin(), channels->end());

  while (!m_transceiver->SendCommand("LSTC", code, lines))
  {
    if (code != 451)
    {
      pthread_mutex_unlock(&m_critSection);
      return PVR_ERROR_SERVER_ERROR;
    }
    Sleep(750);
  }

  for (vector<string>::iterator it = lines.begin(); it < lines.end(); it++)
  {
    string& data(*it);
    CStdString str_result = data;
    int found;

    CTVChannelInfoTag broadcast;
    int idVPID = 0;
    int idAPID1 = 0;
    int idAPID2 = 0;
    int idDPID1 = 0;
    int idDPID2 = 0;
    int idCAID = 0;
    int idTPID = 0;
    CStdString name;
    int id;

    if (m_bCharsetConv)
      XBMC_unknown_to_utf8(str_result);

    // Channel number
    broadcast.m_iClientNum = atol(str_result.c_str());
    str_result.erase(0, str_result.find(" ", 0) + 1);

    // Channel and provider name
    found = str_result.find(":", 0);
    name.assign(str_result, found);
    str_result.erase(0, found + 1);
    found = name.find(";", 0);

    if (found == -1)
    {
      broadcast.m_strChannel = name;
    }
    else
    {
      broadcast.m_strChannel.assign(name, found);
      name.erase(0, found + 1);
    }

    // Channel frequency
    str_result.erase(0, str_result.find(":", 0) + 1);

    // Source descriptor
    str_result.erase(0, str_result.find(":", 0));

    // Source Type
    if (str_result.compare(0, 2, ":C") == 0)
    {
      str_result.erase(0, 3);
    }
    else if (str_result.compare(0, 2, ":T") == 0)
    {
      str_result.erase(0, 3);
    }
    else if (str_result.compare(0, 2, ":S") == 0)
    {
      str_result.erase(0, 2);
      found = str_result.find(":", 0);
      str_result.erase(0, found + 1);
    }
    else if (str_result.compare(0, 2, ":P") == 0)
    {
      str_result.erase(0, 3);
    }


    // Channel symbolrate
    found = str_result.find(":", 0);
    str_result.erase(0, found + 1);

    // Channel program id
    idVPID = atol(str_result.c_str());
    found = str_result.find(":", 0);
    str_result.erase(0, found + 1);

    // Channel audio id's
    found = str_result.find(":", 0);
    name.assign(str_result, found);
    str_result.erase(0, found + 1);
    found = name.find(";", 0);

    if (found == -1)
    {
      id = atol(name.c_str());

      if (id == 0)
      {
        idAPID1 = 0;
        idAPID2 = 0;
        idDPID1 = 0;
        idDPID2 = 0;
      }
      else
      {
        idAPID1 = id;
        found = name.find(",", 0);

        if (found == -1)
        {
          idAPID2 = 0;
        }
        else
        {
          name.erase(0, found + 1);
          idAPID2 = atol(name.c_str());
        }

        idDPID1 = 0;
        idDPID2 = 0;
      }
    }
    else
    {
      int id;
      id = atol(name.c_str());

      if (id == 0)
      {
        idAPID1 = 0;
        idAPID2 = 0;
      }
      else
      {
        idAPID1 = id;
        found = name.find(",", 0);

        if (found == -1)
        {
          idAPID2 = 0;
        }
        else
        {
          name.erase(0, found + 1);
          idAPID2 = atol(name.c_str());
        }
      }

      name.erase(0, name.find(";", 0) + 1);
      id = atoi(name.c_str());
      if (id == 0)
      {
        idDPID1 = 0;
        idDPID2 = 0;
      }
      else
      {
        idDPID1 = id;
        found = name.find(",", 0);

        if (found == -1)
        {
          idDPID2 = 0;
        }
        else
        {
          name.erase(0, found + 1);
          idDPID2 = atol(name.c_str());
        }
      }
    }

    // Teletext id
    idTPID = atoi(str_result.c_str());
    str_result.erase(0, str_result.find(":", 0) + 1);
    broadcast.m_bTeletext = idTPID ? true : false;

    // CAID id
    idCAID = atoi(str_result.c_str());
    str_result.erase(0, str_result.find(":", 0) + 1);

    if (idCAID && m_bOnlyFTA)
      continue;

    broadcast.m_encrypted = idCAID ? true : false;

    if ((idVPID == 0) && (idAPID1 != 0))
    {
      broadcast.m_radio = true;
      broadcast.m_strFileNameAndPath.Format("radio://%i", number);
    }
    else
    {
      broadcast.m_radio = false;
      broadcast.m_strFileNameAndPath.Format("tv://%i", number);
    }

    if (radio == broadcast.m_radio)
    {
      broadcast.m_iChannelNum = number;
      channels->push_back(broadcast);
      number++;
    }
  }

  pthread_mutex_unlock(&m_critSection);
  return PVR_ERROR_NO_ERROR;
}

PVR_ERROR PVRClientVDR::GetChannelSettings(CTVChannelInfoTag *result)
{

  vector<string> lines;
  int            code;
  char           buffer[1024];

  if (result->m_iClientNum < 1)
    return PVR_ERROR_SERVER_ERROR;

  if (!m_transceiver->IsOpen())
    return PVR_ERROR_SERVER_ERROR;

  pthread_mutex_lock(&m_critSection);

  sprintf(buffer, "LSTC %d", result->m_iClientNum);
  while (!m_transceiver->SendCommand(buffer, code, lines))
  {
    if (code != 451)
    {
      pthread_mutex_unlock(&m_critSection);
      return PVR_ERROR_SERVER_ERROR;
    }
    Sleep(750);
  }

  for (vector<string>::iterator it = lines.begin(); it != lines.end(); it++)
  {
    string& data(*it);
    CStdString str_result = data;
    int found;
    int i_tmp;

    result->m_Settings.m_VPID = 0;
    result->m_Settings.m_APID1 = 0;
    result->m_Settings.m_APID2 = 0;
    result->m_Settings.m_DPID1 = 0;
    result->m_Settings.m_DPID2 = 0;
    result->m_Settings.m_CAID = 0;
    CStdString name;
    int id;

    if (m_bCharsetConv)
      XBMC_unknown_to_utf8(str_result);

    // Skip Channel number
    str_result.erase(0, str_result.find(" ", 0) + 1);

    // Channel and provider name
    found = str_result.find(":", 0);
    name.assign(str_result, found);
    str_result.erase(0, found + 1);
    found = name.find(";", 0);

    if (found != -1)
    {
      name.erase(0, found + 1);
      result->m_Settings.m_strProvider = name;
    }

    // Channel frequency
    result->m_Settings.m_Freq = atol(str_result.c_str());
    found = str_result.find(":", 0);
    str_result.erase(0, found + 1);
    found = str_result.find(":", 0);
    result->m_Settings.m_parameter.assign(str_result, found);
    str_result.erase(0, found);

    // Source Type
    if (str_result.compare(0, 2, ":C") == 0)
    {
      result->m_Settings.m_SourceType = src_DVBC;
      result->m_Settings.m_satellite = "Cable";
      str_result.erase(0, 3);
    }
    else if (str_result.compare(0, 2, ":T") == 0)
    {
      result->m_Settings.m_SourceType = src_DVBT;
      result->m_Settings.m_satellite = "Terrestrial";
      str_result.erase(0, 3);
    }
    else if (str_result.compare(0, 2, ":S") == 0)
    {
      result->m_Settings.m_SourceType = src_DVBS;
      str_result.erase(0, 2);
      found = str_result.find(":", 0);
      result->m_Settings.m_satellite.assign(str_result, found);
      str_result.erase(0, found + 1);
    }
    else if (str_result.compare(0, 2, ":P") == 0)
    {
      result->m_Settings.m_SourceType = srcAnalog;
      result->m_Settings.m_satellite = "Analog";
      str_result.erase(0, 3);
    }

    // Channel symbolrate
    result->m_Settings.m_Symbolrate = atol(str_result.c_str());
    str_result.erase(0, str_result.find(":", 0) + 1);

    // Channel program id
    result->m_Settings.m_VPID = atol(str_result.c_str());
    str_result.erase(0, str_result.find(":", 0) + 1);

    // Channel audio id's
    found = str_result.find(":", 0);
    name.assign(str_result, found);
    str_result.erase(0, found + 1);
    found = name.find(";", 0);

    if (found == -1)
    {
      id = atol(name.c_str());

      if (id == 0)
      {
        result->m_Settings.m_APID1 = 0;
        result->m_Settings.m_APID2 = 0;
        result->m_Settings.m_DPID1 = 0;
        result->m_Settings.m_DPID2 = 0;
      }
      else
      {
        result->m_Settings.m_APID1 = id;
        found = name.find(",", 0);

        if (found == -1)
        {
          result->m_Settings.m_APID2 = 0;
        }
        else
        {
          name.erase(0, found + 1);
          result->m_Settings.m_APID2 = atol(name.c_str());
        }
        result->m_Settings.m_DPID1 = 0;
        result->m_Settings.m_DPID2 = 0;
      }
    }
    else
    {
      int id;

      id = atol(name.c_str());

      if (id == 0)
      {
        result->m_Settings.m_APID1 = 0;
        result->m_Settings.m_APID2 = 0;
      }
      else
      {
        result->m_Settings.m_APID1 = id;
        found = name.find(",", 0);

        if (found == -1)
        {
          result->m_Settings.m_APID2 = 0;
        }
        else
        {
          name.erase(0, found + 1);
          result->m_Settings.m_APID2 = atol(name.c_str());
        }
      }

      found = name.find(";", 0);

      name.erase(0, found + 1);
      id = atol(name.c_str());

      if (id == 0)
      {
        result->m_Settings.m_DPID1 = 0;
        result->m_Settings.m_DPID2 = 0;
      }
      else
      {
        result->m_Settings.m_DPID1 = id;
        found = name.find(",", 0);

        if (found == -1)
        {
          result->m_Settings.m_DPID2 = 0;
        }
        else
        {
          name.erase(0, found + 1);
          result->m_Settings.m_DPID2 = atol(name.c_str());
        }
      }
    }

    // Teletext id
    result->m_Settings.m_TPID = atol(str_result.c_str());
    found = str_result.find(":", 0);
    str_result.erase(0, found + 1);

    // CAID id
    result->m_Settings.m_CAID = atol(str_result.c_str());
    found = str_result.find(":", 0);
    str_result.erase(0, found + 1);

    // Service id
    result->m_Settings.m_SID = atol(str_result.c_str());
    found = str_result.find(":", 0);
    str_result.erase(0, found + 1);

    // Network id
    result->m_Settings.m_NID = atol(str_result.c_str());
    found = str_result.find(":", 0);
    str_result.erase(0, found + 1);

    // Transport id
    result->m_Settings.m_TID = atol(str_result.c_str());
    found = str_result.find(":", 0);
    str_result.erase(0, found + 1);

    // Radio id
    result->m_Settings.m_RID = atol(str_result.c_str());
    found = str_result.find(":", 0);
    str_result.erase(0, found + 1);

    // DVB-S2 ?
    if (result->m_Settings.m_SourceType == src_DVBS)
    {
      str_result = result->m_Settings.m_parameter;
      found = str_result.find("S", 0);

      if (found != -1)
      {
        str_result.erase(0, found + 1);
        i_tmp = atol(str_result.c_str());

        if (i_tmp == 1)
        {
          result->m_Settings.m_SourceType = src_DVBS2;
        }
      }
    }

    // Inversion
    str_result = result->m_Settings.m_parameter;
    found = str_result.find("I", 0);
    if (found != -1)
    {
      str_result.erase(0, found + 1);
      i_tmp = atol(str_result.c_str());

      if (i_tmp == 0)
      {
        result->m_Settings.m_Inversion = InvOff;
      }
      else if (i_tmp == 1)
      {
        result->m_Settings.m_Inversion = InvOn;
      }
      else if (i_tmp == 999)
      {
        result->m_Settings.m_Inversion = InvAuto;
      }
    }
    else
    {
      result->m_Settings.m_Inversion = InvAuto;
    }

    // CoderateL
    if (result->m_Settings.m_SourceType == src_DVBT)
    {
      str_result = result->m_Settings.m_parameter;
      found = str_result.find("D", 0);

      if (found != -1)
      {
        str_result.erase(0, found + 1);
        i_tmp = atol(str_result.c_str());

        if (i_tmp == 0)
        {
          result->m_Settings.m_CoderateL = Coderate_None;
        }
        else if (i_tmp == 12)
        {
          result->m_Settings.m_CoderateL = Coderate_1_2;
        }
        else if (i_tmp == 23)
        {
          result->m_Settings.m_CoderateL = Coderate_2_3;
        }
        else if (i_tmp == 34)
        {
          result->m_Settings.m_CoderateL = Coderate_3_4;
        }
        else if (i_tmp == 45)
        {
          result->m_Settings.m_CoderateL = Coderate_4_5;
        }
        else if (i_tmp == 56)
        {
          result->m_Settings.m_CoderateL = Coderate_5_6;
        }
        else if (i_tmp == 67)
        {
          result->m_Settings.m_CoderateL = Coderate_6_7;
        }
        else if (i_tmp == 78)
        {
          result->m_Settings.m_CoderateL = Coderate_7_8;
        }
        else if (i_tmp == 89)
        {
          result->m_Settings.m_CoderateL = Coderate_8_9;
        }
        else if (i_tmp == 910)
        {
          result->m_Settings.m_CoderateL = Coderate_9_10;
        }
        else if (i_tmp == 999 || i_tmp == 910)
        {
          result->m_Settings.m_CoderateL = Coderate_Auto;
        }
      }
      else
      {
        result->m_Settings.m_CoderateL = Coderate_None;
      }
    }
    else
    {
      result->m_Settings.m_CoderateL = Coderate_None;
    }

    // CoderateH
    str_result = result->m_Settings.m_parameter;

    found = str_result.find("C", 0);

    if (found != -1)
    {
      str_result.erase(0, found + 1);
      i_tmp = atol(str_result.c_str());

      if (i_tmp == 0)
      {
        result->m_Settings.m_CoderateH = Coderate_None;
      }
      else if (i_tmp == 12)
      {
        result->m_Settings.m_CoderateH = Coderate_1_2;
      }
      else if (i_tmp == 23)
      {
        result->m_Settings.m_CoderateH = Coderate_2_3;
      }
      else if (i_tmp == 34)
      {
        result->m_Settings.m_CoderateH = Coderate_3_4;
      }
      else if (i_tmp == 45)
      {
        result->m_Settings.m_CoderateH = Coderate_4_5;
      }
      else if (i_tmp == 56)
      {
        result->m_Settings.m_CoderateH = Coderate_5_6;
      }
      else if (i_tmp == 67)
      {
        result->m_Settings.m_CoderateH = Coderate_6_7;
      }
      else if (i_tmp == 78)
      {
        result->m_Settings.m_CoderateH = Coderate_7_8;
      }
      else if (i_tmp == 89)
      {
        result->m_Settings.m_CoderateH = Coderate_8_9;
      }
      else if (i_tmp == 910)
      {
        result->m_Settings.m_CoderateL = Coderate_9_10;
      }
      else if (i_tmp == 999 || i_tmp == 910)
      {
        result->m_Settings.m_CoderateH = Coderate_Auto;
      }
    }
    else
    {
      result->m_Settings.m_CoderateH = Coderate_None;
    }

    // Modulation
    str_result = result->m_Settings.m_parameter;

    found = str_result.find("M", 0);

    if (found != -1)
    {
      str_result.erase(0, found + 1);
      i_tmp = atol(str_result.c_str());

      if (i_tmp == 0)
      {
        result->m_Settings.m_Modulation = modNone;
      }
      else if (i_tmp == 4)
      {
        result->m_Settings.m_Modulation = modQAM4;
      }
      else if (i_tmp == 16)
      {
        result->m_Settings.m_Modulation = modQAM16;
      }
      else if (i_tmp == 32)
      {
        result->m_Settings.m_Modulation = modQAM32;
      }
      else if (i_tmp == 64)
      {
        result->m_Settings.m_Modulation = modQAM64;
      }
      else if (i_tmp == 128)
      {
        result->m_Settings.m_Modulation = modQAM128;
      }
      else if (i_tmp == 256)
      {
        result->m_Settings.m_Modulation = modQAM256;
      }
      else if (i_tmp == 512)
      {
        result->m_Settings.m_Modulation = modQAM512;
      }
      else if (i_tmp == 1024)
      {
        result->m_Settings.m_Modulation = modQAM1024;
      }
      else if (i_tmp == 998)
      {
        result->m_Settings.m_Modulation = modQAMAuto;
      }
      else if (i_tmp == 1)
      {
        result->m_Settings.m_Modulation = modBPSK;
      }
      else if (i_tmp == 2)
      {
        result->m_Settings.m_Modulation = modQPSK;
      }
      else if (i_tmp == 3)
      {
        result->m_Settings.m_Modulation = modOQPSK;
      }
      else if (i_tmp == 5)
      {
        result->m_Settings.m_Modulation = mod8PSK;
      }
      else if (i_tmp == 6)
      {
        result->m_Settings.m_Modulation = mod16APSK;
      }
      else if (i_tmp == 7)
      {
        result->m_Settings.m_Modulation = mod32APSK;
      }
      else if (i_tmp == 8)
      {
        result->m_Settings.m_Modulation = modOFDM;
      }
      else if (i_tmp == 9)
      {
        result->m_Settings.m_Modulation = modCOFDM;
      }
      else if (i_tmp == 10)
      {
        result->m_Settings.m_Modulation = modVSB8;
      }
      else if (i_tmp == 11)
      {
        result->m_Settings.m_Modulation = modVSB16;
      }
    }
    else
    {
      result->m_Settings.m_Modulation = modNone;
    }

    // Bandwith
    str_result = result->m_Settings.m_parameter;

    found = str_result.find("B", 0);

    if (found != -1)
    {
      str_result.erase(0, found + 1);
      i_tmp = atol(str_result.c_str());

      if (i_tmp == 5)
      {
        result->m_Settings.m_Bandwidth = bw_5MHz;
      }
      else if (i_tmp == 6)
      {
        result->m_Settings.m_Bandwidth = bw_6MHz;
      }
      else if (i_tmp == 7)
      {
        result->m_Settings.m_Bandwidth = bw_7MHz;
      }
      else if (i_tmp == 8)
      {
        result->m_Settings.m_Bandwidth = bw_8MHz;
      }
      else if (i_tmp == 999)
      {
        result->m_Settings.m_Bandwidth = bw_Auto;
      }
    }
    else
    {
      result->m_Settings.m_Bandwidth = bw_Auto;
    }

    // Hierarchie
    str_result = result->m_Settings.m_parameter;

    found = str_result.find("Y", 0);

    if (found != -1)
    {
      str_result.erase(0, found + 1);
      i_tmp = atol(str_result.c_str());

      if (i_tmp == 0)
      {
        result->m_Settings.m_Hierarchie = false;
      }
      else if (i_tmp == 1)
      {
        result->m_Settings.m_Hierarchie = true;
      }
    }
    else
    {
      result->m_Settings.m_Hierarchie = false;
    }

    // Alpha
    str_result = result->m_Settings.m_parameter;

    found = str_result.find("A", 0);

    if (found != -1)
    {
      str_result.erase(0, found + 1);
      i_tmp = atol(str_result.c_str());

      if (i_tmp == 0)
      {
        result->m_Settings.m_Alpha = alpha_0;
      }
      else if (i_tmp == 1)
      {
        result->m_Settings.m_Alpha = alpha_1;
      }
      else if (i_tmp == 2)
      {
        result->m_Settings.m_Alpha = alpha_2;
      }
      else if (i_tmp == 4)
      {
        result->m_Settings.m_Alpha = alpha_4;
      }
    }
    else
    {
      result->m_Settings.m_Alpha = alpha_0;
    }

    // Guard
    str_result = result->m_Settings.m_parameter;

    found = str_result.find("G", 0);

    if (found != -1)
    {
      str_result.erase(0, found + 1);
      i_tmp = atol(str_result.c_str());

      if (i_tmp == 4)
      {
        result->m_Settings.m_Guard = guard_1_4;
      }
      else if (i_tmp == 8)
      {
        result->m_Settings.m_Guard = guard_1_8;
      }
      else if (i_tmp == 16)
      {
        result->m_Settings.m_Guard = guard_1_16;
      }
      else if (i_tmp == 32)
      {
        result->m_Settings.m_Guard = guard_1_32;
      }
      else if (i_tmp == 999)
      {
        result->m_Settings.m_Guard = guard_Auto;
      }
    }
    else
    {
      result->m_Settings.m_Guard = guard_Auto;
    }

    // Transmission
    str_result = result->m_Settings.m_parameter;

    found = str_result.find("T", 0);

    if (found != -1)
    {
      str_result.erase(0, found + 1);
      i_tmp = atol(str_result.c_str());

      if (i_tmp == 2)
      {
        result->m_Settings.m_Transmission = transmission_2K;
      }
      else if (i_tmp == 4)
      {
        result->m_Settings.m_Transmission = transmission_4K;
      }
      else if (i_tmp == 8)
      {
        result->m_Settings.m_Transmission = transmission_8K;
      }
      else if (i_tmp == 999)
      {
        result->m_Settings.m_Transmission = transmission_Auto;
      }
    }
    else
    {
      result->m_Settings.m_Transmission = transmission_Auto;
    }

    // Priority
    str_result = result->m_Settings.m_parameter;

    found = str_result.find("P", 0);

    if (found != -1)
    {
      str_result.erase(0, found + 1);
      i_tmp = atol(str_result.c_str());

      if (i_tmp == 0)
      {
        result->m_Settings.m_Priority = false;
      }
      else if (i_tmp == 1)
      {
        result->m_Settings.m_Priority = true;
      }
    }
    else
    {
      result->m_Settings.m_Priority = false;
    }

    // Rolloff
    str_result = result->m_Settings.m_parameter;

    found = str_result.find("O", 0);

    if (found != -1)
    {
      str_result.erase(0, found + 1);
      i_tmp = atol(str_result.c_str());

      if (i_tmp == 0)
      {
        result->m_Settings.m_Rolloff = rolloff_Unknown;
      }
      else if (i_tmp == 20)
      {
        result->m_Settings.m_Rolloff = rolloff_20;
      }
      else if (i_tmp == 25)
      {
        result->m_Settings.m_Rolloff = rolloff_25;
      }
      else if (i_tmp == 35)
      {
        result->m_Settings.m_Rolloff = rolloff_35;
      }
    }
    else
    {
      result->m_Settings.m_Rolloff = rolloff_Unknown;
    }

    // Polarization
    str_result = result->m_Settings.m_parameter;

    if ((int) str_result.find("H", 0) != -1)
      result->m_Settings.m_Polarization = pol_H;

    if ((int) str_result.find("h", 0) != -1)
      result->m_Settings.m_Polarization = pol_H;

    if ((int) str_result.find("V", 0) != -1)
      result->m_Settings.m_Polarization = pol_V;

    if ((int) str_result.find("v", 0) != -1)
      result->m_Settings.m_Polarization = pol_V;

    if ((int) str_result.find("L", 0) != -1)
      result->m_Settings.m_Polarization = pol_L;

    if ((int) str_result.find("l", 0) != -1)
      result->m_Settings.m_Polarization = pol_L;

    if ((int) str_result.find("R", 0) != -1)
      result->m_Settings.m_Polarization = pol_R;

    if ((int) str_result.find("r", 0) != -1)
      result->m_Settings.m_Polarization = pol_R;
  }

  pthread_mutex_unlock(&m_critSection);

  return PVR_ERROR_NO_ERROR;
}

PVR_ERROR PVRClientVDR::UpdateChannelSettings(const CTVChannelInfoTag &chaninfo)
{
  CStdString     m_Summary;
  CStdString     m_Summary_2;
  vector<string> lines;
  int            code;
  char           buffer[1024];

  pthread_mutex_lock(&m_critSection);

  if (chaninfo.m_iClientNum == -1)
  {
    pthread_mutex_unlock(&m_critSection);
    return PVR_ERROR_NOT_SAVED;
  }

  m_Summary.Format("%d %s;%s:%i:" , chaninfo.m_iClientNum

                   , chaninfo.m_strChannel.c_str()
                   , chaninfo.m_Settings.m_strProvider.c_str()
                   , chaninfo.m_Settings.m_Freq);

  if ((chaninfo.m_Settings.m_SourceType == src_DVBS) ||
      (chaninfo.m_Settings.m_SourceType == src_DVBS2))
  {
    if      (chaninfo.m_Settings.m_Polarization == pol_H)
      m_Summary += "h";
    else if (chaninfo.m_Settings.m_Polarization == pol_V)
      m_Summary += "v";
    else if (chaninfo.m_Settings.m_Polarization == pol_L)
      m_Summary += "l";
    else if (chaninfo.m_Settings.m_Polarization == pol_R)
      m_Summary += "r";
  }

  if (chaninfo.m_Settings.m_SourceType == src_DVBT)
  {
    if      (chaninfo.m_Settings.m_Inversion == InvOff)
      m_Summary += "I0";
    else if (chaninfo.m_Settings.m_Inversion == InvOn)
      m_Summary += "I1";
    else if (chaninfo.m_Settings.m_Inversion == InvAuto)
      m_Summary += "I999";
  }

  if (chaninfo.m_Settings.m_SourceType == src_DVBT)
  {
    if      (chaninfo.m_Settings.m_Bandwidth == bw_5MHz)
      m_Summary += "B5";
    else if (chaninfo.m_Settings.m_Bandwidth == bw_6MHz)
      m_Summary += "B6";
    else if (chaninfo.m_Settings.m_Bandwidth == bw_7MHz)
      m_Summary += "B7";
    else if (chaninfo.m_Settings.m_Bandwidth == bw_8MHz)
      m_Summary += "B8";
    else if (chaninfo.m_Settings.m_Bandwidth == bw_Auto)
      m_Summary += "B999";
  }

  if      (chaninfo.m_Settings.m_CoderateH == Coderate_None)
    m_Summary += "C0";
  else if (chaninfo.m_Settings.m_CoderateH == Coderate_1_2)
    m_Summary += "C12";
  else if (chaninfo.m_Settings.m_CoderateH == Coderate_2_3)
    m_Summary += "C23";
  else if (chaninfo.m_Settings.m_CoderateH == Coderate_3_4)
    m_Summary += "C34";
  else if (chaninfo.m_Settings.m_CoderateH == Coderate_4_5)
    m_Summary += "C45";
  else if (chaninfo.m_Settings.m_CoderateH == Coderate_5_6)
    m_Summary += "C56";
  else if (chaninfo.m_Settings.m_CoderateH == Coderate_6_7)
    m_Summary += "C67";
  else if (chaninfo.m_Settings.m_CoderateH == Coderate_7_8)
    m_Summary += "C78";
  else if (chaninfo.m_Settings.m_CoderateH == Coderate_8_9)
    m_Summary += "C89";
  else if (chaninfo.m_Settings.m_CoderateH == Coderate_9_10)
    m_Summary += "C910";
  else if (chaninfo.m_Settings.m_CoderateH == Coderate_Auto)
    m_Summary += "C999";

  if (chaninfo.m_Settings.m_SourceType == src_DVBT)
  {
    if      (chaninfo.m_Settings.m_CoderateL == Coderate_None)
      m_Summary += "D0";
    else if (chaninfo.m_Settings.m_CoderateL == Coderate_1_2)
      m_Summary += "D12";
    else if (chaninfo.m_Settings.m_CoderateL == Coderate_2_3)
      m_Summary += "D23";
    else if (chaninfo.m_Settings.m_CoderateL == Coderate_3_4)
      m_Summary += "D34";
    else if (chaninfo.m_Settings.m_CoderateL == Coderate_4_5)
      m_Summary += "D45";
    else if (chaninfo.m_Settings.m_CoderateL == Coderate_5_6)
      m_Summary += "D56";
    else if (chaninfo.m_Settings.m_CoderateL == Coderate_6_7)
      m_Summary += "D67";
    else if (chaninfo.m_Settings.m_CoderateL == Coderate_7_8)
      m_Summary += "D78";
    else if (chaninfo.m_Settings.m_CoderateL == Coderate_8_9)
      m_Summary += "D89";
    else if (chaninfo.m_Settings.m_CoderateL == Coderate_9_10)
      m_Summary += "D910";
    else if (chaninfo.m_Settings.m_CoderateL == Coderate_Auto)
      m_Summary += "D999";
  }

  if      (chaninfo.m_Settings.m_Modulation == modNone)
    m_Summary += "M0";
  else if (chaninfo.m_Settings.m_Modulation == modQAM4)
    m_Summary += "M4";
  else if (chaninfo.m_Settings.m_Modulation == modQAM16)
    m_Summary += "M16";
  else if (chaninfo.m_Settings.m_Modulation == modQAM32)
    m_Summary += "M32";
  else if (chaninfo.m_Settings.m_Modulation == modQAM64)
    m_Summary += "M64";
  else if (chaninfo.m_Settings.m_Modulation == modQAM128)
    m_Summary += "M128";
  else if (chaninfo.m_Settings.m_Modulation == modQAM256)
    m_Summary += "M256";
  else if (chaninfo.m_Settings.m_Modulation == modQAM512)
    m_Summary += "M512";
  else if (chaninfo.m_Settings.m_Modulation == modQAM1024)
    m_Summary += "M1024";
  else if (chaninfo.m_Settings.m_Modulation == modQAMAuto)
    m_Summary += "M998";
  else if (chaninfo.m_Settings.m_Modulation == modBPSK)
    m_Summary += "M1";
  else if (chaninfo.m_Settings.m_Modulation == modQPSK)
    m_Summary += "M2";
  else if (chaninfo.m_Settings.m_Modulation == modOQPSK)
    m_Summary += "M3";
  else if (chaninfo.m_Settings.m_Modulation == mod8PSK)
    m_Summary += "M5";
  else if (chaninfo.m_Settings.m_Modulation == mod16APSK)
    m_Summary += "M6";
  else if (chaninfo.m_Settings.m_Modulation == mod32APSK)
    m_Summary += "M7";
  else if (chaninfo.m_Settings.m_Modulation == modOFDM)
    m_Summary += "M8";
  else if (chaninfo.m_Settings.m_Modulation == modCOFDM)
    m_Summary += "M9";
  else if (chaninfo.m_Settings.m_Modulation == modVSB8)
    m_Summary += "M10";
  else if (chaninfo.m_Settings.m_Modulation == modVSB16)
    m_Summary += "M11";

  if (chaninfo.m_Settings.m_SourceType == src_DVBT)
  {
    if      (chaninfo.m_Settings.m_Transmission == transmission_2K)
      m_Summary += "T2";
    else if (chaninfo.m_Settings.m_Transmission == transmission_4K)
      m_Summary += "T4";
    else if (chaninfo.m_Settings.m_Transmission == transmission_8K)
      m_Summary += "T8";
    else if (chaninfo.m_Settings.m_Transmission == transmission_Auto)
      m_Summary += "T999";

    if      (chaninfo.m_Settings.m_Guard == guard_1_4)
      m_Summary += "G4";
    else if (chaninfo.m_Settings.m_Guard == guard_1_8)
      m_Summary += "G8";
    else if (chaninfo.m_Settings.m_Guard == guard_1_16)
      m_Summary += "G16";
    else if (chaninfo.m_Settings.m_Guard == guard_1_32)
      m_Summary += "G32";
    else if (chaninfo.m_Settings.m_Guard == guard_Auto)
      m_Summary += "G999";

    if      (chaninfo.m_Settings.m_Hierarchie)
      m_Summary += "Y1";
    else
      m_Summary += "Y0";

    if      (chaninfo.m_Settings.m_Alpha == alpha_0)
      m_Summary += "A0";
    else if (chaninfo.m_Settings.m_Alpha == alpha_1)
      m_Summary += "A1";
    else if (chaninfo.m_Settings.m_Alpha == alpha_2)
      m_Summary += "A2";
    else if (chaninfo.m_Settings.m_Alpha == alpha_4)
      m_Summary += "A4";

    if      (chaninfo.m_Settings.m_Priority)
      m_Summary += "P1";
    else
      m_Summary += "P0";
  }

  if (chaninfo.m_Settings.m_SourceType == src_DVBS2)
  {
    if      (chaninfo.m_Settings.m_Rolloff == rolloff_Unknown)
      m_Summary += "O0";
    else if (chaninfo.m_Settings.m_Rolloff == rolloff_20)
      m_Summary += "O20";
    else if (chaninfo.m_Settings.m_Rolloff == rolloff_25)
      m_Summary += "O25";
    else if (chaninfo.m_Settings.m_Rolloff == rolloff_25)
      m_Summary += "O35";
  }

  if      (chaninfo.m_Settings.m_SourceType == src_DVBS)
    m_Summary += "O35S0:S";
  else if (chaninfo.m_Settings.m_SourceType == src_DVBS2)
    m_Summary += "S1:S";
  else if (chaninfo.m_Settings.m_SourceType == src_DVBC)
    m_Summary += ":C";
  else if (chaninfo.m_Settings.m_SourceType == src_DVBT)
    m_Summary += ":T";

  m_Summary_2.Format(":%i:%i:%i,%i;%i,%i:%i:%i:%i:%i:%i:%i" , chaninfo.m_Settings.m_Symbolrate
                     , chaninfo.m_Settings.m_VPID
                     , chaninfo.m_Settings.m_APID1
                     , chaninfo.m_Settings.m_APID2
                     , chaninfo.m_Settings.m_DPID1
                     , chaninfo.m_Settings.m_DPID2
                     , chaninfo.m_Settings.m_TPID
                     , chaninfo.m_Settings.m_CAID
                     , chaninfo.m_Settings.m_SID
                     , chaninfo.m_Settings.m_NID
                     , chaninfo.m_Settings.m_TID
                     , chaninfo.m_Settings.m_RID);

  m_Summary += m_Summary_2;

  sprintf(buffer, "LSTC %d", chaninfo.m_iClientNum);

  if (!m_transceiver->SendCommand(buffer, code, lines))
  {
    pthread_mutex_unlock(&m_critSection);
    return PVR_ERROR_SERVER_ERROR;
  }

  if (code != 250)
  {
    pthread_mutex_unlock(&m_critSection);
    return PVR_ERROR_NOT_SYNC;
  }

  sprintf(buffer, "MODC %s", m_Summary.c_str());

  if (!m_transceiver->SendCommand(buffer, code, lines))
  {
    pthread_mutex_unlock(&m_critSection);
    return PVR_ERROR_SERVER_ERROR;
  }

  if (code != 250)
  {
    pthread_mutex_unlock(&m_critSection);
    return PVR_ERROR_NOT_SAVED;
  }

  pthread_mutex_unlock(&m_critSection);

  return PVR_ERROR_NO_ERROR;
}

PVR_ERROR PVRClientVDR::AddChannel(const CTVChannelInfoTag &info)
{

  CStdString m_Summary;
  CStdString m_Summary_2;
  bool update_channel;
  int iChannelNum;

  if (!m_transceiver->IsOpen())
  {
    return PVR_ERROR_SERVER_ERROR;
  }

  if (info.m_iClientNum == -1)
  {
    int new_number = GetNumChannels();

    if (new_number == -1)
    {
      new_number = 1;
    }

    iChannelNum = new_number + 1;

    update_channel = false;
  }
  else
  {
    iChannelNum = info.m_iClientNum;
    update_channel = true;
  }

  m_Summary.Format("%d %s;%s:%i:" , iChannelNum

                   , info.m_strChannel.c_str()
                   , info.m_Settings.m_strProvider.c_str()
                   , info.m_Settings.m_Freq);

  if ((info.m_Settings.m_SourceType == src_DVBS) ||
      (info.m_Settings.m_SourceType == src_DVBS2))
  {
    if      (info.m_Settings.m_Polarization == pol_H)
      m_Summary += "h";
    else if (info.m_Settings.m_Polarization == pol_V)
      m_Summary += "v";
    else if (info.m_Settings.m_Polarization == pol_L)
      m_Summary += "l";
    else if (info.m_Settings.m_Polarization == pol_R)
      m_Summary += "r";
  }

  if (info.m_Settings.m_SourceType == src_DVBT)
  {
    if      (info.m_Settings.m_Inversion == InvOff)
      m_Summary += "I0";
    else if (info.m_Settings.m_Inversion == InvOn)
      m_Summary += "I1";
    else if (info.m_Settings.m_Inversion == InvAuto)
      m_Summary += "I999";
  }

  if (info.m_Settings.m_SourceType == src_DVBT)
  {
    if      (info.m_Settings.m_Bandwidth == bw_5MHz)
      m_Summary += "B5";
    else if (info.m_Settings.m_Bandwidth == bw_6MHz)
      m_Summary += "B6";
    else if (info.m_Settings.m_Bandwidth == bw_7MHz)
      m_Summary += "B7";
    else if (info.m_Settings.m_Bandwidth == bw_8MHz)
      m_Summary += "B8";
    else if (info.m_Settings.m_Bandwidth == bw_Auto)
      m_Summary += "B999";
  }

  if      (info.m_Settings.m_CoderateH == Coderate_None)
    m_Summary += "C0";
  else if (info.m_Settings.m_CoderateH == Coderate_1_2)
    m_Summary += "C12";
  else if (info.m_Settings.m_CoderateH == Coderate_2_3)
    m_Summary += "C23";
  else if (info.m_Settings.m_CoderateH == Coderate_3_4)
    m_Summary += "C34";
  else if (info.m_Settings.m_CoderateH == Coderate_4_5)
    m_Summary += "C45";
  else if (info.m_Settings.m_CoderateH == Coderate_5_6)
    m_Summary += "C56";
  else if (info.m_Settings.m_CoderateH == Coderate_6_7)
    m_Summary += "C67";
  else if (info.m_Settings.m_CoderateH == Coderate_7_8)
    m_Summary += "C78";
  else if (info.m_Settings.m_CoderateH == Coderate_8_9)
    m_Summary += "C89";
  else if (info.m_Settings.m_CoderateH == Coderate_9_10)
    m_Summary += "C910";
  else if (info.m_Settings.m_CoderateH == Coderate_Auto)
    m_Summary += "C999";

  if (info.m_Settings.m_SourceType == src_DVBT)
  {
    if      (info.m_Settings.m_CoderateL == Coderate_None)
      m_Summary += "D0";
    else if (info.m_Settings.m_CoderateL == Coderate_1_2)
      m_Summary += "D12";
    else if (info.m_Settings.m_CoderateL == Coderate_2_3)
      m_Summary += "D23";
    else if (info.m_Settings.m_CoderateL == Coderate_3_4)
      m_Summary += "D34";
    else if (info.m_Settings.m_CoderateL == Coderate_4_5)
      m_Summary += "D45";
    else if (info.m_Settings.m_CoderateL == Coderate_5_6)
      m_Summary += "D56";
    else if (info.m_Settings.m_CoderateL == Coderate_6_7)
      m_Summary += "D67";
    else if (info.m_Settings.m_CoderateL == Coderate_7_8)
      m_Summary += "D78";
    else if (info.m_Settings.m_CoderateL == Coderate_8_9)
      m_Summary += "D89";
    else if (info.m_Settings.m_CoderateL == Coderate_9_10)
      m_Summary += "D910";
    else if (info.m_Settings.m_CoderateL == Coderate_Auto)
      m_Summary += "D999";
  }

  if      (info.m_Settings.m_Modulation == modNone)
    m_Summary += "M0";
  else if (info.m_Settings.m_Modulation == modQAM4)
    m_Summary += "M4";
  else if (info.m_Settings.m_Modulation == modQAM16)
    m_Summary += "M16";
  else if (info.m_Settings.m_Modulation == modQAM32)
    m_Summary += "M32";
  else if (info.m_Settings.m_Modulation == modQAM64)
    m_Summary += "M64";
  else if (info.m_Settings.m_Modulation == modQAM128)
    m_Summary += "M128";
  else if (info.m_Settings.m_Modulation == modQAM256)
    m_Summary += "M256";
  else if (info.m_Settings.m_Modulation == modQAM512)
    m_Summary += "M512";
  else if (info.m_Settings.m_Modulation == modQAM1024)
    m_Summary += "M1024";
  else if (info.m_Settings.m_Modulation == modQAMAuto)
    m_Summary += "M998";
  else if (info.m_Settings.m_Modulation == modBPSK)
    m_Summary += "M1";
  else if (info.m_Settings.m_Modulation == modQPSK)
    m_Summary += "M2";
  else if (info.m_Settings.m_Modulation == modOQPSK)
    m_Summary += "M3";
  else if (info.m_Settings.m_Modulation == mod8PSK)
    m_Summary += "M5";
  else if (info.m_Settings.m_Modulation == mod16APSK)
    m_Summary += "M6";
  else if (info.m_Settings.m_Modulation == mod32APSK)
    m_Summary += "M7";
  else if (info.m_Settings.m_Modulation == modOFDM)
    m_Summary += "M8";
  else if (info.m_Settings.m_Modulation == modCOFDM)
    m_Summary += "M9";
  else if (info.m_Settings.m_Modulation == modVSB8)
    m_Summary += "M10";
  else if (info.m_Settings.m_Modulation == modVSB16)
    m_Summary += "M11";

  if (info.m_Settings.m_SourceType == src_DVBT)
  {
    if      (info.m_Settings.m_Transmission == transmission_2K)
      m_Summary += "T2";
    else if (info.m_Settings.m_Transmission == transmission_4K)
      m_Summary += "T4";
    else if (info.m_Settings.m_Transmission == transmission_8K)
      m_Summary += "T8";
    else if (info.m_Settings.m_Transmission == transmission_Auto)
      m_Summary += "T999";

    if      (info.m_Settings.m_Guard == guard_1_4)
      m_Summary += "G4";
    else if (info.m_Settings.m_Guard == guard_1_8)
      m_Summary += "G8";
    else if (info.m_Settings.m_Guard == guard_1_16)
      m_Summary += "G16";
    else if (info.m_Settings.m_Guard == guard_1_32)
      m_Summary += "G32";
    else if (info.m_Settings.m_Guard == guard_Auto)
      m_Summary += "G999";

    if (info.m_Settings.m_Hierarchie)
      m_Summary += "Y1";
    else
      m_Summary += "Y0";

    if      (info.m_Settings.m_Alpha == alpha_0)
      m_Summary += "A0";
    else if (info.m_Settings.m_Alpha == alpha_1)
      m_Summary += "A1";
    else if (info.m_Settings.m_Alpha == alpha_2)
      m_Summary += "A2";
    else if (info.m_Settings.m_Alpha == alpha_4)
      m_Summary += "A4";

    if (info.m_Settings.m_Priority)
      m_Summary += "P1";
    else
      m_Summary += "P0";
  }

  if (info.m_Settings.m_SourceType == src_DVBS2)
  {
    if      (info.m_Settings.m_Rolloff == rolloff_Unknown)
      m_Summary += "O0";
    else if (info.m_Settings.m_Rolloff == rolloff_20)
      m_Summary += "O20";
    else if (info.m_Settings.m_Rolloff == rolloff_25)
      m_Summary += "O25";
    else if (info.m_Settings.m_Rolloff == rolloff_25)
      m_Summary += "O35";
  }

  if      (info.m_Settings.m_SourceType == src_DVBS)
    m_Summary += "O35S0:S";
  else if (info.m_Settings.m_SourceType == src_DVBS2)
    m_Summary += "S1:S";
  else if (info.m_Settings.m_SourceType == src_DVBC)
    m_Summary += ":C";
  else if (info.m_Settings.m_SourceType == src_DVBT)
    m_Summary += ":T";

  m_Summary_2.Format(":%i:%i:%i,%i;%i,%i:%i:%i:%i:%i:%i:%i" , info.m_Settings.m_Symbolrate
                     , info.m_Settings.m_VPID
                     , info.m_Settings.m_APID1
                     , info.m_Settings.m_APID2
                     , info.m_Settings.m_DPID1
                     , info.m_Settings.m_DPID2
                     , info.m_Settings.m_TPID
                     , info.m_Settings.m_CAID
                     , info.m_Settings.m_SID
                     , info.m_Settings.m_NID
                     , info.m_Settings.m_TID
                     , info.m_Settings.m_RID);

  m_Summary += m_Summary_2;

  vector<string> lines;

  int            code;

  char           buffer[1024];

  pthread_mutex_lock(&m_critSection);

  if (!update_channel)
  {
    sprintf(buffer, "NEWC %s", m_Summary.c_str());

    if (!m_transceiver->SendCommand(buffer, code, lines))
    {
      pthread_mutex_unlock(&m_critSection);
      return PVR_ERROR_SERVER_ERROR;
    }

    if (code != 250)
    {
      pthread_mutex_unlock(&m_critSection);
      return PVR_ERROR_NOT_SAVED;
    }
  }
  else
  {
    // Modified channel
    sprintf(buffer, "LSTC %d", iChannelNum);

    if (!m_transceiver->SendCommand(buffer, code, lines))
    {
      pthread_mutex_unlock(&m_critSection);
      return PVR_ERROR_SERVER_ERROR;
    }

    if (code != 250)
    {
      pthread_mutex_unlock(&m_critSection);
      return PVR_ERROR_NOT_SYNC;
    }

    sprintf(buffer, "MODC %s", m_Summary.c_str());

    if (!m_transceiver->SendCommand(buffer, code, lines))
    {
      pthread_mutex_unlock(&m_critSection);
      return PVR_ERROR_SERVER_ERROR;
    }

    if (code != 250)
    {
      pthread_mutex_unlock(&m_critSection);
      return PVR_ERROR_NOT_SAVED;
    }
  }

  pthread_mutex_unlock(&m_critSection);

  return PVR_ERROR_NO_ERROR;
}

PVR_ERROR PVRClientVDR::DeleteChannel(unsigned int number)
{

  vector<string> lines;
  int            code;
  char           buffer[1024];

  if (!m_transceiver->IsOpen())
  {
    return PVR_ERROR_SERVER_ERROR;
  }

  pthread_mutex_lock(&m_critSection);

  sprintf(buffer, "LSTC %d", number);

  if (!m_transceiver->SendCommand(buffer, code, lines))
  {
    return PVR_ERROR_SERVER_ERROR;
  }

  if (code != 250)
  {
    return PVR_ERROR_NOT_SYNC;
  }

  sprintf(buffer, "DELC %d", number);

  if (!m_transceiver->SendCommand(buffer, code, lines))
  {
    return PVR_ERROR_SERVER_ERROR;
  }

  if (code != 250)
  {
    return PVR_ERROR_NOT_DELETED;
  }

  pthread_mutex_unlock(&m_critSection);

  return PVR_ERROR_NO_ERROR;
}

PVR_ERROR PVRClientVDR::RenameChannel(unsigned int number, CStdString &newname)
{

  CStdString     str_part1;
  CStdString     str_part2;
  vector<string> lines;
  int            code;
  char           buffer[1024];
  int            found;

  if (!m_transceiver->IsOpen())
  {
    return PVR_ERROR_SERVER_ERROR;
  }

  pthread_mutex_lock(&m_critSection);

  sprintf(buffer, "LSTC %d", number);

  if (!m_transceiver->SendCommand(buffer, code, lines))
  {
    pthread_mutex_unlock(&m_critSection);
    return PVR_ERROR_SERVER_ERROR;
  }

  if (code != 250)
  {
    pthread_mutex_unlock(&m_critSection);
    return PVR_ERROR_NOT_SYNC;
  }

  vector<string>::iterator it = lines.begin();

  string& data(*it);
  CStdString str_result = data;

  found = str_result.find(" ", 0);
  str_part1.assign(str_result, found + 1);
  str_result.erase(0, found + 1);

  /// Channel and provider name
  found = str_result.find(":", 0);
  str_part2.assign(str_result, found);
  str_result.erase(0, found);
  found = str_part2.find(";", 0);

  if (found == -1)
  {
    str_part2 = newname;
  }
  else
  {
    str_part2.erase(0, found);
    str_part2.insert(0, newname);
  }

  sprintf(buffer, "MODC %s %s %s", str_part1.c_str(), str_part2.c_str(), str_result.c_str());

  if (!m_transceiver->SendCommand(buffer, code, lines))
  {
    pthread_mutex_unlock(&m_critSection);
    return PVR_ERROR_SERVER_ERROR;
  }

  if (code != 250)
  {
    pthread_mutex_unlock(&m_critSection);
    return PVR_ERROR_NOT_SAVED;
  }

  pthread_mutex_unlock(&m_critSection);

  return PVR_ERROR_NO_ERROR;
}

PVR_ERROR PVRClientVDR::MoveChannel(unsigned int number, unsigned int newnumber)
{
  vector<string> lines;
  int            code;
  char           buffer[1024];

  if (!m_transceiver->IsOpen())
      return PVR_ERROR_SERVER_ERROR;

  pthread_mutex_lock(&m_critSection);

  sprintf(buffer, "LSTC %d", number);

  if (!m_transceiver->SendCommand(buffer, code, lines))
  {
    pthread_mutex_unlock(&m_critSection);
    return PVR_ERROR_SERVER_ERROR;
  }

  if (code != 250)
  {
    pthread_mutex_unlock(&m_critSection);
    return PVR_ERROR_NOT_SYNC;
  }

  sprintf(buffer, "MOVC %d %d", number, newnumber);

  if (!m_transceiver->SendCommand(buffer, code, lines))
  {
    pthread_mutex_unlock(&m_critSection);
    return PVR_ERROR_SERVER_ERROR;
  }

  if (code != 250)
  {
    pthread_mutex_unlock(&m_critSection);
    return PVR_ERROR_NOT_SAVED;
  }

  pthread_mutex_unlock(&m_critSection);

  return PVR_ERROR_NO_ERROR;
}

bool PVRClientVDR::GetChannel(unsigned int number, PVR_CHANNEL &channeldata)
{
  vector<string> lines;
  int            code;
  char           buffer[1024];

  if (!m_transceiver->IsOpen())
    return false;

  sprintf(buffer, "LSTC %d", number);
  if (!m_transceiver->SendCommand(buffer, code, lines))
  {
    if (code != 451)
    {
      return false;
    }
    Sleep(750);
  }

  vector<string>::iterator it = lines.begin();
  string& data(*it);

  CStdString str_result = data;
  int found;
  int idVPID = 0;
  int idAPID1 = 0;
  int idAPID2 = 0;
  int idDPID1 = 0;
  int idDPID2 = 0;
  int idCAID = 0;
  int idTPID = 0;
  CStdString name;
  int id;

  if (m_bCharsetConv)
    XBMC_unknown_to_utf8(str_result);

  // Channel number
  channeldata.number = atol(str_result.c_str());
  str_result.erase(0, str_result.find(" ", 0) + 1);

  // Channel and provider name
  found = str_result.find(":", 0);
  name.assign(str_result, found);
  str_result.erase(0, found + 1);
  found = name.find(";", 0);

  if (found == -1)
  {
    channeldata.name = name;
  }
  else
  {
    CStdString name2;
    name2.assign(name, found);
    channeldata.name = name2;
  }

  // Channel frequency
  str_result.erase(0, str_result.find(":", 0) + 1);

  // Source descriptor
  str_result.erase(0, str_result.find(":", 0));

  // Source Type
  if (str_result.compare(0, 2, ":C") == 0)
  {
    str_result.erase(0, 3);
  }
  else if (str_result.compare(0, 2, ":T") == 0)
  {
    str_result.erase(0, 3);
  }
  else if (str_result.compare(0, 2, ":S") == 0)
  {
    str_result.erase(0, 2);
    found = str_result.find(":", 0);
    str_result.erase(0, found + 1);
  }
  else if (str_result.compare(0, 2, ":P") == 0)
  {
    str_result.erase(0, 3);
  }

  // Channel symbolrate
  found = str_result.find(":", 0);
  str_result.erase(0, found + 1);

  // Channel program id
  idVPID = atol(str_result.c_str());
  found = str_result.find(":", 0);
  str_result.erase(0, found + 1);

  // Channel audio id's
  found = str_result.find(":", 0);
  name.assign(str_result, found);
  str_result.erase(0, found + 1);
  found = name.find(";", 0);

  if (found == -1)
  {
    id = atol(name.c_str());

    if (id == 0)
    {
      idAPID1 = 0;
      idAPID2 = 0;
      idDPID1 = 0;
      idDPID2 = 0;
    }
    else
    {
      idAPID1 = id;
      found = name.find(",", 0);

      if (found == -1)
      {
        idAPID2 = 0;
      }
      else
      {
        name.erase(0, found + 1);
        idAPID2 = atol(name.c_str());
      }

      idDPID1 = 0;
      idDPID2 = 0;
    }
  }
  else
  {
    int id;
    id = atol(name.c_str());

    if (id == 0)
    {
      idAPID1 = 0;
      idAPID2 = 0;
    }
    else
    {
      idAPID1 = id;
      found = name.find(",", 0);

      if (found == -1)
      {
        idAPID2 = 0;
      }
      else
      {
        name.erase(0, found + 1);
        idAPID2 = atol(name.c_str());
      }
    }

    name.erase(0, name.find(";", 0) + 1);
    id = atoi(name.c_str());
    if (id == 0)
    {
      idDPID1 = 0;
      idDPID2 = 0;
    }
    else
    {
      idDPID1 = id;
      found = name.find(",", 0);

      if (found == -1)
      {
        idDPID2 = 0;
      }
      else
      {
        name.erase(0, found + 1);
        idDPID2 = atol(name.c_str());
      }
    }
  }

  // Teletext id
  idTPID = atoi(str_result.c_str());
  str_result.erase(0, str_result.find(":", 0) + 1);
  channeldata.teletext = idTPID ? true : false;

  // CAID id
  idCAID = atoi(str_result.c_str());
  str_result.erase(0, str_result.find(":", 0) + 1);
  channeldata.encrypted = idCAID ? true : false;

  channeldata.radio = (idVPID == 0) && (idAPID1 != 0) ? true : false;
  return true;  
}


/************************************************************/
/** Record handling **/

int PVRClientVDR::GetNumRecordings(void)
{
  vector<string>  lines;
  int             code;

  if (!m_transceiver->IsOpen())
    return -1;

  pthread_mutex_lock(&m_critSection);

  if (!m_transceiver->SendCommand("STAT records", code, lines))
  {
    XBMC_log(LOG_ERROR, "PCRClient-vdr: Couldn't get recordings count");
    pthread_mutex_unlock(&m_critSection);
    return -1;
  }

  vector<string>::iterator it = lines.begin();

  string& data(*it);
  pthread_mutex_unlock(&m_critSection);
  return atol(data.c_str());
}

PVR_ERROR PVRClientVDR::GetAllRecordings(VECRECORDINGS *results)
{
  vector<string> lines;
  int            code;
  char           buffer[1024];
  unsigned int   cnt = 0;
  CStdString     strbuffer;

  if (!m_transceiver->IsOpen())
    return PVR_ERROR_SERVER_ERROR;

  pthread_mutex_lock(&m_critSection);

  if (!m_transceiver->SendCommand("LSTR", code, lines))
  {
    pthread_mutex_unlock(&m_critSection);
    return PVR_ERROR_SERVER_ERROR;
  }

  for (vector<string>::iterator it2 = lines.begin(); it2 != lines.end(); it2++)
  {
    string& data(*it2);
    CStdString str_result = data;
    CTVRecordingInfoTag broadcast;

    /* Convert to UTF8 string format */
    if (m_bCharsetConv)
      XBMC_unknown_to_utf8(str_result);

    /* Get recording ID */
    broadcast.m_Index = atol(str_result.c_str());

    /* Get recording name */

    str_result.erase(0, str_result.find(":", 0) + 4); //find : between hour and minutes, go to first char after minutes
    str_result.erase(0, str_result.find(" ", 0) + 1); //now go to last blank before title starts and skip unicode garbage (if present)
    broadcast.m_strTitle = str_result.c_str();

    /* Set file string for replay devices */
    broadcast.m_strFileNameAndPath.Format("record://%i", broadcast.m_Index);

    /* Save it inside list */
    results->push_back(broadcast);
    cnt++;
  }

  std::vector<CTVRecordingInfoTag>::iterator it;

  for (unsigned int i = 0; i < cnt; i++)
  {
    lines.erase(lines.begin(), lines.end());
    it = results->begin() + i;

    sprintf(buffer, "LSTR %d", (*it).m_Index);
    if (!m_transceiver->SendCommand(buffer, code, lines))
    {
      pthread_mutex_unlock(&m_critSection);
      return PVR_ERROR_SERVER_ERROR;
    }

    for (vector<string>::iterator it2 = lines.begin(); it2 != lines.end(); it2++)
    {
      string& data(*it2);
      CStdString str_result = data;

      /* Convert to UTF8 string format */
	  if (m_bCharsetConv)
        XBMC_unknown_to_utf8(str_result);

      /* Get Channelname */
      if (str_result.find("C", 0) == 0)
      {
        str_result.erase(0, 2);
        str_result.erase(0, str_result.find(" ", 0) + 1);
        (*it).m_strChannel = str_result.c_str();
        continue;
      }

      /* Get Title */
      if (str_result.find("T", 0) == 0)
      {
        str_result.erase(0, 2);
        //   (*it).m_strTitle = str_result.c_str();
        continue;
      }

      /* Get short description */
      if (str_result.find("S", 0) == 0)
      {
        str_result.erase(0, 2);
        (*it).m_strPlotOutline   = str_result.c_str();
        continue;
      }

      /* Get description */
      if (str_result.find("D", 0) == 0)
      {
        str_result.erase(0, 2);
        int pos = 0;

        while (1)
        {
          pos = str_result.find("|", pos);
          if (pos < 0)
            break;

          str_result.replace(pos, 1, 1, '\n');
        }

        (*it).m_strPlot = str_result.c_str();
        continue;
      }

      /* Get ID, date and length */
      if (str_result.find("E ", 0) == 0)
      {
        str_result.erase(0, 2);
        // (*it).m_uniqueID = atol(str_result.c_str());
        str_result.erase(0, str_result.find(" ", 0) + 1);
        time_t rec_time = atol(str_result.c_str());
        str_result.erase(0, str_result.find(" ", 0) + 1);
        unsigned int duration = atol(str_result.c_str());

        (*it).m_startTime = CDateTime((time_t)rec_time);
        (*it).m_endTime   = CDateTime((time_t)rec_time + duration);
        (*it).m_duration  = CDateTimeSpan(0, 0, duration / 60, duration % 60);
        (*it).m_Summary.Format("%s", (*it).m_startTime.GetAsLocalizedDateTime(false, false));
        continue;
      }
    }
  }

  pthread_mutex_unlock(&m_critSection);

  return PVR_ERROR_NO_ERROR;
}

PVR_ERROR PVRClientVDR::DeleteRecording(const CTVRecordingInfoTag &recinfo)
{
  vector<string> lines;
  int            code;
  char           buffer[1024];

  if (!m_transceiver->IsOpen())
    return PVR_ERROR_SERVER_ERROR;

  pthread_mutex_lock(&m_critSection);

  sprintf(buffer, "LSTR %d", recinfo.m_Index);
  if (!m_transceiver->SendCommand(buffer, code, lines))
  {
    pthread_mutex_unlock(&m_critSection);
    return PVR_ERROR_SERVER_ERROR;
  }

  if (code != 215)
  {
    pthread_mutex_unlock(&m_critSection);
    return PVR_ERROR_NOT_SYNC;
  }

  sprintf(buffer, "DELR %d", recinfo.m_Index);
  if (!m_transceiver->SendCommand(buffer, code, lines))
  {
    pthread_mutex_unlock(&m_critSection);
    return PVR_ERROR_SERVER_ERROR;
  }

  if (code != 250)
  {
    pthread_mutex_unlock(&m_critSection);
    return PVR_ERROR_NOT_DELETED;
  }

  pthread_mutex_unlock(&m_critSection);

  return PVR_ERROR_NO_ERROR;
}

PVR_ERROR PVRClientVDR::RenameRecording(const CTVRecordingInfoTag &recinfo, CStdString &newname)
{
  vector<string> lines;
  int            code;
  char           buffer[1024];

  if (!m_transceiver->IsOpen())
    return PVR_ERROR_SERVER_ERROR;

  pthread_mutex_lock(&m_critSection);

  sprintf(buffer, "LSTR %d", recinfo.m_Index);
  if (!m_transceiver->SendCommand(buffer, code, lines))
  {
    pthread_mutex_unlock(&m_critSection);
    return PVR_ERROR_SERVER_ERROR;
  }

  if (code != 215)
  {
    pthread_mutex_unlock(&m_critSection);
    return PVR_ERROR_NOT_SYNC;
  }

  sprintf(buffer, "RENR %d %s", recinfo.m_Index, newname.c_str());

  if (!m_transceiver->SendCommand(buffer, code, lines))
  {
    pthread_mutex_unlock(&m_critSection);
    return PVR_ERROR_SERVER_ERROR;
  }

  if (code != 250)
  {
    pthread_mutex_unlock(&m_critSection);
    return PVR_ERROR_NOT_DELETED;
  }

  pthread_mutex_unlock(&m_critSection);

  return PVR_ERROR_NO_ERROR;
}


/************************************************************/
/** Timer handling */

int PVRClientVDR::GetNumTimers(void)
{
  vector<string>  lines;
  int             code;

  if (!m_transceiver->IsOpen())
    return -1;

  pthread_mutex_lock(&m_critSection);

  if (!m_transceiver->SendCommand("STAT timers", code, lines))
  {
    XBMC_log(LOG_ERROR, "PCRClient-vdr: Couldn't get timers count");
    pthread_mutex_unlock(&m_critSection);
    return -1;
  }

  vector<string>::iterator it = lines.begin();

  string& data(*it);
  pthread_mutex_unlock(&m_critSection);
  return atol(data.c_str());
}

PVR_ERROR PVRClientVDR::GetAllTimers(VECTVTIMERS *results)
{
  vector<string> lines;
  int            code;
  int            found;

  if (!m_transceiver->IsOpen())
    return PVR_ERROR_SERVER_ERROR;

  pthread_mutex_lock(&m_critSection);

  if (!m_transceiver->SendCommand("LSTT", code, lines))
  {
    pthread_mutex_unlock(&m_critSection);
    return PVR_ERROR_SERVER_ERROR;
  }

  for (vector<string>::iterator it = lines.begin(); it != lines.end(); it++)
  {
    string& data(*it);
    CStdString str_result = data;
    CStdString name;
    CTVTimerInfoTag timerinfo;

    /**
     * VDR Format given by LSTT:
     * 250-1 1:6:2008-10-27:0013:0055:50:99:Zeiglers wunderbare Welt des Fu�balls:
     * 250 2 0:15:2008-10-26:2000:2138:50:99:ZDFtheaterkanal:
     * 250 3 1:6:MTWTFS-:2000:2129:50:99:WDR K�ln:
     */

    if (m_bCharsetConv)
      XBMC_unknown_to_utf8(str_result);

    /* Id */
    timerinfo.m_Index = atol(str_result.c_str());
    found = str_result.find(" ", 0);
    str_result.erase(0, found + 1);

    /* Active */
    timerinfo.m_Active = (bool) atoi(str_result.c_str());
	  found = str_result.find(":", 0);
    str_result.erase(0, found + 1);

    /* Channel number */
    timerinfo.m_clientNum = atol(str_result.c_str());
    found = str_result.find(":", 0);
    str_result.erase(0, found + 1);

    /* Start/end time */
    int year  = atol(str_result.c_str());
    int month = 0;
    int day   = 0;
    timerinfo.m_FirstDay = NULL;

    if (year != 0)
    {
      timerinfo.m_Repeat = false;
      found = str_result.find("-", 0);
      str_result.erase(0, found + 1);
      month = atol(str_result.c_str());
      found = str_result.find("-", 0);
      str_result.erase(0, found + 1);
      day   = atol(str_result.c_str());
      found = str_result.find(":", 0);
      str_result.erase(0, found + 1);

      timerinfo.m_Repeat_Mon = false;
      timerinfo.m_Repeat_Tue = false;
      timerinfo.m_Repeat_Wed = false;
      timerinfo.m_Repeat_Thu = false;
      timerinfo.m_Repeat_Fri = false;
      timerinfo.m_Repeat_Sat = false;
      timerinfo.m_Repeat_Sun = false;
    }
    else
    {
      timerinfo.m_Repeat = true;

      timerinfo.m_Repeat_Mon = str_result.compare(0, 1, "-") ? true : false;
      timerinfo.m_Repeat_Tue = str_result.compare(1, 1, "-") ? true : false;
      timerinfo.m_Repeat_Wed = str_result.compare(2, 1, "-") ? true : false;
      timerinfo.m_Repeat_Thu = str_result.compare(3, 1, "-") ? true : false;
      timerinfo.m_Repeat_Fri = str_result.compare(4, 1, "-") ? true : false;
      timerinfo.m_Repeat_Sat = str_result.compare(5, 1, "-") ? true : false;
      timerinfo.m_Repeat_Sun = str_result.compare(6, 1, "-") ? true : false;

      str_result.erase(0, 7);
      found = str_result.find("@", 0);

      if (found != -1)
      {
        str_result.erase(0, 1);
        year  = atol(str_result.c_str());
        found = str_result.find("-", 0);
        str_result.erase(0, found + 1);

        month = atol(str_result.c_str());
        found = str_result.find("-", 0);
        str_result.erase(0, found + 1);

        day   = atol(str_result.c_str());
      }

      found = str_result.find(":", 0);
      str_result.erase(0, found + 1);
    }

    name.assign(str_result, 2);

    str_result.erase(0, 2);
    int start_hour = atol(name.c_str());

    name.assign(str_result, 2);
    str_result.erase(0, 3);
    int start_minute = atol(name.c_str());

    name.assign(str_result, 2);
    str_result.erase(0, 2);
    int end_hour = atol(name.c_str());

    name.assign(str_result, 2);
    str_result.erase(0, 3);
    int end_minute = atol(name.c_str());

    if (!timerinfo.m_Repeat)
    {
      int end_day = (start_hour > end_hour ? day + 1 : day);
      timerinfo.m_StartTime = CDateTime(year, month, day, start_hour, start_minute, 0);
      timerinfo.m_StopTime = CDateTime(year, month, end_day, end_hour, end_minute, 0);
    }
    else if (year != 0)
    {
      timerinfo.m_FirstDay = CDateTime(year, month, day, start_hour, start_minute, 0);
      timerinfo.m_StartTime = CDateTime(year, month, day, start_hour, start_minute, 0);
      timerinfo.m_StopTime = CDateTime(year, month, day, end_hour, end_minute, 0);
    }
    else
    {
      timerinfo.m_StartTime = CDateTime(CDateTime::GetCurrentDateTime().GetYear(),
                                        CDateTime::GetCurrentDateTime().GetMonth(),
                                        CDateTime::GetCurrentDateTime().GetDay(),
                                        start_hour, start_minute, 0);
      timerinfo.m_StopTime = CDateTime(CDateTime::GetCurrentDateTime().GetYear(),
                                       CDateTime::GetCurrentDateTime().GetMonth(),
                                       CDateTime::GetCurrentDateTime().GetDay(),
                                       end_hour, end_minute, 0);
    }

    /* Priority */
    timerinfo.m_Priority = atol(str_result.c_str());
    found = str_result.find(":", 0);
    str_result.erase(0, found + 1);

    /* Lifetime */
    timerinfo.m_Lifetime = atol(str_result.c_str());
    found = str_result.find(":", 0);
    str_result.erase(0, found + 1);

    /* Title */
    found = str_result.find(":", 0);
    str_result.erase(found, found + 1);
    timerinfo.m_strTitle = str_result.c_str();

    if (!timerinfo.m_Repeat)
    {
      timerinfo.m_Summary.Format("%s %s %s %s %s", timerinfo.m_StartTime.GetAsLocalizedDate()
                                 , XBMC_get_localized_string(18078)
                                 , timerinfo.m_StartTime.GetAsLocalizedTime("", false)
                                 , XBMC_get_localized_string(18079)
                                 , timerinfo.m_StopTime.GetAsLocalizedTime("", false));
    }
    else if (timerinfo.m_FirstDay != NULL)
    {
      timerinfo.m_Summary.Format("%s-%s-%s-%s-%s-%s-%s %s %s %s %s %s %s"
                                 , timerinfo.m_Repeat_Mon ? XBMC_get_localized_string(18080) : "__"
                                 , timerinfo.m_Repeat_Tue ? XBMC_get_localized_string(18081) : "__"
                                 , timerinfo.m_Repeat_Wed ? XBMC_get_localized_string(18082) : "__"
                                 , timerinfo.m_Repeat_Thu ? XBMC_get_localized_string(18083) : "__"
                                 , timerinfo.m_Repeat_Fri ? XBMC_get_localized_string(18084) : "__"
                                 , timerinfo.m_Repeat_Sat ? XBMC_get_localized_string(18085) : "__"
                                 , timerinfo.m_Repeat_Sun ? XBMC_get_localized_string(18086) : "__"
                                 , XBMC_get_localized_string(18087)
                                 , timerinfo.m_FirstDay.GetAsLocalizedDate(false)
                                 , XBMC_get_localized_string(18078)
                                 , timerinfo.m_StartTime.GetAsLocalizedTime("", false)
                                 , XBMC_get_localized_string(18079)
                                 , timerinfo.m_StopTime.GetAsLocalizedTime("", false));
    }
    else
    {
      timerinfo.m_Summary.Format("%s-%s-%s-%s-%s-%s-%s %s %s %s %s"
                                 , timerinfo.m_Repeat_Mon ? XBMC_get_localized_string(18080) : "__"
                                 , timerinfo.m_Repeat_Tue ? XBMC_get_localized_string(18081) : "__"
                                 , timerinfo.m_Repeat_Wed ? XBMC_get_localized_string(18082) : "__"
                                 , timerinfo.m_Repeat_Thu ? XBMC_get_localized_string(18083) : "__"
                                 , timerinfo.m_Repeat_Fri ? XBMC_get_localized_string(18084) : "__"
                                 , timerinfo.m_Repeat_Sat ? XBMC_get_localized_string(18085) : "__"
                                 , timerinfo.m_Repeat_Sun ? XBMC_get_localized_string(18086) : "__"
                                 , XBMC_get_localized_string(18078)
                                 , timerinfo.m_StartTime.GetAsLocalizedTime("", false)
                                 , XBMC_get_localized_string(18079)
                                 , timerinfo.m_StopTime.GetAsLocalizedTime("", false));
    }

    if ((timerinfo.m_StartTime <= CDateTime::GetCurrentDateTime()) &&
        (timerinfo.m_Active == true))
    {
      timerinfo.m_recStatus = true;
      timerinfo.m_strFileNameAndPath.Format("timer://%i #", timerinfo.m_Index);
    }
    else
    {
      timerinfo.m_strFileNameAndPath.Format("timer://%i %s", timerinfo.m_Index, timerinfo.m_Active ? "*" : " ");
      timerinfo.m_recStatus = false;
    }
    
    PVR_CHANNEL channeldata;
    GetChannel(timerinfo.m_clientNum, channeldata);
    timerinfo.m_strChannel = channeldata.name;
    timerinfo.m_Radio = channeldata.radio;

    results->push_back(timerinfo);
  }

  pthread_mutex_unlock(&m_critSection);

  return PVR_ERROR_NO_ERROR;
}

PVR_ERROR PVRClientVDR::GetTimerInfo(unsigned int timernumber, CTVTimerInfoTag &timerinfo)
{
  vector<string>  lines;
  int             code;
  char            buffer[1024];
  CStdString      name;
  int             found;

  if (!m_transceiver->IsOpen())
    return PVR_ERROR_SERVER_ERROR;

  pthread_mutex_lock(&m_critSection);

  sprintf(buffer, "LSTT %d", timernumber);
  if (!m_transceiver->SendCommand(buffer, code, lines))
  {
    pthread_mutex_unlock(&m_critSection);
    return PVR_ERROR_SERVER_ERROR;
  }

  vector<string>::iterator it = lines.begin();

  string& data(*it);
  CStdString str_result = data;

  if (m_bCharsetConv)
    XBMC_unknown_to_utf8(str_result);

  /* Id */
  timerinfo.m_Index = atol(str_result.c_str());
  found = str_result.find(" ", 0);
  str_result.erase(0, found + 1);

  /* Active */
  timerinfo.m_Active = (bool) atoi(str_result.c_str());
  str_result.erase(0, 2);

  /* Channel number */
  timerinfo.m_clientNum = atol(str_result.c_str());
  found = str_result.find(":", 0);
  str_result.erase(0, found + 1);

  /* Start/end time */
  int year  = atol(str_result.c_str());
  int month = 0;
  int day   = 0;

  timerinfo.m_FirstDay = NULL;

  if (year != 0)
  {
    timerinfo.m_Repeat = false;
    found = str_result.find("-", 0);
    str_result.erase(0, found + 1);
    month = atol(str_result.c_str());
    found = str_result.find("-", 0);
    str_result.erase(0, found + 1);
    day   = atol(str_result.c_str());
    found = str_result.find(":", 0);
    str_result.erase(0, found + 1);

    timerinfo.m_Repeat_Mon = false;
    timerinfo.m_Repeat_Tue = false;
    timerinfo.m_Repeat_Wed = false;
    timerinfo.m_Repeat_Thu = false;
    timerinfo.m_Repeat_Fri = false;
    timerinfo.m_Repeat_Sat = false;
    timerinfo.m_Repeat_Sun = false;
  }
  else
  {
    timerinfo.m_Repeat = true;
    timerinfo.m_Repeat_Mon = str_result.compare(0, 1, "-") ? true : false;
    timerinfo.m_Repeat_Tue = str_result.compare(1, 1, "-") ? true : false;
    timerinfo.m_Repeat_Wed = str_result.compare(2, 1, "-") ? true : false;
    timerinfo.m_Repeat_Thu = str_result.compare(3, 1, "-") ? true : false;
    timerinfo.m_Repeat_Fri = str_result.compare(4, 1, "-") ? true : false;
    timerinfo.m_Repeat_Sat = str_result.compare(5, 1, "-") ? true : false;
    timerinfo.m_Repeat_Sun = str_result.compare(6, 1, "-") ? true : false;

    str_result.erase(0, 7);
    found = str_result.find("@", 0);

    if (found != -1)
    {
      year  = atol(str_result.c_str());
      found = str_result.find("-", 0);
      str_result.erase(0, found + 1);

      month = atol(str_result.c_str());
      found = str_result.find("-", 0);
      str_result.erase(0, found + 1);

      day   = atol(str_result.c_str());
    }

    found = str_result.find(":", 0);

    str_result.erase(0, found + 1);
  }

  name.assign(str_result, 2);

  str_result.erase(0, 2);
  int start_hour = atol(name.c_str());

  name.assign(str_result, 2);
  str_result.erase(0, 3);
  int start_minute = atol(name.c_str());

  name.assign(str_result, 2);
  str_result.erase(0, 2);
  int end_hour = atol(name.c_str());

  name.assign(str_result, 2);
  str_result.erase(0, 3);
  int end_minute = atol(name.c_str());

  if (!timerinfo.m_Repeat)
  {
    int end_day = (start_hour > end_hour ? day + 1 : day);
    timerinfo.m_StartTime = CDateTime(year, month, day, start_hour, start_minute, 0);
    timerinfo.m_StopTime = CDateTime(year, month, end_day, end_hour, end_minute, 0);
  }
  else if (year != 0)
  {
    timerinfo.m_FirstDay = CDateTime(year, month, day, start_hour, start_minute, 0);
    timerinfo.m_StartTime = CDateTime(year, month, day, start_hour, start_minute, 0);
    timerinfo.m_StopTime = CDateTime(year, month, day, end_hour, end_minute, 0);
  }
  else
  {
    timerinfo.m_StartTime = CDateTime(CDateTime::GetCurrentDateTime().GetYear(),
                                      CDateTime::GetCurrentDateTime().GetMonth(),
                                      CDateTime::GetCurrentDateTime().GetDay(),
                                      start_hour, start_minute, 0);
    timerinfo.m_StopTime = CDateTime(CDateTime::GetCurrentDateTime().GetYear(),
                                     CDateTime::GetCurrentDateTime().GetMonth(),
                                     CDateTime::GetCurrentDateTime().GetDay(),
                                     end_hour, end_minute, 0);
  }

  /* Priority */
  timerinfo.m_Priority = atol(str_result.c_str());
  found = str_result.find(":", 0);
  str_result.erase(0, found + 1);

  /* Lifetime */
  timerinfo.m_Lifetime = atol(str_result.c_str());
  found = str_result.find(":", 0);
  str_result.erase(0, found + 1);

  /* Title */
  found = str_result.find(":", 0);
  str_result.erase(found, found + 1);
  timerinfo.m_strTitle = str_result.c_str();
  timerinfo.m_strPath.Format("timer://%i%s", timerinfo.m_Index, timerinfo.m_Active ? " *" : "");

  pthread_mutex_unlock(&m_critSection);

  return PVR_ERROR_NO_ERROR;
}

PVR_ERROR PVRClientVDR::AddTimer(const CTVTimerInfoTag &timerinfo)
{
  CStdString     m_Summary;
  CStdString     m_Summary_2;
  vector<string> lines;
  int            code;
  char           buffer[1024];

  if (!m_transceiver->IsOpen())
    return PVR_ERROR_SERVER_ERROR;

  if (!timerinfo.m_Repeat)
  {
    m_Summary.Format("%d:%d:%04d-%02d-%02d:%02d%02d:%02d%02d:%d:%d:%s"
                     , timerinfo.m_Active
                     , timerinfo.m_clientNum
                     , timerinfo.m_StartTime.GetYear()
                     , timerinfo.m_StartTime.GetMonth()
                     , timerinfo.m_StartTime.GetDay()
                     , timerinfo.m_StartTime.GetHour()
                     , timerinfo.m_StartTime.GetMinute()
                     , timerinfo.m_StopTime.GetHour()
                     , timerinfo.m_StopTime.GetMinute()
                     , timerinfo.m_Priority
                     , timerinfo.m_Lifetime
                     , timerinfo.m_strTitle.c_str());
  }
  else if (timerinfo.m_FirstDay != NULL)
  {
    m_Summary.Format("%d:%d:%c%c%c%c%c%c%c@"
                     , timerinfo.m_Active
                     , timerinfo.m_clientNum
                     , timerinfo.m_Repeat_Mon ? 'M' : '-'
                     , timerinfo.m_Repeat_Tue ? 'T' : '-'
                     , timerinfo.m_Repeat_Wed ? 'W' : '-'
                     , timerinfo.m_Repeat_Thu ? 'T' : '-'
                     , timerinfo.m_Repeat_Fri ? 'F' : '-'
                     , timerinfo.m_Repeat_Sat ? 'S' : '-'
                     , timerinfo.m_Repeat_Sun ? 'S' : '-');
    m_Summary_2.Format("%04d-%02d-%02d:%02d%02d:%02d%02d:%d:%d:%s"
                       , timerinfo.m_FirstDay.GetYear()
                       , timerinfo.m_FirstDay.GetMonth()
                       , timerinfo.m_FirstDay.GetDay()
                       , timerinfo.m_StartTime.GetHour()
                       , timerinfo.m_StartTime.GetMinute()
                       , timerinfo.m_StopTime.GetHour()
                       , timerinfo.m_StopTime.GetMinute()
                       , timerinfo.m_Priority
                       , timerinfo.m_Lifetime
                       , timerinfo.m_strTitle.c_str());
    m_Summary = m_Summary + m_Summary_2;
  }
  else
  {
    m_Summary.Format("%d:%d:%c%c%c%c%c%c%c:%02d%02d:%02d%02d:%d:%d:%s"
                     , timerinfo.m_Active
                     , timerinfo.m_clientNum
                     , timerinfo.m_Repeat_Mon ? 'M' : '-'
                     , timerinfo.m_Repeat_Tue ? 'T' : '-'
                     , timerinfo.m_Repeat_Wed ? 'W' : '-'
                     , timerinfo.m_Repeat_Thu ? 'T' : '-'
                     , timerinfo.m_Repeat_Fri ? 'F' : '-'
                     , timerinfo.m_Repeat_Sat ? 'S' : '-'
                     , timerinfo.m_Repeat_Sun ? 'S' : '-'
                     , timerinfo.m_StartTime.GetHour()
                     , timerinfo.m_StartTime.GetMinute()
                     , timerinfo.m_StopTime.GetHour()
                     , timerinfo.m_StopTime.GetMinute()
                     , timerinfo.m_Priority
                     , timerinfo.m_Lifetime
                     , timerinfo.m_strTitle.c_str());
  }

  pthread_mutex_lock(&m_critSection);

  if (timerinfo.m_Index == -1)
  {
    sprintf(buffer, "NEWT %s", m_Summary.c_str());

    if (!m_transceiver->SendCommand(buffer, code, lines))
    {
      pthread_mutex_unlock(&m_critSection);
      return PVR_ERROR_NOT_SAVED;
    }

    if (code != 250)
    {
      pthread_mutex_unlock(&m_critSection);
      return PVR_ERROR_NOT_SYNC;
    }
  }
  else
  {
    // Modified timer
    sprintf(buffer, "LSTT %d", timerinfo.m_Index);

    if (!m_transceiver->SendCommand(buffer, code, lines))
    {
      pthread_mutex_unlock(&m_critSection);
      return PVR_ERROR_SERVER_ERROR;
    }

    if (code != 250)
    {
      pthread_mutex_unlock(&m_critSection);
      return PVR_ERROR_NOT_SYNC;
    }

    sprintf(buffer, "MODT %d %s", timerinfo.m_Index, m_Summary.c_str());

    if (!m_transceiver->SendCommand(buffer, code, lines))
    {
      pthread_mutex_unlock(&m_critSection);
      return PVR_ERROR_NOT_SAVED;
    }

    if (code != 250)
    {
      pthread_mutex_unlock(&m_critSection);
      return PVR_ERROR_NOT_SYNC;
    }
  }

  pthread_mutex_unlock(&m_critSection);

  return PVR_ERROR_NO_ERROR;
}

PVR_ERROR PVRClientVDR::DeleteTimer(const CTVTimerInfoTag &timerinfo, bool force)
{
  vector<string> lines;
  int            code;
  char           buffer[1024];

  if (!m_transceiver->IsOpen())
    return PVR_ERROR_SERVER_ERROR;

  pthread_mutex_lock(&m_critSection);

  sprintf(buffer, "LSTT %d", timerinfo.m_Index);
  if (!m_transceiver->SendCommand(buffer, code, lines))
  {
    pthread_mutex_unlock(&m_critSection);
    return PVR_ERROR_SERVER_ERROR;
  }

  if (code != 250)
  {
    pthread_mutex_unlock(&m_critSection);
    return PVR_ERROR_NOT_SYNC;
  }

  lines.erase(lines.begin(), lines.end());

  if (force)
  {
    sprintf(buffer, "DELT %d FORCE", timerinfo.m_Index);
  }
  else
  {
    sprintf(buffer, "DELT %d", timerinfo.m_Index);
  }

  if (!m_transceiver->SendCommand(buffer, code, lines))
  {
    vector<string>::iterator it = lines.begin();
    string& data(*it);
    CStdString str_result = data;
    pthread_mutex_unlock(&m_critSection);

    int found = str_result.find("is recording", 0);

    if (found != -1)
    {
      return PVR_ERROR_RECORDING_RUNNING;
    }
    else
    {
      return PVR_ERROR_NOT_DELETED;
    }
  }

  if (code != 250)
  {
    pthread_mutex_unlock(&m_critSection);
    return PVR_ERROR_NOT_SYNC;
  }

  pthread_mutex_unlock(&m_critSection);

  return PVR_ERROR_NO_ERROR;
}

PVR_ERROR PVRClientVDR::RenameTimer(const CTVTimerInfoTag &timerinfo, CStdString &newname)
{
  if (!m_transceiver->IsOpen())
    return PVR_ERROR_SERVER_ERROR;

  pthread_mutex_lock(&m_critSection);

  CTVTimerInfoTag timerinfo1;
  PVR_ERROR ret = GetTimerInfo(timerinfo.m_Index, timerinfo1);

  if (ret != PVR_ERROR_NO_ERROR)
  {
    pthread_mutex_unlock(&m_critSection);
    return ret;
  }

  timerinfo1.m_strTitle = newname;

  pthread_mutex_unlock(&m_critSection);
  return UpdateTimer(timerinfo1);
}

PVR_ERROR PVRClientVDR::UpdateTimer(const CTVTimerInfoTag &timerinfo)
{
  CStdString     m_Summary;
  CStdString     m_Summary_2;
  vector<string> lines;
  int            code;
  char           buffer[1024];

  if (!m_transceiver->IsOpen())
    return PVR_ERROR_SERVER_ERROR;

  if (timerinfo.m_Index == -1)
    return PVR_ERROR_NOT_SAVED;

  if (!timerinfo.m_Repeat)
  {
    m_Summary.Format("%d:%d:%04d-%02d-%02d:%02d%02d:%02d%02d:%d:%d:%s"
                     , timerinfo.m_Active
                     , timerinfo.m_clientNum
                     , timerinfo.m_StartTime.GetYear()
                     , timerinfo.m_StartTime.GetMonth()
                     , timerinfo.m_StartTime.GetDay()
                     , timerinfo.m_StartTime.GetHour()
                     , timerinfo.m_StartTime.GetMinute()
                     , timerinfo.m_StopTime.GetHour()
                     , timerinfo.m_StopTime.GetMinute()
                     , timerinfo.m_Priority
                     , timerinfo.m_Lifetime
                     , timerinfo.m_strTitle.c_str());
  }
  else if (timerinfo.m_FirstDay != NULL)
  {
    m_Summary.Format("%d:%d:%c%c%c%c%c%c%c@"
                     , timerinfo.m_Active
                     , timerinfo.m_clientNum
                     , timerinfo.m_Repeat_Mon ? 'M' : '-'
                     , timerinfo.m_Repeat_Tue ? 'T' : '-'
                     , timerinfo.m_Repeat_Wed ? 'W' : '-'
                     , timerinfo.m_Repeat_Thu ? 'T' : '-'
                     , timerinfo.m_Repeat_Fri ? 'F' : '-'
                     , timerinfo.m_Repeat_Sat ? 'S' : '-'
                     , timerinfo.m_Repeat_Sun ? 'S' : '-');
    m_Summary_2.Format("%04d-%02d-%02d:%02d%02d:%02d%02d:%d:%d:%s"
                       , timerinfo.m_FirstDay.GetYear()
                       , timerinfo.m_FirstDay.GetMonth()
                       , timerinfo.m_FirstDay.GetDay()
                       , timerinfo.m_StartTime.GetHour()
                       , timerinfo.m_StartTime.GetMinute()
                       , timerinfo.m_StopTime.GetHour()
                       , timerinfo.m_StopTime.GetMinute()
                       , timerinfo.m_Priority
                       , timerinfo.m_Lifetime
                       , timerinfo.m_strTitle.c_str());
    m_Summary = m_Summary + m_Summary_2;
  }
  else
  {
    m_Summary.Format("%d:%d:%c%c%c%c%c%c%c:%02d%02d:%02d%02d:%d:%d:%s"
                     , timerinfo.m_Active
                     , timerinfo.m_clientNum
                     , timerinfo.m_Repeat_Mon ? 'M' : '-'
                     , timerinfo.m_Repeat_Tue ? 'T' : '-'
                     , timerinfo.m_Repeat_Wed ? 'W' : '-'
                     , timerinfo.m_Repeat_Thu ? 'T' : '-'
                     , timerinfo.m_Repeat_Fri ? 'F' : '-'
                     , timerinfo.m_Repeat_Sat ? 'S' : '-'
                     , timerinfo.m_Repeat_Sun ? 'S' : '-'
                     , timerinfo.m_StartTime.GetHour()
                     , timerinfo.m_StartTime.GetMinute()
                     , timerinfo.m_StopTime.GetHour()
                     , timerinfo.m_StopTime.GetMinute()
                     , timerinfo.m_Priority
                     , timerinfo.m_Lifetime
                     , timerinfo.m_strTitle.c_str());
  }

  pthread_mutex_lock(&m_critSection);

  sprintf(buffer, "LSTT %d", timerinfo.m_Index);
  if (!m_transceiver->SendCommand(buffer, code, lines))
  {
    pthread_mutex_unlock(&m_critSection);
    return PVR_ERROR_SERVER_ERROR;
  }

  if (code != 250)
  {
    pthread_mutex_unlock(&m_critSection);
    return PVR_ERROR_NOT_SYNC;
  }

  sprintf(buffer, "MODT %d %s", timerinfo.m_Index, m_Summary.c_str());
  if (!m_transceiver->SendCommand(buffer, code, lines))
  {
    pthread_mutex_unlock(&m_critSection);
    return PVR_ERROR_NOT_SAVED;
  }

  if (code != 250)
  {
    pthread_mutex_unlock(&m_critSection);
    return PVR_ERROR_NOT_SYNC;
  }

  pthread_mutex_unlock(&m_critSection);

  return PVR_ERROR_NO_ERROR;
}


/************************************************************/
/** Live stream handling */

bool PVRClientVDR::OpenLiveStream(unsigned int channel)
{
  if (!m_transceiver->IsOpen())
    return false;

  pthread_mutex_lock(&m_critSection);

  if (!m_transceiver->CanStreamLive(channel))
  {
    pthread_mutex_unlock(&m_critSection);
    return false;
  }

  /* Check if a another stream is opened, if yes first close them */
  if (m_socket_video != INVALID_SOCKET)
  {
    shutdown(m_socket_video, SD_BOTH);

    if (m_iCurrentChannel < 0)
      m_transceiver->AbortStreamRecording();
    else
      m_transceiver->AbortStreamLive();

    closesocket(m_socket_video);
  }

  /* Get Stream socked from VDR Backend */
  m_socket_video = m_transceiver->GetStreamLive(channel);

  /* If received socket is invalid, return */
  if (m_socket_video == INVALID_SOCKET)
  {
    XBMC_log(LOG_ERROR, "PCRClient-vdr: Couldn't get socket for live tv");
    pthread_mutex_unlock(&m_critSection);
    return false;
  }

  m_iCurrentChannel = channel;

  pthread_mutex_unlock(&m_critSection);
  return true;
}

void PVRClientVDR::CloseLiveStream()
{
  if (!m_transceiver->IsOpen())
    return;

  pthread_mutex_lock(&m_critSection);

  if (m_socket_video != INVALID_SOCKET)
  {
    m_transceiver->AbortStreamLive();
    closesocket(m_socket_video);
  }

  pthread_mutex_unlock(&m_critSection);

  return;
}

int PVRClientVDR::ReadLiveStream(BYTE* buf, int buf_size)
{
  if (!m_transceiver->IsOpen())
    return 0;

  if (m_socket_video == INVALID_SOCKET)
    return 0;

  fd_set         set_r, set_e;

  struct timeval tv;

  int            res;

  tv.tv_sec = 10;
  tv.tv_usec = 0;
  FD_ZERO(&set_r);
  FD_ZERO(&set_e);
  FD_SET(m_socket_video, &set_r);
  FD_SET(m_socket_video, &set_e);
  res = select(FD_SETSIZE, &set_r, NULL, &set_e, &tv);

  if (res < 0)
  {
    XBMC_log(LOG_ERROR, "PVRClientVDR::Read - select failed");
    return 0;
  }

  if (res == 0)
  {
    XBMC_log(LOG_ERROR, "PVRClientVDR::Read - timeout waiting for data");
    return 0;
  }

  res = recv(m_socket_video, (char*)buf, (size_t)buf_size, 0);

  if (res < 0)
  {
    XBMC_log(LOG_ERROR, "PVRClientVDR::Read - failed");
    return 0;
  }

  if (res == 0)
  {
    XBMC_log(LOG_ERROR, "PVRClientVDR::Read - eof");
    return 0;
  }

  return res;
}

int PVRClientVDR::GetCurrentClientChannel()
{
  return m_iCurrentChannel;
}

bool PVRClientVDR::SwitchChannel(unsigned int channel)
{
  if (!m_transceiver->IsOpen())
    return false;

  pthread_mutex_lock(&m_critSection);

  if (!m_transceiver->CanStreamLive(channel))
  {
    pthread_mutex_unlock(&m_critSection);
    return false;
  }

  if (m_socket_video != INVALID_SOCKET)
  {
    shutdown(m_socket_video, SD_BOTH);
    m_transceiver->AbortStreamLive();
    closesocket(m_socket_video);
  }

  m_socket_video = m_transceiver->GetStreamLive(channel);

  if (m_socket_video == INVALID_SOCKET)
  {
    XBMC_log(LOG_ERROR, "PCRClient-vdr: Couldn't get socket for live tv");
    pthread_mutex_unlock(&m_critSection);
    return false;
  }

  m_iCurrentChannel = channel;

  pthread_mutex_unlock(&m_critSection);
  return true;
}


/************************************************************/
/** Record stream handling */

bool PVRClientVDR::OpenRecordedStream(const CTVRecordingInfoTag &recinfo)
{
  if (!m_transceiver->IsOpen())
  {
    return false;
  }

  pthread_mutex_lock(&m_critSection);

  /* Check if a another stream is opened, if yes first close them */

  if (m_socket_video != INVALID_SOCKET)
  {
    shutdown(m_socket_video, SD_BOTH);

    if (m_iCurrentChannel < 0)
      m_transceiver->AbortStreamRecording();
    else
      m_transceiver->AbortStreamLive();

    closesocket(m_socket_video);
  }

  /* Get Stream socked from VDR Backend */
  m_socket_video = m_transceiver->GetStreamRecording(recinfo.m_Index, &currentPlayingRecordBytes, &currentPlayingRecordFrames);

  pthread_mutex_unlock(&m_critSection);

  if (!currentPlayingRecordBytes)
  {
    return false;
  }

  /* If received socket is invalid, return */
  if (m_socket_video == INVALID_SOCKET)
  {
    XBMC_log(LOG_ERROR, "PCRClient-vdr: Couldn't get socket for recording");
    return false;
  }

  m_iCurrentChannel               = -1;
  currentPlayingRecordPosition    = 0;
  return true;
}

void PVRClientVDR::CloseRecordedStream(void)
{

  if (!m_transceiver->IsOpen())
  {
    return;
  }

  pthread_mutex_lock(&m_critSection);

  if (m_socket_video != INVALID_SOCKET)
  {
    m_transceiver->AbortStreamRecording();
    closesocket(m_socket_video);
  }

  pthread_mutex_unlock(&m_critSection);

  return;
}

int PVRClientVDR::ReadRecordedStream(BYTE* buf, int buf_size)
{

  vector<string> lines;
  int            code;
  char           buffer[1024];
  unsigned long  amountReceived;

  if (!m_transceiver->IsOpen() || m_socket_video == INVALID_SOCKET)
    return 0;

  if (currentPlayingRecordPosition + buf_size > currentPlayingRecordBytes)
    return 0;

  sprintf(buffer, "READ %llu %u", currentPlayingRecordPosition, buf_size);
  if (!m_transceiver->SendCommand(buffer, code, lines))
  {
    return 0;
  }

  vector<string>::iterator it = lines.begin();
  string& data(*it);

  amountReceived = atol(data.c_str());

  fd_set         set_r, set_e;

  struct timeval tv;
  int            res;

  tv.tv_sec = 3;
  tv.tv_usec = 0;

  FD_ZERO(&set_r);
  FD_ZERO(&set_e);
  FD_SET(m_socket_video, &set_r);
  FD_SET(m_socket_video, &set_e);
  res = select(FD_SETSIZE, &set_r, NULL, &set_e, &tv);

  if (res < 0)
  {
    XBMC_log(LOG_ERROR, "PVRClientVDR::ReadRecordedStream - select failed");
    return 0;
  }

  if (res == 0)
  {
    XBMC_log(LOG_ERROR, "PVRClientVDR::ReadRecordedStream - timeout waiting for data");
    return 0;
  }

  res = recv(m_socket_video, (char*)buf, (size_t)buf_size, 0);

  if (res < 0)
  {
    XBMC_log(LOG_ERROR, "PVRClientVDR::ReadRecordedStream - failed");
    return 0;
  }

  if (res == 0)
  {
    XBMC_log(LOG_ERROR, "PVRClientVDR::ReadRecordedStream - eof");
    return 0;
  }

  currentPlayingRecordPosition += res;

  return res;
}

__int64 PVRClientVDR::SeekRecordedStream(__int64 pos, int whence)
{

  if (!m_transceiver->IsOpen())
  {
    return 0;
  }

  __int64 nextPos = currentPlayingRecordPosition;

  switch (whence)
  {

    case SEEK_SET:
      nextPos = pos;
      break;

    case SEEK_CUR:
      nextPos += pos;
      break;

    case SEEK_END:

      if (currentPlayingRecordBytes)
        nextPos = currentPlayingRecordBytes - pos;
      else
        return -1;

      break;

    case SEEK_POSSIBLE:
      return 1;

    default:
      return -1;
  }

  if (nextPos > currentPlayingRecordBytes)
  {
    return 0;
  }

  currentPlayingRecordPosition = nextPos;

  return currentPlayingRecordPosition;
}

__int64 PVRClientVDR::LengthRecordedStream(void)
{
  return currentPlayingRecordBytes;
}

bool PVRClientVDR::TeletextPagePresent(unsigned int channel, unsigned int Page, unsigned int subPage)
{
  vector<string> lines;
  int            code;
  char           buffer[1024];
  unsigned long  amountReceived;

  if (!m_transceiver->IsOpen() || m_socket_video == INVALID_SOCKET)
    return 0;

  sprintf(buffer, "LTXT PRESENT %u %u %u", channel, Page, subPage);
  if (m_transceiver->SendCommand(buffer, code, lines))
  {
    vector<string>::iterator it = lines.begin();
    string& data(*it);
    if (code == 250 && data == "PAGEPRESENT")
      return true;
  }
  
  return false;
}

bool PVRClientVDR::ReadTeletextPage(BYTE *buf, unsigned int channel, unsigned int Page, unsigned int subPage)
{
  vector<string> lines;
  int            code;
  char           buffer[1024];
  unsigned long  amountReceived;

  if (!m_transceiver->IsOpen() || m_socket_video == INVALID_SOCKET)
    return 0;

  sprintf(buffer, "LTXT GET %u %u %u", channel, Page, subPage);
  if (!m_transceiver->SendCommand(buffer, code, lines))
  {
    return false;
  }

  vector<string>::iterator it = lines.begin();
  string& data(*it);
  
  for (int i = 0; i < 40*24+12; i++)
  {
    buf[i] = atol(data.c_str());
    data.erase(0, data.find(" ")+1);
    if (data == "ENDDATA")
      return false;
  }
  return true;
}
