#ifndef __X_SYNC_UTILS_
#define __X_SYNC_UTILS_

#include "XHandle.h"

#ifdef _LINUX

#define STATUS_WAIT_0	((DWORD   )0x00000000L)    
#define WAIT_FAILED		((DWORD)0xFFFFFFFF)
#define WAIT_OBJECT_0	((STATUS_WAIT_0 ) + 0 )
#define WAIT_TIMEOUT	258L
#define INFINITE		0xFFFFFFFF

HANDLE	CreateMutex( LPSECURITY_ATTRIBUTES lpMutexAttributes,  BOOL bInitialOwner,  LPCTSTR lpName );
bool	InitializeRecursiveMutex(HANDLE hMutex, BOOL bInitialOwner);
bool	DestroyRecursiveMutex(HANDLE hMutex);
bool	ReleaseMutex( HANDLE hMutex );

void InitializeCriticalSection(LPCRITICAL_SECTION lpCriticalSection);
void DeleteCriticalSection(LPCRITICAL_SECTION lpCriticalSection);
void EnterCriticalSection(LPCRITICAL_SECTION lpCriticalSection);
void LeaveCriticalSection(LPCRITICAL_SECTION lpCriticalSection);

void GlobalMemoryStatus(LPMEMORYSTATUS lpBuffer);

DWORD WaitForSingleObject( HANDLE hHandle, DWORD dwMilliseconds );
DWORD WaitForMultipleObjects( DWORD nCount, HANDLE* lpHandles, BOOL bWaitAll,  DWORD dwMilliseconds);

#endif 

#endif

