#include "GUIWindowMusicOverlay.h"
#include "settings.h"
#include "guiWindowManager.h"
#include "localizestrings.h"
#include "util.h"
#include "stdstring.h"
#include "application.h"
#include "MusicInfoTag.h"
#include "picture.h"
#include "musicdatabase.h"
#include "filesystem/CDDADirectory.h"
#include "url.h"
#include "lib/libID3/tag.h"
#include "lib/libID3/misc_support.h"
#include "musicInfoTagLoaderFactory.h"

using namespace MUSIC_INFO;

#define CONTROL_LOGO_PIC    1
#define CONTROL_PLAYTIME		2
#define CONTROL_PLAY_LOGO   3
#define CONTROL_PAUSE_LOGO  4
#define CONTROL_INFO			  5
#define CONTROL_BIG_PLAYTIME 6

CGUIWindowMusicOverlay::CGUIWindowMusicOverlay()
:CGUIWindow(0)
{
  m_iFrames=0;
  m_iPosOrgIcon=0;
	m_pTexture=NULL;
}

CGUIWindowMusicOverlay::~CGUIWindowMusicOverlay()
{
}


void CGUIWindowMusicOverlay::OnAction(const CAction &action)
{
  CGUIWindow::OnAction(action);
}

bool CGUIWindowMusicOverlay::OnMessage(CGUIMessage& message)
{
  return CGUIWindow::OnMessage(message);
}

void CGUIWindowMusicOverlay::SetPosition(int iControl, int iStep, int iSteps,int iOrgPos)
{
  int iScreenHeight=g_graphicsContext.GetHeight();
  float fDiff=(float)iScreenHeight-(float)iOrgPos;
  float fPos = fDiff / ((float)iSteps);
  fPos *= -((float)iStep);
  fPos += (float)iScreenHeight;
  CGUIControl* pControl=(CGUIControl*)GetControl(iControl);
  pControl->SetPosition(pControl->GetXPosition(), (int)fPos);
}
void CGUIWindowMusicOverlay::Render()
{
	if (!g_application.m_pPlayer) return;
	if ( g_application.m_pPlayer->HasVideo()) return;
  if (m_iPosOrgIcon==0)
  {
    m_iPosOrgRectangle=GetControl(0)->GetYPosition();
    m_iPosOrgIcon=GetControl(CONTROL_LOGO_PIC)->GetYPosition();
    m_iPosOrgPlay=GetControl(CONTROL_PLAY_LOGO)->GetYPosition();
    m_iPosOrgPause=GetControl(CONTROL_PAUSE_LOGO)->GetYPosition();
    m_iPosOrgInfo=GetControl(CONTROL_INFO)->GetYPosition();
    m_iPosOrgPlayTime=GetControl(CONTROL_PLAYTIME)->GetYPosition();
    m_iPosOrgBigPlayTime=GetControl(CONTROL_BIG_PLAYTIME)->GetYPosition();
  }  
  if ( m_gWindowManager.GetActiveWindow() != WINDOW_VISUALISATION)
  {
    SetPosition(0, 50,50,m_iPosOrgRectangle);
    SetPosition(CONTROL_LOGO_PIC, 50,50,m_iPosOrgIcon);
    SetPosition(CONTROL_PLAY_LOGO, 50,50,m_iPosOrgPlay);
    SetPosition(CONTROL_PAUSE_LOGO, 50,50,m_iPosOrgPause);
    SetPosition(CONTROL_INFO, 50,50,m_iPosOrgInfo);
    SetPosition(CONTROL_PLAYTIME, 50,50,m_iPosOrgPlayTime);
    SetPosition(CONTROL_BIG_PLAYTIME, 50,50,m_iPosOrgBigPlayTime);
    m_iFrames=0;
  }
  else
  {
    int iSteps=25;
    if (m_iFrames < iSteps)
    {
      // scroll up
      SetPosition(0, m_iFrames,iSteps,m_iPosOrgRectangle);
      SetPosition(CONTROL_LOGO_PIC, m_iFrames,iSteps,m_iPosOrgIcon);
      SetPosition(CONTROL_PLAY_LOGO, m_iFrames,iSteps,m_iPosOrgPlay);
      SetPosition(CONTROL_PAUSE_LOGO, m_iFrames,iSteps,m_iPosOrgPause);
      SetPosition(CONTROL_INFO, m_iFrames,iSteps,m_iPosOrgInfo);
      SetPosition(CONTROL_PLAYTIME, m_iFrames,iSteps,m_iPosOrgPlayTime);
      SetPosition(CONTROL_BIG_PLAYTIME, m_iFrames,iSteps,m_iPosOrgBigPlayTime);
      m_iFrames++;
    }
    else if (m_iFrames >=iSteps && m_iFrames<=5*iSteps+iSteps)
    {
      //show
      SetPosition(0, iSteps,iSteps,m_iPosOrgRectangle);
      SetPosition(CONTROL_LOGO_PIC, iSteps,iSteps,m_iPosOrgIcon);
      SetPosition(CONTROL_PLAY_LOGO, iSteps,iSteps,m_iPosOrgPlay);
      SetPosition(CONTROL_PAUSE_LOGO, iSteps,iSteps,m_iPosOrgPause);
      SetPosition(CONTROL_INFO, iSteps,iSteps,m_iPosOrgInfo);
      SetPosition(CONTROL_PLAYTIME, iSteps,iSteps,m_iPosOrgPlayTime);
      SetPosition(CONTROL_BIG_PLAYTIME, iSteps,iSteps,m_iPosOrgBigPlayTime);
      m_iFrames++;
    }
    else if (m_iFrames >=5*iSteps+iSteps )
    {
      if (m_iFrames > 5*iSteps+2*iSteps) 
      {
        m_iFrames=5*iSteps+2*iSteps;
      }
      //scroll down
      SetPosition(0,                    5*iSteps+2*iSteps-m_iFrames,iSteps,m_iPosOrgRectangle);
      SetPosition(CONTROL_LOGO_PIC,     5*iSteps+2*iSteps-m_iFrames,iSteps,m_iPosOrgIcon);
      SetPosition(CONTROL_PLAY_LOGO,    5*iSteps+2*iSteps-m_iFrames,iSteps,m_iPosOrgPlay);
      SetPosition(CONTROL_PAUSE_LOGO,   5*iSteps+2*iSteps-m_iFrames,iSteps,m_iPosOrgPause);
      SetPosition(CONTROL_INFO,         5*iSteps+2*iSteps-m_iFrames,iSteps,m_iPosOrgInfo);
      SetPosition(CONTROL_PLAYTIME,     5*iSteps+2*iSteps-m_iFrames,iSteps,m_iPosOrgPlayTime);
      SetPosition(CONTROL_BIG_PLAYTIME, 5*iSteps+2*iSteps-m_iFrames,iSteps,m_iPosOrgBigPlayTime);
      m_iFrames++;
    }
  }
	__int64 lPTS=g_application.m_pPlayer->GetPTS();
  int hh = (int)(lPTS / 36000) % 100;
  int mm = (int)((lPTS / 600) % 60);
  int ss = (int)((lPTS /  10) % 60);
  //int f1 = lPTS % 10;
	
	char szTime[32];
	if (hh >=1)
	{
		sprintf(szTime,"%02.2i:%02.2i:%02.2i",hh,mm,ss);
	}
	else
	{
		sprintf(szTime,"%02.2i:%02.2i",mm,ss);
	}
	{
		CGUIMessage msg(GUI_MSG_LABEL_SET, GetID(), CONTROL_PLAYTIME); 
		msg.SetLabel(szTime); 
		OnMessage(msg); 
	}
	{
		CGUIMessage msg(GUI_MSG_LABEL_SET, GetID(), CONTROL_BIG_PLAYTIME); 
		msg.SetLabel(szTime); 
		OnMessage(msg); 
	}

	if (g_application.m_pPlayer->IsPaused() )
	{
		CGUIMessage msg1(GUI_MSG_HIDDEN, GetID(), CONTROL_PLAY_LOGO); 
		OnMessage(msg1);
		CGUIMessage msg2(GUI_MSG_VISIBLE, GetID(), CONTROL_PAUSE_LOGO); 
		OnMessage(msg2); 
	}
	else
	{
		CGUIMessage msg1(GUI_MSG_VISIBLE, GetID(), CONTROL_PLAY_LOGO); 
		OnMessage(msg1);
		CGUIMessage msg2(GUI_MSG_HIDDEN, GetID(), CONTROL_PAUSE_LOGO); 
		OnMessage(msg2); 
	}
	CGUIWindow::Render();

	if (m_pTexture)
	{
		
		const CGUIControl* pControl = GetControl(CONTROL_LOGO_PIC); 
		float fXPos=(float)pControl->GetXPosition();
		float fYPos=(float)pControl->GetYPosition();
		int iWidth=pControl->GetWidth();
		int iHeight=pControl->GetHeight();
		g_graphicsContext.Correct(fXPos,fYPos);

		CPicture picture;
		picture.RenderImage(m_pTexture,(int)fXPos,(int)fYPos,iWidth,iHeight,m_iTextureWidth,m_iTextureHeight);
	}
}

void CGUIWindowMusicOverlay::SetID3Tag(ID3_Tag& id3tag)
{
	auto_ptr<char>pYear  (ID3_GetYear( &id3tag  ));
	auto_ptr<char>pTitle (ID3_GetTitle( &id3tag ));
	auto_ptr<char>pArtist(ID3_GetArtist( &id3tag));
	auto_ptr<char>pAlbum (ID3_GetAlbum( &id3tag ));
	auto_ptr<char>pGenre (ID3_GetGenre( &id3tag ));
	int nTrackNum=ID3_GetTrackNum( &id3tag );
	{
		CGUIMessage msg(GUI_MSG_VISIBLE, GetID(), CONTROL_LOGO_PIC); 
		OnMessage(msg);
	}
	{
		CGUIMessage msg1(GUI_MSG_LABEL_RESET, GetID(), CONTROL_INFO); 
		OnMessage(msg1);
	}
	if (NULL != pTitle.get())
	{
		CGUIMessage msg1(GUI_MSG_LABEL_ADD, GetID(), CONTROL_INFO); 
		msg1.SetLabel(pTitle.get() );
		OnMessage(msg1);
	}

	if (NULL != pArtist.get())
	{
		CGUIMessage msg1(GUI_MSG_LABEL_ADD, GetID(), CONTROL_INFO); 
		msg1.SetLabel( pArtist.get());
		OnMessage(msg1);
	}
		
	if ( NULL != pAlbum.get())
	{
		CGUIMessage msg1(GUI_MSG_LABEL_ADD, GetID(), CONTROL_INFO); 
		msg1.SetLabel( pAlbum.get() );
		OnMessage(msg1);
	}
		
	if (nTrackNum >=1)
	{
		CStdString strTrack;
		strTrack.Format("Track %i", nTrackNum);
		
		CGUIMessage msg1(GUI_MSG_LABEL_ADD, GetID(), CONTROL_INFO); 
		msg1.SetLabel( strTrack );
		OnMessage(msg1);
	}
	if (NULL != pYear.get() )
	{
		CStdString strYear;
		strYear.Format("Year:%s", pYear.get());
		
		CGUIMessage msg1(GUI_MSG_LABEL_ADD, GetID(), CONTROL_INFO); 
		msg1.SetLabel( strYear );
		OnMessage(msg1);
	}
}

	void CGUIWindowMusicOverlay::SetCurrentFile(const CStdString& strFile)
	{

    m_iFrames=0;
		CStdString strAlbum=strFile;
		if (m_pTexture)
		{
			m_pTexture->Release();
			m_pTexture=NULL;
		}
		CGUIMessage msg(GUI_MSG_VISIBLE, GetID(), CONTROL_LOGO_PIC); 
		OnMessage(msg);
		if ( CUtil::IsAudio(strFile) )
		{
			CGUIMessage msg1(GUI_MSG_LABEL_RESET, GetID(), CONTROL_INFO); 
			OnMessage(msg1);

			CSong song;
			CMusicDatabase dbs;
			if (dbs.Open())
			{
				bool bContinue(false);
				CURL url(strFile);
				if (url.GetProtocol()=="cdda" )
				{
					VECFILEITEMS  items;
					CCDDADirectory dir;
					if (dir.GetDirectory("D:",items))
					{
						for (int i=0; i < (int)items.size(); ++i)
						{
							CFileItem* pItem=items[i];
							if (pItem->m_strPath==strFile)
							{
								SYSTEMTIME systime;
								song.iTrack		=pItem->m_musicInfoTag.GetTrackNumber();
								song.strAlbum =pItem->m_musicInfoTag.GetAlbum();
								song.strArtist=pItem->m_musicInfoTag.GetArtist();
								song.strFileName=strFile;
								song.strGenre=pItem->m_musicInfoTag.GetGenre();
								song.strTitle=pItem->m_musicInfoTag.GetTitle();
								song.iDuration=pItem->m_musicInfoTag.GetDuration();
								pItem->m_musicInfoTag.GetReleaseDate(systime);
								song.iYear=systime.wYear;
								strAlbum=song.strAlbum ;
								bContinue=true;
							}
							delete pItem;
						}
					}
				}
				else
				{
					bContinue=dbs.GetSongByFileName(strFile, song);
					if (!bContinue && g_stSettings.m_bUseID3)
					{
            // get correct tag parser
						CMusicInfoTagLoaderFactory factory;
						auto_ptr<IMusicInfoTagLoader> pLoader (factory.CreateLoader(strFile));
						if (NULL != pLoader.get())
						{
							CMusicInfoTag tag;
							// get id3tag
							if ( pLoader->Load(strFile,tag))
							{
								SYSTEMTIME systime;
								song.iTrack		=tag.GetTrackNumber();
								song.strAlbum =tag.GetAlbum();
								song.strArtist=tag.GetArtist();
								song.strFileName=strFile;
								song.strGenre=tag.GetGenre();
								song.strTitle=tag.GetTitle();
								song.iDuration=tag.GetDuration();
								tag.GetReleaseDate(systime);
								song.iYear=systime.wYear;
								strAlbum=song.strAlbum ;
								bContinue=true;
							}
						}
					}
				}
				if (bContinue)
				{
					if (song.strTitle.size())
					{
						CGUIMessage msg1(GUI_MSG_LABEL_ADD, GetID(), CONTROL_INFO); 
						msg1.SetLabel(song.strTitle );
						OnMessage(msg1);
						
						if (song.strArtist.size())
						{
							CGUIMessage msg1(GUI_MSG_LABEL_ADD, GetID(), CONTROL_INFO); 
							msg1.SetLabel( song.strArtist );
							OnMessage(msg1);
						}
						
						if ( song.strAlbum.size())
						{
							CGUIMessage msg1(GUI_MSG_LABEL_ADD, GetID(), CONTROL_INFO); 
							msg1.SetLabel( song.strAlbum );
							OnMessage(msg1);
							strAlbum=song.strAlbum;
						}
						
						int iTrack=song.iTrack;
						if (iTrack >=1)
						{
							CStdString strTrack;
							strTrack.Format("Track %i", iTrack);
							
							CGUIMessage msg1(GUI_MSG_LABEL_ADD, GetID(), CONTROL_INFO); 
							msg1.SetLabel( strTrack );
							OnMessage(msg1);
						}

						if (song.iYear >=1900)
						{
							CStdString strYear;
							strYear.Format("Year: %i", song.iYear);
							
							CGUIMessage msg1(GUI_MSG_LABEL_ADD, GetID(), CONTROL_INFO); 
							msg1.SetLabel( strYear );
							OnMessage(msg1);
						}
						if (song.iDuration > 0)
						{
							CStdString strYear;
							strYear.Format("Duration: %i:%02.2i", song.iDuration/60, song.iDuration%60);
							
							CGUIMessage msg1(GUI_MSG_LABEL_ADD, GetID(), CONTROL_INFO); 
							msg1.SetLabel( strYear );
							OnMessage(msg1);
						}
					}
					else 
					{
							CGUIMessage msg1(GUI_MSG_LABEL_ADD, GetID(), CONTROL_INFO); 
							msg1.SetLabel( CUtil::GetFileName(strFile) );
							OnMessage(msg1);
					}
					dbs.Close();
				}
				else 
				{
					CGUIMessage msg1(GUI_MSG_LABEL_ADD, GetID(), CONTROL_INFO); 
					msg1.SetLabel( CUtil::GetFileName(strFile) );
					OnMessage(msg1);
				}
			}
			else
			{
					CGUIMessage msg1(GUI_MSG_LABEL_ADD, GetID(), CONTROL_INFO); 
					msg1.SetLabel( CUtil::GetFileName(strFile) );
					OnMessage(msg1);
			}

			CStdString strThumb;
			CUtil::GetAlbumThumb(strAlbum,strThumb);
			if (CUtil::FileExists(strThumb) )
			{
				CPicture picture;
				m_pTexture=picture.Load(strThumb);
				m_iTextureWidth=picture.GetWidth();
				m_iTextureHeight=picture.GetHeight();
				if (m_pTexture)
				{							
					CGUIMessage msg1(GUI_MSG_HIDDEN, GetID(), CONTROL_LOGO_PIC); 
					OnMessage(msg1);
				}
			}
		}
	}