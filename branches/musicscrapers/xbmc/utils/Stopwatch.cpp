#include "stdafx.h"
#include "stopwatch.h"

CStopWatch::CStopWatch()
{
  m_timerPeriod      = 0.0f;
  m_startTick        = 0;
  m_isRunning        = false;

  // Get the timer frequency (ticks per second)
  LARGE_INTEGER timerFreq;
  QueryPerformanceFrequency( &timerFreq );

  m_timerPeriod = 1.0f / (float)timerFreq.QuadPart;
}

CStopWatch::~CStopWatch()
{
}

bool CStopWatch::IsRunning() const
{
  return m_isRunning;
}

void CStopWatch::StartZero()
{
  m_startTick = GetTicks();
  m_isRunning = true;
}

void CStopWatch::Stop()
{
  if( m_isRunning )
  {
    m_startTick = 0;
    m_isRunning = false;
  }
}

void CStopWatch::Reset()
{
  if (m_isRunning)
    m_startTick = GetTicks();
}

float CStopWatch::GetElapsedSeconds() const
{
  LONGLONG totalTicks = m_isRunning ? (GetTicks() - m_startTick) : 0;
  return (FLOAT)totalTicks * m_timerPeriod;
}

float CStopWatch::GetElapsedMilliseconds() const
{
  return GetElapsedSeconds() * 1000.0f;
}

LONGLONG CStopWatch::GetTicks() const
{
  LARGE_INTEGER currTicks;
  QueryPerformanceCounter( &currTicks );
  return currTicks.QuadPart;
}
