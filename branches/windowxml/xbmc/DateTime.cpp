/*
 *      Copyright (C) 2005-2007 Team XboxMediaCenter
 *      http://www.xboxmediacenter.com
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
 *  along with GNU Make; see the file COPYING.  If not, write to
 *  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 *  http://www.gnu.org/copyleft/gpl.html
 *
 */

#include "stdafx.h"
#include "DateTime.h"

#define SECONDS_PER_DAY 86400UL
#define SECONDS_PER_HOUR 3600UL
#define SECONDS_PER_MINUTE 60UL
#define SECONDS_TO_FILETIME 10000000UL

/////////////////////////////////////////////////
//
// CDateTimeSpan
//

CDateTimeSpan::CDateTimeSpan()
{
  m_timeSpan.dwHighDateTime=0;
  m_timeSpan.dwLowDateTime=0;
}

CDateTimeSpan::CDateTimeSpan(const CDateTimeSpan& span)
{
  m_timeSpan.dwHighDateTime=span.m_timeSpan.dwHighDateTime;
  m_timeSpan.dwLowDateTime=span.m_timeSpan.dwLowDateTime;
}

CDateTimeSpan::CDateTimeSpan(int day, int hour, int minute, int second)
{
  SetDateTimeSpan(day, hour, minute, second);
}

bool CDateTimeSpan::operator >(const CDateTimeSpan& right) const
{
  return CompareFileTime(&m_timeSpan, &right.m_timeSpan)>0;
}

bool CDateTimeSpan::operator >=(const CDateTimeSpan& right) const
{
  return operator >(right) || operator ==(right);
}

bool CDateTimeSpan::operator <(const CDateTimeSpan& right) const
{
  return CompareFileTime(&m_timeSpan, &right.m_timeSpan)<0;
}

bool CDateTimeSpan::operator <=(const CDateTimeSpan& right) const
{
  return operator <(right) || operator ==(right);
}

bool CDateTimeSpan::operator ==(const CDateTimeSpan& right) const
{
  return CompareFileTime(&m_timeSpan, &right.m_timeSpan)==0;
}

bool CDateTimeSpan::operator !=(const CDateTimeSpan& right) const
{
  return !operator ==(right);
}

CDateTimeSpan CDateTimeSpan::operator +(const CDateTimeSpan& right) const
{
  CDateTimeSpan left(*this);

  ULARGE_INTEGER timeLeft;
  left.ToULargeInt(timeLeft);

  ULARGE_INTEGER timeRight;
  right.ToULargeInt(timeRight);

  timeLeft.QuadPart+=timeRight.QuadPart;

  left.FromULargeInt(timeLeft);

  return left;
}

CDateTimeSpan CDateTimeSpan::operator -(const CDateTimeSpan& right) const
{
  CDateTimeSpan left(*this);

  ULARGE_INTEGER timeLeft;
  left.ToULargeInt(timeLeft);

  ULARGE_INTEGER timeRight;
  right.ToULargeInt(timeRight);

  timeLeft.QuadPart-=timeRight.QuadPart;

  left.FromULargeInt(timeLeft);

  return left;
}

const CDateTimeSpan& CDateTimeSpan::operator +=(const CDateTimeSpan& right)
{
  ULARGE_INTEGER timeThis;
  ToULargeInt(timeThis);

  ULARGE_INTEGER timeRight;
  right.ToULargeInt(timeRight);

  timeThis.QuadPart+=timeRight.QuadPart;

  FromULargeInt(timeThis);

  return *this;
}

const CDateTimeSpan& CDateTimeSpan::operator -=(const CDateTimeSpan& right)
{
  ULARGE_INTEGER timeThis;
  ToULargeInt(timeThis);

  ULARGE_INTEGER timeRight;
  right.ToULargeInt(timeRight);

  timeThis.QuadPart-=timeRight.QuadPart;

  FromULargeInt(timeThis);

  return *this;
}

void CDateTimeSpan::ToULargeInt(ULARGE_INTEGER& time) const
{
  time.HighPart=m_timeSpan.dwHighDateTime;
  time.LowPart=m_timeSpan.dwLowDateTime;
}

void CDateTimeSpan::FromULargeInt(const ULARGE_INTEGER& time)
{
  m_timeSpan.dwHighDateTime=time.HighPart;
  m_timeSpan.dwLowDateTime=time.LowPart;
}

void CDateTimeSpan::SetDateTimeSpan(int day, int hour, int minute, int second)
{
  ULARGE_INTEGER time;
  ToULargeInt(time);

  time.QuadPart=(LONGLONG)day*SECONDS_PER_DAY*SECONDS_TO_FILETIME;
  time.QuadPart+=(LONGLONG)hour*SECONDS_PER_HOUR*SECONDS_TO_FILETIME;
  time.QuadPart+=(LONGLONG)minute*SECONDS_PER_MINUTE*SECONDS_TO_FILETIME;
  time.QuadPart+=(LONGLONG)second*SECONDS_TO_FILETIME;

  FromULargeInt(time);
}

int CDateTimeSpan::GetDays() const
{
  ULARGE_INTEGER time;
  ToULargeInt(time);

  return (int)(time.QuadPart/SECONDS_TO_FILETIME)/SECONDS_PER_DAY;
}

int CDateTimeSpan::GetHours() const
{
  ULARGE_INTEGER time;
  ToULargeInt(time);

  return (int)((time.QuadPart/SECONDS_TO_FILETIME)%SECONDS_PER_DAY)/SECONDS_PER_HOUR;
}

int CDateTimeSpan::GetMinutes() const
{
  ULARGE_INTEGER time;
  ToULargeInt(time);

  return (int)((time.QuadPart/SECONDS_TO_FILETIME%SECONDS_PER_DAY)%SECONDS_PER_HOUR)/SECONDS_PER_MINUTE;
}

int CDateTimeSpan::GetSeconds() const
{
  ULARGE_INTEGER time;
  ToULargeInt(time);

  return (int)(((time.QuadPart/SECONDS_TO_FILETIME%SECONDS_PER_DAY)%SECONDS_PER_HOUR)%SECONDS_PER_MINUTE)%SECONDS_PER_MINUTE;
}

void CDateTimeSpan::SetFromPeriod(const CStdString &period)
{
  long days = atoi(period.c_str());
  // find the first non-space and non-number
  int pos = period.find_first_not_of("0123456789 ", 0);
  if (pos >= 0)
  {
    CStdString units = period.Mid(pos, 3);
    if (units.CompareNoCase("wee") == 0)
      days *= 7;
    else if (units.CompareNoCase("mon") == 0)
      days *= 31;
  }

  SetDateTimeSpan(days, 0, 0, 0);
}


/////////////////////////////////////////////////
//
// CDateTime
//

CDateTime::CDateTime()
{
  Reset();
}

CDateTime::CDateTime(const SYSTEMTIME &time)
{
  // we store internally as a FILETIME
  m_state = ToFileTime(time, m_time) ? valid : invalid;
}

CDateTime::CDateTime(const FILETIME &time)
{
  m_time=time;
  SetValid(true);
}

CDateTime::CDateTime(const CDateTime& time)
{
  m_time=time.m_time;
  m_state=time.m_state;
}

CDateTime::CDateTime(const time_t& time)
{
  m_state = ToFileTime(time, m_time) ? valid : invalid;
}

CDateTime::CDateTime(const tm& time)
{
  m_state = ToFileTime(time, m_time) ? valid : invalid;
}

CDateTime::CDateTime(int year, int month, int day, int hour, int minute, int second)
{
  SetDateTime(year, month, day, hour, minute, second);
}

CDateTime CDateTime::GetCurrentDateTime()
{
  // get the current time
  SYSTEMTIME time;
  GetLocalTime(&time);

  return CDateTime(time);
}

const CDateTime& CDateTime::operator =(const SYSTEMTIME& right)
{
  m_state = ToFileTime(right, m_time) ? valid : invalid;

  return *this;
}

const CDateTime& CDateTime::operator =(const FILETIME& right)
{
  m_time=right;
  SetValid(true);

  return *this;
}

const CDateTime& CDateTime::operator =(const time_t& right)
{
  m_state = ToFileTime(right, m_time) ? valid : invalid;

  return *this;
}

const CDateTime& CDateTime::operator =(const tm& right)
{
  m_state = ToFileTime(right, m_time) ? valid : invalid;

  return *this;
}

bool CDateTime::operator >(const CDateTime& right) const
{
  return operator >(right.m_time);
}

bool CDateTime::operator >=(const CDateTime& right) const
{
  return operator >(right) || operator ==(right);
}

bool CDateTime::operator <(const CDateTime& right) const
{
  return operator <(right.m_time);
}

bool CDateTime::operator <=(const CDateTime& right) const
{
  return operator <(right) || operator ==(right);
}

bool CDateTime::operator ==(const CDateTime& right) const
{
  return operator ==(right.m_time);
}

bool CDateTime::operator !=(const CDateTime& right) const
{
  return !operator ==(right);
}

bool CDateTime::operator >(const FILETIME& right) const
{
  return CompareFileTime(&m_time, &right)>0;
}

bool CDateTime::operator >=(const FILETIME& right) const
{
  return operator >(right) || operator ==(right);
}

bool CDateTime::operator <(const FILETIME& right) const
{
  return CompareFileTime(&m_time, &right)<0;
}

bool CDateTime::operator <=(const FILETIME& right) const
{
  return operator <(right) || operator ==(right);
}

bool CDateTime::operator ==(const FILETIME& right) const
{
  return CompareFileTime(&m_time, &right)==0;
}

bool CDateTime::operator !=(const FILETIME& right) const
{
  return !operator ==(right);
}

bool CDateTime::operator >(const SYSTEMTIME& right) const
{
  FILETIME time;
  ToFileTime(right, time);

  return operator >(time);
}

bool CDateTime::operator >=(const SYSTEMTIME& right) const
{
  return operator >(right) || operator ==(right);
}

bool CDateTime::operator <(const SYSTEMTIME& right) const
{
  FILETIME time;
  ToFileTime(right, time);

  return operator <(time);
}

bool CDateTime::operator <=(const SYSTEMTIME& right) const
{
  return operator <(right) || operator ==(right);
}

bool CDateTime::operator ==(const SYSTEMTIME& right) const
{
  FILETIME time;
  ToFileTime(right, time);

  return operator ==(time);
}

bool CDateTime::operator !=(const SYSTEMTIME& right) const
{
  return !operator ==(right);
}

bool CDateTime::operator >(const time_t& right) const
{
  FILETIME time;
  ToFileTime(right, time);

  return operator >(time);
}

bool CDateTime::operator >=(const time_t& right) const
{
  return operator >(right) || operator ==(right);
}

bool CDateTime::operator <(const time_t& right) const
{
  FILETIME time;
  ToFileTime(right, time);

  return operator <(time);
}

bool CDateTime::operator <=(const time_t& right) const
{
  return operator <(right) || operator ==(right);
}

bool CDateTime::operator ==(const time_t& right) const
{
  FILETIME time;
  ToFileTime(right, time);

  return operator ==(time);
}

bool CDateTime::operator !=(const time_t& right) const
{
  return !operator ==(right);
}

bool CDateTime::operator >(const tm& right) const
{
  FILETIME time;
  ToFileTime(right, time);

  return operator >(time);
}

bool CDateTime::operator >=(const tm& right) const
{
  return operator >(right) || operator ==(right);
}

bool CDateTime::operator <(const tm& right) const
{
  FILETIME time;
  ToFileTime(right, time);

  return operator <(time);
}

bool CDateTime::operator <=(const tm& right) const
{
  return operator <(right) || operator ==(right);
}

bool CDateTime::operator ==(const tm& right) const
{
  FILETIME time;
  ToFileTime(right, time);

  return operator ==(time);
}

bool CDateTime::operator !=(const tm& right) const
{
  return !operator ==(right);
}

CDateTime CDateTime::operator +(const CDateTimeSpan& right) const
{
  CDateTime left(*this);

  ULARGE_INTEGER timeLeft;
  left.ToULargeInt(timeLeft);

  ULARGE_INTEGER timeRight;
  right.ToULargeInt(timeRight);

  timeLeft.QuadPart+=timeRight.QuadPart;

  left.FromULargeInt(timeLeft);

  return left;
}

CDateTime CDateTime::operator -(const CDateTimeSpan& right) const
{
  CDateTime left(*this);

  ULARGE_INTEGER timeLeft;
  left.ToULargeInt(timeLeft);

  ULARGE_INTEGER timeRight;
  right.ToULargeInt(timeRight);

  timeLeft.QuadPart-=timeRight.QuadPart;

  left.FromULargeInt(timeLeft);

  return left;
}

const CDateTime& CDateTime::operator +=(const CDateTimeSpan& right)
{
  ULARGE_INTEGER timeThis;
  ToULargeInt(timeThis);

  ULARGE_INTEGER timeRight;
  right.ToULargeInt(timeRight);

  timeThis.QuadPart+=timeRight.QuadPart;

  FromULargeInt(timeThis);

  return *this;
}

const CDateTime& CDateTime::operator -=(const CDateTimeSpan& right)
{
  ULARGE_INTEGER timeThis;
  ToULargeInt(timeThis);

  ULARGE_INTEGER timeRight;
  right.ToULargeInt(timeRight);

  timeThis.QuadPart-=timeRight.QuadPart;

  FromULargeInt(timeThis);

  return *this;
}

CDateTimeSpan CDateTime::operator -(const CDateTime& right) const
{
  CDateTimeSpan left;

  ULARGE_INTEGER timeLeft;
  left.ToULargeInt(timeLeft);

  ULARGE_INTEGER timeThis;
  ToULargeInt(timeThis);

  ULARGE_INTEGER timeRight;
  right.ToULargeInt(timeRight);

  timeLeft.QuadPart=timeThis.QuadPart-timeRight.QuadPart;

  left.FromULargeInt(timeLeft);

  return left;
}

CDateTime::operator FILETIME() const
{
  return m_time;
}

void CDateTime::Serialize(CArchive& ar)
{
  if (ar.IsStoring())
  {
    ar<<(int)m_state;
    if (m_state==valid)
    {
      SYSTEMTIME st;
      GetAsSystemTime(st);
      ar<<st;
    }
  }
  else
  {
    Reset();
    ar>>(int&)m_state;
    if (m_state==valid)
    {
      SYSTEMTIME st;
      ar>>st;
      ToFileTime(st, m_time);
    }
  }
}

void CDateTime::Reset()
{
  SetDateTime(1601, 1, 1, 0, 0, 0);
  SetValid(false);
}

void CDateTime::SetValid(bool yesNo)
{
  m_state=yesNo ? valid : invalid;
}

bool CDateTime::IsValid() const
{
  return m_state==valid;
}

bool CDateTime::ToFileTime(const SYSTEMTIME& time, FILETIME& fileTime) const
{
  return SystemTimeToFileTime(&time, &fileTime)==TRUE;
}

bool CDateTime::ToFileTime(const time_t& time, FILETIME& fileTime) const
{
  LONGLONG ll = Int32x32To64(time, 10000000)+0x19DB1DED53E8000;

  fileTime.dwLowDateTime  = (DWORD)&ll;
  fileTime.dwHighDateTime = (DWORD)(ll >> 32);

  return true;
}

bool CDateTime::ToFileTime(const tm& time, FILETIME& fileTime) const
{
  SYSTEMTIME st;
  ZeroMemory(&st, sizeof(SYSTEMTIME));

  st.wYear=time.tm_year+1900;
  st.wMonth=time.tm_mon+1;
  st.wDayOfWeek=time.tm_wday;
  st.wDay=time.tm_mday;
  st.wHour=time.tm_hour;
  st.wMinute=time.tm_min;
  st.wSecond=time.tm_sec;

  return SystemTimeToFileTime(&st, &fileTime)==TRUE;
}

void CDateTime::ToULargeInt(ULARGE_INTEGER& time) const
{
  time.HighPart=m_time.dwHighDateTime;
  time.LowPart=m_time.dwLowDateTime;
}

void CDateTime::FromULargeInt(const ULARGE_INTEGER& time)
{
  m_time.dwHighDateTime=time.HighPart;
  m_time.dwLowDateTime=time.LowPart;
}

int CDateTime::GetDay() const
{
  SYSTEMTIME st;
  GetAsSystemTime(st);

  return st.wDay;
}

int CDateTime::GetMonth() const
{
  SYSTEMTIME st;
  GetAsSystemTime(st);

  return st.wMonth;
}

int CDateTime::GetYear() const
{
  SYSTEMTIME st;
  GetAsSystemTime(st);

  return st.wYear;
}

int CDateTime::GetHour() const
{
  SYSTEMTIME st;
  GetAsSystemTime(st);

  return st.wHour;
}

int CDateTime::GetMinute() const
{
  SYSTEMTIME st;
  GetAsSystemTime(st);

  return st.wMinute;
}

int CDateTime::GetSecond() const
{
  SYSTEMTIME st;
  GetAsSystemTime(st);

  return st.wSecond;
}

int CDateTime::GetDayOfWeek() const
{
  SYSTEMTIME st;
  GetAsSystemTime(st);

  return st.wDayOfWeek;
}

void CDateTime::SetDateTime(int year, int month, int day, int hour, int minute, int second)
{
  SYSTEMTIME st;
  ZeroMemory(&st, sizeof(SYSTEMTIME));

  st.wYear=year;
  st.wMonth=month;
  st.wDay=day;
  st.wHour=hour;
  st.wMinute=minute;
  st.wSecond=second;

  m_state = ToFileTime(st, m_time) ? valid : invalid;
}

void CDateTime::SetDate(int year, int month, int day)
{
  SetDateTime(year, month, day, 0, 0, 0);
}

void CDateTime::SetTime(int hour, int minute, int second)
{
  // 01.01.1601 00:00:00 is 0 as filetime
  SetDateTime(1601, 1, 1, hour, minute, second);
}

void CDateTime::GetAsSystemTime(SYSTEMTIME& time) const
{
  FileTimeToSystemTime(&m_time, &time);
}

void CDateTime::GetAsTime(time_t& time) const
{
  ULARGE_INTEGER filetime;
  ToULargeInt(filetime);

  time=(time_t)(filetime.QuadPart-0x19DB1DED53E8000UL)/10000000UL;
}

void CDateTime::GetAsTm(tm& time) const
{
  SYSTEMTIME st;
  GetAsSystemTime(st);

  time.tm_year=st.wYear-1900;
  time.tm_mon=st.wMonth-1;
  time.tm_wday=st.wDayOfWeek;
  time.tm_mday=st.wDay;
  time.tm_hour=st.wHour;
  time.tm_min=st.wMinute;
  time.tm_sec=st.wSecond;

  mktime(&time);
}

void CDateTime::GetAsTimeStamp(FILETIME& time) const
{
  ::LocalFileTimeToFileTime(&m_time, &time);
}

CStdString CDateTime::GetAsDBDate() const
{
  SYSTEMTIME st;
  GetAsSystemTime(st);

  CStdString date;
  date.Format("%04i-%02i-%02i", st.wYear, st.wMonth, st.wDay);

  return date;
}

CStdString CDateTime::GetAsLocalizedTime(bool withSeconds/*=true*/) const
{
  CStdString strOut;
  const CStdString& strFormat=g_langInfo.GetTimeFormat();

  SYSTEMTIME dateTime;
  GetAsSystemTime(dateTime);

  // Prefetch meridiem symbol
  const CStdString& strMeridiem=g_langInfo.GetMeridiemSymbol(dateTime.wHour > 11 ? CLangInfo::MERIDIEM_SYMBOL_PM : CLangInfo::MERIDIEM_SYMBOL_AM);

  int length=strFormat.GetLength();
  for (int i=0; i<length; ++i)
  {
    char c=strFormat[i];
    if (c=='\'')
    {
      // To be able to display a "'" in the string,
      // find the last "'" that doesn't follow a "'"
      int pos=i+1;
      while(((pos=strFormat.Find(c,pos+1))>-1 && pos<strFormat.GetLength()) && strFormat[pos+1]=='\'');

      CStdString strPart;
      if (pos>-1)
      {
        // Extract string between ' '
        strPart=strFormat.Mid(i+1, pos-i-1);
        i=pos;
      }
      else
      {
        strPart=strFormat.Mid(i+1, length-i-1);
        i=length;
      }

      strPart.Replace("''", "'");

      strOut+=strPart;
    }
    else if (c=='h' || c=='H') // parse hour (H="24 hour clock")
    {
      int partLength=0;

      int pos=strFormat.find_first_not_of(c,i+1);
      if (pos>-1)
      {
        // Get length of the hour mask, eg. HH
        partLength=pos-i;
        i=pos-1;
      }
      else
      {
        // mask ends at the end of the string, extract it
        partLength=length-i;
        i=length;
      }

      int hour=dateTime.wHour;
      if (c=='h')
      { // recalc to 12 hour clock
        if (hour > 11)
          hour -= (12 * (hour > 12));
        else
          hour += (12 * (hour < 1));
      }

      // Format hour string with the length of the mask
      CStdString str;
      if (partLength==1)
        str.Format("%d", hour);
      else
        str.Format("%02d", hour);

      strOut+=str;
    }
    else if (c=='m') // parse minutes
    {
      int partLength=0;

      int pos=strFormat.find_first_not_of(c,i+1);
      if (pos>-1)
      {
        // Get length of the minute mask, eg. mm
        partLength=pos-i;
        i=pos-1;
      }
      else
      {
        // mask ends at the end of the string, extract it
        partLength=length-i;
        i=length;
      }

      // Format minute string with the length of the mask
      CStdString str;
      if (partLength==1)
        str.Format("%d", dateTime.wMinute);
      else
        str.Format("%02d", dateTime.wMinute);

      strOut+=str;
    }
    else if (c=='s') // parse seconds
    {
      int partLength=0;

      int pos=strFormat.find_first_not_of(c,i+1);
      if (pos>-1)
      {
        // Get length of the seconds mask, eg. ss
        partLength=pos-i;
        i=pos-1;
      }
      else
      {
        // mask ends at the end of the string, extract it
        partLength=length-i;
        i=length;
      }

      if (withSeconds)
      {
        // Format seconds string with the length of the mask
        CStdString str;
        if (partLength==1)
          str.Format("%d", dateTime.wSecond);
        else
          str.Format("%02d", dateTime.wSecond);

        strOut+=str;
      }
      else
        strOut.Delete(strOut.GetLength()-1,1);
    }
    else if (c=='x') // add meridiem symbol
    {
      int partLength=0;

      int pos=strFormat.find_first_not_of(c,i+1);
      if (pos>-1)
      {
        // Get length of the meridiem mask
        partLength=pos-i;
        i=pos-1;
      }
      else
      {
        // mask ends at the end of the string, extract it
        partLength=length-i;
        i=length;
      }

      strOut+=strMeridiem;
    }
    else // everything else pass to output
      strOut+=c;
  }

  return strOut;
}

CStdString CDateTime::GetAsLocalizedDate(bool longDate/*=false*/) const
{
  CStdString strOut;

  const CStdString& strFormat=g_langInfo.GetDateFormat(longDate);

  SYSTEMTIME dateTime;
  GetAsSystemTime(dateTime);

  int length=strFormat.GetLength();

  for (int i=0; i<length; ++i)
  {
    char c=strFormat[i];
    if (c=='\'')
    {
      // To be able to display a "'" in the string,
      // find the last "'" that doesn't follow a "'"
      int pos=i+1;
      while(((pos=strFormat.Find(c,pos+1))>-1 && pos<strFormat.GetLength()) && strFormat[pos+1]=='\'');

      CStdString strPart;
      if (pos>-1)
      {
        // Extract string between ' '
        strPart=strFormat.Mid(i+1, pos-i-1);
        i=pos;
      }
      else
      {
        strPart=strFormat.Mid(i+1, length-i-1);
        i=length;
      }
      strPart.Replace("''", "'");
      strOut+=strPart;
    }
    else if (c=='D') // parse days
    {
      int partLength=0;

      int pos=strFormat.find_first_not_of(c,i+1);
      if (pos>-1)
      {
        // Get length of the day mask, eg. DDDD
        partLength=pos-i;
        i=pos-1;
      }
      else
      {
        // mask ends at the end of the string, extract it
        partLength=length-i;
        i=length;
      }

      // Format string with the length of the mask
      CStdString str;
      if (partLength==1) // single-digit number
        str.Format("%d", dateTime.wDay);
      else if (partLength==2) // two-digit number
        str.Format("%02d", dateTime.wDay);
      else // Day of week string
      {
        switch (dateTime.wDayOfWeek)
        {
          case 1 : str = g_localizeStrings.Get(11); break;
          case 2 : str = g_localizeStrings.Get(12); break;
          case 3 : str = g_localizeStrings.Get(13); break;
          case 4 : str = g_localizeStrings.Get(14); break;
          case 5 : str = g_localizeStrings.Get(15); break;
          case 6 : str = g_localizeStrings.Get(16); break;
          default: str = g_localizeStrings.Get(17); break;
        }
      }
      strOut+=str;
    }
    else if (c=='M') // parse month
    {
      int partLength=0;

      int pos=strFormat.find_first_not_of(c,i+1);
      if (pos>-1)
      {
        // Get length of the month mask, eg. MMMM
        partLength=pos-i;
        i=pos-1;
      }
      else
      {
        // mask ends at the end of the string, extract it
        partLength=length-i;
        i=length;
      }

      // Format string with the length of the mask
      CStdString str;
      if (partLength==1) // single-digit number
        str.Format("%d", dateTime.wMonth);
      else if (partLength==2) // two-digit number
        str.Format("%02d", dateTime.wMonth);
      else // Month string
      {
        switch (dateTime.wMonth)
        {
          case 1 : str = g_localizeStrings.Get(21); break;
          case 2 : str = g_localizeStrings.Get(22); break;
          case 3 : str = g_localizeStrings.Get(23); break;
          case 4 : str = g_localizeStrings.Get(24); break;
          case 5 : str = g_localizeStrings.Get(25); break;
          case 6 : str = g_localizeStrings.Get(26); break;
          case 7 : str = g_localizeStrings.Get(27); break;
          case 8 : str = g_localizeStrings.Get(28); break;
          case 9 : str = g_localizeStrings.Get(29); break;
          case 10: str = g_localizeStrings.Get(30); break;
          case 11: str = g_localizeStrings.Get(31); break;
          default: str = g_localizeStrings.Get(32); break;
        }
      }
      strOut+=str;
    }
    else if (c=='Y') // parse year
    {
      int partLength=0;

      int pos=strFormat.find_first_not_of(c,i+1);
      if (pos>-1)
      {
        // Get length of the year mask, eg. YYYY
        partLength=pos-i;
        i=pos-1;
      }
      else
      {
        // mask ends at the end of the string, extract it
        partLength=length-i;
        i=length;
      }

      // Format string with the length of the mask
      CStdString str;
      str.Format("%d", dateTime.wYear); // four-digit number
      if (partLength<=2)
        str.Delete(0, 2); // two-digit number

      strOut+=str;
    }
    else // everything else pass to output
      strOut+=c;
  }

  return strOut;
}

CStdString CDateTime::GetAsLocalizedDateTime(bool longDate/*=false*/, bool withSeconds/*=true*/) const
{
  return GetAsLocalizedDate(longDate)+" "+GetAsLocalizedTime(withSeconds);
}