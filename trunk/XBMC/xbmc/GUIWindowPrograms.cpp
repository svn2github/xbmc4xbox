#include "stdafx.h"
#include "GUIWindowPrograms.h"
#include "util.h"
#include "Xbox\IoSupport.h"
#include "Xbox\Undocumented.h"
#include "Shortcut.h"
#include "filesystem/HDDirectory.h"
#include "filesystem/directorycache.h"
#include "GUIThumbnailPanel.h"
#include "GUIPassword.h"
#include "GUIDialogContextMenu.h"
#include "GUIDialogMediaSource.h"
#include "GUIDialogTrainerSettings.h"
#include "GUIDialogFileBrowser.h"
#include "xbox/xbeheader.h"
#include "SortFileItem.h"
#include "utils/Trainer.h"
#include "utils/kaiclient.h"

using namespace DIRECTORY;

#define CONTROL_BTNVIEWASICONS 2
#define CONTROL_BTNSORTBY      3
#define CONTROL_BTNSORTASC     4
#define CONTROL_LIST          50
#define CONTROL_THUMBS        51
#define CONTROL_LABELFILES    12

#define CONTROL_BTNSCAN       6

CGUIWindowPrograms::CGUIWindowPrograms(void)
    : CGUIMediaWindow(WINDOW_PROGRAMS, "MyPrograms.xml")
{
  m_thumbLoader.SetObserver(this);
}


CGUIWindowPrograms::~CGUIWindowPrograms(void)
{
}

bool CGUIWindowPrograms::OnMessage(CGUIMessage& message)
{
  switch ( message.GetMessage() )
  {
  case GUI_MSG_WINDOW_DEINIT:
    {
      if (m_thumbLoader.IsLoading())
        m_thumbLoader.StopThread();
      m_database.Close();
    }
    break;

  case GUI_MSG_WINDOW_INIT:
    {
      m_iRegionSet = 0;
      m_dlgProgress = (CGUIDialogProgress*)m_gWindowManager.GetWindow(WINDOW_DIALOG_PROGRESS);

      // remove shortcuts
      if (g_guiSettings.GetBool("MyPrograms.NoShortcuts") && g_stSettings.m_szShortcutDirectory[0] && g_settings.m_vecMyProgramsBookmarks[0].strName.Equals("shortcuts"))
        g_settings.m_vecMyProgramsBookmarks.erase(g_settings.m_vecMyProgramsBookmarks.begin());

      if (g_advancedSettings.m_newMyPrograms)
      {
        // check for a passed destination path
        CStdString strDestination = message.GetStringParam();
        if (!strDestination.IsEmpty())
        {
          message.SetStringParam("");
          CLog::Log(LOGINFO, "Attempting to quickpath to: %s", strDestination.c_str());
          // reset directory path, as we have effectively cleared it here
          m_history.ClearPathHistory();
        }
        // is this the first time accessing this window?
        // a quickpath overrides the a default parameter
        if (m_vecItems.m_strPath == "?" && strDestination.IsEmpty())
        {
          m_vecItems.m_strPath = strDestination = g_stSettings.m_szDefaultVideos;
          CLog::Log(LOGINFO, "Attempting to default to: %s", strDestination.c_str());
        }

        m_database.Open();
        // try to open the destination path
        if (!strDestination.IsEmpty())
        {
          // open playlists location
          if (strDestination.Equals("$PLAYLISTS"))
          {
            m_vecItems.m_strPath = CUtil::VideoPlaylistsLocation();
            CLog::Log(LOGINFO, "  Success! Opening destination path: %s", m_vecItems.m_strPath.c_str());
          }
          else
          {
            // default parameters if the jump fails
            m_vecItems.m_strPath = "";

            bool bIsBookmarkName = false;
            int iIndex = CUtil::GetMatchingShare(strDestination, g_settings.m_vecMyVideoShares, bIsBookmarkName);
            if (iIndex > -1)
            {
              // set current directory to matching share
              if (bIsBookmarkName)
                m_vecItems.m_strPath = g_settings.m_vecMyVideoShares[iIndex].strPath;
              else
                m_vecItems.m_strPath = strDestination;
              CUtil::RemoveSlashAtEnd(m_vecItems.m_strPath);
              CLog::Log(LOGINFO, "  Success! Opened destination path: %s", strDestination.c_str());
            }
            else
            {
              CLog::Log(LOGERROR, "  Failed! Destination parameter (%s) does not match a valid share!", strDestination.c_str());
            }
          }

          // need file filters or GetDirectory in SetHistoryPath fails
          m_rootDir.SetMask(".xbe|.cut");
          m_rootDir.SetShares(g_settings.m_vecMyProgramsBookmarks);
          SetHistoryForPath(m_vecItems.m_strPath);
        }
        m_rootDir.SetMask(".xbe|.cut");
        m_rootDir.SetShares(g_settings.m_vecMyProgramsBookmarks);
        return CGUIMediaWindow::OnMessage(message);
      }

      int iLastControl = m_iLastControl;
      CGUIWindow::OnMessage(message);
      // check for a passed destination path
      CStdString strDestination = message.GetStringParam();
      if (!strDestination.IsEmpty())
      {
        message.SetStringParam("");
        CLog::Log(LOGINFO, "Attempting to quickpath to: %s", strDestination.c_str());
      }
      // otherwise, is this the first time accessing this window?
      else if (m_vecItems.m_strPath == "?")
      {
        m_vecItems.m_strPath = strDestination = g_stSettings.m_szDefaultPrograms;
        CLog::Log(LOGINFO, "Attempting to default to: %s", strDestination.c_str());
      }
      // make controls 100-110 invisible...
      for (int i = 100; i < 110; i++)
      {
        SET_CONTROL_HIDDEN(i);
      }

      m_rootDir.SetShares(g_settings.m_vecMyProgramsBookmarks);
      
      if (g_guiSettings.GetBool("MyPrograms.NoShortcuts"))    // let's hide Scan button
      {
        SET_CONTROL_HIDDEN(CONTROL_BTNSCAN);
      }
      else
      {
        SET_CONTROL_VISIBLE(CONTROL_BTNSCAN);
      }


      int iStartID = 100;

      // create bookmark buttons

      for (int i = 0; i < (int)g_settings.m_vecMyProgramsBookmarks.size(); ++i)
      {
        CShare& share = g_settings.m_vecMyProgramsBookmarks[i];

        SET_CONTROL_VISIBLE(i + iStartID);

        if (CUtil::IsDVD(share.strPath))
        {
          CStdString strName = share.strName.substr(0,share.strName.rfind("(")-1);
          SET_CONTROL_LABEL(i + iStartID, strName);
        }
        else
          SET_CONTROL_LABEL(i + iStartID, share.strName);
      }

      m_database.Open();
      // try to open the destination path
      if (!strDestination.IsEmpty())
      {
        // default parameters if the jump fails
        m_vecItems.m_strPath = "";
        m_shareDirectory = "";
        m_iDepth = 1;
        m_strBookmarkName = "default";
        m_database.GetPathsByBookmark(m_strBookmarkName, m_vecPaths);

        bool bIsBookmarkName = false;
        int iIndex = CUtil::GetMatchingShare(strDestination, g_settings.m_vecMyProgramsBookmarks, bIsBookmarkName);
        if (iIndex > -1)
        {
          // set current directory to matching share
          if (bIsBookmarkName)
          {
            m_vecItems.m_strPath = g_settings.m_vecMyProgramsBookmarks[iIndex].strPath;
            m_history.SetSelectedItem(m_vecItems.m_strPath,"empty");
          }
          else
            m_vecItems.m_strPath = strDestination;
          CUtil::RemoveSlashAtEnd(m_vecItems.m_strPath);
          m_shareDirectory = m_vecItems.m_strPath;
          m_iDepth = g_settings.m_vecMyProgramsBookmarks[iIndex].m_iDepthSize;
          m_strBookmarkName = g_settings.m_vecMyProgramsBookmarks[iIndex].strName;
          CLog::Log(LOGINFO, "  Success! Opened destination path: %s", strDestination.c_str());
        }
        else
        {
          CLog::Log(LOGERROR, "  Failed! Destination parameter (%s) does not match a valid share!", strDestination.c_str());
        }
      }

      m_vecPaths.clear();
      m_database.GetPathsByBookmark(m_strBookmarkName, m_vecPaths);

      Update(m_vecItems.m_strPath); 	 
	  	 
      if (iLastControl > -1) 	 
      { 	 
        SET_CONTROL_FOCUS(iLastControl, 0); 	 
      } 	 
      else 	 
      { 	 
        SET_CONTROL_FOCUS(m_dwDefaultFocusControlID, 0);
      }

      if (m_iSelectedItem > -1) 	 
        m_viewControl.SetSelectedItem(m_iSelectedItem); 	 

      return true;
    }
    break;

  case GUI_MSG_CLICKED:
    {
      int iControl = message.GetSenderId();
      if (iControl == CONTROL_BTNSCAN) // button
      {
        int iTotalApps = 0;
        CStdString strDir = m_vecItems.m_strPath;
        m_vecItems.m_strPath = g_stSettings.m_szShortcutDirectory;

        if (m_dlgProgress)
        {
          m_dlgProgress->SetHeading(211);
          m_dlgProgress->SetLine(0, "");
          m_dlgProgress->SetLine(1, "");
          m_dlgProgress->SetLine(2, "");
          m_dlgProgress->StartModal(GetID());
          m_dlgProgress->Progress();
        }

        CHDDirectory rootDir;

        // remove shortcuts...
        rootDir.SetMask(".cut");
        rootDir.GetDirectory(m_vecItems.m_strPath, m_vecItems);

        for (int i = 0; i < (int)m_vecItems.Size(); ++i)
        {
          CFileItem* pItem = m_vecItems[i];
          if (pItem->IsShortCut())
          {
            DeleteFile(pItem->m_strPath.c_str());
          }
        }

        // create new ones.

        CFileItemList items;
        {
          m_vecItems.m_strPath = "C:";
          rootDir.SetMask(".xbe");
          rootDir.GetDirectory("C:\\", items);
          OnScan(items, iTotalApps);
          items.Clear();
        }

        {
          m_vecItems.m_strPath = "E:";
          rootDir.SetMask(".xbe");
          rootDir.GetDirectory("E:\\", items);
          OnScan(items, iTotalApps);
          items.Clear();
        }

        if (g_advancedSettings.m_useFDrive)
        {
          m_vecItems.m_strPath = "F:";
          rootDir.SetMask(".xbe");
          rootDir.GetDirectory("F:\\", items);
          OnScan(items, iTotalApps);
          items.Clear();
        }
        if (g_advancedSettings.m_useGDrive)
        {
          m_vecItems.m_strPath = "G:";
          rootDir.SetMask(".xbe");
          rootDir.GetDirectory("G:\\", items);
          OnScan(items, iTotalApps);
          items.Clear();
        }


        m_vecItems.m_strPath = strDir;
        CUtil::ClearCache();
        Update(m_vecItems.m_strPath);

        if (m_dlgProgress)
        {
          m_dlgProgress->Close();
        }
      }
      else if (m_viewControl.HasControl(iControl))  // list/thumb control
      {
        // get selected item
        int iAction = message.GetParam1();
        if (ACTION_CONTEXT_MENU == iAction)
        {
          int iItem = m_viewControl.GetSelectedItem();
          // iItem is checked inside OnPopupMenu
          if (OnPopupMenu(iItem))
          { // update bookmark buttons
            int iStartID = 100;
            for (int i = 0; i < (int)g_settings.m_vecMyProgramsBookmarks.size(); ++i)
            {
              CShare& share = g_settings.m_vecMyProgramsBookmarks[i];
              if (CUtil::IsDVD(share.strPath))
              {
                CStdString strName = share.strName.substr(0,share.strName.rfind("(")-1);
                SET_CONTROL_LABEL(i + iStartID, strName);
              }
              else
                SET_CONTROL_LABEL(i + iStartID, share.strName);
            }
            // erase the button following the last updated button
            // incase the user just deleted a bookmark
            SET_CONTROL_HIDDEN((int)g_settings.m_vecMyProgramsBookmarks.size() + iStartID);
            SET_CONTROL_LABEL((int)g_settings.m_vecMyProgramsBookmarks.size() + iStartID, "");
          }
        }
      }
      else if (iControl >= 100 && iControl <= 110)
      {
        // bookmark button
        int iShare = iControl - 100;
        if (iShare < (int)g_settings.m_vecMyProgramsBookmarks.size())
        {
          CShare share = g_settings.m_vecMyProgramsBookmarks[iControl - 100];
          // do nothing if the bookmark is locked, and update the panel with new bookmark settings
          if ( !g_passwordManager.IsItemUnlocked( &share, "myprograms" ) )
          {
            //Update(""); //Does not update clean
            GoParentFolder(); //The best way is to go back to root
            return false;
          }
          m_shareDirectory = share.strPath;    // since m_strDirectory can change, we always want something that won't.
          m_vecItems.m_strPath = share.strPath;
          m_strBookmarkName = share.strName;
          m_database.GetPathsByBookmark(m_strBookmarkName, m_vecPaths);  // fetch all paths in this bookmark
          m_iDepth = share.m_iDepthSize;
          Update(m_vecItems.m_strPath);
        }
      }
    }
    break;
  }

  return CGUIMediaWindow::OnMessage(message);
}

bool CGUIWindowPrograms::OnPopupMenu(int iItem)
{
  // calculate our position
  int iPosX = 200;
  int iPosY = 100;
  const CGUIControl *pList = GetControl(CONTROL_LIST);
  if (pList)
  {
    iPosX = pList->GetXPosition() + pList->GetWidth() / 2;
    iPosY = pList->GetYPosition() + pList->GetHeight() / 2;
  }
  if ( m_vecItems.IsVirtualDirectoryRoot() )
  {
    if (iItem < 0)
    { // TODO: Add option to add shares in this case
      return false;
    }
    // mark the item
    m_vecItems[iItem]->Select(true);

    // and do the popup menu
    if (CGUIDialogContextMenu::BookmarksMenu("myprograms", m_vecItems[iItem], iPosX, iPosY))
    {
	    m_history.SetSelectedItem(m_vecItems[iItem]->m_strPath,"empty");
	    Update("");
      return true;
    }
    m_vecItems[iItem]->Select(false);
    return false;
  }
  else if (iItem >= 0)// assume a program
  {
    // mark the item
    m_vecItems[iItem]->Select(true);
    // popup the context menu
    CGUIDialogContextMenu *pMenu = (CGUIDialogContextMenu *)m_gWindowManager.GetWindow(WINDOW_DIALOG_CONTEXT_MENU);
    if (!pMenu) return false;
    // load our menu
    pMenu->Initialize();
    // add the needed buttons
    
    int btn_Launch = -2;
    int btn_Rename = -2;
    int btn_LaunchIn = -2;
    int btn_Trainers = -2;
    if (m_vecItems[iItem]->IsXBE() || m_vecItems[iItem]->IsShortCut())
    {
      CStdString strLaunch = g_localizeStrings.Get(518); // Launch
      if (g_guiSettings.GetBool("MyPrograms.GameAutoRegion"))
      {
        int iRegion = GetRegion(iItem);
        if (iRegion == VIDEO_NTSCM)
          strLaunch += " (NTSC-M)";
        if (iRegion == VIDEO_NTSCJ)
          strLaunch += " (NTSC-J)";
        if (iRegion == VIDEO_PAL50)
          strLaunch += " (PAL)";
        if (iRegion == VIDEO_PAL60)
          strLaunch += " (PAL-60)";
      }
      btn_Launch = pMenu->AddButton(strLaunch); // launch

      if (g_guiSettings.GetBool("MyPrograms.GameAutoRegion"))
        btn_LaunchIn = pMenu->AddButton(519); // launch in video mode

      btn_Rename = pMenu->AddButton(520); // edit xbe title
      DWORD dwTitleId = CUtil::GetXbeID(m_vecItems[iItem]->m_strPath);
      if (m_database.ItemHasTrainer(dwTitleId))
      {
        CStdString strOptions = g_localizeStrings.Get(12015);
        btn_Trainers = pMenu->AddButton(strOptions); // trainer options
      }
    }
    int btn_ScanTrainers = pMenu->AddButton(12012);
    int btn_UpdateDb = pMenu->AddButton(12387);
    int btn_Settings = pMenu->AddButton(5); // Settings

    // position it correctly
    pMenu->SetPosition(iPosX - pMenu->GetWidth() / 2, iPosY - pMenu->GetHeight() / 2);
    pMenu->DoModal(GetID());

    int btnid = pMenu->GetButton();
    if (!btnid)
    {
      m_vecItems[iItem]->Select(false);
      return false;
    }

    if (btnid == btn_Rename)
    {
      CStdString strDescription = m_vecItems[iItem]->GetLabel();
      if (CGUIDialogKeyboard::ShowAndGetInput(strDescription, g_localizeStrings.Get(16013), false))
      {
        // SetXBEDescription will truncate to 40 characters.
        CUtil::SetXBEDescription(m_vecItems[iItem]->m_strPath,strDescription);
        m_database.SetDescription(m_vecItems[iItem]->m_strPath,strDescription);
        Update(m_vecItems.m_strPath);
      }
    }
    if (btnid == btn_Trainers)
    {
      DWORD dwTitleId = CUtil::GetXbeID(m_vecItems[iItem]->m_strPath);
      if (CGUIDialogTrainerSettings::ShowForTitle(dwTitleId,&m_database))
      {
        SetOverlayIcons();
        m_guiState.reset(CGUIViewState::GetViewState(GetID(), m_vecItems));
        OnSort();
        UpdateButtons();
      }
    }
    if (btnid == btn_ScanTrainers)
    {
      PopulateTrainersList();
      SetOverlayIcons();
      m_guiState.reset(CGUIViewState::GetViewState(GetID(), m_vecItems));
      OnSort();
      UpdateButtons();
    }
    if (btnid == btn_Settings)
    { 
      if (g_passwordManager.bMasterLockSettings && g_passwordManager.IsMasterLockLocked(true)) 
          m_gWindowManager.ActivateWindow(WINDOW_SETTINGS_MYPROGRAMS);
    }
    else if (btnid == btn_Launch)
    {
      OnClick(iItem);
      return true;
    }
    else if (btnid == btn_LaunchIn)
    {
      pMenu->Initialize();
      int btn_PAL;
      int btn_NTSCM;
      int btn_NTSCJ;
      int btn_PAL60;
      CStdString strPAL, strNTSCJ, strNTSCM, strPAL60;
      strPAL = "PAL";
      strNTSCM = "NTSC-M";
      strNTSCJ = "NTSC-J";
      strPAL60 = "PAL-60";
      int iRegion = GetRegion(iItem,true);

      if (iRegion == VIDEO_NTSCM)
        strNTSCM += " (default)";
      if (iRegion == VIDEO_NTSCJ)
        strNTSCJ += " (default)";
      if (iRegion == VIDEO_PAL50)
        strPAL += " (default)";

      btn_PAL = pMenu->AddButton(strPAL);
      btn_NTSCM = pMenu->AddButton(strNTSCM);
      btn_NTSCJ = pMenu->AddButton(strNTSCJ);
      btn_PAL60 = pMenu->AddButton(strPAL60);
      
      pMenu->SetPosition(iPosX - pMenu->GetWidth() / 2, iPosY - pMenu->GetHeight() / 2);
      pMenu->DoModal(GetID());
      int btnid = pMenu->GetButton();
      
      if (btnid == btn_NTSCM)
      {
        m_iRegionSet = VIDEO_NTSCM;
        m_database.SetRegion(m_vecItems[iItem]->m_strPath,1);
      }
      if (btnid == btn_NTSCJ)
      {
        m_iRegionSet = VIDEO_NTSCJ;
        m_database.SetRegion(m_vecItems[iItem]->m_strPath,2);
      }
      if (btnid == btn_PAL)
      {
        m_iRegionSet = VIDEO_PAL50;
        m_database.SetRegion(m_vecItems[iItem]->m_strPath,4);
      }
      if (btnid == btn_PAL60)
      {
        m_iRegionSet = VIDEO_PAL60;
        m_database.SetRegion(m_vecItems[iItem]->m_strPath,8);
      }

      if (btnid > -1)
        OnClick(iItem);
    }
    else if (btnid == btn_UpdateDb)
    {
      if (!m_dlgProgress)
        m_dlgProgress = (CGUIDialogProgress*)m_gWindowManager.GetWindow(WINDOW_DIALOG_PROGRESS);
      m_dlgProgress->SetLine(0, "Validating programs");
      m_dlgProgress->SetLine(1, "");
      m_dlgProgress->SetLine(2, "");
      m_dlgProgress->StartModal(GetID());
      m_dlgProgress->SetHeading(12387);
      m_dlgProgress->SetPercentage(0);
      m_dlgProgress->ShowProgressBar(true);

      m_database.BeginTransaction();
      for (unsigned int i=0;i<m_vecPaths.size();++i)
      {
        if (m_dlgProgress->IsCanceled())
          break;
        m_dlgProgress->SetLine(1,m_vecPaths[i]);
        m_dlgProgress->SetPercentage((int)((float)i/(float)m_vecPaths.size()*100.f));
        m_dlgProgress->Progress();

        CStdString strPath;
        CUtil::AddFileToFolder(m_vecPaths[i],"default.xbe",strPath);
        if (!CFile::Exists(strPath))
        {
          strPath = m_vecPaths[i];
          strPath.Replace("\\","/");
          m_database.DeleteProgram(strPath);
        }
        Update(m_vecItems.m_strPath);
      }
      m_database.CommitTransaction();
      m_dlgProgress->Close();
    }
  }
  else
    return false;

  m_vecItems[iItem]->Select(false);
  return true;
}

void CGUIWindowPrograms::LoadDirectory(const CStdString& strDirectory, int idepth)
{
    m_database.BeginTransaction();
    LoadDirectory2(strDirectory,idepth);
    m_database.CommitTransaction();
}

void CGUIWindowPrograms::LoadDirectory2(const CStdString& strDirectory, int idepth)
{
  WIN32_FIND_DATA wfd;
  bool bOnlyDefaultXBE = g_guiSettings.GetBool("MyPrograms.DefaultXBEOnly");
  bool bFlattenDir = g_guiSettings.GetBool("MyPrograms.Flatten");
  bool bUseDirectoryName = g_guiSettings.GetBool("MyPrograms.UseDirectoryName");

  CFileItemList items;
  m_database.GetProgramsByPath(strDirectory,items,0,g_guiSettings.GetBool("MyPrograms.DefaultXBEOnly"));


  memset(&wfd, 0, sizeof(wfd));
  CStdString strRootDir = strDirectory;
  if (!CUtil::HasSlashAtEnd(strRootDir) )
    strRootDir += "\\";

  CFileItem rootDir(strRootDir, true);
  if ( rootDir.IsDVD() )
  {
    CIoSupport helper;
    helper.Remount("D:", "Cdrom0");
  }
  g_charsetConverter.utf8ToStringCharset(strRootDir);
  CStdString strSearchMask = strRootDir;
  strSearchMask += "*.*";

  FILETIME localTime;
  CAutoPtrFind hFind ( FindFirstFile(strSearchMask.c_str(), &wfd));
  if (!hFind.isValid())
    return ;
  do
  {
    if (wfd.cFileName[0] != 0)
    {
      CFileItem fileName(wfd.cFileName, false);
      CFileItem file(strRootDir + wfd.cFileName, false);
      g_charsetConverter.stringCharsetToUtf8(fileName.m_strPath);
      g_charsetConverter.stringCharsetToUtf8(file.m_strPath);

      if ( (wfd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) )
      {
        if ( fileName.m_strPath != "." && fileName.m_strPath != ".." && !bFlattenDir)
        {
          CFileItem *pItem = new CFileItem(fileName.m_strPath);
          pItem->m_strPath = file.m_strPath;
          pItem->m_bIsFolder = true;
          FileTimeToLocalFileTime(&wfd.ftLastWriteTime, &localTime);
          FileTimeToSystemTime(&localTime, &pItem->m_stTime);
          m_vecItems.Add(pItem);
        }
        else
        {
          if (fileName.m_strPath != "." && fileName.m_strPath != ".." && bFlattenDir && idepth > 0 )
          {
            m_vecPaths1.push_back(file.m_strPath);
            bool foundPath = false;                             
            for (int i = 0; i < (int)m_vecPaths.size(); i++)    
            
            {
              if (file.m_strPath == m_vecPaths[i])
              {
                foundPath = true;              
                break;
              }                                                 
            }                                                   
            if (!foundPath)                                     
              LoadDirectory2(file.m_strPath, idepth - 1);
          }
        }
      }
      else
      {
        int i;
        for (i=0;i<items.Size();++i)
        {
          if (items[i]->m_strPath.CompareNoCase(file.m_strPath)==0)
          {
            m_vecItems.Add(items[i]);
            break;
          }
        }
        if (i < items.Size())
          continue;
        if (bOnlyDefaultXBE ? fileName.IsDefaultXBE() : fileName.IsXBE())
        {
          CStdString strDescription;

          if (!CUtil::GetXBEDescription(file.m_strPath, strDescription) || (bUseDirectoryName && fileName.IsDefaultXBE()) )
          {
            CUtil::GetDirectoryName(file.m_strPath, strDescription);
            // why?? Surely if it's a directory name it will be fatx compatible anyway??
            //CUtil::ShortenFileName(strDescription);
            //CUtil::RemoveIllegalChars(strDescription);
          }

          if (!bFlattenDir || file.IsOnDVD())
          {
            CFileItem *pItem = new CFileItem(strDescription);
            pItem->m_strPath = file.m_strPath;
            pItem->m_bIsFolder = false;
            pItem->m_strTitle=strDescription;
            //pItem->m_dwSize = wfd.nFileSizeLow;

            FileTimeToLocalFileTime(&wfd.ftLastWriteTime, &localTime);
            FileTimeToSystemTime(&localTime, &pItem->m_stTime);
            m_vecItems.Add(pItem);
          }
          else
          {
            DWORD titleId = CUtil::GetXbeID(file.m_strPath);
            m_database.AddProgram(file.m_strPath, titleId, strDescription, m_strBookmarkName);  
          }
        }

        if (fileName.IsShortCut())
        {
          if (!bFlattenDir)
          {
            CFileItem *pItem = new CFileItem(fileName.m_strPath);
            pItem->m_strPath = file.m_strPath;
            pItem->m_bIsFolder = false;
            //pItem->m_dwSize = wfd.nFileSizeLow;

            FileTimeToLocalFileTime(&wfd.ftLastWriteTime, &localTime);
            FileTimeToSystemTime(&localTime, &pItem->m_stTime);
            m_vecItems.Add(pItem);
          }

          else
          {
            DWORD titleId = CUtil::GetXbeID(file.m_strPath);
            m_database.AddProgram(file.m_strPath, titleId, wfd.cFileName, m_strBookmarkName);
          }
           
        }
      }
    }
  }
  while (FindNextFile(hFind, &wfd));
}

bool CGUIWindowPrograms::Update(const CStdString &strDirectory)
{
  if (g_advancedSettings.m_newMyPrograms)
  {
    if (m_thumbLoader.IsLoading())
      m_thumbLoader.StopThread();

    if (!CGUIMediaWindow::Update(strDirectory))
      return false;

    m_thumbLoader.Load(m_vecItems);
    return true;
  }
  bool bFlattenDir = g_guiSettings.GetBool("MyPrograms.Flatten");
  bool bOnlyDefaultXBE = g_guiSettings.GetBool("MyPrograms.DefaultXBEOnly");
  bool bParentPath(false);
  bool bPastBookMark(true);
  CStdString strParentPath;
  CStdString strDir = strDirectory;
  CStdString strShortCutsDir = g_stSettings.m_szShortcutDirectory;
  int idepth = m_iDepth;
  vector<CStdString> vecPaths;
  m_isRoot = false;
  ClearFileItems();

  if (strDirectory == "")  // display only the root shares
  {
    m_isRoot = true;
    for (int i = 0; i < (int)g_settings.m_vecMyProgramsBookmarks.size(); ++i)
    {
      CShare& share = g_settings.m_vecMyProgramsBookmarks[i];
      vector<CStdString> vecShares;
      CFileItem *pItem = new CFileItem(share);
      pItem->m_bIsShareOrDrive = false;
      CUtil::Tokenize(pItem->m_strPath, vecShares, ",");
      CStdString strThumb="";
      int j;
      for (j = 0; j < (int)vecShares.size(); j++)    // use the first folder image that we find in the vector of shares
      {
        CFileItem item(vecShares[j], true);
        if (!item.IsXBE())
        {
          item.SetUserProgramThumb();
          if (item.HasThumbnail())
          {
            pItem->SetThumbnailImage(item.GetThumbnailImage());
            break;
          }
        }
      }
      if (j == vecShares.size())
      {
        CFileItem item(vecShares[0], false);
        if (item.IsHD())
          strThumb = "defaultHardDisk.png";
        if (item.IsDVD())
          CUtil::GetDVDDriveIcon( item.m_strPath, strThumb );
        pItem->SetIconImage(strThumb);
      }

      m_vecItems.Add(pItem);
    }

    m_strParentPath = "";
  }

  CUtil::Tokenize(strDir, vecPaths, ",");

  if (!g_guiSettings.GetBool("MyPrograms.NoShortcuts"))
  {
    if (CUtil::HasSlashAtEnd(strShortCutsDir))
      strShortCutsDir.Delete(strShortCutsDir.size() - 1);

    if (vecPaths.size() > 0 && vecPaths[0] == strShortCutsDir)
      m_strBookmarkName = "shortcuts";
      m_database.GetPathsByBookmark(m_strBookmarkName, m_vecPaths);
  }

  for (int k = 0; k < (int)vecPaths.size(); k++)
  {
    int start = vecPaths[k].find_first_not_of(" \t");
    int end = vecPaths[k].find_last_not_of(" \t") + 1;
    vecPaths[k] = vecPaths[k].substr(start, end - start);
    if (CUtil::HasSlashAtEnd(vecPaths[k]))
    {
      vecPaths[k].Delete(vecPaths[k].size() - 1);
    }
  }

  if (!bFlattenDir)
  {
    vector<CStdString> vecOrigShare;
    CUtil::Tokenize(m_shareDirectory, vecOrigShare, ",");
    if (vecOrigShare.size() && vecPaths.size())
    {
      bParentPath = CUtil::GetParentPath(vecPaths[0], strParentPath);
      if (CUtil::HasSlashAtEnd(vecOrigShare[0]))
        vecOrigShare[0].Delete(vecOrigShare[0].size() - 1);
      if (strParentPath < vecOrigShare[0])
      {
        bPastBookMark = false;
        strParentPath = "";
      }
    }
  }
  else
  {
    strParentPath = "";
  }

  if (strDirectory != "")
  {
    if (m_guiState.get() && !m_guiState->HideParentDirItems())
    {
      CFileItem *pItem = new CFileItem("..");
      pItem->m_strPath = strParentPath;
      pItem->m_bIsShareOrDrive = false;
      pItem->m_bIsFolder = true;
      m_vecItems.Add(pItem);
    }
    m_strParentPath = strParentPath;
  }

  m_iLastControl = GetFocusedControl();

  m_database.BeginTransaction();
  if (!m_vecItems.IsVirtualDirectoryRoot())
  {
    for (int j = 0; j < (int)vecPaths.size(); j++)
    {
      CFileItem item(vecPaths[j], false);
      if (item.IsXBE())     // we've found a single XBE in the path vector
      {
        CStdString strDescription;
        if ( (m_database.GetFile(item.m_strPath, m_vecItems) < 0) && CFile::Exists(item.m_strPath))
        {
          if (!CUtil::GetXBEDescription(item.m_strPath, strDescription))
          {
            CUtil::GetDirectoryName(item.m_strPath, strDescription);
            // why?? Surely if it's a directory name it will be fatx compatible anyway??
            //CUtil::ShortenFileName(strDescription);
            //CUtil::RemoveIllegalChars(strDescription);
          }

          if (item.IsOnDVD())
          {
            WIN32_FILE_ATTRIBUTE_DATA FileAttributeData;
            FILETIME localTime;
            CFileItem *pItem = new CFileItem(strDescription);
            pItem->m_strPath = item.m_strPath;
            pItem->m_bIsFolder = false;
            pItem->m_strTitle=strDescription;
            GetFileAttributesEx(pItem->m_strPath, GetFileExInfoStandard, &FileAttributeData);
            FileTimeToLocalFileTime(&FileAttributeData.ftLastWriteTime, &localTime);
            FileTimeToSystemTime(&localTime, &pItem->m_stTime);
            pItem->m_dwSize = FileAttributeData.nFileSizeLow;
            m_vecItems.Add(pItem);
          }
          else
          {
            DWORD titleId = CUtil::GetXbeID(item.m_strPath);
            m_database.AddProgram(item.m_strPath, titleId, strDescription, m_strBookmarkName);
            m_database.GetFile(item.m_strPath, m_vecItems);
          }
        }
        CStdString directoryName;
        CUtil::GetDirectoryName(item.m_strPath, directoryName);        
        bool found = false;
        for (int i = 0; i < (int)m_vecPaths1.size(); i++)
        {
          if (m_vecPaths1[i] == directoryName) 
          {
            found = true;
            break;
          }
        }
        if (!found)  m_vecPaths1.push_back(directoryName);
      }
      else
      {
        LoadDirectory(item.m_strPath, idepth);
//      m_database.GetProgramsByPath(item.m_strPath, m_vecItems, idepth, bOnlyDefaultXBE);
      }
    }


    if (m_strBookmarkName == "shortcuts")
      bOnlyDefaultXBE = false;   // let's do this so that we don't only grab default.xbe from database when getting shortcuts
    if (bFlattenDir)
    {
      // let's first delete the items in database that no longer exist
      if ( (int)m_vecPaths.size() != (int)m_vecPaths1.size())
      {
        bool found = true;
        for (int j = 0; j < (int)m_vecPaths.size(); j++)
        {
          for (int i = 0; i < (int)m_vecPaths1.size(); i++)
          { 
            if (m_vecPaths1[i]==m_vecPaths[j])
            {
              found = true;
              break;
            }
            found = false;
          }
          if (!found)
          {
            CStdString strDeletePath = m_vecPaths[j];
            strDeletePath.Replace("\\", "/");
            m_database.DeleteProgram(strDeletePath);
          }
        }
      }
      m_database.GetProgramsByBookmark(m_strBookmarkName, m_vecItems, idepth, bOnlyDefaultXBE);
    }
  }  
  m_database.CommitTransaction();
  //CUtil::ClearCache();
  CStdString strSelectedItem = m_history.GetSelectedItem(m_isRoot?"empty":strDirectory);
  if (m_guiState.get() && m_guiState->HideExtensions())
    m_vecItems.RemoveExtensions();
  m_vecItems.FillInDefaultIcons();
  int i;
  for (i=0;i<m_vecItems.Size();++i)
  {
    CStdString strHistory;
    GetDirectoryHistoryString(m_vecItems[i], strHistory);
    if (strHistory == strSelectedItem)
      break;
  }
  if (i==m_vecItems.Size())
    strSelectedItem = "";

  m_vecItems.SetCachedProgramThumbs();

  // set our overlay icons
  SetOverlayIcons();

  m_guiState.reset(CGUIViewState::GetViewState(GetID(), m_vecItems));

  OnSort();
  UpdateButtons();
  m_viewControl.SetSelectedItem(strSelectedItem);
  
  m_thumbLoader.Load(m_vecItems);
  return true;
}

bool CGUIWindowPrograms::OnClick(int iItem)
{
  if (g_advancedSettings.m_newMyPrograms)
    return CGUIMediaWindow::OnClick(iItem);

  if ( iItem < 0 || iItem >= (int)m_vecItems.Size() ) return true;
  CFileItem* pItem = m_vecItems[iItem];
  
  if (pItem->m_strPath == "add" && pItem->GetLabel() == g_localizeStrings.Get(1026)) // 'add source button' in empty root
  {
    if (CGUIDialogMediaSource::ShowAndAddMediaSource("myprograms"))
    {
      Update("");
      return true;
    }
    return false;
  }

  if (pItem->m_bIsFolder)
  {
    // do nothing if the bookmark is locked
    if ( !g_passwordManager.IsItemUnlocked( pItem, "myprograms" ) )
      return true;

    CStdString strSelectedItem = "";
    if (iItem >= 0 && iItem < m_vecItems.Size())
      strSelectedItem = m_vecItems[iItem]->m_strPath;
    if (strSelectedItem != "")
      m_history.SetSelectedItem(strSelectedItem, m_isRoot?"empty":m_vecItems.m_strPath);
    
    if (m_vecItems.IsVirtualDirectoryRoot())
      m_shareDirectory = pItem->m_strPath;
    m_vecItems.m_strPath = pItem->m_strPath;
    m_iDepth = pItem->m_idepth;
    bool bIsBookmarkName(false);
    if (m_isRoot) {
      int iIndex = CUtil::GetMatchingShare(m_shareDirectory, g_settings.m_vecMyProgramsBookmarks, bIsBookmarkName);
      if (iIndex > -1)
      {
        m_vecPaths.clear();
        m_vecPaths1.clear();
        m_strBookmarkName = g_settings.m_vecMyProgramsBookmarks[iIndex].strName;
        m_database.GetPathsByBookmark(m_strBookmarkName, m_vecPaths);
      }
    }
    Update(m_vecItems.m_strPath);
  }
  else
  {
    // xbe exists?
    if (!CFile::Exists(pItem->m_strPath.c_str()))
    {
      CStdString strPath,strFile;
      CUtil::Split(pItem->m_strPath,strPath,strFile);
      if (CUtil::HasSlashAtEnd(strPath))
        strPath.erase(strPath.size()-1);
      strPath.Replace("\\","/");
      m_database.DeleteProgram(strPath);
      Update(m_vecItems.m_strPath);
      return true;
    }

    // launch xbe...
    char szPath[1024];
    char szParameters[1024];
    if (g_guiSettings.GetBool("MyPrograms.Flatten"))
      m_database.IncTimesPlayed(pItem->m_strPath);

    int iRegion = m_iRegionSet?m_iRegionSet:GetRegion(iItem);
    
    DWORD dwTitleId;
    if (!pItem->IsOnDVD())
      dwTitleId = m_database.GetTitleId(pItem->m_strPath);
    else
      dwTitleId = CUtil::GetXbeID(pItem->m_strPath);
    CStdString strTrainer = m_database.GetActiveTrainer(dwTitleId);
    if (strTrainer != "")
    {
      bool bContinue=false;
      if (CKaiClient::GetInstance()->IsEngineConnected())
      {
        if (CGUIDialogYesNo::ShowAndGetInput(20023,20020,20021,20022,714,12013))
          CKaiClient::GetInstance()->EnterVector(CStdString(""),CStdString(""));
        else
          bContinue = true;
      }
      if (!bContinue)
      {
        CTrainer trainer;
        if (trainer.Load(strTrainer))
        {
          m_database.GetTrainerOptions(strTrainer,dwTitleId,trainer.GetOptions(),trainer.GetNumberOfOptions());
          CUtil::InstallTrainer(trainer);
        }
      }
    }

    m_database.Close();
    memset(szParameters, 0, sizeof(szParameters));

    strcpy(szPath, pItem->m_strPath.c_str());

    if (pItem->IsShortCut())
    {
      CUtil::RunShortcut(pItem->m_strPath.c_str());
      return false;
    }
   
    if (strlen(szParameters))
      CUtil::RunXBE(szPath, szParameters,F_VIDEO(iRegion));
    else
      CUtil::RunXBE(szPath,NULL,F_VIDEO(iRegion));
  }

  return true;
}

bool CGUIWindowPrograms::OnPlayMedia(int iItem)
{
  if ( iItem < 0 || iItem >= (int)m_vecItems.Size() ) return false;
  CFileItem* pItem = m_vecItems[iItem];

  if (pItem->m_bIsFolder) return false;

  // launch xbe...
  char szPath[1024];
  char szParameters[1024];
  if (g_guiSettings.GetBool("MyPrograms.Flatten"))
    m_database.IncTimesPlayed(pItem->m_strPath);

  int iRegion = m_iRegionSet?m_iRegionSet:GetRegion(iItem);
  
  DWORD dwTitleId;
  if (!pItem->IsOnDVD())
    dwTitleId = m_database.GetTitleId(pItem->m_strPath);
  else
    dwTitleId = CUtil::GetXbeID(pItem->m_strPath);
  CStdString strTrainer = m_database.GetActiveTrainer(dwTitleId);
  if (strTrainer != "")
  {
    bool bContinue=false;
    if (CKaiClient::GetInstance()->IsEngineConnected())
    {
      if (CGUIDialogYesNo::ShowAndGetInput(20023,20020,20021,20022,714,12013))
        CKaiClient::GetInstance()->EnterVector(CStdString(""),CStdString(""));
      else
        bContinue = true;
    }
    if (!bContinue)
    {
      CTrainer trainer;
      if (trainer.Load(strTrainer))
      {
        m_database.GetTrainerOptions(strTrainer,dwTitleId,trainer.GetOptions(),trainer.GetNumberOfOptions());
        CUtil::InstallTrainer(trainer);
      }
    }
  }

  m_database.Close();
  memset(szParameters, 0, sizeof(szParameters));

  strcpy(szPath, pItem->m_strPath.c_str());

  if (pItem->IsShortCut())
  {
    CUtil::RunShortcut(pItem->m_strPath.c_str());
    return false;
  }
  
  if (strlen(szParameters))
    CUtil::RunXBE(szPath, szParameters,F_VIDEO(iRegion));
  else
    CUtil::RunXBE(szPath,NULL,F_VIDEO(iRegion));
  return true;
}

void CGUIWindowPrograms::OnScan(CFileItemList& items, int& iTotalAppsFound)
{
  // remove username + password from m_strDirectory for display in Dialog
  CURL url(m_vecItems.m_strPath);
  CStdString strStrippedPath;
  url.GetURLWithoutUserDetails(strStrippedPath);

  CStdString strText = g_localizeStrings.Get(212);
  CStdString strTotal;
  strTotal.Format("%i %s", iTotalAppsFound, strText.c_str());
  if (m_dlgProgress)
  {
    m_dlgProgress->SetLine(0, strTotal);
    m_dlgProgress->SetLine(1, "");
    m_dlgProgress->SetLine(2, strStrippedPath);
    m_dlgProgress->Progress();
  }
  //bool   bOnlyDefaultXBE=g_guiSettings.GetBool("MyPrograms.DefaultXBEOnly");
  bool bScanSubDirs = true;
  bool bFound = false;
  DeleteThumbs(items);
  CUtil::CreateShortcuts(items);
  if ((int)m_vecItems.m_strPath.size() != 2) // true for C:, E:, F:, G:
  {
    // first check all files
    for (int i = 0; i < (int)items.Size(); ++i)
    {
      CFileItem *pItem = items[i];
      if (! pItem->m_bIsFolder)
      {
        if (pItem->IsXBE() )
        {
          bScanSubDirs = false;
          break;
        }
      }
    }
  }

  for (int i = 0; i < (int)items.Size(); ++i)
  {
    CFileItem *pItem = items[i];
    if (pItem->m_bIsFolder)
    {
      if (bScanSubDirs && !bFound && !pItem->IsParentFolder())
      {
        // load subfolder
        CStdString strDir = m_vecItems.m_strPath;
        if (pItem->m_strPath != "E:\\UDATA" && pItem->m_strPath != "E:\\TDATA")
        {
          m_vecItems.m_strPath = pItem->m_strPath;
          CFileItemList subDirItems;
          CHDDirectory rootDir;
          rootDir.SetMask(".xbe");
          rootDir.GetDirectory(pItem->m_strPath, subDirItems);
          OnScan(subDirItems, iTotalAppsFound);
          subDirItems.Clear();
          m_vecItems.m_strPath = strDir;
        }
      }
    }
    else
    {
      if (pItem->IsXBE())
      {
        bFound = true;
        iTotalAppsFound++;

        CStdString strText = g_localizeStrings.Get(212);
        strTotal.Format("%i %s", iTotalAppsFound, strText.c_str());
        CStdString strDescription;
        CUtil::GetXBEDescription(pItem->m_strPath, strDescription);
        if (strDescription == "")
          strDescription = CUtil::GetFileName(pItem->m_strPath);
        if (m_dlgProgress)
        {
          m_dlgProgress->SetLine(0, strTotal);
          m_dlgProgress->SetLine(1, "");
          m_dlgProgress->SetLine(2, strStrippedPath);
          m_dlgProgress->Progress();
        }
        // CStdString strIcon;
        // CUtil::GetXBEIcon(pItem->m_strPath, strIcon);
        // ::DeleteFile(strIcon.c_str());

      }
    }
  }
  g_directoryCache.Clear();
}

// TODO 2.0: What does this routine do???
void CGUIWindowPrograms::DeleteThumbs(CFileItemList& items)
{
  CUtil::ClearCache();
  for (int i = 0; i < (int)items.Size(); ++i)
  {
    CFileItem *pItem = items[i];
    if (! pItem->m_bIsFolder)
    {
      if (pItem->IsXBE() )
      {
        ::DeleteFile(pItem->GetCachedProgramThumb().c_str());
      }
    }
  }
}

/// \brief Call to go to parent folder
void CGUIWindowPrograms::GoParentFolder()
{
  if (g_advancedSettings.m_newMyPrograms)
  {
    CGUIMediaWindow::GoParentFolder();
    return;
  }
   // get selected item
  int iItem = m_viewControl.GetSelectedItem();
  CStdString strSelectedItem = "";
  if (iItem >= 0 && iItem < m_vecItems.Size())
    strSelectedItem = m_vecItems[iItem]->m_strPath;
  if (strSelectedItem != "")
    m_history.SetSelectedItem(strSelectedItem, m_isRoot?"empty":m_vecItems.m_strPath);

  m_vecItems.m_strPath = m_strParentPath;
  Update(m_vecItems.m_strPath);
}

int CGUIWindowPrograms::GetRegion(int iItem, bool bReload)
{
  if (!g_guiSettings.GetBool("MyPrograms.GameAutoRegion"))
    return 0;

  int iRegion;
  if (bReload || m_vecItems[iItem]->IsOnDVD())
  {
    CXBE xbe;
    iRegion = xbe.ExtractGameRegion(m_vecItems[iItem]->m_strPath); 
  }
  else
  {
    m_database.Open();
    iRegion = m_database.GetRegion(m_vecItems[iItem]->m_strPath);
    m_database.Close();
  }
  if (iRegion == -1)
  {
    if (g_guiSettings.GetBool("MyPrograms.GameAutoRegion"))
    {
      CXBE xbe;
      iRegion = xbe.ExtractGameRegion(m_vecItems[iItem]->m_strPath);
      if (iRegion < 1 || iRegion > 7)
        iRegion = 0;
      m_database.SetRegion(m_vecItems[iItem]->m_strPath,iRegion);
    }
    else
      iRegion = 0;
  }
  
  if (bReload)
    return CXBE::FilterRegion(iRegion,true);
  else
    return CXBE::FilterRegion(iRegion);
}

void CGUIWindowPrograms::PopulateTrainersList()
{
  CDirectory directory;
  CFileItemList trainers;
  CFileItemList archives;
  CFileItemList inArchives;
  // first, remove any dead items
  std::vector<CStdString> vecTrainerPath;
  m_database.GetAllTrainers(vecTrainerPath);
  if (!m_dlgProgress)
    m_dlgProgress = (CGUIDialogProgress*)m_gWindowManager.GetWindow(WINDOW_DIALOG_PROGRESS);
  m_dlgProgress->SetLine(0,"Validating existing trainers");
  m_dlgProgress->SetLine(1,"");
  m_dlgProgress->SetLine(2,"");
  m_dlgProgress->StartModal(GetID());
  m_dlgProgress->SetHeading(12012);
  m_dlgProgress->ShowProgressBar(true);
  m_dlgProgress->Progress();
  
  bool bBreak=false;
  for (unsigned int i=0;i<vecTrainerPath.size();++i)
  {
    m_dlgProgress->SetPercentage((int)((float)i/(float)vecTrainerPath.size()*100.f));
    CStdString strLine;
    strLine.Format("%s %i / %i",g_localizeStrings.Get(12013).c_str(), i+1,vecTrainerPath.size());
    m_dlgProgress->SetLine(1,strLine);
    m_dlgProgress->Progress();
    m_dlgProgress->Progress();
    if (!CFile::Exists(vecTrainerPath[i]))
      m_database.RemoveTrainer(vecTrainerPath[i]);
    if (m_dlgProgress->IsCanceled())
    {
      bBreak = true;
      break;
    }
  }
  if (!bBreak)
  {
    CLog::Log(LOGDEBUG,"trainerpath %s",g_guiSettings.GetString("MyPrograms.TrainerPath").c_str());
    directory.GetDirectory(g_guiSettings.GetString("MyPrograms.TrainerPath").c_str(),trainers,".xbtf|.etm");
    directory.GetDirectory(g_guiSettings.GetString("MyPrograms.TrainerPath").c_str(),archives,".rar",false); // TODO: ZIP SUPPORT
    for( int i=0;i<archives.Size();++i)
    {
      if (stricmp(CUtil::GetExtension(archives[i]->m_strPath),".rar") == 0)
      {
        g_RarManager.GetFilesInRar(inArchives,archives[i]->m_strPath,false);
        CHDDirectory dir;
        dir.SetMask(".xbtf|.etm");
        for (int j=0;j<inArchives.Size();++j)
          if (dir.IsAllowed(inArchives[j]->m_strPath))
          {
            CFileItem* item = new CFileItem(*inArchives[j]);
            CStdString strPathInArchive = item->m_strPath;
            item->m_strPath.Format("rar://%s,2,,%s,\\%s",g_advancedSettings.m_cachePath.c_str(),archives[i]->m_strPath.c_str(),strPathInArchive.c_str());
            trainers.Add(item);
          }
      }      
    }
    if (!m_dlgProgress)
      m_dlgProgress = (CGUIDialogProgress*)m_gWindowManager.GetWindow(WINDOW_DIALOG_PROGRESS);
    m_dlgProgress->SetPercentage(0);
    m_dlgProgress->ShowProgressBar(true);
  
    CLog::Log(LOGDEBUG,"# trainers %i",trainers.Size());
    m_database.BeginTransaction();
    for (int i=0;i<trainers.Size();++i)
    {
      CLog::Log(LOGDEBUG,"found trainer %s",trainers[i]->m_strPath.c_str());
      m_dlgProgress->SetPercentage((int)((float)(i)/trainers.Size()*100.f));
      CStdString strLine;
      strLine.Format("%s %i / %i",g_localizeStrings.Get(12013).c_str(), i+1,trainers.Size());

      m_dlgProgress->SetLine(0,strLine);
      m_dlgProgress->Progress();
      if (m_database.HasTrainer(trainers[i]->m_strPath)) // skip existing trainers
        continue;

      CTrainer trainer;
      if (trainer.Load(trainers[i]->m_strPath))
      { 
        m_dlgProgress->SetLine(1,trainer.GetName());
        m_dlgProgress->SetLine(2,"");
        m_dlgProgress->Progress();
        unsigned int iTitle1, iTitle2, iTitle3;
        trainer.GetTitleIds(iTitle1,iTitle2,iTitle3);
        if (iTitle1)
          m_database.AddTrainer(iTitle1,trainers[i]->m_strPath);
        if (iTitle2)
          m_database.AddTrainer(iTitle2,trainers[i]->m_strPath);
        if (iTitle3)
          m_database.AddTrainer(iTitle3,trainers[i]->m_strPath);
      }
      if (m_dlgProgress->IsCanceled())
        break;
    }
  }
  m_database.CommitTransaction();
  m_dlgProgress->Close();
  
  SetOverlayIcons();
  m_guiState.reset(CGUIViewState::GetViewState(GetID(), m_vecItems));
  OnSort();
  UpdateButtons();
}

void CGUIWindowPrograms::SetOverlayIcons()
{
  for (int i = 0; i < m_vecItems.Size(); i++)
  {
    CFileItem *item = m_vecItems[i];
    if (item->IsXBE())
    {
      DWORD dwTitleId;
      if (item->IsOnDVD())
        dwTitleId = CUtil::GetXbeID(item->m_strPath);
      else
        dwTitleId = m_database.GetTitleId(item->m_strPath);
      if ((int)dwTitleId == -1)
      {
        dwTitleId = CUtil::GetXbeID(item->m_strPath);
        m_database.SetTitleId(item->m_strPath,dwTitleId);
      }
      if (m_database.ItemHasTrainer(dwTitleId))
      {
        if (m_database.GetActiveTrainer(dwTitleId) != "")
          item->SetOverlayImage(CGUIListItem::ICON_OVERLAY_TRAINED);
        else
          item->SetOverlayImage(CGUIListItem::ICON_OVERLAY_HAS_TRAINER);
      }
    }
  }
}

bool CGUIWindowPrograms::GetDirectory(const CStdString &strDirectory, CFileItemList &items)
{
  if (!CGUIMediaWindow::GetDirectory(strDirectory, items))
    return false;

  if (items.IsVirtualDirectoryRoot())
    return true;

  // flatten any folders
  m_database.BeginTransaction();
  for (int i = 0; i < items.Size(); i++)
  {
    CFileItem *item = items[i];
    if (item->m_bIsFolder && !item->IsParentFolder())
    { // folder item - let's check for a default.xbe file, and flatten if we have one
      CStdString defaultXBE;
      CUtil::AddFileToFolder(item->m_strPath, "default.xbe", defaultXBE);
      if (CFile::Exists(defaultXBE))
      { // yes, format the item up
        item->m_strPath = defaultXBE;
        item->m_bIsFolder = false;
      }
    }
    if (item->IsXBE())
    { // add to database if not already there
      DWORD dwTitleID = m_database.GetProgramInfo(item);
      if (!dwTitleID)
      {
        CStdString description;
        if (CUtil::GetXBEDescription(item->m_strPath, description))
          item->SetLabel(description);
        dwTitleID = CUtil::GetXbeID(item->m_strPath);
        m_database.AddProgramInfo(item, dwTitleID);
      }

      // SetOverlayIcons()
      if (m_database.ItemHasTrainer(dwTitleID))
      {
        if (m_database.GetActiveTrainer(dwTitleID) != "")
          item->SetOverlayImage(CGUIListItem::ICON_OVERLAY_TRAINED);
        else
          item->SetOverlayImage(CGUIListItem::ICON_OVERLAY_HAS_TRAINER);
      }
    }
  }
  m_database.CommitTransaction();

  // set the cached thumbs
  items.SetCachedProgramThumbs();
  return true;
}

void CGUIWindowPrograms::OnWindowLoaded()
{
  CGUIMediaWindow::OnWindowLoaded();
  if (g_advancedSettings.m_newMyPrograms)
  {
    for (int i = 100; i < 110; i++)
    {
      SET_CONTROL_HIDDEN(i);
    }
  }
}
