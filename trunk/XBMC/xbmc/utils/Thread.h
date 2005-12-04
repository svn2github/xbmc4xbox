// Thread.h: interface for the CThread class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_THREAD_H__ACFB7357_B961_4AC1_9FB2_779526219817__INCLUDED_) && !defined(AFX_THREAD_H__67621B15_8724_4B5D_9343_7667075C89F2__INCLUDED_)
#define AFX_THREAD_H__ACFB7357_B961_4AC1_9FB2_779526219817__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "event.h"

class IRunnable
{
public:
  virtual void Run()=0;
};

#ifdef CTHREAD
#undef CTHREAD
#endif

class CThread
{
public:
  CThread();
  CThread(IRunnable* pRunnable);
  virtual ~CThread();
  void Create(bool bAutoDelete = false);
  unsigned long ThreadId() const;
  bool WaitForThreadExit(DWORD dwTimeOutSec);
  bool SetPriority(const int iPriority);
  void SetName( LPCTSTR szThreadName );
  HANDLE ThreadHandle();
  operator HANDLE();
  operator const HANDLE() const;
  bool IsAutoDelete() const;
  virtual void StopThread();

protected:
  virtual void OnStartup(){};
  virtual void OnExit(){};
  virtual void Process();
  CEvent m_eventStop;
  bool m_bAutoDelete;
  bool m_bStop;
  HANDLE m_ThreadHandle;
  DWORD m_dwThreadId;
  IRunnable* m_pRunnable;
private:
  static DWORD WINAPI CThread::staticThread(LPVOID* data);
};

#endif // !defined(AFX_THREAD_H__ACFB7357_B961_4AC1_9FB2_779526219817__INCLUDED_)
