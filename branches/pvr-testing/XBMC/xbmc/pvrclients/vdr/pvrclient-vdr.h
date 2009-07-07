#pragma once

#ifndef __PVRCLIENT_VDR_H__
#define __PVRCLIENT_VDR_H__

/*
 *      Copyright (C) 2005-2008 Team XBMC
 *      http://xbmc.org
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

/*
* for DESCRIPTION see 'PVRClient-vdr.cpp'
*/

#include "vtptransceiver.h"
#include <string>
#include <vector>

/* Master defines for client control */
#include "../../addons/include/xbmc_pvr_types.h"


class PVRClientVDR
{
public:
  /* Class interface */
  PVRClientVDR();
  ~PVRClientVDR();

  /* VTP Listening Thread */
  static void* Process(void*);

  /* Server handling */
  PVR_ERROR GetProperties(PVR_SERVERPROPS *props);

  bool Connect();
  void Disconnect();
  bool IsUp();

  /* General handling */
  const char* GetBackendName();
  const char* GetBackendVersion();
  const char* GetConnectionString();
  PVR_ERROR GetDriveSpace(long long *total, long long *used);
  PVR_ERROR GetClientTime(time_t *time, int *diff_from_gmt);

  /* EPG handling */
  PVR_ERROR GetEPGForChannel(unsigned int number, EPG_DATA &epg, time_t start = NULL, time_t end = NULL);
  PVR_ERROR GetEPGNowInfo(unsigned int number, CTVEPGInfoTag *result);
  PVR_ERROR GetEPGNextInfo(unsigned int number, CTVEPGInfoTag *result);

  /* Channel handling */
  int GetNumChannels(void);
  PVR_ERROR GetChannelList(VECCHANNELS* results, bool radio = false);
  PVR_ERROR GetChannelSettings(CTVChannelInfoTag *result);
  PVR_ERROR UpdateChannelSettings(const CTVChannelInfoTag &chaninfo);
  PVR_ERROR AddChannel(const CTVChannelInfoTag &info);
  PVR_ERROR DeleteChannel(unsigned int number);
  PVR_ERROR RenameChannel(unsigned int number, CStdString &newname);
  PVR_ERROR MoveChannel(unsigned int number, unsigned int newnumber);

  /* Record handling **/
  int GetNumRecordings(void);
  PVR_ERROR GetAllRecordings(VECRECORDINGS *results);
  PVR_ERROR DeleteRecording(const CTVRecordingInfoTag &recinfo);
  PVR_ERROR RenameRecording(const CTVRecordingInfoTag &recinfo, CStdString &newname);

  /* Timer handling */
  int GetNumTimers(void);
  PVR_ERROR GetAllTimers(VECTVTIMERS *results);
  PVR_ERROR GetTimerInfo(unsigned int timernumber, CTVTimerInfoTag &timerinfo);
  PVR_ERROR AddTimer(const CTVTimerInfoTag &timerinfo);
  PVR_ERROR DeleteTimer(const CTVTimerInfoTag &timerinfo, bool force = false);
  PVR_ERROR RenameTimer(const CTVTimerInfoTag &timerinfo, CStdString &newname);
  PVR_ERROR UpdateTimer(const CTVTimerInfoTag &timerinfo);

  /* Live stream handling */
  bool OpenLiveStream(unsigned int channel);
  void CloseLiveStream();
  int ReadLiveStream(BYTE* buf, int buf_size);
  int GetCurrentClientChannel();
  bool SwitchChannel(unsigned int channel);

  /* Record stream handling */
  bool OpenRecordedStream(const CTVRecordingInfoTag &recinfo);
  void CloseRecordedStream(void);
  int ReadRecordedStream(BYTE* buf, int buf_size);
  __int64 SeekRecordedStream(__int64 pos, int whence=SEEK_SET);
  __int64 LengthRecordedStream(void);
  
  bool TeletextPagePresent(unsigned int channel, unsigned int Page, unsigned int subPage);
  bool ReadTeletextPage(BYTE *buf, unsigned int channel, unsigned int Page, unsigned int subPage);

protected:
  static CVTPTransceiver *m_transceiver;
  static SOCKET           m_socket_video;
  static SOCKET           m_socket_data;

private:
  int                     m_iCurrentChannel;
  static bool             m_bConnected;
  pthread_mutex_t         m_critSection;
  pthread_t               m_thread;
  static bool             m_bStop;

  /* Following is for recordings streams */
  uint64_t                currentPlayingRecordBytes;
  uint32_t                currentPlayingRecordFrames;
  uint64_t                currentPlayingRecordPosition;

  void Close();
};

#endif // __PVRCLIENT_VDR_H__
