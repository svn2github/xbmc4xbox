/*
 *      Copyright (C) 2005-2008 Team XBMC
 *      http://www.xbmc.org
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

#include "GUISettings.h"
#include <limits.h>
#include "Settings.h"
#include "GUIDialogFileBrowser.h"
#include "XBAudioConfig.h"
#include "MediaManager.h"
#ifdef _LINUX
#include "LinuxTimezone.h"
#endif
#include "utils/Network.h"
#include "Application.h"
#include "FileSystem/SpecialProtocol.h"
#include "AdvancedSettings.h"
#include "LocalizeStrings.h"
#include "StringUtils.h"
#include "SystemInfo.h"
#include "utils/log.h"
#include "tinyXML/tinyxml.h"
#include "visualizations/Visualisation.h"
#include "WindowingFactory.h"

using namespace std;

// String id's of the masks
#define MASK_MINS   14044
#define MASK_SECS   14045
#define MASK_MS    14046
#define MASK_PERCENT 14047
#define MASK_KBPS   14048
#define MASK_KB    14049
#define MASK_DB    14050

#define MAX_RESOLUTIONS 128

#define TEXT_OFF 351

class CGUISettings g_guiSettings;

#ifdef _LINUX
#define DEFAULT_VISUALISATION "opengl_spectrum.vis"
#elif defined(_WIN32)
#ifdef HAS_DX
#define DEFAULT_VISUALISATION "MilkDrop_win32dx.vis"
#else
#define DEFAULT_VISUALISATION "opengl_spectrum_win32.vis"
#endif
#endif

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
  m_iFormat = iFormat;
  if (m_iFormat < 0)
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
  if (sscanf(strValue, "%x", (unsigned int *)&iHexValue))
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

CSettingPath::CSettingPath(int iOrder, const char *strSetting, int iLabel, const char *strData, int iControlType, bool bAllowEmpty, int iHeadingString)
    : CSettingString(iOrder, strSetting, iLabel, strData, iControlType, bAllowEmpty, iHeadingString)
{
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
}

void CGUISettings::Initialize()
{
  ZeroMemory(&m_replayGain, sizeof(ReplayGainSettings));

  // Pictures settings
  AddGroup(0, 1);
  AddCategory(0, "pictures", 14081);
  AddBool(1, "pictures.usetags", 14082, true);
  AddBool(2,"pictures.generatethumbs",13360,true);
  AddBool(3, "pictures.useexifrotation", 20184, true);
  AddBool(4, "pictures.showvideos", 22022, false);
  // FIXME: hide this setting until it is properly respected. In the meanwhile, default to AUTO.
  AddInt(0, "pictures.displayresolution", 169, (int)RES_AUTORES, (int)RES_AUTORES, 1, (int)RES_AUTORES, SPIN_CONTROL_TEXT);

  AddCategory(0, "slideshow", 108);
  AddInt(1, "slideshow.staytime", 12378, 9, 1, 1, 100, SPIN_CONTROL_INT_PLUS, MASK_SECS);
  AddBool(2, "slideshow.displayeffects", 12379, true);
  AddBool(0, "slideshow.shuffle", 13319, false);

  // Programs settings
//  AddGroup(1, 0);

  // My Weather settings
  AddGroup(2, 8);
  AddCategory(2, "weather", 16000);
  AddString(1, "weather.areacode1", 14019, "USNY0996 - New York, NY", BUTTON_CONTROL_STANDARD);
  AddString(2, "weather.areacode2", 14020, "UKXX0085 - London, United Kingdom", BUTTON_CONTROL_STANDARD);
  AddString(3, "weather.areacode3", 14021, "JAXX0085 - Tokyo, Japan", BUTTON_CONTROL_STANDARD);
  AddSeparator(4, "weather.sep1");
  AddString(5, "weather.plugin", 23000, "", SPIN_CONTROL_TEXT, true);
  AddString(6, "weather.pluginsettings", 23001, "", BUTTON_CONTROL_STANDARD, true);
  AddSeparator(7, "weather.sep2");
  AddString(8, "weather.jumptolocale", 20026, "", BUTTON_CONTROL_STANDARD);

  // My Music Settings
  AddGroup(3, 2);
  AddCategory(3, "mymusic", 16000);

  AddString(1, "mymusic.visualisation", 250, DEFAULT_VISUALISATION, SPIN_CONTROL_TEXT);

  AddSeparator(2, "mymusic.sep1");
  AddBool(3, "mymusic.autoplaynextitem", 489, true);
  AddBool(4, "mymusic.queuebydefault", 14084, false);
  AddSeparator(6, "mymusic.sep2");
  AddPath(7,"mymusic.recordingpath",20005,"select writable folder",BUTTON_CONTROL_PATH_INPUT,false,657);

  AddCategory(3,"musiclibrary",14022);
  AddBool(0, "musiclibrary.enabled", 418, true);
  AddBool(2, "musiclibrary.albumartistsonly", 13414, false);
  AddSeparator(3,"musiclibrary.sep1");
  AddBool(4,"musiclibrary.downloadinfo", 20192, false);
  AddString(6, "musiclibrary.defaultscraper", 20194, "discogs.xml", SPIN_CONTROL_TEXT);
  AddString(7, "musiclibrary.scrapersettings", 21417, "", BUTTON_CONTROL_STANDARD);
  AddBool(8, "musiclibrary.updateonstartup", 22000, false);
  AddBool(0, "musiclibrary.backgroundupdate", 22001, false);
  AddSeparator(9,"musiclibrary.sep2");
  AddString(10, "musiclibrary.cleanup", 334, "", BUTTON_CONTROL_STANDARD);
  AddString(11, "musiclibrary.export", 20196, "", BUTTON_CONTROL_STANDARD);
  AddString(12, "musiclibrary.import", 20197, "", BUTTON_CONTROL_STANDARD);

  AddCategory(3, "musicplayer", 16003);
  AddSeparator(3, "musicplayer.sep1");
  AddInt(4, "musicplayer.replaygaintype", 638, REPLAY_GAIN_ALBUM, REPLAY_GAIN_NONE, 1, REPLAY_GAIN_TRACK, SPIN_CONTROL_TEXT);
  AddInt(0, "musicplayer.replaygainpreamp", 641, 89, 77, 1, 101, SPIN_CONTROL_INT_PLUS, MASK_DB);
  AddInt(0, "musicplayer.replaygainnogainpreamp", 642, 89, 77, 1, 101, SPIN_CONTROL_INT_PLUS, MASK_DB);
  AddBool(0, "musicplayer.replaygainavoidclipping", 643, false);
  AddSeparator(8, "musicplayer.sep2");
  AddInt(9, "musicplayer.crossfade", 13314, 0, 0, 1, 15, SPIN_CONTROL_INT_PLUS, MASK_SECS, TEXT_OFF);
  AddBool(10, "musicplayer.crossfadealbumtracks", 13400, true);
  AddSeparator(11, "musicplayer.sep3");
  AddString(0, "musicplayer.jumptocache", 439, "", BUTTON_CONTROL_STANDARD);

  AddCategory(3, "musicfiles", 744);
  AddBool(1, "musicfiles.usetags", 258, true);
  AddString(2, "musicfiles.trackformat", 13307, "[%N. ]%A - %T", EDIT_CONTROL_INPUT, false, 16016);
  AddString(3, "musicfiles.trackformatright", 13387, "%D", EDIT_CONTROL_INPUT, false, 16016);
  // advanced per-view trackformats.
  AddString(0, "musicfiles.nowplayingtrackformat", 13307, "", EDIT_CONTROL_INPUT, false, 16016);
  AddString(0, "musicfiles.nowplayingtrackformatright", 13387, "", EDIT_CONTROL_INPUT, false, 16016);
  AddString(0, "musicfiles.librarytrackformat", 13307, "", EDIT_CONTROL_INPUT, false, 16016);
  AddString(0, "musicfiles.librarytrackformatright", 13387, "", EDIT_CONTROL_INPUT, false, 16016);
  AddSeparator(4, "musicfiles.sep1");
  AddBool(10, "musicfiles.usecddb", 227, true);
  AddBool(11, "musicfiles.findremotethumbs", 14059, true);

  AddCategory(3, "scrobbler", 15221);
  AddBool(1, "scrobbler.lastfmsubmit", 15201, false);
  AddBool(2, "scrobbler.lastfmsubmitradio", 15250, false);
  AddString(3,"scrobbler.lastfmusername", 15202, "", EDIT_CONTROL_INPUT, false, 15202);
  AddString(4,"scrobbler.lastfmpassword", 15203, "", EDIT_CONTROL_HIDDEN_INPUT, false, 15203);
  AddSeparator(5, "scrobbler.sep1");
  AddBool(6, "scrobbler.librefmsubmit", 15217, false);
  AddString(7, "scrobbler.librefmusername", 15218, "", EDIT_CONTROL_INPUT, false, 15218);
  AddString(8, "scrobbler.librefmpassword", 15219, "", EDIT_CONTROL_HIDDEN_INPUT, false, 15219);

  AddCategory(3, "cddaripper", 620);
  AddPath(1, "cddaripper.path", 20000, "select writable folder", BUTTON_CONTROL_PATH_INPUT, false, 657);
  AddString(2, "cddaripper.trackformat", 13307, "[%N. ]%T - %A", EDIT_CONTROL_INPUT, false, 16016);
  AddInt(3, "cddaripper.encoder", 621, CDDARIP_ENCODER_LAME, CDDARIP_ENCODER_LAME, 1, CDDARIP_ENCODER_WAV, SPIN_CONTROL_TEXT);
  AddInt(4, "cddaripper.quality", 622, CDDARIP_QUALITY_CBR, CDDARIP_QUALITY_CBR, 1, CDDARIP_QUALITY_EXTREME, SPIN_CONTROL_TEXT);
  AddInt(5, "cddaripper.bitrate", 623, 192, 128, 32, 320, SPIN_CONTROL_INT_PLUS, MASK_KBPS);

#ifdef HAS_KARAOKE
  AddCategory(3, "karaoke", 13327);
  AddBool(1, "karaoke.enabled", 13323, false);
  // auto-popup the song selector dialog when the karaoke song was just finished and playlist is empty.
  AddBool(2, "karaoke.autopopupselector", 22037, false);
  AddSeparator(3, "karaoke.sep1");
  AddString(4, "karaoke.font", 22030, "arial.ttf", SPIN_CONTROL_TEXT);
  AddInt(5, "karaoke.fontheight", 22031, 36, 16, 2, 74, SPIN_CONTROL_TEXT); // use text as there is a disk based lookup needed
  AddInt(6, "karaoke.fontcolors", 22032, KARAOKE_COLOR_START, KARAOKE_COLOR_START, 1, KARAOKE_COLOR_END, SPIN_CONTROL_TEXT);
  AddString(7, "karaoke.charset", 22033, "DEFAULT", SPIN_CONTROL_TEXT);
  AddSeparator(8,"karaoke.sep2");
  AddString(10, "karaoke.export", 22038, "", BUTTON_CONTROL_STANDARD);
  AddString(11, "karaoke.importcsv", 22036, "", BUTTON_CONTROL_STANDARD);
#endif

  // System settings
  AddGroup(4, 13000);
  AddCategory(4, "system", 13281);
  // advanced only configuration
  AddBool(1, "system.debuglogging", 20191, false);
  AddPath(2, "system.screenshotpath",20004,"select writable folder",BUTTON_CONTROL_PATH_INPUT,false,657);
  AddSeparator(3, "system.sep1");
  AddInt(4, "system.shutdowntime", 357, 0, 0, 5, 120, SPIN_CONTROL_INT_PLUS, MASK_MINS, TEXT_OFF);
  // In standalone mode we default to another.
  if (g_application.IsStandAlone())
    AddInt(5, "system.shutdownstate", 13008, 0, 1, 1, 5, SPIN_CONTROL_TEXT);
  else
    AddInt(5, "system.shutdownstate", 13008, POWERSTATE_QUIT, 0, 1, 5, SPIN_CONTROL_TEXT);
#if defined(_LINUX) && !defined(__APPLE__)
  AddInt(6, "system.powerbuttonaction", 13015, POWERSTATE_NONE, 0, 1, 5, SPIN_CONTROL_TEXT);
#endif
  AddBool(7, "system.haslcd", 4501, false);

#ifdef __APPLE__
  AddCategory(4, "appleremote", 13600);
  AddInt(1, "appleremote.mode", 13601, APPLE_REMOTE_STANDARD, APPLE_REMOTE_DISABLED, 1, APPLE_REMOTE_MULTIREMOTE, SPIN_CONTROL_TEXT);
  AddBool(2, "appleremote.alwayson", 13602, false);
  AddInt(0, "appleremote.sequencetime", 13603, 500, 50, 50, 1000, SPIN_CONTROL_INT_PLUS, MASK_MS, TEXT_OFF);
#endif

  //AddCategory(4, "autorun", 447);
  AddBool(1, "autorun.dvd", 240, true);
  AddBool(2, "autorun.vcd", 241, true);
  AddBool(3, "autorun.cdda", 242, true);
  AddBool(5, "autorun.video", 244, true);
  AddBool(6, "autorun.music", 245, true);
  AddBool(7, "autorun.pictures", 246, true);

  AddCategory(4, "cache", 439);
  AddInt(0, "cache.harddisk", 14025, 256, 0, 256, 4096, SPIN_CONTROL_INT_PLUS, MASK_KB, TEXT_OFF);
  AddSeparator(0, "cache.sep1");
  AddInt(0, "cachevideo.dvdrom", 14026, 2048, 0, 256, 16384, SPIN_CONTROL_INT_PLUS, MASK_KB, TEXT_OFF);
  AddInt(0, "cachevideo.lan", 14027, 2048, 0, 256, 16384, SPIN_CONTROL_INT_PLUS, MASK_KB, TEXT_OFF);
  AddInt(0, "cachevideo.internet", 14028, 4096, 0, 256, 16384, SPIN_CONTROL_INT_PLUS, MASK_KB, TEXT_OFF);
  AddSeparator(0, "cache.sep2");
  AddInt(0, "cacheaudio.dvdrom", 14030, 256, 0, 256, 4096, SPIN_CONTROL_INT_PLUS, MASK_KB, TEXT_OFF);
  AddInt(0, "cacheaudio.lan", 14031, 256, 0, 256, 4096, SPIN_CONTROL_INT_PLUS, MASK_KB, TEXT_OFF);
  AddInt(0, "cacheaudio.internet", 14032, 256, 0, 256, 4096, SPIN_CONTROL_INT_PLUS, MASK_KB, TEXT_OFF);
  AddSeparator(0, "cache.sep3");
  AddInt(0, "cachedvd.dvdrom", 14034, 2048, 0, 256, 16384, SPIN_CONTROL_INT_PLUS, MASK_KB, TEXT_OFF);
  AddInt(0, "cachedvd.lan", 14035, 2048, 0, 256, 16384, SPIN_CONTROL_INT_PLUS, MASK_KB, TEXT_OFF);
  AddSeparator(0, "cache.sep4");
  AddInt(0, "cacheunknown.internet", 14060, 4096, 0, 256, 16384, SPIN_CONTROL_INT_PLUS, MASK_KB, TEXT_OFF);

  AddCategory(4, "audiooutput", 772);
  AddInt(3, "audiooutput.mode", 337, AUDIO_ANALOG, AUDIO_ANALOG, 1, AUDIO_DIGITAL, SPIN_CONTROL_TEXT);
  AddBool(4, "audiooutput.ac3passthrough", 364, true);
  AddBool(5, "audiooutput.dtspassthrough", 254, true);
#ifdef __APPLE__
  AddString(6, "audiooutput.audiodevice", 545, "Default", SPIN_CONTROL_TEXT);
  //AddString(7, "audiooutput.passthroughdevice", 546, "S/PDIF", BUTTON_CONTROL_INPUT);
  AddBool(7, "audiooutput.downmixmultichannel", 548, true);
#elif defined(_LINUX)
  AddSeparator(6, "audiooutput.sep1");
  AddString(7, "audiooutput.audiodevice", 545, "default", SPIN_CONTROL_TEXT);
  AddString(8, "audiooutput.customdevice", 1300, "", EDIT_CONTROL_INPUT);
  AddSeparator(9, "audiooutput.sep2");
  AddString(10, "audiooutput.passthroughdevice", 546, "iec958", SPIN_CONTROL_TEXT);
  AddString(11, "audiooutput.custompassthrough", 1301, "", EDIT_CONTROL_INPUT);
  AddSeparator(12, "audiooutput.sep3");
  AddBool(13, "audiooutput.downmixmultichannel", 548, true);
#elif defined(_WIN32)
  AddString(6, "audiooutput.audiodevice", 545, "Default", SPIN_CONTROL_TEXT);
  AddBool(7, "audiooutput.downmixmultichannel", 548, true);
#endif

  AddCategory(4, "masterlock", 12360);
  AddString(1, "masterlock.lockcode"       , 20100, "-", BUTTON_CONTROL_STANDARD);
  AddBool(4, "masterlock.startuplock"      , 20076,false);
  // hidden masterlock settings
  AddInt(0,"masterlock.maxretries", 12364, 3, 3, 1, 100, SPIN_CONTROL_TEXT);

  // video settings
  AddGroup(5, 3);
  AddCategory(5, "myvideos", 16000);
  AddBool(0, "myvideos.treatstackasfile", 20051, true);
  AddInt(2, "myvideos.resumeautomatically", 12017, RESUME_ASK, RESUME_NO, 1, RESUME_ASK, SPIN_CONTROL_TEXT);
  AddBool(4, "myvideos.extractflags",20433, true);
  AddBool(5, "myvideos.cleanstrings", 20418, false);

  AddCategory(5, "videolibrary", 14022);

  AddBool(0, "videolibrary.enabled", 418, true);
  AddBool(3, "videolibrary.hideplots", 20369, false);
  AddBool(4, "videolibrary.seasonthumbs", 20382, true);
  AddBool(5, "videolibrary.actorthumbs", 20402, true);
  AddInt(0, "videolibrary.flattentvshows", 20412, 1, 0, 1, 2, SPIN_CONTROL_TEXT);
  AddSeparator(7, "videolibrary.sep2");
  AddBool(8, "videolibrary.updateonstartup", 22000, false);
  AddBool(0, "videolibrary.backgroundupdate", 22001, false);
  AddSeparator(10, "videolibrary.sep3");
  AddString(11, "videolibrary.cleanup", 334, "", BUTTON_CONTROL_STANDARD);
  AddString(12, "videolibrary.export", 647, "", BUTTON_CONTROL_STANDARD);
  AddString(13, "videolibrary.import", 648, "", BUTTON_CONTROL_STANDARD);

  AddCategory(5, "videoplayer", 16003);
  AddString(1, "videoplayer.calibrate", 214, "", BUTTON_CONTROL_STANDARD);
  AddSeparator(3, "videoplayer.sep1");
#ifdef HAVE_LIBVDPAU
  AddInt(4, "videoplayer.rendermethod", 13415, RENDER_METHOD_AUTO, RENDER_METHOD_AUTO, 1, RENDER_METHOD_VDPAU, SPIN_CONTROL_TEXT);
#else
  AddInt(4, "videoplayer.rendermethod", 13415, RENDER_METHOD_AUTO, RENDER_METHOD_AUTO, 1, RENDER_METHOD_SOFTWARE, SPIN_CONTROL_TEXT);
#endif
  // FIXME: hide this setting until it is properly respected. In the meanwhile, default to AUTO.
  //AddInt(5, "videoplayer.displayresolution", 169, (int)RES_AUTORES, (int)RES_AUTORES, 1, (int)CUSTOM+MAX_RESOLUTIONS, SPIN_CONTROL_TEXT);
  AddInt(0, "videoplayer.displayresolution", 169, (int)RES_AUTORES, (int)RES_AUTORES, 1, (int)RES_AUTORES, SPIN_CONTROL_TEXT);
  AddBool(5, "videoplayer.adjustrefreshrate", 170, false);
  AddFloat(6, "videoplayer.aspecterror", 22021, 3.0f, 0.0f, 1.0f, 20.0f);
#ifdef HAVE_LIBVDPAU
  AddBool(0, "videoplayer.strictbinding", 13120, false);
  AddBool(0, "videoplayer.vdpau_allow_xrandr", 13122, false);
#endif
#ifdef HAS_GL
  AddSeparator(7, "videoplayer.sep1.5");
  AddInt(0, "videoplayer.highqualityupscaling", 13112, SOFTWARE_UPSCALING_DISABLED, SOFTWARE_UPSCALING_DISABLED, 1, SOFTWARE_UPSCALING_ALWAYS, SPIN_CONTROL_TEXT);
  AddInt(0, "videoplayer.upscalingalgorithm", 13116, VS_SCALINGMETHOD_BICUBIC_SOFTWARE, VS_SCALINGMETHOD_BICUBIC_SOFTWARE, 1, VS_SCALINGMETHOD_VDPAU_HARDWARE, SPIN_CONTROL_TEXT);
#ifdef HAVE_LIBVDPAU
  AddBool(10, "videoplayer.vdpauUpscalingLevel", 13121, false);
  AddBool(11, "videoplayer.vdpaustudiolevel", 13122, true);
#endif
#endif

  AddSeparator(12, "videoplayer.sep2");
  AddString(0, "videoplayer.jumptocache", 439, "", BUTTON_CONTROL_STANDARD);
  AddSeparator(13, "videoplayer.sep3");
  AddInt(15, "videoplayer.dvdplayerregion", 21372, 0, 0, 1, 8, SPIN_CONTROL_INT_PLUS, -1, TEXT_OFF);
  AddBool(16, "videoplayer.dvdautomenu", 21882, false);

  AddSeparator(18, "videoplayer.sep4");

  //sync settings not available on windows gl build
#if defined(_WIN32) && defined(HAS_GL)
  #define SYNCSETTINGS 0
#else
  #define SYNCSETTINGS 1
#endif
  AddBool(SYNCSETTINGS ? 19 : 0, "videoplayer.usedisplayasclock", 13510, false);
  AddInt(SYNCSETTINGS ? 20 : 0, "videoplayer.synctype", 13500, SYNC_DISCON, SYNC_DISCON, 1, SYNC_RESAMPLE, SPIN_CONTROL_TEXT);
  AddFloat(SYNCSETTINGS ? 0 : 0, "videoplayer.maxspeedadjust", 13504, 5.0f, 0.0f, 0.1f, 10.0f);
  AddInt(SYNCSETTINGS ? 0 : 0, "videoplayer.resamplequality", 13505, RESAMPLE_MID, RESAMPLE_LOW, 1, RESAMPLE_REALLYHIGH, SPIN_CONTROL_TEXT);

  AddSeparator(23, "videoplayer.sep5");
  AddBool(24, "videoplayer.teletextenabled", 23050, true);

  AddCategory(5, "subtitles", 287);
  AddString(1, "subtitles.font", 288, "arial.ttf", SPIN_CONTROL_TEXT);
  AddInt(2, "subtitles.height", 289, 28, 16, 2, 74, SPIN_CONTROL_TEXT); // use text as there is a disk based lookup needed
  AddInt(3, "subtitles.style", 736, FONT_STYLE_BOLD, FONT_STYLE_NORMAL, 1, FONT_STYLE_BOLD_ITALICS, SPIN_CONTROL_TEXT);
  AddInt(4, "subtitles.color", 737, SUBTITLE_COLOR_START + 1, SUBTITLE_COLOR_START, 1, SUBTITLE_COLOR_END, SPIN_CONTROL_TEXT);
  AddString(5, "subtitles.charset", 735, "DEFAULT", SPIN_CONTROL_TEXT);
  AddSeparator(7, "subtitles.sep1");
  AddPath(11, "subtitles.custompath", 21366, "", BUTTON_CONTROL_PATH_INPUT, false, 657);

  // Don't add the category - makes them hidden in the GUI
  //AddCategory(5, "postprocessing", 14041);
  AddBool(2, "postprocessing.enable", 286, false);
  AddBool(3, "postprocessing.auto", 307, true); // only has effect if PostProcessing.Enable is on.
  AddBool(4, "postprocessing.verticaldeblocking", 308, false);
  AddInt(5, "postprocessing.verticaldeblocklevel", 308, 0, 0, 1, 100, SPIN_CONTROL_INT);
  AddBool(6, "postprocessing.horizontaldeblocking", 309, false);
  AddInt(7, "postprocessing.horizontaldeblocklevel", 309, 0, 0, 1, 100, SPIN_CONTROL_INT);
  AddBool(8, "postprocessing.autobrightnesscontrastlevels", 310, false);
  AddBool(9, "postprocessing.dering", 311, false);

  AddCategory(5, "scrapers", 21412);
  AddString(1, "scrapers.moviedefault", 21413, "tmdb.xml", SPIN_CONTROL_TEXT);
  AddString(2, "scrapers.tvshowdefault", 21414, "tvdb.xml", SPIN_CONTROL_TEXT);
  AddString(3, "scrapers.musicvideodefault", 21415, "mtv.xml", SPIN_CONTROL_TEXT);
  AddSeparator(4,"scrapers.sep2");
  AddBool(5, "scrapers.langfallback", 21416, true);

  // network settings
  AddGroup(6, 705);

  AddCategory(6, "services", 14036);
  AddBool(1, "services.upnpserver", 21360, false);
  AddBool(2, "services.upnprenderer", 21881, false);
  AddSeparator(3,"services.sep3");
#ifdef HAS_WEB_SERVER
  AddBool(4,  "services.webserver",        263, false);
#ifdef _LINUX
  AddString(5,"services.webserverport",    730, (geteuid()==0)?"80":"8080", EDIT_CONTROL_NUMBER_INPUT, false, 730);
#else
  AddString(5,"services.webserverport",    730, "80", EDIT_CONTROL_NUMBER_INPUT, false, 730);
#endif
  AddString(6,"services.webserverusername",1048, "xbmc", EDIT_CONTROL_INPUT);
  AddString(7,"services.webserverpassword",733, "", EDIT_CONTROL_HIDDEN_INPUT, true, 733);
  AddSeparator(8,"services.sep1");
#endif
#ifdef HAS_EVENT_SERVER
  AddBool(9,  "services.esenabled",         791, true);
  AddString(0,"services.esport",            792, "9777", EDIT_CONTROL_NUMBER_INPUT, false, 792);
  AddInt(0,   "services.esportrange",       793, 10, 1, 1, 100, SPIN_CONTROL_INT);
  AddInt(0,   "services.esmaxclients",      797, 20, 1, 1, 100, SPIN_CONTROL_INT);
  AddBool(10,  "services.esallinterfaces",   794, false);
  AddInt(0,   "services.esinitialdelay",    795, 750, 5, 5, 10000, SPIN_CONTROL_INT);
  AddInt(0,   "services.escontinuousdelay", 796, 25, 5, 5, 10000, SPIN_CONTROL_INT);
  AddSeparator(11, "services.sep2");
#endif
#ifdef HAS_ZEROCONF
  AddBool(12, "services.zeroconf", 1260, true);
#endif

#ifndef _WIN32
  AddCategory(6, "smb", 1200);
  AddString(3, "smb.winsserver",  1207,   "",  EDIT_CONTROL_IP_INPUT);
  AddString(4, "smb.workgroup",   1202,   "WORKGROUP", EDIT_CONTROL_INPUT, false, 1202);
#endif

  AddCategory(6, "network", 798);
  if (g_application.IsStandAlone())
  {
#ifndef __APPLE__
    AddString(0, "network.interface",775,"", SPIN_CONTROL_TEXT);
    AddInt(0, "network.assignment", 715, NETWORK_DHCP, NETWORK_DHCP, 1, NETWORK_DISABLED, SPIN_CONTROL_TEXT);
    AddString(0, "network.ipaddress", 719, "0.0.0.0", EDIT_CONTROL_IP_INPUT);
    AddString(0, "network.subnet", 720, "255.255.255.0", EDIT_CONTROL_IP_INPUT);
    AddString(0, "network.gateway", 721, "0.0.0.0", EDIT_CONTROL_IP_INPUT);
    AddString(0, "network.dns", 722, "0.0.0.0", EDIT_CONTROL_IP_INPUT);
    AddString(0, "network.dnssuffix", 22002, "", EDIT_CONTROL_INPUT, true);
    AddString(0, "network.essid", 776, "0.0.0.0", BUTTON_CONTROL_STANDARD);
    AddInt(0, "network.enc", 778, ENC_NONE, ENC_NONE, 1, ENC_WPA2, SPIN_CONTROL_TEXT);
    AddString(0, "network.key", 777, "0.0.0.0", EDIT_CONTROL_INPUT);
#ifndef _WIN32
    AddString(0, "network.save", 779, "", BUTTON_CONTROL_STANDARD);
#endif
    AddSeparator(0, "network.sep1");
#endif
  }
  AddBool(13, "network.usehttpproxy", 708, false);
  AddString(14, "network.httpproxyserver", 706, "", EDIT_CONTROL_INPUT);
  AddString(15, "network.httpproxyport", 730, "8080", EDIT_CONTROL_NUMBER_INPUT, false, 707);
  AddString(16, "network.httpproxyusername", 1048, "", EDIT_CONTROL_INPUT);
  AddString(17, "network.httpproxypassword", 733, "", EDIT_CONTROL_HIDDEN_INPUT,true,733);

  // appearance settings
  AddGroup(7, 480);
  AddCategory(7,"lookandfeel", 14037);
  AddString(1, "lookandfeel.skin",166,DEFAULT_SKIN, SPIN_CONTROL_TEXT);
  AddString(2, "lookandfeel.skintheme",15111,"SKINDEFAULT", SPIN_CONTROL_TEXT);
  AddString(3, "lookandfeel.skincolors",14078, "SKINDEFAULT", SPIN_CONTROL_TEXT);
  AddString(4, "lookandfeel.font",13303,"Default", SPIN_CONTROL_TEXT);
  AddInt(5, "lookandfeel.skinzoom",20109, 0, -20, 2, 20, SPIN_CONTROL_INT, MASK_PERCENT);
  AddInt(6, "lookandfeel.startupwindow",512,1, WINDOW_HOME, 1, WINDOW_PYTHON_END, SPIN_CONTROL_TEXT);
  AddSeparator(7, "lookandfeel.sep1");
  AddString(8, "lookandfeel.soundskin",15108,"SKINDEFAULT", SPIN_CONTROL_TEXT);
  AddSeparator(10, "lookandfeel.sep2");
  AddBool(11, "lookandfeel.enablerssfeeds",13305,  true);
  AddString(13, "lookandfeel.rssedit", 21435, "", BUTTON_CONTROL_STANDARD);
  AddSeparator(14, "lookandfeel.sep3");
  AddBool(15, "lookandfeel.enablemouse", 21369, true);
  AddBool(16, "lookandfeel.remoteaskeyboard", 21449, false);
  AddBool(17, "lookandfeel.autorun", 447, false);

  AddCategory(7, "locale", 20026);
  AddString(1, "locale.country", 20026, "USA", SPIN_CONTROL_TEXT);
  AddString(2, "locale.language",248,"english", SPIN_CONTROL_TEXT);
  AddString(3, "locale.charset",735,"DEFAULT", SPIN_CONTROL_TEXT); // charset is set by the language file
#if defined(_LINUX) && !defined(__APPLE__)
  AddSeparator(4, "locale.sep1");
  AddString(8, "locale.timezone", 14079, g_timezone.GetOSConfiguredTimezone(), SPIN_CONTROL_TEXT);
  AddString(7, "locale.timezonecountry", 14080, g_timezone.GetCountryByTimezone(g_timezone.GetOSConfiguredTimezone()), SPIN_CONTROL_TEXT);
#endif
#ifdef HAS_TIME_SERVER
  AddSeparator(9, "locale.sep2");
  AddBool(10, "locale.timeserver", 168, false);
  AddString(11, "locale.timeserveraddress", 731, "pool.ntp.org", EDIT_CONTROL_INPUT);
#endif

  AddCategory(7, "videoscreen", 131);
  AddString(1, "videoscreen.screenmode", 169, "DESKTOP", SPIN_CONTROL_TEXT);

#if defined (__APPLE__) || defined(_WIN32)
  AddInt(3, "videoscreen.displayblanking", 13130, BLANKING_DISABLED, BLANKING_DISABLED, 1, BLANKING_ALL_DISPLAYS, SPIN_CONTROL_TEXT);
#endif

  AddString(3, "videoscreen.guicalibration",214,"", BUTTON_CONTROL_STANDARD);
  AddString(4, "videoscreen.testpattern",226,"", BUTTON_CONTROL_STANDARD);
#if defined(__APPLE__) || defined(_WIN32)
  // OSX does not use a driver set vsync
  AddInt(6, "videoscreen.vsync", 13105, DEFAULT_VSYNC, VSYNC_DISABLED, 1, VSYNC_ALWAYS, SPIN_CONTROL_TEXT);
#else
  AddInt(6, "videoscreen.vsync", 13105, DEFAULT_VSYNC, VSYNC_DISABLED, 1, VSYNC_DRIVER, SPIN_CONTROL_TEXT);
#endif
#if defined(_WIN32) || defined (__APPLE__)
  // We prefer a fake fullscreen mode (window covering the screen rather than dedicated fullscreen)
  // as it works nicer with switching to other applications. However on some systems vsync is broken
  // when we do this (eg non-Aero on ATI in particular) and on others (AppleTV) we can't get XBMC to
  // the front
  bool fakeFullScreen = true;
  bool showSetting = true;
  if (g_sysinfo.IsAeroDisabled())
    fakeFullScreen = false;
  if (g_sysinfo.IsAppleTV())
  {
    fakeFullScreen = false;
    showSetting = false;
  }
  AddBool(showSetting ? 7 : 0, "videoscreen.fakefullscreen", 14083, fakeFullScreen);
#endif

  AddCategory(7, "filelists", 14018);
  AddBool(1, "filelists.hideparentdiritems", 13306, false);
  AddBool(2, "filelists.hideextensions", 497, false);
  AddBool(3, "filelists.ignorethewhensorting", 13399, true);
  AddSeparator(6, "filelists.sep1");
  AddBool(7, "filelists.allowfiledeletion", 14071, false);
  AddBool(8, "filelists.disableaddsourcebuttons", 21382,  false);
  AddSeparator(9, "filelists.sep2");
  AddBool(10, "filelists.showhidden", 21330, false);

  AddCategory(7, "screensaver", 360);
  AddString(1, "screensaver.mode", 356, "Dim", SPIN_CONTROL_TEXT);
  AddString(2, "screensaver.preview", 1000, "", BUTTON_CONTROL_STANDARD);
  AddInt(3, "screensaver.time", 355, 3, 1, 1, 60, SPIN_CONTROL_INT_PLUS, MASK_MINS);
  AddBool(4, "screensaver.usemusicvisinstead", 13392, true);
  AddBool(4, "screensaver.usedimonpause", 22014, true);
  // Note: Application.cpp might hide powersaving settings if not supported.
  AddSeparator(6, "screensaver.sep_powersaving");
  AddInt(7, "screensaver.powersavingtime", 1450, 0, 0, 5, 4 * 60, SPIN_CONTROL_INT_PLUS, MASK_MINS, TEXT_OFF);
  AddSeparator(8, "screensaver.sep1");
  AddInt(9, "screensaver.dimlevel", 362, 20, 0, 10, 80, SPIN_CONTROL_INT_PLUS, MASK_PERCENT);
  AddPath(10, "screensaver.slideshowpath", 774, "F:\\Pictures\\", BUTTON_CONTROL_PATH_INPUT, false, 657);

  AddCategory(7, "window", 0);
  AddInt(0, "window.width",  0, 720, 10, 1, INT_MAX, SPIN_CONTROL_INT);
  AddInt(0, "window.height", 0, 480, 10, 1, INT_MAX, SPIN_CONTROL_INT);

  AddPath(0,"system.playlistspath",20006,"set default",BUTTON_CONTROL_PATH_INPUT,false);
}

CGUISettings::~CGUISettings(void)
{
  Clear();
}

void CGUISettings::AddGroup(int groupID, int labelID)
{
  CSettingsGroup *pGroup = new CSettingsGroup(groupID, labelID);
  if (pGroup)
    settingsGroups.push_back(pGroup);
}

void CGUISettings::AddCategory(int groupID, const char *strSetting, int labelID)
{
  for (unsigned int i = 0; i < settingsGroups.size(); i++)
  {
    if (settingsGroups[i]->GetGroupID() == groupID)
      settingsGroups[i]->AddCategory(CStdString(strSetting).ToLower(), labelID);
  }
}

CSettingsGroup *CGUISettings::GetGroup(int groupID)
{
  for (unsigned int i = 0; i < settingsGroups.size(); i++)
  {
    if (settingsGroups[i]->GetGroupID() == groupID)
      return settingsGroups[i];
  }
  CLog::Log(LOGDEBUG, "Error: Requested setting group (%i) was not found.  "
                      "It must be case-sensitive",
            groupID);
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
  CLog::Log(LOGDEBUG,"Error: Requested setting (%s) was not found.  It must be case-sensitive", strSetting);
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
  CLog::Log(LOGDEBUG,"Error: Requested setting (%s) was not found.  It must be case-sensitive", strSetting);
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
  CLog::Log(LOGDEBUG,"Error: Requested setting (%s) was not found.  It must be case-sensitive", strSetting);
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
  //ASSERT(false);
  CLog::Log(LOGDEBUG,"Error: Requested setting (%s) was not found.  It must be case-sensitive", strSetting);
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
  CLog::Log(LOGDEBUG,"Error: Requested setting (%s) was not found.  It must be case-sensitive", strSetting);
}

void CGUISettings::LoadMasterLock(TiXmlElement *pRootElement)
{
  std::map<CStdString,CSetting*>::iterator it = settingsMap.find("masterlock.maxretries");
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
  CLog::Log(LOGERROR,"Error: Requested setting (%s) was not found.  It must be case-sensitive", strSetting);
  //ASSERT(false);
  return 0;
}

void CGUISettings::SetInt(const char *strSetting, int iSetting)
{
  ASSERT(settingsMap.size());
  mapIter it = settingsMap.find(CStdString(strSetting).ToLower());
  if (it != settingsMap.end())
  {
    ((CSettingInt *)(*it).second)->SetData(iSetting);
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

void CGUISettings::AddPath(int iOrder, const char *strSetting, int iLabel, const char *strData, int iControlType, bool bAllowEmpty, int iHeadingString)
{
  CSettingPath* pSetting = new CSettingPath(iOrder, CStdString(strSetting).ToLower(), iLabel, strData, iControlType, bAllowEmpty, iHeadingString);
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
    if (result->GetData() == "select folder" || result->GetData() == "select writable folder")
    {
      CStdString strData = "";
      if (bPrompt)
      {
        VECSOURCES shares;
        g_mediaManager.GetLocalDrives(shares);
        if (CGUIDialogFileBrowser::ShowAndGetDirectory(shares,g_localizeStrings.Get(result->GetLabel()),strData,result->GetData() == "select writable folder"))
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
  CLog::Log(LOGDEBUG,"Error: Requested setting (%s) was not found.  It must be case-sensitive", strSetting);
  //ASSERT(false);
  // hardcoded return value so that compiler is happy
  return StringUtils::EmptyString;
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
  CLog::Log(LOGDEBUG,"Error: Requested setting (%s) was not found.  It must be case-sensitive", strSetting);
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
  vecSettings unorderedSettings;
  for (mapIter it = settingsMap.begin(); it != settingsMap.end(); it++)
  {
    if ((*it).first.Left(strlen(strGroup)).Equals(strGroup) && (*it).second->GetOrder() > 0 && !(*it).second->IsAdvanced())
      unorderedSettings.push_back((*it).second);
  }
  // now order them...
  sort(unorderedSettings.begin(), unorderedSettings.end(), sortsettings());

  // remove any instances of 2 separators in a row
  bool lastWasSeparator(false);
  for (vecSettings::iterator it = unorderedSettings.begin(); it != unorderedSettings.end(); it++)
  {
    CSetting *setting = *it;
    // only add separators if we don't have 2 in a row
    if (setting->GetType() == SETTINGS_TYPE_SEPARATOR)
    {
      if (!lastWasSeparator)
        settings.push_back(setting);
      lastWasSeparator = true;
    }
    else
    {
      lastWasSeparator = false;
      settings.push_back(setting);
    }
  }
}

void CGUISettings::LoadXML(TiXmlElement *pRootElement, bool hideSettings /* = false */)
{ // load our stuff...
  for (mapIter it = settingsMap.begin(); it != settingsMap.end(); it++)
  {
    LoadFromXML(pRootElement, it, hideSettings);
  }
  // setup logging...
  if (GetBool("system.debuglogging"))
  {
    g_advancedSettings.m_logLevel = std::max(g_advancedSettings.m_logLevelHint, LOG_LEVEL_DEBUG_FREEMEM);
    CLog::Log(LOGNOTICE, "Enabled debug logging due to GUI setting (%d)", g_advancedSettings.m_logLevel);
  }
  // Get hardware based stuff...
  CLog::Log(LOGNOTICE, "Getting hardware information now...");
  if (GetInt("audiooutput.mode") == AUDIO_DIGITAL && !g_audioConfig.HasDigitalOutput())
    SetInt("audiooutput.mode", AUDIO_ANALOG);
  // FIXME: Check if the hardware supports it (if possible ;)
  //SetBool("audiooutput.ac3passthrough", g_audioConfig.GetAC3Enabled());
  //SetBool("audiooutput.dtspassthrough", g_audioConfig.GetDTSEnabled());
  CLog::Log(LOGINFO, "Using %s output", GetInt("audiooutput.mode") == AUDIO_ANALOG ? "analog" : "digital");
  CLog::Log(LOGINFO, "AC3 pass through is %s", GetBool("audiooutput.ac3passthrough") ? "enabled" : "disabled");
  CLog::Log(LOGINFO, "DTS pass through is %s", GetBool("audiooutput.dtspassthrough") ? "enabled" : "disabled");

  g_guiSettings.m_LookAndFeelResolution = GetResolution();
#ifdef __APPLE__
  // trap any previous vsync by driver setting, does not exist on OSX
  if (GetInt("videoscreen.vsync") == VSYNC_DRIVER)
  {
    SetInt("videoscreen.vsync", VSYNC_ALWAYS);
  }
#endif
  // if AppleTV, trap any previous highqualityupscaling setting and set to zero
  if (g_sysinfo.IsAppleTV())
  {
    if (GetInt("videoplayer.highqualityupscaling") != SOFTWARE_UPSCALING_DISABLED)
    {
      SetInt("videoplayer.highqualityupscaling", SOFTWARE_UPSCALING_DISABLED);
    }
  }
 // DXMERGE: This might have been useful?
 // g_videoConfig.SetVSyncMode((VSYNC)GetInt("videoscreen.vsync"));
  CLog::Log(LOGNOTICE, "Checking resolution %i", g_guiSettings.m_LookAndFeelResolution);
  if (!g_graphicsContext.IsValidResolution(g_guiSettings.m_LookAndFeelResolution))
  {
    CLog::Log(LOGNOTICE, "Setting safe mode %i", RES_DESKTOP);
    SetResolution(RES_DESKTOP);
  }

  // Move replaygain settings into our struct
  m_replayGain.iPreAmp = GetInt("musicplayer.replaygainpreamp");
  m_replayGain.iNoGainPreAmp = GetInt("musicplayer.replaygainnogainpreamp");
  m_replayGain.iType = GetInt("musicplayer.replaygaintype");
  m_replayGain.bAvoidClipping = GetBool("musicplayer.replaygainavoidclipping");

  // check if we load the right vis
  if(!CVisualisation::IsValidVisualisation(g_guiSettings.GetString("mymusic.visualisation")))
    g_guiSettings.SetString("mymusic.visualisation", DEFAULT_VISUALISATION);

#if defined(_LINUX) && !defined(__APPLE__)
  CStdString timezone = GetString("locale.timezone");
  if(timezone == "0" || timezone.IsEmpty())
  {
    timezone = g_timezone.GetOSConfiguredTimezone();
    SetString("locale.timezone", timezone);
  }
  g_timezone.SetTimezone(timezone);
#endif
}

void CGUISettings::LoadFromXML(TiXmlElement *pRootElement, mapIter &it, bool advanced /* = false */)
{
  CStdStringArray strSplit;
  StringUtils::SplitString((*it).first, ".", strSplit);
  if (strSplit.size() > 1)
  {
    const TiXmlNode *pChild = pRootElement->FirstChild(strSplit[0].c_str());
    if (pChild)
    {
      const TiXmlElement *pGrandChild = pChild->FirstChildElement(strSplit[1].c_str());
      if (pGrandChild && pGrandChild->FirstChild())
      {
        CStdString strValue = pGrandChild->FirstChild()->Value();
        if (strValue.size() )
        {
          if (strValue != "-")
          { // update our item
            if ((*it).second->GetType() == SETTINGS_TYPE_PATH)
            { // check our path
              int pathVersion = 0;
              pGrandChild->Attribute("pathversion", &pathVersion);
              strValue = CSpecialProtocol::ReplaceOldPath(strValue, pathVersion);
            }
            (*it).second->FromString(strValue);
            if (advanced)
              (*it).second->SetAdvanced();
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
    // don't save advanced settings
    CStdString first = (*it).first;
    if ((*it).second->IsAdvanced())
      continue;

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
        if ((*it).second->GetControlType() == SETTINGS_TYPE_PATH)
          newElement.SetAttribute("pathversion", CSpecialProtocol::path_version);
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

float square_error(float x, float y)
{
  float yonx = (x > 0) ? y / x : 0;
  float xony = (y > 0) ? x / y : 0;
  return std::max(yonx, xony);
}

RESOLUTION CGUISettings::GetResolution() const
{
  return GetResFromString(GetString("videoscreen.screenmode"));
}

RESOLUTION CGUISettings::GetResFromString(const CStdString &res)
{
  if (res == "DESKTOP")
    return RES_DESKTOP;
  else if (res == "WINDOW")
    return RES_WINDOW;
  else if (res.GetLength()==20)
  {
    // format: SWWWWWHHHHHRRR.RRRRR, where S = screen, W = width, H = height, R = refresh
    int screen = atol(res.Mid(0,1).c_str());
    int width = atol(res.Mid(1,5).c_str());
    int height = atol(res.Mid(6,5).c_str());
    float refresh = (float)atof(res.Mid(11).c_str());
    // find the closest match to these in our res vector.  If we have the screen, we score the res
    RESOLUTION bestRes = RES_DESKTOP;
    float bestScore = 0.0f;
    size_t maxRes = g_settings.m_ResInfo.size();
    if (g_Windowing.GetNumScreens())
      maxRes = std::min(maxRes, (size_t)RES_DESKTOP + g_Windowing.GetNumScreens());
    for (unsigned int i = RES_DESKTOP; i < maxRes; ++i)
    {
      const RESOLUTION_INFO &info = g_settings.m_ResInfo[i];
      if (info.iScreen != screen)
        continue;
      float score = 10*(square_error((float)info.iWidth, (float)width) + square_error((float)info.iHeight, (float)height)) + square_error(info.fRefreshRate, refresh);
      if (score > bestScore)
      {
        bestScore = score;
        bestRes = (RESOLUTION)i;
      }
    }
    return bestRes;
  }
  return RES_DESKTOP;
}

void CGUISettings::SetResolution(RESOLUTION res)
{
  CStdString mode;
  if (res == RES_DESKTOP)
    mode = "DESKTOP";
  else if (res == RES_WINDOW)
    mode = "WINDOW";
  else if (res >= RES_CUSTOM && res < (RESOLUTION)g_settings.m_ResInfo.size())
  {
    const RESOLUTION_INFO &info = g_settings.m_ResInfo[res];
    mode.Format("%1i%05i%05i%09.5f", info.iScreen, info.iWidth, info.iHeight, info.fRefreshRate);
  }
  else
  {
    CLog::Log(LOGWARNING, "%s, setting invalid resolution %i", __FUNCTION__, res);
    mode = "DESKTOP";
  }
  SetString("videoscreen.screenmode", mode);
  m_LookAndFeelResolution = res;
}
