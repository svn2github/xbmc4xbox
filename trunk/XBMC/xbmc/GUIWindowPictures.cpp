
#include "stdafx.h"
#include "GUIWindowPictures.h"
#include "Util.h"
#include "Picture.h"
#include "application.h"
#include "GUIPassword.h"
#include "FileSystem/ZipManager.h"
#include "GUIDialogContextMenu.h"
#include "GUIDialogMediaSource.h"
#include "PlayListFactory.h"

#define CONTROL_BTNVIEWASICONS      2
#define CONTROL_BTNSORTBY           3
#define CONTROL_BTNSORTASC          4
#define CONTROL_LIST               50
#define CONTROL_THUMBS             51
#define CONTROL_LABELFILES         12

#define CONTROL_BTNSLIDESHOW   6
#define CONTROL_BTNSLIDESHOW_RECURSIVE   7
#define CONTROL_SHUFFLE      9


CGUIWindowPictures::CGUIWindowPictures(void)
    : CGUIMediaWindow(WINDOW_PICTURES, "MyPics.xml")
{
  m_thumbLoader.SetObserver(this);
}

CGUIWindowPictures::~CGUIWindowPictures(void)
{
}

bool CGUIWindowPictures::OnMessage(CGUIMessage& message)
{
  switch ( message.GetMessage() )
  {
  case GUI_MSG_WINDOW_DEINIT:
    {
      if (m_thumbLoader.IsLoading())
        m_thumbLoader.StopThread();

      if (message.GetParam1() != WINDOW_SLIDESHOW)
      {
        m_ImageLib.Unload();
      }
    }
    break;

  case GUI_MSG_WINDOW_INIT:
    {
      // check for a passed destination path
      CStdString strDestination = message.GetStringParam();
      if (!strDestination.IsEmpty())
      {
        message.SetStringParam("");
        CLog::Log(LOGINFO, "Attempting to quickpath to: %s", strDestination.c_str());
        m_history.ClearPathHistory();
      }
      // otherwise, is this the first time accessing this window?
      else if (m_vecItems.m_strPath == "?")
      {
        m_vecItems.m_strPath = strDestination = g_stSettings.m_szDefaultPictures;
        CLog::Log(LOGINFO, "Attempting to default to: %s", strDestination.c_str());
      }
      
      for (unsigned int i=0;i<m_rootDir.GetNumberOfShares();++i)
      {
        const CShare& share = m_rootDir[i];
        CURL url(share.strPath);
        if ((url.GetProtocol() != "zip") && (url.GetProtocol() != "rar"))
          continue;

        if (share.strEntryPoint.IsEmpty()) // do not unmount 'normal' rars/zips
          continue;
        
        if (url.GetProtocol() == "zip")
          g_ZipManager.release(share.strPath);
        
        strDestination = share.strEntryPoint;
        m_rootDir.RemoveShare(share.strPath);
      }
      
      // try to open the destination path
      if (!strDestination.IsEmpty())
      {
        // default parameters if the jump fails
        m_vecItems.m_strPath = "";

        bool bIsBookmarkName = false;
        int iIndex = CUtil::GetMatchingShare(strDestination, g_settings.m_vecMyPictureShares, bIsBookmarkName);
        if (iIndex > -1)
        {
          // set current directory to matching share
          if (bIsBookmarkName)
            m_vecItems.m_strPath = g_settings.m_vecMyPictureShares[iIndex].strPath;
          else
            m_vecItems.m_strPath = strDestination;
          CUtil::RemoveSlashAtEnd(m_vecItems.m_strPath);
          CLog::Log(LOGINFO, "  Success! Opened destination path: %s", strDestination.c_str());
        }
        else
        {
          CLog::Log(LOGERROR, "  Failed! Destination parameter (%s) does not match a valid share!", strDestination.c_str());
        }
        SetHistoryForPath(m_vecItems.m_strPath);
      }

      m_dlgProgress = (CGUIDialogProgress*)m_gWindowManager.GetWindow(WINDOW_DIALOG_PROGRESS);
            
      if (message.GetParam1() != WINDOW_SLIDESHOW)
      {
        m_ImageLib.Load();
      }

      if (!CGUIMediaWindow::OnMessage(message))
        return false;

      return true;
    }
    break;

  case GUI_MSG_CLICKED:
    {
      int iControl = message.GetSenderId();
      if (iControl == CONTROL_BTNSLIDESHOW) // Slide Show
      {
        OnSlideShow();
      }
      else if (iControl == CONTROL_BTNSLIDESHOW_RECURSIVE) // Recursive Slide Show
      {
        OnSlideShowRecursive();
      }
      else if (iControl == CONTROL_SHUFFLE)
      {
        g_guiSettings.ToggleBool("slideshow.shuffle");
        g_settings.Save();
      }
      else if (m_viewControl.HasControl(iControl))  // list/thumb control
      {
        int iItem = m_viewControl.GetSelectedItem();
        int iAction = message.GetParam1();

        // iItem is checked for validity inside these routines
        if (iAction == ACTION_CONTEXT_MENU || iAction == ACTION_MOUSE_RIGHT_CLICK)
        {
          OnPopupMenu(iItem);
        }
        else if (iAction == ACTION_DELETE_ITEM)
        {
          // is delete allowed?
          if (g_guiSettings.GetBool("filelists.allowfiledeletion"))
            OnDeleteItem(iItem);
          else
            return false;
        }
      }
    }
    break;
  }
  return CGUIMediaWindow::OnMessage(message);
}

void CGUIWindowPictures::UpdateButtons()
{
  CGUIMediaWindow::UpdateButtons();

  // Update the shuffle button
  if (g_guiSettings.GetBool("slideshow.shuffle"))
  {
    CGUIMessage msg2(GUI_MSG_SELECTED, GetID(), CONTROL_SHUFFLE, 0, 0, NULL);
    g_graphicsContext.SendMessage(msg2);
  }
  else
  {
    CGUIMessage msg2(GUI_MSG_DESELECTED, GetID(), CONTROL_SHUFFLE, 0, 0, NULL);
    g_graphicsContext.SendMessage(msg2);
  }

  // check we can slideshow or recursive slideshow
  int nFolders = m_vecItems.GetFolderCount();
  if (nFolders == m_vecItems.Size())
  {
    CONTROL_DISABLE(CONTROL_BTNSLIDESHOW);
  }
  else
  {
    CONTROL_ENABLE(CONTROL_BTNSLIDESHOW);
  }
  if (m_guiState.get() && !m_guiState->HideParentDirItems())
    nFolders--;
  if (m_vecItems.Size() == 0 || nFolders == 0)
  {
    CONTROL_DISABLE(CONTROL_BTNSLIDESHOW_RECURSIVE);
  }
  else
  {
    CONTROL_ENABLE(CONTROL_BTNSLIDESHOW_RECURSIVE);
  }
}

bool CGUIWindowPictures::Update(const CStdString &strDirectory)
{
  if (m_thumbLoader.IsLoading())
    m_thumbLoader.StopThread();

  if (!CGUIMediaWindow::Update(strDirectory))
    return false;

  m_thumbLoader.Load(m_vecItems);

  return true;
}

bool CGUIWindowPictures::OnClick(int iItem)
{
  if ( iItem < 0 || iItem >= (int)m_vecItems.Size() ) return true;
  CFileItem* pItem = m_vecItems[iItem];
  CStdString strPath = pItem->m_strPath;

  if (pItem->IsCBZ()) //mount'n'show'n'unmount
  {
    CUtil::GetDirectory(pItem->m_strPath,strPath);
    CShare shareZip;
    shareZip.strPath.Format("zip://Z:\\filesrar\\,%i,,%s,\\",1, pItem->m_strPath.c_str() );
    shareZip.strEntryPoint = strPath;
    m_rootDir.AddShare(shareZip);
    Update(shareZip.strPath);
    if (m_vecItems.Size() > 0)
    {
      CStdString strEmpty; strEmpty.Empty();
      OnShowPictureRecursive(strEmpty);
    }
    else
    {
      CLog::Log(LOGERROR,"No pictures found in cbz file!");
      m_rootDir.RemoveShare(shareZip.strPath);
      Update(strPath);
    }
    m_iSelectedItem = iItem;

    return true;
  }
  else if (pItem->IsCBR()) // mount'n'show'n'unmount
  {
    CUtil::GetDirectory(pItem->m_strPath,strPath);
    CShare shareRar;
    shareRar.strPath.Format("rar://Z:\\,%i,,%s,\\",EXFILE_AUTODELETE,pItem->m_strPath.c_str() );
    shareRar.strEntryPoint = strPath;
    m_rootDir.AddShare(shareRar);
    m_iSelectedItem = -1;
    Update(shareRar.strPath);
    if (m_vecItems.Size() > 0)
    {
      CStdString strEmpty; strEmpty.Empty();
      OnShowPictureRecursive(strEmpty);
    }
    else
    {
      CLog::Log(LOGERROR,"No pictures found in cbr file!");
      m_rootDir.RemoveShare(shareRar.strPath);
      Update(strPath);
    }
    m_iSelectedItem = iItem;

    return true;
  }
  else if (CGUIMediaWindow::OnClick(iItem))
    return true;

  return false;
}

bool CGUIWindowPictures::OnPlayMedia(int iItem)
{
  if ( iItem < 0 || iItem >= (int)m_vecItems.Size() ) return false;
  CFileItem* pItem = m_vecItems[iItem];
  CStdString strPicture = pItem->m_strPath;
  
  if (pItem->m_strPath == "add" && pItem->GetLabel() == g_localizeStrings.Get(1026)) // 'add source button' in empty root
  {
    if (CGUIDialogMediaSource::ShowAndAddMediaSource("pictures"))
    {
      Update("");
      return true;
    }
    return false;
  }

  CGUIWindowSlideShow *pSlideShow = (CGUIWindowSlideShow *)m_gWindowManager.GetWindow(WINDOW_SLIDESHOW);
  if (!pSlideShow)
    return false;
  if (g_application.IsPlayingVideo())
    g_application.StopPlaying();

  pSlideShow->Reset();
  for (int i = 0; i < (int)m_vecItems.Size();++i)
  {
    CFileItem* pItem = m_vecItems[i];
    if (!pItem->m_bIsFolder && !(CUtil::IsRAR(pItem->m_strPath) || CUtil::IsZIP(pItem->m_strPath)))
    {
      pSlideShow->Add(pItem->m_strPath);
    }
  }
  pSlideShow->Select(strPicture);
  m_gWindowManager.ActivateWindow(WINDOW_SLIDESHOW);

  return true;
}

void CGUIWindowPictures::OnShowPictureRecursive(const CStdString& strPicture)
{
  CGUIWindowSlideShow *pSlideShow = (CGUIWindowSlideShow *)m_gWindowManager.GetWindow(WINDOW_SLIDESHOW);
  if (!pSlideShow)
    return ;
  if (g_application.IsPlayingVideo())
    g_application.StopPlaying();

  pSlideShow->Reset();
  for (int i = 0; i < (int)m_vecItems.Size();++i)
  {
    CFileItem* pItem = m_vecItems[i];
    if (pItem->m_bIsFolder)
      AddDir(pSlideShow, pItem->m_strPath);
    else if (!(CUtil::IsRAR(pItem->m_strPath) || CUtil::IsZIP(pItem->m_strPath)))
      pSlideShow->Add(pItem->m_strPath);
  }
  if (!strPicture.IsEmpty())
    pSlideShow->Select(strPicture);
  m_gWindowManager.ActivateWindow(WINDOW_SLIDESHOW);
}

void CGUIWindowPictures::AddDir(CGUIWindowSlideShow *pSlideShow, const CStdString& strPath)
{
  if (!pSlideShow) return ;
  CFileItemList items;
  m_rootDir.GetDirectory(strPath, items);
  SortItems(items);

  for (int i = 0; i < (int)items.Size();++i)
  {
    CFileItem* pItem = items[i];
    if (pItem->m_bIsFolder)
      AddDir(pSlideShow, pItem->m_strPath);
    else if (!(CUtil::IsRAR(pItem->m_strPath) || CUtil::IsZIP(pItem->m_strPath)))
      pSlideShow->Add(pItem->m_strPath);
  }
}

void CGUIWindowPictures::OnSlideShowRecursive(const CStdString &strPicture)
{
  CGUIWindowSlideShow *pSlideShow = (CGUIWindowSlideShow *)m_gWindowManager.GetWindow(WINDOW_SLIDESHOW);
  if (!pSlideShow)
    return ;

  if (g_application.IsPlayingVideo())
    g_application.StopPlaying();

  pSlideShow->Reset();
  AddDir(pSlideShow, m_vecItems.m_strPath);
  pSlideShow->StartSlideShow();
  if (!strPicture.IsEmpty())
    pSlideShow->Select(strPicture);
  if (pSlideShow->NumSlides())
    m_gWindowManager.ActivateWindow(WINDOW_SLIDESHOW);
}

void CGUIWindowPictures::OnSlideShowRecursive()
{
  CStdString strEmpty = "";
  OnSlideShowRecursive(strEmpty);
}

void CGUIWindowPictures::OnSlideShow()
{
  CStdString strEmpty = "";
  OnSlideShow(strEmpty);
}

void CGUIWindowPictures::OnSlideShow(const CStdString &strPicture)
{
  CGUIWindowSlideShow *pSlideShow = (CGUIWindowSlideShow *)m_gWindowManager.GetWindow(WINDOW_SLIDESHOW);
  if (!pSlideShow)
    return ;
  if (g_application.IsPlayingVideo())
    g_application.StopPlaying();

  pSlideShow->Reset();
  for (int i = 0; i < (int)m_vecItems.Size();++i)
  {
    CFileItem* pItem = m_vecItems[i];
    if (!pItem->m_bIsFolder && !(CUtil::IsRAR(pItem->m_strPath) || CUtil::IsZIP(pItem->m_strPath)))
    {
      pSlideShow->Add(pItem->m_strPath);
    }
  }
  pSlideShow->StartSlideShow();
  if (!strPicture.IsEmpty())
    pSlideShow->Select(strPicture);
  if (pSlideShow->NumSlides())
    m_gWindowManager.ActivateWindow(WINDOW_SLIDESHOW);
}

void CGUIWindowPictures::OnRegenerateThumbs()
{
  if (m_thumbLoader.IsLoading()) return;
  m_thumbLoader.SetRegenerateThumbs(true);
  m_thumbLoader.Load(m_vecItems);
}

void CGUIWindowPictures::OnPopupMenu(int iItem)
{
  // calculate our position
  int iPosX = 200, iPosY = 100;
  const CGUIControl *pList = GetControl(CONTROL_LIST);
  if (pList)
  {
    iPosX = pList->GetXPosition() + pList->GetWidth() / 2;
    iPosY = pList->GetYPosition() + pList->GetHeight() / 2;
  }
  if ( m_vecItems.IsVirtualDirectoryRoot() )
  {
    if (iItem < 0)
    { // we should add the option here of adding shares
      return ;
    }
    // mark the item
    m_vecItems[iItem]->Select(true);
    // and do the popup menu
    if (CGUIDialogContextMenu::BookmarksMenu("pictures", m_vecItems[iItem], iPosX, iPosY))
    {
      Update(m_vecItems.m_strPath);
      return ;
    }
    m_vecItems[iItem]->Select(false);
  }
  else if (iItem >= 0)
  {
    // mark the item
    m_vecItems[iItem]->Select(true);
    // popup the context menu
    CGUIDialogContextMenu *pMenu = (CGUIDialogContextMenu *)m_gWindowManager.GetWindow(WINDOW_DIALOG_CONTEXT_MENU);
    if (!pMenu) return ;
    // load our menu
    pMenu->Initialize();
    // add the needed buttons
    
    int btn_Thumbs       = 0; // Create Thumbnails
    int btn_SlideShow    = 0; // View Slideshow
    int btn_RecSlideShow = 0; // Recursive Slideshow

    // this could be done like the delete button too
    if (m_vecItems.GetFileCount() != 0) 
      btn_SlideShow = pMenu->AddButton(13317);      // View Slideshow
    
    btn_RecSlideShow = pMenu->AddButton(13318);     // Recursive Slideshow
    
    if (!m_thumbLoader.IsLoading()) 
      btn_Thumbs = pMenu->AddButton(13315);         // Create Thumbnails
    
    int btn_Delete = 0, btn_Rename = 0;             // Delete and Rename
    if (g_guiSettings.GetBool("filelists.allowfiledeletion"))
    {
      btn_Delete = pMenu->AddButton(117);           // Delete
      btn_Rename = pMenu->AddButton(118);           // Rename
      // disable these functions if not supported by the protocol
      if (!CUtil::SupportsFileOperations(m_vecItems[iItem]->m_strPath))
      {
        pMenu->EnableButton(btn_Delete, false);
        pMenu->EnableButton(btn_Rename, false);
      }
    }
    //int btn_Settings = -2;
    int btn_Settings = pMenu->AddButton(5);         // Settings

    // position it correctly
    pMenu->SetPosition(iPosX - pMenu->GetWidth() / 2, iPosY - pMenu->GetHeight() / 2);
    pMenu->DoModal();

    int btnid = pMenu->GetButton();

    if (btnid>0)
    {
      if (btnid == btn_SlideShow)
      {
        OnSlideShow(m_vecItems[iItem]->m_strPath);
        return;
      }
      else if (btnid == btn_RecSlideShow)
      {
        OnSlideShowRecursive(m_vecItems[iItem]->m_strPath);
        return;
      }
      else if (btnid == btn_Thumbs)
      {
        OnRegenerateThumbs();
      }
      else if (btnid == btn_Delete)
      {
        OnDeleteItem(iItem);
      }
      //Rename
      else if (btnid == btn_Rename)
      {
        OnRenameItem(iItem);
      }
      else if (btnid == btn_Settings)
      { 
        m_gWindowManager.ActivateWindow(WINDOW_SETTINGS_MYPICTURES);
      }
    }
    if (iItem < m_vecItems.Size())
      m_vecItems[iItem]->Select(false);
  }
}

void CGUIWindowPictures::OnItemLoaded(CFileItem *pItem)
{
  if (pItem->m_bIsFolder && !pItem->m_bIsShareOrDrive && !pItem->HasThumbnail() && !pItem->IsParentFolder())
  {
    // first check for a folder.jpg
    CStdString thumb;
    CUtil::AddFileToFolder(pItem->m_strPath, "folder.jpg", thumb);
    if (CFile::Exists(thumb))
    {
      CPicture pic;
      pic.DoCreateThumbnail(thumb, pItem->GetCachedPictureThumb());
    }
    else
    {
      // we load the directory, grab 4 random thumb files (if available) and then generate
      // the thumb.

      CFileItemList items;
      CDirectory::GetDirectory(pItem->m_strPath, items, g_stSettings.m_pictureExtensions, false, false);

      // create the folder thumb by choosing 4 random thumbs within the folder and putting
      // them into one thumb.
      // count the number of images
      for (int i=0; i < items.Size();)
      {
        if (!items[i]->IsPicture() || items[i]->IsZIP() || items[i]->IsRAR())
          items.Remove(i);
        else
          i++;
      }

      if (items.IsEmpty())
        return; // no images in this folder

      // randomize them
      items.Randomize();

      if (items.Size() < 4)
      { // less than 4 items, so just grab a single random thumb
        CStdString folderThumb(pItem->GetCachedPictureThumb());
        CPicture pic;
        pic.DoCreateThumbnail(items[0]->m_strPath, folderThumb);
      }
      else
      {
        // ok, now we've got the files to get the thumbs from, lets create it...
        // we basically load the 4 thumbs, resample to 62x62 pixels, and add them
        CStdString strFiles[4];
        for (int thumb = 0; thumb < 4; thumb++)
          strFiles[thumb] = items[thumb]->m_strPath;
        CPicture pic;
        pic.CreateFolderThumb(strFiles, pItem->GetCachedPictureThumb());
      }
    }
    // refill in the icon to get it to update
    pItem->SetCachedPictureThumb();
    pItem->FillInDefaultIcon();
  }
}

void CGUIWindowPictures::LoadPlayList(const CStdString& strPlayList)
{
  CLog::Log(LOGDEBUG,"CGUIWindowPictures::LoadPlayList()... converting playlist into slideshow: %s", strPlayList.c_str());
  CPlayListFactory factory;
  auto_ptr<CPlayList> pPlayList (factory.Create(strPlayList));
  if ( NULL != pPlayList.get())
  {
    if (!pPlayList->Load(strPlayList))
    {
      CGUIDialogOK::ShowAndGetInput(6, 0, 477, 0);
      return ; //hmmm unable to load playlist?
    }
  }

  CPlayList playlist = *pPlayList;
  if (playlist.size() > 0)
  {
    // set up slideshow
    CGUIWindowSlideShow *pSlideShow = (CGUIWindowSlideShow *)m_gWindowManager.GetWindow(WINDOW_SLIDESHOW);
    if (!pSlideShow)
      return;
    if (g_application.IsPlayingVideo())
      g_application.StopPlaying();

    // convert playlist items into slideshow items
    pSlideShow->Reset();
    for (int i = 0; i < (int)playlist.size(); ++i)
    {
      CFileItem *pItem = new CFileItem(playlist[i].m_strPath, false);
      //CLog::Log(LOGDEBUG,"-- playlist item: %s", pItem->m_strPath.c_str());
      if (pItem->IsPicture() && !(pItem->IsZIP() || pItem->IsRAR() || pItem->IsCBZ() || pItem->IsCBR()))
        pSlideShow->Add(pItem->m_strPath);
    }

    // start slideshow if there are items
    pSlideShow->StartSlideShow();
    if (pSlideShow->NumSlides())
      m_gWindowManager.ActivateWindow(WINDOW_SLIDESHOW);
  }
}