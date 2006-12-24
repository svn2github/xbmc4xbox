#include "stdafx.h"
#include "GUIWindowGameSaves.h"
#include "util.h"
#include "FileSystem/ZipManager.h"
#include "GUIDialogContextMenu.h"
#include "GUIDialogFileBrowser.h"
//#include "GUIListControl.h"
#include "GUIWindowFileManager.h"
#include "GUIPassword.h"
#include <fstream>
#include "Utils\HTTP.h"
#define CONTROL_BTNVIEWASICONS     2
#define CONTROL_BTNSORTBY          3
#define CONTROL_BTNSORTASC         4
#define CONTROL_LIST              50
#define CONTROL_THUMBS            51
#define CONTROL_LABELFILES        12


CGUIWindowGameSaves::CGUIWindowGameSaves()
: CGUIMediaWindow(WINDOW_GAMESAVES, "MyGameSaves.xml")
{

}

CGUIWindowGameSaves::~CGUIWindowGameSaves()
{
}

void CGUIWindowGameSaves::GoParentFolder()
{
  if (m_history.GetParentPath() == m_vecItems.m_strPath)
    m_history.RemoveParentPath();
  CStdString strParent=m_history.RemoveParentPath();
  CStdString strOldPath(m_vecItems.m_strPath);
  CStdString strPath(m_strParentPath);
  VECSHARES shares;
  bool bIsBookmarkName = false;

  SetupShares();
  m_rootDir.GetShares(shares);

  int iIndex = CUtil::GetMatchingShare(strPath, shares, bIsBookmarkName);

  if (iIndex > -1)
  {
    if (strPath.size() == 2)
      if (strPath[1] == ':')
        CUtil::AddSlashAtEnd(strPath);
    Update(strPath);
  }
  else
  {
    Update(strParent);
  }
  if (!g_guiSettings.GetBool("filelists.fulldirectoryhistory"))
      m_history.RemoveSelectedItem(strOldPath); //Delete current path
}
bool CGUIWindowGameSaves::OnAction(const CAction &action)
{
  return CGUIMediaWindow::OnAction(action);
}

bool CGUIWindowGameSaves::OnClick(int iItem)
{
  if (!m_vecItems[iItem]->m_bIsFolder)
  {
    OnPopupMenu(iItem);
    return true;
  }

  return CGUIMediaWindow::OnClick(iItem);
}

bool CGUIWindowGameSaves::OnMessage(CGUIMessage& message)
{
  switch ( message.GetMessage() )
  {
  case GUI_MSG_CLICKED:
    {
      if (message.GetSenderId() == CONTROL_BTNSORTBY)
      {

        if (CGUIMediaWindow::OnMessage(message))
        {
          for (int i=0;i<m_vecItems.Size();++i)
          {
            CGUIViewState::LABEL_MASKS labelMasks;
            m_guiState->GetSortMethodLabelMasks(labelMasks);
            m_vecItems[i]->FormatLabel2(labelMasks.m_strLabel2File);
          }
          return true;
        }
        else
          return false;
      }
      if (m_viewControl.HasControl(message.GetSenderId()))  // list/thumb control
      {
        
        // get selected item
        int iAction = message.GetParam1();
        if (ACTION_CONTEXT_MENU == iAction)
        {
          // make sure not parent
          int iItem = m_viewControl.GetSelectedItem();
          CFileItem* pItem = m_vecItems[iItem];
          if (!pItem->IsParentFolder() && !pItem->m_bIsShareOrDrive)
            OnPopupMenu(iItem);
        }
      }
    }
  case GUI_MSG_WINDOW_INIT:
    {
      CStdString strDestination = message.GetStringParam();
      if (m_vecItems.m_strPath == "?")
        m_vecItems.m_strPath = "E:\\udata";

      m_rootDir.SetMask("/");
      VECSHARES shares;
      bool bIsBookmarkName = false;
      SetupShares();
      m_rootDir.GetShares(shares);
      int iIndex = CUtil::GetMatchingShare(strDestination, shares, bIsBookmarkName);
      // if bIsBookmarkName == True,   chances are its "Local GameSaves" or something else :D)
      if (iIndex > -1)
      {
        bool bDoStuff = true;
        if (shares[iIndex].m_iHasLock == 2)
        {
        CFileItem item(shares[iIndex]);
          if (!g_passwordManager.IsItemUnlocked(&item,"myprograms"))
          {
            m_vecItems.m_strPath = ""; // no u don't
            bDoStuff = false;
            CLog::Log(LOGINFO, "  Failure! Failed to unlock destination path: %s", strDestination.c_str());
          }
        }
        if (bIsBookmarkName)
        {
          m_vecItems.m_strPath=shares[iIndex].strPath;
        }
        else if (CDirectory::Exists(strDestination))
        {
          m_vecItems.m_strPath = strDestination;
        }
      }
      return CGUIMediaWindow::OnMessage(message);
    }
    break;
  }
  return CGUIMediaWindow::OnMessage(message);
}


bool CGUIWindowGameSaves::OnPlayMedia(int iItem)
{
  CFileItem* pItem=m_vecItems[iItem];
  CStdString strPath = pItem->m_strPath;
  return true;
}

void CGUIWindowGameSaves::Render()
{
  // update control_list / control_thumbs if one or more scripts have stopped / started

  CGUIWindow::Render();
}

/*
bool CGUIWindowGameSaves::Update(const CStdString &strDirectory)
{
  if (m_thumbLoader.IsLoading())
    m_thumbLoader.StopThread();

  if (!CGUIMediaWindow::Update(strDirectory))
    return false;

  m_thumbLoader.Load(m_vecItems);
  return true;
}*/


bool CGUIWindowGameSaves::GetDirectory(const CStdString& strDirectory, CFileItemList& items)
{
  if (!m_rootDir.GetDirectory(strDirectory,items,false))
    return false;

  CLog::Log(LOGDEBUG,"CGUIWindowGameSaves::GetDirectory (%s)", strDirectory.c_str());

  CStdString strParentPath;
  bool bParentExists = CUtil::GetParentPath(strDirectory, strParentPath);
  if (bParentExists)
    m_strParentPath = strParentPath;
  else
    m_strParentPath = "";
  //FILE *newfile;
  CFile newfile;
  // flatten any folders with 1 save
  DWORD dwTick=timeGetTime();
  bool bProgressVisible = false;
  CGUIDialogProgress* m_dlgProgress = (CGUIDialogProgress*)m_gWindowManager.GetWindow(WINDOW_DIALOG_PROGRESS);
  for (int i = 0; i < items.Size(); i++)
  {
    CFileItem *item = items[i];

    if (!bProgressVisible && timeGetTime()-dwTick>1500 && m_dlgProgress)
    { // tag loading takes more then 1.5 secs, show a progress dialog
      m_dlgProgress->SetHeading(189);
      m_dlgProgress->SetLine(0, 20120);
      m_dlgProgress->SetLine(1,"");
      m_dlgProgress->SetLine(2, "");
      m_dlgProgress->StartModal();
      bProgressVisible = true;
    }
    if (!item->m_bIsFolder)
    {
      items.Remove(i);
      i--;
      continue;
    }

    if (item->m_bIsFolder && !item->IsParentFolder() && !item->m_bIsShareOrDrive)
    {
      if (bProgressVisible)
      {
        m_dlgProgress->SetLine(2,item->GetLabel());
        m_dlgProgress->Progress();
      }

      CStdString titlemetaXBX;
      CStdString savemetaXBX;
      CUtil::AddFileToFolder(item->m_strPath, "titlemeta.xbx", titlemetaXBX);
      CUtil::AddFileToFolder(item->m_strPath, "savemeta.xbx", savemetaXBX);
      int domode = 0;
      if (CFile::Exists(titlemetaXBX))
      {
        domode = 1;
        newfile.Open(titlemetaXBX);
      }
      else if (CFile::Exists(savemetaXBX))
      {
        domode = 2;
        newfile.Open(savemetaXBX);
      }
      if (domode != 0)
      {
        WCHAR *yo = new WCHAR[(int)newfile.GetLength()+1];
        newfile.Read(yo,newfile.GetLength());
        CStdString strDescription;
        g_charsetConverter.utf16toUTF8(yo, strDescription);
        int poss = strDescription.find("Name=");
        int pose = strDescription.find("\n",poss+1);
        strDescription = strDescription.Mid(poss+5, pose - poss-6);
        strDescription = CUtil::MakeLegalFileName(strDescription,true,false);
        newfile.Close();
        if (domode == 1)
        {
          CLog::Log(LOGDEBUG, "Loading GameSave info from %s,  data is %s, located at poss %i  pose %i ",  savemetaXBX.c_str(),strDescription.c_str(),poss,pose);
          // check for subfolders with savemeta.xbx
          CFileItemList items2;
          m_rootDir.GetDirectory(item->m_strPath,items2,false);
          int j;
          for (j=0;j<items2.Size();++j)
          {
            if (items2[j]->m_bIsFolder)
            {
              CStdString strPath;
              CUtil::AddFileToFolder(items2[j]->m_strPath,"savemeta.xbx",strPath);
              if (CFile::Exists(strPath))
                break;
            }
          }
          if (j == items2.Size())
          {
            item->m_bIsFolder = false;
            item->m_strPath = titlemetaXBX;
          }
        }
        else
        {
          item->m_bIsFolder = false;
          item->m_strPath = savemetaXBX;
        }
        CLog::Log(LOGINFO, "Loading GameSave info from %s,  data is %s, located at poss %i  pose %i ", titlemetaXBX.c_str(),strDescription.c_str(),poss,pose);
        item->m_musicInfoTag.SetTitle(item->GetLabel());  // Note we set ID as the TITLE to save code makign a SORT ID and a ID varible to the FileItem
        item->SetLabel(strDescription);
        item->FillInDefaultIcon();
        CGUIViewState::LABEL_MASKS labelMasks;
        m_guiState->GetSortMethodLabelMasks(labelMasks);

        item->FormatLabel2(labelMasks.m_strLabel2File);
        item->SetLabelPreformated(true);
      }
    }
  }
  items.SetGameSavesThumbs();
  if (bProgressVisible)
    m_dlgProgress->Close();

  return true;
}

/*
bool CGUIWindowGameSaves::DownloadSaves(CFileItem item)
{
  CStdString strURL;
  string theHtml;
  CHTTP http;

  strURL.Format("http://www.xboxmediacenter.com/xbmc.php?gameid=%s",item.m_musicInfoTag.GetTitle()); // donnos little fix the unleashx.php is broken (content lenght is greater then lenght sent)
  if (http.Get(strURL, theHtml))
  {
    TiXmlDocument gsXml;
    gsXml.Parse(theHtml.c_str(), 0);
    TiXmlElement *pRootElement = gsXml.RootElement();
    if (pRootElement)
    {
      if (pRootElement->Value())
      {
        if (strcmpi(pRootElement->Value(),"xboxsaves") == 0)
        {
          TiXmlElement* pGame = pRootElement->FirstChildElement("game");
          if (pGame)
          {
            TiXmlElement* pSave = pGame->FirstChildElement("save");
            if (pSave->Value())
            {
              CGUIDialogSelect *pDlg = (CGUIDialogSelect*)m_gWindowManager.GetWindow(WINDOW_DIALOG_SELECT);
              if (pDlg)
              {
                pDlg->SetHeading(20320);
                pDlg->Reset();
                pDlg->EnableButton(false);
                std::vector<CStdString> vecSaveUrl;
                vecSaveUrl.clear();
                while (pSave)
                {
                  if (pSave->Value())
                  {
                      pDlg->Add(pSave->Attribute("desc"));
                      vecSaveUrl.push_back(pSave->Attribute("url"));
                  }
                  pSave = pSave->NextSiblingElement("save");
                }
                //pDlg->Sort();
                pDlg->DoModal();
                int iSelectedSave = pDlg->GetSelectedLabel();
                CLog::Log(LOGINFO,"GSM: %i",  iSelectedSave);
                if (iSelectedSave > -1)
                {
                  CLog::Log(LOGINFO,"GSM vecSaveUrl[i] : %s",  vecSaveUrl[iSelectedSave].c_str());
                  if (http.Download(vecSaveUrl[iSelectedSave], "Z:\\gamesave.zip"))
                  {
                    if (g_ZipManager.ExtractArchive("Z:\\gamesave.zip","E:\\"))
                    {
                      ::DeleteFile("E:\\gameid.ini");   // delete file E:\\gameid.ini artifcat continatin info about the save we got
                      CGUIDialogOK::ShowAndGetInput(20317, 0, 20318, 0);
                      return true;
                    }
                  }
                  CGUIDialogOK::ShowAndGetInput(20317, 0, 20319, 0);  // Download Failed
                  CLog::Log(LOGINFO,"GSM: Failed to download: %s",  vecSaveUrl[iSelectedSave].c_str());
                  return true;
                }
                CLog::Log(LOGINFO,"GSM: Action Aborted: %s",  vecSaveUrl[iSelectedSave].c_str());
                return true;
              }
            }
          }
        }
      }
    }
  }
  return false;
}*/

void CGUIWindowGameSaves::OnPopupMenu(int iItem)
{
  if (m_vecItems[iItem]->m_bIsShareOrDrive)
    return;

  // calculate our position
  float posX = 200;
  float posY = 100;
  //CGUIListControl *pList = (CGUIListControl *)GetControl(CONTROL_LIST);
  const CGUIControl *pList = GetControl(CONTROL_LIST);
  if (pList)
  {
    posX = pList->GetXPosition() + pList->GetWidth() / 2;
    posY = pList->GetYPosition() + pList->GetHeight() / 2;
  }
  // popup the context menu
  CGUIDialogContextMenu *pMenu = (CGUIDialogContextMenu *)m_gWindowManager.GetWindow(WINDOW_DIALOG_CONTEXT_MENU);
  if (!pMenu) return ;
  CFileItem* pItem=m_vecItems[iItem];
  CFileItem item(*pItem);
  std::vector<CStdString> token;
  CUtil::Tokenize(pItem->m_strPath,token,"\\");
  CStdString strFileName = CUtil::GetFileName(item.m_strPath);
  // load our menu
  pMenu->Initialize();
  // add the needed buttons
  int btnCopy = pMenu->AddButton(115);
  int btnMove = pMenu->AddButton(116);
  int btnDelete = pMenu->AddButton(117);
  // ONly add if we are on E:\udata\    /
  /*int btnDownload = pMenu->AddButton(20317);

  pMenu->EnableButton(btnDownload, !strFileName.Equals("savemeta.xbx"));*/
  // position it correctly

  pMenu->SetPosition(posX - pMenu->GetWidth() / 2, posY - pMenu->GetHeight() / 2);
  pMenu->DoModal();
  int iButton = pMenu->GetButton();


  if (iButton == btnCopy)
  {
    CStdString value;
    if (!CGUIDialogFileBrowser::ShowAndGetDirectory(g_settings.m_vecMyFilesShares, "to copy save to", value,true))
      return;
    if (!CGUIDialogYesNo::ShowAndGetInput(120,123,20022,20022)) // enable me for confirmation
      return;
    CStdString path;
    if (strFileName.Equals("savemeta.xbx") || strFileName.Equals("titlemeta.xbx") )
    {
      CUtil::GetDirectory(pItem->m_strPath,item.m_strPath);
      item.m_bIsFolder = true;
      // first copy the titlemeta dir
      CFileItemList items2;
      CStdString strParent;
      CUtil::GetParentPath(item.m_strPath,strParent);
      CDirectory::GetDirectory(strParent,items2);
      CUtil::AddFileToFolder(value,CUtil::GetFileName(strParent),path);
      for (int j=0;j<items2.Size();++j)
      {
        if (!items2[j]->m_bIsFolder)
        {
          CStdString strDest;
          CUtil::AddFileToFolder(path,CUtil::GetFileName(items2[j]->m_strPath),strDest);
          CFile::Cache(items2[j]->m_strPath,strDest);
        }
      }
      CUtil::AddFileToFolder(path,CUtil::GetFileName(item.m_strPath),path);
    }
    else
    {
      CUtil::AddFileToFolder(value,CUtil::GetFileName(item.m_strPath),path);
    }

    item.Select(true);
    CLog::Log(LOGINFO,"GSM: Copy of folder confirmed for folder %s",  item.m_strPath.c_str());
    CGUIWindowFileManager::CopyItem(&item,path,true);
  }

  if (iButton == btnDelete)
  {
    CLog::Log(LOGINFO,"GSM: Deletion of folder confirmed for folder %s", pItem->m_strPath.c_str());
    if (strFileName.Equals("savemeta.xbx") || strFileName.Equals("titlemeta.xbx"))
    {
      CUtil::GetDirectory(pItem->m_strPath,item.m_strPath);
      item.m_bIsFolder = true;
    }

    item.Select(true);
    if (CGUIWindowFileManager::DeleteItem(&item))
    {
      CFile::Delete(item.GetThumbnailImage());
      Update(m_vecItems.m_strPath);
    }
  }
  if (iButton == btnMove)
  {
    CStdString value;
    if (!CGUIDialogFileBrowser::ShowAndGetDirectory(g_settings.m_vecMyFilesShares, "to copy save to", value,true))
      return;
    if (!CGUIDialogYesNo::ShowAndGetInput(121,124,20022,20022)) // enable me for confirmation
      return;
    CStdString path;
    if (strFileName.Equals("savemeta.xbx") || strFileName.Equals("titlemeta.xbx"))
    {
      CUtil::GetDirectory(pItem->m_strPath,item.m_strPath);
      item.m_bIsFolder = true;
      // first copy the titlemeta dir
      CFileItemList items2;
      CStdString strParent;
      CUtil::GetParentPath(item.m_strPath,strParent);
      CDirectory::GetDirectory(strParent,items2);

      CUtil::AddFileToFolder(value,CUtil::GetFileName(strParent),path);
      for (int j=0;j<items2.Size();++j) // only copy main title stuff
      {
        if (!items2[j]->m_bIsFolder)
        {
          CStdString strDest;
          CUtil::AddFileToFolder(path,CUtil::GetFileName(items2[j]->m_strPath),strDest);
          CFile::Cache(items2[j]->m_strPath,strDest);
        }
      }

      CUtil::AddFileToFolder(path,CUtil::GetFileName(item.m_strPath),path);
    }
    else
    {
      CUtil::AddFileToFolder(value,CUtil::GetFileName(item.m_strPath),path);
    }

    item.Select(true);
    CLog::Log(LOGINFO,"GSM: Copy of folder confirmed for folder %s",  item.m_strPath.c_str());
    CGUIWindowFileManager::MoveItem(&item,path,true);
    CDirectory::Remove(item.m_strPath);
    Update(m_vecItems.m_strPath);
  }
  /*
  if (iButton == btnDownload)
  {
    CFileItem item(*pItem);

    CHTTP http;
    CStdString strURL;
    if (item.m_musicInfoTag.GetTitle() != "")
    {
      if (!CGUIWindowGameSaves::DownloadSaves(item))
      {
        CGUIDialogOK::ShowAndGetInput(20317, 0, 20321, 0);  // No Saves found
        CLog::Log(LOGINFO,"GSM: No saves available for game on internet: %s",  item.GetLabel().c_str());
      }
      else
      {
        Update(m_vecItems.m_strPath);
      }
    }
  }*/
  //CGUIMessage msg(GUI_MSG_ITEM_SELECT, GetID(),CONTROL_LIST,iItem);
  //OnMessage(msg);
}
