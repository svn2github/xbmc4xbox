
#include "stdafx.h"
#include "application.h"
#include "utils/lcd.h"
#include "xbox\iosupport.h"
#include "xbox/xkutils.h"
#include "utils/log.h"
#include "settings.h"
#include "util.h"
#include "sectionLoader.h"
#include "texturemanager.h"
#include "stdstring.h"
#include "cores/playercorefactory.h"
#include "playlistplayer.h"
#include "musicdatabase.h"
#include "url.h"
#include "autorun.h"
#include "ActionManager.h"
#include "utils/LCDFactory.h"
#include "cores/ModPlayer.h"
#include "cores/mplayer/ASyncDirectSound.h"
#include "GUIButtonControl.h"
#include "GUISpinControl.h"
#include "GUIListControl.h"
#include "GUIThumbnailPanel.h"


#pragma comment (lib,"xbmc/lib/libXenium/XeniumSPIg.lib")    
#ifdef _DEBUG
//	#pragma comment (lib,"lib/filezilla/xbfilezillad.lib") // SECTIONNAME=FILEZILL
	#pragma comment (lib,"xbmc/lib/libXBMS/libXBMSd.lib")    // SECTIONNAME=LIBXBMS
	#pragma comment (lib,"xbmc/lib/libsmb/libsmbd.lib")      // SECTIONNAME=LIBSMB
	#pragma comment (lib,"xbmc/lib/cximage/ImageLibd.lib")   // SECTIONNAME=CXIMAGE
	#pragma comment (lib,"xbmc/lib/libID3/i3dlibd.lib")			 // SECTIONNAME=LIBID3
	#pragma comment (lib,"xbmc/lib/libCDRip/cdripd.lib")		 // SECTIONNAME=LIBCDRIP
	#pragma comment (lib,"xbmc/lib/libPython/pythond.lib")	 // SECTIONNAME=PYTHON/PY_RW
	#pragma comment (lib,"xbmc/lib/libGoAhead/goaheadd.lib") // SECTIONNAME=LIBHTTP
	#pragma comment (lib,"xbmc/lib/sqlLite/libSQLited.lib")    
	#pragma comment (lib,"guilib/debug/guiLib.lib")				   // -
	#pragma comment (lib,"xbmc/cores/dllLoader/debug/dllloader.lib")				   // -
	#pragma comment (lib, "xbmc/lib/libcdio/libcdiod.lib" )
	#pragma comment (lib, "xbmc/lib/libshout/libshoutd.lib" )
	#pragma comment (lib,"xbmc/lib/libRTV/libRTVd.lib")    // SECTIONNAME=LIBRTV
	#pragma comment (lib,"xbmc/lib/mikxbox/mikxboxd.lib")  // SECTIONNAME=MOD_RW,MOD_RX
	#pragma comment (lib,"xbmc/lib/libsidplay/libsidplayd.lib")			// SECTIONNAME=SID_RW,SID_RX
	#pragma comment (lib,"xbmc/lib/libsidplay/libsidutilsd.lib")		// SECTIONNAME=SID_RW,SID_RX
	#pragma comment (lib,"xbmc/lib/libsidplay/resid_builderd.lib")	// SECTIONNAME=SID_RW,SID_RX
#else
//  #pragma comment (lib,"lib/filezilla/xbfilezilla.lib")
	#pragma comment (lib,"xbmc/lib/libXBMS/libXBMS.lib")          
  #pragma comment (lib,"xbmc/lib/libsmb/libsmb.lib")           
	#pragma comment (lib,"xbmc/lib/cximage/ImageLib.lib")
	#pragma comment (lib,"xbmc/lib/libID3/i3dlib.lib")					
	#pragma comment (lib,"xbmc/lib/libCDRip/cdrip.lib")						
	#pragma comment (lib,"xbmc/lib/libPython/python.lib")		 
	#pragma comment (lib,"xbmc/lib/libGoAhead/goahead.lib")
	#pragma comment (lib,"xbmc/lib/sqlLite/libSQLite.lib")    
	#pragma comment (lib,"guiLib/release/guiLib.lib")
	#pragma comment (lib,"xbmc/cores/dllLoader/release/dllloader.lib")				   // -
	#pragma comment (lib, "xbmc/lib/libcdio/libcdio.lib" )
	#pragma comment (lib, "xbmc/lib/libshout/libshout.lib" )
	#pragma comment (lib,"xbmc/lib/libRTV/libRTV.lib")
	#pragma comment (lib,"xbmc/lib/mikxbox/mikxbox.lib")
	#pragma comment (lib,"xbmc/lib/libsidplay/libsidplay.lib")			// SECTIONNAME=SID_RW,SID_RX
	#pragma comment (lib,"xbmc/lib/libsidplay/libsidutils.lib")		// SECTIONNAME=SID_RW,SID_RX
	#pragma comment (lib,"xbmc/lib/libsidplay/resid_builder.lib")	// SECTIONNAME=SID_RW,SID_RX
#endif

extern IDirectSoundRenderer* m_pAudioDecoder;
extern int m_iAudioStreamIDX;
CApplication::CApplication(void)
:m_ctrDpad(220,220)
,m_ctrIR(220,220)
{
    m_iPlaySpeed=1;
    m_strCurrentFile="";
		m_bSpinDown=false;
		m_bOverlayEnabled=true;
		m_pWebServer=NULL;
		m_pFileZilla=NULL;
		m_pPlayer=NULL;
		XSetProcessQuantumLength(5); //default=20msec
		XSetFileCacheSize (256*1024);//default=64kb
		m_dwSpinDownTime=timeGetTime();		
		m_bInactive = false;			// CB: SCREENSAVER PATCH
		m_bScreenSave = false;			// CB: SCREENSAVER PATCH
		m_dwSaverTick=timeGetTime();	// CB: SCREENSAVER PATCH
		m_dwSkinTime = 0;
}	

CApplication::~CApplication(void)
{
	
}


HRESULT CApplication::Create()
{
	CIoSupport helper;
	CStdString strPath;
	char szDevicePath[1024];

	// map Q to home drive of xbe to load the config file
	CUtil::GetHomePath(strPath);
	helper.GetPartition(strPath, szDevicePath);
	strcat(szDevicePath,&strPath.c_str()[2]);

	helper.Mount("Q:","Harddisk0\\Partition2");
	helper.Unmount("Q:");
	helper.Mount("Q:", szDevicePath);	

  ::DeleteFile("Q:\\xbmc.old.log");
  ::MoveFile("Q:\\xbmc.log","Q:\\xbmc.old.log");
  CLog::Log("-----------------------------------------------------------------------");
  CLog::Log("starting...");
  CLog::Log("Q is mapped to:%s",szDevicePath);
	// Transfer the resolution information to our graphics context
	g_graphicsContext.SetD3DParameters(&m_d3dpp, g_settings.m_ResInfo);

  CLog::Log("load settings...");
	m_bAllSettingsLoaded=g_settings.Load(m_bXboxMediacenterLoaded,m_bSettingsLoaded,m_bCalibrationLoaded);
	
  CLog::Log("map drives...");
  CLog::Log("  map drive C:");
	helper.Remap("C:,Harddisk0\\Partition2");
  
  CLog::Log("  map drive E:");
	helper.Remap("E:,Harddisk0\\Partition1");

  CLog::Log("  map drive D:");
	helper.Remount("D:","Cdrom0");

	if (g_stSettings.m_bUseFDrive) 
  {
    CLog::Log("  map drive F:");
    helper.Remap("F:,Harddisk0\\Partition6");
  }

	// used for the LBA-48 hack allowing >120 gig
	if (g_stSettings.m_bUseGDrive) 
  {
    CLog::Log("  map drive G:");
    helper.Remap("G:,Harddisk0\\Partition7");
  }
	
	// check settings to see if another home dir is defined.
	// if there is, we check if it's a xbmc dir and map to it Q:
	if (strlen(g_stSettings.szHomeDir) > 1)
	{
    CLog::Log("map Q: to homedir:%s...",g_stSettings.szHomeDir);
		// home dir is defined in xboxmediacenter.xml
		CStdString strHomePath = g_stSettings.szHomeDir;

		if(!access(strHomePath + "\\skin", 0))
		{
			helper.GetPartition(strHomePath, szDevicePath);
			strcat(szDevicePath, &strHomePath.c_str()[2]);

			CLog::Close();
			helper.Unmount("Q:");
			helper.Mount("Q:", szDevicePath);	
      ::DeleteFile("Q:\\xbmc.old.log");
      ::MoveFile("Q:\\xbmc.log","Q:\\xbmc.old.log");
      CLog::Close();
      CLog::Log("Q is mapped to:%s",szDevicePath);
		}
	}

	CStdString strLanguagePath;
	strLanguagePath.Format("Q:\\language\\%s\\strings.xml", g_stSettings.szDefaultLanguage);

  CLog::Log("load language file:%s",strLanguagePath.c_str());
  g_localizeStrings.Load(strLanguagePath );
	
  CLog::Log("load keymapping");
  g_buttonTranslator.Load();

  CLog::Log("setup DirectX");
	g_graphicsContext.SetGUIResolution(g_stSettings.m_ScreenResolution);
	g_graphicsContext.SetOffset(g_stSettings.m_iUIOffsetX, g_stSettings.m_iUIOffsetY);

  int  iResolution=g_stSettings.m_ScreenResolution;
  CLog::Log(" GUI format %ix%i %s",
            g_settings.m_ResInfo[iResolution].iWidth, 
            g_settings.m_ResInfo[iResolution].iHeight, 
            g_settings.m_ResInfo[iResolution].strMode);
  CLog::Log(" GUI screen offset (%i,%i)", g_stSettings.m_iUIOffsetX, g_stSettings.m_iUIOffsetY);
	m_gWindowManager.Initialize();
	g_actionManager.SetScriptActionCallback(&m_pythonParser);
	return CXBApplicationEx::Create();
}

HRESULT CApplication::Initialize()
{
  CLog::Log("creating subdirectories");
  if (g_stSettings.szThumbnailsDirectory[0]==0)
  {
		strcpy(g_stSettings.szThumbnailsDirectory,"Q:\\thumbs");
  }
  if (g_stSettings.m_szShortcutDirectory[0]==0)
  {
		strcpy(g_stSettings.m_szShortcutDirectory,"Q:\\shortcuts");		
  }
  if (g_stSettings.m_szAlbumDirectory[0]==0)
  {
		strcpy(g_stSettings.m_szAlbumDirectory,"Q:\\albums");		
  }
  if (g_stSettings.m_szMusicRecordingDirectory[0]==0)
  {
		strcpy(g_stSettings.m_szMusicRecordingDirectory,"Q:\\recordings");		
  }
  if (g_stSettings.m_szScreenshotsDirectory[0]==0)
  {
	  strcpy(g_stSettings.m_szScreenshotsDirectory, "Q:\\screenshots");
  }

  CreateDirectory(g_stSettings.szThumbnailsDirectory,NULL);
  CStdString strThumbIMDB=g_stSettings.szThumbnailsDirectory;
  strThumbIMDB+="\\imdb";
  CreateDirectory(strThumbIMDB.c_str(),NULL);

	CreateDirectory(g_stSettings.m_szShortcutDirectory,NULL);
	CreateDirectory(g_stSettings.m_szAlbumDirectory,NULL);
	CreateDirectory(g_stSettings.m_szMusicRecordingDirectory,NULL);
	CreateDirectory(g_stSettings.m_szScreenshotsDirectory, NULL);

  CLog::Log("  thumbnails folder:%s", g_stSettings.szThumbnailsDirectory);
  CLog::Log("  shortcuts folder:%s", g_stSettings.m_szShortcutDirectory);
  CLog::Log("  albums folder:%s", g_stSettings.m_szAlbumDirectory);
  CLog::Log("  recording folder:%s", g_stSettings.m_szMusicRecordingDirectory);
  CLog::Log("  screenshots folder:%s", g_stSettings.m_szScreenshotsDirectory);

	string strAlbumDir=g_stSettings.m_szAlbumDirectory;
	CreateDirectory((strAlbumDir+"\\playlists").c_str(),NULL);
	CreateDirectory((strAlbumDir+"\\cddb").c_str(),NULL);
	CreateDirectory((strAlbumDir+"\\thumbs").c_str(),NULL); // contains the album thumbs
	CreateDirectory((strAlbumDir+"\\thumbs\\temp").c_str(),NULL);
	CreateDirectory("Q:\\python",NULL);
	CreateDirectory("Q:\\scripts",NULL);
	CreateDirectory("Q:\\language",NULL);
	CreateDirectory("Q:\\visualisations",NULL);

  if (g_stSettings.m_szAlternateSubtitleDirectory[0]==0)
  {
    strcpy(g_stSettings.m_szAlternateSubtitleDirectory,"Q:\\subtitles");
  }
  CLog::Log("  subtitle folder:%s", g_stSettings.m_szAlternateSubtitleDirectory);
  CreateDirectory(g_stSettings.m_szAlternateSubtitleDirectory,NULL);
  
  // initialize network
  if (!m_bXboxMediacenterLoaded)
  {
    CLog::Log("using default network settings");
    strcpy (g_stSettings.m_strLocalIPAdres,"192.168.0.100");
    strcpy (g_stSettings.m_strLocalNetmask,"255.255.255.0");
    strcpy (g_stSettings.m_strGateway,"192.168.0.1");
    strcpy (g_stSettings.m_strNameServer,"192.168.0.1");
    g_stSettings.m_bFTPServerEnabled=true;
    g_stSettings.m_bHTTPServerEnabled=false;
    g_stSettings.m_bTimeServerEnabled=false;
  }
  CLog::Log("initialize network ip:[%s] netmask:[%s] gateway:[%s] nameserver:[%s]",
                    g_stSettings.m_strLocalIPAdres,
                    g_stSettings.m_strLocalNetmask,
                    g_stSettings.m_strGateway,
                    g_stSettings.m_strNameServer);

  if ( CUtil::InitializeNetwork(g_stSettings.m_strLocalIPAdres,
                            g_stSettings.m_strLocalNetmask,
                            g_stSettings.m_strGateway,
                            g_stSettings.m_strNameServer) )
  {
      if (g_stSettings.m_bTimeServerEnabled)
      {
        CLog::Log("start timeserver thread");
			  m_sntpClient.Create(); 
      }
      
			if (g_stSettings.m_bHTTPServerEnabled)
			{
        CLog::Log("start webserver");
				CSectionLoader::Load("LIBHTTP");
				m_pWebServer = new CWebServer();
				CStdString ipadres;
				CUtil::GetTitleIP(ipadres);
				m_pWebServer->Start(ipadres.c_str(), 80, "Q:\\web");
			} 

      if ( g_stSettings.m_bFTPServerEnabled)
			{
        CLog::Log("start ftpserver");
				m_pFileZilla = new CXBFileZilla("Q:\\");
				m_pFileZilla->Start();
			}

  }
  else
  {
    CLog::Log("initialize network failed");
  }

	// set filters
	g_graphicsContext.Get3DDevice()->SetTextureStageState(0, D3DTSS_MINFILTER, g_stSettings.m_minFilter );
	g_graphicsContext.Get3DDevice()->SetTextureStageState(0, D3DTSS_MAGFILTER, g_stSettings.m_maxFilter );

	g_graphicsContext.SetD3DDevice(m_pd3dDevice);
  CLog::Log("load default skin:[%s]",g_stSettings.szDefaultSkin);
	LoadSkin(g_stSettings.szDefaultSkin);

  CLog::Log("initializing skin");
  m_gWindowManager.Add(&m_guiHome);											// window id = 0
  m_gWindowManager.Add(&m_guiPrograms);									// window id = 1
	m_gWindowManager.Add(&m_guiPictures);									// window id = 2
  m_gWindowManager.Add(&m_guiMyFiles);									// window id = 3
	m_gWindowManager.Add(&m_guiMyVideo);									// window id = 6
	m_gWindowManager.Add(&m_guiSettings);									// window id = 4
	m_gWindowManager.Add(&m_guiSystemInfo);								// window id = 7
	m_gWindowManager.Add(&m_guiSettingsGeneral);					// window id = 8
	m_gWindowManager.Add(&m_guiSettingsScreen);						// window id = 9
	m_gWindowManager.Add(&m_guiSettingsUICalibration);		// window id = 10
	m_gWindowManager.Add(&m_guiSettingsScreenCalibration);	// window id = 11
	m_gWindowManager.Add(&m_guiSettingsSlideShow);				// window id = 12 slideshow:window id 2007
	m_gWindowManager.Add(&m_guiSettingsFilter);						// window id = 13
	m_gWindowManager.Add(&m_guiSettingsMusic);						// window id = 14
  m_gWindowManager.Add(&m_guiSettingsSubtitles);				// window id = 15
  m_gWindowManager.Add(&m_guiSettingsScreensaver);				// window id = 16
  m_gWindowManager.Add(&m_guiSettingsOSD);					// window id = 18
	m_gWindowManager.Add(&m_guiScripts);									// window id = 20
  m_gWindowManager.Add(&m_guiVideoGenre);								// window id = 21
  m_gWindowManager.Add(&m_guiVideoActors);							// window id = 22
	m_gWindowManager.Add(&m_guiVideoYear);							  // window id = 23
  m_gWindowManager.Add(&m_guiSettingsPrograms);					// window id = 24
	m_gWindowManager.Add(&m_guiVideoTitle);							  // window id = 25
  m_gWindowManager.Add(&m_guiSettingsCache);						// window id = 26
  m_gWindowManager.Add(&m_guiSettingsAutoRun);					// window id = 27
  m_gWindowManager.Add(&m_guiMyVideoPlayList);					// window id = 28
  m_gWindowManager.Add(&m_guiSettingsLCD);					    // window id = 29

  m_gWindowManager.Add(&m_guiDialogYesNo);							// window id = 100
  m_gWindowManager.Add(&m_guiDialogProgress);						// window id = 101
  m_gWindowManager.Add(&m_guiMyMusicPlayList);					// window id = 500
	m_gWindowManager.Add(&m_guiMyMusicSongs);							// window id = 501
  m_gWindowManager.Add(&m_guiMyMusicAlbum);							// window id = 502
	m_gWindowManager.Add(&m_guiMyMusicArtists);						// window id = 503
	m_gWindowManager.Add(&m_guiMyMusicGenres);						// window id = 504
	m_gWindowManager.Add(&m_guiMyMusicTop100);						// window id = 505
  m_gWindowManager.Add(&m_keyboard);										// window id = 1000
	m_gWindowManager.Add(&m_guiDialogSelect);							// window id = 2000
	m_gWindowManager.Add(&m_guiMusicInfo);								// window id = 2001
	m_gWindowManager.Add(&m_guiDialogOK);									// window id = 2002
	m_gWindowManager.Add(&m_guiVideoInfo);								// window id = 2003
	m_gWindowManager.Add(&m_guiScriptsInfo);							// window id = 2004
	m_gWindowManager.Add(&m_guiWindowFullScreen);					// window id = 2005
	m_gWindowManager.Add(&m_guiWindowVisualisation);			// window id = 2006
	m_gWindowManager.Add(&m_guiWindowSlideshow);					// window id = 2007
	m_gWindowManager.Add(&m_guiDialogFileStacking);				// window id = 2008
	m_gWindowManager.Add(&m_guiWindowOSD);						// window id = 2901
	m_gWindowManager.Add(&m_guiWindowScreensaver);				// window id = 2900 Screensaver
	m_gWindowManager.Add(&m_guiMyWeather);						    // window id = 2600 WEATHER
	m_gWindowManager.Add(&m_guiSettingsWeather);				  // window id = 17 WEATHER SETTINGS

	/* window id's 3000 - 3100 are reserved for python */
  CLog::Log("initializing virtual keyboard");	
  m_keyboard.Initialize();

	m_ctrDpad.SetDelays(g_stSettings.m_iMoveDelayController,g_stSettings.m_iRepeatDelayController);
	m_ctrIR.SetDelays(g_stSettings.m_iMoveDelayIR,g_stSettings.m_iRepeatDelayIR);

	m_gWindowManager.ActivateWindow(g_stSettings.m_iStartupWindow);
	CLog::Log("removing tempfiles");	
  CUtil::RemoveTempFiles();

	//	Start Thread for DVD Mediatype detection
  CLog::Log("start dvd mediatype detection");	
	m_DetectDVDType.Create( false);


  if (!m_bAllSettingsLoaded)
  {
    CLog::Log("settings not correct, show dialog");	
		CStdString test;
		CUtil::GetHomePath(test);
    m_guiDialogOK.SetHeading(279);
    m_guiDialogOK.SetLine(0,"Error while loading settings");
    m_guiDialogOK.SetLine(1,test);
    m_guiDialogOK.SetLine(2,L"");
    m_guiDialogOK.DoModal(g_stSettings.m_iStartupWindow);
  }
  if (g_stSettings.m_bLCDUsed)
  {
    if (g_stSettings.m_iLCDMode==LCD_MODE_NOTV)
    {
      // jump to my music when we're in NO tv mode
      m_gWindowManager.ActivateWindow(WINDOW_MUSIC_FILES);
    }
  }
  CLog::Log("initialize done");	
  CLCDFactory factory;
  g_lcd=factory.Create();
  g_lcd->Initialize();
  CUtil::InitGamma();
	return S_OK;
}

void CApplication::DelayLoadSkin()
{
	m_dwSkinTime = timeGetTime() + 2000;
	return;
}

void CApplication::LoadSkin(const CStdString& strSkin)
{
	m_dwSkinTime = 0;

	CStdString strHomePath;
	string strSkinPath = "Q:\\skin\\";
	strSkinPath+=strSkin;

  CLog::Log("  load skin from:%s",strSkinPath.c_str());	


  if ( IsPlaying() )
  {
    CLog::Log(" stop playing...");
    m_pPlayer->closefile();
    m_strCurrentFile="";
		delete m_pPlayer;
		m_pPlayer=NULL;
  }

  CLog::Log("  delete old skin...");
	m_guiWindowVideoOverlay.FreeResources();
	m_guiWindowVideoOverlay.ClearAll();

	m_guiMusicOverlay.FreeResources();
	m_guiMusicOverlay.ClearAll();
	
	m_gWindowManager.DeInitialize();
	g_TextureManager.Cleanup();

	g_fontManager.Clear();
  
  CLog::Log("  load fonts for skin...");
  g_graphicsContext.SetMediaDir(strSkinPath);
  g_fontManager.LoadFonts(strSkinPath+string("\\font.xml")) ;

	LARGE_INTEGER start;
	QueryPerformanceCounter(&start);

  CLog::Log("  load new skin...");
  if (!m_guiHome.Load( strSkinPath+"\\home.xml" ))
  {
    // failed to load home.xml
    // fallback to mediacenter skin
    if ( CUtil::cmpnocase(strSkin.c_str(),"mediacenter") !=0)
    {
      CLog::Log("failed to load home.xml for skin:%s, fallback to mediacenter skin", strSkin.c_str());
      LoadSkin("mediacenter");
      return;
    }
  }
  m_guiPrograms.Load( strSkinPath+"\\myprograms.xml" );  
	m_guiPictures.Load( strSkinPath+"\\mypics.xml" );  
	m_guiMyFiles.Load( strSkinPath+"\\myfiles.xml" );  
	m_guiMyVideo.Load( strSkinPath+"\\myvideo.xml" );  
  m_guiVideoGenre.Load( strSkinPath+"\\myvideogenre.xml" );  
  m_guiVideoActors.Load( strSkinPath+"\\myvideoactors.xml" );  
	m_guiVideoYear.Load( strSkinPath+"\\myvideoYear.xml" );  
  m_guiVideoTitle.Load( strSkinPath+"\\myvideoTitle.xml" );  
	m_guiSettings.Load( strSkinPath+"\\settings.xml" );  
  m_guiMyVideoPlayList.Load( strSkinPath+"\\myvideoplaylist.xml" );  
  m_guiSettingsLCD.Load( strSkinPath+"\\settingsLCD.xml" );  
	m_guiSystemInfo.Load( strSkinPath+"\\SettingsSystemInfo.xml" );  
	m_guiMusicInfo.Load( strSkinPath+"\\DialogAlbumInfo.xml" );  
	m_guiScriptsInfo.Load( strSkinPath+"\\DialogScriptInfo.xml" ); 
	m_guiSettingsGeneral.Load( strSkinPath+"\\SettingsGeneral.xml" );  
  m_guiSettingsPrograms.Load( strSkinPath+"\\SettingsPrograms.xml" );  
	m_guiDialogYesNo.Load( strSkinPath+"\\dialogYesNo.xml" );  
	m_guiDialogProgress.Load( strSkinPath+"\\dialogProgress.xml" );  
  m_guiMyMusicPlayList.Load( strSkinPath+"\\mymusicplaylist.xml" );
	m_guiMyMusicSongs.Load( strSkinPath+"\\mymusicsongs.xml" );
  m_guiMyMusicAlbum.Load( strSkinPath+"\\mymusicalbum.xml" );
	m_guiMyMusicArtists.Load( strSkinPath+"\\mymusicartists.xml" );
	m_guiMyMusicGenres.Load( strSkinPath+"\\mymusicgenres.xml" );
	m_guiMyMusicTop100.Load( strSkinPath+"\\mymusictop100.xml" );
	m_guiDialogSelect.Load( strSkinPath+"\\dialogSelect.xml" );  
	m_guiDialogOK.Load( strSkinPath+"\\dialogOK.xml" );  
	m_guiDialogFileStacking.Load( strSkinPath+"\\dialogFileStacking.xml" );  
	m_guiVideoInfo.Load( strSkinPath+"\\DialogVideoInfo.xml" );  
	m_guiMusicOverlay.Load( strSkinPath+"\\musicOverlay.xml" );  
	m_guiSettingsScreen.Load( strSkinPath+"\\settingsScreen.xml" );
	m_guiSettingsUICalibration.Load( strSkinPath+"\\settingsUICalibration.xml" );
	m_guiSettingsScreenCalibration.Load( strSkinPath+"\\settingsScreenCalibration.xml" );
	m_guiSettingsSlideShow.Load( strSkinPath+"\\SettingsSlideShow.xml" );
	m_guiSettingsScreensaver.Load( strSkinPath+"\\SettingsScreensaver.xml" );
	m_guiSettingsOSD.Load( strSkinPath+"\\SettingsOSD.xml" );
  m_guiSettingsAutoRun.Load( strSkinPath+"\\SettingsAutoRun.xml" );
	m_guiSettingsFilter.Load( strSkinPath+"\\SettingsFilter.xml" );
  m_guiSettingsCache.Load( strSkinPath+"\\SettingsCache.xml" );
	m_guiWindowVideoOverlay.Load( strSkinPath+"\\videoOverlay.xml" );
	m_guiWindowFullScreen.Load( strSkinPath+"\\videoFullScreen.xml" );
	m_guiScripts.Load( strSkinPath+"\\myscripts.xml");
	m_guiWindowVisualisation.Load( strSkinPath+"\\musicVisualisation.xml");
	m_guiSettingsMusic.Load( strSkinPath+"\\SettingsMusic.xml");
	m_guiWindowSlideshow.Load( strSkinPath+"\\slideshow.xml");
	m_guiSettingsSubtitles.Load( strSkinPath+"\\SettingsScreenSubtitles.xml");
	m_guiWindowScreensaver.SetID(WINDOW_SCREENSAVER);		// CB: Matrix Screensaver - saves us having to have our own XML file
	m_guiWindowOSD.Load( strSkinPath+"\\videoOSD.xml" );
	m_guiMyWeather.Load( strSkinPath+"\\myweather.xml");	//WEATHER
	m_guiSettingsWeather.Load(strSkinPath+"\\SettingsWeather.xml");	//WEATHER SETTINGS
	CGUIWindow::FlushReferenceCache(); // flush the cache so it doesn't use memory all the time

	LARGE_INTEGER end, freq;
	QueryPerformanceCounter(&end);
	QueryPerformanceFrequency(&freq);
	CLog::DebugLog("Load Skin XML: %.2fms", 1000.f * (end.QuadPart - start.QuadPart) / freq.QuadPart);

  CLog::Log("  initialize new skin...");
	m_guiMusicOverlay.AllocResources();
	m_guiWindowVideoOverlay.AllocResources();
	m_gWindowManager.AddMsgTarget(this);
	m_gWindowManager.AddMsgTarget(&g_playlistPlayer);
	m_gWindowManager.SetCallback(*this);
  m_gWindowManager.Initialize();
  CLog::Log("  skin loaded...");
}


void CApplication::Render()
{
	// dont show GUI when playing full screen video
	if (m_gWindowManager.GetActiveWindow() == WINDOW_FULLSCREEN_VIDEO)
	{
		if ( g_graphicsContext.IsFullScreenVideo() ) 
		{
			// set soften on/off
			m_pd3dDevice->SetSoftDisplayFilter(g_stSettings.m_bSoften);
			// set the flicker filter - Changed by JM to improve subtitle flicker (using 1)
			// HDTV modes may not need this, though.
			m_pd3dDevice->SetFlickerFilter(g_stSettings.m_bSoften ? 5 : 1);
			
      if (m_pPlayer)
      {
        if (m_pPlayer->IsPaused()) 
        {
	        g_graphicsContext.Lock();
          m_pd3dDevice->Clear( 0L, NULL, D3DCLEAR_TARGET|D3DCLEAR_ZBUFFER|D3DCLEAR_STENCIL, 0x00010001, 1.0f, 0L );
					extern void xbox_video_render_update();
					xbox_video_render_update();
          RenderFullScreen();
          m_gWindowManager.Render();
	        m_pd3dDevice->BlockUntilVerticalBlank();      
	        m_pd3dDevice->Present( NULL, NULL, NULL, NULL );
	        g_graphicsContext.Unlock();
          return;
        }
      }
      Sleep(50);
      ResetScreenSaver();
      return;
		}
	}

  // enable/disable video overlay window 
  if (IsPlayingVideo() && m_gWindowManager.GetActiveWindow() != WINDOW_FULLSCREEN_VIDEO && !m_bScreenSave)
	{
		g_graphicsContext.EnablePreviewWindow(true);
	}
	else
	{
		g_graphicsContext.EnablePreviewWindow(false);
	}

	g_graphicsContext.Lock();

	// draw GUI (always enable soften filter when displaying the UI)
  m_pd3dDevice->Clear( 0L, NULL, D3DCLEAR_TARGET|D3DCLEAR_ZBUFFER, 0x00010001, 1.0f, 0L );
	m_pd3dDevice->SetSoftDisplayFilter(true);
	m_pd3dDevice->SetFlickerFilter(5);

  // render current window/dialog
  m_gWindowManager.Render();

	// check if we're playing a file
	if (  m_bOverlayEnabled )
	{
    // if we're playing a movie
		if ( IsPlayingVideo() && m_gWindowManager.GetActiveWindow() != WINDOW_FULLSCREEN_VIDEO)
		{
      // then show video overlay window
			m_guiWindowVideoOverlay.Render();
		}
		else if ( IsPlayingAudio() ) 
		{
      // audio show audio overlay window
			m_guiMusicOverlay.Render();
		}
	}

  

	{
    // free memory if we got les then 10megs free ram
	  MEMORYSTATUS stat;
	  GlobalMemoryStatus(&stat);
		DWORD dwMegFree=stat.dwAvailPhys / (1024*1024);
		if (dwMegFree <= 10)
		{
			g_TextureManager.Flush();
		}

    // if we're recording an audio stream then show blinking REC
		if (IsPlayingAudio())
		{
			if (m_pPlayer->IsRecording() )
			{
        static iBlinkRecord=0;
				CGUIFont* pFont=g_fontManager.GetFont("font13");
				if (pFont)
				{
					iBlinkRecord++;
					if (iBlinkRecord>25)
						pFont->DrawText(60, 50, 0xffff0000, L"REC"); //Draw REC in RED
					if (iBlinkRecord> 50)
						iBlinkRecord=0;
				}
			}
		}

		// If we have the remote codes enabled, then show them
		if (g_stSettings.m_bDisplayRemoteCodes)
		{
			XBIR_REMOTE* pRemote	= &m_DefaultIR_Remote;
		    static iRemoteCode=0;
			static iShowRemoteCode=0;
			if (pRemote->wButtons)
			{
				iRemoteCode = 255-pRemote->wButtons;	// remote OBC code is 255-wButtons
				iShowRemoteCode = 50;
			}
			if (iShowRemoteCode > 0)
			{
				CStdStringW wszText;
				wszText.Format(L"Remote Code: %i",iRemoteCode);
				CGUIFont* pFont=g_fontManager.GetFont("font13");
				if (pFont)
				{
#ifdef _DEBUG		
					pFont->DrawText( 60, 60, 0xffffffff, wszText);
#else
					pFont->DrawText( 60, 40, 0xffffffff, wszText);
#endif
				}
				iShowRemoteCode--;
			}
		}

#ifdef _DEBUG		
    // in debug mode, show freememory
		CStdStringW wszText;
		wszText.Format(L"FreeMem %i/%iMB",stat.dwAvailPhys  /(1024*1024),
																					  stat.dwTotalPhys  /(1024*1024)  );
    
    CGUIFont* pFont=g_fontManager.GetFont("font13");
    if (pFont)
    {
	    pFont->DrawText( 60, 40, 0xffffffff, wszText);
    }
#endif

  }
  // Present the backbuffer contents to the display
	m_pd3dDevice->BlockUntilVerticalBlank();      
	m_pd3dDevice->Present( NULL, NULL, NULL, NULL );
	g_graphicsContext.Unlock();

}

void CApplication::OnKey(CKey& key)
{
	CAction action;
  // a key has been pressed. 
  // Reset the screensaver timer
  // but not for the analog thumbsticks
  if (key.GetButtonCode() != KEY_BUTTON_LEFT_THUMB_STICK &&
      key.GetButtonCode() != KEY_BUTTON_RIGHT_THUMB_STICK)
  {
    // reset harddisk spindown timer
		m_bSpinDown=false;
    

    ResetScreenSaver();

	  m_bInactive=false;		// reset the inactive flag as a key has been pressed
    // if Screen saver is active
	  if (m_bScreenSave)		
	  {
      // disable screensaver
		  m_bScreenSave = false;	
          
	    // if matrix trails screensaver is active
	    int iWin = m_gWindowManager.GetActiveWindow();
		  if (iWin  == WINDOW_SCREENSAVER)	
		  {
        // then show previous window
			  m_gWindowManager.PreviousWindow();
		  }
		  else										
		  {
        // Fade to dim or black screensaver is active
        // just un-dim the screen
			  m_pd3dDevice->SetGammaRamp(0, &m_OldRamp);	// put the old gamma ramp back in place
		  }
      return;
	  }
  }

	// get the current active window 
	int iWin = m_gWindowManager.GetActiveWindow();
  if (iWin==WINDOW_FULLSCREEN_VIDEO)
  {
    // current active window is full screen video.
    // check if OSD is visible
    CGUIWindowFullScreen *pFSWin = (CGUIWindowFullScreen *)m_gWindowManager.GetWindow(WINDOW_FULLSCREEN_VIDEO);
    if ( pFSWin->OSDVisible() )
    {
      // yes then use OSD section of keymap.xml to map key->action
      g_buttonTranslator.GetAction(WINDOW_OSD, key, action);
    }
    else 
    {
      // no then use the fullscreen window section of keymap.xml to map key->action
      g_buttonTranslator.GetAction(iWin, key, action);
    }
  }
  else 
  {
    // current active window isnt the fullscreen window
    // just use corresponding section from keymap.xml
    // to map key->action
    g_buttonTranslator.GetAction(iWin, key, action);
  }

	// special case for switching between GUI & fullscreen mode. (only if we're playing a movie)
	if ( IsPlayingVideo() )
	{
    // dont allow switching between GUI & fullscreen mode if a dialog is onscreen
    if ( !m_gWindowManager.IsRouted())
    {
      // if user wants 2 show fullscreen window
		  if (action.wID == ACTION_SHOW_GUI && m_gWindowManager.GetActiveWindow() != WINDOW_FULLSCREEN_VIDEO)
		  {
			  // then switch to fullscreen mode
			  m_gWindowManager.ActivateWindow(WINDOW_FULLSCREEN_VIDEO);
			  g_TextureManager.Flush();
        g_TextureManager.Dump();
			  return;
		  }
    }
	}
	else 
	{
    // special case for switching between GUI & visualisation mode. (only if we're playing an audio song)
		if (IsPlayingAudio() )
		{
      // if user wants 2 show visualisation window
			if (action.wID == ACTION_SHOW_GUI && m_gWindowManager.GetActiveWindow() != WINDOW_VISUALISATION)
			{
        // dont allow switching between GUI & visualisation mode if a dialog is onscreen
        if ( !m_gWindowManager.IsRouted())
        {
          // then switch to visualisation
				  m_gWindowManager.ActivateWindow(WINDOW_VISUALISATION);
        }
				return;
			}
		}
	}

  // in normal case
  // just pass the action to the current window and let it handle it
	m_gWindowManager.OnAction(action);

	/* handle extra global presses */
  if (iWin != WINDOW_FULLSCREEN_VIDEO)
  {
    // stop : stops playing current audio song
    if (action.wID == ACTION_STOP)
	  {
      StopPlaying();
	  }  

    // pause : pauses current audio song
	  if (action.wID == ACTION_PAUSE)
	  {
		  if (m_pPlayer)
      {
        m_pPlayer->Pause();
        if (!m_pPlayer->IsPaused())
        {
          SetPlaySpeed(1);
        }
      }
	  }
    

    // previous : play previous song from playlist
	  if (action.wID == ACTION_PREV_ITEM)
	  {
		  g_playlistPlayer.PlayPrevious();
	  }

    // next : play next song from playlist
	  if (action.wID == ACTION_NEXT_ITEM)
	  {
		  g_playlistPlayer.PlayNext();
	  }

    if ( IsPlaying())
    {
      if (!m_pPlayer->IsPaused())
      {
        // if we do a FF/RW in my music then map PLAY action togo back to normal speed
        if (action.wID == ACTION_MUSIC_PLAY)
        {
          
          if (m_iPlaySpeed!=1)
          {
            SetPlaySpeed(1);
          }
        }
        if (action.wID == ACTION_MUSIC_FORWARD || action.wID == ACTION_MUSIC_REWIND)
        {
					if (m_strCurrentPlayer == "sid")
					{
						// sid uses these to track skip
						m_pPlayer->Seek(action.wID == ACTION_MUSIC_FORWARD);
					}
					else
					{
						int iPlaySpeed=m_iPlaySpeed;
						if (action.wID == ACTION_MUSIC_REWIND && iPlaySpeed == 1) // Enables Rewinding
							iPlaySpeed *=-2;
						else if (action.wID == ACTION_MUSIC_REWIND && iPlaySpeed > 1) //goes down a notch if you're FFing
							iPlaySpeed /=2;
						else if (action.wID == ACTION_MUSIC_FORWARD && iPlaySpeed < 1) //goes up a notch if you're RWing
							iPlaySpeed /= 2;
						else 
							iPlaySpeed *= 2;

						if (action.wID == ACTION_MUSIC_FORWARD && iPlaySpeed == -1) //sets iSpeed back to 1 if -1 (didn't plan for a -1)
							iPlaySpeed = 1;
						if (iPlaySpeed > 32 || iPlaySpeed < -32)
							iPlaySpeed = 1;     

						SetPlaySpeed(iPlaySpeed);
					}
        }
      }
    }
  }
}
void CApplication::UpdateLCD()
{
  static lTickCount=0;
  
  if (!g_stSettings.m_bLCDUsed) return;
  long lTimeOut=1000;
  if ( m_iPlaySpeed != 1) lTimeOut=0;
  if ( ((long)GetTickCount()-lTickCount) >=lTimeOut)
  {
    CStdString strTime;
    CStdString strIcon;
    CStdString strLine;
    if (IsPlayingVideo())
    {
      // line 0: play symbol current time/total time
      // line 1: movie filename / title
      // line 2: genre
      // line 3: year
      CStdString strTotalTime;
      unsigned int tmpvar = g_application.m_pPlayer->GetTotalTime();
		  if(tmpvar != 0)
		  {
			  int ihour = tmpvar / 3600;
			  int imin  = (tmpvar-ihour*3600) / 60;
			  int isec = (tmpvar-ihour*3600) % 60;
        strTotalTime.Format("/%2.2d:%2.2d:%2.2d", ihour,imin,isec);
		  }
      else 
      {
			  strTotalTime=" ";
      }

      __int64 lPTS=10*g_application.m_pPlayer->GetTime();
      int hh = (int)(lPTS / 36000) % 100;
      int mm = (int)((lPTS / 600) % 60);
      int ss = (int)((lPTS /  10) % 60);
      if (hh >=1)
	    {
        strTime.Format("%02.2i:%02.2i:%02.2i",hh,mm,ss);
	    }
	    else
	    {
		    strTime.Format("%02.2i:%02.2i",mm,ss);
	    }
      if (m_iPlaySpeed < 1) 
        strIcon.Format("\3RW:%ix", m_iPlaySpeed);
      else if (m_iPlaySpeed > 1) 
        strIcon.Format("\4FF:%ix", m_iPlaySpeed);
      else if (m_pPlayer->IsPaused())
        strIcon.Format("\7");
      else
        strIcon.Format("\5");
      strLine.Format("%s %s%s", strIcon.c_str(), strTime.c_str(), strTotalTime.c_str());
      g_lcd->SetLine(0,strLine);

      strLine=CUtil::GetFileName(m_strCurrentFile);
      int iLine=1;
      if (m_tagCurrentMovie.m_strTitle!="") strLine=m_tagCurrentMovie.m_strTitle;
      g_lcd->SetLine(iLine++,strLine);

      if (iLine<4 && m_tagCurrentMovie.m_strGenre!="") g_lcd->SetLine(iLine++,m_tagCurrentMovie.m_strGenre);
      if (iLine<4 && m_tagCurrentMovie.m_iYear>1900) 
      {
        strLine.Format("%i", m_tagCurrentMovie.m_iYear);
        g_lcd->SetLine(iLine++,strLine);
      }
      if (iLine < 4)
      {
        MEMORYSTATUS stat;
        GlobalMemoryStatus(&stat);
        DWORD dwMegFree=stat.dwAvailPhys / (1024*1024);
        strTime.Format("Freemem:%i meg", dwMegFree);
        g_lcd->SetLine(iLine++,strTime);

      }
      while (iLine < 4) g_lcd->SetLine(iLine++,"");

    }
    else if (IsPlayingAudio())
    {
      // Show:
      // line 0: play symbol current time/total time
      // line 1: song title
      // line 2: artist
      // line 3: release date
      __int64 lPTS=g_application.m_pPlayer->GetPTS();
      int hh = (int)(lPTS / 36000) % 100;
      int mm = (int)((lPTS / 600) % 60);
      int ss = (int)((lPTS /  10) % 60);
      if (hh >=1)
	    {
        strTime.Format("%02.2i:%02.2i:%02.2i",hh,mm,ss);
	    }
	    else
	    {
		    strTime.Format("%02.2i:%02.2i",mm,ss);
	    }
      if (m_iPlaySpeed < 1) 
        strIcon.Format("\3RW:%ix", m_iPlaySpeed);
      else if (m_iPlaySpeed > 1) 
        strIcon.Format("\4FF:%ix", m_iPlaySpeed);
      else if (m_pPlayer->IsPaused())
        strIcon.Format("\7");
      else
        strIcon.Format("\5");
      strLine.Format("%s %s", strIcon.c_str(), strTime.c_str());
      
      int iLine=1;
      if (m_tagCurrentSong.Loaded())
      {
        int iDuration=m_tagCurrentSong.GetDuration();
        if (iDuration>0)
        {
          CStdString strDuration;
          CUtil::SecondsToHMSString(iDuration, strDuration);
          strLine.Format("%s %s/%s", strIcon.c_str(), strTime.c_str(),strDuration.c_str());
        }
        g_lcd->SetLine(0,strLine);
        strLine=m_tagCurrentSong.GetTitle();
        if (strLine=="") strLine=CUtil::GetFileName(m_strCurrentFile);
        if (iLine < 4 && strLine!="") g_lcd->SetLine(iLine++,strLine);
        strLine=m_tagCurrentSong.GetArtist();
        if (iLine < 4 && strLine!="") g_lcd->SetLine(iLine++,strLine);
        SYSTEMTIME systemtime;
        m_tagCurrentSong.GetReleaseDate(systemtime);
        if (iLine < 4 && systemtime.wYear>=1900)
        {
          strLine.Format("%i", systemtime.wYear);
          g_lcd->SetLine(iLine++,strLine);
        }
        while (iLine < 4) g_lcd->SetLine(iLine++,"");
      }
      else
      {
        g_lcd->SetLine(0,strLine);
        g_lcd->SetLine(1,CUtil::GetFileName(m_strCurrentFile));
        g_lcd->SetLine(2,"");
        g_lcd->SetLine(3,"");
      }
    }
    else
    {
      if (g_stSettings.m_iLCDMode==LCD_MODE_NORMAL)
      {
        // line 0: XBMC running...
        // line 1: time/date
        // line 2: free memory (megs)
        // line 3: GUI resolution
        g_lcd->SetLine(0,"XBMC running...");
        SYSTEMTIME time;
	      GetLocalTime(&time);
        strTime.Format("%02.2i:%02.2i:%02.2i %02.2i-%02.2i-%02.2i", time.wHour,time.wMinute,time.wSecond,time.wDay,time.wMonth,time.wYear);
        g_lcd->SetLine(1,strTime);
	      MEMORYSTATUS stat;
	      GlobalMemoryStatus(&stat);
		    DWORD dwMegFree=stat.dwAvailPhys / (1024*1024);
        strTime.Format("Freemem:%i meg", dwMegFree);
        g_lcd->SetLine(2,strTime);
		    int  iResolution=g_graphicsContext.GetVideoResolution();
		    strTime.Format("%ix%i %s", g_settings.m_ResInfo[iResolution].iWidth, g_settings.m_ResInfo[iResolution].iHeight, g_settings.m_ResInfo[iResolution].strMode);
        g_lcd->SetLine(3,strTime);
      }
      if (g_stSettings.m_iLCDMode==LCD_MODE_NOTV)
      {
        // line 0: window name like   My music/songs
        // line 1: current control or selected item
        // line 2: time/date
        // line 3: free memory (megs)
        CStdString strTmp;
        int iWin=m_gWindowManager.GetActiveWindow();        
        CGUIWindow* pWindow=m_gWindowManager.GetWindow(iWin);
        if (pWindow)
        {
          CStdString strLine;
          wstring wstrLine;
          wstrLine=g_localizeStrings.Get(10000+iWin);
          CUtil::Unicode2Ansi(wstrLine,strLine);
          g_lcd->SetLine(0,strLine);

          int iControl=pWindow->GetFocusedControl();
          CGUIControl* pControl=(CGUIControl* )pWindow->GetControl(iControl);
          if (pControl)
          {
            CGUIButtonControl* pButton=dynamic_cast<CGUIButtonControl*>(pControl);
            if (pButton)
              g_lcd->SetLine(1,pButton->GetLabel());
            CGUISpinControl* pSpinControl=dynamic_cast<CGUISpinControl*>(pControl);
            if (pSpinControl)
            {
              strTmp.Format("%i/%i", 1+pSpinControl->GetValue(), pSpinControl->GetMaximum());
              g_lcd->SetLine(1,strTmp);
            }
            CGUIListControl* pListControl=dynamic_cast<CGUIListControl*>(pControl);
            if (pListControl)
            {
              pListControl->GetSelectedItem(strTmp);
              g_lcd->SetLine(1,strTmp);
            }
            CGUIThumbnailPanel* pThumbControl=dynamic_cast<CGUIThumbnailPanel*>(pControl);
            if (pThumbControl)
            {
              pThumbControl->GetSelectedItem(strTmp);
              g_lcd->SetLine(1,strTmp);
            }
          }
          else g_lcd->SetLine(1," ");
          SYSTEMTIME time;
	        GetLocalTime(&time);
          strLine.Format("%02.2i:%02.2i:%02.2i %02.2i-%02.2i-%02.2i", time.wHour,time.wMinute,time.wSecond,time.wDay,time.wMonth,time.wYear);
          g_lcd->SetLine(2,strLine);
	        MEMORYSTATUS stat;
	        GlobalMemoryStatus(&stat);
		      DWORD dwMegFree=stat.dwAvailPhys / (1024*1024);
          strLine.Format("Freemem:%i meg", dwMegFree);
          g_lcd->SetLine(3,strLine);

        }
      }
    }
    lTickCount=GetTickCount();
  }
}

void CApplication::FrameMove()
{
  UpdateLCD();
  // read raw input from controller & remote control
	ReadInput();
	XBIR_REMOTE* pRemote	= &m_DefaultIR_Remote;
	XBGAMEPAD*  pGamepad	= &m_DefaultGamepad;

	WORD wButtons = pGamepad->wButtons;
	WORD wRemotes = pRemote->wButtons;
	WORD wDpad = wButtons&(XINPUT_GAMEPAD_DPAD_UP|XINPUT_GAMEPAD_DPAD_DOWN|XINPUT_GAMEPAD_DPAD_LEFT|XINPUT_GAMEPAD_DPAD_RIGHT);

	BYTE bLeftTrigger = pGamepad->bAnalogButtons[XINPUT_GAMEPAD_LEFT_TRIGGER];
	BYTE bRightTrigger = pGamepad->bAnalogButtons[XINPUT_GAMEPAD_RIGHT_TRIGGER];
	
  // pass them through the delay
	WORD wDir = m_ctrDpad.DpadInput(wDpad,0!=bLeftTrigger,0!=bRightTrigger);
	wRemotes=m_ctrIR.IRInput(wRemotes);

	bool bGotKey=false;

  // map all controller & remote actions to their keys
	if (pGamepad->fX1 || pGamepad->fY1)
	{
		bGotKey=true;
		CKey key(KEY_BUTTON_LEFT_THUMB_STICK,bLeftTrigger,bRightTrigger,pGamepad->fX1,pGamepad->fY1,pGamepad->fX2,pGamepad->fY2);
		OnKey(key);   
	}

	if (pGamepad->fX2 || pGamepad->fY2)
	{
		bGotKey=true;
		CKey key(KEY_BUTTON_RIGHT_THUMB_STICK,bLeftTrigger,bRightTrigger,pGamepad->fX1,pGamepad->fY1,pGamepad->fX2,pGamepad->fY2);
		OnKey(key);   
	}

	if ( wDir & DC_LEFTTRIGGER)
  {
		bGotKey=true;
    CKey key(KEY_BUTTON_LEFT_TRIGGER,bLeftTrigger,bRightTrigger,pGamepad->fX1,pGamepad->fY1,pGamepad->fX2,pGamepad->fY2);
    OnKey(key);   
  }
  if ( wDir & DC_RIGHTTRIGGER)
  {
		bGotKey=true;
    CKey key(KEY_BUTTON_RIGHT_TRIGGER,bLeftTrigger,bRightTrigger,pGamepad->fX1,pGamepad->fY1,pGamepad->fX2,pGamepad->fY2);
    OnKey(key);   
  }
  if ( wDir & DC_LEFT )
  {
		bGotKey=true;
    CKey key(KEY_BUTTON_DPAD_LEFT,bLeftTrigger,bRightTrigger,pGamepad->fX1,pGamepad->fY1,pGamepad->fX2,pGamepad->fY2);
    OnKey(key);   
  }
  if ( wDir & DC_RIGHT)
  {
		bGotKey=true;
    CKey key(KEY_BUTTON_DPAD_RIGHT,bLeftTrigger,bRightTrigger,pGamepad->fX1,pGamepad->fY1,pGamepad->fX2,pGamepad->fY2);
    OnKey(key);   
  }
  if ( wDir & DC_UP )
  {
		bGotKey=true;
    CKey key(KEY_BUTTON_DPAD_UP,bLeftTrigger,bRightTrigger,pGamepad->fX1,pGamepad->fY1,pGamepad->fX2,pGamepad->fY2);
    OnKey(key);   
  }
  if ( wDir & DC_DOWN )
  {
		bGotKey=true;
    CKey key(KEY_BUTTON_DPAD_DOWN,bLeftTrigger,bRightTrigger,pGamepad->fX1,pGamepad->fY1,pGamepad->fX2,pGamepad->fY2);
    OnKey(key);   
  }

  
	if ( pGamepad->wPressedButtons & XINPUT_GAMEPAD_BACK )
  {
		bGotKey=true;
    CKey key(KEY_BUTTON_BACK,bLeftTrigger,bRightTrigger,pGamepad->fX1,pGamepad->fY1,pGamepad->fX2,pGamepad->fY2);
    OnKey(key);   
  }
	if ( pGamepad->wPressedButtons & XINPUT_GAMEPAD_START)
  {
		bGotKey=true;
    CKey key(KEY_BUTTON_START,bLeftTrigger,bRightTrigger,pGamepad->fX1,pGamepad->fY1,pGamepad->fX2,pGamepad->fY2);
    OnKey(key);   
  }

  if ( pGamepad->wPressedButtons & XINPUT_GAMEPAD_LEFT_THUMB)
  {
	  bGotKey=true;
	  CKey key(KEY_BUTTON_LEFT_THUMB_BUTTON,bLeftTrigger,bRightTrigger,pGamepad->fX1,pGamepad->fY1,pGamepad->fX2,pGamepad->fY2);
	  OnKey(key);
  }

    if ( pGamepad->wPressedButtons & XINPUT_GAMEPAD_RIGHT_THUMB)
  {
	  bGotKey=true;
	  CKey key(KEY_BUTTON_RIGHT_THUMB_BUTTON,bLeftTrigger,bRightTrigger,pGamepad->fX1,pGamepad->fY1,pGamepad->fX2,pGamepad->fY2);
	  OnKey(key);
  }


	if (pGamepad->bPressedAnalogButtons[XINPUT_GAMEPAD_A])
  {
		bGotKey=true;
    CKey key(KEY_BUTTON_A,bLeftTrigger,bRightTrigger,pGamepad->fX1,pGamepad->fY1,pGamepad->fX2,pGamepad->fY2);
    OnKey(key);   
  }
	if (pGamepad->bPressedAnalogButtons[XINPUT_GAMEPAD_B])
  {
		bGotKey=true;
    CKey key(KEY_BUTTON_B,bLeftTrigger,bRightTrigger,pGamepad->fX1,pGamepad->fY1,pGamepad->fX2,pGamepad->fY2);
    OnKey(key);
  }
  
	if (pGamepad->bPressedAnalogButtons[XINPUT_GAMEPAD_X])
  {
		bGotKey=true;
    CKey key(KEY_BUTTON_X,bLeftTrigger,bRightTrigger,pGamepad->fX1,pGamepad->fY1,pGamepad->fX2,pGamepad->fY2);
    OnKey(key);
		
  }
	if (pGamepad->bPressedAnalogButtons[XINPUT_GAMEPAD_Y])
  {
		bGotKey=true;
    CKey key(KEY_BUTTON_Y,bLeftTrigger,bRightTrigger,pGamepad->fX1,pGamepad->fY1,pGamepad->fX2,pGamepad->fY2);
    OnKey(key);   
  } 
	if (pGamepad->bPressedAnalogButtons[XINPUT_GAMEPAD_BLACK])
  {
		bGotKey=true;
    CKey key(KEY_BUTTON_BLACK,bLeftTrigger,bRightTrigger,pGamepad->fX1,pGamepad->fY1,pGamepad->fX2,pGamepad->fY2);
    OnKey(key);   
  } 
	if (pGamepad->bPressedAnalogButtons[XINPUT_GAMEPAD_WHITE])
  {
		bGotKey=true;
    CKey key(KEY_BUTTON_WHITE,bLeftTrigger,bRightTrigger,pGamepad->fX1,pGamepad->fY1,pGamepad->fX2,pGamepad->fY2);
    OnKey(key);   
  } 
	
  switch (wRemotes)
	{
		// 0 is invalid
		case 0:
			break;
		// Map all other keys unchanged
		default:
		{
			bGotKey=true;
			CKey key(wRemotes);
			OnKey(key);   
			break;
		}
	}


}


void CApplication::Stop()
{
  try
  {
    CLog::Log("stop all");
	  if (m_pWebServer)
	  {
      CLog::Log("stop webserver");
		  m_pWebServer->Stop();
		  delete m_pWebServer;
		  m_pWebServer = NULL;
	  }

	  if (m_pPlayer)
	  {
      CLog::Log("stop mplayer");
		  delete m_pPlayer;
		  m_pPlayer=NULL;
	  }

    //g_lcd->StopThread();
    CLog::Log("stop python");
	  g_applicationMessenger.Cleanup();
	  m_pythonParser.FreeResources();
    
    CLog::Log("stop dvd detect media");
	  m_DetectDVDType.StopThread();

    CLog::Log("stop LCD");
    g_lcd->Stop();
    

    CLog::Log("stop time server");
	  m_sntpClient.StopThread();

    CLog::Log("unload skin");
	  m_guiMusicOverlay.FreeResources();
	  m_guiWindowVideoOverlay.FreeResources();
	  m_guiWindowOSD.FreeResources();
	  g_fontManager.Clear();
	  m_gWindowManager.DeInitialize();
	  g_TextureManager.Cleanup();

    CLog::Log("unload sections");
	  CSectionLoader::UnloadAll();
    CLog::Log("destroy");
	  Destroy();
    CLog::Log("stopped");
  }
  catch(...)
  {
  }
}

bool CApplication::PlayFile(const CStdString& strFile, bool bRestart)
{
  CStdString strExtension;
  CUtil::GetExtension(strFile,strExtension);
  strExtension.ToLower();
  if (strExtension==".m3u") return false;
  if (strExtension==".b4s") return false;


  m_tagCurrentSong.SetLoaded(false);
  m_tagCurrentMovie.Reset();
  m_iPlaySpeed=1;
  if (!bRestart)
  {
    OutputDebugString("new file set audiostream:0\n");
    g_stSettings.m_iAudioStream=0;
    g_stSettings.m_bNoCache=false;
    g_stSettings.m_bNonInterleaved=false;
    m_iAudioStreamIDX=-1;

    // switch 2 default settings...
    g_settings.m_iBrightness=50;
    g_settings.m_iContrast=50;
    g_settings.m_iGamma=20;
    
  }
  m_strCurrentFile=strFile;
  
	CURL url(m_strCurrentFile);
	CStdString strNewPlayer = "mplayer";
	if ( url.GetProtocol() == "cdda")
	{
		strNewPlayer = "cdda";
	}
	else if (ModPlayer::IsSupportedFormat(url.GetFileType()))
	{
		strNewPlayer = "mod";
	}
	else if (url.GetFileType() == "sid")
	{
		strNewPlayer = "sid";
	}
	if (m_pPlayer)
	{
		if (1||m_strCurrentPlayer != strNewPlayer || !CUtil::IsAudio(m_strCurrentFile) )
		{
			delete m_pPlayer;
			m_pPlayer=NULL;
		}
	}

	m_strCurrentPlayer=strNewPlayer;
	if (!m_pPlayer)
	{
		CPlayerCoreFactory factory;
		m_pPlayer = factory.CreatePlayer(strNewPlayer,*this);
	}
  
  bool bResult=m_pPlayer->openfile(m_strCurrentFile);
	if (bResult) 
	{
		m_guiMusicOverlay.SetCurrentFile(m_strCurrentFile);
		m_guiWindowVideoOverlay.SetCurrentFile(m_strCurrentFile);

	
	  m_dwIdleTime=timeGetTime();

    // if file happens to contain video stream
    if ( IsPlayingVideo())
    {
      // and we're not in fullscreen video mode yet
      if (m_gWindowManager.GetActiveWindow() != WINDOW_FULLSCREEN_VIDEO)
      {
        // then switch to fullscreen video mode
        g_TextureManager.Flush();
		    g_graphicsContext.SetFullScreenVideo(true);
		    m_gWindowManager.ActivateWindow(WINDOW_FULLSCREEN_VIDEO);
      }
      
    }
  }
	return bResult;
}

void CApplication::OnPlayBackEnded()
{
  //playback ended
  m_iPlaySpeed=1;
  OutputDebugString("Playback has finished\n");
	CGUIMessage msg( GUI_MSG_PLAYBACK_ENDED, 0, 0, 0, 0, NULL );
	m_gWindowManager.SendThreadMessage( msg );
}

void CApplication::OnPlayBackStarted()
{
  //spin down harddisk when the current file being played is not on local harddrive and 
  //duration is more then spindown timeoutsetting or duration is unknown (streams)
  if ( 
    !CUtil::IsHD(m_strCurrentFile) && 
    (
      (m_pPlayer->GetTotalTime() <= 0) || 
      (m_pPlayer->GetTotalTime() > g_stSettings.m_iHDSpinDownTime*60)
    )
  )
  {
      m_bSpinDown      = true;
      m_dwSpinDownTime = 0;
	}
}

void CApplication::EnableOverlay()
{
	m_bOverlayEnabled=true;
}
void CApplication::DisableOverlay()
{
	m_bOverlayEnabled=false;
}

bool CApplication::IsPlaying() const
{
	if (!m_pPlayer) return false;
	if (!m_pPlayer->IsPlaying()) return false;
	return true;
}

bool CApplication::IsPlayingAudio() const 
{
	if (!m_pPlayer) return false;
	if (!m_pPlayer->IsPlaying()) return false;
	if (m_pPlayer->HasVideo()) return false;
	if (m_pPlayer->HasAudio()) return true;
	return false;
}

bool CApplication::IsPlayingVideo() const 
{
	if (!m_pPlayer) return false;
	if (!m_pPlayer->IsPlaying()) return false;
	if (m_pPlayer->HasVideo()) return true;
	return false;
}


void CApplication::StopPlaying()
{
  int iWin = m_gWindowManager.GetActiveWindow();
  if ( IsPlaying() )
  {
    m_pPlayer->closefile();
	  //	turn off visualisation window when stopping
	  if (iWin==WINDOW_VISUALISATION)
		  m_gWindowManager.PreviousWindow();
  }
  CGUIMessage msg( GUI_MSG_PLAYBACK_STOPPED, 0, 0, 0, 0, NULL );
  m_gWindowManager.SendThreadMessage( msg );
}


bool CApplication::NeedRenderFullScreen()
{
	if (m_gWindowManager.GetActiveWindow() == WINDOW_FULLSCREEN_VIDEO)
	{
		CGUIWindowFullScreen *pFSWin = (CGUIWindowFullScreen *)m_gWindowManager.GetWindow(WINDOW_FULLSCREEN_VIDEO);
    if (!pFSWin) return false;
		return pFSWin->NeedRenderFullScreen();
	}
  return false;
}
void CApplication::RenderFullScreen()
{
	if (m_gWindowManager.GetActiveWindow() == WINDOW_FULLSCREEN_VIDEO)
	{
		CGUIWindowFullScreen *pFSWin = (CGUIWindowFullScreen *)m_gWindowManager.GetWindow(WINDOW_FULLSCREEN_VIDEO);
    if (!pFSWin) return ;
		pFSWin->RenderFullScreen();
	}
}

void CApplication::ResetScreenSaver()
{
  if (m_bInactive) 
  {
    m_dwSaverTick=timeGetTime();	// Start the timer going ...
  }
}

void CApplication::CheckScreenSaver()
{
	D3DGAMMARAMP Ramp;
	FLOAT fFadeLevel;

  if ( m_gWindowManager.IsRouted()) return;
  if (g_stSettings.m_iLCDMode==LCD_MODE_NOTV) return;
	if (!m_bInactive)
	{
		if (IsPlayingVideo() && !m_pPlayer->IsPaused())	// are we playing a movie and is it paused?
		{
			m_bInactive=false;
		}
		else if (IsPlayingAudio())	// are we playing some music?
		{
			if (m_gWindowManager.GetActiveWindow() == WINDOW_VISUALISATION)
			{
				m_bInactive=false;	// visualisation is on, so we cannot show a screensaver
			}
			else
			{
				m_bInactive=true;	// music playing from GUI, we can display a screensaver
			}
		}
		else				
		{
      // we can display a screensaver
			m_bInactive=true;
		}

    // if we can display a screensaver, then start screensaver timer
		if (m_bInactive) 
		{
			m_dwSaverTick=timeGetTime();	// Start the timer going ...
		}
	}
	else
	{
    // Check we're not already in screensaver mode
		if (!m_bScreenSave)	
		{
      // no, then check the timer if screensaver should pop up
			if ( (long)(timeGetTime() - m_dwSaverTick) >= (long)(g_stSettings.m_iScreenSaverTime*60*1000L) )
			{
        //yes, show the screensaver
				m_bScreenSave = true;
				m_dwSaverTick=timeGetTime();		// Save the current time for the shutdown timeout

				switch ( g_stSettings.m_iScreenSaverMode )
				{
					case SCREENSAVER_FADE:
						{
							fFadeLevel = (FLOAT) g_stSettings.m_iScreenSaverFadeLevel / 100; // 0.07f;
						}
						break;

					case SCREENSAVER_BLACK:
						{
							fFadeLevel = 0;
						}
						break;

					case SCREENSAVER_MATRIX:
						{
              if (!IsPlayingVideo())
              {
							  m_gWindowManager.ActivateWindow(WINDOW_SCREENSAVER);
                return;
              }
              else 
              {
                fFadeLevel = (FLOAT) g_stSettings.m_iScreenSaverFadeLevel / 100; // 0.07f;
              }
						}
						break;
				}

				m_pd3dDevice->GetGammaRamp(&m_OldRamp);	// Store the old gamma ramp
				// Fade to fFadeLevel
				for (float fade=1.f; fade>=fFadeLevel; fade-=0.01f)
				{
					for(int i=0;i<256;i++)
					{
						Ramp.red[i]=(int)((float)m_OldRamp.red[i]*fade);
						Ramp.green[i]=(int)((float)m_OldRamp.green[i]*fade);
						Ramp.blue[i]=(int)((float)m_OldRamp.blue[i]*fade);
					}
					Sleep(5);
					m_pd3dDevice->SetGammaRamp(D3DSGR_IMMEDIATE, &Ramp);	// use immediate to get a smooth fade
				}
			}
		}
	}

	return;
}

void CApplication::CheckShutdown()
{
  // dont shutdown if we're playing a file
  if (m_pPlayer)
  {
    if (m_pPlayer->IsPlaying())
    {
      m_dwSaverTick=timeGetTime();
      return;
    }
  }

	// Note: if the the screensaver is switched on, the shutdown timeout is
	// counted from when the screensaver activates.
	if (!m_bInactive)
	{
		if (IsPlayingVideo() && !m_pPlayer->IsPaused())	// are we playing a movie?
		{
			m_bInactive=false;
		}
		else if (IsPlayingAudio())	// are we playing some music?
		{
			m_bInactive=false;
		}
		else				// nothing doing here, so start the timer going
		{
			m_bInactive=true;
		}

		if (m_bInactive) 
		{
			m_dwSaverTick=timeGetTime();		// Start the timer going ...
		}
	}
	else
	{
		if ( (long)(timeGetTime() - m_dwSaverTick) >= (long)(g_stSettings.m_iShutdownTime*60*1000L) )
		{
			g_application.Stop();		// Stop ourselves
			XKUtils::XBOXPowerOff();	// Turn off the box
		}
	}

	return;
}
void CApplication::SpinHD()
{
  // is harddisk spindown enabled?
	if (!g_stSettings.m_iHDSpinDownTime) return;// dont do HD spindown

  //yes. Can we do a spindown right now?
	if (!m_bSpinDown)
	{
    // no. Check if we're still playing
		if (!m_pPlayer)  
		{
      //not playing anymore, then spindown should b possible
			m_bSpinDown=true;
      m_dwSpinDownTime=timeGetTime();
		}
		else 
    {
      if (!m_pPlayer->IsPlaying()) 
		  {
        //not playing anymore, then spindown should b possible
			  m_bSpinDown=true;
        m_dwSpinDownTime=timeGetTime();
		  }
      else
      {
        // playing a file, is it from HD
        if ( !CUtil::IsHD(m_strCurrentFile) )
		    {
          // no, then spindown should b possible
			    m_bSpinDown=true;
          m_dwSpinDownTime=timeGetTime();
		    }
      }
    }
	}

	// spin down HD after 3 mins of inactivity
  //spindown enabled?
	if (m_bSpinDown)
	{
    // yes, then check the elapsed time
    DWORD dwTimeSpan=timeGetTime() - m_dwSpinDownTime;
		if ( m_dwSpinDownTime==0 || (dwTimeSpan >= ((DWORD)g_stSettings.m_iHDSpinDownTime*60UL*1000UL))  )
		{
      // time has elapsed, spin it down
			m_dwSpinDownTime=timeGetTime();
			CIoSupport helper;
			helper.SpindownHarddisk();
		}
	}

	// clean up player core if its inactive >= 10 sec.
	if (m_pPlayer)
	{
    // not playing anymore?
		if (!m_pPlayer->IsPlaying() )
		{
      // inactive > 10sec?
			bool bTimeOut=(long)(timeGetTime() - m_dwIdleTime) >= 10L*1000L ;
			
      // music stopped.
			if (m_gWindowManager.GetActiveWindow()==WINDOW_VISUALISATION)
			{
				int nPlaylist=g_playlistPlayer.GetCurrentPlaylist();
				if ((nPlaylist==PLAYLIST_MUSIC || nPlaylist==PLAYLIST_MUSIC_TEMP) || bTimeOut)
				{
					if (g_playlistPlayer.GetPlaylist( nPlaylist ).size() ==0 || bTimeOut)
					{
						m_gWindowManager.PreviousWindow();
					}
				}
			}

      // yes
			if ( bTimeOut )
			{
        // then clean up
				delete m_pPlayer;
				m_pPlayer=NULL;
			}
		}
		else
		{
      //still playing, reset cleanup timer
			m_dwIdleTime=timeGetTime();
		}
	}
}

void CApplication::ResetAllControls()
{
	m_guiMusicOverlay.ResetAllControls();
	m_guiWindowVideoOverlay.ResetAllControls();
}

bool CApplication::OnMessage(CGUIMessage& message)
{
  switch ( message.GetMessage() )
  {
		case GUI_MSG_DVDDRIVE_EJECTED_CD:
		{
				//	Update general playlist: Remove DVD playlist items
				int nRemoved = g_playlistPlayer.RemoveDVDItems();
				if ( nRemoved > 0 ) {
					CGUIMessage msg( GUI_MSG_PLAYLIST_CHANGED, 0, 0, 0, 0, NULL );
					m_gWindowManager.SendMessage( msg );
				}
		}
		break;

		case GUI_MSG_PLAYBACK_STOPPED:
		case GUI_MSG_PLAYBACK_ENDED:
    {
			m_dwIdleTime=timeGetTime();
      CStdString strFile=m_strCurrentFile;

      m_strCurrentFile="";
      if (message.GetMessage() == GUI_MSG_PLAYBACK_ENDED) 
      {
        if (CUtil::IsVideo(strFile) && g_playlistPlayer.GetCurrentPlaylist() == PLAYLIST_VIDEO_TEMP)
        {
				  //	Video stacking playback ended
 				  CPlayList& playlist=g_playlistPlayer.GetPlaylist(PLAYLIST_VIDEO_TEMP);

				  if (g_playlistPlayer.GetEntriesNotFound()<playlist.size())
					  g_playlistPlayer.PlayNext(true);
        }
        else
        {
				  //	Normal playback ended
			    if (m_pPlayer) 
			    {
					  CPlayList& playlist=g_playlistPlayer.GetPlaylist(g_playlistPlayer.GetCurrentPlaylist());

					  if (g_playlistPlayer.GetEntriesNotFound()<playlist.size())
						  g_playlistPlayer.PlayNext(true);
  					
			    }
        }
      }
      if (!IsPlayingVideo() && m_gWindowManager.GetActiveWindow()==WINDOW_FULLSCREEN_VIDEO) {
				m_gWindowManager.PreviousWindow();
      }
    }
		break;

		case GUI_MSG_PLAYLIST_PLAY_NEXT_PREV:
		{
			if (message.GetParam1()==PLAYLIST_MUSIC || message.GetParam1()==PLAYLIST_MUSIC_TEMP)
			{
				CPlayList::CPlayListItem* pItem=(CPlayList::CPlayListItem*)message.GetLPVOID();
				if (pItem)
				{
					//	only Increment Top 100 Counter, if we have not clicked from inside the Top 100 view
					if (g_stSettings.m_iMyMusicStartWindow==WINDOW_MUSIC_TOP100)
					{
						break;
					}
					else
					{
						CMusicDatabase db;
						if (db.Open())
						{
							db.IncrTop100CounterByFileName(pItem->GetFileName());
							db.Close();
						}
					}
				}
			}
		}
		break;
	}

	return true;
}

void CApplication::Process()
{
  // checks whats in the DVD drive and tries to autostart the content (xbox games, dvd, cdda, avi files...)
	m_Autorun.HandleAutorun();

	// check if we need to load a new skin
	if (m_dwSkinTime && timeGetTime() >= m_dwSkinTime)
	{
		CGUIMessage msg(GUI_MSG_USER,-1,m_gWindowManager.GetActiveWindow());
		g_graphicsContext.SendMessage(msg);
	}

  // dispatch the messages generated by python or other threads to the current window
	m_gWindowManager.DispatchThreadMessages();

  // process messages which have to be send to the gui
	// (this can only be done after m_gWindowManager.Render())
	g_applicationMessenger.ProcessWindowMessages();

	// process any Python scripts
	m_pythonParser.Process();

  // check if we need 2 spin down the harddisk
	SpinHD();

  // check if we need to activate the screensaver (if enabled)
	if (g_stSettings.m_iScreenSaverMode) CheckScreenSaver();

  // check if we need to shutdown (if enabled)
	if (g_stSettings.m_iShutdownTime) CheckShutdown();

	// process messages, even if a movie is playing
	g_applicationMessenger.ProcessMessages();

}
void CApplication::Restart(bool bSamePosition)
{
  // this function gets called when the user changes a setting (like noninterleaved)
  // and which means we gotta close & reopen the current playing file
  
  // first check if we're playing a file
  if ( !IsPlayingVideo() && !IsPlayingAudio()) return;

  // do we want to return to the current position in the file
  if (false==bSamePosition)
  {
    // no, then just reopen the file and start at the beginning
    PlayFile(m_strCurrentFile,true);
    return;
  }

  // else get current position
  int iPercentage=m_pPlayer->GetPercentage();
  
  // reopen the file
  if (  PlayFile(m_strCurrentFile,true) )
  {
    // and seek to the position
    m_pPlayer->SeekPercentage(iPercentage);
  }
}

const CStdString& CApplication::CurrentFile()
{
  return m_strCurrentFile;
}


void CApplication::SetPlaySpeed(int iSpeed)
{
  if (!IsPlayingAudio() && !IsPlayingVideo()) return;
  if (m_iPlaySpeed==iSpeed) return;
  m_iPlaySpeed=iSpeed;

  //__int64 iTime;
  //if ( IsPlayingVideo())
  //  iTime= m_pPlayer->GetPTS(); // for VIDEO pts are accurate during FF/RW
  //else
  //  iTime= m_pPlayer->GetTime() / (__int64)10; // for AUDIO the playtime is accurate during FF/RW
  m_pPlayer->ToFFRW(m_iPlaySpeed);
  if (m_pAudioDecoder)
  {
    if(m_iPlaySpeed==1)
    {
      //m_pPlayer->SeekTime((int)iTime);
      m_pAudioDecoder->SetCurrentVolume (m_pAudioDecoder->GetMaximumVolume());
    }
    else
      m_pAudioDecoder->SetCurrentVolume (m_pAudioDecoder->GetMinimumVolume());
  }
}

int CApplication::GetPlaySpeed() const
{
  return m_iPlaySpeed;
}
void CApplication::SetCurrentSong(const CMusicInfoTag& tag)
{
  m_tagCurrentSong=tag;
}
void CApplication::SetCurrentMovie(const CIMDBMovie& tag)
{
  m_tagCurrentMovie=tag;
}