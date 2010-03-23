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

#include <dshow.h> //needed for CLSID_VideoRenderer
#include <initguid.h>
#include <dvdmedia.h>
#include <strmif.h>
#include <mtype.h>
#include <wxdebug.h>
#include <combase.h>
#include "util.h"
#include "DSClock.h"
#include "dsconfig.h"
#include "fgmanager.h"
#include "DShowUtil/DShowUtil.h"


#ifdef HAS_VIDEO_PLAYBACK
  #include "cores/VideoRenderers/RenderManager.h"
#endif

#include "cores/IPlayer.h"
#include "File.h"



using namespace XFILE;

#define WM_GRAPHEVENT  WM_USER + 13


/** Video state mode */
enum VideoStateMode { MOVIE_NOTOPENED = 0x00,
                  MOVIE_OPENED    = 0x01,
                  MOVIE_PLAYING   = 0x02,
                  MOVIE_STOPPED   = 0x03,
                  MOVIE_PAUSED    = 0x04 };

class CFGManager;

/** @brief Graph management helper
 *
 * Provide function for playing, pausing, stopping, handling the graph
 */
class CDSGraph
{
public:
  CDSGraph(CDSClock* pClock);
  virtual ~CDSGraph();

  /** Determine if the graph can seek
   * @return True is the graph can seek, false else
   * @remarks If True, it means that the graph can seek forward, backward and absolute
   */
  virtual bool CanSeek();
  //void SetDynamicRangeCompression(long drc);
  
  bool InitializedOutputDevice();

  virtual void ProcessDsWmCommand(WPARAM wParam, LPARAM lParam);
  virtual HRESULT HandleGraphEvent();
  /** @return True if the file reached end, false else */
  bool FileReachedEnd(){ return m_bReachedEnd; };

  /** @return True if the graph is paused, false else */
  virtual bool IsPaused() const;
  
  virtual bool IsDvd() { return m_VideoInfo.isDVD; };
  bool IsInMenu() const;
  bool OnMouseMove(tagPOINT pt);
  bool OnMouseClick(tagPOINT pt);
  /** @return Current play speed */
  virtual int GetPlaySpeed() { return (m_currentRate * 1000); };

  /** Perform a Fast Forward
   * @param[in] currentSpeed Fast Forward speed
   */
  virtual int DoFFRW(int currentRate);
  /** Performs a Seek
   * @param[in] bPlus If true, performs a positive seek. If false, performs a negative seek
   * @param[in] bLargeStep If true, performs a large seek
   */
  virtual void Seek(bool bPlus, bool bLargeStep);
  /** Performs a Seek
   * @param[in] sec Time to seek (in millisecond)
   */
  virtual void SeekInMilliSec(double sec);
  /// Play the graph
  virtual void Play();
  /// Pause the graph
  virtual void Pause();
  /** Stop the graph
   * @param[in] rewind If true, the graph is rewinded
   */
  virtual void Stop(bool rewind = false);
  /// Update total playing time
  virtual void UpdateTotalTime();
  /// Update current playing time
  virtual void UpdateTime();
  /// Update Dvd state
  virtual void UpdateDvdState();
  /// Update current player state
  virtual void UpdateState();
  /// @return Current playing time
  virtual __int64 GetTime();
  /// @return Total playing time in second (media length)
  virtual int GetTotalTime();
  /// @return Total playing time in millisecond (media length)
  __int64 GetTotalTimeInMsec();
  /// @return Current position in percent
  virtual float GetPercentage();

  /** Create the graph and open the file
   * @param[in] file File to play
   * @param[in] options Playing options
   * @return An HRESULT code
   */
  HRESULT SetFile(const CFileItem& file, const CPlayerOptions &options);
  /** Close the file, clean the graph and free resources */
  void CloseFile();

  /** Sets the volume (amplitude) of the audio signal 
   * @param[in] nVolume The volume level in 1/100ths dB Valid values range from -10,000 (silence) to 0 (full volume) 0 = 0 dB -10000 = -100 dB
   */
  void SetVolume(long nVolume);

  /// @return General informations about filters
  CStdString GetGeneralInfo();
  /// @return Informations about the current audio track
  CStdString GetAudioInfo();
  /// @return Informations about the current video track
  CStdString GetVideoInfo();

  CDSClock* m_pDsClock;

  static Com::SmartPtr<IFilterGraph2> m_pFilterGraph;
protected:

  /** Unload the graph and release all the filters
   * @return A HRESULT code
   */
  HRESULT UnloadGraph();
private:
  //Direct Show Filters
  CFGManager*                           m_pGraphBuilder;
  Com::SmartQIPtr<IMediaControl>        m_pMediaControl;  
  Com::SmartQIPtr<IMediaEventEx>        m_pMediaEvent;
  Com::SmartQIPtr<IMediaSeeking>        m_pMediaSeeking;
  Com::SmartQIPtr<IBasicAudio>          m_pBasicAudio;
  //dvd interface
  Com::SmartQIPtr<IVideoWindow>         m_pVideoWindow;
  Com::SmartQIPtr<IDvdInfo2>            m_pDvdInfo2;
  Com::SmartQIPtr<IDvdControl2>         m_pDvdControl2;
  Com::SmartPtr<IDvdState>              m_pDvdState;
  DVD_STATUS	                          m_pDvdStatus;
  std::vector<DvdTitle*>                m_pDvdTitles;
  bool m_bReachedEnd;
  int m_currentRate;
  int m_lAvgTimeToSeek;

  DWORD_PTR m_userId;
  CCriticalSection m_ObjectLock;
  CStdString m_pStrCurrentFrameRate;
  int m_iCurrentFrameRefreshCycle;

  struct SPlayerState
  {
    SPlayerState()
    {
      Clear();
    }

    void Clear()
    {
      timestamp = 0;
      time = 0;
      time_total = 0;
      player_state = "";
      current_filter_state = State_Stopped;
    }
    LONGLONG timestamp;         // last time of update

    LONGLONG time;              // current playback time in millisec
    LONGLONG time_total;        // total playback time in millisec
    FILTER_STATE current_filter_state;

    std::string player_state;  // full player state
  } m_State;
  
  struct SDvdState
  {
    void Clear()
    {
      isInMenu = false;
    }
    bool isInMenu;
  } m_DvdState;

  struct SVideoInfo
  {
    SVideoInfo()
    {
      Clear();
    }
    void Clear()
    {
      time_total    = 0;
      time_format   = GUID_NULL;
      isDVD = false;
    }
    double time_total;        // total playback time
    GUID time_format;
    bool isDVD;
  } m_VideoInfo;
};
