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

#include "Application.h"
#include "GUISettings.h"
#include "Util.h"
#include "GUIWindowTV.h"
#include "GUIWindowManager.h"
#include "utils/GUIInfoManager.h"
#include "PVRManager.h"
#ifdef HAS_VIDEO_PLAYBACK
#include "cores/VideoRenderers/RenderManager.h"
#endif
#include "utils/log.h"
#include "LocalizeStrings.h"
#include "FileSystem/File.h"
#include "StringUtils.h"
#include "utils/TimeUtils.h"
#include "MusicInfoTag.h"
#include "Settings.h"

/* GUI Messages includes */
#include "GUIDialogOK.h"
#include "GUIDialogProgress.h"
#include "GUIDialogSelect.h"

#define CHANNELCHECKDELTA     600 // seconds before checking for changes inside channels list
#define TIMERCHECKDELTA       300 // seconds before checking for changes inside timers list
#define RECORDINGCHECKDELTA   450 // seconds before checking for changes inside recordings list
#define EPGCLEANUPCHECKDELTA  900 // seconds before checking for changes inside recordings list

using namespace std;
using namespace XFILE;
using namespace MUSIC_INFO;
using namespace ADDON;

/**********************************************************************
 * BEGIN OF CLASS **___ CPVRManager __________________________________*
 **                                                                  **/

/********************************************************************
 * CPVRManager constructor
 *
 * It creates the PVRManager, which mostly handle all PVR related
 * operations for XBMC
 ********************************************************************/
CPVRManager::CPVRManager()
{
  InitializeCriticalSection(&m_critSection);
  m_bFirstStart = true;
  CLog::Log(LOGDEBUG,"PVR: created");
}

/********************************************************************
 * CPVRManager destructor
 *
 * Destroy this class
 ********************************************************************/
CPVRManager::~CPVRManager()
{
  DeleteCriticalSection(&m_critSection);
  CLog::Log(LOGDEBUG,"PVR: destroyed");
}

/********************************************************************
 * CPVRManager Start
 *
 * PVRManager Startup
 ********************************************************************/
void CPVRManager::Start()
{
  /* First stop and remove any clients */
  if (!m_clients.empty())
    Stop();

  /* Check if TV is enabled under Settings->Video->TV->Enable */
  if (!g_guiSettings.GetBool("pvrmanager.enabled"))
    return;

  CLog::Log(LOGNOTICE, "PVR: PVRManager starting");

  /* Reset Member variables and System Info swap counters */
  m_hasRecordings           = false;
  m_isRecording             = false;
  m_hasTimers               = false;
  m_CurrentGroupID          = -1;
  m_currentPlayingChannel   = NULL;
  m_currentPlayingRecording = NULL;
  m_PreviousChannel[0]      = -1;
  m_PreviousChannel[1]      = -1;
  m_PreviousChannelIndex    = 0;
  m_infoToggleStart         = NULL;
  m_infoToggleCurrent       = 0;
  m_recordingToggleStart    = NULL;
  m_recordingToggleCurrent  = 0;
  m_LastChannel             = 0;

  /* Discover, load and create chosen Client add-on's. */
  CAddonMgr::Get()->RegisterAddonMgrCallback(ADDON_PVRDLL, this);
  if (!LoadClients())
  {
    CLog::Log(LOGERROR, "PVR: couldn't load any clients");
    return;
  }

  /* Create the supervisor thread to do all background activities */
  Create();
  SetName("XBMC PVR Supervisor");
  SetPriority(-15);
  CLog::Log(LOGNOTICE, "PVR: PVRManager started. Clients loaded = %u", (unsigned int) m_clients.size());
  return;
}

/********************************************************************
 * CPVRManager Stop
 *
 * PVRManager shutdown
 ********************************************************************/
void CPVRManager::Stop()
{
  CLog::Log(LOGNOTICE, "PVR: PVRManager stoping");
  StopThread();

  m_clients.clear();
  m_clientsProps.clear();
  return;
}

/********************************************************************
 * CPVRManager LoadClients
 *
 * Load the client drivers and doing the startup.
 ********************************************************************/
bool CPVRManager::LoadClients()
{
  /* Get all PVR Add on's */
  VECADDONS addons;
  if (!CAddonMgr::Get()->GetAddons(ADDON_PVRDLL, addons, CONTENT_NONE, true))
    return false;

  m_database.Open();

  /* load the clients */
  for (unsigned i=0; i < addons.size(); i++)
  {
    const AddonPtr clientAddon = addons.at(i);

    /* Add client to TV-Database to identify different backend types,
     * if client is already added his id is given.
     */
    long clientID = m_database.AddClient(clientAddon->Name(), clientAddon->UUID());
    if (clientID == -1)
    {
      CLog::Log(LOGERROR, "PVR: Can't Add/Get PVR Client '%s' to to TV Database", clientAddon->Name().c_str());
      continue;
    }

    /* Load the Client libraries, and inside them into client list if
     * success. Client initialization is also performed during loading.
     */
    boost::shared_ptr<CPVRClient> addon = boost::dynamic_pointer_cast<CPVRClient>(clientAddon);
    if (addon && addon->Create(clientID, this))
    {
      m_clients.insert(std::make_pair(clientID, addon));
    }
  }

  m_database.Close();

  // Request each client's basic properties
  GetClientProperties();

  return !m_clients.empty();
}

/********************************************************************
 * CPVRManager GetClientProperties
 *
 * Load the client Properties for every client
 ********************************************************************/
void CPVRManager::GetClientProperties()
{
  m_clientsProps.clear();
  CLIENTMAPITR itr = m_clients.begin();
  while (itr != m_clients.end())
  {
    GetClientProperties((*itr).first);
    itr++;
  }
}

/********************************************************************
 * CPVRManager GetClientProperties
 *
 * Load the client Properties for the given client ID in the
 * Properties list
 ********************************************************************/
void CPVRManager::GetClientProperties(long clientID)
{
  PVR_SERVERPROPS props;
  if (m_clients[clientID]->GetProperties(&props) == PVR_ERROR_NO_ERROR)
  {
    // store the client's properties
    m_clientsProps.insert(std::make_pair(clientID, props));
  }
}

/********************************************************************
 * CPVRManager GetFirstClientID
 *
 * Returns the first loaded client ID
 ********************************************************************/
unsigned long CPVRManager::GetFirstClientID()
{
  CLIENTMAPITR itr = m_clients.begin();
  return m_clients[(*itr).first]->GetID();
}

/********************************************************************
 * CPVRManager OnClientMessage
 *
 * Callback function from Client driver to inform about changed
 * timers, channels, recordings or epg.
 ********************************************************************/
void CPVRManager::OnClientMessage(const long clientID, const PVR_EVENT clientEvent, const char* msg)
{
  /* here the manager reacts to messages sent from any of the clients via the IPVRClientCallback */
  CStdString clientName = m_clients[clientID]->GetBackendName() + ":" + m_clients[clientID]->GetConnectionString();
  switch (clientEvent)
  {
    case PVR_EVENT_UNKNOWN:
      CLog::Log(LOGDEBUG, "%s - PVR: client_%ld unknown event : %s", __FUNCTION__, clientID, msg);
      break;

    case PVR_EVENT_TIMERS_CHANGE:
      {
        CLog::Log(LOGDEBUG, "%s - PVR: client_%ld timers changed", __FUNCTION__, clientID);
        PVRTimers.Update();
        SyncInfo();

        CGUIWindowTV *pTVWin = (CGUIWindowTV *)g_windowManager.GetWindow(WINDOW_TV);
        if (pTVWin)
        pTVWin->UpdateData(TV_WINDOW_TIMERS);
      }
      break;

    case PVR_EVENT_RECORDINGS_CHANGE:
      {
        CLog::Log(LOGDEBUG, "%s - PVR: client_%ld recording list changed", __FUNCTION__, clientID);
        SyncInfo();

        CGUIWindowTV *pTVWin = (CGUIWindowTV *)g_windowManager.GetWindow(WINDOW_TV);
        if (pTVWin)
        pTVWin->UpdateData(TV_WINDOW_RECORDINGS);
      }
      break;

    case PVR_EVENT_CHANNELS_CHANGE:
      {
        CLog::Log(LOGDEBUG, "%s - PVR: client_%ld channel list changed", __FUNCTION__, clientID);
        SyncInfo();

        CGUIWindowTV *pTVWin = (CGUIWindowTV *)g_windowManager.GetWindow(WINDOW_TV);
        if (pTVWin)
        {
          pTVWin->UpdateData(TV_WINDOW_CHANNELS_TV);
          pTVWin->UpdateData(TV_WINDOW_CHANNELS_RADIO);
        }
      }
      break;

    default:
      break;
  }
}

/********************************************************************
 * CPVRManager RequestRestart
 *
 * Restart a client driver
 ********************************************************************/
bool CPVRManager::RequestRestart(const IAddon* addon, bool datachanged)
{
  if (!addon)
    return false;

  CLog::Log(LOGINFO, "PVR: requested restart of clientName:%s, clientGUID:%s", addon->Name().c_str(), addon->UUID().c_str());
  CLIENTMAPITR itr = m_clients.begin();
  while (itr != m_clients.end())
  {
    if (m_clients[(*itr).first]->UUID() == addon->UUID())
    {
      if (m_clients[(*itr).first]->Name() == addon->Name())
      {
        CLog::Log(LOGINFO, "PVR: restarting clientName:%s, clientGUID:%s", addon->Name().c_str(), addon->UUID().c_str());
        StopThread();
        m_clients[(*itr).first]->ReCreate();
        Create();
      }
    }
    itr++;
  }
  return true;
}

/********************************************************************
 * CPVRManager RequestRemoval
 *
 * Unload a client driver
 ********************************************************************/
bool CPVRManager::RequestRemoval(const IAddon* addon)
{
  if (!addon)
    return false;

  CLog::Log(LOGINFO, "PVR: requested removal of clientName:%s, clientGUID:%s", addon->Name().c_str(), addon->UUID().c_str());
  CLIENTMAPITR itr = m_clients.begin();
  while (itr != m_clients.end())
  {
    if (m_clients[(*itr).first]->UUID() == addon->UUID())
    {
      if (m_clients[(*itr).first]->Name() == addon->Name())
      {
        CLog::Log(LOGINFO, "PVR: removing clientName:%s, clientGUID:%s", addon->Name().c_str(), addon->UUID().c_str());
        StopThread();

        m_clients[(*itr).first]->Destroy();
        m_clients.erase((*itr).first);
        if (!m_clients.empty())
          Create();

        return true;
      }
    }
    itr++;
  }

  return false;
}


/*************************************************************/
/** INTERNAL FUNCTIONS                                      **/
/*************************************************************/

/*************************************************************/
/** PVRManager Update and control thread                    **/
/*************************************************************/

void CPVRManager::Process()
{
  /* Get TV Channels from Backends */
  PVRChannelsTV.Load(false);

  /* Get Radio Channels from Backends */
  PVRChannelsRadio.Load(true);

  /* Load the TV Channel group lists */
  PVRChannelGroupsTV.Load(false);

  /* Load the Radio Channel group lists */
  PVRChannelGroupsRadio.Load(true);

  /* Continue last watched channel after first startup */
  if (m_bFirstStart && g_guiSettings.GetInt("pvrplayback.startlast") != START_LAST_CHANNEL_OFF)
  {
    CLog::Log(LOGNOTICE,"PVR: Try to continue last channel");
    m_bFirstStart = false;

    m_database.Open();
    int lastChannel = m_database.GetLastChannel();
    if (lastChannel > 0)
    {
      cPVRChannelInfoTag *tag = cPVRChannels::GetByChannelIDFromAll(lastChannel);
      if (tag)
      {
        cPVRChannels *channels;
        if (!tag->IsRadio())
          channels = &PVRChannelsTV;
        else
          channels = &PVRChannelsRadio;

        if (g_guiSettings.GetInt("pvrplayback.startlast") == START_LAST_CHANNEL_MIN)
          g_settings.m_bStartVideoWindowed = true;

        if (g_application.PlayFile(CFileItem(channels->at(tag->Number()-1))))
          CLog::Log(LOGNOTICE,"PVR: Continue playback of channel '%s'", tag->Name().c_str());
        else
          CLog::Log(LOGERROR,"PVR: Channel '%s' can't continued", tag->Name().c_str());
      }
      else
        CLog::Log(LOGERROR,"PVR: Can't find channel (ID=%i) for continue on startup", lastChannel);
    }
    m_database.Close();
  }

  /* Get Timers from Backends */
  PVRTimers.Load();

  /* Get Recordings from Backend */
  PVRRecordings.Load();

  /* Get Epg's from Backend */
  PVREpgs.Load();

  int Now = CTimeUtils::GetTimeMS()/1000;
  int LastEPGCleanup       = Now;
  m_LastTVChannelCheck     = Now;
  m_LastRadioChannelCheck  = Now+CHANNELCHECKDELTA/2;
  m_LastRecordingsCheck    = Now;
  m_LastEPGUpdate          = Now;
  /* Check the last EPG scan date if XBMC is restarted to prevent a rescan if
     the time is not longer as one hour ago */
  m_database.Open();
  if (m_database.GetLastEPGScanTime() < CDateTime::GetCurrentDateTime()-CDateTimeSpan(0,1,0,0))
    m_LastEPGScan          = Now-(g_guiSettings.GetInt("pvrepg.epgscan")*60*60)+120;
  else
    m_LastEPGScan          = Now;
  m_database.Close();

  while (!m_bStop)
  {
    Now = CTimeUtils::GetTimeMS()/1000;

    /* Check for new or updated TV Channels */
    if (Now - m_LastTVChannelCheck > CHANNELCHECKDELTA) // don't do this too often
    {
      CLog::Log(LOGDEBUG,"PVR: Updating TV Channel list");
      PVRChannelsTV.Update();
      m_LastTVChannelCheck = Now;
    }

    /* Check for new or updated Radio Channels */
    if (Now - m_LastRadioChannelCheck > CHANNELCHECKDELTA) // don't do this too often
    {
      CLog::Log(LOGDEBUG,"PVR: Updating Radio Channel list");
      PVRChannelsRadio.Update();
      m_LastRadioChannelCheck = Now;
    }

    /* Check for new or updated Recordings */
    if (Now - m_LastRecordingsCheck > RECORDINGCHECKDELTA) // don't do this too often
    {
      CLog::Log(LOGDEBUG,"PVR: Updating Recordings list");
      PVRRecordings.Update(true);
      m_LastRecordingsCheck = Now;
    }

    /* Check for new or updated EPG entries */
    if (Now - m_LastEPGScan > g_guiSettings.GetInt("pvrepg.epgscan")*60*60) // don't do this too often
    {
      PVREpgs.Update(true);
      m_LastEPGScan   = Now;
      m_LastEPGUpdate = Now;  // Data is also updated during scan
      LastEPGCleanup  = Now;
    }
    else if (Now - m_LastEPGUpdate > g_guiSettings.GetInt("pvrepg.epgupdate")*60) // don't do this too often
    {
      PVREpgs.Update(false);
      m_LastEPGUpdate = Now;
      LastEPGCleanup  = Now;
    }
    else if (Now - LastEPGCleanup > EPGCLEANUPCHECKDELTA) // don't do this too often
    {
      /* Cleanup EPG Data */
      PVREpgs.Cleanup();
      LastEPGCleanup = Now;
    }

    EnterCriticalSection(&m_critSection);

    /* Get Signal information of the current playing channel */
    if (m_currentPlayingChannel && g_guiSettings.GetBool("pvrplayback.signalquality"))
    {
      m_clients[m_currentPlayingChannel->GetPVRChannelInfoTag()->ClientID()]->SignalQuality(m_qualityInfo);
    }
    LeaveCriticalSection(&m_critSection);

    Sleep(1000);
  }

  /* if a channel or recording is playing stop it first */
  if (m_currentPlayingChannel || m_currentPlayingRecording)
    g_application.StopPlaying();

  /* Remove Epg's from Memory */
  PVREpgs.Unload();

  /* Remove recordings from Memory */
  PVRRecordings.Unload();

  /* Remove Timers from Memory */
  PVRTimers.Unload();

  /* Remove TV Channel groups from Memory */
  PVRChannelGroupsTV.Unload();

  /* Remove Radio Channel groups from Memory */
  PVRChannelGroupsRadio.Unload();

  /* Remove Radio Channels from Memory */
  PVRChannelsRadio.Unload();

  /* Remove TV Channels from Memory */
  PVRChannelsTV.Unload();
}


/*************************************************************/
/** GUIInfoManager FUNCTIONS                                **/
/*************************************************************/

/********************************************************************
 * CPVRManager SyncInfo
 *
 * Synchronize InfoManager related stuff
 ********************************************************************/
void CPVRManager::SyncInfo()
{
  EnterCriticalSection(&m_critSection);

  PVRRecordings.GetNumRecordings() > 0 ? m_hasRecordings = true : m_hasRecordings = false;
  PVRTimers.GetNumTimers()         > 0 ? m_hasTimers     = true : m_hasTimers = false;
  m_isRecording = false;
  cPVRTimerInfoTag *nextTimer = NULL;

  m_nowRecordingTitle.clear();
  m_nowRecordingChannel.clear();
  m_nowRecordingDateTime.clear();
  m_nextRecordingTitle.clear();
  m_nextRecordingChannel.clear();
  m_nextRecordingDateTime.clear();

  if (m_hasTimers)
  {
    for (unsigned int i = 0; i < PVRTimers.size(); ++i)
    {
      if (PVRTimers[i].Active() && (PVRTimers[i].Start() < CDateTime::GetCurrentDateTime() && PVRTimers[i].Stop() > CDateTime::GetCurrentDateTime()))
      {
        m_nowRecordingTitle.push_back(PVRTimers[i].Title());
        m_nowRecordingChannel.push_back(PVRChannelsTV.GetNameForChannel(PVRTimers[i].Number()));
        m_nowRecordingDateTime.push_back(PVRTimers[i].Start().GetAsLocalizedDateTime(false, false));
        m_isRecording = true;
      }
      else if (PVRTimers[i].Active())
      {
        if (!nextTimer || nextTimer->Start() > PVRTimers[i].Start())
          nextTimer = &PVRTimers[i];
      }
    }

    if (nextTimer)
    {
      m_nextRecordingTitle    = nextTimer->Title();
      m_nextRecordingChannel  = PVRChannelsTV.GetNameForChannel(nextTimer->Number());
      m_nextRecordingDateTime = nextTimer->Start().GetAsLocalizedDateTime(false, false);
    }
  }

  LeaveCriticalSection(&m_critSection);
}

/********************************************************************
 * CPVRManager TranslateCharInfo
 *
 * Returns a GUIInfoManager Character String
 ********************************************************************/
#define INFO_TOGGLE_TIME    1500
const char* CPVRManager::TranslateCharInfo(DWORD dwInfo)
{
  if      (dwInfo == PVR_NOW_RECORDING_TITLE)
  {
    if (m_recordingToggleStart == 0)
    {
      SyncInfo();
      m_recordingToggleStart = CTimeUtils::GetTimeMS();
      m_recordingToggleCurrent = 0;
    }
    else
    {
      if (CTimeUtils::GetTimeMS() - m_recordingToggleStart > INFO_TOGGLE_TIME)
      {
        SyncInfo();
        if (m_nowRecordingTitle.size() > 0)
        {
          m_recordingToggleCurrent++;
          if (m_recordingToggleCurrent > m_nowRecordingTitle.size()-1)
            m_recordingToggleCurrent = 0;

          m_recordingToggleStart = CTimeUtils::GetTimeMS();
        }
      }
    }

    if (m_nowRecordingTitle.size() > 0)
      return m_nowRecordingTitle[m_recordingToggleCurrent];
    else
      return "";
  }
  else if (dwInfo == PVR_NOW_RECORDING_CHANNEL)
  {
    if (m_nowRecordingChannel.size() > 0)
      return m_nowRecordingChannel[m_recordingToggleCurrent];
    else
      return "";
  }
  else if (dwInfo == PVR_NOW_RECORDING_DATETIME)
  {
    if (m_nowRecordingDateTime.size() > 0)
      return m_nowRecordingDateTime[m_recordingToggleCurrent];
    else
      return "";
  }
  else if (dwInfo == PVR_NEXT_RECORDING_TITLE)    return m_nextRecordingTitle;
  else if (dwInfo == PVR_NEXT_RECORDING_CHANNEL)  return m_nextRecordingChannel;
  else if (dwInfo == PVR_NEXT_RECORDING_DATETIME) return m_nextRecordingDateTime;
  else if (dwInfo == PVR_BACKEND_NAME)            return m_backendName;
  else if (dwInfo == PVR_BACKEND_VERSION)         return m_backendVersion;
  else if (dwInfo == PVR_BACKEND_HOST)            return m_backendHost;
  else if (dwInfo == PVR_BACKEND_DISKSPACE)       return m_backendDiskspace;
  else if (dwInfo == PVR_BACKEND_CHANNELS)        return m_backendChannels;
  else if (dwInfo == PVR_BACKEND_TIMERS)          return m_backendTimers;
  else if (dwInfo == PVR_BACKEND_RECORDINGS)      return m_backendRecordings;
  else if (dwInfo == PVR_BACKEND_NUMBER)
  {
    if (m_infoToggleStart == 0)
    {
      m_infoToggleStart = CTimeUtils::GetTimeMS();
      m_infoToggleCurrent = 0;
    }
    else
    {
      if (CTimeUtils::GetTimeMS() - m_infoToggleStart > INFO_TOGGLE_TIME)
      {
        if (m_clients.size() > 0)
        {
          m_infoToggleCurrent++;
          if (m_infoToggleCurrent > m_clients.size()-1)
            m_infoToggleCurrent = 0;

          CLIENTMAPITR itr = m_clients.begin();
          for (unsigned int i = 0; i < m_infoToggleCurrent; i++)
            itr++;

          long long kBTotal = 0;
          long long kBUsed  = 0;
          if (m_clients[(*itr).first]->GetDriveSpace(&kBTotal, &kBUsed) == PVR_ERROR_NO_ERROR)
          {
            kBTotal /= 1024; // Convert to MBytes
            kBUsed /= 1024;  // Convert to MBytes
            m_backendDiskspace.Format("%s %0.f GByte - %s: %0.f GByte", g_localizeStrings.Get(19105), (float) kBTotal / 1024, g_localizeStrings.Get(156), (float) kBUsed / 1024);
          }
          else
          {
            m_backendDiskspace = g_localizeStrings.Get(19055);
          }

          int NumChannels = m_clients[(*itr).first]->GetNumChannels();
          if (NumChannels >= 0)
            m_backendChannels.Format("%i", NumChannels);
          else
            m_backendChannels = g_localizeStrings.Get(161);

          int NumTimers = m_clients[(*itr).first]->GetNumTimers();
          if (NumTimers >= 0)
            m_backendTimers.Format("%i", NumTimers);
          else
            m_backendTimers = g_localizeStrings.Get(161);

          int NumRecordings = m_clients[(*itr).first]->GetNumRecordings();
          if (NumRecordings >= 0)
            m_backendRecordings.Format("%i", NumRecordings);
          else
            m_backendRecordings = g_localizeStrings.Get(161);

          m_backendName         = m_clients[(*itr).first]->GetBackendName();
          m_backendVersion      = m_clients[(*itr).first]->GetBackendVersion();
          m_backendHost         = m_clients[(*itr).first]->GetConnectionString();
        }
        else
        {
          m_backendName         = "";
          m_backendVersion      = "";
          m_backendHost         = "";
          m_backendDiskspace    = "";
          m_backendTimers       = "";
          m_backendRecordings   = "";
          m_backendChannels     = "";
        }
        m_infoToggleStart = CTimeUtils::GetTimeMS();
      }
    }

    static CStdString backendClients;
    if (m_clients.size() > 0)
      backendClients.Format("%u %s %u", m_infoToggleCurrent+1, g_localizeStrings.Get(20163), m_clients.size());
    else
      backendClients = g_localizeStrings.Get(14023);

    return backendClients;
  }
  else if (dwInfo == PVR_TOTAL_DISKSPACE)
  {
    long long kBTotal = 0;
    long long kBUsed  = 0;
    CLIENTMAPITR itr = m_clients.begin();
    while (itr != m_clients.end())
    {
      long long clientKBTotal = 0;
      long long clientKBUsed  = 0;

      if (m_clients[(*itr).first]->GetDriveSpace(&clientKBTotal, &clientKBUsed) == PVR_ERROR_NO_ERROR)
      {
        kBTotal += clientKBTotal;
        kBUsed += clientKBUsed;
      }
      itr++;
    }
    kBTotal /= 1024; // Convert to MBytes
    kBUsed /= 1024;  // Convert to MBytes
    m_totalDiskspace.Format("%s %0.f GByte - %s: %0.f GByte", g_localizeStrings.Get(19105), (float) kBTotal / 1024, g_localizeStrings.Get(156), (float) kBUsed / 1024);
    return m_totalDiskspace;
  }
  else if (dwInfo == PVR_NEXT_TIMER)
  {
    cPVRTimerInfoTag *next = PVRTimers.GetNextActiveTimer();
    if (next != NULL)
    {
      m_nextTimer.Format("%s %s %s %s", g_localizeStrings.Get(19106)
                         , next->Start().GetAsLocalizedDate(true)
                         , g_localizeStrings.Get(19107)
                         , next->Start().GetAsLocalizedTime("HH:mm", false));
      return m_nextTimer;
    }
  }
  else if (dwInfo == PVR_PLAYING_DURATION)
  {
    StringUtils::SecondsToTimeString(GetTotalTime()/1000, m_playingDuration, TIME_FORMAT_GUESS);
    return m_playingDuration.c_str();
  }
  else if (dwInfo == PVR_PLAYING_TIME)
  {
    StringUtils::SecondsToTimeString(GetStartTime()/1000, m_playingTime, TIME_FORMAT_GUESS);
    return m_playingTime.c_str();
  }
  else if (dwInfo == PVR_ACTUAL_STREAM_VIDEO_BR)
  {
    static CStdString strBitrate = "";
    if (m_qualityInfo.video_bitrate > 0)
      strBitrate.Format("%.2f Mbit/s", m_qualityInfo.video_bitrate);
    return strBitrate;
  }
  else if (dwInfo == PVR_ACTUAL_STREAM_AUDIO_BR)
  {
    CStdString strBitrate = "";
    if (m_qualityInfo.audio_bitrate > 0)
      strBitrate.Format("%.0f kbit/s", m_qualityInfo.audio_bitrate);
    return strBitrate;
  }
  else if (dwInfo == PVR_ACTUAL_STREAM_DOLBY_BR)
  {
    CStdString strBitrate = "";
    if (m_qualityInfo.dolby_bitrate > 0)
      strBitrate.Format("%.0f kbit/s", m_qualityInfo.dolby_bitrate);
    return strBitrate;
  }
  else if (dwInfo == PVR_ACTUAL_STREAM_SIG)
  {
    CStdString strSignal = "";
    if (m_qualityInfo.signal > 0)
      strSignal.Format("%d %%", m_qualityInfo.signal / 655);
    return strSignal;
  }
  else if (dwInfo == PVR_ACTUAL_STREAM_SNR)
  {
    CStdString strSNR = "";
    if (m_qualityInfo.snr > 0)
      strSNR.Format("%d %%", m_qualityInfo.snr / 655);
    return strSNR;
  }
  else if (dwInfo == PVR_ACTUAL_STREAM_BER)
  {
    CStdString strBER;
    strBER.Format("%08X", m_qualityInfo.ber);
    return strBER;
  }
  else if (dwInfo == PVR_ACTUAL_STREAM_UNC)
  {
    CStdString strUNC;
    strUNC.Format("%08X", m_qualityInfo.unc);
    return strUNC;
  }
  else if (dwInfo == PVR_ACTUAL_STREAM_CLIENT)
  {
    return m_playingClientName;
  }
  else if (dwInfo == PVR_ACTUAL_STREAM_DEVICE)
  {
    CStdString string = m_qualityInfo.frontend_name;
    if (string == "")
      return g_localizeStrings.Get(13205);
    else
      return string;
  }
  else if (dwInfo == PVR_ACTUAL_STREAM_STATUS)
  {
    CStdString string = m_qualityInfo.frontend_status;
    if (string == "")
      return g_localizeStrings.Get(13205);
    else
      return string;
  }
  else if (dwInfo == PVR_ACTUAL_STREAM_CRYPTION)
  {
    if (m_currentPlayingChannel)
      return m_currentPlayingChannel->GetPVRChannelInfoTag()->EncryptionName();
  }
  return "";
}

/********************************************************************
 * CPVRManager TranslateIntInfo
 *
 * Returns a GUIInfoManager integer value
 ********************************************************************/
int CPVRManager::TranslateIntInfo(DWORD dwInfo)
{
  if (dwInfo == PVR_PLAYING_PROGRESS)
  {
    return (float)(((float)GetStartTime() / GetTotalTime()) * 100);
  }
  else if (dwInfo == PVR_ACTUAL_STREAM_SIG_PROGR)
  {
    return (float)(((float)m_qualityInfo.signal / 0xFFFF) * 100);
  }
  else if (dwInfo == PVR_ACTUAL_STREAM_SNR_PROGR)
  {
    return (float)(((float)m_qualityInfo.snr / 0xFFFF) * 100);
  }
  return 0;
}

/********************************************************************
 * CPVRManager TranslateBoolInfo
 *
 * Returns a GUIInfoManager boolean value
 ********************************************************************/
bool CPVRManager::TranslateBoolInfo(DWORD dwInfo)
{
  if (dwInfo == PVR_IS_RECORDING)
    return m_isRecording;
  else if (dwInfo == PVR_HAS_TIMER)
    return m_hasTimers;
  else if (dwInfo == PVR_IS_PLAYING_TV)
    return IsPlayingTV();
  else if (dwInfo == PVR_IS_PLAYING_RADIO)
    return IsPlayingRadio();
  else if (dwInfo == PVR_IS_PLAYING_RECORDING)
    return IsPlayingRecording();
  else if (dwInfo == PVR_ACTUAL_STREAM_ENCRYPTED)
  {
    if (m_currentPlayingChannel)
      return m_currentPlayingChannel->GetPVRChannelInfoTag()->IsEncrypted();
  }

  return false;
}


/*************************************************************/
/** GENERAL FUNCTIONS                                       **/
/*************************************************************/

void CPVRManager::StartChannelScan()
{
  std::vector<long> clients;
  long scanningClientID = -1;

  CLIENTMAPITR itr = m_clients.begin();
  while (itr != m_clients.end())
  {
    if (m_clients[(*itr).first]->ReadyToUse() && GetClientProps(m_clients[(*itr).first]->GetID())->SupportChannelScan)
    {
      clients.push_back(m_clients[(*itr).first]->GetID());
    }
    itr++;
  }

  if (clients.size() > 1)
  {
    CGUIDialogSelect* pDialog= (CGUIDialogSelect*)g_windowManager.GetWindow(WINDOW_DIALOG_SELECT);

    pDialog->Reset();
    pDialog->SetHeading(19119);

    for (unsigned int i = 0; i < clients.size(); i++)
    {
      pDialog->Add(m_clients[clients[i]]->GetBackendName() + ":" + m_clients[clients[i]]->GetConnectionString());
    }

    pDialog->DoModal();

    int selection = pDialog->GetSelectedLabel();
    if (selection >= 0)
    {
      scanningClientID = clients[selection];
    }

  }
  else if (clients.size() == 1)
  {
    scanningClientID = clients[0];
  }

  if (scanningClientID < 0)
  {
    CGUIDialogOK::ShowAndGetInput(19033,0,19192,0);
    return;
  }

  CLog::Log(LOGNOTICE,"PVR: Starting to scan for channels on client %s:%s", m_clients[scanningClientID]->GetBackendName().c_str(), m_clients[scanningClientID]->GetConnectionString().c_str());
  long perfCnt = CTimeUtils::GetTimeMS();

  PVREpgs.InihibitUpdate(true);

  if (m_currentPlayingRecording || m_currentPlayingChannel)
  {
    CLog::Log(LOGNOTICE,"PVR: Is playing data, stopping playback");
    g_application.StopPlaying();
  }
  /* Stop the supervisor thread */
  StopThread();

  if (m_clients[scanningClientID]->StartChannelScan() != PVR_ERROR_NO_ERROR)
  {
    CGUIDialogOK::ShowAndGetInput(19111,0,19193,0);
  }

  /* Create the supervisor thread again */
  Create();
  CLog::Log(LOGNOTICE, "PVR: Channel scan finished after %li.%li seconds", (CTimeUtils::GetTimeMS()-perfCnt)/1000, (CTimeUtils::GetTimeMS()-perfCnt)%1000);
}

void CPVRManager::ResetDatabase()
{
  CLog::Log(LOGNOTICE,"PVR: TV Database is now set to it's initial state");

  CGUIDialogProgress* pDlgProgress = (CGUIDialogProgress*)g_windowManager.GetWindow(WINDOW_DIALOG_PROGRESS);
  pDlgProgress->SetLine(0, "");
  pDlgProgress->SetLine(1, g_localizeStrings.Get(19186));
  pDlgProgress->SetLine(2, "");
  pDlgProgress->StartModal();
  pDlgProgress->Progress();

  PVREpgs.InihibitUpdate(true);

  if (m_currentPlayingRecording || m_currentPlayingChannel)
  {
    CLog::Log(LOGNOTICE,"PVR: Is playing data, stopping playback");
    g_application.StopPlaying();
  }
  pDlgProgress->SetPercentage(10);

  Stop();
  pDlgProgress->SetPercentage(20);

  m_database.Open();
  m_database.EraseEPG();
  pDlgProgress->SetPercentage(30);

  m_database.EraseChannelLinkageMap();
  pDlgProgress->SetPercentage(40);

  m_database.EraseChannelGroups();
  pDlgProgress->SetPercentage(50);

  m_database.EraseRadioChannelGroups();
  pDlgProgress->SetPercentage(60);

  m_database.EraseChannels();
  pDlgProgress->SetPercentage(70);

  m_database.EraseChannelSettings();
  pDlgProgress->SetPercentage(80);

  m_database.EraseClients();
  pDlgProgress->SetPercentage(90);

  m_database.Close();
  CLog::Log(LOGNOTICE,"PVR: TV Database reset finished, starting PVR Subsystem again");
  Start();
  pDlgProgress->SetPercentage(100);
  pDlgProgress->Close();
}

void CPVRManager::ResetEPG()
{
  CLog::Log(LOGNOTICE,"PVR: EPG is being erased");

  CGUIDialogProgress* pDlgProgress = (CGUIDialogProgress*)g_windowManager.GetWindow(WINDOW_DIALOG_PROGRESS);
  pDlgProgress->SetLine(0, "");
  pDlgProgress->SetLine(1, g_localizeStrings.Get(19186));
  pDlgProgress->SetLine(2, "");
  pDlgProgress->StartModal();
  pDlgProgress->Progress();

  PVREpgs.InihibitUpdate(true);

  if (m_currentPlayingRecording || m_currentPlayingChannel)
  {
    CLog::Log(LOGNOTICE,"PVR: Is playing data, stopping playback");
    g_application.StopPlaying();
  }
  pDlgProgress->SetPercentage(10);

  Stop();
  pDlgProgress->SetPercentage(30);

  m_database.Open();
  pDlgProgress->SetPercentage(50);

  m_database.EraseEPG();
  pDlgProgress->SetPercentage(70);

  m_database.Close();
  CLog::Log(LOGNOTICE,"PVR: EPG reset finished, starting PVR Subsystem again");
  Start();
  pDlgProgress->SetPercentage(100);
  pDlgProgress->Close();
}

bool CPVRManager::IsPlayingTV()
{
  if (!m_currentPlayingChannel)
    return false;

  return !m_currentPlayingChannel->GetPVRChannelInfoTag()->IsRadio();
}

bool CPVRManager::IsPlayingRadio()
{
  if (!m_currentPlayingChannel)
    return false;

  return m_currentPlayingChannel->GetPVRChannelInfoTag()->IsRadio();
}

bool CPVRManager::IsPlayingRecording()
{
  if (m_currentPlayingRecording)
    return false;
  else
    return true;
}

PVR_SERVERPROPS *CPVRManager::GetCurrentClientProps()
{
  if (m_currentPlayingChannel)
    return &m_clientsProps[m_currentPlayingChannel->GetPVRChannelInfoTag()->ClientID()];
  else if (m_currentPlayingRecording)
    return &m_clientsProps[m_currentPlayingRecording->GetPVRRecordingInfoTag()->ClientID()];
  else
    return NULL;
}

long CPVRManager::GetCurrentPlayingClientID()
{
  if (m_currentPlayingChannel)
    return m_currentPlayingChannel->GetPVRChannelInfoTag()->ClientID();
  else if (m_currentPlayingRecording)
    return m_currentPlayingRecording->GetPVRRecordingInfoTag()->ClientID();
  else
    return -1;
}

PVR_STREAMPROPS *CPVRManager::GetCurrentStreamProps()
{
  if (m_currentPlayingChannel)
  {
    int cid = m_currentPlayingChannel->GetPVRChannelInfoTag()->ClientID();
    m_clients[cid]->GetStreamProperties(&m_streamProps[cid]);

    return &m_streamProps[cid];
  }
  else
    return NULL;
}

CFileItem *CPVRManager::GetCurrentPlayingItem()
{
  if (m_currentPlayingChannel)
    return m_currentPlayingChannel;
  else if (m_currentPlayingRecording)
    return m_currentPlayingRecording;
  else
    return NULL;
}

CStdString CPVRManager::GetCurrentInputFormat()
{
  if (m_currentPlayingChannel)
    return m_currentPlayingChannel->GetPVRChannelInfoTag()->InputFormat();

  return "";
}

bool CPVRManager::GetCurrentChannel(int *number, bool *radio)
{
  if (m_currentPlayingChannel)
  {
    if (number)
      *number = m_currentPlayingChannel->GetPVRChannelInfoTag()->Number();
    if (radio)
      *radio  = m_currentPlayingChannel->GetPVRChannelInfoTag()->IsRadio();
    return true;
  }
  else
  {
    if (number)
      *number = 1;
    if (radio)
      *radio  = false;
    return false;
  }
}

bool CPVRManager::HaveActiveClients()
{
  if (m_clients.empty())
    return false;

  int ready = 0;
  CLIENTMAPITR itr = m_clients.begin();
  while (itr != m_clients.end())
  {
    if (m_clients[(*itr).first]->ReadyToUse())
      ready++;
    itr++;
  }
  return ready > 0 ? true : false;
}

bool CPVRManager::HaveMenuHooks(long clientID)
{
  if (clientID < 0)
    clientID = GetCurrentPlayingClientID();
  if (clientID < 0)
    return false;
  return m_clients[clientID]->HaveMenuHooks();
}

void CPVRManager::ProcessMenuHooks(long clientID)
{
  if (m_clients[clientID]->HaveMenuHooks())
  {
    PVR_MENUHOOKS *hooks = m_clients[clientID]->GetMenuHooks();
    std::vector<long> hookIDs;

    CGUIDialogSelect* pDialog = (CGUIDialogSelect*)g_windowManager.GetWindow(WINDOW_DIALOG_SELECT);

    pDialog->Reset();
    pDialog->SetHeading(19196);

    for (unsigned int i = 0; i < hooks->size(); i++)
    {
      pDialog->Add(m_clients[clientID]->GetString(hooks->at(i).string_id));
    }

    pDialog->DoModal();

    int selection = pDialog->GetSelectedLabel();
    if (selection >= 0)
    {
      m_clients[clientID]->CallMenuHook(hooks->at(selection));
    }
  }
}

int CPVRManager::GetPreviousChannel()
{
  if (m_currentPlayingChannel == NULL)
    return -1;

  int LastChannel = m_currentPlayingChannel->GetPVRChannelInfoTag()->Number();

  if ((m_PreviousChannel[m_PreviousChannelIndex ^ 1] == LastChannel || LastChannel != m_PreviousChannel[0]) && LastChannel != m_PreviousChannel[1])
    m_PreviousChannelIndex ^= 1;

  return m_PreviousChannel[m_PreviousChannelIndex ^= 1];
}

bool CPVRManager::CanInstantRecording()
{
  if (!m_currentPlayingChannel)
    return false;

  const cPVRChannelInfoTag* tag = m_currentPlayingChannel->GetPVRChannelInfoTag();
  if (m_clientsProps[tag->ClientID()].SupportTimers)
    return true;
  else
    return false;
}

bool CPVRManager::IsRecordingOnPlayingChannel()
{
  if (!m_currentPlayingChannel)
    return false;

  const cPVRChannelInfoTag* tag = m_currentPlayingChannel->GetPVRChannelInfoTag();
  return tag->IsRecording();
}

bool CPVRManager::StartRecordingOnPlayingChannel(bool bOnOff)
{
  if (!m_currentPlayingChannel)
    return false;

  cPVRChannelInfoTag* tag = m_currentPlayingChannel->GetPVRChannelInfoTag();
  if (m_clientsProps[tag->ClientID()].SupportTimers)
  {
    cPVRChannels *channels;
    if (!m_currentPlayingChannel->GetPVRChannelInfoTag()->IsRadio())
      channels = &PVRChannelsTV;
    else
      channels = &PVRChannelsRadio;

    if (bOnOff && tag->IsRecording() == false)
    {
      cPVRTimerInfoTag newtimer(true);
      newtimer.SetTitle(tag->Name());
      CFileItem *item = new CFileItem(newtimer);

      if (!cPVRTimers::AddTimer(*item))
      {
        CGUIDialogOK::ShowAndGetInput(19033,0,19164,0);
        return false;
      }

      channels->at(tag->Number()-1).SetRecording(true); /* Set in channel list */
      tag->SetRecording(true);                          /* and also in current playing item */
      return true;
    }
    else if (tag->IsRecording() == true)
    {
      for (unsigned int i = 0; i < PVRTimers.size(); ++i)
      {
        if (!PVRTimers[i].IsRepeating() && PVRTimers[i].Active() &&
            (PVRTimers[i].Number() == tag->Number()) &&
            (PVRTimers[i].Start() <= CDateTime::GetCurrentDateTime()) &&
            (PVRTimers[i].Stop() >= CDateTime::GetCurrentDateTime()))
        {
          if (cPVRTimers::DeleteTimer(PVRTimers[i], true))
          {
            channels->at(tag->Number()-1).SetRecording(false);  /* Set in channel list */
            tag->SetRecording(false);                           /* and also in current playing item */
            return true;
          }
        }
      }
    }
  }
  return false;
}

void CPVRManager::SaveCurrentChannelSettings()
{
  if (m_currentPlayingChannel)
  {
    // save video settings
    if (g_settings.m_currentVideoSettings != g_settings.m_defaultVideoSettings)
    {
      m_database.Open();
      m_database.SetChannelSettings(m_currentPlayingChannel->GetPVRChannelInfoTag()->ChannelID(), g_settings.m_currentVideoSettings);
      m_database.Close();
    }
  }
}

void CPVRManager::LoadCurrentChannelSettings()
{
  if (m_currentPlayingChannel)
  {
    CVideoSettings loadedChannelSettings;

    // Switch to default options
    g_settings.m_currentVideoSettings = g_settings.m_defaultVideoSettings;

    m_database.Open();
    if (m_database.GetChannelSettings(m_currentPlayingChannel->GetPVRChannelInfoTag()->ChannelID(), loadedChannelSettings))
    {
      if (loadedChannelSettings.m_AudioDelay != g_settings.m_currentVideoSettings.m_AudioDelay)
      {
        g_settings.m_currentVideoSettings.m_AudioDelay = loadedChannelSettings.m_AudioDelay;

        if (g_application.m_pPlayer)
          g_application.m_pPlayer->SetAVDelay(g_settings.m_currentVideoSettings.m_AudioDelay);
      }

      if (loadedChannelSettings.m_AudioStream != g_settings.m_currentVideoSettings.m_AudioStream)
      {
        g_settings.m_currentVideoSettings.m_AudioStream = loadedChannelSettings.m_AudioStream;

        // only change the audio stream if a different one has been asked for
        if (g_application.m_pPlayer->GetAudioStream() != g_settings.m_currentVideoSettings.m_AudioStream)
        {
          g_application.m_pPlayer->SetAudioStream(g_settings.m_currentVideoSettings.m_AudioStream);    // Set the audio stream to the one selected
        }
      }

      if (loadedChannelSettings.m_Brightness != g_settings.m_currentVideoSettings.m_Brightness)
      {
        g_settings.m_currentVideoSettings.m_Brightness = loadedChannelSettings.m_Brightness;
      }

      if (loadedChannelSettings.m_Contrast != g_settings.m_currentVideoSettings.m_Contrast)
      {
        g_settings.m_currentVideoSettings.m_Contrast = loadedChannelSettings.m_Contrast;
      }

      if (loadedChannelSettings.m_Gamma != g_settings.m_currentVideoSettings.m_Gamma)
      {
        g_settings.m_currentVideoSettings.m_Gamma = loadedChannelSettings.m_Gamma;
      }

      if (loadedChannelSettings.m_Crop != g_settings.m_currentVideoSettings.m_Crop)
      {
        g_settings.m_currentVideoSettings.m_Crop = loadedChannelSettings.m_Crop;
        // AutoCrop changes will get picked up automatically by dvdplayer
      }

      if (loadedChannelSettings.m_CropLeft != g_settings.m_currentVideoSettings.m_CropLeft)
      {
        g_settings.m_currentVideoSettings.m_CropLeft = loadedChannelSettings.m_CropLeft;
      }

      if (loadedChannelSettings.m_CropRight != g_settings.m_currentVideoSettings.m_CropRight)
      {
        g_settings.m_currentVideoSettings.m_CropRight = loadedChannelSettings.m_CropRight;
      }

      if (loadedChannelSettings.m_CropTop != g_settings.m_currentVideoSettings.m_CropTop)
      {
        g_settings.m_currentVideoSettings.m_CropTop = loadedChannelSettings.m_CropTop;
      }

      if (loadedChannelSettings.m_CropBottom != g_settings.m_currentVideoSettings.m_CropBottom)
      {
        g_settings.m_currentVideoSettings.m_CropBottom = loadedChannelSettings.m_CropBottom;
      }

      if (loadedChannelSettings.m_VolumeAmplification != g_settings.m_currentVideoSettings.m_VolumeAmplification)
      {
        g_settings.m_currentVideoSettings.m_VolumeAmplification = loadedChannelSettings.m_VolumeAmplification;

        if (g_application.m_pPlayer)
          g_application.m_pPlayer->SetDynamicRangeCompression((long)(g_settings.m_currentVideoSettings.m_VolumeAmplification * 100));
      }

      if (loadedChannelSettings.m_OutputToAllSpeakers != g_settings.m_currentVideoSettings.m_OutputToAllSpeakers)
      {
        g_settings.m_currentVideoSettings.m_OutputToAllSpeakers = loadedChannelSettings.m_OutputToAllSpeakers;
      }

      if (loadedChannelSettings.m_ViewMode != g_settings.m_currentVideoSettings.m_ViewMode)
      {
        g_settings.m_currentVideoSettings.m_ViewMode = loadedChannelSettings.m_ViewMode;

        g_renderManager.SetViewMode(g_settings.m_currentVideoSettings.m_ViewMode);
        g_settings.m_currentVideoSettings.m_CustomZoomAmount = g_settings.m_fZoomAmount;
        g_settings.m_currentVideoSettings.m_CustomPixelRatio = g_settings.m_fPixelRatio;
      }

      if (loadedChannelSettings.m_CustomPixelRatio != g_settings.m_currentVideoSettings.m_CustomPixelRatio)
      {
        g_settings.m_currentVideoSettings.m_CustomPixelRatio = loadedChannelSettings.m_CustomPixelRatio;
      }

      if (loadedChannelSettings.m_CustomZoomAmount != g_settings.m_currentVideoSettings.m_CustomZoomAmount)
      {
        g_settings.m_currentVideoSettings.m_CustomZoomAmount = loadedChannelSettings.m_CustomZoomAmount;
      }

      if (loadedChannelSettings.m_NoiseReduction != g_settings.m_currentVideoSettings.m_NoiseReduction)
      {
        g_settings.m_currentVideoSettings.m_NoiseReduction = loadedChannelSettings.m_NoiseReduction;
      }

      if (loadedChannelSettings.m_Sharpness != g_settings.m_currentVideoSettings.m_Sharpness)
      {
        g_settings.m_currentVideoSettings.m_Sharpness = loadedChannelSettings.m_Sharpness;
      }

      if (loadedChannelSettings.m_SubtitleDelay != g_settings.m_currentVideoSettings.m_SubtitleDelay)
      {
        g_settings.m_currentVideoSettings.m_SubtitleDelay = loadedChannelSettings.m_SubtitleDelay;

        g_application.m_pPlayer->SetSubTitleDelay(g_settings.m_currentVideoSettings.m_SubtitleDelay);
      }

      if (loadedChannelSettings.m_SubtitleOn != g_settings.m_currentVideoSettings.m_SubtitleOn)
      {
        g_settings.m_currentVideoSettings.m_SubtitleOn = loadedChannelSettings.m_SubtitleOn;

        g_application.m_pPlayer->SetSubtitleVisible(g_settings.m_currentVideoSettings.m_SubtitleOn);
      }

      if (loadedChannelSettings.m_SubtitleStream != g_settings.m_currentVideoSettings.m_SubtitleStream)
      {
        g_settings.m_currentVideoSettings.m_SubtitleStream = loadedChannelSettings.m_SubtitleStream;

        g_application.m_pPlayer->SetSubtitle(g_settings.m_currentVideoSettings.m_SubtitleStream);
      }

      if (loadedChannelSettings.m_InterlaceMethod != g_settings.m_currentVideoSettings.m_InterlaceMethod)
      {
        g_settings.m_currentVideoSettings.m_InterlaceMethod = loadedChannelSettings.m_InterlaceMethod;
      }
    }
    m_database.Close();
  }
}

void CPVRManager::SetPlayingGroup(int GroupId)
{
  m_CurrentGroupID = GroupId;
}

void CPVRManager::ResetQualityData()
{
  if (g_guiSettings.GetBool("pvrplayback.signalquality"))
  {
    strncpy(m_qualityInfo.frontend_name, g_localizeStrings.Get(13205).c_str(), 1024);
    strncpy(m_qualityInfo.frontend_status, g_localizeStrings.Get(13205).c_str(), 1024);
  }
  else
  {
    strncpy(m_qualityInfo.frontend_name, g_localizeStrings.Get(13106).c_str(), 1024);
    strncpy(m_qualityInfo.frontend_status, g_localizeStrings.Get(13106).c_str(), 1024);
  }
  m_qualityInfo.snr           = 0;
  m_qualityInfo.signal        = 0;
  m_qualityInfo.ber           = 0;
  m_qualityInfo.unc           = 0;
  m_qualityInfo.video_bitrate = 0;
  m_qualityInfo.audio_bitrate = 0;
  m_qualityInfo.dolby_bitrate = 0;
}

int CPVRManager::GetPlayingGroup()
{
  return m_CurrentGroupID;
}

void CPVRManager::TriggerRecordingsUpdate(bool force)
{
  m_LastRecordingsCheck = CTimeUtils::GetTimeMS()/1000-RECORDINGCHECKDELTA + (force ? 0 : 5);
}

bool CPVRManager::OpenLiveStream(const cPVRChannelInfoTag* tag)
{
  if (tag == NULL)
    return false;

  EnterCriticalSection(&m_critSection);

  /* Check if a channel or recording is already opened and clear it if yes */
  if (m_currentPlayingChannel)
    delete m_currentPlayingChannel;
  if (m_currentPlayingRecording)
    delete m_currentPlayingRecording;

  /* Set the new channel information */
  m_currentPlayingChannel   = new CFileItem(*tag);
  m_currentPlayingRecording = NULL;
  m_scanStart               = CTimeUtils::GetTimeMS();  /* Reset the stream scan timer */
  ResetQualityData();

  /* Open the stream on the Client */
  if (tag->StreamURL().IsEmpty())
  {
    if (!m_clientsProps[tag->ClientID()].HandleInputStream ||
        !m_clients[tag->ClientID()]->OpenLiveStream(*tag))
    {
      delete m_currentPlayingChannel;
      m_currentPlayingChannel = NULL;
      LeaveCriticalSection(&m_critSection);
      return false;
    }
  }

  /* Load now the new channel settings from Database */
  LoadCurrentChannelSettings();

  LeaveCriticalSection(&m_critSection);
  return true;
}

bool CPVRManager::OpenRecordedStream(const cPVRRecordingInfoTag* tag)
{
  if (tag == NULL)
    return false;

  EnterCriticalSection(&m_critSection);

  /* Check if a channel or recording is already opened and clear it if yes */
  if (m_currentPlayingChannel)
    delete m_currentPlayingChannel;
  if (m_currentPlayingRecording)
    delete m_currentPlayingRecording;

  /* Set the new recording information */
  m_currentPlayingRecording = new CFileItem(*tag);
  m_currentPlayingChannel   = NULL;
  m_scanStart               = CTimeUtils::GetTimeMS();  /* Reset the stream scan timer */
  m_playingClientName       = m_clients[tag->ClientID()]->GetBackendName() + ":" + m_clients[tag->ClientID()]->GetConnectionString();

  /* Open the recording stream on the Client */
  bool ret = m_clients[tag->ClientID()]->OpenRecordedStream(*tag);

  LeaveCriticalSection(&m_critSection);
  return ret;
}

CStdString CPVRManager::GetLiveStreamURL(const cPVRChannelInfoTag* tag)
{
  CStdString stream_url;

  EnterCriticalSection(&m_critSection);

  /* Check if a channel or recording is already opened and clear it if yes */
  if (m_currentPlayingChannel)
    delete m_currentPlayingChannel;
  if (m_currentPlayingRecording)
    delete m_currentPlayingRecording;

  /* Set the new channel information */
  m_currentPlayingChannel   = new CFileItem(*tag);
  m_currentPlayingRecording = NULL;
  m_scanStart               = CTimeUtils::GetTimeMS();  /* Reset the stream scan timer */
  ResetQualityData();

  /* Retrieve the dynamily generated stream URL from the Client */
  stream_url = m_clients[tag->ClientID()]->GetLiveStreamURL(*tag);
  if (stream_url.IsEmpty())
  {
    delete m_currentPlayingChannel;
    m_currentPlayingChannel = NULL;
    LeaveCriticalSection(&m_critSection);
    return "";
  }

  LeaveCriticalSection(&m_critSection);

  return stream_url;
}

void CPVRManager::CloseStream()
{
  EnterCriticalSection(&m_critSection);

  if (m_currentPlayingChannel)
  {
    m_playingClientName = "";

    /* Save channel number in database */
    m_database.Open();
    m_database.UpdateLastChannel(*m_currentPlayingChannel->GetPVRChannelInfoTag());
    m_database.Close();

    /* Store current settings inside Database */
    SaveCurrentChannelSettings();

    /* Set quality data to undefined defaults */
    ResetQualityData();

    /* Close the Client connection */
    m_clients[m_currentPlayingChannel->GetPVRChannelInfoTag()->ClientID()]->CloseLiveStream();
    delete m_currentPlayingChannel;
    m_currentPlayingChannel = NULL;
  }
  else if (m_currentPlayingRecording)
  {
    /* Close the Client connection */
    m_clients[m_currentPlayingRecording->GetPVRRecordingInfoTag()->ClientID()]->CloseRecordedStream();
    delete m_currentPlayingRecording;
    m_currentPlayingRecording = NULL;
  }

  LeaveCriticalSection(&m_critSection);
  return;
}

int CPVRManager::ReadStream(void* lpBuf, int64_t uiBufSize)
{
  EnterCriticalSection(&m_critSection);

  int bytesReaded = 0;

  /* Check stream for available video or audio data, if after the scantime no stream
     is present playback is canceled and returns to the window */
  if (m_scanStart)
  {
    if (CTimeUtils::GetTimeMS() - m_scanStart > (unsigned int) g_guiSettings.GetInt("pvrplayback.scantime")*1000)
    {
      CLog::Log(LOGERROR,"PVR: No video or audio data available after %i seconds, playback stopped", g_guiSettings.GetInt("pvrplayback.scantime"));
      LeaveCriticalSection(&m_critSection);
      return 0;
    }
    else if (g_application.IsPlayingVideo() || g_application.IsPlayingAudio())
      m_scanStart = NULL;
  }

  /* Process LiveTV Reading */
  if (m_currentPlayingChannel)
  {
    bytesReaded = m_clients[m_currentPlayingChannel->GetPVRChannelInfoTag()->ClientID()]->ReadLiveStream(lpBuf, uiBufSize);
  }
  /* Process Recording Reading */
  else if (m_currentPlayingRecording)
  {
    bytesReaded = m_clients[m_currentPlayingRecording->GetPVRRecordingInfoTag()->ClientID()]->ReadRecordedStream(lpBuf, uiBufSize);
  }

  LeaveCriticalSection(&m_critSection);
  return bytesReaded;
}

int64_t CPVRManager::LengthStream(void)
{
  int64_t streamLength = 0;

  EnterCriticalSection(&m_critSection);

  if (m_currentPlayingChannel)
  {
    streamLength = 0;
  }
  else if (m_currentPlayingRecording)
  {
    streamLength = m_clients[m_currentPlayingRecording->GetPVRRecordingInfoTag()->ClientID()]->LengthRecordedStream();
  }

  LeaveCriticalSection(&m_critSection);
  return streamLength;
}

int64_t CPVRManager::SeekStream(int64_t iFilePosition, int iWhence/* = SEEK_SET*/)
{
  int64_t streamNewPos = 0;

  EnterCriticalSection(&m_critSection);

  if (m_currentPlayingChannel)
  {
    streamNewPos = 0;
  }
  else if (m_currentPlayingRecording)
  {
    streamNewPos = m_clients[m_currentPlayingRecording->GetPVRRecordingInfoTag()->ClientID()]->SeekRecordedStream(iFilePosition, iWhence);
  }

  LeaveCriticalSection(&m_critSection);
  return streamNewPos;
}

int64_t CPVRManager::GetStreamPosition()
{
  int64_t streamPos = 0;

  EnterCriticalSection(&m_critSection);

  if (m_currentPlayingChannel)
  {
    streamPos = 0;
  }
  else if (m_currentPlayingRecording)
  {
    streamPos = m_clients[m_currentPlayingRecording->GetPVRRecordingInfoTag()->ClientID()]->PositionRecordedStream();
  }

  LeaveCriticalSection(&m_critSection);
  return streamPos;
}

bool CPVRManager::UpdateItem(CFileItem& item)
{
  /* Don't update if a recording is played */
  if (item.IsPVRRecording())
    return true;

  if (!item.IsPVRChannel())
  {
    CLog::Log(LOGERROR, "CPVRManager: UpdateItem no TVChannelTag given!");
    return false;
  }

  g_application.CurrentFileItem() = *m_currentPlayingChannel;
  g_infoManager.SetCurrentItem(*m_currentPlayingChannel);

  cPVRChannelInfoTag* tagNow = item.GetPVRChannelInfoTag();
  if (tagNow->IsRadio())
  {
    CMusicInfoTag* musictag = item.GetMusicInfoTag();
    if (musictag)
    {
      musictag->SetURL(tagNow->Path());
      musictag->SetTitle(tagNow->NowTitle());
      musictag->SetArtist(tagNow->Name());
    //    musictag->SetAlbum(tag->m_strBouquet);
      musictag->SetAlbumArtist(tagNow->Name());
      musictag->SetGenre(tagNow->NowGenre());
      musictag->SetDuration(tagNow->NowDuration());
      musictag->SetLoaded(true);
      musictag->SetComment("");
      musictag->SetLyrics("");
    }
  }

  cPVRChannelInfoTag* tagPrev = item.GetPVRChannelInfoTag();
  if (tagPrev && tagPrev->Number() != m_LastChannel)
  {
    m_LastChannel         = tagPrev->Number();
    m_LastChannelChanged  = CTimeUtils::GetTimeMS();
    m_playingClientName   = m_clients[tagNow->ClientID()]->GetBackendName() + ":" + m_clients[tagNow->ClientID()]->GetConnectionString();
  }
  if (CTimeUtils::GetTimeMS() - m_LastChannelChanged >= g_guiSettings.GetInt("pvrplayback.channelentrytimeout") && m_LastChannel != m_PreviousChannel[m_PreviousChannelIndex])
     m_PreviousChannel[m_PreviousChannelIndex ^= 1] = m_LastChannel;

  return false;
}

bool CPVRManager::ChannelSwitch(unsigned int iChannel, bool isPreviewed/* = false*/)
{
  if (!m_currentPlayingChannel)
    return false;

  cPVRChannels *channels;
  if (!m_currentPlayingChannel->GetPVRChannelInfoTag()->IsRadio())
    channels = &PVRChannelsTV;
  else
    channels = &PVRChannelsRadio;

  if (iChannel > channels->size()+1)
  {
    CGUIDialogOK::ShowAndGetInput(19033,19136,0,0);
    return false;
  }

  EnterCriticalSection(&m_critSection);

  const cPVRChannelInfoTag* tag = &channels->at(iChannel-1);

  /* Store current settings inside Database */
  if (isPreviewed)
    SaveCurrentChannelSettings();

  /* Perform Channelswitch */
  if (!m_clients[tag->ClientID()]->SwitchChannel(*tag))
  {
    CGUIDialogOK::ShowAndGetInput(19033,0,19136,0);
    LeaveCriticalSection(&m_critSection);
    return false;
  }

  /* Update the Playing channel data and the current epg data if it was not previewed */
  if (!isPreviewed)
  {
    delete m_currentPlayingChannel;
    m_currentPlayingChannel = new CFileItem(*tag);
  }

  /* Reset the Audio/Video detection counter */
  m_scanStart = CTimeUtils::GetTimeMS();

  /* Load now the new channel settings from Database */
  LoadCurrentChannelSettings();

  /* Set quality data to undefined defaults */
  ResetQualityData();

  LeaveCriticalSection(&m_critSection);
  return true;
}

bool CPVRManager::ChannelUp(unsigned int *newchannel, bool preview/* = false*/)
{
   if (m_currentPlayingChannel)
   {
    cPVRChannels *channels;
    if (!m_currentPlayingChannel->GetPVRChannelInfoTag()->IsRadio())
      channels = &PVRChannelsTV;
    else
      channels = &PVRChannelsRadio;

    EnterCriticalSection(&m_critSection);

    /* Store current settings inside Database */
    if (!preview)
      SaveCurrentChannelSettings();

    unsigned int currentTVChannel = m_currentPlayingChannel->GetPVRChannelInfoTag()->Number();
    const cPVRChannelInfoTag* tag;
    for (unsigned int i = 1; i < channels->size(); i++)
    {
      currentTVChannel += 1;

      if (currentTVChannel > channels->size())
        currentTVChannel = 1;

      tag = &channels->at(currentTVChannel-1);

      if ((m_CurrentGroupID != -1) && (m_CurrentGroupID != tag->GroupID()))
        continue;

      /* Perform Channelswitch */
      if (preview)
      {
        /* Update the Playing channel data and the current epg data */
        delete m_currentPlayingChannel;
        m_currentPlayingChannel = new CFileItem(*tag);
        *newchannel = currentTVChannel;
        LeaveCriticalSection(&m_critSection);
        return true;
      }
      else if (m_clients[tag->ClientID()]->SwitchChannel(*tag))
      {
        /* Update the Playing channel data and the current epg data */
        delete m_currentPlayingChannel;
        m_currentPlayingChannel = new CFileItem(*tag);
        m_scanStart             = CTimeUtils::GetTimeMS();

        /* Load now the new channel settings from Database */
        LoadCurrentChannelSettings();

        /* Set quality data to undefined defaults */
        ResetQualityData();

        *newchannel = currentTVChannel;
        LeaveCriticalSection(&m_critSection);
        return true;
      }
    }
    LeaveCriticalSection(&m_critSection);
  }

  return false;
}

bool CPVRManager::ChannelDown(unsigned int *newchannel, bool preview/* = false*/)
{
  if (m_currentPlayingChannel)
  {
    cPVRChannels *channels;
    if (!m_currentPlayingChannel->GetPVRChannelInfoTag()->IsRadio())
      channels = &PVRChannelsTV;
    else
      channels = &PVRChannelsRadio;

    EnterCriticalSection(&m_critSection);

    /* Store current settings inside Database */
    if (!preview)
      SaveCurrentChannelSettings();

    int currentTVChannel = m_currentPlayingChannel->GetPVRChannelInfoTag()->Number();
    const cPVRChannelInfoTag* tag;
    for (unsigned int i = 1; i < channels->size(); i++)
    {
      currentTVChannel -= 1;

      if (currentTVChannel <= 0)
        currentTVChannel = channels->size();

      tag = &channels->at(currentTVChannel-1);

      if ((m_CurrentGroupID != -1) && (m_CurrentGroupID != tag->GroupID()))
        continue;

      /* Perform Channelswitch */
      if (preview)
      {
        /* Update the Playing channel data and the current epg data */
        delete m_currentPlayingChannel;
        m_currentPlayingChannel = new CFileItem(*tag);
        *newchannel = currentTVChannel;
        LeaveCriticalSection(&m_critSection);
        return true;
      }
      else if (m_clients[tag->ClientID()]->SwitchChannel(*tag))
      {
        /* Update the Playing channel data and the current epg data */
        delete m_currentPlayingChannel;
        m_currentPlayingChannel = new CFileItem(*tag);
        m_scanStart             = CTimeUtils::GetTimeMS();

        /* Load now the new channel settings from Database */
        LoadCurrentChannelSettings();

        /* Set quality data to undefined defaults */
        ResetQualityData();

        *newchannel = currentTVChannel;
        LeaveCriticalSection(&m_critSection);
        return true;
      }
    }
    LeaveCriticalSection(&m_critSection);
  }
  return false;
}

int CPVRManager::GetTotalTime()
{
  if (m_currentPlayingChannel)
    return m_currentPlayingChannel->GetPVRChannelInfoTag()->NowDuration() * 1000;

  return 0;
}

int CPVRManager::GetStartTime()
{
  /* If it is called without a opened TV channel return with NULL */
  if (!m_currentPlayingChannel)
    return 0;

  /* GetStartTime() is frequently called by DVDPlayer during playback of Live TV, so we
   * check here if the end of the current running event is reached, if yes update the
   * playing file item with the newest EPG data of the now running event.
   */
  cPVRChannelInfoTag* tag = m_currentPlayingChannel->GetPVRChannelInfoTag();
  if (tag->NowEndTime() < CDateTime::GetCurrentDateTime() || tag->NowTitle() == g_localizeStrings.Get(19055))
  {
    EnterCriticalSection(&m_critSection);

    if (UpdateItem(*m_currentPlayingChannel))
    {
      g_application.CurrentFileItem() = *m_currentPlayingChannel;
      g_infoManager.SetCurrentItem(*m_currentPlayingChannel);
    }

    LeaveCriticalSection(&m_critSection);
  }

  /* Calculate here the position we have of the running live TV event.
   * "position in ms" = ("current local time" - "event start local time") * 1000
   */
  CDateTimeSpan time = CDateTime::GetCurrentDateTime() - tag->NowStartTime();
  return time.GetDays()    * 1000 * 60 * 60 * 24
       + time.GetHours()   * 1000 * 60 * 60
       + time.GetMinutes() * 1000 * 60
       + time.GetSeconds() * 1000;
}

CPVRManager g_PVRManager;
