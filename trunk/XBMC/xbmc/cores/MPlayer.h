#pragma once
#include "iplayer.h"
#include "DllLoader/DllLoader.h"
#include "mplayer/mplayer.h"

class CMPlayer : public IPlayer, public CThread
{
public:
  class Options
  {
  public:
    Options();
    bool GetNonInterleaved() const;
    void SetNonInterleaved(bool bOnOff) ;

    bool GetForceIndex() const;
    void SetForceIndex(bool bOnOff);

    bool GetNoCache() const;
    void SetNoCache(bool bOnOff) ;

    bool GetNoIdx() const;
    void SetNoIdx(bool bOnOff) ;

    float GetVolumeAmplification() const;
    void SetVolumeAmplification(float fDB) ;

    int GetAudioStream() const;
    void SetAudioStream(int iStream);

    int GetSubtitleStream() const;
    void SetSubtitleStream(int iStream);

    int GetChannels() const;
    void SetChannels(int iChannels);

    bool GetAC3PassTru();
    void SetAC3PassTru(bool bOnOff);

    bool GetDTSPassTru();
    void SetDTSPassTru(bool bOnOff);

    bool GetLimitedHWAC3();
    void SetLimitedHWAC3(bool bOnOff);

    inline bool GetDeinterlace() { return m_bDeinterlace; };
    inline void SetDeinterlace(bool mDeint) { m_bDeinterlace = mDeint; };
      
    const string GetChannelMapping() const;
    void SetChannelMapping(const string& strMapping);
    void SetSpeed(float fSpeed);
    float GetSpeed() const;
    void SetFPS(float fFPS);
    float GetFPS() const;
    void GetOptions(int& argc, char* argv[]);
    void SetDVDDevice(const string & strDevice);
    void SetFlipBiDiCharset(const string& strCharset);
    void SetRawAudioFormat(const string& strHexRawAudioFormat);

    void SetAutoSync(int iAutoSync);

  private:
    bool m_bDeinterlace;
    bool m_bResampleAudio;
    bool m_bNoCache;
    bool m_bNoIdx;
    float m_fSpeed;
    float m_fFPS;
    int m_iChannels;
    int m_iAudioStream;
    int m_iSubtitleStream;
    int m_iCacheSizeBackBuffer; // percent of cache used for back buffering
    int m_iAutoSync;
    bool m_bAC3PassTru;
    bool m_bDTSPassTru;
    float m_fVolumeAmplification;
    bool m_bNonInterleaved;
    bool m_bForceIndex;
    bool m_bLimitedHWAC3;
    string m_strChannelMapping;
    string m_strDvdDevice;
    string m_strFlipBiDiCharset;
    string m_strHexRawAudioFormat;
    vector<string> m_vecOptions;
  };
  CMPlayer(IPlayerCallback& callback);
  virtual ~CMPlayer();
  virtual void RegisterAudioCallback(IAudioCallback* pCallback);
  virtual void UnRegisterAudioCallback();
  virtual bool OpenFile(const CFileItem& file, __int64 iStartTime);
  virtual bool CloseFile();
  virtual bool IsPlaying() const;
  virtual void Pause();
  virtual bool IsPaused() const;  
  virtual void Unload();
  virtual bool HasVideo();
  virtual bool HasAudio();
  virtual void ToggleOSD();
  virtual void SwitchToNextLanguage();

  virtual void ToggleSubtitles();
  virtual void ToggleFrameDrop();
  virtual void SubtitleOffset(bool bPlus = true);
  virtual void Seek(bool bPlus = true, bool bLargeStep = false);
  virtual void SetVolume(long nVolume);
  virtual void SetDynamicRangeCompression(long drc);
  virtual void SetContrast(bool bPlus = true);
  virtual void SetBrightness(bool bPlus = true);
  virtual void SetHue(bool bPlus = true);
  virtual void SetSaturation(bool bPlus = true);
  virtual void GetAudioInfo( CStdString& strAudioInfo);
  virtual void GetVideoInfo( CStdString& strVideoInfo);
  virtual void GetGeneralInfo( CStdString& strVideoInfo);
  virtual void Update(bool bPauseDrawing = false);
  virtual void GetVideoRect(RECT& SrcRect, RECT& DestRect);
  virtual void GetVideoAspectRatio(float& fAR);
  virtual void AudioOffset(bool bPlus = true);
  virtual void SwitchToNextAudioLanguage();
  virtual bool CanRecord() ;
  virtual bool IsRecording();
  virtual bool Record(bool bOnOff) ;
  virtual void SeekPercentage(float fPercent = 0);
  virtual float GetPercentage();
  virtual void SetAVDelay(float fValue = 0.0f);
  virtual float GetAVDelay();
  virtual float GetActualFPS();

  virtual void SetSubTitleDelay(float fValue = 0.0f);
  virtual float GetSubTitleDelay();

  virtual int GetSubtitleCount();
  virtual int GetSubtitle();
  virtual void GetSubtitleName(int iStream, CStdString &strStreamName);
  virtual void SetSubtitle(int iStream);
  virtual bool GetSubtitleVisible();
  virtual void SetSubtitleVisible(bool bVisible);
  virtual bool GetSubtitleExtension(CStdString &strSubtitleExtension);

  virtual int GetAudioStreamCount();
  virtual int GetAudioStream();
  virtual void GetAudioStreamName(int iStream, CStdString& strStreamName);
  virtual void SetAudioStream(int iStream);

  virtual void SeekTime(__int64 iTime = 0);
  virtual int GetTotalTime();
  virtual __int64 GetTime();
  virtual void ToFFRW(int iSpeed = 0);
  virtual void ShowOSD(bool bOnoff);
  virtual void DoAudioWork();

  virtual bool IsCaching() const {return m_bCaching;};
  virtual int GetCacheLevel() const {return m_CacheLevel;};

  virtual bool GetCurrentSubtitle(CStdStringW& strSubtitle);
  
  CStdString _SubtitleExtension;
protected:
  int GetCacheSize(bool bFileOnHD, bool bFileOnISO, bool bFileOnUDF, bool bFileOnInternet, bool bFileOnLAN, bool bIsVideo, bool bIsAudio, bool bIsDVD);
  CStdString GetDVDArgument(const CStdString& strFile);
  bool load();
  virtual void OnStartup();
  virtual void OnExit();
  virtual void Process();
  void SeekRelativeTime(int iSeconds = 0);
  bool m_bPaused;
  bool m_bIsPlaying;
  bool m_bCaching;
  bool m_bUseFullRecaching;
  int m_CacheLevel;
  float m_fAVDelay;
  DllLoader* m_pDLL;
  __int64 m_iPTS;
  Options options;
  bool m_bSubsVisibleTTF;
  bool m_bIsMplayeropenfile;
  CEvent m_evProcessDone;

};
