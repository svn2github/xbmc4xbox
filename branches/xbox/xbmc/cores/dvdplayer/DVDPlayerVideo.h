#pragma once

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

#include "../../utils/Thread.h"
#include "DVDMessageQueue.h"
#include "DVDDemuxers/DVDDemuxUtils.h"
#include "DVDCodecs/Video/DVDVideoCodec.h"
#include "DVDClock.h"
#include "DVDOverlayContainer.h"
#ifdef HAS_VIDEO_PLAYBACK
#include "cores/VideoRenderers/RenderManager.h"
#endif

enum CodecID;
class CDemuxStreamVideo;
class CDVDOverlayCodecCC;

#define VIDEO_PICTURE_QUEUE_SIZE 1

class CDVDPlayerVideo : public CThread
{
public:
  CDVDPlayerVideo( CDVDClock* pClock
                 , CDVDOverlayContainer* pOverlayContainer
                 , CDVDMessageQueue& parent);
  virtual ~CDVDPlayerVideo();

  bool OpenStream(CDVDStreamInfo &hint);
  void CloseStream(bool bWaitForBuffers);

  void StepFrame();
  void Flush();

  // waits until all available data has been rendered
  // just waiting for packetqueue should be enough for video
  void WaitForBuffers()                             { m_messageQueue.WaitUntilEmpty(); }
  bool AcceptsData()                                { return !m_messageQueue.IsFull(); }
  void SendMessage(CDVDMsg* pMsg)                   { m_messageQueue.Put(pMsg); }

#ifdef HAS_VIDEO_PLAYBACK
  void Update(bool bPauseDrawing)                   { g_renderManager.Update(bPauseDrawing); }
#else
  void Update(bool bPauseDrawing)                   { }
#endif
  void UpdateMenuPicture();
 
  void EnableSubtitle(bool bEnable)                 { m_bRenderSubs = bEnable; }
  bool IsSubtitleEnabled()                          { return m_bRenderSubs; }

  void EnableFrameDrop(bool bEnabled)               { m_bDropFrames = bEnabled; }
  bool IsFrameDropEnabled()                         { return m_bDropFrames; }

  void EnableFullscreen(bool bEnable)               { m_bAllowFullscreen = bEnable; }

#ifdef HAS_VIDEO_PLAYBACK
  void GetVideoRect(RECT& SrcRect, RECT& DestRect)  { g_renderManager.GetVideoRect(SrcRect, DestRect); }
  float GetAspectRatio()                            { return g_renderManager.GetAspectRatio(); }
#else
  void GetVideoRect(RECT& SrcRect, RECT& DestRect)  { }
  float GetAspectRatio()                            { return 4.0f / 3.0f; }
#endif

  double GetDelay()                                { return m_iVideoDelay; }
  void SetDelay(double delay)                      { m_iVideoDelay = delay; }

  double GetSubtitleDelay()                                { return m_iSubtitleDelay; }
  void SetSubtitleDelay(double delay)                      { m_iSubtitleDelay = delay; }

  bool IsStalled()                                  { return m_stalled
                                                          && m_messageQueue.GetDataSize() == 0; }
  int GetNrOfDroppedFrames()                        { return m_iDroppedFrames; }

  bool InitializedOutputDevice();
  
  double GetCurrentPts()                           { return m_iCurrentPts; }

  double GetOutputDelay(); /* returns the expected delay, from that a packet is put in queue */
  std::string GetPlayerInfo();
  int GetVideoBitrate();

  void SetSpeed(int iSpeed);

  // classes
  CDVDMessageQueue m_messageQueue;
  CDVDMessageQueue& m_messageParent;

  CDVDOverlayContainer* m_pOverlayContainer;
  
  CDVDClock* m_pClock;

protected:  
  virtual void OnStartup();
  virtual void OnExit();
  virtual void Process();

#define EOS_ABORT 1
#define EOS_DROPPED 2
#define EOS_VERYLATE 4

  int OutputPicture(DVDVideoPicture* pPicture, double pts);
#ifdef HAS_VIDEO_PLAYBACK
  void ProcessOverlays(DVDVideoPicture* pSource, YV12Image* pDest, double pts);
#endif
  void ProcessVideoUserData(DVDVideoUserData* pVideoUserData, double pts);
  
  double m_iCurrentPts; // last pts displayed
  double m_iVideoDelay;
  double m_iSubtitleDelay;
  double m_FlipTimeStamp; // time stamp of last flippage. used to play at a forced framerate

  int m_iDroppedFrames;
  bool m_bDropFrames;

  float m_fFrameRate;

  struct SOutputConfiguration
  {
    unsigned int width;
    unsigned int height;
    unsigned int dwidth;
    unsigned int dheight;
    unsigned int color_matrix : 4;
    unsigned int color_range  : 1;
    float        framerate;
    bool         inited;
  } m_output; //holds currently configured output

  bool m_bAllowFullscreen;
  bool m_bRenderSubs;
  
  float m_fForcedAspectRatio;
  
  int m_iNrOfPicturesNotToSkip;
  int m_speed;

  double m_droptime;
  double m_dropbase;

  bool m_stalled;
  bool m_started;
  std::string m_codecname;

  /* autosync decides on how much of clock we should use when deciding sleep time */
  /* the value is the same as 63% timeconstant, ie that the step response of */
  /* iSleepTime will be at 63% of iClockSleep after autosync frames */
  unsigned int m_autosync;

  BitstreamStats m_videoStats;
  
  // classes
  CDVDStreamInfo m_hints;
  CDVDVideoCodec* m_pVideoCodec;
  CDVDOverlayCodecCC* m_pOverlayCodecCC;
  
  DVDVideoPicture* m_pTempOverlayPicture;
  
  CRITICAL_SECTION m_critCodecSection;
};

