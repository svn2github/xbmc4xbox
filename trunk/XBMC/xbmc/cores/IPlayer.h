#pragma once
#include "stdstring.h"

class IPlayerCallback
{
public:
	virtual void OnPlayBackEnded()=0;
	virtual void OnPlayBackStarted()=0;
};

class IPlayer
{
public:
	IPlayer(IPlayerCallback& callback):m_callback(callback){};
	virtual ~IPlayer(){};
	virtual bool		openfile(const CStdString& strFile){return false;};
	virtual bool		closefile(){return true;};
	virtual bool		IsPlaying() const {return false;} ;
	virtual void		Pause()=0;
	virtual bool		IsPaused() const=0;
	virtual bool		HasVideo()=0;
	virtual bool		HasAudio()=0;
	virtual void		ToggleOSD()=0;

	virtual __int64	GetPTS() =0;
	virtual void		ToggleSubtitles()=0;
	virtual void		ToggleFrameDrop()=0;
	virtual void		SubtitleOffset(bool bPlus=true)=0;	
	virtual void	  Seek(bool bPlus=true, bool bLargeStep=false)=0;
	virtual void		SetVolume(bool bPlus=true)=0;	
	virtual void		SetContrast(bool bPlus=true)=0;	
	virtual void		SetBrightness(bool bPlus=true)=0;	
	virtual void		SetHue(bool bPlus=true)=0;	
	virtual void		SetSaturation(bool bPlus=true)=0;	
protected:
	IPlayerCallback& m_callback;
};