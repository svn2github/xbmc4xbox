
#include "stdafx.h"
#include "GUIWindowVideoPlayList.h"
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
#include "GUIThumbnailPanel.h"
#define CONTROL_BTNVIEWASICONS		2
#define CONTROL_BTNSORTBY					3
#define CONTROL_BTNSORTASC				4

#define CONTROL_BTNSHUFFLE				20
#define CONTROL_BTNSAVE						21
#define CONTROL_BTNCLEAR					22

#define CONTROL_BTNPLAY						23
#define CONTROL_BTNNEXT						24
#define CONTROL_BTNPREVIOUS				25

#define CONTROL_BTNREPEAT					26
#define CONTROL_BTNREPEATONE			27

#define CONTROL_LABELFILES        12

#define CONTROL_LIST							50
#define CONTROL_THUMBS						51

static int m_nTempPlayListWindow=0;
static CStdString m_strTempPlayListDirectory="";


CGUIWindowVideoPlaylist::CGUIWindowVideoPlaylist(void)
:CGUIWindow(0)
{
  m_strDirectory="";
	m_iLastControl=-1;
	m_iItemSelected=-1;
}
CGUIWindowVideoPlaylist::~CGUIWindowVideoPlaylist(void)
{

}

bool CGUIWindowVideoPlaylist::OnMessage(CGUIMessage& message)
{
  switch ( message.GetMessage() )
  {
		case GUI_MSG_PLAYBACK_STOPPED:
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
      OutputDebugString("deinit guiwindowvideoplaylist!\n");
			m_iItemSelected=GetSelectedItem();
			m_iLastControl=GetFocusedControl();
		}
		break;

    case GUI_MSG_WINDOW_INIT:
		{
      OutputDebugString("init guiwindowvideoplaylist!\n");
			CGUIWindow::OnMessage(message);

			Update("");

			if ((m_iLastControl==CONTROL_THUMBS || m_iLastControl==CONTROL_LIST) && m_vecItems.size()<=0)
			{
				m_iLastControl=CONTROL_BTNVIEWASICONS;
				SET_CONTROL_FOCUS(GetID(), m_iLastControl, 0);
			}

			if (m_iItemSelected>-1)
			{
				CONTROL_SELECT_ITEM(GetID(), CONTROL_LIST,m_iItemSelected);
				CONTROL_SELECT_ITEM(GetID(), CONTROL_THUMBS,m_iItemSelected);
			}

			if (g_application.IsPlayingVideo() && g_playlistPlayer.GetCurrentPlaylist()==PLAYLIST_VIDEO)
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
				CGUIWindow::OnMessage(message);
        g_stSettings.m_iMyVideoPlaylistViewAsIcons++;
        if (g_stSettings.m_iMyVideoPlaylistViewAsIcons > VIEW_AS_LARGEICONS) g_stSettings.m_iMyVideoPlaylistViewAsIcons=VIEW_AS_LIST;
				
        ShowThumbPanel();

				g_settings.Save();
        UpdateButtons();

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
				g_playlistPlayer.SetCurrentPlaylist(PLAYLIST_VIDEO);
				g_playlistPlayer.Reset();
				g_playlistPlayer.Play(GetSelectedItem());
				UpdateButtons();
			}
			else if (iControl==CONTROL_BTNNEXT)
			{
				g_playlistPlayer.SetCurrentPlaylist(PLAYLIST_VIDEO);
				g_playlistPlayer.PlayNext();
			}
			else if (iControl==CONTROL_BTNPREVIOUS)
			{
				g_playlistPlayer.SetCurrentPlaylist(PLAYLIST_VIDEO);
				g_playlistPlayer.PlayPrevious();
			}
			else if (iControl==CONTROL_BTNREPEAT)
			{
				g_stSettings.m_bMyVideoPlaylistRepeat=!g_stSettings.m_bMyVideoPlaylistRepeat;
				g_playlistPlayer.Repeat(PLAYLIST_VIDEO, g_stSettings.m_bMyVideoPlaylistRepeat);
			}
			else if (iControl==CONTROL_BTNREPEATONE)
			{
				static bool bRepeatOne=false;
				bRepeatOne=!bRepeatOne;
				g_playlistPlayer.RepeatOne(PLAYLIST_VIDEO, bRepeatOne);
			}
      else if (iControl==CONTROL_LIST||iControl==CONTROL_THUMBS)  // list/thumb control
      {
        int iItem=GetSelectedItem();
				int iAction=message.GetParam1();

				if (iAction == ACTION_QUEUE_ITEM)
        {
					OnQueueItem(iItem);
        }
        else if (iAction==ACTION_SELECT_ITEM)
        {
          OnClick(iItem);
        }
      }
		}
		break;

	}
	return CGUIWindow::OnMessage(message);
}

void CGUIWindowVideoPlaylist::OnAction(const CAction &action)
{
	if (action.wID==ACTION_PARENT_DIR)
	{
		//	Playlist has no parent dirs
		return;
	}

  if (action.wID==ACTION_PREVIOUS_MENU)
  {
		m_gWindowManager.ActivateWindow(WINDOW_HOME);
		return;
  }

	if (action.wID==ACTION_SHOW_PLAYLIST)
	{
    OutputDebugString("leave videplaylist!\n");
		m_gWindowManager.PreviousWindow();
		return;
	}

	CGUIWindow::OnAction(action);
}

void CGUIWindowVideoPlaylist::ClearPlayList()
{
	ClearFileItems();
	g_playlistPlayer.GetPlaylist(PLAYLIST_VIDEO).Clear();
	if (g_playlistPlayer.GetCurrentPlaylist()==PLAYLIST_VIDEO)
	{
		g_playlistPlayer.Reset();
		g_playlistPlayer.SetCurrentPlaylist(PLAYLIST_NONE);
	}
	UpdateListControl();
	UpdateButtons();
	SET_CONTROL_FOCUS(GetID(), CONTROL_BTNVIEWASICONS, 0);
}

void CGUIWindowVideoPlaylist::ClearFileItems()
{
  CGUIMessage msg1(GUI_MSG_LABEL_RESET,GetID(),CONTROL_LIST,0,0,NULL);
  g_graphicsContext.SendMessage(msg1);         

  CGUIMessage msg2(GUI_MSG_LABEL_RESET,GetID(),CONTROL_THUMBS,0,0,NULL);
  g_graphicsContext.SendMessage(msg2);         

	CFileItemList itemlist(m_vecItems); // will clean up everything

}

void CGUIWindowVideoPlaylist::UpdateListControl()
{
  CGUIMessage msg1(GUI_MSG_LABEL_RESET,GetID(),CONTROL_LIST,0,0,NULL);
  g_graphicsContext.SendMessage(msg1);         

  CGUIMessage msg2(GUI_MSG_LABEL_RESET,GetID(),CONTROL_THUMBS,0,0,NULL);
  g_graphicsContext.SendMessage(msg2);         

	for (int i=0; i < (int)m_vecItems.size(); i++)
	{
		CFileItem* pItem=m_vecItems[i];

		//	Format label for listcontrol
		//	and set thumb/icon for item
		OnFileItemFormatLabel(pItem);
	}

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


void CGUIWindowVideoPlaylist::OnFileItemFormatLabel(CFileItem* pItem)
{
  CUtil::SetThumb(pItem);
	CUtil::FillInDefaultIcon(pItem);
	// Remove extension from title if it exists
	pItem->SetLabel(CUtil::GetTitleFromPath(pItem->GetLabel()));
}

void CGUIWindowVideoPlaylist::DoSort(VECFILEITEMS& items)
{

}

void CGUIWindowVideoPlaylist::UpdateButtons()
{
	//	Update playlist buttons
	if (m_vecItems.size() )
	{
		CONTROL_ENABLE(GetID(), CONTROL_BTNCLEAR);
		CONTROL_ENABLE(GetID(), CONTROL_BTNSAVE);
		CONTROL_ENABLE(GetID(), CONTROL_BTNPLAY);
		CONTROL_ENABLE(GetID(), CONTROL_BTNSHUFFLE);
		CONTROL_ENABLE(GetID(), CONTROL_BTNREPEAT);
		CONTROL_ENABLE(GetID(), CONTROL_BTNREPEATONE);

		if (g_application.IsPlayingVideo() && g_playlistPlayer.GetCurrentPlaylist()==PLAYLIST_VIDEO)
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
		CONTROL_DISABLE(GetID(), CONTROL_BTNCLEAR);
		CONTROL_DISABLE(GetID(), CONTROL_BTNSAVE);
		CONTROL_DISABLE(GetID(), CONTROL_BTNSHUFFLE);
		CONTROL_DISABLE(GetID(), CONTROL_BTNPLAY);
		CONTROL_DISABLE(GetID(), CONTROL_BTNNEXT);
		CONTROL_DISABLE(GetID(), CONTROL_BTNPREVIOUS);
		CONTROL_DISABLE(GetID(), CONTROL_BTNREPEAT);
		CONTROL_DISABLE(GetID(), CONTROL_BTNREPEATONE);
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
	switch (g_stSettings.m_iMyVideoPlaylistViewAsIcons)
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

	//	Update Repeat/Repeat One button
	if (g_playlistPlayer.Repeated(PLAYLIST_VIDEO))
	{
		CONTROL_SELECT(GetID(), CONTROL_BTNREPEAT);
	}

	if (g_playlistPlayer.RepeatedOne(PLAYLIST_VIDEO))
	{
		CONTROL_SELECT(GetID(), CONTROL_BTNREPEATONE);
	}


}

bool CGUIWindowVideoPlaylist::ViewByIcon()
{
  if (g_stSettings.m_iMyVideoPlaylistViewAsIcons != VIEW_AS_LIST) return true;
  return false;
}

int CGUIWindowVideoPlaylist::GetSelectedItem()
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

bool CGUIWindowVideoPlaylist::ViewByLargeIcon()
{
  if (g_stSettings.m_iMyVideoPlaylistViewAsIcons== VIEW_AS_LARGEICONS) return true;
  return false;
}


void CGUIWindowVideoPlaylist::ShowThumbPanel()
{
  int iItem=GetSelectedItem(); 
  if ( ViewByLargeIcon() )
  {
    CGUIThumbnailPanel* pControl=(CGUIThumbnailPanel*)GetControl(CONTROL_THUMBS);
    pControl->ShowBigIcons(true);
  }
  else
  {
    CGUIThumbnailPanel* pControl=(CGUIThumbnailPanel*)GetControl(CONTROL_THUMBS);
    pControl->ShowBigIcons(false);
  }
  if (iItem>-1)
  {
    CONTROL_SELECT_ITEM(GetID(), CONTROL_LIST,m_iItemSelected);
    CONTROL_SELECT_ITEM(GetID(), CONTROL_THUMBS,m_iItemSelected);
  }
}

void CGUIWindowVideoPlaylist::GetDirectory(const CStdString &strDirectory, VECFILEITEMS &items)
{
	if (items.size()) 
	{
		CFileItemList itemlist(items); // will clean up everything
	}

	CPlayList& playlist = g_playlistPlayer.GetPlaylist(PLAYLIST_VIDEO);
	/* copy playlist from general playlist*/
	int iCurrentSong=-1;
	if (g_playlistPlayer.GetCurrentPlaylist() == PLAYLIST_VIDEO)
		iCurrentSong=g_playlistPlayer.GetCurrentSong();

	CStdString strPath,strFileName;
	for (int i=0; i < playlist.size(); ++i)
	{
		const CPlayList::CPlayListItem& item = playlist[i];

		CStdString strFileName   = item.GetFileName();
		CStdString strPath,strFName;
		CUtil::Split( strFileName, strPath, strFName);
		//m_Pathes.insert(strPath);
		
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

void CGUIWindowVideoPlaylist::Update(const CStdString &strDirectory)
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

	ClearFileItems();

	m_history.Set(strSelectedItem,m_strDirectory);
	m_strDirectory=strDirectory;

	GetDirectory(m_strDirectory, m_vecItems);


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

	int iCurrentSong=-1;
	//	Search current playlist item
	if ((m_nTempPlayListWindow==GetID() && m_strTempPlayListDirectory.Find(m_strDirectory) > -1 && g_application.IsPlayingVideo() 
			&& g_playlistPlayer.GetCurrentPlaylist()==PLAYLIST_VIDEO_TEMP) 
			|| (GetID()==WINDOW_VIDEO_PLAYLIST && g_playlistPlayer.GetCurrentPlaylist()==PLAYLIST_VIDEO && g_application.IsPlayingVideo()) )
	{
		iCurrentSong=g_playlistPlayer.GetCurrentSong();
	}

	bool bSelectedFound=false;
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
		if (i==iCurrentSong)
		{
			pItem->Select(true);
		}

	}

}

void CGUIWindowVideoPlaylist::GetDirectoryHistoryString(const CFileItem* pItem, CStdString& strHistoryString)
{
	strHistoryString=pItem->m_strPath;

	if (CUtil::HasSlashAtEnd(strHistoryString))
		strHistoryString.Delete(strHistoryString.size()-1);
}
void CGUIWindowVideoPlaylist::OnClick(int iItem)
{
	CFileItem* pItem=m_vecItems[iItem];
	CStdString strPath=pItem->m_strPath;
	g_playlistPlayer.SetCurrentPlaylist( PLAYLIST_VIDEO);
	g_playlistPlayer.Reset();
 	g_playlistPlayer.Play( iItem );
}

void CGUIWindowVideoPlaylist::OnQueueItem(int iItem)
{
	RemovePlayListItem(iItem);
}

void CGUIWindowVideoPlaylist::RemovePlayListItem(int iItem)
{
	//	The current playing song can't be removed
	if (g_playlistPlayer.GetCurrentPlaylist()==PLAYLIST_VIDEO && g_application.IsPlayingVideo()
			&& g_playlistPlayer.GetCurrentSong()==iItem)
			return;

	g_playlistPlayer.GetPlaylist(PLAYLIST_VIDEO).Remove(iItem);

	//	Correct the current playing song in playlistplayer
	if (g_playlistPlayer.GetCurrentPlaylist()==PLAYLIST_VIDEO && g_application.IsPlayingVideo())
	{
		int iCurrentSong=g_playlistPlayer.GetCurrentSong();
		if (iItem<=iCurrentSong)
		{
			iCurrentSong--;
			g_playlistPlayer.SetCurrentSong(iCurrentSong);
		}
	}

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

	if (m_vecItems.size()<=0)
	{
		SET_CONTROL_FOCUS(GetID(), CONTROL_BTNVIEWASICONS, 0);
	}
	else
	{
		CONTROL_SELECT_ITEM(GetID(), CONTROL_LIST,iItem-1)
		CONTROL_SELECT_ITEM(GetID(), CONTROL_THUMBS,iItem-1)
	}
}

void CGUIWindowVideoPlaylist::ShufflePlayList()
{
	ClearFileItems();
	CPlayList& playlist=g_playlistPlayer.GetPlaylist(PLAYLIST_VIDEO);
	
	CStdString strFileName;
	if (g_application.IsPlayingVideo() && g_playlistPlayer.GetCurrentPlaylist()==PLAYLIST_VIDEO)
	{
		const CPlayList::CPlayListItem& item=playlist[g_playlistPlayer.GetCurrentSong()];
		strFileName=item.GetFileName();
	}
	playlist.Shuffle();
	if (g_playlistPlayer.GetCurrentPlaylist()==PLAYLIST_VIDEO)
		g_playlistPlayer.Reset();

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

/// \brief Save current playlist to playlist folder
void CGUIWindowVideoPlaylist::SavePlayList()
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
/// \brief Display virtual keyboard
///	\param strInput Set as defaultstring in keyboard and retrieves the input from keyboard
bool CGUIWindowVideoPlaylist::GetKeyboard(CStdString& strInput)
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
		return true;
	}

	return false;
}