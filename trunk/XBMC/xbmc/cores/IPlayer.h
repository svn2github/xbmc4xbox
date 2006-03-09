#pragma once
#include "IAudioCallback.h"

//class IChapterProvider;

class IPlayerCallback
{
public:
  virtual void OnPlayBackEnded() = 0;
  virtual void OnPlayBackStarted() = 0;
  virtual void OnPlayBackStopped() = 0;
  virtual void OnQueueNextItem() = 0;
};

class IPlayer
{
public:
  IPlayer(IPlayerCallback& callback): m_callback(callback){};
  virtual ~IPlayer(){};
  virtual void RegisterAudioCallback(IAudioCallback* pCallback) = 0;
  virtual void UnRegisterAudioCallback() = 0;
  virtual bool OpenFile(const CFileItem& file, __int64 iStartTime){ return false;};
  virtual bool QueueNextFile(const CFileItem &file) { return false; };
  virtual bool CloseFile(){ return true;};
  virtual bool IsPlaying() const { return false;} ;
  virtual void Pause() = 0;
  virtual bool IsPaused() const = 0;
  virtual bool HasVideo() = 0;
  virtual bool HasAudio() = 0;
  virtual void ToggleOSD() = 0;
  virtual void SwitchToNextLanguage() = 0;
  virtual void ToggleSubtitles() = 0;
  virtual void ToggleFrameDrop() = 0;
  virtual void SubtitleOffset(bool bPlus = true) = 0;
  virtual void Seek(bool bPlus = true, bool bLargeStep = false) = 0;
  virtual void SeekPercentage(float fPercent = 0){};
  virtual float GetPercentage(){ return 0;};
  virtual void SetVolume(long nVolume){};
  virtual void SetDynamicRangeCompression(long drc){};
  virtual void SetContrast(bool bPlus = true) = 0;
  virtual void SetBrightness(bool bPlus = true) = 0;
  virtual void SetHue(bool bPlus = true) = 0;
  virtual void SetSaturation(bool bPlus = true) = 0;
  virtual void GetAudioInfo( CStdString& strAudioInfo) = 0;
  virtual void GetVideoInfo( CStdString& strVideoInfo) = 0;
  virtual void GetGeneralInfo( CStdString& strVideoInfo) = 0;
  virtual void Update(bool bPauseDrawing = false) = 0;
  virtual void GetVideoRect(RECT& SrcRect, RECT& DestRect) = 0;
  virtual void GetVideoAspectRatio(float& fAR) = 0;
  virtual void AudioOffset(bool bPlus = true){};
  virtual void SwitchToNextAudioLanguage(){};
  virtual void UpdateSubtitlePosition(){};
  virtual void RenderSubtitles(){};
  virtual bool CanRecord() { return false;};
  virtual bool IsRecording() { return false;};
  virtual bool Record(bool bOnOff) { return false;};
  virtual void SetAVDelay(float fValue = 0.0f){};
  virtual float GetAVDelay(){ return 0.0f;};

  virtual void SetSubTitleDelay(float fValue = 0.0f){};
  virtual float GetSubTitleDelay(){ return 0.0f;};
  virtual int GetSubtitleCount(){ return 1;};
  virtual int GetSubtitle(){ return 0;};
  virtual void GetSubtitleName(int iStream, CStdString &strStreamName){};
  virtual void SetSubtitle(int iStream){};
  virtual bool GetSubtitleVisible(){ return false;};
  virtual void SetSubtitleVisible(bool bVisible){};
  virtual bool GetSubtitleExtension(CStdString &strSubtitleExtension){ return false;};

  virtual int GetAudioStreamCount(){ return 1;};
  virtual int GetAudioStream(){ return 0;};
  virtual void GetAudioStreamName(int iStream, CStdString &strStreamName){};
  virtual void SetAudioStream(int iStream){};

  virtual float GetActualFPS() { return 0.0f; };
  virtual void SeekTime(__int64 iTime = 0){};
  virtual __int64 GetTime(){ return 0;};
  virtual int GetTotalTime(){ return 0;};
  virtual int GetBitrate(){ return 0;};
  virtual int GetChannels(){ return 0;};
  virtual int GetBitsPerSample(){ return 0;};
  virtual int GetSampleRate(){ return 0;};
  virtual CStdString GetCodecName(){ return L"";};
  virtual void ToFFRW(int iSpeed = 0){};
  virtual void ShowOSD(bool bOnoff){};
  // Skip to next track/item inside the current media (if supported).
  virtual bool SkipNext(){return false;}

  //Returns true if not playback (paused or stopped beeing filled)
  virtual bool IsCaching() const {return false;};
  //Cache filled in Percent
  virtual int GetCacheLevel() const {return -1;}; 

  virtual bool IsInMenu() const {return false;};
  virtual bool HasMenu() { return false; };

  virtual void DoAudioWork(){};
  virtual bool OnAction(const CAction &action) { return false; };

  virtual bool GetCurrentSubtitle(CStdStringW& strSubtitle) { strSubtitle = ""; return false; }

  //returns a state that is needed for resuming from a specific time
  virtual CStdString GetPlayerState() { return ""; };
  virtual bool SetPlayerState(CStdString state) { return false;};

  //virtual IChapterProvider* GetChapterProvider() { return NULL; }
  
protected:
  IPlayerCallback& m_callback;
};
