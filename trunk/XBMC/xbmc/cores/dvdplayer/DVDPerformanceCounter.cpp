
#include "../../stdafx.h"
#include "DVDPerformanceCounter.h"
#include "DVDMessageQueue.h"

// enable and use xbperfmon
//#define DVD_WITH_PERFORMANCE_COUNTER

HRESULT __stdcall DVDPerformanceCounterAudioQueue(PLARGE_INTEGER numerator, PLARGE_INTEGER demoninator)
{
  numerator->QuadPart = 0LL;
  //g_dvdPerformanceCounter.Lock();
  if (g_dvdPerformanceCounter.m_pAudioQueue)
  {
    int iSize     = g_dvdPerformanceCounter.m_pAudioQueue->GetDataSize();
    int iMaxSize  = g_dvdPerformanceCounter.m_pAudioQueue->GetMaxDataSize();
    if (iMaxSize > 0)
    {
      int iPercent  = (iSize * 100) / iMaxSize;
      if (iPercent > 100) iPercent = 100;
      numerator->QuadPart = iPercent;
    }
  }
  //g_dvdPerformanceCounter.Unlock();
  return S_OK;
}

HRESULT __stdcall DVDPerformanceCounterVideoQueue(PLARGE_INTEGER numerator, PLARGE_INTEGER demoninator)
{
  numerator->QuadPart = 0LL;
  //g_dvdPerformanceCounter.Lock();
  if (g_dvdPerformanceCounter.m_pAudioQueue)
  {
    int iSize     = g_dvdPerformanceCounter.m_pVideoQueue->GetDataSize();
    int iMaxSize  = g_dvdPerformanceCounter.m_pVideoQueue->GetMaxDataSize();
    if (iMaxSize > 0)
    {
      int iPercent  = (iSize * 100) / iMaxSize;
      if (iPercent > 100) iPercent = 100;
      numerator->QuadPart = iPercent;
    }
  }
  //g_dvdPerformanceCounter.Unlock();
  return S_OK;
}

inline __int64 get_thread_cpu_usage(ProcessPerformance* p)
{
  if (p->hThread)
  {
    FILETIME dummy;
    FILETIME current_time_thread;
    FILETIME current_time_system;
    ULARGE_INTEGER old_time_thread;
    ULARGE_INTEGER old_time_system;
    
    old_time_thread.QuadPart = p->timer_thread.QuadPart;
    old_time_system.QuadPart = p->timer_system.QuadPart;
    
    GetThreadTimes(p->hThread, &dummy, &dummy, &current_time_thread, &dummy);
    GetSystemTimeAsFileTime(&current_time_system);
    
    FILETIME_TO_ULARGE_INTEGER(p->timer_thread, current_time_thread);
    FILETIME_TO_ULARGE_INTEGER(p->timer_system, current_time_system);

    __int64 threadTime = (p->timer_thread.QuadPart - old_time_thread.QuadPart);
    __int64 systemTime = (p->timer_system.QuadPart - old_time_system.QuadPart);

    if (systemTime > 0 && threadTime > 0) return ((threadTime * 100) / systemTime);
  }
  return 0LL;
}

HRESULT __stdcall DVDPerformanceCounterVideoDecodePerformance(PLARGE_INTEGER numerator, PLARGE_INTEGER demoninator)
{
  //g_dvdPerformanceCounter.Lock();
  numerator->QuadPart = get_thread_cpu_usage(&g_dvdPerformanceCounter.m_videoDecodePerformance);
  //g_dvdPerformanceCounter.Unlock();
  return S_OK;
}

HRESULT __stdcall DVDPerformanceCounterAudioDecodePerformance(PLARGE_INTEGER numerator, PLARGE_INTEGER demoninator)
{
  //g_dvdPerformanceCounter.Lock();
  numerator->QuadPart = get_thread_cpu_usage(&g_dvdPerformanceCounter.m_audioDecodePerformance);
  //g_dvdPerformanceCounter.Unlock();
  return S_OK;
}

HRESULT __stdcall DVDPerformanceCounterMainPerformance(PLARGE_INTEGER numerator, PLARGE_INTEGER demoninator)
{
  //g_dvdPerformanceCounter.Lock();
  numerator->QuadPart = get_thread_cpu_usage(&g_dvdPerformanceCounter.m_mainPerformance);
  //g_dvdPerformanceCounter.Unlock();
  return S_OK;
}

CDVDPerformanceCounter g_dvdPerformanceCounter;

CDVDPerformanceCounter::CDVDPerformanceCounter()
{
  m_pAudioQueue = NULL;
  m_pVideoQueue = NULL;
  
  memset(&m_videoDecodePerformance, 0, sizeof(m_videoDecodePerformance)); // video decoding
  memset(&m_audioDecodePerformance, 0, sizeof(m_audioDecodePerformance)); // audio decoding + output to audio device
  memset(&m_mainPerformance,        0, sizeof(m_mainPerformance));        // reading files, demuxing, decoding of subtitles + menu overlays
  
  InitializeCriticalSection(&m_critSection);
  
  Initialize();
}

CDVDPerformanceCounter::~CDVDPerformanceCounter()
{
  DeInitialize();
  DeleteCriticalSection(&m_critSection);
}

bool CDVDPerformanceCounter::Initialize()
{
  Lock();
  
#ifdef DVD_WITH_PERFORMANCE_COUNTER

  DmRegisterPerformanceCounter("DVDAudioQueue",               DMCOUNT_SYNC, DVDPerformanceCounterAudioQueue);
  DmRegisterPerformanceCounter("DVDVideoQueue",               DMCOUNT_SYNC, DVDPerformanceCounterVideoQueue);
  DmRegisterPerformanceCounter("DVDVideoDecodePerformance",   DMCOUNT_SYNC, DVDPerformanceCounterVideoDecodePerformance);
  DmRegisterPerformanceCounter("DVDAudioDecodePerformance",   DMCOUNT_SYNC, DVDPerformanceCounterAudioDecodePerformance);
  DmRegisterPerformanceCounter("DVDMainPerformance",          DMCOUNT_SYNC, DVDPerformanceCounterMainPerformance);

#endif

  Unlock();

  return true;
}

void CDVDPerformanceCounter::DeInitialize()
{
  Lock();
  
  Unlock();
}
