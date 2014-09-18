// Thread.h: interface for the CThread class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_THREAD_H__ACFB7357_B961_4AC1_9FB2_779526219817__INCLUDED_) && !defined(AFX_THREAD_H__67621B15_8724_4B5D_9343_7667075C89F2__INCLUDED_)
#define AFX_THREAD_H__ACFB7357_B961_4AC1_9FB2_779526219817__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

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

#include "Event.h"

class IRunnable
{
public:
  virtual void Run()=0;
  virtual ~IRunnable() {}
};

#ifdef CTHREAD
#undef CTHREAD
#endif

// minimum as mandated by XTL
#define THREAD_MINSTACKSIZE 0x10000

class CThread
{
public:
  CThread();
  CThread(IRunnable* pRunnable);
  virtual ~CThread();
  void Create(bool bAutoDelete = false, unsigned stacksize = 0);
  DWORD ThreadId() const;
  bool WaitForThreadExit(DWORD dwMilliseconds);
  DWORD WaitForSingleObject(HANDLE hHandle, DWORD dwMilliseconds);
  DWORD WaitForMultipleObjects(DWORD nCount, CONST HANDLE *lpHandles, BOOL bWaitAll, DWORD dwMilliseconds);
  void Sleep(DWORD dwMilliseconds);
  bool SetPriority(const int iPriority);
  void SetName( LPCTSTR szThreadName );
  HANDLE ThreadHandle();
  operator HANDLE();
  operator const HANDLE() const;
  bool IsAutoDelete() const;
  virtual void StopThread();

  // returns the relative cpu usage of this thread since last call
  float GetRelativeUsage();

protected:
  virtual void OnStartup(){};
  virtual void OnExit(){};
  virtual void Process(); 
  volatile bool m_bStop;
  HANDLE m_ThreadHandle;

private:
  bool m_bAutoDelete;
  HANDLE m_StopEvent;
  unsigned m_ThreadId;
  IRunnable* m_pRunnable;

  unsigned __int64 m_iLastUsage;
  unsigned __int64 m_iLastTime;
  float m_fLastUsage;

private:
  static DWORD WINAPI staticThread(LPVOID* data);
};

#endif // !defined(AFX_THREAD_H__ACFB7357_B961_4AC1_9FB2_779526219817__INCLUDED_)
