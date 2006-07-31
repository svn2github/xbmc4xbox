#include "stdafx.h"
#include "GUISettings.h"
#include "application.h"
#include "util.h"
#include "GUIWindowMusicBase.h"
#include "utils/FanController.h"
#include "XBAudioConfig.h"
#include "XBVideoConfig.h"
#include <xfont.h>
#include "GUIDialogFileBrowser.h"

// String id's of the masks
#define MASK_MINS   14044
#define MASK_SECS   14045
#define MASK_MS    14046
#define MASK_PERCENT 14047
#define MASK_KBPS   14048
#define MASK_KB    14049
#define MASK_DB    14050

#define TEXT_OFF 351

class CGUISettings g_guiSettings;

struct sortsettings
{
  bool operator()(const CSetting* pSetting1, const CSetting* pSetting2)
  {
    return pSetting1->GetOrder() < pSetting2->GetOrder();
  }
};

void CSettingBool::FromString(const CStdString &strValue)
{
  m_bData = (strValue == "true");
}

CStdString CSettingBool::ToString()
{
  return m_bData ? "true" : "false";
}

CSettingSeparator::CSettingSeparator(int iOrder, const char *strSetting)
    : CSetting(iOrder, strSetting, 0, SEPARATOR_CONTROL)
{
}

CSettingFloat::CSettingFloat(int iOrder, const char *strSetting, int iLabel, float fData, float fMin, float fStep, float fMax, int iControlType)
    : CSetting(iOrder, strSetting, iLabel, iControlType)
{
  m_fData = fData;
  m_fMin = fMin;
  m_fStep = fStep;
  m_fMax = fMax;
}

void CSettingFloat::FromString(const CStdString &strValue)
{
  SetData((float)atof(strValue.c_str()));
}

CStdString CSettingFloat::ToString()
{
  CStdString strValue;
  strValue.Format("%f", m_fData);
  return strValue;
}

CSettingInt::CSettingInt(int iOrder, const char *strSetting, int iLabel, int iData, int iMin, int iStep, int iMax, int iControlType, const char *strFormat)
    : CSetting(iOrder, strSetting, iLabel, iControlType)
{
  m_iData = iData;
  m_iMin = iMin;
  m_iMax = iMax;
  m_iStep = iStep;
  m_iFormat = -1;
  m_iLabelMin = -1;
  if (strFormat)
    m_strFormat = strFormat;
  else
    m_strFormat = "%i";
}

CSettingInt::CSettingInt(int iOrder, const char *strSetting, int iLabel, int iData, int iMin, int iStep, int iMax, int iControlType, int iFormat, int iLabelMin)
    : CSetting(iOrder, strSetting, iLabel, iControlType)
{
  m_iData = iData;
  m_iMin = iMin;
  m_iMax = iMax;
  m_iStep = iStep;
  m_iLabelMin = iLabelMin;
  if (iFormat > -1)
    m_iFormat = iFormat;
  else
    m_strFormat = "%i";
}

void CSettingInt::FromString(const CStdString &strValue)
{
  SetData(atoi(strValue.c_str()));
}

CStdString CSettingInt::ToString()
{
  CStdString strValue;
  strValue.Format("%i", m_iData);
  return strValue;
}

void CSettingHex::FromString(const CStdString &strValue)
{
  int iHexValue;
  if (sscanf(strValue, "%x", &iHexValue))
    SetData(iHexValue);
}

CStdString CSettingHex::ToString()
{
  CStdString strValue;
  strValue.Format("%x", m_iData);
  return strValue;
}

CSettingString::CSettingString(int iOrder, const char *strSetting, int iLabel, const char *strData, int iControlType, bool bAllowEmpty, int iHeadingString)
    : CSetting(iOrder, strSetting, iLabel, iControlType)
{
  m_strData = strData;
  m_bAllowEmpty = bAllowEmpty;
  m_iHeadingString = iHeadingString;
}

void CSettingString::FromString(const CStdString &strValue)
{
  m_strData = strValue;
}

CStdString CSettingString::ToString()
{
  return m_strData;
}

void CSettingsGroup::GetCategories(vecSettingsCategory &vecCategories)
{
  vecCategories.clear();
  for (unsigned int i = 0; i < m_vecCategories.size(); i++)
  {
    vecSettings settings;
    // check whether we actually have these settings available.
    g_guiSettings.GetSettingsGroup(m_vecCategories[i]->m_strCategory, settings);
    if (settings.size())
      vecCategories.push_back(m_vecCategories[i]);
  }
}

// Settings are case sensitive
CGUISettings::CGUISettings(void)
{
  ZeroMemory(&m_replayGain, sizeof(ReplayGainSettings));

  // Pictures settings
  AddGroup(0, 1);
  AddCategory(0, "pictures", 16000);
  AddBool(1, "pictures.useautoswitching", 14011, false);
  AddBool(2, "pictures.autoswitchuselargethumbs", 14012, false);
  AddCategory(0, "slideshow", 108);
  AddInt(1, "slideshow.staytime", 12378, 9, 1, 1, 100, SPIN_CONTROL_INT_PLUS, MASK_SECS);
  AddInt(2, "slideshow.transistiontime", 225, 2500, 100, 100, 10000, SPIN_CONTROL_INT_PLUS, MASK_MS);
  AddBool(3, "slideshow.displayeffects", 12379, true);
  AddSeparator(6, "slideshow.sep1");
  AddBool(0, "slideshow.shuffle", 13319, false);

  // Programs settings
  AddGroup(1, 0);
  AddCategory(1, "myprograms", 16000);
  AddBool(1, "myprograms.gameautoregion",511,false);
  AddInt(2, "myprograms.ntscmode", 16110, 0, 0, 1, 3, SPIN_CONTROL_TEXT);
  AddString(3, "myprograms.dashboard", 13006, "C:\\xboxdash.xbe", BUTTON_CONTROL_PATH_INPUT, false, 13006);
  //  TODO: localize 2.0
  AddString(4, "myprograms.trainerpath", 20003, "select folder", BUTTON_CONTROL_PATH_INPUT, false, 20003);

  AddCategory(1,"programfiles",744);
  AddBool(1, "programfiles.useautoswitching", 14011, false);
  AddBool(2, "programfiles.autoswitchuselargethumbs", 14012, false);

  AddCategory(1, "xlinkkai", 714);
  AddBool(1, "xlinkkai.enabled", 14072, false);
  AddBool(2, "xlinkkai.enablenotifications", 14008, true);
  AddString(4, "xlinkkai.username", 709, "", BUTTON_CONTROL_INPUT, false, 709);
  AddString(5, "xlinkkai.password", 710, "", BUTTON_CONTROL_HIDDEN_INPUT, false, 710);
  AddString(6, "xlinkkai.server", 14042, "", BUTTON_CONTROL_IP_INPUT);

  // My Weather settings
  AddGroup(2, 8);
  AddCategory(2, "weather", 16000);
  AddString(1, "weather.areacode1", 14019, "GMXX0154 - Aachen, Germany", BUTTON_CONTROL_STANDARD);
  AddString(2, "weather.areacode2", 14020, "UKXX0085 - London, United Kingdom", BUTTON_CONTROL_STANDARD);
  AddString(3, "weather.areacode3", 14021, "CAXX0343 - Ontario, Canada", BUTTON_CONTROL_STANDARD);

  // My Music Settings
  AddGroup(3, 2);
  AddCategory(3, "mymusic", 16000);
  AddString(1, "mymusic.visualisation", 250, "milkdrop.vis", SPIN_CONTROL_TEXT);
  AddSeparator(2, "mymusic.sep1");
  AddString(4, "mymusic.trackformat", 13307, "%N. %A - %T", BUTTON_CONTROL_INPUT, false, 16016);
  AddString(5, "mymusic.trackformatright", 13387, "%D", BUTTON_CONTROL_INPUT, false, 16016);
  AddSeparator(6, "mymusic.sep2");
  AddBool(7, "mymusic.uselastfm", 15201, false);
  AddBool(8, "mymusic.lastfmrecordtoprofile", 15250, false);
  AddString(9,"mymusic.lastfmusername", 15202, "", BUTTON_CONTROL_INPUT, false, 15202);
  AddString(10,"mymusic.lastfmpassword", 15203, "", BUTTON_CONTROL_HIDDEN_INPUT, false, 15203); 
  AddSeparator(11, "mymusic.sep3");
  AddString(12, "mymusic.cleanupmusiclibrary", 334, "", BUTTON_CONTROL_STANDARD);
  AddBool(13,"mymusic.hideallitems",20127,false);
  // advanced per-view trackformats.
  AddString(0, "mymusic.nowplayingtrackformat", 13307, "", BUTTON_CONTROL_INPUT, false, 16016);
  AddString(0, "mymusic.nowplayingtrackformatright", 13387, "", BUTTON_CONTROL_INPUT, false, 16016);
  AddString(0, "mymusic.librarytrackformat", 13307, "", BUTTON_CONTROL_INPUT, false, 16016);
  AddString(0, "mymusic.librarytrackformatright", 13387, "", BUTTON_CONTROL_INPUT, false, 16016);

  AddCategory(3, "musicfiles", 744);
  AddBool(1, "musicfiles.autoplaynextitem", 489, true);
  AddBool(2, "musicfiles.repeat", 488, false);
  AddSeparator(3, "musicfiles.sep1");
  AddBool(4, "musicfiles.usetags", 258, true);
  AddBool(5, "musicfiles.usecddb", 227, true);
  AddBool(6, "musicfiles.findremotethumbs", 14059, true);
  AddSeparator(7, "musicfiles.sep2");
  AddBool(8, "musicfiles.useautoswitching", 14011, false);
  AddBool(9, "musicfiles.autoswitchuselargethumbs", 14012, false);

  AddCategory(3, "musicplaylist", 136);
  AddBool(1, "musicplaylist.clearplaylistsonend",239,false);

  AddCategory(3, "cddaripper", 620);
  AddString(1, "cddaripper.trackformat", 13307, "%N. %T - %A", BUTTON_CONTROL_INPUT, false, 16016);
  AddInt(2, "cddaripper.encoder", 621, CDDARIP_ENCODER_LAME, CDDARIP_ENCODER_LAME, 1, CDDARIP_ENCODER_WAV, SPIN_CONTROL_TEXT);
  AddInt(3, "cddaripper.quality", 622, CDDARIP_QUALITY_CBR, CDDARIP_QUALITY_CBR, 1, CDDARIP_QUALITY_EXTREME, SPIN_CONTROL_TEXT);
  AddInt(4, "cddaripper.bitrate", 623, 192, 128, 32, 320, SPIN_CONTROL_INT_PLUS, MASK_KBPS);
  //  TODO: localize 2.0
  AddString(5, "cddaripper.path", 20000, "select writable folder", BUTTON_CONTROL_PATH_INPUT, false);

  AddCategory(3, "musicplayer", 16003);
  AddString(1, "musicplayer.jumptoaudiohardware", 16001, "", BUTTON_CONTROL_STANDARD);
  AddBool(2, "musicplayer.outputtoallspeakers", 252, false);
  AddSeparator(3, "musicplayer.sep1");
  AddInt(4, "musicplayer.replaygaintype", 638, REPLAY_GAIN_ALBUM, REPLAY_GAIN_NONE, 1, REPLAY_GAIN_TRACK, SPIN_CONTROL_TEXT);
  AddInt(5, "musicplayer.replaygainpreamp", 641, 89, 77, 1, 101, SPIN_CONTROL_INT_PLUS, MASK_DB);
  AddInt(6, "musicplayer.replaygainnogainpreamp", 642, 89, 77, 1, 101, SPIN_CONTROL_INT_PLUS, MASK_DB);
  AddBool(7, "musicplayer.replaygainavoidclipping", 643, false);
  AddSeparator(8, "musicplayer.sep2");
  AddInt(9, "musicplayer.crossfade", 13314, 0, 0, 1, 10, SPIN_CONTROL_INT_PLUS, MASK_SECS, TEXT_OFF);
  AddBool(10, "musicplayer.crossfadealbumtracks", 13400, true);
  
  AddCategory(3, "karaoke", 13327);
  AddBool(1, "karaoke.enabled", 13323, false);
  AddBool(2, "karaoke.voiceenabled", 13361, false);
  AddInt(3, "karaoke.volume", 13376, 100, 0, 1, 100, SPIN_CONTROL_INT, MASK_PERCENT);
  AddString(4, "karaoke.port0voicemask", 13382, "None", SPIN_CONTROL_TEXT);
  AddString(5, "karaoke.port1voicemask", 13383, "None", SPIN_CONTROL_TEXT);
  AddString(6, "karaoke.port2voicemask", 13384, "None", SPIN_CONTROL_TEXT);
  AddString(7, "karaoke.port3voicemask", 13385, "None", SPIN_CONTROL_TEXT);

  // System settings
  AddGroup(4, 13000);
  AddCategory(4, "system", 13000);
  AddInt(1, "system.hdspindowntime", 229, 0, 0, 1, 60, SPIN_CONTROL_INT_PLUS, MASK_MINS, TEXT_OFF); // Minutes
  AddInt(2, "system.remoteplayhdspindown", 13001, 0, 0, 1, 3, SPIN_CONTROL_TEXT); // off, music, video, both
  AddInt(3, "system.remoteplayhdspindownminduration", 13004, 20, 0, 1, 20, SPIN_CONTROL_INT_PLUS, MASK_MINS); // Minutes
  AddInt(4, "system.remoteplayhdspindowndelay", 13003, 20, 5, 5, 300, SPIN_CONTROL_INT_PLUS, MASK_SECS); // seconds
  AddSeparator(5, "system.sep1");
  AddInt(6, "system.shutdowntime", 357, 0, 0, 5, 120, SPIN_CONTROL_INT_PLUS, MASK_MINS, TEXT_OFF);
  AddBool(7, "system.shutdownwhileplaying", 14043, false);
  AddSeparator(8, "system.sep2");
  AddBool(9, "system.fanspeedcontrol", 13302, false);
  AddInt(10, "system.fanspeed", 13300, CFanController::Instance()->GetFanSpeed(), 5, 1, 50, SPIN_CONTROL_TEXT);
  AddSeparator(11, "system.sep3");
  AddBool(12, "system.autotemperature", 13301, false);
  AddInt(13, "system.targettemperature", 13299, 55, 40, 1, 68, SPIN_CONTROL_TEXT);

  AddCategory(4, "autorun", 447);
  AddBool(1, "autorun.dvd", 240, true);
  AddBool(2, "autorun.vcd", 241, true);
  AddBool(3, "autorun.cdda", 242, true);
  AddBool(4, "autorun.xbox", 243, true);
  AddBool(5, "autorun.video", 244, true);
  AddBool(6, "autorun.music", 245, true);
  AddBool(7, "autorun.pictures", 246, true);

  AddCategory(4, "cache", 439);
  AddInt(1, "cache.harddisk", 14025, 256, 0, 256, 4096, SPIN_CONTROL_INT_PLUS, MASK_KB);
  AddSeparator(2, "cache.sep1");
  AddInt(3, "cachevideo.dvdrom", 14026, 2048, 0, 256, 16384, SPIN_CONTROL_INT_PLUS, MASK_KB);
  AddInt(4, "cachevideo.lan", 14027, 2048, 0, 256, 16384, SPIN_CONTROL_INT_PLUS, MASK_KB);
  AddInt(5, "cachevideo.internet", 14028, 4096, 0, 256, 16384, SPIN_CONTROL_INT_PLUS, MASK_KB);
  AddSeparator(6, "cache.sep2");
  AddInt(7, "cacheaudio.dvdrom", 14030, 256, 0, 256, 4096, SPIN_CONTROL_INT_PLUS, MASK_KB);
  AddInt(8, "cacheaudio.lan", 14031, 256, 0, 256, 4096, SPIN_CONTROL_INT_PLUS, MASK_KB);
  AddInt(9, "cacheaudio.internet", 14032, 256, 0, 256, 4096, SPIN_CONTROL_INT_PLUS, MASK_KB);
  AddSeparator(10, "cache.sep3");
  AddInt(11, "cachedvd.dvdrom", 14034, 2048, 0, 256, 16384, SPIN_CONTROL_INT_PLUS, MASK_KB);
  AddInt(12, "cachedvd.lan", 14035, 2048, 0, 256, 16384, SPIN_CONTROL_INT_PLUS, MASK_KB);
  AddSeparator(13, "cache.sep4");
  AddInt(14, "cacheunknown.internet", 14060, 4096, 0, 256, 16384, SPIN_CONTROL_INT_PLUS, MASK_KB);

  AddCategory(4, "led", 13338);
  AddInt(1, "led.colour", 13339, LED_COLOUR_NO_CHANGE, LED_COLOUR_NO_CHANGE, 1, LED_COLOUR_OFF, SPIN_CONTROL_TEXT);
  AddInt(2, "led.disableonplayback", 13345, LED_PLAYBACK_OFF, LED_PLAYBACK_OFF, 1, LED_PLAYBACK_VIDEO_MUSIC, SPIN_CONTROL_TEXT);

  AddCategory(4, "lcd", 448);
  AddInt(2, "lcd.type", 4501, LCD_TYPE_NONE, LCD_TYPE_NONE, 1, LCD_TYPE_VFD, SPIN_CONTROL_TEXT);
  AddInt(3, "lcd.modchip", 471, MODCHIP_SMARTXX, MODCHIP_SMARTXX, 1, MODCHIP_XECUTER3, SPIN_CONTROL_TEXT);
  AddInt(4, "lcd.backlight", 463, 80, 0, 5, 100, SPIN_CONTROL_INT_PLUS, MASK_PERCENT);
  AddInt(5, "lcd.contrast", 465, 100, 0, 5, 100, SPIN_CONTROL_INT_PLUS, MASK_PERCENT);

  AddCategory(4, "audiooutput", 772);
  AddInt(3, "audiooutput.mode", 337, AUDIO_ANALOG, AUDIO_ANALOG, 1, AUDIO_DIGITAL, SPIN_CONTROL_TEXT);
  AddBool(4, "audiooutput.ac3passthrough", 364, true);
  AddBool(5, "audiooutput.dtspassthrough", 254, true);

  AddCategory(4, "masterlock", 12360);
  AddString(1, "masterlock.lockcode"       , 20100, "-", BUTTON_CONTROL_STANDARD);
  AddSeparator(2, "masterlock.sep1");
  AddBool(4, "masterlock.startuplock"      , 20076,false);
  AddBool(5, "masterlock.enableshutdown"   , 12362,false);
  AddBool(6, "masterlock.automastermode"   , 20101,false);
  AddSeparator(7,"masterlock.sep2" );
  AddBool(8, "masterlock.loginlock",20116,true);

  // hidden masterlock settings
  AddInt(0,"masterlock.maxretries"       , 12364, 3, 3, 1, 100, SPIN_CONTROL_TEXT); 
  
  // video settings
  AddGroup(5, 3);
  AddCategory(5, "myvideos", 16000);
  AddString(1, "myvideos.calibrate", 214, "", BUTTON_CONTROL_STANDARD);
  AddSeparator(2, "myvideos.sep1");
  //  TODO: localize 2.0
  AddBool(7, "myvideos.useexternaldvdplayer", 20001, false);
  AddString(8, "myvideos.externaldvdplayer", 20002, "",  BUTTON_CONTROL_PATH_INPUT, true, 20002);

  AddCategory(5, "videofiles", 744);
  AddBool(1, "videofiles.useautoswitching", 14011, false);
  AddBool(2, "videofiles.autoswitchuselargethumbs", 14012, false);
  
  AddCategory(5, "videoplayer", 16003);
  AddString(1, "videoplayer.jumptoaudiohardware", 16001, "", BUTTON_CONTROL_STANDARD);
  AddSeparator(3, "videoplayer.sep1");
  AddInt(6, "videoplayer.rendermethod", 13354, RENDER_HQ_RGB_SHADER, RENDER_LQ_RGB_SHADER, 1, RENDER_HQ_RGB_SHADER, SPIN_CONTROL_TEXT);
  AddInt(8, "videoplayer.displayresolution", 169, (int)AUTORES, (int)HDTV_1080i, 1, (int)AUTORES, SPIN_CONTROL_TEXT);
  AddInt(9, "videoplayer.framerateconversions", 336, FRAME_RATE_LEAVE_AS_IS, FRAME_RATE_LEAVE_AS_IS, 1, FRAME_RATE_USE_PAL60, SPIN_CONTROL_TEXT);
  AddSeparator(10, "videoplayer.sep3");
  //  TODO: localize 2.0
  AddBool(12, "videoplayer.treatstackasfile", 20051, true);
  AddSeparator(13,"videoplayer.sep4");
  AddBool(14,"videoplayer.autoresume",12017, false);

  AddCategory(5, "subtitles", 287);
  AddString(1, "subtitles.font", 288, "Arial.ttf", SPIN_CONTROL_TEXT);
  AddInt(2, "subtitles.height", 289, 28, 16, 2, 74, SPIN_CONTROL_TEXT); // use text as there is a disk based lookup needed
  AddInt(3, "subtitles.style", 736, XFONT_BOLD, XFONT_NORMAL, 1, XFONT_BOLDITALICS, SPIN_CONTROL_TEXT);
  AddInt(4, "subtitles.color", 737, SUBTITLE_COLOR_START, SUBTITLE_COLOR_START, 1, SUBTITLE_COLOR_END, SPIN_CONTROL_TEXT);
  AddString(5, "subtitles.charset", 735, "DEFAULT", SPIN_CONTROL_TEXT);
  AddBool(6, "subtitles.flipbidicharset", 13304, false);
  AddSeparator(7, "subtitles.sep1");
  AddBool(9, "subtitles.searchrars", 13249, false);

  // JM: Don't add the category - makes them hidden in the GUI
  //AddCategory(5, "postprocessing", 14041);
  AddBool(2, "postprocessing.enable", 286, false);
  AddBool(3, "postprocessing.auto", 307, true); // only has effect if PostProcessing.Enable is on.
  AddBool(4, "postprocessing.verticaldeblocking", 308, false);
  AddInt(5, "postprocessing.verticaldeblocklevel", 308, 0, 0, 1, 100, SPIN_CONTROL_INT);
  AddBool(6, "postprocessing.horizontaldeblocking", 309, false);
  AddInt(7, "postprocessing.horizontaldeblocklevel", 309, 0, 0, 1, 100, SPIN_CONTROL_INT);
  AddBool(8, "postprocessing.autobrightnesscontrastlevels", 310, false);
  AddBool(9, "postprocessing.dering", 311, false);

  AddCategory(5, "filters", 230);
  AddInt(1, "filters.flicker", 13100, 1, 0, 1, 5, SPIN_CONTROL_INT);
  AddBool(2, "filters.soften", 215, false);
  //AddSeparator(3, "filters.sep1");
  //AddBool(7, "filters.useautosync", 15214, false);

  // network settings
  AddGroup(6, 705);
  AddCategory(6, "network", 705);
  AddInt(1, "network.assignment", 715, NETWORK_DASH, NETWORK_DASH, 1, NETWORK_STATIC, SPIN_CONTROL_TEXT);
  AddString(2, "network.ipaddress", 719, "0.0.0.0", BUTTON_CONTROL_IP_INPUT);
  AddString(3, "network.subnet", 720, "255.255.255.0", BUTTON_CONTROL_IP_INPUT);
  AddString(4, "network.gateway", 721, "0.0.0.0", BUTTON_CONTROL_IP_INPUT);
  AddString(5, "network.dns", 722, "0.0.0.0", BUTTON_CONTROL_IP_INPUT);
  AddSeparator(6, "network.sep1");
  AddBool(7, "network.usehttpproxy", 708, false);
  AddString(8, "network.httpproxyserver", 706, "", BUTTON_CONTROL_IP_INPUT);
  AddString(9, "network.httpproxyport", 707, "8080", BUTTON_CONTROL_INPUT, false, 707);
  AddSeparator(10, "network.sep2");
  AddBool(11, "network.enableinternet", 14054, true);
  AddSeparator(12, "network.sep3");
  AddBool(13,   "network.enablerssfeeds",13305,  true);

  //GeminiServer
  AddCategory(6, "servers", 14036);
  AddBool(1,  "servers.ftpserver",        167, true);
  AddString(2,"servers.ftpserveruser",    1245, "xbox", SPIN_CONTROL_TEXT);
  AddString(3,"servers.ftpserverpassword",1246, "xbox", BUTTON_CONTROL_HIDDEN_INPUT, true, 1246);
  AddBool(4,  "servers.ftpautofatx",      771, true);
  AddSeparator(5, "servers.sep1");
  AddBool(6,  "servers.webserver",        263, false);
  AddString(7,"servers.webserverport",    730, "80", BUTTON_CONTROL_INPUT, false, 730);
  AddString(8,"servers.webserverpassword",733, "", BUTTON_CONTROL_HIDDEN_INPUT, true, 733);

  AddCategory(6,"autodetect",           1250  );
  AddBool(1,    "autodetect.onoff",     1251, true);
  AddString(2,  "autodetect.nickname",  1252, "XBMC-NickName",BUTTON_CONTROL_INPUT, false, 1252);
  AddBool(4,    "autodetect.popupinfo", 1254, true);
  AddSeparator(5, "autodetect.sep1");
  AddBool(6,    "autodetect.senduserpw",1255, true);
  AddBool(7,    "autodetect.createlink",1253, false);
  
  AddCategory(6, "smb", 1200);
  AddString(1, "smb.username",    1203,   "", BUTTON_CONTROL_INPUT, true, 1203);
  AddString(2, "smb.password",    1204,   "", BUTTON_CONTROL_HIDDEN_INPUT, true, 1204);
  AddString(3, "smb.winsserver",  1207,   "",  BUTTON_CONTROL_IP_INPUT);
  AddString(4, "smb.workgroup",   1202,   "WORKGROUP", BUTTON_CONTROL_INPUT, false, 1202);

  //  TODO: localize 2.0
  AddCategory(6, "upnp", 20110);
  AddBool(1,    "upnp.autostart", 20111, true);

  // appearance settings
  AddGroup(7, 480);
  AddCategory(7,"lookandfeel", 14037);
  AddString(1, "lookandfeel.skin",166,"Project Mayhem III", SPIN_CONTROL_TEXT);
  AddString(2, "lookandfeel.skintheme",15111,"SKINDEFAULT", SPIN_CONTROL_TEXT);
  AddString(3, "lookandfeel.soundskin",15108,"SKINDEFAULT", SPIN_CONTROL_TEXT);
  AddString(5, "lookandfeel.font",13303,"Default", SPIN_CONTROL_TEXT);
  AddInt(6, "lookandfeel.skinzoom",20109, 0, -20, 2, 20, SPIN_CONTROL_INT, MASK_PERCENT);
  AddInt(7, "lookandfeel.startupwindow",512,1, WINDOW_HOME, 1, WINDOW_PYTHON_END, SPIN_CONTROL_TEXT);
  AddSeparator(8, "lookandfeel.sep1");
  AddString(9, "lookandfeel.language",248,"english", SPIN_CONTROL_TEXT);
  AddString(10, "lookandfeel.region", 20026, "", SPIN_CONTROL_TEXT);
  AddString(11, "lookandfeel.charset",735,"DEFAULT", SPIN_CONTROL_TEXT); // charset is set by the language file
  AddSeparator(12, "lookandfeel.sep2");
  AddInt(13, "lookandfeel.resolution",169,(int)AUTORES, (int)HDTV_1080i, 1, (int)AUTORES, SPIN_CONTROL_TEXT);  
  AddString(14, "lookandfeel.guicalibration",213,"", BUTTON_CONTROL_STANDARD);
  AddSeparator(15, "lookandfeel.sep3");
  
  AddCategory(7, "filelists", 14018);
  AddBool(1, "filelists.hideextensions", 497, false);
  AddBool(2, "filelists.hideparentdiritems", 13306, false);
  AddBool(3, "filelists.ignorethewhensorting", 13399, true);
  AddBool(4, "filelists.allowfiledeletion", 14071, false);
  AddBool(5, "filelists.unrollarchives",516, false);
  AddBool(6, "filelists.fulldirectoryhistory", 15106, true);

  AddCategory(7, "screensaver", 360);
  AddString(1, "screensaver.mode", 356, "Dim", SPIN_CONTROL_TEXT);
  AddString(2, "screensaver.preview", 1000, "", BUTTON_CONTROL_STANDARD);
  AddBool(3, "screensaver.usemusicvisinstead", 13392, true);
  AddInt(4, "screensaver.time", 355, 3, 1, 1, 60, SPIN_CONTROL_INT_PLUS, MASK_MINS);
  AddSeparator(5, "screensaver.sep1");
  AddInt(6, "screensaver.dimlevel", 362, 20, 10, 10, 80, SPIN_CONTROL_INT_PLUS, MASK_PERCENT);
  AddString(7, "screensaver.slideshowpath", 774, "F:\\Pictures\\", BUTTON_CONTROL_PATH_INPUT, false, 774); // GeminiServer: PictureSlideShow
  AddBool(8, "screensaver.slideshowshuffle", 13319, false);

  AddCategory(7, "uifilters", 14053);
  AddInt(1, "uifilters.flicker", 13100, 5, 0, 1, 5, SPIN_CONTROL_INT);
  AddBool(2, "uifilters.soften", 215, false);

  AddCategory(7, "xbdatetime", 14063);
  AddBool(1,   "xbdatetime.timeserver"       , 168  , false);
  AddString(2, "xbdatetime.timeaddress"      , 731  , "207.46.130.100", BUTTON_CONTROL_IP_INPUT);
  AddSeparator(3, "xbdatetime.sep1");
  AddString(4, "xbdatetime.time", 14065, "", BUTTON_CONTROL_MISC_INPUT);
  AddString(5, "xbdatetime.date", 14064, "", BUTTON_CONTROL_MISC_INPUT);

  //  TODO: localize 2.0
    AddString(0,"system.screenshotpath",20004,"select writable folder",BUTTON_CONTROL_PATH_INPUT,false);
    AddString(0,"mymusic.recordingpath",20005,"select writable folder",BUTTON_CONTROL_PATH_INPUT,false);
    AddString(0,"system.playlistspath",20006,"set default",BUTTON_CONTROL_PATH_INPUT,false);
  }

CGUISettings::~CGUISettings(void)
{}

void CGUISettings::AddGroup(DWORD dwGroupID, DWORD dwLabelID)
{
  CSettingsGroup *pGroup = new CSettingsGroup(dwGroupID, dwLabelID);
  if (pGroup)
    settingsGroups.push_back(pGroup);
}

void CGUISettings::AddCategory(DWORD dwGroupID, const char *strSetting, DWORD dwLabelID)
{
  for (unsigned int i = 0; i < settingsGroups.size(); i++)
  {
    if (settingsGroups[i]->GetGroupID() == dwGroupID)
      settingsGroups[i]->AddCategory(CStdString(strSetting).ToLower(), dwLabelID);
  }
}

CSettingsGroup *CGUISettings::GetGroup(DWORD dwGroupID)
{
  for (unsigned int i = 0; i < settingsGroups.size(); i++)
  {
    if (settingsGroups[i]->GetGroupID() == dwGroupID)
      return settingsGroups[i];
  }
  CLog::DebugLog("Error: Requested setting group (%i) was not found.  It must be case-sensitive", dwGroupID);
  return NULL;
}

void CGUISettings::AddSeparator(int iOrder, const char *strSetting)
{
  CSettingSeparator *pSetting = new CSettingSeparator(iOrder, CStdString(strSetting).ToLower());
  if (!pSetting) return;
  settingsMap.insert(pair<CStdString, CSetting*>(CStdString(strSetting).ToLower(), pSetting));
}

void CGUISettings::AddBool(int iOrder, const char *strSetting, int iLabel, bool bData, int iControlType)
{
  CSettingBool* pSetting = new CSettingBool(iOrder, CStdString(strSetting).ToLower(), iLabel, bData, iControlType);
  if (!pSetting) return ;
  settingsMap.insert(pair<CStdString, CSetting*>(CStdString(strSetting).ToLower(), pSetting));
}
bool CGUISettings::GetBool(const char *strSetting) const
{
  ASSERT(settingsMap.size());
  constMapIter it = settingsMap.find(CStdString(strSetting).ToLower());
  if (it != settingsMap.end())
  { // old category
    return ((CSettingBool*)(*it).second)->GetData();
  }
  // Assert here and write debug output
  ASSERT(false);
  CLog::DebugLog("Error: Requested setting (%s) was not found.  It must be case-sensitive", strSetting);
  return false;
}

void CGUISettings::SetBool(const char *strSetting, bool bSetting)
{
  ASSERT(settingsMap.size());
  mapIter it = settingsMap.find(CStdString(strSetting).ToLower());
  if (it != settingsMap.end())
  { // old category
    ((CSettingBool*)(*it).second)->SetData(bSetting);
    return ;
  }
  // Assert here and write debug output
  CLog::DebugLog("Error: Requested setting (%s) was not found.  It must be case-sensitive", strSetting);
}

void CGUISettings::ToggleBool(const char *strSetting)
{
  ASSERT(settingsMap.size());
  mapIter it = settingsMap.find(CStdString(strSetting).ToLower());
  if (it != settingsMap.end())
  { // old category
    ((CSettingBool*)(*it).second)->SetData(!((CSettingBool *)(*it).second)->GetData());
    return ;
  }
  // Assert here and write debug output
  CLog::DebugLog("Error: Requested setting (%s) was not found.  It must be case-sensitive", strSetting);
}

void CGUISettings::AddFloat(int iOrder, const char *strSetting, int iLabel, float fData, float fMin, float fStep, float fMax, int iControlType)
{
  CSettingFloat* pSetting = new CSettingFloat(iOrder, CStdString(strSetting).ToLower(), iLabel, fData, fMin, fStep, fMax, iControlType);
  if (!pSetting) return ;
  settingsMap.insert(pair<CStdString, CSetting*>(CStdString(strSetting).ToLower(), pSetting));
}

float CGUISettings::GetFloat(const char *strSetting) const
{
  ASSERT(settingsMap.size());
  constMapIter it = settingsMap.find(CStdString(strSetting).ToLower());
  if (it != settingsMap.end())
  {
    return ((CSettingFloat *)(*it).second)->GetData();
  }
  // Assert here and write debug output
  ASSERT(false);
  CLog::DebugLog("Error: Requested setting (%s) was not found.  It must be case-sensitive", strSetting);
  return 0.0f;
}

void CGUISettings::SetFloat(const char *strSetting, float fSetting)
{
  ASSERT(settingsMap.size());
  mapIter it = settingsMap.find(CStdString(strSetting).ToLower());
  if (it != settingsMap.end())
  {
    ((CSettingFloat *)(*it).second)->SetData(fSetting);
    return ;
  }
  // Assert here and write debug output
  ASSERT(false);
  CLog::DebugLog("Error: Requested setting (%s) was not found.  It must be case-sensitive", strSetting);
}

void CGUISettings::LoadMasterLock(TiXmlElement *pRootElement)
{
  std::map<CStdString,CSetting*>::iterator it = settingsMap.find("masterlock.enableshutdown");
  if (it != settingsMap.end())
    LoadFromXML(pRootElement, it);
  it = settingsMap.find("masterlock.maxretries");
  if (it != settingsMap.end())
    LoadFromXML(pRootElement, it);
  it = settingsMap.find("masterlock.automastermode");
  if (it != settingsMap.end())
    LoadFromXML(pRootElement, it);
  it = settingsMap.find("masterlock.startuplock");
  if (it != settingsMap.end())
    LoadFromXML(pRootElement, it);
    it = settingsMap.find("autodetect.nickname");
  if (it != settingsMap.end())
    LoadFromXML(pRootElement, it);
}


void CGUISettings::AddInt(int iOrder, const char *strSetting, int iLabel, int iData, int iMin, int iStep, int iMax, int iControlType, const char *strFormat)
{
  CSettingInt* pSetting = new CSettingInt(iOrder, CStdString(strSetting).ToLower(), iLabel, iData, iMin, iStep, iMax, iControlType, strFormat);
  if (!pSetting) return ;
  settingsMap.insert(pair<CStdString, CSetting*>(CStdString(strSetting).ToLower(), pSetting));
}

void CGUISettings::AddInt(int iOrder, const char *strSetting, int iLabel, int iData, int iMin, int iStep, int iMax, int iControlType, int iFormat, int iLabelMin/*=-1*/)
{
  CSettingInt* pSetting = new CSettingInt(iOrder, CStdString(strSetting).ToLower(), iLabel, iData, iMin, iStep, iMax, iControlType, iFormat, iLabelMin);
  if (!pSetting) return ;
  settingsMap.insert(pair<CStdString, CSetting*>(CStdString(strSetting).ToLower(), pSetting));
}

void CGUISettings::AddHex(int iOrder, const char *strSetting, int iLabel, int iData, int iMin, int iStep, int iMax, int iControlType, const char *strFormat)
{
  CSettingHex* pSetting = new CSettingHex(iOrder, CStdString(strSetting).ToLower(), iLabel, iData, iMin, iStep, iMax, iControlType, strFormat);
  if (!pSetting) return ;
  settingsMap.insert(pair<CStdString, CSetting*>(CStdString(strSetting).ToLower(), pSetting));
}

int CGUISettings::GetInt(const char *strSetting) const
{
  ASSERT(settingsMap.size());
  constMapIter it = settingsMap.find(CStdString(strSetting).ToLower());
  if (it != settingsMap.end())
  {
    return ((CSettingInt *)(*it).second)->GetData();
  }
  // Assert here and write debug output
  ASSERT(false);
  CLog::DebugLog("Error: Requested setting (%s) was not found.  It must be case-sensitive", strSetting);
  return 0;
}

void CGUISettings::SetInt(const char *strSetting, int iSetting)
{
  ASSERT(settingsMap.size());
  mapIter it = settingsMap.find(CStdString(strSetting).ToLower());
  if (it != settingsMap.end())
  {
    ((CSettingInt *)(*it).second)->SetData(iSetting);
    if (stricmp(strSetting, "lookandfeel.resolution") == 0)
      g_guiSettings.m_LookAndFeelResolution = (RESOLUTION)iSetting;
    return ;
  }
  // Assert here and write debug output
  ASSERT(false);
}

void CGUISettings::AddString(int iOrder, const char *strSetting, int iLabel, const char *strData, int iControlType, bool bAllowEmpty, int iHeadingString)
{
  CSettingString* pSetting = new CSettingString(iOrder, CStdString(strSetting).ToLower(), iLabel, strData, iControlType, bAllowEmpty, iHeadingString);
  if (!pSetting) return ;
  settingsMap.insert(pair<CStdString, CSetting*>(CStdString(strSetting).ToLower(), pSetting));
}

const CStdString &CGUISettings::GetString(const char *strSetting, bool bPrompt) const
{
  ASSERT(settingsMap.size());
  constMapIter it = settingsMap.find(CStdString(strSetting).ToLower());
  if (it != settingsMap.end())
  {
    CSettingString* result = ((CSettingString *)(*it).second);
    if (result->GetData() == "select folder")
    {
      CStdString strData = "";
      if (bPrompt)
      {
        if (CGUIDialogFileBrowser::ShowAndGetDirectory(g_settings.m_vecMyFilesShares,g_localizeStrings.Get(result->GetLabel()),strData,false))
        {
          result->SetData(strData);
          g_settings.Save();
        }
        else 
          return StringUtils::EmptyString;
      }
      else
        return StringUtils::EmptyString;
    }
    if (result->GetData() == "select writable folder")
    {
      CStdString strData = "";
      if (bPrompt)
      {
        if (CGUIDialogFileBrowser::ShowAndGetDirectory(g_settings.m_vecMyFilesShares,g_localizeStrings.Get(result->GetLabel()),strData,true))
        {
          result->SetData(strData);
          g_settings.Save();
        }
        else
          return StringUtils::EmptyString;
      }
      else
        return StringUtils::EmptyString;
    }
    return result->GetData();
  }
  // Assert here and write debug output
  CLog::DebugLog("Error: Requested setting (%s) was not found.  It must be case-sensitive", strSetting);
  ASSERT(false);
  // hardcoded return value so that compiler is happy
  return ((CSettingString *)(*settingsMap.begin()).second)->GetData();
}

void CGUISettings::SetString(const char *strSetting, const char *strData)
{
  ASSERT(settingsMap.size());
  mapIter it = settingsMap.find(CStdString(strSetting).ToLower());
  if (it != settingsMap.end())
  {
    ((CSettingString *)(*it).second)->SetData(strData);
    return ;
  }
  // Assert here and write debug output
  ASSERT(false);
  CLog::DebugLog("Error: Requested setting (%s) was not found.  It must be case-sensitive", strSetting);
}

CSetting *CGUISettings::GetSetting(const char *strSetting)
{
  ASSERT(settingsMap.size());
  mapIter it = settingsMap.find(CStdString(strSetting).ToLower());
  if (it != settingsMap.end())
    return (*it).second;
  else
    return NULL;
}

// get all the settings beginning with the term "strGroup"
void CGUISettings::GetSettingsGroup(const char *strGroup, vecSettings &settings)
{
  settings.clear();
  for (mapIter it = settingsMap.begin(); it != settingsMap.end(); it++)
  {
    if ((*it).first.Left(strlen(strGroup)).Equals(strGroup) && (*it).second->GetOrder() > 0)
      settings.push_back((*it).second);
  }
  // now order them...
  sort(settings.begin(), settings.end(), sortsettings());
}

void CGUISettings::LoadXML(TiXmlElement *pRootElement, bool hideSettings /* = false */)
{ // load our stuff...
  for (mapIter it = settingsMap.begin(); it != settingsMap.end(); it++)
  {
    LoadFromXML(pRootElement, it, hideSettings);
  }
  // Get hardware based stuff...
  CLog::Log(LOGNOTICE, "Getting hardware information now...");
  if (GetInt("audiooutput.mode") == AUDIO_DIGITAL && !g_audioConfig.HasDigitalOutput())
    SetInt("audiooutput.mode", AUDIO_ANALOG);
  SetBool("audiooutput.ac3passthrough", g_audioConfig.GetAC3Enabled());
  SetBool("audiooutput.dtspassthrough", g_audioConfig.GetDTSEnabled());
  CLog::Log(LOGINFO, "Using %s output", GetInt("audiooutput.mode") == AUDIO_ANALOG ? "analog" : "digital");
  CLog::Log(LOGINFO, "AC3 pass through is %s", GetBool("audiooutput.ac3passthrough") ? "enabled" : "disabled");
  CLog::Log(LOGINFO, "DTS pass through is %s", GetBool("audiooutput.dtspassthrough") ? "enabled" : "disabled");
  g_guiSettings.m_LookAndFeelResolution = (RESOLUTION)GetInt("lookandfeel.resolution");
  CLog::Log(LOGNOTICE, "Checking resolution %i", g_guiSettings.m_LookAndFeelResolution);
  g_videoConfig.PrintInfo();
  if (
    (g_guiSettings.m_LookAndFeelResolution == AUTORES) ||
    (!g_graphicsContext.IsValidResolution(g_guiSettings.m_LookAndFeelResolution))
  )
  {
    RESOLUTION newRes = g_videoConfig.GetBestMode();
    if (g_guiSettings.m_LookAndFeelResolution == AUTORES)
    {
      //"lookandfeel.resolution" will stay at AUTORES, m_LookAndFeelResolution will be the real mode
      CLog::Log(LOGNOTICE, "Setting autoresolution mode %i", newRes);
      g_guiSettings.m_LookAndFeelResolution = newRes;
    }
    else
    {
      CLog::Log(LOGNOTICE, "Setting safe mode %i", newRes);
      SetInt("lookandfeel.resolution", newRes);
    }
  }
  // Move replaygain settings into our struct
  m_replayGain.iPreAmp = GetInt("musicplayer.replaygainpreamp");
  m_replayGain.iNoGainPreAmp = GetInt("musicplayer.replaygainnogainpreamp");
  m_replayGain.iType = GetInt("musicplayer.replaygaintype");
  m_replayGain.bAvoidClipping = GetBool("musicplayer.replaygainavoidclipping");
}

void CGUISettings::LoadFromXML(TiXmlElement *pRootElement, mapIter &it, bool hideSetting /* = false */)
{
  CStdStringArray strSplit;
  StringUtils::SplitString((*it).first, ".", strSplit);
  if (strSplit.size() > 1)
  {
    const TiXmlNode *pChild = pRootElement->FirstChild(strSplit[0].c_str());
    if (pChild)
    {
      const TiXmlNode *pGrandChild = pChild->FirstChild(strSplit[1].c_str());
      if (pGrandChild && pGrandChild->FirstChild())
      {
        CStdString strValue = pGrandChild->FirstChild()->Value();
        if (strValue.size() )
        {
          if (strValue != "-")
          { // update our item
            (*it).second->FromString(strValue);
            if (hideSetting)
              (*it).second->SetHidden();
            CLog::Log(LOGDEBUG, "  %s: %s", (*it).first.c_str(), (*it).second->ToString().c_str());
          }
        }
      }
    }
  }
}

void CGUISettings::SaveXML(TiXmlNode *pRootNode)
{
  for (mapIter it = settingsMap.begin(); it != settingsMap.end(); it++)
  {
    CStdStringArray strSplit;
    StringUtils::SplitString((*it).first, ".", strSplit);
    if (strSplit.size() > 1)
    {
      TiXmlNode *pChild = pRootNode->FirstChild(strSplit[0].c_str());
      if (!pChild)
      { // add our group tag
        TiXmlElement newElement(strSplit[0].c_str());
        pChild = pRootNode->InsertEndChild(newElement);
      }

      if (pChild)
      { // successfully added (or found) our group
        TiXmlElement newElement(strSplit[1]);
        TiXmlNode *pNewNode = pChild->InsertEndChild(newElement);
        if (pNewNode)
        {
          TiXmlText value((*it).second->ToString());
          pNewNode->InsertEndChild(value);
        }
      }
    }
  }
}

void CGUISettings::Clear()
{
  for (mapIter it = settingsMap.begin(); it != settingsMap.end(); it++)
    delete (*it).second;
  settingsMap.clear();
  for (unsigned int i = 0; i < settingsGroups.size(); i++)
    delete settingsGroups[i];
  settingsGroups.clear();
}
