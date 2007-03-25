
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
#include "..\..\utils\GUIInfoManager.h"
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

  m_bDontSkipNextFrame = false;
  
#ifdef DVDDEBUG_MESSAGE_TRACKER
  g_dvdMessageTracker.Init();
#endif
}

CDVDPlayer::~CDVDPlayer()
{
  CloseFile();

  CloseHandle(m_hReadyEvent);
  DeleteCriticalSection(&m_critStreamSection);
#ifdef DVDDEBUG_MESSAGE_TRACKER
  g_dvdMessageTracker.DeInit();
#endif
}

bool CDVDPlayer::OpenFile(const CFileItem& file, const CPlayerOptions &options)
{
  try
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

    // settings that should be set before opening the file
    SetAVDelay(g_stSettings.m_currentVideoSettings.m_AudioDelay);
    
    if (strFile.Find("dvd://") >= 0 ||
        strFile.CompareNoCase("d:\\video_ts\\video_ts.ifo") == 0 ||
        strFile.CompareNoCase("iso9660://video_ts/video_ts.ifo") == 0)
    {
      m_filename = "\\Device\\Cdrom0";
    }
    else 
      m_filename = strFile;

    m_content = file.GetContentType();

    /* otherwise player will think we need to be restarted */
    g_stSettings.m_currentVideoSettings.m_SubtitleCached = true;

    /* allow renderer to switch to fullscreen if requested */
    m_dvdPlayerVideo.EnableFullscreen(options.fullscreen);

    ResetEvent(m_hReadyEvent);
    Create();
    WaitForSingleObject(m_hReadyEvent, INFINITE);

    // Playback might have been stopped due to some error
    if (m_bStop) return false;

    /* check if we got a full dvd state, then use that */
    if( options.state.size() > 0 && m_pInputStream->IsStreamType(DVDSTREAM_TYPE_DVD))
      SetPlayerState(options.state);
    else if( options.starttime > 0 )
      SeekTime( (__int64)(options.starttime * 1000) );
    
    // settings that can only be set after the inputstream, demuxer and or codecs are opened
    SetSubtitleVisible(g_stSettings.m_currentVideoSettings.m_SubtitleOn);
   
    return true;
  }
  catch(...)
  {
    CLog::Log(LOGERROR, __FUNCTION__" - Exception thrown on open");
    return false;
  }
}

bool CDVDPlayer::CloseFile()
{
  CLog::Log(LOGNOTICE, "CDVDPlayer::CloseFile()");

  // set the abort request so that other threads can finish up
  m_bAbortRequest = true;

  // unpause the player
  SetPlaySpeed(DVD_PLAYSPEED_NORMAL);

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
  m_packetcount = 0;

  m_messenger.Init();

  g_dvdPerformanceCounter.EnableMainPerformance(ThreadHandle());
}

void CDVDPlayer::Process()
{
  int video_index = -1;
  int audio_index = -1;

  CLog::Log(LOGNOTICE, "Creating InputStream");
  
  m_pInputStream = CDVDFactoryInputStream::CreateInputStream(this, m_filename, m_content);
  if (!m_pInputStream || !m_pInputStream->Open(m_filename.c_str(), m_content))
  {
    CLog::Log(LOGERROR, "InputStream: Error opening, %s", m_filename.c_str());
    // inputstream will be destroyed in OnExit()
    return;
  }

  if (m_pInputStream->IsStreamType(DVDSTREAM_TYPE_DVD))
  {
    CLog::Log(LOGNOTICE, "DVDPlayer: playing a dvd with menu's");
  }

  CLog::Log(LOGNOTICE, "Creating Demuxer");
  
  try
  {
    m_pDemuxer = CDVDFactoryDemuxer::CreateDemuxer(m_pInputStream);
    if(!m_pDemuxer)
    {
      CLog::Log(LOGERROR, __FUNCTION__" - Error creating demuxer");
      return;
    }

    if (!m_pDemuxer->Open(m_pInputStream))
    {
      CLog::Log(LOGERROR, __FUNCTION__" - Error opening demuxer");
      return;
    }
  }
  catch(...)
  {
    CLog::Log(LOGERROR, __FUNCTION__" - Exception thrown when opeing demuxer");
    return;
  }

  // find first audio / video streams
  for (int i = 0; i < m_pDemuxer->GetNrOfStreams(); i++)
  {
    CDemuxStream* pStream = m_pDemuxer->GetStream(i);
    if (pStream)
    {
      if (pStream->type == STREAM_AUDIO && audio_index < 0) audio_index = i;
      else if (pStream->type == STREAM_VIDEO && video_index < 0) video_index = i;
    }
  }

  //m_dvdPlayerSubtitle.Init();
  //m_dvdPlayerSubtitle.FindSubtitles(m_filename);

  // open the streams
  if (audio_index >= 0) OpenAudioStream(audio_index);
  if (video_index >= 0) OpenVideoStream(video_index);

  // it's demuxers job to fail if it can't find any streams in file
  // if streams can't be found by probing the file, is stram we should still
  // try to play abit since streams may be found later
#if 0 
  // if we use libdvdnav, streaminfo is not avaiable, we will find this later on
  if (m_CurrentVideo.id < 0 && m_CurrentAudio.id < 0 &&
      m_pInputStream->IsStreamType(DVDSTREAM_TYPE_FILE) &&
      !m_pInputStream->HasExtension("vob"))
  {
    CLog::Log(LOGERROR, "%s: could not open codecs\n", m_filename.c_str());
    return;
  }
#endif

  // we are done initializing now, set the readyevent
  SetEvent(m_hReadyEvent);

  m_callback.OnPlayBackStarted();

  int iErrorCounter = 0;

  while (!m_bAbortRequest)
  {
    // if the queues are full, no need to read more
    while (!m_bAbortRequest && (!m_dvdPlayerAudio.AcceptsData() || !m_dvdPlayerVideo.AcceptsData()))
    {
      HandleMessages();
      Sleep(10);
    }

    if (!m_bAbortRequest)
    {

      if(GetPlaySpeed() != DVD_PLAYSPEED_NORMAL && GetPlaySpeed() != DVD_PLAYSPEED_PAUSE)
      {
        bool bMenu = IsInMenu();

        // don't allow rewind in menu
        if (bMenu && GetPlaySpeed() < 0 ) SetPlaySpeed(DVD_PLAYSPEED_NORMAL);

        if (m_CurrentVideo.id >= 0 &&
            m_dvdPlayerVideo.GetCurrentPts() != DVD_NOPTS_VALUE &&
            !bMenu &&
            !m_bDontSkipNextFrame)
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
        CDVDDemuxUtils::FreeDemuxPacket(pPacket);
        pPacket = NULL;
        continue;
      }

      if (!pPacket)
      {
        if (!m_pInputStream) break;
        if (m_pInputStream->IsEOF()) break;

        if (m_pInputStream->IsStreamType(DVDSTREAM_TYPE_DVD))
        {
          CDVDInputStreamNavigator* pStream = static_cast<CDVDInputStreamNavigator*>(m_pInputStream);

          // stream is holding back data untill demuxer has flushed
          if(pStream->IsHeld())
          {
            pStream->SkipHold();
            continue;
          }

          // stills will be skipped
          if(m_dvd.state == DVDSTATE_STILL)
          {
            if (m_dvd.iDVDStillTime < 0xff)
            {
              if (GetTickCount() >= (m_dvd.iDVDStillStartTime + m_dvd.iDVDStillTime * 1000 ))
              {
                m_dvd.iDVDStillTime = 0;
                m_dvd.iDVDStillStartTime = 0;
                pStream->SkipStill();
                continue;
              }
            }
            Sleep(100);
            continue;
          }
        }

        // keep on trying until user wants us to stop.
        iErrorCounter++;
        CLog::Log(LOGERROR, "Error reading data from demuxer");

        // maybe reseting the demuxer at this point would be a good idea, should it have failed in some way.
        // probably not a good idea, the dvdplayer can get stuck (and it does sometimes) in a loop this way.
        if (iErrorCounter > 50)
        {
          CLog::Log(LOGERROR, "got 50 read errors in a row, quiting");
          return;
          //m_pDemuxer->Reset();
          //iErrorCounter = 0;
        }

        continue;
      }

      iErrorCounter = 0;
      m_packetcount++;

      if(m_pInputStream && m_pInputStream->IsStreamType(DVDSTREAM_TYPE_DVD))
      {
        CDVDInputStreamNavigator *pInput = static_cast<CDVDInputStreamNavigator*>(m_pInputStream);

        if (pPacket->dts != DVD_NOPTS_VALUE)
          pPacket->dts -= pInput->GetTimeStampCorrection();
        if (pPacket->pts != DVD_NOPTS_VALUE)
          pPacket->pts -= pInput->GetTimeStampCorrection();
      }

      CDemuxStream *pStream = m_pDemuxer->GetStream(pPacket->iStreamId);

      if (!pStream) 
      {
        CLog::Log(LOGERROR, __FUNCTION__" - Error demux packet doesn't belong to any stream");
        continue;
      }

      // it's a valid data packet, add some more information too it
      
      // this groupId stuff is getting a bit messy, need to find a better way
      // currently it is used to determine if a menu overlay is associated with a picture
      // for dvd's we use as a group id, the current cell and the current title
      // to be a bit more precise we alse count the number of disc's in case of a pts wrap back in the same cell / title
      pPacket->iGroupId = m_pInputStream->GetCurrentGroupId();
      try
      {
        if (m_pInputStream && m_pInputStream->IsStreamType(DVDSTREAM_TYPE_DVD))
        {
          // Stream selection for DVD's this
          // should probably come as messages in the packet instead

          if (pStream->type == STREAM_SUBTITLE &&
              pStream->iPhysicalId == m_dvd.iSelectedSPUStream &&
              pStream->iId != m_CurrentSubtitle.id)
          {
            // dvd subtitle stream changed
            OpenSubtitleStream( pStream->iId );
          }
          else if (pStream->type == STREAM_AUDIO &&
              pStream->iPhysicalId == m_dvd.iSelectedAudioStream &&
              pStream->iId != m_CurrentAudio.id)
          {
            // dvd audio stream changed,          
            OpenAudioStream( pStream->iId );
          }
          else if (pStream->type == STREAM_VIDEO &&
              pStream->iPhysicalId == 0 &&
              pStream->iId != m_CurrentVideo.id)
          {
            // dvd video stream changed
            OpenVideoStream(pStream->iId);
          }

          // check so dvdnavigator didn't want us to close stream,
          // we allow lingering invalid audio/subtitle streams here to let player pass vts/cell borders more cleanly
          if (m_dvd.iSelectedAudioStream < 0 && m_CurrentAudio.id >= 0) CloseAudioStream( true );
          if (m_dvd.iSelectedSPUStream < 0 && m_CurrentVideo.id >= 0)   CloseSubtitleStream( true );
        }
        else
        {
          // for normal files, just open first stream
          if (m_CurrentSubtitle.id < 0 && pStream->type == STREAM_SUBTITLE) OpenSubtitleStream(pStream->iId);
          if (m_CurrentAudio.id < 0    && pStream->type == STREAM_AUDIO)    OpenAudioStream(pStream->iId);
          if (m_CurrentVideo.id < 0    && pStream->type == STREAM_VIDEO)    OpenVideoStream(pStream->iId);

          // check so that none of our streams has become invalid
          if (m_CurrentAudio.id >= 0    && m_pDemuxer->GetStream(m_CurrentAudio.id) == NULL)     CloseAudioStream(false);
          if (m_CurrentSubtitle.id >= 0 && m_pDemuxer->GetStream(m_CurrentSubtitle.id) == NULL)  CloseSubtitleStream(false);
          if (m_CurrentVideo.id >= 0    && m_pDemuxer->GetStream(m_CurrentVideo.id) == NULL)     CloseVideoStream(false);
        }
      }
      catch (...)
      {
        CLog::Log(LOGERROR, __FUNCTION__" - Exception thrown when attempting to open stream");
        break;
      }

      /* process packet if it belongs to selected stream. for dvd's down't allow automatic opening of streams*/
      LockStreams();

      try
      {
        if (pPacket->iStreamId == m_CurrentAudio.id && pStream->type == STREAM_AUDIO)
        {
          ProcessAudioData(pStream, pPacket);
        }
        else if (pPacket->iStreamId == m_CurrentVideo.id && pStream->type == STREAM_VIDEO)
        {
          ProcessVideoData(pStream, pPacket);
        }
        else if (pPacket->iStreamId == m_CurrentSubtitle.id && pStream->type == STREAM_SUBTITLE)
        {
          ProcessSubData(pStream, pPacket);
        }
        else CDVDDemuxUtils::FreeDemuxPacket(pPacket); // free it since we won't do anything with it
      }
      catch(...)
      {
        CLog::Log(LOGERROR, __FUNCTION__" - Exception thrown when processing demux packet");
        break;
      }

      UnlockStreams();
    }
  }
}

void CDVDPlayer::ProcessAudioData(CDemuxStream* pStream, CDVDDemux::DemuxPacket* pPacket)
{
  if (m_CurrentAudio.stream != (void*)pStream)
  {
    /* check so that dmuxer hints or extra data hasn't changed */
    /* if they have, reopen stream */

    if (m_CurrentAudio.hint != CDVDStreamInfo(*pStream, true))
    { 
      // we don't actually have to close audiostream here first, as 
      // we could send it as a stream message. only problem 
      // is how to notify player if a stream change failed.
      CloseAudioStream( true );
      OpenAudioStream( pPacket->iStreamId );
    }

    m_CurrentAudio.stream = (void*)pStream;
  }
  
  CheckContinuity(pPacket, DVDPLAYER_AUDIO);
  if(pPacket->dts != DVD_NOPTS_VALUE)
    m_CurrentAudio.dts = pPacket->dts;

  //If this is the first packet after a discontinuity, send it as a resync
  if (!(m_dvd.iFlagSentStart & DVDPLAYER_AUDIO))
  {
    m_dvd.iFlagSentStart |= DVDPLAYER_AUDIO;
    m_dvdPlayerAudio.SendMessage(new CDVDMsgGeneralSetClock(pPacket->pts, pPacket->dts));
  }

  if (m_CurrentAudio.id >= 0)
    m_dvdPlayerAudio.SendMessage(new CDVDMsgDemuxerPacket(pPacket, pPacket->iSize));
  else
    CDVDDemuxUtils::FreeDemuxPacket(pPacket);
}

void CDVDPlayer::ProcessVideoData(CDemuxStream* pStream, CDVDDemux::DemuxPacket* pPacket)
{
  if (m_CurrentVideo.stream != (void*)pStream)
  {
    /* check so that dmuxer hints or extra data hasn't changed */
    /* if they have reopen stream */

    if (m_CurrentVideo.hint != CDVDStreamInfo(*pStream, true))
    {
      CloseVideoStream(true);
      OpenVideoStream(pPacket->iStreamId);
    }

    m_CurrentVideo.stream = (void*)pStream;
  }


  if (m_bDontSkipNextFrame)
  {
    m_dvdPlayerVideo.SendMessage(new CDVDMsgVideoNoSkip());
    m_bDontSkipNextFrame = false;
  }
  
  if( pPacket->iSize != 4) //don't check the EOF_SEQUENCE of stillframes
  {
    CheckContinuity( pPacket, DVDPLAYER_VIDEO );
    if(pPacket->dts != DVD_NOPTS_VALUE)
      m_CurrentVideo.dts = pPacket->dts;
  }

  //If this is the first packet after a discontinuity, send it as a resync
  if (!(m_dvd.iFlagSentStart & DVDPLAYER_VIDEO))
  {
    m_dvd.iFlagSentStart |= DVDPLAYER_VIDEO;
    
    if (m_CurrentAudio.id < 0 || m_playSpeed != DVD_PLAYSPEED_NORMAL )
      m_dvdPlayerVideo.SendMessage(new CDVDMsgGeneralSetClock(pPacket->pts, pPacket->dts));
    else
      m_dvdPlayerVideo.SendMessage(new CDVDMsgGeneralResync(pPacket->pts, pPacket->dts));
  }

  if (m_CurrentVideo.id >= 0)
    m_dvdPlayerVideo.SendMessage(new CDVDMsgDemuxerPacket(pPacket, pPacket->iSize));
  else
    CDVDDemuxUtils::FreeDemuxPacket(pPacket);
}

void CDVDPlayer::ProcessSubData(CDemuxStream* pStream, CDVDDemux::DemuxPacket* pPacket)
{
  if (pStream->codec == 0x17000) //CODEC_ID_DVD_SUBTITLE)
  {
    CSPUInfo* pSPUInfo = m_dvdspus.AddData(pPacket->pData, pPacket->iSize, pPacket->pts);

    if (pSPUInfo)
    {
      CLog::Log(LOGDEBUG, "CDVDPlayer::ProcessSubData: Got complete SPU packet");
      pSPUInfo->iGroupId = pPacket->iGroupId;
      m_overlayContainer.Add(pSPUInfo);
      pSPUInfo->Release();
      
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

void CDVDPlayer::CheckContinuity(CDVDDemux::DemuxPacket* pPacket, unsigned int source)
{
  if( pPacket->dts == DVD_NOPTS_VALUE )
    return;


  unsigned __int64 mindts, maxdts;
  if(m_CurrentAudio.dts == DVD_NOPTS_VALUE)
    maxdts = mindts = m_CurrentVideo.dts;
  else if(m_CurrentVideo.dts == DVD_NOPTS_VALUE)
    maxdts = mindts = m_CurrentAudio.dts;
  else
  {
    maxdts = max(m_CurrentAudio.dts, m_CurrentVideo.dts);
    mindts = min(m_CurrentAudio.dts, m_CurrentVideo.dts);
  }

  if (source == DVDPLAYER_VIDEO)
  {
    /* check for looping stillframes on non dvd's, dvd's will be detected by long duration check later */
    if( !(m_pInputStream && m_pInputStream->IsStreamType(DVDSTREAM_TYPE_DVD)) )
    {
      /* special case for looping stillframes THX test discs*/
      /* only affect playback when not from dvd */
      if( (m_CurrentAudio.dts > m_CurrentVideo.dts + DVD_MSEC_TO_TIME(200)) 
      && (m_CurrentVideo.dts == pPacket->dts) )
      {
        CLog::Log(LOGDEBUG, "CDVDPlayer::CheckContinuity - Detected looping stillframe");
        SyncronizePlayers(SYNCSOURCE_VIDEO);
        return;
      }
    }

    /* if we haven't received video for a while, but we have been */
    /* getting audio much more recently, make sure video wait's  */
    /* this occurs especially on thx test disc */
    if( (pPacket->dts > m_CurrentVideo.dts + DVD_MSEC_TO_TIME(200)) 
     && (pPacket->dts < m_CurrentAudio.dts + DVD_MSEC_TO_TIME(50)) )
    {
      CLog::Log(LOGDEBUG, "CDVDPlayer::CheckContinuity - Potential long duration frame");
      SyncronizePlayers(SYNCSOURCE_VIDEO);
      return;
    }

  }

  /* if we don't have max and min, we can't do anything more */
  if( mindts == DVD_NOPTS_VALUE || maxdts == DVD_NOPTS_VALUE )
    return;


  /* stream wrap back */
  if( pPacket->dts < mindts )
  {
    /* if video player is rendering a stillframe, we need to make sure */
    /* audio has finished processing it's data otherwise it will be */
    /* displayed too early */
 
    if (m_dvdPlayerVideo.IsStalled() && m_CurrentVideo.dts != DVD_NOPTS_VALUE)
      SyncronizePlayers(SYNCSOURCE_VIDEO);

    if (m_dvdPlayerAudio.IsStalled() && m_CurrentAudio.dts != DVD_NOPTS_VALUE)
      SyncronizePlayers(SYNCSOURCE_AUDIO);

    m_dvd.iFlagSentStart  = 0;
  }

  /* stream jump forward */
  if( pPacket->dts > maxdts + DVD_MSEC_TO_TIME(1000) )
  {
    /* normally don't need to sync players since video player will keep playing at normal fps */
    /* after a discontinuity */
    //SyncronizePlayers(dts, pts, MSGWAIT_ALL);
    m_dvd.iFlagSentStart  = 0;
  }

}
void CDVDPlayer::SyncronizePlayers(DWORD sources)
{

  /* we need a big timeout as audio queue is about 8seconds for 2ch ac3 */
  const int timeout = 10*1000; // in milliseconds

  CDVDMsgGeneralSynchronize* message = new CDVDMsgGeneralSynchronize(timeout, sources);
  if (m_CurrentAudio.id >= 0)
  {
    message->Acquire();
    m_dvdPlayerAudio.SendMessage(message);
  }
  if (m_CurrentVideo.id >= 0)
  {
    message->Acquire();
    m_dvdPlayerVideo.SendMessage(message);
  }
  message->Release();
}

void CDVDPlayer::OnExit()
{
  g_dvdPerformanceCounter.DisableMainPerformance();

  try
  {
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

  }
  catch (...)
  {
    CLog::Log(LOGERROR, __FUNCTION__" - Exception thrown when trying to close down player, memory leak will follow");
    m_pInputStream = NULL;
    m_pDemuxer = NULL;   
  }

  // set event to inform openfile something went wrong in case openfile is still waiting for this event
  SetEvent(m_hReadyEvent);
}

void CDVDPlayer::HandleMessages()
{
  CDVDMsg* pMsg;

  MsgQueueReturnCode ret = m_messenger.Get(&pMsg, 0);
   
  while (ret == MSGQ_OK)
  {
    LockStreams();

    try
    {
      if (pMsg->IsType(CDVDMsg::PLAYER_SEEK))
      {
        CDVDMsgPlayerSeek* pMsgPlayerSeek = (CDVDMsgPlayerSeek*)pMsg;
        
        if (m_pInputStream && m_pInputStream->IsStreamType(DVDSTREAM_TYPE_DVD))
        {
          // need to get the seek based on file positition working in CDVDInputStreamNavigator
          // so that demuxers can control the stream (seeking in this case)
          // for now use time based seeking
          CLog::Log(LOGDEBUG, "CDVDInputStreamNavigator seek to: %d", pMsgPlayerSeek->GetTime());
          if (((CDVDInputStreamNavigator*)m_pInputStream)->Seek(pMsgPlayerSeek->GetTime()))
          {
            CLog::Log(LOGDEBUG, "CDVDInputStreamNavigator seek to: %d, succes", pMsgPlayerSeek->GetTime());
            FlushBuffers();
          }
          else 
            CLog::Log(LOGWARNING, "error while seeking");
        }
        else
        {
          CLog::Log(LOGDEBUG, "demuxer seek to: %d", pMsgPlayerSeek->GetTime());
          if (m_pDemuxer && m_pDemuxer->Seek(pMsgPlayerSeek->GetTime()))
          {
            CLog::Log(LOGDEBUG, "demuxer seek to: %d, succes", pMsgPlayerSeek->GetTime());
            FlushBuffers();
          }
          else CLog::Log(LOGWARNING, "error while seeking");
        }
        // make sure video player displays next frame
        m_dvdPlayerVideo.StepFrame();

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

        // for dvd's we set it using the navigater, it will then send a change audio to the dvdplayer
        if (m_pInputStream && m_pInputStream->IsStreamType(DVDSTREAM_TYPE_DVD))
        {          
          CDVDInputStreamNavigator* pInput = (CDVDInputStreamNavigator*)m_pInputStream;
          if( pInput->SetActiveAudioStream(pMsgPlayerSetAudioStream->GetStreamId()) )
          {
            m_dvd.iSelectedAudioStream = -1;
            CloseAudioStream(false);            
          }
        }
        else if (m_pDemuxer)
        {
          CDemuxStream* pStream = m_pDemuxer->GetStreamFromAudioId(pMsgPlayerSetAudioStream->GetStreamId());
          if (pStream) OpenAudioStream(pStream->iId);
        }
      }
      else if (pMsg->IsType(CDVDMsg::PLAYER_SET_SUBTITLESTREAM))
      {
        CDVDMsgPlayerSetSubtitleStream* pMsgPlayerSetSubtileStream = (CDVDMsgPlayerSetSubtitleStream*)pMsg;

        // for dvd's we set it using the navigater, it will then send a change subtitle to the dvdplayer
        if (m_pInputStream && m_pInputStream->IsStreamType(DVDSTREAM_TYPE_DVD))
        {
          CDVDInputStreamNavigator* pStream = (CDVDInputStreamNavigator*)m_pInputStream;
          if( pStream->SetActiveSubtitleStream(pMsgPlayerSetSubtileStream->GetStreamId()) )
          {
            m_dvd.iSelectedSPUStream = -1;
            CloseSubtitleStream(false);            
          }
        }
        else if (m_pDemuxer)
        {
          CDemuxStream* pStream = m_pDemuxer->GetStreamFromSubtitleId(pMsgPlayerSetSubtileStream->GetStreamId());
          if (pStream) OpenSubtitleStream(pStream->iId);
        }        
      }
      else if (pMsg->IsType(CDVDMsg::PLAYER_SET_SUBTITLESTREAM_VISIBLE))
      {
        CDVDMsgBool* pValue = (CDVDMsgBool*)pMsg;

        m_dvdPlayerVideo.EnableSubtitle(pValue->m_value);

        if (m_pInputStream && m_pInputStream->IsStreamType(DVDSTREAM_TYPE_DVD))
        {
          CDVDInputStreamNavigator* pStream = (CDVDInputStreamNavigator*)m_pInputStream;
          pStream->EnableSubtitleStream(pValue->m_value);
        }
      }
      else if (pMsg->IsType(CDVDMsg::PLAYER_SET_STATE))
      {
        CDVDMsgPlayerSetState* pMsgPlayerSetState = (CDVDMsgPlayerSetState*)pMsg;

        if (m_pInputStream && m_pInputStream->IsStreamType(DVDSTREAM_TYPE_DVD))
        {
          ((CDVDInputStreamNavigator*)m_pInputStream)->SetNavigatorState(pMsgPlayerSetState->GetState());
        }
      }
      else if (pMsg->IsType(CDVDMsg::GENERAL_FLUSH))
      {
        FlushBuffers();
      }
    }
    catch (...)
    {
      CLog::Log(LOGERROR, __FUNCTION__" - Exception thrown when handling message");
    }
    
    UnlockStreams();
    
    pMsg->Release();
    ret = m_messenger.Get(&pMsg, 0);
  }
}

void CDVDPlayer::SetPlaySpeed(int speed)
{
  m_playSpeed = speed;  

  // if playspeed is different then DVD_PLAYSPEED_NORMAL or DVD_PLAYSPEED_PAUSE
  // audioplayer, stops outputing audio to audiorendere, but still tries to
  // sleep an correct amount for each packet
  // videoplayer just plays faster after the clock speed has been increased
  // 1. disable audio
  // 2. skip frames and adjust their pts or the clock
  m_clock.SetSpeed(speed); 
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
{
}

void CDVDPlayer::ToggleSubtitles()
{
  SetSubtitleVisible(!GetSubtitleVisible());
}

bool CDVDPlayer::CanSeek()
{
  return GetTotalTime() > 0;
}

void CDVDPlayer::Seek(bool bPlus, bool bLargeStep)
{
#if 0
  // sadly this doesn't work for now, audio player must
  // drop packets at the same rate as we play frames
  if( m_playSpeed == DVD_PLAYSPEED_PAUSE && bPlus && !bLargeStep)
  {
    m_dvdPlayerVideo.StepFrame();
    return;
  }
#endif

  if (g_advancedSettings.m_videoUseTimeSeeking && GetTotalTime() > 2*g_advancedSettings.m_videoTimeSeekForwardBig)
  {
    int seek = 0;
    if (bLargeStep)
      seek = bPlus ? g_advancedSettings.m_videoTimeSeekForwardBig : g_advancedSettings.m_videoTimeSeekBackwardBig;
    else
      seek = bPlus ? g_advancedSettings.m_videoTimeSeekForward : g_advancedSettings.m_videoTimeSeekBackward;
    // do the seek
    SeekTime(GetTime() + seek * 1000);
  }
  else
  {
    float percent = GetPercentage();
    if (bLargeStep)
      percent += bPlus ? g_advancedSettings.m_videoPercentSeekForwardBig : g_advancedSettings.m_videoPercentSeekBackwardBig;
    else
      percent += bPlus ? g_advancedSettings.m_videoPercentSeekForward : g_advancedSettings.m_videoPercentSeekBackward;

    if (percent >= 0 && percent <= 100)
    {
      // should be modified to seektime
      SeekPercentage(percent);
    }
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
  strPlayerInfo.Format("vq size: %i, cpu: %i%%", bsize, (int)(m_dvdPlayerVideo.GetRelativeUsage()*100));
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
  else if (m_pDemuxer)
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

  if (m_pInputStream && m_pInputStream->IsStreamType(DVDSTREAM_TYPE_DVD))
  {
    // get stream name from dvd, don't check for demuxer stream
    // since the stream may not be avaiable yet (it is a few secs later)
    CDVDInputStreamNavigator* pStreamNav = (CDVDInputStreamNavigator*)m_pInputStream;
    strStreamName += pStreamNav->GetSubtitleStreamLanguage(iStream);
  }
  else if (m_pDemuxer)
  {
    CDemuxStream* pStream = m_pDemuxer->GetStreamFromSubtitleId(iStream);

    if (!pStream)
    {
      CLog::Log(LOGWARNING, "CDVDPlayer::GetSubtitleName: Invalid stream: id %i", iStream);
      strStreamName += " (Invalid)";
      return;
    }

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
  if (m_pInputStream && m_pInputStream->IsStreamType(DVDSTREAM_TYPE_DVD))
  {
    // for dvd's when we are in the menu, just return users preference
    CDVDInputStreamNavigator* pStream = (CDVDInputStreamNavigator*)m_pInputStream;
    if(pStream->IsInMenu())
      return g_stSettings.m_currentVideoSettings.m_SubtitleOn;
  }

  return m_dvdPlayerVideo.IsSubtitleEnabled();
}

void CDVDPlayer::SetSubtitleVisible(bool bVisible)
{
  g_stSettings.m_currentVideoSettings.m_SubtitleOn = bVisible;
  m_messenger.Put(new CDVDMsgBool(CDVDMsg::PLAYER_SET_SUBTITLESTREAM_VISIBLE, bVisible));
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
  else if (m_pDemuxer)
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
  CDemuxStreamAudio* pStream = NULL;
  if (m_pDemuxer) pStream = m_pDemuxer->GetStreamFromAudioId(iStream);

  if (m_pInputStream && m_pInputStream->IsStreamType(DVDSTREAM_TYPE_DVD))
  {
    // get stream name from dvd, don't check for demuxer stream
    // since the stream may not be avaiable yet (it is a few secs later)
    CDVDInputStreamNavigator* pStreamNav = (CDVDInputStreamNavigator*)m_pInputStream;
    strStreamName += pStreamNav->GetAudioStreamLanguage(iStream);
  }
  else
  {
    if (!pStream)
    {
      CLog::Log(LOGWARNING, "CDVDPlayer::GetAudioStreamName: Invalid stream: id %i", iStream);
      strStreamName += " (Invalid)";
      return;
    }
  
    CStdString strName;
    pStream->GetStreamName(strName);
    strStreamName += strName;
  }

  if (pStream)
  {
    std::string strType;
    pStream->GetStreamType(strType);
    if (strType.length() > 0)
    {
      strStreamName += " - ";
      strStreamName += strType;
    }
  }
}

void CDVDPlayer::SetAudioStream(int iStream)
{
  m_messenger.Put(new CDVDMsgPlayerSetAudioStream(iStream));
}

void CDVDPlayer::SeekTime(__int64 iTime)
{
  if(iTime<0) 
    iTime = 0;
  m_messenger.Put(new CDVDMsgPlayerSeek((int)iTime));
}

// return the time in milliseconds
__int64 CDVDPlayer::GetTime()
{
  // get timing and seeking from libdvdnav for dvd's
  if (m_pInputStream && m_pInputStream->IsStreamType(DVDSTREAM_TYPE_DVD))
  {
    return ((CDVDInputStreamNavigator*)m_pInputStream)->GetTime(); // we should take our buffers into account
  }

  __int64 iMsecs = (m_clock.GetClock() / (DVD_TIME_BASE / 1000));
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

bool CDVDPlayer::GetSubtitleExtension(CStdString &strSubtitleExtension)
{
  return false;
}

bool CDVDPlayer::OpenAudioStream(int iStream)
{
  if (!m_pDemuxer)
  {
    CLog::Log(LOGERROR, __FUNCTION__": No Demuxer");
    return false;
  }
  
  CDemuxStream* pStream = m_pDemuxer->GetStream(iStream);
  if (!pStream)
  {
    CLog::Log(LOGERROR, __FUNCTION__": Stream %d doesn't exist", iStream);
    return false;
  }

  if (pStream->disabled)
    return false;

  if (pStream->type != STREAM_AUDIO)
  {
    CLog::Log(LOGERROR, __FUNCTION__": Streamtype is not STREAM_AUDIO");
    return false;
  }

  if( m_CurrentAudio.id >= 0 ) CloseAudioStream(true);

  CLog::Log(LOGNOTICE, "Opening audio stream: %i", iStream);

  if( m_CurrentAudio.id < 0 &&  m_CurrentVideo.id >= 0 )
  {
    // up until now we wheren't playing audio, but we did play video
    // this will change what is used to sync the dvdclock.
    // since the new audio data doesn't have to have any relation
    // to the current video data in the packet que, we have to
    // wait for it to empty

    // this happens if a new cell has audio data, but previous didn't
    // and both have video data

    SyncronizePlayers(SYNCSOURCE_AUDIO);
  }

  CDVDStreamInfo hint(*pStream, true);
//  if( m_CurrentAudio.id >= 0 )
//  {
//    CDVDStreamInfo* phint = new CDVDStreamInfo(hint);
//    m_dvdPlayerAudio.SendMessage(new CDVDMsgGeneralStreamChange(phint));
//  }
  bool success = false;
  try
  {
    success = m_dvdPlayerAudio.OpenStream( hint );
  }
  catch (...)
  {
    CLog::Log(LOGERROR, __FUNCTION__" - Exception thrown in player");
    success = false;
  }

  if (!success)
  {
    /* mark stream as disabled, to disallaw further attempts*/
    CLog::Log(LOGWARNING, __FUNCTION__" - Unsupported stream %d. Stream disabled.", iStream);
    pStream->disabled = true;

    return false;
  }

  /* store information about stream */
  m_CurrentAudio.id = iStream;
  m_CurrentAudio.hint = hint;
  m_CurrentAudio.stream = (void*)pStream;

  /* audio normally won't consume full cpu, so let it have prio */
  m_dvdPlayerAudio.SetPriority(GetThreadPriority(*this)+1);

  /* set aspect ratio as requested by navigator for dvd's */
  if( m_pInputStream && m_pInputStream->IsStreamType(DVDSTREAM_TYPE_DVD) )
    m_dvdPlayerVideo.m_messageQueue.Put(new CDVDMsgVideoSetAspect(static_cast<CDVDInputStreamNavigator*>(m_pInputStream)->GetVideoAspectRatio()));

  return true;
}

bool CDVDPlayer::OpenVideoStream(int iStream)
{
  if (!m_pDemuxer)
  {
    CLog::Log(LOGERROR, __FUNCTION__": No Demuxer avilable");
    return false;
  }
  
  CDemuxStream* pStream = m_pDemuxer->GetStream(iStream);
  if( !pStream )
  {
    CLog::Log(LOGERROR, __FUNCTION__": Stream %d doesn't exist", iStream);
    return false;
  }

  if( pStream->disabled )
    return false;

  if (pStream->type != STREAM_VIDEO)
  {
    CLog::Log(LOGERROR, __FUNCTION__": Streamtype is not STREAM_VIDEO");
    return false;
  }

  if ( m_CurrentVideo.id >= 0) CloseVideoStream(true);

  CLog::Log(LOGNOTICE, "Opening video stream: %i", iStream);

  CDVDStreamInfo hint(*pStream, true);

  bool success = false;
  try
  {    
    success = m_dvdPlayerVideo.OpenStream(hint);
  }
  catch (...)
  {
    CLog::Log(LOGERROR, __FUNCTION__" - Exception thrown in player");
    success = false;
  }

  if (!success)
  {
    // mark stream as disabled, to disallaw further attempts
    CLog::Log(LOGWARNING, __FUNCTION__" - Unsupported stream %d. Stream disabled.", iStream);
    pStream->disabled = true;
    return false;
  }

  /* store information about stream */
  m_CurrentVideo.id = iStream;
  m_CurrentVideo.hint = hint;
  m_CurrentVideo.stream = (void*)pStream;

  /* use same priority for video thread as demuxing thread, as */
  /* otherwise demuxer will starve if video consumes the full cpu */
  m_dvdPlayerVideo.SetPriority(GetThreadPriority(*this));
  return true;

}

bool CDVDPlayer::OpenSubtitleStream(int iStream)
{
  if (!m_pDemuxer)
  {
    CLog::Log(LOGERROR, __FUNCTION__": No Demuxer avilable");
    return false;
  }

  CDemuxStream* pStream = m_pDemuxer->GetStream(iStream);
  if( !pStream )
  {
    CLog::Log(LOGERROR, __FUNCTION__": Stream %d doesn't exist", iStream);
    return false;
  }

  if( pStream->disabled )
    return false;

  if (m_CurrentSubtitle.id >= 0) CloseSubtitleStream(true);

  CLog::Log(LOGNOTICE, "Opening Subtitle stream: %i", iStream);

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
  m_dvdspus.FlushCurrentPacket();

  if( !bKeepOverlays ) m_overlayContainer.Clear();

  m_CurrentSubtitle.id = -1;
  m_CurrentSubtitle.dts = DVD_NOPTS_VALUE;
  m_CurrentSubtitle.hint.Clear();

  return true;
}

void CDVDPlayer::FlushBuffers()
{
  m_pDemuxer->Flush();
  m_dvdPlayerAudio.Flush();
  m_dvdPlayerVideo.Flush();
  //m_dvdPlayerSubtitle.Flush();

  // clear subtitle and menu overlays
  m_overlayContainer.Clear();

  m_dvd.iFlagSentStart = 0; //We will have a discontinuity here
}

// since we call ffmpeg functions to decode, this is being called in the same thread as ::Process() is
int CDVDPlayer::OnDVDNavResult(void* pData, int iMessage)
{
  if (m_pInputStream->IsStreamType(DVDSTREAM_TYPE_DVD))
  {
    CDVDInputStreamNavigator* pStream = (CDVDInputStreamNavigator*)m_pInputStream;

    switch (iMessage)
    {
    case DVDNAV_STILL_FRAME:
      {
        //CLog::Log(LOGDEBUG, "DVDNAV_STILL_FRAME");

        dvdnav_still_event_t *still_event = (dvdnav_still_event_t *)pData;
        // should wait the specified time here while we let the player running
        // after that call dvdnav_still_skip(m_dvdnav);

        if (m_dvd.state != DVDSTATE_STILL)
        {
          // else notify the player we have recieved a still frame

          m_dvd.iDVDStillTime = still_event->length;
          m_dvd.iDVDStillStartTime = GetTickCount();

          /* adjust for the output delay in the video queue */
          DWORD time = 0;
          if( m_CurrentVideo.stream )
          {
            time = (DWORD)(m_dvdPlayerVideo.GetOutputDelay() / ( DVD_TIME_BASE / 1000 ));
            if( time < 10000 && time > 0 ) 
              m_dvd.iDVDStillStartTime += time;
          }
          m_dvd.state = DVDSTATE_STILL;
          CLog::Log(LOGDEBUG, "DVDNAV_STILL_FRAME - waiting %i sec, with delay of %d sec", still_event->length, time / 1000);
        }
        return NAVRESULT_HOLD;
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
        bool visible = !(iStream & 0x80);

        // only modify user preference if we are not in menu
        // and we actually have a subtitle stream
        if(!pStream->IsInMenu() && iStream != -1)
          g_stSettings.m_currentVideoSettings.m_SubtitleOn = visible;

        m_dvdPlayerVideo.EnableSubtitle(visible);

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

        //Force an aspect ratio that is set in the dvdheaders if available
        CDVDMsgVideoSetAspect *aspect = new CDVDMsgVideoSetAspect(pStream->GetVideoAspectRatio());
        if( m_dvdPlayerVideo.m_messageQueue.Put(aspect) != MSGQ_OK )
        {
          aspect->Release();
          m_dvdPlayerVideo.SetAspectRatio(pStream->GetVideoAspectRatio());
        }

        return NAVRESULT_HOLD;
      }
      break;
    case DVDNAV_CELL_CHANGE:
      {
        dvdnav_cell_change_event_t* cell_change_event = (dvdnav_cell_change_event_t*)pData;
        CLog::Log(LOGDEBUG, "DVDNAV_CELL_CHANGE");
        //if (cell_change_event->pgN != m_dvd.iCurrentCell)
        {
          m_dvd.iCurrentCell = cell_change_event->pgN;
        }

        m_dvd.state = DVDSTATE_NORMAL;
        m_dvdspus.Reset();
        
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
      }
      break;
    case DVDNAV_HOP_CHANNEL:
      {
        // This event is issued whenever a non-seamless operation has been executed.
        // Applications with fifos should drop the fifos content to speed up responsiveness.
        CLog::Log(LOGDEBUG, "DVDNAV_HOP_CHANNEL");
        m_messenger.Put(new CDVDMsgGeneralFlush());
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


    if( m_dvd.state == DVDSTATE_STILL && pStream->GetTotalButtons() == 0 )
    {
      switch(action.wID)
      {
        case ACTION_NEXT_ITEM:
        case ACTION_MOVE_RIGHT:
        case ACTION_MOVE_UP:
        case ACTION_SELECT_ITEM:
          {
            /* this will force us out of the stillframe */
            CLog::Log(LOGDEBUG, __FUNCTION__ " - User asked to exit stillframe");
            m_dvd.iDVDStillStartTime = 0;
            m_dvd.iDVDStillTime = 0;
            return true;
          }
          break;
      }        
    }


    switch (action.wID)
    {
    case ACTION_PREV_ITEM:  // SKIP-:
      {
        CLog::Log(LOGDEBUG, " - pushed prev");
        pStream->OnPrevious();
        g_infoManager.SetDisplayAfterSeek();
        return true;
      }
      break;
    case ACTION_NEXT_ITEM:  // SKIP+:
      {
        CLog::Log(LOGDEBUG, " - pushed next");
        pStream->OnNext();
        g_infoManager.SetDisplayAfterSeek();
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
    if( m_dvd.state == DVDSTATE_STILL )
      return true;
    else
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

    //Update any forced overlays.
    for(VecOverlays::iterator it = pVecOverlays->begin(); it != pVecOverlays->end(); it++ )     
    {
      if ((*it)->IsOverlayType(DVDOVERLAY_TYPE_SPU))
      {
        CDVDOverlaySpu* pOverlaySpu = (CDVDOverlaySpu*)(*it);

        // make sure its a forced (menu) overlay
        if (pOverlaySpu->bForced)
        {
          // set menu spu color and alpha data if there is a valid menu overlay
          pStream->GetCurrentButtonInfo(pOverlaySpu, &m_dvdspus, iAction);
        }
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

bool CDVDPlayer::GetCurrentSubtitle(CStdString& strSubtitle)
{
  __int64 pts = m_clock.GetClock();
  bool bGotSubtitle = false;//m_dvdPlayerSubtitle.GetCurrentSubtitle(strSubtitle, pts);
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
