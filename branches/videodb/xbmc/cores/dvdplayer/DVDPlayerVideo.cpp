
#include "../../stdafx.h"
#include "DVDPlayer.h"
#include "DVDPlayerVideo.h"
#include "DVDCodecs\DVDFactoryCodec.h"
#include "DVDCodecs\DVDCodecUtils.h"
#include "DVDCodecs\Video\DVDVideoPPFFmpeg.h"
#include "DVDDemuxers\DVDDemux.h"
#include "DVDDemuxers\DVDDemuxUtils.h"
#include "..\..\util.h"
#include "DVDOverlayRenderer.h"
#include "DVDPerformanceCounter.h"
#include "DVDCodecs\DVDCodecs.h"
#include "DVDCodecs\Overlay\DVDOverlayCodecCC.h"

static __forceinline __int64 abs(__int64 value)
{
  if( value < 0 )
    return -value;
  else
    return value;
}
#define ROUNDED_DIV(a,b) (((a)>0 ? (a) + ((b)>>1) : (a) - ((b)>>1))/(b))

CDVDPlayerVideo::CDVDPlayerVideo(CDVDClock* pClock, CDVDOverlayContainer* pOverlayContainer) 
: CThread()
{
  m_pClock = pClock;
  m_pOverlayContainer = pOverlayContainer;
  m_pTempOverlayPicture = NULL;
  m_pVideoCodec = NULL;
  m_pOverlayCodecCC = NULL;
  m_bInitializedOutputDevice = false;
  
  SetSpeed(DVD_PLAYSPEED_NORMAL);
  m_bRenderSubs = false;
  m_iVideoDelay = 0;
  m_fForcedAspectRatio = 0;
  m_iNrOfPicturesNotToSkip = 0;
  InitializeCriticalSection(&m_critCodecSection);
  m_messageQueue.SetMaxDataSize(5 * 256 * 1024); // 1310720
  g_dvdPerformanceCounter.EnableVideoQueue(&m_messageQueue);
  
  m_iCurrentPts = DVD_NOPTS_VALUE;
  m_iDroppedFrames = 0;
  m_fFrameRate = 25;
}

CDVDPlayerVideo::~CDVDPlayerVideo()
{
  g_dvdPerformanceCounter.DisableVideoQueue();
  DeleteCriticalSection(&m_critCodecSection);
}

__int64 CDVDPlayerVideo::GetOutputDelay()
{
    __int64 time = m_messageQueue.GetPacketCount(CDVDMsg::DEMUXER_PACKET);
    if( m_fFrameRate )
      time = (__int64)( (time * DVD_TIME_BASE) / m_fFrameRate );
    else
      time = 0LL;
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

  if( m_fFrameRate > 100 || m_fFrameRate < 5 )
  {
    CLog::Log(LOGERROR, "CDVDPlayerVideo::OpenStream - Invalid framerate %d, using forced 25fps and just trust timestamps", (int)m_fFrameRate);
    m_fFrameRate = 25;
    m_autosync = 1; // avoid using frame time as we don't know it accurate
  }



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

  m_messageQueue.Init();

  CLog::Log(LOGNOTICE, "Creating video thread");
  Create();

  return true;
}

void CDVDPlayerVideo::CloseStream(bool bWaitForBuffers)
{
  m_messageQueue.Abort();

  // wait for decode_video thread to end
  CLog::Log(LOGNOTICE, "waiting for video thread to exit");

  StopThread(); // will set this->m_bStop to true  

  m_messageQueue.End();
  m_pOverlayContainer->Clear();

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
}

void CDVDPlayerVideo::OnStartup()
{
  CThread::SetName("CDVDPlayerVideo");
  m_iDroppedFrames = 0;
  
  m_iCurrentPts = DVD_NOPTS_VALUE;
  m_iFlipTimeStamp = m_pClock->GetAbsoluteClock();
  m_DetectedStill = false;
  
  memset(&m_output, 0, sizeof(m_output));
  
  g_dvdPerformanceCounter.EnableVideoDecodePerformance(ThreadHandle());
}

void CDVDPlayerVideo::Process()
{
  CLog::Log(LOGNOTICE, "running thread: video_thread");

  DVDVideoPicture picture;
  CDVDVideoPPFFmpeg mDeinterlace(CDVDVideoPPFFmpeg::ED_DEINT_FFMPEG);

  memset(&picture, 0, sizeof(DVDVideoPicture));
  
  __int64 pts = 0;

  unsigned int iFrameTime = (unsigned int)(DVD_TIME_BASE / m_fFrameRate) ;  

  int iDropped = 0; //frames dropped in a row
  bool bRequestDrop = false;  

  while (!m_bStop)
  {
    while (m_speed == DVD_PLAYSPEED_PAUSE && !m_messageQueue.RecievedAbortRequest()) Sleep(5);

    int iQueueTimeOut = (m_DetectedStill ? iFrameTime / 4 : iFrameTime * 4) / 1000;
    
    CDVDMsg* pMsg;
    MsgQueueReturnCode ret = m_messageQueue.Get(&pMsg, iQueueTimeOut);
   
    if (MSGQ_IS_ERROR(ret) || ret == MSGQ_ABORT) break;
    else if (ret == MSGQ_TIMEOUT)
    {
      //Okey, start rendering at stream fps now instead, we are likely in a stillframe
      if( !m_DetectedStill )
      {
        CLog::Log(LOGINFO, "CDVDPlayerVideo - Stillframe detected, switching to forced %d fps", (DVD_TIME_BASE / iFrameTime));
        m_DetectedStill = true;
        pts+= iFrameTime*4;
      }

      //Waiting timed out, output last picture
      if( picture.iFlags & DVP_FLAG_ALLOCATED )
      {
        //Remove interlaced flag before outputting
        //no need to output this as if it was interlaced
        picture.iFlags &= ~DVP_FLAG_INTERLACED;
        picture.iFlags |= DVP_FLAG_NOSKIP;
        OutputPicture(&picture, pts);
        pts+= iFrameTime;
      }

      continue;
    }

    if (pMsg->IsType(CDVDMsg::GENERAL_SYNCHRONIZE))
    {
      CLog::Log(LOGDEBUG, "CDVDPlayerVideo - CDVDMsg::GENERAL_SYNCHRONIZE");

      CDVDMsgGeneralSynchronize* pMsgGeneralSynchronize = (CDVDMsgGeneralSynchronize*)pMsg;
      pMsgGeneralSynchronize->Wait(&m_bStop, SYNCSOURCE_VIDEO);

      pMsgGeneralSynchronize->Release();

      /* we may be very much off correct pts here, but next picture may be a still*/
      /* make sure it isn't dropped */
      m_iNrOfPicturesNotToSkip = 5;
      continue;
    }
    else if (pMsg->IsType(CDVDMsg::GENERAL_SET_CLOCK))
    {
      CLog::Log(LOGDEBUG, "CDVDPlayerVideo::Process - Resync recieved.");
      
      CDVDMsgGeneralSetClock* pMsgGeneralSetClock = (CDVDMsgGeneralSetClock*)pMsg;

      //DVDPlayer asked us to sync playback clock
      if( pMsgGeneralSetClock->GetPts() != DVD_NOPTS_VALUE )
        pts = pMsgGeneralSetClock->GetPts();      
      else if( pMsgGeneralSetClock->GetDts() != DVD_NOPTS_VALUE )
        pts = pMsgGeneralSetClock->GetDts();

      __int64 delay = m_iFlipTimeStamp - m_pClock->GetAbsoluteClock();
      
      if( delay > iFrameTime ) delay = iFrameTime;
      else if( delay < 0 ) delay = 0;

      m_pClock->Discontinuity(CLOCK_DISC_NORMAL, pts, delay);
      CLog::Log(LOGDEBUG, "CDVDPlayerVideo:: Resync - clock:%I64d, delay:%I64d", pts, delay);      

      pMsgGeneralSetClock->Release();
      continue;
    } 
    else if (pMsg->IsType(CDVDMsg::GENERAL_RESYNC))
    {
      CLog::Log(LOGDEBUG, "CDVDPlayerVideo - CDVDMsg::GENERAL_RESYNC"); 
      
      CDVDMsgGeneralResync* pMsgGeneralResync = (CDVDMsgGeneralResync*)pMsg;
      
      //DVDPlayer asked us to sync playback clock
      if( pMsgGeneralResync->GetPts() != DVD_NOPTS_VALUE )
        pts = pMsgGeneralResync->GetPts();      
      else if( pMsgGeneralResync->GetDts() != DVD_NOPTS_VALUE )
        pts = pMsgGeneralResync->GetDts();

      pMsgGeneralResync->Release();
      continue;
    }
    else if (pMsg->IsType(CDVDMsg::VIDEO_SET_ASPECT))
    {
      CLog::Log(LOGDEBUG, "CDVDPlayerVideo - CDVDMsg::VIDEO_SET_ASPECT");
      m_fForcedAspectRatio = ((CDVDMsgVideoSetAspect*)pMsg)->GetAspect();
    }
    else if (pMsg->IsType(CDVDMsg::GENERAL_FLUSH)) // private mesasage sent by (CDVDPlayerVideo::Flush())
    {
      EnterCriticalSection(&m_critCodecSection);
      if(m_pVideoCodec)
        m_pVideoCodec->Reset();
      LeaveCriticalSection(&m_critCodecSection);
    }
    
    if (m_DetectedStill)
    {
      CLog::Log(LOGINFO, "CDVDPlayerVideo - Stillframe left, switching to normal playback");      
      m_DetectedStill = false;

      //don't allow the first frames after a still to be dropped
      //sometimes we get multiple stills (long duration frames) after each other
      //in normal mpegs
      m_iNrOfPicturesNotToSkip = 5; 
    }

    EnterCriticalSection(&m_critCodecSection);

    if (pMsg->IsType(CDVDMsg::VIDEO_NOSKIP))
    {
      // libmpeg2 is also returning incomplete frames after a dvd cell change
      // so the first few pictures are not the correct ones to display in some cases
      // just display those together with the correct one.
      // (setting it to 2 will skip some menu stills, 5 is working ok for me).
      m_iNrOfPicturesNotToSkip = 5;
    }
    
    if( iDropped > 30 )
    { // if we dropped too many pictures in a row, insert a forced picure
      m_iNrOfPicturesNotToSkip++;
      iDropped = 0;
    }

    if (m_speed < 0)
    { // playing backward, don't drop any pictures
      m_iNrOfPicturesNotToSkip = 5;
    }

    // if we have pictures that can't be skipped, never request drop
    if( m_iNrOfPicturesNotToSkip > 0 ) bRequestDrop = false;

    // tell codec if next frame should be dropped
    // problem here, if one packet contains more than one frame
    // both frames will be dropped in that case instead of just the first
    // decoder still needs to provide an empty image structure, with correct flags
    m_pVideoCodec->SetDropState(bRequestDrop);
    
    if (pMsg->IsType(CDVDMsg::DEMUXER_PACKET))
    {
      CDVDMsgDemuxerPacket* pMsgDemuxerPacket = (CDVDMsgDemuxerPacket*)pMsg;
      CDVDDemux::DemuxPacket* pPacket = pMsgDemuxerPacket->GetPacket();
      
      int iDecoderState = m_pVideoCodec->Decode(pPacket->pData, pPacket->iSize);

      /* decoders might leave in an invalid mmx state, make sure this is reset before continuing */
      __asm { 
        emms
      }

      // loop while no error
      while (!(iDecoderState & VC_ERROR))
      {
        // check for a new picture
        if (iDecoderState & VC_PICTURE)
        {

          // try to retrieve the picture (should never fail!), unless there is a demuxer bug ofcours
          memset(&picture, 0, sizeof(DVDVideoPicture));
          if (m_pVideoCodec->GetPicture(&picture))
          {
            picture.iGroupId = pPacket->iGroupId;
            
            if (picture.iDuration == 0)
              // should not use iFrameTime here. cause framerate can change while playing video
              picture.iDuration = iFrameTime;

            if (m_iNrOfPicturesNotToSkip > 0)
            {
              picture.iFlags |= DVP_FLAG_NOSKIP;
              m_iNrOfPicturesNotToSkip--;
            }
            
            //Deinterlace if codec said format was interlaced or if we have selected we want to deinterlace
            //this video
            EINTERLACEMETHOD mInt = g_stSettings.m_currentVideoSettings.m_InterlaceMethod;
            if( mInt == VS_INTERLACEMETHOD_DEINTERLACE )
            {
              mDeinterlace.Process(&picture);
              mDeinterlace.GetPicture(&picture);
            }
            else if( mInt == VS_INTERLACEMETHOD_RENDER_WEAVE || mInt == VS_INTERLACEMETHOD_RENDER_WEAVE_INVERTED )
            { 
              /* if we are syncing frames, dvdplayer will be forced to play at a given framerate */
              /* unless we directly sync to the correct pts, we won't get a/v sync as video can never catch up */
              picture.iFlags |= DVP_FLAG_NOAUTOSYNC;
            }
            

            if ((picture.iFrameType == FRAME_TYPE_I || picture.iFrameType == FRAME_TYPE_UNDEF) &&
                pPacket->dts != DVD_NOPTS_VALUE) //Only use pts when we have an I frame, or unknown
            {
              pts = pPacket->dts;
            }
            
            //Check if dvd has forced an aspect ratio
            if( m_fForcedAspectRatio != 0.0f )
            {
              picture.iDisplayWidth = (int) (picture.iDisplayHeight * m_fForcedAspectRatio);
            }

            EOUTPUTSTATUS iResult;
            do 
            {
              try 
              {
                iResult = OutputPicture(&picture, pts);
              }
              catch (...)
              {
                CLog::Log(LOGERROR, __FUNCTION__" - Exception caught when outputing picture");
                iResult = EOS_ABORT;
              }

              if (iResult == EOS_ABORT) break;

              // guess next frame pts. iDuration is always valid
              pts += picture.iDuration;
            }
            while (picture.iRepeatPicture-- > 0);

            bRequestDrop = false;
            if( iResult == EOS_ABORT )
            {
              //if we break here and we directly try to decode again wihout 
              //flushing the video codec things break for some reason
              //i think the decoder (libmpeg2 atleast) still has a pointer
              //to the data, and when the packet is freed that will fail.
              iDecoderState = m_pVideoCodec->Decode(NULL, NULL);
              break;
            }
            else if( iResult == EOS_DROPPED )
            {
              m_iDroppedFrames++;
              iDropped++;
            }
            else if( iResult == EOS_DROPPED_VERYLATE )
            {
              m_iDroppedFrames++;
              iDropped++;
              bRequestDrop = true;
            }
          }
          else
          {
            CLog::Log(LOGWARNING, "Decoder Error getting videoPicture.");
            m_pVideoCodec->Reset();
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
        if (iDecoderState & VC_BUFFER) break;
        try
        {
          // the decoder didn't need more data, flush the remaning buffer
          iDecoderState = m_pVideoCodec->Decode(NULL, NULL);
        }
        catch(...)
        {
          CLog::Log(LOGERROR, __FUNCTION__" - Exception caught when decoding data");
          iDecoderState = VC_ERROR;
        }
      }

      // if decoder had an error, tell it to reset to avoid more problems
      if( iDecoderState & VC_ERROR ) m_pVideoCodec->Reset();
    }
    
    LeaveCriticalSection(&m_critCodecSection);
    
    // all data is used by the decoder, we can safely free it now
    pMsg->Release();
  }

  CLog::Log(LOGNOTICE, "thread end: video_thread");

  CLog::Log(LOGNOTICE, "uninitting video device");
}

void CDVDPlayerVideo::OnExit()
{
  g_dvdPerformanceCounter.DisableVideoDecodePerformance();
  
  g_renderManager.UnInit();
  m_bInitializedOutputDevice = false;  

  if (m_pOverlayCodecCC)
  {
    m_pOverlayCodecCC->Dispose();
    m_pOverlayCodecCC = NULL;
  }
  
  CLog::Log(LOGNOTICE, "thread end: video_thread");
}

void CDVDPlayerVideo::ProcessVideoUserData(DVDVideoUserData* pVideoUserData, __int64 pts)
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
        if (!m_pOverlayCodecCC->Open())
        {
          delete m_pOverlayCodecCC;
          m_pOverlayCodecCC = NULL;
        }
      }
      
      if (m_pOverlayCodecCC)
      {
        int iState = 0;
        while (!(iState & OC_ERROR) && !(iState & OC_BUFFER))
        {
          iState = m_pOverlayCodecCC->Decode(data, size, pts);
          data = NULL;
          size = 0;
          
          if (iState & OC_OVERLAY)
          {
            CDVDOverlay* pOverlay = m_pOverlayCodecCC->GetOverlay();
            m_pOverlayContainer->Add(pOverlay);
          }
        }
      }
    }
  }
}

bool CDVDPlayerVideo::InitializedOutputDevice()
{
  return m_bInitializedOutputDevice;
}

void CDVDPlayerVideo::SetSpeed(int speed)
{
  m_speed = speed;
}

void CDVDPlayerVideo::Flush()
{ 
  /* flush using message as this get's called from dvdplayer thread */
  /* and any demux packet that has been taken out of queue need to *
  /* be disposed of before we flush */
  m_messageQueue.Flush();
  m_messageQueue.Put(new CDVDMsgGeneralFlush());  
}

void CDVDPlayerVideo::ProcessOverlays(DVDVideoPicture* pSource, YV12Image* pDest, __int64 pts)
{
  // remove any overlays that are out of time
  m_pOverlayContainer->CleanUp(pts);

  // rendering spu overlay types directly on video memory costs a lot of processing power.
  // thus we allocate a temp picture, copy the original to it (needed because the same picture can be used more than once).
  // then do all the rendering on that temp picture and finaly copy it to video memory.
  // In almost all cases this is 5 or more times faster!.
  bool bHasSpecialOverlay = m_pOverlayContainer->ContainsOverlayType(DVDOVERLAY_TYPE_SPU);
  
  if (bHasSpecialOverlay)
  {
    if (m_pTempOverlayPicture && (m_pTempOverlayPicture->iWidth != pSource->iWidth || m_pTempOverlayPicture->iHeight != pSource->iHeight))
    {
      CDVDCodecUtils::FreePicture(m_pTempOverlayPicture);
      m_pTempOverlayPicture = NULL;
    }
    
    if (!m_pTempOverlayPicture) m_pTempOverlayPicture = CDVDCodecUtils::AllocatePicture(pSource->iWidth, pSource->iHeight);
  }

  if (bHasSpecialOverlay && m_pTempOverlayPicture) CDVDCodecUtils::CopyPicture(m_pTempOverlayPicture, pSource);
  else CDVDCodecUtils::CopyPictureToOverlay(pDest, pSource);
  
  m_pOverlayContainer->Lock();

  VecOverlays* pVecOverlays = m_pOverlayContainer->GetOverlays();
  VecOverlaysIter it = pVecOverlays->begin();
  
  //Check all overlays and render those that should be rendered, based on time and forced
  //Both forced and subs should check timeing, pts == 0 in the stillframe case
  while (it != pVecOverlays->end())
  {
    CDVDOverlay* pOverlay = *it;
    if ((pOverlay->bForced || m_bRenderSubs) && (pOverlay->iGroupId == pSource->iGroupId) && 
        ((pOverlay->iPTSStartTime <= pts && (pOverlay->iPTSStopTime >= pts || pOverlay->iPTSStopTime == 0LL)) || pts == 0))
    {
      if (bHasSpecialOverlay && m_pTempOverlayPicture) CDVDOverlayRenderer::Render(m_pTempOverlayPicture, pOverlay);
      else CDVDOverlayRenderer::Render(pDest, pOverlay);
    }
    it++;
  }
  
  m_pOverlayContainer->Unlock();
  
  if (bHasSpecialOverlay && m_pTempOverlayPicture)
  {
    CDVDCodecUtils::CopyPictureToOverlay(pDest, m_pTempOverlayPicture);
  }
}

CDVDPlayerVideo::EOUTPUTSTATUS CDVDPlayerVideo::OutputPicture(DVDVideoPicture* pPicture, __int64 pts)
{
  if (m_messageQueue.RecievedAbortRequest()) return EOS_ABORT;

  if (!m_bInitializedOutputDevice)
  {
    CLog::Log(LOGNOTICE, "Initializing video device");

    g_renderManager.PreInit();
    m_bInitializedOutputDevice = true;
  }

  /* check so that our format or aspect has changed. if it has, reconfigure renderer */
  if (m_output.width != pPicture->iWidth
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

    g_renderManager.Configure(pPicture->iWidth, pPicture->iHeight, pPicture->iDisplayWidth, pPicture->iDisplayHeight, m_fFrameRate, flags);

    m_output.width = pPicture->iWidth;
    m_output.height = pPicture->iHeight;
    m_output.dwidth = pPicture->iDisplayWidth;
    m_output.dheight = pPicture->iDisplayHeight;
    m_output.framerate = m_fFrameRate;
    m_output.color_matrix = pPicture->color_matrix;
    m_output.color_range = pPicture->color_range;
  }

  if (m_bInitializedOutputDevice)
  {
    // make sure we do not drop pictures that should not be skipped
    if (pPicture->iFlags & DVP_FLAG_NOSKIP) pPicture->iFlags &= ~DVP_FLAG_DROPPED;
    
    if( !(pPicture->iFlags & DVP_FLAG_DROPPED) )
    {
      // copy picture to overlay
      YV12Image image;
      unsigned int index = g_renderManager.GetImage(&image);
      if( index<0 ) return EOS_DROPPED;

      ProcessOverlays(pPicture, &image, pts);
      
      // tell the renderer that we've finished with the image (so it can do any
      // post processing before FlipPage() is called.)
      g_renderManager.ReleaseImage(index);

    }

    //User set delay
    pts += m_iVideoDelay;

    // calculate the time we need to delay this picture before displaying
    __int64 iSleepTime, iClockSleep, iFrameSleep, iCurrentClock;

    // snapshot current clock, to use the same value in all calculations
    iCurrentClock = m_pClock->GetAbsoluteClock();

    //sleep calculated by pts to clock comparison
    iClockSleep = pts - m_pClock->GetClock();

    // dropping to a very low framerate is not correct (it should not happen at all)
    // clock and audio could be adjusted
    if (iClockSleep > DVD_MSEC_TO_TIME(500) ) 
      iClockSleep = DVD_MSEC_TO_TIME(500); // drop to a minimum of 2 frames/sec


    // sleep calculated by duration of frame
    iFrameSleep = m_iFlipTimeStamp - iCurrentClock;
    // negative frame sleep possible after a stillframe, don't allow
    if( iFrameSleep < 0 ) iFrameSleep = 0;

    if (m_speed < 0)
    {
      // don't sleep when going backwords, just push frames out
      iSleepTime = 0;
    }
    else if( m_DetectedStill )
    { // when we render a still, we can't sync to clock anyway
      iSleepTime = iFrameSleep;
    }
    else
    {
      /* try to decide on how to sync framerate */
      /* during a discontinuity, we have have to rely on frame duration completly*/
      /* otherwise we adjust against the playback clock to some extent */

      /* decouple clock and video as we approach a discontinuity */
      /* decouple clock and video a while after a discontinuity */
      const __int64 distance = m_pClock->DistanceToDisc();

      if( abs(distance) < pPicture->iDuration*3 )
        iSleepTime = iFrameSleep + (iClockSleep - iFrameSleep) * 0;
      else if( pPicture->iFlags & DVP_FLAG_NOAUTOSYNC )
        iSleepTime = iClockSleep;
      else
        iSleepTime = iFrameSleep + (iClockSleep - iFrameSleep) / m_autosync;
    }

    /* adjust for speed */
    if( m_speed > DVD_PLAYSPEED_NORMAL )
      iSleepTime = iSleepTime * DVD_PLAYSPEED_NORMAL / m_speed;

#ifdef PROFILE /* during profiling, try to play as fast as possible */
    iSleepTime = 0;
#endif

    // present the current pts of this frame to user, and include the actual
    // presentation delay, to allow him to adjust for it
    m_iCurrentPts = pts - (iSleepTime > 0 ? iSleepTime : 0);

    // timestamp when we think next picture should be displayed based on current duration
    m_iFlipTimeStamp = iCurrentClock;
    m_iFlipTimeStamp += iSleepTime > 0 ? iSleepTime : 0;
    m_iFlipTimeStamp += pPicture->iDuration;

    if( iSleepTime < 0 )
    { // we are late, try to figure out how late
      // this could be improved if we had some average time
      // for decoding. so we know if we need to drop frames
      // in decoder
      if( !(pPicture->iFlags & DVP_FLAG_NOSKIP) )
      {

        iSleepTime*= -1;
        if( iSleepTime > 4*pPicture->iDuration )
        { // two frames late, signal that we are late. this will drop frames in decoder, untill we have an ok frame
          pPicture->iFlags |= DVP_FLAG_DROPPED;
          return EOS_DROPPED_VERYLATE;
        }
        else if( iSleepTime > 2*pPicture->iDuration )
        { // one frame late, drop in renderer     
          pPicture->iFlags |= DVP_FLAG_DROPPED;
          return EOS_DROPPED;
        }

      }
      iSleepTime = 0;
    }

    if( (pPicture->iFlags & DVP_FLAG_DROPPED) ) return EOS_DROPPED;

    // set fieldsync if picture is interlaced
    EFIELDSYNC mDisplayField = FS_NONE;
    if( pPicture->iFlags & DVP_FLAG_INTERLACED )
    {
      if( pPicture->iFlags & DVP_FLAG_TOP_FIELD_FIRST )
        mDisplayField = FS_ODD;
      else
        mDisplayField = FS_EVEN;
    }

    // present this image after the given delay
    const __int64 delay = iCurrentClock + iSleepTime - m_pClock->GetAbsoluteClock();
    g_renderManager.FlipPage( (DWORD)(delay / (DVD_TIME_BASE / 1000)), -1, mDisplayField);
  }
  return EOS_OK;
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

__int64 CDVDPlayerVideo::GetDelay()
{
  return m_iVideoDelay;
}

void CDVDPlayerVideo::SetDelay(__int64 delay)
{
  m_iVideoDelay = delay;
}