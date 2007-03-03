
#include "../../stdafx.h"
#include "DVDPlayerAudio.h"
#include "DVDCodecs\Audio\DVDAudioCodec.h"
#include "DVDCodecs\DVDCodecs.h"
#include "DVDCodecs\DVDFactoryCodec.h"
#include "DVDPerformanceCounter.h"

static inline __int64 abs(__int64 x)
{
  return x > 0 ? x : -x;
}

#include "..\..\util.h"

void CPTSQueue::Add(__int64 pts, __int64 delay)
{
  TPTSItem item;
  item.pts = pts;
  item.timestamp = CDVDClock::GetAbsoluteClock() + delay;
  m_quePTSQueue.push(item);

  // call function to make sure the queue 
  // doesn't grow should nobody call it
  Current();
}
void CPTSQueue::Flush()
{
  while( !m_quePTSQueue.empty() ) m_quePTSQueue.pop();
  m_currentPTSItem.timestamp = 0;
  m_currentPTSItem.pts = DVD_NOPTS_VALUE;
}

__int64 CPTSQueue::Current()
{   
  while( !m_quePTSQueue.empty() && CDVDClock::GetAbsoluteClock() >= m_quePTSQueue.front().timestamp )
  {
    m_currentPTSItem = m_quePTSQueue.front();
    m_quePTSQueue.pop();
  }

  if( m_currentPTSItem.timestamp == 0 ) return m_currentPTSItem.pts;

  return m_currentPTSItem.pts + (CDVDClock::GetAbsoluteClock() - m_currentPTSItem.timestamp);  
}  


CDVDPlayerAudio::CDVDPlayerAudio(CDVDClock* pClock) : CThread()
{
  m_pClock = pClock;
  m_pAudioCodec = NULL;
  m_bInitializedOutputDevice = false;
  m_audioClock = 0;

  m_currentPTSItem.pts = DVD_NOPTS_VALUE;
  m_currentPTSItem.timestamp = 0;

  SetSpeed(DVD_PLAYSPEED_NORMAL);

  InitializeCriticalSection(&m_critCodecSection);
  m_messageQueue.SetMaxDataSize(10 * 16 * 1024);
  g_dvdPerformanceCounter.EnableAudioQueue(&m_messageQueue);
}

CDVDPlayerAudio::~CDVDPlayerAudio()
{
  g_dvdPerformanceCounter.DisableAudioQueue();

  // close the stream, and don't wait for the audio to be finished
  // CloseStream(true);
  DeleteCriticalSection(&m_critCodecSection);
}

bool CDVDPlayerAudio::OpenStream( CDVDStreamInfo &hints )                                 
{
  // should alway's be NULL!!!!, it will probably crash anyway when deleting m_pAudioCodec here.
  if (m_pAudioCodec)
  {
    CLog::Log(LOGFATAL, "CDVDPlayerAudio::OpenStream() m_pAudioCodec != NULL");
    return false;
  }

  /* try to open decoder without probing, we could actually allow us to continue here */
  if( !OpenDecoder(hints) ) return false;

  m_messageQueue.Init();

  CLog::Log(LOGNOTICE, "Creating audio thread");
  Create();

  return true;
}

void CDVDPlayerAudio::CloseStream(bool bWaitForBuffers)
{
  // wait until buffers are empty
  if (bWaitForBuffers) m_messageQueue.WaitUntilEmpty();

  // send abort message to the audio queue
  m_messageQueue.Abort();

  CLog::Log(LOGNOTICE, "waiting for audio thread to exit");

  // shut down the adio_decode thread and wait for it
  StopThread(); // will set this->m_bStop to true
  this->WaitForThreadExit(INFINITE);

  // uninit queue
  m_messageQueue.End();

  CLog::Log(LOGNOTICE, "Deleting audio codec");
  if (m_pAudioCodec)
  {
    m_pAudioCodec->Dispose();
    delete m_pAudioCodec;
    m_pAudioCodec = NULL;
  }

  // flush any remaining pts values
  m_ptsQueue.Flush();
}

bool CDVDPlayerAudio::OpenDecoder(CDVDStreamInfo &hints, BYTE* buffer /* = NULL*/, unsigned int size /* = 0*/)
{
  EnterCriticalSection(&m_critCodecSection);

  /* close current audio codec */
  if( m_pAudioCodec )
  {
    CLog::Log(LOGNOTICE, "Deleting audio codec");
    m_pAudioCodec->Dispose();
    SAFE_DELETE(m_pAudioCodec);
  }

  /* if we have an audio device open, close it. (we could check for a change in output format too) */
  if( m_bInitializedOutputDevice )
  { 
    m_dvdAudio.Destroy();
    m_bInitializedOutputDevice = false;
  }

  /* store our stream hints */
  m_streaminfo = hints;

  CLog::Log(LOGNOTICE, "Finding audio codec for: %i", m_streaminfo.codec);
  m_pAudioCodec = CDVDFactoryCodec::CreateAudioCodec( m_streaminfo );
  if( !m_pAudioCodec )
  {
    CLog::Log(LOGERROR, "Unsupported audio codec");

    m_streaminfo.Clear();
    LeaveCriticalSection(&m_critCodecSection);
    return false;
  }

  /* update codec information from what codec gave ut */  
  m_streaminfo.channels = m_pAudioCodec->GetChannels();
  m_streaminfo.samplerate = m_pAudioCodec->GetSampleRate();
  
  LeaveCriticalSection(&m_critCodecSection);

  return true;
}

// decode one audio frame and returns its uncompressed size
int CDVDPlayerAudio::DecodeFrame(DVDAudioFrame &audioframe, bool bDropPacket)
{
  CDVDDemux::DemuxPacket* pPacket = pAudioPacket;
  int n=48000*2*16/8, len;

  //Store amount left at this point, and what last pts was
  unsigned __int64 first_pkt_pts = 0;
  int first_pkt_size = 0; 
  int first_pkt_used = 0;
  int result = 0;

  // initial data timeout, will be set to frame duration later*/
  unsigned int datatimeout = 1000;

  // make sure the sent frame is clean
  memset(&audioframe, 0, sizeof(DVDAudioFrame));

  if (pPacket)
  {
    first_pkt_pts = pPacket->pts;
    first_pkt_size = pPacket->iSize;
    first_pkt_used = first_pkt_size - audio_pkt_size;
  }

  for (;;)
  {
    /* NOTE: the audio packet can contain several frames */
    while( audio_pkt_size > 0 )
    {
      if( !m_pAudioCodec ) 
      {
        audio_pkt_size=0;
        return DECODE_FLAG_ERROR;
      }

      len = m_pAudioCodec->Decode(audio_pkt_data, audio_pkt_size);
      if (len < 0)
      {
        /* if error, we skip the packet */
        CLog::Log(LOGERROR, "CDVDPlayerAudio::Process - Decode Error. Skipping audio packet");
        audio_pkt_size=0;
        m_pAudioCodec->Reset();
        return DECODE_FLAG_ERROR;
      }

      // fix for fucked up decoders
      if( len > audio_pkt_size )
      {        
        CLog::Log(LOGERROR, "CDVDPlayerAudio:DecodeFrame - Codec tried to consume more data than available. Potential memory corruption");        
        audio_pkt_size=0;
        m_pAudioCodec->Reset();
        assert(0);
      }

      // get decoded data and the size of it
      audioframe.size = m_pAudioCodec->GetData(&audioframe.data);

      audio_pkt_data += len;
      audio_pkt_size -= len;

      if (audioframe.size <= 0) continue;

      audioframe.pts = m_audioClock;

      // compute duration.
      n = m_pAudioCodec->GetChannels() * m_pAudioCodec->GetBitsPerSample() / 8 * m_pAudioCodec->GetSampleRate();
      if (n > 0)
      {
        // safety check, if channels == 0, n will result in 0, and that will result in a nice devide exception
        audioframe.duration = (unsigned int)(((__int64)audioframe.size * DVD_TIME_BASE) / n);

        // increase audioclock to after the packet
        m_audioClock += audioframe.duration;
        datatimeout = audioframe.duration*2;
      }

      //If we are asked to drop this packet, return a size of zero. then it won't be played
      //we currently still decode the audio.. this is needed since we still need to know it's 
      //duration to make sure clock is updated correctly.
      if( bDropPacket )
      {
        result |= DECODE_FLAG_DROP;
      }
      return result;
    }
    // free the current packet
    if (pPacket)
    {
      CDVDDemuxUtils::FreeDemuxPacket(pPacket);
      pPacket = NULL;
      pAudioPacket = NULL;
    }

    if (m_messageQueue.RecievedAbortRequest()) return DECODE_FLAG_ABORT;
    
    // read next packet and return -1 on error
    LeaveCriticalSection(&m_critCodecSection); //Leave here as this might stall a while
    
    CDVDMsg* pMsg;
    MsgQueueReturnCode ret = m_messageQueue.Get(&pMsg, datatimeout);
    
    EnterCriticalSection(&m_critCodecSection);
    
    if (ret == MSGQ_TIMEOUT) 
    {
      m_Stalled = true;
      continue;
    }

    if (MSGQ_IS_ERROR(ret) || ret == MSGQ_ABORT) return DECODE_FLAG_ABORT;

    if (pMsg->IsType(CDVDMsg::DEMUXER_PACKET))
    {
      CDVDMsgDemuxerPacket* pMsgDemuxerPacket = (CDVDMsgDemuxerPacket*)pMsg;
      pPacket = pMsgDemuxerPacket->GetPacket();
      pMsgDemuxerPacket->m_pPacket = NULL; // XXX, test
      pAudioPacket = pPacket;
      audio_pkt_data = pPacket->pData;
      audio_pkt_size = pPacket->iSize;
      m_Stalled = false;
    }
    else if (pMsg->IsType(CDVDMsg::GENERAL_STREAMCHANGE))
    {
      CDVDMsgGeneralStreamChange* pMsgStreamChange = (CDVDMsgGeneralStreamChange*)pMsg;
      CDVDStreamInfo* hints = pMsgStreamChange->GetStreamInfo();

      /* recieved a stream change, reopen codec. */
      /* we should really not do this untill first packet arrives, to have a probe buffer */      

      /* try to open decoder, if none is found keep consuming packets */
      OpenDecoder( *hints );

      pMsg->Release();
      continue;
    }
    else if (pMsg->IsType(CDVDMsg::GENERAL_SYNCHRONIZE))
    {
      CDVDMsgGeneralSynchronize* pMsgGeneralSynchronize = (CDVDMsgGeneralSynchronize*)pMsg;
      
      CLog::Log(LOGDEBUG, "CDVDPlayerAudio - CDVDMsg::GENERAL_SYNCHRONIZE");

      pMsgGeneralSynchronize->Wait( &m_bStop, SYNCSOURCE_AUDIO );
      
      pMsg->Release();
      continue;
    } 
    else if (pMsg->IsType(CDVDMsg::GENERAL_SET_CLOCK))
    { //player asked us to set dvdclock on this
      CDVDMsgGeneralSetClock* pMsgGeneralSetClock = (CDVDMsgGeneralSetClock*)pMsg;
      result |= DECODE_FLAG_RESYNC;
      
      if( pMsgGeneralSetClock->GetDts() != DVD_NOPTS_VALUE )
        m_audioClock = pMsgGeneralSetClock->GetDts();

      pMsg->Release();
      continue;
    } 
    else if (pMsg->IsType(CDVDMsg::GENERAL_RESYNC))
    { //player asked us to set internal clock
      CDVDMsgGeneralResync* pMsgGeneralResync = (CDVDMsgGeneralResync*)pMsg;                  

      if( pMsgGeneralResync->GetDts() != DVD_NOPTS_VALUE )
        m_audioClock = pMsgGeneralResync->GetDts();
      
      pMsg->Release();
      continue;
    }

        
    if (pMsg->IsType(CDVDMsg::DEMUXER_PACKET))
    {
      if (pPacket->pts != DVD_NOPTS_VALUE) // CDVDMsg::DEMUXER_PACKET, pPacket is already set above
      {
        if (first_pkt_size == 0) 
        { //first package
          m_audioClock = pPacket->pts;        
        }
        else if (first_pkt_pts > pPacket->pts)
        { //okey first packet in this continous stream, make sure we use the time here        
          m_audioClock = pPacket->pts;        
        }
        else if((unsigned __int64)m_audioClock < pPacket->pts || (unsigned __int64)m_audioClock > pPacket->pts)
        {
          //crap, moved outsided correct pts
          //Use pts from current packet, untill we find a better value for it.
          //Should be ok after a couple of frames, as soon as it starts clean on a packet
          m_audioClock = pPacket->pts;
        }
        else if(first_pkt_size == first_pkt_used)
        { //Nice starting up freshly on the start of a packet, use pts from it
          m_audioClock = pPacket->pts;
        }
      }
    }
    pMsg->Release();
  }
}

void CDVDPlayerAudio::OnStartup()
{
  CThread::SetName("CDVDPlayerAudio");
  pAudioPacket = NULL;
  m_audioClock = 0;
  audio_pkt_data = NULL;
  audio_pkt_size = 0;

  m_Stalled = true;

  g_dvdPerformanceCounter.EnableAudioDecodePerformance(ThreadHandle());
}

void CDVDPlayerAudio::Process()
{
  CLog::Log(LOGNOTICE, "running thread: CDVDPlayerAudio::Process()");

  int result;
 
  DVDAudioFrame audioframe;

  while (!m_bStop)
  {
    //make sure player doesn't keep processing data while paused
    while (m_speed == DVD_PLAYSPEED_PAUSE && !m_messageQueue.RecievedAbortRequest()) Sleep(5);

    //Don't let anybody mess with our global variables
    EnterCriticalSection(&m_critCodecSection);
    result = DecodeFrame(audioframe, m_speed != DVD_PLAYSPEED_NORMAL); // blocks if no audio is available, but leaves critical section before doing so
    LeaveCriticalSection(&m_critCodecSection);

    if( result & DECODE_FLAG_ERROR )  continue;

    if( result & DECODE_FLAG_ABORT )
    {
      CLog::Log(LOGDEBUG, "CDVDPlayerAudio::Process - Abort recieved, exiting thread");
      break;
    }

#ifdef PROFILE /* during profiling we just drop all packets, after having decoded */
  m_pClock->Discontinuity(CLOCK_DISC_NORMAL, m_audioClock, 0);
  continue;
#endif

    if( result & DECODE_FLAG_DROP )
    {
      //frame should be dropped. Don't let audio move ahead of the current time thou
      //we need to be able to start playing at any time
      //when playing backwords, we try to keep as small buffers as possible

      // set the time at this delay
      m_ptsQueue.Add(audioframe.pts, m_dvdAudio.GetDelay());

      if (m_speed > 0)
      {
        __int64 timestamp = m_pClock->GetAbsoluteClock() + (audioframe.duration * DVD_PLAYSPEED_NORMAL) / m_speed;
        while( !m_bStop && timestamp > m_pClock->GetAbsoluteClock() ) Sleep(1);
      }
      continue;
    }
    
    if( audioframe.size == 0 )
      continue;

    // we have succesfully decoded an audio frame, openup the audio device if not already done
    if (!m_bInitializedOutputDevice)
      m_bInitializedOutputDevice = InitializeOutputDevice();

    // add any packets play
    m_dvdAudio.AddPackets(audioframe.data, audioframe.size);

    // store the delay for this pts value so we can calculate the current playing
    m_ptsQueue.Add(audioframe.pts, m_dvdAudio.GetDelay() - audioframe.duration);

    // if we wanted to resync, we resync on this packet
    if( result & DECODE_FLAG_RESYNC )
    {
      m_pClock->Discontinuity(CLOCK_DISC_NORMAL, audioframe.pts, m_dvdAudio.GetDelay() - audioframe.duration);
      CLog::Log(LOGDEBUG, "CDVDPlayerAudio:: Resync - clock:%I64d, delay:%I64d", audioframe.pts, m_dvdAudio.GetDelay() - audioframe.duration);
    }
    
    // don't try to fix a desynced clock, until we played out the full audio buffer
    if( abs(m_pClock->DistanceToDisc()) < m_dvdAudio.GetDelay() )
      continue;
    
    if( m_ptsQueue.Current() == DVD_NOPTS_VALUE )
      continue;

    __int64 clock = m_pClock->GetClock();
    __int64 error = m_ptsQueue.Current() - clock;

    if( abs(error) > DVD_MSEC_TO_TIME(5) )
    {
      m_pClock->Discontinuity(CLOCK_DISC_NORMAL, clock+error, 0);      
      CLog::Log(LOGDEBUG, "CDVDPlayerAudio:: Discontinuty - was:%I64d, should be:%I64d, error:%I64d", clock, clock+error, error);
    }
  }
}

void CDVDPlayerAudio::OnExit()
{
  g_dvdPerformanceCounter.DisableAudioDecodePerformance();
  
  // destroy audio device
  CLog::Log(LOGNOTICE, "Closing audio device");
  m_dvdAudio.Destroy();
  m_bInitializedOutputDevice = false;

  CLog::Log(LOGNOTICE, "thread end: CDVDPlayerAudio::OnExit()");
}

void CDVDPlayerAudio::SetSpeed(int speed)
{ 
  m_speed = speed;
  
  if (m_speed == DVD_PLAYSPEED_PAUSE)
  {
    m_ptsQueue.Flush();
    m_dvdAudio.Pause();
  }
  else 
    m_dvdAudio.Resume();
}

void CDVDPlayerAudio::Flush()
{
  m_messageQueue.Flush();
  m_dvdAudio.Flush();
  m_ptsQueue.Flush();

  if (m_pAudioCodec)
  {
    EnterCriticalSection(&m_critCodecSection);
    audio_pkt_size = 0;
    audio_pkt_data = NULL;
    if( pAudioPacket )
    {
      CDVDDemuxUtils::FreeDemuxPacket(pAudioPacket);
      pAudioPacket = NULL;
    }

    m_pAudioCodec->Reset();
    LeaveCriticalSection(&m_critCodecSection);
  }
}

void CDVDPlayerAudio::WaitForBuffers()
{
  // make sure there are no more packets available
  m_messageQueue.WaitUntilEmpty();
  
  // make sure almost all has been rendered
  // leave 500ms to avound buffer underruns

  while( m_dvdAudio.GetDelay() > DVD_TIME_BASE/2 )
  {
    Sleep(5);
  }
}

bool CDVDPlayerAudio::InitializeOutputDevice()
{
  int iChannels = m_pAudioCodec->GetChannels();
  int iSampleRate = m_pAudioCodec->GetSampleRate();
  int iBitsPerSample = m_pAudioCodec->GetBitsPerSample();
  bool bPassthrough = m_pAudioCodec->NeedPasstrough();

  if (iChannels == 0 || iSampleRate == 0 || iBitsPerSample == 0)
  {
    CLog::Log(LOGERROR, "Unable to create audio device, (iChannels == 0 || iSampleRate == 0 || iBitsPerSample == 0)");
    return false;
  }

  CLog::Log(LOGNOTICE, "Creating audio device with codec id: %i, channels: %i, sample rate: %i, %s", m_streaminfo.codec, iChannels, iSampleRate, bPassthrough ? "pass-through" : "no pass-through");
  if (m_dvdAudio.Create(iChannels, iSampleRate, iBitsPerSample, bPassthrough)) // always 16 bit with ffmpeg ?
  {
    return true;
  }

  CLog::Log(LOGERROR, "Failed Creating audio device with codec id: %i, channels: %i, sample rate: %i", m_streaminfo.codec, iChannels, iSampleRate);
  return false;
}
