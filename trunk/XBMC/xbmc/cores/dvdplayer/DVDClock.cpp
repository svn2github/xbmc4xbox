
#include "../../stdafx.h"
#include "DVDClock.h"
#include <math.h>


CDVDClock::CDVDClock()
{
  QueryPerformanceFrequency(&m_systemFrequency);
  m_bReset = true;
  m_iPaused = 0I64;
  m_iDisc = 0I64;
}

CDVDClock::~CDVDClock()
{}

__int64 CDVDClock::GetClock()
{
  LARGE_INTEGER current;
  
  if (m_iPaused > 0) return m_iPaused;
  
  if (m_bReset)
  {
    QueryPerformanceCounter(&m_startClock);
    m_systemUsed.QuadPart = m_systemFrequency.QuadPart;
    m_iDisc = 0I64;
    m_iPaused = 0I64;
    m_bReset = false;
  }
  
  QueryPerformanceCounter(&current);

  current.QuadPart -= m_startClock.QuadPart;
  return current.QuadPart * DVD_TIME_BASE / m_systemUsed.QuadPart + m_iDisc;
}

void CDVDClock::Discontinuity(ClockDiscontinuityType type, __int64 currentPts)
{
  switch (type)
  {
  case CLOCK_DISC_FULL:
    {
      m_bReset = true;
      break;
    }
  case CLOCK_DISC_NORMAL:
    {
      //Reset speed to normal
      m_systemUsed.QuadPart = m_systemFrequency.QuadPart;

      QueryPerformanceCounter(&m_startClock);
      m_iDisc = currentPts;

      break;
    }
  }
}

void CDVDClock::Pause()
{
  m_iPaused = GetClock();
}

void CDVDClock::Resume()
{
  m_bReset = true;
  m_iPaused = 0I64;
}
  
bool CDVDClock::HadDiscontinuity(__int64 delay)
{
  if(m_iDisc + delay > GetClock())
    return true;
  else
    return false;
}

void CDVDClock::AdjustSpeedToMatch(__int64 currPts )
{
  //Placeholder for now, haven't been able to figure out a good routine for this yet.

  //__int64 diff = ABS(GetClock() - currPts);
  //int speedpercent;


  //if(diff < 1000)
  //  speedpercent = 0;
  //else if(diff < 5000)
  //  speedpercent = 1;
  //else if(diff < 10000)
  //  speedpercent = 5;
  //else if(diff < 100000)
  //  speedpercent = 20;
  //else
  //  speedpercent = 50;

  //  if(currPts > GetClock())
  //  {
  //    speedpercent*=-1;
  //  }
  //  speedpercent+=10000;

  //  __int64 newfreq = speedpercent*m_systemFrequency.QuadPart/10000;

  //  LARGE_INTEGER current;
  //  QueryPerformanceCounter(&current);
  //  m_startClock.QuadPart += current.QuadPart/newfreq - current.QuadPart/m_systemUsed.QuadPart;
  //  m_systemUsed.QuadPart = newfreq;

  //  CLog::DebugLog("Diff: %I64d, FreqDiff: %I64d ticks",diff, m_systemFrequency.QuadPart - m_systemUsed.QuadPart);
}