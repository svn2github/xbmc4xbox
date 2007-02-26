/*!
\file GUIInfoManager.h
\brief 
*/

#ifndef GUILIB_GUIInfoManager_H
#define GUILIB_GUIInfoManager_H
#pragma once

#include "../MusicInfoTag.h"
#include "../FileItem.h"
#include "../videodatabase.h"
#include "../StringUtils.h"
#include "../Temperature.h"
#include "../utils/criticalsection.h"

#define OPERATOR_NOT  3
#define OPERATOR_AND  2
#define OPERATOR_OR   1

#define PLAYER_HAS_MEDIA              1
#define PLAYER_HAS_AUDIO              2
#define PLAYER_HAS_VIDEO              3
#define PLAYER_PLAYING                4
#define PLAYER_PAUSED                 5 
#define PLAYER_REWINDING              6
#define PLAYER_REWINDING_2x           7
#define PLAYER_REWINDING_4x           8
#define PLAYER_REWINDING_8x           9
#define PLAYER_REWINDING_16x         10
#define PLAYER_REWINDING_32x         11
#define PLAYER_FORWARDING            12
#define PLAYER_FORWARDING_2x         13
#define PLAYER_FORWARDING_4x         14
#define PLAYER_FORWARDING_8x         15
#define PLAYER_FORWARDING_16x        16
#define PLAYER_FORWARDING_32x        17
#define PLAYER_CAN_RECORD            18
#define PLAYER_RECORDING             19
#define PLAYER_CACHING               20 
#define PLAYER_DISPLAY_AFTER_SEEK    21
#define PLAYER_PROGRESS              22
#define PLAYER_SEEKBAR               23
#define PLAYER_SEEKTIME              24
#define PLAYER_SEEKING               25
#define PLAYER_SHOWTIME              26
#define PLAYER_TIME                  27  
#define PLAYER_TIME_REMAINING        28
#define PLAYER_DURATION              29
#define PLAYER_SHOWCODEC             30
#define PLAYER_SHOWINFO              31
#define PLAYER_VOLUME                32
#define PLAYER_MUTED                 33
#define PLAYER_HASDURATION           34

#define WEATHER_CONDITIONS          100
#define WEATHER_TEMPERATURE         101
#define WEATHER_LOCATION            102
#define WEATHER_IS_FETCHED          103

#define SYSTEM_TIME                 110
#define SYSTEM_DATE                 111
#define SYSTEM_CPU_TEMPERATURE      112
#define SYSTEM_GPU_TEMPERATURE      113
#define SYSTEM_FAN_SPEED            114
#define SYSTEM_FREE_SPACE_C         115
/* 
#define SYSTEM_FREE_SPACE_D         116 //116 is reserved for space on D
*/
#define SYSTEM_FREE_SPACE_E         117
#define SYSTEM_FREE_SPACE_F         118
#define SYSTEM_FREE_SPACE_G         119
#define SYSTEM_BUILD_VERSION        120
#define SYSTEM_BUILD_DATE           121
#define SYSTEM_ETHERNET_LINK_ACTIVE 122
#define SYSTEM_FPS                  123
#define SYSTEM_KAI_CONNECTED        124
#define SYSTEM_ALWAYS_TRUE          125   // useful for <visible fade="10" start="hidden">true</visible>, to fade in a control
#define SYSTEM_ALWAYS_FALSE         126   // used for <visible fade="10">false</visible>, to fade out a control (ie not particularly useful!)
#define SYSTEM_MEDIA_DVD            127
#define SYSTEM_DVDREADY             128
#define SYSTEM_HAS_ALARM            129
#define SYSTEM_AUTODETECTION        130
#define SYSTEM_FREE_MEMORY          131
#define SYSTEM_SCREEN_MODE          132
#define SYSTEM_SCREEN_WIDTH         133
#define SYSTEM_SCREEN_HEIGHT        134
#define SYSTEM_CURRENT_WINDOW       135
#define SYSTEM_CURRENT_CONTROL      136
#define SYSTEM_XBOX_NICKNAME        137
#define SYSTEM_DVD_LABEL            138
#define SYSTEM_HAS_DRIVE_F          139
#define SYSTEM_HASLOCKS             140
#define SYSTEM_ISMASTER             141
#define SYSTEM_TRAYOPEN	            142
#define SYSTEM_KAI_ENABLED          143
#define SYSTEM_ALARM_POS            144
#define SYSTEM_LOGGEDON             145
#define SYSTEM_PROFILENAME          146
#define SYSTEM_PROFILETHUMB         147
#define SYSTEM_HAS_LOGINSCREEN      148
#define SYSTEM_HAS_DRIVE_G          149

// reserved for systeminfo stuff
#define SYSTEM_HDD_SMART            150
#define SYSTEM_HDD_TEMPERATURE      151
#define SYSTEM_HDD_MODEL            152
#define SYSTEM_HDD_SERIAL           153
#define SYSTEM_HDD_FIRMWARE         154
#define SYSTEM_HDD_PASSWORD         156
#define SYSTEM_HDD_LOCKSTATE        157
#define SYSTEM_HDD_LOCKKEY          158

#define SYSTEM_DVD_MODEL            650
#define SYSTEM_DVD_FIRMWARE         651
#define SYSTEM_MPLAYER_VERSION      652
#define SYSTEM_KERNEL_VERSION       653
#define SYSTEM_UPTIME               654
#define SYSTEM_TOTALUPTIME          655
#define SYSTEM_CPUFREQUENCY         656
#define SYSTEM_XBOX_VERSION         657
#define SYSTEM_AV_CABLE_PACK_INFO   658
#define SYSTEM_SCREEN_RESOLUTION    659
#define SYSTEM_VIDEO_ENCODER_INFO   660

#define SYSTEM_INTERNET_STATE       159
//

#define LCD_PLAY_ICON               160
#define LCD_PROGRESS_BAR            161
#define LCD_CPU_TEMPERATURE         162
#define LCD_GPU_TEMPERATURE         163
#define LCD_FAN_SPEED               164
#define LCD_DATE                    166
#define LCD_FREE_SPACE_C            167
/*
#define LCD_FREE_SPACE_D            168 // 168 is reserved for space on D
*/
#define LCD_FREE_SPACE_E            169
#define LCD_FREE_SPACE_F            170
#define LCD_FREE_SPACE_G            171

#define NETWORK_IP_ADDRESS          190

#define MUSICPLAYER_TITLE           200
#define MUSICPLAYER_ALBUM           201
#define MUSICPLAYER_ARTIST          202
#define MUSICPLAYER_GENRE           203
#define MUSICPLAYER_YEAR            204
#define MUSICPLAYER_TIME            205
#define MUSICPLAYER_TIME_REMAINING  206
#define MUSICPLAYER_TIME_SPEED      207
#define MUSICPLAYER_TRACK_NUMBER    208
#define MUSICPLAYER_DURATION        209
#define MUSICPLAYER_COVER           210
#define MUSICPLAYER_BITRATE         211
#define MUSICPLAYER_PLAYLISTLEN     212
#define MUSICPLAYER_PLAYLISTPOS     213
#define MUSICPLAYER_CHANNELS        214
#define MUSICPLAYER_BITSPERSAMPLE   215
#define MUSICPLAYER_SAMPLERATE      216
#define MUSICPLAYER_CODEC           217
#define MUSICPLAYER_DISC_NUMBER     218

#define VIDEOPLAYER_TITLE           250
#define VIDEOPLAYER_GENRE           251
#define VIDEOPLAYER_DIRECTOR        252
#define VIDEOPLAYER_YEAR            253
#define VIDEOPLAYER_TIME            254
#define VIDEOPLAYER_TIME_REMAINING  255
#define VIDEOPLAYER_TIME_SPEED      256
#define VIDEOPLAYER_DURATION        257
#define VIDEOPLAYER_COVER           258
#define VIDEOPLAYER_USING_OVERLAYS  259
#define VIDEOPLAYER_ISFULLSCREEN    260
#define VIDEOPLAYER_HASMENU         261
#define VIDEOPLAYER_PLAYLISTLEN     262
#define VIDEOPLAYER_PLAYLISTPOS     263
#define VIDEOPLAYER_EVENT           264
#define VIDEOPLAYER_ORIGINALTITLE   265

#define AUDIOSCROBBLER_ENABLED      300
#define AUDIOSCROBBLER_CONN_STATE   301
#define AUDIOSCROBBLER_SUBMIT_INT   302
#define AUDIOSCROBBLER_FILES_CACHED 303
#define AUDIOSCROBBLER_SUBMIT_STATE 304

#define LISTITEM_START              310
#define LISTITEM_THUMB              310
#define LISTITEM_LABEL              311
#define LISTITEM_TITLE              312
#define LISTITEM_TRACKNUMBER        313
#define LISTITEM_ARTIST             314
#define LISTITEM_ALBUM              315
#define LISTITEM_YEAR               316
#define LISTITEM_GENRE              317
#define LISTITEM_ICON               318
#define LISTITEM_DIRECTOR           319
#define LISTITEM_OVERLAY            320
#define LISTITEM_LABEL2             321
#define LISTITEM_FILENAME           322
#define LISTITEM_DATE               323
#define LISTITEM_SIZE               324
#define LISTITEM_RATING             325
#define LISTITEM_PROGRAM_COUNT      326
#define LISTITEM_DURATION           327
#define LISTITEM_ISPLAYING          328
#define LISTITEM_ISSELECTED         329
#define LISTITEM_END                329

#define MUSICPM_ENABLED             350
#define MUSICPM_SONGSPLAYED         351
#define MUSICPM_MATCHINGSONGS       352
#define MUSICPM_MATCHINGSONGSPICKED 353
#define MUSICPM_MATCHINGSONGSLEFT   354
#define MUSICPM_RELAXEDSONGSPICKED  355
#define MUSICPM_RANDOMSONGSPICKED   356

#define CONTAINER_FOLDERTHUMB       360
#define CONTAINER_FOLDERPATH        361

#define PLAYLIST_LENGTH             390
#define PLAYLIST_POSITION           391
#define PLAYLIST_RANDOM             392
#define PLAYLIST_REPEAT             393
#define PLAYLIST_ISRANDOM           394
#define PLAYLIST_ISREPEAT           395
#define PLAYLIST_ISREPEATONE        396

#define VISUALISATION_LOCKED        400
#define VISUALISATION_PRESET        401
#define VISUALISATION_NAME          402
#define VISUALISATION_ENABLED       403

#define SKIN_HAS_THEME_START        500
#define SKIN_HAS_THEME_END          599 // allow for max 100 themes

#define SKIN_BOOL                   600
#define SKIN_STRING                 601
#define SKIN_HAS_MUSIC_OVERLAY      602
#define SKIN_HAS_VIDEO_OVERLAY      603

#define XLINK_KAI_USERNAME          701
#define SKIN_THEME                  702

#define WINDOW_IS_TOPMOST           9994
#define WINDOW_IS_VISIBLE           9995
#define WINDOW_NEXT                 9996
#define WINDOW_PREVIOUS             9997
#define WINDOW_IS_MEDIA             9998
#define WINDOW_ACTIVE_START         WINDOW_HOME
#define WINDOW_ACTIVE_END           WINDOW_PYTHON_END

#define SYSTEM_IDLE_TIME_START      20000
#define SYSTEM_IDLE_TIME_FINISH     21000 // 1000 seconds

#define CONTROL_IS_VISIBLE          29998
#define CONTROL_GROUP_HAS_FOCUS     29999
#define CONTROL_HAS_FOCUS           30000
#define BUTTON_SCROLLER_HAS_ICON    30001

#define VERSION_STRING "pre-2.1"

// the multiple information vector
#define MULTI_INFO_START              40000
#define MULTI_INFO_END                41000 // 1000 references is all we have for now
#define COMBINED_VALUES_START        100000

// forward
class CInfoPortion;

// structure to hold multiple integer data
// for storage referenced from a single integer
class GUIInfo
{
public:
  GUIInfo(int info, int data1 = 0, int data2 = 0)
  {
    m_info = info;
    m_data1 = data1;
    m_data2 = data2;
  }
  bool operator ==(const GUIInfo &right) const
  {
    return (m_info == right.m_info && m_data1 == right.m_data1 && m_data2 == right.m_data2);
  };
  int m_info;
  int m_data1;
  int m_data2;
};

/*!
 \ingroup strings
 \brief 
 */
class CGUIInfoManager
{
public:
  CGUIInfoManager(void);
  virtual ~CGUIInfoManager(void);

  void Clear();

  int TranslateString(const CStdString &strCondition);
  bool GetBool(int condition, DWORD dwContextWindow = 0);
  int GetInt(int info) const;
  string GetLabel(int info);

  CStdString GetImage(int info, int contextWindow);

  CStdString GetTime(bool bSeconds = false);
  CStdString GetDate(bool bNumbersOnly = false);

  void SetCurrentItem(CFileItem &item);
  void ResetCurrentItem();
  // Current song stuff
  /// \brief Retrieves tag info (if necessary) and fills in our current song path.
  void SetCurrentSong(CFileItem &item);
  void SetCurrentAlbumThumb(const CStdString thumbFileName);
  void SetCurrentSongTag(const CMusicInfoTag &tag) { m_currentFile.m_musicInfoTag = tag; m_currentFile.m_lStartOffset = 0;};
  const CMusicInfoTag &GetCurrentSongTag() const { return m_currentFile.m_musicInfoTag; };

  // Current movie stuff
  void SetCurrentMovie(CFileItem &item);
  const CIMDBMovie &GetCurrentMovie() const { return m_currentMovie; };

  CStdString GetMusicLabel(int item);
  CStdString GetVideoLabel(int item);
  CStdString GetPlaylistLabel(int item);
  CStdString GetMusicPartyModeLabel(int item);
  string GetFreeSpace(int drive, bool shortText = false);
  __int64 GetPlayTime();  // in ms
  CStdString GetCurrentPlayTime();
  int GetPlayTimeRemaining();
  int GetTotalPlayTime();
  CStdString GetCurrentPlayTimeRemaining();
  CStdString GetVersion();
  CStdString GetBuild();
  bool SystemHasInternet();
  CStdString SystemHasInternet_s();
  
  bool GetDisplayAfterSeek() const;
  void SetDisplayAfterSeek(DWORD TimeOut = 2500);
  void SetSeeking(bool seeking) { m_playerSeeking = seeking; };
  void SetShowTime(bool showtime) { m_playerShowTime = showtime; };
  void SetShowCodec(bool showcodec) { m_playerShowCodec = showcodec; };
  void SetShowInfo(bool showinfo) { m_playerShowInfo = showinfo; };
  void ToggleShowCodec() { m_playerShowCodec = !m_playerShowCodec; };
  void ToggleShowInfo() { m_playerShowInfo = !m_playerShowInfo; };

  bool m_performingSeek;

  string GetSystemHeatInfo(const CStdString &strInfo);
  CStdString GetATAInfo(int info);
  CStdString SystemInfoValues(int info);

  void UpdateFPS();
  inline float GetFPS() const { return m_fps; };

  void SetAutodetectedXbox(bool set) { m_hasAutoDetectedXbox = set; };
  bool HasAutodetectedXbox() const { return m_hasAutoDetectedXbox; };

  void SetNextWindow(int windowID) { m_nextWindowID = windowID; };
  void SetPreviousWindow(int windowID) { m_prevWindowID = windowID; };

  void ParseLabel(const CStdString &strLabel, vector<CInfoPortion> &multiInfo);
  CStdString GetMultiLabel(const vector<CInfoPortion> &multiInfo);

  void ResetCache();

  CStdString GetItemLabel(const CFileItem *item, int info);
  CStdString GetItemMultiLabel(const CFileItem *item, const vector<CInfoPortion> &multiInfo);
  CStdString GetItemImage(const CFileItem *item, int info);
  bool       GetItemBool(const CFileItem *item, int info, DWORD contextWindow);

protected:
  bool GetMultiInfoBool(const GUIInfo &info, DWORD dwContextWindow = 0) const;
  const CStdString &GetMultiInfoLabel(const GUIInfo &info) const;
  int TranslateSingleString(const CStdString &strCondition);

  // Conditional string parameters for testing are stored in a vector for later retrieval.
  // The offset into the string parameters array is returned.
  int ConditionalStringParameter(const CStdString &strParameter);
  int AddMultiInfo(const GUIInfo &info);

  CStdString GetAudioScrobblerLabel(int item);

  // Conditional string parameters are stored here
  CStdStringArray m_stringParameters;

  // Array of multiple information mapped to a single integer lookup
  vector<GUIInfo> m_multiInfo;

  bool GetTuxBoxEvents();
  CStdString m_currentMovieDuration;
  
  // Current playing stuff
  CFileItem m_currentFile;
  CIMDBMovie m_currentMovie;
  CStdString m_currentMovieThumb;
  unsigned int m_lastMusicBitrateTime;
  unsigned int m_MusicBitrate;
  int i_SmartRequest;

  // fan stuff
  DWORD m_lastSysHeatInfoTime;
  int m_fanSpeed;
  CTemperature m_gpuTemp;
  CTemperature m_cpuTemp;
  
  // hdd stuff
  DWORD m_lastHddInfoTime;
  BYTE b_HddTemp;
  CStdString strHDDModel, strHDDSerial,strHDDFirmware,strHDDpw,strHDDLockState;
  bool m_hddRequest;
  CStdString strDVDModel, strDVDFirmware;
  bool m_dvdRequest;
  bool b_ata_request;

  DWORD m_lastSysInfoTime;
  CStdString m_mplayerversion;
  CStdString m_kernelversion;
  CStdString m_systemuptime;
  CStdString m_systemtotaluptime;
  CStdString m_cpufrequency;
  CStdString m_xboxversion;
  CStdString m_avcablepackinfo;
  CStdString m_videoencoder;
  bool b_sys_request;





  //Fullscreen OSD Stuff
  DWORD m_AfterSeekTimeout;
  bool m_playerSeeking;
  bool m_playerShowTime;
  bool m_playerShowCodec;
  bool m_playerShowInfo;

  // FPS counters
  float m_fps;
  unsigned int m_frameCounter;
  unsigned int m_lastFPSTime;

  // Xbox Autodetect stuff
  bool m_hasAutoDetectedXbox;

  int m_nextWindowID;
  int m_prevWindowID;

  class CCombinedValue
  {
  public:
    CStdString m_info;    // the text expression
    int m_id;             // the id used to identify this expression
    list<int> m_postfix;  // the postfix binary expression
    void operator=(const CCombinedValue& mSrc);
  };

  int GetOperator(const char ch);
  int TranslateBooleanExpression(const CStdString &expression);
  bool EvaluateBooleanExpression(const CCombinedValue &expression, bool &result, DWORD dwContextWindow);

  std::vector<CCombinedValue> m_CombinedValues;

  // routines for caching the bool results
  bool IsCached(int condition, DWORD contextWindow, bool &result) const;
  void CacheBool(int condition, DWORD contextWindow, bool result);
  std::map<int, bool> m_boolCache;

  CCriticalSection m_critInfo;
};

/*!
 \ingroup strings
 \brief 
 */
extern CGUIInfoManager g_infoManager;
#endif

