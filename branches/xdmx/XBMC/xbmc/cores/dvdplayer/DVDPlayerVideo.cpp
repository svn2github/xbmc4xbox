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

#include "stdafx.h"
#include "Settings.h"
#include "VideoReferenceClock.h"
#include "DVDPlayer.h"
#include "DVDPlayerVideo.h"
#include "DVDCodecs/DVDFactoryCodec.h"
#include "DVDCodecs/DVDCodecUtils.h"
#include "DVDCodecs/Video/DVDVideoPPFFmpeg.h"
#include "DVDCodecs/Video/DVDVideoCodecFFmpeg.h"
#include "DVDDemuxers/DVDDemux.h"
#include "DVDDemuxers/DVDDemuxUtils.h"
#include "../../Util.h"
#include "DVDOverlayRenderer.h"
#include "DVDPerformanceCounter.h"
#include "DVDCodecs/DVDCodecs.h"
#include "DVDCodecs/Overlay/DVDOverlayCodecCC.h"
#include "DVDCodecs/Overlay/DVDOverlaySSA.h"
#include <sstream>
#include <iomanip>
#include <numeric>

using namespace std;

class CPulldownCorrection
{
public:
  CPulldownCorrection()
  {
    m_duration = 0.0;
    m_accum    = 0;
    m_total    = 0;
    m_next     = m_pattern.end();
  }

  void init(double fps, int *begin, int *end)
  {
    std::copy(begin, end, std::back_inserter(m_pattern));
    m_duration = DVD_TIME_BASE / fps;
    m_accum    = 0;
    m_total    = std::accumulate(m_pattern.begin(), m_pattern.end(), 0);
    m_next     = m_pattern.begin();
  }

  double pts()
  {
    double input  = m_duration * std::distance(m_pattern.begin(), m_next);
    double output = m_duration * m_accum / m_total;
    return output - input;
  }

  double dur()
  {
    return m_duration * m_pattern.size() * *m_next / m_total;
  }

  void next()
  {
    m_accum += *m_next;
    if(++m_next == m_pattern.end())
    {
      m_next  = m_pattern.begin();
      m_accum = 0;
    }
  }

  bool enabled()
  {
    return m_pattern.size() > 0;
  }
private:
  double                     m_duration;
  int                        m_total;
  int                        m_accum;
  std::vector<int>           m_pattern;
  std::vector<int>::iterator m_next;
};


CDVDPlayerVideo::CDVDPlayerVideo( CDVDClock* pClock
                                , CDVDOverlayContainer* pOverlayContainer
                                , CDVDMessageQueue& parent)
: CThread()
, m_messageQueue("video")
, m_messageParent(parent)
{
  m_pClock = pClock;
  m_pOverlayContainer = pOverlayContainer;
  m_pTempOverlayPicture = NULL;
  m_pVideoCodec = NULL;
  m_pOverlayCodecCC = NULL;
  m_speed = DVD_PLAYSPEED_NORMAL;

  m_bRenderSubs = false;
  m_stalled = false;
  m_started = false;
  m_iVideoDelay = 0;
  m_iSubtitleDelay = 0;
  m_fForcedAspectRatio = 0;
  m_iNrOfPicturesNotToSkip = 0;
  InitializeCriticalSection(&m_critCodecSection);
  m_messageQueue.SetMaxDataSize(40 * 256 * 1024);
  g_dvdPerformanceCounter.EnableVideoQueue(&m_messageQueue);

  m_iCurrentPts = DVD_NOPTS_VALUE;
  m_iDroppedFrames = 0;
  m_fFrameRate = 25;
  m_bAllowFullscreen = false;
  memset(&m_output, 0, sizeof(m_output));
}

CDVDPlayerVideo::~CDVDPlayerVideo()
{
  StopThread();
  g_dvdPerformanceCounter.DisableVideoQueue();
  DeleteCriticalSection(&m_critCodecSection);
  g_VideoReferenceClock.StopThread();
}

double CDVDPlayerVideo::GetOutputDelay()
{
    double time = m_messageQueue.GetPacketCount(CDVDMsg::DEMUXER_PACKET);
    if( m_fFrameRate )
      time = (time * DVD_TIME_BASE) / m_fFrameRate;
    else
      time = 0.0;

    if( m_speed != 0 )
      time = time * DVD_PLAYSPEED_NORMAL / abs(m_speed);

    return time;
}

bool CDVDPlayerVideo::OpenStream( CDVDStreamInfo &hint )
{

  if (hint.fpsrate && hint.fpsscale)
  {
    m_fFrameRate = (float)hint.fpsrate / hint.fpsscale;
    m_autosync = 10;
  }
  else
  {
    m_fFrameRate = 25;
    m_autosync = 1; // avoid using frame time as we don't know it accurate
  }

  if (hint.vfr)
    m_autosync = 1;

  if( m_fFrameRate > 100 || m_fFrameRate < 5 )
  {
    CLog::Log(LOGERROR, "CDVDPlayerVideo::OpenStream - Invalid framerate %d, using forced 25fps and just trust timestamps", (int)m_fFrameRate);
    m_fFrameRate = 25;
    m_autosync = 1; // avoid using frame time as we don't know it accurate
  }

  // use aspect in stream if available
  m_fForcedAspectRatio = hint.aspect;

  // should alway's be NULL!!!!, it will probably crash anyway when deleting m_pVideoCodec here.
  if (m_pVideoCodec)
  {
    CLog::Log(LOGFATAL, "CDVDPlayerVideo::OpenStream() m_pVideoCodec != NULL");
    return false;
  }

  CLog::Log(LOGNOTICE, "Creating video codec with codec id: %i", hint.codec);
  m_pVideoCodec = CDVDFactoryCodec::CreateVideoCodec( hint );

  if( !m_pVideoCodec )
  {
    CLog::Log(LOGERROR, "Unsupported video codec");
    return false;
  }

  m_stalled = false;
  m_started = false;
  m_codecname = m_pVideoCodec->GetName();

  m_messageQueue.Init();

  if(g_guiSettings.GetBool("videoplayer.usedisplayasclock") && g_VideoReferenceClock.ThreadHandle() == NULL)
  {
    g_VideoReferenceClock.Create();
    //we have to wait for the clock to start otherwise alsa can cause trouble
    if (!g_VideoReferenceClock.WaitStarted(2000))
      CLog::Log(LOGDEBUG, "g_VideoReferenceClock didn't start in time");
  }

  CLog::Log(LOGNOTICE, "Creating video thread");
  Create();

  return true;
}

void CDVDPlayerVideo::CloseStream(bool bWaitForBuffers)
{
  // wait until buffers are empty
  if (bWaitForBuffers && m_speed > 0) m_messageQueue.WaitUntilEmpty();

  m_messageQueue.Abort();

  // wait for decode_video thread to end
  CLog::Log(LOGNOTICE, "waiting for video thread to exit");

  StopThread(); // will set this->m_bStop to true

  m_messageQueue.End();

  CLog::Log(LOGNOTICE, "deleting video codec");
  if (m_pVideoCodec)
  {
    m_pVideoCodec->Dispose();
    delete m_pVideoCodec;
    m_pVideoCodec = NULL;
  }

  if (m_pTempOverlayPicture)
  {
    CDVDCodecUtils::FreePicture(m_pTempOverlayPicture);
    m_pTempOverlayPicture = NULL;
  }

  //tell the clock we stopped playing video
  m_pClock->UpdateFramerate(0.0);
}

void CDVDPlayerVideo::OnStartup()
{
  CThread::SetName("CDVDPlayerVideo");
  m_iDroppedFrames = 0;

  m_iCurrentPts = DVD_NOPTS_VALUE;
  m_FlipTimeStamp = m_pClock->GetAbsoluteClock();

#ifdef HAS_VIDEO_PLAYBACK
  if(!m_output.inited)
  {
    g_renderManager.PreInit();
    m_output.inited = true;
  }
#endif
  g_dvdPerformanceCounter.EnableVideoDecodePerformance(ThreadHandle());
}

void CDVDPlayerVideo::Process()
{
  CLog::Log(LOGNOTICE, "running thread: video_thread");

  DVDVideoPicture picture;
  CDVDVideoPPFFmpeg mDeinterlace(g_advancedSettings.m_videoPPFFmpegType);
  CPulldownCorrection pulldown;

  memset(&picture, 0, sizeof(DVDVideoPicture));

  double pts = 0;
  double frametime = (double)DVD_TIME_BASE / m_fFrameRate;

  int iDropped = 0; //frames dropped in a row
  bool bRequestDrop = false;

  m_videoStats.Start();

  while (!m_bStop)
  {
    int iQueueTimeOut = (int)(m_stalled ? frametime / 4 : frametime * 10) / 1000;
    int iPriority = (m_speed == DVD_PLAYSPEED_PAUSE && m_iNrOfPicturesNotToSkip == 0) ? 1 : 0;

    CDVDMsg* pMsg;
    MsgQueueReturnCode ret = m_messageQueue.Get(&pMsg, iQueueTimeOut, iPriority);

    if (MSGQ_IS_ERROR(ret) || ret == MSGQ_ABORT)
    {
      CLog::Log(LOGERROR, "Got MSGQ_ABORT or MSGO_IS_ERROR return true");
      break;
    }
    else if (ret == MSGQ_TIMEOUT)
    {
      // if we only wanted priority messages, this isn't a stall
      if( iPriority )
        continue;

      //Okey, start rendering at stream fps now instead, we are likely in a stillframe
      if( !m_stalled && m_started )
      {
        CLog::Log(LOGINFO, "CDVDPlayerVideo - Stillframe detected, switching to forced %f fps", m_fFrameRate);
        m_stalled = true;
        pts+= frametime*4;
      }

      //Waiting timed out, output last picture
      if( picture.iFlags & DVP_FLAG_ALLOCATED )
      {
        //Remove interlaced flag before outputting
        //no need to output this as if it was interlaced
        picture.iFlags &= ~DVP_FLAG_INTERLACED;
        picture.iFlags |= DVP_FLAG_NOSKIP;
        OutputPicture(&picture, pts);
        pts+= frametime;
      }

      continue;
    }

    CDVDMsg::Message message_type = pMsg->GetMessageType();
    switch (message_type) {
      case CDVDMsg::GENERAL_SYNCHRONIZE:
      {
        ((CDVDMsgGeneralSynchronize*)pMsg)->Wait( &m_bStop, SYNCSOURCE_VIDEO );
        CLog::Log(LOGDEBUG, "CDVDPlayerVideo - CDVDMsg::GENERAL_SYNCHRONIZE");
        /* we may be very much off correct pts here, but next picture may be a still*/
        /* make sure it isn't dropped */
        m_iNrOfPicturesNotToSkip = 5;
      }
      break;
      case CDVDMsg::GENERAL_RESYNC:
      {
        CDVDMsgGeneralResync* pMsgGeneralResync = (CDVDMsgGeneralResync*)pMsg;

        if(pMsgGeneralResync->m_timestamp != DVD_NOPTS_VALUE)
          pts = pMsgGeneralResync->m_timestamp;

        double delay = m_FlipTimeStamp - m_pClock->GetAbsoluteClock();
        if( delay > frametime ) delay = frametime;
        else if( delay < 0 )    delay = 0;

        if(pMsgGeneralResync->m_clock)
        {
          CLog::Log(LOGDEBUG, "CDVDPlayerVideo - CDVDMsg::GENERAL_RESYNC(%f, 1)", pts);
          m_pClock->Discontinuity(CLOCK_DISC_NORMAL, pts, delay);
        }
        else
          CLog::Log(LOGDEBUG, "CDVDPlayerVideo - CDVDMsg::GENERAL_RESYNC(%f, 0)", pts);
      }
      break;
      case CDVDMsg::GENERAL_DELAY:
      {
        if (m_speed != DVD_PLAYSPEED_PAUSE)
        {
          double timeout = static_cast<CDVDMsgDouble*>(pMsg)->m_value;

          CLog::Log(LOGDEBUG, "CDVDPlayerVideo - CDVDMsg::GENERAL_DELAY(%f)", timeout);

          timeout *= (double)DVD_PLAYSPEED_NORMAL / abs(m_speed);
          timeout += CDVDClock::GetAbsoluteClock();

          while(!m_bStop && CDVDClock::GetAbsoluteClock() < timeout)
            Sleep(1);
        }
      }
      break;
      case CDVDMsg::VIDEO_SET_ASPECT:
      {
        CLog::Log(LOGDEBUG, "CDVDPlayerVideo - CDVDMsg::VIDEO_SET_ASPECT");
        m_fForcedAspectRatio = *((CDVDMsgDouble*)pMsg);
      }
      break;
      case CDVDMsg::GENERAL_FLUSH:
      {
        // private message sent by (CDVDPlayerVideo::Flush())
        EnterCriticalSection(&m_critCodecSection);
        if(m_pVideoCodec)
          m_pVideoCodec->Reset();
        LeaveCriticalSection(&m_critCodecSection);
        
        m_pullupCorrection.Flush();
      }
      break;
      case CDVDMsg::VIDEO_NOSKIP:
      {
        // libmpeg2 is also returning incomplete frames after a dvd cell change
        // so the first few pictures are not the correct ones to display in some cases
        // just display those together with the correct one.
        // (setting it to 2 will skip some menu stills, 5 is working ok for me).
        m_iNrOfPicturesNotToSkip = 5;
      }
      break;
      case CDVDMsg::PLAYER_SETSPEED:
      {
        m_speed = static_cast<CDVDMsgInt*>(pMsg)->m_value;
        if(m_speed == DVD_PLAYSPEED_PAUSE)
          m_iNrOfPicturesNotToSkip = 0;
      }
      break;
      case CDVDMsg::DEMUXER_PACKET:
      {
        DemuxPacket* pPacket = ((CDVDMsgDemuxerPacket*)pMsg)->GetPacket();
        bool bPacketDrop     = ((CDVDMsgDemuxerPacket*)pMsg)->GetPacketDrop();

        m_started = true;
        if (m_stalled)
        {
          CLog::Log(LOGINFO, "CDVDPlayerVideo - Stillframe left, switching to normal playback");
          m_stalled = false;

          //don't allow the first frames after a still to be dropped
          //sometimes we get multiple stills (long duration frames) after each other
          //in normal mpegs
          m_iNrOfPicturesNotToSkip = 5;
        }
        else if( iDropped*frametime > DVD_MSEC_TO_TIME(100) )
        { // if we dropped too many pictures in a row, insert a forced picture
          m_iNrOfPicturesNotToSkip++;
        }

#ifdef PROFILE
        bRequestDrop = false;
#else
        if (m_iNrOfPicturesNotToSkip > 0) bRequestDrop = false;
        if (m_speed < 0)                  bRequestDrop = false;
#endif

        // if player want's us to drop this packet, do so nomatter what
        if(bPacketDrop)
          bRequestDrop = true;


        EnterCriticalSection(&m_critCodecSection);
        // tell codec if next frame should be dropped
        // problem here, if one packet contains more than one frame
        // both frames will be dropped in that case instead of just the first
        // decoder still needs to provide an empty image structure, with correct flags
        m_pVideoCodec->SetDropState(bRequestDrop);

        int iDecoderState = m_pVideoCodec->Decode(pPacket->pData, pPacket->iSize, pPacket->pts);
        m_videoStats.AddSampleBytes(pPacket->iSize);
        // assume decoder dropped a picture if it didn't give us any
        // picture from a demux packet, this should be reasonable
        // for libavformat as a demuxer as it normally packetizes
        // pictures when they come from demuxer
        if(bRequestDrop && !bPacketDrop && (iDecoderState & VC_BUFFER))
        //if(bRequestDrop && !bPacketDrop && (iDecoderState & VC_BUFFER) && !(iDecoderState & VC_PICTURE))
        {
          m_iDroppedFrames++;
          iDropped++;
        }

        // loop while no error
        while (!m_bStop)
        {
          // if decoder was flushed, we need to seek back again to resume rendering
          if (iDecoderState & VC_FLUSHED)
          {
            CLog::Log(LOGDEBUG, "CDVDPlayerVideo - video decoder was flushed");
            m_messageParent.Put(new CDVDMsgPlayerSeek(pts/1000, true, true, true));
          }

          // if decoder had an error, tell it to reset to avoid more problems
          if (iDecoderState & VC_ERROR)
          {
            CLog::Log(LOGDEBUG, "CDVDPlayerVideo - video decoder returned error");
            m_pVideoCodec->Reset();
            break;
          }

          // check for a new picture
          if (iDecoderState & VC_PICTURE)
          {
            // try to retrieve the picture (should never fail!), unless there is a demuxer bug ofcours
            memset(&picture, 0, sizeof(DVDVideoPicture));
            if (m_pVideoCodec->GetPicture(&picture))
            {
              picture.iGroupId = pPacket->iGroupId;

              if(picture.iDuration == 0)
                picture.iDuration = frametime;

              if(bPacketDrop)
                picture.iFlags |= DVP_FLAG_DROPPED;

              if (m_iNrOfPicturesNotToSkip > 0)
              {
                picture.iFlags |= DVP_FLAG_NOSKIP;
                m_iNrOfPicturesNotToSkip--;
              }

              /* try to figure out a pts for this frame */
              if(picture.pts == DVD_NOPTS_VALUE && pPacket->dts != DVD_NOPTS_VALUE)
                picture.pts = pPacket->dts;

              /* use forced aspect if any */
              if( m_fForcedAspectRatio != 0.0f )
                picture.iDisplayWidth = (int) (picture.iDisplayHeight * m_fForcedAspectRatio);

              //Deinterlace if codec said format was interlaced or if we have selected we want to deinterlace
              //this video
              EINTERLACEMETHOD mInt = g_stSettings.m_currentVideoSettings.m_InterlaceMethod;
              if( mInt == VS_INTERLACEMETHOD_DEINTERLACE )
              {
                if(mDeinterlace.Process(&picture))
                  mDeinterlace.GetPicture(&picture);
              }
              else if( mInt == VS_INTERLACEMETHOD_RENDER_WEAVE 
                    || mInt == VS_INTERLACEMETHOD_RENDER_WEAVE_INVERTED )
              {
                /* if we are syncing frames, dvdplayer will be forced to play at a given framerate */
                /* unless we directly sync to the correct pts, we won't get a/v sync as video can never catch up */
                picture.iFlags |= DVP_FLAG_NOAUTOSYNC;
              }

              /* if frame has a pts (usually originiating from demux packet), use that */
              if(picture.pts != DVD_NOPTS_VALUE)
              {
                if(pulldown.enabled())
                  picture.pts += pulldown.pts();

                pts = picture.pts;
              }

              int iResult;
              do
              {
                if(pulldown.enabled())
                {
                  picture.iDuration = pulldown.dur();
                  pulldown.next();
                }

                try
                {
                  iResult = OutputPicture(&picture, pts);
                }
                catch (...)
                {
                  CLog::Log(LOGERROR, "%s - Exception caught when outputing picture", __FUNCTION__);
                  iResult = EOS_ABORT;
                }
                m_pVideoCodec->ReleasePicture(&picture);

                if (iResult == EOS_ABORT) break;

                // guess next frame pts. iDuration is always valid
                pts += picture.iDuration * m_speed / abs(m_speed);
              }
              while (!m_bStop && picture.iRepeatPicture-- > 0);

              if( iResult & EOS_ABORT )
              {
                //if we break here and we directly try to decode again wihout
                //flushing the video codec things break for some reason
                //i think the decoder (libmpeg2 atleast) still has a pointer
                //to the data, and when the packet is freed that will fail.
                iDecoderState = m_pVideoCodec->Decode(NULL, 0, DVD_NOPTS_VALUE);
                break;
              }

              if( (iResult & EOS_DROPPED) && !bPacketDrop )
              {
                m_iDroppedFrames++;
                iDropped++;
              }
              else
                iDropped = 0;

              bRequestDrop = (iResult & EOS_VERYLATE) == EOS_VERYLATE;
            }
            else
            {
              /*
              CLog::Log(LOGWARNING, "Decoder Error getting videoPicture.");
              m_pVideoCodec->Reset();
              */
            }
          }

          /*
          if (iDecoderState & VC_USERDATA)
          {
            // found some userdata while decoding a frame
            // could be closed captioning
            DVDVideoUserData videoUserData;
            if (m_pVideoCodec->GetUserData(&videoUserData))
            {
              ProcessVideoUserData(&videoUserData, pts);
            }
          }
          */

          // if the decoder needs more data, we just break this loop
          // and try to get more data from the videoQueue
          if (iDecoderState & VC_BUFFER)
            break;

          // the decoder didn't need more data, flush the remaning buffer
          iDecoderState = m_pVideoCodec->Decode(NULL, 0, DVD_NOPTS_VALUE);
        }


        LeaveCriticalSection(&m_critCodecSection);
      }
      break;

      default:
      break;
    }

    // all data is used by the decoder, we can safely free it now
    pMsg->Release();
  }
}

void CDVDPlayerVideo::OnExit()
{
  g_dvdPerformanceCounter.DisableVideoDecodePerformance();

  if (m_pOverlayCodecCC)
  {
    m_pOverlayCodecCC->Dispose();
    m_pOverlayCodecCC = NULL;
  }

  CLog::Log(LOGNOTICE, "thread end: video_thread");
}

void CDVDPlayerVideo::ProcessVideoUserData(DVDVideoUserData* pVideoUserData, double pts)
{
  // check userdata type
  BYTE* data = pVideoUserData->data;
  int size = pVideoUserData->size;

  if (size >= 2)
  {
    if (data[0] == 'C' && data[1] == 'C')
    {
      data += 2;
      size -= 2;

      // closed captioning
      if (!m_pOverlayCodecCC)
      {
        m_pOverlayCodecCC = new CDVDOverlayCodecCC();
        CDVDCodecOptions options;
        CDVDStreamInfo info;
        if (!m_pOverlayCodecCC->Open(info, options))
        {
          delete m_pOverlayCodecCC;
          m_pOverlayCodecCC = NULL;
        }
      }

      if (m_pOverlayCodecCC)
      {
        m_pOverlayCodecCC->Decode(data, size, DVD_NOPTS_VALUE, DVD_NOPTS_VALUE);

        CDVDOverlay* overlay;
        while((overlay = m_pOverlayCodecCC->GetOverlay()) != NULL)
        {
          overlay->iGroupId = 0;
          overlay->iPTSStartTime += pts;
          if(overlay->iPTSStopTime != 0.0)
            overlay->iPTSStopTime += pts;

          m_pOverlayContainer->Add(overlay);
          overlay->Release();
        }
      }
    }
  }
}

bool CDVDPlayerVideo::InitializedOutputDevice()
{
#ifdef HAS_VIDEO_PLAYBACK
  return g_renderManager.IsStarted();
#else
  return false;
#endif
}

void CDVDPlayerVideo::SetSpeed(int speed)
{
  if(m_messageQueue.IsInited())
    m_messageQueue.Put( new CDVDMsgInt(CDVDMsg::PLAYER_SETSPEED, speed), 1 );
  else
    m_speed = speed;
}

void CDVDPlayerVideo::StepFrame()
{
  m_iNrOfPicturesNotToSkip++;
}

void CDVDPlayerVideo::Flush()
{
  /* flush using message as this get's called from dvdplayer thread */
  /* and any demux packet that has been taken out of queue need to */
  /* be disposed of before we flush */
  m_messageQueue.Flush();
  m_messageQueue.Put(new CDVDMsg(CDVDMsg::GENERAL_FLUSH));
}

#ifdef HAS_VIDEO_PLAYBACK
void CDVDPlayerVideo::ProcessOverlays(DVDVideoPicture* pSource, YV12Image* pDest, double pts)
{
  // remove any overlays that are out of time
  m_pOverlayContainer->CleanUp(min(pts, pts - m_iSubtitleDelay));

  if(pSource->format == DVDVideoPicture::FMT_VDPAU)
    return;

  // rendering spu overlay types directly on video memory costs a lot of processing power.
  // thus we allocate a temp picture, copy the original to it (needed because the same picture can be used more than once).
  // then do all the rendering on that temp picture and finaly copy it to video memory.
  // In almost all cases this is 5 or more times faster!.
  bool bHasSpecialOverlay = m_pOverlayContainer->ContainsOverlayType(DVDOVERLAY_TYPE_SPU)
                         || m_pOverlayContainer->ContainsOverlayType(DVDOVERLAY_TYPE_IMAGE)
                         || m_pOverlayContainer->ContainsOverlayType(DVDOVERLAY_TYPE_SSA);

  if (bHasSpecialOverlay)
  {
    if (m_pTempOverlayPicture && (m_pTempOverlayPicture->iWidth != pSource->iWidth || m_pTempOverlayPicture->iHeight != pSource->iHeight))
    {
      CDVDCodecUtils::FreePicture(m_pTempOverlayPicture);
      m_pTempOverlayPicture = NULL;
    }

    if (!m_pTempOverlayPicture) m_pTempOverlayPicture = CDVDCodecUtils::AllocatePicture(pSource->iWidth, pSource->iHeight);
  }

  if (bHasSpecialOverlay && m_pTempOverlayPicture)
  {
    CDVDCodecUtils::CopyPicture(m_pTempOverlayPicture, pSource);
  }
  else
  {
    if (pSource->format == DVDVideoPicture::FMT_YUV420P)
    {
      CDVDCodecUtils::CopyPicture(pDest, pSource);
    }
    else
    {
      CDVDCodecUtils::CopyNV12Picture(pDest, pSource);
    }
  }
  m_pOverlayContainer->Lock();

  VecOverlays* pVecOverlays = m_pOverlayContainer->GetOverlays();
  VecOverlaysIter it = pVecOverlays->begin();

  //Check all overlays and render those that should be rendered, based on time and forced
  //Both forced and subs should check timeing, pts == 0 in the stillframe case
  while (it != pVecOverlays->end())
  {
    CDVDOverlay* pOverlay = *it++;
    if(!pOverlay->bForced && !m_bRenderSubs)
      continue;

    if(pOverlay->iGroupId != pSource->iGroupId)
      continue;

    double pts2 = pOverlay->bForced ? pts : pts - m_iSubtitleDelay;

    if((pOverlay->iPTSStartTime <= pts2 && (pOverlay->iPTSStopTime >= pts2 || pOverlay->iPTSStopTime == 0LL)) || pts == 0)
    {
      if (bHasSpecialOverlay && m_pTempOverlayPicture)
        CDVDOverlayRenderer::Render(m_pTempOverlayPicture, pOverlay, pts2);
      else
        CDVDOverlayRenderer::Render(pDest, pOverlay, pts2);
    }
  }

  m_pOverlayContainer->Unlock();

  if (bHasSpecialOverlay && m_pTempOverlayPicture)
    CDVDCodecUtils::CopyPicture(pDest, m_pTempOverlayPicture);
}
#endif

int CDVDPlayerVideo::OutputPicture(DVDVideoPicture* pPicture, double pts)
{
#ifdef HAS_VIDEO_PLAYBACK
  /* check so that our format or aspect has changed. if it has, reconfigure renderer */
  if (!g_renderManager.IsConfigured()
   || m_output.width != pPicture->iWidth
   || m_output.height != pPicture->iHeight
   || m_output.dwidth != pPicture->iDisplayWidth
   || m_output.dheight != pPicture->iDisplayHeight
   || m_output.framerate != m_fFrameRate
   || ( m_output.color_matrix != pPicture->color_matrix && pPicture->color_matrix != 0 ) // don't reconfigure on unspecified
   || m_output.color_range != pPicture->color_range)
  {
    CLog::Log(LOGNOTICE, " fps: %f, pwidth: %i, pheight: %i, dwidth: %i, dheight: %i",
      m_fFrameRate, pPicture->iWidth, pPicture->iHeight, pPicture->iDisplayWidth, pPicture->iDisplayHeight);
    unsigned flags = 0;
    if(pPicture->color_range == 1)
      flags |= CONF_FLAGS_YUV_FULLRANGE;

    switch(pPicture->color_matrix)
    {
      case 7: // SMPTE 240M (1987)
        flags |= CONF_FLAGS_YUVCOEF_240M;
        break;
      case 6: // SMPTE 170M
      case 5: // ITU-R BT.470-2
      case 4: // FCC
        flags |= CONF_FLAGS_YUVCOEF_BT601;
        break;
      case 3: // RESERVED
      case 2: // UNSPECIFIED
      case 1: // ITU-R Rec.709 (1990) -- BT.709
      default:
        flags |= CONF_FLAGS_YUVCOEF_BT709;
    }

    if(m_bAllowFullscreen)
    {
      flags |= CONF_FLAGS_FULLSCREEN;
      m_bAllowFullscreen = false; // only allow on first configure
    }

    CLog::Log(LOGDEBUG,"%s - change configuration. %dx%d. framerate: %4.2f",__FUNCTION__,pPicture->iWidth, pPicture->iHeight,m_fFrameRate);
    if(!g_renderManager.Configure(pPicture->iWidth, pPicture->iHeight, pPicture->iDisplayWidth, pPicture->iDisplayHeight, m_fFrameRate, flags))
    {
      CLog::Log(LOGERROR, "%s - failed to configure renderer", __FUNCTION__);
      return EOS_ABORT;
    }

    m_output.width = pPicture->iWidth;
    m_output.height = pPicture->iHeight;
    m_output.dwidth = pPicture->iDisplayWidth;
    m_output.dheight = pPicture->iDisplayHeight;
    m_output.framerate = m_fFrameRate;
    m_output.color_matrix = pPicture->color_matrix;
    m_output.color_range = pPicture->color_range;
  }

  double maxfps  = 60.0;
  bool   limited = false;
  int    result  = 0;

  if (!g_renderManager.IsStarted()) {
    CLog::Log(LOGERROR, "%s - renderer not started", __FUNCTION__);
    return EOS_ABORT;
  }
  maxfps = g_renderManager.GetMaximumFPS();

  // check if our output will limit speed
  if(m_fFrameRate * abs(m_speed) / DVD_PLAYSPEED_NORMAL > maxfps*0.9)
    limited = true;

  // signal to clock what our framerate is, it may want to adjust it's
  // speed to better match with our video renderer's output speed
  // TODO - this should be based on m_fFrameRate, as the timestamps
  //        in the stream matches that better, durations can vary
  int refreshrate = m_pClock->UpdateFramerate(1.0 / (pPicture->iDuration / DVD_TIME_BASE));
  if (refreshrate > 0) //refreshrate of -1 means the videoreferenceclock is not running
  {
    //correct any pattern in the timestamps if the videoreferenceclock is running
    m_pullupCorrection.Add(pts);
    pts += m_pullupCorrection.Correction();
    
    //when using the videoreferenceclock, a frame is always presented one vblank interval too late
    pts -= (1.0 / refreshrate) * DVD_TIME_BASE; 
  }
  else
  {
    m_pullupCorrection.Flush();
  }
  
  //User set delay
  pts += m_iVideoDelay;

  // calculate the time we need to delay this picture before displaying
  double iSleepTime, iClockSleep, iFrameSleep, iCurrentClock, iFrameDuration;

  iCurrentClock = m_pClock->GetAbsoluteClock(); // snapshot current clock
  iClockSleep = pts - m_pClock->GetClock();  //sleep calculated by pts to clock comparison
  iFrameSleep = m_FlipTimeStamp - iCurrentClock; // sleep calculated by duration of frame
  iFrameDuration = pPicture->iDuration;

  // correct sleep times based on speed
  if(m_speed)
  {
    iClockSleep = iClockSleep * DVD_PLAYSPEED_NORMAL / m_speed;
    iFrameSleep = iFrameSleep * DVD_PLAYSPEED_NORMAL / abs(m_speed);
    iFrameDuration = iFrameDuration * DVD_PLAYSPEED_NORMAL / abs(m_speed);
  }

  // dropping to a very low framerate is not correct (it should not happen at all)
  iClockSleep = min(iClockSleep, DVD_MSEC_TO_TIME(500));
  iFrameSleep = min(iFrameSleep, DVD_MSEC_TO_TIME(500));

  if( m_stalled )
  { // when we render a still, we can't sync to clock anyway
    iSleepTime = iFrameSleep;
  }
  else
  {
    // try to decide on how to sync framerate
    if( pPicture->iFlags & DVP_FLAG_NOAUTOSYNC )
      iSleepTime = iClockSleep;
    else
      iSleepTime = iFrameSleep + (iClockSleep - iFrameSleep) / m_autosync;
  }

#ifdef PROFILE /* during profiling, try to play as fast as possible */
  iSleepTime = 0;
#endif

  // present the current pts of this frame to user, and include the actual
  // presentation delay, to allow him to adjust for it
  if( !m_stalled )
    m_iCurrentPts = pts - max(0.0, iSleepTime);

  // timestamp when we think next picture should be displayed based on current duration
  m_FlipTimeStamp  = iCurrentClock;
  m_FlipTimeStamp += max(0.0, iSleepTime);
  m_FlipTimeStamp += iFrameDuration;

  // ask decoder to drop frames next round, as we are very late
  if( (limited == false  && iClockSleep < -DVD_MSEC_TO_TIME(100))
  ||  (limited == true   && iClockSleep < -iFrameDuration*0.5 ) )
    result |= EOS_VERYLATE;

  if( m_speed < 0 )
  {
    if( iClockSleep < -DVD_MSEC_TO_TIME(200)
    && !(pPicture->iFlags & DVP_FLAG_NOSKIP) )
      return result | EOS_DROPPED;
  }

  if( (pPicture->iFlags & DVP_FLAG_DROPPED) )
    return result | EOS_DROPPED;

  if( m_speed != DVD_PLAYSPEED_NORMAL && limited )
  {
    // calculate frame dropping pattern to render at this speed
    // we do that by deciding if this or next frame is closest
    // to the flip timestamp
    double current   = fabs(m_dropbase -  m_droptime);
    double next      = fabs(m_dropbase - (m_droptime + iFrameDuration));
    double frametime = (double)DVD_TIME_BASE / maxfps;

    m_droptime += iFrameDuration;
#ifndef PROFILE
    if( next < current && !(pPicture->iFlags & DVP_FLAG_NOSKIP) )
      return result | EOS_DROPPED;
#endif

    while(!m_bStop && m_dropbase < m_droptime)             m_dropbase += frametime;
    while(!m_bStop && m_dropbase - frametime > m_droptime) m_dropbase -= frametime;
  }
  else
  {
    m_droptime = 0.0f;
    m_dropbase = 0.0f;
  }

  // set fieldsync if picture is interlaced
  EFIELDSYNC mDisplayField = FS_NONE;
  if( pPicture->iFlags & DVP_FLAG_INTERLACED )
  {
    if( pPicture->iFlags & DVP_FLAG_TOP_FIELD_FIRST )
      mDisplayField = FS_ODD;
    else
      mDisplayField = FS_EVEN;
  }

  // copy picture to overlay
  YV12Image image;

  int index = g_renderManager.GetImage(&image);

  // video device might not be done yet
  while (index < 0 && !CThread::m_bStop &&
         CDVDClock::GetAbsoluteClock() < iCurrentClock + iSleepTime )
  {
    Sleep(1);
    index = g_renderManager.GetImage(&image);
  }

  if (index < 0)
    return EOS_DROPPED;

  ProcessOverlays(pPicture, &image, pts);

  // tell the renderer that we've finished with the image (so it can do any
  // post processing before FlipPage() is called.)
  g_renderManager.ReleaseImage(index);
  g_renderManager.FlipPage(CThread::m_bStop, (iCurrentClock + iSleepTime) / DVD_TIME_BASE, -1, mDisplayField);

  return result;
#else
  // no video renderer, let's mark it as dropped
  return EOS_DROPPED;
#endif
}

void CDVDPlayerVideo::UpdateMenuPicture()
{
  if (m_pVideoCodec)
  {
    DVDVideoPicture picture;
    EnterCriticalSection(&m_critCodecSection);
    if (m_pVideoCodec->GetPicture(&picture))
    {
      picture.iFlags |= DVP_FLAG_NOSKIP;
      OutputPicture(&picture, 0);
    }
    LeaveCriticalSection(&m_critCodecSection);
  }
}

std::string CDVDPlayerVideo::GetPlayerInfo()
{
  std::ostringstream s;
  s << "vq:"     << setw(2) << min(99,100 * m_messageQueue.GetDataSize() / m_messageQueue.GetMaxDataSize()) << "%";
  s << ", dc:"   << m_codecname;
  s << ", MB/s:" << fixed << setprecision(2) << (double)GetVideoBitrate() / (1024.0*1024.0);
  s << ", drop:" << m_iDroppedFrames;

  int pc = m_pullupCorrection.GetPatternLength();
  if (pc > 0)
    s << ", pc:" << pc;
  else
    s << ", pc:none";

  return s.str();
}

int CDVDPlayerVideo::GetVideoBitrate()
{
  return (int)m_videoStats.GetBitrate();
}

