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

#include "timers.h"
#include "../../addons/include/xbmc_addon_lib++.h"

#define SECSINDAY  86400

template<class T> inline T max(T a, T b) { return a >= b ? a : b; }

cTimer::cTimer()
{
  startTime = stopTime = index = 0;
  recording = pending = inVpsMargin = false;
  flags = tfActive;
  weekdays = 0;
  aux = NULL;
}

cTimer::~cTimer()
{
  free(aux);
}

bool cTimer::Parse(const char *s)
{
  char *channelbuffer = NULL;
  char *daybuffer = NULL;
  char *filebuffer = NULL;
  free(aux);
  aux = NULL;

  char *s2 = NULL;
  int l2 = strlen(s);
  while (l2 > 0 && isspace(s[l2 - 1]))
        l2--;

  if (s[l2 - 1] == ':') 
  {
    s2 = (char *)malloc(sizeof(char) * (l2 + 3));
    strcat(strn0cpy(s2, s, l2 + 1), " \n");
    s = s2;
  }
  bool result = false;
  if (8 <= sscanf(s, "%u %u :%a[^:]:%a[^:]:%d :%d :%d :%d :%a[^:\n]:%a[^\n]", &index, &flags, &channelbuffer, &daybuffer, &start, &stop, &priority, &lifetime, &filebuffer, &aux))
  {
    if (aux && !*skipspace(aux))
    {
      free(aux);
      aux = NULL;
    }
    //TODO add more plausibility checks
    result = ParseDay(daybuffer, day, weekdays);

    strn0cpy(file, filebuffer, 256);
    strreplace(file, '|', ':');

    if (isnumber(channelbuffer))
      channel = atoi(channelbuffer);
    else
    {
      XBMC_log(LOG_ERROR, "PCRClient-vdr: channel not defined for timer");
      result = false;
    }
  }
  free(channelbuffer);
  free(daybuffer);
  free(filebuffer);
  free(s2);
  return result;
}


bool cTimer::ParseDay(const char *s, time_t &Day, int &WeekDays)
{
  // possible formats are:
  // 19
  // 2005-03-19
  // MTWTFSS
  // MTWTFSS@19
  // MTWTFSS@2005-03-19

  Day = 0;
  WeekDays = 0;
  s = skipspace(s);
  if (!*s)
     return false;
  const char *a = strchr(s, '@');
  const char *d = a ? a + 1 : isdigit(*s) ? s : NULL;
  if (d) {
     if (strlen(d) == 10) {
        struct tm tm_r;
        if (3 == sscanf(d, "%d-%d-%d", &tm_r.tm_year, &tm_r.tm_mon, &tm_r.tm_mday)) {
           tm_r.tm_year -= 1900;
           tm_r.tm_mon--;
           tm_r.tm_hour = tm_r.tm_min = tm_r.tm_sec = 0;
           tm_r.tm_isdst = -1; // makes sure mktime() will determine the correct DST setting
           Day = mktime(&tm_r);
           }
        else
           return false;
        }
     else {
        // handle "day of month" for compatibility with older versions:
        char *tail = NULL;
        int day = strtol(d, &tail, 10);
        if (tail && *tail || day < 1 || day > 31)
           return false;
        time_t t = time(NULL);
        int DaysToCheck = 61; // 61 to handle months with 31/30/31
        for (int i = -1; i <= DaysToCheck; i++) {
            time_t t0 = IncDay(t, i);
            if (GetMDay(t0) == day) {
               Day = SetTime(t0, 0);
               break;
               }
            }
        }
     }
  if (a || !isdigit(*s)) {
     if ((a && a - s == 7) || strlen(s) == 7) {
        for (const char *p = s + 6; p >= s; p--) {
            WeekDays <<= 1;
            WeekDays |= (*p != '-');
            }
        }
     else
        return false;
     }
  return true;
}

bool cTimer::IsSingleEvent(void) const
{
  return !weekdays;
}

int cTimer::GetMDay(time_t t)
{
  struct tm tm_r;
  return localtime_r(&t, &tm_r)->tm_mday;
}

int cTimer::GetWDay(time_t t)
{
  struct tm tm_r;
  int weekday = localtime_r(&t, &tm_r)->tm_wday;
  return weekday == 0 ? 6 : weekday - 1; // we start with Monday==0!
}

bool cTimer::DayMatches(time_t t) const
{
  return IsSingleEvent() ? SetTime(t, 0) == day : (weekdays & (1 << GetWDay(t))) != 0;
}

time_t cTimer::IncDay(time_t t, int Days)
{
  struct tm tm_r;
  tm tm = *localtime_r(&t, &tm_r);
  tm.tm_mday += Days; // now tm_mday may be out of its valid range
  int h = tm.tm_hour; // save original hour to compensate for DST change
  tm.tm_isdst = -1;   // makes sure mktime() will determine the correct DST setting
  t = mktime(&tm);    // normalize all values
  tm.tm_hour = h;     // compensate for DST change
  return mktime(&tm); // calculate final result
}

time_t cTimer::SetTime(time_t t, int SecondsFromMidnight)
{
  struct tm tm_r;
  tm tm = *localtime_r(&t, &tm_r);
  tm.tm_hour = SecondsFromMidnight / 3600;
  tm.tm_min = (SecondsFromMidnight % 3600) / 60;
  tm.tm_sec =  SecondsFromMidnight % 60;
  tm.tm_isdst = -1; // makes sure mktime() will determine the correct DST setting
  return mktime(&tm);
}

void cTimer::ClrFlags(uint Flags)
{
  flags &= ~Flags;
}

void cTimer::InvFlags(uint Flags)
{
  flags ^= Flags;
}

bool cTimer::HasFlags(uint Flags) const
{
  return (flags & Flags) == Flags;
}

bool cTimer::Matches(time_t t, bool Directly, int Margin) const
{
  startTime = stopTime = 0;
  if (t == 0)
     t = time(NULL);

  int begin  = TimeToInt(start); // seconds from midnight
  int length = TimeToInt(stop) - begin;
  if (length < 0)
     length += SECSINDAY;

  if (IsSingleEvent()) {
     startTime = SetTime(day, begin);
     stopTime = startTime + length;
     }
  else {
     for (int i = -1; i <= 7; i++) {
         time_t t0 = IncDay(day ? max(day, t) : t, i);
         if (DayMatches(t0)) {
            time_t a = SetTime(t0, begin);
            time_t b = a + length;
            if ((!day || a >= day) && t < b) {
               startTime = a;
               stopTime = b;
               break;
               }
            }
         }
     if (!startTime)
        startTime = IncDay(t, 7); // just to have something that's more than a week in the future
     else if (!Directly && (t > startTime || t > day + SECSINDAY + 3600)) // +3600 in case of DST change
        day = 0;
     }

  if (HasFlags(tfActive)) 
  {
    return startTime <= t + Margin && t < stopTime; // must stop *before* stopTime to allow adjacent timers
  }
  return false;
}

time_t cTimer::StartTime(void) const
{
  if (!startTime)
     Matches();
  return startTime;
}

time_t cTimer::StopTime(void) const
{
  if (!stopTime)
     Matches();
  return stopTime;
}

int cTimer::TimeToInt(int t)
{
  return (t / 100 * 60 + t % 100) * 60;
}
