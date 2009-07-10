#pragma once
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

/*
 * for DESCRIPTION see 'TVTimerInfoTag.cpp'
 */

#include "DateTime.h"

class CFileItem;
class CTVEPGInfoTag;

class cPVRTimerInfoTag
{
public:
  cPVRTimerInfoTag();
  cPVRTimerInfoTag(const CFileItem& item);
  cPVRTimerInfoTag(bool Init);

  bool operator ==(const cPVRTimerInfoTag& right) const;
  bool operator !=(const cPVRTimerInfoTag& right) const;

  void Reset();
  const CStdString GetStatus() const;
  int Compare(const cPVRTimerInfoTag &timer) const;
  time_t StartTime(void) const;
  time_t StopTime(void) const;

  CStdString      m_strTitle;             /// Name of this timer
  CStdString      m_Summary;              /// Summary string with the time to show inside a GUI list

  bool            m_Active;               /// Active flag, if it is false backend ignore the timer
  int             m_channelNum;           /// Integer value of the channel number
  int             m_clientID;             /// ID of the backend
  int             m_clientIndex;          /// Index number of the tag, given by the backend, -1 for new
  int             m_clientNum;            /// Integer value of the client number
  bool            m_Radio;                /// Is Radio channel if set
  bool            m_recStatus;            /// Is this timer recording?

  CDateTime       m_StartTime;            /// Start time
  CDateTime       m_StopTime;             /// Stop time

  bool            m_Repeat;               /// Repeating timer if true, use the m_FirstDay and repeat flags
  CDateTime       m_FirstDay;             /// If it is a repeating timer the first date it starts
  int             m_Weekdays;             /// Bit based store of weekdays to repeat

  int             m_Priority;             /// Priority of the timer
  int             m_Lifetime;             /// Lifetime of the timer in days

  CStdString      m_strFileNameAndPath;   /// Filename is only for reference

};

typedef std::vector<cPVRTimerInfoTag> VECTVTIMERS;

class cPVRTimers : public std::vector<cPVRTimerInfoTag> 
{
private:

public:
  cPVRTimers(void);
  cPVRTimerInfoTag *GetTimer(cPVRTimerInfoTag *Timer);
  cPVRTimerInfoTag *GetMatch(CDateTime t);
  cPVRTimerInfoTag *GetMatch(time_t t);
  cPVRTimerInfoTag *GetMatch(const CTVEPGInfoTag *Epg, int *Match = NULL);
  cPVRTimerInfoTag *GetNextActiveTimer(void);
  void Clear();
};

extern cPVRTimers PVRTimers;
