
#include "stdafx.h"
#include "GUIWindowMusicSongs.h"
#include "settings.h"
#include "guiWindowManager.h"
#include "localizestrings.h"
#include "PlayListFactory.h"
#include "util.h"
#include "url.h"
#include "PlayListM3U.h"
#include "application.h"
#include "playlistplayer.h"
#include <algorithm>
#include "GuiUserMessages.h"
#include "SectionLoader.h"
#include "cdrip/cddaripper.h"
#include "cuedocument.h"
#include "AutoSwitch.h"

#define CONTROL_BTNVIEWASICONS		2
#define CONTROL_BTNSORTBY					3
#define CONTROL_BTNSORTASC				4

#define CONTROL_BTNTYPE						6
#define CONTROL_BTNPLAYLISTS			7
#define CONTROL_BTNSCAN						9
#define CONTROL_BTNREC						10
#define CONTROL_BTNRIP						11

#define CONTROL_LABELFILES        12

#define CONTROL_LIST							50
#define CONTROL_THUMBS						51

struct SSortMusicSongs
{
	bool operator()(CFileItem* pStart, CFileItem* pEnd)
	{
    CFileItem& rpStart=*pStart;
    CFileItem& rpEnd=*pEnd;
		if (rpStart.GetLabel()=="..") return true;
		if (rpEnd.GetLabel()=="..") return false;
		bool bGreater=true;
		if (m_bSortAscending) bGreater=false;

	   if (rpStart.m_bIsFolder == rpEnd.m_bIsFolder)
		{
			char szfilename1[1024];
			char szfilename2[1024];

			switch ( m_iSortMethod ) 
			{
				case 0:	//	Sort by Listlabel
					strcpy(szfilename1, rpStart.GetLabel().c_str());
					strcpy(szfilename2, rpEnd.GetLabel().c_str());
				break;

				case 1: // Sort by Date
          if ( rpStart.m_stTime.wYear > rpEnd.m_stTime.wYear ) return bGreater;
					if ( rpStart.m_stTime.wYear < rpEnd.m_stTime.wYear ) return !bGreater;
					
					if ( rpStart.m_stTime.wMonth > rpEnd.m_stTime.wMonth ) return bGreater;
					if ( rpStart.m_stTime.wMonth < rpEnd.m_stTime.wMonth ) return !bGreater;
					
					if ( rpStart.m_stTime.wDay > rpEnd.m_stTime.wDay ) return bGreater;
					if ( rpStart.m_stTime.wDay < rpEnd.m_stTime.wDay ) return !bGreater;

					if ( rpStart.m_stTime.wHour > rpEnd.m_stTime.wHour ) return bGreater;
					if ( rpStart.m_stTime.wHour < rpEnd.m_stTime.wHour ) return !bGreater;

					if ( rpStart.m_stTime.wMinute > rpEnd.m_stTime.wMinute ) return bGreater;
					if ( rpStart.m_stTime.wMinute < rpEnd.m_stTime.wMinute ) return !bGreater;

					if ( rpStart.m_stTime.wSecond > rpEnd.m_stTime.wSecond ) return bGreater;
					if ( rpStart.m_stTime.wSecond < rpEnd.m_stTime.wSecond ) return !bGreater;
					return true;
				break;

        case 2:	//	Sort by Size
          if ( rpStart.m_dwSize > rpEnd.m_dwSize) return bGreater;
					if ( rpStart.m_dwSize < rpEnd.m_dwSize) return !bGreater;
					return true;
        break;

        case 3:	//	Sort by TrackNum
					if ( rpStart.m_musicInfoTag.GetTrackNumber() > rpEnd.m_musicInfoTag.GetTrackNumber()) return bGreater;
					if ( rpStart.m_musicInfoTag.GetTrackNumber() < rpEnd.m_musicInfoTag.GetTrackNumber()) return !bGreater;
					return true;
        break;

        case 4:	//	Sort by Duration
					if ( rpStart.m_musicInfoTag.GetDuration() > rpEnd.m_musicInfoTag.GetDuration()) return bGreater;
					if ( rpStart.m_musicInfoTag.GetDuration() < rpEnd.m_musicInfoTag.GetDuration()) return !bGreater;
					return true;
        break;

        case 5:	//	Sort by Title
 					strcpy(szfilename1, rpStart.m_musicInfoTag.GetTitle());
					strcpy(szfilename2, rpEnd.m_musicInfoTag.GetTitle());
        break;

				case 6:	//	Sort by Artist
 					strcpy(szfilename1, rpStart.m_musicInfoTag.GetArtist());
					strcpy(szfilename2, rpEnd.m_musicInfoTag.GetArtist());
				break;

        case 7:	//	Sort by Album
 					strcpy(szfilename1, rpStart.m_musicInfoTag.GetAlbum());
					strcpy(szfilename2, rpEnd.m_musicInfoTag.GetAlbum());
        break;

        case 8:	//	Sort by FileName
 					strcpy(szfilename1, rpStart.m_strPath);
					strcpy(szfilename2, rpEnd.m_strPath);
        break;

        case 9:	//	Sort by share type
					if ( rpStart.m_iDriveType > rpEnd.m_iDriveType) return bGreater;
					if ( rpStart.m_iDriveType < rpEnd.m_iDriveType) return !bGreater;
 					strcpy(szfilename1, rpStart.GetLabel());
					strcpy(szfilename2, rpEnd.GetLabel());
        break;

				default:	//	Sort by Filename by default
					strcpy(szfilename1, rpStart.GetLabel().c_str());
					strcpy(szfilename2, rpEnd.GetLabel().c_str());
				break;
			}


			for (int i=0; i < (int)strlen(szfilename1); i++)
				szfilename1[i]=tolower((unsigned char)szfilename1[i]);
			
			for (int i=0; i < (int)strlen(szfilename2); i++)
			{
				szfilename2[i]=tolower((unsigned char)szfilename2[i]);
			}
			//return (rpStart.strPath.compare( rpEnd.strPath )<0);

			if (m_bSortAscending)
				return (strcmp(szfilename1,szfilename2)<0);
			else
				return (strcmp(szfilename1,szfilename2)>=0);
		}
    if (!rpStart.m_bIsFolder) return false;
		return true;
	}

	int m_iSortMethod;
	int m_bSortAscending;
	CStdString m_strDirectory;
};

CGUIWindowMusicSongs::CGUIWindowMusicSongs(void)
:CGUIWindowMusicBase()
{
	m_strDirectory="?";
	m_bScan=false;
	m_iViewAsIcons=-1;
	m_iViewAsIconsRoot=-1;
}

CGUIWindowMusicSongs::~CGUIWindowMusicSongs(void)
{

}

bool CGUIWindowMusicSongs::OnMessage(CGUIMessage& message)
{
  switch ( message.GetMessage() )
  {
    case GUI_MSG_WINDOW_INIT:
		{
			//	This window is started by the home window.
			//	Now we decide which my music window has to be shown and
			//	switch to the my music window the user last activated.
			if (g_stSettings.m_iMyMusicStartWindow!=GetID())
			{
				m_gWindowManager.ActivateWindow(g_stSettings.m_iMyMusicStartWindow);
				return false;
			}

			bool bFirstTime=false;
			if (m_strDirectory=="?")
			{
				bFirstTime=true;
				m_strDirectory=g_stSettings.m_szDefaultMusic;
			}

			if (CUtil::IsCDDA(m_strDirectory) || CUtil::IsDVD(m_strDirectory) || CUtil::IsISO9660(m_strDirectory))
			{
				//	No disc in drive but current directory is a dvd share
				if (!CDetectDVDMedia::IsDiscInDrive())
					m_strDirectory.Empty();

				//	look if disc has changed outside this window and url is still the same
				CStdString strDVDUrl=m_rootDir.GetDVDDriveUrl();
				if (CUtil::IsCDDA(m_strDirectory) && !CUtil::IsCDDA(strDVDUrl))
					m_strDirectory.Empty();
				if (CUtil::IsDVD(m_strDirectory) && !CUtil::IsDVD(strDVDUrl))
					m_strDirectory.Empty();
				if (CUtil::IsISO9660(m_strDirectory) && !CUtil::IsISO9660(strDVDUrl))
					m_strDirectory.Empty();
			}

			if (m_iViewAsIcons==-1 && m_iViewAsIconsRoot==-1)
			{
				m_iViewAsIcons=g_stSettings.m_iMyMusicSongsViewAsIcons;
				m_iViewAsIconsRoot=g_stSettings.m_iMyMusicSongsRootViewAsIcons;
			}

			CGUIWindowMusicBase::OnMessage(message);

			if (bFirstTime)
			{
				//	Set directory history for default path
				SetHistoryForPath(m_strDirectory);
				bFirstTime=false;
			}

			return true;
		}
		break;

		case GUI_MSG_DIRECTORY_SCANNED:
		{
			const CStdString& strDirectory=message.GetStringParam();

			//	Only update thumb on a local drive
			if (CUtil::IsHD(strDirectory))
			{
				CStdString strParent;
				CUtil::GetParentPath(strDirectory, strParent);
				if (strDirectory==m_strDirectory || strParent==m_strDirectory)
				{
					OutputDebugString("Update()\n");
					Update(m_strDirectory);
				}
			}
		}
		break;

		case GUI_MSG_SCAN_FINISHED:
		{
			UpdateButtons();
		}
		break;

		case GUI_MSG_CLICKED:
    {
      int iControl=message.GetSenderId();

 			if (iControl==CONTROL_BTNSORTBY) // sort by
      {
				if (m_strDirectory.IsEmpty())
				{
					if (g_stSettings.m_iMyMusicSongsRootSortMethod==0)
						g_stSettings.m_iMyMusicSongsRootSortMethod=9;
					else
						g_stSettings.m_iMyMusicSongsRootSortMethod=0;
				}
				else
				{
					g_stSettings.m_iMyMusicSongsSortMethod++;
					if (g_stSettings.m_iMyMusicSongsSortMethod >=9) g_stSettings.m_iMyMusicSongsSortMethod=0;
					if (g_stSettings.m_iMyMusicSongsSortMethod >=3) g_stSettings.m_iMyMusicSongsSortMethod=8;
				}
				g_settings.Save();

				int nItem=GetSelectedItem();
				CFileItem*pItem=m_vecItems[nItem];
				CStdString strSelected=pItem->m_strPath;
        
        UpdateButtons();
        UpdateListControl();

				CStdString strDirectory=m_strDirectory;
				if (CUtil::HasSlashAtEnd(strDirectory))
					strDirectory.Delete(strDirectory.size()-1);
				if (!m_strDirectory.IsEmpty() && m_nTempPlayListWindow==GetID() && m_strTempPlayListDirectory==strDirectory && g_application.IsPlayingAudio() && g_playlistPlayer.GetCurrentPlaylist()==PLAYLIST_MUSIC_TEMP)
				{
					int nSong=g_playlistPlayer.GetCurrentSong();
					const CPlayList::CPlayListItem item=g_playlistPlayer.GetPlaylist(PLAYLIST_MUSIC_TEMP)[nSong];
					g_playlistPlayer.GetPlaylist(PLAYLIST_MUSIC_TEMP).Clear();
					g_playlistPlayer.Reset();
					int nFolderCount=0;
					for (int i = 0; i < (int)m_vecItems.size(); i++) 
					{
						CFileItem* pItem = m_vecItems[i];
						if (pItem->m_bIsFolder) 
						{
							nFolderCount++;
							continue;
						}
						CPlayList::CPlayListItem playlistItem ;
						CUtil::ConvertFileItemToPlayListItem(pItem, playlistItem);
						g_playlistPlayer.GetPlaylist(PLAYLIST_MUSIC_TEMP).Add(playlistItem);
						if (item.GetFileName()==pItem->m_strPath)
							g_playlistPlayer.SetCurrentSong(i-nFolderCount);
					}
				}

				for (int i=0; i<(int)m_vecItems.size(); i++)
				{
					CFileItem* pItem = m_vecItems[i];
					if (pItem->m_strPath==strSelected)
					{
						CONTROL_SELECT_ITEM(CONTROL_LIST,i);
						CONTROL_SELECT_ITEM(CONTROL_THUMBS,i);
						break;
					}
				}
			}
			else if (iControl==CONTROL_BTNVIEWASICONS)
      {
				CGUIWindowMusicBase::OnMessage(message);
				g_stSettings.m_iMyMusicSongsRootViewAsIcons=m_iViewAsIconsRoot;
				g_stSettings.m_iMyMusicSongsViewAsIcons=m_iViewAsIcons;
				g_settings.Save();
				return true;
			}
      else if (iControl==CONTROL_BTNSORTASC) // sort asc
      {
				if (m_strDirectory.IsEmpty())
	        g_stSettings.m_bMyMusicSongsRootSortAscending=!g_stSettings.m_bMyMusicSongsRootSortAscending;
				else
	        g_stSettings.m_bMyMusicSongsSortAscending=!g_stSettings.m_bMyMusicSongsSortAscending;

				g_settings.Save();
        UpdateButtons();
        UpdateListControl();
      }
			else if (iControl==CONTROL_BTNPLAYLISTS)
			{
				CStdString strDirectory;
				strDirectory.Format("%s\\playlists",g_stSettings.m_szAlbumDirectory);
				if (strDirectory!=m_strDirectory)
				{
					Update(strDirectory);
				}
			}
 			else if (iControl==CONTROL_BTNSCAN)
			{
				OnScan();
			}
			else if (iControl==CONTROL_BTNREC)
			{
				if (g_application.IsPlayingAudio() )
				{
					if (g_application.m_pPlayer->CanRecord() )
					{
						bool bIsRecording=g_application.m_pPlayer->IsRecording();
						g_application.m_pPlayer->Record(!bIsRecording);
						UpdateButtons();
					}
				}
			}
			else if (iControl == CONTROL_BTNRIP)
			{
				CCdInfo *pCdInfo = CDetectDVDMedia::GetCdInfo();
				if (CDetectDVDMedia::IsDiscInDrive() && pCdInfo && pCdInfo->IsAudio(1))
				{
					if (!CUtil::IsCDDA(g_application.CurrentFile()))
					{
						CCDDARipper ripper;
						ripper.RipCD();
					}
					else
					{
						CGUIDialogOK*	pDlgOK = (CGUIDialogOK*)m_gWindowManager.GetWindow(WINDOW_DIALOG_OK);
						pDlgOK->SetHeading(257); // Error
						pDlgOK->SetLine(0, "Can't rip CD or Track while playing from CD"); // 
						pDlgOK->SetLine(1, ""); // 
						pDlgOK->SetLine(2, "");
						pDlgOK->DoModal(GetID());
					}
				}
			}
		}
		break;
	}

	return CGUIWindowMusicBase::OnMessage(message);
}

void CGUIWindowMusicSongs::GetDirectory(const CStdString &strDirectory, VECFILEITEMS &items)
{
	if (items.size() )
	{
		// cleanup items
		CFileItemList itemlist(items);
	}

	CStdString strParentPath;
	bool bParentExists=CUtil::GetParentPath(strDirectory, strParentPath);

	CStdString strPlayListDir;
	strPlayListDir.Format("%s\\playlists",g_stSettings.m_szAlbumDirectory);
	if (strPlayListDir==strDirectory) 
	{
		bParentExists=true;
		strParentPath=m_strPrevDir;
	}

	// check if current directory is a root share
	if ( !m_rootDir.IsShare(strDirectory) )
	{
		// no, do we got a parent dir?
		if ( bParentExists )
		{
			// yes
			if (!g_guiSettings.GetBool("MusicLists.HideParentDirItems"))
			{
				CFileItem *pItem = new CFileItem("..");
				pItem->m_strPath=strParentPath;
				pItem->m_bIsFolder=true;
				pItem->m_bIsShareOrDrive=false;
				items.push_back(pItem);
			}
			m_strParentPath = strParentPath;
		}
	}
	else
	{
		// yes, this is the root of a share
		// add parent path to the virtual directory
		if (!g_guiSettings.GetBool("MusicLists.HideParentDirItems"))
		{
			CFileItem *pItem = new CFileItem("..");
			pItem->m_strPath="";
			pItem->m_bIsShareOrDrive=false;
			pItem->m_bIsFolder=true;
			items.push_back(pItem);
		}
		m_strParentPath = "";
	}
	m_rootDir.GetDirectory(strDirectory,items);

	// check for .CUE files here.
	FilterItems(items);

	if (strPlayListDir!=strDirectory) 
	{
		m_strPrevDir=strDirectory;
	}

}

void CGUIWindowMusicSongs::OnScan()
{
	if (g_guiSettings.GetBool("MusicLibrary.UseBackgroundScanner"))
	{
		if (g_application.m_guiDialogMusicScan.IsRunning())
		{
			g_application.m_guiDialogMusicScan.StopScanning();
			UpdateButtons();
			return;
		}
		// check whether we have scanned here before
		bool bUpdateAll = false;
		CStdString strPaths;
		strPaths = g_musicDatabase.GetSubpathsFromPath(m_strDirectory);
		if (strPaths.length() > 2)
		{	// yes, we have, we should prompt the user to ask if they want
			// to do a full scan, or just add new items...
			CGUIDialogYesNo *pDialog = &(g_application.m_guiDialogYesNo);
			pDialog->SetHeading(189);
			pDialog->SetLine(0,702);
			pDialog->SetLine(1,703);
			pDialog->SetLine(2,704);
			pDialog->DoModal(GetID());
			if (pDialog->IsConfirmed())	bUpdateAll = true;
		}

		g_application.m_guiDialogMusicScan.StartScanning(m_strDirectory, bUpdateAll);
		UpdateButtons();
		return;
	}
	else
	{
		// remove username + password from m_strDirectory for display in Dialog
		CURL url(m_strDirectory);
		CStdString strStrippedPath;
		url.GetURLWithoutUserDetails(strStrippedPath);

		DWORD dwTick=timeGetTime();

		// check whether we have scanned here before
		bool m_bUpdateAll = false;
		CStdString strPaths;
		strPaths = g_musicDatabase.GetSubpathsFromPath(m_strDirectory);
		if (strPaths.length() > 2)
		{	// yes, we have, we should prompt the user to ask if they want
			// to do a full scan, or just add new items...
			CGUIDialogYesNo *pDialog = &(g_application.m_guiDialogYesNo);
			pDialog->SetHeading(189);
			pDialog->SetLine(0,702);
			pDialog->SetLine(1,703);
			pDialog->SetLine(2,704);
			pDialog->DoModal(GetID());
			if (pDialog->IsConfirmed())	m_bUpdateAll = true;
		}

		m_dlgProgress->SetHeading(189);
		m_dlgProgress->SetLine(0, 330);
		m_dlgProgress->SetLine(1,"");
		m_dlgProgress->SetLine(2,strStrippedPath );
		m_dlgProgress->StartModal(GetID());
		m_dlgProgress->Progress();

		// Preload section for ID3 cover art reading
		CSectionLoader::Load("CXIMAGE");
		CSectionLoader::Load("LIBMP4");

		CUtil::ThumbCacheClear();

		bool bOverlayAllowed=g_graphicsContext.IsOverlayAllowed();

		if (bOverlayAllowed)
			g_graphicsContext.SetOverlay(false);

		g_musicDatabase.BeginTransaction();

		bool bOKtoScan = true;
		if (m_bUpdateAll)
		{
			m_dlgProgress->SetLine(2,701);
			m_dlgProgress->Progress();
			bOKtoScan = g_musicDatabase.RemoveSongsFromPaths(strPaths);
		}
		// enable scan mode in OnRetrieveMusicInfo()
		m_bScan=true;

		if (bOKtoScan)
		{
			if (m_bUpdateAll)
			{
				m_dlgProgress->SetLine(2,700);
				m_dlgProgress->Progress();
				bOKtoScan = g_musicDatabase.CleanupAlbumsArtistsGenres(strPaths);
			}

			bool bCommit = false;
			if (bOKtoScan)
				bCommit = DoScan(m_vecItems);

			if (bCommit)
			{
				g_musicDatabase.CommitTransaction();
				if (m_bUpdateAll)
				{
					m_dlgProgress->SetLine(2,331);
					m_dlgProgress->Progress();
					g_musicDatabase.Compress();
				}
			}
			else
				g_musicDatabase.RollbackTransaction();
			m_dlgProgress->SetLine(0,328);
			m_dlgProgress->SetLine(1,"");
			m_dlgProgress->SetLine(2,330 );
			m_dlgProgress->Progress();
		}
		else
			g_musicDatabase.RollbackTransaction();

		g_musicDatabase.EmptyCache();

		CSectionLoader::Unload("CXIMAGE");
		CSectionLoader::Unload("LIBMP4");

		// disable scan mode
		m_bScan=false;

		CUtil::ThumbCacheClear();

		if (bOverlayAllowed)
			g_graphicsContext.SetOverlay(true);

		m_dlgProgress->Close();

		int iItem=GetSelectedItem();
		Update(m_strDirectory);
		CONTROL_SELECT_ITEM(CONTROL_LIST, iItem);
		CONTROL_SELECT_ITEM(CONTROL_THUMBS, iItem);

		dwTick = timeGetTime() - dwTick;
		CStdString strTmp, strTmp1;
		CUtil::SecondsToHMSString(dwTick/1000, strTmp1);
		strTmp.Format("My Music: Scanning for music info without worker thread, operation took %s", strTmp1); 
		CLog::Log(LOGNOTICE,strTmp.c_str());
	}
}

bool CGUIWindowMusicSongs::DoScan(VECFILEITEMS& items)
{
	// remove username + password from m_strDirectory for display in Dialog
	CURL url(m_strDirectory);
	CStdString strStrippedPath;
	url.GetURLWithoutUserDetails(strStrippedPath);

	m_dlgProgress->SetLine(2,strStrippedPath );
	m_dlgProgress->Progress();

	OnRetrieveMusicInfo(items);
	g_musicDatabase.CheckVariousArtistsAndCoverArt();
	
	if (m_dlgProgress->IsCanceled()) return false;
	
	bool bCancel=false;
	for (int i=0; i < (int)items.size(); ++i)
	{
    g_application.ResetScreenSaver();
		CFileItem *pItem= items[i];
		if (m_dlgProgress->IsCanceled())
		{
			bCancel=true;
			break;
		}
		if ( pItem->m_bIsFolder)
		{
			if (pItem->GetLabel() != "..")
			{
				// load subfolder
				CStdString strDir=m_strDirectory;
				m_strDirectory=pItem->m_strPath;
				VECFILEITEMS subDirItems;
				CFileItemList itemlist(subDirItems);
				m_rootDir.GetDirectory(pItem->m_strPath,subDirItems);
				// filter items in the sub dir (for .cue sheet support)
				FilterItems(subDirItems);

				if (!DoScan(subDirItems))
				{
					bCancel=true;
				}
				
				m_strDirectory=strDir;
				if (bCancel) break;
			}
		}
	}
	
	return !bCancel;
}

void CGUIWindowMusicSongs::LoadPlayList(const CStdString& strPlayList)
{
  // load a playlist like .m3u, .pls
  // first get correct factory to load playlist
	CPlayListFactory factory;
	auto_ptr<CPlayList> pPlayList (factory.Create(strPlayList));
	if ( NULL != pPlayList.get())
	{
    // load it
		if (!pPlayList->Load(strPlayList))
		{
			CGUIDialogOK* pDlgOK = (CGUIDialogOK*)m_gWindowManager.GetWindow(WINDOW_DIALOG_OK);
			if (pDlgOK)
			{
				pDlgOK->SetHeading(6);
				pDlgOK->SetLine(0,L"");
				pDlgOK->SetLine(1,477);
				pDlgOK->SetLine(2,L"");
				pDlgOK->DoModal(GetID());
			}
			return; //hmmm unable to load playlist?
		}

    // how many songs are in the new playlist
    if (pPlayList->size() == 1)
    {
      // just 1 song? then play it (no need to have a playlist of 1 song)
      CPlayList::CPlayListItem item=(*pPlayList)[0];
      g_application.PlayFile(CFileItem(item));
      return;
    }

    // clear current playlist
		g_playlistPlayer.GetPlaylist(PLAYLIST_MUSIC).Clear();

		//	if autoshuffle playlist on load option is enabled
    //  then shuffle the playlist
    // (dont do this for shoutcast .pls files)
		CStdString strFileName;
		if ((*pPlayList).size())
			strFileName=(*pPlayList)[0].GetFileName();
		if (!CUtil::IsShoutCast(strFileName) && g_guiSettings.GetBool("MusicLibrary.ShufflePlaylistsOnLoad"))
			pPlayList->Shuffle();

    // add each item of the playlist to the playlistplayer
		for (int i=0; i < (int)pPlayList->size(); ++i)
		{
			const CPlayList::CPlayListItem& playListItem =(*pPlayList)[i];
			CStdString strLabel=playListItem.GetDescription();
			if (strLabel.size()==0) 
				strLabel=CUtil::GetTitleFromPath(playListItem.GetFileName());

			CPlayList::CPlayListItem playlistItem;
			playlistItem.SetDescription(playListItem.GetDescription());
			playlistItem.SetDuration(playListItem.GetDuration());
			playlistItem.SetFileName(playListItem.GetFileName());
			g_playlistPlayer.GetPlaylist( PLAYLIST_MUSIC ).Add(playlistItem);
		}
	} 

  // if we got a playlist
	if (g_playlistPlayer.GetPlaylist( PLAYLIST_MUSIC ).size() )
	{
    // then get 1st song
		CPlayList& playlist=g_playlistPlayer.GetPlaylist( PLAYLIST_MUSIC );
		const CPlayList::CPlayListItem& item=playlist[0];

    // and start playing it
		g_playlistPlayer.SetCurrentPlaylist(PLAYLIST_MUSIC);
		g_playlistPlayer.Reset();
		g_playlistPlayer.Play(0);

    // and activate the playlist window if its not activated yet
    if (GetID() == m_gWindowManager.GetActiveWindow())
    {
      m_gWindowManager.ActivateWindow(WINDOW_MUSIC_PLAYLIST);
    }
	}
}

void CGUIWindowMusicSongs::UpdateButtons()
{
	CGUIWindowMusicBase::UpdateButtons();

	bool bIsPlaying=g_application.IsPlayingAudio();
	bool bCanRecord=false;
	bool bIsRecording=false;

	if (bIsPlaying)
	{
		bCanRecord=g_application.m_pPlayer->CanRecord();
		bIsRecording=g_application.m_pPlayer->IsRecording();
	}

	//	Update Record button
	if (bIsPlaying && bCanRecord)
	{
		CONTROL_ENABLE(CONTROL_BTNREC);
		if (bIsRecording)
		{
				SET_CONTROL_LABEL(CONTROL_BTNREC,265);//Stop Recording
		}
		else
		{
				SET_CONTROL_LABEL(CONTROL_BTNREC,264);//Record
		}
	}
	else
	{
		SET_CONTROL_LABEL(CONTROL_BTNREC,264);//Record
		CONTROL_DISABLE(CONTROL_BTNREC);
	}

	// Update CDDA Rip button
	CCdInfo *pCdInfo = CDetectDVDMedia::GetCdInfo();
	if (CDetectDVDMedia::IsDiscInDrive() && pCdInfo && pCdInfo->IsAudio(1))
	{
		CONTROL_ENABLE(CONTROL_BTNRIP);
	}
	else
	{
		CONTROL_DISABLE(CONTROL_BTNRIP);
	}

	// Disable scan button if shoutcast
	if (m_strDirectory.IsEmpty() || CUtil::IsShoutCast(m_strDirectory))
	{
		CONTROL_DISABLE(CONTROL_BTNSCAN);
	}
	else
	{
		CONTROL_ENABLE(CONTROL_BTNSCAN);
	}

	if (g_application.m_guiDialogMusicScan.IsRunning())
	{
		SET_CONTROL_LABEL(CONTROL_BTNSCAN,14056);	// Stop Scan
	}
	else
	{
		SET_CONTROL_LABEL(CONTROL_BTNSCAN,102);	//	Scan
	}

	//	Update sorting control
	bool bSortAscending=false;
	if (m_strDirectory.IsEmpty())
		bSortAscending=g_stSettings.m_bMyMusicSongsRootSortAscending;
	else
		bSortAscending=g_stSettings.m_bMyMusicSongsSortAscending;

	if (bSortAscending)
	{
    CGUIMessage msg(GUI_MSG_DESELECTED,GetID(), CONTROL_BTNSORTASC);
    g_graphicsContext.SendMessage(msg);
  }
  else
  {
    CGUIMessage msg(GUI_MSG_SELECTED,GetID(), CONTROL_BTNSORTASC);
    g_graphicsContext.SendMessage(msg);
  }

	//	Update listcontrol and view by icon/list button
	const CGUIControl* pControl=GetControl(CONTROL_THUMBS);
  if (pControl)
  {
	  if (!pControl->IsVisible())
	  {
		  CGUIMessage msg(GUI_MSG_ITEM_SELECTED,GetID(),CONTROL_LIST,0,0,NULL);
		  g_graphicsContext.SendMessage(msg);
		  int iItem=msg.GetParam1();
		  CONTROL_SELECT_ITEM(CONTROL_THUMBS,iItem);
	  }
  }
	pControl=GetControl(CONTROL_LIST);
  if (pControl)
  {
	  if (!pControl->IsVisible())
	  {
		  CGUIMessage msg(GUI_MSG_ITEM_SELECTED,GetID(),CONTROL_THUMBS,0,0,NULL);
		  g_graphicsContext.SendMessage(msg);
		  int iItem=msg.GetParam1();
		  CONTROL_SELECT_ITEM(CONTROL_LIST,iItem);
	  }
  }

	SET_CONTROL_HIDDEN(CONTROL_LIST);
	SET_CONTROL_HIDDEN(CONTROL_THUMBS);

	bool bViewIcon = false;
  int iString;
	if ( m_strDirectory.IsEmpty() ) 
  {
		switch (m_iViewAsIconsRoot)
    {
      case VIEW_AS_LIST:
        iString=101; // view as icons
      break;
      
      case VIEW_AS_ICONS:
        iString=100;  // view as large icons
        bViewIcon=true;
      break;
      case VIEW_AS_LARGEICONS:
        iString=417; // view as list
        bViewIcon=true;
      break;
    }
	}
	else 
  {
		switch (m_iViewAsIcons)
    {
      case VIEW_AS_LIST:
        iString=101; // view as icons
      break;
      
      case VIEW_AS_ICONS:
        iString=100;  // view as large icons
        bViewIcon=true;
      break;
      case VIEW_AS_LARGEICONS:
        iString=417; // view as list
        bViewIcon=true;
      break;
    }		
	}

	if (bViewIcon) 
	{
		SET_CONTROL_VISIBLE(CONTROL_THUMBS);
	}
	else
	{
		SET_CONTROL_VISIBLE(CONTROL_LIST);
	}

	SET_CONTROL_LABEL(CONTROL_BTNVIEWASICONS,iString);

	//	Update object count label
	int iItems=m_vecItems.size();
	if (iItems)
	{
		CFileItem* pItem=m_vecItems[0];
		if (pItem->GetLabel()=="..") iItems--;
	}
  WCHAR wszText[20];
  const WCHAR* szText=g_localizeStrings.Get(127).c_str();
  swprintf(wszText,L"%i %s", iItems,szText);

	SET_CONTROL_LABEL(CONTROL_LABELFILES,wszText);

	//	Update sort by button
	if (m_strDirectory.IsEmpty())
	{
		if (g_stSettings.m_iMyMusicSongsRootSortMethod==0)
		{
			SET_CONTROL_LABEL(CONTROL_BTNSORTBY,g_stSettings.m_iMyMusicSongsRootSortMethod+103);
		}
		else
		{
			SET_CONTROL_LABEL(CONTROL_BTNSORTBY,498);	//	Sort by: Type
		}
	}
	else
	{
		if (g_stSettings.m_iMyMusicSongsSortMethod<=2)
		{
			//	Sort by Name(ItemLabel), Date, Size
			SET_CONTROL_LABEL(CONTROL_BTNSORTBY,g_stSettings.m_iMyMusicSongsSortMethod+103);
		}
		else
		{
			//	Sort by FileName
			SET_CONTROL_LABEL(CONTROL_BTNSORTBY,363);
		}
	}
}

void CGUIWindowMusicSongs::OnClick(int iItem)
{
	CFileItem* pItem=m_vecItems[iItem];
	CStdString strPath=pItem->m_strPath;
	if (pItem->m_bIsFolder)
	{
		if ( pItem->m_bIsShareOrDrive ) 
		{
      if ( !HaveBookmarkPermissions( pItem, "music" ) )
        return;

			if ( !HaveDiscOrConnection( pItem->m_strPath, pItem->m_iDriveType ) )
				return;
		}
		Update(strPath);
	}
	else
	{
		if ( CUtil::IsPlayList(strPath)  )
		{
			LoadPlayList(strPath);
		}
		else
		{
			if (g_guiSettings.GetBool("MyMusic.AutoPlayNextItem"))
			{
				//play and add current directory to temporary playlist
				int nFolderCount=0;
				g_playlistPlayer.GetPlaylist( PLAYLIST_MUSIC_TEMP ).Clear();
				g_playlistPlayer.Reset();
				int iNoSongs=0;
				for ( int i = 0; i < (int) m_vecItems.size(); i++ ) 
				{
					CFileItem* pItem = m_vecItems[i];
					if ( pItem->m_bIsFolder ) 
					{
						nFolderCount++;
						continue;
					}
					if (!CUtil::IsPlayList(pItem->m_strPath))
					{
						CPlayList::CPlayListItem playlistItem ;
						CUtil::ConvertFileItemToPlayListItem(pItem, playlistItem);
						g_playlistPlayer.GetPlaylist(PLAYLIST_MUSIC_TEMP).Add(playlistItem);
					}
					else if (i<=iItem)
						iNoSongs++;
				}

				//	Save current window and directory to know where the selected item was
				m_nTempPlayListWindow=GetID();
				m_strTempPlayListDirectory=m_strDirectory;
				if (CUtil::HasSlashAtEnd(m_strTempPlayListDirectory))
					m_strTempPlayListDirectory.Delete(m_strTempPlayListDirectory.size()-1);

				g_playlistPlayer.SetCurrentPlaylist(PLAYLIST_MUSIC_TEMP);
				g_playlistPlayer.Play(iItem-nFolderCount-iNoSongs);
			}
			else
			{
				g_application.PlayFile(*pItem);
			}
		}
	}
}

void CGUIWindowMusicSongs::OnFileItemFormatLabel(CFileItem* pItem)
{
	//	set label 1
	if (pItem->m_musicInfoTag.Loaded())
	{
		CStdString str;
		CMusicInfoTag& tag=pItem->m_musicInfoTag;
		CStdString strTitle=tag.GetTitle();
		CStdString strArtist=tag.GetArtist();
		if (strTitle.size()) 
		{
			if (strArtist)
			{
				int iTrack=tag.GetTrackNumber();
				if (iTrack>0 && !g_guiSettings.GetBool("MusicLists.HideTrackNumber"))
					str.Format("%02.2i. %s - %s",iTrack, tag.GetArtist().c_str(), tag.GetTitle().c_str());
				else 
					str.Format("%s - %s", tag.GetArtist().c_str(), tag.GetTitle().c_str());
			}
			else
			{
				int iTrack=tag.GetTrackNumber();
				if (iTrack>0)
					str.Format("%02.2i. %s ",iTrack, tag.GetTitle().c_str());
				else 
					str.Format("%s", tag.GetTitle().c_str());
			}
			pItem->SetLabel( str );
		}
	}
	else
	{	// No tag, so we disable the file extension if it has one
		if (g_guiSettings.GetBool("FileLists.HideExtensions"))
			CUtil::RemoveExtension(pItem);
	}

	//	set label 2

	int nMyMusicSortMethod=0;
	if (m_strDirectory.IsEmpty())
		nMyMusicSortMethod=g_stSettings.m_iMyMusicSongsRootSortMethod;
	else
		nMyMusicSortMethod=g_stSettings.m_iMyMusicSongsSortMethod;

	if (nMyMusicSortMethod==0||nMyMusicSortMethod==2||nMyMusicSortMethod==8)
	{
		if (pItem->m_bIsFolder)
		{
			if (!CUtil::IsShoutCast(pItem->m_strPath))
			  pItem->SetLabel2("");
		}
		else 
		{
			if (pItem->m_dwSize > 0) 
			{
				CStdString strFileSize;
				CUtil::GetFileSize(pItem->m_dwSize, strFileSize);
				pItem->SetLabel2(strFileSize);
			}
			if ((nMyMusicSortMethod==0 || nMyMusicSortMethod==8) && pItem->m_musicInfoTag.Loaded()) 
			{
				int nDuration=pItem->m_musicInfoTag.GetDuration();
				if (nDuration)
				{
					CStdString strDuration;
					CUtil::SecondsToHMSString(nDuration, strDuration);
					pItem->SetLabel2(strDuration);
				}
			}
			//	cdda items always have duration
			if ((nMyMusicSortMethod==0 || nMyMusicSortMethod==8) && CStdString(CUtil::GetExtension(pItem->m_strPath))==".cdda")
			{
				int nDuration=pItem->m_musicInfoTag.GetDuration();
				if (nDuration)
				{
					CStdString strDuration;
					CUtil::SecondsToHMSString(nDuration, strDuration);
					pItem->SetLabel2(strDuration);
				}
			}
		}
	}
	else
	{
		if (pItem->m_stTime.wYear && (!CUtil::IsShoutCast(pItem->m_strPath)))
		{
			CStdString strDateTime;
			CUtil::GetDate(pItem->m_stTime, strDateTime);
			pItem->SetLabel2(strDateTime);
		}
		else if (!CUtil::IsShoutCast(pItem->m_strPath))
			pItem->SetLabel2("");
	}

	//	set thumbs and default icons
	if (!pItem->m_bIsShareOrDrive)
	{
		CUtil::SetMusicThumb(pItem);
		CUtil::FillInDefaultIcon(pItem);
	}
}

void CGUIWindowMusicSongs::DoSort(VECFILEITEMS& items)
{
	SSortMusicSongs sortmethod;

	sortmethod.m_strDirectory=m_strDirectory;

	if (m_strDirectory.IsEmpty())
	{
		sortmethod.m_iSortMethod=g_stSettings.m_iMyMusicSongsRootSortMethod;
		sortmethod.m_bSortAscending=g_stSettings.m_bMyMusicSongsRootSortAscending;
	}
	else
	{
		sortmethod.m_iSortMethod=g_stSettings.m_iMyMusicSongsSortMethod;
		sortmethod.m_bSortAscending=g_stSettings.m_bMyMusicSongsSortAscending;
	}

	sort(items.begin(), items.end(), sortmethod);
}

void CGUIWindowMusicSongs::OnRetrieveMusicInfo(VECFILEITEMS& items)
{

  //**** TO SPEED UP SCANNING *****
  // We need to speedup scanning of music
  // what we could do is:
  // 1. cache all genres & artists (with name and id) in a map (read them @ start of this routine)
  // 2. cache current path with database id
  // 3. scan the entire dir and then if we find a new song:
  // 4.   check if genre is known already (in cached map) if not then add it to database & map
  // 4.   check if artist is known already (in cached map) if not then add it to database & map
  // 5.   add the new song to the database 
  //         we need a new version of musicdatabase::addsong() for this
  //         which just adds the song (and not the artists/genre/path like musicdatabase::addsong does now!)
  //
  // this way we prevent that for each call to musicdatabase::addsong
  //    -the genre id is looked up and or added
  //    -the artist id is looked up and or added
  //    -the path id is lookup and or added

  
	int nFolderCount=CUtil::GetFolderCount(items);
	// Skip items with folders only
	if (nFolderCount == (int)items.size())
		return;

	int nFileCount=(int)items.size()-nFolderCount;

	CStdString strItem;
  MAPSONGS songsMap;
  // get all information for all files in current directory from database 
  g_musicDatabase.GetSongsByPath(m_strDirectory,songsMap);

	if (!m_bScan)
	{
		// Nothing in database and id3 tags disabled; dont load tags from cdda files
		if ((songsMap.size()==0 && !g_guiSettings.GetBool("MyMusic.UseTags")) || CUtil::IsCDDA(m_strDirectory))
		{
			g_application.ResetScreenSaver();
			return;
		}
	}

	bool bShowProgress=false;
	bool bProgressVisible=false;

	//	When loading a directory not in database show a progress dialog
	if (songsMap.size()==0 && !m_bScan && !m_gWindowManager.IsRouted())
		bShowProgress=true;

	DWORD dwTick=timeGetTime();

	// for every file found, but skip folder
  for (int i=0; i < (int)items.size(); ++i)
	{
    g_application.ResetScreenSaver();
		CFileItem* pItem=items[i];
		CStdString strExtension;
		CUtil::GetExtension(pItem->m_strPath,strExtension);

		//	Should we init a progress dialog
		if (bShowProgress && !bProgressVisible)
		{
			DWORD dwElapsed = timeGetTime() - dwTick;

			//	if tag loading took more then 1.5 secs. till now
			//	show the progress dialog 
			if (dwElapsed>1500)
			{
				if (m_dlgProgress) 
				{
					CURL url(m_strDirectory);
					CStdString strStrippedPath;
					url.GetURLWithoutUserDetails(strStrippedPath);
					m_dlgProgress->SetHeading(189);
					m_dlgProgress->SetLine(0, 505);
					m_dlgProgress->SetLine(1,"");
					m_dlgProgress->SetLine(2,strStrippedPath );
					m_dlgProgress->StartModal(GetID());
					m_dlgProgress->ShowProgressBar(true);
					m_dlgProgress->SetPercentage((i*100)/items.size());
					m_dlgProgress->Progress();
					bProgressVisible=true;
				}
			}
		}		

		if (bProgressVisible && (i%10)==0 && i>0)
		{
			m_dlgProgress->SetPercentage((i*100)/items.size());
			m_dlgProgress->Progress();
		}

		//	Progress key presses from controller or remote
		if (bProgressVisible || m_bScan)
			if (m_dlgProgress) m_dlgProgress->ProgressKeys();

		//	Canceled by the user, finish
		if (bProgressVisible && m_dlgProgress->IsCanceled())
		{
			if (m_dlgProgress) m_dlgProgress->Close();
			return;
		}

    // dont try reading id3tags for folders, playlists or shoutcast streams
		if (!pItem->m_bIsFolder && !CUtil::IsPlayList(pItem->m_strPath) && !CUtil::IsShoutCast(pItem->m_strPath) )
		{
      // is tag for this file already loaded?
			bool bNewFile=false;
			CMusicInfoTag& tag=pItem->m_musicInfoTag;
			if (!tag.Loaded() )
			{
        // no, then we gonna load it.
        // first search for file in our list of the current directory
				CSong song;
        bool bFound(false);
				IMAPSONGS it=songsMap.find(pItem->m_strPath);
				if (it!=songsMap.end())
				{
					song=it->second;
					bFound=true;
				}
        if (!bFound && !m_bScan)
        {
          // try finding it in the database
          CStdString strPathName;
          CStdString strFileName;
          CUtil::Split(pItem->m_strPath, strPathName, strFileName);
          if (strPathName != m_strDirectory)
          {
            if ( g_musicDatabase.GetSongByFileName(pItem->m_strPath, song) )
            {
              bFound=true;
            }
          }
        }
				if ( !bFound )
				{
          // if id3 tag scanning is turned on OR we're scanning the directory
          // then parse id3tag from file
					if (g_guiSettings.GetBool("MyMusic.UseTags") || m_bScan)
					{
            // get correct tag parser
						CMusicInfoTagLoaderFactory factory;
						auto_ptr<IMusicInfoTagLoader> pLoader (factory.CreateLoader(pItem->m_strPath));
						if (NULL != pLoader.get())
						{						
                // get id3tag
							if ( pLoader->Load(pItem->m_strPath,tag))
							{
								bNewFile=true;
							}
						}
					}
				}
				else // of if ( !bFound )
				{
					tag.SetSong(song);
				}
			}//if (!tag.Loaded() )
			else if (m_bScan)
			{
				IMAPSONGS it=songsMap.find(pItem->m_strPath);
				if (it==songsMap.end())
					bNewFile=true;
			}

			if (tag.Loaded() && m_bScan && bNewFile)
			{
				CSong song(tag);
				song.iStartOffset = pItem->m_lStartOffset;
				song.iEndOffset = pItem->m_lEndOffset;
				g_musicDatabase.AddSong(song,false);
			}
		}//if (!pItem->m_bIsFolder)
	}

	if (bShowProgress)
	{
		if (m_dlgProgress) m_dlgProgress->Close();
		return;
	}
}

void CGUIWindowMusicSongs::OnSearchItemFound(const CFileItem* pSelItem)
{
	if (pSelItem->m_bIsFolder)
	{
		CStdString strPath=pSelItem->m_strPath;
		CStdString strParentPath;
		CUtil::GetParentPath(strPath, strParentPath);

		Update(strParentPath);

		SetHistoryForPath(strParentPath);

		strPath=pSelItem->m_strPath;
		CURL url(strPath);
		if (url.GetProtocol()=="smb" && !CUtil::HasSlashAtEnd(strPath))
			strPath+="/";

		for (int i=0; i<(int)m_vecItems.size(); i++)
		{
			CFileItem* pItem=m_vecItems[i];
			if (pItem->m_strPath==strPath)
			{
				CONTROL_SELECT_ITEM(CONTROL_LIST, i);
				CONTROL_SELECT_ITEM(CONTROL_THUMBS, i);
				break;
			}
		}
	}
	else
	{
		CStdString strPath, strFileName;
		CUtil::Split(pSelItem->m_strPath, strPath, strFileName);
		
		Update(strPath);

		CStdString strParentPath;
		while (CUtil::GetParentPath(strPath, strParentPath))
		{
			m_history.Set(strPath, strParentPath);
			strPath=strParentPath;
		}
		m_history.Set(strPath, "");

		for (int i=0; i<(int)m_vecItems.size(); i++)
		{
			CFileItem* pItem=m_vecItems[i];
			if (pItem->m_strPath==pSelItem->m_strPath)
			{
				CONTROL_SELECT_ITEM(CONTROL_LIST, i);
				CONTROL_SELECT_ITEM(CONTROL_THUMBS, i);
				break;
			}
		}
	}

	const CGUIControl* pControl=GetControl(CONTROL_LIST);
	if (pControl && pControl->IsVisible())
	{
		SET_CONTROL_FOCUS(CONTROL_LIST, 0);
	}
	else
	{
		SET_CONTROL_FOCUS(CONTROL_THUMBS, 0);
	}
}

/// \brief Search for a song or a artist with search string \e strSearch in the musicdatabase and return the found \e items
/// \param strSearch The search string 
/// \param items Items Found
void CGUIWindowMusicSongs::DoSearch(const CStdString& strSearch,VECFILEITEMS& items)
{
	VECALBUMS albums;
	g_musicDatabase.FindAlbumsByName(strSearch, albums);

	if (albums.size())
	{
		CStdString strAlbum=g_localizeStrings.Get(483);	//	Album
		for (int i=0; i<(int)albums.size(); i++)
		{
			CAlbum& album=albums[i];
			CFileItem* pItem=new CFileItem(album);
			pItem->SetLabel("[" + strAlbum + "] " + album.strAlbum + " - " + album.strArtist);
			items.push_back(pItem);
		}
	}

	VECSONGS songs;
	g_musicDatabase.FindSongsByNameAndArtist(strSearch, songs);

	if (songs.size())
	{
		CStdString strSong=g_localizeStrings.Get(179);	//	Song
		for (int i=0; i<(int)songs.size(); i++)
		{
			CSong& song=songs[i];
			CFileItem* pItem=new CFileItem(song);
			pItem->SetLabel("[" + strSong + "] " + song.strTitle + " - " + song.strArtist + " - " + song.strAlbum);
			items.push_back(pItem);
		}
	}
}

void CGUIWindowMusicSongs::Update(const CStdString &strDirectory)
{
	CGUIWindowMusicBase::Update(strDirectory);
	if (!m_strDirectory.IsEmpty() && g_guiSettings.GetBool("MusicLists.UseAutoSwitching"))
	{
		m_iViewAsIcons = CAutoSwitch::GetView(m_vecItems);

		int iFocusedControl=GetFocusedControl();

		ShowThumbPanel();
		UpdateButtons();

		if (iFocusedControl==CONTROL_LIST || iFocusedControl==CONTROL_THUMBS)
		{
			int iControl = CONTROL_LIST;
			if (m_iViewAsIcons != VIEW_AS_LIST) iControl = CONTROL_THUMBS;
			SET_CONTROL_FOCUS(iControl, 0);
		}
	}
}

void CGUIWindowMusicSongs::GetDirectoryHistoryString(const CFileItem* pItem, CStdString& strHistoryString)
{
	if (pItem->m_bIsShareOrDrive)
	{
		//	We are in the virual directory

		//	History string of the DVD drive
		//	must be handel separately
		if (pItem->m_iDriveType==SHARE_TYPE_DVD)
		{
			//	Remove disc label from item label
			//	and use as history string, m_strPath
			//	can change for new discs
			CStdString strLabel=pItem->GetLabel();
			int nPosOpen=strLabel.Find('(');
			int nPosClose=strLabel.ReverseFind(')');
			if (nPosOpen>-1 && nPosClose>-1 && nPosClose>nPosOpen)
			{
				strLabel.Delete(nPosOpen+1, (nPosClose)-(nPosOpen+1));
				strHistoryString=strLabel;
			}
			else
				strHistoryString=strLabel;
		}
		else
		{
			//	Other items in virual directory
			CStdString strPath=pItem->m_strPath;
			while (CUtil::HasSlashAtEnd(strPath))
				strPath.Delete(strPath.size()-1);

			strHistoryString=pItem->GetLabel()+strPath;
		}
	}
	else
	{
		//	Normal directory items
		strHistoryString=pItem->m_strPath;

		if (CUtil::HasSlashAtEnd(strHistoryString))
			strHistoryString.Delete(strHistoryString.size()-1);
	}
}

void CGUIWindowMusicSongs::SetHistoryForPath(const CStdString& strDirectory)
{
	if (!strDirectory.IsEmpty())
	{
		//	Build the directory history for default path
		CStdString strPath, strParentPath;
		strPath=strDirectory;
		VECFILEITEMS items;
		CFileItemList itemlist(items);
		GetDirectory("", items);

		while (CUtil::GetParentPath(strPath, strParentPath))
		{
			bool bSet=false;
			for (int i=0; i<(int)items.size(); ++i)
			{
				CFileItem* pItem=items[i];
				while (CUtil::HasSlashAtEnd(pItem->m_strPath))
					pItem->m_strPath.Delete(pItem->m_strPath.size()-1);
				if (pItem->m_strPath==strPath)
				{
					CStdString strHistory;
					GetDirectoryHistoryString(pItem, strHistory);
					m_history.Set(strHistory, "");
					return;
				}
			}

			m_history.Set(strPath, strParentPath);
			strPath=strParentPath;
			while (CUtil::HasSlashAtEnd(strPath))
				strPath.Delete(strPath.size()-1);
		}
	}
}

void CGUIWindowMusicSongs::FilterItems(VECFILEITEMS &items)
{
	// Handle .CUE sheet files...
	VECSONGS itemstoadd;
	VECARTISTS itemstodelete;
	for (int i=0; i<(int)items.size(); i++)
	{
		CFileItem *pItem = items[i];
		if (!pItem->m_bIsFolder)
		{	// see if it's a .CUE sheet
			if (CUtil::IsCUESheet(pItem->m_strPath))
			{
				CCueDocument cuesheet;
				if (cuesheet.Parse(pItem->m_strPath))
				{
					VECSONGS newitems;
					cuesheet.GetSongs(newitems);
					// queue the cue sheet and the underlying media file for deletion
					if (CUtil::FileExists(cuesheet.GetMediaPath()))
					{
						itemstodelete.push_back(pItem->m_strPath);
						itemstodelete.push_back(cuesheet.GetMediaPath());
						// get the additional stuff (year, genre etc.) from the underlying media files tag.
						CMusicInfoTagLoaderFactory factory;
						CMusicInfoTag tag;
						auto_ptr<IMusicInfoTagLoader> pLoader (factory.CreateLoader(cuesheet.GetMediaPath()));
						if (NULL != pLoader.get())
						{						
							// get id3tag
							pLoader->Load(cuesheet.GetMediaPath(),tag);
						}
						// fill in any missing entries from underlying media file
						for (int j=0; j<(int)newitems.size(); j++)
						{
							CSong song = newitems[j];
							if (tag.Loaded())
							{
								if (song.strAlbum.empty() && !tag.GetAlbum().empty()) song.strAlbum = tag.GetAlbum();
								if (song.strGenre.empty() && !tag.GetGenre().empty()) song.strGenre = tag.GetGenre();
								if (song.strArtist.empty() && !tag.GetArtist().empty()) song.strArtist = tag.GetArtist();
								SYSTEMTIME dateTime;
								tag.GetReleaseDate(dateTime);
								if (dateTime.wYear > 1900) song.iYear = dateTime.wYear;
							}
							if (!song.iDuration && tag.GetDuration()>0)
							{	// must be the last song
								song.iDuration = (tag.GetDuration()*75 - song.iStartOffset+37)/75;
							}
							// add this item to the list
							itemstoadd.push_back(song);
						}
					}
					else
					{	// remove the .cue sheet from the directory
						itemstodelete.push_back(pItem->m_strPath);
					}
				}
			}
		}
	}
	// now delete the .CUE files and underlying media files.
	for (int i=0; i<(int)itemstodelete.size(); i++)
	{
		for (int j=0; j<(int)items.size(); j++)
		{
			CFileItem *pItem = items[j];
			if (pItem->m_strPath == itemstodelete[i])
			{	// delete this item
				delete pItem;
				items.erase(items.begin()+j);
				break;
			}
		}
	}
	// and add the files from the .CUE sheet
	for (int i=0; i<(int)itemstoadd.size(); i++)
	{
		// now create the file item, and add to the item list.
		CFileItem *pItem = new CFileItem(itemstoadd[i]);
		items.push_back(pItem);
	}
}

void CGUIWindowMusicSongs::OnPopupMenu(int iItem)
{
	// calculate our position
	int iPosX=200;
	int iPosY=100;
	const CGUIControl *pList = GetControl(CONTROL_LIST);
	if (pList)
	{
		iPosX = pList->GetXPosition()+pList->GetWidth()/2;
		iPosY = pList->GetYPosition()+pList->GetHeight()/2;
	}	
	if ( m_strDirectory.IsEmpty() )
	{
		// mark the item
		m_vecItems[iItem]->Select(true);
		// and do the popup menu
		if (CGUIDialogContextMenu::BookmarksMenu("music", m_vecItems[iItem]->GetLabel(), m_vecItems[iItem]->m_strPath, m_vecItems[iItem]->m_iLockMode, !(m_vecItems[iItem]->m_iBadPwdCount < g_stSettings.m_iMasterLockMaxRetry), iPosX, iPosY))
		{
			m_rootDir.SetShares(g_settings.m_vecMyMusicShares);
			Update(m_strDirectory);
			return;
		}
		m_vecItems[iItem]->Select(false);
		return;
	}
	CGUIWindowMusicBase::OnPopupMenu(iItem);
}