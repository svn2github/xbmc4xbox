#include "mplayer.h"
#include "mplayer/mplayer.h"
#include "../util.h"
#include "../settings.h"
#include "../filesystem/fileshoutcast.h"
extern CFileShoutcast* m_pShoutCastRipper;
extern "C" void dllReleaseAll( );
#include "EMUkernel32.h"

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

extern "C" void free_registry(void);

CMPlayer::Options::Options()
{
    m_iChannels=0;
    m_bAC3PassTru=false;
    m_strChannelMapping="";
    m_fVolumeAmplification=0.0f;
}

float CMPlayer::Options::GetVolumeAmplification() const
{
  return m_fVolumeAmplification;
}

void CMPlayer::Options::SetVolumeAmplification(float fDB) 
{
  if (fDB < -200.0f) fDB=-200.0f;
  if (fDB >   60.0f) fDB=60.0f;
  m_fVolumeAmplification=fDB;
}


int CMPlayer::Options::GetChannels() const
{
  return m_iChannels;
}
void CMPlayer::Options::SetChannels(int iChannels)
{
  m_iChannels=iChannels;
}

bool CMPlayer::Options::GetAC3PassTru()
{
  return m_bAC3PassTru;
}
void CMPlayer::Options::SetAC3PassTru(bool bOnOff)
{
  m_bAC3PassTru=bOnOff;
}

const string CMPlayer::Options::GetChannelMapping() const
{
  return m_strChannelMapping;
}

void CMPlayer::Options::SetChannelMapping(const string& strMapping)
{
  m_strChannelMapping=strMapping;
}


void CMPlayer::Options::GetOptions(int& argc, char* argv[])
{
  CStdString strTmp;
  m_vecOptions.erase(m_vecOptions.begin(),m_vecOptions.end());
  m_vecOptions.push_back("xbmc.exe");
  m_vecOptions.push_back("-autoq");
  m_vecOptions.push_back("6");
	if ( m_iChannels) 
  {
    m_vecOptions.push_back("-channels");
    strTmp.Format("%i", m_iChannels);
    m_vecOptions.push_back(strTmp);
  }
	if ( m_strChannelMapping.size()) 
  {
    m_vecOptions.push_back("-af");
    m_vecOptions.push_back(m_strChannelMapping);
  }
  if ( m_bAC3PassTru)
  {
    m_vecOptions.push_back("-ac");
    m_vecOptions.push_back("hwac3");
  }

  if ( g_stSettings.m_bPostProcessing || g_stSettings.m_bDeInterlace )
  {
    m_vecOptions.push_back("-vf");
    if ( g_stSettings.m_bDeInterlace)
      m_vecOptions.push_back("pp=lb");
    else
      m_vecOptions.push_back("pp");
  }

  if (m_fVolumeAmplification > 0.1f || m_fVolumeAmplification < -0.1f)
  {
    strTmp.Format("volume=%2.2f:0",m_fVolumeAmplification);
    m_vecOptions.push_back("-af");
    m_vecOptions.push_back(strTmp);
  }

  m_vecOptions.push_back("1.avi");

  argc=(int)m_vecOptions.size();
  for (int i=0; i < argc; ++i)
  {
    argv[i]=(char*)m_vecOptions[i].c_str();
  }
  argv[argc]=NULL;
}


CMPlayer::CMPlayer(IPlayerCallback& callback)
:IPlayer(callback)
{
	criticalsection_head = NULL;
	m_pDLL=NULL;
	m_bIsPlaying=false;
	m_bPaused=false;
}

CMPlayer::~CMPlayer()
{
	while( criticalsection_head )
	{
		CriticalSection_List * entry = criticalsection_head;
		criticalsection_head = entry->Next;
		delete entry;
	}	//fix winnt and xbox critical data mismatch issue.
	free_registry();	//free memory take by registry structures
	m_bIsPlaying=false;
	StopThread();
	Unload();
}
bool CMPlayer::load()
{
	m_bIsPlaying=false;
	StopThread();
	Unload();
	if (!m_pDLL)
	{
		m_pDLL = new DllLoader("Q:\\mplayer\\mplayer.dll");
		if( !m_pDLL->Parse() )
		{
			OutputDebugString("cmplayer::openfile() parse failed\n");
			delete m_pDLL;
			m_pDLL=NULL;
			return false;
		}
		if( !m_pDLL->ResolveImports()  )
		{
			OutputDebugString("cmplayer::openfile() resolve imports failed\n");
		}
		mplayer_load_dll(*m_pDLL);
	}
  return true;
}
bool CMPlayer::openfile(const CStdString& strFile)
{
	closefile();

  int iCacheSize=1024;
  if (CUtil::IsDVD(strFile) || CUtil::IsISO9660(strFile) || CUtil::IsRemote(strFile) )
  {
    iCacheSize=8192;
  }
  if (CUtil::IsAudio(strFile) && !CUtil::IsVideo(strFile) )
  {
    iCacheSize=0;
  }
  if ( CUtil::IsShoutCast(strFile) ) 
  {
    iCacheSize=256;
  }
  
	CUtil::CacheSubtitles(strFile);
  CUtil::PrepareSubtitleFonts();
	m_iPTS			= 0;
	m_bPaused	  = false;

	load();

  char *argv[30];
  int argc=8;
  Options options;
  options.SetChannels(6);
  options.SetVolumeAmplification(g_stSettings.m_fVolumeAmplification);
  options.GetOptions(argc,argv);
  
	mplayer_init(argc,argv);
	
  mplayer_setcache_size(iCacheSize);
	
	int iRet=mplayer_open_file(strFile.c_str());
	if (iRet < 0)
	{
		OutputDebugString("cmplayer::openfile() openfile failed\n");
		closefile();
		return false;
	}
	m_bIsPlaying= true;

	char strFourCC[10];
	char strAudioCodec[128];
	long lBitRate;
	long lSampleRate;
	int	 iChannels;
	BOOL bVBR;
	mplayer_GetAudioInfo(strFourCC,strAudioCodec, &lBitRate, &lSampleRate, &iChannels, &bVBR);

	// if AC3 pass Thru is enabled
	bool bSupportsSPDIFOut=(XGetAudioFlags() & (DSSPEAKER_ENABLE_AC3 | DSSPEAKER_ENABLE_DTS)) != 0;
 
	if (bSupportsSPDIFOut && g_stSettings.m_bAC3PassThru )
	{
		// and the movie has an AC3 audio stream
		if ( strstr(strAudioCodec,"AC3-liba52") && (lSampleRate==48000) )
		{
			//then close file and reopen it, 
			//but now enable the AC3 passthru audio codecs hwac3
			mplayer_close_file();
      options.SetChannels(2);
      options.SetAC3PassTru(true);
			options.GetOptions(argc,argv);
			load();
			mplayer_init(argc,argv);
			mplayer_setcache_size(iCacheSize);

			int iRet=mplayer_open_file(strFile.c_str());
			if (iRet < 0)
			{
				OutputDebugString("cmplayer::openfile() openfile failed\n");
				closefile();
				return false;
			}
			m_bIsPlaying= true;
		}
	}

	if( strstr(strAudioCodec,"DMO") && (iChannels==6) )
	{
		mplayer_close_file();
    options.SetChannels(6);
    options.SetChannelMapping("channels=6:6:0:0:1:1:2:4:3:5:4:2:5:3");
		options.GetOptions(argc,argv);
		load();
		mplayer_init(argc,argv);
    mplayer_setcache_size(iCacheSize);
		int iRet=mplayer_open_file(strFile.c_str());
		if (iRet < 0)
		{
			OutputDebugString("cmplayer::openfile() openfile failed\n");
			closefile();
			return false;
		}
		m_bIsPlaying= true;
	}	

	mplayer_GetAudioInfo(strFourCC,strAudioCodec, &lBitRate, &lSampleRate, &iChannels, &bVBR);

 
	if ( !strstr(strAudioCodec,"SPDIF") ) 
	{
    if (!bSupportsSPDIFOut)
    {
      iChannels=2;
    }
		switch(iChannels)
		{
			case 1:
			case 2:
			case 4:
				mplayer_close_file();
        if ( iChannels !=2) options.SetChannels(iChannels);
        else options.SetChannels(0);
        options.GetOptions(argc,argv);
				load();
				mplayer_init(argc,argv);
        mplayer_setcache_size(iCacheSize);

				iRet=mplayer_open_file(strFile.c_str());
				if (iRet < 0)
				{
					OutputDebugString("cmplayer::openfile() openfile failed\n");
					closefile();
					return false;
				}
				m_bIsPlaying= true;
			break;
			
			case 5:
				mplayer_close_file();

        options.SetChannels(6);
        options.SetChannelMapping("channels=6:5:0:0:1:1:2:2:3:3:4:4:5:5");
        options.GetOptions(argc,argv);
				load();
				mplayer_init(argc,argv);
        mplayer_setcache_size(iCacheSize);
				iRet=mplayer_open_file(strFile.c_str());
				if (iRet < 0)
				{
					OutputDebugString("cmplayer::openfile() openfile failed\n");
					closefile();
					return false;
				}
				m_bIsPlaying= true;
			break;

			case 3:
				mplayer_close_file();
				
        options.SetChannels(4);
        options.SetChannelMapping("channels=4:4:0:0:1:1:2:2:2:3");
        options.GetOptions(argc,argv);
				load();
				mplayer_init(argc,argv);
        mplayer_setcache_size(iCacheSize);
				iRet=mplayer_open_file(strFile.c_str());
				if (iRet < 0)
				{
					OutputDebugString("cmplayer::openfile() openfile failed\n");
					closefile();
					return false;
				}
				m_bIsPlaying= true;
			break;
			
			case 6:
			default:
			break;
		}
	}

	if ( ThreadHandle() == NULL)
	{
		Create();
	}
	
	return true;
}

bool CMPlayer::closefile()
{
	m_bIsPlaying=false;
	StopThread();
	
	return true;
}

bool CMPlayer::IsPlaying() const
{
	return m_bIsPlaying;
}


void CMPlayer::OnStartup()
{
}

void CMPlayer::OnExit()
{
}

void CMPlayer::Process()
{
	if (m_pDLL && m_bIsPlaying)
	{
		m_callback.OnPlayBackStarted();
		do 
		{
			if (!m_bPaused)
			{
				int iRet=mplayer_process();
				if (iRet < 0)
				{
					m_bIsPlaying=false;
				}
				
				__int64 iPTS=mplayer_get_pts();
				if (iPTS)
				{
					m_iPTS=iPTS;
				}
			}
			else 
			{
				Sleep(100);
			}
		} while (m_bIsPlaying);
		mplayer_close_file();
	}
	m_bIsPlaying=false;
	if (!m_bStop)
	{
		m_callback.OnPlayBackEnded();
	}
}

void CMPlayer::Unload()
{
	if (m_pDLL)
	{
		delete m_pDLL;
		dllReleaseAll( );
		m_pDLL=NULL;
	}
}

void  CMPlayer::Pause()
{
	m_bPaused=!m_bPaused;
}

bool CMPlayer::IsPaused() const
{
	return m_bPaused;
}


__int64	CMPlayer::GetPTS()
{
	if (!m_pDLL) return 0;
	if (!m_bIsPlaying) return 0;
	return m_iPTS;
}

bool CMPlayer::HasVideo()
{
	return (mplayer_HasVideo()==TRUE);
}

bool CMPlayer::HasAudio()
{
	return (mplayer_HasAudio()==TRUE);
}

void CMPlayer::ToggleOSD()
{
	mplayer_put_key('o');
}

void CMPlayer::SwitchToNextLanguage()
{
	mplayer_put_key('l');
}

void CMPlayer::ToggleSubtitles()
{
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
	if (bLargeStep)
	{
		if (bPlus)
			mplayer_put_key(KEY_PAGE_UP);
		else
			mplayer_put_key(KEY_PAGE_DOWN);
	}
	else
	{
		if (bPlus)
			mplayer_put_key(KEY_UP);
		else
			mplayer_put_key(KEY_DOWN);
	}
}

void CMPlayer::ToggleFrameDrop()
{
	mplayer_put_key('d');
}


void CMPlayer::SetVolume(bool bPlus)
{
	if (bPlus)
		mplayer_put_key('0');
	else
		mplayer_put_key('9');
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
	int	 iChannels;
	BOOL bVBR;
	if (!m_bIsPlaying||!mplayer_HasAudio())
	{
		strAudioInfo="";
		return;
	}
	mplayer_GetAudioInfo(strFourCC,strAudioCodec, &lBitRate, &lSampleRate, &iChannels, &bVBR);
	float fSampleRate=((float)lSampleRate) / 1000.0f;
	if (bVBR)
	{
		strAudioInfo.Format("audio:%s (%s) VBR br:%i sr:%02.2f khz chns:%i",
									strFourCC,strAudioCodec,lBitRate,fSampleRate,iChannels);
	}
	else
	{
		strAudioInfo.Format("audio:%s (%s) CBR br:%i sr:%02.2f khz chns:%i",
									strFourCC,strAudioCodec,lBitRate,fSampleRate,iChannels);
	}
}

void CMPlayer::GetVideoInfo( CStdString& strVideoInfo)
{

	char  strFourCC[10];
	char  strVideoCodec[128];
	float fFPS;
	unsigned int   iWidth;
	unsigned int   iHeight;
	long  lFrames2Early;
	long  lFrames2Late;
	if (!m_bIsPlaying)
	{
		strVideoInfo="";
		return;
	}
	mplayer_GetVideoInfo(strFourCC,strVideoCodec, &fFPS, &iWidth,&iHeight, &lFrames2Early, &lFrames2Late);
	strVideoInfo.Format("video:%s fps:%02.2f %ix%i early/late:%i/%i", 
											strFourCC,fFPS,iWidth,iHeight,lFrames2Early,lFrames2Late);
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
		strVideoInfo="";
		return;
	}
	mplayer_GetGeneralInfo(&lFramesDropped, &iQuality, &iCacheFilled, &fTotalCorrection, &fAVDelay);
	strVideoInfo.Format("dropped:%i Q:%i cache:%i ct:%2.2f av:%2.2f", 
												lFramesDropped, iQuality, iCacheFilled, fTotalCorrection, fAVDelay);
}

extern void xbox_audio_registercallback(IAudioCallback* pCallback);
extern void xbox_audio_unregistercallback();
extern void xbox_video_getRect(RECT& SrcRect, RECT& DestRect);
extern void xbox_video_getAR(float& fAR);
extern void xbox_video_update(bool bPauseDrawing);
extern void xbox_video_update_subtitle_position();
extern void xbox_video_render_subtitles(bool bUseBackBuffer);

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

void CMPlayer::UpdateSubtitlePosition()
{
	xbox_video_update_subtitle_position();
}

void CMPlayer::RenderSubtitles()
{
	xbox_video_render_subtitles(false);
}

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
	if (bOnOff==false && IsRecording()==false) return true;
	if (bOnOff) 
			return m_pShoutCastRipper->Record();
	
	m_pShoutCastRipper->StopRecording();
	return true;
}