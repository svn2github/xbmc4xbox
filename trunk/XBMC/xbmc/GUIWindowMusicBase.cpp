
#include "stdafx.h"
#include "guiwindowmusicbase.h"
#include "settings.h"
#include "sectionloader.h"
#include "guiWindowManager.h"
#include "localizestrings.h"
#include "GUIDialogSelect.h"
#include "utils/MusicInfoScraper.h"
#include "musicInfoTagLoaderFactory.h"
#include "GUIWindowMusicInfo.h"
#include "GUIDialogOK.h"
#include "filesystem/HDdirectory.h"
#include "PlayListFactory.h"
#include "util.h"
#include "url.h"
#include "keyboard/virtualkeyboard.h"
#include "PlayListM3U.h"
#include "application.h"
#include "playlistplayer.h"
#include <algorithm>
#include "GuiUserMessages.h"
#include "GUIThumbnailPanel.h"

#define CONTROL_BTNVIEWASICONS		2
#define CONTROL_BTNTYPE						6
#define CONTROL_BTNSEARCH					8

#define CONTROL_LIST							50
#define CONTROL_THUMBS						51

using namespace MUSIC_GRABBER;
using namespace DIRECTORY;
using namespace PLAYLIST;

int CGUIWindowMusicBase::m_nTempPlayListWindow=0;
CStdString CGUIWindowMusicBase::m_strTempPlayListDirectory="";

CGUIWindowMusicBase::CGUIWindowMusicBase ()
:CGUIWindow(0)
{
	m_nSelectedItem=-1;
	m_iLastControl=-1;
}

CGUIWindowMusicBase::~CGUIWindowMusicBase ()
{

}

/// \brief Handle actions on window.
/// \param action Action that can be reacted on.
void CGUIWindowMusicBase::OnAction(const CAction& action)
{
	if (action.wID==ACTION_PARENT_DIR)
	{
		GoParentFolder();
		return;
	}

  if (action.wID==ACTION_PREVIOUS_MENU)
  {
		g_musicDatabase.Close();
		CUtil::ThumbCacheClear();
		CUtil::RemoveTempFiles();
		m_gWindowManager.ActivateWindow(WINDOW_HOME);
		return;
  }

	if (action.wID==ACTION_SHOW_PLAYLIST)
	{
		m_gWindowManager.ActivateWindow(WINDOW_MUSIC_PLAYLIST);
		return;
	}

	CGUIWindow::OnAction(action);
}

/*!
	\brief Handle messages on window.
	\param message GUI Message that can be reacted on.
	\return if a message can't be processed, return \e false

	On these messages this class reacts.\n
	When retrieving...
		- #GUI_MSG_PLAYBACK_ENDED\n 
			...and...
		- #GUI_MSG_PLAYBACK_STOPPED\n
			...it deselects the current playing item in list/thumb control, 
			if we are in a temporary playlist or in playlistwindow
		- #GUI_MSG_PLAYLIST_PLAY_NEXT_PREV\n
			...the next playing item is set in list/thumb control
		- #GUI_MSG_DVDDRIVE_EJECTED_CD\n
			...it will look, if m_strDirectory contains a path from a DVD share.
			If it is, Update() is called with a empty directory.
		- #GUI_MSG_DVDDRIVE_CHANGED_CD\n
			...and m_strDirectory is empty, Update is called to renew icons after 
			disc is changed.
		- #GUI_MSG_WINDOW_DEINIT\n
			...the last focused control is saved to m_iLastControl.
		- #GUI_MSG_WINDOW_INIT\n
			...the musicdatabase is opend and the music extensions and shares are set.
			The last focused control is set.
		- #GUI_MSG_CLICKED\n
			... the base class reacts on the following controls:\n
				Buttons:\n
				- #CONTROL_BTNVIEWASICONS - switch between list, thumb and with large items
				- #CONTROL_BTNTYPE - switch between music windows
				- #CONTROL_BTNSEARCH - Search for items\n
				Other Controls:
				- #CONTROL_LIST and #CONTROL_THUMB\n
					Have the following actions in message them clicking on them.
					- #ACTION_QUEUE_ITEM - add selected item to playlist
					- #ACTION_SHOW_INFO - retrieve album info from the internet
					- #ACTION_SELECT_ITEM - Item has been selected. Overwrite OnClick() to react on it
	*/
bool CGUIWindowMusicBase::OnMessage(CGUIMessage& message)
{
  switch ( message.GetMessage() )
  {
		case GUI_MSG_PLAYBACK_ENDED:
		case GUI_MSG_PLAYBACK_STOPPED:
		{
			CStdString strDirectory=m_strDirectory;
			if (CUtil::HasSlashAtEnd(strDirectory))
				strDirectory.Delete(strDirectory.size()-1);
			if ((m_nTempPlayListWindow==GetID() && m_strTempPlayListDirectory==strDirectory)
					|| (GetID()==WINDOW_MUSIC_PLAYLIST) )
			{
				for (int i=0; i < (int)m_vecItems.size(); ++i)
				{
					CFileItem* pItem=m_vecItems[i];
					if (pItem && pItem->IsSelected())
					{
						pItem->Select(false);
						break;
					}
				}
			}
		}
		break;

		case GUI_MSG_PLAYLIST_PLAY_NEXT_PREV:
		{
			// started playing another song...
			int nCurrentPlaylist=message.GetParam1();
			CStdString strDirectory=m_strDirectory;
			if (CUtil::HasSlashAtEnd(strDirectory))
				strDirectory.Delete(strDirectory.size()-1);
			if ((nCurrentPlaylist==PLAYLIST_MUSIC_TEMP && m_nTempPlayListWindow==GetID() && m_strTempPlayListDirectory==strDirectory )
					|| (GetID()==WINDOW_MUSIC_PLAYLIST && nCurrentPlaylist==PLAYLIST_MUSIC))
			{
				int nCurrentItem=LOWORD(message.GetParam2());
				int nPreviousItem=(int)HIWORD(message.GetParam2());

				int nFolderCount=CUtil::GetFolderCount(m_vecItems);

				//	is the previous item in this directory
				for (int i=nFolderCount, n=0; i<(int)m_vecItems.size(); i++)
				{
					CFileItem* pItem=m_vecItems[i];

					if (pItem)
						pItem->Select(false);
				}

				if (nFolderCount+nCurrentItem<(int)m_vecItems.size())
				{
					for (int i=nFolderCount, n=0; i<(int)m_vecItems.size(); i++)
					{
						CFileItem* pItem=m_vecItems[i];

						if (pItem)
						{
							if (!CUtil::IsPlayList(pItem->m_strPath) && !CUtil::IsNFO(pItem->m_strPath))
								n++;
							if ((n-1)==nCurrentItem)
							{
								pItem->Select(true);
								break;
							}
						}
					}	//	for (int i=nFolderCount, n=0; i<(int)m_vecItems.size(); i++)
				}

			}
		}
		break;

		case GUI_MSG_DVDDRIVE_EJECTED_CD:
		{
			if ( !m_strDirectory.IsEmpty() ) 
			{
				if (CUtil::IsCDDA(m_strDirectory) || CUtil::IsDVD(m_strDirectory) || CUtil::IsISO9660(m_strDirectory)) 
				{
					//	Disc has changed and we are inside a DVD Drive share, get out of here :)
					Update("");
				}
			}
			else 
			{
				int iItem = GetSelectedItem();
				Update(m_strDirectory);
				CONTROL_SELECT_ITEM(GetID(), CONTROL_LIST,iItem)
				CONTROL_SELECT_ITEM(GetID(), CONTROL_THUMBS,iItem)
			}
		}
		break;

		case GUI_MSG_DVDDRIVE_CHANGED_CD:
		{
			if (m_strDirectory.IsEmpty()) 
			{
				int iItem = GetSelectedItem();
				Update(m_strDirectory);
				CONTROL_SELECT_ITEM(GetID(), CONTROL_LIST,iItem)
				CONTROL_SELECT_ITEM(GetID(), CONTROL_THUMBS,iItem)
			}
		}
		break;

    case GUI_MSG_WINDOW_DEINIT:
		{
			m_nSelectedItem=GetSelectedItem();
			m_iLastControl=GetFocusedControl();
      ClearFileItems();
			CSectionLoader::Unload("LIBID3");
		}
		break;

    case GUI_MSG_WINDOW_INIT:
		{
			CGUIWindow::OnMessage(message);

			CSectionLoader::Load("LIBID3");

			if (!g_musicDatabase.IsOpen())
				g_musicDatabase.Open();

			m_dlgProgress = (CGUIDialogProgress*)m_gWindowManager.GetWindow(WINDOW_DIALOG_PROGRESS);

			m_rootDir.SetMask(g_stSettings.m_szMyMusicExtensions);
			m_rootDir.SetShares(g_settings.m_vecMyMusicShares);
			
			if (m_iLastControl>-1)
			{
				SET_CONTROL_FOCUS(GetID(), m_iLastControl, 0);
			}

			Update(m_strDirectory);
 
			if (m_nSelectedItem>-1)
			{
				CONTROL_SELECT_ITEM(GetID(), CONTROL_LIST,m_nSelectedItem);
				CONTROL_SELECT_ITEM(GetID(), CONTROL_THUMBS,m_nSelectedItem);
			}

			return true;
		}
		break;

    case GUI_MSG_CLICKED:
    {
      int iControl=message.GetSenderId();
			if (iControl==CONTROL_BTNVIEWASICONS)
      {
				if ( m_strDirectory.IsEmpty() )
				{
					m_iViewAsIconsRoot++;
					if (m_iViewAsIconsRoot > VIEW_AS_LARGEICONS) m_iViewAsIconsRoot=VIEW_AS_LIST;
				}
				else
				{
					m_iViewAsIcons++;
					if (m_iViewAsIcons > VIEW_AS_LARGEICONS) m_iViewAsIcons=VIEW_AS_LIST;
				}
				ShowThumbPanel();
				UpdateButtons();
			}
			else if (iControl==CONTROL_BTNTYPE)
			{
				CGUIMessage msg(GUI_MSG_ITEM_SELECTED, GetID(),CONTROL_BTNTYPE);
				m_gWindowManager.SendMessage(msg);

				int nWindow=WINDOW_MUSIC_FILES+msg.GetParam1();

				if (nWindow==GetID())
					return true;

				g_stSettings.m_iMyMusicStartWindow=nWindow;
				g_settings.Save();
				m_gWindowManager.ActivateWindow(g_stSettings.m_iMyMusicStartWindow);
				SET_CONTROL_FOCUS(g_stSettings.m_iMyMusicStartWindow, CONTROL_BTNTYPE, 0);
				return true;
			}
			else if (iControl==CONTROL_BTNSEARCH)
			{
				OnSearch();
			}
      else if (iControl==CONTROL_LIST||iControl==CONTROL_THUMBS)  // list/thumb control
      {
        int iItem=GetSelectedItem();
				int iAction=message.GetParam1();

				if (iAction == ACTION_QUEUE_ITEM)
        {
					OnQueueItem(iItem);
        }
        else if (iAction==ACTION_SHOW_INFO)
        {
          OnInfo(iItem);
        }
        else if (iAction==ACTION_SELECT_ITEM)
        {
          OnClick(iItem);
        }
      }
    }
	}

  return CGUIWindow::OnMessage(message);
}

/// \brief Remove items from list/thumb control and \e m_vecItems.
void CGUIWindowMusicBase::ClearFileItems()
{
  CGUIMessage msg1(GUI_MSG_LABEL_RESET,GetID(),CONTROL_LIST,0,0,NULL);
  g_graphicsContext.SendMessage(msg1);         

  CGUIMessage msg2(GUI_MSG_LABEL_RESET,GetID(),CONTROL_THUMBS,0,0,NULL);
  g_graphicsContext.SendMessage(msg2);         

	CFileItemList itemlist(m_vecItems); // will clean up everything
}

/// \brief Updates list/thumb control
/// Sets item labels (text and thumbs), sorts items and adds them to the control
void CGUIWindowMusicBase::UpdateListControl()
{
  CGUIMessage msg1(GUI_MSG_LABEL_RESET,GetID(),CONTROL_LIST,0,0,NULL);
  g_graphicsContext.SendMessage(msg1);         

  CGUIMessage msg2(GUI_MSG_LABEL_RESET,GetID(),CONTROL_THUMBS,0,0,NULL);
  g_graphicsContext.SendMessage(msg2);         

	//	Cache available album thumbs
  VECFILEITEMS qitems;
  CHDDirectory dir;

  CStdString strThumb=g_stSettings.m_szAlbumDirectory;
  strThumb+="\\thumbs";
  {
    CFileItemList itemlist(qitems); // will clean up everything
    dir.GetDirectory(strThumb.c_str(),qitems);			//	precache Q:\albums\thumbs directory
  }
  {
    strThumb+="\\temp";
    CFileItemList itemlist(qitems); // will clean up everything
    dir.GetDirectory(strThumb.c_str(),qitems);			//	precache Q:\albums\thumbs\temp directory
  }

	for (int i=0; i < (int)m_vecItems.size(); i++)
	{
		CFileItem* pItem=m_vecItems[i];
		m_lPlayListSeq = (i+1);

		//	Format label for listcontrol
		//	and set thumb/icon for item
		OnFileItemFormatLabel(pItem);
	}

  CUtil::ClearCache();

	DoSort(m_vecItems);

	ShowThumbPanel();

	for (int i=0; i < (int)m_vecItems.size(); i++)
	{
		CFileItem* pItem=m_vecItems[i];

    CGUIMessage msg(GUI_MSG_LABEL_ADD,GetID(),CONTROL_LIST,0,0,(void*)pItem);
    g_graphicsContext.SendMessage(msg);

    CGUIMessage msg2(GUI_MSG_LABEL_ADD,GetID(),CONTROL_THUMBS,0,0,(void*)pItem);
    g_graphicsContext.SendMessage(msg2);
	}
}

/// \brief Returns the selected list/thumb control item
int CGUIWindowMusicBase::GetSelectedItem()
{
	int iControl;

	if ( ViewByIcon() ) 
	{
		iControl=CONTROL_THUMBS;
	}
	else
		iControl=CONTROL_LIST;

  CGUIMessage msg(GUI_MSG_ITEM_SELECTED,GetID(),iControl,0,0,NULL);
  g_graphicsContext.SendMessage(msg);

  int iItem=msg.GetParam1();
	return iItem;
}

/// \brief Set window to a specific directory
/// \param strDirectory The directory to be displayed in list/thumb control
void CGUIWindowMusicBase::Update(const CStdString &strDirectory)
{
	// get selected item
	int iItem=GetSelectedItem();
	CStdString strSelectedItem="";
	if (iItem >=0 && iItem < (int)m_vecItems.size())
	{
		CFileItem* pItem=m_vecItems[iItem];
		if (pItem->m_bIsFolder && pItem->GetLabel() != "..")
		{
			GetDirectoryHistoryString(pItem, strSelectedItem);
		}
	}

	m_iLastControl=GetFocusedControl();

	ClearFileItems();

	m_history.Set(strSelectedItem,m_strDirectory);
	m_strDirectory=strDirectory;

	GetDirectory(m_strDirectory, m_vecItems);

	RetrieveMusicInfo();

  UpdateListControl();
	UpdateButtons();

	strSelectedItem=m_history.Get(m_strDirectory);

	if (m_iLastControl==CONTROL_THUMBS || m_iLastControl==CONTROL_LIST)
	{
		if (ViewByIcon()) 
		{	
			SET_CONTROL_FOCUS(GetID(), CONTROL_THUMBS, 0);
		}
		else 
		{
			SET_CONTROL_FOCUS(GetID(), CONTROL_LIST, 0);
		}
	}

	int iCurrentPlaylistSong=-1;
	//	Search current playlist item
	CStdString strCurrentDirectory=m_strDirectory;
	if (CUtil::HasSlashAtEnd(strCurrentDirectory))
		strCurrentDirectory.Delete(strCurrentDirectory.size()-1);
	if ((m_nTempPlayListWindow==GetID() && m_strTempPlayListDirectory==strCurrentDirectory && g_application.IsPlayingAudio() 
			&& g_playlistPlayer.GetCurrentPlaylist()==PLAYLIST_MUSIC_TEMP) 
			|| (GetID()==WINDOW_MUSIC_PLAYLIST && g_playlistPlayer.GetCurrentPlaylist()==PLAYLIST_MUSIC && g_application.IsPlayingAudio()) )
	{
		iCurrentPlaylistSong=g_playlistPlayer.GetCurrentSong();
	}

	bool bSelectedFound=false, bCurrentSongFound=false;
	int iSongInDirectory=-1;
	for (int i=0; i < (int)m_vecItems.size(); ++i)
	{
		CFileItem* pItem=m_vecItems[i];

		//	Update selected item
		if (!bSelectedFound)
		{
			CStdString strHistory;
			GetDirectoryHistoryString(pItem, strHistory);
			if (strHistory==strSelectedItem)
			{
				CONTROL_SELECT_ITEM(GetID(), CONTROL_LIST,i);
				CONTROL_SELECT_ITEM(GetID(), CONTROL_THUMBS,i);
				bSelectedFound=true;
			}
		}

		//	synchronize playlist with current directory
		if (!bCurrentSongFound && iCurrentPlaylistSong>-1)
		{
			if (!pItem->m_bIsFolder && !CUtil::IsPlayList(pItem->m_strPath) && !CUtil::IsNFO(pItem->m_strPath))
				iSongInDirectory++;
			if (iSongInDirectory==iCurrentPlaylistSong)
			{
				pItem->Select(true);
				bCurrentSongFound=true;
			}
		}

	}

}

/// \brief Call to go to parent folder
void CGUIWindowMusicBase::GoParentFolder()
{
	if (m_vecItems.size()==0) return;
	CFileItem* pItem=m_vecItems[0];
	if (pItem->m_bIsFolder)
	{
		if (pItem->GetLabel()=="..")
		{
			CStdString strPath=pItem->m_strPath;
			Update(strPath);
		}
	}
}

/// \brief Tests if a network/removeable share is available
/// \param strPath Root share to go into
/// \param iDriveType If share is remote, dvd or hd. See: CShare
/// \return If drive is available, returns \e true 
/// \todo Handle not connected to a remote share
bool CGUIWindowMusicBase::HaveDiscOrConnection( CStdString& strPath, int iDriveType )
{
	if ( iDriveType==SHARE_TYPE_DVD ) 
	{
		CDetectDVDMedia::WaitMediaReady();
		if ( !CDetectDVDMedia::IsDiscInDrive() ) 
		{
			CGUIDialogOK* dlg = (CGUIDialogOK*)m_gWindowManager.GetWindow(WINDOW_DIALOG_OK);
      if (dlg)
      {
			  dlg->SetHeading( 218 );
			  dlg->SetLine( 0, 219 );
			  dlg->SetLine( 1, L"" );
			  dlg->SetLine( 2, L"" );
			  dlg->DoModal( GetID() );
      }
			//	Update listcontrol, maybe share 
			//	was selected while disc change
			int iItem = GetSelectedItem();
			Update( m_strDirectory );
			CONTROL_SELECT_ITEM(GetID(), CONTROL_LIST,iItem)
			CONTROL_SELECT_ITEM(GetID(), CONTROL_THUMBS,iItem)
			return false;
		}
	}
	else if (iDriveType==SHARE_TYPE_REMOTE) 
	{
		// TODO: Handle not connected to a remote share
		if ( !CUtil::IsEthernetConnected() ) 
		{
			CGUIDialogOK* dlg = (CGUIDialogOK*)m_gWindowManager.GetWindow(WINDOW_DIALOG_OK);
      if (dlg)
      {
			  dlg->SetHeading( 220 );
			  dlg->SetLine( 0, 221 );
			  dlg->SetLine( 1, L"" );
			  dlg->SetLine( 2, L"" );
			  dlg->DoModal( GetID() );
      }
			return false;
		}
	}

	return true;
}

/// \brief Retrieves music info for albums from allmusic.com and displays them in CGUIWindowMusicInfo
/// \param iItem Item in list/thumb control
void CGUIWindowMusicBase::OnInfo(int iItem)
{
	int iSelectedItem=GetSelectedItem();
	bool bUpdate=false;
	CGUIDialogOK* pDlgOK = (CGUIDialogOK*)m_gWindowManager.GetWindow(WINDOW_DIALOG_OK);
  CFileItem* pItem;
	pItem=m_vecItems[iItem];

  CStdString strPath;
	if (pItem->m_bIsFolder)
		strPath=pItem->m_strPath;
	else
	{
		CStdString strFileName;
		CUtil::Split(pItem->m_strPath, strPath, strFileName);
	}

	//	Try to find an album name for this item.
	//	Only save to database, if album name is found there.
	bool bSaveDb=false;
	CStdString strLabel=pItem->GetLabel();

	if (pItem->m_bIsFolder)
	{
		//	if a folder has an album set,
		//	we can be sure that it is in database
		CAlbum album;
		if (pItem->m_musicInfoTag.Loaded())
		{
			CStdString strAlbum=pItem->m_musicInfoTag.GetAlbum();
			if (strAlbum.size())
			{
				strLabel=strAlbum;
				bSaveDb=true;
			}
		}
		else if (g_musicDatabase.GetAlbumByPath(pItem->m_strPath, album))
		{	//	Normal folder, query database for album name
			if (album.strAlbum.size())
			{
				strLabel=album.strAlbum;
				bSaveDb=true;
			}
		}
		else
		{
			//	No album name found for folder. Look into
			//	the directory, but don't save to database
			VECFILEITEMS items;
			m_rootDir.GetDirectory(pItem->m_strPath, items);
			OnRetrieveMusicInfo(items);

			//	Get first album name found in directory
			for (int i=0; i<(int)items.size(); i++)
			{
				CFileItem* pItem=items[i];
				if (pItem->m_musicInfoTag.Loaded())
				{
					if (pItem->m_musicInfoTag.GetAlbum().size())
					{
						strLabel=pItem->m_musicInfoTag.GetAlbum();
						break;
					}
				}
			}
		}
	}
	else if (pItem->m_musicInfoTag.Loaded())
	{	//	Handle files
		CAlbum album;
		CStdString strAlbum=pItem->m_musicInfoTag.GetAlbum();
		//	Is album in database?
		if (g_musicDatabase.GetAlbumByPath(strPath, album))
		{
			//	yes, save query results to database
			strLabel=album.strAlbum;
			bSaveDb=true;
		}
		else
			//	no, don't save
			strLabel=strAlbum;
	}


	// check cache
	CAlbum albuminfo;
	if ( g_musicDatabase.GetAlbumInfo(strLabel, strPath, albuminfo) )
	{
		VECSONGS songs;
		g_musicDatabase.GetSongsByAlbum(strLabel, strPath, songs);

		vector<CMusicSong> vecSongs;
		for (int i=0; i<(int)songs.size(); i++)
		{
			CSong& song=songs[i];

			CMusicSong musicSong(song.iTrack, song.strTitle, song.iDuration);
			vecSongs.push_back(musicSong);
		}

		CMusicAlbumInfo album;
		album.Set(albuminfo);
		album.SetSongs(vecSongs);

		CGUIWindowMusicInfo *pDlgAlbumInfo= (CGUIWindowMusicInfo*)m_gWindowManager.GetWindow(WINDOW_MUSIC_INFO);
    if (pDlgAlbumInfo)
    {
			pDlgAlbumInfo->SetAlbum(album);
			pDlgAlbumInfo->DoModal(GetID());
    }
		return;
	}

	// show dialog box indicating we're searching the album
  if (m_dlgProgress)
  {
	  m_dlgProgress->SetHeading(185);
	  m_dlgProgress->SetLine(0,strLabel);
	  m_dlgProgress->SetLine(1,"");
	  m_dlgProgress->SetLine(2,"");
	  m_dlgProgress->StartModal(GetID());
	  m_dlgProgress->Progress();
  }
	bool bDisplayErr=false;
	
	// find album info
	CMusicInfoScraper scraper;
	if (scraper.FindAlbuminfo(strLabel))
	{
		if (m_dlgProgress) m_dlgProgress->Close();
		// did we found at least 1 album?
		int iAlbumCount=scraper.GetAlbumCount();
		if (iAlbumCount >=1)
		{
			//yes
			// if we found more then 1 album, let user choose one
			int iSelectedAlbum=0;
			if (iAlbumCount > 1)
			{
				//show dialog with all albums found
				const WCHAR* szText=g_localizeStrings.Get(181).c_str();
				CGUIDialogSelect *pDlg= (CGUIDialogSelect*)m_gWindowManager.GetWindow(WINDOW_DIALOG_SELECT);
        if (pDlg)
        {
				  pDlg->SetHeading(szText);
				  pDlg->Reset();
				  for (int i=0; i < iAlbumCount; ++i)
				  {
					  CMusicAlbumInfo& info = scraper.GetAlbum(i);
					  pDlg->Add(info.GetTitle2());
				  }
				  pDlg->DoModal(GetID());

				  // and wait till user selects one
				  iSelectedAlbum= pDlg->GetSelectedLabel();
				  if (iSelectedAlbum< 0) return;
        }
			}

			// ok, now show dialog we're downloading the album info
			CMusicAlbumInfo& album = scraper.GetAlbum(iSelectedAlbum);
			if (m_dlgProgress) 
      {
        m_dlgProgress->SetHeading(185);
			  m_dlgProgress->SetLine(0,album.GetTitle2());
			  m_dlgProgress->SetLine(1,"");
			  m_dlgProgress->SetLine(2,"");
			  m_dlgProgress->StartModal(GetID());
			  m_dlgProgress->Progress();
      }

			// download the album info
			bool bLoaded=album.Loaded();
			if (!bLoaded) 
				bLoaded=album.Load();
			if ( bLoaded )
			{
				// set album title from musicinfotag, not the one we got from allmusic.com
				album.SetTitle(strLabel);
				// set path, needed to store album in database
				album.SetAlbumPath(strPath);

				if (bSaveDb)
				{
					CAlbum albuminfo;
					albuminfo.strAlbum  = album.GetTitle();
					albuminfo.strArtist = album.GetArtist();
					albuminfo.strGenre  = album.GetGenre();
					albuminfo.strTones  = album.GetTones();
					albuminfo.strStyles = album.GetStyles();
					albuminfo.strReview = album.GetReview();
					albuminfo.strImage  = album.GetImageURL();
					albuminfo.iRating   = album.GetRating();
					albuminfo.iYear 		= atol( album.GetDateOfRelease().c_str() );
					albuminfo.strPath   = album.GetAlbumPath();
					// save to database
					g_musicDatabase.AddAlbumInfo(albuminfo);
				}
				if (m_dlgProgress) 
					m_dlgProgress->Close();

				// ok, show album info
				CGUIWindowMusicInfo *pDlgAlbumInfo= (CGUIWindowMusicInfo*)m_gWindowManager.GetWindow(WINDOW_MUSIC_INFO);
        if (pDlgAlbumInfo)
        {
				  pDlgAlbumInfo->SetAlbum(album);
				  pDlgAlbumInfo->DoModal(GetID());
        }
				bUpdate=true;
			}
			else
			{
				// failed 2 download album info
				bDisplayErr=true;
			}
		}
		else 
		{
			// no albums found
			bDisplayErr=true;
		}
	}
	else
	{
		// unable 2 connect to www.allmusic.com
		bDisplayErr=true;
	}
	// if an error occured, then notice the user
	if (bDisplayErr)
	{
		if (m_dlgProgress) 
      m_dlgProgress->Close();
    if (pDlgOK)
    {
		  pDlgOK->SetHeading(187);
		  pDlgOK->SetLine(0,L"");
		  pDlgOK->SetLine(1,187);
		  pDlgOK->SetLine(2,L"");
		  pDlgOK->DoModal(GetID());
    }
	}
	if (bUpdate)
	{
		Update(m_strDirectory);
		CONTROL_SELECT_ITEM(GetID(), CONTROL_LIST,iSelectedItem);
		CONTROL_SELECT_ITEM(GetID(), CONTROL_THUMBS,iSelectedItem);
	}
}

/// \brief Can be overwritten to implement an own tag filling function.
/// \param items File items to fill
void CGUIWindowMusicBase::OnRetrieveMusicInfo(VECFILEITEMS& items)
{

}

/// \brief Retrieve tag information for \e m_vecItems
void CGUIWindowMusicBase::RetrieveMusicInfo()
{
	DWORD dwTick=timeGetTime();

	OnRetrieveMusicInfo(m_vecItems);

	dwTick = timeGetTime() - dwTick;
	CStdString strTmp;
	strTmp.Format("RetrieveMusicInfo() took %imsec\n",dwTick); 
	OutputDebugString(strTmp.c_str());
}

/// \brief Add selected list/thumb control item to playlist and start playing
/// \param iItem Selected Item in list/thumb control
void CGUIWindowMusicBase::OnQueueItem(int iItem)
{
	// add item 2 playlist
	const CFileItem* pItem=m_vecItems[iItem];
	AddItemToPlayList(pItem);
	
	//move to next item
	CONTROL_SELECT_ITEM(GetID(), CONTROL_LIST,iItem+1);
	CONTROL_SELECT_ITEM(GetID(), CONTROL_THUMBS,iItem+1);
	if (g_playlistPlayer.GetPlaylist(PLAYLIST_MUSIC).size() && !g_application.IsPlayingAudio() )
	{
		g_playlistPlayer.Reset();
		g_playlistPlayer.SetCurrentPlaylist(PLAYLIST_MUSIC);
		g_playlistPlayer.Play(0);
	}
}

/// \brief Add file or folder and its subfolders to playlist
/// \param pItem The file item to add 
void CGUIWindowMusicBase::AddItemToPlayList(const CFileItem* pItem) 
{
	if (pItem->m_bIsFolder)
	{
		// recursive
		if (pItem->GetLabel() == "..") return;
		CStdString strDirectory=m_strDirectory;
		m_strDirectory=pItem->m_strPath;
		VECFILEITEMS items;
		CFileItemList itemlist(items);
		GetDirectory(m_strDirectory, items);
		DoSort(items);
		for (int i=0; i < (int) items.size(); ++i)
		{
			AddItemToPlayList(items[i]);
		}
		m_strDirectory=strDirectory;
	}
	else
	{
    if (!CUtil::IsNFO(pItem->m_strPath) && CUtil::IsAudio(pItem->m_strPath) && !CUtil::IsPlayList(pItem->m_strPath))
		{
			CPlayList::CPlayListItem playlistItem ;
			playlistItem.SetFileName(pItem->m_strPath);
			playlistItem.SetDescription(pItem->GetLabel());
			playlistItem.SetDuration(pItem->m_musicInfoTag.GetDuration());
			g_playlistPlayer.GetPlaylist( PLAYLIST_MUSIC ).Add(playlistItem);
		}
	}
}

/// \brief Make the actual search for the OnSearch function.
/// \param strSearch The search string 
/// \param items Items Found
void CGUIWindowMusicBase::DoSearch(const CStdString& strSearch,VECFILEITEMS& items)
{

}

/// \brief Search the current directory for a string got from the virtual keyboard
void CGUIWindowMusicBase::OnSearch()
{
	CStdString strSearch;
	if ( !GetKeyboard(strSearch) )
		return;

	strSearch.ToLower();
	if (m_dlgProgress)
  {
    m_dlgProgress->SetHeading(194);
	  m_dlgProgress->SetLine(0,strSearch);
	  m_dlgProgress->SetLine(1,L"");
	  m_dlgProgress->SetLine(2,L"");
	  m_dlgProgress->StartModal(GetID());
	  m_dlgProgress->Progress();
  }
	VECFILEITEMS items;
	DoSearch(strSearch, items);

	if (items.size())
	{
		CGUIDialogSelect* pDlgSelect=(CGUIDialogSelect*)m_gWindowManager.GetWindow(WINDOW_DIALOG_SELECT);
		pDlgSelect->Reset();
		pDlgSelect->SetHeading(283);
		CUtil::SortFileItemsByName(items);

		for (int i=0; i<(int)items.size(); i++)
		{
			CFileItem* pItem=items[i];
			pDlgSelect->Add(pItem->GetLabel());
		}

		pDlgSelect->DoModal(GetID());

		int iItem=pDlgSelect->GetSelectedLabel();
		if (iItem < 0)
		{
			if (m_dlgProgress) m_dlgProgress->Close();
			return;
		}

		CFileItem* pSelItem=new CFileItem(*items[iItem]);

		{
			CFileItemList itemlist(items);	//	will cleanup everything
		}

		OnSearchItemFound(pSelItem);

		delete pSelItem;
		if (m_dlgProgress) m_dlgProgress->Close();
	}
	else
	{
		if (m_dlgProgress) m_dlgProgress->Close();

		CGUIDialogOK* dlg = (CGUIDialogOK*)m_gWindowManager.GetWindow(WINDOW_DIALOG_OK);
    if (dlg)
    {
		  dlg->SetHeading( 194 );
		  dlg->SetLine( 0, 284 );
		  dlg->SetLine( 1, L"" );
		  dlg->SetLine( 2, L"" );
		  dlg->DoModal( GetID() );
    }
	}
}

/// \brief Display virtual keyboard
///	\param strInput Set as defaultstring in keyboard and retrieves the input from keyboard
bool CGUIWindowMusicBase::GetKeyboard(CStdString& strInput)
{
	CXBVirtualKeyboard* pKeyboard = (CXBVirtualKeyboard*)m_gWindowManager.GetWindow(WINDOW_VIRTUAL_KEYBOARD);
  if (!pKeyboard) return false;
	pKeyboard->Reset();
	WCHAR wsString[1024];
  swprintf( wsString,L"%S", strInput.c_str() );
	pKeyboard->SetText(wsString);
	pKeyboard->DoModal(m_gWindowManager.GetActiveWindow());
	if (pKeyboard->IsConfirmed())
	{
		const WCHAR* pSearchString=pKeyboard->GetText();
		CUtil::Unicode2Ansi(pSearchString,strInput);
		if (strInput.IsEmpty())
			return false;

		return true;
	}

	return false;
}

/// \brief Is thumb or list control visible
/// \return Returns \e true, if thumb control is visible
bool CGUIWindowMusicBase::ViewByIcon()
{
  if ( m_strDirectory.IsEmpty() )
  {
    if (m_iViewAsIconsRoot != VIEW_AS_LIST) return true;
  }
  else
  {
    if (m_iViewAsIcons != VIEW_AS_LIST) return true;
  }
  return false;
}

/// \brief Is thumb control in large icons mode
/// \return Returns \e true, if thumb control is in large icons mode
bool CGUIWindowMusicBase::ViewByLargeIcon()
{
  if ( m_strDirectory.IsEmpty() )
  {
    if (m_iViewAsIconsRoot == VIEW_AS_LARGEICONS) return true;
  }
  else
  {
    if (m_iViewAsIcons== VIEW_AS_LARGEICONS) return true;
  }
  return false;
}

/// \brief Switch thumb control between large and normal icons
void CGUIWindowMusicBase::ShowThumbPanel()
{
  int iItem=GetSelectedItem(); 
  if ( ViewByLargeIcon() )
  {
    CGUIThumbnailPanel* pControl=(CGUIThumbnailPanel*)GetControl(CONTROL_THUMBS);
		if (pControl)
		{
      pControl->ShowBigIcons(true);
		}
  }
  else
  {
    CGUIThumbnailPanel* pControl=(CGUIThumbnailPanel*)GetControl(CONTROL_THUMBS);
		if (pControl)
		{
      pControl->ShowBigIcons(false);
		}
  }
  if (iItem>-1)
  {
    CONTROL_SELECT_ITEM(GetID(), CONTROL_LIST,iItem);
    CONTROL_SELECT_ITEM(GetID(), CONTROL_THUMBS,iItem);
  }
}

/// \brief Can be overwritten to build an own history string for \c m_history
/// \param pItem Item to build the history string from
/// \param strHistoryString History string build as return value
void CGUIWindowMusicBase::GetDirectoryHistoryString(const CFileItem* pItem, CStdString& strHistoryString)
{
	strHistoryString=pItem->m_strPath;

	if (CUtil::HasSlashAtEnd(strHistoryString))
		strHistoryString.Delete(strHistoryString.size()-1);
}

void CGUIWindowMusicBase::UpdateButtons()
{
	//	Update window selection control

	//	Remove labels from the window selection
	CGUIMessage msg(GUI_MSG_LABEL_RESET,GetID(),CONTROL_BTNTYPE);
	g_graphicsContext.SendMessage(msg);

	//	Add labels to the window selection
	CStdString strItem=g_localizeStrings.Get(134);	//	Songs
	CGUIMessage msg2(GUI_MSG_LABEL_ADD,GetID(),CONTROL_BTNTYPE);
	msg2.SetLabel(strItem);
	g_graphicsContext.SendMessage(msg2);

	strItem=g_localizeStrings.Get(132);	//	Album
	msg2.SetLabel(strItem);
	g_graphicsContext.SendMessage(msg2);

	strItem=g_localizeStrings.Get(133);	//	Artist
	msg2.SetLabel(strItem);
	g_graphicsContext.SendMessage(msg2);

	strItem=g_localizeStrings.Get(135);	//	Genre
	msg2.SetLabel(strItem);
	g_graphicsContext.SendMessage(msg2);

	strItem=g_localizeStrings.Get(271);	//	Top 100
	msg2.SetLabel(strItem);
	g_graphicsContext.SendMessage(msg2);

	//	Select the current window as default item
	CONTROL_SELECT_ITEM(GetID(), CONTROL_BTNTYPE, g_stSettings.m_iMyMusicStartWindow-WINDOW_MUSIC_FILES);
}

///	\brief React on the selected search item
///	\param pItem Search result item
void CGUIWindowMusicBase::OnSearchItemFound(const CFileItem* pItem)
{

}
