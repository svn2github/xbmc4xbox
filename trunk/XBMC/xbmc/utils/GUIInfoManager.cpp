#include "../stdafx.h"
#include "GUIInfoManager.h"
#include "../GUIDialogSeekBar.h"
#include "Weather.h"
#include "../Application.h"
#include "../Util.h"
#include "../lib/libscrobbler/scrobbler.h"
#include "../playlistplayer.h"
#include "../ButtonTranslator.h"
#include "../Visualizations/Visualisation.h"
#include "../MusicDatabase.h"
#include "KaiClient.h"
#include "GUIButtonScroller.h"
#include "../utils/Alarmclock.h"
#include "../utils/lcd.h"
#include "../GUIMediaWindow.h"
#ifdef PRE_SKIN_VERSION_2_0_COMPATIBILITY
#include "SkinInfo.h"
#endif

#include <stack>

#define VERSION_STRING "1.1.0"

// stuff for current song
#include "../filesystem/CDDADirectory.h"
#include "../musicInfoTagLoaderFactory.h"
#include "../filesystem/SndtrkDirectory.h"

#include "FanController.h"


extern char g_szTitleIP[32];

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
//Defined in header instead as they are needed at other places
//#define PLAYER_CACHING               20
//#define PLAYER_DISPLAY_AFTER_SEEK    21
//#define PLAYER_PROGRESS              22
//#define PLAYER_SEEKBAR               23
//#define PLAYER_SEEKTIME              24
//#define PLAYER_SEEKING               25
//#define PLAYER_SHOWTIME              26
#define PLAYER_TIME                  27  
#define PLAYER_TIME_REMAINING        28
#define PLAYER_DURATION              29
//#define PLAYER_SHOWCODEC             30
//#define PLAYER_SHOWINFO              31
#define PLAYER_VOLUME                32
#define PLAYER_MUTED                 33

#define WEATHER_CONDITIONS          100
#define WEATHER_TEMPERATURE         101
#define WEATHER_LOCATION            102

#define SYSTEM_TIME                 110
#define SYSTEM_DATE                 111
#define SYSTEM_CPU_TEMPERATURE      112
#define SYSTEM_GPU_TEMPERATURE      113
#define SYSTEM_FAN_SPEED            114
#define SYSTEM_FREE_SPACE_C         115
// 116 is reserved for space on D
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
#define SYSTEM_NO_SUCH_ALARM        128
#define SYSTEM_HAS_ALARM            129
#define SYSTEM_AUTODETECTION        130
#define SYSTEM_FREE_MEMORY          131
#define SYSTEM_SCREEN_MODE          132
#define SYSTEM_SCREEN_WIDTH         133
#define SYSTEM_SCREEN_HEIGHT        134
#define SYSTEM_CURRENT_WINDOW       135
#define SYSTEM_CURRENT_CONTROL      136

#define LCD_PLAY_ICON               180
#define LCD_PROGRESS_BAR            181

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

#define AUDIOSCROBBLER_ENABLED      300
#define AUDIOSCROBBLER_CONN_STATE   301
#define AUDIOSCROBBLER_SUBMIT_INT   302
#define AUDIOSCROBBLER_FILES_CACHED 303
#define AUDIOSCROBBLER_SUBMIT_STATE 304

#define LISTITEM_THUMB              310
#define LISTITEM_LABEL              311
#define LISTITEM_TITLE              312
#define LISTITEM_TRACKNUMBER        313
#define LISTITEM_ARTIST             314
#define LISTITEM_ALBUM              315
#define LISTITEM_YEAR               316
#define LISTITEM_GENRE              317

#define VISUALISATION_LOCKED        400
#define VISUALISATION_PRESET        401
#define VISUALISATION_NAME          402
#define VISUALISATION_ENABLED       403

#define SKIN_HAS_THEME_START        500
#define SKIN_HAS_THEME_END          509 // allow for max 10 themes

#define SKIN_HAS_SETTING_START      510
#define SKIN_HAS_SETTING_END        600 // allow 90

#define WINDOW_ACTIVE_START         WINDOW_HOME
#define WINDOW_ACTIVE_END           WINDOW_PYTHON_END

#define SYSTEM_IDLE_TIME_START      20000
#define SYSTEM_IDLE_TIME_FINISH     21000 // 1000 seconds

#define CONTROL_GROUP_HAS_FOCUS     29999
#define CONTROL_HAS_FOCUS_START     30000
#define CONTROL_HAS_FOCUS_END       31000 // only up to control id 1000

#define BUTTON_SCROLLER_HAS_ICON_START 31000
#define BUTTON_SCROLLER_HAS_ICON_END   31200  // only allow 100 buttons (normally start at 101)

// the multiple information vector
#define MULTI_INFO_START              40000
#define MULTI_INFO_END                41000 // 1000 references is all we have for now

#define COMBINED_VALUES_START        100000

CGUIInfoManager g_infoManager;

void CGUIInfoManager::CCombinedValue::operator =(const CGUIInfoManager::CCombinedValue& mSrc)
{
  this->m_info = mSrc.m_info;
  this->m_id = mSrc.m_id;
  this->m_postfix = mSrc.m_postfix;
}

CGUIInfoManager::CGUIInfoManager(void)
{
  m_lastSysHeatInfoTime = 0;
  m_lastMusicBitrateTime = 0;
  m_fanSpeed = 0;
  m_gpuTemp = 0;
  m_cpuTemp = 0;
  m_AfterSeekTimeout = 0;
  m_playerSeeking = false;
  m_performingSeek = false;
}

CGUIInfoManager::~CGUIInfoManager(void)
{
}
/// \brief Translates a string as given by the skin into an int that we use for more
/// efficient retrieval of data. Can handle combined strings on the form
/// Player.Caching + VideoPlayer.IsFullscreen (Logical and)
/// Player.HasVideo | Player.HasAudio (Logical or)
int CGUIInfoManager::TranslateString(const CStdString &strCondition)
{
  if (strCondition.find_first_of("|") != strCondition.npos ||
      strCondition.find_first_of("+") != strCondition.npos ||
      strCondition.find_first_of("[") != strCondition.npos ||
      strCondition.find_first_of("]") != strCondition.npos)
  {
    // Have a boolean expression
    // Check if this was added before
    std::vector<CCombinedValue>::iterator it;
    for(it = m_CombinedValues.begin(); it != m_CombinedValues.end(); it++)
    {
      if(strCondition.CompareNoCase(it->m_info) == 0)
        return it->m_id;
    }
    return TranslateBooleanExpression(strCondition);
  }  
  //Just single command.
  return TranslateSingleString(strCondition);
}


/// \brief Translates a string as given by the skin into an int that we use for more
/// efficient retrieval of data.
int CGUIInfoManager::TranslateSingleString(const CStdString &strCondition)
{
  if (strCondition.IsEmpty()) return 0;
  CStdString strTest = strCondition;
  strTest.ToLower();
  strTest.TrimLeft(" ");
  strTest.TrimRight(" ");
  bool bNegate = strTest[0] == '!';
  int ret = 0;

  if(bNegate)
    strTest.Delete(0, 1);

  CStdString strCategory = strTest.Left(strTest.Find("."));

  // translate conditions...
  if (strTest.Equals("false") || strTest.Equals("no") || strTest.Equals("off") || strTest.Equals("disabled")) ret = SYSTEM_ALWAYS_FALSE;
  else if (strTest.Equals("true") || strTest.Equals("yes") || strTest.Equals("on") || strTest.Equals("enabled")) ret = SYSTEM_ALWAYS_TRUE;
  if (strCategory.Equals("player"))
  {
    if (strTest.Equals("player.hasmedia")) ret = PLAYER_HAS_MEDIA;
    else if (strTest.Equals("player.hasaudio")) ret = PLAYER_HAS_AUDIO;
    else if (strTest.Equals("player.hasvideo")) ret = PLAYER_HAS_VIDEO;
    else if (strTest.Equals("player.playing")) ret = PLAYER_PLAYING;
    else if (strTest.Equals("player.paused")) ret = PLAYER_PAUSED;
    else if (strTest.Equals("player.rewinding")) ret = PLAYER_REWINDING;
    else if (strTest.Equals("player.forwarding")) ret = PLAYER_FORWARDING;
    else if (strTest.Equals("player.rewinding2x")) ret = PLAYER_REWINDING_2x;
    else if (strTest.Equals("player.rewinding4x")) ret = PLAYER_REWINDING_4x;
    else if (strTest.Equals("player.rewinding8x")) ret = PLAYER_REWINDING_8x;
    else if (strTest.Equals("player.rewinding16x")) ret = PLAYER_REWINDING_16x;
    else if (strTest.Equals("player.rewinding32x")) ret = PLAYER_REWINDING_32x;
    else if (strTest.Equals("player.forwarding2x")) ret = PLAYER_FORWARDING_2x;
    else if (strTest.Equals("player.forwarding4x")) ret = PLAYER_FORWARDING_4x;
    else if (strTest.Equals("player.forwarding8x")) ret = PLAYER_FORWARDING_8x;
    else if (strTest.Equals("player.forwarding16x")) ret = PLAYER_FORWARDING_16x;
    else if (strTest.Equals("player.forwarding32x")) ret = PLAYER_FORWARDING_32x;
    else if (strTest.Equals("player.canrecord")) ret = PLAYER_CAN_RECORD;
    else if (strTest.Equals("player.recording")) ret = PLAYER_RECORDING;
    else if (strTest.Equals("player.displayafterseek")) ret = PLAYER_DISPLAY_AFTER_SEEK;
    else if (strTest.Equals("player.caching")) ret = PLAYER_CACHING;
    else if (strTest.Equals("player.seekbar")) ret = PLAYER_SEEKBAR;
    else if (strTest.Equals("player.seektime")) ret = PLAYER_SEEKTIME;
    else if (strTest.Equals("player.progress")) ret = PLAYER_PROGRESS;
    else if (strTest.Equals("player.seeking")) ret = PLAYER_SEEKING;
    else if (strTest.Equals("player.showtime")) ret = PLAYER_SHOWTIME;
    else if (strTest.Equals("player.showcodec")) ret = PLAYER_SHOWCODEC;
    else if (strTest.Equals("player.showinfo")) ret = PLAYER_SHOWINFO;
    else if (strTest.Equals("player.time")) ret = PLAYER_TIME;
    else if (strTest.Equals("player.timeremaining")) ret = PLAYER_TIME_REMAINING;
    else if (strTest.Equals("player.duration")) ret = PLAYER_DURATION;
    else if (strTest.Equals("player.volume")) ret = PLAYER_VOLUME;
    else if (strTest.Equals("player.muted")) ret = PLAYER_MUTED;
  }
  else if (strCategory.Equals("weather"))
  {
    if (strTest.Equals("weather.conditions")) ret = WEATHER_CONDITIONS;
    else if (strTest.Equals("weather.temperature")) ret = WEATHER_TEMPERATURE;
    else if (strTest.Equals("weather.location")) ret = WEATHER_LOCATION;
  }
  else if (strCategory.Equals("system"))
  {
    if (strTest.Equals("system.date")) ret = SYSTEM_DATE;
    else if (strTest.Equals("system.time")) ret = SYSTEM_TIME;
    else if (strTest.Equals("system.cputemperature")) ret = SYSTEM_CPU_TEMPERATURE;
    else if (strTest.Equals("system.gputemperature")) ret = SYSTEM_GPU_TEMPERATURE;
    else if (strTest.Equals("system.fanspeed")) ret = SYSTEM_FAN_SPEED;
    else if (strTest.Equals("system.freespace(c)")) ret = SYSTEM_FREE_SPACE_C;
    else if (strTest.Equals("system.freespace(e)")) ret = SYSTEM_FREE_SPACE_E;
    else if (strTest.Equals("system.freespace(f)")) ret = SYSTEM_FREE_SPACE_F;
    else if (strTest.Equals("system.freespace(g)")) ret = SYSTEM_FREE_SPACE_G;
    else if (strTest.Equals("system.buildversion")) ret = SYSTEM_BUILD_VERSION;
    else if (strTest.Equals("system.builddate")) ret = SYSTEM_BUILD_DATE;
    else if (strTest.Equals("system.hasnetwork")) ret = SYSTEM_ETHERNET_LINK_ACTIVE;
    else if (strTest.Equals("system.fps")) ret = SYSTEM_FPS;
    else if (strTest.Equals("system.kaiconnected")) ret = SYSTEM_KAI_CONNECTED;
    else if (strTest.Equals("system.hasmediadvd")) ret = SYSTEM_MEDIA_DVD;
    else if (strTest.Equals("system.autodetection")) ret = SYSTEM_AUTODETECTION;
    else if (strTest.Equals("system.freememory")) ret = SYSTEM_FREE_MEMORY;
    else if (strTest.Equals("system.screenmode")) ret = SYSTEM_SCREEN_MODE;
    else if (strTest.Equals("system.screenwidth")) ret = SYSTEM_SCREEN_WIDTH;
    else if (strTest.Equals("system.screenheight")) ret = SYSTEM_SCREEN_HEIGHT;
    else if (strTest.Equals("system.currentwindow")) ret = SYSTEM_CURRENT_WINDOW;
    else if (strTest.Equals("system.currentcontrol")) ret = SYSTEM_CURRENT_CONTROL;
    else if (strTest.Left(16).Equals("system.idletime("))
    {
      int time = atoi((strTest.Mid(16, strTest.GetLength() - 17).c_str()));
      if (time > SYSTEM_IDLE_TIME_FINISH - SYSTEM_IDLE_TIME_START)
        time = SYSTEM_IDLE_TIME_FINISH - SYSTEM_IDLE_TIME_START;
      if (time > 0)
        ret = SYSTEM_IDLE_TIME_START + time;
    }
    else if (strTest.Left(16).Equals("system.hasalarm("))
      ret = SYSTEM_NO_SUCH_ALARM+g_alarmClock.hasAlarm(strTest.Mid(16,strTest.size()-17));
  }
  else if (strCategory.Equals("lcd"))
  {
    if (strTest.Equals("lcd.playicon")) ret = LCD_PLAY_ICON;
    else if (strTest.Equals("lcd.progressbar")) ret = LCD_PROGRESS_BAR;
  }
  else if (strCategory.Equals("network"))
  {
    if (strTest.Equals("network.ipaddress")) ret = NETWORK_IP_ADDRESS;
  }
  else if (strCategory.Equals("musicplayer"))
  {
    if (strTest.Equals("musicplayer.title")) ret = MUSICPLAYER_TITLE;
    else if (strTest.Equals("musicplayer.album")) ret = MUSICPLAYER_ALBUM;
    else if (strTest.Equals("musicplayer.artist")) ret = MUSICPLAYER_ARTIST;
    else if (strTest.Equals("musicplayer.year")) ret = MUSICPLAYER_YEAR;
    else if (strTest.Equals("musicplayer.genre")) ret = MUSICPLAYER_GENRE;
    else if (strTest.Equals("musicplayer.time")) ret = MUSICPLAYER_TIME;
    else if (strTest.Equals("musicplayer.timeremaining")) ret = MUSICPLAYER_TIME_REMAINING;
    else if (strTest.Equals("musicplayer.timespeed")) ret = MUSICPLAYER_TIME_SPEED;
    else if (strTest.Equals("musicplayer.tracknumber")) ret = MUSICPLAYER_TRACK_NUMBER;
    else if (strTest.Equals("musicplayer.duration")) ret = MUSICPLAYER_DURATION;
    else if (strTest.Equals("musicplayer.cover")) ret = MUSICPLAYER_COVER;
    else if (strTest.Equals("musicplayer.bitrate")) ret = MUSICPLAYER_BITRATE;
    else if (strTest.Equals("musicplayer.playlistlength")) ret = MUSICPLAYER_PLAYLISTLEN;
    else if (strTest.Equals("musicplayer.playlistposition")) ret = MUSICPLAYER_PLAYLISTPOS;
    else if (strTest.Equals("musicplayer.channels")) ret = MUSICPLAYER_CHANNELS;
    else if (strTest.Equals("musicplayer.bitspersample")) ret = MUSICPLAYER_BITSPERSAMPLE;
    else if (strTest.Equals("musicplayer.samplerate")) ret = MUSICPLAYER_SAMPLERATE;
    else if (strTest.Equals("musicplayer.codec")) ret = MUSICPLAYER_CODEC;
  }
  else if (strCategory.Equals("videoplayer"))
  {
    if (strTest.Equals("videoplayer.title")) ret = VIDEOPLAYER_TITLE;
    else if (strTest.Equals("videoplayer.genre")) ret = VIDEOPLAYER_GENRE;
    else if (strTest.Equals("videoplayer.director")) ret = VIDEOPLAYER_DIRECTOR;
    else if (strTest.Equals("videoplayer.year")) ret = VIDEOPLAYER_YEAR;
    else if (strTest.Equals("videoplayer.time")) ret = VIDEOPLAYER_TIME;
    else if (strTest.Equals("videoplayer.timeremaining")) ret = VIDEOPLAYER_TIME_REMAINING;
    else if (strTest.Equals("videoplayer.timespeed")) ret = VIDEOPLAYER_TIME_SPEED;
    else if (strTest.Equals("videoplayer.duration")) ret = VIDEOPLAYER_DURATION;
    else if (strTest.Equals("videoplayer.cover")) ret = VIDEOPLAYER_COVER;
    else if (strTest.Equals("videoplayer.usingoverlays")) ret = VIDEOPLAYER_USING_OVERLAYS;
    else if (strTest.Equals("videoplayer.isfullscreen")) ret = VIDEOPLAYER_ISFULLSCREEN;
    else if (strTest.Equals("videoplayer.hasmenu")) ret = VIDEOPLAYER_HASMENU;
  }
  else if (strCategory.Equals("audioscrobbler"))
  {
    if (strTest.Equals("audioscrobbler.enabled")) ret = AUDIOSCROBBLER_ENABLED;
    else if (strTest.Equals("audioscrobbler.connectstate")) ret = AUDIOSCROBBLER_CONN_STATE;
    else if (strTest.Equals("audioscrobbler.submitinterval")) ret = AUDIOSCROBBLER_SUBMIT_INT;
    else if (strTest.Equals("audioscrobbler.filescached")) ret = AUDIOSCROBBLER_FILES_CACHED;
    else if (strTest.Equals("audioscrobbler.submitstate")) ret = AUDIOSCROBBLER_SUBMIT_STATE;
  }
  else if (strCategory.Equals("listitem"))
  {
    if (strTest.Equals("listitem.thumb")) ret = LISTITEM_THUMB;
    else if (strTest.Equals("listitem.label")) ret = LISTITEM_LABEL;
    else if (strTest.Equals("listitem.title")) ret = LISTITEM_TITLE;
    else if (strTest.Equals("listitem.tracknumber")) ret = LISTITEM_TRACKNUMBER;
    else if (strTest.Equals("listitem.artist")) ret = LISTITEM_ARTIST;
    else if (strTest.Equals("listitem.album")) ret = LISTITEM_ALBUM;
    else if (strTest.Equals("listitem.year")) ret = LISTITEM_YEAR;
    else if (strTest.Equals("listitem.genre")) ret = LISTITEM_GENRE;
  }
  else if (strCategory.Equals("visualisation"))
  {
    if (strTest.Equals("visualisation.locked")) ret = VISUALISATION_LOCKED;
    else if (strTest.Equals("visualisation.preset")) ret = VISUALISATION_PRESET;
    else if (strTest.Equals("visualisation.name")) ret = VISUALISATION_NAME;
    else if (strTest.Equals("visualisation.enabled")) ret = VISUALISATION_ENABLED;
  }
  else if (strCategory.Equals("skin"))
  {
    if (strTest.Left(16).Equals("skin.hassetting("))
    {
      CStdString settingName;
      settingName.Format("%s.%s", g_guiSettings.GetString("LookAndFeel.Skin").c_str(), strTest.Mid(16, strTest.GetLength() - 17).c_str());
      ret = SKIN_HAS_SETTING_START + ConditionalStringParameter(settingName);
    }
    else if (strTest.Left(14).Equals("skin.hastheme("))
      ret = SKIN_HAS_THEME_START + ConditionalStringParameter(strTest.Mid(14, strTest.GetLength() -  15));
  }
  else if (strTest.Left(16).Equals("window.isactive("))
  {
    int winID = g_buttonTranslator.TranslateWindowString(strTest.Mid(16, strTest.GetLength() - 17).c_str());
    if (winID != WINDOW_INVALID)
      ret = winID;
  }
  else if (strTest.Left(17).Equals("control.hasfocus("))
  {
    int controlID = atoi(strTest.Mid(17, strTest.GetLength() - 18).c_str());
    if (controlID)
      ret = controlID + CONTROL_HAS_FOCUS_START;
  }
  else if (strTest.Left(13).Equals("controlgroup("))
  {
    int groupID = atoi(strTest.Mid(13).c_str());
    int controlID = 0;
    int controlPos = strTest.Find(".hasfocus(");
    if (controlPos > 0)
      controlID = atoi(strTest.Mid(controlPos + 10).c_str());
    if (groupID && controlID)
    {
      return AddMultiInfo(GUIInfo(bNegate ? -CONTROL_GROUP_HAS_FOCUS : CONTROL_GROUP_HAS_FOCUS, groupID, controlID));
    }
  }
  else if ((strTest.Left(23).Equals("buttonscroller.hasicon(") && g_SkinInfo.GetVersion() < 1.8) || strTest.Left(24).Equals("buttonscroller.hasfocus("))
  {
    int controlID = atoi(strTest.Mid(g_SkinInfo.GetVersion() < 1.8 ? 23 : 24, strTest.GetLength() - 24).c_str());
    if (controlID)
      ret = controlID + BUTTON_SCROLLER_HAS_ICON_START;
  }

  return bNegate ? -ret : ret;
}

wstring CGUIInfoManager::GetLabel(int info)
{
  CStdString strLabel;
  switch (info)
  {
  case WEATHER_CONDITIONS:
    strLabel = g_weatherManager.GetInfo(WEATHER_LABEL_CURRENT_COND);
    break;
  case WEATHER_TEMPERATURE:
    strLabel = g_weatherManager.GetInfo(WEATHER_LABEL_CURRENT_TEMP);
    break;
  case WEATHER_LOCATION:
    strLabel = g_weatherManager.GetInfo(WEATHER_LABEL_LOCATION);
    break;
  case SYSTEM_TIME:
    strLabel = GetTime();
    break;
  case SYSTEM_DATE:
    strLabel = GetDate();
    break;
  case SYSTEM_FPS:
    strLabel.Format("%02.2f", m_fps);
    break;
  case PLAYER_VOLUME:
    strLabel.Format("%2.1f dB", (float)g_stSettings.m_nVolumeLevel * 0.01f);
    break;
  case PLAYER_TIME:
    strLabel = GetCurrentPlayTime();
    break;
  case PLAYER_TIME_REMAINING:
    strLabel = GetCurrentPlayTimeRemaining();
    break;
  case PLAYER_DURATION:
    if (g_application.IsPlayingAudio())
      strLabel = GetMusicLabel(info);
    else
      strLabel = GetVideoLabel(info);
    break;
  case MUSICPLAYER_TITLE:
  case MUSICPLAYER_ALBUM:
  case MUSICPLAYER_ARTIST:
  case MUSICPLAYER_GENRE:
  case MUSICPLAYER_YEAR:
  case MUSICPLAYER_TIME:
  case MUSICPLAYER_TIME_SPEED:
  case MUSICPLAYER_TIME_REMAINING:
  case MUSICPLAYER_TRACK_NUMBER:
  case MUSICPLAYER_DURATION:
  case MUSICPLAYER_BITRATE:
  case MUSICPLAYER_PLAYLISTLEN:
  case MUSICPLAYER_PLAYLISTPOS:
  case MUSICPLAYER_CHANNELS:
  case MUSICPLAYER_BITSPERSAMPLE:
  case MUSICPLAYER_SAMPLERATE:
  case MUSICPLAYER_CODEC:
    strLabel = GetMusicLabel(info);
  break;
  case VIDEOPLAYER_TITLE:
  case VIDEOPLAYER_GENRE:
  case VIDEOPLAYER_DIRECTOR:
  case VIDEOPLAYER_YEAR:
  case VIDEOPLAYER_TIME:
  case VIDEOPLAYER_TIME_REMAINING:
  case VIDEOPLAYER_TIME_SPEED:
  case VIDEOPLAYER_DURATION:
    strLabel = GetVideoLabel(info);
  break;
  case SYSTEM_FREE_SPACE_C:
  case SYSTEM_FREE_SPACE_E:
  case SYSTEM_FREE_SPACE_F:
  case SYSTEM_FREE_SPACE_G:
    return GetFreeSpace(info);
  break;
  case SYSTEM_CPU_TEMPERATURE:
    return GetSystemHeatInfo("cpu");
    break;
  case SYSTEM_GPU_TEMPERATURE:
    return GetSystemHeatInfo("gpu");
    break;
  case SYSTEM_FAN_SPEED:
    return GetSystemHeatInfo("fan");
    break;
  case SYSTEM_BUILD_VERSION:
    strLabel = GetVersion();
    break;
  case SYSTEM_BUILD_DATE:
    strLabel = GetBuild();
    break;
  case SYSTEM_FREE_MEMORY:
    {
      MEMORYSTATUS stat;
      GlobalMemoryStatus(&stat);
      strLabel.Format("%i Mb", stat.dwAvailPhys / (1024 * 1024));
    }
    break;
  case SYSTEM_SCREEN_MODE:
    strLabel = g_settings.m_ResInfo[g_graphicsContext.GetVideoResolution()].strMode;
    break;
  case SYSTEM_SCREEN_WIDTH:
    strLabel.Format("%i", g_settings.m_ResInfo[g_graphicsContext.GetVideoResolution()].iWidth);
    break;
  case SYSTEM_SCREEN_HEIGHT:
    strLabel.Format("%i", g_settings.m_ResInfo[g_graphicsContext.GetVideoResolution()].iHeight);
    break;
  case SYSTEM_CURRENT_WINDOW:
    return g_localizeStrings.Get(m_gWindowManager.GetActiveWindow());
    break;
  case SYSTEM_CURRENT_CONTROL:
    {
      CGUIWindow *window = m_gWindowManager.GetWindow(m_gWindowManager.GetActiveWindow());
      if (window)
      {
        CGUIControl *control = (CGUIControl* )window->GetControl(window->GetFocusedControl());
        if (control)
          strLabel = control->GetDescription();
      }
    }
    break;
  case LCD_PLAY_ICON:
    {
      int iPlaySpeed = g_application.GetPlaySpeed();
      if (iPlaySpeed < 1)
        strLabel.Format("\3:%ix", iPlaySpeed);
      else if (iPlaySpeed > 1)
        strLabel.Format("\4:%ix", iPlaySpeed);
      else if (g_application.m_pPlayer && g_application.m_pPlayer->IsPaused())
        strLabel.Format("\7");
      else
        strLabel.Format("\5");
    }
    break;
  case LCD_PROGRESS_BAR:
    if (g_lcd) strLabel = g_lcd->GetProgressBar(g_application.GetTime(), g_application.GetTotalTime());
    break;
  case NETWORK_IP_ADDRESS:
    {
      const WCHAR* pszIP = g_localizeStrings.Get(150).c_str();
      WCHAR wzIP[32];
      swprintf(wzIP, L"%s: %S", pszIP, g_szTitleIP);
      wstring strReturn = wzIP;
      return strReturn;
    }
    break;
  case AUDIOSCROBBLER_CONN_STATE:
  case AUDIOSCROBBLER_SUBMIT_INT:
  case AUDIOSCROBBLER_FILES_CACHED:
  case AUDIOSCROBBLER_SUBMIT_STATE:
    strLabel=GetAudioScrobblerLabel(info);
    break;
  case VISUALISATION_PRESET:
    {
      CGUIMessage msg(GUI_MSG_GET_VISUALISATION, 0, 0);
      g_graphicsContext.SendMessage(msg);
      if (msg.GetLPVOID())
      {
        CVisualisation *pVis = (CVisualisation *)msg.GetLPVOID();
        char *preset = pVis->GetPreset();
        if (preset)
        {
          strLabel = preset;
          CUtil::RemoveExtension(strLabel);
        }
      }
    }
    break;
  case VISUALISATION_NAME:
    {
      strLabel = g_guiSettings.GetString("MyMusic.Visualisation");
      if (strLabel != "None" && strLabel.size() > 4)
      { // make it look pretty
        strLabel = strLabel.Left(strLabel.size() - 4);
        strLabel[0] = toupper(strLabel[0]);
      }
    }
    break;
  case PLAYER_SEEKTIME:
    {
      strLabel = ((CGUIDialogSeekBar*)m_gWindowManager.GetWindow(WINDOW_DIALOG_SEEK_BAR))->GetSeekTimeLabel();
    }
    break;
  case LISTITEM_LABEL:
  case LISTITEM_TITLE:
  case LISTITEM_TRACKNUMBER:
  case LISTITEM_ARTIST:
  case LISTITEM_ALBUM:
  case LISTITEM_YEAR:
  case LISTITEM_GENRE:
    {
      CGUIWindow *pWindow = m_gWindowManager.GetWindow(m_gWindowManager.GetActiveWindow());
      if (pWindow && pWindow->IsMediaWindow())
      {
        strLabel = GetItemLabel(((CGUIMediaWindow *)pWindow)->GetCurrentListItem(), info);
      }
    }
    break;
  }

  // convert our CStdString to a wstring (which the label expects!)
  WCHAR szLabel[256];
  swprintf(szLabel, L"%S", strLabel.c_str() );
  wstring strReturn = szLabel;
  return strReturn;
}

// tries to get a integer value for use in progressbars/sliders and such
int CGUIInfoManager::GetInt(int info) const
{
  if (info == PLAYER_VOLUME)
    return g_application.GetVolume();
  else if( g_application.IsPlaying() && g_application.m_pPlayer)
  {
    switch( info )
    {
    case PLAYER_PROGRESS:
      return (int)(g_application.GetPercentage());
    case PLAYER_SEEKBAR:
      return (int)(((CGUIDialogSeekBar*)m_gWindowManager.GetWindow(WINDOW_DIALOG_SEEK_BAR))->GetPercentage());
    case PLAYER_CACHING:
      return (int)(g_application.m_pPlayer->GetCacheLevel());
    }
  }
  return 0;
}
// checks the condition and returns it as necessary.  Currently used
// for toggle button controls and visibility of images.
bool CGUIInfoManager::GetBool(int condition1, DWORD dwContextWindow) const
{
  if(  condition1 >= COMBINED_VALUES_START && (condition1 - COMBINED_VALUES_START) < (int)(m_CombinedValues.size()) )
  {
    const CCombinedValue &comb = m_CombinedValues[condition1 - COMBINED_VALUES_START];
    bool result;
    if (EvaluateBooleanExpression(comb, result, dwContextWindow))
      return result;
    return false;
  }

  int condition = abs(condition1);
  bool bReturn = false;

  // GeminiServer: Ethernet Link state checking
  // Will check if the Xbox has a Ethernet Link connection! [Cable in!]
  // This can used for the skinner to switch off Network or Inter required functions
  if ( condition == SYSTEM_ALWAYS_TRUE)
    bReturn = true;
  else if (condition == SYSTEM_ALWAYS_FALSE)
    bReturn = false;
  else if (condition == SYSTEM_ETHERNET_LINK_ACTIVE)
    bReturn = (XNetGetEthernetLinkStatus() & XNET_ETHERNET_LINK_ACTIVE);
  else if (condition > SYSTEM_IDLE_TIME_START && condition <= SYSTEM_IDLE_TIME_FINISH)
    bReturn = (g_application.GlobalIdleTime() >= condition - SYSTEM_IDLE_TIME_START);
  else if (condition >= WINDOW_ACTIVE_START && condition <= WINDOW_ACTIVE_END)// check for Window.IsActive(window)
    bReturn = m_gWindowManager.IsWindowActive(condition);
  else if (condition == SYSTEM_HAS_ALARM)
    bReturn = true;
  else if (condition == SYSTEM_NO_SUCH_ALARM)
    bReturn = false;
  else if (condition == SYSTEM_AUTODETECTION)
    bReturn = HasAutodetectedXbox();
  else if (condition == PLAYER_MUTED)
    bReturn = g_stSettings.m_bMute;
  else if (condition >= CONTROL_HAS_FOCUS_START && condition <= CONTROL_HAS_FOCUS_END)
  {
    if( !dwContextWindow ) dwContextWindow = m_gWindowManager.GetActiveWindow();

    CGUIWindow *pWindow = m_gWindowManager.GetWindow(dwContextWindow);
    if (pWindow) 
      bReturn = (pWindow->GetFocusedControl() == condition - CONTROL_HAS_FOCUS_START);
  }
  else if (condition >= BUTTON_SCROLLER_HAS_ICON_START && condition <= BUTTON_SCROLLER_HAS_ICON_END)
  {
    if( !dwContextWindow ) dwContextWindow = m_gWindowManager.GetActiveWindow();

    CGUIWindow *pWindow = m_gWindowManager.GetWindow(dwContextWindow);
    if (pWindow)
    {
      CGUIControl *pControl = (CGUIControl *)pWindow->GetControl(pWindow->GetFocusedControl());
      if (pControl && pControl->GetControlType() == CGUIControl::GUICONTROL_BUTTONBAR)
        bReturn = ((CGUIButtonScroller *)pControl)->GetActiveButtonID() == condition - BUTTON_SCROLLER_HAS_ICON_START;
    }
  }
  else if (condition == SYSTEM_KAI_CONNECTED)
    bReturn = g_guiSettings.GetBool("XLinkKai.Enabled") && CKaiClient::GetInstance()->IsEngineConnected();
  else if (condition == SYSTEM_MEDIA_DVD)
  {
    // GeminiServer: DVD Drive state
    CIoSupport TrayIO;
    int iTrayState = TrayIO.GetTrayState();
    if ( iTrayState == DRIVE_CLOSED_MEDIA_PRESENT || iTrayState == TRAY_CLOSED_MEDIA_PRESENT )
      bReturn = true;
    else 
      bReturn = false;
  }
  else if (condition == PLAYER_SHOWINFO)
    bReturn = m_playerShowInfo;
  else if (condition == PLAYER_SHOWCODEC)
    bReturn = m_playerShowCodec;
  else if (condition >= SKIN_HAS_THEME_START && condition <= SKIN_HAS_THEME_END)
  { // Note that the code used here could probably be extended to general
    // settings conditions (parameter would need to store both the setting name and
    // the and the comparison string)
    CStdString theme = g_guiSettings.GetString("LookAndFeel.SkinTheme").ToLower();
    CUtil::RemoveExtension(theme);
    bReturn = theme.Equals(m_stringParameters[condition - SKIN_HAS_THEME_START]);
  }
  else if (condition >= SKIN_HAS_SETTING_START && condition <= SKIN_HAS_SETTING_END)
    bReturn = g_settings.GetSkinSetting(m_stringParameters[condition - SKIN_HAS_SETTING_START].c_str());
  else if (condition >= MULTI_INFO_START && condition <= MULTI_INFO_END)
  {
    return GetMultiInfoBool(m_multiInfo[condition - MULTI_INFO_START], dwContextWindow);
  }
  else if (g_application.IsPlaying())
  {
    switch (condition)
    {
    case PLAYER_HAS_MEDIA:
      bReturn = true;
      break;
    case PLAYER_HAS_AUDIO:
      bReturn = g_application.IsPlayingAudio();
      break;
    case PLAYER_HAS_VIDEO:
      bReturn = g_application.IsPlayingVideo();
      break;
    case PLAYER_PLAYING:
      bReturn = !g_application.m_pPlayer->IsPaused() && (g_application.GetPlaySpeed() == 1);
      break;
    case PLAYER_PAUSED:
      bReturn = g_application.m_pPlayer->IsPaused();
      break;
    case PLAYER_REWINDING:
      bReturn = g_application.GetPlaySpeed() < 1;
      break;
    case PLAYER_FORWARDING:
      bReturn = g_application.GetPlaySpeed() > 1;
      break;
    case PLAYER_REWINDING_2x:
      bReturn = g_application.GetPlaySpeed() == -2;
      break;
    case PLAYER_REWINDING_4x:
      bReturn = g_application.GetPlaySpeed() == -4;
      break;
    case PLAYER_REWINDING_8x:
      bReturn = g_application.GetPlaySpeed() == -8;
      break;
    case PLAYER_REWINDING_16x:
      bReturn = g_application.GetPlaySpeed() == -16;
      break;
    case PLAYER_REWINDING_32x:
      bReturn = g_application.GetPlaySpeed() == -32;
      break;
    case PLAYER_FORWARDING_2x:
      bReturn = g_application.GetPlaySpeed() == 2;
      break;
    case PLAYER_FORWARDING_4x:
      bReturn = g_application.GetPlaySpeed() == 4;
      break;
    case PLAYER_FORWARDING_8x:
      bReturn = g_application.GetPlaySpeed() == 8;
      break;
    case PLAYER_FORWARDING_16x:
      bReturn = g_application.GetPlaySpeed() == 16;
      break;
    case PLAYER_FORWARDING_32x:
      bReturn = g_application.GetPlaySpeed() == 32;
      break;
    case PLAYER_CAN_RECORD:
      bReturn = g_application.m_pPlayer->CanRecord();
      break;
    case PLAYER_RECORDING:
      bReturn = g_application.m_pPlayer->IsRecording();
    break;
    case PLAYER_DISPLAY_AFTER_SEEK:
      bReturn = GetDisplayAfterSeek();
    break;
    case PLAYER_CACHING:
      bReturn = g_application.m_pPlayer->IsCaching();
    break;
    case PLAYER_SEEKBAR:
      bReturn = ((CGUIDialogSeekBar*)m_gWindowManager.GetWindow(WINDOW_DIALOG_SEEK_BAR))->IsRunning();
    break;
    case PLAYER_SEEKING:
      bReturn = m_playerSeeking;
    break;
    case PLAYER_SHOWTIME:
      bReturn = m_playerShowTime;
    break;
    case AUDIOSCROBBLER_ENABLED:
      bReturn = g_guiSettings.GetBool("MyMusic.UseAudioScrobbler");
    break;
    case VIDEOPLAYER_USING_OVERLAYS:
      bReturn = (g_guiSettings.GetInt("VideoPlayer.RenderMethod") == RENDER_OVERLAYS);
    break;
    case VIDEOPLAYER_ISFULLSCREEN:
      bReturn = m_gWindowManager.GetActiveWindow() == WINDOW_FULLSCREEN_VIDEO;
    break;
    case VIDEOPLAYER_HASMENU:
      bReturn = g_application.m_pPlayer->HasMenu();
    break;
    case VISUALISATION_LOCKED:
      {
        CGUIMessage msg(GUI_MSG_GET_VISUALISATION, 0, 0);
        g_graphicsContext.SendMessage(msg);
        if (msg.GetLPVOID())
        {
          CVisualisation *pVis = (CVisualisation *)msg.GetLPVOID();
          bReturn = pVis->IsLocked();
        }
      }
    break;
    case VISUALISATION_ENABLED:
      bReturn = g_guiSettings.GetString("MyMusic.Visualisation") != "None";
    break;
    }
  }
  return (condition1 < 0) ? !bReturn : bReturn;
}

/// \brief Examines the multi information sent and returns true or false accordingly.
bool CGUIInfoManager::GetMultiInfoBool(const GUIInfo &info, DWORD dwContextWindow) const
{
  bool bReturn = false;
  int condition = abs(info.m_info);
  switch (condition)
  {
    case CONTROL_GROUP_HAS_FOCUS:
      if( !dwContextWindow ) dwContextWindow = m_gWindowManager.GetActiveWindow();

      CGUIWindow *pWindow = m_gWindowManager.GetWindow(dwContextWindow);
      if (pWindow) 
        bReturn = pWindow->ControlGroupHasFocus(info.m_data1, info.m_data2);
    break;
  }
  return (info.m_info < 0) ? !bReturn : bReturn;
}

/// \brief Obtains the filename of the image to show from whichever subsystem is needed
CStdString CGUIInfoManager::GetImage(int info)
{
  if (info == WEATHER_CONDITIONS)
    return g_weatherManager.GetInfo(WEATHER_IMAGE_CURRENT_ICON);
  else if (info == MUSICPLAYER_COVER)
  {
    if (!g_application.IsPlayingAudio()) return "";
    return m_currentSong.HasThumbnail() ? m_currentSong.GetThumbnailImage() : "defaultAlbumCover.png";
  }
  else if (info == VIDEOPLAYER_COVER)
  {
    if (!g_application.IsPlayingVideo()) return "";
    return m_currentMovieThumb;
  }
  else if (info == LISTITEM_THUMB)
  {
    CGUIWindow *pWindow = m_gWindowManager.GetWindow(m_gWindowManager.GetActiveWindow());
    if (pWindow && pWindow->IsMediaWindow())
    {
      const CFileItem *item = ((CGUIMediaWindow *)pWindow)->GetCurrentListItem();
      if (item)
      {
        return item->GetThumbnailImage();
      }
    }
  }
  return "";
}

CStdString CGUIInfoManager::GetDate(bool bNumbersOnly)
{
  CStdString text;
  SYSTEMTIME time;
  GetLocalTime(&time);

  if (bNumbersOnly)
  {
    CStdString strDate;
    if (g_guiSettings.GetInt("XBDateTime.DateFormat") == DATETIME_FORMAT_EU)
      text.Format("%d-%d-%d", time.wDay, time.wMonth, time.wYear);
    else
      text.Format("%d-%d-%d", time.wMonth, time.wDay, time.wYear);
  }
  else
  {
    CStdString day;
    switch (time.wDayOfWeek)
    {
    case 1 : day = g_localizeStrings.Get(11); break;
    case 2 : day = g_localizeStrings.Get(12); break;
    case 3 : day = g_localizeStrings.Get(13); break;
    case 4 : day = g_localizeStrings.Get(14); break;
    case 5 : day = g_localizeStrings.Get(15); break;
    case 6 : day = g_localizeStrings.Get(16); break;
    default: day = g_localizeStrings.Get(17); break;
    }

    CStdString month;
    switch (time.wMonth)
    {
    case 1 : month = g_localizeStrings.Get(21); break;
    case 2 : month = g_localizeStrings.Get(22); break;
    case 3 : month = g_localizeStrings.Get(23); break;
    case 4 : month = g_localizeStrings.Get(24); break;
    case 5 : month = g_localizeStrings.Get(25); break;
    case 6 : month = g_localizeStrings.Get(26); break;
    case 7 : month = g_localizeStrings.Get(27); break;
    case 8 : month = g_localizeStrings.Get(28); break;
    case 9 : month = g_localizeStrings.Get(29); break;
    case 10: month = g_localizeStrings.Get(30); break;
    case 11: month = g_localizeStrings.Get(31); break;
    default: month = g_localizeStrings.Get(32); break;
    }

    if (day.size() && month.size())
    {
      if (g_guiSettings.GetInt("XBDateTime.DateFormat") == DATETIME_FORMAT_EU)
        text.Format("%s, %d %s", day.c_str(), time.wDay, month.c_str());
      else
        text.Format("%s, %s %d", day.c_str(), month.c_str(), time.wDay);
    }
    else
      text.Format("no date");
  }
  return text;
}

CStdString CGUIInfoManager::GetTime(bool bSeconds)
{
  CStdString text;
  SYSTEMTIME time;
  GetLocalTime(&time);

  INT iHour = time.wHour;

  if (g_guiSettings.GetInt("XBDateTime.TimeFormat") == DATETIME_FORMAT_US)
  {
    if (iHour > 11)
    {
      iHour -= (12 * (iHour > 12));
      if (bSeconds)
        text.Format("%2d:%02d:%02d PM", iHour, time.wMinute, time.wSecond);
      else
        text.Format("%2d:%02d PM", iHour, time.wMinute);
    }
    else
    {
      iHour += (12 * (iHour < 1));
      if (bSeconds)
        text.Format("%2d:%02d:%02d AM", iHour, time.wMinute, time.wSecond);
      else
        text.Format("%2d:%02d AM", iHour, time.wMinute);
    }
  }
  else
  {
    if (bSeconds)
      text.Format("%2d:%02d:%02d", iHour, time.wMinute, time.wSecond);
    else
      text.Format("%02d:%02d", iHour, time.wMinute);
  }
  return text;
}

CStdString CGUIInfoManager::GetMusicLabel(int item)
{
  if (!g_application.IsPlayingAudio()) return "";
  CMusicInfoTag& tag = m_currentSong.m_musicInfoTag;
  switch (item)
  {
  case MUSICPLAYER_TITLE:
    if (tag.GetTitle().size()) { return tag.GetTitle(); }
    break;
  case MUSICPLAYER_ALBUM: 
    if (tag.GetAlbum().size()) { return tag.GetAlbum(); }
    break;
  case MUSICPLAYER_ARTIST: 
    if (tag.GetArtist().size()) { return tag.GetArtist(); }
    break;
  case MUSICPLAYER_YEAR: 
    if (tag.GetYear().size()) { return tag.GetYear(); }
    break;
  case MUSICPLAYER_GENRE: 
    if (tag.GetGenre().size()) { return tag.GetGenre(); }
    break;
  case MUSICPLAYER_TIME:
    return GetCurrentPlayTime();
  case MUSICPLAYER_TIME_REMAINING:
    return GetCurrentPlayTimeRemaining();
  case MUSICPLAYER_TIME_SPEED:
    {
      CStdString strTime;
      if (g_application.GetPlaySpeed() != 1)
        strTime.Format("%s (%ix)", GetCurrentPlayTime().c_str(), g_application.GetPlaySpeed());
      else
        strTime = GetCurrentPlayTime();
      return strTime;
    }
    break;
  case MUSICPLAYER_TRACK_NUMBER:
    {
      CStdString strTrack;
      if (tag.Loaded() && tag.GetTrackNumber() > 0)
      {
        strTrack.Format("%02i", tag.GetTrackNumber());
        return strTrack;
      }
    }
    break;
  case MUSICPLAYER_DURATION:
  case PLAYER_DURATION:
    {
      CStdString strDuration = "00:00";
      if (tag.GetDuration() > 0)
        CUtil::SecondsToHMSString(tag.GetDuration(), strDuration);
      else
      {
        unsigned int iTotal = (unsigned int)g_application.GetTotalTime();
        if (iTotal > 0)
          CUtil::SecondsToHMSString(iTotal, strDuration, true);
      }
      return strDuration;
    }
    break;
  case MUSICPLAYER_PLAYLISTLEN:
    {
      CStdString strPlayListLength = L"";
      if (g_playlistPlayer.GetCurrentPlaylist() == PLAYLIST_MUSIC || g_playlistPlayer.GetCurrentPlaylist() == PLAYLIST_MUSIC_TEMP)
  	  {
  			strPlayListLength.Format("%i", g_playlistPlayer.GetPlaylist(g_playlistPlayer.GetCurrentPlaylist()).size());
  	  }
      return strPlayListLength;
  	}
	  break;
  case MUSICPLAYER_PLAYLISTPOS:
    {
      CStdString strPlayListPosition = L"";
      if (g_playlistPlayer.GetCurrentPlaylist() == PLAYLIST_MUSIC || g_playlistPlayer.GetCurrentPlaylist() == PLAYLIST_MUSIC_TEMP)
  	  {
  			strPlayListPosition.Format("%i",g_playlistPlayer.GetCurrentSong() + 1);
  	  }
      return strPlayListPosition;
  	}
  	break;
  case MUSICPLAYER_BITRATE:
    {
      float fTimeSpan = (float)(timeGetTime() - m_lastMusicBitrateTime);
      if (fTimeSpan >= 500.0f)
      {
        m_MusicBitrate = g_application.m_pPlayer->GetBitrate();
        m_lastMusicBitrateTime = timeGetTime();
      }
      CStdString strBitrate = L"";
      if (m_MusicBitrate > 0)
        strBitrate.Format("%i", m_MusicBitrate);
      return strBitrate;
    }
    break;
  case MUSICPLAYER_CHANNELS:
    {
      CStdString strChannels = L"";
	    if (g_application.m_pPlayer->GetChannels() > 0)
	    {
	      strChannels.Format("%i", g_application.m_pPlayer->GetChannels());
	    }
      return strChannels;
    }
    break;
  case MUSICPLAYER_BITSPERSAMPLE:
    {
      CStdString strBitsPerSample = L"";
	    if (g_application.m_pPlayer->GetBitsPerSample() > 0)
	    {
	      strBitsPerSample.Format("%i", g_application.m_pPlayer->GetBitsPerSample());
	    }
      return strBitsPerSample;
    }
    break;
  case MUSICPLAYER_SAMPLERATE:
    {
      CStdString strSampleRate = L"";
	    if (g_application.m_pPlayer->GetSampleRate() > 0)
	    {
	      strSampleRate.Format("%i",g_application.m_pPlayer->GetSampleRate());
	    }
      return strSampleRate;
    }
    break;
  case MUSICPLAYER_CODEC:
    {
      CStdString strCodec;
      strCodec.Format("%s", g_application.m_pPlayer->GetCodecName().c_str());
      return strCodec;
    }
    break;
  }
  return "";
}

CStdString CGUIInfoManager::GetVideoLabel(int item)
{
  if (!g_application.IsPlayingVideo()) return "";
  switch (item)
  {
  case VIDEOPLAYER_TITLE:
    return m_currentMovie.m_strTitle;
    break;
  case VIDEOPLAYER_GENRE:
    return m_currentMovie.m_strGenre;
    break;
  case VIDEOPLAYER_DIRECTOR:
    return m_currentMovie.m_strDirector;
    break;
  case VIDEOPLAYER_YEAR:
    if (m_currentMovie.m_iYear > 0)
    {
      CStdString strYear;
      strYear.Format("%i", m_currentMovie.m_iYear);
      return strYear;
    }
    break;
  case VIDEOPLAYER_TIME:
    return GetCurrentPlayTime();
  case VIDEOPLAYER_TIME_REMAINING:
    return GetCurrentPlayTimeRemaining();
  case VIDEOPLAYER_TIME_SPEED:
    {
      CStdString strTime;
      if (g_application.GetPlaySpeed() != 1)
        strTime.Format("%s (%ix)", GetCurrentPlayTime().c_str(), g_application.GetPlaySpeed());
      else
        strTime = GetCurrentPlayTime();
      return strTime;
    }
    break;
  case VIDEOPLAYER_DURATION:
  case PLAYER_DURATION:
    {
      CStdString strDuration = "00:00:00";
      unsigned int iTotal = (unsigned int)g_application.GetTotalTime();
      if (iTotal > 0)
        CUtil::SecondsToHMSString(iTotal, strDuration, true);
      return strDuration;
    }
    break;
  }
  return "";
}

__int64 CGUIInfoManager::GetPlayTime()
{
  if (g_application.IsPlaying())
  {
    __int64 lPTS = (__int64)(g_application.GetTime() * 1000);
    if (lPTS < 0) lPTS = 0;
    return lPTS;
  }
  return 0;
}

CStdString CGUIInfoManager::GetCurrentPlayTime()
{
  CStdString strTime;
  if (g_application.IsPlayingAudio())
    CUtil::SecondsToHMSString((int)(GetPlayTime()/1000), strTime);
  else if (g_application.IsPlayingVideo())
    CUtil::SecondsToHMSString((int)(GetPlayTime()/1000), strTime, true);
  return strTime;
}

int CGUIInfoManager::GetTotalPlayTime()
{
  int iTotalTime = (int)g_application.GetTotalTime();
  return iTotalTime > 0 ? iTotalTime : 0;
}

int CGUIInfoManager::GetPlayTimeRemaining()
{
  int iReverse = GetTotalPlayTime() - (int)g_application.GetTime();
  return iReverse > 0 ? iReverse : 0;
}

CStdString CGUIInfoManager::GetCurrentPlayTimeRemaining()
{
  CStdString strTime;
  if (g_application.IsPlayingAudio())
    CUtil::SecondsToHMSString(GetPlayTimeRemaining(), strTime);
  else if (g_application.IsPlayingVideo())
    CUtil::SecondsToHMSString(GetPlayTimeRemaining(), strTime, true);
  return strTime;
}

void CGUIInfoManager::ResetCurrentItem()
{ 
  m_currentSong.Reset();
  m_currentMovie.Reset();
  m_currentMovieThumb = "";
}

void CGUIInfoManager::SetCurrentItem(CFileItem &item)
{
  if (item.IsLastFM()) return; //last.fm handles it's own songinfo
  ResetCurrentItem();
  if (item.IsAudio())
    SetCurrentSong(item);
  else
    SetCurrentMovie(item);
}

void CGUIInfoManager::SetCurrentAlbumThumb(const CStdString thumbFileName)
{
  if (CFile::Exists(thumbFileName))
    m_currentSong.SetThumbnailImage(thumbFileName);
  else
  {
    m_currentSong.SetThumbnailImage("");
    m_currentSong.FillInDefaultIcon();
  }
}

void CGUIInfoManager::SetCurrentSong(CFileItem &item)
{
  CLog::Log(LOGDEBUG,"CGUIInfoManager::SetCurrentSong(%s)",item.m_strPath.c_str());
  m_currentSong = item;

  // Get a reference to the item's tag
  CMusicInfoTag& tag = m_currentSong.m_musicInfoTag;
  // check if we don't have the tag already loaded
  if (!tag.Loaded())
  {
    // we have a audio file.
    // Look if we have this file in database...
    bool bFound = false;
    if (g_musicDatabase.Open())
    {
      CSong song;
      bFound = g_musicDatabase.GetSongByFileName(m_currentSong.m_strPath, song);
      m_currentSong.m_musicInfoTag.SetSong(song);
      g_musicDatabase.Close();
    }

    if (!bFound)
    {
      // always get id3 info for the overlay
      CMusicInfoTagLoaderFactory factory;
      auto_ptr<IMusicInfoTagLoader> pLoader (factory.CreateLoader(m_currentSong.m_strPath));
      // Do we have a tag loader for this file type?
      if (NULL != pLoader.get())
        pLoader->Load(m_currentSong.m_strPath, tag);
    }
  }

  // If we have tag information, ...
  if (tag.Loaded())
  {
    if (!tag.GetTitle().size())
    {
      // No title in tag, show filename only
      CSndtrkDirectory dir;
      char NameOfSong[64];
      if (dir.FindTrackName(m_currentSong.m_strPath, NameOfSong))
        tag.SetTitle(NameOfSong);
      else
        tag.SetTitle( CUtil::GetTitleFromPath(m_currentSong.m_strPath) );
    }
  } // if (tag.Loaded())
  else
  {
    // If we have a cdda track without cddb information,...
    if (m_currentSong.IsCDDA())
    {
      // we have the tracknumber...
      int iTrack = tag.GetTrackNumber();
      if (iTrack >= 1)
      {
        CStdString strText = g_localizeStrings.Get(435); // "Track"
        if (strText.GetAt(strText.size() - 1) != ' ')
          strText += " ";
        CStdString strTrack;
        strTrack.Format(strText + "%i", iTrack);
        tag.SetTitle(strTrack);
        tag.SetLoaded(true);
      }
    } // if (!tag.Loaded() && url.GetProtocol()=="cdda" )
    else
    { // at worse, set our title as the filename
      tag.SetTitle( CUtil::GetTitleFromPath(m_currentSong.m_strPath) );
    } // we now have at least the title
    tag.SetLoaded(true);
  }

  // find a thumb for this file.
  if (m_currentSong.IsInternetStream())
  {
    CLog::Log(LOGDEBUG,"Streaming media detected... using %s to find a thumb", g_application.m_strPlayListFile.c_str());
    CFileItem* pItemTemp = new CFileItem(g_application.m_strPlayListFile,false);
    pItemTemp->SetMusicThumb();
    CStdString strThumb = pItemTemp->GetThumbnailImage();
    if (CFile::Exists(strThumb))
      m_currentSong.SetThumbnailImage(strThumb);
  }
  else
    m_currentSong.SetMusicThumb();
  m_currentSong.FillInDefaultIcon();
}

void CGUIInfoManager::SetCurrentMovie(CFileItem &item)
{
  CLog::Log(LOGDEBUG,"CGUIInfoManager::SetCurrentMovie(%s)",item.m_strPath.c_str());

  CVideoDatabase dbs;
  dbs.Open();
  if (dbs.HasMovieInfo(item.m_strPath))
  {
    dbs.GetMovieInfo(item.m_strPath, m_currentMovie);
    CLog::Log(LOGDEBUG,"CGUIInfoManager:SetCurrentMovie(), got movie info!");
    CLog::Log(LOGDEBUG,"  Title = %s", m_currentMovie.m_strTitle.c_str());
    CLog::Log(LOGDEBUG,"  IMDB# = %s", m_currentMovie.m_strIMDBNumber.c_str());
  }
  dbs.Close();

  if (m_currentMovie.m_strTitle.IsEmpty())
  { // at least fill in the filename
    m_currentMovie.m_strTitle = CUtil::GetTitleFromPath(item.m_strPath);
  }
  if (m_currentMovie.m_strPath.IsEmpty())
  {
    m_currentMovie.m_strPath = item.m_strPath;
  }
  // Find a thumb for this file.
  item.SetThumb();
  if (!item.HasThumbnail())
  { // get IMDb thumb if we have one
    CStdString strThumb;
    CUtil::GetVideoThumbnail(m_currentMovie.m_strIMDBNumber, strThumb);
    if (CFile::Exists(strThumb))
      item.SetThumbnailImage(strThumb);
  }
  // find a thumb for this stream
  if (item.IsInternetStream())
  {
    // case where .strm is used to start an audio stream
    if (g_application.IsPlayingAudio())
    {
      SetCurrentSong(item);
      return;
    }

    // else its a video
    CLog::Log(LOGDEBUG,"Streaming media detected... using %s to find a thumb", g_application.m_strPlayListFile.c_str());
    CFileItem* pItemTemp = new CFileItem(g_application.m_strPlayListFile,false);
    pItemTemp->SetThumb();
    CStdString strThumb = pItemTemp->GetThumbnailImage();
    if (CFile::Exists(strThumb))
      item.SetThumbnailImage(strThumb);
  }

  item.FillInDefaultIcon();
  m_currentMovieThumb = item.GetThumbnailImage();
}

wstring CGUIInfoManager::GetSystemHeatInfo(const CStdString &strInfo)
{
  if (timeGetTime() - m_lastSysHeatInfoTime >= 1000)
  { // update our variables
    m_lastSysHeatInfoTime = timeGetTime();
    m_fanSpeed = CFanController::Instance()->GetFanSpeed();
    m_gpuTemp = CFanController::Instance()->GetGPUTemp();
    m_cpuTemp = CFanController::Instance()->GetCPUTemp();
  }
  if (strInfo == "cpu")
  {
    WCHAR CPUText[32];
    if (g_guiSettings.GetInt("Weather.TemperatureUnits") == 1 /*DEGREES_F*/)
      swprintf(CPUText, L"%s %2.2f%cF", g_localizeStrings.Get(140).c_str(), ((9.0 / 5.0) * m_cpuTemp) + 32.0, 176);
    else
      swprintf(CPUText, L"%s %2.2f%cC", g_localizeStrings.Get(140).c_str(), m_cpuTemp, 176);
    return CPUText;
  }
  else if (strInfo == "gpu")
  {
    WCHAR GPUText[32];
    if (g_guiSettings.GetInt("Weather.TemperatureUnits") == 1 /*DEGREES_F*/)
      swprintf(GPUText, L"%s %2.2f%cF", g_localizeStrings.Get(141).c_str(), ((9.0 / 5.0) * m_gpuTemp) + 32.0, 176);
    else
      swprintf(GPUText, L"%s %2.2f%cC", g_localizeStrings.Get(141).c_str(), m_gpuTemp, 176);
    return GPUText;
  }
  else if (strInfo == "fan")
  {
    WCHAR FanText[32];
    swprintf(FanText, L"%s: %i%%", g_localizeStrings.Get(13300).c_str(), m_fanSpeed * 2);
    return FanText;
  }
  return L"";
}

wstring CGUIInfoManager::GetFreeSpace(int drive)
{
  ULARGE_INTEGER lTotalFreeBytes;
  WCHAR wszHD[64];
  wstring strReturn;

  char cDrive = drive - SYSTEM_FREE_SPACE_C + 'C';
  CStdString strDriveFind;
  strDriveFind.Format("%c:\\", cDrive);
  const WCHAR *pszDrive = g_localizeStrings.Get(155).c_str();
  const WCHAR *pszFree = g_localizeStrings.Get(160).c_str();
  const WCHAR *pszUnavailable = g_localizeStrings.Get(161).c_str();
  if (GetDiskFreeSpaceEx( strDriveFind.c_str(), NULL, NULL, &lTotalFreeBytes))
  {
    swprintf(wszHD, L"%s %c: %u Mb ", pszDrive, cDrive, lTotalFreeBytes.QuadPart / 1048576); //To make it MB
    wcscat(wszHD, pszFree);
  }
  else
  {
    swprintf(wszHD, L"%s %c: ", pszDrive, cDrive);
    wcscat(wszHD, pszUnavailable);
  }
  strReturn = wszHD;
  return strReturn;
}

CStdString CGUIInfoManager::GetVersion()
{
  CStdString tmp;
  tmp.Format("%s", VERSION_STRING);
  return tmp;
}

CStdString CGUIInfoManager::GetBuild()
{
  CStdString tmp;
  tmp.Format("%s", __DATE__);
  return tmp;
}

void CGUIInfoManager::SetDisplayAfterSeek(DWORD dwTimeOut)
{
  if(dwTimeOut>0)    
    m_AfterSeekTimeout = timeGetTime() +  dwTimeOut;
  else
    m_AfterSeekTimeout = 0;
}

bool CGUIInfoManager::GetDisplayAfterSeek() const
{
  return (timeGetTime() < m_AfterSeekTimeout);
}

CStdString CGUIInfoManager::GetAudioScrobblerLabel(int item)
{
  switch (item)
  {
  case AUDIOSCROBBLER_CONN_STATE:
    return CScrobbler::GetInstance()->GetConnectionState();
    break;
  case AUDIOSCROBBLER_SUBMIT_INT:
    return CScrobbler::GetInstance()->GetSubmitInterval();
    break;
  case AUDIOSCROBBLER_FILES_CACHED:
    return CScrobbler::GetInstance()->GetFilesCached();
    break;
  case AUDIOSCROBBLER_SUBMIT_STATE:
    return CScrobbler::GetInstance()->GetSubmitState();
    break;
  }

  return "";
}

#define OPERATOR_NOT  3
#define OPERATOR_AND  2
#define OPERATOR_OR   1

int CGUIInfoManager::GetOperator(const char ch)
{
  if (ch == '[')
    return 5;
  else if (ch == ']')
    return 4;
  else if (ch == '!')
    return OPERATOR_NOT;
  else if (ch == '+')
    return OPERATOR_AND;
  else if (ch == '|')
    return OPERATOR_OR;
  else
    return 0;
}

bool CGUIInfoManager::EvaluateBooleanExpression(const CCombinedValue &expression, bool &result, DWORD dwContextWindow) const
{
  // stack to save our bool state as we go
  stack<bool> save;

  for (list<int>::const_iterator it = expression.m_postfix.begin(); it != expression.m_postfix.end(); ++it)
  {
    int expr = *it;
    if (expr == -OPERATOR_NOT)
    { // NOT the top item on the stack
      if (save.size() < 1) return false;
      bool expr = save.top();
      save.pop();
      save.push(!expr);
    }
    else if (expr == -OPERATOR_AND)
    { // AND the top two items on the stack
      if (save.size() < 2) return false;
      bool right = save.top(); save.pop();
      bool left = save.top(); save.pop();
      save.push(left && right);
    }
    else if (expr == -OPERATOR_OR)
    { // OR the top two items on the stack
      if (save.size() < 2) return false;
      bool right = save.top(); save.pop();
      bool left = save.top(); save.pop();
      save.push(left || right);
    }
    else  // operator
      save.push(GetBool(expr, dwContextWindow));
  }
  if (save.size() != 1) return false;
  result = save.top();
  return true;
}

int CGUIInfoManager::TranslateBooleanExpression(const CStdString &expression)
{
  CCombinedValue comb;
  comb.m_info = expression;
  comb.m_id = COMBINED_VALUES_START + m_CombinedValues.size();

  // operator stack
  stack<char> save;

  CStdString operand;

  for (unsigned int i = 0; i < expression.size(); i++)
  {
    if (GetOperator(expression[i]))
    {
      // cleanup any operand, translate and put into our expression list
      if (!operand.IsEmpty())
      {
        int iOp = TranslateSingleString(operand);
        if (iOp)
          comb.m_postfix.push_back(iOp);
        operand.clear();
      }
      // handle closing parenthesis
      if (expression[i] == ']')
      {
        while (save.size())
        {
          char oper = save.top();
          save.pop();

          if (oper == '[')
            break;

          comb.m_postfix.push_back(-GetOperator(oper));
        }
      }
      else
      {
        // all other operators we pop off the stack any operator
        // that has a higher priority than the one we have.
        while (!save.empty() && GetOperator(save.top()) > GetOperator(expression[i]))
        {
          // only handle parenthesis once they're closed.
          if (save.top() == '[' && expression[i] != ']')
            break;

          comb.m_postfix.push_back(-GetOperator(save.top()));  // negative denotes operator
          save.pop();
        }
        save.push(expression[i]);
      }
    }
    else
    {
      operand += expression[i];
    }
  }

  if (!operand.empty())
    comb.m_postfix.push_back(TranslateSingleString(operand));

  // finish up by adding any operators
  while (!save.empty())
  {
    comb.m_postfix.push_back(-GetOperator(save.top()));
    save.pop();
  }

  // test evaluate
  bool test;
  if (!EvaluateBooleanExpression(comb, test, WINDOW_INVALID))
    CLog::Log(LOGERROR, "Error evaluating boolean expression %s", expression.c_str());
  // success - add to our combined values
  m_CombinedValues.push_back(comb);
  return comb.m_id;
}

void CGUIInfoManager::Clear()
{
  m_currentSong.Reset();
  m_currentMovie.Reset();
  m_CombinedValues.clear();
}

void CGUIInfoManager::UpdateFPS()
{
  m_frameCounter++;
  float fTimeSpan = (float)(timeGetTime() - m_lastFPSTime);
  if (fTimeSpan >= 1000.0f)
  {
    fTimeSpan /= 1000.0f;
    m_fps = m_frameCounter / fTimeSpan;
    m_lastFPSTime = timeGetTime();
    m_frameCounter = 0;
  }
}

int CGUIInfoManager::AddMultiInfo(const GUIInfo &info)
{
  // check to see if we have this info already
  for (unsigned int i = 0; i < m_multiInfo.size(); i++)
    if (m_multiInfo[i] == info)
      return (int)i;
  // return the new offset
  m_multiInfo.push_back(info);
  return (int)m_multiInfo.size() + MULTI_INFO_START - 1;
}

int CGUIInfoManager::ConditionalStringParameter(const CStdString &parameter)
{
  // check to see if we have this parameter already
  for (unsigned int i = 0; i < m_stringParameters.size(); i++)
    if (parameter.Equals(m_stringParameters[i]))
      return (int)i;
  // return the new offset
  m_stringParameters.push_back(parameter);
  return (int)m_stringParameters.size() - 1;
}

CStdString CGUIInfoManager::GetItemLabel(const CFileItem *item, int info)
{
  if (!item) return "";
  if (info == LISTITEM_LABEL) return item->GetLabel();
  else if (info == LISTITEM_TITLE) return item->m_musicInfoTag.GetTitle();
  else if (info == LISTITEM_TRACKNUMBER)
  {
    CStdString track;
    track.Format("%i", item->m_musicInfoTag.GetTrackNumber());
    return track;
  }
  else if (info == LISTITEM_ARTIST) return item->m_musicInfoTag.GetArtist();
  else if (info == LISTITEM_ALBUM) return item->m_musicInfoTag.GetAlbum();
  else if (info == LISTITEM_YEAR) return item->m_musicInfoTag.GetYear();
  else if (info == LISTITEM_GENRE) return item->m_musicInfoTag.GetGenre();
  return "";
}


CStdString CGUIInfoManager::ParseLabel(const CStdString& strLabel)
{
  CStdString strReturn = "";
  int iPos1 = 0;
  int iPos2 = strLabel.Find('$', iPos1);
  bool bDoneSomething = !(iPos1 == iPos2);
  while (iPos2 >= 0)
  {
    if( (iPos2 > iPos1) && bDoneSomething )
    {
      strReturn += strLabel.Mid(iPos1, iPos2 - iPos1);
      bDoneSomething = false;  
    }

    int iPos3 = 0;
    CStdString str;
    CStdString strInfo = "INFO[";
    CStdString strLocalize = "LOCALIZE[";

    // $INFO[something]
    if (strLabel.Mid(iPos2 + 1,strInfo.size()).Equals(strInfo))
    {
      iPos2 += strInfo.size() + 1;
      iPos3 = strLabel.Find(']', iPos2);
      CStdString strValue = strLabel.Mid(iPos2,(iPos3 - iPos2));
      int iInfo = g_infoManager.TranslateString(strValue);
      if (iInfo)
      {
        str = g_infoManager.GetLabel(iInfo);
        if (str.size() > 0)
          bDoneSomething = true;
      }
    }

    // $LOCALIZE[something]
    else if (strLabel.Mid(iPos2 + 1,strLocalize.size()).Equals(strLocalize))
    {
      iPos2 += strLocalize.size() + 1;
      iPos3 = strLabel.Find(']', iPos2);
      CStdString strValue = strLabel.Mid(iPos2,(iPos3 - iPos2));
      if (CUtil::IsNaturalNumber(strValue))
      {
        int iLocalize = atoi(strValue);
        str = g_localizeStrings.Get(iLocalize);
        if (str.size() > 0)
          bDoneSomething = true;
      }
    }

    // $$ prints $
    else if (strLabel[iPos2 + 1] == '$')
    { 
      iPos3 = iPos2 + 1;
      str = '$';
      bDoneSomething = true;
    }

    //Okey, nothing found, just print it right out
    else
    {
      iPos3 = iPos2;
      str = '$';
      bDoneSomething = true;
    }

    strReturn += str;
    iPos1 = iPos3 + 1;
    iPos2 = strLabel.Find('$', iPos1);
  }

  if (iPos1 < (int)strLabel.size())
    strReturn += strLabel.Right(strLabel.size() - iPos1);

  return strReturn;
}
