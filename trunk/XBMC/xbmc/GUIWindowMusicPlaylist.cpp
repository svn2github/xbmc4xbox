#include "GUIWindowMusicPlayList.h"
#include "settings.h"
#include "guiWindowManager.h"
#include "localizestrings.h"
#include "PlayListFactory.h"
#include "util.h"
#include "url.h"
#include "keyboard/virtualkeyboard.h"
#include "PlayListM3U.h"
#include "application.h"
#include "playlistplayer.h"
#include <algorithm>
#include "GuiUserMessages.h"

#define CONTROL_BTNVIEWASICONS		2
#define CONTROL_BTNSORTBY					3
#define CONTROL_BTNSORTASC				4

#define CONTROL_BTNSHUFFLE				20
#define CONTROL_BTNSAVE						21
#define CONTROL_BTNCLEAR					22

#define CONTROL_BTNPLAY						23
#define CONTROL_BTNNEXT						24
#define CONTROL_BTNPREVIOUS				25

#define CONTROL_LABELFILES        12

#define CONTROL_LIST							50
#define CONTROL_THUMBS						51

CGUIWindowMusicPlayList::CGUIWindowMusicPlayList(void)
:CGUIWindowMusicBase()
{

}
CGUIWindowMusicPlayList::~CGUIWindowMusicPlayList(void)
{

}

bool CGUIWindowMusicPlayList::OnMessage(CGUIMessage& message)
{
  switch ( message.GetMessage() )
  {
		case GUI_MSG_PLAYBACK_STOPPED:
		{
			if (g_playlistPlayer.GetCurrentPlaylist()==PLAYLIST_MUSIC)
				UpdateButtons();
		}
		break;

		case GUI_MSG_PLAYLIST_CHANGED:
		{
			//	global playlist changed outside playlist window
			Update("");

			if ((m_iLastControl==CONTROL_THUMBS || m_iLastControl==CONTROL_LIST) && m_vecItems.size()<=0)
			{
				m_iLastControl=CONTROL_BTNVIEWASICONS;
				SET_CONTROL_FOCUS(GetID(), m_iLastControl, 0);
			}

		}
		break;

		case GUI_MSG_WINDOW_DEINIT:
		{
			m_nSelectedItem=GetSelectedItem();
		}
		break;

    case GUI_MSG_WINDOW_INIT:
		{
			CGUIWindowMusicBase::OnMessage(message);

			m_iViewAsIconsRoot=g_stSettings.m_iMyMusicPlaylistViewAsIcons;

			Update(m_strDirectory);

			if ((m_iLastControl==CONTROL_THUMBS || m_iLastControl==CONTROL_LIST) && m_vecItems.size()<=0)
			{
				m_iLastControl=CONTROL_BTNVIEWASICONS;
				SET_CONTROL_FOCUS(GetID(), m_iLastControl, 0);
			}

			if (m_nSelectedItem>-1)
			{
				CONTROL_SELECT_ITEM(GetID(), CONTROL_LIST,m_nSelectedItem);
				CONTROL_SELECT_ITEM(GetID(), CONTROL_THUMBS,m_nSelectedItem);
			}

			if (g_application.IsPlayingAudio() && g_playlistPlayer.GetCurrentPlaylist()==PLAYLIST_MUSIC)
			{
				int iSong=g_playlistPlayer.GetCurrentSong();
				if (iSong >= 0 && iSong<=(int)m_vecItems.size())
				{
					CONTROL_SELECT_ITEM(GetID(), CONTROL_LIST,iSong);
					CONTROL_SELECT_ITEM(GetID(), CONTROL_THUMBS,iSong);
				}
			}
			return true;
		}
		break;

		case GUI_MSG_DVDDRIVE_EJECTED_CD:
		case GUI_MSG_DVDDRIVE_CHANGED_CD:
			return true;
		break;

		case GUI_MSG_CLICKED:
    {
      int iControl=message.GetSenderId();

      if (iControl==CONTROL_BTNVIEWASICONS)
      {
				CGUIWindowMusicBase::OnMessage(message);
				g_stSettings.m_iMyMusicPlaylistViewAsIcons=m_iViewAsIconsRoot;
				g_settings.Save();
			}
			else if (iControl==CONTROL_BTNSHUFFLE)
			{
				ShufflePlayList();
			}
			else if (iControl==CONTROL_BTNSAVE)
			{
				SavePlayList();
			}
			else if (iControl==CONTROL_BTNCLEAR)
			{
				ClearPlayList();
			}
			else if (iControl==CONTROL_BTNPLAY)
			{
				g_playlistPlayer.SetCurrentPlaylist(PLAYLIST_MUSIC);
				g_playlistPlayer.Play(GetSelectedItem());
				UpdateButtons();
			}
			else if (iControl==CONTROL_BTNNEXT)
			{
				g_playlistPlayer.SetCurrentPlaylist(PLAYLIST_MUSIC);
				g_playlistPlayer.PlayNext();
			}
			else if (iControl==CONTROL_BTNPREVIOUS)
			{
				g_playlistPlayer.SetCurrentPlaylist(PLAYLIST_MUSIC);
				g_playlistPlayer.PlayPrevious();
			}
		}
		break;

	}
	return CGUIWindowMusicBase::OnMessage(message);
}

void CGUIWindowMusicPlayList::OnAction(const CAction &action)
{
	if (action.wID==ACTION_PARENT_DIR)
	{
		//	Playlist has no parent dirs
		return;
	}

	if (action.wID==ACTION_SHOW_PLAYLIST)
	{
		m_gWindowManager.PreviousWindow();
		return;
	}

	CGUIWindowMusicBase::OnAction(action);
}

void CGUIWindowMusicPlayList::GetDirectory(const CStdString &strDirectory, VECFILEITEMS &items)
{
	if (items.size()) 
	{
		CFileItemList itemlist(items); // will clean up everything
	}

	CPlayList& playlist = g_playlistPlayer.GetPlaylist(PLAYLIST_MUSIC);
	/* copy playlist from general playlist*/
	int iCurrentSong=-1;
	if (g_playlistPlayer.GetCurrentPlaylist() == PLAYLIST_MUSIC)
		iCurrentSong=g_playlistPlayer.GetCurrentSong();

	CStdString strPath,strFileName;
	for (int i=0; i < playlist.size(); ++i)
	{
		const CPlayList::CPlayListItem& item = playlist[i];

		CStdString strFileName   = item.GetFileName();
		CStdString strPath,strFName;
		CUtil::Split( strFileName, strPath, strFName);
		m_Pathes.insert(strPath);
		
		CFileItem *pItem				 = new CFileItem(item.GetDescription());
		pItem->m_strPath			   = strFileName;
		pItem->m_bIsFolder		   = false;
		pItem->m_bIsShareOrDrive = false;

		if (item.GetDuration())
		{
			int nDuration=item.GetDuration();
			if (nDuration > 0)
			{
				CStdString str;
				CUtil::SecondsToHMSString(nDuration, str);
				pItem->SetLabel2(str);
			}
			else
				pItem->SetLabel2("");
		}
		items.push_back(pItem);
	}
}

void CGUIWindowMusicPlayList::SavePlayList()
{
	CStdString strNewFileName;
	if (GetKeyboard(strNewFileName))
	{
		// need 2 rename it
		CStdString strPath=g_stSettings.m_szAlbumDirectory;
		strPath+="\\playlists\\";

		strPath += strNewFileName;
		strPath+=".m3u";
		CPlayListM3U playlist;
		for (int i=0; i < (int)m_vecItems.size(); ++i)
		{
			CFileItem* pItem = m_vecItems[i];
			CPlayList::CPlayListItem newItem;
			newItem.SetFileName(pItem->m_strPath);
			newItem.SetDescription(pItem->GetLabel());
			if (pItem->m_musicInfoTag.Loaded())
				newItem.SetDuration(pItem->m_musicInfoTag.GetDuration());
			else
				newItem.SetDuration(0);
			playlist.Add(newItem);
		}
		playlist.Save(strPath);
	}
}

void CGUIWindowMusicPlayList::ClearPlayList()
{
	ClearFileItems();
	g_playlistPlayer.GetPlaylist(PLAYLIST_MUSIC).Clear();
	UpdateListControl();
	UpdateButtons();
	SET_CONTROL_FOCUS(GetID(), CONTROL_BTNVIEWASICONS, 0);
}

void CGUIWindowMusicPlayList::ShufflePlayList()
{
	ClearFileItems();
	CPlayList& playlist=g_playlistPlayer.GetPlaylist(PLAYLIST_MUSIC);
	
	CStdString strFileName;
	if (g_application.IsPlayingAudio() && g_playlistPlayer.GetCurrentPlaylist()==PLAYLIST_MUSIC)
	{
		const CPlayList::CPlayListItem& item=playlist[g_playlistPlayer.GetCurrentSong()];
		strFileName=item.GetFileName();
	}
	playlist.Shuffle();

	if (!strFileName.IsEmpty()) 
	{
		for (int i=0; i < playlist.size(); i++)
		{
			const CPlayList::CPlayListItem& item=playlist[i];
			if (item.GetFileName()==strFileName)
				g_playlistPlayer.SetCurrentSong(i);
		}
	}

	Update(m_strDirectory);
}

void CGUIWindowMusicPlayList::RemovePlayListItem(int iItem)
{
	const CFileItem* pItem=m_vecItems[iItem];
	CStdString strFileName=pItem->m_strPath;
	g_playlistPlayer.GetPlaylist(PLAYLIST_MUSIC).Remove(strFileName);
	int iCount=0;
	ivecItems it=m_vecItems.begin();
	while (it!=m_vecItems.end())
	{
		if (iCount==iItem)
		{
			m_vecItems.erase(it);
			break;
		}
		++it;
		iCount++;
	}
	UpdateListControl();
	UpdateButtons();
	CONTROL_SELECT_ITEM(GetID(), CONTROL_LIST,iItem)
	CONTROL_SELECT_ITEM(GetID(), CONTROL_THUMBS,iItem)
}

void CGUIWindowMusicPlayList::UpdateButtons()
{
	//	Update playlist buttons
	if (m_vecItems.size() )
	{
		CONTROL_ENABLE(GetID(), CONTROL_BTNSHUFFLE);
		CONTROL_ENABLE(GetID(), CONTROL_BTNSAVE);
		CONTROL_ENABLE(GetID(), CONTROL_BTNCLEAR);
		CONTROL_ENABLE(GetID(), CONTROL_BTNPLAY);

		if (g_application.IsPlayingAudio() && g_playlistPlayer.GetCurrentPlaylist()==PLAYLIST_MUSIC)
		{
			CONTROL_ENABLE(GetID(), CONTROL_BTNNEXT);
			CONTROL_ENABLE(GetID(), CONTROL_BTNPREVIOUS);
		}
		else
		{
			CONTROL_DISABLE(GetID(), CONTROL_BTNNEXT);
			CONTROL_DISABLE(GetID(), CONTROL_BTNPREVIOUS);
		}
	}
	else
	{
		CONTROL_DISABLE(GetID(), CONTROL_BTNSHUFFLE);
		CONTROL_DISABLE(GetID(), CONTROL_BTNSAVE);
		CONTROL_DISABLE(GetID(), CONTROL_BTNCLEAR);
		CONTROL_DISABLE(GetID(), CONTROL_BTNPLAY);
		CONTROL_DISABLE(GetID(), CONTROL_BTNNEXT);
		CONTROL_DISABLE(GetID(), CONTROL_BTNPREVIOUS);
	}

	//	Update listcontrol and and view by icon/list button
	const CGUIControl* pControl=GetControl(CONTROL_THUMBS);
  if (pControl)
  {
	  if (!pControl->IsVisible())
	  {
		  CGUIMessage msg(GUI_MSG_ITEM_SELECTED,GetID(),CONTROL_LIST,0,0,NULL);
		  g_graphicsContext.SendMessage(msg);
		  int iItem=msg.GetParam1();
		  CONTROL_SELECT_ITEM(GetID(), CONTROL_THUMBS,iItem);
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
		  CONTROL_SELECT_ITEM(GetID(), CONTROL_LIST,iItem);
	  }
  }

	SET_CONTROL_HIDDEN(GetID(), CONTROL_LIST);
	SET_CONTROL_HIDDEN(GetID(), CONTROL_THUMBS);

	bool bViewIcon = false;
  int iString;
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

	if (bViewIcon) 
  {
    SET_CONTROL_VISIBLE(GetID(), CONTROL_THUMBS);
  }
  else
  {
    SET_CONTROL_VISIBLE(GetID(), CONTROL_LIST);
  }

	SET_CONTROL_LABEL(GetID(), CONTROL_BTNVIEWASICONS,iString);

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

	SET_CONTROL_LABEL(GetID(), CONTROL_LABELFILES,wszText);
}

void CGUIWindowMusicPlayList::OnClick(int iItem)
{
	CFileItem* pItem=m_vecItems[iItem];
	CStdString strPath=pItem->m_strPath;
	g_playlistPlayer.SetCurrentPlaylist( PLAYLIST_MUSIC );
 	g_playlistPlayer.Play( iItem );
}

void CGUIWindowMusicPlayList::OnQueueItem(int iItem)
{
	RemovePlayListItem(iItem);
}

void CGUIWindowMusicPlayList::OnFileItemFormatLabel(CFileItem* pItem)
{
	if (pItem->m_musicInfoTag.Loaded())
	{
		//	set label 1
		CStdString str;
		CMusicInfoTag& tag=pItem->m_musicInfoTag;
		CStdString strTitle=tag.GetTitle();
		CStdString strArtist=tag.GetArtist();
		if (strTitle.size()) 
		{
			if (strArtist)
			{
				int iTrack=tag.GetTrackNumber();
				if (iTrack>0)
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
			pItem->SetLabel(str);
		}

		//	set label 2
		int nDuration=tag.GetDuration();
		if (nDuration > 0)
		{
			CUtil::SecondsToHMSString(nDuration, str);
			pItem->SetLabel2(str);
		}
	}
	//	set thumbs and default icons
	CUtil::SetMusicThumb(pItem);
	CUtil::FillInDefaultIcon(pItem);
}

void CGUIWindowMusicPlayList::DoSort(VECFILEITEMS& items)
{

}

void CGUIWindowMusicPlayList::OnRetrieveMusicInfo(VECFILEITEMS& items)
{
	if (items.size()<=0)
		return;

	MAPSONGS songsMap;
	m_database.GetSongsByPathes(m_Pathes, songsMap);

	CSong song;
	for (int i=0; i<(int)m_vecItems.size(); i++)
	{
		CFileItem* pItem=m_vecItems[i];

		IMAPSONGS it=songsMap.find(pItem->m_strPath);
		if (it!=songsMap.end())
		{
			song=it->second;
			pItem->m_musicInfoTag.SetAlbum(song.strAlbum);
			pItem->m_musicInfoTag.SetArtist(song.strArtist);
			pItem->m_musicInfoTag.SetGenre(song.strGenre);
			pItem->m_musicInfoTag.SetDuration(song.iDuration);
			pItem->m_musicInfoTag.SetTitle(song.strTitle);
			pItem->m_musicInfoTag.SetTrackNumber(song.iTrack);
			pItem->m_musicInfoTag.SetLoaded(true);
		}
		else
		{
			if (g_stSettings.m_bUseID3)
			{
				// get correct tag parser
				CMusicInfoTagLoaderFactory factory;
				auto_ptr<IMusicInfoTagLoader> pLoader (factory.CreateLoader(pItem->m_strPath));
				if (NULL != pLoader.get())
						// get id3tag
					pLoader->Load(pItem->m_strPath,pItem->m_musicInfoTag);
			}
		}
	}

	songsMap.erase(songsMap.begin(),songsMap.end());
}

