#include "stdafx.h"
#include "GUIMediaWindow.h"
#include "util.h"
#include "detectdvdtype.h"
#include "PlayListPlayer.h"
#include "FileSystem/ZipManager.h"
#include "GUIPassword.h"
#include "Application.h"
#include "xbox/network.h"
#include "PartyModeManager.h"
#include "GUIDialogMediaSource.h"
#include "GUIWindowFileManager.h"

#include "GUIImage.h"
#include "GUIMultiImage.h"

#define CONTROL_BTNVIEWASICONS     2
#define CONTROL_BTNSORTBY          3
#define CONTROL_BTNSORTASC         4
#define CONTROL_LIST              50
#define CONTROL_THUMBS            51
#define CONTROL_BIG_LIST          52
#define CONTROL_BIG_THUMBS        53
#define CONTROL_LABELFILES        12


CGUIMediaWindow::CGUIMediaWindow(DWORD id, const char *xmlFile)
    : CGUIWindow(id, xmlFile)
{
  m_vecItems.m_strPath = "?";
  m_iLastControl = -1;
  m_iSelectedItem = -1;

  m_guiState.reset(CGUIViewState::GetViewState(GetID(), m_vecItems));
}

CGUIMediaWindow::~CGUIMediaWindow()
{
}

void CGUIMediaWindow::OnWindowLoaded()
{
  CGUIWindow::OnWindowLoaded();
  m_viewControl.Reset();
  m_viewControl.SetParentWindow(GetID());
  m_viewControl.AddView(VIEW_METHOD_LIST, GetControl(CONTROL_LIST));
  m_viewControl.AddView(VIEW_METHOD_ICONS, GetControl(CONTROL_THUMBS));
  m_viewControl.AddView(VIEW_METHOD_LARGE_ICONS, GetControl(CONTROL_BIG_THUMBS));
  m_viewControl.AddView(VIEW_METHOD_LARGE_LIST, GetControl(CONTROL_BIG_LIST));
  m_viewControl.SetViewControlID(CONTROL_BTNVIEWASICONS);
  SetupShares();
}

void CGUIMediaWindow::OnWindowUnload()
{
  CGUIWindow::OnWindowUnload();
  m_viewControl.Reset();
}

CFileItem *CGUIMediaWindow::GetCurrentListItem()
{
  int iItem = m_viewControl.GetSelectedItem();
  if (iItem < 0) return NULL;
  return m_vecItems[iItem];
}

bool CGUIMediaWindow::OnAction(const CAction &action)
{
  if (action.wID == ACTION_PARENT_DIR)
  {
    if (m_vecItems.IsVirtualDirectoryRoot())
      m_gWindowManager.PreviousWindow();
    else
      GoParentFolder();
    return true;
  }

  if (action.wID == ACTION_PREVIOUS_MENU)
  {
    m_gWindowManager.PreviousWindow();
    return true;
  }

  return CGUIWindow::OnAction(action);
}

bool CGUIMediaWindow::OnMessage(CGUIMessage& message)
{
  switch ( message.GetMessage() )
  {
  case GUI_MSG_WINDOW_DEINIT:
    {
      m_iSelectedItem = m_viewControl.GetSelectedItem();
      m_iLastControl = GetFocusedControlID();
      CGUIWindow::OnMessage(message);
      // Call ClearFileItems() after our window has finished doing any WindowClose
      // animations
      ClearFileItems();
      return true;
    }
    break;

  case GUI_MSG_CLICKED:
    {
      int iControl = message.GetSenderId();
      if (iControl == CONTROL_BTNVIEWASICONS)
      {
        if (m_guiState.get())
          while (!m_viewControl.HasViewMode(m_guiState->SetNextViewAsControl()));

        UpdateButtons();
        return true;
      }
      else if (iControl == CONTROL_BTNSORTASC) // sort asc
      {
        if (m_guiState.get())
          m_guiState->SetNextSortOrder();
        UpdateFileList();
        return true;
      }
      else if (iControl == CONTROL_BTNSORTBY) // sort by
      {
        if (m_guiState.get())
          m_guiState->SetNextSortMethod();
        UpdateFileList();
        return true;
      }
      else if (m_viewControl.HasControl(iControl))  // list/thumb control
      {
        int iItem = m_viewControl.GetSelectedItem();
        int iAction = message.GetParam1();
        if (iItem < 0) break;
        if (iAction == ACTION_SELECT_ITEM || iAction == ACTION_MOUSE_LEFT_CLICK)
        {
          OnClick(iItem);
        }
      }
    }
    break;

  case GUI_MSG_SETFOCUS:
    {
      if (m_viewControl.HasControl(message.GetControlId()) && m_viewControl.GetCurrentControl() != message.GetControlId())
      {
        m_viewControl.SetFocused();
        return true;
      }
    }
    break;

  case GUI_MSG_NOTIFY_ALL:
    { // Message is received even if this window is inactive
      if (message.GetParam1() == GUI_MSG_WINDOW_RESET)
      {        
        m_vecItems.m_strPath = "?";
        return true;
      }

      if ( message.GetParam1() == GUI_MSG_REFRESH_THUMBS )
      {
        for (int i = 0; i < m_vecItems.Size(); i++)
          m_vecItems[i]->FreeMemory();
        break;  // the window will take care of any info images
      }
      if (message.GetParam1() == GUI_MSG_REMOVED_MEDIA)
      {
        if (m_vecItems.IsVirtualDirectoryRoot() && IsActive())
        {
          int iItem = m_viewControl.GetSelectedItem();
          Update(m_vecItems.m_strPath);
          m_viewControl.SetSelectedItem(iItem);
        }
        else if (m_vecItems.IsRemovable())
        { // check that we have this removable share still
          if (!m_rootDir.IsInShare(m_vecItems.m_strPath))
          { // don't have this share any more
            if (IsActive()) Update("");
            else 
            {
              m_history.ClearPathHistory();
              m_vecItems.m_strPath="";
            }
          }
        }
        return true;
      }
      else if (message.GetParam1()==GUI_MSG_UPDATE_BOOKMARKS)
      { // State of the bookmarks changed, so update our view
        if (m_vecItems.IsVirtualDirectoryRoot() && IsActive())
        {
          int iItem = m_viewControl.GetSelectedItem();
          Update(m_vecItems.m_strPath);
          m_viewControl.SetSelectedItem(iItem);
        }
        return true;
      }
    }
    break;
  case GUI_MSG_PLAYBACK_STARTED:
  case GUI_MSG_PLAYBACK_ENDED:
  case GUI_MSG_PLAYBACK_STOPPED:
  case GUI_MSG_PLAYLIST_CHANGED:
  case GUI_MSG_PLAYLISTPLAYER_STOPPED:
  case GUI_MSG_PLAYLISTPLAYER_STARTED:
  case GUI_MSG_PLAYLISTPLAYER_CHANGED:
    { // send a notify all to all controls on this window
      CGUIMessage msg(GUI_MSG_NOTIFY_ALL, GetID(), 0, GUI_MSG_REFRESH_LIST);
      return OnMessage(msg);
    }
  }

  return CGUIWindow::OnMessage(message);
}

// \brief Updates the states (enable, disable, visible...) 
// of the controls defined by this window
// Override this function in a derived class to add new controls
void CGUIMediaWindow::UpdateButtons()
{
  if (m_guiState.get())
  {
    // Update sorting controls
    if (m_guiState->GetDisplaySortOrder()==SORT_ORDER_NONE)
    {
      CONTROL_DISABLE(CONTROL_BTNSORTASC);
    }
    else
    {
      CONTROL_ENABLE(CONTROL_BTNSORTASC);
      if (m_guiState->GetDisplaySortOrder()==SORT_ORDER_ASC)
      {
        CGUIMessage msg(GUI_MSG_DESELECTED, GetID(), CONTROL_BTNSORTASC);
        g_graphicsContext.SendMessage(msg);
      }
      else
      {
        CGUIMessage msg(GUI_MSG_SELECTED, GetID(), CONTROL_BTNSORTASC);
        g_graphicsContext.SendMessage(msg);
      }
    }

    // Update list/thumb control
    m_viewControl.SetCurrentView(m_guiState->GetViewAsControl());

    // Update sort by button
    if (m_guiState->GetSortMethod()==SORT_METHOD_NONE)
    {
      CONTROL_DISABLE(CONTROL_BTNSORTBY);
    }
    else
    {
      CONTROL_ENABLE(CONTROL_BTNSORTBY);
    }
    SET_CONTROL_LABEL(CONTROL_BTNSORTBY, m_guiState->GetSortMethodLabel());
  }

  int iItems = m_vecItems.Size();
  if (iItems)
  {
    CFileItem* pItem = m_vecItems[0];
    if (pItem->IsParentFolder()) iItems--;
  }
  CStdString items;
  items.Format("%i %s", iItems, g_localizeStrings.Get(127).c_str());
  SET_CONTROL_LABEL(CONTROL_LABELFILES, items);
}

void CGUIMediaWindow::ClearFileItems()
{
  m_viewControl.Clear();
  m_vecItems.Clear(); // will clean up everything
}

// \brief Sorts Fileitems based on the sort method and sort oder provided by guiViewState
void CGUIMediaWindow::SortItems(CFileItemList &items)
{
  auto_ptr<CGUIViewState> guiState(CGUIViewState::GetViewState(GetID(), items));

  if (guiState.get())
  {
    items.Sort(guiState->GetSortMethod(), guiState->GetDisplaySortOrder());

    // Should these items be saved to the hdd
    if (items.GetCacheToDisc())
      items.Save();
  }
}

// \brief Formats item labels based on the formatting provided by guiViewState
void CGUIMediaWindow::FormatItemLabels()
{
  if (!m_guiState.get())
    return;

  CGUIViewState::LABEL_MASKS labelMasks;
  m_guiState->GetSortMethodLabelMasks(labelMasks);

  for (int i=0; i<m_vecItems.Size(); ++i)
  {
    CFileItem* pItem=m_vecItems[i];

    if (pItem->IsLabelPreformated())
      continue;

    pItem->FormatLabel(pItem->m_bIsFolder ? labelMasks.m_strLabelFolder : labelMasks.m_strLabelFile);
    pItem->FormatLabel2(pItem->m_bIsFolder ? labelMasks.m_strLabel2Folder : labelMasks.m_strLabel2File);
  }
}

// \brief Prepares and adds the fileitems list/thumb panel
void CGUIMediaWindow::OnSort()
{
  FormatItemLabels();
  SortItems(m_vecItems);
}

/*!
  \brief Overwrite to fill fileitems from a source
  \param strDirectory Path to read
  \param items Fill with items specified in \e strDirectory
  */
bool CGUIMediaWindow::GetDirectory(const CStdString &strDirectory, CFileItemList &items)
{
  // cleanup items
  if (items.Size())
    items.Clear();

  CStdString strParentPath=m_history.GetParentPath();

  CLog::Log(LOGDEBUG,"CGUIMediaWindow::GetDirectory (%s)", strDirectory.c_str());
  CLog::Log(LOGDEBUG,"  ParentPath = [%s]", strParentPath.c_str());

  if (m_guiState.get() && !m_guiState->HideParentDirItems())
  {
    CFileItem *pItem = new CFileItem("..");
    pItem->m_strPath = strParentPath;
    pItem->m_bIsFolder = true;
    pItem->m_bIsShareOrDrive = false;
    items.Add(pItem);
  }

  if (!m_rootDir.GetDirectory(strDirectory, items))
    return false;

  return true;
}

// \brief Set window to a specific directory
// \param strDirectory The directory to be displayed in list/thumb control
// This function calls OnPrepareFileItems() and OnFinalizeFileItems()
bool CGUIMediaWindow::Update(const CStdString &strDirectory)
{
  // get selected item
  int iItem = m_viewControl.GetSelectedItem();
  CStdString strSelectedItem = "";
  if (iItem >= 0 && iItem < m_vecItems.Size())
  {
    CFileItem* pItem = m_vecItems[iItem];
    if (!pItem->IsParentFolder())
    {
      GetDirectoryHistoryString(pItem, strSelectedItem);
    }
  }

  CStdString strOldDirectory = m_vecItems.m_strPath;

  m_history.SetSelectedItem(strSelectedItem, strOldDirectory);

  ClearFileItems();

  if (!GetDirectory(strDirectory, m_vecItems))
  {
    CLog::Log(LOGERROR,"CGUIMediaWindow::GetDirectory(%s) failed", strDirectory.c_str());
    // if the directory is the same as the old directory, then we'll return
    // false.  Else, we assume we can get the previous directory
    if (strDirectory.Equals(strOldDirectory))
      return false;

    // We assume, we can get the parent 
    // directory again, but we have to 
    // return false to be able to eg. show 
    // an error message.    
    CStdString strParentPath = m_history.GetParentPath();
    m_history.RemoveParentPath();
    Update(strParentPath);
    return false;
  } 

  // if we're getting the root bookmark listing
  // make sure the path history is clean
  if (strDirectory.IsEmpty())
    m_history.ClearPathHistory();

  int iWindow = GetID();
  bool bOkay = (iWindow == WINDOW_MUSIC_FILES || iWindow == WINDOW_VIDEO_FILES || iWindow == WINDOW_FILES || iWindow == WINDOW_PICTURES || iWindow == WINDOW_PROGRAMS);
  if (strDirectory.IsEmpty() && m_vecItems.IsEmpty() && bOkay) // add 'add source button'
  {
    CStdString strLabel = g_localizeStrings.Get(1026);
    CFileItem *pItem = new CFileItem(strLabel);
    pItem->m_strPath = "add";
    pItem->SetThumbnailImage("settings-network-focus.png");
    pItem->SetLabel(strLabel);
    pItem->SetLabelPreformated(true);
    m_vecItems.Add(pItem);
  }
  m_iLastControl = GetFocusedControlID();

  //  Ask the derived class if it wants to load additional info
  //  for the fileitems like media info or additional 
  //  filtering on the items, setting thumbs.
  OnPrepareFileItems(m_vecItems);

  m_vecItems.FillInDefaultIcons();

  m_guiState.reset(CGUIViewState::GetViewState(GetID(), m_vecItems));

  OnSort();
  UpdateButtons();

  // Ask the devived class if it wants to do custom list operations,
  // eg. changing the label
  OnFinalizeFileItems(m_vecItems);

  m_viewControl.SetItems(m_vecItems);

  strSelectedItem = m_history.GetSelectedItem(m_vecItems.m_strPath);

  bool bSelectedFound = false;
  //int iSongInDirectory = -1;
  for (int i = 0; i < m_vecItems.Size(); ++i)
  {
    CFileItem* pItem = m_vecItems[i];

    // Update selected item
    if (!bSelectedFound)
    {
      CStdString strHistory;
      GetDirectoryHistoryString(pItem, strHistory);
      if (strHistory == strSelectedItem)
      {
        m_viewControl.SetSelectedItem(i);
        bSelectedFound = true;
      }
    }
  }

  // if we haven't found the selected item, select the first item
  if (!bSelectedFound)
    m_viewControl.SetSelectedItem(0);

  m_history.AddPath(strDirectory);

  //m_history.DumpPathHistory();

  return true;
}

// \brief This function will be called by Update() before the
// labels of the fileitems are formatted. Override this function
// to set custom thumbs or load additional media info. 
// It's used to load tag info for music.
void CGUIMediaWindow::OnPrepareFileItems(CFileItemList &items)
{

}

// \brief This function will be called by Update() after the
// labels of the fileitems are formatted. Override this function
// to modify the fileitems. Eg. to modify the item label
void CGUIMediaWindow::OnFinalizeFileItems(CFileItemList &items)
{

}

// \brief With this function you can react on a users click in the list/thumb panel.
// It returns true, if the click is handled.
// This function calls OnPlayMedia()
bool CGUIMediaWindow::OnClick(int iItem)
{
  if ( iItem < 0 || iItem >= (int)m_vecItems.Size() ) return true;
  CFileItem* pItem = m_vecItems[iItem];

  if (pItem->IsParentFolder())
  {
    GoParentFolder();
    return true;
  }
  else if (pItem->m_bIsFolder)
  {
    if ( pItem->m_bIsShareOrDrive )
    {
      const CStdString& strLockType=m_guiState->GetLockType();
      if (g_settings.m_vecProfiles[0].getLockMode() != LOCK_MODE_EVERYONE)
        if (!strLockType.IsEmpty() && !g_passwordManager.IsItemUnlocked(pItem, strLockType))
            return true;

      if (!HaveDiscOrConnection(pItem->m_strPath, pItem->m_iDriveType))
        return true;
    }

    CFileItem directory(*pItem);
    if (!Update(directory.m_strPath))
      ShowShareErrorMessage(&directory);

    return true;
  }
  else
  {
    m_iSelectedItem = m_viewControl.GetSelectedItem();

    if (pItem->IsInternetStream())
    {
      CFileItem item = (CFileItem)*pItem;
      return g_application.PlayMedia(item, m_guiState->GetPlaylist());
    }
    if (pItem->IsPlayList())
    {
      CStdString strPath=pItem->m_strPath;
      LoadPlayList(pItem->m_strPath);
      return true;
    }
    // this is particular to music as only music allows "auto play next item"
    else if (m_guiState.get() && m_guiState->AutoPlayNextItem() && !g_partyModeManager.IsEnabled())
    {
      if (pItem->m_strPath == "add" && pItem->GetLabel() == g_localizeStrings.Get(1026)) // 'add source button' in empty root
      {
        if (CGUIDialogMediaSource::ShowAndAddMediaSource("music"))
        {
          Update("");
          return true;
        }
        return false;
      }
      //play and add current directory to temporary playlist
      int iPlaylist=m_guiState->GetPlaylist();
      if (iPlaylist!=PLAYLIST_NONE)
      {
        g_playlistPlayer.ClearPlaylist(iPlaylist);
        g_playlistPlayer.Reset();
        int nFolderCount = 0;
        int iNoSongs = 0;
        for ( int i = 0; i < m_vecItems.Size(); i++ )
        {
          CFileItem* pItem = m_vecItems[i];

          if (pItem->m_bIsFolder)
          {
            nFolderCount++;
            continue;
          }
          if (!pItem->IsPlayList() && !pItem->IsZIP() && !pItem->IsRAR())
          {
            g_playlistPlayer.Add(iPlaylist, pItem);
          }
          else if (i <= iItem)
            iNoSongs++;
        }

        // Save current window and directory to know where the selected item was
        if (m_guiState.get())
          m_guiState->SetPlaylistDirectory(m_vecItems.m_strPath);

        // figure out where we start playback
        int iOrder = iItem - nFolderCount - iNoSongs;
        if (g_playlistPlayer.IsShuffled(iPlaylist))
        {
          int iIndex = g_playlistPlayer.GetPlaylist(iPlaylist).FindOrder(iOrder);
          g_playlistPlayer.GetPlaylist(iPlaylist).Swap(0, iIndex);
          iOrder = 0;
        }

        // play
        g_playlistPlayer.SetCurrentPlaylist(iPlaylist);
        g_playlistPlayer.Play(iOrder);
      }
      return true;
    }
    else
    {
      return OnPlayMedia(iItem);
    }
  }

  return false;
}

// \brief Checks if there is a disc in the dvd drive and whether the
// network is connected or not.
bool CGUIMediaWindow::HaveDiscOrConnection(CStdString& strPath, int iDriveType)
{
  if (iDriveType==SHARE_TYPE_DVD)
  {
    MEDIA_DETECT::CDetectDVDMedia::WaitMediaReady();
    if (!MEDIA_DETECT::CDetectDVDMedia::IsDiscInDrive())
    {
      CGUIDialogOK::ShowAndGetInput(218, 219, 0, 0);
      return false;
    }
  }
  else if (iDriveType==SHARE_TYPE_REMOTE)
  {
    // TODO: Handle not connected to a remote share
    if ( !g_network.IsEthernetConnected() )
    {
      CGUIDialogOK::ShowAndGetInput(220, 221, 0, 0);
      return false;
    }
  }

  return true;
}

// \brief Shows a standard errormessage for a given pItem.
void CGUIMediaWindow::ShowShareErrorMessage(CFileItem* pItem)
{
  if (pItem->m_bIsShareOrDrive)
  {
    int idMessageText=0;
    const CURL& url=pItem->GetAsUrl();
    const CStdString& strHostName=url.GetHostName();

    if (pItem->m_iDriveType!=SHARE_TYPE_REMOTE) //  Local shares incl. dvd drive
      idMessageText=15300;
    else if (url.GetProtocol()=="xbms" && strHostName.IsEmpty()) //  xbms server discover
      idMessageText=15302;
    else if (url.GetProtocol()=="smb" && strHostName.IsEmpty()) //  smb workgroup
      idMessageText=15303;
    else  //  All other remote shares
      idMessageText=15301;

    CGUIDialogOK::ShowAndGetInput(220, idMessageText, 0, 0);
  }
}

// \brief The functon goes up one level in the directory tree
void CGUIMediaWindow::GoParentFolder()
{
  //m_history.DumpPathHistory();

  // remove current directory if its on the stack
  if (m_history.GetParentPath() == m_vecItems.m_strPath)
      m_history.RemoveParentPath();

  // if vector is not empty, pop parent
  // if vector is empty, parent is bookmark listing
  CStdString strParent=m_history.RemoveParentPath();

  CStdString strOldPath(m_vecItems.m_strPath);
  Update(strParent);

  if (!g_guiSettings.GetBool("filelists.fulldirectoryhistory"))
    m_history.RemoveSelectedItem(strOldPath); //Delete current path
}

// \brief Override the function to change the default behavior on how
// a selected item history should look like
void CGUIMediaWindow::GetDirectoryHistoryString(const CFileItem* pItem, CStdString& strHistoryString)
{
  if (pItem->m_bIsShareOrDrive)
  {
    // We are in the virual directory

    // History string of the DVD drive
    // must be handel separately
    if (pItem->m_iDriveType == SHARE_TYPE_DVD)
    {
      // Remove disc label from item label
      // and use as history string, m_strPath
      // can change for new discs
      CStdString strLabel = pItem->GetLabel();
      int nPosOpen = strLabel.Find('(');
      int nPosClose = strLabel.ReverseFind(')');
      if (nPosOpen > -1 && nPosClose > -1 && nPosClose > nPosOpen)
      {
        strLabel.Delete(nPosOpen + 1, (nPosClose) - (nPosOpen + 1));
        strHistoryString = strLabel;
      }
      else
        strHistoryString = strLabel;
    }
    else
    {
      // Other items in virual directory
      CStdString strPath = pItem->m_strPath;
      while (CUtil::HasSlashAtEnd(strPath))
        strPath.Delete(strPath.size() - 1);

      strHistoryString = pItem->GetLabel() + strPath;
    }
  }
  else if (pItem->m_lEndOffset>pItem->m_lStartOffset && pItem->m_lStartOffset != -1)
  {
    // Could be a cue item, all items of a cue share the same filename
    // so add the offsets to build the history string
    strHistoryString.Format("%ld%ld", pItem->m_lStartOffset, pItem->m_lEndOffset);
    strHistoryString += pItem->m_strPath;

    if (CUtil::HasSlashAtEnd(strHistoryString))
      strHistoryString.Delete(strHistoryString.size() - 1);
  }
  else
  {
    // Normal directory items
    strHistoryString = pItem->m_strPath;

    if (CUtil::HasSlashAtEnd(strHistoryString))
      strHistoryString.Delete(strHistoryString.size() - 1);
  }
  strHistoryString.ToLower();
}

// \brief Call this function to create a directory history for the
// path given by strDirectory.
void CGUIMediaWindow::SetHistoryForPath(const CStdString& strDirectory)
{
  // Make sure our shares are configured
  SetupShares();
  if (!strDirectory.IsEmpty())
  {
    // Build the directory history for default path
    CStdString strPath, strParentPath;
    strPath = strDirectory;
    while (CUtil::HasSlashAtEnd(strPath))
      strPath.Delete(strPath.size() - 1);

    CFileItemList items;
    m_rootDir.GetDirectory("", items);

    m_history.ClearPathHistory();

    while (CUtil::GetParentPath(strPath, strParentPath))
    {
      bool bSet = false;
      for (int i = 0; i < (int)items.Size(); ++i)
      {
        CFileItem* pItem = items[i];
        while (CUtil::HasSlashAtEnd(pItem->m_strPath))
          pItem->m_strPath.Delete(pItem->m_strPath.size() - 1);
        if (pItem->m_strPath == strPath)
        {
          CStdString strHistory;
          GetDirectoryHistoryString(pItem, strHistory);
          m_history.SetSelectedItem(strHistory, "");
          m_history.AddPathFront(strPath);
          m_history.AddPathFront("");

          //m_history.DumpPathHistory();
          return ;
        }
      }

      m_history.AddPathFront(strPath);
      m_history.SetSelectedItem(strPath, strParentPath);
      strPath = strParentPath;
      while (CUtil::HasSlashAtEnd(strPath))
        strPath.Delete(strPath.size() - 1);
    }
  }
  else
    m_history.ClearPathHistory();

  //m_history.DumpPathHistory();
}

// \brief Override if you want to change the default behavior, what is done
// when the user clicks on a file.
// This function is called by OnClick()
bool CGUIMediaWindow::OnPlayMedia(int iItem)
{
  // Reset Playlistplayer, playback started now does
  // not use the playlistplayer.
  g_playlistPlayer.Reset();
  g_playlistPlayer.SetCurrentPlaylist(PLAYLIST_NONE);
  CFileItem* pItem=m_vecItems[iItem];

  bool bResult = g_application.PlayFile(*pItem);
  if (pItem->m_lStartOffset == STARTOFFSET_RESUME)
    pItem->m_lStartOffset = 0;
  
  return bResult;
}

// \brief Synchonize the fileitems with the playlistplayer
// It recreated the playlist of the playlistplayer based
// on the fileitems of the window
void CGUIMediaWindow::UpdateFileList()
{
  int nItem = m_viewControl.GetSelectedItem();
  CFileItem* pItem = m_vecItems[nItem];
  const CStdString& strSelected = pItem->m_strPath;

  OnSort();
  UpdateButtons();
  
  m_viewControl.SetItems(m_vecItems);
  m_viewControl.SetSelectedItem(strSelected);

  //  set the currently playing item as selected, if its in this directory
  if (m_guiState.get() && m_guiState->IsCurrentPlaylistDirectory(m_vecItems.m_strPath))
  {
    int iPlaylist=m_guiState->GetPlaylist();
    int nSong = g_playlistPlayer.GetCurrentSong();
    CFileItem playlistItem;
    if (nSong > -1 && iPlaylist > -1)
      playlistItem=g_playlistPlayer.GetPlaylist(iPlaylist)[nSong];
    
    g_playlistPlayer.ClearPlaylist(iPlaylist);
    g_playlistPlayer.Reset();
    int nFolderCount = 0;
    int iNoSongs = 0;
    for (int i = 0; i < (int)m_vecItems.Size(); i++)
    {
      CFileItem* pItem = m_vecItems[i];
      if (pItem->m_bIsFolder)
      {
        nFolderCount++;
        continue;
      }
      if (!pItem->IsPlayList() && !pItem->IsZIP() && !pItem->IsRAR())
      {
        g_playlistPlayer.Add(iPlaylist, pItem);
      }
      else 
        iNoSongs++;

      if (pItem->m_strPath == playlistItem.m_strPath &&
          pItem->m_lStartOffset == playlistItem.m_lStartOffset)
        g_playlistPlayer.SetCurrentSong(i - nFolderCount - iNoSongs);
    }
  }
}

void CGUIMediaWindow::OnDeleteItem(int iItem)
{
  if ( iItem < 0 || iItem >= m_vecItems.Size()) return;
  CFileItem item(*m_vecItems[iItem]);
  
  if (item.IsPlayList())
    item.m_bIsFolder = false;

	if (g_settings.m_vecProfiles[g_settings.m_iLastLoadedProfileIndex].getLockMode() != LOCK_MODE_EVERYONE && g_settings.m_vecProfiles[g_settings.m_iLastLoadedProfileIndex].filesLocked())
    if (!g_passwordManager.IsMasterLockUnlocked(true))
      return;

  if (!CGUIWindowFileManager::DeleteItem(&item))
    return;
  Update(m_vecItems.m_strPath);
  m_viewControl.SetSelectedItem(iItem);
}

void CGUIMediaWindow::OnRenameItem(int iItem)
{
  if ( iItem < 0 || iItem >= m_vecItems.Size()) return;

  if (g_settings.m_vecProfiles[g_settings.m_iLastLoadedProfileIndex].getLockMode() != LOCK_MODE_EVERYONE && g_settings.m_vecProfiles[g_settings.m_iLastLoadedProfileIndex].filesLocked())
    if (!g_passwordManager.IsMasterLockUnlocked(true))
      return;

  if (!CGUIWindowFileManager::RenameFile(m_vecItems[iItem]->m_strPath))
    return;
  Update(m_vecItems.m_strPath);
  m_viewControl.SetSelectedItem(iItem);
}

void CGUIMediaWindow::OnInitWindow()
{
  Update(m_vecItems.m_strPath);

  if (m_iSelectedItem > -1)
    m_viewControl.SetSelectedItem(m_iSelectedItem);

  CGUIWindow::OnInitWindow();
}

CGUIControl *CGUIMediaWindow::GetFirstFocusableControl(int id)
{
  if (m_viewControl.HasControl(id))
    id = m_viewControl.GetCurrentControl();
  return CGUIWindow::GetFirstFocusableControl(id);
}

void CGUIMediaWindow::SetupShares()
{
  // Setup shares and filemasks for this window
  CFileItemList items;
  CGUIViewState* viewState=CGUIViewState::GetViewState(GetID(), items);
  if (viewState)
  {
    m_rootDir.SetMask(viewState->GetExtensions());
    m_rootDir.SetShares(viewState->GetShares());
    delete viewState;
  }
}