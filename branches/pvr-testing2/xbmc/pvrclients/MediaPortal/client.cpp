/*
 *      Copyright (C) 2005-2010 Team XBMC
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
#include "../../addons/include/xbmc_pvr_dll.h"
#include "pvrclient-mediaportal.h"

using namespace std;

cPVRClientMediaPortal *g_client = NULL;
bool m_bCreated         = false;
ADDON_STATUS curStatus  = STATUS_UNKNOWN;
int g_clientID          = -1;

/* User adjustable settings are saved here.
 * Default values are defined inside client.h
 * and exported to the other source files.
 */
std::string m_sHostname = DEFAULT_HOST;
int m_iPort             = DEFAULT_PORT;
bool m_bOnlyFTA         = DEFAULT_FTA_ONLY;
bool m_bRadioEnabled    = DEFAULT_RADIO;
bool m_bCharsetConv     = DEFAULT_CHARCONV;
int m_iConnectTimeout   = DEFAULT_TIMEOUT;
bool m_bNoBadChannels   = DEFAULT_BADCHANNELS;
bool m_bHandleMessages  = DEFAULT_HANDLE_MSG;
std::string g_szUserPath    = "";
std::string g_szClientPath  = "";
std::string g_sTVGroup      = "";
std::string g_sRadioGroup   = "";

extern "C" {

/***********************************************************
 * Standard AddOn related public library functions
 ***********************************************************/

//-- Create -------------------------------------------------------------------
// Called after loading of the dll, all steps to become Client functional
// must be performed here.
//-----------------------------------------------------------------------------
ADDON_STATUS Create(void* hdl, void* props)
{
  if (!props)
    return STATUS_UNKNOWN;

  PVR_PROPS* pvrprops = (PVR_PROPS*)props;

  XBMC_register_me(pvrprops->hdl);
  PVR_register_me(pvrprops->hdl);

  XBMC_log(LOG_DEBUG, "Creating MediaPortal PVR-Client");

  curStatus      = STATUS_UNKNOWN;
  g_client       = new cPVRClientMediaPortal();
  g_clientID     = pvrprops->clientID;
  g_szUserPath   = pvrprops->userpath;
  g_szClientPath = pvrprops->clientpath;

  /* Read setting "host" from settings.xml */
  char buffer[1024];

  if (XBMC_get_setting("host", &buffer))
    m_sHostname = buffer;
  else
  {
    /* If setting is unknown fallback to defaults */
    XBMC_log(LOG_ERROR, "Couldn't get 'host' setting, falling back to '127.0.0.1' as default");
    m_sHostname = DEFAULT_HOST;
  }

  /* Read setting "port" from settings.xml */
  if (!XBMC_get_setting("port", &m_iPort))
  {
    /* If setting is unknown fallback to defaults */
    XBMC_log(LOG_ERROR, "Couldn't get 'port' setting, falling back to '9596' as default");
    m_iPort = DEFAULT_PORT;
  }

  /* Read setting "ftaonly" from settings.xml */
  if (!XBMC_get_setting("ftaonly", &m_bOnlyFTA))
  {
    /* If setting is unknown fallback to defaults */
    XBMC_log(LOG_ERROR, "Couldn't get 'ftaonly' setting, falling back to 'false' as default");
    m_bOnlyFTA = DEFAULT_FTA_ONLY;
  }

  /* Read setting "useradio" from settings.xml */
  if (!XBMC_get_setting("useradio", &m_bRadioEnabled))
  {
    /* If setting is unknown fallback to defaults */
    XBMC_log(LOG_ERROR, "Couldn't get 'useradio' setting, falling back to 'true' as default");
    m_bRadioEnabled = DEFAULT_RADIO;
  }

  /* Read setting "convertchar" from settings.xml */
  if (!XBMC_get_setting("convertchar", &m_bCharsetConv))
  {
    /* If setting is unknown fallback to defaults */
    XBMC_log(LOG_ERROR, "Couldn't get 'convertchar' setting, falling back to 'false' as default");
    m_bCharsetConv = DEFAULT_CHARCONV;
  }

  /* Read setting "timeout" from settings.xml */
  if (!XBMC_get_setting("timeout", &m_iConnectTimeout))
  {
    /* If setting is unknown fallback to defaults */
    XBMC_log(LOG_ERROR, "Couldn't get 'timeout' setting, falling back to %i seconds as default", DEFAULT_TIMEOUT);
    m_iConnectTimeout = DEFAULT_TIMEOUT;
  }

  if (!XBMC_get_setting("tvgroup", &buffer))
  {
    /* If setting is unknown fallback to defaults */
    XBMC_log(LOG_ERROR, "Couldn't get 'tvgroup' setting, falling back to '' as default");
  } else {
    g_sTVGroup = buffer;
  }

  if (!XBMC_get_setting("radiogroup", &buffer))
  {
    /* If setting is unknown fallback to defaults */
    XBMC_log(LOG_ERROR, "Couldn't get 'tvgroup' setting, falling back to '' as default");
  } else {
    g_sRadioGroup = buffer;
  }

  /* Create connection to MediaPortal XBMC TV client */
  if (!g_client->Connect())
    curStatus = STATUS_LOST_CONNECTION;
  else
    curStatus = STATUS_OK;

  m_bCreated = true;
  return curStatus;
}

//-- Destroy ------------------------------------------------------------------
// Used during destruction of the client, all steps to do clean and safe Create
// again must be done.
//-----------------------------------------------------------------------------
void Destroy()
{
  if (m_bCreated)
  {
    g_client->Disconnect();

    delete g_client;
    g_client = NULL;

    m_bCreated = false;
  }
  curStatus = STATUS_UNKNOWN;
}

//-- GetStatus ----------------------------------------------------------------
// Report the current Add-On Status to XBMC
//-----------------------------------------------------------------------------
ADDON_STATUS GetStatus()
{
  return curStatus;
}

//-- HasSettings --------------------------------------------------------------
// Report "true", yes this AddOn have settings
//-----------------------------------------------------------------------------
bool HasSettings()
{
  return true;
}

addon_settings_t GetSettings()
{
  return NULL;
}

//-- SetSetting ---------------------------------------------------------------
// Called everytime a setting is changed by the user and to inform AddOn about
// new setting and to do required stuff to apply it.
//-----------------------------------------------------------------------------
ADDON_STATUS SetSetting(const char *settingName, const void *settingValue)
{
  string str = settingName;
  if (str == "host")
  {
    string tmp_sHostname;
    XBMC_log(LOG_INFO, "Changed Setting 'host' from %s to %s", m_sHostname.c_str(), (const char*) settingValue);
    tmp_sHostname = m_sHostname;
    m_sHostname = (const char*) settingValue;
    if (tmp_sHostname != m_sHostname)
      return STATUS_NEED_RESTART;
  }
  else if (str == "port")
  {
    XBMC_log(LOG_INFO, "Changed Setting 'port' from %u to %u", m_iPort, *(int*) settingValue);
    if (m_iPort != *(int*) settingValue)
    {
      m_iPort = *(int*) settingValue;
      return STATUS_NEED_RESTART;
    }
  }
  else if (str == "ftaonly")
  {
    XBMC_log(LOG_INFO, "Changed Setting 'ftaonly' from %u to %u", m_bOnlyFTA, *(bool*) settingValue);
    m_bOnlyFTA = *(bool*) settingValue;
  }
  else if (str == "useradio")
  {
    XBMC_log(LOG_INFO, "Changed Setting 'useradio' from %u to %u", m_bRadioEnabled, *(bool*) settingValue);
    m_bRadioEnabled = *(bool*) settingValue;
  }
  else if (str == "convertchar")
  {
    XBMC_log(LOG_INFO, "Changed Setting 'convertchar' from %u to %u", m_bCharsetConv, *(bool*) settingValue);
    m_bCharsetConv = *(bool*) settingValue;
  }
  else if (str == "timeout")
  {
    XBMC_log(LOG_INFO, "Changed Setting 'timeout' from %u to %u", m_iConnectTimeout, *(int*) settingValue);
    m_iConnectTimeout = *(int*) settingValue;
  }
  else if (str == "tvgroup")
  {
    XBMC_log(LOG_INFO, "Changed Setting 'tvgroup' from %s to %s", g_sTVGroup.c_str(), (const char*) settingValue);
    g_sTVGroup = (const char*) settingValue;
  }
  else if (str == "radiogroup")
  {
    XBMC_log(LOG_INFO, "Changed Setting 'radiogroup' from %s to %s", g_sTVGroup.c_str(), (const char*) settingValue);
    g_sTVGroup = (const char*) settingValue;
  }

  return STATUS_OK;
}

//-- Remove ------------------------------------------------------------------
// Used during destruction of the client, all steps to do clean and safe Create
// again must be done.
//-----------------------------------------------------------------------------
void Remove()
{
  if (m_bCreated)
  {
    g_client->Disconnect();

    delete g_client;
    g_client = NULL;

    m_bCreated = false;
  }
  curStatus = STATUS_UNKNOWN;
}

/***********************************************************
 * PVR Client AddOn specific public library functions
 ***********************************************************/

//-- GetProperties ------------------------------------------------------------
// Tell XBMC our requirements
//-----------------------------------------------------------------------------
PVR_ERROR GetProperties(PVR_SERVERPROPS* props)
{
  return g_client->GetProperties(props);
}

PVR_ERROR GetStreamProperties(PVR_STREAMPROPS* props)
{
  return PVR_ERROR_NOT_IMPLEMENTED;
}

//-- GetBackendName -----------------------------------------------------------
// Return the Name of the Backend
//-----------------------------------------------------------------------------
const char * GetBackendName()
{
  return g_client->GetBackendName();
}

//-- GetBackendVersion --------------------------------------------------------
// Return the Version of the Backend as String
//-----------------------------------------------------------------------------
const char * GetBackendVersion()
{
  return g_client->GetBackendVersion();
}

//-- GetConnectionString ------------------------------------------------------
// Return a String with connection info, if available
//-----------------------------------------------------------------------------
const char * GetConnectionString()
{
  return g_client->GetConnectionString();
}

//-- GetDriveSpace ------------------------------------------------------------
// Return the Total and Free Drive space on the PVR Backend
//-----------------------------------------------------------------------------
PVR_ERROR GetDriveSpace(long long *total, long long *used)
{
  return g_client->GetDriveSpace(total, used);
}

PVR_ERROR GetBackendTime(time_t *localTime, int *gmtOffset)
{
  return g_client->GetMPTVTime(localTime, gmtOffset);
}

int GetNumBouquets()
{
  return 0;
}

PVR_ERROR RequestEPGForChannel(PVRHANDLE handle, const PVR_CHANNEL &channel, time_t start, time_t end)
{
  return g_client->RequestEPGForChannel(channel, handle, start, end);
}

int GetNumChannels()
{
  return g_client->GetNumChannels();
}

PVR_ERROR RequestChannelList(PVRHANDLE handle, int radio)
{
  return g_client->RequestChannelList(handle, radio);
}

int GetNumRecordings(void)
{
  return g_client->GetNumRecordings();
}

PVR_ERROR RequestRecordingsList(PVRHANDLE handle)
{
  return g_client->RequestRecordingsList(handle);
}

PVR_ERROR DeleteRecording(const PVR_RECORDINGINFO &recinfo)
{
  return g_client->DeleteRecording(recinfo);
}

PVR_ERROR RenameRecording(const PVR_RECORDINGINFO &recinfo, const char *newname)
{
  return g_client->RenameRecording(recinfo, newname);
}

int GetNumTimers(void)
{
  return g_client->GetNumTimers();
}

PVR_ERROR RequestTimerList(PVRHANDLE handle)
{
  return g_client->RequestTimerList(handle);
}

PVR_ERROR AddTimer(const PVR_TIMERINFO &timerinfo)
{
  return g_client->AddTimer(timerinfo);
}

PVR_ERROR DeleteTimer(const PVR_TIMERINFO &timerinfo, bool force)
{
  return g_client->DeleteTimer(timerinfo, force);
}

PVR_ERROR RenameTimer(const PVR_TIMERINFO &timerinfo, const char *newname)
{
  return g_client->RenameTimer(timerinfo, newname);
}

PVR_ERROR UpdateTimer(const PVR_TIMERINFO &timerinfo)
{
  return g_client->UpdateTimer(timerinfo);
}

bool OpenLiveStream(const PVR_CHANNEL &channelinfo)
{
  return g_client->OpenLiveStream(channelinfo);
}

void CloseLiveStream()
{
  return g_client->CloseLiveStream();
}

int ReadLiveStream(BYTE* buf, int buf_size)
{
  return g_client->ReadLiveStream(buf, buf_size);
}

long long SeekLiveStream(long long pos, int whence)
{
  return -1;
}

long long LengthLiveStream(void)
{
  return -1;
}

int GetCurrentClientChannel()
{
  return g_client->GetCurrentClientChannel();
}

bool SwitchChannel(const PVR_CHANNEL &channelinfo)
{
  return g_client->SwitchChannel(channelinfo);
}

PVR_ERROR SignalQuality(PVR_SIGNALQUALITY &qualityinfo)
{
  return g_client->SignalQuality(qualityinfo);
}

bool OpenRecordedStream(const PVR_RECORDINGINFO &recinfo)
{
  return g_client->OpenRecordedStream(recinfo);
}

void CloseRecordedStream(void)
{
  return g_client->CloseRecordedStream();
}

int ReadRecordedStream(unsigned char* buf, int buf_size)
{
  return g_client->ReadRecordedStream(buf, buf_size);
}

long long SeekRecordedStream(long long pos, int whence)
{
  return g_client->SeekRecordedStream(pos, whence);
}

long long LengthRecordedStream(void)
{
  return g_client->LengthRecordedStream();
}

// MG: added for Mediaportal
const char * GetLiveStreamURL(const PVR_CHANNEL &channelinfo)
{
  return g_client->GetLiveStreamURL(channelinfo);
}

} //extern "C"
