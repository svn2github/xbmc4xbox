#ifndef __XBMC_PVR_H__
#define __XBMC_PVR_H__

#include "xbmc_addon_dll.h"               /* Dll related functions available to all AddOn's */
#include "xbmc_pvr_types.h"

extern "C"
{
  // Functions that your PVR client must implement, also you must implement the functions from
  // xbmc_addon_dll.h
  ADDON_STATUS Create(ADDON_HANDLE hdl, int ClientID);
  PVR_ERROR GetProperties(PVR_SERVERPROPS* pProps);
  const char* GetBackendName();
  const char* GetBackendVersion();
  const char* GetConnectionString();
  PVR_ERROR GetDriveSpace(long long *total, long long *used);
  PVR_ERROR GetEPGForChannel(unsigned int number, EPG_DATA &epg, time_t start, time_t end);
  PVR_ERROR GetEPGNowInfo(unsigned int number, PVR_PROGINFO &result);
  PVR_ERROR GetEPGNextInfo(unsigned int number, PVR_PROGINFO &result);
  int GetNumBouquets();
  int GetNumChannels();
  int GetNumRecordings();
  int GetNumTimers();
  PVR_ERROR RequestChannelList(PVRHANDLE handle, int radio);
//  PVR_ERROR GetChannelSettings(cPVRChannelInfoTag *result);
//  PVR_ERROR UpdateChannelSettings(const cPVRChannelInfoTag &chaninfo);
//  PVR_ERROR AddChannel(const cPVRChannelInfoTag &info);
//  PVR_ERROR DeleteChannel(unsigned int number);
//  PVR_ERROR RenameChannel(unsigned int number, CStdString &newname);
//  PVR_ERROR MoveChannel(unsigned int number, unsigned int newnumber);
  PVR_ERROR GetAllRecordings(VECRECORDINGS *results);
  PVR_ERROR DeleteRecording(const CTVRecordingInfoTag &recinfo);
  PVR_ERROR RenameRecording(const CTVRecordingInfoTag &recinfo, CStdString &newname);
  PVR_ERROR RequestTimerList(PVRHANDLE handle);
  PVR_ERROR AddTimer(const PVR_TIMERINFO &timerinfo);
  PVR_ERROR DeleteTimer(const PVR_TIMERINFO &timerinfo, bool force);
  PVR_ERROR RenameTimer(const PVR_TIMERINFO &timerinfo, const char *newname);
  PVR_ERROR UpdateTimer(const PVR_TIMERINFO &timerinfo);
  bool OpenLiveStream(unsigned int channel);
  void CloseLiveStream();
  int ReadLiveStream(BYTE* buf, int buf_size);
  int GetCurrentClientChannel();
  bool SwitchChannel(unsigned int channel);
  bool OpenRecordedStream(const CTVRecordingInfoTag &recinfo);
  void CloseRecordedStream(void);
  int ReadRecordedStream(BYTE* buf, int buf_size);
  __int64 SeekRecordedStream(__int64 pos, int whence=SEEK_SET);
  __int64 LengthRecordedStream(void);
  bool TeletextPagePresent(unsigned int channel, unsigned int Page, unsigned int subPage);
  bool ReadTeletextPage(BYTE *buf, unsigned int channel, unsigned int Page, unsigned int subPage);

  // function to export the above structure to XBMC
  void __declspec(dllexport) get_addon(struct PVRClient* pClient)
  {
    pClient->Create                 = Create;
    pClient->GetProperties          = GetProperties;
    pClient->GetConnectionString    = GetConnectionString;
    pClient->GetBackendName         = GetBackendName;
    pClient->GetBackendVersion      = GetBackendVersion;
    pClient->GetDriveSpace          = GetDriveSpace;
    pClient->GetNumBouquets         = GetNumBouquets;
    pClient->GetNumChannels         = GetNumChannels;
    pClient->GetNumRecordings       = GetNumRecordings;
    pClient->GetNumTimers           = GetNumTimers;
    pClient->GetEPGForChannel       = GetEPGForChannel;
    pClient->GetEPGNowInfo          = GetEPGNowInfo;
    pClient->GetEPGNextInfo         = GetEPGNextInfo;
    pClient->RequestChannelList     = RequestChannelList;
//    pClient->GetChannelSettings     = GetChannelSettings;
//    pClient->UpdateChannelSettings  = UpdateChannelSettings;
//    pClient->AddChannel             = AddChannel;
//    pClient->DeleteChannel          = DeleteChannel;
//    pClient->RenameChannel          = RenameChannel;
//    pClient->MoveChannel            = MoveChannel;
    pClient->GetAllRecordings       = GetAllRecordings;
    pClient->DeleteRecording        = DeleteRecording;
    pClient->RenameRecording        = RenameRecording;
    pClient->RequestTimerList       = RequestTimerList;
    pClient->AddTimer               = AddTimer;
    pClient->DeleteTimer            = DeleteTimer;
    pClient->RenameTimer            = RenameTimer;
    pClient->UpdateTimer            = UpdateTimer;
    pClient->OpenLiveStream         = OpenLiveStream;
    pClient->CloseLiveStream        = CloseLiveStream;
    pClient->ReadLiveStream         = ReadLiveStream;
    pClient->GetCurrentClientChannel= GetCurrentClientChannel;
    pClient->SwitchChannel          = SwitchChannel;
    pClient->OpenRecordedStream     = OpenRecordedStream;
    pClient->CloseRecordedStream    = CloseRecordedStream;
    pClient->ReadRecordedStream     = ReadRecordedStream;
    pClient->SeekRecordedStream     = SeekRecordedStream;
    pClient->LengthRecordedStream   = LengthRecordedStream;
    pClient->TeletextPagePresent    = TeletextPagePresent;
    pClient->ReadTeletextPage       = ReadTeletextPage;
  };
};

#endif
