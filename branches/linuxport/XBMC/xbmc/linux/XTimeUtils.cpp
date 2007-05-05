
#include "XTimeUtils.h"
#include <time.h>

#define WIN32_TIME_OFFSET ((unsigned long long)(369 * 365 + 89) * 24 * 3600 * 10000000)

#ifdef _LINUX

DWORD timeGetTime(void)
{
  struct timezone tz;
  struct timeval tim;
  gettimeofday(&tim, &tz);
  DWORD result = tim.tv_usec;
  result += ((DWORD) tim.tv_sec) * 1000;
  return result;
}

void Sleep(DWORD dwMilliSeconds)
{
  SDL_Delay(dwMilliSeconds);
}

VOID GetLocalTime(LPSYSTEMTIME sysTime) 
{
  const time_t t = time(NULL);
  struct tm* now = localtime(&t);
  sysTime->wYear = now->tm_year + 1900;
  sysTime->wMonth = now->tm_mon + 1;
  sysTime->wDayOfWeek = now->tm_wday;
  sysTime->wDay = now->tm_mday;
  sysTime->wHour = now->tm_hour;
  sysTime->wMinute = now->tm_min;
  sysTime->wSecond = now->tm_sec;
  sysTime->wMilliseconds = 0;
}

DWORD GetTickCount(void) 
{
  return SDL_GetTicks();
}

// "Load Skin XML: %.2fms", 1000.f * (end.QuadPart - start.QuadPart) / freq.QuadPart
BOOL QueryPerformanceCounter(LARGE_INTEGER *lpPerformanceCount) {
	return true;
}

BOOL QueryPerformanceFrequency(LARGE_INTEGER *lpFrequency) {
	return true;
}

BOOL FileTimeToLocalFileTime(const FILETIME* lpFileTime, LPFILETIME lpLocalFileTime) 
{
  return true;
}

BOOL   SystemTimeToFileTime(const SYSTEMTIME* lpSystemTime,  LPFILETIME lpFileTime) 
{
  struct tm sysTime;
  sysTime.tm_year = lpSystemTime->wYear - 1900;
  sysTime.tm_mon =  lpSystemTime->wMonth - 1;
  sysTime.tm_wday = lpSystemTime->wDayOfWeek;
  sysTime.tm_mday = lpSystemTime-> wDay;
  sysTime.tm_hour = lpSystemTime-> wHour;
  sysTime.tm_min = lpSystemTime-> wMinute;
  sysTime.tm_sec = lpSystemTime->wSecond;
  
  time_t t = mktime(&sysTime);

  ULARGE_INTEGER result;
  result.QuadPart = (unsigned long long) t * 10000000 + (unsigned long long) lpSystemTime->wMilliseconds * 10000;
  result.QuadPart += WIN32_TIME_OFFSET;
  
  lpFileTime->dwLowDateTime = result.LowPart;
  lpFileTime->dwHighDateTime = result.HighPart;
    
  return 1;
}

LONG   CompareFileTime(const FILETIME* lpFileTime1, const FILETIME* lpFileTime2) 
{
  ULARGE_INTEGER t1;
  t1.LowPart = lpFileTime1->dwLowDateTime;
  t1.HighPart = lpFileTime1->dwHighDateTime;

  ULARGE_INTEGER t2;
  t2.LowPart = lpFileTime2->dwLowDateTime;
  t2.HighPart = lpFileTime2->dwHighDateTime;

  if (t1.QuadPart == t2.QuadPart)
     return 0;
  else if (t1.QuadPart < t1.QuadPart)
     return -1;
  else
     return 1;
}

BOOL   FileTimeToSystemTime( const FILETIME* lpFileTime, LPSYSTEMTIME lpSystemTime) 
{
  ULARGE_INTEGER fileTime;
  fileTime.LowPart = lpFileTime->dwLowDateTime;
  fileTime.HighPart = lpFileTime->dwHighDateTime;

  fileTime.QuadPart -= WIN32_TIME_OFFSET;
  fileTime.QuadPart /= 10000; /* to milliseconds */
  lpSystemTime->wMilliseconds = fileTime.QuadPart % 1000;
  fileTime.QuadPart /= 1000; /* to seconds */
  
  time_t ft = fileTime.QuadPart;
  struct tm* tm_ft = gmtime(&ft);  
  lpSystemTime->wYear = tm_ft->tm_year + 1900;
  lpSystemTime->wMonth = tm_ft->tm_mon + 1;
  lpSystemTime->wDayOfWeek = tm_ft->tm_wday;
  lpSystemTime->wDay = tm_ft->tm_mday;
  lpSystemTime->wHour = tm_ft->tm_hour;
  lpSystemTime->wMinute = tm_ft->tm_min;
  lpSystemTime->wSecond = tm_ft->tm_sec;

  return 1;
}

BOOL   LocalFileTimeToFileTime( const FILETIME* lpLocalFileTime, LPFILETIME lpFileTime) 
{
  ULARGE_INTEGER l;
  l.LowPart = lpLocalFileTime->dwLowDateTime;
  l.HighPart = lpLocalFileTime->dwHighDateTime;
 
  l.QuadPart += (unsigned long long) timezone * 10000000;
  
  lpFileTime->dwLowDateTime = l.LowPart;
  lpFileTime->dwHighDateTime = l.HighPart;
  
  return 1;
}

#endif

