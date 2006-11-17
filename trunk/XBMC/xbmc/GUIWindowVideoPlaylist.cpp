#include "stdafx.h"
#include "GUIWindowVideoPlayList.h"
#include "PlayListFactory.h"
#include "Util.h"
#include "PlayListM3U.h"
#include "Application.h"
#include "PlayListPlayer.h"
#include "GUIThumbnailPanel.h"
#include "GUIListControl.h"
#include "GUIDialogContextMenu.h"

#define CONTROL_BTNVIEWASICONS     2 
#define CONTROL_BTNSORTBY          3
#define CONTROL_BTNSORTASC         4
#define CONTROL_LIST              50
#define CONTROL_LABELFILES        12

#define CONTROL_BTNSHUFFLE    20
#define CONTROL_BTNSAVE      21
#define CONTROL_BTNCLEAR     22

#define CONTROL_BTNPLAY      23
#define CONTROL_BTNNEXT      24
#define CONTROL_BTNPREVIOUS    25

#define CONTROL_BTNREPEAT     26
#define CONTROL_BTNRANDOMIZE  28

CGUIWindowVideoPlaylist::CGUIWindowVideoPlaylist()
: CGUIWindowVideoBase(WINDOW_VIDEO_PLAYLIST, "MyVideoPlaylist.xml")
{
  iPos = -1;
}

CGUIWindowVideoPlaylist::~CGUIWindowVideoPlaylist()
{
}

bool CGUIWindowVideoPlaylist::OnMessage(CGUIMessage& message)
{
  switch ( message.GetMessage() )
  {

  case GUI_MSG_PLAYLISTPLAYER_RANDOM:
  case GUI_MSG_PLAYLISTPLAYER_REPEAT:
    {
      UpdateButtons();
    }
    break;

  case GUI_MSG_WINDOW_DEINIT:
    {
      iPos = -1;
    }
    break;

  case GUI_MSG_WINDOW_INIT:
    {
      m_vecItems.m_strPath="playlistvideo://";

      if (!CGUIWindowVideoBase::OnMessage(message))
        return false;

      if (m_vecItems.Size() <= 0)
      {
        m_iLastControl = CONTROL_BTNVIEWASICONS;
        SET_CONTROL_FOCUS(m_iLastControl, 0);
      }

      if (g_application.IsPlayingVideo() && g_playlistPlayer.GetCurrentPlaylist() == PLAYLIST_VIDEO)
      {
        int iSong = g_playlistPlayer.GetCurrentSong();
        if (iSong >= 0 && iSong <= (int)m_vecItems.Size())
        {
          m_viewControl.SetSelectedItem(iSong);
        }
      }

      return true;
    }
    break;

  case GUI_MSG_CLICKED:
    {
      int iControl = message.GetSenderId();
      if (iControl == CONTROL_BTNRANDOMIZE)
      {
        g_stSettings.m_bMyVideoPlaylistShuffle = !g_playlistPlayer.IsShuffled(PLAYLIST_VIDEO);
        g_settings.Save();
        g_playlistPlayer.SetShuffle(PLAYLIST_VIDEO, g_stSettings.m_bMyVideoPlaylistShuffle);
        UpdateButtons();
      }
      else if (iControl == CONTROL_BTNSHUFFLE)
      {
        ShufflePlayList();
        UpdateButtons();
      }
      else if (iControl == CONTROL_BTNSAVE)
      {
        SavePlayList();
      }
      else if (iControl == CONTROL_BTNCLEAR)
      {
        ClearPlayList();
      }
      else if (iControl == CONTROL_BTNPLAY)
      {
        g_playlistPlayer.SetCurrentPlaylist(PLAYLIST_VIDEO);
        g_playlistPlayer.Reset();
        g_playlistPlayer.Play(m_viewControl.GetSelectedItem());
        UpdateButtons();
      }
      else if (iControl == CONTROL_BTNNEXT)
      {
        g_playlistPlayer.SetCurrentPlaylist(PLAYLIST_VIDEO);
        g_playlistPlayer.PlayNext();
      }
      else if (iControl == CONTROL_BTNPREVIOUS)
      {
        g_playlistPlayer.SetCurrentPlaylist(PLAYLIST_VIDEO);
        g_playlistPlayer.PlayPrevious();
      }
      else if (iControl == CONTROL_BTNREPEAT)
      {
        // increment repeat state
        PLAYLIST::REPEAT_STATE state = g_playlistPlayer.GetRepeat(PLAYLIST_VIDEO);
        if (state == PLAYLIST::REPEAT_NONE)
          g_playlistPlayer.SetRepeat(PLAYLIST_VIDEO, PLAYLIST::REPEAT_ALL);
        else if (state == PLAYLIST::REPEAT_ALL)
          g_playlistPlayer.SetRepeat(PLAYLIST_VIDEO, PLAYLIST::REPEAT_ONE);
        else
          g_playlistPlayer.SetRepeat(PLAYLIST_VIDEO, PLAYLIST::REPEAT_NONE);

        // save settings
        g_stSettings.m_bMyVideoPlaylistRepeat = g_playlistPlayer.GetRepeat(PLAYLIST_VIDEO) == PLAYLIST::REPEAT_ALL;
        g_settings.Save();

        UpdateButtons();
      }
      else if (m_viewControl.HasControl(iControl))  // list/thumb control
      {
        int iAction = message.GetParam1();
        int iItem = m_viewControl.GetSelectedItem();
        if (iAction == ACTION_DELETE_ITEM || iAction == ACTION_MOUSE_MIDDLE_CLICK)
        {
          RemovePlayListItem(iItem);
        }
      }
    }
    break;
  }
  return CGUIWindowVideoBase::OnMessage(message);
}

bool CGUIWindowVideoPlaylist::OnAction(const CAction &action)
{
  if (action.wID == ACTION_PARENT_DIR)
  {
    // Playlist has no parent dirs
    return true;
  }
  if (action.wID == ACTION_SHOW_PLAYLIST)
  {
    m_gWindowManager.PreviousWindow();
    return true;
  }
  if ((action.wID == ACTION_MOVE_ITEM_UP) || (action.wID == ACTION_MOVE_ITEM_DOWN))
  {
    int iItem = -1;
    int iFocusedControl = GetFocusedControlID();
    if (m_viewControl.HasControl(iFocusedControl))
      iItem = m_viewControl.GetSelectedItem();
    OnMove(iItem, action.wID);
    return true;
  }
  return CGUIWindowVideoBase::OnAction(action);
}

bool CGUIWindowVideoPlaylist::MoveCurrentPlayListItem(int iItem, int iAction, bool bUpdate /* = true */)
{
  int iSelected = iItem;
  int iNew = iSelected;
  if (iAction == ACTION_MOVE_ITEM_UP)
    iNew--;
  else
    iNew++;

  // is the currently playing item affected?
  bool bFixCurrentSong = false;
  if ((g_playlistPlayer.GetCurrentPlaylist() == PLAYLIST_VIDEO) && (g_application.IsPlayingVideo()) &&
    ((g_playlistPlayer.GetCurrentSong() == iSelected) || (g_playlistPlayer.GetCurrentSong() == iNew)))
    bFixCurrentSong = true;

  CPlayList& playlist = g_playlistPlayer.GetPlaylist(PLAYLIST_VIDEO);
  if (playlist.Swap(iSelected, iNew))
  {
    // Correct the current playing song in playlistplayer
    if (bFixCurrentSong)
    {
      int iCurrentSong = g_playlistPlayer.GetCurrentSong();
      if (iSelected == iCurrentSong)
        iCurrentSong = iNew;
      else if (iNew == iCurrentSong)
        iCurrentSong = iSelected;
      g_playlistPlayer.SetCurrentSong(iCurrentSong);
      m_vecItems[iCurrentSong]->Select(true);
    }

    if (bUpdate)
      Update(m_vecItems.m_strPath);
    return true;
  }

  return false;
}


void CGUIWindowVideoPlaylist::ClearPlayList()
{
  ClearFileItems();
  g_playlistPlayer.ClearPlaylist(PLAYLIST_VIDEO);
  if (g_playlistPlayer.GetCurrentPlaylist() == PLAYLIST_VIDEO)
  {
    g_playlistPlayer.Reset();
    g_playlistPlayer.SetCurrentPlaylist(PLAYLIST_NONE);
  }
  OnSort();
  UpdateButtons();
  SET_CONTROL_FOCUS(CONTROL_BTNVIEWASICONS, 0);
}

void CGUIWindowVideoPlaylist::UpdateButtons()
{
  // Update playlist buttons
  if (m_vecItems.Size() )
  {
    CONTROL_ENABLE(CONTROL_BTNCLEAR);
    CONTROL_ENABLE(CONTROL_BTNSAVE);
    CONTROL_ENABLE(CONTROL_BTNPLAY);
    CONTROL_ENABLE(CONTROL_BTNSHUFFLE);
    CONTROL_ENABLE(CONTROL_BTNRANDOMIZE);
    CONTROL_ENABLE(CONTROL_BTNREPEAT);

    if (g_application.IsPlayingVideo() && g_playlistPlayer.GetCurrentPlaylist() == PLAYLIST_VIDEO)
    {
      CONTROL_ENABLE(CONTROL_BTNNEXT);
      CONTROL_ENABLE(CONTROL_BTNPREVIOUS);
    }
    else
    {
      CONTROL_DISABLE(CONTROL_BTNNEXT);
      CONTROL_DISABLE(CONTROL_BTNPREVIOUS);
    }
  }
  else
  {
    CONTROL_DISABLE(CONTROL_BTNCLEAR);
    CONTROL_DISABLE(CONTROL_BTNSAVE);
    CONTROL_DISABLE(CONTROL_BTNSHUFFLE);
    CONTROL_DISABLE(CONTROL_BTNRANDOMIZE);
    CONTROL_DISABLE(CONTROL_BTNPLAY);
    CONTROL_DISABLE(CONTROL_BTNNEXT);
    CONTROL_DISABLE(CONTROL_BTNPREVIOUS);
    CONTROL_DISABLE(CONTROL_BTNREPEAT);
  }

  CGUIMediaWindow::UpdateButtons();

  // update buttons
  CONTROL_DESELECT(CONTROL_BTNSHUFFLE);
  CONTROL_DESELECT(CONTROL_BTNRANDOMIZE);
  if (g_playlistPlayer.GetPlaylist(PLAYLIST_VIDEO).IsShuffled())
    CONTROL_SELECT(CONTROL_BTNSHUFFLE);
  if (g_playlistPlayer.IsShuffled(PLAYLIST_VIDEO))
    CONTROL_SELECT(CONTROL_BTNRANDOMIZE);

  // update repeat button
  int iRepeat = 595 + g_playlistPlayer.GetRepeat(PLAYLIST_VIDEO);
  SET_CONTROL_LABEL(CONTROL_BTNREPEAT, g_localizeStrings.Get(iRepeat));
}

bool CGUIWindowVideoPlaylist::OnPlayMedia(int iItem)
{
  if ( iItem < 0 || iItem >= (int)m_vecItems.Size() ) return false;
  CFileItem* pItem = m_vecItems[iItem];
  CStdString strPath = pItem->m_strPath;
  g_playlistPlayer.SetCurrentPlaylist(PLAYLIST_VIDEO);
  g_playlistPlayer.Reset();
  g_playlistPlayer.Play( iItem );

  return true;
}

void CGUIWindowVideoPlaylist::RemovePlayListItem(int iItem)
{
  // The current playing song can't be removed
  if (g_playlistPlayer.GetCurrentPlaylist() == PLAYLIST_VIDEO && g_application.IsPlayingVideo()
      && g_playlistPlayer.GetCurrentSong() == iItem)
    return ;

  g_playlistPlayer.GetPlaylist(PLAYLIST_VIDEO).Remove(iItem);

  // Correct the current playing song in playlistplayer
  if (g_playlistPlayer.GetCurrentPlaylist() == PLAYLIST_VIDEO && g_application.IsPlayingVideo())
  {
    int iCurrentSong = g_playlistPlayer.GetCurrentSong();
    if (iItem <= iCurrentSong)
    {
      iCurrentSong--;
      g_playlistPlayer.SetCurrentSong(iCurrentSong);
      m_vecItems[iCurrentSong]->Select(true);
    }
  }

  Update(m_vecItems.m_strPath);

  if (m_vecItems.Size() <= 0)
  {
    SET_CONTROL_FOCUS(CONTROL_BTNVIEWASICONS, 0);
  }
  else
  {
    m_viewControl.SetSelectedItem(iItem - 1);
  }
}

void CGUIWindowVideoPlaylist::ShufflePlayList()
{
  int iPlaylist = PLAYLIST_VIDEO;
  ClearFileItems();
  CPlayList& playlist = g_playlistPlayer.GetPlaylist(iPlaylist);

  CStdString strFileName;
  if (g_application.IsPlayingVideo() && g_playlistPlayer.GetCurrentPlaylist() == iPlaylist)
  {
    const CPlayList::CPlayListItem& item = playlist[g_playlistPlayer.GetCurrentSong()];
    strFileName = item.GetFileName();
  }

  // shuffle or unshuffle?
  playlist.IsShuffled() ? playlist.UnShuffle() : playlist.Shuffle();
  if (g_playlistPlayer.GetCurrentPlaylist() == iPlaylist)
    g_playlistPlayer.Reset();

  if (!strFileName.IsEmpty())
  {
    for (int i = 0; i < playlist.size(); i++)
    {
      const CPlayList::CPlayListItem& item = playlist[i];
      if (item.GetFileName() == strFileName)
        g_playlistPlayer.SetCurrentSong(i);
    }
  }

  Update(m_vecItems.m_strPath);
}

/// \brief Save current playlist to playlist folder
void CGUIWindowVideoPlaylist::SavePlayList()
{
  CStdString strNewFileName;
  if (CGUIDialogKeyboard::ShowAndGetInput(strNewFileName, g_localizeStrings.Get(16012), false))
  {
    // need 2 rename it
    CStdString strPath;
    CUtil::RemoveIllegalChars( strNewFileName );
    strNewFileName += ".m3u";
    CUtil::AddFileToFolder(CUtil::VideoPlaylistsLocation(), strNewFileName, strPath);

    CPlayListM3U playlist;
    for (int i = 0; i < m_vecItems.Size(); ++i)
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
    CLog::Log(LOGDEBUG, "Saving video playlist: [%s]", strPath.c_str());
    playlist.Save(strPath);
  }
}

void CGUIWindowVideoPlaylist::OnPopupMenu(int iItem, bool bContextDriven /* = true */)
{
  if ( iItem < 0 || iItem >= m_vecItems.Size() ) return ;
  // calculate our position
  float posX = 200;
  float posY = 100;
  CGUIListControl *pList = (CGUIListControl *)GetControl(CONTROL_LIST);
  if (pList)
  {
    posX = pList->GetXPosition() + pList->GetWidth() / 2;
    posY = pList->GetYPosition() + pList->GetHeight() / 2;
  }
  // mark the item
  m_vecItems[iItem]->Select(true);
  // popup the context menu
  CGUIDialogContextMenu *pMenu = (CGUIDialogContextMenu *)m_gWindowManager.GetWindow(WINDOW_DIALOG_CONTEXT_MENU);
  if (!pMenu) return ;
  // load our menu
  pMenu->Initialize();

  // is this playlist playing?
  bool bIsPlaying = false;
  bool bItemIsPlaying = false;
  if ((g_playlistPlayer.GetCurrentPlaylist() == PLAYLIST_VIDEO) && (g_application.IsPlayingVideo()))
  {
    bIsPlaying = true;
    int i = g_playlistPlayer.GetCurrentSong();
    if (iItem == i) bItemIsPlaying = true;
  }
  // add the buttons
  int btn_Move = 0;   // move item
  int btn_MoveTo = 0; // move item here
  int btn_Cancel = 0; // cancel move
  int btn_MoveUp = 0; // move up
  int btn_MoveDn = 0; // move down
  int btn_Delete = 0; // delete

  if (iPos < 0)
  {
    btn_Move = pMenu->AddButton(13251);       // move item
    btn_MoveUp = pMenu->AddButton(13332);     // move up
    if (iItem == 0)
      pMenu->EnableButton(btn_MoveUp, false); // disable if top item
    btn_MoveDn = pMenu->AddButton(13333);     // move down
    if (iItem == (m_vecItems.Size()-1))
      pMenu->EnableButton(btn_MoveDn, false); // disable if bottom item
    btn_Delete = pMenu->AddButton(15015);     // delete
    if (bItemIsPlaying)
      pMenu->EnableButton(btn_Delete, false); // disable if current item
  }
  // after selecting "move item" only two choices
  else
  {
    btn_MoveTo = pMenu->AddButton(13252);         // move item here
    if (iItem == iPos)
      pMenu->EnableButton(btn_MoveTo, false);     // disable the button if its the same position or current item
    btn_Cancel = pMenu->AddButton(13253);         // cancel move
  }
  int btn_Return = pMenu->AddButton(12011);     // return to my videos

  // position it correctly
  pMenu->SetPosition(posX - pMenu->GetWidth() / 2, posY - pMenu->GetHeight() / 2);
  pMenu->DoModal();

  int btnid = pMenu->GetButton();
  if (btnid > 0)
  {
    // move item
    if (btnid == btn_Move)
    {
      iPos = iItem;
    }
    // move item here
    else if (btnid == btn_MoveTo && iPos >= 0)
    {
      MoveItem(iPos, iItem);
      iPos = -1;
    }
    // cancel move
    else if (btnid == btn_Cancel)
    {
      iPos = -1;
    }
    // move up
    else if (btnid == btn_MoveUp)
    {
      OnMove(iItem, ACTION_MOVE_ITEM_UP);
    }
    // move down
    else if (btnid == btn_MoveDn)
    {
      OnMove(iItem, ACTION_MOVE_ITEM_DOWN);
    }
    // delete
    else if (btnid == btn_Delete)
    {
      RemovePlayListItem(iItem);
      return;
    }
    // return to my videos
    else if (btnid == btn_Return)
    {
      m_gWindowManager.ActivateWindow(WINDOW_VIDEO_FILES);
      return;
    }
  }
  m_vecItems[iItem]->Select(false);

  // mark the currently playing item
  if (bIsPlaying)
  {
    int i = g_playlistPlayer.GetCurrentSong();
    m_vecItems[i]->Select(true);
  }
}

void CGUIWindowVideoPlaylist::OnMove(int iItem, int iAction)
{
  if (iItem < 0 || iItem >= m_vecItems.Size()) return;
  MoveCurrentPlayListItem(iItem, iAction);
}

void CGUIWindowVideoPlaylist::MoveItem(int iStart, int iDest)
{
  if (iStart < 0 || iStart >= m_vecItems.Size()) return;
  if (iDest < 0 || iDest >= m_vecItems.Size()) return;

  // default to move up
  int iAction = ACTION_MOVE_ITEM_UP;
  int iDirection = -1;
  // are we moving down?
  if (iStart < iDest)
  {
    iAction = ACTION_MOVE_ITEM_DOWN;
    iDirection = 1;
  }

  // keep swapping until you get to the destination or you
  // hit the currently playing song
  int i = iStart;
  while (i != iDest)
  {
    // try to swap adjacent items
    if (MoveCurrentPlayListItem(i, iAction, false))
      i = i + (1 * iDirection);
    // we hit currently playing song, so abort
    else
      break;
  }
  Update(m_vecItems.m_strPath);
}
