
#include "../../stdafx.h"
#include "DVDPlayer.h"

#include "DVDInputStreams\DVDInputStream.h"
#include "DVDInputStreams\DVDFactoryInputStream.h"
#include "DVDInputStreams\DVDInputStreamNavigator.h"

#include "DVDDemuxers\DVDDemux.h"
#include "DVDDemuxers\DVDDemuxUtils.h"
#include "DVDDemuxers\DVDFactoryDemuxer.h"

#include "DVDCodecs\DVDCodecs.h"

#include "..\..\util.h"
#include "../../utils/GUIInfoManager.h"
#include "DVDPerformanceCounter.h"

CDVDPlayer::CDVDPlayer(IPlayerCallback& callback)
    : IPlayer(callback),
      CThread(),
      m_dvdPlayerVideo(&m_clock, &m_overlayContainer),
      m_dvdPlayerAudio(&m_clock),
      m_dvdPlayerSubtitle(&m_overlayContainer)
{
  m_pDemuxer = NULL;
  m_pInputStream = NULL;

  m_hReadyEvent = CreateEvent(NULL, true, false, NULL);

  InitializeCriticalSection(&m_critStreamSection);

  memset(&m_dvd, 0, sizeof(DVDInfo));
  m_dvd.iCurrentCell = -1;
  m_dvd.iNAVPackStart = -1;
  m_dvd.iNAVPackFinish = -1;
  m_dvd.iFlagSentStart = 0;
  m_dvd.iSelectedAudioStream = -1;
  m_dvd.iSelectedSPUStream = -1;


  m_filename[0] = '\0';
  m_bAbortRequest = false;

  m_CurrentAudio.id = -1;
  m_CurrentAudio.dts = DVD_NOPTS_VALUE;
  m_CurrentAudio.hint.Clear();
  m_CurrentAudio.stream = NULL;

  m_CurrentVideo.id = -1;
  m_CurrentVideo.dts = DVD_NOPTS_VALUE;
  m_CurrentVideo.hint.Clear();
  m_CurrentVideo.stream = NULL;

  m_CurrentSubtitle.id = -1;
  m_CurrentSubtitle.dts = DVD_NOPTS_VALUE;
  m_CurrentSubtitle.hint.Clear();
  m_CurrentSubtitle.stream = NULL;


  m_bRenderSubtitle = false;
  m_bDontSkipNextFrame = false;
}

CDVDPlayer::~CDVDPlayer()
{
  CloseFile();

  CloseHandle(m_hReadyEvent);
  DeleteCriticalSection(&m_critStreamSection);
}

bool CDVDPlayer::OpenFile(const CFileItem& file, __int64 iStartTime)
{
  CStdString strFile = file.m_strPath;

  CLog::Log(LOGNOTICE, "DVDPlayer: Opening: %s", strFile.c_str());

  // if playing a file close it first
  // this has to be changed so we won't have to close it.
  CloseFile();

  m_bAbortRequest = false;
  SetPlaySpeed(DVD_PLAYSPEED_NORMAL);

  m_dvd.state = DVDSTATE_NORMAL;
  m_dvd.iSelectedSPUStream = -1;
  m_dvd.iSelectedAudioStream = -1;

  if (strFile.Find("dvd://") >= 0 ||
      strFile.CompareNoCase("d:\\video_ts\\video_ts.ifo") == 0 ||
      strFile.CompareNoCase("iso9660://video_ts/video_ts.ifo") == 0)
  {
    strcpy(m_filename, "\\Device\\Cdrom0");
  }
  else strcpy(m_filename, strFile.c_str());

  ResetEvent(m_hReadyEvent);
  Create();
  WaitForSingleObject(m_hReadyEvent, INFINITE);

  SetSubtitleVisible(g_stSettings.m_currentVideoSettings.m_SubtitleOn);
  SetAVDelay(g_stSettings.m_currentVideoSettings.m_AudioDelay);

  // if we are playing a media file with pictures, we should wait for the video output device to be initialized
  // if we don't wait, the fullscreen window will init with a picture that is 0 pixels width and high
  bool bProcessThreadIsAlive = true;
  if (m_CurrentVideo.id >= 0 ||
      m_pInputStream && (m_pInputStream->IsStreamType(DVDSTREAM_TYPE_DVD) || m_pInputStream->HasExtension("vob")))
  {
    while (bProcessThreadIsAlive && !m_bAbortRequest && !m_dvdPlayerVideo.InitializedOutputDevice())
    {
      bProcessThreadIsAlive = !WaitForThreadExit(0);
      Sleep(1);
    }
  }

  // m_bPlaying could be set to false in the meantime, which indicates an error
  return (bProcessThreadIsAlive && !m_bStop);
}

bool CDVDPlayer::CloseFile()
{
  CLog::Log(LOGNOTICE, "CDVDPlayer::CloseFile()");

  // set the abort request so that other threads can finish up
  m_bAbortRequest = true;

  // unpause the player
  SetPlaySpeed(DVD_PLAYSPEED_NORMAL);

  // flush all buffers, and let OnExit do the rest of all the work
  // doing all the closing in OnExit requires less thread synchronisation and locking
  FlushBuffers();

  CLog::Log(LOGNOTICE, "DVDPlayer: waiting for threads to exit");

  // wait for the main thread to finish up
  // since this main thread cleans up all other resources and threads
  // we are done after the StopThread call
  StopThread();

  CLog::Log(LOGNOTICE, "DVDPlayer: finished waiting");

  return true;
}

bool CDVDPlayer::IsPlaying() const
{
  return !m_bStop;
}

void CDVDPlayer::OnStartup()
{
  CThread::SetName("CDVDPlayer");
  m_CurrentVideo.id = -1;
  m_CurrentAudio.id = -1;

  m_messenger.Init();

  g_dvdPerformanceCounter.EnableMainPerformance(ThreadHandle());
}

void CDVDPlayer::Process()
{
  int video_index = -1;
  int audio_index = -1;

  CLog::Log(LOGNOTICE, "Creating InputStream");
  m_pInputStream = CDVDFactoryInputStream::CreateInputStream(this, m_filename);
  if (!m_pInputStream || !m_pInputStream->Open(m_filename))
  {
    CLog::Log(LOGERROR, "InputStream: Error opening, %s", m_filename);
    // inputstream will be destroyed in OnExit()
    return;
  }

  if (m_pInputStream->IsStreamType(DVDSTREAM_TYPE_DVD))
  {
    CLog::Log(LOGNOTICE, "DVDPlayer: playing a dvd with menu's");
  }

  CLog::Log(LOGNOTICE, "Creating Demuxer");
  m_pDemuxer = CDVDFactoryDemuxer::CreateDemuxer(m_pInputStream);
  if (!m_pDemuxer || !m_pDemuxer->Open(m_pInputStream))
  {
    CLog::Log(LOGERROR, "Demuxer: Error opening, demuxer");
    // inputstream and the demuxer will be destroyed in OnExit()
    return;
  }

  // find first audio / video streams
  for (int i = 0; i < m_pDemuxer->GetNrOfStreams(); i++)
  {
    CDemuxStream* pStream = m_pDemuxer->GetStream(i);
    if (pStream->type == STREAM_AUDIO && audio_index < 0) audio_index = i;
    else if (pStream->type == STREAM_VIDEO && video_index < 0) video_index = i;
  }

  //m_dvdPlayerSubtitle.Init();
  //m_dvdPlayerSubtitle.FindSubtitles(m_filename);

  // open the streams
  if (audio_index >= 0) OpenAudioStream(audio_index);
  if (video_index >= 0) OpenVideoStream(video_index);

  // if we use libdvdnav, streaminfo is not avaiable, we will find this later on
  if (m_CurrentVideo.id < 0 && m_CurrentAudio.id < 0 &&
      m_pInputStream->IsStreamType(DVDSTREAM_TYPE_FILE) &&
      !m_pInputStream->HasExtension("vob"))
  {
    CLog::Log(LOGERROR, "%s: could not open codecs\n", m_filename);
    return;
  }

  // we are done initializing now, set the readyevent
  SetEvent(m_hReadyEvent);

  m_callback.OnPlayBackStarted();

  int iErrorCounter = 0;

  while (!m_bAbortRequest)
  {
    // if the queues are full, no need to read more
    while (!m_bAbortRequest && (!m_dvdPlayerAudio.AcceptsData() || !m_dvdPlayerVideo.AcceptsData()))
    {
      Sleep(10);
    }

    if (!m_bAbortRequest)
    {

      if(GetPlaySpeed() != DVD_PLAYSPEED_NORMAL && GetPlaySpeed() != DVD_PLAYSPEED_PAUSE)
      {
        bool bMenu = IsInMenu();

        // don't allow rewind in menu
        if (bMenu && GetPlaySpeed() < 0 ) SetPlaySpeed(DVD_PLAYSPEED_NORMAL);

        if( m_CurrentVideo.id >= 0
          && m_dvdPlayerVideo.GetCurrentPts() != DVD_NOPTS_VALUE
          && !bMenu )
        {

          // check how much off clock video is when ff/rw:ing
          // a problem here is that seeking isn't very accurate
          // and since the clock will be resynced after seek
          // we might actually not really be playing at the wanted
          // speed. we'd need to have some way to not resync the clock
          // after a seek to remember timing. still need to handle
          // discontinuities somehow
          // when seeking, give the player a headstart of 1 second to make sure the time it takes
          // to seek doesn't make a difference.
          __int64 iError = m_clock.GetClock() - m_dvdPlayerVideo.GetCurrentPts();

          if(GetPlaySpeed() > 0 && iError > DVD_MSEC_TO_TIME(1 * GetPlaySpeed()) )
          {
            CLog::Log(LOGDEBUG, "CDVDPlayer::Process - FF Seeking to catch up");
            SeekTime( GetTime() + GetPlaySpeed());
          }
          else if(GetPlaySpeed() < 0 && iError < DVD_MSEC_TO_TIME(1 * GetPlaySpeed()) )
          {
            SeekTime( GetTime() + GetPlaySpeed());
          }
          m_bDontSkipNextFrame = true;
        }
      }


      // handle messages send to this thread, like seek or demuxer reset requests
      HandleMessages();
      m_bReadAgain = false;

      // read a data frame from stream.
      CDVDDemux::DemuxPacket* pPacket = m_pDemuxer->Read();

      // in a read action, the dvd navigator can do certain actions that require
      // us to read again
      if (m_bReadAgain)
      {
        if (pPacket)
        {
          CDVDDemuxUtils::FreeDemuxPacket(pPacket);
        }
        continue;
      }

      if (!pPacket)
      {
        if( !m_pInputStream ) break;
        if( m_pInputStream->IsEOF() ) break;
        else if (m_dvd.state == DVDSTATE_WAIT && m_pInputStream->IsStreamType(DVDSTREAM_TYPE_DVD))
        {
          static_cast<CDVDInputStreamNavigator*>(m_pInputStream)->SkipWait();
          continue;
        }
        else if (m_dvd.state == DVDSTATE_STILL && m_pInputStream->IsStreamType(DVDSTREAM_TYPE_DVD)) continue;
        else
        {
          // keep on trying until user wants us to stop.
          iErrorCounter++;
          CLog::Log(LOGERROR, "Error reading data from demuxer");

          // maybe reseting the demuxer at this point would be a good idea, should it have failed in some way.
          if( iErrorCounter > 50 )
          {
            m_pDemuxer->Reset();
            iErrorCounter = 0;
          }

          continue;
        }
      }

      iErrorCounter = 0;

      // process subtitles
      if (pPacket->dts != DVD_NOPTS_VALUE)
      {
        //m_dvdPlayerSubtitle.Process(pPacket->dts);
      }


      CDemuxStream *pStream = m_pDemuxer->GetStream(pPacket->iStreamId);

      if( !pStream ) 
      {
        CLog::Log(LOGERROR, "CDVDPlayer::Process - Error demux packet doesn't belong to any stream");
        continue;
      }

      if (m_pInputStream && m_pInputStream->IsStreamType(DVDSTREAM_TYPE_DVD))
      {
        // Stream selection for DVD's this
        // should probably come as messages in the packet instead

        if( pStream->type == STREAM_SUBTITLE
        && pStream->iPhysicalId == m_dvd.iSelectedSPUStream
        && pStream->iId != m_CurrentSubtitle.id )
        {
          // dvd subtitle stream changed
          CloseSubtitleStream( true );
          OpenSubtitleStream( pStream->iId );
        }
        else if( pStream->type == STREAM_AUDIO
        && pStream->iPhysicalId == m_dvd.iSelectedAudioStream
        && pStream->iId != m_CurrentAudio.id )
        {
          // dvd audio stream changed,
          OpenAudioStream( pStream->iId );
        }
        else if( pStream->type == STREAM_VIDEO && m_CurrentVideo.id < 0 )
        {
          OpenVideoStream(pStream->iId);
        }

        // check so dvdnavigator didn't want us to close stream,
        // we allow lingering invalid audio/subtitle streams here to let player pass vts/cell borders more cleanly
        if( m_dvd.iSelectedAudioStream < 0 && m_CurrentAudio.id >= 0 ) CloseAudioStream( true );
        if( m_dvd.iSelectedSPUStream < 0 && m_CurrentVideo.id >= 0 )   CloseSubtitleStream( true );

        // make sure video stream hasn't been invalidated
        if (m_CurrentVideo.id >= 0 && m_pDemuxer->GetStream(m_CurrentVideo.id) == NULL) CloseVideoStream(false);
      }
      else
      {
        // for normal files, just open first stream
        if( m_CurrentSubtitle.id < 0 && pStream->type == STREAM_SUBTITLE ) OpenSubtitleStream(pStream->iId);
        if( m_CurrentAudio.id < 0    && pStream->type == STREAM_AUDIO )    OpenAudioStream(pStream->iId);
        if( m_CurrentVideo.id < 0    && pStream->type == STREAM_VIDEO )    OpenVideoStream(pStream->iId);

        // check so that none of our streams has become invalid
        if (m_CurrentAudio.id >= 0    && m_pDemuxer->GetStream(m_CurrentAudio.id) == NULL)     CloseAudioStream(false);
        if (m_CurrentSubtitle.id >= 0 && m_pDemuxer->GetStream(m_CurrentSubtitle.id) == NULL ) CloseSubtitleStream(false);
        if (m_CurrentVideo.id >= 0    && m_pDemuxer->GetStream(m_CurrentVideo.id) == NULL)     CloseVideoStream(false);
      }


      /* process packet if it belongs to selected stream. for dvd's down't allow automatic opening of streams*/
      LockStreams();
      {
        if (pPacket->iStreamId == m_CurrentAudio.id)
        {
          ProcessAudioData(pStream, pPacket);
        }
        else if (pPacket->iStreamId == m_CurrentVideo.id)
        {
          ProcessVideoData(pStream, pPacket);
        }
        else if (pPacket->iStreamId == m_CurrentSubtitle.id)
        {
          ProcessSubData(pStream, pPacket);
        }
        else CDVDDemuxUtils::FreeDemuxPacket(pPacket); // free it since we won't do anything with it
      }
      UnlockStreams();
    }
  }
}

void CDVDPlayer::ProcessAudioData(CDemuxStream* pStream, CDVDDemux::DemuxPacket* pPacket)
{
  // if we have no audio stream yet, openup the audio device now
  if (m_CurrentAudio.id < 0) OpenAudioStream( pPacket->iStreamId );

  if (m_CurrentAudio.stream != (void*)pStream)
  {
    /* check so that dmuxer hints or extra data hasn't changed */
    /* if they have, reopen stream */

    if( m_CurrentAudio.hint != CDVDStreamInfo(*pStream, true) )
    { 
      // we don't actually have to close audiostream here first, as 
      // we could send it as a stream message. only problem 
      // is how to notify player if a stream change failed.
      CloseAudioStream( true );
      OpenAudioStream( pPacket->iStreamId );
    }

    m_CurrentAudio.stream = (void*)pStream;
  }

  /* we should check for discontinuity here */
  if( pPacket->dts != DVD_NOPTS_VALUE )
  {
    if( m_CurrentAudio.dts == DVD_NOPTS_VALUE )
    {
      /* NOP */
    }
    else if( pPacket->dts + DVD_MSEC_TO_TIME(1) < m_CurrentAudio.dts )
    { /* timestamps is wrapping back */
      SyncronizePlayers(pPacket->dts, pPacket->dts);
    }
    else if( pPacket->dts > m_CurrentAudio.dts + DVD_MSEC_TO_TIME(200) )
    { /* timestamps larger than 200ms after last timestamp */
      /* should be using packet duration if avaliable */
      SyncronizePlayers(pPacket->dts, pPacket->dts);
    }
    m_CurrentAudio.dts = pPacket->dts;
  }

  //If this is the first packet after a discontinuity, send it as a resync
  if( !(m_dvd.iFlagSentStart & 1) )
  {
    m_dvd.iFlagSentStart |= 1;
    m_dvdPlayerAudio.SendMessage(new CDVDMsgGeneralResync(pPacket->pts, pPacket->dts));
  }

  if (m_CurrentAudio.id >= 0)
    m_dvdPlayerAudio.SendMessage(new CDVDMsgDemuxerPacket(pPacket, pPacket->iSize));
  else
    CDVDDemuxUtils::FreeDemuxPacket(pPacket);
}

void CDVDPlayer::ProcessVideoData(CDemuxStream* pStream, CDVDDemux::DemuxPacket* pPacket)
{
  // if we have no video stream yet, openup the video device now
  if (m_CurrentVideo.id < 0) OpenVideoStream(pPacket->iStreamId);

  if (m_CurrentVideo.stream != (void*)pStream)
  {
    /* check so that dmuxer hints or extra data hasn't changed */
    /* if they have reopen stream */

    if( m_CurrentVideo.hint != CDVDStreamInfo(*pStream, true) )
    {
      CloseVideoStream(true);
      OpenVideoStream( pPacket->iStreamId );
    }

    m_CurrentVideo.stream = (void*)pStream;
  }


  if (m_bDontSkipNextFrame)
  {
    m_dvdPlayerVideo.SendMessage(new CDVDMsgVideoNoSkip());
    m_bDontSkipNextFrame = false;
  }

  /* we should check for discontinuity here */
  if( pPacket->dts != DVD_NOPTS_VALUE )
  {
    /* this detection doesn't work correctly for still frames that are wrapping back */
    /* problem being that each still frame consist's of two packets, one for that image*/
    /* and one for the END_SEQUENCE_CODE with our hack in ffmpeg. */
    /* both packets has the same dts value, wich makes it impossible to make this check */
    /* monotonly increasing timestamps (<=), wich is probably what should be what we should check */

    /* to avoid unneeded resyncs if demuxer isn't exact on the dts we add 1ms to the value we check*/
    if( m_CurrentVideo.dts == DVD_NOPTS_VALUE )
    {
      /* NOP */
    }
    else if( pPacket->dts + DVD_MSEC_TO_TIME(1) < m_CurrentVideo.dts )
    { /* timestamps are wrapping back */
      SyncronizePlayers(pPacket->dts, pPacket->dts);
    }
    else if( pPacket->dts > m_CurrentVideo.dts + DVD_MSEC_TO_TIME(200) )
    { /* timestamps higher than last packet */
      /* should be using packet duration if avaliable*/
      SyncronizePlayers(pPacket->dts, pPacket->dts);
    }
    m_CurrentVideo.dts = pPacket->dts;
  }

  //If this is the first packet after a discontinuity, send it as a resync
  if( !(m_dvd.iFlagSentStart & 1) )
  {
    m_dvd.iFlagSentStart |= 1;
    m_dvdPlayerVideo.SendMessage(new CDVDMsgGeneralResync(pPacket->pts, pPacket->dts));
  }

  if (m_CurrentVideo.id >= 0)
    m_dvdPlayerVideo.SendMessage(new CDVDMsgDemuxerPacket(pPacket, pPacket->iSize));
  else
    CDVDDemuxUtils::FreeDemuxPacket(pPacket);
}

void CDVDPlayer::ProcessSubData(CDemuxStream* pStream, CDVDDemux::DemuxPacket* pPacket)
{
  // if no subtitle stream is selected, select this one
  if( m_CurrentSubtitle.id < 0 ) OpenSubtitleStream( pStream->iId );

  if (pStream->codec == 0x17000) //CODEC_ID_DVD_SUBTITLE)
  {
    CSPUInfo* pSPUInfo = m_dvdspus.AddData(pPacket->pData, pPacket->iSize, pPacket->pts);

    if (pSPUInfo)
    {
      CLog::Log(LOGDEBUG, "CDVDPlayer::ProcessSubData: Got complete SPU packet");

      m_overlayContainer.Add(pSPUInfo);

      if (pSPUInfo->bForced)
      {
        // recieved new menu overlay (button), retrieve cropping information
        UpdateOverlayInfo(LIBDVDNAV_BUTTON_NORMAL);
      }
    }
  }

  // free it, content is already copied into a special structure
  CDVDDemuxUtils::FreeDemuxPacket(pPacket);
}

void CDVDPlayer::SyncronizePlayers(__int64 dts, __int64 pts)
{
  //TODO make sure dts/pts is a little bit away from last syncevent
  //static __int64 olddts = DVD_NOPTS_VALUE;
  
  //if( olddts - DVD_MSEC_TO_TIME(500) < dts && dts < olddts + DVD_MSEC_TO_TIME(500) ) return;
  //olddts = dts;

  int players = 0;
  if( m_CurrentAudio.id >= 0 ) players++;
  if( m_CurrentVideo.id >= 0 ) players++;

  // timeout of this packet.. if timeout has passed when processed, no waiting will be done
  // since our audio queue is quite large (around 7secs on 2ch ac3) this needs to be quite big
  // it should only be a fallback if something goes wron anyway
  const int timeout = 10*1000; // in milliseconds

  if( players )
  {
    CDVDMsgGeneralSynchronize* message = new CDVDMsgGeneralSynchronize(timeout);
    if( m_CurrentAudio.id >= 0 )
    {
      message->Acquire();
      m_dvdPlayerAudio.SendMessage(message);
    }
    if( m_CurrentVideo.id >= 0 )
    {
      message->Acquire();
      m_dvdPlayerVideo.SendMessage(message);
    }
    message->Release();

    CDVDMsgGeneralSetClock* clock = new CDVDMsgGeneralSetClock(pts, dts);
    if( m_CurrentVideo.id >= 0 &&  m_CurrentAudio.id < 0 )
    {
      clock->Acquire();
      m_dvdPlayerVideo.SendMessage(clock);
    }
    else if( m_CurrentAudio.id >= 0 )
    {
      clock->Acquire();
      m_dvdPlayerAudio.m_messageQueue.Put(clock);
    }
    clock->Release();
  }
}

void CDVDPlayer::OnExit()
{
  g_dvdPerformanceCounter.DisableMainPerformance();

  CLog::Log(LOGNOTICE, "CDVDPlayer::OnExit()");

  // close each stream
  if (!m_bAbortRequest) CLog::Log(LOGNOTICE, "DVDPlayer: eof, waiting for queues to empty");
  if (m_CurrentAudio.id >= 0)
  {
    CLog::Log(LOGNOTICE, "DVDPlayer: closing audio stream");
    CloseAudioStream(!m_bAbortRequest);
  }
  if (m_CurrentVideo.id >= 0)
  {
    CLog::Log(LOGNOTICE, "DVDPlayer: closing video stream");
    CloseVideoStream(!m_bAbortRequest);
  }

  // destroy the demuxer
  if (m_pDemuxer)
  {
    CLog::Log(LOGNOTICE, "CDVDPlayer::OnExit() deleting demuxer");
    m_pDemuxer->Dispose();
    delete m_pDemuxer;
  }
  m_pDemuxer = NULL;

  // destroy the inputstream
  if (m_pInputStream)
  {
    CLog::Log(LOGNOTICE, "CDVDPlayer::OnExit() deleting input stream");
    delete m_pInputStream;
  }
  m_pInputStream = NULL;

  // close subtitle stuff
  CLog::Log(LOGNOTICE, "CDVDPlayer::OnExit() deiniting subtitle handler");
  m_dvdPlayerSubtitle.DeInit();

  // if we didn't stop playing, advance to the next item in xbmc's playlist
  if (!m_bAbortRequest) m_callback.OnPlayBackEnded();

  m_messenger.End();
  
  // set event to inform openfile something went wrong in case openfile is still waiting for this event
  SetEvent(m_hReadyEvent);
}

void CDVDPlayer::HandleMessages()
{
  CDVDMsg* pMsg;
  
  MsgQueueReturnCode ret = m_messenger.Get(&pMsg, 0);
   
  while (ret == MSGQ_OK)
  {
    if (pMsg->IsType(CDVDMsg::PLAYER_SEEK))
    {
      CDVDMsgPlayerSeek* pMsgPlayerSeek = (CDVDMsgPlayerSeek*)pMsg;
      
      if (m_pInputStream && m_pInputStream->IsStreamType(DVDSTREAM_TYPE_DVD))
      {
        // We can't seek in a menu
        if (!IsInMenu())
        {
          // need to get the seek based on file positition working in CDVDInputStreamNavigator
          // so that demuxers can control the stream (seeking in this case)
          // for now use time based seeking
          CLog::Log(LOGDEBUG, "CDVDInputStreamNavigator seek to: %d", pMsgPlayerSeek->GetTime());
          if (((CDVDInputStreamNavigator*)m_pInputStream)->Seek(pMsgPlayerSeek->GetTime()))
          {
            CLog::Log(LOGDEBUG, "CDVDInputStreamNavigator seek to: %d, succes", pMsgPlayerSeek->GetTime());
            //FlushBuffers(); buffers will be flushed by a hop from dvdnavigator
          }
          else CLog::Log(LOGWARNING, "error while seeking");
        }
        else CLog::Log(LOGWARNING, "can't seek in a menu");
      }
      else
      {
        CLog::Log(LOGDEBUG, "demuxer seek to: %d", pMsgPlayerSeek->GetTime());
        if (m_pDemuxer->Seek(pMsgPlayerSeek->GetTime()))
        {
          CLog::Log(LOGDEBUG, "demuxer seek to: %d, succes", pMsgPlayerSeek->GetTime());
          FlushBuffers();
        }
        else CLog::Log(LOGWARNING, "error while seeking");
      }

      // set flag to indicate we have finished a seeking request
      g_infoManager.m_performingSeek = false;
    }
    else if (pMsg->IsType(CDVDMsg::DEMUXER_RESET))
    {
        m_CurrentAudio.stream = NULL;
        m_CurrentVideo.stream = NULL;
        m_CurrentSubtitle.stream = NULL;

        // we need to reset the demuxer, probably because the streams have changed
        m_pDemuxer->Reset();
    }
    else if (pMsg->IsType(CDVDMsg::PLAYER_SET_AUDIOSTREAM))
    {
      CDVDMsgPlayerSetAudioStream* pMsgPlayerSetAudioStream = (CDVDMsgPlayerSetAudioStream*)pMsg;

      CDemuxStream* pStream = m_pDemuxer->GetStreamFromAudioId(pMsgPlayerSetAudioStream->GetStreamId());
      if (pStream)
      {
        LockStreams();

        CloseAudioStream(false);

        // for dvd's we set it using the navigater, it will then send a change audio to the dvdplayer
        if (m_pInputStream && m_pInputStream->IsStreamType(DVDSTREAM_TYPE_DVD))
        {
          m_dvd.iSelectedAudioStream = -1;

          CDVDInputStreamNavigator* pInput = (CDVDInputStreamNavigator*)m_pInputStream;
          pInput->SetActiveAudioStream(pMsgPlayerSetAudioStream->GetStreamId());
        }
        else if (m_pDemuxer)
        {
          CDemuxStream* pStream = m_pDemuxer->GetStreamFromAudioId(pMsgPlayerSetAudioStream->GetStreamId());
          if( pStream ) OpenAudioStream(pStream->iId);
        }

        UnlockStreams();
      }
    }
    else if (pMsg->IsType(CDVDMsg::PLAYER_SET_SUBTITLESTREAM))
    {
      CDVDMsgPlayerSetSubtitleStream* pMsgPlayerSetSubtileStream = (CDVDMsgPlayerSetSubtitleStream*)pMsg;

      LockStreams();

      CloseSubtitleStream(false);

      // for dvd's we set it using the navigater, it will then send a change subtitle to the dvdplayer
      if (m_pInputStream && m_pInputStream->IsStreamType(DVDSTREAM_TYPE_DVD))
      {
        m_dvd.iSelectedSPUStream = -1;

        CDVDInputStreamNavigator* pStream = (CDVDInputStreamNavigator*)m_pInputStream;
        pStream->SetActiveSubtitleStream(pMsgPlayerSetSubtileStream->GetStreamId());
      }
      else if (m_pDemuxer)
      {
        CDemuxStream* pStream = m_pDemuxer->GetStreamFromSubtitleId(pMsgPlayerSetSubtileStream->GetStreamId());
        if (pStream) OpenSubtitleStream(pStream->iId);
      }

      UnlockStreams();
    }
    else if (pMsg->IsType(CDVDMsg::PLAYER_SET_SUBTITLESTREAM))
    {
      CDVDMsgPlayerSetState* pMsgPlayerSetState = (CDVDMsgPlayerSetState*)pMsg;

      if (m_pInputStream && m_pInputStream->IsStreamType(DVDSTREAM_TYPE_DVD))
      {
        ((CDVDInputStreamNavigator*)m_pInputStream)->SetNavigatorState(pMsgPlayerSetState->GetState());
      }
    }


    pMsg->Release();
    ret = m_messenger.Get(&pMsg, 0);
  }
}

void CDVDPlayer::SetPlaySpeed(int speed)
{
  m_playSpeed = speed;

  // the clock needs to be paused or unpaused by seperate calls
  // audio and video part do not
  if (speed == DVD_PLAYSPEED_NORMAL) m_clock.Resume();
  else if (speed == DVD_PLAYSPEED_PAUSE) m_clock.Pause();
  else m_clock.SetSpeed(speed / DVD_PLAYSPEED_NORMAL); // XXX

  // if playspeed is different then DVD_PLAYSPEED_NORMAL or DVD_PLAYSPEED_PAUSE
  // audioplayer, stops outputing audio to audiorendere, but still tries to
  // sleep an correct amount for each packet
  // videoplayer just plays faster after the clock speed has been increased
  // 1. disable audio
  // 2. skip frames and adjust their pts or the clock
  m_dvdPlayerAudio.SetSpeed(speed);
  m_dvdPlayerVideo.SetSpeed(speed);
}

void CDVDPlayer::Pause()
{
  int iSpeed = GetPlaySpeed();

  // return to normal speed if it was paused before, pause otherwise
  if (iSpeed == DVD_PLAYSPEED_PAUSE) SetPlaySpeed(DVD_PLAYSPEED_NORMAL);
  else SetPlaySpeed(DVD_PLAYSPEED_PAUSE);
}

bool CDVDPlayer::IsPaused() const
{
  return (m_playSpeed == DVD_PLAYSPEED_PAUSE);
}

bool CDVDPlayer::HasVideo()
{
  if (m_pInputStream)
  {
    if (m_pInputStream->IsStreamType(DVDSTREAM_TYPE_DVD) || m_CurrentVideo.id >= 0) return true;
  }
  return false;
}

bool CDVDPlayer::HasAudio()
{
  return (m_CurrentAudio.id >= 0);
}

void CDVDPlayer::SwitchToNextLanguage()
{}

void CDVDPlayer::ToggleSubtitles()
{
  SetSubtitleVisible(!GetSubtitleVisible());
  //m_bRenderSubtitle = !m_bRenderSubtitle;
}

void CDVDPlayer::SubtitleOffset(bool bPlus)
{
}

void CDVDPlayer::Seek(bool bPlus, bool bLargeStep)
{
  float percent = (bLargeStep ? 10.0f : 2.0f);

  if (bPlus) percent += GetPercentage();
  else percent = GetPercentage() - percent;

  if (percent >= 0 && percent <= 100)
  {
    // should be modified to seektime
    SeekPercentage(percent);
  }
}

void CDVDPlayer::ToggleFrameDrop()
{
}

void CDVDPlayer::GetAudioInfo(CStdString& strAudioInfo)
{
  string strDemuxerInfo;
  CStdString strPlayerInfo;
  if (!m_bStop && m_CurrentAudio.id >= 0)
  {
    CDemuxStream* pStream = m_pDemuxer->GetStream(m_CurrentAudio.id);
    if (pStream && pStream->type == STREAM_AUDIO)
    {
      ((CDemuxStreamAudio*)pStream)->GetStreamInfo(strDemuxerInfo);
    }
  }

  int bsize = m_dvdPlayerAudio.m_messageQueue.GetDataSize();
  if (bsize > 0) bsize = (int)(((double)m_dvdPlayerAudio.m_messageQueue.GetDataSize() / m_dvdPlayerAudio.m_messageQueue.GetMaxDataSize()) * 100);
  if (bsize > 99) bsize = 99;
  strPlayerInfo.Format("aq size: %i, cpu: %i%%", bsize, (int)(m_dvdPlayerAudio.GetRelativeUsage()*100));
  strAudioInfo.Format("D( %s ), P( %s )", strDemuxerInfo.c_str(), strPlayerInfo.c_str());
}

void CDVDPlayer::GetVideoInfo(CStdString& strVideoInfo)
{
  if( m_bStop ) return;

  string strDemuxerInfo;
  CStdString strPlayerInfo;
  if (m_CurrentVideo.id >= 0)
  {
    CDemuxStream* pStream = m_pDemuxer->GetStream(m_CurrentVideo.id);
    if (pStream && pStream->type == STREAM_VIDEO)
    {
      ((CDemuxStreamVideo*)pStream)->GetStreamInfo(strDemuxerInfo);
    }
  }

  int bsize = m_dvdPlayerVideo.m_messageQueue.GetDataSize();
  if (bsize > 0) bsize = (int)(((double)m_dvdPlayerVideo.m_messageQueue.GetDataSize() / m_dvdPlayerVideo.m_messageQueue.GetMaxDataSize()) * 100);
  if (bsize > 99) bsize = 99;
  strPlayerInfo.Format("vq size: %i, cpu: %i%%", bsize, (int)(m_dvdPlayerVideo.GetRelativeUsage()*100+m_dvdPlayerVideo.m_PresentThread.GetRelativeUsage()*100));
  strVideoInfo.Format("D( %s ), P( %s )", strDemuxerInfo.c_str(), strPlayerInfo.c_str());
}

void CDVDPlayer::GetGeneralInfo(CStdString& strGeneralInfo)
{
  if (!m_bStop)
  {
    double dDelay = (double)m_dvdPlayerVideo.GetDelay() / DVD_TIME_BASE;

    __int64 apts = m_dvdPlayerAudio.GetCurrentPts();
    __int64 vpts = m_dvdPlayerVideo.GetCurrentPts();
    double dDiff = 0;

    if( apts != DVD_NOPTS_VALUE && vpts != DVD_NOPTS_VALUE )
      dDiff = (double)(apts - vpts) / DVD_TIME_BASE;

    int iFramesDropped = m_dvdPlayerVideo.GetNrOfDroppedFrames();

    strGeneralInfo.Format("DVD Player ad:%6.3f, a/v:%6.3f, dropped:%d, cpu: %i%%", dDelay, dDiff, iFramesDropped, (int)(CThread::GetRelativeUsage()*100));
  }
}

void CDVDPlayer::AudioOffset(bool bPlus)
{
}

void CDVDPlayer::SwitchToNextAudioLanguage()
{
}

void CDVDPlayer::SeekPercentage(float iPercent)
{

  __int64 iTotalMsec = GetTotalTimeInMsec();
  __int64 iTime = (__int64)(iTotalMsec * iPercent / 100);

  SeekTime(iTime);
}

float CDVDPlayer::GetPercentage()
{
  __int64 iTotalTime = GetTotalTimeInMsec();

  if (iTotalTime != 0)
  {
    return GetTime() * 100 / (float)iTotalTime;
  }

  return 0.0f;
}

//This is how much audio is delayed to video, we count the oposite in the dvdplayer
void CDVDPlayer::SetAVDelay(float fValue)
{
  m_dvdPlayerVideo.SetDelay( - (__int64)(fValue * DVD_TIME_BASE) ) ;
}

float CDVDPlayer::GetAVDelay()
{
  return - m_dvdPlayerVideo.GetDelay() / (float)DVD_TIME_BASE;
}

void CDVDPlayer::SetSubTitleDelay(float fValue)
{
}

float CDVDPlayer::GetSubTitleDelay()
{
  return 0.0;
}

// priority: 1: libdvdnav, 2: external subtitles, 3: muxed subtitles
int CDVDPlayer::GetSubtitleCount()
{
  // for dvd's we get the info from libdvdnav
  if (m_pInputStream && m_pInputStream->IsStreamType(DVDSTREAM_TYPE_DVD))
  {
    CDVDInputStreamNavigator* pStream = (CDVDInputStreamNavigator*)m_pInputStream;
    return pStream->GetSubTitleStreamCount();
  }

  // for external subtitle files
  // if (m_subtitle.vecSubtitleFiles.size() > 0) return m_subtitle.vecSubtitleFiles.size();

  // last one is the demuxer
  if (m_pDemuxer) return m_pDemuxer->GetNrOfSubtitleStreams();

  return 0;
}

int CDVDPlayer::GetSubtitle()
{
  // libdvdnav
  if (m_pInputStream && m_pInputStream->IsStreamType(DVDSTREAM_TYPE_DVD))
  {
    CDVDInputStreamNavigator* pStream = (CDVDInputStreamNavigator*)m_pInputStream;
    return pStream->GetActiveSubtitleStream();
  }

  // external subtitles
  else
  {
    int iSubtitleStreams = m_pDemuxer->GetNrOfSubtitleStreams();
    for (int i = 0; i < iSubtitleStreams; i++)
    {
      CDemuxStream* pStream = m_pDemuxer->GetStreamFromSubtitleId(i);
      if (pStream && pStream->iId == m_CurrentSubtitle.id) return i;
    }
  }
  return -1;
}

void CDVDPlayer::GetSubtitleName(int iStream, CStdString &strStreamName)
{
  strStreamName.Format("%d. ", iStream);
  CDemuxStream* pStream = m_pDemuxer->GetStreamFromSubtitleId(iStream);

  if( !pStream )
  {
    CLog::Log(LOGWARNING, "CDVDPlayer::GetSubtitleName: Invalid stream: id %i", iStream);
    strStreamName += " (Invalid)";
    return;
  }

  if (m_pInputStream && m_pInputStream->IsStreamType(DVDSTREAM_TYPE_DVD))
  {
    CDVDInputStreamNavigator* pStream = (CDVDInputStreamNavigator*)m_pInputStream;
    strStreamName += pStream->GetSubtitleStreamLanguage(iStream);
  }
  else
  {
    CStdString strName;
    pStream->GetStreamName(strName);
    strStreamName += strName;
  }
}

void CDVDPlayer::SetSubtitle(int iStream)
{
  m_messenger.Put(new CDVDMsgPlayerSetSubtitleStream(iStream));
}

bool CDVDPlayer::GetSubtitleVisible()
{
  //if (!m_bRenderSubtitle) return false;

  //int id = m_dvd.iSelectedSPUStream - 0x20;
  //if (id >= 0 && id <= 0x3f) return true;

  //return false;

  return m_bRenderSubtitle;
}

void CDVDPlayer::SetSubtitleVisible(bool bVisible)
{
  g_stSettings.m_currentVideoSettings.m_SubtitleOn = bVisible;
  m_bRenderSubtitle = bVisible;

  // maybe it is better to let m_dvdPlayerVideo use a callback to get this value
  m_dvdPlayerVideo.EnableSubtitle(bVisible);

  if (m_pInputStream && m_pInputStream->IsStreamType(DVDSTREAM_TYPE_DVD))
  {
    CDVDInputStreamNavigator* pStream = (CDVDInputStreamNavigator*)m_pInputStream;
    pStream->EnableSubtitleStream(bVisible);
  }
}

int CDVDPlayer::GetAudioStreamCount()
{
  // for dvd's we get the information from libdvdnav
  if (m_pInputStream && m_pInputStream->IsStreamType(DVDSTREAM_TYPE_DVD))
  {
    CDVDInputStreamNavigator* pStream = (CDVDInputStreamNavigator*)m_pInputStream;
    return pStream->GetAudioStreamCount();
  }
  else if (m_pDemuxer)
  {
    return m_pDemuxer->GetNrOfAudioStreams();
  }
  return 0;
}

int CDVDPlayer::GetAudioStream()
{
  if (m_pInputStream && m_pInputStream->IsStreamType(DVDSTREAM_TYPE_DVD))
  {
    CDVDInputStreamNavigator* pStream = (CDVDInputStreamNavigator*)m_pInputStream;
    return pStream->GetActiveAudioStream();
  }
  else
  {
    int iAudioStreams = m_pDemuxer->GetNrOfAudioStreams();
    for (int i = 0; i < iAudioStreams; i++)
    {
      CDemuxStream* pStream = m_pDemuxer->GetStreamFromAudioId(i);
      if (pStream && pStream->iId == m_CurrentAudio.id) return i;
    }
  }
  return -1;
}

void CDVDPlayer::GetAudioStreamName(int iStream, CStdString& strStreamName)
{
  strStreamName.Format("%d. ", iStream);
  CDemuxStreamAudio* pStream = m_pDemuxer->GetStreamFromAudioId(iStream);

  if( !pStream )
  {
    CLog::Log(LOGWARNING, "CDVDPlayer::GetAudioStreamName: Invalid stream: id %i", iStream);
    strStreamName += " (Invalid)";
    return;
  }

  if (m_pInputStream && m_pInputStream->IsStreamType(DVDSTREAM_TYPE_DVD))
  {
    CDVDInputStreamNavigator* pStream = (CDVDInputStreamNavigator*)m_pInputStream;
    strStreamName += pStream->GetAudioStreamLanguage(iStream);
  }
  else
  {
    CStdString strName;
    pStream->GetStreamName(strName);
    strStreamName += strName;
  }

  std::string strType;
  pStream->GetStreamType(strType);
  if( strType.length() > 0 )
  {
    strStreamName += " - ";
    strStreamName += strType;
  }
}

void CDVDPlayer::SetAudioStream(int iStream)
{
  m_messenger.Put(new CDVDMsgPlayerSetAudioStream(iStream));
}

void CDVDPlayer::SeekTime(__int64 iTime)
{
  if (m_pDemuxer) m_messenger.Put(new CDVDMsgPlayerSeek((int)iTime));
}

// return the time in milliseconds
__int64 CDVDPlayer::GetTime()
{
  // get timing and seeking from libdvdnav for dvd's
  if (m_pInputStream && m_pInputStream->IsStreamType(DVDSTREAM_TYPE_DVD))
  {
    return ((CDVDInputStreamNavigator*)m_pInputStream)->GetTime(); // we should take our buffers into account
  }

  int iMsecs = (int)(m_clock.GetClock() / (DVD_TIME_BASE / 1000));
  //if (m_pDemuxer)
  //{
  //  int iMsecsStart = m_pDemuxer->GetStreamStart();
  //  if (iMsecs > iMsecsStart) iMsecs -=iMsecsStart;
  //}

  return iMsecs;
}

// return length in msec
__int64 CDVDPlayer::GetTotalTimeInMsec()
{
  // get timing and seeking from libdvdnav for dvd's
  if (m_pInputStream && m_pInputStream->IsStreamType(DVDSTREAM_TYPE_DVD))
  {
    return ((CDVDInputStreamNavigator*)m_pInputStream)->GetTotalTime(); // we should take our buffers into account
  }

  if (m_pDemuxer) return m_pDemuxer->GetStreamLenght();
  return 0;

}

// return length in seconds.. this should be changed to return in milleseconds throughout xbmc
int CDVDPlayer::GetTotalTime()
{
  return (int)(GetTotalTimeInMsec() / 1000);
}

void CDVDPlayer::ToFFRW(int iSpeed)
{
  // can't rewind in menu as seeking isn't possible
  // forward is fine
  if (iSpeed < 0 && IsInMenu()) return;
  SetPlaySpeed(iSpeed * DVD_PLAYSPEED_NORMAL);
}

void CDVDPlayer::ShowOSD(bool bOnoff)
{}

bool CDVDPlayer::GetSubtitleExtension(CStdString &strSubtitleExtension)
{
  return false;
}

void CDVDPlayer::UpdateSubtitlePosition()
{}

void CDVDPlayer::RenderSubtitles()
{}

bool CDVDPlayer::OpenAudioStream(int iStream)
{
  CLog::Log(LOGNOTICE, "Opening audio stream: %i", iStream);

  CDemuxStream* pStream = m_pDemuxer->GetStream(iStream);

  if( !pStream )
  {
    CLog::Log(LOGERROR, "Stream doesn't exist");
    return false;
  }

  if( pStream->disabled )
    return false;

  if (pStream->type != STREAM_AUDIO)
  {
    CLog::Log(LOGERROR, "Streamtype is not STREAM_AUDIO");
    return false;
  }

  if( m_CurrentAudio.id < 0 &&  m_CurrentVideo.id >= 0 )
  { // up until now we wheren't playing audio, but we did play video
    // this will change what is used to sync the dvdclock.
    // since the new audio data doesn't have to have any relation
    // to the current video data in the packet que, we have to
    // wait for it to empty

    // this happens if a new cell has audio data, but previous didn't
    // and both have video data

    m_dvdPlayerVideo.WaitForBuffers();
  }

  CDVDStreamInfo hint(*pStream, true);
  if( m_CurrentAudio.id >= 0 )
  {
    CDVDStreamInfo* phint = new CDVDStreamInfo(hint);
    m_dvdPlayerAudio.SendMessage(new CDVDMsgGeneralStreamChange(phint));
  }
  else if (!m_dvdPlayerAudio.OpenStream( hint ) )
  {
    /* mark stream as disabled, to disallaw further attempts*/
    CLog::Log(LOGWARNING, "CDVDPlayer::OpenAudioStream - Unsupported stream %d. Stream disabled.", iStream);
    pStream->disabled = true;

    return false;
  }

  /* store information about stream */
  m_CurrentAudio.id = iStream;
  m_CurrentAudio.hint = hint;
  m_CurrentAudio.stream = (void*)pStream;

  m_dvdPlayerAudio.SetPriority(THREAD_PRIORITY_HIGHEST + 4);

  return true;
}

bool CDVDPlayer::OpenVideoStream(int iStream)
{
  CLog::Log(LOGNOTICE, "Opening video stream: %i", iStream);

  CDemuxStream* pStream = m_pDemuxer->GetStream(iStream);

  if( !pStream )
  {
    CLog::Log(LOGERROR, "Stream doesn't exist");
    return false;
  }

  if( pStream->disabled )
    return false;

  if (pStream->type != STREAM_VIDEO)
  {
    CLog::Log(LOGERROR, "Streamtype is not STREAM_VIDEO");
    return false;
  }

  CDVDStreamInfo hint(*pStream, true);

  if (!m_dvdPlayerVideo.OpenStream(hint))
  {
    // mark stream as disabled, to disallaw further attempts
    CLog::Log(LOGWARNING, "CDVDPlayer::OpenVideoStream - Unsupported stream %d. Stream disabled.", iStream);
    pStream->disabled = true;
    return false;
  }

  /* store information about stream */
  m_CurrentVideo.id = iStream;
  m_CurrentVideo.hint = hint;
  m_CurrentVideo.stream = (void*)pStream;

  m_dvdPlayerVideo.SetPriority(THREAD_PRIORITY_ABOVE_NORMAL);

  return true;

}

bool CDVDPlayer::OpenSubtitleStream(int iStream)
{
  CDemuxStream* pStream = m_pDemuxer->GetStream(iStream);

  if( !pStream ) return false;

  m_CurrentSubtitle.id = iStream;
  m_CurrentSubtitle.hint.Assign(*pStream, true);
  m_CurrentSubtitle.stream = (void*)pStream;

  return true;
}

bool CDVDPlayer::CloseAudioStream(bool bWaitForBuffers)
{
  CLog::Log(LOGNOTICE, "Closing audio stream");

  LockStreams();
  // cannot close the stream if it does not exist
  if (m_CurrentAudio.id >= 0)
  {
    m_dvdPlayerAudio.CloseStream(bWaitForBuffers);

    m_CurrentAudio.id = -1;
    m_CurrentAudio.dts = DVD_NOPTS_VALUE;
    m_CurrentAudio.hint.Clear();
  }
  UnlockStreams();

  return true;
}

bool CDVDPlayer::CloseVideoStream(bool bWaitForBuffers) // bWaitForBuffers currently not used
{
  CLog::Log(LOGNOTICE, "Closing video stream");

  if (m_CurrentVideo.id < 0) return false; // can't close stream if the stream does not exist

  m_dvdPlayerVideo.CloseStream(bWaitForBuffers);

  m_CurrentVideo.id = -1;
  m_CurrentVideo.dts = DVD_NOPTS_VALUE;
  m_CurrentVideo.hint.Clear();

  return true;
}

bool CDVDPlayer::CloseSubtitleStream(bool bKeepOverlays)
{
  m_dvdspus.Flush();

  if( !bKeepOverlays ) m_overlayContainer.Clear();

  m_CurrentSubtitle.id = -1;
  m_CurrentSubtitle.dts = DVD_NOPTS_VALUE;
  m_CurrentSubtitle.hint.Clear();

  return true;
}

void CDVDPlayer::FlushBuffers()
{
  //m_pDemuxer->Flush();
  m_dvdPlayerAudio.Flush();
  m_dvdPlayerVideo.Flush();
  //m_dvdPlayerSubtitle.Flush();

  // clear subtitle and menu overlays
  m_overlayContainer.Clear();

  m_dvd.iFlagSentStart = 0; //We will have a discontinuity here

  //m_bReadAgain = true; // XXX
  // this makes sure a new packet is read
}

// since we call ffmpeg functions to decode, this is being called in the same thread as ::Process() is
int CDVDPlayer::OnDVDNavResult(void* pData, int iMessage)
{
  if (m_pInputStream->IsStreamType(DVDSTREAM_TYPE_DVD))
  {
    CDVDInputStreamNavigator* pStream = (CDVDInputStreamNavigator*)m_pInputStream;

    switch (iMessage)
    {
    case DVDNAV_WAIT:
      {
        // we are about to pass a cell border, this might trigger a vts change, or some sort of stream change
        // make sure demuxer has processed all it's data, wait will be skipped when demuxer returns a read error
        if( m_dvd.state != DVDSTATE_WAIT )
        {
          CLog::Log(LOGDEBUG, "DVDNAV_WAIT");
          m_dvd.state = DVDSTATE_WAIT;
        }
        return NAVRESULT_HOLD;
      }
      break;
    case DVDNAV_STILL_FRAME:
      {
        //CLog::Log(LOGDEBUG, "DVDNAV_STILL_FRAME");

        dvdnav_still_event_t *still_event = (dvdnav_still_event_t *)pData;
        // should wait the specified time here while we let the player running
        // after that call dvdnav_still_skip(m_dvdnav);

        if (m_dvd.state == DVDSTATE_STILL)
        {
          if (m_dvd.iDVDStillTime < 0xff)
          {
            if (((GetTickCount() - m_dvd.iDVDStillStartTime) / 1000) >= ((DWORD)m_dvd.iDVDStillTime))
            {
              m_dvd.iDVDStillTime = 0;
              m_dvd.iDVDStillStartTime = 0;
              pStream->SkipStill();

              return NAVRESULT_NOP;
            }
            else Sleep(200);
          }
          else Sleep(10);
          return NAVRESULT_HOLD;
        }
        else
        {
          // else notify the player we have recieved a still frame
          CLog::Log(LOGDEBUG, "recieved still frame, waiting %i sec", still_event->length);

          m_dvd.iDVDStillTime = still_event->length;
          m_dvd.iDVDStillStartTime = GetTickCount();
          m_dvd.bDisplayedStill = false;
          m_dvd.state = DVDSTATE_STILL;
        }
      }
      break;
    case DVDNAV_SPU_CLUT_CHANGE:
      {
        CLog::Log(LOGDEBUG, "DVDNAV_SPU_CLUT_CHANGE");
        for (int i = 0; i < 16; i++)
        {
          BYTE* color = m_dvdspus.m_clut[i];
          BYTE* t = (BYTE*) & (((unsigned __int32*)pData)[i]);

          color[0] = t[2]; // Y
          color[1] = t[1]; // Cr
          color[2] = t[0]; // Cb
        }

        m_dvdspus.m_bHasClut = true;
      }
      break;
    case DVDNAV_SPU_STREAM_CHANGE:
      {
        CLog::Log(LOGDEBUG, "DVDNAV_SPU_STREAM_CHANGE");

        dvdnav_spu_stream_change_event_t* event = (dvdnav_spu_stream_change_event_t*)pData;

        int iStream = event->physical_wide;
        if (!(iStream & 0x80) && !IsInMenu())
        {
          // subtitles are enabled in the dvd, do so in the osd if we are in the main movie
          g_stSettings.m_currentVideoSettings.m_SubtitleOn = true;
          m_bRenderSubtitle = true;
          m_dvdPlayerVideo.EnableSubtitle(true);
        }
        else
        {
          // not in main movie, or subtitles are disabled
          g_stSettings.m_currentVideoSettings.m_SubtitleOn = false;
          m_bRenderSubtitle = false;
          m_dvdPlayerVideo.EnableSubtitle(false);
        }

        if (iStream >= 0)
          m_dvd.iSelectedSPUStream = (iStream & ~0x80);
        else
          m_dvd.iSelectedSPUStream = -1;

        m_CurrentSubtitle.stream = NULL;
      }
      break;
    case DVDNAV_AUDIO_STREAM_CHANGE:
      {
        CLog::Log(LOGDEBUG, "DVDNAV_AUDIO_STREAM_CHANGE");

        // This should be the correct way i think, however we don't have any streams right now
        // since the demuxer hasn't started so it doesn't change. not sure how to do this.
        dvdnav_audio_stream_change_event_t* event = (dvdnav_audio_stream_change_event_t*)pData;

        // Tell system what audiostream should be opened by default
        if (event->logical >= 0)
          m_dvd.iSelectedAudioStream = event->physical;
        else
          m_dvd.iSelectedAudioStream = -1;

        m_CurrentAudio.stream = NULL;
      }
      break;
    case DVDNAV_HIGHLIGHT:
      {
        dvdnav_highlight_event_t* pInfo = (dvdnav_highlight_event_t*)pData;
        int iButton = pStream->GetCurrentButton();
        CLog::Log(LOGDEBUG, "DVDNAV_HIGHLIGHT: Highlight button %d\n", iButton);

        UpdateOverlayInfo(LIBDVDNAV_BUTTON_NORMAL);
      }
      break;
    case DVDNAV_VTS_CHANGE:
      {
        dvdnav_vts_change_event_t* vts_change_event = (dvdnav_vts_change_event_t*)pData;
        CLog::Log(LOGDEBUG, "DVDNAV_VTS_CHANGE");

        //Make sure we clear all the old overlays here, or else old forced items are left.
        m_overlayContainer.Clear();

        // reset the demuxer, this also imples closing the video and the audio system
        // this is a bit tricky cause it's the demuxer that's is making this call in the end
        // so we send a message to indicate the main loop that the demuxer needs a reset
        // this also means the libdvdnav may not return any data packets after this command
        m_messenger.Put(new CDVDMsgDemuxerReset());
        m_bReadAgain = true;

        //Force an aspect ratio that is set in the dvdheaders if available
        //techinally wrong place to do, should really be done when next video packet
        //is decoded.. but won't cause too many problems
        m_dvdPlayerVideo.SetAspectRatio(pStream->GetVideoAspectRatio());

        return NAVRESULT_ERROR;
      }
      break;
    case DVDNAV_CELL_CHANGE:
      {
        dvdnav_cell_change_event_t* cell_change_event = (dvdnav_cell_change_event_t*)pData;
        CLog::Log(LOGDEBUG, "DVDNAV_CELL_CHANGE");
        //if (cell_change_event->pgN != m_dvd.iCurrentCell)
        {
          m_dvd.iCurrentCell = cell_change_event->pgN;
          // m_iCommands |= DVDCOMMAND_SYNC;
        }

        m_dvd.state = DVDSTATE_NORMAL;

        m_bDontSkipNextFrame = true;
      }
      break;
    case DVDNAV_NAV_PACKET:
      {
          pci_t* pci = (pci_t*)pData;

          // this should be possible to use to make sure we get
          // seamless transitions over these boundaries
          // if we remember the old vobunits boundaries
          // when a packet comes out of demuxer that has
          // pts values outside that boundary, it belongs
          // to the new vobunit, wich has new timestamps

          // ptm is in a 90khz clock.
          unsigned __int64 pts = ((unsigned __int64)pci->pci_gi.vobu_s_ptm) * 1000 / 90;
          if( pts != m_dvd.iNAVPackFinish )
          {
            // we have a discontinuity in our stream.
            CLog::Log(LOGDEBUG, "DVDNAV_DISCONTINUITY(from: %I64d, to: %I64d)", m_dvd.iNAVPackFinish, pts);

            // don't allow next frame to be skipped
            m_bDontSkipNextFrame = true;

            // only set this when we have the first non continous packet
            // so that we know at what packets this new continuous stream starts
            m_dvd.iNAVPackStart = pts;
          }

          //m_dvd.iNAVPackStart = pts;
          m_dvd.iNAVPackFinish = ((unsigned __int64)pci->pci_gi.vobu_e_ptm) * 1000 / 90;
      }
      break;
    case DVDNAV_HOP_CHANNEL:
      {
        // This event is issued whenever a non-seamless operation has been executed.
        // Applications with fifos should drop the fifos content to speed up responsiveness.
        CLog::Log(LOGDEBUG, "DVDNAV_HOP_CHANNEL");

        FlushBuffers();
        // m_bReadAgain = true;
        return NAVRESULT_ERROR;
      }
      break;
    case DVDNAV_STOP:
      {
        CLog::Log(LOGDEBUG, "DVDNAV_STOP");
        m_dvd.state = DVDSTATE_NORMAL;
      }
      break;
    default:
    {}
      break;
    }
  }
  return NAVRESULT_NOP;
}

bool CDVDPlayer::OnAction(const CAction &action)
{
  if (m_pInputStream && m_pInputStream->IsStreamType(DVDSTREAM_TYPE_DVD))
  {
    CDVDInputStreamNavigator* pStream = (CDVDInputStreamNavigator*)m_pInputStream;

    switch (action.wID)
    {
    case ACTION_PREV_ITEM:  // SKIP-:
      {
        CLog::Log(LOGDEBUG, " - pushed prev");
        pStream->OnPrevious();
        return true;
      }
      break;
    case ACTION_NEXT_ITEM:  // SKIP+:
      {
        CLog::Log(LOGDEBUG, " - pushed next");
        pStream->OnNext();
        return true;
      }
      break;
    case ACTION_SHOW_VIDEOMENU:   // start button
      {
        CLog::Log(LOGDEBUG, " - go to menu");
        pStream->OnMenu();
        // send a message to everyone that we've gone to the menu
        CGUIMessage msg(GUI_MSG_VIDEO_MENU_STARTED, 0, 0);
        g_graphicsContext.SendMessage(msg);
        return true;
      }
      break;
    }

    if (pStream->IsInMenu())
    {
      switch (action.wID)
      {
      case ACTION_PREVIOUS_MENU:
        {
          CLog::Log(LOGDEBUG, " - menu back");
          pStream->OnBack();
        }
        break;
      case ACTION_MOVE_LEFT:
        {
          CLog::Log(LOGDEBUG, " - move left");
          pStream->OnLeft();
        }
        break;
      case ACTION_MOVE_RIGHT:
        {
          CLog::Log(LOGDEBUG, " - move right");
          pStream->OnRight();
        }
        break;
      case ACTION_MOVE_UP:
        {
          CLog::Log(LOGDEBUG, " - move up");
          pStream->OnUp();
        }
        break;
      case ACTION_MOVE_DOWN:
        {
          CLog::Log(LOGDEBUG, " - move down");
          pStream->OnDown();
        }
        break;
      case ACTION_SELECT_ITEM:
        {
          CLog::Log(LOGDEBUG, " - button select");

          // show button pushed overlay
          UpdateOverlayInfo(LIBDVDNAV_BUTTON_CLICKED);

          m_dvd.iSelectedSPUStream = -1;

          pStream->ActivateButton();
        }
        break;
      case REMOTE_0:
      case REMOTE_1:
      case REMOTE_2:
      case REMOTE_3:
      case REMOTE_4:
      case REMOTE_5:
      case REMOTE_6:
      case REMOTE_7:
      case REMOTE_8:
      case REMOTE_9:
        {
          // Offset from key codes back to button number
          int button = action.wID - REMOTE_0;
          CLog::Log(LOGDEBUG, " - button pressed %d", button);
          pStream->SelectButton(button);
        }
       break;
      default:
        return false;
        break;
      }
      return true; // message is handled
    }
  }

  // return false to inform the caller we didn't handle the message
  return false;
}

bool CDVDPlayer::IsInMenu() const
{
  if (m_pInputStream && m_pInputStream->IsStreamType(DVDSTREAM_TYPE_DVD))
  {
    CDVDInputStreamNavigator* pStream = (CDVDInputStreamNavigator*)m_pInputStream;
    return pStream->IsInMenu();
  }
  return false;
}

/*
 * iAction should be LIBDVDNAV_BUTTON_NORMAL or LIBDVDNAV_BUTTON_CLICKED
 */
void CDVDPlayer::UpdateOverlayInfo(int iAction)
{
  if (m_pInputStream->IsStreamType(DVDSTREAM_TYPE_DVD))
  {
    CDVDInputStreamNavigator* pStream = (CDVDInputStreamNavigator*)m_pInputStream;

    VecOverlays* pVecOverlays = m_overlayContainer.GetOverlays();

    //It's the last packet recieved that is of interest currently
    if (pVecOverlays->size() > 0)
    {
      CDVDOverlay* pOverlay = pVecOverlays->back();

      if (pOverlay->IsOverlayType(DVDOVERLAY_TYPE_SPU))
      {
        CDVDOverlaySpu* pOverlaySpu = (CDVDOverlaySpu*)pOverlay;

        // make sure its a forced (menu) overlay
        if (pOverlaySpu->bForced)
        {
          // set menu spu color and alpha data if there is a valid menu overlay
          pStream->GetCurrentButtonInfo(pOverlaySpu, &m_dvdspus, iAction);
        }
      }
      else
      {
        CLog::Log(LOGERROR, "CDVDPlayer::UpdateOverlayInfo : last overlay container is not of a dvd SPU type, unable to update overlay info");
      }
    }
  }
}

bool CDVDPlayer::HasMenu()
{
  if (m_pInputStream && m_pInputStream->IsStreamType(DVDSTREAM_TYPE_DVD))
    return true;
  else
    return false;
}

//IChapterProvider* CDVDPlayer::GetChapterProvider()
//{
//  if (m_pInputStream && m_pInputStream->m_streamType == DVDSTREAM_TYPE_DVD)
//  {
//    m_chapterReader.SetFile(m_filename);
//    m_chapterReader.SetCurrentTitle(1);
//    return (&m_chapterReader);
//  }
//  return NULL;
//}

bool CDVDPlayer::GetCurrentSubtitle(CStdStringW& strSubtitle)
{
  __int64 pts = m_dvdPlayerVideo.GetCurrentPts();
  bool bGotSubtitle = m_dvdPlayerSubtitle.GetCurrentSubtitle(strSubtitle, pts);
  return bGotSubtitle;
}


CStdString CDVDPlayer::GetPlayerState()
{
  if (!m_pInputStream) return "";

  if (m_pInputStream->IsStreamType(DVDSTREAM_TYPE_DVD))
  {
    CDVDInputStreamNavigator* pStream = (CDVDInputStreamNavigator*)m_pInputStream;

    std::string buffer;
    if( pStream->GetNavigatorState(buffer) ) return buffer;
  }

  return "";
}

bool CDVDPlayer::SetPlayerState(CStdString state)
{
  m_messenger.Put(new CDVDMsgPlayerSetState(state));
  return true;
}
