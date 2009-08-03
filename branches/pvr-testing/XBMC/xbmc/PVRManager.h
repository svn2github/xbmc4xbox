#pragma once
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

#include "utils/Thread.h"
#include "utils/Addon.h"
#include "FileItem.h"
#include "TVDatabase.h"
#include "pvrclients/IPVRClient.h"
#include "utils/AddonManager.h"
#include "utils/PVRChannels.h"
#include "utils/PVRRecordings.h"
#include "utils/PVRTimers.h"

#include <vector>

typedef std::map< long, IPVRClient* >           CLIENTMAP;
typedef std::map< long, IPVRClient* >::iterator CLIENTMAPITR;

class CPVRManager : IPVRClientCallback
                  , public ADDON::IAddonCallback
                  , private CThread
{
public:
  CPVRManager();
  ~CPVRManager();

  void Start();
  void Stop();
  bool LoadClients();
  void GetClientProperties(); // call GetClientProperties(long clientID) for each client connected
  void GetClientProperties(long clientID); // request the PVR_SERVERPROPS struct from each client

  /* Synchronize Thread */
  virtual void Process();

  /* Manager access */
  static void RemoveInstance();
  static void ReleaseInstance();
  static bool IsInstantiated() { return m_instance != NULL; }

  static CPVRManager* GetInstance();
  unsigned long GetFirstClientID();
  static CLIENTMAP* Clients() { return &m_clients; }
  CTVDatabase *GetTVDatabase() { return &m_database; }

  /* addon specific */
  bool RequestRestart(const ADDON::CAddon* addon, bool datachanged);
  bool RequestRemoval(const ADDON::CAddon* addon);
  ADDON_STATUS SetSetting(const ADDON::CAddon* addon, const char *settingName, const void *settingValue);

  bool HaveClients() { return !m_clients.empty(); }

  /* Event handling */
  void	      OnClientMessage(const long clientID, const PVR_EVENT clientEvent, const char* msg);
  const char* TranslateInfo(DWORD dwInfo);
  static bool HasTimer() { return m_hasTimers;  }
  static bool IsRecording() { return m_isRecording; }
  bool        IsRecording(unsigned int channel, bool radio = false);
  static bool IsPlayingTV();
  static bool IsPlayingRadio();

  int GetGroupList(CFileItemList* results);
  void AddGroup(const CStdString &newname);
  bool RenameGroup(unsigned int GroupId, const CStdString &newname);
  bool DeleteGroup(unsigned int GroupId);
  bool ChannelToGroup(unsigned int number, unsigned int GroupId, bool radio = false);
  int GetPrevGroupID(int current_group_id);
  int GetNextGroupID(int current_group_id);
  CStdString GetGroupName(int GroupId);
  int GetFirstChannelForGroupID(int GroupId, bool radio = false);

  /* Backend Channel handling */
  bool AddBackendChannel(const CFileItem &item);
  bool DeleteBackendChannel(unsigned int index);
  bool RenameBackendChannel(unsigned int index, CStdString &newname);
  bool MoveBackendChannel(unsigned int index, unsigned int newindex);
  bool UpdateBackendChannel(const CFileItem &item);

  /* Live stream handling */
  bool OpenLiveStream(unsigned int channel, bool radio = false);
  void CloseLiveStream();
  void PauseLiveStream(bool OnOff);
  int ReadLiveStream(BYTE* buf, int buf_size);
  __int64 SeekLiveStream(__int64 pos, int whence=SEEK_SET);
  int GetCurrentChannel(bool radio = false);
  CFileItem *GetCurrentChannelItem();
  bool ChannelSwitch(unsigned int channel);
  bool ChannelUp(unsigned int *newchannel);
  bool ChannelDown(unsigned int *newchannel);
  int GetPreviousChannel();
  int GetTotalTime();
  int GetStartTime();
  bool UpdateItem(CFileItem& item);
  void SetPlayingGroup(int GroupId);
  int GetPlayingGroup();

  /* Recorded stream handling */
  bool OpenRecordedStream(unsigned int record);
  void CloseRecordedStream(void);
  int ReadRecordedStream(BYTE* buf, int buf_size);
  __int64 SeekRecordedStream(__int64 pos, int whence=SEEK_SET);
  __int64 LengthRecordedStream(void);
  bool RecordChannel(unsigned int channel, bool bOnOff, bool radio = false);

  bool TeletextPagePresent(const CFileItem &item, int Page, int subPage);
  bool GetTeletextPage(const CFileItem &item, int Page, int subPage, BYTE* buf);

  void                SetCurrentPlayingProgram(CFileItem& item);
  void                SyncInfo(); // synchronize InfoManager related stuff

protected:

private:
  static CPVRManager   *m_instance;
  static CLIENTMAP      m_clients; // pointer to each enabled client's interface
  std::map< long, PVR_SERVERPROPS > m_clientsProps; // store the properties of each client locally
  DWORD                 m_infoToggleStart;
  unsigned int          m_infoToggleCurrent;
  CTVDatabase           m_database;

  static bool         m_isRecording;
  static bool         m_hasRecordings;
  static bool         m_hasTimers;

  CStdString          m_nextRecordingDateTime;
  CStdString          m_nextRecordingChannel;
  CStdString          m_nextRecordingTitle;
  CStdString          m_nowRecordingDateTime;
  CStdString          m_nowRecordingChannel;
  CStdString          m_nowRecordingTitle;
  CStdString          m_backendName;
  CStdString          m_backendVersion;
  CStdString          m_backendHost;
  CStdString          m_backendDiskspace;
  CStdString          m_backendTimers;
  CStdString          m_backendRecordings;
  CStdString          m_backendChannels;
  CStdString          m_totalDiskspace;
  CStdString          m_nextTimer;

  int                 m_CurrentChannelID;
  int                 m_CurrentGroupID;
  unsigned int        m_HiddenChannels;

  CHANNELGROUPS_DATA  m_channel_group;

  CRITICAL_SECTION    m_critSection;

  DWORD               m_scanStart;

  void                GetChannels();

  static CFileItem   *m_currentPlayingChannel;
  static CFileItem   *m_currentPlayingRecording;
  int                 m_PreviousChannel[2];
  int                 m_PreviousChannelIndex;
  DWORD               m_LastChannelChanged;
  int                 m_LastChannel;
};
