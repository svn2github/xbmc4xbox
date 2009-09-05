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
  PVR_ERROR RequestEPGForChannel(PVRHANDLE handle, unsigned int number, time_t start, time_t end);
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
  PVR_ERROR RequestRecordingsList(PVRHANDLE handle);
  PVR_ERROR DeleteRecording(const PVR_RECORDINGINFO &recinfo);
  PVR_ERROR RenameRecording(const PVR_RECORDINGINFO &recinfo, const char *newname);
  PVR_ERROR RequestTimerList(PVRHANDLE handle);
  PVR_ERROR AddTimer(const PVR_TIMERINFO &timerinfo);
  PVR_ERROR DeleteTimer(const PVR_TIMERINFO &timerinfo, bool force);
  PVR_ERROR RenameTimer(const PVR_TIMERINFO &timerinfo, const char *newname);
  PVR_ERROR UpdateTimer(const PVR_TIMERINFO &timerinfo);
  bool OpenLiveStream(const PVR_CHANNEL &channelinfo);
  void CloseLiveStream();
  int ReadLiveStream(BYTE* buf, int buf_size);
  __int64 SeekLiveStream(__int64 pos, int whence=SEEK_SET);
  __int64 LengthLiveStream(void);
  int GetCurrentClientChannel();
  bool SwitchChannel(const PVR_CHANNEL &channelinfo);
  bool OpenRecordedStream(const PVR_RECORDINGINFO &recinfo);
  void CloseRecordedStream(void);
  int ReadRecordedStream(BYTE* buf, int buf_size);
  __int64 SeekRecordedStream(__int64 pos, int whence=SEEK_SET);
  __int64 LengthRecordedStream(void);

  bool OpenTVDemux(PVRDEMUXHANDLE handle, const PVR_CHANNEL &channelinfo);
  bool OpenRecordingDemux(PVRDEMUXHANDLE handle, const PVR_RECORDINGINFO &recinfo);
  void DisposeDemux();
  void ResetDemux();
  void FlushDemux();
  void AbortDemux();
  void SetDemuxSpeed(int iSpeed);
  demux_packet_t* ReadDemux();
  bool SeekDemuxTime(int time, bool backwords, double* startpts);
  int GetDemuxStreamLength();

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
    pClient->RequestEPGForChannel   = RequestEPGForChannel;
    pClient->RequestChannelList     = RequestChannelList;
//    pClient->GetChannelSettings     = GetChannelSettings;
//    pClient->UpdateChannelSettings  = UpdateChannelSettings;
//    pClient->AddChannel             = AddChannel;
//    pClient->DeleteChannel          = DeleteChannel;
//    pClient->RenameChannel          = RenameChannel;
//    pClient->MoveChannel            = MoveChannel;
    pClient->RequestRecordingsList  = RequestRecordingsList;
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
    pClient->SeekLiveStream         = SeekLiveStream;
    pClient->LengthLiveStream       = LengthLiveStream;
    pClient->GetCurrentClientChannel= GetCurrentClientChannel;
    pClient->SwitchChannel          = SwitchChannel;
    pClient->OpenRecordedStream     = OpenRecordedStream;
    pClient->CloseRecordedStream    = CloseRecordedStream;
    pClient->ReadRecordedStream     = ReadRecordedStream;
    pClient->SeekRecordedStream     = SeekRecordedStream;
    pClient->LengthRecordedStream   = LengthRecordedStream;
    pClient->OpenTVDemux            = OpenTVDemux;
    pClient->OpenRecordingDemux     = OpenRecordingDemux;
    pClient->DisposeDemux           = DisposeDemux;
    pClient->ResetDemux             = ResetDemux;
    pClient->FlushDemux             = FlushDemux;
    pClient->AbortDemux             = AbortDemux;
    pClient->SetDemuxSpeed          = SetDemuxSpeed;
    pClient->ReadDemux              = ReadDemux;
    pClient->SeekDemuxTime          = SeekDemuxTime;
    pClient->GetDemuxStreamLength   = GetDemuxStreamLength;
  };
};

#endif
