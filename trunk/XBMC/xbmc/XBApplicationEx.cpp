//-----------------------------------------------------------------------------
// File: XBApplicationEx.cpp
//
// Desc: Application class for the XBox samples.
//
// Hist: 11.01.00 - New for November XDK release
//       12.15.00 - Changes for December XDK release
//       12.19.01 - Changes for March XDK release
//
// Copyright (c) Microsoft Corporation. All rights reserved.
//-----------------------------------------------------------------------------

#include "stdafx.h"
#include "XBApplicationEx.h"
#include <D3D8Perf.h>
#include "XBVideoConfig.h"


//-----------------------------------------------------------------------------
// Global access to common members
//-----------------------------------------------------------------------------
CXBApplicationEx* g_pXBApp = NULL;
static LPDIRECT3DDEVICE8 g_pd3dDevice = NULL;

// Deadzone for the gamepad inputs
const SHORT XINPUT_DEADZONE = (SHORT)( 0.24f * FLOAT(0x7FFF) );




//-----------------------------------------------------------------------------
// Name: CXBApplication()
// Desc: Constructor
//-----------------------------------------------------------------------------
CXBApplicationEx::CXBApplicationEx()
{
  // Initialize member variables
  g_pXBApp = this;

  // Direct3D variables
  m_pD3D = NULL;
  m_pd3dDevice = NULL;
  //    m_pDepthBuffer    = NULL;
  m_pBackBuffer = NULL;

  // Variables to perform app timing
  m_bPaused = FALSE;
  m_fTime = 0.0f;
  m_fElapsedTime = 0.0f;
  m_fAppTime = 0.0f;
  m_fElapsedAppTime = 0.0f;
  m_strFrameRate[0] = L'\0';
  m_bStop = false;

  // Set up the presentation parameters for a double-buffered, 640x480,
  // 32-bit display using depth-stencil. Override these parameters in
  // your derived class as your app requires.
  ZeroMemory( &m_d3dpp, sizeof(m_d3dpp) );
  m_d3dpp.BackBufferWidth = 720;
  m_d3dpp.BackBufferHeight = 576;
  m_d3dpp.BackBufferFormat = D3DFMT_LIN_A8R8G8B8;
  m_d3dpp.BackBufferCount = 1;
  m_d3dpp.EnableAutoDepthStencil = FALSE;
  m_d3dpp.AutoDepthStencilFormat = D3DFMT_LIN_D16;
  m_d3dpp.SwapEffect = D3DSWAPEFFECT_DISCARD;
  m_d3dpp.FullScreen_PresentationInterval = D3DPRESENT_INTERVAL_IMMEDIATE;

  // Specify number and type of input devices this app will be using. By
  // default, you can use 0 and NULL, which triggers XInputDevices() to
  // pre-alloc the default number and types of devices. To use chat or
  // other devices, override these variables in your derived class.
  m_dwNumInputDeviceTypes = 0;
  m_InputDeviceTypes = NULL;
}




//-----------------------------------------------------------------------------
// Name: Create()
// Desc: Create the app
//-----------------------------------------------------------------------------
HRESULT CXBApplicationEx::Create()
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




//-----------------------------------------------------------------------------
// Name: Destroy()
// Desc: Cleanup objects
//-----------------------------------------------------------------------------
VOID CXBApplicationEx::Destroy()
{
  // Perform app-specific cleanup
  Cleanup();

  // Release display objects
  SAFE_RELEASE( m_pd3dDevice );
  SAFE_RELEASE( m_pD3D );
}




//-----------------------------------------------------------------------------
// Name: Run()
// Desc:
//-----------------------------------------------------------------------------
INT CXBApplicationEx::Run()
{
  CLog::Log(LOGNOTICE, "Running the application..." );

  // Get the frequency of the timer
  LARGE_INTEGER qwTicksPerSec;
  QueryPerformanceFrequency( &qwTicksPerSec );
  FLOAT fSecsPerTick = 1.0f / (FLOAT)qwTicksPerSec.QuadPart;

  // Save the start time
  LARGE_INTEGER qwTime, qwLastTime, qwElapsedTime;
  QueryPerformanceCounter( &qwTime );
  qwLastTime.QuadPart = qwTime.QuadPart;

  LARGE_INTEGER qwAppTime, qwElapsedAppTime;
  qwAppTime.QuadPart = 0;
  qwElapsedTime.QuadPart = 0;
  qwElapsedAppTime.QuadPart = 0;

  BYTE processExceptionCount = 0;
  BYTE frameMoveExceptionCount = 0;
  BYTE renderExceptionCount = 0;

  const BYTE MAX_EXCEPTION_COUNT = 10;

  // Run the game loop, animating and rendering frames
  while (!m_bStop)
  {


    //-----------------------------------------
    // Perform app timing
    //-----------------------------------------

    // Check Start button
    if ( m_DefaultGamepad.wPressedButtons & XINPUT_GAMEPAD_START )
      m_bPaused = !m_bPaused;

    // Get the current time (keep in LARGE_INTEGER format for precision)
    QueryPerformanceCounter( &qwTime );
    qwElapsedTime.QuadPart = qwTime.QuadPart - qwLastTime.QuadPart;
    qwLastTime.QuadPart = qwTime.QuadPart;
    if ( m_bPaused )
      qwElapsedAppTime.QuadPart = 0;
    else
      qwElapsedAppTime.QuadPart = qwElapsedTime.QuadPart;
    qwAppTime.QuadPart += qwElapsedAppTime.QuadPart;

    // Store the current time values as floating point
    m_fTime = fSecsPerTick * ((FLOAT)(qwTime.QuadPart));
    m_fElapsedTime = fSecsPerTick * ((FLOAT)(qwElapsedTime.QuadPart));
    m_fAppTime = fSecsPerTick * ((FLOAT)(qwAppTime.QuadPart));
    m_fElapsedAppTime = fSecsPerTick * ((FLOAT)(qwElapsedAppTime.QuadPart));

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
      FrameMove();
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
      Render();
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

  }
  CLog::Log(LOGNOTICE, "application stopped..." );
  return 0;
}




//-----------------------------------------------------------------------------
// Name: RenderGradientBackground()
// Desc: Draws a gradient filled background
//-----------------------------------------------------------------------------
HRESULT CXBApplicationEx::RenderGradientBackground( DWORD dwTopColor,
    DWORD dwBottomColor )
{
  // First time around, allocate a vertex buffer
  static LPDIRECT3DVERTEXBUFFER8 g_pVB = NULL;
  if ( g_pVB == NULL )
  {
    m_pd3dDevice->CreateVertexBuffer( 4*5*sizeof(FLOAT), D3DUSAGE_WRITEONLY,
                                      0L, D3DPOOL_DEFAULT, &g_pVB );
    struct BACKGROUNDVERTEX { D3DXVECTOR4 p; D3DCOLOR color; };
    BACKGROUNDVERTEX* v;
    g_pVB->Lock( 0, 0, (BYTE**)&v, 0L );
    v[0].p = D3DXVECTOR4( 0 - 0.5f, 0 - 0.5f, 1.0f, 1.0f ); v[0].color = dwTopColor;
    v[1].p = D3DXVECTOR4( 640 - 0.5f, 0 - 0.5f, 1.0f, 1.0f ); v[1].color = dwTopColor;
    v[2].p = D3DXVECTOR4( 0 - 0.5f, 480 - 0.5f, 1.0f, 1.0f ); v[2].color = dwBottomColor;
    v[3].p = D3DXVECTOR4( 640 - 0.5f, 480 - 0.5f, 1.0f, 1.0f ); v[3].color = dwBottomColor;
    g_pVB->Unlock();
  }

  // Set states
  m_pd3dDevice->SetTexture( 0, NULL );
  m_pd3dDevice->SetTextureStageState( 0, D3DTSS_COLOROP, D3DTOP_DISABLE );
  m_pd3dDevice->SetRenderState( D3DRS_ZENABLE, FALSE );
  m_pd3dDevice->SetRenderState( D3DRS_ALPHABLENDENABLE, FALSE );
  m_pd3dDevice->SetVertexShader( D3DFVF_XYZRHW | D3DFVF_DIFFUSE );
  m_pd3dDevice->SetStreamSource( 0, g_pVB, 5*sizeof(FLOAT) );

  m_pd3dDevice->DrawPrimitive( D3DPT_TRIANGLESTRIP, 0, 2 );

  // Clear the zbuffer
  //m_pd3dDevice->Clear( 0, NULL, D3DCLEAR_ZBUFFER|D3DCLEAR_STENCIL, 0x00010001, 1.0f, 0L );

  return S_OK;
}

inline float DeadZone(float &f)
{
  if (f > g_stSettings.m_fAnalogDeadzoneController)
    return (f - g_stSettings.m_fAnalogDeadzoneController)/(1.0f - g_stSettings.m_fAnalogDeadzoneController);
  else if (f < -g_stSettings.m_fAnalogDeadzoneController)
    return (f + g_stSettings.m_fAnalogDeadzoneController)/(1.0f - g_stSettings.m_fAnalogDeadzoneController);
  else
    return 0.0f;
}

inline float MaxTrigger(XBGAMEPAD &gamepad)
{
  float max = fabs(gamepad.fX1);
  if (fabs(gamepad.fX2) > max) max = fabs(gamepad.fX2);
  if (fabs(gamepad.fY1) > max) max = fabs(gamepad.fY1);
  if (fabs(gamepad.fY2) > max) max = fabs(gamepad.fY2);
  return max;
}

void CXBApplicationEx::ReadInput()
{
  //-----------------------------------------
  // Handle input
  //-----------------------------------------

  // Read the input from the IR remote
  XBInput_GetInput( m_IR_Remote );
  ZeroMemory( &m_DefaultIR_Remote, sizeof(m_DefaultIR_Remote) );

  for ( DWORD i = 0; i < 4; i++ )
  {
    if ( m_IR_Remote[i].hDevice)
    {
      m_DefaultIR_Remote.wButtons = m_IR_Remote[i].wButtons;
    }
  }

  // Read the input from the mouse
  g_Mouse.Update();

  // Read the input from the keyboard
  g_Keyboard.Update();

  // Read the input for all connected gampads
  XBInput_GetInput( m_Gamepad );

  // Lump inputs of all connected gamepads into one common structure.
  // This is done so apps that need only one gamepad can function with
  // any gamepad.
  ZeroMemory( &m_DefaultGamepad, sizeof(m_DefaultGamepad) );

  float maxTrigger = 0.0f;
  for ( DWORD i = 0; i < 4; i++ )
  {
    if ( m_Gamepad[i].hDevice )
    {
      if (maxTrigger < MaxTrigger(m_Gamepad[i]))
      {
        maxTrigger = MaxTrigger(m_Gamepad[i]);
        m_DefaultGamepad.fX1 = m_Gamepad[i].fX1;
        m_DefaultGamepad.fY1 = m_Gamepad[i].fY1;
        m_DefaultGamepad.fX2 = m_Gamepad[i].fX2;
        m_DefaultGamepad.fY2 = m_Gamepad[i].fY2;
      }
      m_DefaultGamepad.wButtons |= m_Gamepad[i].wButtons;
      m_DefaultGamepad.wPressedButtons |= m_Gamepad[i].wPressedButtons;
      m_DefaultGamepad.wLastButtons |= m_Gamepad[i].wLastButtons;

      for ( DWORD b = 0; b < 8; b++ )
      {
        m_DefaultGamepad.bAnalogButtons[b] |= m_Gamepad[i].bAnalogButtons[b];
        m_DefaultGamepad.bPressedAnalogButtons[b] |= m_Gamepad[i].bPressedAnalogButtons[b];
        m_DefaultGamepad.bLastAnalogButtons[b] |= m_Gamepad[i].bLastAnalogButtons[b];
      }
    }
  }

  // Secure the deadzones of the analog sticks
  m_DefaultGamepad.fX1 = DeadZone(m_DefaultGamepad.fX1);
  m_DefaultGamepad.fY1 = DeadZone(m_DefaultGamepad.fY1);
  m_DefaultGamepad.fX2 = DeadZone(m_DefaultGamepad.fX2);
  m_DefaultGamepad.fY2 = DeadZone(m_DefaultGamepad.fY2);

}

void CXBApplicationEx::Process()
{}
