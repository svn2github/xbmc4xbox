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
#include "FileItem.h"
#include "TVDatabase.h"
#include "pvrclients/IPVRClient.h"
#include "utils/TVChannelInfoTag.h"
#include "utils/TVRecordInfoTag.h"
#include "utils/TVTimerInfoTag.h"

#include <vector>

class CPVRManager : IPVRClientCallback
                  , private CThread
{
public:
  CPVRManager();
  ~CPVRManager();

  void Start();
  void Stop();
  IPVRClient *LoadClient();

  /* Synchronize Thread */
  virtual void Process();

  /* Manager access */
  static void RemoveInstance();
  static void ReleaseInstance();
  static bool IsInstantiated() { return m_instance != NULL; }

  static CPVRManager* GetInstance();
  unsigned long GetCurrentClientID() { return m_currentClientID; }

  /* Server handling */
  bool IsConnected();
  bool IsSynchronized() { return m_synchronized; }
  CURL GetConnString();

  /* Feature flags */
  bool SupportEPG();
  bool SupportRecording();
  bool SupportRadio();
  bool SupportTimers();
  bool SupportChannelSettings();
  bool SupportTeletext();
  bool SupportDirector();

  /* Event handling */
  void        ConnectionLost() { LostConnection(); }
  void	      OnClientMessage(const long clientID, const PVR_EVENT clientEvent, const char* msg);
  const char* TranslateInfo(DWORD dwInfo);
  static bool HasTimer() { return m_hasTimers;  }
  static bool IsRecording() { return m_isRecording; }
  bool        IsRecording(unsigned int channel, bool radio = false);
  static bool IsPlayingTV() { return m_isPlayingTV; }
  static bool IsPlayingRadio() { return m_isPlayingRadio; }

  /* General handling */
  CStdString GetBackendName();
  CStdString GetBackendVersion();
  bool GetDriveSpace(long long *total, long long *used, int *percent);

  bool GetEPGInfo(unsigned int number, CFileItem& now, CFileItem& next, bool radio = false);
  int GetEPGAll(CFileItemList* results, bool radio = false);
  int GetEPGNow(CFileItemList* results, bool radio = false);
  int GetEPGNext(CFileItemList* results, bool radio = false);
  int GetEPGChannel(unsigned int number, CFileItemList* results, bool radio = false);

  /* Channel handling */
  int GetNumChannels();
  int GetNumHiddenChannels();
  int GetTVChannels(CFileItemList* results, int group_id = -1, bool hidden = false);
  int GetRadioChannels(CFileItemList* results, int group_id = -1, bool hidden = false);
  void MoveChannel(unsigned int oldindex, unsigned int newindex, bool radio = false);
  void HideChannel(unsigned int number, bool radio);
  void SetChannelIcon(unsigned int number, CStdString icon, bool radio = false);
  CStdString GetChannelIcon(unsigned int number, bool radio = false);
  CStdString GetNameForChannel(unsigned int number, bool radio = false);
  bool GetFrontendChannelNumber(unsigned int client_no, int *frontend_no, bool *isRadio);
  int GetClientChannelNumber(unsigned int frontend_no, bool radio = false);
  int GetChannelID(unsigned int frontend_no, bool radio = false);
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

  /* Record handling **/
  int GetNumRecordings();
  int GetAllRecordings(CFileItemList* results);
  bool DeleteRecording(unsigned int index);
  bool RenameRecording(unsigned int index, CStdString &newname);

  /* Timer handling */
  int GetNumTimers();
  int GetAllTimers(CFileItemList* results);
  bool AddTimer(const CFileItem &item);
  bool DeleteTimer(unsigned int index, bool force = false);
  bool RenameTimer(unsigned int index, CStdString &newname);
  bool UpdateTimer(const CFileItem &item);
  CDateTime NextTimerDate(void);

  /* Live stream handling */
  bool OpenLiveStream(unsigned int channel, bool radio = false);
  void CloseLiveStream();
  void PauseLiveStream(bool OnOff);
  int ReadLiveStream(BYTE* buf, int buf_size);
  __int64 SeekLiveStream(__int64 pos, int whence=SEEK_SET);
  int GetCurrentChannel(bool radio = false);
  bool ChannelSwitch(unsigned int channel);
  bool ChannelUp(unsigned int *newchannel);
  bool ChannelDown(unsigned int *newchannel);
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

protected:
  bool ConnectClient();
  void DisconnectClient();
  void LostConnection();

private:
  static CPVRManager   *m_instance;
  IPVRClient*           m_client;       // pointer to enabled client interface
  PVR_SERVERPROPS       m_clientProps;  // store the properties of each client locally
  CTVDatabase           m_database;
  unsigned long         m_currentClientID;
  bool                  m_bConnectionLost;
  bool                  m_synchronized;

  static bool         m_isPlayingTV;
  static bool         m_isPlayingRadio;
  static bool         m_isPlayingRecording;
  static bool         m_isRecording;
  static bool         m_hasRecordings;
  static bool         m_hasTimers;

  CStdString          m_nextRecordingDateTime;
  CStdString          m_nextRecordingChannel;
  CStdString          m_nextRecordingTitle;
  CStdString          m_nowRecordingDateTime;
  CStdString          m_nowRecordingChannel;
  CStdString          m_nowRecordingTitle;

  int                 m_CurrentRadioChannel;
  int                 m_CurrentTVChannel;
  int                 m_CurrentChannelID;
  int                 m_CurrentGroupID;
  unsigned int        m_HiddenChannels;

  VECCHANNELS         m_channels_tv;
  VECCHANNELS         m_channels_radio;
  VECRECORDINGS       m_recordings;
  VECTVTIMERS         m_timers;
  CHANNELGROUPS_DATA  m_channel_group;

  CRITICAL_SECTION    m_critSection;

  DWORD               m_scanStart;

  void                SyncInfo(); // synchronize InfoManager related stuff
  void                GetChannels();
  void                GetTimers();
  void                GetRecordings();
  CTVChannelInfoTag  *GetChannelByNumber(int Number, bool radio, int SkipGap = 0);
  CTVChannelInfoTag  *GetChannelByID(int Id, bool radio, int SkipGap = 0);
  void                SetMusicInfoTag(CFileItem& item, unsigned int channel);
  void                SetCurrentPlayingProgram();
  void                LoadVideoSettings(unsigned int channel_id, bool update = true);
  void                SaveVideoSettings(unsigned int channel_id);
};
