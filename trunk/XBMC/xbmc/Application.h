#pragma once

#include <xtl.h>
#include <xgraphics.h>
#include <stdio.h>
#include <vector>
#include <memory>
#include <stdlib.h>
#include "xbapplicationex.h"
#include "applicationmessenger.h"
#include "GUIWindowManager.h"
#include "guiwindow.h"
#include "GUIMessage.h"
#include "GUIButtonControl.h"
#include "GUIImage.h"
#include "GUIFontManager.h"
#include "SkinInfo.h"
#include "key.h"
#include "utils/imdb.h"

#include "GUIWindowHome.h"
#include "GUIWindowPrograms.h"
#include "GUIWindowSettingsPrograms.h"
#include "GUIWindowPictures.h"
#include "GUIWindowFileManager.h"
#include "GUIWindowVideoFiles.h"
#include "GUIWindowVideoGenre.h"
#include "GUIWindowVideoActors.h"
#include "GUIWindowVideoYear.h"
#include "GUIWindowVideoTitle.h"
#include "GUIWindowSettings.h"
#include "GUIDialogInvite.h"
#include "GUIDialogKeyboard.h"
#include "GUIDialogYesNo.h"
#include "GUIDialogOK.h"
#include "GUIDialogProgress.h"
#include "GUIDialogSelect.h"
#include "GUIDialogFileStacking.h"
#include "GUIDialogVolumeBar.h"
#include "GUIDialogSubMenu.h"
#include "GUIDialogContextMenu.h"
#include "GUIWindowSystemInfo.h"
#include "GUIWindowSettingsLCD.h"
#include "GUIWindowSettingsGeneral.h"
//#include "GUIWindowSettingsScreen.h"
#include "GUIWindowSettingsMyVideo.h"
#include "GUIWindowSettingsUICalibration.h"
#include "GUIWindowSettingsScreenCalibration.h"
#include "GUIWindowSettingsSubtitles.h"
#include "GUIWindowScreensaver.h"		// CB: Matrix Screensaver
#include "GUIWindowOSD.h"
#include "GUIWindowMusicInfo.h"
#include "GUIWindowVideoInfo.h"
#include "GUIWindowScriptsInfo.h"
#include "GUIWindowMusicOverlay.h"
#include "GUIWindowFullScreen.h"
#include "GUIWindowVideoOverlay.h"
#include "GUIWindowVideoPlaylist.h"
#include "GUIWindowSettingsSlideShow.h"
#include "GUIWindowSettingsScreensaver.h"
#include "GUIWindowSettingsCDRipper.h"
#include "guiwindowsettingsautorun.h"
#include "guiwindowsettingsfilter.h"
//#include "guiwindowsettingsmusic.h"
#include "GUIWindowSettingsMyMusic.h"
#include "GUIWindowScripts.h"
#include "GUIWindowVisualisation.h"
#include "GUIWindowSlideshow.h"
#include "GUIWindowMusicPlaylist.h"
#include "GUIWindowMusicSongs.h"
#include "GUIWindowMusicAlbum.h"
#include "GUIWindowMusicArtists.h"
#include "GUIWindowMusicGenres.h"
#include "GUIWindowMusicTop100.h"
#include "GUIWindowBuddies.h"		//BUDDIES
#include "GUIWindowWeather.h"		//WEATHER
#include "GUIWindowSettingsWeather.h"	//WEATHER SETTINGS
#include "GUIWindowSettingsNetwork.h"
#include "GUIWindowSettingsNetworkIP.h"
#include "GUIWindowSettingsNetworkProxy.h"
#include "GUIWindowSettingsNetworkKai.h"
#include "GUIWindowSettingsNetworkFTP.h"
#include "GUIWindowSettingsNetworkWeb.h"
#include "GUIWindowSettingsNetworkTime.h"
#include "GUIWindowSettingsCache.h"
#include "GUIWindowPointer.h"		// Mouse pointer
#include "LocalizeStrings.h"
#include "utils/sntp.h"
#include "utils/delaycontroller.h"
#include "keyboard/virtualkeyboard.h"
#include "lib/libPython/XBPython.h"
#include "lib/libGoAhead/webserver.h"
#include "lib/libfilezilla/xbfilezilla.h"
#include "cores/IPlayer.h"
#include "DetectDVDType.h"
#include "Autorun.h"
#include "IMsgTargetCallback.h"
#include "ButtonTranslator.h"
#include "musicInfoTag.h"
#include "GUIWindowSettingsSkinLanguage.h"
#include "GUIWindowSettingsUserInterface.h"
#include "GUIWindowSettingsAudio.h"
#include "GUIWindowSettingsSystem.h"
#include "GUIWindowSettingsProfile.h"

using namespace std;
using namespace MEDIA_DETECT;
using namespace MUSIC_INFO;

class CApplication : public CXBApplicationEx, public IPlayerCallback, public IMsgTargetCallback
{
public:
  CApplication(void);
  virtual ~CApplication(void);
  virtual HRESULT 			Initialize();
  virtual void					FrameMove();
  virtual void					Render();
	virtual HRESULT 			Create();

  void StartServices();
  void StopServices();
  void StartWebServer();
  void StopWebServer();
  void StartFtpServer();
  void StopFtpServer();
  void StartTimeServer();
  void StopTimeServer();

	void					Stop();
	void					RestartApp();
	void					LoadSkin(const CStdString& strSkin);
	bool					LoadUserWindows(const CStdString& strSkinPath);
	void					DelayLoadSkin();
	void					CancelDelayLoadSkin();
	const CStdString&		CurrentFile();
	const CStdString&		GetCurrentPlayer();
	virtual bool			OnMessage(CGUIMessage& message);
	virtual	void			OnPlayBackEnded();
	virtual	void			OnPlayBackStarted();
	bool					PlayFile(const CFileItem& item,bool bRestart=false);
	void					StopPlaying();
	void					Restart(bool bSamePosition=true);
	bool					IsPlaying() const ;
	bool					IsPlayingAudio() const ;
	bool					IsPlayingVideo() const ;
	void					OnKey(CKey& key);
	void					RenderFullScreen();
	bool					NeedRenderFullScreen();
  bool          MustBlockHDSpinDown(bool bCheckThisForNormalSpinDown = true);
  void          CheckNetworkHDSpinDown(bool playbackStarted = false);
	void					CheckHDSpindown();
	void				  CheckScreenSaver();		// CB: SCREENSAVER PATCH
	void				  CheckShutdown();
	void					SetCurrentSong(const CMusicInfoTag& tag);
	void					SetCurrentMovie(const CIMDBMovie& tag);
	CMusicInfoTag* GetCurrentSong();
	CIMDBMovie*		GetCurrentMovie();

	void					ResetAllControls();
	virtual void			Process();
	void					ResetScreenSaver();
	int						GetVolume() const;
	void					SetVolume(int iPercent);
	int						GetPlaySpeed() const;
	void					SetPlaySpeed(int iSpeed);
	bool									IsButtonDown(DWORD code);
	bool				ResetScreenSaverWindow();

	CGUIWindowHome									m_guiHome;
  CGUIWindowPrograms							m_guiPrograms;
  CGUIWindowSettingsPrograms      m_guiSettingsPrograms;
  CGUIWindowSettingsProfile       m_guiSettingsProfile;
  CGUIWindowSettingsSystem        m_guiSettingsSytem;
	CGUIWindowPictures							m_guiPictures;
	CGUIDialogInvite								m_guiDialogInvite;
	CGUIDialogKeyboard							m_guiDialogKeyboard;
	CGUIDialogYesNo									m_guiDialogYesNo;
	CGUIDialogProgress							m_guiDialogProgress;
	CGUIDialogOK										m_guiDialogOK;
	CGUIDialogVolumeBar							m_guiDialogVolumeBar;
	CGUIDialogSubMenu								m_guiDialogSubMenu;
	CGUIDialogContextMenu						m_guiDialogContextMenu;
	CGUIWindowFileManager						m_guiFileManager;
	CGUIWindowVideoFiles						m_guiMyVideo;
	CGUIWindowSettings							m_guiSettings;
	CGUIWindowSystemInfo						m_guiSystemInfo;
	CGUIWindowSettingsGeneral				m_guiSettingsGeneral;
	CGUIWindowMusicInfo							m_guiMusicInfo;
	CGUIWindowVideoInfo							m_guiVideoInfo;
	CGUIWindowScriptsInfo						m_guiScriptsInfo;
	//CGUIWindowSettingsScreen			m_guiSettingsScreen;		// now m_guiSettingsMyVideo
	CGUIWindowSettingsUICalibration	m_guiSettingsUICalibration;
	CGUIWindowSettingsScreenCalibration m_guiSettingsScreenCalibration;
	CGUIWindowSettingsSlideShow			m_guiSettingsSlideShow;
	CGUIWindowSettingsScreensaver		m_guiSettingsScreensaver;
	CGUIWindowSettingsCDRipper			m_guiSettingsCDRipper;
	//CGUIWindowSettingsOSD					m_guiSettingsOSD;
  CGUIWindowSettingsAutoRun       m_guiSettingsAutoRun;
	CGUIWindowScripts								m_guiScripts;
	CGUIWindowSettingsFilter				m_guiSettingsFilter;
	CGUIDialogSelect								m_guiDialogSelect;
	CGUIDialogFileStacking					m_guiDialogFileStacking;
	CGUIWindowMusicOverlay					m_guiMusicOverlay;
	CGUIWindowFullScreen						m_guiWindowFullScreen;
	CGUIWindowVideoOverlay					m_guiWindowVideoOverlay;
	CGUIWindowVisualisation					m_guiWindowVisualisation;
	//CGUIWindowSettingsMusic				m_guiSettingsMusic;	// now m_guiSettingsMyMusic
	CGUIWindowSlideShow							m_guiWindowSlideshow;
  CGUIWindowMusicPlayList					m_guiMyMusicPlayList;
  CGUIWindowVideoPlaylist         m_guiMyVideoPlayList;
	CGUIWindowMusicSongs						m_guiMyMusicSongs;
  CGUIWindowMusicAlbum						m_guiMyMusicAlbum;
	CGUIWindowMusicArtists					m_guiMyMusicArtists;
	CGUIWindowMusicGenres						m_guiMyMusicGenres;
	CGUIWindowMusicTop100						m_guiMyMusicTop100;
	CGUIWindowScreensaver						m_guiWindowScreensaver;
  CGUIWindowSettingsLCD           m_guiSettingsLCD;
  CGUIWindowSettingsSubtitles     m_guiSettingsSubtitles;
  CGUIWindowVideoGenre            m_guiVideoGenre;
  CGUIWindowVideoActors           m_guiVideoActors;
	CGUIWindowVideoYear             m_guiVideoYear;
	CGUIWindowVideoTitle            m_guiVideoTitle;
	CGUIWindowWeather								m_guiMyWeather;	//WEATHER
	CGUIWindowBuddies						    m_guiMyBuddies;	//BUDDIES
	CGUIWindowSettingsWeather				m_guiSettingsWeather; //WEATHER SETTINGS
	CGUIWindowSettingsNetwork				m_guiSettingsNetwork;
	CGUIWindowSettingsNetworkIP				m_guiSettingsNetworkIP;
	CGUIWindowSettingsNetworkProxy			m_guiSettingsNetworkProxy;
	CGUIWindowSettingsNetworkKai			m_guiSettingsNetworkKai;
	CGUIWindowSettingsNetworkWeb			m_guiSettingsNetworkWeb;
	CGUIWindowSettingsNetworkFTP			m_guiSettingsNetworkFTP;
	CGUIWindowSettingsNetworkTime			m_guiSettingsNetworkTime;
	CGUIWindowOSD							m_guiWindowOSD;
	CGUIWindowSettingsCache					m_guiSettingsCache;
	CGUIWindowSettingsSkinLanguage	m_guiSettingsSkinLanguage;
	CGUIWindowSettingsUserInterface	m_guiSettingsUserInterface;
	CGUIWindowSettingsAudio					m_guiSettingsAudio;
	CGUIWindowSettingsMyVideo				m_guiSettingsMyVideo;
	CGUIWindowSettingsMyMusic				m_guiSettingsMyMusic;
	CGUIWindowPointer						m_guiPointer;

	CXBVirtualKeyboard   						m_keyboard;
	CSNTPClient											m_sntpClient;
	CDetectDVDMedia									m_DetectDVDType;
	CAutorun												m_Autorun;
	CDelayController								m_ctrDpad;
	CDelayController								m_ctrIR;
	CWebServer*											m_pWebServer;
	CXBFileZilla*										m_pFileZilla;
	IPlayer*												m_pPlayer;

	bool		m_bSpinDown;
  bool    m_bNetworkSpinDown;
	DWORD		m_dwSpinDownTime;
	DWORD		m_dwIdleTime;
	bool		m_bInactive;	// CB: SCREENSAVER PATCH
	bool		m_bScreenSave;	// CB: SCREENSAVER PATCH
	DWORD		m_dwSaverTick;	// CB: SCREENSAVER PATCH
	DWORD		m_dwSkinTime;

	char		m_CurrDAAPHost[64];
	void		*m_DAAPPtr;
	int			m_DAAPDBID;
	void		*m_DAAPSong;
	UINT64		m_DAAPSongSize;
	void		*m_DAAPArtistPtr;

protected:
  CStdString							m_strCurrentPlayer;
  void                    UpdateLCD();
  bool					  SwitchToFullScreen();
	void										FatalErrorHandler(bool InitD3D, bool MapDrives, bool InitNetwork);
  bool                    m_bXboxMediacenterLoaded,m_bSettingsLoaded;
  CIMDBMovie              m_tagCurrentMovie;
  int                     m_iPlaySpeed;
  bool                    m_bAllSettingsLoaded;
  CFileItem              m_itemCurrentFile;
  D3DGAMMARAMP m_OldRamp;			// CB: SCREENSAVER PATCH
};

extern CApplication g_application;
extern CStdString g_LoadErrorStr;
