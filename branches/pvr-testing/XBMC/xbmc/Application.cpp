/*
 *      Copyright (C) 2005-2008 Team XBMC
 *      http://xbmc.org
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
#include "Application.h"
#include "KeyboardLayoutConfiguration.h"
#include "Util.h"
#include "Picture.h"
#include "TextureManager.h"
#include "cores/PlayerCoreFactory.h"
#include "cores/dvdplayer/DVDFileInfo.h"
#include "PlayListPlayer.h"
#include "Autorun.h"
#include "ActionManager.h"
#ifdef HAS_LCD
#include "utils/LCDFactory.h"
#else
#include "GUILabelControl.h"  // needed for CInfoLabel
#include "GUIImage.h"
#endif
#include "GUIControlProfiler.h"
#include "XBVideoConfig.h"
#include "LangCodeExpander.h"
#include "utils/GUIInfoManager.h"
#include "PlayListFactory.h"
#include "GUIFontManager.h"
#include "GUIColorManager.h"
#include "SkinInfo.h"
#ifdef HAS_PYTHON
#include "lib/libPython/XBPython.h"
#endif
#include "ButtonTranslator.h"
#include "GUIAudioManager.h"
#include "lib/libscrobbler/lastfmscrobbler.h"
#include "lib/libscrobbler/librefmscrobbler.h"
#include "GUIPassword.h"
#include "ApplicationMessenger.h"
#include "SectionLoader.h"
#include "cores/DllLoader/DllLoaderContainer.h"
#include "GUIUserMessages.h"
#include "FileSystem/DirectoryCache.h"
#include "FileSystem/StackDirectory.h"
#include "FileSystem/SpecialProtocol.h"
#include "FileSystem/DllLibCurl.h"
#include "FileSystem/CMythSession.h"
#include "FileSystem/PluginDirectory.h"
#ifdef HAS_FILESYSTEM_SAP
#include "FileSystem/SAPDirectory.h"
#endif
#ifdef HAS_FILESYSTEM_HTSP
#include "FileSystem/HTSPDirectory.h"
#endif
#include "utils/TuxBoxUtil.h"
#include "utils/SystemInfo.h"
#include "ApplicationRenderer.h"
#include "GUILargeTextureManager.h"
#include "LastFmManager.h"
#include "PVRManager.h"
#include "SmartPlaylist.h"
#include "FileSystem/RarManager.h"
#include "PlayList.h"
#include "Surface.h"
#include "PowerManager.h"
#include "DPMSSupport.h"

#if defined(FILESYSTEM) && !defined(_LINUX)
#include "FileSystem/FileDAAP.h"
#endif
#ifdef HAS_UPNP
#include "UPnP.h"
#include "FileSystem/UPnPDirectory.h"
#endif
#if defined(_LINUX) && defined(HAS_FILESYSTEM_SMB)
#include "FileSystem/SMBDirectory.h"
#endif
#include "PartyModeManager.h"
#ifdef HAS_VIDEO_PLAYBACK
#include "cores/VideoRenderers/RenderManager.h"
#endif
#ifdef HAS_KARAOKE
#include "karaoke/karaokelyricsmanager.h"
#include "karaoke/GUIDialogKaraokeSongSelector.h"
#include "karaoke/GUIWindowKaraokeLyrics.h"
#endif
#include "AudioContext.h"
#include "GUIFontTTF.h"
#include "utils/Network.h"
#include "Zeroconf.h"
#include "ZeroconfBrowser.h"
#ifndef _LINUX
#include "utils/Win32Exception.h"
#endif
#ifdef HAS_WEB_SERVER
#include "lib/libGoAhead/XBMChttp.h"
#include "lib/libGoAhead/WebServer.h"
#endif
#ifdef HAS_FTP_SERVER
#include "lib/libfilezilla/xbfilezilla.h"
#endif
#ifdef HAS_TIME_SERVER
#include "utils/Sntp.h"
#endif
#ifdef HAS_XFONT
#include <xfont.h>  // for textout functions
#endif
#ifdef HAS_EVENT_SERVER
#include "utils/EventServer.h"
#endif
#ifdef HAS_DBUS_SERVER
#include "utils/DbusServer.h"
#endif


// Windows includes
#include "GUIWindowManager.h"
#include "GUIWindowHome.h"
#include "GUIStandardWindow.h"
#include "GUIWindowSettings.h"
#include "GUIWindowFileManager.h"
#include "GUIWindowSettingsCategory.h"
#include "GUIWindowMusicPlaylist.h"
#include "GUIWindowMusicSongs.h"
#include "GUIWindowMusicNav.h"
#include "GUIWindowMusicPlaylistEditor.h"
#include "GUIWindowVideoPlaylist.h"
#include "GUIWindowMusicInfo.h"
#include "GUIWindowVideoInfo.h"
#include "GUIWindowVideoFiles.h"
#include "GUIWindowVideoNav.h"
#include "GUIWindowSettingsProfile.h"
#include "GUIWindowTestPattern.h"
#include "GUIWindowSettingsScreenCalibration.h"
#include "GUIWindowPrograms.h"
#include "GUIWindowPictures.h"
#include "GUIWindowScripts.h"
#include "GUIWindowTV.h"
#include "GUIWindowWeather.h"
#include "GUIWindowLoginScreen.h"
#include "GUIWindowVisualisation.h"
#include "GUIWindowSystemInfo.h"
#include "GUIWindowScreensaver.h"
#include "GUIWindowSlideShow.h"
#include "GUIWindowStartup.h"
#include "GUIWindowFullScreen.h"
#include "GUIWindowOSD.h"
#include "GUIWindowMusicOverlay.h"
#include "GUIWindowVideoOverlay.h"

// Dialog includes
#include "GUIDialogMusicOSD.h"
#include "GUIDialogVisualisationSettings.h"
#include "GUIDialogVisualisationPresetList.h"
#include "GUIWindowScriptsInfo.h"
#include "GUIDialogNetworkSetup.h"
#include "GUIDialogMediaSource.h"
#include "GUIDialogVideoSettings.h"
#include "GUIDialogAudioSubtitleSettings.h"
#include "GUIDialogVideoBookmarks.h"
#include "GUIDialogProfileSettings.h"
#include "GUIDialogLockSettings.h"
#include "GUIDialogContentSettings.h"
#include "GUIDialogVideoScan.h"
#include "GUIDialogBusy.h"

#include "GUIDialogKeyboard.h"
#include "GUIDialogYesNo.h"
#include "GUIDialogOK.h"
#include "GUIDialogProgress.h"
#include "GUIDialogSelect.h"
#include "GUIDialogFileStacking.h"
#include "GUIDialogNumeric.h"
#include "GUIDialogGamepad.h"
#include "GUIDialogSubMenu.h"
#include "GUIDialogFavourites.h"
#include "GUIDialogButtonMenu.h"
#include "GUIDialogContextMenu.h"
#include "GUIDialogMusicScan.h"
#include "GUIDialogEpgScan.h"
#include "GUIDialogPlayerControls.h"
#include "GUIDialogSongInfo.h"
#include "GUIDialogSmartPlaylistEditor.h"
#include "GUIDialogSmartPlaylistRule.h"
#include "GUIDialogPictureInfo.h"
#include "GUIDialogAddonSettings.h"
#ifdef HAS_LINUX_NETWORK
#include "GUIDialogAccessPoints.h"
#endif
#include "GUIDialogFullScreenInfo.h"
#include "GUIDialogTVEPGProgInfo.h"
#include "GUIDialogTVRecordingInfo.h"
#include "GUIDialogTVTimerSettings.h"
#include "GUIDialogTVChannels.h"
#include "GUIDialogTVGuide.h"
#include "GUIDialogTVGroupManager.h"
#include "GUIDialogTVTeletext.h"
#include "GUIDialogSlider.h"
#include "cores/dlgcache.h"

#ifdef HAS_PERFORMANCE_SAMPLE
#include "utils/PerformanceSample.h"
#else
#define MEASURE_FUNCTION
#endif

#ifdef HAS_SDL_AUDIO
#include <SDL/SDL_mixer.h>
#endif
#if defined(HAS_SDL) && defined(_WIN32)
#include <SDL/SDL_syswm.h>
#endif
#ifdef _WIN32
#include <shlobj.h>
#include "win32util.h"
#endif
#ifdef HAS_XRANDR
#include "XRandR.h"
#endif
#ifdef __APPLE__
#include "CocoaInterface.h"
#include "XBMCHelper.h"
#endif
#ifdef HAS_HAL
#include "linux/LinuxFileSystem.h"
#endif
#ifdef HAS_EVENT_SERVER
#include "utils/EventServer.h"
#endif
#ifdef HAVE_LIBVDPAU
#include "cores/dvdplayer/DVDCodecs/Video/VDPAU.h"
#endif
#ifdef HAS_DBUS_SERVER
#include "utils/DbusServer.h"
#endif

#include "lib/libcdio/logging.h"
#include "MediaManager.h"

#ifdef _LINUX
#include "XHandle.h"
#endif

using namespace std;
using namespace XFILE;
using namespace DIRECTORY;
using namespace MEDIA_DETECT;
using namespace PLAYLIST;
using namespace VIDEO;
using namespace MUSIC_INFO;
#ifdef HAS_EVENT_SERVER
using namespace EVENTSERVER;
#endif
#ifdef HAS_DBUS_SERVER
using namespace DBUSSERVER;
#endif

// uncomment this if you want to use release libs in the debug build.
// Atm this saves you 7 mb of memory
#define USE_RELEASE_LIBS

#if defined(_WIN32)
 #if defined(_DEBUG) && !defined(USE_RELEASE_LIBS)
  #if defined(HAS_FILESYSTEM)
    #pragma comment (lib,"../../xbmc/lib/libXBMS/libXBMSd.lib")    // SECTIONNAME=LIBXBMS
    #pragma comment (lib,"../../xbmc/lib/libxdaap/libxdaapd.lib") // SECTIONNAME=LIBXDAAP
    #pragma comment (lib,"../../xbmc/lib/libRTV/libRTVd_win32.lib")
  #endif
  #pragma comment (lib,"../../xbmc/lib/libGoAhead/goahead_win32d.lib") // SECTIONNAME=LIBHTTP
  #pragma comment (lib,"../../xbmc/lib/sqLite/libSQLite3_win32d.lib")
  #pragma comment (lib,"../../xbmc/lib/libshout/libshout_win32d.lib" )
  #pragma comment (lib,"../../xbmc/lib/libcdio/libcdio_win32d.lib" )
  #pragma comment (lib,"../../xbmc/lib/libiconv/libiconvd.lib")
  #pragma comment (lib,"../../xbmc/lib/libfribidi/libfribidid.lib")
  #pragma comment (lib,"../../xbmc/lib/libpcre/libpcred.lib")
  #pragma comment (lib,"../../xbmc/lib/libsamplerate/libsamplerate_win32d.lib")
 #else
  #ifdef HAS_FILESYSTEM
    #pragma comment (lib,"../../xbmc/lib/libXBMS/libXBMS.lib")
    #pragma comment (lib,"../../xbmc/lib/libxdaap/libxdaap.lib")
    #pragma comment (lib,"../../xbmc/lib/libRTV/libRTV_win32.lib")
  #endif
  #pragma comment (lib,"../../xbmc/lib/libGoAhead/goahead_win32.lib")
  #pragma comment (lib,"../../xbmc/lib/sqLite/libSQLite3_win32.lib")
  #pragma comment (lib,"../../xbmc/lib/libshout/libshout_win32.lib" )
  #pragma comment (lib,"../../xbmc/lib/libcdio/libcdio_win32.lib" )
  #pragma comment (lib,"../../xbmc/lib/libiconv/libiconv.lib")
  #pragma comment (lib,"../../xbmc/lib/libfribidi/libfribidi.lib")
  #pragma comment (lib,"../../xbmc/lib/libpcre/libpcre.lib")
  #pragma comment (lib,"../../xbmc/lib/libsamplerate/libsamplerate_win32.lib")
 #endif
#endif

#define MAX_FFWD_SPEED 5

CStdString g_LoadErrorStr;

//extern IDirectSoundRenderer* m_pAudioDecoder;
CApplication::CApplication(void) : m_itemCurrentFile(new CFileItem), m_progressTrackingItem(new CFileItem)
{
  m_iPlaySpeed = 1;
#ifdef HAS_WEB_SERVER
  m_pWebServer = NULL;
  m_pXbmcHttp = NULL;
  m_prevMedia="";
#endif
  m_pFileZilla = NULL;
  m_pPlayer = NULL;
  m_bScreenSave = false;
  m_dpms = NULL;
  m_dpmsIsActive = false;
  m_iScreenSaveLock = 0;
  m_dwSkinTime = 0;
  m_bInitializing = true;
  m_eForcedNextPlayer = EPC_NONE;
  m_strPlayListFile = "";
  m_nextPlaylistItem = -1;
  m_bPlaybackStarting = false;

#ifdef HAS_GLX
  XInitThreads();
#endif

  //true while we in IsPaused mode! Workaround for OnPaused, which must be add. after v2.0
  m_bIsPaused = false;

  m_bWasFullScreenBeforeMinimize = false;

  /* for now always keep this around */
#ifdef HAS_KARAOKE
  m_pKaraokeMgr = new CKaraokeLyricsManager();
#endif
  m_currentStack = new CFileItemList;

#ifdef HAS_SDL
  m_frameCount = 0;
  m_frameMutex = SDL_CreateMutex();
  m_frameCond = SDL_CreateCond();
#endif

  m_bPresentFrame = false;
  m_bPlatformDirectories = true;

  m_bStandalone = false;
  m_bEnableLegacyRes = false;
  m_bRunResumeJobs = false;
  m_lastActionCode = 0;
#ifdef _WIN32PC
  m_SSysParam = new CWIN32Util::SystemParams::SysParam;
#endif
}

CApplication::~CApplication(void)
{
  delete m_currentStack;

#ifdef HAS_KARAOKE
  if(m_pKaraokeMgr)
    delete m_pKaraokeMgr;
#endif

#ifdef HAS_SDL
  if (m_frameMutex)
    SDL_DestroyMutex(m_frameMutex);

  if (m_frameCond)
    SDL_DestroyCond(m_frameCond);
#endif
  delete m_dpms;

#ifdef _WIN32PC
  if( m_SSysParam )
    delete m_SSysParam;
#endif
}

// text out routine for below
#ifdef HAS_XFONT
static void __cdecl FEH_TextOut(XFONT* pFont, int iLine, const wchar_t* fmt, ...)
{
  wchar_t buf[100];
  va_list args;
  va_start(args, fmt);
  _vsnwprintf(buf, 100, fmt, args);
  va_end(args);

  if (!(iLine & 0x8000))
    CLog::Log(LOGFATAL, "%S", buf);

  bool Center = (iLine & 0x10000) > 0;
  pFont->SetTextAlignment(Center ? XFONT_TOP | XFONT_CENTER : XFONT_TOP | XFONT_LEFT);

  iLine &= 0x7fff;

  for (int i = 0; i < 2; i++)
  {
    D3DRECT rc = { 0, 50 + 25 * iLine, 720, 50 + 25 * (iLine + 1) };
    D3DDevice::Clear(1, &rc, D3DCLEAR_TARGET, 0, 0, 0);
    pFont->TextOut(g_application.m_pBackBuffer, buf, -1, Center ? 360 : 80, 50 + 25*iLine);
    D3DDevice::Present(0, 0, 0, 0);
  }
}
#endif

HWND g_hWnd = NULL;

void CApplication::InitBasicD3D()
{
#ifndef HAS_SDL
  bool bPal = g_videoConfig.HasPAL();
  CLog::Log(LOGINFO, "Init display in default mode: %s", bPal ? "PAL" : "NTSC");
  // init D3D with defaults (NTSC or PAL standard res)
  m_d3dpp.BackBufferWidth = 720;
  m_d3dpp.BackBufferHeight = bPal ? 576 : 480;
  m_d3dpp.BackBufferFormat = D3DFMT_LIN_X8R8G8B8;
  m_d3dpp.BackBufferCount = 1;
  m_d3dpp.EnableAutoDepthStencil = FALSE;
  m_d3dpp.SwapEffect = D3DSWAPEFFECT_COPY;
  m_d3dpp.PresentationInterval = 0;
  m_d3dpp.Windowed = TRUE;
  m_d3dpp.hDeviceWindow = g_hWnd;

  if (!(m_pD3D = Direct3DCreate9(D3D_SDK_VERSION)))
  {
    CLog::Log(LOGFATAL, "FATAL ERROR: Unable to create Direct3D!");
    Sleep(INFINITE); // die
  }
#endif

  // Check if we have the required modes available
#ifndef HAS_SDL
  g_videoConfig.GetModes(m_pD3D);
#else
  g_videoConfig.GetModes();
#endif
  if (!g_graphicsContext.IsValidResolution(g_guiSettings.m_LookAndFeelResolution))
  {
    // Oh uh - doesn't look good for starting in their wanted screenmode
    CLog::Log(LOGERROR, "The screen resolution requested is not valid, resetting to a valid mode");
    g_guiSettings.m_LookAndFeelResolution = g_videoConfig.GetSafeMode();
    CLog::Log(LOGERROR, "Resetting to mode %s", g_settings.m_ResInfo[g_guiSettings.m_LookAndFeelResolution].strMode);
    CLog::Log(LOGERROR, "Done reset");
  }

  // Transfer the resolution information to our graphics context
#ifndef HAS_SDL
  g_graphicsContext.SetD3DParameters(&m_d3dpp);
#endif
  g_graphicsContext.SetVideoResolution(g_guiSettings.m_LookAndFeelResolution, TRUE);

  // Create the device
#if !defined(HAS_SDL)
  if (m_pD3D->CreateDevice(0, D3DDEVTYPE_REF, NULL, D3DCREATE_SOFTWARE_VERTEXPROCESSING, &m_d3dpp, &m_pd3dDevice) != S_OK)
  {
    CLog::Log(LOGFATAL, "FATAL ERROR: Unable to create D3D Device!");
    Sleep(INFINITE); // die
  }
#endif

  if (m_splash)
  {
#ifndef HAS_SDL_OPENGL
    m_splash->Stop();
#else
    m_splash->Hide();
#endif
  }

#ifndef HAS_SDL

  m_pd3dDevice->Clear(0, NULL, D3DCLEAR_TARGET, 0, 0, 0);
  m_pd3dDevice->Present( NULL, NULL, NULL, NULL );
#endif
}

// This function does not return!
void CApplication::FatalErrorHandler(bool InitD3D, bool MapDrives, bool InitNetwork)
{
  // XBMC couldn't start for some reason...
  // g_LoadErrorStr should contain the reason
  fprintf(stderr, "Fatal error encountered, aborting\n");
  fprintf(stderr, "Error log at %sxbmc.log\n", g_stSettings.m_logFolder.c_str());
  abort();
}

extern "C" void __stdcall init_emu_environ();
extern "C" void __stdcall update_emu_environ();

//
// Utility function used to copy files from the application bundle
// over to the user data directory in Application Support/XBMC.
//
static void CopyUserDataIfNeeded(const CStdString &strPath, const CStdString &file)
{
  CStdString destPath = CUtil::AddFileToFolder(strPath, file);
  if (!CFile::Exists(destPath))
  {
    // need to copy it across
    CStdString srcPath = CUtil::AddFileToFolder("special://xbmc/userdata/", file);
    CFile::Cache(srcPath, destPath);
  }
}

void CApplication::Preflight()
{
  // run any platform preflight scripts.
#ifdef __APPLE__
  CStdString install_path;

  CUtil::GetHomePath(install_path);
  setenv("XBMC_HOME", install_path.c_str(), 0);
  install_path += "/tools/osx/preflight";
  system(install_path.c_str());
#endif
}

HRESULT CApplication::Create(HWND hWnd)
{
  g_guiSettings.Initialize();  // Initialize default Settings
  g_settings.Initialize(); //Initialize default AdvancedSettings

#ifdef _WIN32PC
  CWIN32Util::SystemParams::GetDefaults( m_SSysParam );
  CWIN32Util::SystemParams::SetCustomParams();
#endif

#ifdef _LINUX
  tzset();   // Initialize timezone information variables
#endif

  g_hWnd = hWnd;

#ifndef HAS_SDL
  HRESULT hr = S_OK;
#endif

  // Grab a handle to our thread to be used later in identifying the render thread.
  m_threadID = GetCurrentThreadId();

#ifndef _LINUX
  //floating point precision to 24 bits (faster performance)
  _controlfp(_PC_24, _MCW_PC);


  /* install win32 exception translator, win32 exceptions
   * can now be caught using c++ try catch */
  win32_exception::install_handler();
#endif

  CProfile *profile;

  // only the InitDirectories* for the current platform should return
  // non-null (if at all i.e. to set a profile)
  // putting this before the first log entries saves another ifdef for g_stSettings.m_logFolder
  profile = InitDirectoriesLinux();
  if (!profile)
    profile = InitDirectoriesOSX();
  if (!profile)
    profile = InitDirectoriesWin32();
  if (profile)
  {
    profile->setName("Master user");
    profile->setLockMode(LOCK_MODE_EVERYONE);
    profile->setLockCode("");
    profile->setDate("");
    g_settings.m_vecProfiles.push_back(*profile);
    delete profile;
  }

  CLog::Log(LOGNOTICE, "-----------------------------------------------------------------------");
#if defined(_LINUX) && !defined(__APPLE__)
  CLog::Log(LOGNOTICE, "Starting XBMC, Platform: GNU/Linux.  Built on %s (SVN:%s)", __DATE__, SVN_REV);
#elif defined(__APPLE__)
  CLog::Log(LOGNOTICE, "Starting XBMC, Platform: Mac OS X.  Built on %s (SVN:%s)", __DATE__, SVN_REV);
#elif defined(_WIN32)
  CLog::Log(LOGNOTICE, "Starting XBMC, Platform: %s.  Built on %s (SVN:%s, compiler %i)",g_sysinfo.GetKernelVersion().c_str(), __DATE__, SVN_REV, _MSC_VER);
  CLog::Log(LOGNOTICE, g_cpuInfo.getCPUModel().c_str());
  CLog::Log(LOGNOTICE, CWIN32Util::GetResInfoString());
  CLog::Log(LOGNOTICE, "Running with %s rights", (CWIN32Util::IsCurrentUserLocalAdministrator() == TRUE) ? "administrator" : "restricted");
#endif
  CSpecialProtocol::LogPaths();

  char szXBEFileName[1024];
  CIoSupport::GetXbePath(szXBEFileName);
  CLog::Log(LOGNOTICE, "The executable running is: %s", szXBEFileName);
  CLog::Log(LOGNOTICE, "Log File is located: %sxbmc.log", g_stSettings.m_logFolder.c_str());
  CLog::Log(LOGNOTICE, "-----------------------------------------------------------------------");

  CStdString strExecutablePath;
  CUtil::GetHomePath(strExecutablePath);

  // if we are running from DVD our UserData location will be TDATA
  if (CUtil::IsDVD(strExecutablePath))
  {
    // TODO: Should we copy over any UserData folder from the DVD?
    if (!CFile::Exists("special://masterprofile/guisettings.xml")) // first run - cache userdata folder
    {
      CFileItemList items;
      CUtil::GetRecursiveListing("special://xbmc/userdata",items,"");
      for (int i=0;i<items.Size();++i)
          CFile::Cache(items[i]->m_strPath,"special://masterprofile/"+CUtil::GetFileName(items[i]->m_strPath));
    }
    g_settings.m_vecProfiles[0].setDirectory("special://masterprofile/");
    g_stSettings.m_logFolder = "special://masterprofile/";
  }

#ifdef HAS_XRANDR
  g_xrandr.LoadCustomModeLinesToAllOutputs();
#endif

  // Init our DllLoaders emu env
  init_emu_environ();


#ifndef HAS_SDL
  CLog::Log(LOGNOTICE, "Setup DirectX");
  // Create the Direct3D object
  if ( NULL == ( m_pD3D = Direct3DCreate9(D3D_SDK_VERSION) ) )
  {
    CLog::Log(LOGFATAL, "XBAppEx: Unable to create Direct3D!" );
    return E_FAIL;
  }
#else
  CLog::Log(LOGNOTICE, "Setup SDL");

  /* Clean up on exit, exit on window close and interrupt */
  atexit(SDL_Quit);

  Uint32 sdlFlags = SDL_INIT_VIDEO;

#ifdef HAS_SDL_AUDIO
  sdlFlags |= SDL_INIT_AUDIO;
#endif

#ifdef HAS_SDL_JOYSTICK
  sdlFlags |= SDL_INIT_JOYSTICK;
#endif


#ifdef _LINUX
  // for nvidia cards - vsync currently ALWAYS enabled.
  // the reason is that after screen has been setup changing this env var will make no difference.
  setenv("__GL_SYNC_TO_VBLANK", "1", 0);
  setenv("__GL_YIELD", "USLEEP", 0);
#endif

  if (SDL_Init(sdlFlags) != 0)
  {
        CLog::Log(LOGFATAL, "XBAppEx: Unable to initialize SDL: %s", SDL_GetError());
        return E_FAIL;
  }


  // for python scripts that check the OS
#ifdef __APPLE__
  setenv("OS","OS X",true);
#elif defined(_LINUX)
  CPicture pic;
  SDL_WM_SetIcon(pic.Load(_P("special://xbmc/media/icon.png")), NULL);
  setenv("OS","Linux",true);
#else
  CPicture pic;
  SDL_WM_SetIcon(pic.Load(_P("special://xbmc/media/icon32x32.png")), NULL);
#endif
#endif

  //list available videomodes
#ifndef HAS_SDL
  g_videoConfig.GetModes(m_pD3D);
  //init the present parameters with values that are supported
  RESOLUTION initialResolution = g_videoConfig.GetInitialMode(m_pD3D, &m_d3dpp);
  g_graphicsContext.SetD3DParameters(&m_d3dpp);
#else
  g_videoConfig.GetModes();
  //init the present parameters with values that are supported
  RESOLUTION initialResolution = g_videoConfig.GetInitialMode();
#endif

  // Initialize core peripheral port support. Note: If these parameters
  // are 0 and NULL, respectively, then the default number and types of
  // controllers will be initialized.

#if defined(HAS_SDL) && defined(_WIN32)
  SDL_SysWMinfo wmInfo;
  SDL_VERSION(&wmInfo.version)
  int te = SDL_GetWMInfo( &wmInfo );
  g_hWnd = wmInfo.window;
  m_messageHandler.Initialize();
#endif

  // Create the Mouse and Keyboard devices
  g_Mouse.Initialize(&hWnd);
  g_Keyboard.Initialize(hWnd);
#ifdef HAS_LIRC
  g_RemoteControl.Initialize();
#endif
#ifdef HAS_SDL_JOYSTICK
  g_Joystick.Initialize(hWnd);
#endif

  CLog::Log(LOGINFO, "Drives are mapped");

  CLog::Log(LOGNOTICE, "load settings...");
  g_LoadErrorStr = "Unable to load settings";
  g_settings.m_iLastUsedProfileIndex = g_settings.m_iLastLoadedProfileIndex;
  if (g_settings.bUseLoginScreen && g_settings.m_iLastLoadedProfileIndex != 0)
    g_settings.m_iLastLoadedProfileIndex = 0;

  m_bAllSettingsLoaded = g_settings.Load(m_bXboxMediacenterLoaded, m_bSettingsLoaded);
  if (!m_bAllSettingsLoaded)
    FatalErrorHandler(true, true, true);

  update_emu_environ();//apply the GUI settings

#ifdef __APPLE__
  // Configure and possible manually start the helper.
  g_xbmcHelper.Configure();
#endif

  if (!g_graphicsContext.IsValidResolution(g_guiSettings.m_LookAndFeelResolution))
  {
    // Oh uh - doesn't look good for starting in their wanted screenmode
    CLog::Log(LOGERROR, "The screen resolution requested is not valid, resetting to a valid mode");
    g_guiSettings.m_LookAndFeelResolution = initialResolution;
  }

  // TODO LINUX SDL - Check that the resolution is ok
#ifndef HAS_SDL
  m_d3dpp.Windowed = TRUE;
  m_d3dpp.hDeviceWindow = g_hWnd;

  // Transfer the new resolution information to our graphics context
  g_graphicsContext.SetD3DParameters(&m_d3dpp);
  g_graphicsContext.SetVideoResolution(g_guiSettings.m_LookAndFeelResolution, TRUE);


  if ( FAILED( hr = m_pD3D->CreateDevice(0, D3DDEVTYPE_HAL, NULL,
                                         D3DCREATE_MULTITHREADED | D3DCREATE_HARDWARE_VERTEXPROCESSING,
                                         &m_d3dpp, &m_pd3dDevice ) ) )
  {
    // try software vertex processing
    if ( FAILED( hr = m_pD3D->CreateDevice(0, D3DDEVTYPE_HAL, NULL,
                                          D3DCREATE_MULTITHREADED | D3DCREATE_SOFTWARE_VERTEXPROCESSING,
                                          &m_d3dpp, &m_pd3dDevice ) ) )
    {
      // and slow as arse reference processing
      if ( FAILED( hr = m_pD3D->CreateDevice(0, D3DDEVTYPE_REF, NULL,
                                            D3DCREATE_MULTITHREADED | D3DCREATE_SOFTWARE_VERTEXPROCESSING,
                                            &m_d3dpp, &m_pd3dDevice ) ) )
      {

        CLog::Log(LOGFATAL, "XBAppEx: Could not create D3D device!" );
        CLog::Log(LOGFATAL, " width/height:(%ix%i)" , m_d3dpp.BackBufferWidth, m_d3dpp.BackBufferHeight);
        CLog::Log(LOGFATAL, " refreshrate:%i" , m_d3dpp.FullScreen_RefreshRateInHz);
        if (m_d3dpp.Flags & D3DPRESENTFLAG_WIDESCREEN)
          CLog::Log(LOGFATAL, " 16:9 widescreen");
        else
          CLog::Log(LOGFATAL, " 4:3");

        if (m_d3dpp.Flags & D3DPRESENTFLAG_INTERLACED)
          CLog::Log(LOGFATAL, " interlaced");
        if (m_d3dpp.Flags & D3DPRESENTFLAG_PROGRESSIVE)
          CLog::Log(LOGFATAL, " progressive");
        return hr;
      }
    }
  }
  g_graphicsContext.SetD3DDevice(m_pd3dDevice);
  g_graphicsContext.CaptureStateBlock();
  // set filters
  g_graphicsContext.Get3DDevice()->SetSamplerState(0, D3DSAMP_MINFILTER, D3DTEXF_LINEAR /*g_stSettings.m_minFilter*/ );
  g_graphicsContext.Get3DDevice()->SetSamplerState(0, D3DSAMP_MAGFILTER, D3DTEXF_LINEAR /*g_stSettings.m_maxFilter*/ );
  CUtil::InitGamma();
#endif

  // set GUI res and force the clear of the screen
  g_graphicsContext.SetVideoResolution(g_guiSettings.m_LookAndFeelResolution, TRUE, true);

#if defined(_WIN32PC) && defined(HAS_SDL_OPENGL)
  CWIN32Util::CheckGLVersion();
#endif

  // initialize our charset converter
  g_charsetConverter.reset();

  // Load the langinfo to have user charset <-> utf-8 conversion
  CStdString strLanguage = g_guiSettings.GetString("locale.language");
  strLanguage[0] = toupper(strLanguage[0]);

  CStdString strLangInfoPath;
  strLangInfoPath.Format("special://xbmc/language/%s/langinfo.xml", strLanguage.c_str());

  CLog::Log(LOGINFO, "load language info file: %s", strLangInfoPath.c_str());
  g_langInfo.Load(strLangInfoPath);

  m_splash = new CSplash("special://xbmc/media/Splash.png");
#ifndef HAS_SDL_OPENGL
  m_splash->Start();
#else
  m_splash->Show();
#endif

  CStdString strLanguagePath;
  strLanguagePath.Format("special://xbmc/language/%s/strings.xml", strLanguage.c_str());

  CLog::Log(LOGINFO, "load language file:%s", strLanguagePath.c_str());
  if (!g_localizeStrings.Load(strLanguagePath))
    FatalErrorHandler(false, false, true);

  CLog::Log(LOGINFO, "load keymapping");
  if (!CButtonTranslator::GetInstance().Load())
    FatalErrorHandler(false, false, true);

  // check the skin file for testing purposes
  CStdString strSkinBase = "special://home/skin/";
  CStdString strSkinPath = strSkinBase + g_guiSettings.GetString("lookandfeel.skin");
  if (!CFile::Exists(strSkinPath)) {
    strSkinBase = "special://xbmc/skin/";
    strSkinPath = strSkinBase + g_guiSettings.GetString("lookandfeel.skin");
  }
  CLog::Log(LOGINFO, "Checking skin version of: %s", g_guiSettings.GetString("lookandfeel.skin").c_str());
  if (!g_SkinInfo.Check(strSkinPath))
  {
    // reset to the default skin (DEFAULT_SKIN)
    CLog::Log(LOGINFO, "The above skin isn't suitable - checking the version of the default: %s", DEFAULT_SKIN);
    strSkinPath = strSkinBase + DEFAULT_SKIN;
    if (!g_SkinInfo.Check(strSkinPath))
    {
      g_LoadErrorStr.Format("No suitable skin version found.\nWe require at least version %5.4f \n", g_SkinInfo.GetMinVersion());
      FatalErrorHandler(false, false, true);
    }
  }
  int iResolution = g_graphicsContext.GetVideoResolution();
  CLog::Log(LOGINFO, "GUI format %ix%i %s",
            g_settings.m_ResInfo[iResolution].iWidth,
            g_settings.m_ResInfo[iResolution].iHeight,
            g_settings.m_ResInfo[iResolution].strMode);
  m_gWindowManager.Initialize();

#ifdef HAS_PYTHON
  g_actionManager.SetScriptActionCallback(&g_pythonParser);
#endif

  g_Mouse.SetEnabled(g_guiSettings.GetBool("lookandfeel.enablemouse"));

  CUtil::InitRandomSeed();

#ifdef _WIN32PC
  CWIN32Util::AddRemovableDrives();
#endif

  return CXBApplicationEx::Create(hWnd);
}

CProfile* CApplication::InitDirectoriesLinux()
{
/*
   The following is the directory mapping for Platform Specific Mode:

   special://xbmc/          => [read-only] system directory (/usr/share/xbmc)
   special://home/          => [read-write] user's directory that will override special://xbmc/ system-wide
                               installations like skins, screensavers, etc.
                               ($HOME/.xbmc)
                               NOTE: XBMC will look in both special://xbmc/skin and special://xbmc/skin for skins.
                                     Same applies to screensavers, sounds, etc.
   special://masterprofile/ => [read-write] userdata of master profile. It will by default be
                               mapped to special://home/userdata ($HOME/.xbmc/userdata)
   special://profile/       => [read-write] current profile's userdata directory.
                               Generally special://masterprofile for the master profile or
                               special://masterprofile/profiles/<profile_name> for other profiles.

   NOTE: All these root directories are lowercase. Some of the sub-directories
         might be mixed case.
*/

#if defined(_LINUX) && !defined(__APPLE__)
  CProfile* profile = NULL;

  CStdString userName;
  if (getenv("USER"))
    userName = getenv("USER");
  else
    userName = "root";

  CStdString userHome;
  if (getenv("HOME"))
    userHome = getenv("HOME");
  else
    userHome = "/root";

  CStdString strHomePath;
  CUtil::GetHomePath(strHomePath);
  setenv("XBMC_HOME", strHomePath.c_str(), 0);

  if (m_bPlatformDirectories)
  {
    // map our special drives
    CSpecialProtocol::SetXBMCPath(strHomePath);
    CSpecialProtocol::SetHomePath(userHome + "/.xbmc");
    CSpecialProtocol::SetMasterProfilePath(userHome + "/.xbmc/userdata");

    CStdString strTempPath = CUtil::AddFileToFolder(userHome, ".xbmc/temp");
    CSpecialProtocol::SetTempPath(strTempPath);

    CUtil::AddSlashAtEnd(strTempPath);
    g_stSettings.m_logFolder = strTempPath;

    bool bCopySystemPlugins = false;
    if (!CDirectory::Exists("special://home/plugins") )
       bCopySystemPlugins = true;

    CDirectory::Create("special://home/");
    CDirectory::Create("special://temp/");
    CDirectory::Create("special://home/skin");
    CDirectory::Create("special://home/addons");
    CDirectory::Create("special://home/addons/multitype");
    CDirectory::Create("special://home/addons/visualisations");
    CDirectory::Create("special://home/addons/pvr");
    CDirectory::Create("special://home/addons/scripts");
    CDirectory::Create("special://home/addons/scrapers");
    CDirectory::Create("special://home/addons/scrapers/pvr");
    CDirectory::Create("special://home/addons/scrapers/video");
    CDirectory::Create("special://home/addons/scrapers/music");
    CDirectory::Create("special://home/addons/scrapers/programs");
    CDirectory::Create("special://home/addons/screensavers");
    CDirectory::Create("special://home/addons/plugins");
    CDirectory::Create("special://home/addons/plugins/pvr");
    CDirectory::Create("special://home/addons/plugins/music");
    CDirectory::Create("special://home/addons/plugins/video");
    CDirectory::Create("special://home/addons/plugins/programs");
    CDirectory::Create("special://home/addons/plugins/pictures");
    CDirectory::Create("special://home/addons/plugins/weather");
    CDirectory::Create("special://home/addons/dsp-audio");
    CDirectory::Create("special://home/keymaps");
    CDirectory::Create("special://home/visualisations");
    CDirectory::Create("special://home/screensavers");
    CDirectory::Create("special://home/sounds");
    CDirectory::Create("special://home/system");
    CDirectory::Create("special://home/plugins");
    CDirectory::Create("special://home/plugins/video");
    CDirectory::Create("special://home/plugins/music");
    CDirectory::Create("special://home/plugins/pictures");
    CDirectory::Create("special://home/plugins/programs");
    CDirectory::Create("special://home/plugins/weather");
    CDirectory::Create("special://home/scripts");
    CDirectory::Create("special://home/scripts/My Scripts");    // FIXME: both scripts should be in 1 directory

    if (!CFile::Exists("special://home/scripts/Common Scripts"))
    {
      if (symlink( INSTALL_PATH "/scripts",  _P("special://home/scripts/Common Scripts").c_str() ) != 0)
        CLog::Log(LOGERROR, "Failed to create common scripts symlink.");
    }

    CDirectory::Create("special://masterprofile");

    // copy required files
    //CopyUserDataIfNeeded("special://masterprofile/", "Keymap.xml");  // Eventual FIXME.
    CopyUserDataIfNeeded("special://masterprofile/", "RssFeeds.xml");
    CopyUserDataIfNeeded("special://masterprofile/", "Lircmap.xml");
    CopyUserDataIfNeeded("special://masterprofile/", "LCD.xml");
    
    // copy system-wide plugins into userprofile
    if ( bCopySystemPlugins )
       CUtil::CopyDirRecursive("special://xbmc/plugins", "special://home/plugins");
  }
  else
  {
    CUtil::AddSlashAtEnd(strHomePath);
    g_stSettings.m_logFolder = strHomePath;

    CSpecialProtocol::SetXBMCPath(strHomePath);
    CSpecialProtocol::SetHomePath(strHomePath);
    CSpecialProtocol::SetMasterProfilePath(CUtil::AddFileToFolder(strHomePath, "userdata"));

    CStdString strTempPath = CUtil::AddFileToFolder(strHomePath, "temp");
    CSpecialProtocol::SetTempPath(strTempPath);
    CDirectory::Create("special://temp/");

    CUtil::AddSlashAtEnd(strTempPath);
    g_stSettings.m_logFolder = strTempPath;
  }

  g_settings.m_vecProfiles.clear();
  g_settings.LoadProfiles( PROFILES_FILE );

  if (g_settings.m_vecProfiles.size()==0)
  {
    profile = new CProfile;
    profile->setDirectory("special://masterprofile/");
  }
  return profile;
#else
  return NULL;
#endif
}

CProfile* CApplication::InitDirectoriesOSX()
{
#ifdef __APPLE__
  CProfile* profile = NULL;

  CStdString userName;
  if (getenv("USER"))
    userName = getenv("USER");
  else
    userName = "root";

  CStdString userHome;
  if (getenv("HOME"))
    userHome = getenv("HOME");
  else
    userHome = "/root";

  CStdString strHomePath;
  CUtil::GetHomePath(strHomePath);
  setenv("XBMC_HOME", strHomePath.c_str(), 0);

  // OSX always runs with m_bPlatformDirectories == true
  if (m_bPlatformDirectories)
  {
    // map our special drives
    CSpecialProtocol::SetXBMCPath(strHomePath);
    CSpecialProtocol::SetHomePath(userHome + "/Library/Application Support/XBMC");
    CSpecialProtocol::SetMasterProfilePath(userHome + "/Library/Application Support/XBMC/userdata");

#ifdef __APPLE__
    CStdString strTempPath = CUtil::AddFileToFolder(userHome, ".xbmc/");
    CDirectory::Create(strTempPath);
#endif

    strTempPath = CUtil::AddFileToFolder(userHome, ".xbmc/temp");
    CSpecialProtocol::SetTempPath(strTempPath);

#ifdef __APPLE__
    strTempPath = userHome + "/Library/Logs";
#endif
    CUtil::AddSlashAtEnd(strTempPath);
    g_stSettings.m_logFolder = strTempPath;

    bool bCopySystemPlugins = false;
    if (!CDirectory::Exists("special://home/plugins") )
       bCopySystemPlugins = true;

    CDirectory::Create("special://home/");
    CDirectory::Create("special://temp/");
    CDirectory::Create("special://home/skin");
    CDirectory::Create("special://home/addons");
    CDirectory::Create("special://home/addons/multitype");
    CDirectory::Create("special://home/addons/visualisations");
    CDirectory::Create("special://home/addons/pvr");
    CDirectory::Create("special://home/addons/scripts");
    CDirectory::Create("special://home/addons/scrapers");
    CDirectory::Create("special://home/addons/scrapers/pvr");
    CDirectory::Create("special://home/addons/scrapers/video");
    CDirectory::Create("special://home/addons/scrapers/music");
    CDirectory::Create("special://home/addons/scrapers/programs");
    CDirectory::Create("special://home/addons/screensavers");
    CDirectory::Create("special://home/addons/plugins");
    CDirectory::Create("special://home/addons/plugins/pvr");
    CDirectory::Create("special://home/addons/plugins/music");
    CDirectory::Create("special://home/addons/plugins/video");
    CDirectory::Create("special://home/addons/plugins/programs");
    CDirectory::Create("special://home/addons/plugins/pictures");
    CDirectory::Create("special://home/addons/plugins/weather");
    CDirectory::Create("special://home/addons/dsp-audio");
    CDirectory::Create("special://home/keymaps");
    CDirectory::Create("special://home/visualisations");
    CDirectory::Create("special://home/screensavers");
    CDirectory::Create("special://home/sounds");
    CDirectory::Create("special://home/system");
    CDirectory::Create("special://home/plugins");
    CDirectory::Create("special://home/plugins/video");
    CDirectory::Create("special://home/plugins/music");
    CDirectory::Create("special://home/plugins/pictures");
    CDirectory::Create("special://home/plugins/programs");
    CDirectory::Create("special://home/plugins/weather");
    CDirectory::Create("special://home/scripts");
    CDirectory::Create("special://home/scripts/My Scripts"); // FIXME: both scripts should be in 1 directory
#ifdef __APPLE__
    strTempPath = strHomePath + "/scripts";
#else
    strTempPath = INSTALL_PATH "/scripts";
#endif
    symlink( strTempPath.c_str(),  _P("special://home/scripts/Common Scripts").c_str() );

    CDirectory::Create("special://masterprofile/");

    // copy required files
    //CopyUserDataIfNeeded("special://masterprofile/", "Keymap.xml"); // Eventual FIXME.
    CopyUserDataIfNeeded("special://masterprofile/", "RssFeeds.xml");
    CopyUserDataIfNeeded("special://masterprofile/", "Lircmap.xml");
    CopyUserDataIfNeeded("special://masterprofile/", "LCD.xml");

    // copy system-wide plugins into userprofile
    if ( bCopySystemPlugins )
       CUtil::CopyDirRecursive("special://xbmc/plugins", "special://home/plugins");
  }
  else
  {
    CUtil::AddSlashAtEnd(strHomePath);
    g_stSettings.m_logFolder = strHomePath;

    CSpecialProtocol::SetXBMCPath(strHomePath);
    CSpecialProtocol::SetHomePath(strHomePath);
    CSpecialProtocol::SetMasterProfilePath(CUtil::AddFileToFolder(strHomePath, "userdata"));

    CStdString strTempPath = CUtil::AddFileToFolder(strHomePath, "temp");
    CSpecialProtocol::SetTempPath(strTempPath);
    CDirectory::Create("special://temp/");

    CUtil::AddSlashAtEnd(strTempPath);
    g_stSettings.m_logFolder = strTempPath;
  }

  g_settings.m_vecProfiles.clear();
  g_settings.LoadProfiles( PROFILES_FILE );

  if (g_settings.m_vecProfiles.size()==0)
  {
    profile = new CProfile;
    profile->setDirectory("special://masterprofile/");
  }
  return profile;
#else
  return NULL;
#endif
}

CProfile* CApplication::InitDirectoriesWin32()
{
#ifdef _WIN32PC
  CProfile* profile = NULL;
  CStdString strExecutablePath;

  CUtil::GetHomePath(strExecutablePath);
  SetEnvironmentVariable("XBMC_HOME", strExecutablePath.c_str());
  CSpecialProtocol::SetXBMCPath(strExecutablePath);

  if (m_bPlatformDirectories)
  {

    CStdString strWin32UserFolder = CWIN32Util::GetProfilePath();

    // create user/app data/XBMC
    CStdString homePath = CUtil::AddFileToFolder(strWin32UserFolder, "XBMC");

    // move log to platform dirs
    g_stSettings.m_logFolder = homePath;
    CUtil::AddSlashAtEnd(g_stSettings.m_logFolder);

    // map our special drives
    CSpecialProtocol::SetXBMCPath(strExecutablePath);
    CSpecialProtocol::SetHomePath(homePath);
    CSpecialProtocol::SetMasterProfilePath(CUtil::AddFileToFolder(homePath, "userdata"));
    SetEnvironmentVariable("XBMC_PROFILE_USERDATA",_P("special://masterprofile").c_str());

    bool bCopySystemPlugins = false;
    if (!CDirectory::Exists("special://home/plugins") )
       bCopySystemPlugins = true;

    CDirectory::Create("special://home/");
    CDirectory::Create("special://home/skin");
    CDirectory::Create("special://home/addons");
    CDirectory::Create("special://home/addons/multitype");
    CDirectory::Create("special://home/addons/visualisations");
    CDirectory::Create("special://home/addons/pvr");
    CDirectory::Create("special://home/addons/scripts");
    CDirectory::Create("special://home/addons/scrapers");
    CDirectory::Create("special://home/addons/scrapers/pvr");
    CDirectory::Create("special://home/addons/scrapers/video");
    CDirectory::Create("special://home/addons/scrapers/music");
    CDirectory::Create("special://home/addons/scrapers/programs");
    CDirectory::Create("special://home/addons/screensavers");
    CDirectory::Create("special://home/addons/plugins");
    CDirectory::Create("special://home/addons/plugins/pvr");
    CDirectory::Create("special://home/addons/plugins/music");
    CDirectory::Create("special://home/addons/plugins/video");
    CDirectory::Create("special://home/addons/plugins/programs");
    CDirectory::Create("special://home/addons/plugins/pictures");
    CDirectory::Create("special://home/addons/plugins/weather");
    CDirectory::Create("special://home/addons/dsp-audio");
    CDirectory::Create("special://home/keymaps");
    CDirectory::Create("special://home/visualisations");
    CDirectory::Create("special://home/screensavers");
    CDirectory::Create("special://home/sounds");
    CDirectory::Create("special://home/system");
    CDirectory::Create("special://home/plugins");
    CDirectory::Create("special://home/plugins/video");
    CDirectory::Create("special://home/plugins/music");
    CDirectory::Create("special://home/plugins/pictures");
    CDirectory::Create("special://home/plugins/programs");
    CDirectory::Create("special://home/plugins/weather");
    CDirectory::Create("special://home/scripts");

    CDirectory::Create("special://masterprofile");

    // copy required files
    //CopyUserDataIfNeeded("special://masterprofile/", "Keymap.xml");  // Eventual FIXME.
    CopyUserDataIfNeeded("special://masterprofile/", "RssFeeds.xml");
    CopyUserDataIfNeeded("special://masterprofile/", "favourites.xml");
    CopyUserDataIfNeeded("special://masterprofile/", "Lircmap.xml");
    CopyUserDataIfNeeded("special://masterprofile/", "LCD.xml");

    // copy system-wide plugins into userprofile
    if ( bCopySystemPlugins )
       CUtil::CopyDirRecursive("special://xbmc/plugins", "special://home/plugins");

    // create user/app data/XBMC/cache
    CSpecialProtocol::SetTempPath(CUtil::AddFileToFolder(homePath,"cache"));
    CDirectory::Create("special://temp");
  }
  else
  {
    g_stSettings.m_logFolder = strExecutablePath;
    CUtil::AddSlashAtEnd(g_stSettings.m_logFolder);
    CStdString strTempPath = CUtil::AddFileToFolder(strExecutablePath, "cache");
    CSpecialProtocol::SetTempPath(strTempPath);
    CDirectory::Create("special://temp/");

    CSpecialProtocol::SetHomePath(strExecutablePath);
    CSpecialProtocol::SetMasterProfilePath(CUtil::AddFileToFolder(strExecutablePath,"userdata"));
    SetEnvironmentVariable("XBMC_PROFILE_USERDATA",_P("special://masterprofile/").c_str());
  }

  g_settings.m_vecProfiles.clear();
  g_settings.LoadProfiles(PROFILES_FILE);

  if (g_settings.m_vecProfiles.size()==0)
  {
    profile = new CProfile;
    profile->setDirectory("special://masterprofile/");
  }

  // Expand the DLL search path with our directories
  CWIN32Util::ExtendDllPath();

  return profile;
#else
  return NULL;
#endif
}

HRESULT CApplication::Initialize()
{
  // turn off cdio logging
  cdio_loglevel_default = CDIO_LOG_ERROR;

  CLog::Log(LOGINFO, "creating subdirectories");

  CLog::Log(LOGINFO, "userdata folder: %s", g_settings.GetProfileUserDataFolder().c_str());
  CLog::Log(LOGINFO, "recording folder:%s", g_guiSettings.GetString("mymusic.recordingpath",false).c_str());
  CLog::Log(LOGINFO, "screenshots folder:%s", g_guiSettings.GetString("pictures.screenshotpath",false).c_str());

  // UserData folder layout:
  // UserData/
  //   Database/
  //     CDDb/
  //   Thumbnails/
  //     Music/
  //       temp/
  //     0 .. F/

  CDirectory::Create(g_settings.GetUserDataFolder());
  CDirectory::Create(g_settings.GetProfileUserDataFolder());
  g_settings.CreateProfileFolders();

  CDirectory::Create(g_settings.GetProfilesThumbFolder());

  CDirectory::Create("special://temp/temp"); // temp directory for python and dllGetTempPathA

#ifdef _LINUX // TODO: Win32 has no special://home/ mapping by default, so we
              //       must create these here. Ideally this should be using special://home/ and
              //       be platform agnostic (i.e. unify the InitDirectories*() functions)
  if (!m_bPlatformDirectories)
#endif
  {
    CDirectory::Create("special://xbmc/scripts");
    CDirectory::Create("special://xbmc/addons");
    CDirectory::Create("special://xbmc/addons/multitype");
    CDirectory::Create("special://xbmc/addons/visualisations");
    CDirectory::Create("special://xbmc/addons/pvr");
    CDirectory::Create("special://xbmc/addons/scripts");
    CDirectory::Create("special://xbmc/addons/scrapers");
    CDirectory::Create("special://xbmc/addons/screensavers");
    CDirectory::Create("special://xbmc/addons/plugin-pvr");
    CDirectory::Create("special://xbmc/addons/plugin-music");
    CDirectory::Create("special://xbmc/addons/plugin-video");
    CDirectory::Create("special://xbmc/addons/plugin-program");
    CDirectory::Create("special://xbmc/addons/plugin-pictures");
    CDirectory::Create("special://xbmc/addons/dsp-audio");
    CDirectory::Create("special://xbmc/plugins");
    CDirectory::Create("special://xbmc/plugins/music");
    CDirectory::Create("special://xbmc/plugins/video");
    CDirectory::Create("special://xbmc/plugins/pictures");
    CDirectory::Create("special://xbmc/plugins/programs");
    CDirectory::Create("special://xbmc/plugins/weather");
    CDirectory::Create("special://xbmc/language");
    CDirectory::Create("special://xbmc/visualisations");
    CDirectory::Create("special://xbmc/sounds");
    CDirectory::Create(CUtil::AddFileToFolder(g_settings.GetUserDataFolder(),"visualisations"));
  }

  // initialize network
  if (!m_bXboxMediacenterLoaded)
  {
    CLog::Log(LOGINFO, "using default network settings");
    g_guiSettings.SetString("network.ipaddress", "192.168.0.100");
    g_guiSettings.SetString("network.subnet", "255.255.255.0");
    g_guiSettings.SetString("network.gateway", "192.168.0.1");
    g_guiSettings.SetString("network.dns", "192.168.0.1");
    g_guiSettings.SetBool("servers.ftpserver", true);
    g_guiSettings.SetBool("servers.webserver", false);
    g_guiSettings.SetBool("locale.timeserver", false);
  }

  StartServices();

  // Init DPMS, before creating the corresponding setting control.
  m_dpms = new DPMSSupport(g_graphicsContext.getScreenSurface());
  g_guiSettings.GetSetting("screensaver.sep_powersaving")->SetVisible(
      m_dpms->IsSupported());
  g_guiSettings.GetSetting("screensaver.powersavingtime")->SetVisible(
      m_dpms->IsSupported());

  m_gWindowManager.Add(new CGUIWindowHome);                     // window id = 0

  CLog::Log(LOGNOTICE, "load default skin:[%s]", g_guiSettings.GetString("lookandfeel.skin").c_str());
  LoadSkin(g_guiSettings.GetString("lookandfeel.skin"));

  m_gWindowManager.Add(new CGUIWindowPrograms);                 // window id = 1
  m_gWindowManager.Add(new CGUIWindowPictures);                 // window id = 2
  m_gWindowManager.Add(new CGUIWindowFileManager);      // window id = 3
  m_gWindowManager.Add(new CGUIWindowVideoFiles);          // window id = 6
  m_gWindowManager.Add(new CGUIWindowSettings);                 // window id = 4
  m_gWindowManager.Add(new CGUIWindowTV);                       // window id = 9
  m_gWindowManager.Add(new CGUIDialogTVEPGProgInfo);            // window id = 600
  m_gWindowManager.Add(new CGUIDialogTVTimerSettings);          // window id = 602
  m_gWindowManager.Add(new CGUIDialogTVChannels);               // window id = 603
  m_gWindowManager.Add(new CGUIDialogTVGuide);                  // window id = 603
  m_gWindowManager.Add(new CGUIDialogTVRecordingInfo);          // window id = 603
  m_gWindowManager.Add(new CGUIDialogTVGroupManager);           // window id = 603
  m_gWindowManager.Add(new CGUIDialogTVTeletext);               // window id =
  m_gWindowManager.Add(new CGUIWindowSystemInfo);               // window id = 7
  m_gWindowManager.Add(new CGUIWindowTestPattern);      // window id = 8
  m_gWindowManager.Add(new CGUIWindowSettingsScreenCalibration); // window id = 11
  m_gWindowManager.Add(new CGUIWindowSettingsCategory);         // window id = 12 slideshow:window id 2007
  m_gWindowManager.Add(new CGUIWindowScripts);                  // window id = 20
  m_gWindowManager.Add(new CGUIWindowVideoNav);                 // window id = 36
  m_gWindowManager.Add(new CGUIWindowVideoPlaylist);            // window id = 28
  m_gWindowManager.Add(new CGUIWindowLoginScreen);            // window id = 29
  m_gWindowManager.Add(new CGUIWindowSettingsProfile);          // window id = 34
  m_gWindowManager.Add(new CGUIDialogYesNo);              // window id = 100
  m_gWindowManager.Add(new CGUIDialogProgress);           // window id = 101
  m_gWindowManager.Add(new CGUIDialogKeyboard);           // window id = 103
  m_gWindowManager.Add(&m_guiDialogVolumeBar);          // window id = 104
  m_gWindowManager.Add(&m_guiDialogSeekBar);            // window id = 115
  m_gWindowManager.Add(new CGUIDialogSubMenu);            // window id = 105
  m_gWindowManager.Add(new CGUIDialogContextMenu);        // window id = 106
  m_gWindowManager.Add(&m_guiDialogKaiToast);           // window id = 107
  m_gWindowManager.Add(new CGUIDialogNumeric);            // window id = 109
  m_gWindowManager.Add(new CGUIDialogGamepad);            // window id = 110
  m_gWindowManager.Add(new CGUIDialogButtonMenu);         // window id = 111
  m_gWindowManager.Add(new CGUIDialogMusicScan);          // window id = 112
  m_gWindowManager.Add(new CGUIDialogPlayerControls);     // window id = 113
#ifdef HAS_KARAOKE
  m_gWindowManager.Add(new CGUIDialogKaraokeSongSelectorSmall); // window id 143
  m_gWindowManager.Add(new CGUIDialogKaraokeSongSelectorLarge); // window id 144
#endif
  m_gWindowManager.Add(new CGUIDialogSlider);             // window id = 145
  m_gWindowManager.Add(new CGUIDialogMusicOSD);           // window id = 120
  m_gWindowManager.Add(new CGUIDialogVisualisationSettings);     // window id = 121
  m_gWindowManager.Add(new CGUIDialogVisualisationPresetList);   // window id = 122
  m_gWindowManager.Add(new CGUIDialogVideoSettings);             // window id = 123
  m_gWindowManager.Add(new CGUIDialogAudioSubtitleSettings);     // window id = 124
  m_gWindowManager.Add(new CGUIDialogVideoBookmarks);      // window id = 125
  // Don't add the filebrowser dialog - it's created and added when it's needed
  m_gWindowManager.Add(new CGUIDialogNetworkSetup);  // window id = 128
  m_gWindowManager.Add(new CGUIDialogMediaSource);   // window id = 129
  m_gWindowManager.Add(new CGUIDialogProfileSettings); // window id = 130
  m_gWindowManager.Add(new CGUIDialogVideoScan);      // window id = 133
  m_gWindowManager.Add(new CGUIDialogFavourites);     // window id = 134
  m_gWindowManager.Add(new CGUIDialogSongInfo);       // window id = 135
  m_gWindowManager.Add(new CGUIDialogSmartPlaylistEditor);       // window id = 136
  m_gWindowManager.Add(new CGUIDialogSmartPlaylistRule);       // window id = 137
  m_gWindowManager.Add(new CGUIDialogBusy);      // window id = 138
  m_gWindowManager.Add(new CGUIDialogPictureInfo);      // window id = 139
  m_gWindowManager.Add(new CGUIDialogAddonSettings);      // window id = 140
#ifdef HAS_LINUX_NETWORK
  m_gWindowManager.Add(new CGUIDialogAccessPoints);      // window id = 141
#endif
  m_gWindowManager.Add(new CGUIDialogEpgScan);      // window id = 146

  m_gWindowManager.Add(new CGUIDialogLockSettings); // window id = 131

  m_gWindowManager.Add(new CGUIDialogContentSettings);        // window id = 132

  m_gWindowManager.Add(new CGUIWindowMusicPlayList);          // window id = 500
  m_gWindowManager.Add(new CGUIWindowMusicSongs);             // window id = 501
  m_gWindowManager.Add(new CGUIWindowMusicNav);               // window id = 502
  m_gWindowManager.Add(new CGUIWindowMusicPlaylistEditor);    // window id = 503

  m_gWindowManager.Add(new CGUIDialogSelect);             // window id = 2000
  m_gWindowManager.Add(new CGUIWindowMusicInfo);                // window id = 2001
  m_gWindowManager.Add(new CGUIDialogOK);                 // window id = 2002
  m_gWindowManager.Add(new CGUIWindowVideoInfo);                // window id = 2003
  m_gWindowManager.Add(new CGUIWindowScriptsInfo);              // window id = 2004
  m_gWindowManager.Add(new CGUIWindowFullScreen);         // window id = 2005
  m_gWindowManager.Add(new CGUIWindowVisualisation);      // window id = 2006
  m_gWindowManager.Add(new CGUIWindowSlideShow);          // window id = 2007
  m_gWindowManager.Add(new CGUIDialogFileStacking);       // window id = 2008
#ifdef HAS_KARAOKE
  m_gWindowManager.Add(new CGUIWindowKaraokeLyrics);      // window id = 2009
#endif

  m_gWindowManager.Add(new CGUIWindowOSD);                // window id = 2901
  m_gWindowManager.Add(new CGUIWindowMusicOverlay);       // window id = 2903
  m_gWindowManager.Add(new CGUIWindowVideoOverlay);       // window id = 2904
  m_gWindowManager.Add(new CGUIWindowScreensaver);        // window id = 2900 Screensaver
  m_gWindowManager.Add(new CGUIWindowWeather);            // window id = 2600 WEATHER
  m_gWindowManager.Add(new CGUIWindowStartup);            // startup window (id 2999)

  /* window id's 3000 - 3100 are reserved for python */

  SAFE_DELETE(m_splash);

  if (g_guiSettings.GetBool("masterlock.startuplock") &&
      g_settings.m_vecProfiles[0].getLockMode() != LOCK_MODE_EVERYONE &&
     !g_settings.m_vecProfiles[0].getLockCode().IsEmpty())
  {
     g_passwordManager.CheckStartUpLock();
  }

  // check if we should use the login screen
  if (g_settings.bUseLoginScreen)
  {
    m_gWindowManager.ActivateWindow(WINDOW_LOGIN_SCREEN);
  }
  else
  {
    RESOLUTION res = INVALID;
    CStdString startupPath = g_SkinInfo.GetSkinPath("Startup.xml", &res);
    int startWindow = g_guiSettings.GetInt("lookandfeel.startupwindow");
    // test for a startup window, and activate that instead of home
    if (CFile::Exists(startupPath) && (!g_SkinInfo.OnlyAnimateToHome() || startWindow == WINDOW_HOME))
    {
      m_gWindowManager.ActivateWindow(WINDOW_STARTUP);
    }
    else
    {
      // We need to Popup the WindowHome to initiate the GUIWindowManger for MasterCode popup dialog!
      // Then we can start the StartUpWindow! To prevent BlackScreen if the target Window is Protected with MasterCode!
      m_gWindowManager.ActivateWindow(WINDOW_HOME);
      if (startWindow != WINDOW_HOME)
        m_gWindowManager.ActivateWindow(startWindow);
    }
  }

#ifdef HAS_PYTHON
  g_pythonParser.bStartup = true;
#endif
  g_sysinfo.Refresh();

  CLog::Log(LOGINFO, "removing tempfiles");
  CUtil::RemoveTempFiles();

  if (!m_bAllSettingsLoaded)
  {
    CLog::Log(LOGWARNING, "settings not correct, show dialog");
    CStdString test;
    CUtil::GetHomePath(test);
    CGUIDialogOK *dialog = (CGUIDialogOK *)m_gWindowManager.GetWindow(WINDOW_DIALOG_OK);
    if (dialog)
    {
      dialog->SetHeading(279);
      dialog->SetLine(0, "Error while loading settings");
      dialog->SetLine(1, test);
      dialog->SetLine(2, "");;
      dialog->DoModal();
    }
  }

  //  Show mute symbol
  if (g_stSettings.m_nVolumeLevel == VOLUME_MINIMUM)
    Mute();

  // if the user shutoff the xbox during music scan
  // restore the settings
  if (g_stSettings.m_bMyMusicIsScanning)
  {
    CLog::Log(LOGWARNING,"System rebooted during music scan! ... restoring UseTags and FindRemoteThumbs");
    RestoreMusicScanSettings();
  }

  if (g_guiSettings.GetBool("videolibrary.updateonstartup"))
  {
    CLog::Log(LOGNOTICE, "Updating video library on startup");
    CGUIDialogVideoScan *scanner = (CGUIDialogVideoScan *)m_gWindowManager.GetWindow(WINDOW_DIALOG_VIDEO_SCAN);
    SScraperInfo info;
    VIDEO::SScanSettings settings;
    if (scanner && !scanner->IsScanning())
      scanner->StartScanning("",info,settings,false);
  }

#ifdef HAS_HAL
  g_HalManager.Initialize();
#endif

  if (g_guiSettings.GetBool("musiclibrary.updateonstartup"))
  {
    CLog::Log(LOGNOTICE, "Updating music library on startup");
    CGUIDialogMusicScan *scanner = (CGUIDialogMusicScan *)m_gWindowManager.GetWindow(WINDOW_DIALOG_MUSIC_SCAN);
    if (scanner && !scanner->IsScanning())
      scanner->StartScanning("");
  }

  if (g_guiSettings.GetBool("pvrmanager.enabled"))
  {
    StartPVRManager();
  }

  m_slowTimer.StartZero();

#ifdef __APPLE__
  g_xbmcHelper.CaptureAllInput();
#endif

  g_powerManager.Initialize();

  CLog::Log(LOGNOTICE, "initialize done");

  m_bInitializing = false;

  // reset our screensaver (starts timers etc.)
  ResetScreenSaver();
  return S_OK;
}

void CApplication::StartWebServer()
{
#ifdef HAS_WEB_SERVER
  if (g_guiSettings.GetBool("servers.webserver") && m_network.IsAvailable())
  {
    int webPort = atoi(g_guiSettings.GetString("servers.webserverport"));
    CLog::Log(LOGNOTICE, "Webserver: Starting...");
#ifdef _LINUX
    if (webPort < 1024 && geteuid() != 0)
    {
        CLog::Log(LOGERROR, "Cannot start Web Server as port is smaller than 1024 and user is not root");
        return;
    }
#endif
    CSectionLoader::Load("LIBHTTP");
    if (m_network.GetFirstConnectedInterface())
    {
       m_pWebServer = new CWebServer();
       m_pWebServer->Start(m_network.GetFirstConnectedInterface()->GetCurrentIPAddress().c_str(), webPort, "special://xbmc/web", false);
    }
    if (m_pWebServer)
    {
      m_pWebServer->SetUserName(g_guiSettings.GetString("servers.webserverusername").c_str());
      m_pWebServer->SetPassword(g_guiSettings.GetString("servers.webserverpassword").c_str());

      // publish web frontend and API services
      CZeroconf::GetInstance()->PublishService("servers.webserver", "_http._tcp", "XBMC Web Server", webPort);
      CZeroconf::GetInstance()->PublishService("servers.webapi", "_xbmc-web._tcp", "XBMC HTTP API", webPort);
    }
    if (m_pWebServer && m_pXbmcHttp && g_stSettings.m_HttpApiBroadcastLevel>=1)
      getApplicationMessenger().HttpApi("broadcastlevel; StartUp;1");
  }
#endif
}

void CApplication::StopWebServer()
{
#ifdef HAS_WEB_SERVER
  if (m_pWebServer)
  {
    CLog::Log(LOGNOTICE, "Webserver: Stopping...");
    m_pWebServer->Stop();
    delete m_pWebServer;
    m_pWebServer = NULL;
    CSectionLoader::Unload("LIBHTTP");
    CLog::Log(LOGNOTICE, "Webserver: Stopped...");
    CZeroconf::GetInstance()->RemoveService("servers.webserver");
    CZeroconf::GetInstance()->RemoveService("servers.webapi");
  }
#endif
}

void CApplication::StartFtpServer()
{
#ifdef HAS_FTP_SERVER
  if ( g_guiSettings.GetBool("servers.ftpserver") && m_network.IsAvailable() )
  {
    CLog::Log(LOGNOTICE, "XBFileZilla: Starting...");
    if (!m_pFileZilla)
    {
      CStdString xmlpath = "special://xbmc/system/";
      // if user didn't upgrade properly,
      // check whether UserData/FileZilla Server.xml exists
      if (CFile::Exists(g_settings.GetUserDataItem("FileZilla Server.xml")))
        xmlpath = g_settings.GetUserDataFolder();

      // check file size and presence
      CFile xml;
      if (xml.Open(xmlpath+"FileZilla Server.xml") && xml.GetLength() > 0)
      {
        m_pFileZilla = new CXBFileZilla(_P(xmlpath));
        m_pFileZilla->Start(false);
      }
      else
      {
        // 'FileZilla Server.xml' does not exist or is corrupt,
        // falling back to ftp emergency recovery mode
        CLog::Log(LOGNOTICE, "XBFileZilla: 'FileZilla Server.xml' is missing or is corrupt!");
        CLog::Log(LOGNOTICE, "XBFileZilla: Starting ftp emergency recovery mode");
        StartFtpEmergencyRecoveryMode();
      }
      xml.Close();
    }
  }
#endif
}

void CApplication::StopFtpServer()
{
#ifdef HAS_FTP_SERVER
  if (m_pFileZilla)
  {
    CLog::Log(LOGINFO, "XBFileZilla: Stopping...");

    std::vector<SXFConnection> mConnections;
    std::vector<SXFConnection>::iterator it;

    m_pFileZilla->GetAllConnections(mConnections);

    for(it = mConnections.begin();it != mConnections.end();it++)
    {
      m_pFileZilla->CloseConnection(it->mId);
    }

    m_pFileZilla->Stop();
    delete m_pFileZilla;
    m_pFileZilla = NULL;

    CLog::Log(LOGINFO, "XBFileZilla: Stopped");
  }
#endif
}

void CApplication::StartTimeServer()
{
#ifdef HAS_TIME_SERVER
  if (g_guiSettings.GetBool("locale.timeserver") && m_network.IsAvailable() )
  {
    if( !m_psntpClient )
    {
      CSectionLoader::Load("SNTP");
      CLog::Log(LOGNOTICE, "start timeserver client");

      m_psntpClient = new CSNTPClient();
      m_psntpClient->Update();
    }
  }
#endif
}

void CApplication::StopTimeServer()
{
#ifdef HAS_TIME_SERVER
  if( m_psntpClient )
  {
    CLog::Log(LOGNOTICE, "stop time server client");
    SAFE_DELETE(m_psntpClient);
    CSectionLoader::Unload("SNTP");
  }
#endif
}

void CApplication::StartUPnP()
{
#ifdef HAS_UPNP
    StartUPnPClient();
    StartUPnPServer();
    StartUPnPRenderer();
#endif
}

void CApplication::StopUPnP()
{
#ifdef HAS_UPNP
  if (CUPnP::IsInstantiated())
  {
    CLog::Log(LOGNOTICE, "stopping upnp");
    CUPnP::ReleaseInstance();
  }
#endif
}

void CApplication::StartEventServer()
{
#ifdef HAS_EVENT_SERVER
  CEventServer* server = CEventServer::GetInstance();
  if (!server)
  {
    CLog::Log(LOGERROR, "ES: Out of memory");
    return;
  }
  if (g_guiSettings.GetBool("remoteevents.enabled"))
  {
    CLog::Log(LOGNOTICE, "ES: Starting event server");
    server->StartServer();
  }
#endif
}

bool CApplication::StopEventServer(bool promptuser)
{
#ifdef HAS_EVENT_SERVER
  CEventServer* server = CEventServer::GetInstance();
  if (!server)
  {
    CLog::Log(LOGERROR, "ES: Out of memory");
    return false;
  }
  if (promptuser)
  {
    if (server->GetNumberOfClients() > 0)
    {
      bool cancelled = false;
      if (!CGUIDialogYesNo::ShowAndGetInput(13140, 13141, 13142, 20022,
                                            -1, -1, cancelled, 10000)
          || cancelled)
      {
        CLog::Log(LOGNOTICE, "ES: Not stopping event server");
        return false;
      }
    }
    CLog::Log(LOGNOTICE, "ES: Stopping event server with confirmation");
  }
  else
  {
    CLog::Log(LOGNOTICE, "ES: Stopping event server");
  }
  CEventServer::GetInstance()->StopServer();
  return true;
#endif
}

void CApplication::RefreshEventServer()
{
#ifdef HAS_EVENT_SERVER
  if (g_guiSettings.GetBool("remoteevents.enabled"))
  {
    CEventServer::GetInstance()->RefreshSettings();
  }
#endif
}

void CApplication::StartDbusServer()
{
#ifdef HAS_DBUS_SERVER
  CDbusServer* serverDbus = CDbusServer::GetInstance();
  if (!serverDbus)
  {
    CLog::Log(LOGERROR, "DS: Out of memory");
    return;
  }
  CLog::Log(LOGNOTICE, "DS: Starting dbus server");
  serverDbus->StartServer( this );
#endif
}

bool CApplication::StopDbusServer()
{
#ifdef HAS_DBUS_SERVER
  CDbusServer* serverDbus = CDbusServer::GetInstance();
  if (!serverDbus)
  {
    CLog::Log(LOGERROR, "DS: Out of memory");
    return false;
  }
  CDbusServer::GetInstance()->StopServer();
#endif
  return true;
}

void CApplication::StartUPnPRenderer()
{
#ifdef HAS_UPNP
  if (g_guiSettings.GetBool("upnp.renderer"))
  {
    CLog::Log(LOGNOTICE, "starting upnp renderer");
    CUPnP::GetInstance()->StartRenderer();
  }
#endif
}

void CApplication::StopUPnPRenderer()
{
#ifdef HAS_UPNP
  if (CUPnP::IsInstantiated())
  {
    CLog::Log(LOGNOTICE, "stopping upnp renderer");
    CUPnP::GetInstance()->StopRenderer();
  }
#endif
}

void CApplication::StartUPnPClient()
{
#ifdef HAS_UPNP
  if (g_guiSettings.GetBool("upnp.client"))
  {
    CLog::Log(LOGNOTICE, "starting upnp client");
    CUPnP::GetInstance()->StartClient();
  }
#endif
}

void CApplication::StopUPnPClient()
{
#ifdef HAS_UPNP
  if (CUPnP::IsInstantiated())
  {
    CLog::Log(LOGNOTICE, "stopping upnp client");
    CUPnP::GetInstance()->StopClient();
  }
#endif
}

void CApplication::StartUPnPServer()
{
#ifdef HAS_UPNP
  if (g_guiSettings.GetBool("upnp.server"))
  {
    CLog::Log(LOGNOTICE, "starting upnp server");
    CUPnP::GetInstance()->StartServer();
  }
#endif
}

void CApplication::StopUPnPServer()
{
#ifdef HAS_UPNP
  if (CUPnP::IsInstantiated())
  {
    CLog::Log(LOGNOTICE, "stopping upnp server");
    CUPnP::GetInstance()->StopServer();
  }
#endif
}

void CApplication::StartZeroconf()
{
#ifdef HAS_ZEROCONF
  //entry in guisetting only present if HAS_ZEROCONF is set
  if(g_guiSettings.GetBool("servers.zeroconf"))
  {
    CLog::Log(LOGNOTICE, "starting zeroconf publishing");
    CZeroconf::GetInstance()->Start();
  }
#endif
}

void CApplication::StopZeroconf()
{
#ifdef HAS_ZEROCONF
  if(CZeroconf::IsInstantiated())
  {
    CLog::Log(LOGNOTICE, "stopping zeroconf publishing");
    CZeroconf::GetInstance()->Stop();
  }
#endif
}

void CApplication::StartPVRManager()
{
  if (g_guiSettings.GetBool("pvrmanager.enabled"))
  {
    CLog::Log(LOGINFO, "starting PVRManager");
    CPVRManager::GetInstance()->Start();
  }
}

void CApplication::StopPVRManager()
{
  CLog::Log(LOGINFO, "stopping PVRManager");
  CPVRManager::GetInstance()->Stop();
}

void CApplication::DimLCDOnPlayback(bool dim)
{
#ifdef HAS_LCD
  if(g_lcd && dim && (g_guiSettings.GetInt("lcd.disableonplayback") != LED_PLAYBACK_OFF) && (g_guiSettings.GetInt("lcd.type") != LCD_TYPE_NONE))
  {
    if ( (IsPlayingVideo()) && g_guiSettings.GetInt("lcd.disableonplayback") == LED_PLAYBACK_VIDEO)
      g_lcd->SetBackLight(0);
    if ( (IsPlayingAudio()) && g_guiSettings.GetInt("lcd.disableonplayback") == LED_PLAYBACK_MUSIC)
      g_lcd->SetBackLight(0);
    if ( ((IsPlayingVideo() || IsPlayingAudio())) && g_guiSettings.GetInt("lcd.disableonplayback") == LED_PLAYBACK_VIDEO_MUSIC)
      g_lcd->SetBackLight(0);
  }
  else if(!dim)
#ifdef _LINUX
    g_lcd->SetBackLight(1);
#else
    g_lcd->SetBackLight(g_guiSettings.GetInt("lcd.backlight"));
#endif
#endif
}

void CApplication::StartServices()
{
#ifndef _WIN32PC
  // Start Thread for DVD Mediatype detection
  CLog::Log(LOGNOTICE, "start dvd mediatype detection");
  m_DetectDVDType.Create(false, THREAD_MINSTACKSIZE);
#endif

  CLog::Log(LOGNOTICE, "initializing playlistplayer");
  g_playlistPlayer.SetRepeat(PLAYLIST_MUSIC, g_stSettings.m_bMyMusicPlaylistRepeat ? PLAYLIST::REPEAT_ALL : PLAYLIST::REPEAT_NONE);
  g_playlistPlayer.SetShuffle(PLAYLIST_MUSIC, g_stSettings.m_bMyMusicPlaylistShuffle);
  g_playlistPlayer.SetRepeat(PLAYLIST_VIDEO, g_stSettings.m_bMyVideoPlaylistRepeat ? PLAYLIST::REPEAT_ALL : PLAYLIST::REPEAT_NONE);
  g_playlistPlayer.SetShuffle(PLAYLIST_VIDEO, g_stSettings.m_bMyVideoPlaylistShuffle);
  CLog::Log(LOGNOTICE, "DONE initializing playlistplayer");

#ifdef HAS_LCD
  CLCDFactory factory;
  g_lcd = factory.Create();
  if (g_lcd)
  {
    g_lcd->Initialize();
  }
#endif
}

void CApplication::StopServices()
{
  m_network.NetworkMessage(CNetwork::SERVICES_DOWN, 0);

#ifndef _WIN32PC
  CLog::Log(LOGNOTICE, "stop dvd detect media");
  m_DetectDVDType.StopThread();
#endif
}

void CApplication::DelayLoadSkin()
{
  m_dwSkinTime = timeGetTime() + 2000;
  return ;
}

void CApplication::CancelDelayLoadSkin()
{
  m_dwSkinTime = 0;
}

void CApplication::ReloadSkin()
{
  CGUIMessage msg(GUI_MSG_LOAD_SKIN, (DWORD) -1, m_gWindowManager.GetActiveWindow());
  g_graphicsContext.SendMessage(msg);
  // Reload the skin, restoring the previously focused control.  We need this as
  // the window unload will reset all control states.
  CGUIWindow* pWindow = m_gWindowManager.GetWindow(m_gWindowManager.GetActiveWindow());
  unsigned iCtrlID = pWindow->GetFocusedControlID();
  g_application.LoadSkin(g_guiSettings.GetString("lookandfeel.skin"));
  pWindow = m_gWindowManager.GetWindow(m_gWindowManager.GetActiveWindow());
  if (pWindow && pWindow->HasSaveLastControl())
  {
    CGUIMessage msg3(GUI_MSG_SETFOCUS, m_gWindowManager.GetActiveWindow(), iCtrlID, 0);
    pWindow->OnMessage(msg3);
  }
}

void CApplication::LoadSkin(const CStdString& strSkin)
{
  bool bPreviousPlayingState=false;
  bool bPreviousRenderingState=false;
  if (g_application.m_pPlayer && g_application.IsPlayingVideo())
  {
    bPreviousPlayingState = !g_application.m_pPlayer->IsPaused();
    if (bPreviousPlayingState)
      g_application.m_pPlayer->Pause();
#ifdef HAS_VIDEO_PLAYBACK
    if (!g_renderManager.Paused())
    {
      if (m_gWindowManager.GetActiveWindow() == WINDOW_FULLSCREEN_VIDEO)
     {
        m_gWindowManager.ActivateWindow(WINDOW_HOME);
        bPreviousRenderingState = true;
      }
    }
#endif
  }
  //stop the busy renderer if it's running before we lock the graphiccontext or we could deadlock.
  g_ApplicationRenderer.Stop();
  // close the music and video overlays (they're re-opened automatically later)
  CSingleLock lock(g_graphicsContext);

  m_dwSkinTime = 0;

  CStdString strHomePath;
  CStdString strSkinPath = g_settings.GetSkinFolder(strSkin);

  CLog::Log(LOGINFO, "  load skin from:%s", strSkinPath.c_str());

  // save the current window details
  int currentWindow = m_gWindowManager.GetActiveWindow();
  vector<DWORD> currentModelessWindows;
  m_gWindowManager.GetActiveModelessWindows(currentModelessWindows);

  CLog::Log(LOGINFO, "  delete old skin...");
  UnloadSkin();

  // Load in the skin.xml file if it exists
  g_SkinInfo.Load(strSkinPath);

  CLog::Log(LOGINFO, "  load fonts for skin...");
  g_graphicsContext.SetMediaDir(strSkinPath);
  g_directoryCache.ClearSubPaths(strSkinPath);
  if (g_langInfo.ForceUnicodeFont() && !g_fontManager.IsFontSetUnicode(g_guiSettings.GetString("lookandfeel.font")))
  {
    CLog::Log(LOGINFO, "    language needs a ttf font, loading first ttf font available");
    CStdString strFontSet;
    if (g_fontManager.GetFirstFontSetUnicode(strFontSet))
    {
      CLog::Log(LOGINFO, "    new font is '%s'", strFontSet.c_str());
      g_guiSettings.SetString("lookandfeel.font", strFontSet);
      g_settings.Save();
    }
    else
      CLog::Log(LOGERROR, "    no ttf font found, but needed for the language %s.", g_guiSettings.GetString("locale.language").c_str());
  }
  g_colorManager.Load(g_guiSettings.GetString("lookandfeel.skincolors"));

  g_fontManager.LoadFonts(g_guiSettings.GetString("lookandfeel.font"));

  // load in the skin strings
  CStdString skinPath, skinEnglishPath;
  CUtil::AddFileToFolder(strSkinPath, "language", skinPath);
  CUtil::AddFileToFolder(skinPath, g_guiSettings.GetString("locale.language"), skinPath);
  CUtil::AddFileToFolder(skinPath, "strings.xml", skinPath);

  CUtil::AddFileToFolder(strSkinPath, "language", skinEnglishPath);
  CUtil::AddFileToFolder(skinEnglishPath, "English", skinEnglishPath);
  CUtil::AddFileToFolder(skinEnglishPath, "strings.xml", skinEnglishPath);

  g_localizeStrings.LoadSkinStrings(skinPath, skinEnglishPath);

  LARGE_INTEGER start;
  QueryPerformanceCounter(&start);

  CLog::Log(LOGINFO, "  load new skin...");
  CGUIWindowHome *pHome = (CGUIWindowHome *)m_gWindowManager.GetWindow(WINDOW_HOME);
  if (!g_SkinInfo.Check(strSkinPath) || !pHome || !pHome->Load("Home.xml"))
  {
    // failed to load home.xml
    // fallback to default skin
    if ( strcmpi(strSkin.c_str(), DEFAULT_SKIN) != 0)
    {
      CLog::Log(LOGERROR, "failed to load home.xml for skin:%s, fallback to \"%s\" skin", strSkin.c_str(), DEFAULT_SKIN);
      g_guiSettings.SetString("lookandfeel.skin", DEFAULT_SKIN);
      LoadSkin(g_guiSettings.GetString("lookandfeel.skin"));
      return ;
    }
  }

  // Load the user windows
  LoadUserWindows(strSkinPath);

  LARGE_INTEGER end, freq;
  QueryPerformanceCounter(&end);
  QueryPerformanceFrequency(&freq);
  CLog::Log(LOGDEBUG,"Load Skin XML: %.2fms", 1000.f * (end.QuadPart - start.QuadPart) / freq.QuadPart);

  CLog::Log(LOGINFO, "  initialize new skin...");
  m_guiPointer.AllocResources(true);
  m_guiDialogVolumeBar.AllocResources(true);
  m_guiDialogSeekBar.AllocResources(true);
  m_guiDialogKaiToast.AllocResources(true);
  m_guiDialogMuteBug.AllocResources(true);
  m_gWindowManager.AddMsgTarget(this);
  m_gWindowManager.AddMsgTarget(&g_playlistPlayer);
  m_gWindowManager.AddMsgTarget(&g_infoManager);
  m_gWindowManager.SetCallback(*this);
  m_gWindowManager.Initialize();
  g_audioManager.Initialize(CAudioContext::DEFAULT_DEVICE);
  g_audioManager.Load();

  CGUIDialogFullScreenInfo* pDialog = NULL;
  RESOLUTION res;
  CStdString strPath = g_SkinInfo.GetSkinPath("DialogFullScreenInfo.xml", &res);
  if (CFile::Exists(strPath))
    pDialog = new CGUIDialogFullScreenInfo;

  if (pDialog)
    m_gWindowManager.Add(pDialog); // window id = 142

  CLog::Log(LOGINFO, "  skin loaded...");

  // leave the graphics lock
  lock.Leave();
  g_ApplicationRenderer.Start();

  // restore windows
  if (currentWindow != WINDOW_INVALID)
  {
    m_gWindowManager.ActivateWindow(currentWindow);
    for (unsigned int i = 0; i < currentModelessWindows.size(); i++)
    {
      CGUIDialog *dialog = (CGUIDialog *)m_gWindowManager.GetWindow(currentModelessWindows[i]);
      if (dialog) dialog->Show();
    }
  }

  if (g_application.m_pPlayer && g_application.IsPlayingVideo())
  {
    if (bPreviousPlayingState)
      g_application.m_pPlayer->Pause();
    if (bPreviousRenderingState)
      m_gWindowManager.ActivateWindow(WINDOW_FULLSCREEN_VIDEO);
  }
}

void CApplication::UnloadSkin()
{
  g_ApplicationRenderer.Stop();
  g_audioManager.DeInitialize(CAudioContext::DEFAULT_DEVICE);

  m_gWindowManager.DeInitialize();

  //These windows are not handled by the windowmanager (why not?) so we should unload them manually
  CGUIMessage msg(GUI_MSG_WINDOW_DEINIT, 0, 0);
  m_guiPointer.OnMessage(msg);
  m_guiPointer.ResetControlStates();
  m_guiPointer.FreeResources(true);
  m_guiDialogMuteBug.OnMessage(msg);
  m_guiDialogMuteBug.ResetControlStates();
  m_guiDialogMuteBug.FreeResources(true);

  // remove the skin-dependent window
  m_gWindowManager.Delete(WINDOW_DIALOG_FULLSCREEN_INFO);

  g_TextureManager.Cleanup();

  g_fontManager.Clear();

  g_colorManager.Clear();

  g_charsetConverter.reset();

  g_infoManager.Clear();
}

bool CApplication::LoadUserWindows(const CStdString& strSkinPath)
{
  WIN32_FIND_DATA FindFileData;
  WIN32_FIND_DATA NextFindFileData;
  HANDLE hFind;
  TiXmlDocument xmlDoc;
  RESOLUTION resToUse = INVALID;

  // Start from wherever home.xml is
  g_SkinInfo.GetSkinPath("Home.xml", &resToUse);
  std::vector<CStdString> vecSkinPath;
  if (resToUse == HDTV_1080i)
    vecSkinPath.push_back(CUtil::AddFileToFolder(strSkinPath, g_SkinInfo.GetDirFromRes(HDTV_1080i)));
  if (resToUse == HDTV_720p)
    vecSkinPath.push_back(CUtil::AddFileToFolder(strSkinPath, g_SkinInfo.GetDirFromRes(HDTV_720p)));
  if (resToUse == PAL_16x9 || resToUse == NTSC_16x9 || resToUse == HDTV_480p_16x9 || resToUse == HDTV_720p || resToUse == HDTV_1080i)
    vecSkinPath.push_back(CUtil::AddFileToFolder(strSkinPath, g_SkinInfo.GetDirFromRes(g_SkinInfo.GetDefaultWideResolution())));
  vecSkinPath.push_back(CUtil::AddFileToFolder(strSkinPath, g_SkinInfo.GetDirFromRes(g_SkinInfo.GetDefaultResolution())));
  for (unsigned int i = 0;i < vecSkinPath.size();++i)
  {
    CStdString strPath = CUtil::AddFileToFolder(vecSkinPath[i], "custom*.xml");
    CLog::Log(LOGINFO, "Loading user windows, path %s", vecSkinPath[i].c_str());
    hFind = FindFirstFile(_P(strPath).c_str(), &NextFindFileData);

    CStdString strFileName;
    while (hFind != INVALID_HANDLE_VALUE)
    {
      FindFileData = NextFindFileData;

      if (!FindNextFile(hFind, &NextFindFileData))
      {
        FindClose(hFind);
        hFind = INVALID_HANDLE_VALUE;
      }

      // skip "up" directories, which come in all queries
      if (!strcmp(FindFileData.cFileName, ".") || !strcmp(FindFileData.cFileName, ".."))
        continue;

      strFileName = CUtil::AddFileToFolder(vecSkinPath[i], FindFileData.cFileName);
      CLog::Log(LOGINFO, "Loading skin file: %s", strFileName.c_str());
      CStdString strLower(FindFileData.cFileName);
      strLower.MakeLower();
      strLower = CUtil::AddFileToFolder(vecSkinPath[i], strLower);
      if (!xmlDoc.LoadFile(strFileName) && !xmlDoc.LoadFile(strLower))
      {
        CLog::Log(LOGERROR, "unable to load:%s, Line %d\n%s", strFileName.c_str(), xmlDoc.ErrorRow(), xmlDoc.ErrorDesc());
        continue;
      }

      // Root element should be <window>
      TiXmlElement* pRootElement = xmlDoc.RootElement();
      CStdString strValue = pRootElement->Value();
      if (!strValue.Equals("window"))
      {
        CLog::Log(LOGERROR, "file :%s doesnt contain <window>", strFileName.c_str());
        continue;
      }

      // Read the <type> element to get the window type to create
      // If no type is specified, create a CGUIWindow as default
      CGUIWindow* pWindow = NULL;
      CStdString strType;
      if (pRootElement->Attribute("type"))
        strType = pRootElement->Attribute("type");
      else
      {
        const TiXmlNode *pType = pRootElement->FirstChild("type");
        if (pType && pType->FirstChild())
          strType = pType->FirstChild()->Value();
      }
      if (strType.Equals("dialog"))
        pWindow = new CGUIDialog(0, "");
      else if (strType.Equals("submenu"))
        pWindow = new CGUIDialogSubMenu();
      else if (strType.Equals("buttonmenu"))
        pWindow = new CGUIDialogButtonMenu();
      else
        pWindow = new CGUIStandardWindow();

      int id = WINDOW_INVALID;
      if (!pRootElement->Attribute("id", &id))
      {
        const TiXmlNode *pType = pRootElement->FirstChild("id");
        if (pType && pType->FirstChild())
          id = atol(pType->FirstChild()->Value());
      }
      // Check to make sure the pointer isn't still null
      if (pWindow == NULL || id == WINDOW_INVALID)
      {
        CLog::Log(LOGERROR, "Out of memory / Failed to create new object in LoadUserWindows");
        return false;
      }
      if (m_gWindowManager.GetWindow(WINDOW_HOME + id))
      {
        delete pWindow;
        continue;
      }
      // set the window's xml file, and add it to the window manager.
      pWindow->SetXMLFile(FindFileData.cFileName);
      pWindow->SetID(WINDOW_HOME + id);
      m_gWindowManager.AddCustomWindow(pWindow);
    }
    CloseHandle(hFind);
  }
  return true;
}

void CApplication::RenderNoPresent()
{
  MEASURE_FUNCTION;

#ifdef HAS_SDL
  int vsync_mode = g_videoConfig.GetVSyncMode();
#endif
  // dont show GUI when playing full screen video
  if (g_graphicsContext.IsFullScreenVideo() && IsPlaying() && !IsPaused())
  {
#ifdef HAS_SDL
    if (vsync_mode==VSYNC_VIDEO)
      g_graphicsContext.getScreenSurface()->EnableVSync(true);
#endif
    if (m_bPresentFrame)
      g_renderManager.Present();
    else
      g_renderManager.RenderUpdate(true, 0, 255);

    if (NeedRenderFullScreen())
      RenderFullScreen();
    RenderMemoryStatus();

    ResetScreenSaver();
    g_infoManager.ResetCache();
    return;
  }

  g_graphicsContext.AcquireCurrentContext();

#ifdef HAS_SDL
  if (vsync_mode==VSYNC_ALWAYS)
    g_graphicsContext.getScreenSurface()->EnableVSync(true);
  else if (vsync_mode!=VSYNC_DRIVER)
    g_graphicsContext.getScreenSurface()->EnableVSync(false);
#endif

  g_ApplicationRenderer.Render();

}

void CApplication::DoRender()
{
#ifndef HAS_SDL
  if(!m_pd3dDevice)
    return;
#endif

  g_graphicsContext.Lock();

#ifndef HAS_SDL
  m_pd3dDevice->BeginScene();
#endif

  m_gWindowManager.UpdateModelessVisibility();

  // draw GUI
  g_graphicsContext.Clear();
  //SWATHWIDTH of 4 improves fillrates (performance investigator)
  m_gWindowManager.Render();


  // if we're recording an audio stream then show blinking REC
  if (!g_graphicsContext.IsFullScreenVideo())
  {
    if (m_pPlayer && m_pPlayer->IsRecording() )
    {
      static int iBlinkRecord = 0;
      iBlinkRecord++;
      if (iBlinkRecord > 25)
      {
        CGUIFont* pFont = g_fontManager.GetFont("font13");
        CGUITextLayout::DrawText(pFont, 60, 50, 0xffff0000, 0, "REC", 0);
      }

      if (iBlinkRecord > 50)
        iBlinkRecord = 0;
    }
  }

  // Now render any dialogs
  m_gWindowManager.RenderDialogs();

  // Render the mouse pointer
  if (g_Mouse.IsActive())
  {
    m_guiPointer.Render();
  }

  {
    // reset image scaling and effect states
    g_graphicsContext.SetRenderingResolution(g_graphicsContext.GetVideoResolution(), 0, 0, false);

    // If we have the remote codes enabled, then show them
    if (g_advancedSettings.m_displayRemoteCodes)
    {
      // TODO ?
    }

    RenderMemoryStatus();
  }

  RenderScreenSaver();

#ifndef HAS_SDL
  m_pd3dDevice->EndScene();
#endif

  g_graphicsContext.Unlock();

  // reset our info cache - we do this at the end of Render so that it is
  // fresh for the next process(), or after a windowclose animation (where process()
  // isn't called)
  g_infoManager.ResetCache();
}

static int screenSaverFadeAmount = 0;

void CApplication::RenderScreenSaver()
{
  bool draw = false;
  float amount = 0.0f;
  if (m_screenSaverMode == "Dim")
    amount = 1.0f - g_guiSettings.GetInt("screensaver.dimlevel")*0.01f;
  else if (m_screenSaverMode == "Black")
    amount = 1.0f; // fully fade
  // special case for dim screensaver
  if (amount > 0.f)
  {
    if (m_bScreenSave)
    {
      draw = true;
      if (screenSaverFadeAmount < 100)
      {
        screenSaverFadeAmount = std::min(100, screenSaverFadeAmount + 2);  // around a second to fade
      }
    }
    else
    {
      if (screenSaverFadeAmount > 0)
      {
        draw = true;
        screenSaverFadeAmount = std::max(0, screenSaverFadeAmount - 4);  // around a half second to unfade
      }
    }
  }
  if (draw)
  {
    DWORD color = ((DWORD)(screenSaverFadeAmount * amount * 2.55f) & 0xff) << 24;
    CGUITexture::DrawQuad(CRect(0, 0, (float)g_graphicsContext.GetWidth(), (float)g_graphicsContext.GetHeight()), color);
  }
}

bool CApplication::WaitFrame(DWORD timeout)
{
  bool done = false;
#ifdef HAS_SDL
  // Wait for all other frames to be presented
  SDL_mutexP(m_frameMutex);
  while(m_frameCount > 0)
  {
    int result = SDL_CondWaitTimeout(m_frameCond, m_frameMutex, timeout);
    if(result == SDL_MUTEX_TIMEDOUT)
      break;
    if(result < 0)
      CLog::Log(LOGWARNING, "CApplication::WaitFrame - error from conditional wait");
  }  
  done = m_frameCount == 0;
  SDL_mutexV(m_frameMutex);
#endif
  return done;
}

void CApplication::NewFrame()
{
#ifdef HAS_SDL
  // We just posted another frame. Keep track and notify.
  SDL_mutexP(m_frameMutex);
  m_frameCount++;
  SDL_mutexV(m_frameMutex);

  SDL_CondBroadcast(m_frameCond);
#endif
}

void CApplication::Render()
{
  if (!m_AppActive && !m_bStop && (!IsPlayingVideo() || IsPaused()))
  {
    Sleep(1);
    ResetScreenSaver();
    return;
  }

  MEASURE_FUNCTION;

  { // frame rate limiter (really bad, but it does the trick :p)
    static unsigned int lastFrameTime = 0;
    unsigned int currentTime = timeGetTime();
    int nDelayTime = 0;
    // Less fps in DPMS or Black screensaver
    bool lowfps = (m_dpmsIsActive
                   || (m_bScreenSave && (m_screenSaverMode == "Black")
                       && (screenSaverFadeAmount >= 100)));
    unsigned int singleFrameTime = 10; // default limit 100 fps


    m_bPresentFrame = false;
    if (g_graphicsContext.IsFullScreenVideo() && !IsPaused())
    {
#ifdef HAS_SDL
      SDL_mutexP(m_frameMutex);

      // If we have frames or if we get notified of one, consume it.
      while(m_frameCount == 0)
      {
        int result = SDL_CondWaitTimeout(m_frameCond, m_frameMutex, 100);
        if(result == SDL_MUTEX_TIMEDOUT)
          break;
        if(result < 0)
          CLog::Log(LOGWARNING, "CApplication::Render - error from conditional wait");
      }

      m_bPresentFrame = m_frameCount > 0;
      SDL_mutexV(m_frameMutex);
#else
      m_bPresentFrame = true;
#endif
    }
    else
    {
      // engage the frame limiter as needed
      bool limitFrames = lowfps;
      if (g_videoConfig.GetVSyncMode() == VSYNC_DISABLED ||
          g_videoConfig.GetVSyncMode() == VSYNC_VIDEO)
        limitFrames = true; // not using vsync.
      else if ((g_infoManager.GetFPS() > g_graphicsContext.GetFPS() + 10) && g_infoManager.GetFPS() > 1000/singleFrameTime)
        limitFrames = true; // using vsync, but it isn't working.

      if (limitFrames)
      {
        if(lowfps)
          singleFrameTime = 200;  // 5 fps, <=200 ms latency to wake up

        if (lastFrameTime + singleFrameTime > currentTime)
          nDelayTime = lastFrameTime + singleFrameTime - currentTime;
        Sleep(nDelayTime);
      }
    }

    lastFrameTime = timeGetTime();
  }
  g_graphicsContext.Lock();
  RenderNoPresent();
  g_graphicsContext.Flip();
  g_infoManager.UpdateFPS();
  g_graphicsContext.Unlock();

#ifdef HAS_SDL
  SDL_mutexP(m_frameMutex);
  if(m_frameCount > 0)
    m_frameCount--;
  SDL_mutexV(m_frameMutex);
  SDL_CondBroadcast(m_frameCond);
#endif
}

void CApplication::RenderMemoryStatus()
{
  MEASURE_FUNCTION;

  g_cpuInfo.getUsedPercentage(); // must call it to recalculate pct values

  if (LOG_LEVEL_DEBUG_FREEMEM <= g_advancedSettings.m_logLevel)
  {
    // reset the window scaling and fade status
    RESOLUTION res = g_graphicsContext.GetVideoResolution();
    g_graphicsContext.SetRenderingResolution(res, 0, 0, false);

    CStdString info;
    MEMORYSTATUS stat;
    GlobalMemoryStatus(&stat);
    CStdString profiling = CGUIControlProfiler::IsRunning() ? " (profiling)" : "";
#ifdef __APPLE__
    double dCPU = m_resourceCounter.GetCPUUsage();
    info.Format("FreeMem %ju/%ju MB, FPS %2.1f, CPU-Total %d%%. CPU-XBMC %4.2f%%%s", stat.dwAvailPhys/(1024*1024), stat.dwTotalPhys/(1024*1024),
              g_infoManager.GetFPS(), g_cpuInfo.getUsedPercentage(), dCPU, profiling.c_str());
#elif !defined(_LINUX)
    CStdString strCores = g_cpuInfo.GetCoresUsageString();
    info.Format("FreeMem %d/%d Kb, FPS %2.1f, %s%s", stat.dwAvailPhys/1024, stat.dwTotalPhys/1024, g_infoManager.GetFPS(), strCores.c_str(), profiling.c_str());
#else
    double dCPU = m_resourceCounter.GetCPUUsage();
    CStdString strCores = g_cpuInfo.GetCoresUsageString();
    info.Format("FreeMem %d/%d Kb, FPS %2.1f, %s. CPU-XBMC %4.2f%%%s", stat.dwAvailPhys/1024, stat.dwTotalPhys/1024,
              g_infoManager.GetFPS(), strCores.c_str(), dCPU, profiling.c_str());
#endif

    static int yShift = 20;
    static int xShift = 40;
    static unsigned int lastShift = time(NULL);
    time_t now = time(NULL);
    if (now - lastShift > 10)
    {
      yShift *= -1;
      if (now % 5 == 0)
        xShift *= -1;
      lastShift = now;
    }

    float x = xShift + 0.04f * g_graphicsContext.GetWidth() + g_settings.m_ResInfo[res].Overscan.left;
    float y = yShift + 0.04f * g_graphicsContext.GetHeight() + g_settings.m_ResInfo[res].Overscan.top;

    CGUITextLayout::DrawOutlineText(g_fontManager.GetFont("font13"), x, y, 0xffffffff, 0xff000000, 2, info);
  }
}
// OnKey() translates the key into a CAction which is sent on to our Window Manager.
// The window manager will return true if the event is processed, false otherwise.
// If not already processed, this routine handles global keypresses.  It returns
// true if the key has been processed, false otherwise.

bool CApplication::OnKey(CKey& key)
{
  // Turn the mouse off, as we've just got a keypress from controller or remote
  g_Mouse.SetInactive();
  CAction action;

  // get the current active window
  int iWin = m_gWindowManager.GetActiveWindow() & WINDOW_ID_MASK;

  // this will be checked for certain keycodes that need
  // special handling if the screensaver is active
  CButtonTranslator::GetInstance().GetAction(iWin, key, action);

  // a key has been pressed.
  // Reset the screensaver timer
  // but not for the analog thumbsticks/triggers
  if (!key.IsAnalogButton())
  {
    // reset Idle Timer
    m_idleTimer.StartZero();
    bool processKey = AlwaysProcess(action);

    ResetScreenSaver();

    // allow some keys to be processed while the screensaver is active
    if (WakeUpScreenSaverAndDPMS() && !processKey)
    {
      g_Keyboard.Reset();
      return true;
    }
  }

  // change this if we have a dialog up
  if (m_gWindowManager.HasModalDialog())
  {
    iWin = m_gWindowManager.GetTopMostModalDialogID() & WINDOW_ID_MASK;
  }
  if (iWin == WINDOW_DIALOG_FULLSCREEN_INFO)
  { // fullscreen info dialog - special case
    CButtonTranslator::GetInstance().GetAction(iWin, key, action);

#ifdef HAS_SDL
    g_Keyboard.Reset();
#endif
    if (OnAction(action))
      return true;

    // fallthrough to the main window
    iWin = WINDOW_FULLSCREEN_VIDEO;
  }
  if (iWin == WINDOW_FULLSCREEN_VIDEO)
  {
    // current active window is full screen video.
    if (g_application.m_pPlayer && g_application.m_pPlayer->IsInMenu())
    {
      // if player is in some sort of menu, (ie DVDMENU) map buttons differently
      CButtonTranslator::GetInstance().GetAction(WINDOW_VIDEO_MENU, key, action);
    }
    else
    {
      // no then use the fullscreen window section of keymap.xml to map key->action
      CButtonTranslator::GetInstance().GetAction(iWin, key, action);
    }
  }
  else
  {
    // current active window isnt the fullscreen window
    // just use corresponding section from keymap.xml
    // to map key->action

    // first determine if we should use keyboard input directly
    bool useKeyboard = key.FromKeyboard() && (iWin == WINDOW_DIALOG_KEYBOARD || iWin == WINDOW_DIALOG_NUMERIC);
    CGUIWindow *window = m_gWindowManager.GetWindow(iWin);
    if (window)
    {
      CGUIControl *control = window->GetFocusedControl();
      if (control)
      {
        if (control->GetControlType() == CGUIControl::GUICONTROL_EDIT ||
            (control->IsContainer() && g_Keyboard.GetShift()))
          useKeyboard = true;
      }
    }
    if (useKeyboard)
    {
      if (key.GetFromHttpApi())
      {
        if (key.GetButtonCode() != KEY_INVALID)
          action.wID = (WORD) key.GetButtonCode();
        action.unicode = (WCHAR)key.GetUnicode();
      }
      else
      { // see if we've got an ascii key
        if (g_Keyboard.GetUnicode())
        {
          action.wID = (WORD)g_Keyboard.GetAscii() | KEY_ASCII; // Only for backwards compatibility
          action.unicode = g_Keyboard.GetUnicode();
        }
        else
        {
          action.wID = (WORD)g_Keyboard.GetKey() | KEY_VKEY;
          action.unicode = 0;
        }
      }
#ifdef HAS_SDL
      g_Keyboard.Reset();
#endif
      if (OnAction(action))
        return true;
      // failed to handle the keyboard action, drop down through to standard action
    }
    if (key.GetFromHttpApi())
    {
      if (key.GetButtonCode() != KEY_INVALID)
      {
        action.wID = (WORD) key.GetButtonCode();
        CButtonTranslator::GetInstance().GetAction(iWin, key, action);
      }
    }
    else
      CButtonTranslator::GetInstance().GetAction(iWin, key, action);
  }
  if (!key.IsAnalogButton())
    CLog::Log(LOGDEBUG, "%s: %i pressed, action is %i", __FUNCTION__, (int) key.GetButtonCode(), action.wID);

  //  Play a sound based on the action
  g_audioManager.PlayActionSound(action);

#ifdef HAS_SDL
  g_Keyboard.Reset();
#endif

  return OnAction(action);
}

bool CApplication::OnAction(CAction &action)
{
#ifdef HAS_WEB_SERVER
  // Let's tell the outside world about this action
  if (m_pXbmcHttp && g_stSettings.m_HttpApiBroadcastLevel>=2)
  {
    CStdString tmp;
    tmp.Format("%i",action.wID);
    getApplicationMessenger().HttpApi("broadcastlevel; OnAction:"+tmp+";2");
  }
#endif

  if (action.wID == m_lastActionCode)
    action.holdTime = (unsigned int)m_lastActionTimer.GetElapsedMilliseconds();
  else
    m_lastActionTimer.StartZero();
  m_lastActionCode = action.wID;

  // special case for switching between GUI & fullscreen mode.
  if (action.wID == ACTION_SHOW_GUI)
  { // Switch to fullscreen mode if we can
    if (SwitchToFullScreen())
    {
      m_navigationTimer.StartZero();
      return true;
    }
  }

  if (action.wID == ACTION_TOGGLE_FULLSCREEN)
  {
    g_graphicsContext.ToggleFullScreenRoot();
    return true;
  }

  // in normal case
  // just pass the action to the current window and let it handle it
  if (m_gWindowManager.OnAction(action))
  {
    m_navigationTimer.StartZero();
    return true;
  }

  // handle extra global presses

  // screenshot : take a screenshot :)
  if (action.wID == ACTION_TAKE_SCREENSHOT)
  {
    CUtil::TakeScreenshot();
    return true;
  }
  // built in functions : execute the built-in
  if (action.wID == ACTION_BUILT_IN_FUNCTION)
  {
    CUtil::ExecBuiltIn(action.strAction);
    m_navigationTimer.StartZero();
    return true;
  }

  // reload keymaps
  if (action.wID == ACTION_RELOAD_KEYMAPS)
  {
    CButtonTranslator::GetInstance().Clear();
    CButtonTranslator::GetInstance().Load();
  }

  // show info : Shows the current video or song information
  if (action.wID == ACTION_SHOW_INFO)
  {
    g_infoManager.ToggleShowInfo();
    return true;
  }

  // codec info : Shows the current song, video or picture codec information
  if (action.wID == ACTION_SHOW_CODEC)
  {
    g_infoManager.ToggleShowCodec();
    return true;
  }

  if ((action.wID == ACTION_INCREASE_RATING || action.wID == ACTION_DECREASE_RATING) && IsPlayingAudio())
  {
    const CMusicInfoTag *tag = g_infoManager.GetCurrentSongTag();
    if (tag)
    {
      *m_itemCurrentFile->GetMusicInfoTag() = *tag;
      char rating = tag->GetRating();
      bool needsUpdate(false);
      if (rating > '0' && action.wID == ACTION_DECREASE_RATING)
      {
        m_itemCurrentFile->GetMusicInfoTag()->SetRating(rating - 1);
        needsUpdate = true;
      }
      else if (rating < '5' && action.wID == ACTION_INCREASE_RATING)
      {
        m_itemCurrentFile->GetMusicInfoTag()->SetRating(rating + 1);
        needsUpdate = true;
      }
      if (needsUpdate)
      {
        CMusicDatabase db;
        if (db.Open())      // OpenForWrite() ?
        {
          db.SetSongRating(m_itemCurrentFile->m_strPath, m_itemCurrentFile->GetMusicInfoTag()->GetRating());
          db.Close();
        }
        // send a message to all windows to tell them to update the fileitem (eg playlistplayer, media windows)
        CGUIMessage msg(GUI_MSG_NOTIFY_ALL, 0, 0, GUI_MSG_UPDATE_ITEM, 0, m_itemCurrentFile);
        g_graphicsContext.SendMessage(msg);
      }
    }
    return true;
  }

  // stop : stops playing current audio song
  if (action.wID == ACTION_STOP)
  {
    StopPlaying();
    return true;
  }

  // previous : play previous song from playlist
  if (action.wID == ACTION_PREV_ITEM)
  {
    // first check whether we're within 3 seconds of the start of the track
    // if not, we just revert to the start of the track
    if (m_pPlayer && m_pPlayer->CanSeek() && GetTime() > 3)
    {
      SeekTime(0);
      SetPlaySpeed(1);
    }
    else
    {
      g_playlistPlayer.PlayPrevious();
    }
    return true;
  }

  // next : play next song from playlist
  if (action.wID == ACTION_NEXT_ITEM)
  {
    if (IsPlaying() && m_pPlayer->SkipNext())
      return true;

    g_playlistPlayer.PlayNext();

    return true;
  }

  if ( IsPlaying())
  {
    // OSD toggling
    if (action.wID == ACTION_SHOW_OSD)
    {
      if (IsPlayingVideo() && IsPlayingFullScreenVideo())
      {
        CGUIWindowOSD *pOSD = (CGUIWindowOSD *)m_gWindowManager.GetWindow(WINDOW_OSD);
        if (pOSD)
        {
          if (pOSD->IsDialogRunning())
            pOSD->Close();
          else
            pOSD->DoModal();
          return true;
        }
      }
    }

    // pause : pauses current audio song
    if (action.wID == ACTION_PAUSE && m_iPlaySpeed == 1)
    {
      m_pPlayer->Pause();
#ifdef HAS_KARAOKE
      m_pKaraokeMgr->SetPaused( m_pPlayer->IsPaused() );
#endif
      if (!m_pPlayer->IsPaused())
      { // unpaused - set the playspeed back to normal
        SetPlaySpeed(1);
      }
      if (!g_guiSettings.GetBool("lookandfeel.soundsduringplayback"))
        g_audioManager.Enable(m_pPlayer->IsPaused());
      return true;
    }
    if (!m_pPlayer->IsPaused())
    {
      // if we do a FF/RW in my music then map PLAY action togo back to normal speed
      // if we are playing at normal speed, then allow play to pause
      if (action.wID == ACTION_PLAYER_PLAY || action.wID == ACTION_PAUSE)
      {
        if (m_iPlaySpeed != 1)
        {
          SetPlaySpeed(1);
        }
        else
        {
          m_pPlayer->Pause();
        }
        return true;
      }
      if (action.wID == ACTION_PLAYER_FORWARD || action.wID == ACTION_PLAYER_REWIND)
      {
        int iPlaySpeed = m_iPlaySpeed;
        if (action.wID == ACTION_PLAYER_REWIND && iPlaySpeed == 1) // Enables Rewinding
          iPlaySpeed *= -2;
        else if (action.wID == ACTION_PLAYER_REWIND && iPlaySpeed > 1) //goes down a notch if you're FFing
          iPlaySpeed /= 2;
        else if (action.wID == ACTION_PLAYER_FORWARD && iPlaySpeed < 1) //goes up a notch if you're RWing
          iPlaySpeed /= 2;
        else
          iPlaySpeed *= 2;

        if (action.wID == ACTION_PLAYER_FORWARD && iPlaySpeed == -1) //sets iSpeed back to 1 if -1 (didn't plan for a -1)
          iPlaySpeed = 1;
        if (iPlaySpeed > 32 || iPlaySpeed < -32)
          iPlaySpeed = 1;

        SetPlaySpeed(iPlaySpeed);
        return true;
      }
      else if ((action.fAmount1 || GetPlaySpeed() != 1) && (action.wID == ACTION_ANALOG_REWIND || action.wID == ACTION_ANALOG_FORWARD))
      {
        // calculate the speed based on the amount the button is held down
        int iPower = (int)(action.fAmount1 * MAX_FFWD_SPEED + 0.5f);
        // returns 0 -> MAX_FFWD_SPEED
        int iSpeed = 1 << iPower;
        if (iSpeed != 1 && action.wID == ACTION_ANALOG_REWIND)
          iSpeed = -iSpeed;
        g_application.SetPlaySpeed(iSpeed);
        if (iSpeed == 1)
          CLog::Log(LOGDEBUG,"Resetting playspeed");
        return true;
      }
    }
    // allow play to unpause
    else
    {
      if (action.wID == ACTION_PLAYER_PLAY)
      {
        // unpause, and set the playspeed back to normal
        m_pPlayer->Pause();
        if (!g_guiSettings.GetBool("lookandfeel.soundsduringplayback"))
          g_audioManager.Enable(m_pPlayer->IsPaused());

        g_application.SetPlaySpeed(1);
        return true;
      }
    }
  }
  if (action.wID == ACTION_MUTE)
  {
    Mute();
    return true;
  }

  if (action.wID == ACTION_TOGGLE_DIGITAL_ANALOG)
  {
    if(g_guiSettings.GetInt("audiooutput.mode")==AUDIO_DIGITAL)
      g_guiSettings.SetInt("audiooutput.mode", AUDIO_ANALOG);
    else
      g_guiSettings.SetInt("audiooutput.mode", AUDIO_DIGITAL);
    g_application.Restart();
    if (m_gWindowManager.GetActiveWindow() == WINDOW_SETTINGS_SYSTEM)
    {
      CGUIMessage msg(GUI_MSG_WINDOW_INIT, 0,0,WINDOW_INVALID,m_gWindowManager.GetActiveWindow());
      m_gWindowManager.SendMessage(msg);
    }
    return true;
  }

  // Check for global volume control
  if (action.fAmount1 && (action.wID == ACTION_VOLUME_UP || action.wID == ACTION_VOLUME_DOWN))
  {
    if (!m_pPlayer || !m_pPlayer->IsPassthrough())
    {
      // increase or decrease the volume
      int volume = g_stSettings.m_nVolumeLevel + g_stSettings.m_dynamicRangeCompressionLevel;

      // calculate speed so that a full press will equal 1 second from min to max
      float speed = float(VOLUME_MAXIMUM - VOLUME_MINIMUM);
      if( action.fRepeat )
        speed *= action.fRepeat;
      else
        speed /= 50; //50 fps
      if (g_stSettings.m_bMute)
      {
        // only unmute if volume is to be increased, otherwise leave muted
        if (action.wID == ACTION_VOLUME_DOWN)
          return true;
        Mute();
        return true;
      }
      if (action.wID == ACTION_VOLUME_UP)
      {
        volume += (int)((float)fabs(action.fAmount1) * action.fAmount1 * speed);
      }
      else
      {
        volume -= (int)((float)fabs(action.fAmount1) * action.fAmount1 * speed);
      }

      SetHardwareVolume(volume);
  #ifndef HAS_SDL_AUDIO
      g_audioManager.SetVolume(g_stSettings.m_nVolumeLevel);
  #else
      g_audioManager.SetVolume((int)(128.f * (g_stSettings.m_nVolumeLevel - VOLUME_MINIMUM) / (float)(VOLUME_MAXIMUM - VOLUME_MINIMUM)));
  #endif
    }
    // show visual feedback of volume change...
    m_guiDialogVolumeBar.Show();
    m_guiDialogVolumeBar.OnAction(action);
    return true;
  }
  // Check for global seek control
  if (IsPlaying() && action.fAmount1 && (action.wID == ACTION_ANALOG_SEEK_FORWARD || action.wID == ACTION_ANALOG_SEEK_BACK))
  {
    if (!m_pPlayer->CanSeek()) return false;
    m_guiDialogSeekBar.OnAction(action);
    return true;
  }
  if (action.wID == ACTION_GUIPROFILE_BEGIN)
  {
    CGUIControlProfiler::Instance().SetOutputFile(_P("special://home/guiprofiler.xml"));
    CGUIControlProfiler::Instance().Start();
    return true;
  }
  return false;
}

void CApplication::UpdateLCD()
{
#ifdef HAS_LCD
  static long lTickCount = 0;

  if (!g_lcd || g_guiSettings.GetInt("lcd.type") == LCD_TYPE_NONE)
    return ;
  long lTimeOut = 1000;
  if ( m_iPlaySpeed != 1)
    lTimeOut = 0;
  if ( ((long)GetTickCount() - lTickCount) >= lTimeOut)
  {
    if (g_application.NavigationIdleTime() < 5)
      g_lcd->Render(ILCD::LCD_MODE_NAVIGATION);
    else if (IsPlayingVideo())
      g_lcd->Render(ILCD::LCD_MODE_VIDEO);
    else if (IsPlayingAudio())
      g_lcd->Render(ILCD::LCD_MODE_MUSIC);
    else if (IsInScreenSaver())
      g_lcd->Render(ILCD::LCD_MODE_SCREENSAVER);
    else
      g_lcd->Render(ILCD::LCD_MODE_GENERAL);

    // reset tick count
    lTickCount = GetTickCount();
  }
#endif
}

void CApplication::FrameMove()
{
  MEASURE_FUNCTION;

  // currently we calculate the repeat time (ie time from last similar keypress) just global as fps
  float frameTime = m_frameTime.GetElapsedSeconds();
  m_frameTime.StartZero();
  // never set a frametime less than 2 fps to avoid problems when debuggin and on breaks
  if( frameTime > 0.5 ) frameTime = 0.5;

  // check if there are notifications to display
  if (m_guiDialogKaiToast.DoWork())
  {
    if (!m_guiDialogKaiToast.IsDialogRunning())
    {
      m_guiDialogKaiToast.Show();
    }
  }

  UpdateLCD();

  // read raw input from controller, remote control, mouse and keyboard
  ReadInput();
  // process input actions
  bool didSomething = ProcessMouse();
  didSomething |= ProcessHTTPApiButtons();
  didSomething |= ProcessKeyboard();
  didSomething |= ProcessRemote(frameTime);
  didSomething |= ProcessGamepad(frameTime);
  didSomething |= ProcessEventServer(frameTime);
  // reset our previous action code
  if (!didSomething)
    m_lastActionCode = 0;
}

bool CApplication::ProcessGamepad(float frameTime)
{
#ifdef HAS_SDL_JOYSTICK
  int iWin = m_gWindowManager.GetActiveWindow() & WINDOW_ID_MASK;
  if (m_gWindowManager.HasModalDialog())
  {
    iWin = m_gWindowManager.GetTopMostModalDialogID() & WINDOW_ID_MASK;
  }
  int bid;
  g_Joystick.Update();
  if (g_Joystick.GetButton(bid))
  {
    // reset Idle Timer
    m_idleTimer.StartZero();

    ResetScreenSaver();
    if (WakeUpScreenSaverAndDPMS())
    {
      g_Joystick.Reset(true);
      return true;
    }

    CAction action;
    bool fullrange;
    string jname = g_Joystick.GetJoystick();
    if (CButtonTranslator::GetInstance().TranslateJoystickString(iWin, jname.c_str(), bid, JACTIVE_BUTTON, action.wID, action.strAction, fullrange))
    {
      action.fAmount1 = 1.0f;
      action.fRepeat = 0.0f;
      g_audioManager.PlayActionSound(action);
      g_Joystick.Reset();
      g_Mouse.SetInactive();
      return OnAction(action);
    }
    else
    {
      g_Joystick.Reset();
    }
  }
  if (g_Joystick.GetAxis(bid))
  {
    CAction action;
    bool fullrange;

    string jname = g_Joystick.GetJoystick();
    action.fAmount1 = g_Joystick.GetAmount();

    if (action.fAmount1<0)
    {
      bid = -bid;
    }
    if (CButtonTranslator::GetInstance().TranslateJoystickString(iWin, jname.c_str(), bid, JACTIVE_AXIS, action.wID, action.strAction, fullrange))
    {
      ResetScreenSaver();
      if (WakeUpScreenSaverAndDPMS())
      {
        return true;
      }

      if (fullrange)
      {
        action.fAmount1 = (action.fAmount1+1.0f)/2.0f;
      }
      else
      {
        action.fAmount1 = fabs(action.fAmount1);
      }
      action.fAmount2 = 0.0;
      action.fRepeat = 0.0;
      g_audioManager.PlayActionSound(action);
      g_Joystick.Reset();
      g_Mouse.SetInactive();
      return OnAction(action);
    }
    else
    {
      g_Joystick.ResetAxis(abs(bid));
    }
  }
  int position;
  if (g_Joystick.GetHat(bid, position))
  {
    CAction action;
    bool fullrange;

    string jname = g_Joystick.GetJoystick();
    bid = position<<16|bid;

    if (CButtonTranslator::GetInstance().TranslateJoystickString(iWin, jname.c_str(), bid, JACTIVE_HAT, action.wID, action.strAction, fullrange))
    {
      action.fAmount1 = 1.0f;
      action.fRepeat = 0.0f;
      g_audioManager.PlayActionSound(action);
      g_Joystick.Reset();
      g_Mouse.SetInactive();
      return OnAction(action);
    }
  }
#endif
  return false;
}

bool CApplication::ProcessRemote(float frameTime)
{
  // run resume jobs if we are coming from suspend/hibernate
  if (m_bRunResumeJobs)
    g_powerManager.Resume(); 

#ifdef HAS_LIRC
  if (g_RemoteControl.GetButton())
  {
    // time depends on whether the movement is repeated (held down) or not.
    // If it is, we use the FPS timer to get a repeatable speed.
    // If it isn't, we use 20 to get repeatable jumps.
    float time = (g_RemoteControl.IsHolding()) ? frameTime : 0.020f;
    CKey key(g_RemoteControl.GetButton(), 0, 0, 0, 0, 0, 0, time);
    g_RemoteControl.Reset();
    return OnKey(key);
  }
#endif
  return false;
}

bool CApplication::ProcessMouse()
{
  MEASURE_FUNCTION;

  if (!g_Mouse.IsActive() || !m_AppFocused)
    return false;

  // Reset the screensaver and idle timers
  m_idleTimer.StartZero();
  ResetScreenSaver();
  if (WakeUpScreenSaverAndDPMS())
    return true;

  // call OnAction with ACTION_MOUSE
  CAction action;
  action.wID = ACTION_MOUSE;
  action.fAmount1 = (float) m_guiPointer.GetXPosition();
  action.fAmount2 = (float) m_guiPointer.GetYPosition();

  return m_gWindowManager.OnAction(action);
}

void  CApplication::CheckForTitleChange()
{
#ifdef HAS_WEB_SERVER
  if (g_stSettings.m_HttpApiBroadcastLevel>=1)
  {
    if (IsPlayingVideo())
    {
      const CVideoInfoTag* tagVal = g_infoManager.GetCurrentMovieTag();
      if (m_pXbmcHttp && tagVal && !(tagVal->m_strTitle.IsEmpty()))
      {
        CStdString msg=m_pXbmcHttp->GetOpenTag()+"MovieTitle:"+tagVal->m_strTitle+m_pXbmcHttp->GetCloseTag();
        if (m_prevMedia!=msg && g_stSettings.m_HttpApiBroadcastLevel>=1)
        {
          getApplicationMessenger().HttpApi("broadcastlevel; MediaChanged:"+msg+";1");
          m_prevMedia=msg;
        }
      }
    }
    else if (IsPlayingAudio())
    {
      const CMusicInfoTag* tagVal=g_infoManager.GetCurrentSongTag();
      if (m_pXbmcHttp && tagVal)
      {
        CStdString msg="";
        if (!tagVal->GetTitle().IsEmpty())
          msg=m_pXbmcHttp->GetOpenTag()+"AudioTitle:"+tagVal->GetTitle()+m_pXbmcHttp->GetCloseTag();
        if (!tagVal->GetArtist().IsEmpty())
          msg+=m_pXbmcHttp->GetOpenTag()+"AudioArtist:"+tagVal->GetArtist()+m_pXbmcHttp->GetCloseTag();
        if (m_prevMedia!=msg)
        {
          getApplicationMessenger().HttpApi("broadcastlevel; MediaChanged:"+msg+";1");
          m_prevMedia=msg;
        }
      }
    }
  }
#endif
}

bool CApplication::ProcessHTTPApiButtons()
{
#ifdef HAS_WEB_SERVER
  if (m_pXbmcHttp)
  {
    // copy key from webserver, and reset it in case we're called again before
    // whatever happens in OnKey()
    CKey keyHttp(m_pXbmcHttp->GetKey());
    m_pXbmcHttp->ResetKey();
    if (keyHttp.GetButtonCode() != KEY_INVALID)
    {
      if (keyHttp.GetButtonCode() == KEY_VMOUSE) //virtual mouse
      {
        CAction action;
        action.wID = ACTION_MOUSE;
        g_Mouse.SetLocation(CPoint(keyHttp.GetLeftThumbX(), keyHttp.GetLeftThumbY()));
        if (keyHttp.GetLeftTrigger()!=0)
          g_Mouse.bClick[keyHttp.GetLeftTrigger()-1]=true;
        if (keyHttp.GetRightTrigger()!=0)
          g_Mouse.bDoubleClick[keyHttp.GetRightTrigger()-1]=true;
        action.fAmount1 = keyHttp.GetLeftThumbX();
        action.fAmount2 = keyHttp.GetLeftThumbY();
        m_gWindowManager.OnAction(action);
      }
      else
        OnKey(keyHttp);
      return true;
    }
  }
  return false;
#endif
}

bool CApplication::ProcessEventServer(float frameTime)
{
#ifdef HAS_EVENT_SERVER
  CEventServer* es = CEventServer::GetInstance();
  if (!es || !es->Running() || es->GetNumberOfClients()==0)
    return false;

  // process any queued up actions
  if (es->ExecuteNextAction())
  {
    // reset idle timers
    m_idleTimer.StartZero();
    ResetScreenSaver();
    WakeUpScreenSaverAndDPMS();
  }

  // now handle any buttons or axis
  std::string joystickName;
  bool isAxis = false;
  float fAmount = 0.0;

  WORD wKeyID = es->GetButtonCode(joystickName, isAxis, fAmount);

  if (wKeyID)
  {
    if (joystickName.length() > 0)
    {
      if (isAxis == true)
      {
        if (fabs(fAmount) >= 0.08)
          m_lastAxisMap[joystickName][wKeyID] = fAmount;
        else
          m_lastAxisMap[joystickName].erase(wKeyID);
      }

      return ProcessJoystickEvent(joystickName, wKeyID, isAxis, fAmount);
    }
    else
    {
      CKey key;
      if(wKeyID == KEY_BUTTON_LEFT_ANALOG_TRIGGER)
        key = CKey(wKeyID, (BYTE)(255*fAmount), 0, 0.0, 0.0, 0.0, 0.0, frameTime);
      else if(wKeyID == KEY_BUTTON_RIGHT_ANALOG_TRIGGER)
        key = CKey(wKeyID, 0, (BYTE)(255*fAmount), 0.0, 0.0, 0.0, 0.0, frameTime);
      else if(wKeyID == KEY_BUTTON_LEFT_THUMB_STICK_LEFT)
        key = CKey(wKeyID, 0, 0, -fAmount, 0.0, 0.0, 0.0, frameTime);
      else if(wKeyID == KEY_BUTTON_LEFT_THUMB_STICK_RIGHT)
        key = CKey(wKeyID, 0, 0,  fAmount, 0.0, 0.0, 0.0, frameTime);
      else if(wKeyID == KEY_BUTTON_LEFT_THUMB_STICK_UP)
        key = CKey(wKeyID, 0, 0, 0.0,  fAmount, 0.0, 0.0, frameTime);
      else if(wKeyID == KEY_BUTTON_LEFT_THUMB_STICK_DOWN)
        key = CKey(wKeyID, 0, 0, 0.0, -fAmount, 0.0, 0.0, frameTime);
      else if(wKeyID == KEY_BUTTON_RIGHT_THUMB_STICK_LEFT)
        key = CKey(wKeyID, 0, 0, 0.0, 0.0, -fAmount, 0.0, frameTime);
      else if(wKeyID == KEY_BUTTON_RIGHT_THUMB_STICK_RIGHT)
        key = CKey(wKeyID, 0, 0, 0.0, 0.0,  fAmount, 0.0, frameTime);
      else if(wKeyID == KEY_BUTTON_RIGHT_THUMB_STICK_UP)
        key = CKey(wKeyID, 0, 0, 0.0, 0.0, 0.0,  fAmount, frameTime);
      else if(wKeyID == KEY_BUTTON_RIGHT_THUMB_STICK_DOWN)
        key = CKey(wKeyID, 0, 0, 0.0, 0.0, 0.0, -fAmount, frameTime);
      else
        key = CKey(wKeyID);
      return OnKey(key);
    }
  }

  if (m_lastAxisMap.size() > 0)
  {
    // Process all the stored axis.
    for (map<std::string, map<int, float> >::iterator iter = m_lastAxisMap.begin(); iter != m_lastAxisMap.end(); ++iter)
    {
      for (map<int, float>::iterator iterAxis = (*iter).second.begin(); iterAxis != (*iter).second.end(); ++iterAxis)
        ProcessJoystickEvent((*iter).first, (*iterAxis).first, true, (*iterAxis).second);
    }
  }

  {
    CAction action;
    action.wID = ACTION_MOUSE;
    if (es->GetMousePos(action.fAmount1, action.fAmount2) && g_Mouse.IsEnabled())
    {
      CPoint point;
      point.x = action.fAmount1;
      point.y = action.fAmount2;
      g_Mouse.SetLocation(point, true);

      return m_gWindowManager.OnAction(action);
    }
  }
#endif
  return false;
}

bool CApplication::ProcessJoystickEvent(const std::string& joystickName, int wKeyID, bool isAxis, float fAmount)
{
#if defined(HAS_EVENT_SERVER) && defined(HAS_SDL_JOYSTICK)
  m_idleTimer.StartZero();

   // Make sure to reset screen saver, mouse.
   ResetScreenSaver();
   if (WakeUpScreenSaverAndDPMS())
     return true;

#ifdef HAS_SDL_JOYSTICK
   g_Joystick.Reset();
#endif
   g_Mouse.SetInactive();

   // Figure out what window we're taking the event for.
   WORD iWin = m_gWindowManager.GetActiveWindow() & WINDOW_ID_MASK;
   if (m_gWindowManager.HasModalDialog())
       iWin = m_gWindowManager.GetTopMostModalDialogID() & WINDOW_ID_MASK;

   // This code is copied from the OnKey handler, it should be factored out.
   if (iWin == WINDOW_FULLSCREEN_VIDEO &&
       g_application.m_pPlayer &&
       g_application.m_pPlayer->IsInMenu())
   {
     // If player is in some sort of menu, (ie DVDMENU) map buttons differently.
     iWin = WINDOW_VIDEO_MENU;
   }

   bool fullRange = false;
   CAction action;
   action.fAmount1 = fAmount;

   //if (action.fAmount1 < 0.0)
   // wKeyID = -wKeyID;

   // Translate using regular joystick translator.
   if (CButtonTranslator::GetInstance().TranslateJoystickString(iWin, joystickName.c_str(), wKeyID, isAxis ? JACTIVE_AXIS : JACTIVE_BUTTON, action.wID, action.strAction, fullRange))
   {
     action.fRepeat = 0.0f;
     g_audioManager.PlayActionSound(action);
     return OnAction(action);
   }
   else
   {
     CLog::Log(LOGDEBUG, "ERROR mapping joystick action. Joystick: %s %i",joystickName.c_str(), wKeyID);
   }
#endif

   return false;
}

bool CApplication::ProcessKeyboard()
{
  MEASURE_FUNCTION;

  // process the keyboard buttons etc.
  BYTE vkey = g_Keyboard.GetKey();
  WCHAR unicode = g_Keyboard.GetUnicode();
  if (vkey || unicode)
  {
    // got a valid keypress - convert to a key code
    WORD wkeyID;
    if (vkey) // FIXME, every ascii has a vkey so vkey would always and ascii would never be processed, but fortunately OnKey uses wkeyID only to detect keyboard use and the real key is recalculated correctly.
      wkeyID = (WORD)vkey | KEY_VKEY;
    else
      wkeyID = KEY_UNICODE;
    //  CLog::Log(LOGDEBUG,"Keyboard: time=%i key=%i", timeGetTime(), vkey);
    CKey key(wkeyID);
    key.SetHeld(g_Keyboard.KeyHeld());
    return OnKey(key);
  }
  return false;
}

HRESULT CApplication::Cleanup()
{
  try
  {
    m_gWindowManager.Delete(WINDOW_MUSIC_PLAYLIST);
    m_gWindowManager.Delete(WINDOW_MUSIC_PLAYLIST_EDITOR);
    m_gWindowManager.Delete(WINDOW_MUSIC_FILES);
	m_gWindowManager.Delete(WINDOW_TV);
    m_gWindowManager.Delete(WINDOW_MUSIC_NAV);
    m_gWindowManager.Delete(WINDOW_MUSIC_INFO);
    m_gWindowManager.Delete(WINDOW_VIDEO_INFO);
    m_gWindowManager.Delete(WINDOW_VIDEO_FILES);
    m_gWindowManager.Delete(WINDOW_VIDEO_PLAYLIST);
    m_gWindowManager.Delete(WINDOW_VIDEO_NAV);
    m_gWindowManager.Delete(WINDOW_FILES);
    m_gWindowManager.Delete(WINDOW_MUSIC_INFO);
    m_gWindowManager.Delete(WINDOW_VIDEO_INFO);
    m_gWindowManager.Delete(WINDOW_DIALOG_YES_NO);
    m_gWindowManager.Delete(WINDOW_DIALOG_PROGRESS);
    m_gWindowManager.Delete(WINDOW_DIALOG_NUMERIC);
    m_gWindowManager.Delete(WINDOW_DIALOG_GAMEPAD);
    m_gWindowManager.Delete(WINDOW_DIALOG_SUB_MENU);
    m_gWindowManager.Delete(WINDOW_DIALOG_BUTTON_MENU);
    m_gWindowManager.Delete(WINDOW_DIALOG_CONTEXT_MENU);
    m_gWindowManager.Delete(WINDOW_DIALOG_MUSIC_SCAN);
    m_gWindowManager.Delete(WINDOW_DIALOG_PLAYER_CONTROLS);
    m_gWindowManager.Delete(WINDOW_DIALOG_KARAOKE_SONGSELECT);
    m_gWindowManager.Delete(WINDOW_DIALOG_KARAOKE_SELECTOR);
    m_gWindowManager.Delete(WINDOW_DIALOG_MUSIC_OSD);
    m_gWindowManager.Delete(WINDOW_DIALOG_VIS_SETTINGS);
    m_gWindowManager.Delete(WINDOW_DIALOG_VIS_PRESET_LIST);
    m_gWindowManager.Delete(WINDOW_DIALOG_SELECT);
    m_gWindowManager.Delete(WINDOW_DIALOG_OK);
    m_gWindowManager.Delete(WINDOW_DIALOG_FILESTACKING);
    m_gWindowManager.Delete(WINDOW_DIALOG_KEYBOARD);
    m_gWindowManager.Delete(WINDOW_FULLSCREEN_VIDEO);
    m_gWindowManager.Delete(WINDOW_DIALOG_PROFILE_SETTINGS);
    m_gWindowManager.Delete(WINDOW_DIALOG_LOCK_SETTINGS);
    m_gWindowManager.Delete(WINDOW_DIALOG_NETWORK_SETUP);
    m_gWindowManager.Delete(WINDOW_DIALOG_MEDIA_SOURCE);
    m_gWindowManager.Delete(WINDOW_DIALOG_VIDEO_OSD_SETTINGS);
    m_gWindowManager.Delete(WINDOW_DIALOG_AUDIO_OSD_SETTINGS);
    m_gWindowManager.Delete(WINDOW_DIALOG_VIDEO_BOOKMARKS);
    m_gWindowManager.Delete(WINDOW_DIALOG_VIDEO_SCAN);
    m_gWindowManager.Delete(WINDOW_DIALOG_CONTENT_SETTINGS);
    m_gWindowManager.Delete(WINDOW_DIALOG_FAVOURITES);
    m_gWindowManager.Delete(WINDOW_DIALOG_SONG_INFO);
    m_gWindowManager.Delete(WINDOW_DIALOG_SMART_PLAYLIST_EDITOR);
    m_gWindowManager.Delete(WINDOW_DIALOG_SMART_PLAYLIST_RULE);
    m_gWindowManager.Delete(WINDOW_DIALOG_BUSY);
    m_gWindowManager.Delete(WINDOW_DIALOG_PICTURE_INFO);
    m_gWindowManager.Delete(WINDOW_DIALOG_ADDON_SETTINGS);
    m_gWindowManager.Delete(WINDOW_DIALOG_ACCESS_POINTS);
    m_gWindowManager.Delete(WINDOW_DIALOG_SLIDER);

    m_gWindowManager.Delete(WINDOW_DIALOG_PVRCLIENT_SETTINGS);
    m_gWindowManager.Delete(WINDOW_DIALOG_TV_GUIDE_INFO);
    m_gWindowManager.Delete(WINDOW_DIALOG_TV_RECORDING_INFO);
    m_gWindowManager.Delete(WINDOW_DIALOG_TV_TIMER_SETTING);
    m_gWindowManager.Delete(WINDOW_DIALOG_TV_GROUP_MANAGER);
    m_gWindowManager.Delete(WINDOW_DIALOG_TV_CHANNEL_MANAGER);
    m_gWindowManager.Delete(WINDOW_DIALOG_TV_OSD_CHANNELS);
    m_gWindowManager.Delete(WINDOW_DIALOG_TV_OSD_GUIDE);
    m_gWindowManager.Delete(WINDOW_DIALOG_TV_OSD_TELETEXT);
    m_gWindowManager.Delete(WINDOW_DIALOG_TV_OSD_DIRECTOR);

    m_gWindowManager.Delete(WINDOW_STARTUP);
    m_gWindowManager.Delete(WINDOW_LOGIN_SCREEN);
    m_gWindowManager.Delete(WINDOW_VISUALISATION);
    m_gWindowManager.Delete(WINDOW_KARAOKELYRICS);
    m_gWindowManager.Delete(WINDOW_SETTINGS_MENU);
    m_gWindowManager.Delete(WINDOW_SETTINGS_PROFILES);
    m_gWindowManager.Delete(WINDOW_SETTINGS_MYPICTURES);  // all the settings categories
    m_gWindowManager.Delete(WINDOW_TEST_PATTERN);
    m_gWindowManager.Delete(WINDOW_SCREEN_CALIBRATION);
    m_gWindowManager.Delete(WINDOW_SYSTEM_INFORMATION);
    m_gWindowManager.Delete(WINDOW_SCREENSAVER);
    m_gWindowManager.Delete(WINDOW_OSD);
    m_gWindowManager.Delete(WINDOW_MUSIC_OVERLAY);
    m_gWindowManager.Delete(WINDOW_VIDEO_OVERLAY);
    m_gWindowManager.Delete(WINDOW_SCRIPTS_INFO);
    m_gWindowManager.Delete(WINDOW_SLIDESHOW);
    m_gWindowManager.Delete(WINDOW_DIALOG_EPG_SCAN);

    m_gWindowManager.Delete(WINDOW_HOME);
    m_gWindowManager.Delete(WINDOW_PROGRAMS);
    m_gWindowManager.Delete(WINDOW_PICTURES);
    m_gWindowManager.Delete(WINDOW_SCRIPTS);
    m_gWindowManager.Delete(WINDOW_WEATHER);

    m_gWindowManager.Delete(WINDOW_SETTINGS_MYPICTURES);
    m_gWindowManager.Remove(WINDOW_SETTINGS_MYPROGRAMS);
    m_gWindowManager.Remove(WINDOW_SETTINGS_MYWEATHER);
    m_gWindowManager.Remove(WINDOW_SETTINGS_MYMUSIC);
    m_gWindowManager.Remove(WINDOW_SETTINGS_SYSTEM);
    m_gWindowManager.Remove(WINDOW_SETTINGS_MYVIDEOS);
    m_gWindowManager.Remove(WINDOW_SETTINGS_NETWORK);
    m_gWindowManager.Remove(WINDOW_SETTINGS_APPEARANCE);
    m_gWindowManager.Remove(WINDOW_SETTINGS_MYTV);
    m_gWindowManager.Remove(WINDOW_DIALOG_KAI_TOAST);

    m_gWindowManager.Remove(WINDOW_DIALOG_SEEK_BAR);
    m_gWindowManager.Remove(WINDOW_DIALOG_VOLUME_BAR);

    CLog::Log(LOGNOTICE, "unload sections");
    CSectionLoader::UnloadAll();

#ifdef HAS_PERFORMANCE_SAMPLE
    CLog::Log(LOGNOTICE, "performance statistics");
    m_perfStats.DumpStats();
#endif

  // reset our d3d params before we destroy
#ifndef HAS_SDL
    g_graphicsContext.SetD3DDevice(NULL);
    g_graphicsContext.SetD3DParameters(NULL);
#endif

    //  Shutdown as much as possible of the
    //  application, to reduce the leaks dumped
    //  to the vc output window before calling
    //  _CrtDumpMemoryLeaks(). Most of the leaks
    //  shown are no real leaks, as parts of the app
    //  are still allocated.

    g_localizeStrings.Clear();
    g_LangCodeExpander.Clear();
    g_charsetConverter.clear();
    g_directoryCache.Clear();
    CButtonTranslator::GetInstance().Clear();
    CLastfmScrobbler::RemoveInstance();
    CLibrefmScrobbler::RemoveInstance();
    CLastFmManager::RemoveInstance();
	//StopPVRManager();
#ifdef HAS_EVENT_SERVER
    CEventServer::RemoveInstance();
#endif
#ifdef HAS_DBUS_SERVER
    CDbusServer::RemoveInstance();
#endif
    DllLoaderContainer::Clear();
    g_playlistPlayer.Clear();
    g_settings.Clear();
    g_guiSettings.Clear();
    g_Mouse.Cleanup();

#ifdef _LINUX
    CXHandle::DumpObjectTracker();
#endif

#ifdef _CRTDBG_MAP_ALLOC
    _CrtDumpMemoryLeaks();
    while(1); // execution ends
#endif
    return S_OK;
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "Exception in CApplication::Cleanup()");
    return E_FAIL;
  }
}

void CApplication::Stop()
{
  try
  {
#ifdef HAS_WEB_SERVER
    if (m_pXbmcHttp)
    {
      if (g_stSettings.m_HttpApiBroadcastLevel >= 1)
        getApplicationMessenger().HttpApi("broadcastlevel; ShutDown;1");

      m_pXbmcHttp->shuttingDown = true;
    }
#endif

#ifdef _WIN32PC
    CWIN32Util::SystemParams::SetDefaults( m_SSysParam );
#endif

    CLog::Log(LOGNOTICE, "Storing total System Uptime");
    g_stSettings.m_iSystemTimeTotalUp = g_stSettings.m_iSystemTimeTotalUp + (int)(timeGetTime() / 60000);

    // Update the settings information (volume, uptime etc. need saving)
    if (CFile::Exists(g_settings.GetSettingsFile()))
    {
      CLog::Log(LOGNOTICE, "Saving settings");
      g_settings.Save();
    }
    else
      CLog::Log(LOGNOTICE, "Not saving settings (settings.xml is not present)");

    m_bStop = true;
    CLog::Log(LOGNOTICE, "stop all");

    // stop scanning before we kill the network and so on
    CGUIDialogMusicScan *musicScan = (CGUIDialogMusicScan *)m_gWindowManager.GetWindow(WINDOW_DIALOG_MUSIC_SCAN);
    if (musicScan)
      musicScan->StopScanning();

    CGUIDialogVideoScan *videoScan = (CGUIDialogVideoScan *)m_gWindowManager.GetWindow(WINDOW_DIALOG_VIDEO_SCAN);
    if (videoScan)
      videoScan->StopScanning();

    StopServices();
    //Sleep(5000);

#ifdef __APPLE__
    g_xbmcHelper.ReleaseAllInput();
#endif

    if (m_pPlayer)
    {
      CLog::Log(LOGNOTICE, "stop player");
      delete m_pPlayer;
      m_pPlayer = NULL;
    }

#if HAS_FILESYTEM_DAAP
    CLog::Log(LOGNOTICE, "stop daap clients");
    g_DaapClient.Release();
#endif
#ifdef HAS_FILESYSTEM_SAP
    CLog::Log(LOGNOTICE, "stop sap announcement listener");
    g_sapsessions.StopThread();
#endif
#ifdef HAS_ZEROCONF
    if(CZeroconfBrowser::IsInstantiated())
    {
      CLog::Log(LOGNOTICE, "stop zeroconf browser");
      CZeroconfBrowser::GetInstance()->Stop();
      CZeroconfBrowser::ReleaseInstance();
    }
#endif
    m_applicationMessenger.Cleanup();

    CLog::Log(LOGNOTICE, "clean cached files!");
    g_RarManager.ClearCache(true);

    CLog::Log(LOGNOTICE, "unload skin");
    UnloadSkin();

#ifdef __APPLE__
    if (g_xbmcHelper.IsAlwaysOn() == false)
      g_xbmcHelper.Stop();
#endif

/* Python resource freeing must be done after skin has been unloaded, not before
   some windows still need it when deinitializing during skin unloading. */
#ifdef HAS_PYTHON
  CLog::Log(LOGNOTICE, "stop python");
  g_pythonParser.FreeResources();
#endif
#ifdef HAS_LCD
    if (g_lcd)
    {
      g_lcd->Stop();
      delete g_lcd;
      g_lcd=NULL;
    }
#endif

#ifdef HAS_HAL
    g_HalManager.Stop();
#endif

    CLog::Log(LOGNOTICE, "stopped");
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "Exception in CApplication::Stop()");
  }

  // we may not get to finish the run cycle but exit immediately after a call to g_application.Stop()
  // so we may never get to Destroy() in CXBApplicationEx::Run(), we call it here.
  Destroy();
}

bool CApplication::PlayMedia(const CFileItem& item, int iPlaylist)
{
  if (item.IsLastFM())
  {
    g_partyModeManager.Disable();
    return CLastFmManager::GetInstance()->ChangeStation(item.GetAsUrl());
  }
  if (item.IsSmartPlayList())
  {
    CDirectory dir;
    CFileItemList items;
    if (dir.GetDirectory(item.m_strPath, items) && items.Size())
    {
      CSmartPlaylist smartpl;
      //get name and type of smartplaylist, this will always succeed as GetDirectory also did this.
      smartpl.OpenAndReadName(item.m_strPath);
      CPlayList playlist;
      playlist.Add(items);
      return ProcessAndStartPlaylist(smartpl.GetName(), playlist, (smartpl.GetType() == "songs" || smartpl.GetType() == "albums") ? PLAYLIST_MUSIC:PLAYLIST_VIDEO);
    }
  }
  else if (item.IsPlayList() || item.IsInternetStream())
  {
    CDlgCache* dlgCache = new CDlgCache(5000, g_localizeStrings.Get(10214), item.GetLabel());

    //is or could be a playlist
    auto_ptr<CPlayList> pPlayList (CPlayListFactory::Create(item));
    bool gotPlayList = (pPlayList.get() && pPlayList->Load(item.m_strPath));

    if (dlgCache)
    {
       dlgCache->Close();
       if (dlgCache->IsCanceled())
          return true;
    }

    if (gotPlayList)
    {

      if (iPlaylist != PLAYLIST_NONE)
        return ProcessAndStartPlaylist(item.m_strPath, *pPlayList, iPlaylist);
      else
      {
        CLog::Log(LOGWARNING, "CApplication::PlayMedia called to play a playlist %s but no idea which playlist to use, playing first item", item.m_strPath.c_str());
        if(pPlayList->size())
          return PlayFile(*(*pPlayList)[0], false);
      }
    }
  }

  //nothing special just play
  return PlayFile(item, false);
}

// PlayStack()
// For playing a multi-file video.  Particularly inefficient
// on startup, as we are required to calculate the length
// of each video, so we open + close each one in turn.
// A faster calculation of video time would improve this
// substantially.
bool CApplication::PlayStack(const CFileItem& item, bool bRestart)
{
  if (!item.IsStack())
    return false;

  // see if we have the info in the database
  // TODO: If user changes the time speed (FPS via framerate conversion stuff)
  //       then these times will be wrong.
  //       Also, this is really just a hack for the slow load up times we have
  //       A much better solution is a fast reader of FPS and fileLength
  //       that we can use on a file to get it's time.
  vector<long> times;
  bool haveTimes(false);
  CVideoDatabase dbs;
  if (dbs.Open())
  {
    dbs.GetVideoSettings(item.m_strPath, g_stSettings.m_currentVideoSettings);
    haveTimes = dbs.GetStackTimes(item.m_strPath, times);
    dbs.Close();
  }


  // calculate the total time of the stack
  CStackDirectory dir;
  dir.GetDirectory(item.m_strPath, *m_currentStack);
  long totalTime = 0;
  for (int i = 0; i < m_currentStack->Size(); i++)
  {
    if (haveTimes)
      (*m_currentStack)[i]->m_lEndOffset = times[i];
    else
    {
      int duration;
      if (!CDVDFileInfo::GetFileDuration((*m_currentStack)[i]->m_strPath, duration))
      {
        m_currentStack->Clear();
        return false;
      }
      totalTime += duration / 1000;
      (*m_currentStack)[i]->m_lEndOffset = totalTime;
      times.push_back(totalTime);
    }
  }

  double seconds = item.m_lStartOffset / 75.0;

  if (!haveTimes || item.m_lStartOffset == STARTOFFSET_RESUME )
  {  // have our times now, so update the dB
    if (dbs.Open())
    {
      if( !haveTimes )
        dbs.SetStackTimes(item.m_strPath, times);

      if( item.m_lStartOffset == STARTOFFSET_RESUME )
      {
        // can only resume seek here, not dvdstate
        CBookmark bookmark;
        if( dbs.GetResumeBookMark(item.m_strPath, bookmark) )
          seconds = bookmark.timeInSeconds;
        else
          seconds = 0.0f;
      }
      dbs.Close();
    }
  }

  *m_itemCurrentFile = item;
  m_currentStackPosition = 0;
  m_eCurrentPlayer = EPC_NONE; // must be reset on initial play otherwise last player will be used

  if (seconds > 0)
  {
    // work out where to seek to
    for (int i = 0; i < m_currentStack->Size(); i++)
    {
      if (seconds < (*m_currentStack)[i]->m_lEndOffset)
      {
        CFileItem item(*(*m_currentStack)[i]);
        long start = (i > 0) ? (*m_currentStack)[i-1]->m_lEndOffset : 0;
        item.m_lStartOffset = (long)(seconds - start) * 75;
        m_currentStackPosition = i;
        return PlayFile(item, true);
      }
    }
  }

  return PlayFile(*(*m_currentStack)[0], true);
}

bool CApplication::PlayFile(const CFileItem& item, bool bRestart)
{
  if (!bRestart)
  {
    SaveCurrentFileSettings();

    OutputDebugString("new file set audiostream:0\n");
    // Switch to default options
    g_stSettings.m_currentVideoSettings = g_stSettings.m_defaultVideoSettings;
    // see if we have saved options in the database

    m_iPlaySpeed = 1;
    *m_itemCurrentFile = item;
    m_nextPlaylistItem = -1;
    m_currentStackPosition = 0;
    m_currentStack->Clear();
  }

  if (item.IsPlayList())
    return false;

  if (item.IsPlugin())
  { // we modify the item so that it becomes a real URL
    CFileItem item_new;
    if (DIRECTORY::CPluginDirectory::GetPluginResult(item.m_strPath, item_new))
      return PlayFile(item_new, false);
    return false;
  }

  // if we have a stacked set of files, we need to setup our stack routines for
  // "seamless" seeking and total time of the movie etc.
  // will recall with restart set to true
  if (item.IsStack())
    return PlayStack(item, bRestart);

  //Is TuxBox, this should probably be moved to CFileTuxBox
  if(item.IsTuxBox())
  {
    CLog::Log(LOGDEBUG, "%s - TuxBox URL Detected %s",__FUNCTION__, item.m_strPath.c_str());

    if(g_tuxboxService.IsRunning())
      g_tuxboxService.Stop();

    CFileItem item_new;
    if(g_tuxbox.CreateNewItem(item, item_new))
    {

      // Make sure it doesn't have a player
      // so we actually select one normally
      m_eCurrentPlayer = EPC_NONE;

      // keep the tuxbox:// url as playing url
      // and give the new url to the player
      if(PlayFile(item_new, true))
      {
        if(!g_tuxboxService.IsRunning())
          g_tuxboxService.Start();
        return true;
      }
    }
    return false;
  }

  CPlayerOptions options;
  PLAYERCOREID eNewCore = EPC_NONE;
  if( bRestart )
  {
    // have to be set here due to playstack using this for starting the file
    options.starttime = item.m_lStartOffset / 75.0;
    if (m_itemCurrentFile->IsStack() && m_currentStack->Size() > 0 && m_itemCurrentFile->m_lStartOffset != 0)
      m_itemCurrentFile->m_lStartOffset = STARTOFFSET_RESUME; // to force fullscreen switching

    if( m_eForcedNextPlayer != EPC_NONE )
      eNewCore = m_eForcedNextPlayer;
    else if( m_eCurrentPlayer == EPC_NONE )
      eNewCore = CPlayerCoreFactory::GetDefaultPlayer(item);
    else
      eNewCore = m_eCurrentPlayer;
  }
  else
  {
    options.starttime = item.m_lStartOffset / 75.0;

    if (item.IsVideo())
    {
      // open the d/b and retrieve the bookmarks for the current movie
      CVideoDatabase dbs;
      dbs.Open();
      dbs.GetVideoSettings(item.m_strPath, g_stSettings.m_currentVideoSettings);

      if( item.m_lStartOffset == STARTOFFSET_RESUME )
      {
        options.starttime = 0.0f;
        CBookmark bookmark;
        if(dbs.GetResumeBookMark(item.m_strPath, bookmark))
        {
          options.starttime = bookmark.timeInSeconds;
          options.state = bookmark.playerState;
        }
      }
      else if (item.HasVideoInfoTag())
      {
        const CVideoInfoTag *tag = item.GetVideoInfoTag();

        if (tag->m_iBookmarkId != -1 && tag->m_iBookmarkId != 0)
        {
          CBookmark bookmark;
          dbs.GetBookMarkForEpisode(*tag, bookmark);
          options.starttime = bookmark.timeInSeconds;
          options.state = bookmark.playerState;
        }
      }

      dbs.Close();
    }

    if (m_eForcedNextPlayer != EPC_NONE)
      eNewCore = m_eForcedNextPlayer;
    else
      eNewCore = CPlayerCoreFactory::GetDefaultPlayer(item);
  }

  // this really aught to be inside !bRestart, but since PlayStack
  // uses that to init playback, we have to keep it outside
  int playlist = g_playlistPlayer.GetCurrentPlaylist();
  if (playlist == PLAYLIST_VIDEO && g_playlistPlayer.GetPlaylist(playlist).size() > 1)
  { // playing from a playlist by the looks
    // don't switch to fullscreen if we are not playing the first item...
    options.fullscreen = !g_playlistPlayer.HasPlayedFirstFile() && g_advancedSettings.m_fullScreenOnMovieStart && !g_stSettings.m_bStartVideoWindowed;
  }
  else if(m_itemCurrentFile->IsStack() && m_currentStack->Size() > 0)
  {
    // TODO - this will fail if user seeks back to first file in stack
    if(m_currentStackPosition == 0 || m_itemCurrentFile->m_lStartOffset == STARTOFFSET_RESUME)
      options.fullscreen = g_advancedSettings.m_fullScreenOnMovieStart && !g_stSettings.m_bStartVideoWindowed;
    else
      options.fullscreen = false;
    // reset this so we don't think we are resuming on seek
    m_itemCurrentFile->m_lStartOffset = 0;
  }
  else
    options.fullscreen = g_advancedSettings.m_fullScreenOnMovieStart && !g_stSettings.m_bStartVideoWindowed;

  // reset m_bStartVideoWindowed as it's a temp setting
  g_stSettings.m_bStartVideoWindowed = false;
  // reset any forced player
  m_eForcedNextPlayer = EPC_NONE;

#ifdef HAS_KARAOKE
  //We have to stop parsing a cdg before mplayer is deallocated
  // WHY do we have to do this????
  if (m_pKaraokeMgr)
    m_pKaraokeMgr->Stop();
#endif

  // tell system we are starting a file
  m_bPlaybackStarting = true;

  // We should restart the player, unless the previous and next tracks are using
  // one of the players that allows gapless playback (paplayer, dvdplayer)
  if (m_pPlayer)
  {
    if ( !(m_eCurrentPlayer == eNewCore && (m_eCurrentPlayer == EPC_DVDPLAYER || m_eCurrentPlayer  == EPC_PAPLAYER)) )
    {
      delete m_pPlayer;
      m_pPlayer = NULL;
    }
  }

  if (!m_pPlayer)
  {
    m_eCurrentPlayer = eNewCore;
    m_pPlayer = CPlayerCoreFactory::CreatePlayer(eNewCore, *this);
  }

  // Workaround for bug/quirk in SDL_Mixer on OSX.
  // TODO: Remove after GUI Sounds redux
#if defined(__APPLE__)
  if (!g_guiSettings.GetBool("lookandfeel.soundsduringplayback"))
    g_audioManager.Enable(false);
#endif

  bool bResult;
  if (m_pPlayer)
    bResult = m_pPlayer->OpenFile(item, options);
  else
  {
    CLog::Log(LOGERROR, "Error creating player for item %s (File doesn't exist?)", item.m_strPath.c_str());
    bResult = false;
  }

  if(bResult)
  {
    if (m_iPlaySpeed != 1)
    {
      int iSpeed = m_iPlaySpeed;
      m_iPlaySpeed = 1;
      SetPlaySpeed(iSpeed);
    }

#ifdef HAS_VIDEO_PLAYBACK
    if( IsPlayingVideo() )
    {
      // if player didn't manange to switch to fullscreen by itself do it here
      if( options.fullscreen && g_renderManager.IsStarted()
       && m_gWindowManager.GetActiveWindow() != WINDOW_FULLSCREEN_VIDEO )
       SwitchToFullScreen();

      // Save information about the stream if we currently have no data
      if (item.HasVideoInfoTag())
      {
        CVideoInfoTag *details = m_itemCurrentFile->GetVideoInfoTag();
        if (!details->HasStreamDetails())
        {
          if (m_pPlayer->GetStreamDetails(details->m_streamDetails) && details->HasStreamDetails())
          {
            CVideoDatabase dbs;
            dbs.Open();
            dbs.SetStreamDetailsForFileId(details->m_streamDetails, details->m_iFileId);
            dbs.Close();
            CUtil::DeleteVideoDatabaseDirectoryCache();
          }
        }
      }
    }
#endif

#if !defined(__APPLE__)
    if (!g_guiSettings.GetBool("lookandfeel.soundsduringplayback"))
      g_audioManager.Enable(false);
#endif
  }
  m_bPlaybackStarting = false;
  if(bResult)
  {
    // we must have started, otherwise player might send this later
    if(IsPlaying())
      OnPlayBackStarted();
  }
  else
  {
    // we send this if it isn't playlistplayer that is doing this
    int next = g_playlistPlayer.GetNextSong();
    int size = g_playlistPlayer.GetPlaylist(g_playlistPlayer.GetCurrentPlaylist()).size();
    if(next < 0
    || next >= size)
      OnPlayBackStopped();
  }

  return bResult;
}

void CApplication::OnPlayBackEnded()
{
  if(m_bPlaybackStarting)
    return;

  // informs python script currently running playback has ended
  // (does nothing if python is not loaded)
#ifdef HAS_PYTHON
  g_pythonParser.OnPlayBackEnded();
#endif

#ifdef HAS_WEB_SERVER
  // Let's tell the outside world as well
  if (m_pXbmcHttp && g_stSettings.m_HttpApiBroadcastLevel>=1)
    getApplicationMessenger().HttpApi("broadcastlevel; OnPlayBackEnded;1");
#endif
  if (IsPlayingAudio())
  {
    CLastfmScrobbler::GetInstance()->SubmitQueue();
    CLibrefmScrobbler::GetInstance()->SubmitQueue();
  }

  CLog::Log(LOGDEBUG, "%s - Playback has finished", __FUNCTION__);

  CGUIMessage msg(GUI_MSG_PLAYBACK_ENDED, 0, 0);
  m_gWindowManager.SendThreadMessage(msg);
}

void CApplication::OnPlayBackStarted()
{
  if(m_bPlaybackStarting)
    return;

#ifdef HAS_PYTHON
  // informs python script currently running playback has started
  // (does nothing if python is not loaded)
  g_pythonParser.OnPlayBackStarted();
#endif

#ifdef HAS_WEB_SERVER
  // Let's tell the outside world as well
  if (m_pXbmcHttp && g_stSettings.m_HttpApiBroadcastLevel>=1)
    getApplicationMessenger().HttpApi("broadcastlevel; OnPlayBackStarted;1");
#endif

  CLog::Log(LOGDEBUG, "%s - Playback has started", __FUNCTION__);

  CGUIMessage msg(GUI_MSG_PLAYBACK_STARTED, 0, 0);
  m_gWindowManager.SendThreadMessage(msg);
}

void CApplication::OnQueueNextItem()
{
  // informs python script currently running that we are requesting the next track
  // (does nothing if python is not loaded)
#ifdef HAS_PYTHON
  g_pythonParser.OnQueueNextItem(); // currently unimplemented
#endif

#ifdef HAS_WEB_SERVER
  // Let's tell the outside world as well
  if (m_pXbmcHttp && g_stSettings.m_HttpApiBroadcastLevel>=1)
    getApplicationMessenger().HttpApi("broadcastlevel; OnQueueNextItem;1");
#endif
  CLog::Log(LOGDEBUG, "Player has asked for the next item");

  if(IsPlayingAudio())
  {
    CLastfmScrobbler::GetInstance()->SubmitQueue();
    CLibrefmScrobbler::GetInstance()->SubmitQueue();
  }

  CGUIMessage msg(GUI_MSG_QUEUE_NEXT_ITEM, 0, 0);
  m_gWindowManager.SendThreadMessage(msg);
}

void CApplication::OnPlayBackStopped()
{
  if(m_bPlaybackStarting)
    return;

  // informs python script currently running playback has ended
  // (does nothing if python is not loaded)
#ifdef HAS_PYTHON
  g_pythonParser.OnPlayBackStopped();
#endif

#ifdef HAS_WEB_SERVER
  // Let's tell the outside world as well
  if (m_pXbmcHttp && g_stSettings.m_HttpApiBroadcastLevel>=1)
    getApplicationMessenger().HttpApi("broadcastlevel; OnPlayBackStopped;1");
#endif
  CLastfmScrobbler::GetInstance()->SubmitQueue();
  CLibrefmScrobbler::GetInstance()->SubmitQueue();

  CLog::Log(LOGDEBUG, "%s - Playback was stopped", __FUNCTION__);

  CGUIMessage msg( GUI_MSG_PLAYBACK_STOPPED, 0, 0 );
  m_gWindowManager.SendThreadMessage(msg);
}

bool CApplication::IsPlaying() const
{
  if (!m_pPlayer)
    return false;
  if (!m_pPlayer->IsPlaying())
    return false;
  return true;
}

bool CApplication::IsPaused() const
{
  if (!m_pPlayer)
    return false;
  if (!m_pPlayer->IsPlaying())
    return false;
  return m_pPlayer->IsPaused();
}

bool CApplication::IsPlayingAudio() const
{
  if (!m_pPlayer)
    return false;
  if (!m_pPlayer->IsPlaying())
    return false;
  if (m_pPlayer->HasVideo())
    return false;
  if (m_pPlayer->HasAudio())
    return true;
  return false;
}

bool CApplication::IsPlayingVideo() const
{
  if (!m_pPlayer)
    return false;
  if (!m_pPlayer->IsPlaying())
    return false;
  if (m_pPlayer->HasVideo())
    return true;

  return false;
}

bool CApplication::IsPlayingFullScreenVideo() const
{
  return IsPlayingVideo() && g_graphicsContext.IsFullScreenVideo();
}

void CApplication::SaveFileState()
{
  CStdString progressTrackingFile = m_progressTrackingItem->m_strPath;

  if (progressTrackingFile != "")
  {
    if (m_progressTrackingItem->IsVideo())
    {
      CLog::Log(LOGDEBUG, "%s - Saving file state for video file %s", __FUNCTION__, progressTrackingFile.c_str());

      CVideoDatabase videodatabase;
      if (videodatabase.Open())
      {
        if (m_progressTrackingPlayCountUpdate)
        {
          CLog::Log(LOGDEBUG, "%s - Marking video file %s as watched", __FUNCTION__, progressTrackingFile.c_str());

          // consider this item as played
          videodatabase.MarkAsWatched(*m_progressTrackingItem);
          CUtil::DeleteVideoDatabaseDirectoryCache();
          CGUIMessage message(GUI_MSG_NOTIFY_ALL, m_gWindowManager.GetActiveWindow(), 0, GUI_MSG_UPDATE, 0);
          g_graphicsContext.SendMessage(message);
        }

        if (g_stSettings.m_currentVideoSettings != g_stSettings.m_defaultVideoSettings)
        {
          videodatabase.SetVideoSettings(progressTrackingFile, g_stSettings.m_currentVideoSettings);
        }

        if (m_progressTrackingVideoResumeBookmark.timeInSeconds < 0.0f)
        {
          videodatabase.ClearBookMarksOfFile(progressTrackingFile, CBookmark::RESUME);
        }
        else
        if (m_progressTrackingVideoResumeBookmark.timeInSeconds > 0.0f)
        {
          videodatabase.AddBookMarkToFile(progressTrackingFile, m_progressTrackingVideoResumeBookmark, CBookmark::RESUME);
        }

        videodatabase.Close();
      }
    }
    else
    {
      CLog::Log(LOGDEBUG, "%s - Saving file state for audio file %s", __FUNCTION__, progressTrackingFile.c_str());

      if (m_progressTrackingPlayCountUpdate)
      {
        // Can't write to the musicdatabase while scanning for music info
        CGUIDialogMusicScan *dialog = (CGUIDialogMusicScan *)m_gWindowManager.GetWindow(WINDOW_DIALOG_MUSIC_SCAN);
        if (dialog && !dialog->IsDialogRunning())
        {
          // consider this item as played
          CLog::Log(LOGDEBUG, "%s - Marking audio file %s as listened", __FUNCTION__, progressTrackingFile.c_str());

          CMusicDatabase musicdatabase;
          if (musicdatabase.Open())
          {
            musicdatabase.IncrTop100CounterByFileName(progressTrackingFile);
            musicdatabase.Close();
          }
        }
      }
    }
  }
}

void CApplication::UpdateFileState()
{
  // No resume for livetv
  if (m_progressTrackingItem->IsLiveTV())
    return;

  // Did the file change?
  if (m_progressTrackingItem->m_strPath != "" && m_progressTrackingItem->m_strPath != CurrentFile())
  {
    SaveFileState();

    // Reset tracking item
    m_progressTrackingItem->Reset();
  }
  else
  if (IsPlayingVideo() || IsPlayingAudio())
  {
    if (m_progressTrackingItem->m_strPath == "")
    {
      // Init some stuff
      *m_progressTrackingItem = CurrentFileItem();
      m_progressTrackingPlayCountUpdate = false;
    }

    if ((m_progressTrackingItem->IsAudio() && g_advancedSettings.m_audioPlayCountMinimumPercent > 0 &&
        GetPercentage() >= g_advancedSettings.m_audioPlayCountMinimumPercent) ||
        (m_progressTrackingItem->IsVideo() && g_advancedSettings.m_videoPlayCountMinimumPercent > 0 &&
        GetPercentage() >= g_advancedSettings.m_videoPlayCountMinimumPercent))
    {
      m_progressTrackingPlayCountUpdate = true;
    }

    if (m_progressTrackingItem->IsVideo())
    {
      // Update bookmark for save
      m_progressTrackingVideoResumeBookmark.player = CPlayerCoreFactory::GetPlayerName(m_eCurrentPlayer);
      m_progressTrackingVideoResumeBookmark.playerState = m_pPlayer->GetPlayerState();
      m_progressTrackingVideoResumeBookmark.thumbNailImage.Empty();

      if (g_advancedSettings.m_videoIgnoreAtEnd > 0 &&
          GetTotalTime() - GetTime() < g_advancedSettings.m_videoIgnoreAtEnd)
      {
        // Delete the bookmark
        m_progressTrackingVideoResumeBookmark.timeInSeconds = -1.0f;
      }
      else
      if (GetTime() > g_advancedSettings.m_videoIgnoreAtStart)
      {
        // Update the bookmark
        m_progressTrackingVideoResumeBookmark.timeInSeconds = GetTime();
        m_progressTrackingVideoResumeBookmark.totalTimeInSeconds = GetTotalTime();
      }
      else
      {
        // Do nothing
        m_progressTrackingVideoResumeBookmark.timeInSeconds = 0.0f;
      }
    }
  }
}

void CApplication::StopPlaying()
{
  int iWin = m_gWindowManager.GetActiveWindow();
  if ( IsPlaying() )
  {
#ifdef HAS_KARAOKE
    if( m_pKaraokeMgr )
      m_pKaraokeMgr->Stop();
#endif

    // turn off visualisation window when stopping
    if (iWin == WINDOW_VISUALISATION)
      m_gWindowManager.PreviousWindow();

    if (m_pPlayer)
      m_pPlayer->CloseFile();

    g_partyModeManager.Disable();
  }
}

bool CApplication::NeedRenderFullScreen()
{
  if (m_gWindowManager.GetActiveWindow() == WINDOW_FULLSCREEN_VIDEO)
  {
    m_gWindowManager.UpdateModelessVisibility();

    if (m_gWindowManager.HasDialogOnScreen()) return true;
    if (g_Mouse.IsActive()) return true;

    CGUIWindowFullScreen *pFSWin = (CGUIWindowFullScreen *)m_gWindowManager.GetWindow(WINDOW_FULLSCREEN_VIDEO);
    if (!pFSWin)
      return false;
    return pFSWin->NeedRenderFullScreen();
  }
  return false;
}

void CApplication::RenderFullScreen()
{
  MEASURE_FUNCTION;

  g_ApplicationRenderer.Render(true);
}

void CApplication::DoRenderFullScreen()
{
  MEASURE_FUNCTION;

  if (g_graphicsContext.IsFullScreenVideo())
  {
    // make sure our overlays are closed
    CGUIDialog *overlay = (CGUIDialog *)m_gWindowManager.GetWindow(WINDOW_VIDEO_OVERLAY);
    if (overlay) overlay->Close(true);
    overlay = (CGUIDialog *)m_gWindowManager.GetWindow(WINDOW_MUSIC_OVERLAY);
    if (overlay) overlay->Close(true);

    CGUIWindowFullScreen *pFSWin = (CGUIWindowFullScreen *)m_gWindowManager.GetWindow(WINDOW_FULLSCREEN_VIDEO);
    if (!pFSWin)
      return ;
    pFSWin->RenderFullScreen();

    if (m_gWindowManager.HasDialogOnScreen())
      m_gWindowManager.RenderDialogs();
    // Render the mouse pointer, if visible...
    if (g_Mouse.IsActive())
      g_application.m_guiPointer.Render();
  }
}

void CApplication::ResetScreenSaver()
{
  // reset our timers
  m_shutdownTimer.StartZero();

  // screen saver timer is reset only if we're not already in screensaver or
  // DPMS mode
  if ((!m_bScreenSave && m_iScreenSaveLock == 0) && !m_dpmsIsActive)
    ResetScreenSaverTimer();
}

void CApplication::ResetScreenSaverTimer()
{
#ifdef __APPLE__
  Cocoa_UpdateSystemActivity();
#endif
  m_screenSaverTimer.StartZero();
}

bool CApplication::WakeUpScreenSaverAndDPMS()
{
  // First reset DPMS, if active
  if (m_dpmsIsActive)
  {
    // TODO: if screensaver lock is specified but screensaver is not active
    // (DPMS came first), activate screensaver now.
    m_dpms->DisablePowerSaving();
    m_dpmsIsActive = false;
    ResetScreenSaverTimer();
    return !m_bScreenSave || WakeUpScreenSaver();
  }
  else
    return WakeUpScreenSaver();
}

bool CApplication::WakeUpScreenSaver()
{
  if (m_iScreenSaveLock == 2)
    return false;

  // if Screen saver is active
  if (m_bScreenSave)
  {
    int iProfile = g_settings.m_iLastLoadedProfileIndex;
    if (m_iScreenSaveLock == 0)
      if (g_guiSettings.GetBool("screensaver.uselock")                           &&
          g_settings.m_vecProfiles[0].getLockMode() != LOCK_MODE_EVERYONE        &&
          g_settings.m_vecProfiles[iProfile].getLockMode() != LOCK_MODE_EVERYONE &&
         !g_guiSettings.GetString("screensaver.mode").Equals("Black")            &&
        !(g_guiSettings.GetBool("screensaver.usemusicvisinstead")                &&
         !g_guiSettings.GetString("screensaver.mode").Equals("Black")            &&
          g_application.IsPlayingAudio())                                          )
      {
        m_iScreenSaveLock = 2;
        CGUIMessage msg(GUI_MSG_CHECK_LOCK,0,0);
        m_gWindowManager.GetWindow(WINDOW_SCREENSAVER)->OnMessage(msg);
      }
    if (m_iScreenSaveLock == -1)
    {
      m_iScreenSaveLock = 0;
      return true;
    }

    // disable screensaver
    m_bScreenSave = false;
    m_iScreenSaveLock = 0;
    ResetScreenSaverTimer();

    if (m_screenSaverMode == "Visualisation" || m_screenSaverMode == "Slideshow" || m_screenSaverMode == "Fanart Slideshow")
    {
      // we can just continue as usual from vis mode
      return false;
    }
    else if (m_screenSaverMode == "Dim" || m_screenSaverMode == "Black")
      return true;
    else if (m_screenSaverMode != "None")
    { // we're in screensaver window
      if (m_gWindowManager.GetActiveWindow() == WINDOW_SCREENSAVER)
        m_gWindowManager.PreviousWindow();  // show the previous window
    }
    return true;
  }
  else
    return false;
}

void CApplication::CheckScreenSaverAndDPMS()
{
  bool maybeScreensaver =
      !m_dpmsIsActive && !m_bScreenSave
      && g_guiSettings.GetString("screensaver.mode") != "None";
  bool maybeDPMS =
      !m_dpmsIsActive && m_dpms->IsSupported()
      && g_guiSettings.GetInt("screensaver.powersavingtime") > 0;

  // Has the screen saver window become active?
  if (maybeScreensaver && m_gWindowManager.IsWindowActive(WINDOW_SCREENSAVER))
  {
    m_bScreenSave = true;
    maybeScreensaver = false;
  }

  if (!maybeScreensaver && !maybeDPMS) return;  // Nothing to do.

  // See if we need to reset timer.
  // * Are we playing a video and it is not paused?
  if ((IsPlayingVideo() && !m_pPlayer->IsPaused())
      // * Are we playing some music in fullscreen vis?
      || (IsPlayingAudio() && m_gWindowManager.GetActiveWindow() == WINDOW_VISUALISATION))
  {
    ResetScreenSaverTimer();
    return;
  }

  float elapsed = m_screenSaverTimer.GetElapsedSeconds();

  // DPMS has priority (it makes the screensaver not needed)
  if (maybeDPMS
      && elapsed > g_guiSettings.GetInt("screensaver.powersavingtime") * 60)
  {
    m_dpms->EnablePowerSaving(m_dpms->GetSupportedModes()[0]);
    m_dpmsIsActive = true;
    WakeUpScreenSaver();
  }
  else if (maybeScreensaver
           && elapsed > g_guiSettings.GetInt("screensaver.time") * 60)
  {
    ActivateScreenSaver();
  }
}

// activate the screensaver.
// if forceType is true, we ignore the various conditions that can alter
// the type of screensaver displayed
void CApplication::ActivateScreenSaver(bool forceType /*= false */)
{
  m_bScreenSave = true;

  // Get Screensaver Mode
  m_screenSaverMode = g_guiSettings.GetString("screensaver.mode");

  if (!forceType)
  {
    // set to Dim in the case of a dialog on screen or playing video
    if (m_gWindowManager.HasModalDialog() || (IsPlayingVideo() && g_guiSettings.GetBool("screensaver.usedimonpause")))
      m_screenSaverMode = "Dim";
    // Check if we are Playing Audio and Vis instead Screensaver!
    else if (IsPlayingAudio() && g_guiSettings.GetBool("screensaver.usemusicvisinstead") && g_guiSettings.GetString("mymusic.visualisation") != "None")
    { // activate the visualisation
      m_screenSaverMode = "Visualisation";
      m_gWindowManager.ActivateWindow(WINDOW_VISUALISATION);
      return;
    }
  }
  // Picture slideshow
  if (m_screenSaverMode == "SlideShow" || m_screenSaverMode == "Fanart Slideshow")
  {
    // reset our codec info - don't want that on screen
    g_infoManager.SetShowCodec(false);
    m_applicationMessenger.PictureSlideShow(g_guiSettings.GetString("screensaver.slideshowpath"), true);
  }
  else if (m_screenSaverMode == "Dim")
    return;
  else if (m_screenSaverMode == "Black")
    return;
  else if (m_screenSaverMode != "None")
    m_gWindowManager.ActivateWindow(WINDOW_SCREENSAVER);
}

void CApplication::CheckShutdown()
{
  CGUIDialogMusicScan *pMusicScan = (CGUIDialogMusicScan *)m_gWindowManager.GetWindow(WINDOW_DIALOG_MUSIC_SCAN);
  CGUIDialogVideoScan *pVideoScan = (CGUIDialogVideoScan *)m_gWindowManager.GetWindow(WINDOW_DIALOG_VIDEO_SCAN);

  // first check if we should reset the timer
  bool resetTimer = false;
  if (IsPlaying()) // is something playing?
    resetTimer = true;

#ifdef HAS_FTP_SERVER
  if (m_pFileZilla && m_pFileZilla->GetNoConnections() != 0) // is FTP active ?
    resetTimer = true;
#endif

  if (pMusicScan && pMusicScan->IsScanning()) // music scanning?
    resetTimer = true;

  if (pVideoScan && pVideoScan->IsScanning()) // video scanning?
    resetTimer = true;

  if (m_gWindowManager.IsWindowActive(WINDOW_DIALOG_PROGRESS)) // progress dialog is onscreen
    resetTimer = true;

  if (resetTimer)
  {
    m_shutdownTimer.StartZero();
    return;
  }

  if ( m_shutdownTimer.GetElapsedSeconds() > g_guiSettings.GetInt("system.shutdowntime") * 60 )
  {
    // Since it is a sleep instead of a shutdown, let's set everything to reset when we wake up.
    m_shutdownTimer.Stop();

    // Sleep the box
    getApplicationMessenger().Shutdown();
  }
}

bool CApplication::OnMessage(CGUIMessage& message)
{
  switch ( message.GetMessage() )
  {
  case GUI_MSG_NOTIFY_ALL:
    {
      if (message.GetParam1()==GUI_MSG_REMOVED_MEDIA)
      {
        // Update general playlist: Remove DVD playlist items
        int nRemoved = g_playlistPlayer.RemoveDVDItems();
        if ( nRemoved > 0 )
        {
          CGUIMessage msg( GUI_MSG_PLAYLIST_CHANGED, 0, 0 );
          m_gWindowManager.SendMessage( msg );
        }
        // stop the file if it's on dvd (will set the resume point etc)
        if (m_itemCurrentFile->IsOnDVD())
          StopPlaying();
      }
    }
    break;

  case GUI_MSG_PLAYBACK_STARTED:
    {
      // Update our infoManager with the new details etc.
      if (m_nextPlaylistItem >= 0)
      { // we've started a previously queued item
        CFileItemPtr item = g_playlistPlayer.GetPlaylist(g_playlistPlayer.GetCurrentPlaylist())[m_nextPlaylistItem];
        // update the playlist manager
        WORD currentSong = g_playlistPlayer.GetCurrentSong();
        DWORD dwParam = ((currentSong & 0xffff) << 16) | (m_nextPlaylistItem & 0xffff);
        CGUIMessage msg(GUI_MSG_PLAYLISTPLAYER_CHANGED, 0, 0, g_playlistPlayer.GetCurrentPlaylist(), dwParam, item);
        m_gWindowManager.SendThreadMessage(msg);
        g_playlistPlayer.SetCurrentSong(m_nextPlaylistItem);
        *m_itemCurrentFile = *item;
      }
      g_infoManager.SetCurrentItem(*m_itemCurrentFile);
      CLastFmManager::GetInstance()->OnSongChange(*m_itemCurrentFile);
      g_partyModeManager.OnSongChange(true);

      DimLCDOnPlayback(true);

      if (IsPlayingAudio())
      {
        // Start our cdg parser as appropriate
#ifdef HAS_KARAOKE
        if (m_pKaraokeMgr && g_guiSettings.GetBool("karaoke.enabled") && !m_itemCurrentFile->IsInternetStream())
        {
          m_pKaraokeMgr->Stop();
          if (m_itemCurrentFile->IsMusicDb())
          {
            if (!m_itemCurrentFile->HasMusicInfoTag() || !m_itemCurrentFile->GetMusicInfoTag()->Loaded())
            {
              IMusicInfoTagLoader* tagloader = CMusicInfoTagLoaderFactory::CreateLoader(m_itemCurrentFile->m_strPath);
              tagloader->Load(m_itemCurrentFile->m_strPath,*m_itemCurrentFile->GetMusicInfoTag());
              delete tagloader;
            }
            m_pKaraokeMgr->Start(m_itemCurrentFile->GetMusicInfoTag()->GetURL());
          }
          else
            m_pKaraokeMgr->Start(m_itemCurrentFile->m_strPath);
        }
#endif
        // Let scrobbler know about the track
        const CMusicInfoTag* tag=g_infoManager.GetCurrentSongTag();
        if (tag)
        {
          CLastfmScrobbler::GetInstance()->AddSong(*tag, CLastFmManager::GetInstance()->IsRadioEnabled());
          CLibrefmScrobbler::GetInstance()->AddSong(*tag, CLastFmManager::GetInstance()->IsRadioEnabled());
        }
      }
      
      return true;
    }
    break;

  case GUI_MSG_QUEUE_NEXT_ITEM:
    {
      // Check to see if our playlist player has a new item for us,
      // and if so, we check whether our current player wants the file
      int iNext = g_playlistPlayer.GetNextSong();
      CPlayList& playlist = g_playlistPlayer.GetPlaylist(g_playlistPlayer.GetCurrentPlaylist());
      if (iNext < 0 || iNext >= playlist.size())
      {
        if (m_pPlayer) m_pPlayer->OnNothingToQueueNotify();
        return true; // nothing to do
      }
      // ok, grab the next song
      CFileItemPtr item = playlist[iNext];
      // ok - send the file to the player if it wants it
      if (m_pPlayer && m_pPlayer->QueueNextFile(*item))
      { // player wants the next file
        m_nextPlaylistItem = iNext;
      }
      return true;
    }
    break;

  case GUI_MSG_PLAYBACK_STOPPED:
  case GUI_MSG_PLAYBACK_ENDED:
  case GUI_MSG_PLAYLISTPLAYER_STOPPED:
    {
#ifdef HAS_KARAOKE
      if (m_pKaraokeMgr )
        m_pKaraokeMgr->Stop();
#endif

      // first check if we still have items in the stack to play
      if (message.GetMessage() == GUI_MSG_PLAYBACK_ENDED)
      {
        if (m_itemCurrentFile->IsStack() && m_currentStack->Size() > 0 && m_currentStackPosition < m_currentStack->Size() - 1)
        { // just play the next item in the stack
          PlayFile(*(*m_currentStack)[++m_currentStackPosition], true);
          return true;
        }
      }

      // reset the current playing file
      m_itemCurrentFile->Reset();
      g_infoManager.ResetCurrentItem();
      m_currentStack->Clear();

      // stop lastfm
      if (CLastFmManager::GetInstance()->IsRadioEnabled())
        CLastFmManager::GetInstance()->StopRadio();

      if (message.GetMessage() == GUI_MSG_PLAYBACK_ENDED)
      {
        // sending true to PlayNext() effectively passes bRestart to PlayFile()
        // which is not generally what we want (except for stacks, which are
        // handled above)
        g_playlistPlayer.PlayNext();
      }
      else
      {
        if (m_pPlayer)
        {
          delete m_pPlayer;
          m_pPlayer = 0;
        }
      }

      if (!IsPlaying())
      {
        g_audioManager.Enable(true);
        DimLCDOnPlayback(false);
      }

      if (!IsPlayingVideo() && m_gWindowManager.GetActiveWindow() == WINDOW_FULLSCREEN_VIDEO)
      {
        m_gWindowManager.PreviousWindow();
      }

      if (!IsPlayingAudio() && g_playlistPlayer.GetCurrentPlaylist() == PLAYLIST_NONE && m_gWindowManager.GetActiveWindow() == WINDOW_VISUALISATION)
      {
        g_settings.Save();  // save vis settings
        WakeUpScreenSaverAndDPMS();
        m_gWindowManager.PreviousWindow();
      }

      // reset the audio playlist on finish
      if (!IsPlayingAudio() && (g_guiSettings.GetBool("mymusic.clearplaylistsonend")) && (g_playlistPlayer.GetCurrentPlaylist() == PLAYLIST_MUSIC))
      {
        g_playlistPlayer.ClearPlaylist(PLAYLIST_MUSIC);
        g_playlistPlayer.Reset();
        g_playlistPlayer.SetCurrentPlaylist(PLAYLIST_NONE);
      }

      // DVD ejected while playing in vis ?
      if (!IsPlayingAudio() && (m_itemCurrentFile->IsCDDA() || m_itemCurrentFile->IsOnDVD()) && !g_mediaManager.IsDiscInDrive() && m_gWindowManager.GetActiveWindow() == WINDOW_VISUALISATION)
      {
        // yes, disable vis
        g_settings.Save();    // save vis settings
        WakeUpScreenSaverAndDPMS();
        m_gWindowManager.PreviousWindow();
      }
      
      return true;
    }
    break;

  case GUI_MSG_PLAYLISTPLAYER_STARTED:
  case GUI_MSG_PLAYLISTPLAYER_CHANGED:
    {
      return true;
    }
    break;
  case GUI_MSG_FULLSCREEN:
    { // Switch to fullscreen, if we can
      SwitchToFullScreen();
      return true;
    }
    break;
  case GUI_MSG_EXECUTE:
    if (message.GetStringParam().length() > 0)
      return ExecuteXBMCAction(message.GetStringParam());
    else {
      CGUIActionDescriptor action = message.GetAction();
      action.m_sourceWindowId = message.GetControlId(); // set source window id, 
      return ExecuteAction(action);
    }

    break;
  }
  return false;
}

bool CApplication::ExecuteXBMCAction(std::string actionStr)
    {
      // see if it is a user set string
      CLog::Log(LOGDEBUG,"%s : Translating %s", __FUNCTION__, actionStr.c_str());
      CGUIInfoLabel info(actionStr, "");
      actionStr = info.GetLabel(0);
      CLog::Log(LOGDEBUG,"%s : To %s", __FUNCTION__, actionStr.c_str());

      // user has asked for something to be executed
      if (CUtil::IsBuiltIn(actionStr))
        CUtil::ExecBuiltIn(actionStr);
      else
      {
        // try translating the action from our ButtonTranslator
        WORD actionID;
        if (CButtonTranslator::TranslateActionString(actionStr.c_str(), actionID))
        {
          CAction action;
          action.wID = actionID;
          action.fAmount1 = 1.0f;
          OnAction(action);
          return true;
        }
        CFileItem item(actionStr, false);
#ifdef HAS_PYTHON
        if (item.IsPythonScript())
        { // a python script
          g_pythonParser.evalFile(item.m_strPath.c_str());
        }
        else
#endif
        if (item.IsAudio() || item.IsVideo())
        { // an audio or video file
          PlayFile(item);
        }
        else
          return false;
      }
      return true;
    }

bool CApplication::ExecuteAction(CGUIActionDescriptor action)
{
  if (action.m_lang == CGUIActionDescriptor::LANG_XBMC)
  {
    return ExecuteXBMCAction(action.m_action);
  }
  else if (action.m_lang == CGUIActionDescriptor::LANG_PYTHON)
  {
    // Determine the context of the action, if possible
    g_pythonParser.evalString(action.m_action);

    return true;
  }
  return false;
}

void CApplication::Process()
{
  MEASURE_FUNCTION;

  // check if we need to load a new skin
  if (m_dwSkinTime && timeGetTime() >= m_dwSkinTime)
  {
    ReloadSkin();
  }

  // dispatch the messages generated by python or other threads to the current window
  m_gWindowManager.DispatchThreadMessages();

  // process messages which have to be send to the gui
  // (this can only be done after m_gWindowManager.Render())
  m_applicationMessenger.ProcessWindowMessages();

#ifdef HAS_PYTHON
  // process any Python scripts
  g_pythonParser.Process();
#endif

  // process messages, even if a movie is playing
  m_applicationMessenger.ProcessMessages();
  if (g_application.m_bStop) return; //we're done, everything has been unloaded

  // check if we can free unused memory
#ifndef _LINUX
  g_audioManager.FreeUnused();
#endif

  // check how far we are through playing the current item
  // and do anything that needs doing (lastfm submission, playcount updates etc)
  CheckPlayingProgress();

  // update sound
  if (m_pPlayer)
    m_pPlayer->DoAudioWork();

  // do any processing that isn't needed on each run
  if( m_slowTimer.GetElapsedMilliseconds() > 500 )
  {
    m_slowTimer.Reset();
    ProcessSlow();
  }
}

// We get called every 500ms
void CApplication::ProcessSlow()
{
  // Store our file state for use on close()
  UpdateFileState();

  if (IsPlayingAudio())
  {
    CLastfmScrobbler::GetInstance()->UpdateStatus();
    CLibrefmScrobbler::GetInstance()->UpdateStatus();
  }

  // Check if we need to activate the screensaver / DPMS.
  CheckScreenSaverAndDPMS();

  // Check if we need to shutdown (if enabled).
#ifdef __APPLE__
  if (g_guiSettings.GetInt("system.shutdowntime") && g_advancedSettings.m_fullScreen)
#else
  if (g_guiSettings.GetInt("system.shutdowntime"))
#endif
  {
    CheckShutdown();
  }

  // check if we should restart the player
  CheckDelayedPlayerRestart();

  //  check if we can unload any unreferenced dlls or sections
  CSectionLoader::UnloadDelayed();

  // check for any idle curl connections
  g_curlInterface.CheckIdle();

  // check for any idle myth sessions
  CCMythSession::CheckIdle();

#ifdef HAS_FILESYSTEM_HTSP
  // check for any idle htsp sessions
  HTSP::CHTSPDirectorySession::CheckIdle();
#endif

#ifdef HAS_TIME_SERVER
  // check for any needed sntp update
  if(m_psntpClient && m_psntpClient->UpdateNeeded())
    m_psntpClient->Update();
#endif

#ifdef HAS_KARAOKE
  if ( m_pKaraokeMgr )
    m_pKaraokeMgr->ProcessSlow();
#endif

  // LED - LCD SwitchOn On Paused! m_bIsPaused=TRUE -> LED/LCD is ON!
  if(IsPaused() != m_bIsPaused)
  {
#ifdef HAS_LCD
    if(g_guiSettings.GetBool("lcd.enableonpaused"))
      DimLCDOnPlayback(m_bIsPaused);
#endif
    m_bIsPaused = IsPaused();
  }

  g_largeTextureManager.CleanupUnusedImages();

  // checks whats in the DVD drive and tries to autostart the content (xbox games, dvd, cdda, avi files...)
  m_Autorun.HandleAutorun();

  // update upnp server/renderer states
  if(CUPnP::IsInstantiated())
    CUPnP::GetInstance()->UpdateState();

  //Check to see if current playing Title has changed and whether we should broadcast the fact
  CheckForTitleChange();

#if defined(_LINUX) && defined(HAS_FILESYSTEM_SMB)
  smb.CheckIfIdle();
#endif

// Update HalManager to get newly connected media
#ifdef HAS_HAL
  while(g_HalManager.Update()) ;  //If there is 1 message it might be another one in queue, we take care of them directly
  if (CLinuxFileSystem::AnyDeviceChange())
  { // changes have occured - update our shares
    CGUIMessage msg(GUI_MSG_NOTIFY_ALL,0,0,GUI_MSG_REMOVED_MEDIA);
    m_gWindowManager.SendThreadMessage(msg);
  }
#endif
#ifdef HAS_LIRC
  if (g_RemoteControl.IsInUse() && !g_RemoteControl.IsInitialized())
    g_RemoteControl.Initialize();
#endif
}

// Global Idle Time in Seconds
// idle time will be resetet if on any OnKey()
// int return: system Idle time in seconds! 0 is no idle!
int CApplication::GlobalIdleTime()
{
  if(!m_idleTimer.IsRunning())
  {
    m_idleTimer.Stop();
    m_idleTimer.StartZero();
  }
  return (int)m_idleTimer.GetElapsedSeconds();
}

float CApplication::NavigationIdleTime()
{
  if (!m_navigationTimer.IsRunning())
  {
    m_navigationTimer.Stop();
    m_navigationTimer.StartZero();
  }
  return m_navigationTimer.GetElapsedSeconds();
}

void CApplication::DelayedPlayerRestart()
{
  m_restartPlayerTimer.StartZero();
}

void CApplication::CheckDelayedPlayerRestart()
{
  if (m_restartPlayerTimer.GetElapsedSeconds() > 3)
  {
    m_restartPlayerTimer.Stop();
    m_restartPlayerTimer.Reset();
    Restart(true);
  }
}

void CApplication::Restart(bool bSamePosition)
{
  // this function gets called when the user changes a setting (like noninterleaved)
  // and which means we gotta close & reopen the current playing file

  // first check if we're playing a file
  if ( !IsPlayingVideo() && !IsPlayingAudio())
    return ;

  if( !m_pPlayer )
    return ;

  // do we want to return to the current position in the file
  if (false == bSamePosition)
  {
    // no, then just reopen the file and start at the beginning
    PlayFile(*m_itemCurrentFile, true);
    return ;
  }

  // else get current position
  double time = GetTime();

  // get player state, needed for dvd's
  CStdString state = m_pPlayer->GetPlayerState();

  // set the requested starttime
  m_itemCurrentFile->m_lStartOffset = (long)(time * 75.0);

  // reopen the file
  if ( PlayFile(*m_itemCurrentFile, true) && m_pPlayer )
    m_pPlayer->SetPlayerState(state);
}

const CStdString& CApplication::CurrentFile()
{
  return m_itemCurrentFile->m_strPath;
}

CFileItem& CApplication::CurrentFileItem()
{
  return *m_itemCurrentFile;
}

void CApplication::Mute(void)
{
  if (g_stSettings.m_bMute)
  { // muted - unmute.
    // check so we don't get stuck in some muted state
    if( g_stSettings.m_iPreMuteVolumeLevel == 0 )
      g_stSettings.m_iPreMuteVolumeLevel = 1;
    SetVolume(g_stSettings.m_iPreMuteVolumeLevel);
  }
  else
  { // mute
    g_stSettings.m_iPreMuteVolumeLevel = GetVolume();
    SetVolume(0);
  }
}

void CApplication::SetVolume(int iPercent)
{
  // convert the percentage to a mB (milliBell) value (*100 for dB)
  long hardwareVolume = (long)((float)iPercent * 0.01f * (VOLUME_MAXIMUM - VOLUME_MINIMUM) + VOLUME_MINIMUM);
  SetHardwareVolume(hardwareVolume);
  g_audioManager.SetVolume(iPercent);
}

void CApplication::SetHardwareVolume(long hardwareVolume)
{
  // TODO DRC
  if (hardwareVolume >= VOLUME_MAXIMUM) // + VOLUME_DRC_MAXIMUM
    hardwareVolume = VOLUME_MAXIMUM;// + VOLUME_DRC_MAXIMUM;
  if (hardwareVolume <= VOLUME_MINIMUM)
  {
    hardwareVolume = VOLUME_MINIMUM;
  }
  // update our settings
  if (hardwareVolume > VOLUME_MAXIMUM)
  {
    g_stSettings.m_dynamicRangeCompressionLevel = hardwareVolume - VOLUME_MAXIMUM;
    g_stSettings.m_nVolumeLevel = VOLUME_MAXIMUM;
  }
  else
  {
    g_stSettings.m_dynamicRangeCompressionLevel = 0;
    g_stSettings.m_nVolumeLevel = hardwareVolume;
  }

  // update mute state
  if(!g_stSettings.m_bMute && hardwareVolume <= VOLUME_MINIMUM)
  {
    g_stSettings.m_bMute = true;
    if (!m_guiDialogMuteBug.IsDialogRunning())
      m_guiDialogMuteBug.Show();
  }
  else if(g_stSettings.m_bMute && hardwareVolume > VOLUME_MINIMUM)
  {
    g_stSettings.m_bMute = false;
    if (m_guiDialogMuteBug.IsDialogRunning())
      m_guiDialogMuteBug.Close();
  }

  // and tell our player to update the volume
  if (m_pPlayer)
  {
    m_pPlayer->SetVolume(g_stSettings.m_nVolumeLevel);
    // TODO DRC
//    m_pPlayer->SetDynamicRangeCompression(g_stSettings.m_dynamicRangeCompressionLevel);
  }
}

int CApplication::GetVolume() const
{
  // converts the hardware volume (in mB) to a percentage
  return int(((float)(g_stSettings.m_nVolumeLevel + g_stSettings.m_dynamicRangeCompressionLevel - VOLUME_MINIMUM)) / (VOLUME_MAXIMUM - VOLUME_MINIMUM)*100.0f + 0.5f);
}

int CApplication::GetSubtitleDelay() const
{
  // converts subtitle delay to a percentage
  return int(((float)(g_stSettings.m_currentVideoSettings.m_SubtitleDelay + g_advancedSettings.m_videoSubsDelayRange)) / (2 * g_advancedSettings.m_videoSubsDelayRange)*100.0f + 0.5f);
}

int CApplication::GetAudioDelay() const
{
  // converts subtitle delay to a percentage
  return int(((float)(g_stSettings.m_currentVideoSettings.m_AudioDelay + g_advancedSettings.m_videoAudioDelayRange)) / (2 * g_advancedSettings.m_videoAudioDelayRange)*100.0f + 0.5f);
}

void CApplication::SetPlaySpeed(int iSpeed)
{
  if (!IsPlayingAudio() && !IsPlayingVideo())
    return ;
  if (m_iPlaySpeed == iSpeed)
    return ;
  if (!m_pPlayer->CanSeek())
    return;
  if (m_pPlayer->IsPaused())
  {
    if (
      ((m_iPlaySpeed > 1) && (iSpeed > m_iPlaySpeed)) ||
      ((m_iPlaySpeed < -1) && (iSpeed < m_iPlaySpeed))
    )
    {
      iSpeed = m_iPlaySpeed; // from pause to ff/rw, do previous ff/rw speed
    }
    m_pPlayer->Pause();
  }
  m_iPlaySpeed = iSpeed;

  m_pPlayer->ToFFRW(m_iPlaySpeed);
  if (m_iPlaySpeed == 1)
  { // restore volume
    m_pPlayer->SetVolume(g_stSettings.m_nVolumeLevel);
  }
  else
  { // mute volume
    m_pPlayer->SetVolume(VOLUME_MINIMUM);
  }
}

int CApplication::GetPlaySpeed() const
{
  return m_iPlaySpeed;
}

// Returns the total time in seconds of the current media.  Fractional
// portions of a second are possible - but not necessarily supported by the
// player class.  This returns a double to be consistent with GetTime() and
// SeekTime().
double CApplication::GetTotalTime() const
{
  double rc = 0.0;

  if (IsPlaying() && m_pPlayer)
  {
    if (m_itemCurrentFile->IsStack() && m_currentStack->Size() > 0)
      rc = (*m_currentStack)[m_currentStack->Size() - 1]->m_lEndOffset;
    else
      rc = m_pPlayer->GetTotalTime();
  }

  return rc;
}

void CApplication::ResetPlayTime()
{
  if (IsPlaying() && m_pPlayer)
    m_pPlayer->ResetTime();
}

// Returns the current time in seconds of the currently playing media.
// Fractional portions of a second are possible.  This returns a double to
// be consistent with GetTotalTime() and SeekTime().
double CApplication::GetTime() const
{
  double rc = 0.0;

  if (IsPlaying() && m_pPlayer)
  {
    if (m_itemCurrentFile->IsStack() && m_currentStack->Size() > 0)
    {
      long startOfCurrentFile = (m_currentStackPosition > 0) ? (*m_currentStack)[m_currentStackPosition-1]->m_lEndOffset : 0;
      rc = (double)startOfCurrentFile + m_pPlayer->GetTime() * 0.001;
    }
    else
      rc = static_cast<double>(m_pPlayer->GetTime() * 0.001f);
  }

  return rc;
}

// Sets the current position of the currently playing media to the specified
// time in seconds.  Fractional portions of a second are valid.  The passed
// time is the time offset from the beginning of the file as opposed to a
// delta from the current position.  This method accepts a double to be
// consistent with GetTime() and GetTotalTime().
void CApplication::SeekTime( double dTime )
{
  if (IsPlaying() && m_pPlayer && (dTime >= 0.0))
  {
    if (!m_pPlayer->CanSeek()) return;
    if (m_itemCurrentFile->IsStack() && m_currentStack->Size() > 0)
    {
      // find the item in the stack we are seeking to, and load the new
      // file if necessary, and calculate the correct seek within the new
      // file.  Otherwise, just fall through to the usual routine if the
      // time is higher than our total time.
      for (int i = 0; i < m_currentStack->Size(); i++)
      {
        if ((*m_currentStack)[i]->m_lEndOffset > dTime)
        {
          long startOfNewFile = (i > 0) ? (*m_currentStack)[i-1]->m_lEndOffset : 0;
          if (m_currentStackPosition == i)
            m_pPlayer->SeekTime((__int64)((dTime - startOfNewFile) * 1000.0));
          else
          { // seeking to a new file
            m_currentStackPosition = i;
            CFileItem item(*(*m_currentStack)[i]);
            item.m_lStartOffset = (long)((dTime - startOfNewFile) * 75.0);
            // don't just call "PlayFile" here, as we are quite likely called from the
            // player thread, so we won't be able to delete ourselves.
            m_applicationMessenger.PlayFile(item, true);
          }
          return;
        }
      }
    }
    // convert to milliseconds and perform seek
    m_pPlayer->SeekTime( static_cast<__int64>( dTime * 1000.0 ) );
  }
}

float CApplication::GetPercentage() const
{
  if (IsPlaying() && m_pPlayer)
  {
    if (IsPlayingAudio() && m_itemCurrentFile->HasMusicInfoTag())
    {
      const CMusicInfoTag& tag = *m_itemCurrentFile->GetMusicInfoTag();
      if (tag.GetDuration() > 0)
        return (float)(GetTime() / tag.GetDuration() * 100);
    }

    if (m_itemCurrentFile->IsStack() && m_currentStack->Size() > 0)
      return (float)(GetTime() / GetTotalTime() * 100);
    else
      return m_pPlayer->GetPercentage();
  }
  return 0.0f;
}

void CApplication::SeekPercentage(float percent)
{
  if (IsPlaying() && m_pPlayer && (percent >= 0.0))
  {
    if (!m_pPlayer->CanSeek()) return;
    if (m_itemCurrentFile->IsStack() && m_currentStack->Size() > 0)
      SeekTime(percent * 0.01 * GetTotalTime());
    else
      m_pPlayer->SeekPercentage(percent);
  }
}

// SwitchToFullScreen() returns true if a switch is made, else returns false
bool CApplication::SwitchToFullScreen()
{
  // if playing from the video info window, close it first!
  if (m_gWindowManager.HasModalDialog() && m_gWindowManager.GetTopMostModalDialogID() == WINDOW_VIDEO_INFO)
  {
    CGUIWindowVideoInfo* pDialog = (CGUIWindowVideoInfo*)m_gWindowManager.GetWindow(WINDOW_VIDEO_INFO);
    if (pDialog) pDialog->Close(true);
  }

  // don't switch if there is a dialog on screen or the slideshow is active
  if (/*m_gWindowManager.HasModalDialog() ||*/ m_gWindowManager.GetActiveWindow() == WINDOW_SLIDESHOW)
    return false;

  // See if we're playing a video, and are in GUI mode
  if ( IsPlayingVideo() && m_gWindowManager.GetActiveWindow() != WINDOW_FULLSCREEN_VIDEO)
  {
#ifdef HAS_SDL
    // Reset frame count so that timing is FPS will be correct.
    SDL_mutexP(m_frameMutex);
    m_frameCount = 0;
    SDL_mutexV(m_frameMutex);
#endif

    // then switch to fullscreen mode
    m_gWindowManager.ActivateWindow(WINDOW_FULLSCREEN_VIDEO);
    return true;
  }
  // special case for switching between GUI & visualisation mode. (only if we're playing an audio song)
  if (IsPlayingAudio() && m_gWindowManager.GetActiveWindow() != WINDOW_VISUALISATION)
  { // then switch to visualisation
    m_gWindowManager.ActivateWindow(WINDOW_VISUALISATION);
    return true;
  }
  return false;
}

void CApplication::Minimize(bool minimize)
{
  if (minimize)
  {
    m_bWasFullScreenBeforeMinimize = g_graphicsContext.IsFullScreenRoot();
    if (m_bWasFullScreenBeforeMinimize) g_graphicsContext.SetFullScreenRoot(false);
#ifdef HAS_SDL
    SDL_WM_IconifyWindow();
#endif
  }
  else
  {
    if (m_bWasFullScreenBeforeMinimize) g_graphicsContext.SetFullScreenRoot(true);
  }
}

PLAYERCOREID CApplication::GetCurrentPlayer()
{
  return m_eCurrentPlayer;
}

// when a scan is initiated, save current settings
// and enable tag reading and remote thums
void CApplication::SaveMusicScanSettings()
{
  CLog::Log(LOGINFO,"Music scan has started... Enabling tag reading, and remote thumbs");
  g_stSettings.m_bMyMusicIsScanning = true;
  g_settings.Save();
}

void CApplication::RestoreMusicScanSettings()
{
  g_stSettings.m_bMyMusicIsScanning = false;
  g_settings.Save();
}

void CApplication::CheckPlayingProgress()
{
  // check if we haven't rewound past the start of the file
  if (IsPlaying())
  {
    int iSpeed = g_application.GetPlaySpeed();
    if (iSpeed < 1)
    {
      iSpeed *= -1;
      int iPower = 0;
      while (iSpeed != 1)
      {
        iSpeed >>= 1;
        iPower++;
      }
      if (g_infoManager.GetPlayTime() / 1000 < iPower)
      {
        g_application.SetPlaySpeed(1);
        g_application.SeekTime(0);
      }
    }
  }
}

bool CApplication::ProcessAndStartPlaylist(const CStdString& strPlayList, CPlayList& playlist, int iPlaylist)
{
  CLog::Log(LOGDEBUG,"CApplication::ProcessAndStartPlaylist(%s, %i)",strPlayList.c_str(), iPlaylist);

  // initial exit conditions
  // no songs in playlist just return
  if (playlist.size() == 0)
    return false;

  // illegal playlist
  if (iPlaylist < PLAYLIST_MUSIC || iPlaylist > PLAYLIST_VIDEO)
    return false;

  // setup correct playlist
  g_playlistPlayer.ClearPlaylist(iPlaylist);

  // if the playlist contains an internet stream, this file will be used
  // to generate a thumbnail for musicplayer.cover
  g_application.m_strPlayListFile = strPlayList;

  // add the items to the playlist player
  g_playlistPlayer.Add(iPlaylist, playlist);

  // if we have a playlist
  if (g_playlistPlayer.GetPlaylist(iPlaylist).size())
  {
    // start playing it
    g_playlistPlayer.SetCurrentPlaylist(iPlaylist);
    g_playlistPlayer.Reset();
    g_playlistPlayer.Play();
    return true;
  }
  return false;
}

void CApplication::StartFtpEmergencyRecoveryMode()
{
#ifdef HAS_FTP_SERVER
  m_pFileZilla = new CXBFileZilla(NULL);
  m_pFileZilla->Start();

  // Default settings
  m_pFileZilla->mSettings.SetMaxUsers(0);
  m_pFileZilla->mSettings.SetWelcomeMessage("XBMC emergency recovery console FTP.");

  // default user
  CXFUser* pUser;
  m_pFileZilla->AddUser("xbox", pUser);
  pUser->SetPassword("xbox");
  pUser->SetShortcutsEnabled(false);
  pUser->SetUseRelativePaths(false);
  pUser->SetBypassUserLimit(false);
  pUser->SetUserLimit(0);
  pUser->SetIPLimit(0);
  pUser->AddDirectory("/", XBFILE_READ | XBFILE_WRITE | XBFILE_DELETE | XBFILE_APPEND | XBDIR_DELETE | XBDIR_CREATE | XBDIR_LIST | XBDIR_SUBDIRS | XBDIR_HOME);
  pUser->AddDirectory("C:\\", XBFILE_READ | XBFILE_WRITE | XBFILE_DELETE | XBFILE_APPEND | XBDIR_DELETE | XBDIR_CREATE | XBDIR_LIST | XBDIR_SUBDIRS);
  pUser->AddDirectory("D:\\", XBFILE_READ | XBDIR_LIST | XBDIR_SUBDIRS);
  pUser->AddDirectory("E:\\", XBFILE_READ | XBFILE_WRITE | XBFILE_DELETE | XBFILE_APPEND | XBDIR_DELETE | XBDIR_CREATE | XBDIR_LIST | XBDIR_SUBDIRS);
  pUser->AddDirectory("Q:\\", XBFILE_READ | XBFILE_WRITE | XBFILE_DELETE | XBFILE_APPEND | XBDIR_DELETE | XBDIR_CREATE | XBDIR_LIST | XBDIR_SUBDIRS);
  //Add. also Drive F/G
  if (CIoSupport::DriveExists('F')){
    pUser->AddDirectory("F:\\", XBFILE_READ | XBFILE_WRITE | XBFILE_DELETE | XBFILE_APPEND | XBDIR_DELETE | XBDIR_CREATE | XBDIR_LIST | XBDIR_SUBDIRS);
  }
  if (CIoSupport::DriveExists('G')){
    pUser->AddDirectory("G:\\", XBFILE_READ | XBFILE_WRITE | XBFILE_DELETE | XBFILE_APPEND | XBDIR_DELETE | XBDIR_CREATE | XBDIR_LIST | XBDIR_SUBDIRS);
  }
  pUser->CommitChanges();
#endif
}

void CApplication::SaveCurrentFileSettings()
{
  if (m_itemCurrentFile->IsVideo())
  {
    // save video settings
    if (g_stSettings.m_currentVideoSettings != g_stSettings.m_defaultVideoSettings)
    {
      CVideoDatabase dbs;
      dbs.Open();
      dbs.SetVideoSettings(m_itemCurrentFile->m_strPath, g_stSettings.m_currentVideoSettings);
      dbs.Close();
    }
  }
}

bool CApplication::AlwaysProcess(const CAction& action)
{
  // check if this button is mapped to a built-in function
  if (action.strAction)
  {
    CStdString builtInFunction, param;
    CUtil::SplitExecFunction(action.strAction, builtInFunction, param);
    builtInFunction.ToLower();

    // should this button be handled normally or just cancel the screensaver?
    if (   builtInFunction.Equals("powerdown")
        || builtInFunction.Equals("reboot")
        || builtInFunction.Equals("restart")
        || builtInFunction.Equals("restartapp")
        || builtInFunction.Equals("suspend")
        || builtInFunction.Equals("hibernate")
        || builtInFunction.Equals("quit")
        || builtInFunction.Equals("shutdown"))
    {
      return true;
    }
  }

  return false;
}

CApplicationMessenger& CApplication::getApplicationMessenger()
{
   return m_applicationMessenger;
}

bool CApplication::IsPresentFrame()
{
#ifdef HAS_SDL // TODO:DIRECTX
  SDL_mutexP(m_frameMutex);
  bool ret = m_bPresentFrame;
  SDL_mutexV(m_frameMutex);

  return ret;
#else
  return false;
#endif
}

#if defined(HAS_LINUX_NETWORK)
CNetworkLinux& CApplication::getNetwork()
{
  return m_network;
}
#elif defined(HAS_WIN32_NETWORK)
CNetworkWin32& CApplication::getNetwork()
{
  return m_network;
}
#else
CNetwork& CApplication::getNetwork()
{
  return m_network;
}

#endif
#ifdef HAS_PERFORMANCE_SAMPLE
CPerformanceStats &CApplication::GetPerformanceStats()
{
  return m_perfStats;
}
#endif
