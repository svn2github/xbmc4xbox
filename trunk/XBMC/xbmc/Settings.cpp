
#include "stdafx.h"
#include "settings.h"
#include "application.h"
#include "util.h"
#include "utils/log.h"
#include "localizestrings.h"
#include "stdstring.h"
#include "GraphicContext.h"
#include "GUIWindowMusicBase.h"
#include "utils/FanController.h"
#include "LangCodeExpander.h"

using namespace std;

class CSettings g_settings;
struct CSettings::stSettings g_stSettings;

extern CStdString g_LoadErrorStr;

CSettings::CSettings(void)
{
	m_ResInfo[HDTV_1080i].Overscan.left = 0;
	m_ResInfo[HDTV_1080i].Overscan.top = 0;
	m_ResInfo[HDTV_1080i].Overscan.right = 1920;
	m_ResInfo[HDTV_1080i].Overscan.bottom = 1080;
	m_ResInfo[HDTV_1080i].iSubtitles = (int)(0.965*1080);
	m_ResInfo[HDTV_1080i].iOSDYOffset = 0;	// Y offset for OSD (applied to all Y pos in skin)
	m_ResInfo[HDTV_1080i].iWidth = 1920;
	m_ResInfo[HDTV_1080i].iHeight = 1080;
	m_ResInfo[HDTV_1080i].dwFlags = D3DPRESENTFLAG_INTERLACED|D3DPRESENTFLAG_WIDESCREEN;
	m_ResInfo[HDTV_1080i].fPixelRatio = 1.0f;
	strcpy(m_ResInfo[HDTV_1080i].strMode,"1080i 16:9");

	m_ResInfo[HDTV_720p].Overscan.left = 0;
	m_ResInfo[HDTV_720p].Overscan.top = 0;
	m_ResInfo[HDTV_720p].Overscan.right = 1280;
	m_ResInfo[HDTV_720p].Overscan.bottom = 720;
	m_ResInfo[HDTV_720p].iSubtitles = (int)(0.965*720);
	m_ResInfo[HDTV_720p].iOSDYOffset = 0;	// Y offset for OSD (applied to all Y pos in skin)
	m_ResInfo[HDTV_720p].iWidth = 1280;
	m_ResInfo[HDTV_720p].iHeight = 720;
	m_ResInfo[HDTV_720p].dwFlags = D3DPRESENTFLAG_PROGRESSIVE|D3DPRESENTFLAG_WIDESCREEN;
	m_ResInfo[HDTV_720p].fPixelRatio = 1.0f;
	strcpy(m_ResInfo[HDTV_720p].strMode,"720p 16:9");

	m_ResInfo[HDTV_480p_4x3].Overscan.left = 0;
	m_ResInfo[HDTV_480p_4x3].Overscan.top = 0;
	m_ResInfo[HDTV_480p_4x3].Overscan.right = 720;
	m_ResInfo[HDTV_480p_4x3].Overscan.bottom = 480;
	m_ResInfo[HDTV_480p_4x3].iSubtitles = (int)(0.9*480);
	m_ResInfo[HDTV_480p_4x3].iOSDYOffset = 0;	// Y offset for OSD (applied to all Y pos in skin)
	m_ResInfo[HDTV_480p_4x3].iWidth = 720;
	m_ResInfo[HDTV_480p_4x3].iHeight = 480;
	m_ResInfo[HDTV_480p_4x3].dwFlags = D3DPRESENTFLAG_PROGRESSIVE;
	m_ResInfo[HDTV_480p_4x3].fPixelRatio = 4320.0f/4739.0f;
	strcpy(m_ResInfo[HDTV_480p_4x3].strMode,"480p 4:3");

	m_ResInfo[HDTV_480p_16x9].Overscan.left = 0;
	m_ResInfo[HDTV_480p_16x9].Overscan.top = 0;
	m_ResInfo[HDTV_480p_16x9].Overscan.right = 720;
	m_ResInfo[HDTV_480p_16x9].Overscan.bottom = 480;
	m_ResInfo[HDTV_480p_16x9].iSubtitles = (int)(0.965*480);
	m_ResInfo[HDTV_480p_16x9].iOSDYOffset = 0;	// Y offset for OSD (applied to all Y pos in skin)
	m_ResInfo[HDTV_480p_16x9].iWidth = 720;
	m_ResInfo[HDTV_480p_16x9].iHeight = 480;
	m_ResInfo[HDTV_480p_16x9].dwFlags = D3DPRESENTFLAG_PROGRESSIVE|D3DPRESENTFLAG_WIDESCREEN;
	m_ResInfo[HDTV_480p_16x9].fPixelRatio = 4320.0f/4739.0f*4.0f/3.0f;
	strcpy(m_ResInfo[HDTV_480p_16x9].strMode,"480p 16:9");

	m_ResInfo[NTSC_4x3].Overscan.left = 0;
	m_ResInfo[NTSC_4x3].Overscan.top = 0;
	m_ResInfo[NTSC_4x3].Overscan.right = 720;
	m_ResInfo[NTSC_4x3].Overscan.bottom = 480;
	m_ResInfo[NTSC_4x3].iSubtitles = (int)(0.9*480);
	m_ResInfo[NTSC_4x3].iOSDYOffset = 0;	// Y offset for OSD (applied to all Y pos in skin)
	m_ResInfo[NTSC_4x3].iWidth = 720;
	m_ResInfo[NTSC_4x3].iHeight = 480;
	m_ResInfo[NTSC_4x3].dwFlags = 0;
	m_ResInfo[NTSC_4x3].fPixelRatio = 4320.0f/4739.0f;
	strcpy(m_ResInfo[NTSC_4x3].strMode,"NTSC 4:3");

	m_ResInfo[NTSC_16x9].Overscan.left = 0;
	m_ResInfo[NTSC_16x9].Overscan.top = 0;
	m_ResInfo[NTSC_16x9].Overscan.right = 720;
	m_ResInfo[NTSC_16x9].Overscan.bottom = 480;
	m_ResInfo[NTSC_16x9].iSubtitles = (int)(0.965*480);
	m_ResInfo[NTSC_16x9].iOSDYOffset = 0;	// Y offset for OSD (applied to all Y pos in skin)
	m_ResInfo[NTSC_16x9].iWidth = 720;
	m_ResInfo[NTSC_16x9].iHeight = 480;
	m_ResInfo[NTSC_16x9].dwFlags = D3DPRESENTFLAG_WIDESCREEN;
	m_ResInfo[NTSC_16x9].fPixelRatio = 4320.0f/4739.0f*4.0f/3.0f;
	strcpy(m_ResInfo[NTSC_16x9].strMode,"NTSC 16:9");

	m_ResInfo[PAL_4x3].Overscan.left = 0;
	m_ResInfo[PAL_4x3].Overscan.top = 0;
	m_ResInfo[PAL_4x3].Overscan.right = 720;
	m_ResInfo[PAL_4x3].Overscan.bottom = 576;
	m_ResInfo[PAL_4x3].iSubtitles = (int)(0.9*576);
	m_ResInfo[PAL_4x3].iOSDYOffset = 0;	// Y offset for OSD (applied to all Y pos in skin)
	m_ResInfo[PAL_4x3].iWidth = 720;
	m_ResInfo[PAL_4x3].iHeight = 576;
	m_ResInfo[PAL_4x3].dwFlags = 0;
	m_ResInfo[PAL_4x3].fPixelRatio = 128.0f/117.0f;
	strcpy(m_ResInfo[PAL_4x3].strMode,"PAL 4:3");

	m_ResInfo[PAL_16x9].Overscan.left = 0;
	m_ResInfo[PAL_16x9].Overscan.top = 0;
	m_ResInfo[PAL_16x9].Overscan.right = 720;
	m_ResInfo[PAL_16x9].Overscan.bottom = 576;
	m_ResInfo[PAL_16x9].iSubtitles = (int)(0.965*576);
	m_ResInfo[PAL_16x9].iOSDYOffset = 0;	// Y offset for OSD (applied to all Y pos in skin)
	m_ResInfo[PAL_16x9].iWidth = 720;
	m_ResInfo[PAL_16x9].iHeight = 576;
	m_ResInfo[PAL_16x9].dwFlags = D3DPRESENTFLAG_WIDESCREEN;
	m_ResInfo[PAL_16x9].fPixelRatio = 128.0f/117.0f*4.0f/3.0f;
	strcpy(m_ResInfo[PAL_16x9].strMode,"PAL 16:9");

	m_ResInfo[PAL60_4x3].Overscan.left = 0;
	m_ResInfo[PAL60_4x3].Overscan.top = 0;
	m_ResInfo[PAL60_4x3].Overscan.right = 720;
	m_ResInfo[PAL60_4x3].Overscan.bottom = 480;
	m_ResInfo[PAL60_4x3].iSubtitles = (int)(0.9*480);
	m_ResInfo[PAL60_4x3].iOSDYOffset = 0;	// Y offset for OSD (applied to all Y pos in skin)
	m_ResInfo[PAL60_4x3].iWidth = 720;
	m_ResInfo[PAL60_4x3].iHeight = 480;
	m_ResInfo[PAL60_4x3].dwFlags = 0;
	m_ResInfo[PAL60_4x3].fPixelRatio = 4320.0f/4739.0f;
	strcpy(m_ResInfo[PAL60_4x3].strMode,"PAL60 4:3");

	m_ResInfo[PAL60_16x9].Overscan.left = 0;
	m_ResInfo[PAL60_16x9].Overscan.top = 0;
	m_ResInfo[PAL60_16x9].Overscan.right = 720;
	m_ResInfo[PAL60_16x9].Overscan.bottom = 480;
	m_ResInfo[PAL60_16x9].iSubtitles = (int)(0.965*480);
	m_ResInfo[PAL60_16x9].iOSDYOffset = 0;	// Y offset for OSD (applied to all Y pos in skin)
	m_ResInfo[PAL60_16x9].iWidth = 720;
	m_ResInfo[PAL60_16x9].iHeight = 480;
	m_ResInfo[PAL60_16x9].dwFlags = D3DPRESENTFLAG_WIDESCREEN;
	m_ResInfo[PAL60_16x9].fPixelRatio = 4320.0f/4739.0f*4.0f/3.0f;
	strcpy(m_ResInfo[PAL60_16x9].strMode,"PAL60 16:9");

	strcpy(g_stSettings.m_szExternalDVDPlayer,"");
	strcpy(g_stSettings.m_szExternalCDDAPlayer,"");

	g_stSettings.m_bMyVideoActorStack =false;
	g_stSettings.m_bMyVideoGenreStack =false;
	g_stSettings.m_bMyVideoYearStack =false;

	// default to fuzzy stacking, to stay consistent with older versions
	g_stSettings.m_iMyVideoVideoStack = STACK_NONE;

	strcpy(g_stSettings.m_szMyVideoStackTokens, "cd|part");
	strcpy(g_stSettings.m_szMyVideoStackSeparators, "- _.");

	StringUtils::SplitString(g_stSettings.m_szMyVideoStackTokens, "|", g_settings.m_szMyVideoStackTokensArray);
	g_settings.m_szMyVideoStackSeparatorsString = g_stSettings.m_szMyVideoStackSeparators;

	for (int i = 0; i < (int)g_settings.m_szMyVideoStackTokensArray.size(); i++)
		g_settings.m_szMyVideoStackTokensArray[i].MakeLower();

	g_stSettings.m_bMyVideoCleanTitles = false;
	strcpy(g_stSettings.m_szMyVideoCleanTokens, "divx|xvid|3ivx|ac3|ac351|dts|mp3|wma|m4a|mp4|aac|ogg|scr|ts|sharereactor|dvd|dvdrip");
	strcpy(g_stSettings.m_szMyVideoCleanSeparators, "- _.[({+");

	StringUtils::SplitString(g_stSettings.m_szMyVideoCleanTokens, "|", g_settings.m_szMyVideoCleanTokensArray);
	g_settings.m_szMyVideoCleanSeparatorsString = g_stSettings.m_szMyVideoCleanSeparators;

	for (int i = 0; i < (int)g_settings.m_szMyVideoCleanTokensArray.size(); i++)
		g_settings.m_szMyVideoCleanTokensArray[i].MakeLower();

	g_stSettings.m_bNonInterleaved=false;
	g_stSettings.m_bNoCache=false;
	g_stSettings.m_bUseFDrive=true;
	g_stSettings.m_bUseGDrive=false;
	g_stSettings.m_bUsePCDVDROM=false;
	g_stSettings.m_bDetectAsIso=false;
	g_stSettings.dwFileVersion =CONFIG_VERSION;
	g_stSettings.m_iMyProgramsViewAsIcons=1;
	g_stSettings.m_bMyProgramsSortAscending=true;
	strcpy(g_stSettings.szDashboard,"C:\\xboxdash.xbe");
	strcpy(g_stSettings.m_szAlternateSubtitleDirectory,"");
	strcpy(g_stSettings.szOnlineArenaPassword,"");
	strcpy(g_stSettings.szOnlineArenaDescription,"It's Good To Play Together!");
	strcpy(g_stSettings.szHomeDir,"");

	strcpy(g_stSettings.m_szMyPicturesExtensions,".bmp|.jpg|.png|.gif|.pcx|.tif|.jpeg");
	strcpy(g_stSettings.m_szMyMusicExtensions,".ac3|.aac|.pls|.strm|.rm|.sc|.mpa|.wav|.wma|.ogg|.mp3|.mp2|.m3u");
	strcpy(g_stSettings.m_szMyVideoExtensions,".nfo|.rm|.m3u|.ifo|.mov|.qt|.divx|.xvid|.bivx|.vob|.pva|.wmv|.asf|.asx|.ogm|.m2v|.avi|.bin|.dat|.mpg|.mpeg|.mkv|.avc|.vp3|.svq3|.nuv|.viv|.dv|.fli");

	strcpy( g_stSettings.m_szDefaultPrograms, "");
	strcpy( g_stSettings.m_szDefaultMusic, "");
	strcpy( g_stSettings.m_szDefaultPictures, "");
	strcpy( g_stSettings.m_szDefaultFiles, "");
	strcpy( g_stSettings.m_szDefaultVideos, "");
	strcpy( g_stSettings.m_szCDDBIpAdres,"");
	strcpy (g_stSettings.m_szMusicRecordingDirectory,"");

	g_stSettings.m_bMyMusicSongInfoInVis=true;
	g_stSettings.m_bMyMusicSongThumbInVis=true;

	g_stSettings.m_bMyMusicSongsRootSortAscending=true;
	g_stSettings.m_bMyMusicSongsSortAscending=true;

	g_stSettings.m_bMyMusicAlbumRootSortAscending=true;
	g_stSettings.m_bMyMusicAlbumSortAscending=true;
	g_stSettings.m_bMyMusicAlbumShowRecent=false;

	g_stSettings.m_bMyMusicArtistsRootSortAscending=true;
	g_stSettings.m_bMyMusicArtistsAlbumsSortAscending=true;
	g_stSettings.m_bMyMusicArtistsAllSongsSortAscending=true;
	g_stSettings.m_bMyMusicArtistsAlbumSongsSortAscending=true;

	g_stSettings.m_bMyMusicGenresRootSortAscending=true;
	g_stSettings.m_bMyMusicGenresSortAscending=true;

	g_stSettings.m_iMyMusicPlaylistViewAsIcons=1;
	g_stSettings.m_bMyMusicPlaylistRepeat=true;
	g_stSettings.m_bMyMusicPlaylistShuffle=false;

	g_stSettings.m_bMyVideoSortAscending=true;
	g_stSettings.m_bMyVideoRootSortAscending=true;

	g_stSettings.m_bMyVideoGenreSortAscending=true;
	g_stSettings.m_bMyVideoGenreRootSortAscending=true;

	g_stSettings.m_bMyVideoActorSortAscending=true;
	g_stSettings.m_bMyVideoActorRootSortAscending=true;

	g_stSettings.m_bMyVideoYearSortAscending=true;
	g_stSettings.m_bMyVideoYearRootSortAscending=true;

	g_stSettings.m_bMyVideoTitleSortAscending=true;

	g_stSettings.m_iMyVideoPlaylistViewAsIcons=1;
	g_stSettings.m_bMyVideoPlaylistRepeat=true;
	g_stSettings.m_bMyVideoPlaylistShuffle=false;

	g_stSettings.m_bMyFilesSourceViewAsIcons=false;
	g_stSettings.m_bMyFilesSourceRootViewAsIcons=true;
	g_stSettings.m_bMyFilesDestViewAsIcons=false;
	g_stSettings.m_bMyFilesDestRootViewAsIcons=true;

	g_stSettings.m_bMyPicturesSortAscending=true;
	g_stSettings.m_bMyPicturesRootSortAscending=true;

	g_stSettings.m_bScriptsViewAsIcons = false;
	g_stSettings.m_bScriptsRootViewAsIcons = false;
	g_stSettings.m_bScriptsSortAscending = true;

	g_stSettings.m_bMyFilesSourceSortAscending=true;
	g_stSettings.m_bMyFilesSourceRootSortAscending=true;
	g_stSettings.m_bMyFilesDestSortAscending=true;
	g_stSettings.m_bMyFilesDestRootSortAscending=true;
	g_stSettings.m_iViewMode = VIEW_MODE_NORMAL;
	g_stSettings.m_fZoomAmount = 1.0f;
	g_stSettings.m_fPixelRatio = 1.0f;
	g_stSettings.m_fCustomZoomAmount=1.0f;
	g_stSettings.m_fCustomPixelRatio=1.0f;

	g_stSettings.m_bDisplayRemoteCodes=false;
	g_stSettings.m_mplayerDebug=false;
	g_stSettings.m_iSambaDebugLevel = 0;
	strcpy(g_stSettings.m_strSambaWorkgroup, "WORKGROUP");
	strcpy(g_stSettings.m_strSambaWinsServer, "");

	g_stSettings.m_nVolumeLevel = 0;
	g_stSettings.m_iLogLevel = LOGNOTICE;

	g_stSettings.m_bShowFreeMem=false;

  m_iLastLoadedProfileIndex = -1;

	xbmcXmlLoaded = false;
}

CSettings::~CSettings(void)
{
}


void CSettings::Save() const
{
  if (g_application.m_bStop)
  {
    //don't save settings when we're busy stopping the application
    //a lot of screens try to save settings on deinit and deinit is called
    //for every screen when the application is stopping.
    return;
  }
	if (!SaveSettings("T:\\settings.xml", true))
	{
		CLog::Log(LOGERROR, "Unable to save settings to T:\\settings.xml");
	}
}

bool CSettings::Load(bool& bXboxMediacenter, bool& bSettings)
{
	// load settings file...
	bXboxMediacenter=bSettings=false;
	CLog::Log(LOGNOTICE, "loading T:\\settings.xml");
	if (!LoadSettings("T:\\settings.xml", true))
	{
		CLog::Log(LOGERROR, "Unable to load T:\\settings.xml, creating new T:\\settings.xml with default values");
		Save();
		if (!(bSettings=LoadSettings("T:\\settings.xml", true)))
			return false;
	}

	// load xml file...
	CLog::Log(LOGNOTICE, "loading Q:\\XboxMediaCenter.xml");
	CStdString strXMLFile = "Q:\\XboxMediaCenter.xml";
	TiXmlDocument xmlDoc;
	if ( !xmlDoc.LoadFile( strXMLFile.c_str() ) )
	{
		g_LoadErrorStr.Format("%s, Line %d\n%s", strXMLFile.c_str(), xmlDoc.ErrorRow(), xmlDoc.ErrorDesc());
		return false;
	}

	TiXmlElement* pRootElement =xmlDoc.RootElement();
	CStdString strValue=pRootElement->Value();
	if ( strValue != "xboxmediacenter")
	{
		g_LoadErrorStr.Format("%s Doesn't contain <xboxmediacenter>",strXMLFile.c_str());
		return false;
	}

	GetInteger(pRootElement, "loglevel", g_stSettings.m_iLogLevel, LOGWARNING, LOGDEBUG, LOGNONE);
	GetBoolean(pRootElement, "showfreemem", g_stSettings.m_bShowFreeMem);

	TiXmlElement* pFileTypeIcons =pRootElement->FirstChildElement("filetypeicons");
	TiXmlNode* pFileType=pFileTypeIcons->FirstChild();
	while (pFileType)
	{
		CFileTypeIcon icon;
		icon.m_strName=".";
		icon.m_strName+=pFileType->Value();
		icon.m_strIcon=pFileType->FirstChild()->Value();
		m_vecIcons.push_back(icon);
		pFileType=pFileType->NextSibling();
	}
    
	TiXmlElement* pMasterLockElement =pRootElement->FirstChildElement("masterlock");
	if (pMasterLockElement)
	{
		GetInteger(pMasterLockElement, "maxretry", g_stSettings.m_iMasterLockMaxRetry , 0, 0, 100);
		GetInteger(pMasterLockElement, "enableshutdown", g_stSettings.m_iMasterLockEnableShutdown , 0, 0, 1);
		GetInteger(pMasterLockElement, "protectshares", g_stSettings.m_iMasterLockProtectShares , 0, 0, 1);
		GetInteger(pMasterLockElement, "mastermode", g_stSettings.m_iMasterLockMode , 0, 0, 3);
		GetString(pMasterLockElement, "mastercode", g_stSettings.szMasterLockCode, "");
		GetInteger(pMasterLockElement, "startuplock", g_stSettings.m_iMasterLockStartupLock , 0, 0, 1);
	}

	TiXmlElement* pSambaElement =pRootElement->FirstChildElement("samba");
	if (pSambaElement)
	{
		GetString(pSambaElement, "workgroup", g_stSettings.m_strSambaWorkgroup, "WORKGROUP");
		GetString(pSambaElement, "winsserver", g_stSettings.m_strSambaWinsServer, "");
		GetInteger(pSambaElement, "debuglevel", g_stSettings.m_iSambaDebugLevel , 0, 0, 100);
		GetString(pSambaElement, "defaultusername", g_stSettings.m_strSambaDefaultUserName, "");
		GetString(pSambaElement, "defaultpassword", g_stSettings.m_strSambaDefaultPassword, "");

	}

	TiXmlElement* pDelaysElement =pRootElement->FirstChildElement("delays");
	if (pDelaysElement)
	{
		TiXmlElement* pRemoteDelays			=pDelaysElement->FirstChildElement("remote");
		TiXmlElement* pControllerDelays =pDelaysElement->FirstChildElement("controller");
		if (pRemoteDelays)
		{
			GetInteger(pRemoteDelays, "move", g_stSettings.m_iMoveDelayIR,220,1,INT_MAX);
			GetInteger(pRemoteDelays, "repeat", g_stSettings.m_iRepeatDelayIR,220,1,INT_MAX);
		}

		if (pControllerDelays)
		{
			GetInteger(pControllerDelays, "move", g_stSettings.m_iMoveDelayController,220,1,INT_MAX);
			GetInteger(pControllerDelays, "repeat", g_stSettings.m_iRepeatDelayController,220,1,INT_MAX);
			GetFloat(pControllerDelays, "deadzone", g_stSettings.m_fAnalogDeadzoneController, 0.1f, 0.0f, 1.0f);
		}
	}

	GetString(pRootElement, "home", g_stSettings.szHomeDir, "");
	while ( CUtil::HasSlashAtEnd(g_stSettings.szHomeDir) )
	{
		g_stSettings.szHomeDir[strlen(g_stSettings.szHomeDir)-1]=0;
	}
	GetString(pRootElement, "dashboard", g_stSettings.szDashboard,"C:\\xboxdash.xbe");

	GetString(pRootElement, "CDDBIpAdres", g_stSettings.m_szCDDBIpAdres,"194.97.4.18");
	if (g_stSettings.m_szCDDBIpAdres == "194.97.4.18")
		GetString(pRootElement, "CDDBIpAddress", g_stSettings.m_szCDDBIpAdres,"194.97.4.18");
	//g_stSettings.m_bUseCDDB=GetBoolean(pRootElement, "CDDBEnabled");



	GetString(pRootElement, "thumbnails",g_stSettings.szThumbnailsDirectory,"");
	GetString(pRootElement, "shortcuts", g_stSettings.m_szShortcutDirectory,"");
	GetString(pRootElement, "screenshots", g_stSettings.m_szScreenshotsDirectory, "");
	GetString(pRootElement, "recordings", g_stSettings.m_szMusicRecordingDirectory,"");

	GetString(pRootElement, "albums", g_stSettings.m_szAlbumDirectory,"");
	GetString(pRootElement, "subtitles", g_stSettings.m_szAlternateSubtitleDirectory,"");

	GetString(pRootElement, "pictureextensions", g_stSettings.m_szMyPicturesExtensions,".bmp|.jpg|.png|.gif|.pcx|.tif|.jpeg");

	GetString(pRootElement, "musicextensions", g_stSettings.m_szMyMusicExtensions,".ac3|.aac|.strm|.pls|.rm|.sc|.mpa|.wav|.wma|.ogg|.mp3|.mp2|.m3u");
	GetString(pRootElement, "videoextensions", g_stSettings.m_szMyVideoExtensions,".nfo|.rm|.m3u|.ifo|.mov|.qt|.divx|.xvid|.bivx|.vob|.pva|.wmv|.asf|.asx|.ogm|.m2v|.avi|.bin|.dat|.mpg|.mpeg|.mkv|.avc|.vp3|.svq3|.nuv|.viv|.dv|.fli");

	GetInteger(pRootElement, "startwindow", g_stSettings.m_iStartupWindow,0,0,INT_MAX);
	g_stSettings.m_iStartupWindow += WINDOW_HOME;	// windows referenced from WINDOW_HOME

	GetBoolean(pRootElement, "useFDrive", g_stSettings.m_bUseFDrive);
	GetBoolean(pRootElement, "useGDrive", g_stSettings.m_bUseGDrive);
	GetBoolean(pRootElement, "usePCDVDROM", g_stSettings.m_bUsePCDVDROM);

	GetBoolean(pRootElement, "detectAsIso", g_stSettings.m_bDetectAsIso);

	GetBoolean(pRootElement, "displayremotecodes", g_stSettings.m_bDisplayRemoteCodes);

	GetString(pRootElement, "dvdplayer", g_stSettings.m_szExternalDVDPlayer,"");
	GetString(pRootElement, "cddaplayer", g_stSettings.m_szExternalCDDAPlayer,"");
	GetBoolean(pRootElement, "mplayerdebug", g_stSettings.m_mplayerDebug);

	GetString(pRootElement, "CDDARipPath", g_stSettings.m_strRipPath, "");

	CStdString strDir;

	strDir=g_stSettings.m_szShortcutDirectory;
	ConvertHomeVar(strDir);
	strcpy( g_stSettings.m_szShortcutDirectory, strDir.c_str() );

	strDir=g_stSettings.szThumbnailsDirectory;
	ConvertHomeVar(strDir);
	strcpy( g_stSettings.szThumbnailsDirectory, strDir.c_str() );


	strDir=g_stSettings.m_szAlbumDirectory;
	ConvertHomeVar(strDir);
	strcpy( g_stSettings.m_szAlbumDirectory, strDir.c_str() );

	strDir=g_stSettings.m_szMusicRecordingDirectory;
	ConvertHomeVar(strDir);
	strcpy( g_stSettings.m_szMusicRecordingDirectory, strDir.c_str() );

	strDir=g_stSettings.m_szScreenshotsDirectory;
	ConvertHomeVar(strDir);
	strcpy( g_stSettings.m_szScreenshotsDirectory, strDir.c_str() );
	while ( CUtil::HasSlashAtEnd(g_stSettings.m_szScreenshotsDirectory) )
	{
		g_stSettings.m_szScreenshotsDirectory[strlen(g_stSettings.m_szScreenshotsDirectory)-1]=0;
	}

	if (g_stSettings.m_szShortcutDirectory[0])
	{
		CShare share;
		share.strPath=g_stSettings.m_szShortcutDirectory;
		share.strName="shortcuts";
		m_vecMyProgramsBookmarks.push_back(share);
	}


	strDir=g_stSettings.m_szAlternateSubtitleDirectory;
	ConvertHomeVar(strDir);
	strcpy( g_stSettings.m_szAlternateSubtitleDirectory, strDir.c_str() );


	// Home page button scroller
	LoadHomeButtons(pRootElement);

	// parse my programs bookmarks...
	CStdString strDefault;
	GetShares(pRootElement,"myprograms",m_vecMyProgramsBookmarks,strDefault);
	if (strDefault.size())
		strcpy( g_stSettings.m_szDefaultPrograms, strDefault.c_str());

	GetShares(pRootElement,"pictures",m_vecMyPictureShares,strDefault);
	if (strDefault.size())
		strcpy( g_stSettings.m_szDefaultPictures, strDefault.c_str());

	GetShares(pRootElement,"files",m_vecMyFilesShares,strDefault);
	if (strDefault.size())
		strcpy( g_stSettings.m_szDefaultFiles, strDefault.c_str());

	GetShares(pRootElement,"music",m_vecMyMusicShares,strDefault);
	if (strDefault.size())
		strcpy( g_stSettings.m_szDefaultMusic, strDefault.c_str());

	GetShares(pRootElement,"video",m_vecMyVideoShares,strDefault);
	if (strDefault.size())
		strcpy( g_stSettings.m_szDefaultVideos, strDefault.c_str());

  g_LangCodeExpander.LoadUserCodes(pRootElement->FirstChildElement("languagecodes"));

	bXboxMediacenter=true;
	return true;
}

void CSettings::ConvertHomeVar(CStdString& strText)
{
	// Replaces first occurence of $HOME with the home directory.
	// "$HOME\bookmarks" becomes for instance "e:\apps\xbmp\bookmarks"

	char szText[1024];
	char szTemp[1024];
	char *pReplace,*pReplace2;

	CStdString strHomePath = "Q:";
	strcpy(szText,strText.c_str());

	pReplace = strstr(szText, "$HOME");

	if (pReplace!=NULL)
	{
		pReplace2 = pReplace + sizeof("$HOME")-1;
		strcpy(szTemp, pReplace2);
		strcpy(pReplace, strHomePath.c_str() );
		strcat(szText, szTemp);
	}
	strText=szText;
}

void CSettings::GetShares(const TiXmlElement* pRootElement, const CStdString& strTagName, VECSHARES& items,CStdString& strDefault)
{
	CLog::Log(LOGDEBUG, "  Parsing <%s> tag", strTagName.c_str());
	strDefault="";
	const TiXmlNode *pChild = pRootElement->FirstChild(strTagName.c_str());
	if (pChild)
	{
		pChild = pChild->FirstChild();
		while (pChild>0)
		{
			CStdString strValue=pChild->Value();
			if (strValue=="bookmark")
			{
				const TiXmlNode *pNodeName=pChild->FirstChild("name");
				const TiXmlNode *pPathName=pChild->FirstChild("path");
				const TiXmlNode *pCacheNode=pChild->FirstChild("cache");
				const TiXmlNode *pDepthNode=pChild->FirstChild("depth");
				const TiXmlNode *pLockMode=pChild->FirstChild("lockmode");
				const TiXmlNode *pLockCode=pChild->FirstChild("lockcode");
				const TiXmlNode *pBadPwdCount=pChild->FirstChild("badpwdcount");
				if (pNodeName && pPathName)
				{
					const char* szName=pNodeName->FirstChild()->Value();
					CLog::Log(LOGDEBUG, "    Share Name: %s", szName);
					const char* szPath=pPathName->FirstChild()->Value();
					CLog::Log(LOGDEBUG, "    Share Path: %s", szPath);

					CShare share;
					share.strName=szName;
					share.strPath=szPath;
					share.m_iBufferSize=0;
					share.m_iDepthSize=1;
         	share.m_iLockMode=LOCK_MODE_EVERYONE;
         	share.m_strLockCode="";
         	share.m_iBadPwdCount=0;
					CStdString strPath=share.strPath;
					strPath.ToUpper();
					if (strPath.Left(4)=="UDF:")
					{
						share.m_iDriveType=SHARE_TYPE_VIRTUAL_DVD;
						share.strPath="D:\\";
					}
					else if (strPath.Left(11) =="SOUNDTRACK:")
						share.m_iDriveType=SHARE_TYPE_LOCAL;
					else if (CUtil::IsISO9660(share.strPath))
						share.m_iDriveType=SHARE_TYPE_VIRTUAL_DVD;
					else if (CUtil::IsDVD(share.strPath))
						share.m_iDriveType = SHARE_TYPE_DVD;
					else if (CUtil::IsRemote(share.strPath))
						share.m_iDriveType = SHARE_TYPE_REMOTE;
					else if (CUtil::IsHD(share.strPath))
						share.m_iDriveType = SHARE_TYPE_LOCAL;
					else
						share.m_iDriveType = SHARE_TYPE_UNKNOWN;


					if (pCacheNode)
					{
						share.m_iBufferSize=atoi( pCacheNode->FirstChild()->Value() );
					}

					if (pDepthNode)
					{
						share.m_iDepthSize=atoi( pDepthNode->FirstChild()->Value() );
					}

				 	if (pLockMode)
				 	{
						share.m_iLockMode=atoi( pLockMode->FirstChild()->Value() );
					}

         	if (pLockCode)
         	{                        
           	share.m_strLockCode=pLockCode->FirstChild()->Value();
          }

				 	if (pBadPwdCount)
				 	{
					 	share.m_iBadPwdCount=atoi( pBadPwdCount->FirstChild()->Value() );
				 	}

					ConvertHomeVar(share.strPath);

					items.push_back(share);
				}
				else
				{
					CLog::Log(LOGERROR, "    <name> and/or <path> not properly defined within <bookmark>");
				}

			}
			if (strValue=="default")
			{
				const TiXmlNode *pValueNode=pChild->FirstChild();
				if (pValueNode)
				{
					const char* pszText=pChild->FirstChild()->Value();
					if (strlen(pszText) > 0)
						strDefault=pszText;
					CLog::Log(LOGDEBUG, "    Setting <default> share to : %s", strDefault.c_str());
				}
			}
			pChild=pChild->NextSibling();
		}
	}
	else
	{
		CLog::Log(LOGERROR, "  <%s> tag is missing or XboxMediaCenter.xml is malformed", strTagName.c_str());
	}
}

void CSettings::GetString(const TiXmlElement* pRootElement, const CStdString& strTagName, char* szValue, const CStdString& strDefaultValue)
{
	strcpy(szValue,"");
	const TiXmlNode *pChild = pRootElement->FirstChild(strTagName.c_str());
	if (pChild)
	{
		if (pChild->FirstChild())
		{
			CStdString strValue=pChild->FirstChild()->Value();
			if (strValue.size() )
			{
				if (strValue !="-")
					strcpy(szValue,strValue.c_str());
			}
		}
	}
	if (strlen(szValue)==0)
	{
		strcpy(szValue,strDefaultValue.c_str());
	}

	CLog::Log(LOGDEBUG, "  %s: %s", strTagName.c_str(), szValue);
}

void CSettings::GetInteger(const TiXmlElement* pRootElement, const CStdString& strTagName, int& iValue, const int iDefault, const int iMin, const int iMax)
{
	const TiXmlNode *pChild = pRootElement->FirstChild(strTagName.c_str());
	if (pChild)
	{
		iValue = atoi( pChild->FirstChild()->Value() );
		if ((iValue<iMin) || (iValue>iMax)) iValue=iDefault;
	}
	else
		iValue=iDefault;

	CLog::Log(LOGDEBUG, "  %s: %d", strTagName.c_str(), iValue);
}

void CSettings::GetFloat(const TiXmlElement* pRootElement, const CStdString& strTagName, float& fValue, const float fDefault, const float fMin, const float fMax)
{
	const TiXmlNode *pChild = pRootElement->FirstChild(strTagName.c_str());
	if (pChild)
	{
		fValue = (float)atof( pChild->FirstChild()->Value() );
		if ((fValue<fMin) || (fValue>fMax)) fValue=fDefault;
	}
	else
		fValue=fDefault;

	CLog::Log(LOGDEBUG, "  %s: %f", strTagName.c_str(), fValue);
}

void CSettings::GetBoolean(const TiXmlElement* pRootElement, const CStdString& strTagName, bool& bValue)
{
	char szString[128];
	GetString(pRootElement,strTagName,szString,"");
	if ( CUtil::cmpnocase(szString,"enabled")==0 ||
		CUtil::cmpnocase(szString,"yes")==0 ||
		CUtil::cmpnocase(szString,"on")==0 ||
		CUtil::cmpnocase(szString,"true")==0 )
	{
		bValue = true;
	}
	else if (strlen(szString)!=0)
		bValue = false;
}

void CSettings::GetHex(const TiXmlNode* pRootElement, const CStdString& strTagName, DWORD& dwHexValue, DWORD dwDefaultValue)
{
	const TiXmlNode *pChild = pRootElement->FirstChild(strTagName.c_str());
	if (pChild)
	{
		sscanf(pChild->FirstChild()->Value(),"%x", &dwHexValue);
	}
	else
	{
		dwHexValue = dwDefaultValue;
	}
}

void CSettings::SetString(TiXmlNode* pRootNode, const CStdString& strTagName, const CStdString& strValue) const
{
	CStdString strPersistedValue = strValue;
	if (strPersistedValue.length()==0)
	{
		strPersistedValue = '-';
	}

	TiXmlElement newElement(strTagName);
	TiXmlNode *pNewNode = pRootNode->InsertEndChild(newElement);
	if (pNewNode)
	{
		TiXmlText value(strValue);
		pNewNode->InsertEndChild(value);
	}
}

void CSettings::SetInteger(TiXmlNode* pRootNode, const CStdString& strTagName, int iValue) const
{
	CStdString strValue;
	strValue.Format("%d",iValue);
	SetString(pRootNode, strTagName, strValue);
}

void CSettings::SetFloat(TiXmlNode* pRootNode, const CStdString& strTagName, float fValue) const
{
	CStdString strValue;
	strValue.Format("%f",fValue);
	SetString(pRootNode, strTagName, strValue);
}

void CSettings::SetBoolean(TiXmlNode* pRootNode, const CStdString& strTagName, bool bValue) const
{
	if (bValue)
		SetString(pRootNode, strTagName, "true");
	else
		SetString(pRootNode, strTagName, "false");
}

void CSettings::SetHex(TiXmlNode* pRootNode, const CStdString& strTagName, DWORD dwHexValue) const
{
	CStdString strValue;
	strValue.Format("%x",dwHexValue);
	SetString(pRootNode, strTagName, strValue);
}

bool CSettings::LoadCalibration(const TiXmlElement* pElement, const CStdString& strSettingsFile)
{
	// reset the calibration to the defaults
	//g_graphicsContext.SetD3DParameters(NULL, m_ResInfo);
	//for (int i=0; i<10; i++)
	//	g_graphicsContext.ResetScreenParameters((RESOLUTION)i);

  const TiXmlElement *pRootElement;
  CStdString strTagName = pElement->Value();
  if (!strcmp(strTagName.c_str(), "calibration")) {
    pRootElement = pElement;
  }
  else {
	  pRootElement = pElement->FirstChildElement("calibration");
  }
	if (!pRootElement)
	{
    g_LoadErrorStr.Format("%s Doesn't contain <calibration>", strSettingsFile.c_str());
    //be nice, try to load from "old" calibration.xml file
    if (CUtil::FileExists("T:\\calibration.xml")) {
	    TiXmlDocument xmlDoc;
	    if (!xmlDoc.LoadFile("T:\\calibration.xml"))
	    {
		    return false;
	    }
	    TiXmlElement *pOldConfigRootElement = xmlDoc.RootElement();
      return LoadCalibration(pOldConfigRootElement, "T:\\calibration.xml");
    }
    return false;
	}
	TiXmlElement *pResolution = pRootElement->FirstChildElement("resolution");
	while (pResolution)
	{
		// get the data for this resolution
		int iRes;
		GetInteger(pResolution, "id", iRes, (int)PAL_4x3, HDTV_1080i, PAL60_16x9); //PAL4x3 as default data
		GetString(pResolution, "description", m_ResInfo[iRes].strMode, m_ResInfo[iRes].strMode);
		// get the appropriate "safe graphics area" = 10% for 4x3, 3.5% for 16x9
		float fSafe;
		if (iRes == PAL_4x3 || iRes == NTSC_4x3 || iRes == PAL60_4x3 || iRes == HDTV_480p_4x3)
			fSafe = 0.1f;
		else
			fSafe = 0.035f;
		GetInteger(pResolution, "subtitles", m_ResInfo[iRes].iSubtitles,(int)((1-fSafe)*m_ResInfo[iRes].iHeight),m_ResInfo[iRes].iHeight/2,m_ResInfo[iRes].iHeight*5/4);
		GetFloat(pResolution, "pixelratio", m_ResInfo[iRes].fPixelRatio,128.0f/117.0f,0.5f,2.0f);
		GetInteger(pResolution, "osdyoffset", m_ResInfo[iRes].iOSDYOffset,0,-m_ResInfo[iRes].iHeight,m_ResInfo[iRes].iHeight);

		// get the overscan info
		TiXmlElement *pOverscan = pResolution->FirstChildElement("overscan");
		if (pOverscan)
		{
			GetInteger(pOverscan, "left", m_ResInfo[iRes].Overscan.left,0,-m_ResInfo[iRes].iWidth/4,m_ResInfo[iRes].iWidth/4);
			GetInteger(pOverscan, "top", m_ResInfo[iRes].Overscan.top,0,-m_ResInfo[iRes].iHeight/4,m_ResInfo[iRes].iHeight/4);
			GetInteger(pOverscan, "right", m_ResInfo[iRes].Overscan.right,m_ResInfo[iRes].iWidth,m_ResInfo[iRes].iWidth/2,m_ResInfo[iRes].iWidth*3/2);
			GetInteger(pOverscan, "bottom", m_ResInfo[iRes].Overscan.bottom,m_ResInfo[iRes].iHeight,m_ResInfo[iRes].iHeight/2,m_ResInfo[iRes].iHeight*3/2);
		}
		CLog::Log(LOGINFO, "  calibration for %s %ix%i",m_ResInfo[iRes].strMode,m_ResInfo[iRes].iWidth,m_ResInfo[iRes].iHeight);
		CLog::Log(LOGINFO, "    subtitle yposition:%i pixelratio:%03.3f offsets:(%i,%i)->(%i,%i) osdyoffset:%i",
			m_ResInfo[iRes].iSubtitles, m_ResInfo[iRes].fPixelRatio,
			m_ResInfo[iRes].Overscan.left,m_ResInfo[iRes].Overscan.top,
			m_ResInfo[iRes].Overscan.right,m_ResInfo[iRes].Overscan.bottom,
			m_ResInfo[iRes].iOSDYOffset);

		// iterate around
		pResolution = pResolution->NextSiblingElement("resolution");
	}
  return true;
}

bool CSettings::SaveCalibration(TiXmlNode* pRootNode) const
{
	TiXmlElement xmlRootElement("calibration");
	TiXmlNode *pRoot = pRootNode->InsertEndChild(xmlRootElement);
	for (int i=0; i<10; i++)
	{
		// Write the resolution tag
		TiXmlElement resElement("resolution");
		TiXmlNode *pNode = pRoot->InsertEndChild(resElement);
		// Now write each of the pieces of information we need...
		SetString(pNode, "description", m_ResInfo[i].strMode);
		SetInteger(pNode, "id", i);
		SetInteger(pNode, "subtitles", m_ResInfo[i].iSubtitles);
		SetInteger(pNode, "osdyoffset", m_ResInfo[i].iOSDYOffset);
		SetFloat(pNode, "pixelratio", m_ResInfo[i].fPixelRatio);
		// create the overscan child
		TiXmlElement overscanElement("overscan");
		TiXmlNode *pOverscanNode = pNode->InsertEndChild(overscanElement);
		SetInteger(pOverscanNode, "left", m_ResInfo[i].Overscan.left);
		SetInteger(pOverscanNode, "top", m_ResInfo[i].Overscan.top);
		SetInteger(pOverscanNode, "right", m_ResInfo[i].Overscan.right);
		SetInteger(pOverscanNode, "bottom", m_ResInfo[i].Overscan.bottom);
	}
  return true;
}

bool CSettings::LoadSettings(const CStdString& strSettingsFile, const bool loadprofiles)
{
	// load the xml file
	TiXmlDocument xmlDoc;
	if (!xmlDoc.LoadFile(strSettingsFile))
	{
		g_LoadErrorStr.Format("%s, Line %d\n%s", strSettingsFile.c_str(), xmlDoc.ErrorRow(), xmlDoc.ErrorDesc());
		return false;
	}
	TiXmlElement *pRootElement = xmlDoc.RootElement();
	if (CUtil::cmpnocase(pRootElement->Value(),"settings")!=0)
	{
		g_LoadErrorStr.Format("%s\nDoesn't contain <settings>",strSettingsFile.c_str());
		return false;
	}

  if (loadprofiles) {
    LoadProfiles(pRootElement, strSettingsFile);
    if (m_vecProfiles.size() == 0) {
      //no profiles yet, make one based on the default settings
      CProfile profile;
      profile.setFileName("profile0.xml");
      profile.setName("Default settings");
      m_vecProfiles.push_back(profile);
      SaveSettingsToProfile(0);
    }
  }

	// mypictures
	TiXmlElement *pElement = pRootElement->FirstChildElement("mypictures");
	if (pElement)
	{
		GetInteger(pElement, "picturesviewicons", g_stSettings.m_iMyPicturesViewAsIcons,VIEW_AS_LIST,VIEW_AS_LIST,VIEW_AS_LARGEICONS);
		GetInteger(pElement, "picturesrooticons", g_stSettings.m_iMyPicturesRootViewAsIcons,VIEW_AS_LIST,VIEW_AS_LIST,VIEW_AS_LARGEICONS);
		GetInteger(pElement, "picturessortmethod",g_stSettings.m_iMyPicturesSortMethod,0,0,2);
		GetInteger(pElement, "picturessortmethodroot",g_stSettings.m_iMyPicturesRootSortMethod,0,0,3);
		GetBoolean(pElement, "picturessortascending", g_stSettings.m_bMyPicturesSortAscending);
		GetBoolean(pElement, "picturessortascendingroot", g_stSettings.m_bMyPicturesRootSortAscending);
	}
	// myfiles
	pElement = pRootElement->FirstChildElement("myfiles");
	if (pElement)
	{
		TiXmlElement *pChild = pElement->FirstChildElement("source");
		if (pChild)
		{
			GetBoolean(pChild, "srcfilesviewicons", g_stSettings.m_bMyFilesSourceViewAsIcons);
			GetBoolean(pChild, "srcfilesrooticons", g_stSettings.m_bMyFilesSourceRootViewAsIcons);
			GetInteger(pChild, "srcfilessortmethod",g_stSettings.m_iMyFilesSourceSortMethod,0,0,2);
			GetInteger(pChild, "srcfilessortmethodroot",g_stSettings.m_iMyFilesSourceRootSortMethod,0,0,3);
			GetBoolean(pChild, "srcfilessortascending", g_stSettings.m_bMyFilesSourceSortAscending);
			GetBoolean(pChild, "srcfilessortascendingroot", g_stSettings.m_bMyFilesSourceRootSortAscending);
		}
		pChild = pElement->FirstChildElement("dest");
		if (pChild)
		{
			GetBoolean(pChild, "dstfilesviewicons", g_stSettings.m_bMyFilesDestViewAsIcons);
			GetBoolean(pChild, "dstfilesrooticons", g_stSettings.m_bMyFilesDestRootViewAsIcons);
			GetInteger(pChild, "dstfilessortmethod",g_stSettings.m_iMyFilesDestSortMethod,0,0,2);
			GetInteger(pChild, "dstfilessortmethodroot",g_stSettings.m_iMyFilesDestRootSortMethod,0,0,3);
			GetBoolean(pChild, "dstfilessortascending", g_stSettings.m_bMyFilesDestSortAscending);
			GetBoolean(pChild, "dstfilessortascendingroot", g_stSettings.m_bMyFilesDestRootSortAscending);
		}
	}

	// mymusic settings
	pElement = pRootElement->FirstChildElement("mymusic");
	if (pElement)
	{
		TiXmlElement *pChild = pElement->FirstChildElement("songs");
		if (pChild)
		{
			GetInteger(pChild, "songsviewicons", g_stSettings.m_iMyMusicSongsViewAsIcons,VIEW_AS_LIST,VIEW_AS_LIST,VIEW_AS_LARGEICONS);
			GetInteger(pChild, "songsrooticons", g_stSettings.m_iMyMusicSongsRootViewAsIcons,VIEW_AS_ICONS,VIEW_AS_LIST,VIEW_AS_LARGEICONS);
			GetInteger(pChild, "songssortmethod",g_stSettings.m_iMyMusicSongsSortMethod,0,0,8);
			GetInteger(pChild, "songssortmethodroot",g_stSettings.m_iMyMusicSongsRootSortMethod,0,0,9);
			GetBoolean(pChild, "songssortascending",g_stSettings.m_bMyMusicSongsSortAscending);
			GetBoolean(pChild, "songssortascendingroot",g_stSettings.m_bMyMusicSongsRootSortAscending);
		}
		pChild = pElement->FirstChildElement("album");
		if (pChild)
		{
			GetInteger(pChild, "albumviewicons", g_stSettings.m_iMyMusicAlbumViewAsIcons,VIEW_AS_LIST,VIEW_AS_LIST,VIEW_AS_ICONS);
			GetInteger(pChild, "albumrooticons", g_stSettings.m_iMyMusicAlbumRootViewAsIcons,VIEW_AS_ICONS,VIEW_AS_LIST,VIEW_AS_ICONS);
			GetInteger(pChild, "albumsortmethod",g_stSettings.m_iMyMusicAlbumSortMethod,3,3,5);
			GetInteger(pChild, "albumsortmethodroot",g_stSettings.m_iMyMusicAlbumRootSortMethod,7,6,7);
			GetBoolean(pChild, "albumsortascending",g_stSettings.m_bMyMusicAlbumSortAscending);
			GetBoolean(pChild, "albumsortascendingroot",g_stSettings.m_bMyMusicAlbumRootSortAscending);
			GetBoolean(pChild, "albumshowrecentalbums",g_stSettings.m_bMyMusicAlbumShowRecent);
		}
		pChild = pElement->FirstChildElement("artist");
		if (pChild)
		{
			GetInteger(pChild, "artistsongsviewicons", g_stSettings.m_iMyMusicArtistsSongsViewAsIcons,VIEW_AS_LIST,VIEW_AS_LIST,VIEW_AS_LARGEICONS);
			GetInteger(pChild, "artistalbumsviewicons", g_stSettings.m_iMyMusicArtistsAlbumsViewAsIcons,VIEW_AS_LIST,VIEW_AS_LIST,VIEW_AS_LARGEICONS);
			GetInteger(pChild, "artistrooticons", g_stSettings.m_iMyMusicArtistsRootViewAsIcons,VIEW_AS_LIST,VIEW_AS_LIST,VIEW_AS_LARGEICONS);
			GetInteger(pChild, "artistallsongssortmethod",g_stSettings.m_iMyMusicArtistsAllSongsSortMethod,4,5,5); //title
			GetInteger(pChild, "artistsortmethodroot",g_stSettings.m_iMyMusicArtistsRootSortMethod,0,0,0); //Only name (??)
			GetInteger(pChild, "artistalbumsortmethod",g_stSettings.m_iMyMusicArtistsAlbumsSortMethod,7,6,7);
			GetInteger(pChild, "artistalbumsongssortmethod",g_stSettings.m_iMyMusicArtistsAlbumsSongsSortMethod,3,3,5);
			GetBoolean(pChild, "artistsortalbumsascending",g_stSettings.m_bMyMusicArtistsAlbumsSortAscending);
			GetBoolean(pChild, "artistsortallsongsascending",g_stSettings.m_bMyMusicArtistsAllSongsSortAscending);
			GetBoolean(pChild, "artistsortalbumsongsascending",g_stSettings.m_bMyMusicArtistsAlbumSongsSortAscending);
			GetBoolean(pChild, "artistsortascendingroot",g_stSettings.m_bMyMusicArtistsRootSortAscending);
		}
		pChild = pElement->FirstChildElement("genre");
		if (pChild)
		{
			GetInteger(pChild, "genreviewicons", g_stSettings.m_iMyMusicGenresViewAsIcons,VIEW_AS_LIST,VIEW_AS_LIST,VIEW_AS_LARGEICONS);
			GetInteger(pChild, "genrerooticons", g_stSettings.m_iMyMusicGenresRootViewAsIcons,VIEW_AS_LIST,VIEW_AS_LIST,VIEW_AS_LARGEICONS);
			GetInteger(pChild, "genresortmethod",g_stSettings.m_iMyMusicGenresSortMethod,5,4,5);	//	titel
			GetInteger(pChild, "genresortmethodroot",g_stSettings.m_iMyMusicGenresRootSortMethod,0,0,0); // Only name (??)
			GetBoolean(pChild, "genresortascending",g_stSettings.m_bMyMusicGenresSortAscending);
			GetBoolean(pChild, "genresortascendingroot",g_stSettings.m_bMyMusicGenresRootSortAscending);
		}
		pChild = pElement->FirstChildElement("top100");
		if (pChild)
		{
			GetInteger(pChild, "top100rooticons", g_stSettings.m_iMyMusicTop100ViewAsIcons,VIEW_AS_LIST,VIEW_AS_ICONS,VIEW_AS_ICONS);
		}
		pChild = pElement->FirstChildElement("playlist");
		if (pChild)
		{
			GetInteger(pChild, "playlistrooticons", g_stSettings.m_iMyMusicPlaylistViewAsIcons,VIEW_AS_LIST,VIEW_AS_LIST,VIEW_AS_LARGEICONS);
			GetBoolean(pChild, "repeat", g_stSettings.m_bMyMusicPlaylistRepeat);
			GetBoolean(pChild, "shuffle", g_stSettings.m_bMyMusicPlaylistShuffle);
			
		}
		GetInteger(pElement, "startwindow",g_stSettings.m_iMyMusicStartWindow,WINDOW_MUSIC_FILES,WINDOW_MUSIC_FILES,WINDOW_MUSIC_TOP100);//501; view songs
		GetBoolean(pElement, "songinfoinvis",g_stSettings.m_bMyMusicSongInfoInVis);
    GetBoolean(pElement, "songthumbinvis", g_stSettings.m_bMyMusicSongThumbInVis);
	}
	// myvideos settings
	pElement = pRootElement->FirstChildElement("myvideos");
	if (pElement)
	{
		GetInteger(pElement, "startwindow",g_stSettings.m_iVideoStartWindow,WINDOW_VIDEOS,WINDOW_VIDEO_GENRE,WINDOW_VIDEO_TITLE);
		GetInteger(pElement, "stackvideomode", g_stSettings.m_iMyVideoVideoStack, STACK_NONE, STACK_NONE, STACK_FUZZY);
		GetBoolean(pElement, "stackgenre", g_stSettings.m_bMyVideoGenreStack);
		GetBoolean(pElement, "stackactor", g_stSettings.m_bMyVideoActorStack);
		GetBoolean(pElement, "stackyear", g_stSettings.m_bMyVideoYearStack);
		GetString(pElement, "stacktokens", g_stSettings.m_szMyVideoStackTokens, g_stSettings.m_szMyVideoStackTokens);
		GetString(pElement, "stackseparators", g_stSettings.m_szMyVideoStackSeparators, g_stSettings.m_szMyVideoStackSeparators);

		StringUtils::SplitString(g_stSettings.m_szMyVideoStackTokens, "|", g_settings.m_szMyVideoStackTokensArray);
		g_settings.m_szMyVideoStackSeparatorsString = g_stSettings.m_szMyVideoStackSeparators;

		for (int i = 0; i < (int)g_settings.m_szMyVideoStackTokensArray.size(); i++)
			g_settings.m_szMyVideoStackTokensArray[i].MakeLower();

		GetBoolean(pElement, "cleantitles", g_stSettings.m_bMyVideoCleanTitles);
		GetString(pElement, "cleantokens", g_stSettings.m_szMyVideoCleanTokens, g_stSettings.m_szMyVideoCleanTokens);
		GetString(pElement, "cleanseparators", g_stSettings.m_szMyVideoCleanSeparators, g_stSettings.m_szMyVideoCleanSeparators);

		StringUtils::SplitString(g_stSettings.m_szMyVideoCleanTokens, "|", g_settings.m_szMyVideoCleanTokensArray);
		g_settings.m_szMyVideoCleanSeparatorsString = g_stSettings.m_szMyVideoCleanSeparators;

		for (int i = 0; i < (int)g_settings.m_szMyVideoCleanTokensArray.size(); i++)
			g_settings.m_szMyVideoCleanTokensArray[i].MakeLower();

		GetInteger(pElement, "videoplaylistviewicons", g_stSettings.m_iMyVideoPlaylistViewAsIcons,VIEW_AS_LIST,VIEW_AS_LIST,VIEW_AS_LARGEICONS);
		GetBoolean(pElement, "videoplaylistrepeat",g_stSettings.m_bMyVideoPlaylistRepeat);
		GetBoolean(pElement, "videoplaylistshuffle",g_stSettings.m_bMyVideoPlaylistShuffle);
		GetInteger(pElement, "videoviewicons", g_stSettings.m_iMyVideoViewAsIcons,VIEW_AS_LIST,VIEW_AS_LIST,VIEW_AS_LARGEICONS);
		GetInteger(pElement, "videorooticons", g_stSettings.m_iMyVideoRootViewAsIcons,VIEW_AS_LIST,VIEW_AS_LIST,VIEW_AS_LARGEICONS);
		GetInteger(pElement, "videosortmethod",g_stSettings.m_iMyVideoSortMethod,0,0,2);
		GetInteger(pElement, "videosortmethodroot",g_stSettings.m_iMyVideoRootSortMethod,0,0,3);
		GetBoolean(pElement, "videosortascending", g_stSettings.m_bMyVideoSortAscending);
		GetBoolean(pElement, "videosortascendingroot", g_stSettings.m_bMyVideoRootSortAscending);

		GetInteger(pElement, "genreviewicons", g_stSettings.m_iMyVideoGenreViewAsIcons,VIEW_AS_LIST,VIEW_AS_LIST,VIEW_AS_LARGEICONS);
		GetInteger(pElement, "genrerooticons", g_stSettings.m_iMyVideoGenreRootViewAsIcons,VIEW_AS_LIST,VIEW_AS_LIST,VIEW_AS_LARGEICONS);
		GetInteger(pElement, "genresortmethod",g_stSettings.m_iMyVideoGenreSortMethod,0,0,2);
		GetInteger(pElement, "genresortmethodroot",g_stSettings.m_iMyVideoGenreRootSortMethod,0,0,0);	//	by label only
		GetBoolean(pElement, "genresortascending", g_stSettings.m_bMyVideoGenreSortAscending);
		GetBoolean(pElement, "genresortascendingroot", g_stSettings.m_bMyVideoGenreRootSortAscending);

		GetInteger(pElement, "actorviewicons", g_stSettings.m_iMyVideoActorViewAsIcons,VIEW_AS_LIST,VIEW_AS_LIST,VIEW_AS_LARGEICONS);
		GetInteger(pElement, "actorrooticons", g_stSettings.m_iMyVideoActorRootViewAsIcons,VIEW_AS_LIST,VIEW_AS_LIST,VIEW_AS_LARGEICONS);
		GetInteger(pElement, "actorsortmethod",g_stSettings.m_iMyVideoActorSortMethod,0,0,2);
		GetInteger(pElement, "actorsortmethodroot",g_stSettings.m_iMyVideoActorRootSortMethod,0,0,0);	//	by label only
		GetBoolean(pElement, "actorsortascending", g_stSettings.m_bMyVideoActorSortAscending);
		GetBoolean(pElement, "actorsortascendingroot", g_stSettings.m_bMyVideoActorRootSortAscending);

		GetInteger(pElement, "yearviewicons", g_stSettings.m_iMyVideoYearViewAsIcons,VIEW_AS_LIST,VIEW_AS_LIST,VIEW_AS_LARGEICONS);
		GetInteger(pElement, "yearrooticons", g_stSettings.m_iMyVideoYearRootViewAsIcons,VIEW_AS_LIST,VIEW_AS_LIST,VIEW_AS_LARGEICONS);
		GetInteger(pElement, "yearsortmethod",g_stSettings.m_iMyVideoYearSortMethod,0,0,2);
		GetInteger(pElement, "yearsortmethodroot",g_stSettings.m_iMyVideoYearRootSortMethod,0,0,0);	//	by label only
		GetBoolean(pElement, "yearsortascending", g_stSettings.m_bMyVideoYearSortAscending);
		GetBoolean(pElement, "yearsortascendingroot", g_stSettings.m_bMyVideoYearRootSortAscending);

		GetInteger(pElement, "titleviewicons", g_stSettings.m_iMyVideoTitleViewAsIcons,VIEW_AS_LIST,VIEW_AS_LIST,VIEW_AS_LARGEICONS);
		GetInteger(pElement, "titlerooticons", g_stSettings.m_iMyVideoTitleRootViewAsIcons,VIEW_AS_LIST,VIEW_AS_LIST,VIEW_AS_LARGEICONS);
		GetInteger(pElement, "titlesortmethod",g_stSettings.m_iMyVideoTitleSortMethod,0,0,3);
		GetBoolean(pElement, "titlesortascending", g_stSettings.m_bMyVideoTitleSortAscending);

		GetInteger(pElement, "smallstepbackseconds", g_stSettings.m_iSmallStepBackSeconds,7,1,INT_MAX);
		GetInteger(pElement, "smallstepbacktries", g_stSettings.m_iSmallStepBackTries,3,1,10);
		GetInteger(pElement, "smallstepbackdelay", g_stSettings.m_iSmallStepBackDelay,300,100,5000); //MS
	}
	// myscripts settings
	pElement = pRootElement->FirstChildElement("myscripts");
	if (pElement)
	{
		GetBoolean(pElement, "scriptsviewicons", g_stSettings.m_bScriptsViewAsIcons);
		GetBoolean(pElement, "scriptsrooticons", g_stSettings.m_bScriptsRootViewAsIcons);
		GetInteger(pElement, "scriptssortmethod",g_stSettings.m_iScriptsSortMethod,0,0,2);
		GetBoolean(pElement, "scriptssortascending", g_stSettings.m_bScriptsSortAscending);
	}
	// general settings
	pElement = pRootElement->FirstChildElement("general");
	if (pElement)
	{
		GetInteger(pElement, "audiostream",g_stSettings.m_iAudioStream,-1,-1,INT_MAX);

		GetString(pElement, "kaiarenapass",	g_stSettings.szOnlineArenaPassword, "");
		GetString(pElement, "kaiarenadesc",	g_stSettings.szOnlineArenaDescription, "");
	}

	// screen settings
	pElement = pRootElement->FirstChildElement("screen");
	if (pElement)
	{
		GetInteger(pElement, "viewmode", g_stSettings.m_iViewMode, VIEW_MODE_NORMAL, VIEW_MODE_NORMAL, VIEW_MODE_CUSTOM);
		GetFloat(pElement, "zoomamount", g_stSettings.m_fCustomZoomAmount, 1.0f, 1.0f, 2.0f);
		GetFloat(pElement, "pixelratio", g_stSettings.m_fCustomPixelRatio, 1.0f, 0.5f, 2.0f);
	}
	// audio settings
	pElement = pRootElement->FirstChildElement("audio");
	if (pElement)
	{
		GetInteger(pElement, "volumelevel", g_stSettings.m_nVolumeLevel, VOLUME_MAXIMUM, VOLUME_MINIMUM, VOLUME_MAXIMUM);
	}

	// my programs
	pElement = pRootElement->FirstChildElement("myprograms");
	if (pElement)
	{
		GetInteger(pElement, "programsviewicons", g_stSettings.m_iMyProgramsViewAsIcons,1,0,2);
		GetInteger(pElement, "programssortmethod", g_stSettings.m_iMyProgramsSortMethod,0,0,3);
		GetBoolean(pElement, "programssortascending", g_stSettings.m_bMyProgramsSortAscending);
	}

	LoadCalibration(pRootElement, strSettingsFile);

	g_guiSettings.LoadXML(pRootElement);

	return true;
}

bool CSettings::SaveSettings(const CStdString& strSettingsFile, const bool saveprofiles) const
{
	TiXmlDocument xmlDoc;
	TiXmlElement xmlRootElement("settings");
	TiXmlNode *pRoot = xmlDoc.InsertEndChild(xmlRootElement);
	if (!pRoot) return false;
	// write our tags one by one - just a big list for now (can be flashed up later)

  if (saveprofiles) {
    SaveProfiles(pRoot);
  }

	// myprograms settings
	TiXmlElement programsNode("myprograms");
	TiXmlNode *pNode = pRoot->InsertEndChild(programsNode);
	if (!pNode) return false;
	SetInteger(pNode, "programsviewicons", g_stSettings.m_iMyProgramsViewAsIcons);
	SetInteger(pNode, "programssortmethod", g_stSettings.m_iMyProgramsSortMethod);
	SetBoolean(pNode, "programssortascending", g_stSettings.m_bMyProgramsSortAscending);

	// mypictures settings
	TiXmlElement picturesNode("mypictures");
	pNode = pRoot->InsertEndChild(picturesNode);
	if (!pNode) return false;
	SetInteger(pNode, "picturesviewicons", g_stSettings.m_iMyPicturesViewAsIcons);
	SetInteger(pNode, "picturesrooticons", g_stSettings.m_iMyPicturesRootViewAsIcons);
	SetInteger(pNode, "picturessortmethod",g_stSettings.m_iMyPicturesSortMethod);
	SetInteger(pNode, "picturessortmethodroot",g_stSettings.m_iMyPicturesRootSortMethod);
	SetBoolean(pNode, "picturessortascending", g_stSettings.m_bMyPicturesSortAscending);
	SetBoolean(pNode, "picturessortascendingroot", g_stSettings.m_bMyPicturesRootSortAscending);

	// myfiles settings
	TiXmlElement filesNode("myfiles");
	pNode = pRoot->InsertEndChild(filesNode);
	if (!pNode) return false;
	{
		TiXmlElement childNode("source");
		TiXmlNode *pChild = pNode->InsertEndChild(childNode);
		if (!pChild) return false;
		SetBoolean(pChild, "srcfilesviewicons", g_stSettings.m_bMyFilesSourceViewAsIcons);
		SetBoolean(pChild, "srcfilesrooticons", g_stSettings.m_bMyFilesSourceRootViewAsIcons);
		SetInteger(pChild, "srcfilessortmethod",g_stSettings.m_iMyFilesSourceSortMethod);
		SetInteger(pChild, "srcfilessortmethodroot",g_stSettings.m_iMyFilesSourceRootSortMethod);
		SetBoolean(pChild, "srcfilessortascending", g_stSettings.m_bMyFilesSourceSortAscending);
		SetBoolean(pChild, "srcfilessortascendingroot", g_stSettings.m_bMyFilesSourceRootSortAscending);
	}
	{
		TiXmlElement childNode("dest");
		TiXmlNode *pChild = pNode->InsertEndChild(childNode);
		if (!pChild) return false;
		SetBoolean(pChild, "dstfilesviewicons", g_stSettings.m_bMyFilesDestViewAsIcons);
		SetBoolean(pChild, "dstfilesrooticons", g_stSettings.m_bMyFilesDestRootViewAsIcons);
		SetInteger(pChild, "dstfilessortmethod",g_stSettings.m_iMyFilesDestSortMethod);
		SetInteger(pChild, "dstfilessortmethodroot",g_stSettings.m_iMyFilesDestRootSortMethod);
		SetBoolean(pChild, "dstfilessortascending", g_stSettings.m_bMyFilesDestSortAscending);
		SetBoolean(pChild, "dstfilessortascendingroot", g_stSettings.m_bMyFilesDestRootSortAscending);
	}

	// mymusic settings
	TiXmlElement musicNode("mymusic");
	pNode = pRoot->InsertEndChild(musicNode);
	if (!pNode) return false;
	{
		TiXmlElement childNode("songs");
		TiXmlNode *pChild = pNode->InsertEndChild(childNode);
		if (!pChild) return false;
		SetInteger(pChild, "songsviewicons", g_stSettings.m_iMyMusicSongsViewAsIcons);
		SetInteger(pChild, "songsrooticons", g_stSettings.m_iMyMusicSongsRootViewAsIcons);
		SetInteger(pChild, "songssortmethod",g_stSettings.m_iMyMusicSongsSortMethod);
		SetInteger(pChild, "songssortmethodroot",g_stSettings.m_iMyMusicSongsRootSortMethod);
		SetBoolean(pChild, "songssortascending",g_stSettings.m_bMyMusicSongsSortAscending);
		SetBoolean(pChild, "songssortascendingroot",g_stSettings.m_bMyMusicSongsRootSortAscending);
	}
	{
		TiXmlElement childNode("album");
		TiXmlNode *pChild = pNode->InsertEndChild(childNode);
		if (!pChild) return false;
		SetInteger(pChild, "albumviewicons", g_stSettings.m_iMyMusicAlbumViewAsIcons);
		SetInteger(pChild, "albumrooticons", g_stSettings.m_iMyMusicAlbumRootViewAsIcons);
		SetInteger(pChild, "albumsortmethod",g_stSettings.m_iMyMusicAlbumSortMethod);
		SetInteger(pChild, "albumsortmethodroot",g_stSettings.m_iMyMusicAlbumRootSortMethod);
		SetBoolean(pChild, "albumsortascending",g_stSettings.m_bMyMusicAlbumSortAscending);
		SetBoolean(pChild, "albumsortascendingroot",g_stSettings.m_bMyMusicAlbumRootSortAscending);
		SetBoolean(pChild, "albumshowrecentalbums",g_stSettings.m_bMyMusicAlbumShowRecent);
	}
	{
		TiXmlElement childNode("artist");
		TiXmlNode *pChild = pNode->InsertEndChild(childNode);
		if (!pChild) return false;
		SetInteger(pChild, "artistsongsviewicons", g_stSettings.m_iMyMusicArtistsSongsViewAsIcons);
		SetInteger(pChild, "artistalbumsviewicons", g_stSettings.m_iMyMusicArtistsAlbumsViewAsIcons);
		SetInteger(pChild, "artistrooticons", g_stSettings.m_iMyMusicArtistsRootViewAsIcons);
		SetInteger(pChild, "artistallsongssortmethod",g_stSettings.m_iMyMusicArtistsAllSongsSortMethod);
		SetInteger(pChild, "artistsortmethodroot",g_stSettings.m_iMyMusicArtistsRootSortMethod);
		SetInteger(pChild, "artistalbumsortmethod",g_stSettings.m_iMyMusicArtistsAlbumsSortMethod);
		SetInteger(pChild, "artistalbumsongssortmethod",g_stSettings.m_iMyMusicArtistsAlbumsSongsSortMethod);
		SetBoolean(pChild, "artistsortalbumsascending",g_stSettings.m_bMyMusicArtistsAlbumsSortAscending);
		SetBoolean(pChild, "artistsortallsongsascending",g_stSettings.m_bMyMusicArtistsAllSongsSortAscending);
		SetBoolean(pChild, "artistsortalbumsongsascending",g_stSettings.m_bMyMusicArtistsAlbumSongsSortAscending);
		SetBoolean(pChild, "artistsortascendingroot",g_stSettings.m_bMyMusicArtistsRootSortAscending);
	}
	{
		TiXmlElement childNode("genre");
		TiXmlNode *pChild = pNode->InsertEndChild(childNode);
		if (!pChild) return false;
		SetInteger(pChild, "genreviewicons", g_stSettings.m_iMyMusicGenresViewAsIcons);
		SetInteger(pChild, "genrerooticons", g_stSettings.m_iMyMusicGenresRootViewAsIcons);
		SetInteger(pChild, "genresortmethod",g_stSettings.m_iMyMusicGenresSortMethod);
		SetInteger(pChild, "genresortmethodroot",g_stSettings.m_iMyMusicGenresRootSortMethod);
		SetBoolean(pChild, "genresortascending",g_stSettings.m_bMyMusicGenresSortAscending);
		SetBoolean(pChild, "genresortascendingroot",g_stSettings.m_bMyMusicGenresRootSortAscending);
	}
	{
		TiXmlElement childNode("top100");
		TiXmlNode *pChild = pNode->InsertEndChild(childNode);
		if (!pChild) return false;
		SetInteger(pChild, "top100rooticons", g_stSettings.m_iMyMusicTop100ViewAsIcons);
	}
	{
		TiXmlElement childNode("playlist");
		TiXmlNode *pChild = pNode->InsertEndChild(childNode);
		if (!pChild) return false;
		SetInteger(pChild, "playlistrooticons", g_stSettings.m_iMyMusicPlaylistViewAsIcons);
		SetBoolean(pChild, "repeat", g_stSettings.m_bMyMusicPlaylistRepeat);
		SetBoolean(pChild, "shuffle", g_stSettings.m_bMyMusicPlaylistShuffle);
	}

	SetInteger(pNode, "startwindow",g_stSettings.m_iMyMusicStartWindow);
	SetBoolean(pNode, "songinfoinvis",g_stSettings.m_bMyMusicSongInfoInVis);
  SetBoolean(pNode, "songthumbinvis", g_stSettings.m_bMyMusicSongThumbInVis);

	// myvideos settings
	TiXmlElement videosNode("myvideos");
	pNode = pRoot->InsertEndChild(videosNode);
	if (!pNode) return false;

	SetInteger(pNode, "startwindow",g_stSettings.m_iVideoStartWindow);
	SetInteger(pNode, "stackvideomode", g_stSettings.m_iMyVideoVideoStack);
	SetBoolean(pNode, "stackgenre", g_stSettings.m_bMyVideoGenreStack);
	SetBoolean(pNode, "stackactor", g_stSettings.m_bMyVideoActorStack);
	SetBoolean(pNode, "stackyear", g_stSettings.m_bMyVideoYearStack);
	SetString(pNode, "stacktokens", g_stSettings.m_szMyVideoStackTokens);
	SetString(pNode, "stackseparators", g_stSettings.m_szMyVideoStackSeparators);

	SetBoolean(pNode, "cleantitles", g_stSettings.m_bMyVideoCleanTitles);
	SetString(pNode, "cleantokens", g_stSettings.m_szMyVideoCleanTokens);
	SetString(pNode, "cleanseparators", g_stSettings.m_szMyVideoCleanSeparators);

	SetInteger(pNode, "videoplaylistviewicons", g_stSettings.m_iMyVideoPlaylistViewAsIcons);
	SetBoolean(pNode, "videoplaylistrepeat", g_stSettings.m_bMyVideoPlaylistRepeat);
	SetBoolean(pNode, "videoplaylistshuffle", g_stSettings.m_bMyVideoPlaylistShuffle);
	SetInteger(pNode, "videoviewicons", g_stSettings.m_iMyVideoViewAsIcons);
	SetInteger(pNode, "videorooticons", g_stSettings.m_iMyVideoRootViewAsIcons);
	SetInteger(pNode, "videosortmethod",g_stSettings.m_iMyVideoSortMethod);
	SetInteger(pNode, "videosortmethodroot",g_stSettings.m_iMyVideoRootSortMethod);
	SetBoolean(pNode, "videosortascending", g_stSettings.m_bMyVideoSortAscending);
	SetBoolean(pNode, "videosortascendingroot", g_stSettings.m_bMyVideoRootSortAscending);

	SetInteger(pNode, "genreviewicons", g_stSettings.m_iMyVideoGenreViewAsIcons);
	SetInteger(pNode, "genrerooticons", g_stSettings.m_iMyVideoGenreRootViewAsIcons);
	SetInteger(pNode, "genresortmethod",g_stSettings.m_iMyVideoGenreSortMethod);
	SetInteger(pNode, "genresortmethodroot",g_stSettings.m_iMyVideoGenreRootSortMethod);
	SetBoolean(pNode, "genresortascending", g_stSettings.m_bMyVideoGenreSortAscending);
	SetBoolean(pNode, "genresortascendingroot", g_stSettings.m_bMyVideoGenreRootSortAscending);

	SetInteger(pNode, "actorviewicons", g_stSettings.m_iMyVideoActorViewAsIcons);
	SetInteger(pNode, "actorrooticons", g_stSettings.m_iMyVideoActorRootViewAsIcons);
	SetInteger(pNode, "actorsortmethod",g_stSettings.m_iMyVideoActorSortMethod);
	SetInteger(pNode, "actorsortmethodroot",g_stSettings.m_iMyVideoActorRootSortMethod);
	SetBoolean(pNode, "actorsortascending", g_stSettings.m_bMyVideoActorSortAscending);
	SetBoolean(pNode, "actorsortascendingroot", g_stSettings.m_bMyVideoActorRootSortAscending);

	SetInteger(pNode, "yearviewicons", g_stSettings.m_iMyVideoYearViewAsIcons);
	SetInteger(pNode, "yearrooticons", g_stSettings.m_iMyVideoYearRootViewAsIcons);
	SetInteger(pNode, "yearsortmethod",g_stSettings.m_iMyVideoYearSortMethod);
	SetInteger(pNode, "yearsortmethodroot",g_stSettings.m_iMyVideoYearRootSortMethod);
	SetBoolean(pNode, "yearsortascending", g_stSettings.m_bMyVideoYearSortAscending);
	SetBoolean(pNode, "yearsortascendingroot", g_stSettings.m_bMyVideoYearRootSortAscending);

	SetInteger(pNode, "titleviewicons", g_stSettings.m_iMyVideoTitleViewAsIcons);
	SetInteger(pNode, "titlerooticons", g_stSettings.m_iMyVideoTitleRootViewAsIcons);
	SetInteger(pNode, "titlesortmethod",g_stSettings.m_iMyVideoTitleSortMethod);
	SetBoolean(pNode, "titlesortascending", g_stSettings.m_bMyVideoTitleSortAscending);

	SetInteger(pNode, "smallstepbackseconds", g_stSettings.m_iSmallStepBackSeconds);
	SetInteger(pNode, "smallstepbacktries", g_stSettings.m_iSmallStepBackTries);
	SetInteger(pNode, "smallstepbackdelay", g_stSettings.m_iSmallStepBackDelay);

	// myscripts settings
	TiXmlElement scriptsNode("myscripts");
	pNode = pRoot->InsertEndChild(scriptsNode);
	if (!pNode) return false;
	SetBoolean(pNode, "scriptsviewicons", g_stSettings.m_bScriptsViewAsIcons);
	SetBoolean(pNode, "scriptsrooticons", g_stSettings.m_bScriptsRootViewAsIcons);
	SetInteger(pNode, "scriptssortmethod",g_stSettings.m_iScriptsSortMethod);
	SetBoolean(pNode, "scriptssortascending", g_stSettings.m_bScriptsSortAscending);

	// general settings
	TiXmlElement generalNode("general");
	pNode = pRoot->InsertEndChild(generalNode);
	if (!pNode) return false;
	SetString(pNode, "kaiarenapass",	g_stSettings.szOnlineArenaPassword);
	SetString(pNode, "kaiarenadesc",	g_stSettings.szOnlineArenaDescription);

	// screen settings
	TiXmlElement screenNode("screen");
	pNode = pRoot->InsertEndChild(screenNode);
	if (!pNode) return false;
	SetInteger(pNode, "viewmode", g_stSettings.m_iViewMode);
	SetFloat(pNode, "zoomamount", g_stSettings.m_fCustomZoomAmount);
	SetFloat(pNode, "pixelratio", g_stSettings.m_fCustomPixelRatio);

	// audio settings
	TiXmlElement audioNode("audio");
	pNode = pRoot->InsertEndChild(audioNode);
	if (!pNode) return false;

	SetInteger(pNode, "volumelevel", g_stSettings.m_nVolumeLevel);

  SaveCalibration(pRoot);

	g_guiSettings.SaveXML(pRoot);
	// save the file
	return xmlDoc.SaveFile(strSettingsFile);
}

bool CSettings::LoadProfile(int index)
{
  CProfile& profile = m_vecProfiles.at(index);
  if (LoadSettings("T:\\" + profile.getFileName(), false))
  {
    m_iLastLoadedProfileIndex = index;
    Save();
    return true;
  }
  return false;
}

void CSettings::DeleteProfile(int index)
{
  for (IVECPROFILES iProfile = g_settings.m_vecProfiles.begin(); iProfile != g_settings.m_vecProfiles.end(); ++iProfile)
  {
	  if (iProfile == &g_settings.m_vecProfiles.at(index))
	  {
      if (index == m_iLastLoadedProfileIndex) {m_iLastLoadedProfileIndex = -1;}
      ::DeleteFile("T:\\" + iProfile->getFileName());
      m_vecProfiles.erase(iProfile);
      Save();
      break;
	  }
  }
}

bool CSettings::SaveSettingsToProfile(int index)
{
  CProfile& profile = m_vecProfiles.at(index);
  return SaveSettings("T:\\" + profile.getFileName(), false);
}


bool CSettings::LoadProfiles(const TiXmlElement* pRootElement, const CStdString& strSettingsFile)
{
 	CLog::Log(LOGINFO, "  Parsing <profiles> tag");
	const TiXmlElement *pChild = pRootElement->FirstChildElement("profiles");
	if (pChild)
	{
    GetInteger(pChild, "lastloaded", m_iLastLoadedProfileIndex, -1, -1, INT_MAX);
		TiXmlNode *pChildNode = pChild->FirstChild();
		while (pChildNode>0)
		{
			CStdString strValue=pChildNode->Value();
			if (strValue=="profile")
			{
				const TiXmlNode *pProfileName=pChildNode->FirstChild("name");
				const TiXmlNode *pProfileFile=pChildNode->FirstChild("file");
				if (pProfileName && pProfileFile)
				{
					const char* szName=pProfileName->FirstChild()->Value();
					CLog::Log(LOGDEBUG, "    Profile Name: %s", szName);
					const char* szPath=pProfileFile->FirstChild()->Value();
					CLog::Log(LOGDEBUG, "    Profile Filename: %s", szPath);

					CProfile profile;
          CStdString str = szName;
					profile.setName(str);
          str = szPath;
					profile.setFileName(str);
          m_vecProfiles.push_back(profile);
				}
				else
				{
					CLog::Log(LOGERROR, "    <name> and/or <file> not properly defined within <profile>");
				}
			}
			pChildNode=pChildNode->NextSibling();
		}
    return true;
	}
	else
	{
		CLog::Log(LOGERROR, "  <profiles> tag is missing or %s is malformed", strSettingsFile.c_str());
    return false;
	}
}


bool CSettings::SaveProfiles(TiXmlNode* pRootElement) const
{
	TiXmlElement xmlProfilesElement("profiles");
	TiXmlNode *pProfileNode = pRootElement->InsertEndChild(xmlProfilesElement);
  if (!pProfileNode) return false;
  SetInteger(pProfileNode, "lastloaded", m_iLastLoadedProfileIndex);
  for (int i=0; i<(int)m_vecProfiles.size(); ++i)
  {
    const CProfile& profile=m_vecProfiles.at(i);

		TiXmlElement profileElement("profile");
		TiXmlNode *pNode = pProfileNode->InsertEndChild(profileElement);
		if (!pNode) return false;
		SetString(pNode, "name", profile.getName());
		SetString(pNode, "file", profile.getFileName());
  }
  return true;
}

bool CSettings::LoadXml()
{
	// load xml file - we use the xbe path in case we were loaded as dash
	if (!xbmcXmlLoaded)
	{
		CStdString strPath;
		char szXBEFileName[1024];
		CIoSupport helper;
		helper.GetXbePath(szXBEFileName);
		strrchr(szXBEFileName,'\\')[0] = 0;
		strPath.Format("%s\\%s", szXBEFileName, "XboxMediaCenter.xml");
		if ( !xbmcXml.LoadFile( strPath.c_str() ) )
		{
			return false;
		}
		xbmcXmlLoaded = true;
	}
	return true;
}

bool CSettings::UpdateBookmark(const CStdString &strType, const CStdString &strOldName, const CStdString &strUpdateElement, const CStdString &strUpdateText)
{
	if (!LoadXml()) return false;

	VECSHARES *pShares = NULL;
	if (strType == "programs") pShares = &m_vecMyProgramsBookmarks;
	if (strType == "files") pShares = &m_vecMyFilesShares;
	if (strType == "music") pShares = &m_vecMyMusicShares;
	if (strType == "videos") pShares = &m_vecMyVideoShares;
	if (strType == "pictures") pShares = &m_vecMyPictureShares;

	if (!pShares) return false;

	for (IVECSHARES it = pShares->begin(); it != pShares->end(); it++)
	{
		if ((*it).strName == strOldName)
		{
     	if ("name" == strUpdateElement)
       	(*it).strName = strUpdateText;
     	else if ("path" == strUpdateElement)
       	(*it).strPath = strUpdateText;
     	else if ("lockmode" == strUpdateElement)
       	(*it).m_iLockMode = atoi(strUpdateText);
     	else if ("lockcode" == strUpdateElement)
       	(*it).m_strLockCode = strUpdateText;
     	else if ("badpwdcount" == strUpdateElement)
       	(*it).m_iBadPwdCount = atoi(strUpdateText);
     	else
       	return false;
		}
	}
	// Return bookmark of
	TiXmlElement *pRootElement = xbmcXml.RootElement();
	TiXmlNode *pNode = NULL;
	TiXmlNode *pIt = NULL;

	pNode = pRootElement->FirstChild(strType);

	// if valid bookmark, find child at pos (id)
	if (pNode)
	{
		pIt = pNode->FirstChild("bookmark");
		while (pIt)
		{
			TiXmlNode *pChild = pIt->FirstChild("name");
			if (pChild && pChild->FirstChild()->Value() == strOldName)
			{
				pChild = pIt->FirstChild(strUpdateElement);
				if (pChild)
       	{
        	pIt->FirstChild(strUpdateElement)->FirstChild()->SetValue(strUpdateText);
       	}
      	else
       	{
        	TiXmlText xmlText(strUpdateText);
        	TiXmlElement eElement(strUpdateElement);
        	eElement.InsertEndChild(xmlText);
        	pIt->ToElement()->InsertEndChild(eElement);
        }
       	break;
			}
			else
				pIt = pIt->NextSibling("bookmark");
		}
	}
	return xbmcXml.SaveFile();
}

bool CSettings::DeleteBookmark(const CStdString &strType, const CStdString &strName, const CStdString &strPath)
{
	if (!LoadXml()) return false;

	VECSHARES *pShares = NULL;
	if (strType == "files") pShares = &m_vecMyFilesShares;
	if (strType == "music") pShares = &m_vecMyMusicShares;
	if (strType == "videos") pShares = &m_vecMyVideoShares;
	if (strType == "pictures") pShares = &m_vecMyPictureShares;

	if (!pShares) return false;

	for (IVECSHARES it = pShares->begin(); it != pShares->end(); it++)
	{
		if ((*it).strName == strName && (*it).strPath == strPath)
		{
			pShares->erase(it);
			break;
		}
	}
	// Return bookmark of
	TiXmlElement *pRootElement = xbmcXml.RootElement();
	TiXmlNode *pNode = NULL;
	TiXmlNode *pIt = NULL;

	pNode = pRootElement->FirstChild(strType);

	// if valid bookmark, find child at pos (id)
	if (pNode)
	{
		pIt = pNode->FirstChild("bookmark");
		while (pIt)
		{
			TiXmlNode *pChild = pIt->FirstChild("name");
			if (pChild && pChild->FirstChild()->Value() == strName)
			{
				pChild->FirstChild()->SetValue(strName);
				pChild = pIt->FirstChild("path");
				if (pChild && pChild->FirstChild()->Value() == strPath)
				{
					pNode->RemoveChild(pIt);
					break;
				}
			}
			else
				pIt = pIt->NextSibling("bookmark");
		}
	}
	return xbmcXml.SaveFile();
}

bool CSettings::AddBookmark(const CStdString &strType, const CStdString &strName, const CStdString &strPath)
{
	if (!LoadXml()) return false;

	VECSHARES *pShares = NULL;
	if (strType == "files") pShares = &m_vecMyFilesShares;
	if (strType == "music") pShares = &m_vecMyMusicShares;
	if (strType == "videos") pShares = &m_vecMyVideoShares;
	if (strType == "pictures") pShares = &m_vecMyPictureShares;

	if (!pShares) return false;

	CShare share;
	share.strName=strName;
	share.strPath=strPath;
	share.m_iBufferSize=0;
	share.m_iDepthSize=1;
	CStdString strPath1=share.strPath;
	strPath1.ToUpper();
	if (strPath1.Left(4)=="UDF:")
	{
		share.m_iDriveType=SHARE_TYPE_VIRTUAL_DVD;
		share.strPath="D:\\";
	}
	else if (strPath1.Left(11) =="SOUNDTRACK:")
		share.m_iDriveType=SHARE_TYPE_LOCAL;
	else if (CUtil::IsISO9660(share.strPath))
		share.m_iDriveType=SHARE_TYPE_VIRTUAL_DVD;
	else if (CUtil::IsDVD(share.strPath))
		share.m_iDriveType = SHARE_TYPE_DVD;
	else if (CUtil::IsRemote(share.strPath))
		share.m_iDriveType = SHARE_TYPE_REMOTE;
	else if (CUtil::IsHD(share.strPath))
		share.m_iDriveType = SHARE_TYPE_LOCAL;
	else
		share.m_iDriveType = SHARE_TYPE_UNKNOWN;

	pShares->push_back(share);

	// Add to the xml file
	TiXmlElement *pRootElement = xbmcXml.RootElement();
	TiXmlNode *pNode = NULL;

	pNode = pRootElement->FirstChild(strType);

	// create a new Element
	TiXmlText xmlName(strName);
	TiXmlText xmlPath(strPath);
	TiXmlElement eName("name");
	TiXmlElement ePath("path");
	eName.InsertEndChild(xmlName);
	ePath.InsertEndChild(xmlPath);

	TiXmlElement bookmark("bookmark");
	bookmark.InsertEndChild(eName);
	bookmark.InsertEndChild(ePath);

	if (pNode)
	{
		pNode->ToElement()->InsertEndChild(bookmark);
	}
	return xbmcXml.SaveFile();
}

bool CSettings::SetBookmarkLocks(const CStdString &strType, bool bEngageLocks)
{
 	if (!LoadXml()) return false;

	VECSHARES *pShares = NULL;
	if (strType == "files") pShares = &g_settings.m_vecMyFilesShares;
	if (strType == "music") pShares = &g_settings.m_vecMyMusicShares;
	if (strType == "videos") pShares = &g_settings.m_vecMyVideoShares;
	if (strType == "pictures") pShares = &g_settings.m_vecMyPictureShares;
	if (strType == "myprograms") pShares = &g_settings.m_vecMyProgramsBookmarks;

	if (!pShares) return false;

    for (IVECSHARES it = pShares->begin(); it != pShares->end(); it++)
    {
        if ((*it).m_iLockMode < 0 && bEngageLocks)
            (*it).m_iLockMode = (*it).m_iLockMode * -1;
        else if ((*it).m_iLockMode > 0 && !bEngageLocks)
            (*it).m_iLockMode = (*it).m_iLockMode * -1;
    }
    // Return bookmark of
    TiXmlElement *pRootElement = xbmcXml.RootElement();
    TiXmlNode *pNode = NULL;
    TiXmlNode *pIt = NULL;

    pNode = pRootElement->FirstChild(strType);
    bool bXmlChanged = false;

    // if valid bookmark, find child at pos (id)
    if (pNode)
    {
        pIt = pNode->FirstChild("bookmark");
        while (pIt)
        {
            TiXmlNode *pChild = pIt->FirstChild("lockmode");
            if (pChild)
            {
                CStdString strLockModeValue = pChild->FirstChild()->Value();
                if (strLockModeValue.Mid(0, 1) == "-" && bEngageLocks)
                {
                    strLockModeValue = strLockModeValue.Mid(1, strlen(strLockModeValue) - 1);
                    pIt->FirstChild("lockmode")->FirstChild()->SetValue(strLockModeValue);
                    bXmlChanged = true;
                }
                else if (strLockModeValue.Mid(0, 1) != "-" && !bEngageLocks)
                {
                    strLockModeValue = "-" + strLockModeValue;
                    pIt->FirstChild("lockmode")->FirstChild()->SetValue(strLockModeValue);
                    bXmlChanged = true;
                }
            }
            pIt = pIt->NextSibling("bookmark");
        }
    }
    if (bXmlChanged)
        return xbmcXml.SaveFile();
    else
        return true;
}

void CSettings::LoadHomeButtons(TiXmlElement* pRootElement)
{
	TiXmlElement *pElement = pRootElement->FirstChildElement("homebuttons");
	if (pElement)
	{
		TiXmlElement *pChild = pElement->FirstChildElement("button");
		while (pChild)
		{
			char temp[1024];
			int iIcon;
			GetString(pChild, "execute", temp, "");
			GetInteger(pChild, "icon", iIcon, ICON_TYPE_NONE, ICON_TYPE_NONE, ICON_TYPE_NONE+19);
			char temp2[1024];
			GetString(pChild, "label", temp2, "");
			CButtonScrollerSettings::CButton *pButton = NULL;
			if ((temp2[0]>='A')&&(temp2[0]<='z'))
			{
				WCHAR wszLabel[1024];
				swprintf(wszLabel,L"%S",temp2);
				pButton = new CButtonScrollerSettings::CButton(wszLabel, temp, iIcon);
			}
			else
			{
				DWORD dwLabelID=atol(temp2);
				pButton = new CButtonScrollerSettings::CButton(dwLabelID, temp, iIcon);
			}
			if (pButton->m_strExecute.size()>0)
				g_settings.m_buttonSettings.m_vecButtons.push_back(pButton);
			pChild=pChild->NextSiblingElement();
		}
		GetInteger(pElement, "default", g_settings.m_buttonSettings.m_iDefaultButton, 0, 0, (int)g_settings.m_buttonSettings.m_vecButtons.size());
	}
	if (g_settings.m_buttonSettings.m_vecButtons.size() == 0)
	{	// fill in the defaults
		CButtonScrollerSettings::CButton *button1 = new CButtonScrollerSettings::CButton(0, "XBMC.ActivateWindow(1)", ICON_TYPE_PROGRAMS);
		CButtonScrollerSettings::CButton *button2 = new CButtonScrollerSettings::CButton(7, "XBMC.ActivateWindow(3)", ICON_TYPE_FILES);
		CButtonScrollerSettings::CButton *button3 = new CButtonScrollerSettings::CButton(1, "XBMC.ActivateWindow(2)", ICON_TYPE_PICTURES);
		CButtonScrollerSettings::CButton *button4 = new CButtonScrollerSettings::CButton(2, "XBMC.ActivateWindow(501)", ICON_TYPE_MUSIC);
		CButtonScrollerSettings::CButton *button5 = new CButtonScrollerSettings::CButton(3, "XBMC.ActivateWindow(6)", ICON_TYPE_VIDEOS);
		CButtonScrollerSettings::CButton *button6 = new CButtonScrollerSettings::CButton(8, "XBMC.ActivateWindow(2600)", ICON_TYPE_WEATHER);
		CButtonScrollerSettings::CButton *button7 = new CButtonScrollerSettings::CButton(5, "XBMC.ActivateWindow(4)", ICON_TYPE_SETTINGS);
		g_settings.m_buttonSettings.m_vecButtons.push_back(button1);
		g_settings.m_buttonSettings.m_vecButtons.push_back(button2);
		g_settings.m_buttonSettings.m_vecButtons.push_back(button3);
		g_settings.m_buttonSettings.m_vecButtons.push_back(button4);
		g_settings.m_buttonSettings.m_vecButtons.push_back(button5);
		g_settings.m_buttonSettings.m_vecButtons.push_back(button6);
		g_settings.m_buttonSettings.m_vecButtons.push_back(button7);
		g_settings.m_buttonSettings.m_iDefaultButton = 0;
	}
}

bool CSettings::SaveHomeButtons()
{
	// Load the xml file...
	if (!LoadXml()) return false;

	// Find the <homebuttons> tag.
	TiXmlElement *pRootElement = xbmcXml.RootElement();
	TiXmlNode *pNode = pRootElement->FirstChild("homebuttons");
	TiXmlNode *pIt = NULL;

	// if we've found the <homebutton> tag
	if (pNode)
	{	// delete it
		pRootElement->RemoveChild(pNode);
	}
	
	// readd it
	TiXmlElement xmlHomeButtons("homebuttons");
	pRootElement->InsertEndChild(xmlHomeButtons);
	pNode = pRootElement->FirstChild("homebuttons");
	if (!pNode) return false;
	// now add them all from our settings information
	for (unsigned int i=0; i<g_settings.m_buttonSettings.m_vecButtons.size(); i++)
	{
		// create a new <button> entry
		CStdString strLabel;
		CStdString strDescription;
		if (g_settings.m_buttonSettings.m_vecButtons[i]->m_dwLabel == -1)
		{
			strLabel = g_settings.m_buttonSettings.m_vecButtons[i]->m_strLabel;
			strDescription = strLabel;
		}
		else
		{
			strLabel.Format("%i", g_settings.m_buttonSettings.m_vecButtons[i]->m_dwLabel);
			strDescription = g_localizeStrings.Get(g_settings.m_buttonSettings.m_vecButtons[i]->m_dwLabel);
		}
		TiXmlText xmlDescription(strDescription);
		TiXmlText xmlLabel(strLabel);
		TiXmlText xmlExecute(g_settings.m_buttonSettings.m_vecButtons[i]->m_strExecute);
		CStdString strIcon;
		strIcon.Format("%i", g_settings.m_buttonSettings.m_vecButtons[i]->m_iIcon);
		TiXmlText xmlIcon(strIcon);
		TiXmlElement eDescription("description");
		TiXmlElement eLabel("label");
		TiXmlElement eExecute("execute");
		TiXmlElement eIcon("icon");

		eDescription.InsertEndChild(xmlDescription);
		eLabel.InsertEndChild(xmlLabel);
		eExecute.InsertEndChild(xmlExecute);
		eIcon.InsertEndChild(xmlIcon);

		TiXmlElement button("button");
		button.InsertEndChild(eDescription);
		button.InsertEndChild(eLabel);
		button.InsertEndChild(eExecute);
		button.InsertEndChild(eIcon);

		pNode->InsertEndChild(button);
	}
	// now save the default button...
	pIt = pNode->FirstChild("default");
	if (pIt)
	{	// delete it
		pNode->RemoveChild(pIt);
	}
	CStdString strDefault;
	strDefault.Format("%i", g_settings.m_buttonSettings.m_iDefaultButton);
	TiXmlText xmlDefault(strDefault);
	TiXmlElement eDefault("default");
	eDefault.InsertEndChild(xmlDefault);
	pNode->InsertEndChild(eDefault);

	return xbmcXml.SaveFile();
}

bool CSettings::LoadFolderViews(const CStdString &strFolderXML, VECFOLDERVIEWS &vecFolders)
{	// load xml file...
	CStdString strXMLFile = "T:\\";
	strXMLFile += strFolderXML;

	TiXmlDocument xmlDoc;
	if ( !xmlDoc.LoadFile( strXMLFile.c_str() ) )
	{
		CLog::Log(LOGERROR, "LoadFolderViews - Unable to load XML file %s", strFolderXML.c_str());
		return false;
	}

	TiXmlElement* pRootElement =xmlDoc.RootElement();
	CStdString strValue=pRootElement->Value();
	if ( strValue != "folderviews")
	{
		g_LoadErrorStr.Format("%s Doesn't contain <folderviews>",strXMLFile.c_str());
		return false;
	}

	// cleanup vecFolders if necessary...
	if (vecFolders.size())
	{
		for (unsigned int i=0; i<vecFolders.size(); i++)
			delete vecFolders[i];
		vecFolders.clear();
	}
	// parse the view mode for each folder
	const TiXmlNode *pChild = pRootElement->FirstChild("folder");
	while (pChild)
	{
		CStdString strPath;
		int iView = VIEW_AS_LIST;
		int iSort = 0;
		bool bSortUp = true;
		TiXmlNode *pPath = pChild->FirstChild("path");
		if (pPath && pPath->FirstChild())
			strPath = pPath->FirstChild()->Value();
		TiXmlNode *pView = pChild->FirstChild("view");
		if (pView && pView->FirstChild())
			iView = atoi(pView->FirstChild()->Value());
		TiXmlNode *pSort = pChild->FirstChild("sort");
		if (pSort && pSort->FirstChild())
			iSort = atoi(pSort->FirstChild()->Value());
		TiXmlNode *pDirection = pChild->FirstChild("direction");
		if (pDirection && pDirection->FirstChild())
			bSortUp = pDirection->FirstChild()->Value() == "up";
		// fill in element
		if (!strPath.IsEmpty())
		{
			CFolderView *pFolderView = new CFolderView(strPath, iView, iSort, bSortUp);
			if (pFolderView)
				vecFolders.push_back(pFolderView);
		}
		// run to next <folder> element
		pChild = pChild->NextSibling("folder");
	}
	return true;
}

bool CSettings::SaveFolderViews(const CStdString &strFolderXML, VECFOLDERVIEWS &vecFolders)
{
	TiXmlDocument xmlDoc;
	TiXmlElement xmlRootElement("folderviews");
	TiXmlNode *pRoot = xmlDoc.InsertEndChild(xmlRootElement);
	if (!pRoot) return false;
	// write our folders one by one
	CStdString strView, strSort, strSortUp;
	for (unsigned int i=0; i<vecFolders.size(); i++)
	{
		CFolderView *pFolderView = vecFolders[i];
		strView.Format("%i", pFolderView->m_iView);
		strSort.Format("%i", pFolderView->m_iSort);
		strSortUp = pFolderView->m_bSortAscending ? "up" : "down";

		TiXmlText xmlPath(pFolderView->m_strPath.IsEmpty() ? "ROOT" : pFolderView->m_strPath);
			
		TiXmlText xmlView(strView);
		TiXmlText xmlSort(strSort);
		TiXmlText xmlSortUp(strSortUp);

		TiXmlElement ePath("path");
		TiXmlElement eView("view");
		TiXmlElement eSort("sort");
		TiXmlElement eSortUp("direction");

		ePath.InsertEndChild(xmlPath);
		eView.InsertEndChild(xmlView);
		eSort.InsertEndChild(xmlSort);
		eSortUp.InsertEndChild(xmlSortUp);

		TiXmlElement folderNode("folder");
		folderNode.InsertEndChild(ePath);
		folderNode.InsertEndChild(eView);
		folderNode.InsertEndChild(eSort);
		folderNode.InsertEndChild(eSortUp);

		pRoot->InsertEndChild(folderNode);
	}
	return xmlDoc.SaveFile(strFolderXML);
}