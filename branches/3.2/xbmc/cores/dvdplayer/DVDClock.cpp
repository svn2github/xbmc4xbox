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
 
#include "stdafx.h"
#include "DVDClock.h"
#include <math.h>

LARGE_INTEGER CDVDClock::m_systemFrequency;
CDVDClock::CDVDClock()
{
  if(!m_systemFrequency.QuadPart)
    QueryPerformanceFrequency(&m_systemFrequency);

  m_systemUsed = m_systemFrequency;
  m_pauseClock.QuadPart = 0;
  m_bReset = true;
  m_iDisc = 0;
}

CDVDClock::~CDVDClock()
{}

// Returns the current absolute clock in units of DVD_TIME_BASE (usually microseconds).
double CDVDClock::GetAbsoluteClock()
{
  if(!m_systemFrequency.QuadPart)
    QueryPerformanceFrequency(&m_systemFrequency);

  LARGE_INTEGER current;
  QueryPerformanceCounter(&current);

  return DVD_TIME_BASE * (double)current.QuadPart / m_systemFrequency.QuadPart;
}

double CDVDClock::GetClock()
{
  CSharedLock lock(m_critSection);
  LARGE_INTEGER current;
    
  if (m_bReset)
  {
    QueryPerformanceCounter(&m_startClock);
    m_systemUsed = m_systemFrequency;
    m_pauseClock.QuadPart = 0;
    m_iDisc = 0;
    m_bReset = false;
  }

  if (m_pauseClock.QuadPart)
    current = m_pauseClock;
  else
    QueryPerformanceCounter(&current);

  current.QuadPart -= m_startClock.QuadPart;
  return DVD_TIME_BASE * (double)current.QuadPart / m_systemUsed.QuadPart + m_iDisc;
}

void CDVDClock::SetSpeed(int iSpeed)
{
  // this will sometimes be a little bit of due to rounding errors, ie clock might jump abit when changing speed
  CExclusiveLock lock(m_critSection);

  if(iSpeed == DVD_PLAYSPEED_PAUSE)
  {
    if(!m_pauseClock.QuadPart)
      QueryPerformanceCounter(&m_pauseClock);
    return;
  }
  
  LARGE_INTEGER current;
  __int64 newfreq = m_systemFrequency.QuadPart * DVD_PLAYSPEED_NORMAL / iSpeed;
  
  QueryPerformanceCounter(&current);
  if( m_pauseClock.QuadPart )
  {
    m_startClock.QuadPart += current.QuadPart - m_pauseClock.QuadPart;
    m_pauseClock.QuadPart = 0;
  }

  m_startClock.QuadPart = current.QuadPart - ( newfreq * (current.QuadPart - m_startClock.QuadPart) ) / m_systemUsed.QuadPart;
  m_systemUsed.QuadPart = newfreq;    
}

void CDVDClock::Discontinuity(double currentPts)
{
  CExclusiveLock lock(m_critSection);
  QueryPerformanceCounter(&m_startClock);
  if(m_pauseClock.QuadPart)
    m_pauseClock.QuadPart = m_startClock.QuadPart;
  m_iDisc = currentPts;
  m_bReset = false;
}

void CDVDClock::Pause()
{
  CExclusiveLock lock(m_critSection);
  if(!m_pauseClock.QuadPart)
    QueryPerformanceCounter(&m_pauseClock);
}

void CDVDClock::Resume()
{
  CExclusiveLock lock(m_critSection);
  if( m_pauseClock.QuadPart )
  {
    LARGE_INTEGER current;
    QueryPerformanceCounter(&current);

    m_startClock.QuadPart += current.QuadPart - m_pauseClock.QuadPart;
    m_pauseClock.QuadPart = 0;
  }  
}

