
#include "stdafx.h"
#include "GUIWindowFullScreen.h"
#include "settings.h"
#include "application.h"
#include "util.h"
//#include "osd/OSDOptionFloatRange.h"
//#include "osd/OSDOptionIntRange.h"
//#include "osd/OSDOptionBoolean.h"
//#include "osd/OSDOptionButton.h"
#include "cores/mplayer/mplayer.h"
#include "utils/singlelock.h"
#include "videodatabase.h"
#include "cores/mplayer/ASyncDirectSound.h"
#include "playlistplayer.h"

#include <stdio.h>

#define BLUE_BAR    0
#define LABEL_ROW1 10
#define LABEL_ROW2 11
#define LABEL_ROW3 12

#define BTN_OSD_VIDEO 13
#define BTN_OSD_AUDIO 14
#define BTN_OSD_SUBTITLE 15

#define MENU_ACTION_AVDELAY       1 
#define MENU_ACTION_SEEK          2
#define MENU_ACTION_SUBTITLEDELAY 3
#define MENU_ACTION_SUBTITLEONOFF 4
#define MENU_ACTION_SUBTITLELANGUAGE 5
#define MENU_ACTION_INTERLEAVED 6
#define MENU_ACTION_FRAMERATECONVERSIONS 7
#define MENU_ACTION_AUDIO_STREAM 8

#define MENU_ACTION_NEW_BOOKMARK  9
#define MENU_ACTION_NEXT_BOOKMARK  10
#define MENU_ACTION_CLEAR_BOOKMARK  11

#define MENU_ACTION_NOCACHE 12

#define IMG_PAUSE     16
#define IMG_2X	      17
#define IMG_4X	      18
#define IMG_8X		  19
#define IMG_16X       20
#define IMG_32X       21

#define LABEL_CURRENT_TIME 22
extern IDirectSoundRenderer* m_pAudioDecoder;
extern int m_iAudioStreamIDX;
CGUIWindowFullScreen::CGUIWindowFullScreen(void)
:CGUIWindow(0)
{
	m_strTimeStamp[0]=0;
	m_iTimeCodePosition=0;
	m_bShowTime=false;
	m_bShowInfo=false;
	m_bShowCurrentTime=false;
	m_dwTimeStatusShowTime=0;
	m_dwTimeCodeTimeout=0;
	m_fFPS=0;
	m_fFrameCounter=0.0f;
	m_dwFPSTime=timeGetTime();

  // audio
  //  - language
  //  - volume
  //  - stream

  // video
  //  - Create Bookmark (294)
  //  - Cycle bookmarks (295)
  //  - Clear bookmarks (296)
  //  - jump to specific time
  //  - slider
  //  - av delay

  // subtitles
  //  - delay
  //  - language
  
}

CGUIWindowFullScreen::~CGUIWindowFullScreen(void)
{
}


void CGUIWindowFullScreen::OnAction(const CAction &action)
{
  
	if (m_bOSDVisible)
	{
		if (action.wID == ACTION_SHOW_OSD && !g_application.m_guiWindowOSD.SubMenuVisible())	// hide the OSD
		{
      CSingleLock lock(m_section);         
      OutputDebugString("CGUIWindowFullScreen::HIDEOSD\n");
			CGUIMessage msg(GUI_MSG_WINDOW_DEINIT,0,0,0,0,NULL);
			g_application.m_guiWindowOSD.OnMessage(msg);	// Send a de-init msg to the OSD
			m_bOSDVisible=false;
		}
		else
		{
      OutputDebugString("CGUIWindowFullScreen::OnAction() reset timeout\n");
      m_dwOSDTimeOut=timeGetTime();
			g_application.m_guiWindowOSD.OnAction(action);	// route keys to OSD window
		}
		return;
	}
    
	switch (action.wID)
	{

    // previous : play previous song from playlist
    case ACTION_PREV_ITEM:
	  {
		  g_playlistPlayer.PlayPrevious();
	  }
    break;

    // next : play next song from playlist
    case ACTION_NEXT_ITEM:
	  {
		  g_playlistPlayer.PlayNext();
	  }
    break;

		case ACTION_SHOW_GUI:
    {
			// switch back to the menu
			OutputDebugString("Switching to GUI\n");
			m_gWindowManager.PreviousWindow();
			if (g_application.m_pPlayer)
				g_application.m_pPlayer->Update();
			OutputDebugString("Now in GUI\n");

			return;
    }
    break;

		case ACTION_ASPECT_RATIO:
		{
			m_bShowStatus=true;
			m_dwTimeStatusShowTime=timeGetTime();
			// zoom->stretch
			if (g_stSettings.m_bZoom)
			{
				g_stSettings.m_bZoom=false;
				g_stSettings.m_bStretch=true;
				g_settings.Save();
				return;
			}
			// stretch->normal
			if (g_stSettings.m_bStretch)
			{
				g_stSettings.m_bZoom=false;
				g_stSettings.m_bStretch=false;
				g_settings.Save();
				return;
			}
			// normal->zoom
			g_stSettings.m_bZoom=true;
			g_stSettings.m_bStretch=false;
			g_settings.Save();
			return;
		}
		break;
		
    case ACTION_STEP_BACK:
    {
      int iPercent=g_application.m_pPlayer->GetPercentage();
      if (iPercent>=2)
      {
        g_application.m_pPlayer->SeekPercentage(iPercent-2);
      }
    }
		break;

		case ACTION_STEP_FORWARD:
    {      
      int iPercent=g_application.m_pPlayer->GetPercentage();
			if (iPercent+2<=100)
      {
        g_application.m_pPlayer->SeekPercentage(iPercent+2);
      }
    }
    break;

		case ACTION_BIG_STEP_BACK:
    {
      int iPercent=g_application.m_pPlayer->GetPercentage();
			if (iPercent>=10)
      {
        g_application.m_pPlayer->SeekPercentage(iPercent-10);
      }
    }
    break;

		case ACTION_BIG_STEP_FORWARD:
    {
      int iPercent=g_application.m_pPlayer->GetPercentage();
			if (iPercent+10<=100)
      {
        g_application.m_pPlayer->SeekPercentage(iPercent+10);
      }
    }
    break;

    case ACTION_SHOW_MPLAYER_OSD:
      g_application.m_pPlayer->ToggleOSD();
    break;

		case ACTION_SHOW_OSD:	// Show the OSD
    {	
      //CSingleLock lock(m_section);      
      OutputDebugString("CGUIWindowFullScreen:SHOWOSD\n");
      m_dwOSDTimeOut=timeGetTime();
      
	    CGUIMessage msg(GUI_MSG_WINDOW_INIT,0,0,0,0,NULL);
	    g_application.m_guiWindowOSD.OnMessage(msg);	// Send an init msg to the OSD
      m_bOSDVisible=true;
      
    }
    break;
			
		case ACTION_SHOW_SUBTITLES:
    {	
      g_application.m_pPlayer->ToggleSubtitles();
    }
		break;

		case ACTION_SHOW_CODEC:
    {
      m_bShowInfo = !m_bShowInfo;
    }
		break;

		case ACTION_NEXT_SUBTITLE:
    {
      g_application.m_pPlayer->SwitchToNextLanguage();
    }
		break;

		case ACTION_STOP:
    {
			g_application.m_pPlayer->closefile();
			// Switch back to the previous window (GUI)
			m_gWindowManager.PreviousWindow();
    }
		break;

		// PAUSE action is handled globally in the Application class
		case ACTION_PAUSE:
			if (g_application.m_pPlayer) g_application.m_pPlayer->Pause();
		break;

    case ACTION_SUBTITLE_DELAY_MIN:
			g_application.m_pPlayer->SubtitleOffset(false);
		break;
		case ACTION_SUBTITLE_DELAY_PLUS:
			g_application.m_pPlayer->SubtitleOffset(true);
		break;
		case ACTION_AUDIO_DELAY_MIN:
			g_application.m_pPlayer->AudioOffset(false);
		break;
		case ACTION_AUDIO_DELAY_PLUS:
			g_application.m_pPlayer->AudioOffset(true);
		break;
		case ACTION_AUDIO_NEXT_LANGUAGE:
			//g_application.m_pPlayer->AudioOffset(false);
		break;
		case ACTION_PLAY:
			if (g_application.m_pPlayer)
      {
        if (g_application.m_pPlayer->IsPaused())
				  g_application.m_pPlayer->Pause();
        else
        {
          g_application.SetPlaySpeed(1);
        }
      }
		m_bShowCurrentTime = true;
    m_dwTimeCodeTimeout=timeGetTime();
		break;
		case ACTION_REWIND:
			ChangetheSpeed(ACTION_REWIND);
		break;
		case ACTION_FORWARD:
			ChangetheSpeed(ACTION_FORWARD);
		break;
		case REMOTE_0:
			ChangetheTimeCode(REMOTE_0);
		break;
		case REMOTE_1:
			ChangetheTimeCode(REMOTE_1);
		break;
		case REMOTE_2:
			ChangetheTimeCode(REMOTE_2);
		break;
		case REMOTE_3:
			ChangetheTimeCode(REMOTE_3);
		break;
		case REMOTE_4:
			ChangetheTimeCode(REMOTE_4);
		break;
		case REMOTE_5:
			ChangetheTimeCode(REMOTE_5);
		break;
		case REMOTE_6:
			ChangetheTimeCode(REMOTE_6);
		break;
		case REMOTE_7:
			ChangetheTimeCode(REMOTE_7);
		break;
		case REMOTE_8:
			ChangetheTimeCode(REMOTE_8);
		break;
		case REMOTE_9:
			ChangetheTimeCode(REMOTE_9);
		break;


 		case ACTION_SMALL_STEP_BACK:
     {
 
 		int orgpos=(int)g_application.m_pPlayer->GetTime();
		int triesleft=g_stSettings.m_iSmallStepBackTries;
        int jumpsize = g_stSettings.m_iSmallStepBackSeconds; // secs
        int setpos=(orgpos > jumpsize) ? orgpos-jumpsize : 0; // First jump = 2*jumpsize
        int newpos;
        do
        {
			setpos = (setpos > jumpsize) ? setpos-jumpsize : 0;
 			g_application.m_pPlayer->SeekTime(setpos);
 			Sleep(g_stSettings.m_iSmallStepBackDelay); // delay to let mplayer finish its seek (in ms)
 			newpos = (int)g_application.m_pPlayer->GetTime();
 		} while ( (newpos>orgpos-jumpsize) && (setpos>0) && (--triesleft>0));
  	}
    break;
	}
	CGUIWindow::OnAction(action);
}

bool CGUIWindowFullScreen::OnMessage(CGUIMessage& message)
{	
  
	if (m_bOSDVisible)
	{
		//if (timeGetTime()-m_dwOSDTimeOut > 5000)
		if ( (timeGetTime() - m_dwOSDTimeOut) > (g_stSettings.m_iOSDTimeout * 1000))
		{
			CSingleLock lock(m_section);
			CGUIMessage msg(GUI_MSG_WINDOW_DEINIT,0,0,0,0,NULL);
			g_application.m_guiWindowOSD.OnMessage(msg);	// Send a de-init msg to the OSD
			m_bOSDVisible=false;
			return g_application.m_guiWindowOSD.OnMessage(message);	// route messages to OSD window
		}

		switch (message.GetMessage())
		{
			case GUI_MSG_SETFOCUS:
			case GUI_MSG_LOSTFOCUS:
			case GUI_MSG_CLICKED:
			case GUI_MSG_WINDOW_INIT:
			case GUI_MSG_WINDOW_DEINIT:
				OutputDebugString("CGUIWindowFullScreen::OnMessage() reset timeout\n");
				m_dwOSDTimeOut=timeGetTime();
			break;
		}
		return g_application.m_guiWindowOSD.OnMessage(message);	// route messages to OSD window
	}

	switch (message.GetMessage())
	{
		case GUI_MSG_WINDOW_INIT:
		{
      m_bLastRender=false;
      m_bOSDVisible=false;
      CUtil::SetBrightnessContrastGammaPercent(g_settings.m_iBrightness,g_settings.m_iContrast,g_settings.m_iGamma,true);
      g_graphicsContext.SetFullScreenVideo(false);//turn off to prevent calibration to OSD position
      CGUIWindow::OnMessage(message);
      g_graphicsContext.Lock();
			g_graphicsContext.Get3DDevice()->Clear( 0L, NULL, D3DCLEAR_TARGET|D3DCLEAR_ZBUFFER|D3DCLEAR_STENCIL, 0x00010001, 1.0f, 0L );
			g_graphicsContext.SetFullScreenVideo( true );
			g_graphicsContext.Get3DDevice()->Present( NULL, NULL, NULL, NULL );
			g_graphicsContext.Unlock();
			if (g_application.m_pPlayer)
				g_application.m_pPlayer->Update();
      HideOSD();
      m_iCurrentBookmark=0;
			return true;
		}
		case GUI_MSG_WINDOW_DEINIT:
		{
      CSingleLock lock(m_section);
      if (m_bOSDVisible)
      {
        CGUIMessage msg(GUI_MSG_WINDOW_DEINIT,0,0,0,0,NULL);
			  g_application.m_guiWindowOSD.OnMessage(msg);	// Send a de-init msg to the OSD
      }
      m_bOSDVisible=false;
      CGUIWindow::OnMessage(message);
      CUtil::RestoreBrightnessContrastGamma();
			g_graphicsContext.Lock();
			g_graphicsContext.SetFullScreenVideo( false );
			g_graphicsContext.Unlock();
			if (g_application.m_pPlayer)
				g_application.m_pPlayer->Update(true);	
			// Pause so that we make sure that our fullscreen renderer has finished...
			Sleep(100);
      
      m_iCurrentBookmark=0;
      HideOSD();
      return true;
		}
    case GUI_MSG_SETFOCUS:
    case GUI_MSG_LOSTFOCUS:
      if (m_bOSDVisible) return true;
      if (message.GetSenderId() != WINDOW_FULLSCREEN_VIDEO) return true;
    break;
	}
	return CGUIWindow::OnMessage(message);
}

// Dummy override of Render() - RenderFullScreen() is where the action takes place
// this is called via mplayer when the video window is flipped (indicating a frame
// change) so that we get smooth video playback
void CGUIWindowFullScreen::Render()
{
	return;
}

bool CGUIWindowFullScreen::NeedRenderFullScreen()
{
  CSingleLock lock(m_section);      
	if (g_application.m_pPlayer) 
  {
    if (g_application.m_pPlayer->IsPaused() )return true;
  }
  
  if (g_application.GetPlaySpeed() != 1) 
  {
		m_bShowCurrentTime = true;
    m_dwTimeCodeTimeout=timeGetTime();
  }
  if (m_bShowTime) return true;
  if (m_bShowStatus) return true;
  if (m_bShowInfo) return true;
  if (m_bShowCurrentTime) return true;
  if (m_bOSDVisible) return true;
  if (m_bLastRender)
  {
    m_bLastRender=false;
  }
  return false;
}

void CGUIWindowFullScreen::RenderFullScreen()
{
  if (g_application.GetPlaySpeed() != 1) 
  {
		m_bShowCurrentTime = true;
    m_dwTimeCodeTimeout=timeGetTime();
  }

  m_bLastRender=true;
	m_fFrameCounter+=1.0f;
	FLOAT fTimeSpan=(float)(timeGetTime()-m_dwFPSTime);
	if (fTimeSpan >=1000.0f)
	{
		fTimeSpan/=1000.0f;
		m_fFPS=(m_fFrameCounter/fTimeSpan);
		m_dwFPSTime=timeGetTime();
		m_fFrameCounter=0;
	}
	if (!g_application.m_pPlayer) return;

  bool bRenderGUI(false);
	if (g_application.m_pPlayer->IsPaused() )
  {
    SET_CONTROL_VISIBLE(GetID(),IMG_PAUSE);  
    bRenderGUI=true;
  }
  else
  {
    SET_CONTROL_HIDDEN(GetID(),IMG_PAUSE);  
  }
 
	if (m_bShowStatus)
	{
		if ( (timeGetTime() - m_dwTimeStatusShowTime) > (g_stSettings.m_iOSDTimeout * 1000))
		//if ( (timeGetTime() - m_dwTimeStatusShowTime) >=5000)
		{
			m_bShowStatus=false;
			return;
		}
    bRenderGUI=true;
		CStdString strStatus;
		if (g_stSettings.m_bZoom) strStatus="Zoom";
		else if (g_stSettings.m_bStretch) strStatus="Stretch";
		else strStatus="Normal";

		if (g_stSettings.m_bSoften)
			strStatus += "  |  Soften";
		else
			strStatus += "  |  No Soften";

		RECT SrcRect;
		RECT DestRect;
		g_application.m_pPlayer->GetVideoRect(SrcRect, DestRect);
		CStdString strRects;
		float fAR;
		g_application.m_pPlayer->GetVideoAspectRatio(fAR);
		strRects.Format(" | (%i,%i)-(%i,%i)->(%i,%i)-(%i,%i) AR:%2.2f", 
											SrcRect.left,SrcRect.top,
											SrcRect.right,SrcRect.bottom,
											DestRect.left,DestRect.top,
											DestRect.right,DestRect.bottom, fAR);
		strStatus += strRects;

		CStdString strStatus2;
		int  iResolution=g_graphicsContext.GetVideoResolution();
		strStatus2.Format("%ix%i %s", g_settings.m_ResInfo[iResolution].iWidth, g_settings.m_ResInfo[iResolution].iHeight, g_settings.m_ResInfo[iResolution].strMode);

		{
			CGUIMessage msg(GUI_MSG_LABEL_SET, GetID(), LABEL_ROW1); 
			msg.SetLabel(strStatus); 
			OnMessage(msg);
		}
		{
			CGUIMessage msg(GUI_MSG_LABEL_SET, GetID(), LABEL_ROW2); 
			msg.SetLabel(strStatus2); 
			OnMessage(msg);
		}
		{
			CGUIMessage msg(GUI_MSG_LABEL_SET, GetID(), LABEL_ROW3); 
			msg.SetLabel(""); 
			OnMessage(msg);
		}
	}

	//------------------------
	if (m_bShowInfo) 
  {
    bRenderGUI=true;
	  // show audio codec info
	  CStdString strAudio, strVideo, strGeneral;
	  g_application.m_pPlayer->GetAudioInfo(strAudio);
	  {	
		  CGUIMessage msg(GUI_MSG_LABEL_SET, GetID(), LABEL_ROW1); 
		  msg.SetLabel(strAudio); 
		  OnMessage(msg);
	  }
	  // show video codec info
	  g_application.m_pPlayer->GetVideoInfo(strVideo);
	  {	
		  CGUIMessage msg(GUI_MSG_LABEL_SET, GetID(), LABEL_ROW2); 
		  msg.SetLabel(strVideo); 
		  OnMessage(msg);
	  }
	  // show general info
	  g_application.m_pPlayer->GetGeneralInfo(strGeneral);
	  {	
		  CStdString strGeneralFPS;
		  strGeneralFPS.Format("fps:%02.2f %s", m_fFPS, strGeneral.c_str() );
		  CGUIMessage msg(GUI_MSG_LABEL_SET, GetID(), LABEL_ROW3); 
		  msg.SetLabel(strGeneralFPS); 
		  OnMessage(msg);
	  }
  }
  if (m_bOSDVisible)
  {
	  // tell the OSD window to draw itself
    CSingleLock lock(m_section);
	  g_application.m_guiWindowOSD.Render();

    return;
  }
    
	if (m_bShowTime && m_iTimeCodePosition != 0)
	{
		if ( (timeGetTime() - m_dwTimeCodeTimeout) >=2500)
		{
			m_bShowTime=false;
			m_iTimeCodePosition = 0;
			return;
		}
	  bRenderGUI=true;
    char displaytime[32] = "??:??/??:??:?? [??:??:??]";
		CGUIMessage msg(GUI_MSG_LABEL_SET, GetID(), LABEL_ROW1); 
		for(int count = 0; count < m_iTimeCodePosition; count++)
		{
			if(m_strTimeStamp[count] == -1)
				displaytime[count] = ':';
			else
				displaytime[count] = (char)m_strTimeStamp[count]+48;
		}
		unsigned int tmpvar = g_application.m_pPlayer->GetTotalTime();
		if(tmpvar != 0)
		{
			int ihour = tmpvar / 3600;
			int imin  = (tmpvar-ihour*3600) / 60;
			int isec = (tmpvar-ihour*3600) % 60;
			sprintf(&displaytime[5], "/%2.2d:%2.2d:%2.2d", ihour,imin,isec);
		}
    else 
    {
			sprintf(&displaytime[5], "/00:00:00");
    }
    __int64 iCurrentTime=g_application.m_pPlayer->GetTime();
    if(iCurrentTime != 0)
		{
			__int64 ihour = iCurrentTime / (__int64)3600;
			__int64 imin  = (iCurrentTime-ihour*3600) / 60;
			__int64 isec = (iCurrentTime-ihour*3600) % 60;
			sprintf(&displaytime[14], " [%2.2d:%2.2d:%2.2d]", (int)ihour,(int)imin,(int)isec);
    }
    else
    {
      sprintf(&displaytime[14], " [??:??:??]");
    }
		msg.SetLabel(displaytime); 
    OnMessage(msg);
  }	

  int iSpeed=g_application.GetPlaySpeed();
	if(iSpeed!=1)
	{
		SET_CONTROL_HIDDEN(GetID(),IMG_2X);
		SET_CONTROL_HIDDEN(GetID(),IMG_4X);
		SET_CONTROL_HIDDEN(GetID(),IMG_8X);
		SET_CONTROL_HIDDEN(GetID(),IMG_16X);
		SET_CONTROL_HIDDEN(GetID(),IMG_32X);
		bRenderGUI=true;
		if(iSpeed == 2 || iSpeed == -2)
		{
			SET_CONTROL_VISIBLE(GetID(),IMG_2X);
		}
		else if(iSpeed == 4 || iSpeed == -4)
		{
			SET_CONTROL_VISIBLE(GetID(),IMG_4X);
		}
		else if(iSpeed == 8 || iSpeed == -8)
		{
			SET_CONTROL_VISIBLE(GetID(),IMG_8X);
		}
		else if(iSpeed == 16 || iSpeed == -16)
		{
			SET_CONTROL_VISIBLE(GetID(),IMG_16X);
		}
		else if(iSpeed == 32 || iSpeed == -32)
		{
			SET_CONTROL_VISIBLE(GetID(),IMG_32X);
		}
		else
		{
			SET_CONTROL_HIDDEN(GetID(),IMG_2X);
			SET_CONTROL_HIDDEN(GetID(),IMG_4X);
			SET_CONTROL_HIDDEN(GetID(),IMG_8X);
			SET_CONTROL_HIDDEN(GetID(),IMG_16X);
			SET_CONTROL_HIDDEN(GetID(),IMG_32X);
		}
	}
	else
	{
		SET_CONTROL_HIDDEN(GetID(),IMG_2X);
		SET_CONTROL_HIDDEN(GetID(),IMG_4X);
		SET_CONTROL_HIDDEN(GetID(),IMG_8X);
		SET_CONTROL_HIDDEN(GetID(),IMG_16X);
		SET_CONTROL_HIDDEN(GetID(),IMG_32X);
	}

	// Render current time if requested
	if (m_bShowCurrentTime)
	{
    CStdString strTime;
		bRenderGUI =true;
		SET_CONTROL_VISIBLE(GetID(),LABEL_CURRENT_TIME);
    __int64 lPTS=10*g_application.m_pPlayer->GetTime();
    int hh = (int)(lPTS / 36000) % 100;
    int mm = (int)((lPTS / 600) % 60);
    int ss = (int)((lPTS /  10) % 60);
    
	  if (hh>=1)
		  strTime.Format("%02.2i:%02.2i:%02.2i",hh,mm,ss);
	  else
		  strTime.Format("%02.2i:%02.2i",mm,ss);
		CGUIMessage msg(GUI_MSG_LABEL_SET, GetID(), LABEL_CURRENT_TIME); 
		msg.SetLabel(strTime); 
		OnMessage(msg); 
    if ( (timeGetTime() - m_dwTimeCodeTimeout) >=2500)
		{
      m_bShowCurrentTime = false;
		}
	}
	else
	{
		SET_CONTROL_HIDDEN(GetID(),LABEL_CURRENT_TIME);
	}

  if ( bRenderGUI)
  {
	if (g_application.m_pPlayer->IsPaused() || iSpeed != 1)
	{
	  SET_CONTROL_HIDDEN(GetID(),LABEL_ROW1);
      SET_CONTROL_HIDDEN(GetID(),LABEL_ROW2);
      SET_CONTROL_HIDDEN(GetID(),LABEL_ROW3);
      SET_CONTROL_HIDDEN(GetID(),BLUE_BAR);
	}
    else if (m_bShowStatus||m_bShowInfo)
    {
      SET_CONTROL_VISIBLE(GetID(),LABEL_ROW1);
      SET_CONTROL_VISIBLE(GetID(),LABEL_ROW2);
      SET_CONTROL_VISIBLE(GetID(),LABEL_ROW3);
      SET_CONTROL_VISIBLE(GetID(),BLUE_BAR);
    }
    else if (m_bShowTime)
	{
		SET_CONTROL_VISIBLE(GetID(),LABEL_ROW1);
		SET_CONTROL_HIDDEN(GetID(),LABEL_ROW2);
		SET_CONTROL_HIDDEN(GetID(),LABEL_ROW3);
		SET_CONTROL_VISIBLE(GetID(),BLUE_BAR);
    }
    else
    {
      SET_CONTROL_HIDDEN(GetID(),LABEL_ROW1);
      SET_CONTROL_HIDDEN(GetID(),LABEL_ROW2);
      SET_CONTROL_HIDDEN(GetID(),LABEL_ROW3);
      SET_CONTROL_HIDDEN(GetID(),BLUE_BAR);
    }
	  CGUIWindow::Render();
  }
}

void CGUIWindowFullScreen::HideOSD()
{
  CSingleLock lock(m_section);      
	m_osdMenu.Clear();
  SET_CONTROL_HIDDEN(GetID(),BTN_OSD_VIDEO);
  SET_CONTROL_HIDDEN(GetID(),BTN_OSD_AUDIO);
  SET_CONTROL_HIDDEN(GetID(),BTN_OSD_SUBTITLE);

  SET_CONTROL_VISIBLE(GetID(),LABEL_ROW1);
  SET_CONTROL_VISIBLE(GetID(),LABEL_ROW2);
  SET_CONTROL_VISIBLE(GetID(),LABEL_ROW3);
  SET_CONTROL_VISIBLE(GetID(),BLUE_BAR);
}

void CGUIWindowFullScreen::ShowOSD()
{

}

bool CGUIWindowFullScreen::OSDVisible() const
{
  return m_bOSDVisible;
}

void CGUIWindowFullScreen::OnExecute(int iAction, const IOSDOption* option)
{

}
void CGUIWindowFullScreen::ChangetheTimeCode(DWORD remote)
{
	if(remote >=58 && remote <= 67) //Make sure it's only for the remote
	{
		m_bShowTime = true;
		m_dwTimeCodeTimeout=timeGetTime();
		int	itime = remote - 58;
		if(m_iTimeCodePosition <= 4 && m_iTimeCodePosition != 2)
		{
			m_strTimeStamp[m_iTimeCodePosition++] = itime;
			if(m_iTimeCodePosition == 2)
				m_strTimeStamp[m_iTimeCodePosition++] = -1;
		}
    if(m_iTimeCodePosition > 4)
		{
			long itotal,ih,im,is=0;                 
			ih =  (m_strTimeStamp[0]-0)*10;
			ih += (m_strTimeStamp[1]-0);   
			im =  (m_strTimeStamp[3]-0)*10;   
			im += (m_strTimeStamp[4]-0);   
			im*=60;
			ih*=3600; 
			itotal = ih+im+is;
			if(itotal < g_application.m_pPlayer->GetTotalTime())
				g_application.m_pPlayer->SeekTime(itotal);
			m_iTimeCodePosition = 0;
      m_bShowTime=false;
		}
	}
}
void CGUIWindowFullScreen::ChangetheSpeed(DWORD action)
{
  int iSpeed=g_application.GetPlaySpeed();
	if (action == ACTION_REWIND && iSpeed == 1) // Enables Rewinding
		iSpeed *=-2;
	else if (action == ACTION_REWIND && iSpeed > 1) //goes down a notch if you're FFing
		iSpeed /=2;
	else if (action == ACTION_FORWARD && iSpeed < 1) //goes up a notch if you're RWing
		iSpeed /= 2;
	else 
		iSpeed *= 2;

	if (action == ACTION_FORWARD && iSpeed == -1) //sets iSpeed back to 1 if -1 (didn't plan for a -1)
		iSpeed = 1;
	if (iSpeed > 32 || iSpeed < -32)
		iSpeed = 1;
  g_application.SetPlaySpeed(iSpeed);
}	

void CGUIWindowFullScreen::Update()
{

}