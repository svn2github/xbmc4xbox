#pragma once
#include "iplayer.h"
#include "dllLoader/dll.h"
#include "mplayer/mplayer.h"
#include "stdstring.h"
#include "../utils/thread.h"
#include "../utils/event.h"

class CMPlayer : public IPlayer, public CThread
{
public:
	CMPlayer(IPlayerCallback& callback);
	virtual ~CMPlayer();
	virtual bool		openfile(const CStdString& strFile);
	virtual bool		closefile();
	virtual bool		IsPlaying() const;
	virtual void		Pause();
	virtual bool		IsPaused() const;
	virtual void		Unload();
	virtual bool		HasVideo();
	virtual bool		HasAudio();
	virtual void		ToggleOSD();

	virtual __int64	GetPTS() ;
	virtual void		ToggleSubtitles();
	virtual void		ToggleFrameDrop();
	virtual void		SubtitleOffset(bool bPlus=true);
	virtual void	  Seek(bool bPlus=true, bool bLargeStep=false);
	virtual void		SetVolume(bool bPlus=true);	
	virtual void		SetContrast(bool bPlus=true);	
	virtual void		SetBrightness(bool bPlus=true);	
	virtual void		SetHue(bool bPlus=true);	
	virtual void		SetSaturation(bool bPlus=true);	
	virtual void		GetAudioInfo( CStdString& strAudioInfo);
	virtual void		GetVideoInfo( CStdString& strVideoInfo);
	virtual void		GetGeneralInfo( CStdString& strVideoInfo);
protected:
	virtual void		OnStartup();
	virtual void		OnExit();
	virtual void		Process();
	bool						m_bPaused;
	bool						m_bIsPlaying;
	bool						m_bStopPlaying;
	DllLoader*			m_pDLL;
	CEvent					m_startEvent;
	__int64					m_iPTS;
	DWORD						m_dwTime;
};