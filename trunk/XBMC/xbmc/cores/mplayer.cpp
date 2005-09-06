#include "../stdafx.h"
#include "mplayer.h"
#include "mplayer/mplayer.h"
#include "../util.h"
#include "../filesystem/fileshoutcast.h"
#include "DllLoader/exports/emu_kernel32.h"
#include "dlgcache.h"
#include "../FileSystem/FileSmb.h"
#include "../XBAudioConfig.h"
#include "../XBVideoConfig.h"
#include "../langcodeexpander.h"
#include "../VideoDatabase.h"
#include "../utils/GUIInfoManager.h"
#include "VideoRenderers/RenderManager.h"


#define KEY_ENTER 13
#define KEY_TAB 9

#define KEY_BASE 0x100

/*  Function keys  */
#define KEY_F (KEY_BASE+64)

/* Control keys */
#define KEY_CTRL (KEY_BASE)
#define KEY_BACKSPACE (KEY_CTRL+0)
#define KEY_DELETE (KEY_CTRL+1)
#define KEY_INSERT (KEY_CTRL+2)
#define KEY_HOME (KEY_CTRL+3)
#define KEY_END (KEY_CTRL+4)
#define KEY_PAGE_UP (KEY_CTRL+5)
#define KEY_PAGE_DOWN (KEY_CTRL+6)
#define KEY_ESC (KEY_CTRL+7)

/* Control keys short name */
#define KEY_BS KEY_BACKSPACE
#define KEY_DEL KEY_DELETE
#define KEY_INS KEY_INSERT
#define KEY_PGUP KEY_PAGE_UP
#define KEY_PGDOWN KEY_PAGE_DOWN
#define KEY_PGDWN KEY_PAGE_DOWN

/* Cursor movement */
#define KEY_CRSR (KEY_BASE+16)
#define KEY_RIGHT (KEY_CRSR+0)
#define KEY_LEFT (KEY_CRSR+1)
#define KEY_DOWN (KEY_CRSR+2)
#define KEY_UP (KEY_CRSR+3)

/* XF86 Multimedia keyboard keys */
#define KEY_XF86_BASE (0x100+384)
#define KEY_XF86_PAUSE (KEY_XF86_BASE+1)
#define KEY_XF86_STOP (KEY_XF86_BASE+2)
#define KEY_XF86_PREV (KEY_XF86_BASE+3)
#define KEY_XF86_NEXT (KEY_XF86_BASE+4)

/* Keypad keys */
#define KEY_KEYPAD (KEY_BASE+32)
#define KEY_KP0 (KEY_KEYPAD+0)
#define KEY_KP1 (KEY_KEYPAD+1)
#define KEY_KP2 (KEY_KEYPAD+2)
#define KEY_KP3 (KEY_KEYPAD+3)
#define KEY_KP4 (KEY_KEYPAD+4)
#define KEY_KP5 (KEY_KEYPAD+5)
#define KEY_KP6 (KEY_KEYPAD+6)
#define KEY_KP7 (KEY_KEYPAD+7)
#define KEY_KP8 (KEY_KEYPAD+8)
#define KEY_KP9 (KEY_KEYPAD+9)
#define KEY_KPDEC (KEY_KEYPAD+10)
#define KEY_KPINS (KEY_KEYPAD+11)
#define KEY_KPDEL (KEY_KEYPAD+12)
#define KEY_KPENTER (KEY_KEYPAD+13)

//Transforms a string into a language code used by dvd's
#define DVDLANGCODE(x) ((int)(x[1]|(x[0]<<8)))

void xbox_audio_do_work();
void xbox_audio_wait_completion();
void audio_pause();
void audio_resume();

extern void tracker_free_mplayer_dlls(void);
extern "C" void save_registry(void);
extern "C" void free_registry(void);
extern void xbox_video_CheckScreenSaver(); // Screensaver check
extern CFileShoutcast* m_pShoutCastRipper;
extern "C" void dllReleaseAll( );

const char * dvd_audio_stream_types[8] =
  { "ac3", "unknown", "mpeg1", "mpeg2ext", "lpcm", "unknown", "dts" };

const char * dvd_audio_stream_channels[6] =
  { "mono", "stereo", "unknown", "unknown", "5.1/6.1", "5.1" };

static CDlgCache* m_dlgCache = NULL;

#define MPLAYERBACKBUFFER 20

CMPlayer::Options::Options()
{
  m_bResampleAudio = false;
  m_bNoCache = false;
  m_bNoIdx = false;
  m_iChannels = 0;
  m_bAC3PassTru = false;
  m_bDTSPassTru = false;
  m_strChannelMapping = "";
  m_fVolumeAmplification = 0.0f;
  m_bNonInterleaved = false;
  m_fSpeed = 1.0f;
  m_fFPS = 0.0f;
  m_iAutoSync = 0;

  m_iAudioStream = -1;
  m_iSubtitleStream = -1;

  m_strDvdDevice = "";
  m_strFlipBiDiCharset = "";
  m_strHexRawAudioFormat = "";

  m_bLimitedHWAC3 = false;
  m_bDeinterlace = false;
}
void CMPlayer::Options::SetFPS(float fFPS)
{
  m_fFPS = fFPS;
}
float CMPlayer::Options::GetFPS() const
{
  return m_fFPS;
}

bool CMPlayer::Options::GetNoCache() const
{
  return m_bNoCache;
}
void CMPlayer::Options::SetNoCache(bool bOnOff)
{
  m_bNoCache = bOnOff;
}

bool CMPlayer::Options::GetNoIdx() const
{
  return m_bNoIdx;
}

void CMPlayer::Options::SetNoIdx(bool bOnOff)
{
  m_bNoIdx = bOnOff;
}

void CMPlayer::Options::SetSpeed(float fSpeed)
{
  m_fSpeed = fSpeed;
}
float CMPlayer::Options::GetSpeed() const
{
  return m_fSpeed;
}

bool CMPlayer::Options::GetNonInterleaved() const
{
  return m_bNonInterleaved;
}
void CMPlayer::Options::SetNonInterleaved(bool bOnOff)
{
  m_bNonInterleaved = bOnOff;
}


int CMPlayer::Options::GetAudioStream() const
{
  return m_iAudioStream;
}
void CMPlayer::Options::SetAudioStream(int iStream)
{
  m_iAudioStream = iStream;
}

int CMPlayer::Options::GetSubtitleStream() const
{
  return m_iSubtitleStream;
}
void CMPlayer::Options::SetSubtitleStream(int iStream)
{
  m_iSubtitleStream = iStream;
}

float CMPlayer::Options::GetVolumeAmplification() const
{
  return m_fVolumeAmplification;
}

void CMPlayer::Options::SetVolumeAmplification(float fDB)
{
  if (fDB < -200.0f) fDB = -200.0f;
  if (fDB > 60.0f) fDB = 60.0f;
  m_fVolumeAmplification = fDB;
}


int CMPlayer::Options::GetChannels() const
{
  return m_iChannels;
}
void CMPlayer::Options::SetChannels(int iChannels)
{
  m_iChannels = iChannels;
}

bool CMPlayer::Options::GetAC3PassTru()
{
  return m_bAC3PassTru;
}
void CMPlayer::Options::SetAC3PassTru(bool bOnOff)
{
  m_bAC3PassTru = bOnOff;
}

bool CMPlayer::Options::GetDTSPassTru()
{
  return m_bDTSPassTru;
}
void CMPlayer::Options::SetDTSPassTru(bool bOnOff)
{
  m_bDTSPassTru = bOnOff;
}

bool CMPlayer::Options::GetLimitedHWAC3()
{
  return m_bLimitedHWAC3;
}
void CMPlayer::Options::SetLimitedHWAC3(bool bOnOff)
{
  m_bLimitedHWAC3 = bOnOff;
}

const string CMPlayer::Options::GetChannelMapping() const
{
  return m_strChannelMapping;
}

void CMPlayer::Options::SetChannelMapping(const string& strMapping)
{
  m_strChannelMapping = strMapping;
}

void CMPlayer::Options::SetDVDDevice(const string & strDevice)
{
  m_strDvdDevice = strDevice;
}

void CMPlayer::Options::SetFlipBiDiCharset(const string& strCharset)
{
  m_strFlipBiDiCharset = strCharset;
}

void CMPlayer::Options::SetRawAudioFormat(const string& strHexRawAudioFormat)
{
  m_strHexRawAudioFormat = strHexRawAudioFormat;
}

void CMPlayer::Options::SetAutoSync(int iAutoSync)
{
  m_iAutoSync = iAutoSync;
}

void CMPlayer::Options::GetOptions(int& argc, char* argv[])
{
  CStdString strTmp;
  m_vecOptions.erase(m_vecOptions.begin(), m_vecOptions.end());
  m_vecOptions.push_back("xbmc.exe");

  // enable direct rendering (mplayer directly draws on xbmc's overlay texture)
  m_vecOptions.push_back("-dr");
  if (m_strHexRawAudioFormat.length() > 0)
  {
    m_vecOptions.push_back("-rawaudio");
    m_vecOptions.push_back("on:format=" + m_strHexRawAudioFormat); //0x2000
  }

  if (CLog::GetLevel() == LOGDEBUG)
    m_vecOptions.push_back("-v");
  else
    m_vecOptions.push_back("-quiet");

  if (m_bNoCache)
  {
    m_vecOptions.push_back("-nocache");
  }

  if (m_bNoIdx)
  {
    m_vecOptions.push_back("-noidx");
  }

  //MOVED TO mplayer.conf
  //Enable mplayer's internal highly accurate sleeping.
  //m_vecOptions.push_back("-softsleep");

  //limit A-V sync correction in order to get smoother playback.
  //defaults to 0.01 but for high quality videos 0.0001 results in
  // much smoother playback but slow reaction time to fix A-V desynchronization
  //m_vecOptions.push_back("-mc");
  //m_vecOptions.push_back("0.0001");

  // smooth out audio driver timer (audio drivers arent perect)
  //Higher values mean more smoothing,but avoid using numbers too high,
  //as they will cause independent timing from the sound card and may result in
  //an A-V desync
  //m_vecOptions.push_back("-autosync");
  //m_vecOptions.push_back("30");

  if (m_fSpeed != 1.0f)
  {
    // set playback speed
    m_vecOptions.push_back("-speed");
    strTmp.Format("%f", m_fSpeed);
    m_vecOptions.push_back(strTmp);
  }
  //This shouldn't be set as then the speed adjustment will be applied to this new fps, and not original.
  //Video will play way to fast, while audio will play correctly, so mplayer will constantly need to adjust.
  //Should only be set on videos that has wrong header info
  if( m_fFPS != 0.0f )
  {
    m_vecOptions.push_back("-fps");
    strTmp.Format("%f", m_fFPS);
    m_vecOptions.push_back(strTmp);

    // set subtitle fps
    m_vecOptions.push_back("-subfps");
    strTmp.Format("%f", m_fFPS);
    m_vecOptions.push_back(strTmp);
  }

  if ( m_iAudioStream >= 0)
  {
    CLog::Log(LOGINFO, " Playing audio stream: %d", m_iAudioStream);
    m_vecOptions.push_back("-aid");
    strTmp.Format("%i", m_iAudioStream);
    m_vecOptions.push_back(strTmp);
  }

  if ( m_iSubtitleStream >= 0)
  {
    CLog::Log(LOGINFO, " Playing subtitle stream: %d", m_iSubtitleStream);
    m_vecOptions.push_back("-sid");
    strTmp.Format("%i", m_iSubtitleStream);
    m_vecOptions.push_back(strTmp);
  }
  //MOVED TO mplayer.conf to allow it to be overridden on a per file basis
  // else
  // {
  //Force mplayer to allways allocate a subtitle demuxer, otherwise we
  //might not be able to enable it later. Will make sure later that it isn't visible.
  //For after 1.0 add command to ask for a specific language as an alternative.
  //m_vecOptions.push_back("-sid");
  //m_vecOptions.push_back("0");
  // }

  if ( m_iChannels)
  {
    // set number of audio channels
    m_vecOptions.push_back("-channels");
    strTmp.Format("%i", m_iChannels);
    m_vecOptions.push_back(strTmp);
  }
  if ( m_strChannelMapping.size())
  {
    // set audio channel mapping
    m_vecOptions.push_back("-af");
    m_vecOptions.push_back(m_strChannelMapping);
  }

  if ( m_iAutoSync )
  {
    // Enable autosync
    m_vecOptions.push_back("-autosync");
    strTmp.Format("%i", m_iAutoSync);
    m_vecOptions.push_back(strTmp);
  }

  if ( m_bAC3PassTru || m_bDTSPassTru)
  {
    // this is nice, we can ask mplayer to try hwac3 filter (used for ac3/dts pass-through) first
    // and if it fails try a52 filter (used for ac3 software decoding) and if that fails
    // try the other audio codecs (mp3, wma,...)
    m_vecOptions.push_back("-ac");
    CStdString buf;

    if (m_bDTSPassTru) buf += "hwdts,";
    if (m_bAC3PassTru) buf += "hwac3,";
    m_vecOptions.push_back(buf.c_str());

  }
  else
  { // TODO: add a gui option instead of just using "1"? (how applicable is this though?)
    // dynamic range compression for ac3/dts
    m_vecOptions.push_back("-a52drc");
    m_vecOptions.push_back("1");
  }
  if ( m_bLimitedHWAC3 )
    m_vecOptions.push_back("-limitedhwac3");

  if (m_strFlipBiDiCharset.length() > 0)
  {
    CLog::Log(LOGINFO, "Flipping bi-directional subtitles in charset %s", m_strFlipBiDiCharset.c_str());
    m_vecOptions.push_back("-fribidi-charset");
    m_vecOptions.push_back(m_strFlipBiDiCharset.c_str());
    m_vecOptions.push_back("-flip-hebrew");
  }
  else
  {
    CLog::Log(LOGINFO, "Flipping bi-directional subtitles disabled");
    m_vecOptions.push_back("-noflip-hebrew");
  }

  
  { //Setup any video filter we want, ie postprocessing, noise...
    strTmp.Empty();
    vector<CStdString> vecPPOptions;

    if ( m_bDeinterlace )
    {
      vecPPOptions.push_back("ci");
    }

    if ( g_guiSettings.GetBool("PostProcessing.Enable") )
    {
      if (g_guiSettings.GetBool("PostProcessing.Auto"))
      {
        // enable auto quality &postprocessing
        m_vecOptions.push_back("-autoq");
        m_vecOptions.push_back("100");

        //Just add an empty string so we know we need to add the pp filter
        vecPPOptions.push_back("default"); 
      }
      else
      {
        // manual postprocessing
        CStdString strOpt;

        if ( g_guiSettings.GetBool("PostProcessing.DeRing") )
        { // add dering filter
          vecPPOptions.push_back("dr:a");
        }
        if (g_guiSettings.GetBool("PostProcessing.VerticalDeBlocking"))
        {
          // add vertical deblocking filter
          if (g_guiSettings.GetInt("PostProcessing.VerticalDeBlockLevel") > 0) 
            strOpt.Format("vb:%i", g_guiSettings.GetInt("PostProcessing.VerticalDeBlockLevel"));
          else 
            strOpt = "vb:a";

          vecPPOptions.push_back(strOpt);
        }
        if (g_guiSettings.GetBool("PostProcessing.HorizontalDeBlocking"))
        {
          // add horizontal deblocking filter
          if (g_guiSettings.GetInt("PostProcessing.HorizontalDeBlockLevel") > 0) 
            strOpt.Format("hb:%i", g_guiSettings.GetInt("PostProcessing.HorizontalDeBlockLevel"));
          else 
            strOpt = "hb:a";

          vecPPOptions.push_back(strOpt);
        }
        if (g_guiSettings.GetBool("PostProcessing.AutoBrightnessContrastLevels"))
        {
          // add auto brightness/contrast levels
          vecPPOptions.push_back("al");
        }
      }
    }

    //Only enable post processing if something is selected
    if (vecPPOptions.size() > 0)
    {
      strTmp.Empty();
      strTmp += "pp=";

      for (unsigned int i = 0; i < vecPPOptions.size(); ++i)
      {
        strTmp += vecPPOptions[i];
        strTmp += "/";
      }
      strTmp.TrimRight("/");
    }
  }

  if (g_stSettings.m_currentVideoSettings.m_FilmGrain > 0)
  {
    CStdString strOpt;
    if (strTmp.size() > 0)
      strTmp += ",";

    strOpt.Format("noise=%dta:%dta", g_stSettings.m_currentVideoSettings.m_FilmGrain, g_stSettings.m_currentVideoSettings.m_FilmGrain);
    strTmp += strOpt;
  }

  //Check if we wanted any video filters
  if (strTmp.size() > 0)
  {
    m_vecOptions.push_back("-vf");
    m_vecOptions.push_back(strTmp);
  }


  if (m_fVolumeAmplification > 0.1f || m_fVolumeAmplification < -0.1f)
  {
    //add volume amplification audio filter
    strTmp.Format("volume=%2.2f:0", m_fVolumeAmplification);
    m_vecOptions.push_back("-af");
    m_vecOptions.push_back(strTmp);
  }
  //  if (m_bResampleAudio)
  //{
  //m_vecOptions.push_back("-af");
  // 48kHz resampling
  // format is: rate=sample_rate:bSloppy:quality
  // where bSloppy is 1 if we don't care about accurate output rate
  // and quality is:
  //     0   - linear interpolation (fast, but bad quality)
  //     1   - polyphase filtered with integer math
  //     2   - polyphase filtered with float math (slowest)
  // m_vecOptions.push_back("resample=48000:0:1");//,format=2unsignedint");
  //}

  if (m_bNonInterleaved)
  {
    // open file in non-interleaved way
    m_vecOptions.push_back("-ni");
  }

  if (m_strDvdDevice.length() > 0)
  {
    CStdString strDevice;
    strDevice = """" + m_strDvdDevice;

    //Make sure we only use forward slashes for path
    //since mplayer is manly *nix based this causes less problems.
    //Our standard file system handles this aswell.
    strDevice.Replace("\\", "/");
    strDevice.TrimRight("/");
    strDevice += """";

    m_vecOptions.push_back("-dvd-device");
    m_vecOptions.push_back(strDevice);
  }

  //Only display forced subs for dvds.
  //m_vecOptions.push_back("-forcedsubsonly");

  // Turn sub delay on
  if (g_stSettings.m_currentVideoSettings.m_SubtitleDelay != 0.0f)
  {
    m_vecOptions.push_back("-subdelay");
    CStdString strOpt;
    strOpt.Format("%2.2f", g_stSettings.m_currentVideoSettings.m_SubtitleDelay);
    m_vecOptions.push_back(strOpt.c_str());
  }

  m_vecOptions.push_back("1.avi");

  argc = (int)m_vecOptions.size();
  for (int i = 0; i < argc; ++i)
  {
    argv[i] = (char*)m_vecOptions[i].c_str();
  }
  argv[argc] = NULL;
}


CMPlayer::CMPlayer(IPlayerCallback& callback)
    : IPlayer(callback)
{
  criticalsection_head = NULL;
  m_pDLL = NULL;
  m_bIsPlaying = false;
  m_bPaused = false;
  m_bIsMplayeropenfile = false;
  m_strPath = "";
  m_bCaching = false;
  m_bSubsVisibleTTF=false;
  m_bUseFullRecaching = false;
  m_fAVDelay = 0.0f;
}

CMPlayer::~CMPlayer()
{
  CloseFile();

  //Make sure the thread really is completly closed
  //things will fail otherwise.
  while(!WaitForThreadExit(2000))
  {
    CLog::Log(LOGWARNING, "CMPlayer::~CMPlayer() - Waiting for thread to exit...");
  }

  Unload();
  while ( criticalsection_head )
  {
    CriticalSection_List * entry = criticalsection_head;
    criticalsection_head = entry->Next;
    try
    {
      delete entry;
    }
    catch(...)
    {
      CLog::Log(LOGERROR, "CMPlayer::~CMPlayer() - Unable to delete CriticalSection.. Skipping");
    }
  } //fix winnt and xbox critical data mismatch issue.
  save_registry(); // save registry to disk
  free_registry(); //free memory take by registry structures
  smb.Purge(); // close any open smb sessions
}
bool CMPlayer::load()
{
  CloseFile();
  Unload();
  if (!m_pDLL)
  {
    //  if(!g_guiSettings.GetBool("MyVideos.AlternateMPlayer"))
    //    {
    m_pDLL = new DllLoader("Q:\\system\\players\\mplayer\\mplayer.dll", true);
    /*    }
        else
        {
          m_pDLL = new DllLoader("Q:\\mplayer\\mplayer.dll",true);
        }*/

    if ( !m_pDLL->Parse() )
    {
      CLog::Log(LOGERROR, "cmplayer::load() parse failed");
      delete m_pDLL;
      m_pDLL = NULL;
      return false;
    }
    if ( !m_pDLL->ResolveImports() )
    {
      CLog::Log(LOGERROR, "cmplayer::load() resolve imports failed");
    }
    mplayer_load_dll(*m_pDLL);
  }
  return true;
}
void update_cache_dialog(const char* tmp)
{
  static bool bWroteOutput = false;
  //Make sure we lock here as this is called from the cache thread thread
  CSingleLock lock(g_graphicsContext);
  if (m_dlgCache)
  {
    try {

    CStdString message = tmp;
    message.Trim();
    if (int i = message.Find("Cache fill:") >= 0)
    {
      if (int j = message.Find('%') >= 0)
      {
        CStdString strPercentage = message.Mid(i + 11, j - i + 11);

        //filter percentage, update progressbar
        float fPercentage = 0;
        sscanf(strPercentage.c_str(), "%f", &fPercentage);
        m_dlgCache->ShowProgressBar(true);
        //assuming here mplayer cache prefill is set to 20%...
        m_dlgCache->SetPercentage(int(fPercentage * (100 / 20)));
        if(!bWroteOutput)
        {
          m_dlgCache->SetMessage("Caching...");
          bWroteOutput = true;
        }
        return;
      }
    }
    else if(int i = message.Find("VobSub parsing:") >= 0)
      if (int j = message.Find('%') >= 0)
      {
        CStdString strPercentage = message.Mid(i + 15, j - i + 15);

        //filter percentage, update progressbar
        int iPercentage = 0;
        sscanf(strPercentage.c_str(), "%d", &iPercentage);
        m_dlgCache->ShowProgressBar(true);
        m_dlgCache->SetPercentage(iPercentage);
        if(!bWroteOutput)
        {
          m_dlgCache->SetMessage("Parsing VobSub...");
          bWroteOutput = true;
        }
        return;
      }
    else
    {
      m_dlgCache->ShowProgressBar(false);
    }
    bWroteOutput = false;
    //Escape are identifiers for infovalues
    message.Replace("$", "$$");

    m_dlgCache->SetMessage(message);

    }
    catch(...)
    {
      CLog::Log(LOGERROR, "Exception in update_cache_dialog()");
    }
  }
}

extern void xbox_audio_switch_channel(int iAudioStream, bool bAudioOnAllSpeakers); //lowlevel audio

bool CMPlayer::OpenFile(const CFileItem& file, __int64 iStartTime)
{
  //Close any prevoiusely playing file
  CloseFile();

  int iRet = -1;
  int iCacheSize = 1024;
  int iCacheSizeBackBuffer = MPLAYERBACKBUFFER; // 50 % backbuffer is mplayers default
  bool bFileOnHD(false);
  bool bFileOnISO(false);
  bool bFileOnUDF(false);
  bool bFileOnInternet(false);
  bool bFileOnLAN(false);
  bool bFileIsDVDImage(false);
  bool bFileIsDVDIfoFile(false);
  

  CStdString strFile = file.m_strPath;
  m_strPath = strFile;
  CURL url(strFile);
  if ( file.IsHD() ) bFileOnHD = true;
  else if ( file.IsISO9660() ) bFileOnISO = true;
  else if ( file.IsDVD() ) bFileOnUDF = true;
  else if ( file.IsInternetStream() ) bFileOnInternet = true;
  else bFileOnLAN = true;

  bool bIsVideo = file.IsVideo();
  bool bIsAudio = file.IsAudio();
  bool bIsDVD(false);

  bFileIsDVDImage = file.IsDVDImage();
  bFileIsDVDIfoFile = file.IsDVDFile(false, true);

  CLog::DebugLog("file:%s IsDVDImage:%i IsDVDIfoFile:%i", strFile.c_str(), bFileIsDVDImage , bFileIsDVDIfoFile);
  if (strFile.Find("dvd://") >= 0 || bFileIsDVDImage || bFileIsDVDIfoFile)
  {
    bIsDVD = true;
    bIsVideo = true;
  }

  iCacheSize = GetCacheSize(bFileOnHD, bFileOnISO, bFileOnUDF, bFileOnInternet, bFileOnLAN, bIsVideo, bIsAudio, bIsDVD);
  try
  {
    if (bFileOnInternet)
    {      
      m_dlgCache = new CDlgCache(0);
      m_bUseFullRecaching = true;
    }
    else
    {
      m_dlgCache = new CDlgCache(3000);
    }
    if (iCacheSize == 0)
    {
      //let mplayer figure out what to do with the cache
      iCacheSize = -1;
    }

    CLog::Log(LOGINFO, "mplayer play:%s cachesize:%i", strFile.c_str(), iCacheSize);

    // cache (remote) subtitles to HD
    if (!bFileOnInternet && bIsVideo && !bIsDVD)
    {

      m_dlgCache->SetMessage("Caching subtitles...");
      CUtil::CacheSubtitles(strFile, _SubtitleExtension, m_dlgCache);
      
      if( m_dlgCache )
      {
        //If caching was canceled, bail here
        if( m_dlgCache->IsCanceled() ) throw 0;
        m_dlgCache->ShowProgressBar(false);
      }

      CUtil::PrepareSubtitleFonts();
    }
    else
      CUtil::ClearSubtitles();
    m_iPTS = 0;
    m_bPaused = false;

    // first init mplayer. This is needed 2 find out all information
    // like audio channels, fps etc
    load();

    char *argv[30];
    int argc = 8;
    //Options options;
    if (file.IsVideo())
    {
      options.SetNonInterleaved(g_stSettings.m_currentVideoSettings.m_NonInterleaved);

    }
    options.SetNoCache(g_stSettings.m_currentVideoSettings.m_NoCache);


    if (g_guiSettings.GetBool("Subtitles.FlipBiDiCharSet") && g_charsetConverter.isBidiCharset(g_guiSettings.GetString("Subtitles.CharSet")) > 0)
    {
      options.SetFlipBiDiCharset(g_guiSettings.GetString("Subtitles.CharSet"));
    }

    bool bSupportsAC3Out = g_audioConfig.GetAC3Enabled();
    bool bSupportsDTSOut = g_audioConfig.GetDTSEnabled();

    // shoutcast is always stereo
    if (file.IsShoutCast() )
    {
      options.SetChannels(0);
      options.SetAC3PassTru(false);
      options.SetDTSPassTru(false);
    }
    else
    {
      //Since the xbox downmixes any multichannel track to dolby surround if we don't have a dd reciever,
      //allways tell mplayer to output the maximum number of channels.
      options.SetChannels(6);

      // if we're using digital out
      // then try using direct passtrough
      if (g_guiSettings.GetInt("AudioOutput.Mode") == AUDIO_DIGITAL)
      {
        options.SetAC3PassTru(bSupportsAC3Out);
        options.SetDTSPassTru(bSupportsDTSOut);

        if ((g_guiSettings.GetBool("VideoPlayer.OutputToAllSpeakers")) && (bIsVideo) || (g_guiSettings.GetBool("MusicPlayer.OutputToAllSpeakers")) && (!bIsVideo))
          options.SetLimitedHWAC3(true); //Will limit hwac3 to not kick in on 2.0 channel streams
      }
    }

    if (bIsVideo)
    {
      options.SetVolumeAmplification((float)g_guiSettings.GetInt("VideoPlayer.VolumeAmplification"));
    }

    //Make sure we set the dvd-device parameter if we are playing dvdimages or dvdfolders
    if (bFileIsDVDImage)
    {
      options.SetDVDDevice(strFile);
      CLog::Log(LOGINFO, " dvddevice: %s", strFile.c_str());
    }
    else if (bFileIsDVDIfoFile)
    {
      CStdString strPath;
      CUtil::GetParentPath(strFile, strPath);
      if (strPath.Equals("D:\\VIDEO_TS\\", false) || strPath.Equals("D:\\VIDEO_TS", false))
        options.SetDVDDevice("D:\\"); //Properly mastered dvd, lets mplayer open the dvd properly
      else
        options.SetDVDDevice(strPath);
      CLog::Log(LOGINFO, " dvddevice: %s", strPath.c_str());
    }

    CStdString strExtension;
    CUtil::GetExtension(strFile, strExtension);
    strExtension.MakeLower();

    if (strExtension == ".avi")
    {
      // check length of file, as mplayer can't handle opendml very well
      CFile* pFile = new CFile();
      __int64 len = 0;
      if (pFile->Open(strFile.c_str(), true))
      {
        len = pFile->GetLength();
      }
      delete pFile;

      if (len > 0x7fffffff)
      {
        // fixes large opendml avis - mplayer can't handle big indices
        options.SetNoIdx(true);
        CLog::Log(LOGINFO, "Trying to play a large avi file. Setting -noidx");
      }
    }

    if (file.IsRAR())
    {
      options.SetNoIdx(true);
      CLog::Log(LOGINFO, "Trying to play a rar file (%s). Setting -noidx", strFile.c_str());
      // -noidx enables mplayer to play an .avi file from a streaming source that is not seekable.
      // it tells mplayer to _not_ try getting the avi index from the end of the file.
      // this option is enabled if we try play video from a rar file, as there is a modified version
      // of ccxstream which sends unrared data when you request the first .rar of a movie.
      // This means you can play a movie gaplessly from 50 rar files without unraring, which is neat.
    }


    //Make sure we remeber what subtitle stream and audiostream we where playing so that stacked items gets the same.
    //These will be reset in Application.Playfile if the restart parameter isn't set.
    if (g_stSettings.m_currentVideoSettings.m_AudioStream >= 0)
      options.SetAudioStream(g_stSettings.m_currentVideoSettings.m_AudioStream);

    if (g_stSettings.m_currentVideoSettings.m_SubtitleStream >= 0)
      options.SetSubtitleStream(g_stSettings.m_currentVideoSettings.m_SubtitleStream);


    //force mplayer to play ac3 and dts files with correct codec
    if (strExtension == ".ac3")
    {
      options.SetRawAudioFormat("0x2000");
    }
    else if (strExtension == ".dts")
    {
      options.SetRawAudioFormat("0x2001");
    }

    //Enable smoothing of audio clock to create smoother playback.
    if( g_guiSettings.GetBool("Filters.UseAutosync") )
      options.SetAutoSync(30);

    if( g_stSettings.m_currentVideoSettings.m_InterlaceMethod == VS_INTERLACEMETHOD_DEINTERLACE )
      options.SetDeinterlace(true);
    else
      options.SetDeinterlace(false);


    options.GetOptions(argc, argv);


    //CLog::Log(LOGINFO, "  open 1st time");

    mplayer_init(argc, argv);
    mplayer_setcache_size(iCacheSize);
    mplayer_setcache_backbuffer(iCacheSizeBackBuffer);
    mplayer_SlaveCommand("osd 0");

    if (bFileIsDVDImage || bFileIsDVDIfoFile)
    {
      m_bIsMplayeropenfile = true;
      iRet = mplayer_open_file(GetDVDArgument(strFile).c_str());
    }
    else
    {
      m_bIsMplayeropenfile = true;
      iRet = mplayer_open_file(strFile.c_str());
    }
    if (iRet <= 0 || (m_dlgCache && m_dlgCache->IsCanceled()))
    {
      throw iRet;
    }

    // Seek to the correct starting position
    if (iStartTime) SeekTime(iStartTime);

    if (bFileOnInternet)
    {
      // for streaming we're done.
    }
    else
    {
      // for other files, check the codecs/audio channels etc etc...
      char strFourCC[10], strVidFourCC[10];
      char strAudioCodec[128], strVideoCodec[128];
      long lBitRate;
      long lSampleRate;
      int iChannels;
      BOOL bVBR;
      float fFPS;
      unsigned int iWidth;
      unsigned int iHeight;
      long lFrames2Early;
      long lFrames2Late;
      bool bNeed2Restart = false;

      // get the audio & video info from the file
      mplayer_GetAudioInfo(strFourCC, strAudioCodec, &lBitRate, &lSampleRate, &iChannels, &bVBR);
      mplayer_GetVideoInfo(strVidFourCC, strVideoCodec, &fFPS, &iWidth, &iHeight, &lFrames2Early, &lFrames2Late);


      // do we need 2 do frame rate conversions ?
      if (g_stSettings.m_currentVideoSettings.m_AdjustFrameRate && file.IsVideo() )
      {
        if (g_videoConfig.HasPAL())
        {
          // PAL. Framerate for pal=25.0fps
          // do frame rate conversions for NTSC movie playback under PAL
          if (fFPS >= 23.0 && fFPS <= 24.5f )
          {
            // 23.978  fps -> 25fps frame rate conversion
            options.SetSpeed(25.0f / fFPS);
            options.SetAC3PassTru(false);
            options.SetDTSPassTru(false);
            bNeed2Restart = true;
            CLog::Log(LOGINFO, "  --restart cause we use ntsc->pal framerate conversion");
          }
        }
        else
        {
          //NTSC framerate=23.976 fps
          // do frame rate conversions for PAL movie playback under NTSC
          if (fFPS >= 24 && fFPS <= 25.5)
          {
            options.SetSpeed(23.976f / fFPS);
            options.SetAC3PassTru(false);
            options.SetDTSPassTru(false);
            bNeed2Restart = true;
            CLog::Log(LOGINFO, "  --restart cause we use pal->ntsc framerate conversion");
          }
        }
      }
      // check if the codec is AC3 or DTS
      bool bDTS = strstr(strAudioCodec, "DTS") != 0;
      bool bAC3 = strstr(strAudioCodec, "AC3") != 0;


      if (lSampleRate != 48000 && (bAC3 || bDTS)) //Fallback if we run into a weird ac3/dts track.
      {
        options.SetAC3PassTru(false);
        options.SetDTSPassTru(false);
        bNeed2Restart = true;
      }

      //Solve 3/5/>6 channels issue, all AC3/DTS passthrough exception code need place before
      //this block, because here we need decisive answer to passthrough or not.
      //bNeed2Restart Catch those SPDIF codec need to restart case, when restart they changed
      //to none passthrough mode
      if ( strstr(strAudioCodec, "SPDIF") == 0 || bNeed2Restart)
        if ( iChannels == 5 || iChannels > 6 )
        {
          options.SetChannelMapping("channels=6");
          bNeed2Restart = true;
        }
        else if ( iChannels == 3 )
        {
          options.SetChannelMapping("channels=4");
          bNeed2Restart = true;
        }

      if (bNeed2Restart)
      {
        CLog::Log(LOGINFO, "  --------------- restart ---------------");
        //CLog::Log(LOGINFO, "  open 2nd time");
        if (m_bIsMplayeropenfile)
        {
          m_bIsMplayeropenfile = false;
          mplayer_close_file();
        }
        options.GetOptions(argc, argv);
        load();

        mplayer_init(argc, argv);
        mplayer_setcache_size(iCacheSize);
        mplayer_setcache_backbuffer(iCacheSizeBackBuffer);
        mplayer_SlaveCommand("osd 0");

        if (bFileIsDVDImage || bFileIsDVDIfoFile)
        {
          m_bIsMplayeropenfile = true;
          iRet = mplayer_open_file(GetDVDArgument(strFile).c_str());
        }
        else
        {
          m_bIsMplayeropenfile = true;
          iRet = mplayer_open_file(strFile.c_str());
        }
        if (iRet <= 0 || (m_dlgCache && m_dlgCache->IsCanceled()))
        {
          throw iRet;
        }
        // Seek to the correct starting position
        if (iStartTime) SeekTime(iStartTime);

      }
    }

    // set up defaults
    SetSubtitleVisible(g_stSettings.m_currentVideoSettings.m_SubtitleOn);
    if (g_stSettings.m_currentVideoSettings.m_SubtitleStream < 0)
    {
      if (bIsDVD) //Default subtitles for dvd's off
        SetSubtitleVisible(false);
      else
        SetSubtitleVisible(true);
    }
    SetAVDelay(g_stSettings.m_currentVideoSettings.m_AudioDelay);
    if (g_stSettings.m_currentVideoSettings.m_AudioStream < -1)
    { // check + fix up the stereo/left/right setting
      bool bAudioOnAllSpeakers = (g_guiSettings.GetInt("AudioOutput.Mode") == AUDIO_DIGITAL) && ((g_guiSettings.GetBool("VideoPlayer.OutputToAllSpeakers") && HasVideo()) || (g_guiSettings.GetBool("MusicPlayer.OutputToAllSpeakers") && !HasVideo()));
      xbox_audio_switch_channel(-1 - g_stSettings.m_currentVideoSettings.m_AudioStream, bAudioOnAllSpeakers);
    }
    bIsVideo = HasVideo();
    bIsAudio = HasAudio();


    //Close progress dialog completly without fade if this is video.
    if( m_dlgCache && bIsVideo)
    {
      CSingleLock lock(g_graphicsContext);
      m_dlgCache->Close(true); 
      m_dlgCache = NULL;
    }


    m_bIsPlaying = true;
    if ( ThreadHandle() == NULL)
    {
      Create();
    }

  }
  catch (...)
  {
    CLog::Log(LOGERROR, "cmplayer::openfile() %s failed", strFile.c_str());
    iRet=-1;
    CloseFile();
    Unload();
  }

  if( m_dlgCache )
  {
    //Lock here so that mplayer is not using the the object
    CSingleLock lock(g_graphicsContext);
    //Only call Close, the object will be deleted when it's thread ends.
    //this makes sure the object is not deleted while in use
    m_dlgCache->Close(false); 
    m_dlgCache = NULL;
  }

  // mplayer return values:
  // -1 internal error
  // 0  try next file (eg. file doesn't exitst)
  // 1  successfully started playing
  return (iRet > 0);
}

bool CMPlayer::CloseFile()
{

  if( m_bIsPlaying )
  {
    StopThread();
    m_bIsPlaying = false;
  }

  //Check if file is still open
  if(m_bIsMplayeropenfile)
  { //Attempt to let mplayer clean up after it self
    try
    {
      m_bIsMplayeropenfile = false;
      mplayer_close_file();
    }
    catch(...)
    {
      char buf[255];
      mplayer_get_current_module(buf, 255);
      CLog::Log(LOGERROR, "CMPlayer::CloseFile() - Unknown Exception in mplayer_close_file() - %s", buf);
    }
  }

  return true;
}

bool CMPlayer::IsPlaying() const
{
  return m_bIsPlaying;
}


void CMPlayer::OnStartup()
{}

void CMPlayer::OnExit()
{}

void CMPlayer::Process()
{
  bool bHasVideo = HasVideo();
  bool bWaitingRestart = false;

  if (!m_pDLL || !m_bIsPlaying) return;

    m_callback.OnPlayBackStarted();

    int exceptionCount = 0;
    time_t mark = time(NULL);
    bool FirstLoop = true;

    do
    {
      try
      {
        if (!m_bPaused)
        {

          //Set audio delay we wish to use, take into account the delay caused by using an async flip
          mplayer_setAVDelay(m_fAVDelay + g_renderManager.GetAsyncFlipTime() * 0.001f);

          // we're playing
          int iRet = mplayer_process();

          //Set to notify that we are out of the process loop
          //can be used to syncronize seeking
          m_evProcessDone.Set();

          if (iRet < 0) break;

          __int64 iPTS = mplayer_get_pts();
          if (iPTS)
          {
            m_iPTS = iPTS;
          }

          FirstLoop = false;
        }
        else // we're paused
        {
          if (HasVideo())
          {
            xbox_video_CheckScreenSaver();  // Check for screen saver
            Sleep(100);
          }
          else
            Sleep(10);
        }

        //Only count forward buffer as part of buffer level
        //Cachelevel will be set to 0 when decoder has to wait for more data
        //Cachelevel will be negative if the current mplayer.dll doesn't support it
        m_CacheLevel = mplayer_GetCacheLevel() * 100 / (100 - MPLAYERBACKBUFFER);
        if (m_bUseFullRecaching && !options.GetNoCache() && m_CacheLevel >= 0)
        {
          if(!m_bPaused && !m_bCaching && m_CacheLevel==0 )
          {
            m_bCaching=true;
            Pause();
          }
          else if( m_bPaused && m_bCaching && m_CacheLevel > 85 )
          {
            m_bCaching=false;
            Pause();
          }
        }

        //Check to see we are not rendering text subtitles internal
        if( CUtil::IsUsingTTFSubtitles() && mplayer_SubtitleVisible() && mplayer_isTextSubLoaded())
        {
          mplayer_showSubtitle(false);
          m_bSubsVisibleTTF=true;
        }
        
        if( (options.GetDeinterlace() && g_stSettings.m_currentVideoSettings.m_InterlaceMethod != VS_INTERLACEMETHOD_DEINTERLACE) 
          || (!options.GetDeinterlace() && g_stSettings.m_currentVideoSettings.m_InterlaceMethod == VS_INTERLACEMETHOD_DEINTERLACE) )
        {
          if( !bWaitingRestart )
          {
            //We need to restart now as interlacing mode has changed
            bWaitingRestart = true;
            g_applicationMessenger.MediaRestart(false);
          }
        }

        //Let other threads do something, should mplayer be occupying full cpu
        //Sleep(0);
      }
      catch (...)
      {
        char module[100];
        mplayer_get_current_module(module, sizeof(module));
        CLog::Log(LOGERROR, "mplayer generated exception in %s", module);
        //CLog::Log(LOGERROR, "mplayer generated exception!");
        exceptionCount++;

        // bad codec detection
        if (FirstLoop) break;

      }

      if (exceptionCount > 0)
      {
        time_t t = time(NULL);
        if (t != mark)
        {
          mark = t;
          exceptionCount--;
        }
      }
    }
    while ((!m_bStop) && (exceptionCount < 5));

    if (!m_bStop)
    {
      xbox_audio_wait_completion();
    }
    _SubtitleExtension.Empty();
  
  // Save our settings for the current movie for later
  if (bHasVideo)
  {
    if (!m_bStop) // reset resume time if we reached the end of the movie
      g_stSettings.m_currentVideoSettings.m_ResumeTime = 0;
    CVideoDatabase dbs;
    dbs.Open();
    dbs.SetVideoSettings(m_strPath, g_stSettings.m_currentVideoSettings);
    dbs.Close();
  }

  //Set m_bIsPlaying to false here to make sure closefile doesn't try to close the file again
  m_bIsPlaying = false;
  CloseFile();

  if (m_bStop)
  {
    //Can't be sent here as it apperently causes the mplayer class to be deleted
    //m_callback.OnPlayBackStopped();
  }
  else
  {
    m_callback.OnPlayBackEnded();
  }
  m_strPath = "";
}

void CMPlayer::Unload()
{
  if (m_pDLL)
  {
    //Make sure we stop playback before unloading
    CloseFile();
    try
    {
      delete m_pDLL;
      dllReleaseAll( );
      m_pDLL = NULL;
    }
    catch(std::exception& e)
    {
      CLog::Log(LOGERROR, "CMPlayer::Unload() - Exception: %s", e.what());
    }
    catch(...)
    {
      CLog::Log(LOGERROR, "CMPlayer::Unload() - Unknown Exception");
    }
  }
}

void CMPlayer::Pause()
{
  if (!m_bPaused)
  {
    m_bPaused = true;
    if (!HasVideo())
      audio_pause();
  }
  else
  {
    m_bPaused = false;
    m_bCaching = false;
    if (!HasVideo())
      audio_resume();
    mplayer_ToFFRW(1); //Tell mplayer we have resumed playback
  }
}

bool CMPlayer::IsPaused() const
{
  return m_bPaused;
}

bool CMPlayer::HasVideo()
{
  return (mplayer_HasVideo() == TRUE);
}

bool CMPlayer::HasAudio()
{
  return (mplayer_HasAudio() == TRUE);
}

void CMPlayer::ToggleOSD()
{
  OutputDebugString("toggle mplayer OSD\n");
  mplayer_put_key('o');
}

void CMPlayer::SwitchToNextLanguage()
{
  mplayer_put_key('l');
}

void CMPlayer::ToggleSubtitles()
{
  if (CUtil::IsUsingTTFSubtitles() && mplayer_isTextSubLoaded())
    m_bSubsVisibleTTF = !m_bSubsVisibleTTF;
  else
    mplayer_put_key('s');
}


void CMPlayer::SubtitleOffset(bool bPlus)
{
  if (bPlus)
    mplayer_put_key('x');
  else
    mplayer_put_key('z');
}

void CMPlayer::Seek(bool bPlus, bool bLargeStep)
{
  // Use relative time seeking if we dont know the length of the video
  // or its explicitly enabled, and the length is alteast twice the size of the largest forward seek value

  __int64 iTime = GetTotalTime();

  if ((iTime == 0) || (g_guiSettings.GetBool("VideoPlayer.UseTimeBasedSeeking") && iTime > 2*g_stSettings.m_iMyVideoTimeSeekForwardBig))
  {
    if (bLargeStep)
    {
      if (bPlus)
        SeekRelativeTime(g_stSettings.m_iMyVideoTimeSeekForwardBig);
      else
        SeekRelativeTime(g_stSettings.m_iMyVideoTimeSeekBackwardBig);
    }
    else
    {
      if (bPlus)
        SeekRelativeTime(g_stSettings.m_iMyVideoTimeSeekForward);
      else
        SeekRelativeTime(g_stSettings.m_iMyVideoTimeSeekBackward);
    }
  }
  else
  {
    float percent = 0.0f;
    if (bLargeStep)
    {
      if (bPlus)
        percent += g_stSettings.m_iMyVideoPercentSeekForwardBig;
      else
        percent += g_stSettings.m_iMyVideoPercentSeekBackwardBig;
    }
    else
    {
      if (bPlus)
        percent += g_stSettings.m_iMyVideoPercentSeekForward;
      else
        percent += g_stSettings.m_iMyVideoPercentSeekBackward;
    }

    //If current time isn't bound by the total time, 
    //we have to seek using absolute percentage instead
    if( GetTime() > iTime * 1000 )
    {
      SeekPercentage(GetPercentage()+percent);
    }
    else
    {
      percent *= iTime;
      percent /= 100;
      
      //Seek a minimum of 1 second
      if( percent < 1 && percent > 0 )
        percent = 1;
      else if( percent > -1 && percent < 0 )
        percent = -1;

      SeekRelativeTime( (int)percent );

    }    
  }
}

void CMPlayer::SeekRelativeTime(int iSeconds)
{
  CStdString strCommand;
  strCommand.Format("seek %+i 0",iSeconds);
  mplayer_SlaveCommand(strCommand.c_str());

  //We are in the normal mplayer thread, return as otherwise we would
  //hardlock waiting for those events
  if( GetCurrentThreadId() == ThreadId() ) return;

  //Wait till process has finished twice, 
  //otherwise we can't be sure the seek has finished
  m_evProcessDone.WaitMSec(1000);
  m_evProcessDone.WaitMSec(1000);
}

void CMPlayer::ToggleFrameDrop()
{
  mplayer_put_key('d');
}

int CMPlayer::GetVolume()
{
  return mplayer_getVolume();
}
void CMPlayer::SetVolume(long nVolume)
{
  mplayer_setVolume(nVolume);
}

void CMPlayer::SetContrast(bool bPlus)
{
  if (bPlus)
    mplayer_put_key('2');
  else
    mplayer_put_key('1');
}

void CMPlayer::SetBrightness(bool bPlus)
{
  if (bPlus)
    mplayer_put_key('4');
  else
    mplayer_put_key('3');
}
void CMPlayer::SetHue(bool bPlus)
{
  if (bPlus)
    mplayer_put_key('6');
  else
    mplayer_put_key('5');
}
void CMPlayer::SetSaturation(bool bPlus)
{
  if (bPlus)
    mplayer_put_key('8');
  else
    mplayer_put_key('7');
}



void CMPlayer::GetAudioInfo( CStdString& strAudioInfo)
{
  char strFourCC[10];
  char strAudioCodec[128];
  long lBitRate;
  long lSampleRate;
  int iChannels;
  BOOL bVBR;
  if (!m_bIsPlaying || !mplayer_HasAudio())
  {
    strAudioInfo = "";
    return ;
  }
  mplayer_GetAudioInfo(strFourCC, strAudioCodec, &lBitRate, &lSampleRate, &iChannels, &bVBR);
  float fSampleRate = ((float)lSampleRate) / 1000.0f;
  if (bVBR)
  {
    strAudioInfo.Format("audio:(%s) VBR br:%i sr:%02.2f khz chns:%i",
                        strAudioCodec, lBitRate, fSampleRate, iChannels);
  }
  else
  {
    strAudioInfo.Format("audio:(%s) CBR br:%i sr:%02.2f khz chns:%i",
                        strAudioCodec, lBitRate, fSampleRate, iChannels);
  }
}

void CMPlayer::GetVideoInfo( CStdString& strVideoInfo)
{

  char strFourCC[10];
  char strVideoCodec[128];
  float fFPS;
  unsigned int iWidth;
  unsigned int iHeight;
  long lFrames2Early;
  long lFrames2Late;
  if (!m_bIsPlaying)
  {
    strVideoInfo = "";
    return ;
  }
  mplayer_GetVideoInfo(strFourCC, strVideoCodec, &fFPS, &iWidth, &iHeight, &lFrames2Early, &lFrames2Late);
  strVideoInfo.Format("video:%s fps:%02.2f %ix%i early/late:%i/%i",
                      strVideoCodec, fFPS, iWidth, iHeight, lFrames2Early, lFrames2Late);
}


void CMPlayer::GetGeneralInfo( CStdString& strVideoInfo)
{
  long lFramesDropped;
  int iQuality;
  int iCacheFilled;
  float fTotalCorrection;
  float fAVDelay;
  if (!m_bIsPlaying)
  {
    strVideoInfo = "";
    return ;
  }
  mplayer_GetGeneralInfo(&lFramesDropped, &iQuality, &iCacheFilled, &fTotalCorrection, &fAVDelay);
  strVideoInfo.Format("dropped:%i Q:%i cache:%i%% ct:%2.2f av:%2.2f",
                      lFramesDropped, iQuality, iCacheFilled, fTotalCorrection, fAVDelay);
}

extern void xbox_audio_registercallback(IAudioCallback* pCallback);
extern void xbox_audio_unregistercallback();
extern void xbox_video_getRect(RECT& SrcRect, RECT& DestRect);
extern void xbox_video_getAR(float& fAR);
extern void xbox_video_update(bool bPauseDrawing);
//extern void xbox_video_update_subtitle_position();
//extern void xbox_video_render_subtitles();

void CMPlayer::Update(bool bPauseDrawing)
{
  xbox_video_update(bPauseDrawing);
}

void CMPlayer::GetVideoRect(RECT& SrcRect, RECT& DestRect)
{
  xbox_video_getRect(SrcRect, DestRect);
}

void CMPlayer::GetVideoAspectRatio(float& fAR)
{
  xbox_video_getAR(fAR);
}

void CMPlayer::RegisterAudioCallback(IAudioCallback* pCallback)
{
  xbox_audio_registercallback(pCallback);
}

void CMPlayer::UnRegisterAudioCallback()
{
  xbox_audio_unregistercallback();
}
void CMPlayer::AudioOffset(bool bPlus)
{
  if (bPlus)
    mplayer_put_key('+');
  else
    mplayer_put_key('-');
}
void CMPlayer::SwitchToNextAudioLanguage()
{
}

//void CMPlayer::UpdateSubtitlePosition()
//{
// xbox_video_update_subtitle_position();
//}
//
//void CMPlayer::RenderSubtitles()
//{
// xbox_video_render_subtitles();
//}

bool CMPlayer::CanRecord()
{
  if (!m_pShoutCastRipper) return false;
  return m_pShoutCastRipper->CanRecord();
}
bool CMPlayer::IsRecording()
{
  if (!m_pShoutCastRipper) return false;
  return m_pShoutCastRipper->IsRecording();
}
bool CMPlayer::Record(bool bOnOff)
{
  if (!m_pShoutCastRipper) return false;
  if (bOnOff && IsRecording()) return true;
  if (bOnOff == false && IsRecording() == false) return true;
  if (bOnOff)
    return m_pShoutCastRipper->Record();

  m_pShoutCastRipper->StopRecording();
  return true;
}


void CMPlayer::SeekPercentage(float percent)
{
  if (percent > 100) percent = 100;
  if (percent < 0) percent = 0;

  mplayer_setPercentage( (int)percent );

  //We are in the normal mplayer thread, return as otherwise we would
  //hardlock waiting for those events
  if( GetCurrentThreadId() == ThreadId() ) return;

  //Wait till process has finished twice, 
  //otherwise we can't be sure the seek has finished
  m_evProcessDone.WaitMSec(1000);
  m_evProcessDone.WaitMSec(1000);
}

float CMPlayer::GetPercentage()
{
  return (float)mplayer_getPercentage( );
}


void CMPlayer::SetAVDelay(float fValue)
{
  m_fAVDelay = fValue;
}

float CMPlayer::GetAVDelay()
{
  return m_fAVDelay;
}

void CMPlayer::SetSubTitleDelay(float fValue)
{
  mplayer_setSubtitleDelay(fValue);
}

float CMPlayer::GetSubTitleDelay()
{
  return mplayer_getSubtitleDelay();
}

int CMPlayer::GetSubtitleCount()
{
  return mplayer_getSubtitleCount();
}

int CMPlayer::GetSubtitle()
{
  return mplayer_getSubtitle();
};

void CMPlayer::GetSubtitleName(int iStream, CStdString &strStreamName)
{

  xbmc_subtitle sub;
  memset(&sub, 0, sizeof(xbmc_subtitle));

  mplayer_getSubtitleInfo(iStream, &sub);

  if (strlen(sub.name) > 0)
  {
    if (!g_LangCodeExpander.Lookup(strStreamName, sub.name))
    {
      strStreamName = sub.name;
    }
  }
  else
  {
    strStreamName = "";
  }

};

void CMPlayer::SetSubtitle(int iStream)
{
  mplayer_setSubtitle(iStream);
  options.SetSubtitleStream(iStream);
  g_stSettings.m_currentVideoSettings.m_SubtitleStream = iStream;
};

bool CMPlayer::GetSubtitleVisible()
{
  return( m_bSubsVisibleTTF || mplayer_SubtitleVisible() != 0 );
}
void CMPlayer::SetSubtitleVisible(bool bVisible)
{
  g_stSettings.m_currentVideoSettings.m_SubtitleOn = bVisible;
  if (CUtil::IsUsingTTFSubtitles() && mplayer_isTextSubLoaded())
  {
    m_bSubsVisibleTTF = bVisible;
    mplayer_showSubtitle(false);
  }
  else
  {
    m_bSubsVisibleTTF = false;
    mplayer_showSubtitle(bVisible);
  }
}

int CMPlayer::GetAudioStreamCount()
{
  return mplayer_getAudioStreamCount();
}

int CMPlayer::GetAudioStream()
{
  return mplayer_getAudioStream();
}

void CMPlayer::GetAudioStreamName(int iStream, CStdString& strStreamName)
{
  stream_language_t slt;
  memset(&slt, 0, sizeof(stream_language_t));
  mplayer_getAudioStreamInfo(iStream, &slt);
  if (slt.language != 0)
  {
    CStdString strName;
    if (!g_LangCodeExpander.LookupDVDLangCode(strName, slt.language))
    {
      strName = "UNKNOWN:";
      strName += (char)(slt.language >> 8) & 255;
      strName += (char)(slt.language & 255);
    }

    strStreamName.Format("%s - %s(%s)", strName.c_str(), dvd_audio_stream_types[slt.type], dvd_audio_stream_channels[slt.channels]);
  }
  else
  {
    strStreamName = "";
  }
}

void CMPlayer::SetAudioStream(int iStream)
{
  //Make sure we get the correct aid for the stream
  //Really bad way cause we need to restart and there is no good way currently to restart mplayer without onloading it first
  g_stSettings.m_currentVideoSettings.m_AudioStream = mplayer_getAudioStreamInfo(iStream, NULL);
  options.SetAudioStream(g_stSettings.m_currentVideoSettings.m_AudioStream);
  //we need to restart after here for change to take effect
  g_applicationMessenger.MediaRestart(false);
}


void CMPlayer::SeekTime(__int64 iTime)
{
  mplayer_setTimeMs(iTime);
  g_infoManager.m_performingSeek = false;

  //We are in the normal mplayer thread, return as otherwise we would
  //hardlock waiting for those events
  if( GetCurrentThreadId() == ThreadId() ) return;

  //Wait till process has finished twice, 
  //otherwise we can't be sure the seek has finished
  m_evProcessDone.WaitMSec(1000);
  m_evProcessDone.WaitMSec(1000);
}

//Time in milleseconds
__int64 CMPlayer::GetTime()
{
  if (m_bIsPlaying)
  {
    if (HasVideo()) //As mplayer has the audio counter 10 times to big. Should be fixed
      return 1000*mplayer_getCurrentTime();
    else
      return 100*m_iPTS;
  }
  return 0;
}

int CMPlayer::GetTotalTime()
{
  if (m_bIsPlaying)
  {
    return mplayer_getTime();
  }
  return 0;
}

void CMPlayer::ToFFRW(int iSpeed)
{
  mplayer_ToFFRW( iSpeed);
}


int CMPlayer::GetCacheSize(bool bFileOnHD, bool bFileOnISO, bool bFileOnUDF, bool bFileOnInternet, bool bFileOnLAN, bool bIsVideo, bool bIsAudio, bool bIsDVD)
{
  if (g_stSettings.m_currentVideoSettings.m_NoCache) return 0;

  if (bFileOnHD)
  {
    if ( bIsDVD ) return g_guiSettings.GetInt("Cache.HardDisk");
    if ( bIsVideo) return g_guiSettings.GetInt("Cache.HardDisk");
    if ( bIsAudio) return g_guiSettings.GetInt("Cache.HardDisk");
  }
  if (bFileOnISO || bFileOnUDF)
  {
    if ( bIsDVD ) return g_guiSettings.GetInt("CacheDVD.DVDRom");
    if ( bIsVideo) return g_guiSettings.GetInt("CacheVideo.DVDRom");
    if ( bIsAudio) return g_guiSettings.GetInt("CacheAudio.DVDRom");
  }
  if (bFileOnInternet)
  {
    //  if ( bIsDVD  ) return g_guiSettings.GetInt("CacheDVD.Internet");
    if ( bIsVideo) return g_guiSettings.GetInt("CacheVideo.Internet");
    if ( bIsAudio) return g_guiSettings.GetInt("CacheAudio.Internet");
    //File is on internet however we don't know what type.
    return g_guiSettings.GetInt("CacheUnknown.Internet");
    //Apperently fixes DreamBox playback.
    //return 4096;
  }
  if (bFileOnLAN)
  {
    if ( bIsDVD ) return g_guiSettings.GetInt("CacheDVD.LAN");
    if ( bIsVideo) return g_guiSettings.GetInt("CacheVideo.LAN");
    if ( bIsAudio) return g_guiSettings.GetInt("CacheAudio.LAN");
  }
  return 1024;
}

CStdString CMPlayer::GetDVDArgument(const CStdString& strFile)
{

  int iTitle = CUtil::GetDVDIfoTitle(strFile);
  if (iTitle == 0)
    return CStdString("dvd://");
  else
  {
    CStdString strBuf;
    strBuf.Format("dvd://%i", iTitle);
    return strBuf;
  }
}
void CMPlayer::ShowOSD(bool bOnoff)
{
  if (bOnoff) mplayer_showosd(1);
  else mplayer_showosd(0);
}

void CMPlayer::DoAudioWork()
{
  xbox_audio_do_work();
}

bool CMPlayer::GetSubtitleExtension(CStdString &strSubtitleExtension)
{
  strSubtitleExtension = _SubtitleExtension;
  return (!_SubtitleExtension.IsEmpty());
}

float CMPlayer::GetActualFPS()
{
  char strFourCC[10];
  char strVideoCodec[128];
  float fFPS;
  unsigned int iWidth;
  unsigned int iHeight;
  long lFrames2Early;
  long lFrames2Late;
  mplayer_GetVideoInfo(strFourCC, strVideoCodec, &fFPS, &iWidth, &iHeight, &lFrames2Early, &lFrames2Late);
  return options.GetSpeed()*fFPS;
}