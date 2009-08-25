/*
 *      Copyright (C) 2005-2009 Team XBMC
 *      http://www.xbmc.org
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License along
 *  with this program; if not, write to the Free Software Foundation, Inc.,
 *  51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 *
 */

#include "stdafx.h"
#include "XBApplicationEx.h"
#include "Settings.h"
#include "Application.h"
#ifdef HAS_PERFORMANCE_SAMPLE
#include "utils/PerformanceSample.h"
#else
#define MEASURE_FUNCTION
#endif
#include "GUIFontManager.h"


//-----------------------------------------------------------------------------
// Name: CXBApplication()
// Desc: Constructor
//-----------------------------------------------------------------------------
CXBApplicationEx::CXBApplicationEx()
{
  // Variables to perform app timing
  m_bStop = false;
  m_AppActive = true;
  m_AppFocused = true;
}

/* CXBApplicationEx() destructor */
CXBApplicationEx::~CXBApplicationEx()
{}

/* Create the app */
HRESULT CXBApplicationEx::Create(HWND hWnd)
{
  HRESULT hr;

  // Initialize the app's device-dependent objects
  if ( FAILED( hr = Initialize() ) )
  {
    CLog::Log(LOGERROR, "XBAppEx: Call to Initialize() failed!" );
    return hr;
  }

  return S_OK;
}

/* Destroy the app */
VOID CXBApplicationEx::Destroy()
{
  CLog::Log(LOGNOTICE, "destroy");
  // Perform app-specific cleanup
  Cleanup();
}

/* Function that runs the application */
INT CXBApplicationEx::Run()
{
  CLog::Log(LOGNOTICE, "Running the application..." );

  BYTE processExceptionCount = 0;
  BYTE frameMoveExceptionCount = 0;
  BYTE renderExceptionCount = 0;

#ifndef _DEBUG
  const BYTE MAX_EXCEPTION_COUNT = 10;
#endif

  // Run xbmc
  while (!m_bStop)
  {
#ifdef HAS_PERFORMANCE_SAMPLE
    CPerformanceSample sampleLoop("XBApplicationEx-loop");
#endif
    //-----------------------------------------
    // Animate and render a frame
    //-----------------------------------------
#ifndef _DEBUG
    try
    {
#endif
      Process();
      //reset exception count
      processExceptionCount = 0;

#ifndef _DEBUG

    }
    catch (...)
    {
      CLog::Log(LOGERROR, "exception in CApplication::Process()");
      processExceptionCount++;
      //MAX_EXCEPTION_COUNT exceptions in a row? -> bail out
      if (processExceptionCount > MAX_EXCEPTION_COUNT)
      {
        CLog::Log(LOGERROR, "CApplication::Process(), too many exceptions");
        throw;
      }
    }
#endif
    // Frame move the scene
#ifndef _DEBUG
    try
    {
#endif
      if (!m_bStop) FrameMove();
      //reset exception count
      frameMoveExceptionCount = 0;

#ifndef _DEBUG

    }
    catch (...)
    {
      CLog::Log(LOGERROR, "exception in CApplication::FrameMove()");
      frameMoveExceptionCount++;
      //MAX_EXCEPTION_COUNT exceptions in a row? -> bail out
      if (frameMoveExceptionCount > MAX_EXCEPTION_COUNT)
      {
        CLog::Log(LOGERROR, "CApplication::FrameMove(), too many exceptions");
        throw;
      }
    }
#endif

    // Render the scene
#ifndef _DEBUG
    try
    {
#endif
      if (!m_bStop) Render();
      //reset exception count
      renderExceptionCount = 0;

#ifndef _DEBUG

    }
    catch (...)
    {
      CLog::Log(LOGERROR, "exception in CApplication::Render()");
      renderExceptionCount++;
      //MAX_EXCEPTION_COUNT exceptions in a row? -> bail out
      if (renderExceptionCount > MAX_EXCEPTION_COUNT)
      {
        CLog::Log(LOGERROR, "CApplication::Render(), too many exceptions");
        throw;
      }
    }
#endif

  } // while (!m_bStop)
  Destroy();

  CLog::Log(LOGNOTICE, "application stopped..." );
  return 0;
}


/*
 * elis
 * 
bool CXBApplicationEx::ProcessWin32Shortcuts(SDL_Event& event)
{
  static bool alt = false;
  static CAction action;

  alt = !!(SDL_GetModState() & (XBMCXBMCKMOD_LALT | XBMCXBMCKMOD_RALT));

  if (event.key.type == SDL_KEYDOWN)
  {
    if(alt)
    {
      switch(event.key.keysym.sym)
      {
      case SDLK_F4:  // alt-F4 to quit
        if (!g_application.m_bStop)
          g_application.getApplicationMessenger().Quit();
      case SDLK_RETURN:  // alt-Return to toggle fullscreen
        {
          action.wID = ACTION_TOGGLE_FULLSCREEN;
          g_application.OnAction(action);
          return true;
        }
        return false;
      default:
        return false;
      }
    }
  }
  return false;
}

bool CXBApplicationEx::ProcessLinuxShortcuts(SDL_Event& event)
{
  bool alt = false;

  alt = !!(SDL_GetModState() & (XBMCXBMCKMOD_LALT  | XBMCXBMCKMOD_RALT));

  if (alt && event.key.type == SDL_KEYDOWN)
  {
    switch(event.key.keysym.sym)
    {
      case SDLK_TAB:  // ALT+TAB to minimize/hide
        g_application.Minimize();
        return true;
      default:
        break;
    }
  }
  return false;
 
}

#endif // HAS_SDL
*/
