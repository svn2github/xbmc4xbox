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

/*
 * for DESCRIPTION see 'TVRecordInfoTag.cpp'
 */

#include "VideoInfoTag.h"
#include "TVEPGInfoTag.h"
#include "settings/VideoSettings.h"
#include "utils/Thread.h"
#include "DateTime.h"
#include "../addons/include/xbmc_pvr_types.h"

class cPVRRecordingInfoTag : public CVideoInfoTag
{
private:
  int           m_clientID;           /// ID of the backend
  int           m_clientIndex;        /// Index number of the reecording on the client, given by the backend, -1 for unknown
  CStdString    m_strChannel;         /// Channel name where recording from
  CDateTime     m_recordingTime;      /// Recording start time
  CDateTimeSpan m_duration;           /// Duration
  int           m_Priority;
  int           m_Lifetime;
  CStdString    m_strFileNameAndPath; /// Filename for PVRManager to open and read stream
  
  void DisplayError(PVR_ERROR err) const;

public:
  cPVRRecordingInfoTag();
  bool operator ==(const cPVRRecordingInfoTag& right) const;
  bool operator !=(const cPVRRecordingInfoTag& right) const;
  void Reset(void);
  
  long ClientID(void) const { return m_clientID; }
  void SetClientID(int ClientId) { m_clientID = ClientId; }
  long ClientIndex(void) const { return m_clientIndex; }
  void SetClientIndex(int ClientIndex) { m_clientIndex = ClientIndex; }
  CStdString ChannelName(void) const { return m_strChannel; }
  void SetChannelName(CStdString name) { m_strChannel = name; }
  CDateTime RecordingTime(void) const { return m_recordingTime; }
  void SetRecordingTime(CDateTime time) { m_recordingTime = time; }
  CDateTimeSpan Duration(void) const { return m_duration; }
  void SetDuration(CDateTimeSpan duration) { m_duration = duration; }
  int DurationSeconds() const;
  int Lifetime(void) const { return m_Lifetime; }
  void SetLifetime(int Lifetime) { m_Lifetime = Lifetime; }
  int Priority(void) const { return m_Priority; }
  void SetPriority(int Priority) { m_Priority = Priority; }
  CStdString Path(void) const { return m_strFileNameAndPath; }
  void SetPath(CStdString path) { m_strFileNameAndPath = path; }
  CStdString Title(void) const { return m_strTitle; }
  void SetTitle(CStdString Title) { m_strTitle = Title; }
  CStdString PlotOutline(void) const { return m_strPlotOutline; }
  void SetPlotOutline(CStdString PlotOutline) { m_strPlotOutline = PlotOutline; }
  CStdString Plot(void) const { return m_strPlot; }
  void SetPlot(CStdString Plot) { m_strPlot = Plot; }

  bool Delete(void) const;
  bool Rename(CStdString &newName) const;

};


class cPVRRecordings : public std::vector<cPVRRecordingInfoTag> 
                     , private CThread 
{
private:
  CCriticalSection  m_critSection;
  virtual void Process();

public:
  cPVRRecordings(void);
  bool Load() { return Update(true); }
  bool Update(bool Wait = false);
  int GetNumRecordings();
  int GetRecordings(CFileItemList* results);
  static bool DeleteRecording(const CFileItem &item);
  static bool RenameRecording(CFileItem &item, CStdString &newname);
  bool RemoveRecording(const CFileItem &item);
  void Clear();
};

extern cPVRRecordings PVRRecordings;
