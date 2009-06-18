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
#include "XBMC_PC.h"
#ifndef HAS_SDL
#include <d3d8.h>
#endif
#include "../../xbmc/Application.h"
#include "WIN32Util.h"
#include "shellapi.h"
#include "dbghelp.h"

//-----------------------------------------------------------------------------
// Resource defines
//-----------------------------------------------------------------------------

#include "resource.h"
#define IDI_MAIN_ICON          101 // Application icon
#define IDR_MAIN_ACCEL         113 // Keyboard accelerator

typedef BOOL (WINAPI *MINIDUMPWRITEDUMP)(HANDLE hProcess, DWORD dwPid, HANDLE hFile, MINIDUMP_TYPE DumpType,
                                        CONST PMINIDUMP_EXCEPTION_INFORMATION ExceptionParam,
                                        CONST PMINIDUMP_USER_STREAM_INFORMATION UserStreamParam,
                                        CONST PMINIDUMP_CALLBACK_INFORMATION CallbackParam);


//-----------------------------------------------------------------------------
// Globals
//-----------------------------------------------------------------------------

CApplication g_application;
CXBMC_PC *g_xbmcPC;

CXBMC_PC::CXBMC_PC()
{
  g_advancedSettings.m_startFullScreen = false;
}

CXBMC_PC::~CXBMC_PC()
{
  // todo: deinitialization code
}

HRESULT CXBMC_PC::Create( HINSTANCE hInstance, LPSTR commandLine )
{
  m_hInstance = hInstance;
  HRESULT hr = S_OK;

  CStdStringW strcl(commandLine);
  LPWSTR *szArglist;
  int nArgs;

  szArglist = CommandLineToArgvW(strcl.c_str(), &nArgs);
  if(szArglist != NULL)
  {
    for(int i=0;i<nArgs;i++)
    {
      CStdStringW strArgW(szArglist[i]);
      if(strArgW.Equals(L"-fs"))
        g_advancedSettings.m_startFullScreen = true;
      else if(strArgW.Equals(L"-p") || strArgW.Equals(L"--portable"))
        g_application.EnablePlatformDirectories(false);
      else if(strArgW.Equals(L"-d"))
      {
        if(++i < nArgs)
        {
          int iSleep = _wtoi(szArglist[i]);
          if(iSleep > 0 && iSleep < 360)
            Sleep(iSleep*1000);
          else
            --i;
        }
      }
    }
    LocalFree(szArglist);
  }

  return S_OK;
}


INT CXBMC_PC::Run()
{
  g_application.Create(NULL);

  // we don't want to see the "no disc in drive" windows message box
  SetErrorMode(SEM_FAILCRITICALERRORS|SEM_NOOPENFILEERRORBOX);

  g_application.Run();
  return 0;
}

// Minidump creation function 
LONG WINAPI CreateMiniDump( EXCEPTION_POINTERS* pEp ) 
{
  // Create the dump file where the xbmc.exe resides
  CStdString errorMsg;
  CStdString dumpFile;
  dumpFile.Format("xbmc.r%s.dmp", SVN_REV);
  HANDLE hFile = CreateFile(dumpFile.c_str(), GENERIC_READ | GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL ); 

  // Call MiniDumpWriteDump api with the dump file
  if ( hFile && ( hFile != INVALID_HANDLE_VALUE ) ) 
  {
    // Load the DBGHELP DLL
    HMODULE hDbgHelpDll = ::LoadLibrary("DBGHELP.DLL");       
    if (hDbgHelpDll)
    {
      MINIDUMPWRITEDUMP pDump = (MINIDUMPWRITEDUMP)::GetProcAddress(hDbgHelpDll, "MiniDumpWriteDump");
      if (pDump)
      {
        // Initialize minidump structure
        MINIDUMP_EXCEPTION_INFORMATION mdei; 
        mdei.ThreadId           = GetCurrentThreadId(); 
        mdei.ExceptionPointers  = pEp; 
        mdei.ClientPointers     = FALSE; 

        // Call the minidump api with normal dumping
        // We can get more detail information by using other minidump types but the dump file will be
        // extermely large.
        BOOL rv = pDump(GetCurrentProcess(), GetCurrentProcessId(), hFile, MiniDumpNormal, &mdei, 0, NULL); 
        if( !rv ) 
        {
          errorMsg.Format("MiniDumpWriteDump failed with error id %d", GetLastError());
          MessageBox(NULL, errorMsg.c_str(), "XBMC: Error", MB_OK|MB_ICONERROR); 
        } 
      }
      else
      {
        errorMsg.Format("MiniDumpWriteDump failed to load with error id %d", GetLastError());
        MessageBox(NULL, errorMsg.c_str(), "XBMC: Error", MB_OK|MB_ICONERROR);
      }
      
      // Close the DLL
      FreeLibrary(hDbgHelpDll);
    }
    else
    {
      errorMsg.Format("LoadLibrary 'DBGHELP.DLL' failed with error id %d", GetLastError());
      MessageBox(NULL, errorMsg.c_str(), "XBMC: Error", MB_OK|MB_ICONERROR);
    }

    // Close the file 
    CloseHandle( hFile ); 
  }
  else 
  {
    errorMsg.Format("CreateFile '%s' failed with error id %d", dumpFile.c_str(), GetLastError());
    MessageBox(NULL, errorMsg.c_str(), "XBMC: Error", MB_OK|MB_ICONERROR); 
  }

  return pEp->ExceptionRecord->ExceptionCode;;
}

//-----------------------------------------------------------------------------
// Name: WinMain()
// Desc: The application's entry point
//-----------------------------------------------------------------------------
INT WINAPI WinMain( HINSTANCE hInst, HINSTANCE, LPSTR commandLine, INT )
{
  // Initializes CreateMiniDump to handle exceptions.
  SetUnhandledExceptionFilter( CreateMiniDump );

  // check if XBMC is already running
  HWND m_hwnd = FindWindow("SDL_app","XBMC Media Center");
  if(m_hwnd != NULL)
  {
    // switch to the running instance
    ShowWindow(m_hwnd,SW_RESTORE);
    SetForegroundWindow(m_hwnd);
    return 0;
  }

  CXBMC_PC myApp;

  g_xbmcPC = &myApp;

  if(CWIN32Util::GetDesktopColorDepth() < 32)
  {
    //FIXME: replace it by a SDL window for all ports
    MessageBox(NULL, "Desktop Color Depth isn't 32Bit", "XBMC: Fatal Error", MB_OK|MB_ICONERROR);
    return 0;
  }

  // update the current drive mask
  CWIN32Util::UpdateDriveMask();

  if (FAILED(myApp.Create(hInst, commandLine)))
    return 1;

  return myApp.Run();
}
