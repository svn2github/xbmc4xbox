
#include "GUIWindowFullScreen.h"
#include "settings.h"
#include "application.h"
#include "util.h"
#include "osd/OSDOptionFloatRange.h"
#include "osd/OSDOptionIntRange.h"
#include "osd/OSDOptionBoolean.h"
#include "cores/mplayer/mplayer.h"

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

#define IMG_PAUSE     16

CGUIWindowFullScreen::CGUIWindowFullScreen(void)
:CGUIWindow(0)
{
	m_bShowInfo=false;
	m_dwLastTime=0;
	m_fFPS=0;
	m_fFrameCounter=0.0f;
	m_dwFPSTime=timeGetTime();

  // audio
  //  - language
  //  - volume
  //  - stream

  // video
  //  - Create Bookmark
  //  - Cycle bookmarks
  //  - Clear bookmarks
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
	  switch (action.wID)
    {
		  case ACTION_SHOW_OSD:
			  //g_application.m_pPlayer->ToggleOSD();
        m_bOSDVisible=!m_bOSDVisible;
        if (m_bOSDVisible) ShowOSD();
        else HideOSD();
		  break;

		case ACTION_SHOW_GUI:
			// switch back to the menu
			OutputDebugString("Switching to GUI\n");
			m_gWindowManager.PreviousWindow();
			if (g_application.m_pPlayer)
				g_application.m_pPlayer->Update();
			OutputDebugString("Now in GUI\n");
			return;
		break;
		default:
        m_osdMenu.OnAction(*this,action);
        SET_CONTROL_FOCUS(GetID(), m_osdMenu.GetSelectedMenu()+BTN_OSD_VIDEO); 
		  break;
    }
    return;
  }
	switch (action.wID)
	{
		case ACTION_SHOW_GUI:
			// switch back to the menu
			OutputDebugString("Switching to GUI\n");
			m_gWindowManager.PreviousWindow();
			if (g_application.m_pPlayer)
				g_application.m_pPlayer->Update();
			OutputDebugString("Now in GUI\n");
			return;
		break;
		case ACTION_ASPECT_RATIO:
		{
			m_bShowStatus=true;
			m_dwLastTime=timeGetTime();
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

		case ACTION_SHOW_OSD:
			//g_application.m_pPlayer->ToggleOSD();
      m_bOSDVisible=!m_bOSDVisible;
      if (m_bOSDVisible) ShowOSD();
      else HideOSD();
		break;
			
		case ACTION_SHOW_SUBTITLES:
			g_application.m_pPlayer->ToggleSubtitles();
		break;

		case ACTION_SHOW_CODEC:
			m_bShowInfo = !m_bShowInfo;
		break;

		case ACTION_NEXT_SUBTITLE:
			g_application.m_pPlayer->SwitchToNextLanguage();
		break;

		case ACTION_STOP:
			g_application.m_pPlayer->closefile();
			// Switch back to the previous window (GUI)
			m_gWindowManager.PreviousWindow();
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
	}
	CGUIWindow::OnAction(action);
}

bool CGUIWindowFullScreen::OnMessage(CGUIMessage& message)
{
	switch (message.GetMessage())
	{
		case GUI_MSG_WINDOW_INIT:
		{
      m_bOSDVisible=false;
			CGUIWindow::OnMessage(message);
			g_graphicsContext.Lock();
			g_graphicsContext.SetFullScreenVideo( true );
			g_graphicsContext.Unlock();
			if (g_application.m_pPlayer)
				g_application.m_pPlayer->Update();
      HideOSD();
			return true;
		}
		case GUI_MSG_WINDOW_DEINIT:
		{
			g_graphicsContext.Lock();
			g_graphicsContext.SetFullScreenVideo( false );
			g_graphicsContext.Unlock();
			if (g_application.m_pPlayer)
				g_application.m_pPlayer->Update(true);	
			// Pause so that we make sure that our fullscreen renderer has finished...
			Sleep(100);
      m_bOSDVisible=false;
      HideOSD();
		}
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

void CGUIWindowFullScreen::RenderFullScreen()
{
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
		if ( (timeGetTime() - m_dwLastTime) >=5000)
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
	  CGUIWindow::Render();
    m_osdMenu.Draw();
    return;
  }
  if ( bRenderGUI)
  {
    if (m_bShowStatus||m_bShowInfo)
    {
      SET_CONTROL_VISIBLE(GetID(),LABEL_ROW1);
      SET_CONTROL_VISIBLE(GetID(),LABEL_ROW2);
      SET_CONTROL_VISIBLE(GetID(),LABEL_ROW3);
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
  m_osdMenu.Clear();
  COSDSubMenu videoMenu(291,100,100);
  float fValue=g_application.m_pPlayer->GetAVDelay();
  COSDOptionFloatRange optionAVDelay(MENU_ACTION_AVDELAY,297,-10.0f,10.0f,0.01f,fValue);

  int iValue=g_application.m_pPlayer->GetPercentage();
  COSDOptionIntRange   optionPercentage(MENU_ACTION_SEEK,298,true,0,100,1,iValue);
  videoMenu.AddOption(&optionAVDelay);
  videoMenu.AddOption(&optionPercentage);
  

  COSDSubMenu audioMenu(292,100,100);

  COSDSubMenu SubtitleMenu(293,100,100);
  fValue=g_application.m_pPlayer->GetSubTitleDelay();
  COSDOptionFloatRange optionSubtitleDelay(MENU_ACTION_SUBTITLEDELAY,303,-10.0f,10.0f,0.01f,fValue);
  
  iValue=mplayer_getSubtitle();
  COSDOptionIntRange   optionSubtitleLanguage(MENU_ACTION_SUBTITLELANGUAGE,304,false,0,mplayer_getSubtitleCount()-1,1,iValue);

  iValue=mplayer_SubtitleVisible();
  COSDOptionBoolean    optionEnable(MENU_ACTION_SUBTITLEONOFF,305, (iValue!=0));

  SubtitleMenu.AddOption(&optionSubtitleDelay);
  SubtitleMenu.AddOption(&optionSubtitleLanguage);
  SubtitleMenu.AddOption(&optionEnable);

  m_osdMenu.AddSubMenu(videoMenu);
  m_osdMenu.AddSubMenu(audioMenu);
  m_osdMenu.AddSubMenu(SubtitleMenu);

  SET_CONTROL_VISIBLE(GetID(),BTN_OSD_VIDEO);
  SET_CONTROL_VISIBLE(GetID(),BTN_OSD_AUDIO);
  SET_CONTROL_VISIBLE(GetID(),BTN_OSD_SUBTITLE);

  SET_CONTROL_HIDDEN(GetID(),LABEL_ROW1);
  SET_CONTROL_HIDDEN(GetID(),LABEL_ROW2);
  SET_CONTROL_HIDDEN(GetID(),LABEL_ROW3);
  SET_CONTROL_HIDDEN(GetID(),BLUE_BAR);
}

bool CGUIWindowFullScreen::OSDVisible() const
{
  return m_bOSDVisible;
}

void CGUIWindowFullScreen::OnExecute(int iAction, const IOSDOption* option)
{
  switch (iAction)
  {
    case MENU_ACTION_SEEK:
    {
      const COSDOptionIntRange* intOption = (const COSDOptionIntRange*)option;
      g_application.m_pPlayer->SeekPercentage(intOption->GetValue());
    }
    break;

    case MENU_ACTION_AVDELAY:
    {
      const COSDOptionFloatRange* floatOption = (const COSDOptionFloatRange*)option;
      g_application.m_pPlayer->SetAVDelay(floatOption->GetValue());
    }
    break;

    case MENU_ACTION_SUBTITLEDELAY:
    {
      const COSDOptionFloatRange* floatOption = (const COSDOptionFloatRange*)option;
      g_application.m_pPlayer->SetSubTittleDelay(floatOption->GetValue());
    }
    break;

    
    case MENU_ACTION_SUBTITLEONOFF:
    {
      const COSDOptionBoolean* boolOption = (const COSDOptionBoolean*)option;
      mplayer_showSubtitle(boolOption->GetValue());
    }
    break;
    
    case MENU_ACTION_SUBTITLELANGUAGE:
    {
      const COSDOptionIntRange* intOption = (const COSDOptionIntRange*)option;
      mplayer_setSubtitle(intOption->GetValue());
   }
    break;
  }
}