#pragma once
#include "xbapplicationex.h"

#include "MusicInfoTag.h"
#include "IMsgTargetCallback.h"
#include "Key.h"
#include "FileItem.h"

#include "GUIDialogSeekBar.h"
#include "GUIDialogKaiToast.h"
#include "GUIDialogVolumeBar.h"
#include "GUIDialogMuteBug.h"
#include "GUIWindowPointer.h"   // Mouse pointer

#include "utils/delaycontroller.h"
#include "cores/IPlayer.h"
#include "cores/playercorefactory.h"
#include "PlaylistPlayer.h"
#include "DetectDVDType.h"
#include "Autorun.h"
#include "utils/Splash.h"
#include "utils/IMDB.h"
#include "utils/stopwatch.h"

using namespace MEDIA_DETECT;
using namespace MUSIC_INFO;

class CWebServer;
class CXBFileZilla;
class CSNTPClient;
class CCdgParser;

class CApplication : public CXBApplicationEx, public IPlayerCallback, public IMsgTargetCallback
{
public:
  CApplication(void);
  virtual ~CApplication(void);
  virtual HRESULT Initialize();
  virtual void FrameMove();
  virtual void Render();
#ifndef HAS_XBOX_D3D
  virtual void RenderNoPresent();
#endif
  virtual HRESULT Create(HWND hWnd);
  void StartServices();
  void StopServices();
  void StartKai();
  void StopKai();
  void StartWebServer();
  void StopWebServer();
  void StartFtpServer();
  void StopFtpServer();
  void StartTimeServer();
  void StopTimeServer();
  void StartUPnP();
  void StopUPnP();
  void StartLEDControl(bool switchoff = false);
  void DimLCDOnPlayback(bool dim);
  void PrintXBEToLCD(const char* xbePath);
  void CheckDate();		//GeminiServer CheckDate
  DWORD GetThreadId() const { return m_threadID; };
  void Stop();
  void RestartApp();
  void LoadSkin(const CStdString& strSkin);
  void UnloadSkin();
  bool LoadUserWindows(const CStdString& strSkinPath);
  void DelayLoadSkin();
  void CancelDelayLoadSkin();
  void ReloadSkin();
  const CStdString& CurrentFile();
  CFileItem& CurrentFileItem();
  virtual bool OnMessage(CGUIMessage& message);
  const EPLAYERCORES GetCurrentPlayer();
  virtual void OnPlayBackEnded();
  virtual void OnPlayBackStarted();
  virtual void OnPlayBackStopped();
  virtual void OnQueueNextItem();
  bool PlayMedia(const CFileItem& item, int iPlaylist = PLAYLIST_MUSIC);
  bool ProcessAndStartPlaylist(const CStdString& strPlayList, CPlayList& playlist, int iPlaylist);
  bool PlayFile(const CFileItem& item, bool bRestart = false);
  void StopPlaying();
  void Restart(bool bSamePosition = true);
  void DelayedPlayerRestart();
  void CheckDelayedPlayerRestart();
  void RenderFullScreen();
  bool NeedRenderFullScreen();
  bool IsPlaying() const ;
  bool IsPaused() const;
  bool IsPlayingAudio() const ;
  bool IsPlayingVideo() const ;
  bool OnKey(CKey& key);
  bool OnAction(const CAction &action);
  void RenderMemoryStatus();
  bool MustBlockHDSpinDown(bool bCheckThisForNormalSpinDown = true);
  void CheckNetworkHDSpinDown(bool playbackStarted = false);
  void CheckHDSpindown();
  void CheckShutdown();
  void CheckScreenSaver();   // CB: SCREENSAVER PATCH
  void CheckPlayingProgress();
  void CheckAudioScrobblerStatus();
  void ActivateScreenSaver(bool forceType = false);

  virtual void Process();
  void ProcessSlow();
  void ResetScreenSaver();
  int GetVolume() const;
  void SetVolume(int iPercent);
  void Mute(void);
  int GetPlaySpeed() const;
  void SetPlaySpeed(int iSpeed);
  bool IsButtonDown(DWORD code);
  bool AnyButtonDown();
  bool ResetScreenSaverWindow();
  void SetKaiNotification(const CStdString& aCaption, const CStdString& aDescription, CGUIImage* aIcon=NULL);
  double GetTotalTime() const;
  double GetTime() const;
  float GetPercentage() const;
  void SeekPercentage(float percent);
  void SeekTime( double dTime = 0.0 );
  void ResetPlayTime();

  void SaveMusicScanSettings();
  void RestoreMusicScanSettings();
  void CheckMusicPlaylist();

  CGUIDialogVolumeBar m_guiDialogVolumeBar;
  CGUIDialogSeekBar m_guiDialogSeekBar;
  CGUIDialogKaiToast m_guiDialogKaiToast;
  CGUIDialogMuteBug m_guiDialogMuteBug;
  CGUIWindowPointer m_guiPointer;

  CAutorun m_Autorun;
  CDetectDVDMedia m_DetectDVDType;
  CDelayController m_ctrDpad;
  CSNTPClient *m_psntpClient;
  CWebServer* m_pWebServer;
  CXBFileZilla* m_pFileZilla;
  IPlayer* m_pPlayer;

  bool m_bSpinDown;
  bool m_bNetworkSpinDown;
  DWORD m_dwSpinDownTime;

  inline bool IsInScreenSaver() { return m_bScreenSave; };
  int m_iScreenSaveLock; // spiff: are we checking for a lock? if so, ignore the screensaver state, if -1 we have failed to input locks

  DWORD m_dwSkinTime;
  bool m_bIsPaused;

  CCdgParser* m_pCdgParser;

  EPLAYERCORES m_eForcedNextPlayer;
  CStdString m_strPlayListFile;
  
  int GlobalIdleTime();
  bool SetControllerRumble(FLOAT m_fLeftMotorSpeed, FLOAT m_fRightMotorSpeed,int iDuration);

protected:
  friend CApplicationMessenger;
  // screensaver
  bool m_bInactive;
  bool m_bScreenSave;
  CStdString m_screenSaverMode;
  DWORD m_dwSaverTick;
  D3DGAMMARAMP m_OldRamp;

  // timer information
  CStopWatch m_idleTimer;
  CStopWatch m_restartPlayerTimer;
  CStopWatch m_frameTime;
  CStopWatch m_navigationTimer;
  CStopWatch m_slowTimer;

  CFileItem m_itemCurrentFile;
  CFileItemList m_currentStack;
  CSplash* m_splash;
  DWORD m_threadID;       // application thread ID.  Used in applicationMessanger to know where we are firing a thread with delay from.
  EPLAYERCORES m_eCurrentPlayer;
  bool m_bXboxMediacenterLoaded;
  bool m_bSettingsLoaded;
  bool m_bAllSettingsLoaded;
  bool m_bInitializing;
  bool m_playCountUpdated;
  
  int m_iPlaySpeed;
  int m_currentStackPosition;
  int m_nextPlaylistItem;

  static LONG WINAPI UnhandledExceptionFilter(struct _EXCEPTION_POINTERS *ExceptionInfo);

  void SetHardwareVolume(long hardwareVolume);
  void UpdateLCD();
  void FatalErrorHandler(bool InitD3D, bool MapDrives, bool InitNetwork);
  void InitBasicD3D();
  
  
  bool PlayStack(const CFileItem& item, bool bRestart);
  bool SwitchToFullScreen();
  bool ProcessMouse();
  bool ProcessHTTPApiButtons();
  bool ProcessKeyboard();
  bool ProcessRemote(float frameTime);
  bool ProcessGamepad(float frameTime);
  void CheckForDebugButtonCombo();
  
  float NavigationIdleTime();
 
};

extern CApplication g_application;
extern CStdString g_LoadErrorStr;
