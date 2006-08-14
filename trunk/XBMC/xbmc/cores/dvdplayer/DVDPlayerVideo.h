
#pragma once

#include "../../utils/thread.h"
#include "DVDMessageQueue.h"
#include "DVDDemuxers\DVDDemuxUtils.h"
#include "DVDCodecs\Video\DVDVideoCodec.h"
#include "DVDClock.h"
#include "DVDOverlayContainer.h"
#include "../VideoRenderers/RenderManager.h"

enum CodecID;
class CDemuxStreamVideo;
class CDVDOverlayCodecCC;

#define VIDEO_PICTURE_QUEUE_SIZE 1

class CDVDPlayerVideo : public CThread
{
public:
  CDVDPlayerVideo(CDVDClock* pClock, CDVDOverlayContainer* pOverlayContainer);
  virtual ~CDVDPlayerVideo();

  bool OpenStream(CDVDStreamInfo &hint);
  void CloseStream(bool bWaitForBuffers);

  void Pause();
  void Resume();
  void Flush();

  // waits untill all available data has been rendered
  // just waiting for packetqueue should be enough for video
  void WaitForBuffers()                             { m_messageQueue.WaitUntilEmpty(); }
  bool AcceptsData()                                { return !m_messageQueue.IsFull(); }
  void SendMessage(CDVDMsg* pMsg)                   { m_messageQueue.Put(pMsg); }
  
  void Update(bool bPauseDrawing)                   { g_renderManager.Update(bPauseDrawing); }
  void UpdateMenuPicture();
 
  void EnableSubtitle(bool bEnable)                 { m_bRenderSubs = bEnable; }
  void GetVideoRect(RECT& SrcRect, RECT& DestRect)  { g_renderManager.GetVideoRect(SrcRect, DestRect); }
  float GetAspectRatio()                            { return g_renderManager.GetAspectRatio(); }

  __int64 GetDelay();
  void SetDelay(__int64 delay);

  bool IsStalled()                                  { return m_DetectedStill;  }
  int GetNrOfDroppedFrames()                        { return m_iDroppedFrames; }

  bool InitializedOutputDevice();
  
  __int64 GetCurrentPts()                           { return m_iCurrentPts; }

  __int64 GetOutputDelay(); /* returns the expected delay, from that a packet is put in queue */

  void SetSpeed(int iSpeed);

  // classes
  CDVDMessageQueue m_messageQueue;
  CDVDOverlayContainer* m_pOverlayContainer;
  
  CDVDClock* m_pClock;

protected:  
  virtual void OnStartup();
  virtual void OnExit();
  virtual void Process();

  enum EOUTPUTSTATUS
  {
    EOS_OK=0,
    EOS_ABORT=1,
    EOS_DROPPED_VERYLATE=2,
    EOS_DROPPED=3,
  };

  EOUTPUTSTATUS OutputPicture(DVDVideoPicture* pPicture, __int64 pts);
  void ProcessOverlays(DVDVideoPicture* pSource, YV12Image* pDest, __int64 pts);
  void ProcessVideoUserData(DVDVideoUserData* pVideoUserData, __int64 pts);
  
  __int64 m_iCurrentPts; // last pts displayed
  __int64 m_iVideoDelay; // not really needed to be an __int64  
  __int64 m_iFlipTimeStamp; // time stamp of last flippage. used to play at a forced framerate

  int m_iDroppedFrames;
  bool m_bInitializedOutputDevice;  
  float m_fFrameRate;

  struct SOutputConfiguration
  {
    unsigned int width;
    unsigned int height;
    unsigned int dwidth;
    unsigned int dheight;
    float framerate;
  } m_output; //holds currently configured output

  
  bool m_bRenderSubs;
  
  float m_fForcedAspectRatio;
  
  int m_iNrOfPicturesNotToSkip;
  int m_speed;

  bool m_DetectedStill;

  /* autosync decides on how much of clock we should use when deciding sleep time */
  /* the value is the same as 63% timeconstant, ie that the step response of */
  /* iSleepTime will be at 63% of iClockSleep after autosync frames */
  unsigned int m_autosync;
  
  // classes
  CDVDDemuxSPU* m_pDVDSpu;
  CDVDVideoCodec* m_pVideoCodec;
  CDVDOverlayCodecCC* m_pOverlayCodecCC;
  
  DVDVideoPicture* m_pTempOverlayPicture;
  
  CRITICAL_SECTION m_critCodecSection;
};

