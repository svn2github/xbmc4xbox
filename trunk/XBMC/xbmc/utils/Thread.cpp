/*
* XBoxMediaPlayer
* Copyright (c) 2002 Frodo
* Portions Copyright (c) by the authors of ffmpeg and xvid
*
* This program is free software; you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation; either version 2 of the License, or
* (at your option) any later version.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with this program; if not, write to the Free Software
* Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
*/

#include "../stdafx.h"
#include "Thread.h"
#include <process.h>


//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////
#ifndef _MT
#pragma message( "Please compile using multithreaded run-time libraries" )
#endif
typedef unsigned (WINAPI *PBEGINTHREADEX_THREADFUNC)(LPVOID lpThreadParameter);

#define MS_VC_EXCEPTION 0x406d1388
typedef struct tagTHREADNAME_INFO 
{ 
  DWORD dwType; // must be 0x1000 
  LPCSTR szName; // pointer to name (in same addr space) 
  DWORD dwThreadID; // thread ID (-1 caller thread) 
  DWORD dwFlags; // reserved for future use, most be zero 
} THREADNAME_INFO;

CThread::CThread()
{
  m_bStop = false;

  m_bAutoDelete = false;
  m_dwThreadId = 0;
  m_ThreadHandle = NULL;
  m_iLastTime = 0;
  m_iLastUsage = 0;
  m_fLastUsage = 0.0f;

  m_pRunnable=NULL;
}

CThread::CThread(IRunnable* pRunnable)
{
  m_bStop = false;

  m_bAutoDelete = false;
  m_dwThreadId = 0;
  m_ThreadHandle = NULL;
  m_iLastTime = 0;
  m_iLastUsage = 0;
  m_fLastUsage = 0.0f;

  m_pRunnable=pRunnable;
}

CThread::~CThread()
{
  if (m_ThreadHandle != NULL)
  {
    CloseHandle(m_ThreadHandle);
  }
  m_ThreadHandle = NULL;
}


DWORD WINAPI CThread::staticThread(LPVOID* data)
{
  //DBG"thread start");

  CThread* pThread = (CThread*)(data);
  bool bDelete( pThread->IsAutoDelete() );
  pThread->OnStartup();
  pThread->Process();
  pThread->OnExit();
  pThread->m_eventStop.Set();
  if ( bDelete )
  {
    delete pThread;
    pThread = NULL;
  }
  _endthreadex(123);
  return 0;
}

void CThread::Create(bool bAutoDelete)
{
  if (m_ThreadHandle != NULL)
  {
    throw 1; //ERROR should not b possible!!!
  }
  m_iLastTime = GetTickCount();
  m_iLastTime *= 10000;
  m_bAutoDelete = bAutoDelete;
  m_eventStop.Reset();
  m_bStop = false;
  m_ThreadHandle = (HANDLE)_beginthreadex(NULL, 0, (PBEGINTHREADEX_THREADFUNC)staticThread, (void*)this, 0, (unsigned*) & m_dwThreadId);
  //m_ThreadHandle = CreateThread(NULL,0,(LPTHREAD_START_ROUTINE)staticThread,(LPVOID)this,0,&m_dwThreadId);
}


bool CThread::IsAutoDelete() const
{
  return m_bAutoDelete;
}

void CThread::StopThread()
{
  m_bStop = true;
  if (m_ThreadHandle)
  {
    WaitForSingleObject(m_ThreadHandle, INFINITE);
    CloseHandle(m_ThreadHandle);
    m_ThreadHandle = NULL;
  }
}

unsigned long CThread::ThreadId() const
{
  return m_dwThreadId;
}


CThread::operator HANDLE()
{
  return m_ThreadHandle;
}

CThread::operator const HANDLE() const
{
  return m_ThreadHandle;
}

bool CThread::SetPriority(const int iPriority)
// Set thread priority
// Return true for success
{
  if (m_ThreadHandle)
  {
    return ( SetThreadPriority( m_ThreadHandle, iPriority ) == TRUE );
  }
  else
  {
    return false;
  }
}

#ifdef _XBOX
void CThread::SetName( LPCTSTR szThreadName )
{
  THREADNAME_INFO info; 
  info.dwType = 0x1000; 
  info.szName = szThreadName; 
  info.dwThreadID = m_dwThreadId; 
  info.dwFlags = 0; 
  __try 
  { 
    RaiseException(MS_VC_EXCEPTION, 0, sizeof(info) / sizeof(DWORD), (DWORD *)&info); 
  } 
  __except (EXCEPTION_CONTINUE_EXECUTION) 
  { 
  }  
}
#else
void CThread::SetName( LPCTSTR szThreadName )
{
}
#endif

bool CThread::WaitForThreadExit(DWORD dwmsTimeOut)
// Waits for thread to exit, timeout in given number of msec.
// Returns true when thread ended
{

  if (!m_ThreadHandle) return true;
  DWORD dwExitCode;
  WaitForSingleObject(m_ThreadHandle, dwmsTimeOut);

  GetExitCodeThread(m_ThreadHandle, &dwExitCode);
  if (dwExitCode != STILL_ACTIVE)
  {
    CloseHandle(m_ThreadHandle);
    m_ThreadHandle = NULL;
    return true;
  }
  return false;
}

HANDLE CThread::ThreadHandle()
{
  return m_ThreadHandle;
}

void CThread::Process()
{
  if(m_pRunnable)
    m_pRunnable->Run(); 
}

float CThread::GetRelativeUsage()
{
  unsigned __int64 iTime = GetTickCount();
  iTime *= 10000; // convert into 100ns tics

  // only update every 1 second
  if( iTime < m_iLastTime + 1000*10000 ) return m_fLastUsage;

  FILETIME CreationTime, ExitTime, UserTime, KernelTime;
  if( GetThreadTimes( m_ThreadHandle, &CreationTime, &ExitTime, &KernelTime, &UserTime ) )
  {    

    unsigned __int64 iUsage = 0;
    iUsage += (((unsigned __int64)UserTime.dwHighDateTime) << 32) + ((unsigned __int64)UserTime.dwLowDateTime);
    iUsage += (((unsigned __int64)KernelTime.dwHighDateTime) << 32) + ((unsigned __int64)KernelTime.dwLowDateTime);

    m_fLastUsage = (float)( iUsage - m_iLastUsage ) / (float)( iTime - m_iLastTime );
    
    m_iLastUsage = iUsage;
    m_iLastTime = iTime;

    return m_fLastUsage;
  }    
  return 0.0f; 
}