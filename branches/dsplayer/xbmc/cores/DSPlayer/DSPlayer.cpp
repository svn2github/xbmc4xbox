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

#include "DSPlayer.h"
#include "winsystemwin32.h" //Important needed to get the right hwnd
#include "utils/GUIInfoManager.h"
#include "Application.h"
#include "Settings.h"
#include "FileItem.h"
#include "utils/log.h"
#include "RegExp.h"
#include "URL.h"

#include "dshowutil/dshowutil.h" // unload loaded filters

#ifdef HAS_VIDEO_PLAYBACK
#include "cores/VideoRenderers/RenderManager.h"
#endif
using namespace std;

DSPLAYER_STATE CDSPlayer::PlayerState = DSPLAYER_CLOSED;
CFileItem CDSPlayer::currentFileItem;

CDSPlayer::CDSPlayer(IPlayerCallback& callback)
    : IPlayer(callback),
      CThread(),
      m_pDsGraph()
{
  m_hReadyEvent = CreateEvent(NULL, true, false, NULL);
}

CDSPlayer::~CDSPlayer()
{
  if (PlayerState != DSPLAYER_CLOSED)
    CloseFile();

  StopThread();

  DShowUtil::UnloadExternalObjects();
  CLog::Log(LOGDEBUG, "%s External objects unloaded", __FUNCTION__);
}

bool CDSPlayer::OpenFile(const CFileItem& file,const CPlayerOptions &options)
{
  if(PlayerState != DSPLAYER_CLOSED)
    CloseFile();

  PlayerState = DSPLAYER_LOADING;
  HRESULT hr;

  currentFileItem = file;

  //Creating the graph and querying every filter required for the playback
  ResetEvent(m_hReadyEvent);
  m_Filename = file.GetAsUrl();
  m_PlayerOptions = options;
  m_currentSpeed = 10000;
  m_currentRate = 1.0;

  hr = m_pDsGraph.SetFile(file, m_PlayerOptions);
  if ( FAILED(hr) )
  {
    CLog::Log(LOGERROR,"%s failed to start this file with dsplayer %s", __FUNCTION__,file.GetAsUrl().GetFileName().c_str());
    CloseFile();
    return false;
  }
  
  Create();
  WaitForSingleObject(m_hReadyEvent, INFINITE);
  return true;
}
bool CDSPlayer::CloseFile()
{
  if (PlayerState == DSPLAYER_CLOSED)
    return true;

  PlayerState = DSPLAYER_CLOSING;

  m_pDsGraph.CloseFile();
  
  CLog::Log(LOGNOTICE, "%s DSPlayer is now closed", __FUNCTION__);
  
  PlayerState = DSPLAYER_CLOSED;

  return true;
}

bool CDSPlayer::IsPlaying() const
{
  return !m_bStop;
}

bool CDSPlayer::HasVideo() const
{
  return true;
}
bool CDSPlayer::HasAudio() const
{
  return true;
}

//TO DO EVERY INFO FOR THE GUI
void CDSPlayer::GetAudioInfo(CStdString& strAudioInfo)
{
  CSingleLock lock(m_StateSection);
  strAudioInfo = m_pDsGraph.GetAudioInfo();
  //"CDSPlayer:GetAudioInfo";
  //this is the function from dvdplayeraudio
  //s << "aq:"     << setw(2) << min(99,100 * m_messageQueue.GetDataSize() / m_messageQueue.GetMaxDataSize()) << "%";
  //s << ", kB/s:" << fixed << setprecision(2) << (double)GetAudioBitrate() / 1024.0;
  //return s.str();
}

void CDSPlayer::GetVideoInfo(CStdString& strVideoInfo)
{  
  CSingleLock lock(m_StateSection);
  strVideoInfo = m_pDsGraph.GetVideoInfo();
  //strVideoInfo.Format("D(%s) P(%s)", m_pDsGraph.GetPlayerInfo().c_str());
}

void CDSPlayer::GetGeneralInfo(CStdString& strGeneralInfo)
{
  strGeneralInfo = m_pDsGraph.GetGeneralInfo();
  //GetGeneralInfo.Format("CPU:( ThreadRelative:%2i%% FiltersThread:%2i%% )"                         
  //                       , (int)(CThread::GetRelativeUsage()*100)
  //               , (int) (m_pDsGraph.GetRelativeUsage()*100));
  //strGeneralInfo = "CDSPlayer:GetGeneralInfo";

}

//CThread
void CDSPlayer::OnStartup()
{
  CThread::SetName("CDSPlayer");
}

void CDSPlayer::OnExit()
{
  /*try
  {
    m_pDsGraph.CloseFile();
   
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "%s - Exception thrown when trying to close the graph", __FUNCTION__);
  }*/

  if (PlayerState == DSPLAYER_CLOSING)
    m_callback.OnPlayBackStopped();
  else
    m_callback.OnPlayBackEnded();

  m_bStop = true;
}

void CDSPlayer::HandleStart()
{
  if (m_PlayerOptions.starttime>0)
    SendMessage(g_hWnd,WM_COMMAND, ID_SEEK_TO ,((LPARAM)m_PlayerOptions.starttime * 1000 ));
  //SendMessage(g_hWnd,WM_COMMAND, ID_DS_HIDE_SUB ,0);
  //In case ffdshow has the subtitles filter enabled
  
  // That's done by XBMC when starting playing
  /*if ( CStreamsManager::getSingleton()->GetSubtitleCount() == 0 )
    CStreamsManager::getSingleton()->SetSubtitleVisible(false);
  else
  {
    //If there more than one we will load the first one in the list
    // CStreamsManager::getSingleton()->SetSubtitle(0); // No Need, already connected on graph setup
    CStreamsManager::getSingleton()->SetSubtitleVisible(true);
  }*/
}

void CDSPlayer::Process()
{
  m_callback.OnPlayBackStarted();

  bool pStartPosDone = false;
  // allow renderer to switch to fullscreen if requested
  //m_pDsGraph.EnableFullscreen(true);
  // make sure application know our info
  //UpdateApplication(0);
  //UpdatePlayState(0);

  SetEvent(m_hReadyEvent);
  while (PlayerState != DSPLAYER_CLOSING && PlayerState != DSPLAYER_CLOSED)
  {
    if (PlayerState == DSPLAYER_CLOSING || PlayerState == DSPLAYER_CLOSED)
      break;

    //The graph need to be started to handle those stuff
    if (!pStartPosDone)
    {
      HandleStart();
      pStartPosDone = true;
    }

    if (PlayerState == DSPLAYER_CLOSING || PlayerState == DSPLAYER_CLOSED)
      break;

    m_pDsGraph.HandleGraphEvent();

    if (PlayerState == DSPLAYER_CLOSING || PlayerState == DSPLAYER_CLOSED)
      break;
    
    //Handle fastforward stuff
    if (m_currentSpeed == 0)
    {      
      Sleep(250);
    } 
    else if (m_currentSpeed != 10000)
    {
      m_pDsGraph.DoFFRW(m_currentSpeed);
      Sleep(100);
    } 
    else
    {
      Sleep(250);
      m_pDsGraph.UpdateTime();
      CChaptersManager::getSingleton()->UpdateChapters();
    }

    if (PlayerState == DSPLAYER_CLOSING || PlayerState == DSPLAYER_CLOSED)
      break;

    if (m_pDsGraph.FileReachedEnd())
    { 
      CLog::Log(LOGDEBUG,"%s Graph detected end of video file",__FUNCTION__);
      CloseFile();
      break;
    }
  }  
}

void CDSPlayer::Stop()
{
  SendMessage(g_hWnd,WM_COMMAND, ID_STOP_DSPLAYER,0);
}

void CDSPlayer::Pause()
{
  
  if ( PlayerState == DSPLAYER_PAUSED )
  {
    m_currentSpeed = 10000;
    m_callback.OnPlayBackResumed();    
  } 
  else
  {
    m_currentSpeed = 0;
    m_callback.OnPlayBackPaused();
  }

  SendMessage(g_hWnd,WM_COMMAND, ID_PLAY_PAUSE,0);
}
void CDSPlayer::ToFFRW(int iSpeed)
{
  if (iSpeed != 1)
    g_infoManager.SetDisplayAfterSeek();
  switch(iSpeed)
  {
    case -1:
      m_currentRate = -1;
      m_currentSpeed=-10000;
      break;
    case -2:
      m_currentRate = -2;
      m_currentSpeed = -15000;
      break;
    case -4:
      m_currentRate = -4;
      m_currentSpeed = -30000;
      break;
    case -8:
      m_currentRate = -8;
      m_currentSpeed = -45000;
      break;
    case -16:
      m_currentRate = -16;
      m_currentSpeed = -60000;
      break;
    case -32:
      m_currentRate = -32;
      m_currentSpeed = -75000;
      break;
    case 1:
      m_currentRate = 1;
      m_currentSpeed = 10000;
      SendMessage(g_hWnd,WM_COMMAND, ID_PLAY_PLAY,0);
    //mediaCtrl.Run();
      break;
    case 2:
      m_currentRate = 2;
      m_currentSpeed = 15000;
      break;
    case 4:
      m_currentRate = 4;
      m_currentSpeed = 30000;
      break;
    case 8:
      m_currentRate = 8;
      m_currentSpeed = 45000;
      break;
    case 16:
      m_currentRate = 16;
      m_currentSpeed = 60000;
      break;
    default:
      m_currentRate = 32;
      m_currentSpeed = 75000;
      break;
  }
  //SendMessage(g_hWnd,WM_COMMAND, ID_SET_SPEEDRATE,iSpeed);
}

void CDSPlayer::Seek(bool bPlus, bool bLargeStep)
{
  if (bPlus)
  {
    if (!bLargeStep)
      SendMessage(g_hWnd, WM_COMMAND, ID_SEEK_FORWARDSMALL,0);
    else
      SendMessage(g_hWnd, WM_COMMAND, ID_SEEK_FORWARDLARGE,0);
  }
  else
  {
    if (!bLargeStep)
      SendMessage(g_hWnd, WM_COMMAND, ID_SEEK_BACKWARDSMALL,0);
    else
      SendMessage(g_hWnd, WM_COMMAND, ID_SEEK_BACKWARDLARGE,0);
  }
}

void CDSPlayer::SeekPercentage(float iPercent)
{
  SendMessage(g_hWnd,WM_COMMAND, ID_SEEK_PERCENT,(LPARAM)iPercent);
}

bool CDSPlayer::OnAction(const CAction &action)
{
#define THREAD_ACTION(action) \
  do { \
    if(GetCurrentThreadId() != CThread::ThreadId()) { \
      m_messenger.Put(new CDVDMsgType<CAction>(CDVDMsg::GENERAL_GUI_ACTION, action)); \
      return true; \
    } \
  } while(false)

  switch(action.actionId)
  {
    case ACTION_NEXT_ITEM:
    case ACTION_PAGE_UP:
      if(GetChapterCount() > 0)
      {
        SeekChapter( GetChapter() + 1 );
        g_infoManager.SetDisplayAfterSeek();
        return true;
      }
      else
        break;
    case ACTION_PREV_ITEM:
    case ACTION_PAGE_DOWN:
      if(GetChapterCount() > 0)
      {
        SeekChapter( GetChapter() - 1 );
        g_infoManager.SetDisplayAfterSeek();
        return true;
      }
      else
        break;
  }
  
  // return false to inform the caller we didn't handle the message
  return false;
  
}

