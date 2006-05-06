#include "stdafx.h"
#include "GUIWindowFileManager.h"
#include "Application.h"
#include "Util.h"
#include "xbox/xbeheader.h"
#include "FileSystem/Directory.h"
#include "FileSystem/ZipManager.h"
#include "Picture.h"
#include "GUIDialogContextMenu.h"
#include "GUIListControl.h"
#include "GUIDialogMediaSource.h"
#include "GUIPassword.h"
#include "lib/libPython/XBPython.h"
#include "GUIWindowSlideShow.h"
#include "PlayListFactory.h"
#include "utils/GUIInfoManager.h"

using namespace XFILE;

#define ACTION_COPY   1
#define ACTION_MOVE   2
#define ACTION_DELETE 3
#define ACTION_CREATEFOLDER 4
#define ACTION_DELETEFOLDER 5

#define CONTROL_BTNVIEWASICONS  2
#define CONTROL_BTNSORTBY     3
#define CONTROL_BTNSORTASC    4
#define CONTROL_BTNNEWFOLDER      6
#define CONTROL_BTNSELECTALL   7
#define CONTROL_BTNCOPY           10
#define CONTROL_BTNMOVE           11
#define CONTROL_BTNDELETE         8
#define CONTROL_BTNRENAME         9

#define CONTROL_NUMFILES_LEFT   12
#define CONTROL_NUMFILES_RIGHT  13

#define CONTROL_LEFT_LIST     20
#define CONTROL_RIGHT_LIST    21

#define CONTROL_CURRENTDIRLABEL_LEFT  101
#define CONTROL_CURRENTDIRLABEL_RIGHT  102

CGUIWindowFileManager::CGUIWindowFileManager(void)
    : CGUIWindow(WINDOW_FILES, "FileManager.xml")
{
  m_dlgProgress = NULL;
  m_Directory[0].m_strPath = "?";
  m_Directory[1].m_strPath = "?";
  m_Directory[0].m_bIsFolder = true;
  m_Directory[1].m_bIsFolder = true;
}

CGUIWindowFileManager::~CGUIWindowFileManager(void)
{}

bool CGUIWindowFileManager::OnAction(const CAction &action)
{
  int item;
  int list = GetFocusedList();
  if (action.wID == ACTION_DELETE_ITEM)
  {
    if (CanDelete(list))
    {
      bool bDeselect = SelectItem(list, item);
      OnDelete(list);
      if (bDeselect) m_vecItems[list][item]->Select(false);
    }
    return true;
  }
  if (action.wID == ACTION_COPY_ITEM)
  {
    if (CanCopy(list))
    {
      bool bDeselect = SelectItem(list, item);
      OnCopy(list);
      if (bDeselect) m_vecItems[list][item]->Select(false);
    }
    return true;
  }
  if (action.wID == ACTION_MOVE_ITEM)
  {
    if (CanMove(list))
    {
      bool bDeselect = SelectItem(list, item);
      OnMove(list);
      if (bDeselect) m_vecItems[list][item]->Select(false);
    }
    return true;
  }
  if (action.wID == ACTION_RENAME_ITEM)
  {
    if (CanRename(list))
    {
      bool bDeselect = SelectItem(list, item);
      OnRename(list);
      if (bDeselect) m_vecItems[list][item]->Select(false);
    }
    return true;
  }
  if (action.wID == ACTION_PARENT_DIR)
  {
    GoParentFolder(list);
    return true;
  }
  if (action.wID == ACTION_PREVIOUS_MENU)
  {
    m_gWindowManager.PreviousWindow();
    return true;
  }
  return CGUIWindow::OnAction(action);
}

bool CGUIWindowFileManager::OnMessage(CGUIMessage& message)
{
  switch ( message.GetMessage() )
  {
  case GUI_MSG_NOTIFY_ALL:
    { // Message is received even if window is inactive
      //  Is there a dvd share in this window?
      if (!m_rootDir.GetDVDDriveUrl().IsEmpty())
      {
        if (message.GetParam1()==GUI_MSG_DVDDRIVE_EJECTED_CD)
        {
          for (int i = 0; i < 2; i++)
          {
            if (m_Directory[i].IsVirtualDirectoryRoot() && IsActive())
            {
              int iItem = GetSelectedItem(i);
              Update(i, m_Directory[i].m_strPath);
              CONTROL_SELECT_ITEM(CONTROL_LEFT_LIST + i, iItem)
            }
            else if (m_Directory[i].IsCDDA() || m_Directory[i].IsOnDVD())
            { // Disc has changed and we are inside a DVD Drive share, get out of here :)
              if (IsActive()) Update(i, "");
              else m_Directory[i].m_strPath="";
            }
          }
        }
        else if (message.GetParam1()==GUI_MSG_DVDDRIVE_CHANGED_CD)
        { // State of the dvd-drive changed (Open/Busy label,...), so update it
          for (int i = 0; i < 2; i++)
          {
            if (m_Directory[i].IsVirtualDirectoryRoot() && IsActive())
            {
              int iItem = GetSelectedItem(i);
              Update(i, m_Directory[i].m_strPath);
              CONTROL_SELECT_ITEM(CONTROL_LEFT_LIST + i, iItem)
            }
          }
        }
      }
    }
    break;
  case GUI_MSG_WINDOW_DEINIT:
    {
      CGUIWindow::OnMessage(message);
      m_dlgProgress = NULL;
      ClearFileItems(0);
      ClearFileItems(1);
      return true;
    }
    break;

  case GUI_MSG_WINDOW_INIT:
    {
      SetInitialPath(message.GetStringParam());
      message.SetStringParam("");

      return CGUIWindow::OnMessage(message);
    }
    break;
  case GUI_MSG_CLICKED:
    {
      int iControl = message.GetSenderId();

      if (iControl == CONTROL_LEFT_LIST || iControl == CONTROL_RIGHT_LIST)  // list/thumb control
      {
        // get selected item
        int list = iControl - CONTROL_LEFT_LIST;
        int iItem = GetSelectedItem(list);
        int iAction = message.GetParam1();

        // iItem is checked for validity inside these routines
        if (iAction == ACTION_HIGHLIGHT_ITEM || iAction == ACTION_MOUSE_LEFT_CLICK)
        {
          OnMark(list, iItem);
          if (!g_Mouse.IsActive())
          {
            //move to next item
            CGUIMessage msg(GUI_MSG_ITEM_SELECT, GetID(), iControl, iItem + 1, 0, NULL);
            g_graphicsContext.SendMessage(msg);
          }
        }
        else if (iAction == ACTION_SELECT_ITEM || iAction == ACTION_MOUSE_LEFT_DOUBLE_CLICK)
        {
          OnClick(list, iItem);
        }
        else if (iAction == ACTION_CONTEXT_MENU || iAction == ACTION_MOUSE_RIGHT_CLICK)
        {
          OnPopupMenu(list, iItem);
        }
      }
    }
    break;
  }
  return CGUIWindow::OnMessage(message);
}

void CGUIWindowFileManager::OnSort(int iList)
{
  // always sort the list by label in ascending order
  for (int i = 0; i < m_vecItems[iList].Size(); i++)
  {
    CFileItem* pItem = m_vecItems[iList][i];
    if (pItem->m_bIsFolder && !pItem->m_dwSize)
      pItem->SetLabel2("");
    else
      pItem->SetFileSizeLabel();

    // Set free space on disc
    if (pItem->m_bIsShareOrDrive)
    {
      if (pItem->IsHD())
      {
        ULARGE_INTEGER ulBytesFree;
        if (GetDiskFreeSpaceEx(pItem->m_strPath.c_str(), &ulBytesFree, NULL, NULL))
        {
          pItem->m_dwSize = ulBytesFree.QuadPart;
          pItem->SetFileSizeLabel();
        }
      }
      else if (pItem->IsDVD() && CDetectDVDMedia::IsDiscInDrive())
      {
        ULARGE_INTEGER ulBytesTotal;
        if (GetDiskFreeSpaceEx(pItem->m_strPath.c_str(), NULL, &ulBytesTotal, NULL))
        {
          pItem->m_dwSize = ulBytesTotal.QuadPart;
          pItem->SetFileSizeLabel();
        }
      }
    } // if (pItem->m_bIsShareOrDrive)

  }

  m_vecItems[iList].Sort(SORT_METHOD_LABEL, SORT_ORDER_ASC);
  // UpdateControl(iList);
}

void CGUIWindowFileManager::ClearFileItems(int iList)
{
  CGUIMessage msg(GUI_MSG_LABEL_RESET, GetID(), iList + CONTROL_LEFT_LIST, 0, 0, NULL);
  g_graphicsContext.SendMessage(msg);

  m_vecItems[iList].Clear(); // will clean up everything
}

void CGUIWindowFileManager::UpdateButtons()
{

  /*
   // Update sorting control
   bool bSortOrder=false;
   if ( m_bViewSource ) 
   {
    if (m_strSourceDirectory.IsEmpty())
     bSortOrder=g_stSettings.m_bMyFilesSourceRootSortOrder;
    else
     bSortOrder=g_stSettings.m_bMyFilesSourceSortOrder;
   }
   else
   {
    if (m_strDestDirectory.IsEmpty())
     bSortOrder=g_stSettings.m_bMyFilesDestRootSortOrder;
    else
     bSortOrder=g_stSettings.m_bMyFilesDestSortOrder;
   }
   
   if (bSortOrder)
    {
      CGUIMessage msg(GUI_MSG_DESELECTED,GetID(), CONTROL_BTNSORTASC);
      g_graphicsContext.SendMessage(msg);
    }
    else
    {
      CGUIMessage msg(GUI_MSG_SELECTED,GetID(), CONTROL_BTNSORTASC);
      g_graphicsContext.SendMessage(msg);
    }
   
  */ 
  // update our current directory labels
  CStdString strDir;
  CURL(m_Directory[0].m_strPath).GetURLWithoutUserDetails(strDir);
  SET_CONTROL_LABEL(CONTROL_CURRENTDIRLABEL_LEFT, strDir);
  CURL(m_Directory[1].m_strPath).GetURLWithoutUserDetails(strDir);
  SET_CONTROL_LABEL(CONTROL_CURRENTDIRLABEL_RIGHT, strDir);

  // update the number of items in each list
  UpdateItemCounts();
}

void CGUIWindowFileManager::UpdateItemCounts()
{
  for (int i = 0; i < 2; i++)
  {
    unsigned int selectedCount = 0;
    unsigned int totalCount = 0;
    __int64 selectedSize = 0;
    __int64 totalSize = 0;
    for (int j = 0; j < m_vecItems[i].Size(); j++)
    {
      CFileItem *item = m_vecItems[i][j];
      if (item->IsParentFolder()) continue;
      if (item->IsSelected())
      {
        selectedCount++;
        selectedSize += item->m_dwSize;
      }
      totalCount++;
      totalSize += item->m_dwSize;
    }
    CStdString items;
    if (selectedCount > 0)
      items.Format("%i/%i %s (%s)", selectedCount, totalCount, g_localizeStrings.Get(127).c_str(), StringUtils::SizeToString(selectedSize).c_str());
    else
      items.Format("%i %s", totalCount, g_localizeStrings.Get(127).c_str());
    SET_CONTROL_LABEL(CONTROL_NUMFILES_LEFT + i, items);
  }
}

bool CGUIWindowFileManager::Update(int iList, const CStdString &strDirectory)
{
  // get selected item
  int iItem = GetSelectedItem(iList);
  CStdString strSelectedItem = "";

  if (iItem >= 0 && iItem < (int)m_vecItems[iList].Size())
  {
    CFileItem* pItem = m_vecItems[iList][iItem];
    if (!pItem->IsParentFolder())
    {
      GetDirectoryHistoryString(pItem, strSelectedItem);
      m_history[iList].SetSelectedItem(strSelectedItem, m_Directory[iList].m_strPath);
    }
  }

  CStdString strOldDirectory=m_Directory[iList].m_strPath;
  m_Directory[iList].m_strPath = strDirectory;

  CFileItemList items;
  if (!GetDirectory(iList, m_Directory[iList].m_strPath, items))
  {
    m_Directory[iList].m_strPath = strOldDirectory;
    return false;
  }

  m_history[iList].SetSelectedItem(strSelectedItem, strOldDirectory);

  ClearFileItems(iList);

  m_vecItems[iList].AppendPointer(items);
  m_vecItems[iList].m_strPath = items.m_strPath;
  items.ClearKeepPointer();

  // if we have a .tbn file, use itself as the thumb
  for (int i = 0; i < (int)m_vecItems[iList].Size(); i++)
  {
    CFileItem *pItem = m_vecItems[iList][i];
    CStdString strExtension;
    CUtil::GetExtension(pItem->m_strPath, strExtension);
    if (pItem->IsHD() && strExtension == ".tbn")
    {
      pItem->SetThumbnailImage(pItem->m_strPath);
    }
  }
  m_vecItems[iList].FillInDefaultIcons();

  OnSort(iList);
  UpdateButtons();
  UpdateControl(iList);

  strSelectedItem = m_history[iList].GetSelectedItem(m_Directory[iList].m_strPath);
  for (int i = 0; i < m_vecItems[iList].Size(); ++i)
  {
    CFileItem* pItem = m_vecItems[iList][i];
    CStdString strHistory;
    GetDirectoryHistoryString(pItem, strHistory);
    if (strHistory == strSelectedItem)
    {
      CONTROL_SELECT_ITEM(iList + CONTROL_LEFT_LIST, i);
      break;
    }
  }

  return true;
}


void CGUIWindowFileManager::OnClick(int iList, int iItem)
{
  if ( iList < 0 || iList > 2) return ;
  if ( iItem < 0 || iItem >= m_vecItems[iList].Size() ) return ;

  CFileItem *pItem = m_vecItems[iList][iItem];
  if (pItem->m_strPath == "add" && pItem->GetLabel() == g_localizeStrings.Get(1026)) // 'add source button' in empty root
  {
    if (CGUIDialogMediaSource::ShowAndAddMediaSource("files"))
    {
      Update(0,m_Directory[0].m_strPath);
      Update(1,m_Directory[1].m_strPath);
    }
    return;
  }

  if (pItem->m_bIsFolder)
  {
    // save path + drive type as HaveBookmarkPermissions does a Refresh()
    CStdString strPath = pItem->m_strPath;
    int iDriveType = pItem->m_iDriveType;
    if ( pItem->m_bIsShareOrDrive )
    {
      if ( !g_passwordManager.IsItemUnlocked( pItem, "files" ) )
      {
        Refresh();
        return ;
      }

      if ( !HaveDiscOrConnection( strPath, iDriveType ) )
        return ;
    }
    if (!Update(iList, strPath))
      ShowShareErrorMessage(pItem);
  }
  else if (pItem->IsZIP() || pItem->IsCBZ()) // mount zip archive
  {
    CShare shareZip;
    shareZip.strPath.Format("zip://Z:\\temp\\,%i,,%s,\\",1, pItem->m_strPath.c_str() );
    m_rootDir.AddShare(shareZip);
    Update(iList, shareZip.strPath);
  }
  else if (pItem->IsRAR() || pItem->IsCBR())
  {
    CShare shareRar;
    shareRar.strPath.Format("rar://Z:\\,%i,,%s,\\",EXFILE_AUTODELETE,pItem->m_strPath.c_str() );
    m_rootDir.AddShare(shareRar);
    Update(iList, shareRar.strPath);
  }
  else
  {
    OnStart(pItem);
    return ;
  }
  // UpdateButtons();
}

void CGUIWindowFileManager::OnStart(CFileItem *pItem)
{
  // start playlists from file manager
  if (pItem->IsPlayList())
  {
    CStdString strPlayList = pItem->m_strPath;
    CPlayListFactory factory;
    auto_ptr<CPlayList> pPlayList (factory.Create(strPlayList));
    if (NULL != pPlayList.get())
    {
      if (!pPlayList->Load(strPlayList))
      {
        CGUIDialogOK::ShowAndGetInput(6, 0, 477, 0);
        return;
      }
    }
    g_application.ProcessAndStartPlaylist(strPlayList, *pPlayList, PLAYLIST_MUSIC_TEMP);
    return;
  }
  if (pItem->IsAudio() || pItem->IsVideo())
  {
    g_application.PlayFile(*pItem);
    return ;
  }
  if (pItem->IsPythonScript())
  {
    g_pythonParser.evalFile(pItem->m_strPath.c_str());
    return ;
  }
  if (pItem->IsXBE())
  {
    int iRegion;
    if (g_guiSettings.GetBool("MyPrograms.GameAutoRegion"))
    {
      CXBE xbe;
      iRegion = xbe.ExtractGameRegion(pItem->m_strPath);
      if (iRegion < 1 || iRegion > 7)
        iRegion = 0;
      iRegion = xbe.FilterRegion(iRegion);
    }
    else
      iRegion = 0;
    CUtil::RunXBE(pItem->m_strPath.c_str(),NULL,F_VIDEO(iRegion));
  }
  else if (pItem->IsShortCut())
    CUtil::RunShortcut(pItem->m_strPath);
  if (pItem->IsPicture())
  {
    CGUIWindowSlideShow *pSlideShow = (CGUIWindowSlideShow *)m_gWindowManager.GetWindow(WINDOW_SLIDESHOW);
    if (!pSlideShow)
      return ;
    if (g_application.IsPlayingVideo())
      g_application.StopPlaying();

    pSlideShow->Reset();
    pSlideShow->Add(pItem->m_strPath);
    pSlideShow->Select(pItem->m_strPath);

    m_gWindowManager.ActivateWindow(WINDOW_SLIDESHOW);
  }
}

bool CGUIWindowFileManager::HaveDiscOrConnection( CStdString& strPath, int iDriveType )
{
  if ( iDriveType == SHARE_TYPE_DVD )
  {
    CDetectDVDMedia::WaitMediaReady();
    
    if ( !CDetectDVDMedia::IsDiscInDrive() )
    {
      CGUIDialogOK::ShowAndGetInput(218, 219, 0, 0);
      int iList = GetFocusedList();
      int iItem = GetSelectedItem(iList);
      Update(iList, "");
      CONTROL_SELECT_ITEM(iList + CONTROL_LEFT_LIST, iItem)
      return false;
    }
  }
  else if ( iDriveType == SHARE_TYPE_REMOTE )
  {
    // TODO: Handle not connected to a remote share
    if ( !CUtil::IsEthernetConnected() )
    {
      CGUIDialogOK::ShowAndGetInput(220, 221, 0, 0);
      return false;
    }
  }
  else
    return true;
  return true;
}

void CGUIWindowFileManager::UpdateControl(int iList)
{
  CGUIMessage msg(GUI_MSG_LABEL_RESET, GetID(), iList + CONTROL_LEFT_LIST, 0, 0, NULL);
  g_graphicsContext.SendMessage(msg);

  for (int i = 0; i < m_vecItems[iList].Size(); i++)
  {
    CFileItem* pItem = m_vecItems[iList][i];
    CGUIMessage msg(GUI_MSG_LABEL_ADD, GetID(), iList + CONTROL_LEFT_LIST, 0, 0, (void*)pItem);
    g_graphicsContext.SendMessage(msg);
  }
}

void CGUIWindowFileManager::OnMark(int iList, int iItem)
{
  CFileItem* pItem = m_vecItems[iList][iItem];

  if (!pItem->m_bIsShareOrDrive)
  {
    if (!pItem->IsParentFolder())
    {
      // MARK file
      pItem->Select(!pItem->IsSelected());
    }
  }

  UpdateItemCounts();
  // UpdateButtons();
}

bool CGUIWindowFileManager::DoProcessFile(int iAction, const CStdString& strFile, const CStdString& strDestFile)
{
  CStdString strShortSourceFile;
  CStdString strShortDestFile;
  CURL(strFile).GetURLWithoutUserDetails(strShortSourceFile);
  CURL(strDestFile).GetURLWithoutUserDetails(strShortDestFile);
  switch (iAction)
  {
  case ACTION_COPY:
    {
      CLog::Log(LOGDEBUG,"FileManager: copy %s->%s\n", strFile.c_str(), strDestFile.c_str());

      CURL url(strFile);
      if (!(url.GetProtocol() == "rar://"))
        if (m_dlgProgress)
        {
          m_dlgProgress->SetLine(0, 115);
          m_dlgProgress->SetLine(1, strShortSourceFile);
          m_dlgProgress->SetLine(2, strShortDestFile);
          m_dlgProgress->Progress();
        }

        if (url.GetProtocol() == "rar://")
        {
          CStdString strOriginalCachePath = g_advancedSettings.m_cachePath;
          CStdString strDestPath;
          CUtil::GetDirectory(strDestFile,strDestPath);
          g_advancedSettings.m_cachePath = strDestPath;
          CLog::Log(LOGDEBUG, "CacheRarredFile: dest=%s, file=%s",strDestPath.c_str(), url.GetFileName().c_str());
          bool bResult = g_RarManager.CacheRarredFile(strDestPath,url.GetHostName(),url.GetFileName(),0,strDestPath,1);
          g_advancedSettings.m_cachePath = strOriginalCachePath;
          return bResult;
        }
        else
        if (!CFile::Cache(strFile.c_str(), strDestFile.c_str(), this, NULL))
          return false;
    }
    break;


  case ACTION_MOVE:
    {
      CLog::Log(LOGDEBUG,"FileManager: move %s->%s\n", strFile.c_str(), strDestFile.c_str());

      if (m_dlgProgress)
      {
        m_dlgProgress->SetLine(0, 116);
        m_dlgProgress->SetLine(1, strShortSourceFile);
        m_dlgProgress->SetLine(2, strShortDestFile);
        m_dlgProgress->Progress();
      }

      if (strFile[1] == ':' && strFile[0] == strDestFile[0])
      {
        // quick move on same drive
        CFile::Rename(strFile.c_str(), strDestFile.c_str());
      }
      else
      {
        if (CFile::Cache(strFile.c_str(), strDestFile.c_str(), this, NULL ) )
        {
          CFile::Delete(strFile.c_str());
        }
        else
          return false;
      }
    }
    break;

  case ACTION_DELETE:
    {
      CLog::Log(LOGDEBUG,"FileManager: delete %s\n", strFile.c_str());

      CFile::Delete(strFile.c_str());
      if (m_dlgProgress)
      {
        m_dlgProgress->SetLine(0, 117);
        m_dlgProgress->SetLine(1, strShortSourceFile);
        m_dlgProgress->SetLine(2, "");
        m_dlgProgress->Progress();
      }
    }
    break;

  case ACTION_DELETEFOLDER:
    {
      CLog::Log(LOGDEBUG,"FileManager: delete folder %s\n", strFile.c_str());

      CDirectory::Remove(strFile);
      if (m_dlgProgress)
      {
        m_dlgProgress->SetLine(0, 117);
        m_dlgProgress->SetLine(1, strShortSourceFile);
        m_dlgProgress->SetLine(2, "");
        m_dlgProgress->Progress();
      }
    }
    break;

  case ACTION_CREATEFOLDER:
    {
      CLog::Log(LOGDEBUG,"FileManager: create folder %s\n", strFile.c_str());

      CDirectory::Create(strFile);

      if (m_dlgProgress)
      {
        m_dlgProgress->SetLine(0, 119);
        m_dlgProgress->SetLine(1, strShortSourceFile);
        m_dlgProgress->SetLine(2, "");
        m_dlgProgress->Progress();
      }
    }
    break;
  }

  if (m_dlgProgress) m_dlgProgress->Progress();
  return !m_dlgProgress->IsCanceled();
}

bool CGUIWindowFileManager::DoProcessFolder(int iAction, const CStdString& strPath, const CStdString& strDestFile)
{
  CLog::Log(LOGDEBUG,"FileManager, processing folder: %s",strPath.c_str());
  CFileItemList items;
  //m_rootDir.GetDirectory(strPath, items);
  CDirectory::GetDirectory(strPath, items, "", false);
  for (int i = 0; i < items.Size(); i++)
  {
    CFileItem* pItem = items[i];
    pItem->Select(true);
    CLog::Log(LOGDEBUG,"  -- %s",pItem->m_strPath.c_str());
  }

  if (!DoProcess(iAction, items, strDestFile)) return false;

  if (iAction == ACTION_MOVE)
  {
    CDirectory::Remove(strPath);
  }
  return true;
}

bool CGUIWindowFileManager::DoProcess(int iAction, CFileItemList & items, const CStdString& strDestFile)
{
  bool bCancelled(false);
  for (int iItem = 0; iItem < items.Size(); ++iItem)
  {
    CFileItem* pItem = items[iItem];
    if (pItem->IsSelected())
    {
      CStdString strCorrectedPath = pItem->m_strPath;
      if (CUtil::HasSlashAtEnd(strCorrectedPath))
      {
        strCorrectedPath = strCorrectedPath.Left(strCorrectedPath.size() - 1);
      }
      CStdString strFileName = CUtil::GetFileName(strCorrectedPath);
      CStdString strnewDestFile;
      CUtil::AddFileToFolder(strDestFile, strFileName.c_str(), strnewDestFile);
      if (pItem->m_bIsFolder)
      {
        // create folder on dest. drive
        if (iAction != ACTION_DELETE)
        {
          CLog::Log(LOGDEBUG, "Create folder with strDestFile=%s, strnewDestFile=%s, pFileName=%s", strDestFile.c_str(), strnewDestFile.c_str(), strFileName.c_str());
          if (!DoProcessFile(ACTION_CREATEFOLDER, strnewDestFile, strnewDestFile)) return false;
        }
        if (!DoProcessFolder(iAction, strCorrectedPath, strnewDestFile)) return false;
        if (iAction == ACTION_DELETE)
        {
          if (!DoProcessFile(ACTION_DELETEFOLDER, strCorrectedPath, pItem->m_strPath)) return false;
        }
      }
      else
      {
        if (!DoProcessFile(iAction, strCorrectedPath, strnewDestFile)) return false;
      }
    }
  }
  return true;
}

void CGUIWindowFileManager::OnCopy(int iList)
{
  if (!CGUIDialogYesNo::ShowAndGetInput(120, 123, 0, 0))
    return;

  ResetProgressBar();

  DoProcess(ACTION_COPY, m_vecItems[iList], m_Directory[1 - iList].m_strPath);

  if (m_dlgProgress) m_dlgProgress->Close();

  Refresh(1 - iList);
}

void CGUIWindowFileManager::OnMove(int iList)
{
  if (!CGUIDialogYesNo::ShowAndGetInput(121, 124, 0, 0))
    return;

  ResetProgressBar();

  DoProcess(ACTION_MOVE, m_vecItems[iList], m_Directory[1 - iList].m_strPath);

  if (m_dlgProgress) m_dlgProgress->Close();

  Refresh();
}

void CGUIWindowFileManager::OnDelete(int iList)
{
  if (!CGUIDialogYesNo::ShowAndGetInput(122, 125, 0, 0))
    return;

  ResetProgressBar(false);

  DoProcess(ACTION_DELETE, m_vecItems[iList], m_Directory[iList].m_strPath);

  if (m_dlgProgress) m_dlgProgress->Close();

  Refresh(iList);
}

void CGUIWindowFileManager::OnRename(int iList)
{
  CStdString strFile;
  for (int i = 0; i < m_vecItems[iList].Size();++i)
  {
    CFileItem* pItem = m_vecItems[iList][i];
    if (pItem->IsSelected())
    {
      strFile = pItem->m_strPath;
      break;
    }
  }

  RenameFile(strFile);

  Refresh(iList);
}

void CGUIWindowFileManager::OnSelectAll(int iList)
{
  for (int i = 0; i < m_vecItems[iList].Size();++i)
  {
    CFileItem* pItem = m_vecItems[iList][i];
    if (!pItem->IsParentFolder())
    {
      pItem->Select(true);
    }
  }
}

bool CGUIWindowFileManager::RenameFile(const CStdString &strFile)
{
  CStdString strFileName = CUtil::GetFileName(strFile);
  CStdString strPath = strFile.Left(strFile.size() - strFileName.size());
  if (CGUIDialogKeyboard::ShowAndGetInput(strFileName, g_localizeStrings.Get(16013), false))
  {
    strPath += strFileName;
    
    CStdString strLog;
    strLog.Format("FileManager: rename %s->%s\n", strFile.c_str(), strPath.c_str());
    OutputDebugString(strLog.c_str());
    CLog::Log(LOGINFO,"%s",strLog.c_str());

    CFile::Rename(strFile.c_str(), strPath.c_str());
    return true;
  }
  return false;
}

void CGUIWindowFileManager::OnNewFolder(int iList)
{
  CStdString strNewFolder = "";
  if (CGUIDialogKeyboard::ShowAndGetInput(strNewFolder, g_localizeStrings.Get(16014), false))
  {
    CStdString strNewPath = m_Directory[iList].m_strPath;
    if (!CUtil::HasSlashAtEnd(strNewPath) ) CUtil::AddSlashAtEnd(strNewPath);
    strNewPath += strNewFolder;
    CDirectory::Create(strNewPath);
    Refresh(iList);
  
    //  select the new folder
    for (int i=0; i<m_vecItems[iList].Size(); ++i)
    {
      CFileItem* pItem=m_vecItems[iList][i];
      CStdString strPath=pItem->m_strPath;
      if (CUtil::HasSlashAtEnd(strPath)) CUtil::RemoveSlashAtEnd(strPath);
      if (strPath==strNewPath)
      {
        CONTROL_SELECT_ITEM(iList + CONTROL_LEFT_LIST, i);
        break;
      }
    }
  }
}

void CGUIWindowFileManager::Refresh(int iList)
{
  int nSel = GetSelectedItem(iList);
  // update the list views
  Update(iList, m_Directory[iList].m_strPath);

  // UpdateButtons();
  // UpdateControl(iList);

  while (nSel > m_vecItems[iList].Size())
    nSel--;

  CONTROL_SELECT_ITEM(iList + CONTROL_LEFT_LIST, nSel);
}


void CGUIWindowFileManager::Refresh()
{
  int iList = GetFocusedList();
  int nSel = GetSelectedItem(iList);
  // update the list views
  Update(0, m_Directory[0].m_strPath);
  Update(1, m_Directory[1].m_strPath);

  // UpdateButtons();
  //  UpdateControl(0);
  // UpdateControl(1);

  while (nSel > (int)m_vecItems[iList].Size())
    nSel--;

  CONTROL_SELECT_ITEM(iList + CONTROL_LEFT_LIST, nSel);
}

int CGUIWindowFileManager::GetSelectedItem(int iControl)
{
  CGUIListControl *pControl = (CGUIListControl *)GetControl(iControl + CONTROL_LEFT_LIST);
  if (!pControl || !m_vecItems[iControl].Size()) return -1;
  return pControl->GetSelectedItem();
}

void CGUIWindowFileManager::GoParentFolder(int iList)
{
  CURL url(m_Directory[iList].m_strPath);
  if ((url.GetProtocol() == "rar") || (url.GetProtocol() == "zip")) 
  {
    // check for step-below, if, unmount rar
    if (url.GetFileName().IsEmpty())
    {
      if (url.GetProtocol() == "zip") 
        g_ZipManager.release(m_Directory[iList].m_strPath); // release resources
      m_rootDir.RemoveShare(m_Directory[iList].m_strPath);
      CStdString strPath;
      CUtil::GetDirectory(url.GetHostName(),strPath);
      Update(iList,strPath);
      return;
    }
  }
  CStdString strPath(m_strParentPath[iList]), strOldPath(m_Directory[iList].m_strPath);
  Update(iList, strPath);

  if (!g_guiSettings.GetBool("LookAndFeel.FullDirectoryHistory"))
    m_history[iList].RemoveSelectedItem(strOldPath); //Delete current path
}

void CGUIWindowFileManager::Render()
{
  CGUIWindow::Render();
}

bool CGUIWindowFileManager::OnFileCallback(void* pContext, int ipercent)
{
  if (m_dlgProgress)
  {
    m_dlgProgress->SetPercentage(ipercent);
    m_dlgProgress->Progress();
    if (m_dlgProgress->IsCanceled()) return false;
  }
  return true;
}

/// \brief Build a directory history string
/// \param pItem Item to build the history string from
/// \param strHistoryString History string build as return value
void CGUIWindowFileManager::GetDirectoryHistoryString(const CFileItem* pItem, CStdString& strHistoryString)
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
  else
  {
    // Normal directory items
    strHistoryString = pItem->m_strPath;

    if (CUtil::HasSlashAtEnd(strHistoryString))
      strHistoryString.Delete(strHistoryString.size() - 1);
  }
}

bool CGUIWindowFileManager::GetDirectory(int iList, const CStdString &strDirectory, CFileItemList &items)
{
  CStdString strParentPath;
  bool bParentExists = CUtil::GetParentPath(strDirectory, strParentPath);

  // check if current directory is a root share
  if ( !m_rootDir.IsShare(strDirectory) )
  {
    // no, do we got a parent dir?
    if ( bParentExists )
    {
      // yes
      if (!g_guiSettings.GetBool("FileLists.HideParentDirItems"))
      {
        CFileItem *pItem = new CFileItem("..");
        pItem->m_strPath = strParentPath;
        pItem->m_bIsFolder = true;
        pItem->m_bIsShareOrDrive = false;
        items.Add(pItem);
      }
      m_strParentPath[iList] = strParentPath;
    }
  }
  else
  {
    // yes, this is the root of a share
    // add parent path to the virtual directory
    if (!g_guiSettings.GetBool("FileLists.HideParentDirItems"))
    {
      CFileItem *pItem = new CFileItem("..");
      pItem->m_strPath = "";
      pItem->m_bIsFolder = true;
      pItem->m_bIsShareOrDrive = false;
      items.Add(pItem);
    }
    m_strParentPath[iList] = "";
  }

  return m_rootDir.GetDirectory(strDirectory, items, false);
}

bool CGUIWindowFileManager::CanRename(int iList)
{
  // TODO: Renaming of shares (requires writing to xboxmediacenter.xml)
  // this might be able to be done via the webserver code stuff...
  if (m_Directory[iList].IsVirtualDirectoryRoot()) return false;
  if (m_Directory[iList].IsReadOnly()) return false;

  return true;
}

bool CGUIWindowFileManager::CanCopy(int iList)
{
  // can't copy if the destination is not writeable, or if the source is a share!
  // TODO: Perhaps if the source is removeable media (DVD/CD etc.) we could
  // put ripping/backup in here.
  if (m_Directory[1 - iList].IsVirtualDirectoryRoot()) return false;
  if (m_Directory[iList].IsVirtualDirectoryRoot()) return false;
  if (m_Directory[1 -iList].IsReadOnly()) return false;
  return true;
}

bool CGUIWindowFileManager::CanMove(int iList)
{
  // can't move if the destination is not writeable, or if the source is a share or not writeable!
  if (m_Directory[0].IsVirtualDirectoryRoot() || m_Directory[0].IsReadOnly()) return false;
  if (m_Directory[1].IsVirtualDirectoryRoot() || m_Directory[1].IsReadOnly()) return false;
  return true;
}

bool CGUIWindowFileManager::CanDelete(int iList)
{
  if (m_Directory[iList].IsVirtualDirectoryRoot()) return false;
  if (m_Directory[iList].IsReadOnly()) return false;
  return true;
}

bool CGUIWindowFileManager::CanNewFolder(int iList)
{
  if (m_Directory[iList].IsVirtualDirectoryRoot()) return false;
  if (m_Directory[iList].IsReadOnly()) return false;
  return true;
}

int CGUIWindowFileManager::NumSelected(int iList)
{
  int iSelectedItems = 0;
  for (int iItem = 0; iItem < m_vecItems[iList].Size(); ++iItem)
  {
    if (m_vecItems[iList][iItem]->IsSelected()) iSelectedItems++;
  }
  return iSelectedItems;
}

int CGUIWindowFileManager::GetFocusedList() const
{
  return GetFocusedControl() - CONTROL_LEFT_LIST;
}

void CGUIWindowFileManager::OnPopupMenu(int list, int item)
{
  if (list < 0 || list > 2) return ;
  bool bDeselect = SelectItem(list, item);
  // calculate the position for our menu
  int iPosX = 200;
  int iPosY = 100;
  CGUIListControl *pList = (CGUIListControl *)GetControl(list + CONTROL_LEFT_LIST);
  if (pList)
  {
    iPosX = pList->GetXPosition() + pList->GetWidth() / 2;
    iPosY = pList->GetYPosition() + pList->GetHeight() / 2;
  }
  if (m_Directory[list].IsVirtualDirectoryRoot())
  {
    if (item < 0)
    { // TODO: We should add the option here for shares to be added if there aren't any
      return ;
    }

    // and do the popup menu
    if (CGUIDialogContextMenu::BookmarksMenu("files", m_vecItems[list][item], iPosX, iPosY))
    {
      m_rootDir.SetShares(g_settings.m_vecMyFilesShares);
      if (m_Directory[1 - list].IsVirtualDirectoryRoot())
        Refresh();
      else
        Refresh(list);
      return ;
    }
    m_vecItems[list][item]->Select(false);
    return ;
  }
  if (item >= m_vecItems[list].Size()) return ;
  // popup the context menu
  CGUIDialogContextMenu *pMenu = (CGUIDialogContextMenu *)m_gWindowManager.GetWindow(WINDOW_DIALOG_CONTEXT_MENU);
  if (pMenu)
  {
    bool showEntry=(!m_vecItems[list][item]->IsParentFolder() || (m_vecItems[list][item]->IsParentFolder() && m_vecItems[list].GetSelectedCount()>0));
    // load our menu
    pMenu->Initialize();
    // add the needed buttons
    pMenu->AddButton(188); // SelectAll
    pMenu->AddButton(118); // Rename
    pMenu->AddButton(117); // Delete
    pMenu->AddButton(115); // Copy
    pMenu->AddButton(116); // Move
    pMenu->AddButton(119); // New Folder
    pMenu->AddButton(13393); // Calculate Size
    pMenu->EnableButton(1, item >= 0);
    pMenu->EnableButton(2, item >= 0 && CanRename(list) && !m_vecItems[list][item]->IsParentFolder());
    pMenu->EnableButton(3, item >= 0 && CanDelete(list) && showEntry);
    pMenu->EnableButton(4, item >= 0 && CanCopy(list) && showEntry);
    pMenu->EnableButton(5, item >= 0 && CanMove(list) && showEntry);
    pMenu->EnableButton(6, CanNewFolder(list));
    pMenu->EnableButton(7, item >=0 && m_vecItems[list][item]->m_bIsFolder && !m_vecItems[list][item]->IsParentFolder());
    // position it correctly
    pMenu->SetPosition(iPosX - pMenu->GetWidth() / 2, iPosY - pMenu->GetHeight() / 2);
    pMenu->DoModal(GetID());
    switch (pMenu->GetButton())
    {
    case 1:
      OnSelectAll(list);
      bDeselect=false;
      break;
    case 2:
      OnRename(list);
      break;
    case 3:
      OnDelete(list);
      break;
    case 4:
      OnCopy(list);
      break;
    case 5:
      OnMove(list);
      break;
    case 6:
      OnNewFolder(list);
      break;
    case 7:
      {
        // setup the progress dialog, and show it
        CGUIDialogProgress *progress = (CGUIDialogProgress *)m_gWindowManager.GetWindow(WINDOW_DIALOG_PROGRESS);
        if (progress)
        {
          progress->SetHeading(13394);
          for (int i=0; i < 3; i++)
            progress->SetLine(i, "");
          progress->StartModal(GetID());
        }

        //  Calculate folder size for each selected item
        for (int i=0; i<m_vecItems[list].Size(); ++i)
        {
          CFileItem* pItem=m_vecItems[list][i];
          if (pItem->m_bIsFolder && pItem->IsSelected())
          {
            __int64 folderSize = CalculateFolderSize(pItem->m_strPath, progress);
            if (folderSize >= 0)
            {
              pItem->m_dwSize = folderSize;
              pItem->SetFileSizeLabel();
            }
          }
        }
        if (progress) progress->Close();
      }
      break;
    default:
      break;
    }
    if (bDeselect && item >= 0)
    { // deselect item as we didn't do anything
      m_vecItems[list][item]->Select(false);
    }
  }
}

// Highlights the item in the list under the cursor
// returns true if we should deselect the item, false otherwise
bool CGUIWindowFileManager::SelectItem(int list, int &item)
{
  // get the currently selected item in the list
  item = GetSelectedItem(list);
  // select the item if we need to
  if (!NumSelected(list) && !m_vecItems[list][item]->IsParentFolder())
  {
    m_vecItems[list][item]->Select(true);
    return true;
  }
  return false;
}

// recursively calculates the selected folder size
__int64 CGUIWindowFileManager::CalculateFolderSize(const CStdString &strDirectory, CGUIDialogProgress *pProgress)
{
  if (pProgress)
  { // update our progress control
    pProgress->Progress();
    pProgress->SetLine(1, strDirectory);
    if (pProgress->IsCanceled())
      return -1;
  }
  // start by calculating the size of the files in this folder...
  __int64 totalSize = 0;
  CFileItemList items;
  m_rootDir.GetDirectory(strDirectory, items, false);
  for (int i=0; i < items.Size(); i++)
  {
    if (items[i]->m_bIsFolder && !items[i]->IsParentFolder()) // folder
    {
      __int64 folderSize = CalculateFolderSize(items[i]->m_strPath, pProgress);
      if (folderSize < 0) return -1;
      totalSize += folderSize;
    }
    else // file
      totalSize += items[i]->m_dwSize;
  }
  return totalSize;
}

bool CGUIWindowFileManager::DeleteItem(const CFileItem *pItem)
{
  if (!pItem) return false;
  CLog::Log(LOGDEBUG,"FileManager::DeleteItem: %s",pItem->GetLabel().c_str());

  // prompt user for confirmation of file/folder deletion
  CGUIDialogYesNo* pDialog = (CGUIDialogYesNo*)m_gWindowManager.GetWindow(WINDOW_DIALOG_YES_NO);
  if (pDialog)
  {
    pDialog->SetHeading(122);
    pDialog->SetLine(0, 125);
    pDialog->SetLine(1, CUtil::GetFileName(pItem->m_strPath));
    pDialog->SetLine(2, "");
    pDialog->DoModal(m_gWindowManager.GetActiveWindow());
    if (!pDialog->IsConfirmed()) return false;
  }

  // Create a temporary item list containing the file/folder for deletion
  CFileItem *pItemTemp = new CFileItem(*pItem);
  CFileItemList items;
  items.Add(pItemTemp);

  // grab the real filemanager window, set up the progress bar,
  // and process the delete action
  CGUIWindowFileManager *pFileManager = (CGUIWindowFileManager *)m_gWindowManager.GetWindow(WINDOW_FILES);
  if (pFileManager)
  {
    pFileManager->m_dlgProgress = (CGUIDialogProgress *)m_gWindowManager.GetWindow(WINDOW_DIALOG_PROGRESS);
    pFileManager->ResetProgressBar(false);
    pFileManager->DoProcess(ACTION_DELETE, items, "");
    if (pFileManager->m_dlgProgress) pFileManager->m_dlgProgress->Close();
  }
  return true;
}

void CGUIWindowFileManager::ShowShareErrorMessage(CFileItem* pItem)
{
  if (pItem->m_bIsShareOrDrive)
  {
    int idMessageText=0;
    CURL url(pItem->m_strPath);
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

void CGUIWindowFileManager::OnWindowLoaded()
{
  CGUIWindow::OnWindowLoaded();
  // disable the page spin controls
  CGUIListControl *pControl = (CGUIListControl *)GetControl(CONTROL_LEFT_LIST);
  if (pControl) pControl->SetPageControlVisible(false);
  pControl = (CGUIListControl *)GetControl(CONTROL_RIGHT_LIST);
  if (pControl) pControl->SetPageControlVisible(false);
}

void CGUIWindowFileManager::OnInitWindow()
{
  m_dlgProgress = (CGUIDialogProgress*)m_gWindowManager.GetWindow(WINDOW_DIALOG_PROGRESS);
  if (m_dlgProgress) m_dlgProgress->SetHeading(126);

  for (int i = 0; i < 2; i++)
  {
    Update(i, m_Directory[i].m_strPath);
  }

  CGUIWindow::OnInitWindow();
}

void CGUIWindowFileManager::SetInitialPath(const CStdString &path)
{
  // check for a passed destination path
  CStdString strDestination = path;
  CStdString strFTPTMP = g_localizeStrings.Get(1251); // ShareName: XBOX Autodetection
  if (!g_infoManager.HasAutodetectedXbox())
    m_rootDir.RemoveShareName(strFTPTMP);
  //--------------------------------
  if (g_guiSettings.GetBool("Autodetect.OnOff"))
  {
    if (g_infoManager.HasAutodetectedXbox() && !strDestination.IsEmpty()) // XBOX Autodetection: add a dummy share
    {
      m_rootDir.RemoveShareName(strFTPTMP); // Remove a previos share: to be sure to not have double stuff..
      CShare shareTmp;
      shareTmp.strName.Format(strFTPTMP);
      shareTmp.strPath.Format(strDestination,1);
      m_rootDir.AddShare(shareTmp);
    }
  }
  //--------------------------------

  m_rootDir.SetShares(g_settings.m_vecMyFilesShares);
  if (!strDestination.IsEmpty())
  {
    CLog::Log(LOGINFO, "Attempting to quickpath to: %s", strDestination.c_str());
  }
  // otherwise, is this the first time accessing this window?
  else if (m_Directory[0].m_strPath == "?")
  {
    m_Directory[0].m_strPath = strDestination = g_stSettings.m_szDefaultFiles;
    CLog::Log(LOGINFO, "Attempting to default to: %s", strDestination.c_str());
  }
  // try to open the destination path
  if (!strDestination.IsEmpty())
  {
    // default parameters if the jump fails
    m_Directory[0].m_strPath = "";

    bool bIsBookmarkName = false;
    int iIndex = CUtil::GetMatchingShare(strDestination, g_settings.m_vecMyFilesShares, bIsBookmarkName);
    if (iIndex > -1)
    {
      // set current directory to matching share
      if (bIsBookmarkName)
        m_Directory[0].m_strPath = g_settings.m_vecMyFilesShares[iIndex].strPath;
      else
        m_Directory[0].m_strPath = strDestination;
      CUtil::RemoveSlashAtEnd(m_Directory[0].m_strPath);
      CLog::Log(LOGINFO, "  Success! Opened destination path: %s", strDestination.c_str());
    }
    else
    {
      CLog::Log(LOGERROR, "  Failed! Destination parameter (%s) does not match a valid share!", strDestination.c_str());
    }
  }

  if (m_Directory[1].m_strPath == "?") m_Directory[1].m_strPath = "";
}

void CGUIWindowFileManager::ResetProgressBar(bool showProgress /*= true */)
{
  if (m_dlgProgress)
  {
    m_dlgProgress->SetHeading(126);
    m_dlgProgress->SetLine(0, 0);
    m_dlgProgress->SetLine(1, 0);
    m_dlgProgress->SetLine(2, 0);
    m_dlgProgress->StartModal(GetID());
    m_dlgProgress->SetPercentage(0);
    m_dlgProgress->ShowProgressBar(showProgress);
  }
}