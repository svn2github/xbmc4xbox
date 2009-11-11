#pragma once

/*
 *      Copyright (C) 2005-2009 Team XBMC
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

#include "StdString.h"
#include "StringUtils.h"
#define NO_DSHOW_STRSAFE
#include <dshow.h> //needed for CLSID_VideoRenderer

#define _ATL_EX_CONVERSION_MACROS_ONLY
#include <atlbase.h>
#include <atlstr.h>

#include <initguid.h>
#include <dvdmedia.h>
#include <strmif.h>
#include <mtype.h>
#include <wxdebug.h>
#include <combase.h>
#include "util.h"
#include "subpic/isubpic.h"
#include "igraphbuilder2.h"
#include "filters/dx9allocatorpresenter.h"
#include "Filters/IMPCVideoDecFilter.h"


#include <atlcoll.h>

#ifdef HAS_VIDEO_PLAYBACK
  #include "cores/VideoRenderers/RenderManager.h"
#endif




#include "File.h"

using namespace XFILE;

#define WM_GRAPHEVENT	WM_USER + 13

#define TIME_FORMAT_TO_MS 10000      // 10 ^ 4

enum VideoStateMode { MOVIE_NOTOPENED = 0x00,
                  MOVIE_OPENED    = 0x01,
                  MOVIE_PLAYING   = 0x02,
                  MOVIE_STOPPED   = 0x03,
                  MOVIE_PAUSED    = 0x04 };

class CDSGraph
{
public:
  CDSGraph();
  virtual ~CDSGraph();
  void Update(bool bPauseDrawing)                               { g_renderManager.Update(bPauseDrawing); }
  void GetVideoRect(CRect& SrcRect, CRect& DestRect)            { g_renderManager.GetVideoRect(SrcRect, DestRect); }
  virtual bool CanSeek();
  float GetAspectRatio()                                        { return g_renderManager.GetAspectRatio(); }
  void SetDynamicRangeCompression(long drc);
  
  bool InitializedOutputDevice();

  virtual void ProcessDsWmCommand(WPARAM wParam, LPARAM lParam);
  virtual HRESULT HandleGraphEvent();
  virtual double GetPlaySpeed();
  virtual void SetPlaySpeed(int iSpeed);
  virtual void DoFFRW(int currentSpeed);
  virtual void Seek(bool bPlus, bool bLargeStep);
  virtual void Pause();
  virtual void UpdateTime();
  virtual void UpdateState();
  virtual void UpdateCurrentVideoInfo(CStdString currentFile);
  virtual __int64 GetTime();
  virtual int GetTotalTime();
  __int64 GetTotalTimeInMsec();
  virtual float GetPercentage();

  
  HRESULT SetFile(const CFileItem& file);
  void OnPlayStop();
  void CloseFile();
  HRESULT CloseGraph();
  
  

//USER ACTIONS
  void SetVolume(long nVolume);

//INFORMATION REQUESTED FOR THE GUI
  std::string GetGeneralInfo();
  std::string GetAudioInfo() { return m_VideoInfo.codec_audio.c_str(); };
  std::string GetVideoInfo() { return m_VideoInfo.codec_video.c_str(); };

  
 
protected:
  bool m_bAbortRequest;
  bool m_bAllowFullscreen;
  CStdString m_Filename;
  
  int m_PlaybackRate;
  CFile m_File;
  
  
  DWORD_PTR g_userId;

  float m_fFrameRate;
  struct SPlayerState
  {
    void Clear()
    {
      timestamp     = 0;
      time    = 0;
      time_total      = 0;
      player_state  = "";
    }
	double timestamp;         // last time of update

    double time;              // current playback time
    double time_total;        // total playback time
    FILTER_STATE current_filter_state;

    std::string player_state;  // full player state
  } m_State;
  struct SVideoInfo
  {
    void Clear()
    {
      time_total    = 0;
      codec_video   = "";
      codec_audio   = "";
	  dxva_info     ="";
	  time_format   = GUID_NULL;
    }
    double time_total;        // total playback time
    CStdString codec_video;
    CStdString codec_audio;
    CStdString dxva_info;
	GUID time_format;
  } m_VideoInfo;

private:
  VideoStateMode                    m_pMode;
  
  //Direct Show Filters
  CComPtr<IGraphBuilder2>               m_pGraphBuilder;
  CComQIPtr<IMediaControl>              m_pMediaControl;  
  CComQIPtr<IMediaEventEx>              m_pMediaEvent;
  CComQIPtr<IMediaSeeking>              m_pMediaSeeking;
  CComQIPtr<IBasicAudio>                m_pBasicAudio;
  CComQIPtr<IBasicVideo2>               m_pBasicVideo;
  CComPtr<IMPCVideoDecFilter>	        m_pIMpcDecFilter;
  CComPtr<ISubPicAllocatorPresenter>    m_pAllocator;
protected:
  
  
};
