#pragma once

#include "dshowutil/dshowutil.h"
#include "DShowUtil/DshowCommon.h"
#include <initguid.h>
#include "moreuuids.h"
#include <dmodshow.h>
#include <D3d9.h>
#include "DShowUtil/MediaTypeEx.h"
#include "Filters/ffdshow_constants.h"

#include "DSGraph.h"
#include "log.h"
#include "CharsetConverter.h"
#include "RegExp.h"

struct SStreamInfos
{
  unsigned int IAMStreamSelect_Index;
  CStdString name;
  CStdString codecname;
  DWORD flags;
  IUnknown *pObj; ///< Output pin of the splitter
  IUnknown *pUnk; ///< Input pin of the filter
  LCID  lcid;
  DWORD group;

  virtual void Clear()
  {
    IAMStreamSelect_Index = 0;
    flags = 0;
    pObj = 0;
    pUnk = 0;
    lcid = 0;
    group = 0;
    name = "";
    codecname = "";
  }
};

struct SAudioStreamInfos: SStreamInfos
{
  unsigned int channels;
  unsigned int bitrate;
  unsigned int samplerate;

  virtual void Clear()
  {
    SStreamInfos::Clear();

    channels = 0;
    bitrate = 0;
    samplerate = 0;
  }
};

struct SVideoStreamInfos: SStreamInfos
{
  unsigned int width;
  unsigned int height;
  DWORD fourcc;

  virtual void Clear()
  {
    SStreamInfos::Clear();

    width = 0;
    height = 0;
    fourcc = 0;
  }
};

struct SSubtitleStreamInfos: SStreamInfos
{
  CStdString encoding;
  bool external;

  virtual void Clear()
  {
    SStreamInfos::Clear();

    encoding = "";
    external = false;
  }
};

struct SExternalSubtitleInfos: SSubtitleStreamInfos
{
  CStdString path;

  virtual void Clear()
  {
    SSubtitleStreamInfos::Clear();

    path = "";
  }
};

class CStreamsManager
{
public:
  static CStreamsManager *m_pSingleton;
  static CStreamsManager *getSingleton();
  static void Destroy();

  std::vector<SAudioStreamInfos *> GetAudios();
  std::vector<SSubtitleStreamInfos *> GetSubtitles();

  int  GetAudioStreamCount();
  int  GetAudioStream();
  void GetAudioStreamName(int iStream, CStdString &strStreamName);
  void SetAudioStream(int iStream);
  bool IsChangingStream();

  int  GetSubtitleCount();
  int  GetSubtitle();
  void GetSubtitleName(int iStream, CStdString &strStreamName);
  void SetSubtitle(int iStream);
  bool GetSubtitleVisible();
  void SetSubtitleVisible( bool bVisible );
  void SetSubtitleDelay(float fValue = 0.0f);
  float GetSubtitleDelay(void);
  bool AddSubtitle(const CStdString& subFilePath);
  
  void LoadStreams();
  IAMStreamSelect *GetStreamSelector() { return m_pIAMStreamSelect; }

  int GetChannels();
  int GetBitsPerSample();
  int GetSampleRate();

  int GetPictureWidth();
  int GetPictureHeight();

  CStdString GetAudioCodecName();
  CStdString GetVideoCodecName();

  bool InitManager(IFilterGraph2 *graphBuilder, CDSGraph *DSGraph);

  void GetStreamInfos(AM_MEDIA_TYPE *mt, SStreamInfos *s);

private:
  CStreamsManager(void);
  ~CStreamsManager(void);

  void SetStreamInternal(int iStream, SStreamInfos * s);
  void UnconnectSubtitlePins(void);

  std::vector<SAudioStreamInfos *> m_audioStreams;
  std::vector<SSubtitleStreamInfos *> m_subtitleStreams;
  IAMStreamSelect *m_pIAMStreamSelect;

  IFilterGraph2* m_pGraphBuilder;
  CDSGraph* m_pGraph;
  IBaseFilter* m_pSplitter;

  bool m_init;
  bool m_bChangingAudioStream;
  bool m_bSubtitlesVisible;

  SVideoStreamInfos m_videoStream;
  bool m_bSubtitlesUnconnected;

  IPin* m_SubtitleInputPin;
};
