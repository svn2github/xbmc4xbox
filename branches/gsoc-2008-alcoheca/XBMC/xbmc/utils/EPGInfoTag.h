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

#include "VideoInfoTag.h"
#include "DateTime.h"

/* Enums from mythTV & libcmyth source code */

enum RecStatus {
  rsDeleted = -5,
  rsStopped = -4,
  rsRecorded = -3,
  rsRecording = -2,
  rsWillRecord = -1,
  rsUnknown = 0,
  rsDontRecord = 1,
  rsPrevRecording = 2,
  rsCurrentRecording = 3,
  rsEarlierRecording = 4,
  rsTooManyRecordings = 5,
  rsCancelled = 6,
  rsConflict = 7,
  rsLaterShowing = 8,
  rsRepeat = 9,
  rsLowDiskspace = 11,
  rsTunerBusy = 12
};

enum CommFlagStatus {
  COMM_FLAG_NOT_FLAGGED = 0,
  COMM_FLAG_DONE        = 1,
  COMM_FLAG_PROCESSING  = 2,
  COMM_FLAG_COMMFREE    = 3
};

enum TranscodingStatus {
  TRANSCODING_NOT_TRANSCODED = 0,
  TRANSCODING_COMPLETE       = 1,
  TRANSCODING_RUNNING        = 2
};

enum AvailableStatus {
  asAvailable = 0,
  asNotYetAvailable,
  asPendingDelete,
  asFileNotFound,
  asZeroByte,
  asDeleted
};

enum AudioProps_t {
  AUD_UNKNOWN       = 0x00,
  AUD_STEREO        = 0x01,
  AUD_MONO          = 0x02,
  AUD_SURROUND      = 0x04,
  AUD_DOLBY         = 0x08,
  AUD_HARDHEAR      = 0x10,
  AUD_VISUALIMPAIR  = 0x20
}; typedef std::vector< AudioProps_t > AudioProps;

enum VideoProps_t {
  VID_UNKNOWN       = 0x00,
  VID_HDTV          = 0x01,
  VID_WIDESCREEN    = 0x02,
  VID_AVC           = 0x04
}; typedef std::vector< VideoProps_t > VideoProps;

enum SubtitleTypes_t {
  SUB_UNKNOWN       = 0x00,
  SUB_HARDHEAR      = 0x01,
  SUB_NORMAL        = 0x02,
  SUB_ONSCREEN      = 0x04,
  SUB_SIGNED        = 0x08
}; typedef std::vector< SubtitleTypes_t > SubtitleTypes;

class CEPGInfoTag : public CVideoInfoTag
{
public:
  CEPGInfoTag(long uniqueBroadcastID);
  CEPGInfoTag(DWORD sourceID);
  CEPGInfoTag() { Reset(); };
  void Reset();
  const long GetDbID() const { return m_uniqueBroadcastID; };
  const DWORD GetSourceID() const { return m_sourceID; };

  CStdString    m_strSource;
  CStdString    m_strBouquet;
  CStdString    m_strChannel;
  int           m_channelNum;

  CStdString    m_seriesID;
  CStdString    m_episodeID;

  CDateTime     m_startTime;
  CDateTime     m_endTime;
  CDateTimeSpan m_duration;
  CDateTime     m_firstAired;
  bool          m_repeat;

  VideoProps    m_videoProps;
  AudioProps    m_audioProps;
  SubtitleTypes m_subTypes;

  bool          m_commFree;

  RecStatus         m_recStatus;
  CommFlagStatus    m_commFlagStatus;
  TranscodingStatus m_transCodeStatus;
  AvailableStatus   m_availableStatus;

private:
  DWORD m_sourceID; // plugin's ID this tag came from
  long m_uniqueBroadcastID; // db's unique identifier for this tag

};

typedef std::vector<CEPGInfoTag> VECPROGRAMMES;
