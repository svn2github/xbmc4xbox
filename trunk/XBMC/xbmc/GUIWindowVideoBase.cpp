/*
 *      Copyright (C) 2005-2007 Team XboxMediaCenter
 *      http://www.xboxmediacenter.com
 *
 *  This Program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your option)
 *  any later version.
 *
 *  This Program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with GNU Make; see the file COPYING.  If not, write to
 *  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 *  http://www.gnu.org/copyleft/gpl.html
 *
 */

#include "stdafx.h"
#include "GUIWindowVideoBase.h"
#include "Util.h"
#include "Utils/IMDB.h"
#include "Utils/HTTP.h"
#include "Utils/RegExp.h"
#include "Utils/GUIInfoManager.h"
#include "GUIWindowVideoInfo.h"
#include "GUIDialogFileBrowser.h"
#include "GUIDialogVideoScan.h"
#include "PlayListFactory.h"
#include "Application.h"
#include "NFOFile.h"
#include "Picture.h"
#include "utils/fstrcmp.h"
#include "PlayListPlayer.h"
#ifdef PRE_SKIN_VERSION_2_1_COMPATIBILITY
#include "GUIFontManager.h"
#endif
#include "GUIPassword.h"
#include "FileSystem/ZipManager.h"
#include "FileSystem/StackDirectory.h"
#include "GUIDialogContextMenu.h"
#include "GUIDialogFileStacking.h"
#include "GUIDialogMediaSource.h"
#include "GUIWindowFileManager.h"
#include "FileSystem/VideoDatabaseDirectory.h"

#include "SkinInfo.h"

using namespace XFILE;
using namespace DIRECTORY;
using namespace PLAYLIST;
using namespace VIDEODATABASEDIRECTORY;

#define CONTROL_BTNVIEWASICONS     2
#define CONTROL_BTNSORTBY          3
#define CONTROL_BTNSORTASC         4
#define CONTROL_BTNTYPE            5
#define CONTROL_LIST              50
#define CONTROL_THUMBS            51
#define CONTROL_BIGLIST           52
#define CONTROL_LABELFILES        12
#define CONTROL_LABELEMPTY        13

#define CONTROL_PLAY_DVD          6
#define CONTROL_STACK             7
#define CONTROL_BTNSCAN           8
#define CONTROL_IMDB              9

CGUIWindowVideoBase::CGUIWindowVideoBase(DWORD dwID, const CStdString &xmlFile)
    : CGUIMediaWindow(dwID, xmlFile)
{
  m_bDisplayEmptyDatabaseMessage = false;
  m_thumbLoader.SetObserver(this);
}

CGUIWindowVideoBase::~CGUIWindowVideoBase()
{
}

bool CGUIWindowVideoBase::OnMessage(CGUIMessage& message)
{
  switch ( message.GetMessage() )
  {
  case GUI_MSG_WINDOW_DEINIT:
    if (m_thumbLoader.IsLoading())
      m_thumbLoader.StopThread();
    m_database.Close();
    break;

  case GUI_MSG_WINDOW_INIT:
    {
      m_database.Open();

      m_dlgProgress = (CGUIDialogProgress*)m_gWindowManager.GetWindow(WINDOW_DIALOG_PROGRESS);

      // save current window, unless the current window is the video playlist window
      if (GetID() != WINDOW_VIDEO_PLAYLIST && g_stSettings.m_iVideoStartWindow != GetID())
      {
        g_stSettings.m_iVideoStartWindow = GetID();
        g_settings.Save();
      }

      return CGUIMediaWindow::OnMessage(message);
    }
    break;

  case GUI_MSG_CLICKED:
    {
      int iControl = message.GetSenderId();
      if (iControl == CONTROL_PLAY_DVD)
      {
        // play movie...
        CUtil::PlayDVD();
      }
      else if (iControl == CONTROL_BTNTYPE)
      {
        CGUIMessage msg(GUI_MSG_ITEM_SELECTED, GetID(), CONTROL_BTNTYPE);
        m_gWindowManager.SendMessage(msg);

        int nSelected = msg.GetParam1();
        int nNewWindow = WINDOW_VIDEO_FILES;
        switch (nSelected)
        {
        case 0:  // Movies
          nNewWindow = WINDOW_VIDEO_FILES;
          break;
        case 1:  // Library
          nNewWindow = WINDOW_VIDEO_NAV;
          break;
        }

        if (nNewWindow != GetID())
        {
          g_stSettings.m_iVideoStartWindow = nNewWindow;
          g_settings.Save();
          m_gWindowManager.ChangeActiveWindow(nNewWindow);
          CGUIMessage msg2(GUI_MSG_SETFOCUS, nNewWindow, CONTROL_BTNTYPE);
          g_graphicsContext.SendMessage(msg2);
        }

        return true;
      }
      else if (m_viewControl.HasControl(iControl))  // list/thumb control
      {
        // get selected item
        int iItem = m_viewControl.GetSelectedItem();
        int iAction = message.GetParam1();

        // iItem is checked for validity inside these routines
        if (iAction == ACTION_QUEUE_ITEM || iAction == ACTION_MOUSE_MIDDLE_CLICK)
        {
          OnQueueItem(iItem);
          return true;
        }
        else if (iAction == ACTION_SHOW_INFO)
        {
          SScraperInfo info;
          CStdString strDir;
          if (iItem < 0 || iItem >= m_vecItems.Size())
            return false;

          if (m_vecItems[iItem]->IsVideoDb() && m_vecItems[iItem]->HasVideoInfoTag() && !m_vecItems[iItem]->GetVideoInfoTag()->m_strPath.IsEmpty())
          {
            strDir = m_vecItems[iItem]->GetVideoInfoTag()->m_strPath;
          }
          else
            CUtil::GetDirectory(m_vecItems[iItem]->m_strPath,strDir);

          int iFound;
          m_database.GetScraperForPath(strDir,info.strPath,info.strContent,iFound);
          CScraperParser parser;
          if (parser.Load("q:\\system\\scrapers\\video\\"+info.strPath))
            info.strTitle = parser.GetName();

          if (info.strContent.IsEmpty() && !(m_database.HasMovieInfo(m_vecItems[iItem]->m_strPath) || m_database.HasTvShowInfo(strDir) || m_database.HasEpisodeInfo(m_vecItems[iItem]->m_strPath)))
          {
            // hack
            CStdString strOldPath = m_vecItems[iItem]->m_strPath;
            m_vecItems[iItem]->m_strPath = strDir;
            OnAssignContent(iItem,1,info);
            m_vecItems[iItem]->m_strPath = strOldPath;
            return true;
          }

          if (info.strContent.Equals("tvshows") && iFound == 1) // dont lookup on root tvshow folder
            return true;

          OnInfo(iItem,info);

          return true;
        }
        else if (iAction == ACTION_CONTEXT_MENU || iAction == ACTION_MOUSE_RIGHT_CLICK)
        {
          OnPopupMenu(iItem);
          return true;
        }
        else if (iAction == ACTION_PLAYER_PLAY && !g_application.IsPlayingVideo())
        {
          OnResumeItem(iItem);
          return true;
        }
        else if (iAction == ACTION_DELETE_ITEM)
        {
          // is delete allowed?
          // must be at the title window
          if (GetID() == WINDOW_VIDEO_NAV)
            OnDeleteItem(iItem);

          // or be at the files window and have file deletion enabled
          else if (GetID() == WINDOW_VIDEO_FILES && g_guiSettings.GetBool("filelists.allowfiledeletion"))
            OnDeleteItem(iItem);

          // or be at the video playlists location
          else if (m_vecItems.m_strPath.Equals("special://videoplaylists/"))
            OnDeleteItem(iItem);
          else
            return false;
          
          return true;
        }
      }
      else if (iControl == CONTROL_IMDB)
      {
        OnManualIMDB();
      }
    }
    break;
  }
  return CGUIMediaWindow::OnMessage(message);
}

void CGUIWindowVideoBase::UpdateButtons()
{
  // Remove labels from the window selection
  CGUIMessage msg(GUI_MSG_LABEL_RESET, GetID(), CONTROL_BTNTYPE);
  g_graphicsContext.SendMessage(msg);

  // Add labels to the window selection
  CStdString strItem = g_localizeStrings.Get(744); // Files
  CGUIMessage msg2(GUI_MSG_LABEL_ADD, GetID(), CONTROL_BTNTYPE);
  msg2.SetLabel(strItem);
  g_graphicsContext.SendMessage(msg2);

  strItem = g_localizeStrings.Get(14022); // Library
  msg2.SetLabel(strItem);
  g_graphicsContext.SendMessage(msg2);

  // Select the current window as default item
  int nWindow = g_stSettings.m_iVideoStartWindow-WINDOW_VIDEO_FILES;
  CONTROL_SELECT_ITEM(CONTROL_BTNTYPE, nWindow);

  // disable scan and manual imdb controls if internet lookups are disabled
  if (g_guiSettings.GetBool("network.enableinternet"))
  {
    CONTROL_ENABLE(CONTROL_BTNSCAN);
    CONTROL_ENABLE(CONTROL_IMDB);
  }
  else
  {
    CONTROL_DISABLE(CONTROL_BTNSCAN);
    CONTROL_DISABLE(CONTROL_IMDB);
  }

  CGUIMediaWindow::UpdateButtons();
}

void CGUIWindowVideoBase::OnInfo(int iItem, const SScraperInfo& info)
{
  if ( iItem < 0 || iItem >= m_vecItems.Size() ) return ;
  CFileItem* pItem = m_vecItems[iItem];
  // ShowIMDB can kill the item as this window can be closed while we do it,
  // so take a copy of the item now
  CFileItem item(*pItem);
  if (item.IsVideoDb() && item.HasVideoInfoTag())
  {
    if (item.GetVideoInfoTag()->m_strFileNameAndPath.IsEmpty())
      item.m_strPath = item.GetVideoInfoTag()->m_strPath;
    else
      item.m_strPath = item.GetVideoInfoTag()->m_strFileNameAndPath;
  }
  ShowIMDB(&item, info);
  Update(m_vecItems.m_strPath);
}

// ShowIMDB is called as follows:
// 1.  To lookup info on a file.
// 2.  To lookup info on a folder (which may or may not contain a file)
// 3.  To lookup info just for fun (no file or folder related)

// We just need the item object for this.
// A "blank" item object is sent for 3.
// If a folder is sent, currently it sets strFolder and bFolder
// this is only used for setting the folder thumb, however.

// Steps should be:

// 1.  Check database to see if we have this information already
// 2.  Else, check for a nfoFile to get the URL
// 3.  Run a loop to check for refresh
// 4.  If no URL is present do a search to get the URL
// 4.  Once we have the URL, download the details
// 5.  Once we have the details, add to the database if necessary (case 1,2)
//     and show the information.
// 6.  Check for a refresh, and if so, go to 3.

void CGUIWindowVideoBase::ShowIMDB(CFileItem *item, const SScraperInfo& info)
{
  /*
  CLog::Log(LOGDEBUG,"CGUIWindowVideoBase::ShowIMDB");
  CLog::Log(LOGDEBUG,"  strMovie  = [%s]", strMovie.c_str());
  CLog::Log(LOGDEBUG,"  strFile   = [%s]", strFile.c_str());
  CLog::Log(LOGDEBUG,"  strFolder = [%s]", strFolder.c_str());
  CLog::Log(LOGDEBUG,"  bFolder   = [%s]", ((int)bFolder ? "true" : "false"));
  */

  CGUIDialogProgress* pDlgProgress = (CGUIDialogProgress*)m_gWindowManager.GetWindow(WINDOW_DIALOG_PROGRESS);
  CGUIDialogSelect* pDlgSelect = (CGUIDialogSelect*)m_gWindowManager.GetWindow(WINDOW_DIALOG_SELECT);
  CGUIWindowVideoInfo* pDlgInfo = (CGUIWindowVideoInfo*)m_gWindowManager.GetWindow(WINDOW_VIDEO_INFO);

  CIMDB IMDB;
  IMDB.SetScraperInfo(info);

  bool bUpdate = false;
  bool bFound = false;

  if (!pDlgProgress) return ;
  if (!pDlgSelect) return ;
  if (!pDlgInfo) return ;
  CUtil::ClearCache();

  // 1.  Check for already downloaded information, and if we have it, display our dialog
  //     Return if no Refresh is needed.
  bool bHasInfo=false;

  CVideoInfoTag movieDetails;
  movieDetails.Reset();
  if (info.strContent.Equals("movies"))
  {
    g_infoManager.m_content = "movies";
    if (m_database.HasMovieInfo(item->m_strPath))
    {
      bHasInfo = true;
      m_database.GetMovieInfo(item->m_strPath, movieDetails);
    }
  }
  if (info.strContent.Equals("tvshows"))
  {
    if (item->m_bIsFolder)
    {
      g_infoManager.m_content = "tvshows";
      if (m_database.HasTvShowInfo(item->m_strPath))
      {
        bHasInfo = true;
        m_database.GetTvShowInfo(item->m_strPath, movieDetails);
      }
    }
    else
    {
      long lEpisodeHint=-1;
      if (item->HasVideoInfoTag())
        lEpisodeHint = item->GetVideoInfoTag()->m_iEpisode;
      long lEpisodeId=-1;
      g_infoManager.m_content = "episodes";
      if ((lEpisodeId = m_database.GetEpisodeInfo(item->m_strPath,lEpisodeHint)) > -1)
      {
        bHasInfo = true;
        m_database.GetEpisodeInfo(item->m_strPath, movieDetails, lEpisodeId);
      }
    }
  }
  if (bHasInfo)
  {
    if (info.strContent.IsEmpty()) // disable refresh button
      movieDetails.m_strIMDBNumber = "xx"+movieDetails.m_strIMDBNumber;
    *item->GetVideoInfoTag() = movieDetails;
    pDlgInfo->SetMovie(item);
    pDlgInfo->DoModal();
    item->SetThumbnailImage(pDlgInfo->GetThumbnail());
    if ( !pDlgInfo->NeedRefresh() ) return ;
  }
  
  // quietly return if Internet lookups are disabled
  if (!g_guiSettings.GetBool("network.enableinternet")) return ;
  if (!g_settings.m_vecProfiles[g_settings.m_iLastLoadedProfileIndex].canWriteDatabases() && !g_passwordManager.bMasterUser)
    return;

  CIMDBUrl url;
  bool hasDetails(false);

  // 2. Look for a nfo File to get the search URL
  CStdString nfoFile = CVideoInfoScanner::GetnfoFile(item);
  if ( !nfoFile.IsEmpty() )
  {
    CLog::Log(LOGDEBUG,"Found matching nfo file: %s", nfoFile.c_str());
    CNfoFile nfoReader(info.strContent);
    if ( nfoReader.Create(nfoFile) == S_OK)
    {
      if (nfoReader.m_strScraper == "NFO")
      {
        CLog::Log(LOGDEBUG, __FUNCTION__" Got details from nfo");
        nfoReader.GetDetails(movieDetails);
        hasDetails = true;
      }
      else
      {
        CScraperUrl scrUrl(nfoReader.m_strImDbUrl); 
	      url.m_scrURL.push_back(scrUrl);
        url.m_strID = nfoReader.m_strImDbNr;
        SScraperInfo info2(info);
        info2.strPath = nfoReader.m_strScraper;
        IMDB.SetScraperInfo(info2);
        CLog::Log(LOGDEBUG,"-- nfo scraper: %s", nfoReader.m_strScraper.c_str());
        CLog::Log(LOGDEBUG,"-- nfo url: %s", url.m_scrURL[0].m_url.c_str());
      }
    }
    else
      CLog::Log(LOGERROR,"Unable to find an imdb url in nfo file: %s", nfoFile.c_str());
  }

  CStdString movieName = item->GetLabel();
  // 3. Run a loop so that if we Refresh we re-run this block
  bool needsRefresh(false);
  do
  {
    // 4. if we don't have a url, or need to refresh the search
    //    then do the web search
    if (!hasDetails && (url.m_scrURL.size() == 0 || needsRefresh))
    {
      // 4a. show dialog that we're busy querying www.imdb.com
      CStdString strHeading;
      CScraperParser parser;
      SScraperInfo info2(info);
      parser.Load("Q:\\system\\scrapers\\video\\"+info.strPath);
      info2.strTitle = parser.GetName();
      IMDB.SetScraperInfo(info2);
      strHeading.Format(g_localizeStrings.Get(197),info2.strTitle.c_str());
      pDlgProgress->SetHeading(strHeading);
      pDlgProgress->SetLine(0, movieName);
      pDlgProgress->SetLine(1, "");
      pDlgProgress->SetLine(2, "");
      pDlgProgress->StartModal();
      pDlgProgress->Progress();

      // 4b. do the websearch
      IMDB_MOVIELIST movielist;
      if (info.strContent.Equals("tvshows") && !item->m_bIsFolder)
        hasDetails = true;

      if (!hasDetails && IMDB.FindMovie(movieName, movielist, pDlgProgress))
      {
        pDlgProgress->Close();
        if (movielist.size() > 0)
        {
          int iString = 196;
          if (info.strContent.Equals("tvshows"))
            iString = 20356;
          pDlgSelect->SetHeading(iString);
          pDlgSelect->Reset();
          for (unsigned int i = 0; i < movielist.size(); ++i)
            pDlgSelect->Add(movielist[i].m_strTitle);
          pDlgSelect->EnableButton(true);
          pDlgSelect->SetButtonLabel(413); // manual
          pDlgSelect->DoModal();

          // and wait till user selects one
          int iSelectedMovie = pDlgSelect->GetSelectedLabel();
          if (iSelectedMovie >= 0)
            url = movielist[iSelectedMovie];
          else if (!pDlgSelect->IsButtonPressed())
          {
            return; // user backed out
          }
        }
      }
    }
    // 4c. Check if url is still empty - occurs if user has selected to do a manual
    //     lookup, or if the IMDb lookup failed or was cancelled.
    if (!hasDetails && url.m_scrURL.size() == 0)
    {
      // Check for cancel of the progress dialog
      pDlgProgress->Close();
      if (pDlgProgress->IsCanceled())
      {
        return;
      }

      // Prompt the user to input the movieName
      int iString = 16009;
      if (info.strContent.Equals("tvshows"))
        iString = 20357;
      if (!CGUIDialogKeyboard::ShowAndGetInput(movieName, g_localizeStrings.Get(iString), false))
      {
        return; // user backed out
      }

      needsRefresh = true;
    }
    else
    {
      // 5. Download the movie information
      // show dialog that we're downloading the movie info
      CVideoInfoScanner scanner;
      CFileItemList list;
      CStdString strPath=item->m_strPath;
      if (item->IsVideoDb())
      {
        list.Add(new CFileItem(*item->GetVideoInfoTag()));
        strPath = item->GetVideoInfoTag()->m_strPath;
      }
      else
        list.Add(new CFileItem(*item));

      if (item->m_bIsFolder)
        CUtil::GetParentPath(strPath,list.m_strPath);
      else
        CUtil::GetDirectory(strPath,list.m_strPath);

      int iString=198;
      if (info.strContent.Equals("tvshows"))
      {
        if (item->m_bIsFolder)
          iString = 20353;
        else
          iString = 20361;
      }
      pDlgProgress->SetHeading(iString);
      pDlgProgress->SetLine(0, movieName);
      pDlgProgress->SetLine(1, url.m_strTitle);
      pDlgProgress->SetLine(2, "");
      pDlgProgress->StartModal();
      pDlgProgress->Progress();
      if (bHasInfo)
      {
        if (info.strContent.Equals("movies"))
          m_database.DeleteMovie(item->m_strPath);
        if (info.strContent.Equals("tvshows") && !item->m_bIsFolder)
          m_database.DeleteEpisode(item->m_strPath,movieDetails.m_iDbId);
        if (info.strContent.Equals("tvshows") && item->m_bIsFolder)
          m_database.DeleteTvShow(item->m_strPath);
      }
      if (scanner.RetrieveVideoInfo(list,false,info,&url,pDlgProgress))
      {
        if (info.strContent.Equals("movies"))
          m_database.GetMovieInfo(item->m_strPath,movieDetails);
        if (info.strContent.Equals("tvshows"))
        {
          // update tvshow info to get updated episode numbers
          if (item->m_bIsFolder)
            m_database.GetTvShowInfo(item->m_strPath,movieDetails); 
          else
            m_database.GetEpisodeInfo(item->m_strPath,movieDetails); 
        }

        // got all movie details :-)
        OutputDebugString("got details\n");
        pDlgProgress->Close();

        // now show the imdb info
        OutputDebugString("show info\n");

        // Add to the database if applicable
        if (info.strContent.Equals("movies") && item->m_strPath && (g_settings.m_vecProfiles[g_settings.m_iLastLoadedProfileIndex].canWriteDatabases() || g_passwordManager.bMasterUser))
        {
          m_database.SetDetailsForMovie(item->m_strPath, movieDetails);
        }

        // remove directory caches
        CUtil::DeleteVideoDatabaseDirectoryCache();

        *item->GetVideoInfoTag() = movieDetails;
        pDlgInfo->SetMovie(item);
        pDlgInfo->DoModal();
        item->SetThumbnailImage(pDlgInfo->GetThumbnail());
        needsRefresh = pDlgInfo->NeedRefresh();
      }
      else
      {
        pDlgProgress->Close();
        if (pDlgProgress->IsCanceled())
        {
          return; // user cancelled
        }
        OutputDebugString("failed to get details\n");
        // show dialog...
        CGUIDialogOK *pDlgOK = (CGUIDialogOK*)m_gWindowManager.GetWindow(WINDOW_DIALOG_OK);
        if (pDlgOK)
        {
          pDlgOK->SetHeading(195);
          pDlgOK->SetLine(0, movieName);
          pDlgOK->SetLine(1, "");
          pDlgOK->SetLine(2, "");
          pDlgOK->SetLine(3, "");
          pDlgOK->DoModal();
        }
        return;
      }
    }
  // 6. Check for a refresh
  } while (needsRefresh);
}

void CGUIWindowVideoBase::Render()
{
  if (m_bDisplayEmptyDatabaseMessage)
  {
    SET_CONTROL_LABEL(CONTROL_LABELEMPTY,g_localizeStrings.Get(745)+'\n'+g_localizeStrings.Get(746))
  }
  else
  {
    SET_CONTROL_LABEL(CONTROL_LABELEMPTY,"")
  }

  CGUIMediaWindow::Render();
}

void CGUIWindowVideoBase::OnManualIMDB()
{
  CStdString strInput;
  if (!CGUIDialogKeyboard::ShowAndGetInput(strInput, g_localizeStrings.Get(16009), false)) return ;

  CFileItem item(strInput);
  item.m_strPath = "Z:\\";
  ::DeleteFile(item.GetCachedVideoThumb().c_str());

  SScraperInfo info;
  info.strContent = "movies";
  info.strPath = "imdb.xml";
  info.strTitle = "IMDb";

  ShowIMDB(&item,info);
  
  return ;
}

bool CGUIWindowVideoBase::IsCorrectDiskInDrive(const CStdString& strFileName, const CStdString& strDVDLabel)
{
  CDetectDVDMedia::WaitMediaReady();
  CCdInfo* pCdInfo = CDetectDVDMedia::GetCdInfo();
  if (pCdInfo == NULL) return false;
  if (!CFile::Exists(strFileName)) return false;
  CStdString label = pCdInfo->GetDiscLabel().TrimRight(" ");
  int iLabelCD = label.GetLength();
  int iLabelDB = strDVDLabel.GetLength();
  if (iLabelDB < iLabelCD) return false;
  CStdString dbLabel = strDVDLabel.Left(iLabelCD);
  return (dbLabel == label);
}

bool CGUIWindowVideoBase::CheckMovie(const CStdString& strFileName)
{
  if (!m_database.HasMovieInfo(strFileName) ) return true;

  CVideoInfoTag movieDetails;
  m_database.GetMovieInfo(strFileName, movieDetails);
  CFileItem movieFile(movieDetails.m_strFileNameAndPath, false);
  if ( !movieFile.IsOnDVD()) return true;
  CGUIDialogOK *pDlgOK = (CGUIDialogOK*)m_gWindowManager.GetWindow(WINDOW_DIALOG_OK);
  if (!pDlgOK) return true;
  while (1)
  {
//    if (IsCorrectDiskInDrive(strFileName, movieDetails.m_strDVDLabel))
 //   {
      return true;
 //   }
    pDlgOK->SetHeading( 428);
    pDlgOK->SetLine( 0, 429 );
//    pDlgOK->SetLine( 1, movieDetails.m_strDVDLabel );
    pDlgOK->SetLine( 2, "" );
    pDlgOK->DoModal();
    if (!pDlgOK->IsConfirmed())
    {
      break;
    }
  }
  return false;
}

void CGUIWindowVideoBase::OnQueueItem(int iItem)
{
  if ( iItem < 0 || iItem >= m_vecItems.Size() ) return ;

  int iOldSize=g_playlistPlayer.GetPlaylist(PLAYLIST_VIDEO).size();

  CFileItem item(*m_vecItems[iItem]);
  if (item.IsRAR() || item.IsZIP())
    return;

  //  Allow queuing of unqueueable items
  //  when we try to queue them directly
  if (!item.CanQueue())
    item.SetCanQueue(true);

  CFileItemList queuedItems;
  AddItemToPlayList(&item, queuedItems);
  g_playlistPlayer.Add(PLAYLIST_VIDEO, queuedItems);
  // video does not auto play on queue like music
  m_viewControl.SetSelectedItem(iItem + 1);
}

void CGUIWindowVideoBase::AddItemToPlayList(const CFileItem* pItem, CFileItemList &queuedItems)
{
  if (!pItem->CanQueue() || pItem->IsRAR() || pItem->IsZIP() || pItem->IsParentFolder()) // no zip/rar enques thank you!
    return;

  if (pItem->m_bIsFolder)
  {
    if (pItem->IsParentFolder()) return;

    // Check if we add a locked share
    if ( pItem->m_bIsShareOrDrive )
    {
      CFileItem item = *pItem;
      if ( !g_passwordManager.IsItemUnlocked( &item, "video" ) )
        return ;
    }

    // recursive
    CFileItemList items;
    GetDirectory(pItem->m_strPath, items);
    SortItems(items);

    for (int i = 0; i < items.Size(); ++i)
    {
      if (items[i]->m_bIsFolder)
      {
        CStdString strPath = items[i]->m_strPath;
        if (CUtil::HasSlashAtEnd(strPath))
          strPath.erase(strPath.size()-1);
        strPath.ToLower();
        if (strPath.size() > 6)
        {
          CStdString strSub = strPath.substr(strPath.size()-6);
          if (strPath.Mid(strPath.size()-6).Equals("sample")) // skip sample folders
            continue;
        }
      }
      AddItemToPlayList(items[i], queuedItems);
    }
  }
  else
  {
    // just an item
    if (pItem->IsPlayList())
    {
      auto_ptr<CPlayList> pPlayList (CPlayListFactory::Create(*pItem));
      if ( NULL != pPlayList.get())
      {
        // load it
        if (!pPlayList->Load(pItem->m_strPath))
        {
          CGUIDialogOK::ShowAndGetInput(6, 0, 477, 0);
          return; //hmmm unable to load playlist?
        }

        CPlayList playlist = *pPlayList;
        for (int i = 0; i < (int)playlist.size(); ++i)
          AddItemToPlayList(&playlist[i], queuedItems);
        return;
      }
    }
    else if(pItem->IsInternetStream())
    { // just queue the internet stream, it will be expanded on play
      queuedItems.Add(new CFileItem(*pItem));
    }
    else if (pItem->IsVideoDb())
    { // this case is needed unless we allow IsVideo() to return true for videodb items,
      // but then we have issues with playlists of videodb items
      queuedItems.Add(new CFileItem(*pItem->GetVideoInfoTag()));
    }
    else if (!pItem->IsNFO() && pItem->IsVideo())
    {
      queuedItems.Add(new CFileItem(*pItem));
    }

  }
}

void CGUIWindowVideoBase::DisplayEmptyDatabaseMessage(bool bDisplay)
{
  m_bDisplayEmptyDatabaseMessage = bDisplay;
}

int  CGUIWindowVideoBase::GetResumeItemOffset(const CFileItem *item)
{
  m_database.Open();
  long startoffset = 0;

  if (item->IsStack() && !g_guiSettings.GetBool("myvideos.treatstackasfile") )
  {

    CStdStringArray movies;
    GetStackedFiles(item->m_strPath, movies);

    /* check if any of the stacked files have a resume bookmark */
    for(unsigned i = 0; i<movies.size();i++)
    {
      CBookmark bookmark;
      if(m_database.GetResumeBookMark(movies[i], bookmark))
      {
        startoffset = (long)(bookmark.timeInSeconds*75);
        startoffset += 0x10000000 * (i+1); /* store file number in here */
        break;
      }
    }
  }
  else if (!item->IsNFO() && !item->IsPlayList())
  {
    CBookmark bookmark;
    CStdString strPath = item->m_strPath;
    if (item->IsVideoDb() && item->HasVideoInfoTag())
      strPath = item->GetVideoInfoTag()->m_strFileNameAndPath;

    if (m_database.GetResumeBookMark(strPath, bookmark))
      startoffset = (long)(bookmark.timeInSeconds*75);
  }
  m_database.Close();
  return startoffset;
}

bool CGUIWindowVideoBase::OnClick(int iItem)
{
  if (g_guiSettings.GetBool("myvideos.autoresume"))
    OnResumeItem(iItem);
  else
    return CGUIMediaWindow::OnClick(iItem);

  return true;
}

void CGUIWindowVideoBase::OnRestartItem(int iItem)
{
  CGUIMediaWindow::OnClick(iItem);
}

void CGUIWindowVideoBase::OnResumeItem(int iItem)
{
  m_vecItems[iItem]->m_lStartOffset = STARTOFFSET_RESUME;
  CGUIMediaWindow::OnClick(iItem);
}

void CGUIWindowVideoBase::OnPopupMenu(int iItem, bool bContextDriven /* = true */)
{
  // empty list in files view?
  if (GetID() == WINDOW_VIDEO_FILES && m_vecItems.Size() == 0)
    bContextDriven = false;
  if (bContextDriven && (iItem < 0 || iItem >= m_vecItems.Size())) return;

  // calculate our position
  float posX = 200, posY = 100;
  const CGUIControl *pList = GetControl(CONTROL_LIST);
  if (pList)
  {
    posX = pList->GetXPosition() + pList->GetWidth() / 2;
    posY = pList->GetYPosition() + pList->GetHeight() / 2;
  }

  // popup the context menu
  CGUIDialogContextMenu *pMenu = (CGUIDialogContextMenu *)m_gWindowManager.GetWindow(WINDOW_DIALOG_CONTEXT_MENU);
  if (!pMenu) return ;

  // load our menu
  pMenu->Initialize();

  // contextual buttons
  int btn_PlayPart = 0;       // For stacks
  int btn_Queue = 0;					// Add to Playlist
  int btn_PlayWith = 0;				// Play
  int btn_Restart = 0;				// Restart Video from Beginning
  int btn_Resume = 0;					// Resume Video
  int btn_Show_Info = 0;			// Show Video Information
  int btn_AddToDatabase = 0;  // Manual add to Database
  int btn_Assign = 0;         // Assign content to directory
  int btn_Update = 0;         // Update content information
  int btn_Mark_UnWatched = 0;	// Clear Watched Status (DB)
  int btn_Mark_Watched = 0;		// Set Watched Status (DB)
  int btn_Update_Title = 0;		// Change Title (DB)
  int btn_SetThumb = 0;       // Set Season Thumb (DB)
  int btn_Delete = 0;					// Delete
  int btn_Rename = 0;					// Rename
  int btn_Default = 0;        // Set default
  int btn_ClearDefault=0;     // Clear default

  bool bSelected = false;
  VECPLAYERCORES vecCores;
  int iFound = 0;
  SScraperInfo info;

  // contextual items only appear when the list is not empty
  if (bContextDriven)
  {
    // mark the item
    bSelected = m_vecItems[iItem]->IsSelected(); // item may already be selected (playlistitem)
    m_vecItems[iItem]->Select(true);

    // get players
    CPlayerCoreFactory::GetPlayers(*m_vecItems[iItem], vecCores);

    if (m_vecItems[iItem]->HasVideoInfoTag())
    {
      m_database.GetScraperForPath(m_vecItems[iItem]->GetVideoInfoTag()->m_strPath,info.strPath,info.strContent,iFound);
    }
    else
      m_database.GetScraperForPath(m_vecItems[iItem]->m_strPath,info.strPath,info.strContent,iFound);
    
    int iString = 13346;
    if (info.strContent.Equals("tvshows"))
    {
      if (m_vecItems[iItem]->m_bIsFolder)
        iString = 20351;
      else 
        iString = 20352;
    }

    bool bIsGotoParent = m_vecItems[iItem]->IsParentFolder();
    if (!bIsGotoParent)
    {
      // don't show the add to playlist button in playlist window
      if (GetID() != WINDOW_VIDEO_PLAYLIST)
      {
        if (m_vecItems[iItem]->IsStack())
        {
          vector<long> times;
          if (m_database.GetStackTimes(m_vecItems[iItem]->m_strPath,times))
            btn_PlayPart = pMenu->AddButton(20324);
        }
        if (GetID() == WINDOW_VIDEO_NAV)
        {
          if (!m_vecItems.m_strPath.IsEmpty())
            btn_Queue = pMenu->AddButton(13347);      // Add to Playlist
        }
        else
          btn_Queue = pMenu->AddButton(13347);      // Add to Playlist

        if (vecCores.size() >= 1)
          btn_PlayWith = pMenu->AddButton(15213);
        // allow a folder to be ad-hoc queued and played by the default player
        else if (GetID() == WINDOW_VIDEO_FILES && (m_vecItems[iItem]->m_bIsFolder || m_vecItems[iItem]->IsPlayList()))
          btn_PlayWith = pMenu->AddButton(208);

        // if autoresume is enabled then add restart video button
        // check to see if the Resume Video button is applicable
        if (GetResumeItemOffset(m_vecItems[iItem]) > 0)
          if (g_guiSettings.GetBool("myvideos.autoresume"))
            btn_Restart = pMenu->AddButton(20132);    // Restart Video
          else
            btn_Resume = pMenu->AddButton(13381);     // Resume Video
      }

      if (GetID() == WINDOW_VIDEO_FILES && (g_settings.m_vecProfiles[g_settings.m_iLastLoadedProfileIndex].canWriteDatabases() || g_passwordManager.bMasterUser))
      {
        CScraperParser parser;
        if (parser.Load("q:\\system\\scrapers\\video\\"+info.strPath))
          info.strTitle = parser.GetName();

        if (m_vecItems[iItem]->m_bIsFolder)
        {
          if (iFound==0)
          {
            CStdString strPath(m_vecItems[iItem]->m_strPath);
            CUtil::AddSlashAtEnd(strPath);
            if ((info.strContent.Equals("movies") && m_database.HasMovieInfo(strPath)) || (info.strContent.Equals("tvshows") && m_database.HasTvShowInfo(strPath)))
              btn_Show_Info = pMenu->AddButton(iString);

            CGUIDialogVideoScan *pScanDlg = (CGUIDialogVideoScan *)m_gWindowManager.GetWindow(WINDOW_DIALOG_VIDEO_SCAN);
            if (!pScanDlg || (pScanDlg && !pScanDlg->IsScanning()))
              btn_Assign = pMenu->AddButton(20333);
          }
          else
          {
            btn_Show_Info = pMenu->AddButton(iString);
            CGUIDialogVideoScan *pScanDlg = (CGUIDialogVideoScan *)m_gWindowManager.GetWindow(WINDOW_DIALOG_VIDEO_SCAN);
            if (pScanDlg && pScanDlg->IsScanning())
            {
              btn_Update = pMenu->AddButton(13353);	// Stop Scanning
            }
            else
            {
              btn_Update = pMenu->AddButton(13349);
              btn_Assign = pMenu->AddButton(20333);
            }
          }
        }
        else
        {
          if ((info.strContent.Equals("movies") && (iFound > 0 || m_database.HasMovieInfo(m_vecItems[iItem]->m_strPath))) || m_database.HasEpisodeInfo(m_vecItems[iItem]->m_strPath))
            btn_Show_Info = pMenu->AddButton(iString);
          m_database.Open();
          if (!bIsGotoParent)
          {
            if (m_database.GetMovieInfo(m_vecItems[iItem]->m_strPath)<0 && m_database.GetEpisodeInfo(m_vecItems[iItem]->m_strPath)<0)
              btn_AddToDatabase = pMenu->AddButton(527); // Add to Database
          }
          m_database.Close();
        }
      }
    }

    if (GetID() == WINDOW_VIDEO_NAV)
    {
      if (!m_vecItems[iItem]->m_bIsFolder || info.strContent.Equals("tvshows"))
        btn_Show_Info = pMenu->AddButton(iString);
    }

    // is the item a database movie?
    if (!bIsGotoParent && GetID() == WINDOW_VIDEO_NAV && (g_settings.m_vecProfiles[g_settings.m_iLastLoadedProfileIndex].canWriteDatabases() || g_passwordManager.bMasterUser))
    {
      CVideoDatabaseDirectory dir;
      NODE_TYPE node = dir.GetDirectoryChildType(m_vecItems.m_strPath);

      if (node == NODE_TYPE_TITLE_TVSHOWS) // tvshow
      {
        CGUIDialogVideoScan *pScanDlg = (CGUIDialogVideoScan *)m_gWindowManager.GetWindow(WINDOW_DIALOG_VIDEO_SCAN);
        if (pScanDlg && pScanDlg->IsScanning())
        {
          btn_Update = pMenu->AddButton(13353);	// Stop Scanning
        }
        else
          btn_Update = pMenu->AddButton(13349);

        btn_Update_Title = pMenu->AddButton(16105); //Edit Title
      }

      if (node == NODE_TYPE_TITLE_MOVIES || node == NODE_TYPE_EPISODES)
      {
        if (m_vecItems[iItem]->GetVideoInfoTag()->m_bWatched)
          btn_Mark_UnWatched = pMenu->AddButton(16104); //Mark as UnWatched
        else
          btn_Mark_Watched = pMenu->AddButton(16103);   //Mark as Watched
        btn_Update_Title = pMenu->AddButton(16105); //Edit Title
      }
      if (dir.GetDirectoryChildType(m_vecItems.m_strPath) == NODE_TYPE_SEASONS && !dir.IsAllItem(m_vecItems[iItem]->m_strPath))
        btn_SetThumb = pMenu->AddButton(20371);
      
      if (node == NODE_TYPE_TITLE_MOVIES || node == NODE_TYPE_EPISODES || node == NODE_TYPE_TITLE_TVSHOWS)
        btn_Delete = pMenu->AddButton(646);
    }
    if (!bIsGotoParent)
    {
      // video playlists or file operations are allowed
      if ((m_vecItems.m_strPath.Equals("special://videoplaylists/")) || (GetID() == WINDOW_VIDEO_FILES && g_guiSettings.GetBool("filelists.allowfiledeletion")))
      {
        if (!m_vecItems[iItem]->IsReadOnly())
        { // enable only if writeable
          btn_Delete = pMenu->AddButton(117);
          btn_Rename = pMenu->AddButton(118);
        }
      }
      
      //Set default and/or clear default
      CVideoDatabaseDirectory dir;
      NODE_TYPE nodetype = dir.GetDirectoryType(m_vecItems[iItem]->m_strPath);
      if (
        GetID() == WINDOW_VIDEO_NAV &&
        !bIsGotoParent && 
        !m_vecItems.m_strPath.Equals("special://videoplaylists/") && 
        (nodetype == NODE_TYPE_ROOT || nodetype == NODE_TYPE_OVERVIEW || nodetype == NODE_TYPE_TVSHOWS_OVERVIEW || nodetype == NODE_TYPE_MOVIES_OVERVIEW) 
      )
      {
        if (!m_vecItems[iItem]->m_strPath.Equals(g_stSettings.m_szDefaultVideoLibView))
          btn_Default = pMenu->AddButton(13335); // set default
        if (strcmp(g_stSettings.m_szDefaultVideoLibView, ""))
          btn_ClearDefault = pMenu->AddButton(13403); // clear default
      }
    }
  } // if (bContextDriven)
  // non-contextual buttons
  int btn_Settings = pMenu->AddButton(5);			// Settings
  int btn_GoToRoot = 0;
  if (!m_vecItems.m_strPath.IsEmpty())
    btn_GoToRoot = pMenu->AddButton(20128);

  int btn_Switch = 0;													// Switch Media
  int btn_NowPlaying = 0;											// Now Playing

  // Switch Media is only visible in files window
  if (GetID() == WINDOW_VIDEO_FILES)
  {
    btn_Switch = pMenu->AddButton(523); // switch media
  }

  // Now Playing... at the very bottom of the list for easy access
  if (g_playlistPlayer.GetPlaylist(PLAYLIST_VIDEO).size() > 0)
      btn_NowPlaying = pMenu->AddButton(13350);

  // position it correctly
  pMenu->SetPosition(posX - pMenu->GetWidth() / 2, posY - pMenu->GetHeight() / 2);
  pMenu->DoModal();

  int btnid = pMenu->GetButton();
  if (btnid>0)
  {
    // play part
    if (btnid == btn_PlayPart)
    {
      CFileItemList items;
      CDirectory::GetDirectory(m_vecItems[iItem]->m_strPath,items);
      CGUIDialogFileStacking* dlg = (CGUIDialogFileStacking*)m_gWindowManager.GetWindow(WINDOW_DIALOG_FILESTACKING);
      if (!dlg)
        return;
      dlg->SetNumberOfFiles(items.Size());
      dlg->DoModal();
      int btn2 = dlg->GetSelectedFile();
      if (btn2 > 0)
      {
        if (btn2 > 1)
        {
          vector<long> times;
          if (!m_database.GetStackTimes(m_vecItems[iItem]->m_strPath, times)) // need to calculate them times
            return;

          m_vecItems[iItem]->m_lStartOffset = times[btn2-2]*75; // wtf?
        }
        else
          m_vecItems[iItem]->m_lStartOffset = 0;

        OnClick(iItem);
      }
    }
    // queue
    if (btnid == btn_Queue)
    {
      OnQueueItem(iItem);
    }
    // play
    else if (btnid == btn_PlayWith)
    {
      // if folder, play with default player
      if (m_vecItems[iItem]->m_bIsFolder)
      {
        PlayItem(iItem);
      }
      else
      {
        g_application.m_eForcedNextPlayer = CPlayerCoreFactory::SelectPlayerDialog(vecCores, posX, posY);
        if( g_application.m_eForcedNextPlayer != EPC_NONE )
          OnClick(iItem);
      }
    }
    // restart
    else if (btnid == btn_Restart)
    {
      OnRestartItem(iItem);
    }
    // resume
    else if (btnid == btn_Resume)
    {
      OnResumeItem(iItem);
    }
    else if (btnid == btn_Assign)
    {
      OnAssignContent(iItem,iFound,info);
    }
    else if (btnid  == btn_Update) // update content 
    {
      CGUIDialogVideoScan *pScanDlg = (CGUIDialogVideoScan *)m_gWindowManager.GetWindow(WINDOW_DIALOG_VIDEO_SCAN);
      if (pScanDlg && pScanDlg->IsScanning())
      {
        pScanDlg->StopScanning();
      }
      else
      {
        if (m_vecItems[iItem]->IsVideoDb())
          OnScan(m_vecItems[iItem]->GetVideoInfoTag()->m_strPath,info);
        else
          OnScan(m_vecItems[iItem]->m_strPath,info);
      }
    }
    // video info
    else if (btnid == btn_Show_Info)
    {
      OnInfo(iItem,info);
    }
    // unwatched
    else if (btnid == btn_Mark_UnWatched)
    {
      MarkUnWatched(iItem);
    }
    // watched
    else if (btnid == btn_Mark_Watched)
    {
      MarkWatched(iItem);
    }
    // update title
    else if (btnid == btn_Update_Title)
    {
      UpdateVideoTitle(iItem);
    }
    else if (btnid == btn_SetThumb)
    {
      CVideoInfoTag tag;
      CVideoDatabaseDirectory dir;
      CQueryParams params;
      dir.GetQueryParams(m_vecItems[iItem]->m_strPath,params);
      m_database.GetTvShowInfo("",tag,params.GetTvShowId());
      if (CGUIDialogFileBrowser::ShowAndGetImage(g_settings.m_vecMyVideoShares, g_localizeStrings.Get(20372), tag.m_strPath))
      {
        CStdString thumb(m_vecItems[iItem]->GetCachedSeasonThumb());
        CPicture picture;
        if (picture.DoCreateThumbnail(tag.m_strPath, thumb))
        {
          CUtil::DeleteVideoDatabaseDirectoryCache();
          Update(m_vecItems.m_strPath);
        }
        else
          CLog::Log(LOGERROR,"  Could not cache season thumb: %s",tag.m_strPath.c_str());
      }
    }
    // delete
    else if (btnid == btn_Delete)
    {
      OnDeleteItem(iItem);
    }
    // rename
    else if (btnid == btn_Rename)
    {
      OnRenameItem(iItem);
    }
    // settings
    else if (btnid == btn_Settings)
    {
      m_gWindowManager.ActivateWindow(WINDOW_SETTINGS_MYVIDEOS);
      return;
    }
    // go to root
    else if (btnid == btn_GoToRoot)
    {
      Update("");
      return;
    }
    // switch media
    else if (btnid == btn_Switch)
    {
      CGUIDialogContextMenu::SwitchMedia("video", m_vecItems.m_strPath, posX, posY);
      return;
    }
    // now playing
    else if (btnid ==  btn_NowPlaying)
    {
      m_gWindowManager.ActivateWindow(WINDOW_VIDEO_PLAYLIST);
      return;
    }
    else if (btnid == btn_AddToDatabase)
    {
      AddToDatabase(iItem);
    }
    else if (btnid == btn_Default) // Set default
    {
      strcpy(g_stSettings.m_szDefaultVideoLibView, GetQuickpathName(m_vecItems[iItem]->m_strPath).c_str());
      g_settings.Save();
    }
    else if (btnid == btn_ClearDefault) // Clear default
    {
      strcpy(g_stSettings.m_szDefaultVideoLibView, "");
      g_settings.Save();
    }
  }
  if (iItem < m_vecItems.Size() && iItem > -1)
    m_vecItems[iItem]->Select(bSelected);
}

void CGUIWindowVideoBase::GetStackedFiles(const CStdString &strFilePath1, vector<CStdString> &movies)
{
  CStdString strFilePath = strFilePath1;  // we're gonna be altering it

  movies.clear();

  CURL url(strFilePath);
  if (url.GetProtocol() == "stack")
  {
    CStackDirectory dir;
    CFileItemList items;
    dir.GetDirectory(strFilePath, items);
    for (int i = 0; i < items.Size(); ++i)
      movies.push_back(items[i]->m_strPath);
  }
  if (movies.empty())
    movies.push_back(strFilePath);
}

bool CGUIWindowVideoBase::OnPlayMedia(int iItem)
{
  // Reset Playlistplayer, playback started now does
  // not use the playlistplayer.
  g_playlistPlayer.Reset();
  g_playlistPlayer.SetCurrentPlaylist(PLAYLIST_NONE);

  if ( iItem < 0 || iItem >= (int)m_vecItems.Size() ) return false;
  CFileItem* pItem = m_vecItems[iItem];

  if (pItem->m_strPath == "add" && pItem->GetLabel() == g_localizeStrings.Get(1026)) // 'add source button' in empty root
  {
    if (CGUIDialogMediaSource::ShowAndAddMediaSource("video"))
    {
      Update("");
      return true;
    }
    return false;
  }

  CFileItem item(*pItem);
  if (pItem->IsVideoDb())
  {
    item = CFileItem(*pItem->GetVideoInfoTag());
    item.m_lStartOffset = pItem->m_lStartOffset;
  }

  PlayMovie(&item);

  return true;
}

void CGUIWindowVideoBase::PlayMovie(const CFileItem *item)
{
  CFileItemList movieList;
  int selectedFile = 1;
  long startoffset = item->m_lStartOffset;

  if (item->IsStack() && !g_guiSettings.GetBool("myvideos.treatstackasfile"))
  {
    CStdStringArray movies;
    GetStackedFiles(item->m_strPath, movies);

    if( item->m_lStartOffset == STARTOFFSET_RESUME )
    {
      startoffset = GetResumeItemOffset(item);

      if( startoffset & 0xF0000000 ) /* file is specified as a flag */
      {
        selectedFile = (startoffset>>28);
        startoffset = startoffset & ~0xF0000000;
      }
      else
      {
        /* attempt to start on a specific time in a stack */
        /* if we are lucky, we might have stored timings for */
        /* this stack at some point */

        m_database.Open();

        /* figure out what file this time offset is */
        vector<long> times;
        m_database.GetStackTimes(item->m_strPath, times);
        long totaltime = 0;
        for(unsigned i = 0; i < times.size(); i++)
        {
          totaltime += times[i]*75;
          if( startoffset < totaltime )
          {
            selectedFile = i+1;
            startoffset -= totaltime - times[i]*75; /* rebase agains selected file */
            break;
          }
        }
        m_database.Close();
      }
    }
    else
    { // show file stacking dialog
      CGUIDialogFileStacking* dlg = (CGUIDialogFileStacking*)m_gWindowManager.GetWindow(WINDOW_DIALOG_FILESTACKING);
      if (dlg)
      {
        dlg->SetNumberOfFiles(movies.size());
        dlg->DoModal();
        selectedFile = dlg->GetSelectedFile();
        if (selectedFile < 1) return ;
      }
    }
    // add to our movie list
    for (unsigned int i = 0; i < movies.size(); i++)
    {
      movieList.Add(new CFileItem(movies[i], false));
    }
  }
  else
  {
    movieList.Add(new CFileItem(*item));
  }

  g_playlistPlayer.Reset();
  g_playlistPlayer.SetCurrentPlaylist(PLAYLIST_VIDEO);
  CPlayList& playlist = g_playlistPlayer.GetPlaylist(PLAYLIST_VIDEO);
  playlist.Clear();
  for (int i = selectedFile - 1; i < (int)movieList.Size(); ++i)
  {
    CPlayList::CPlayListItem playlistItem;
    CUtil::ConvertFileItemToPlayListItem(movieList[i], playlistItem);
    if (i == selectedFile - 1)
      playlistItem.m_lStartOffset = startoffset;
    playlist.Add(playlistItem);
  }
  // play movie...
  g_playlistPlayer.Play(0);
}

void CGUIWindowVideoBase::OnDeleteItem(int iItem)
{
  if ( iItem < 0 || iItem >= m_vecItems.Size()) return;
  // HACK: stacked files need to be treated as folders in order to be deleted
  if (m_vecItems[iItem]->IsStack())
    m_vecItems[iItem]->m_bIsFolder = true;
  if (g_settings.m_vecProfiles[g_settings.m_iLastLoadedProfileIndex].getLockMode() != LOCK_MODE_EVERYONE && g_settings.m_vecProfiles[g_settings.m_iLastLoadedProfileIndex].filesLocked())
    if (!g_passwordManager.IsMasterLockUnlocked(true))
      return;
  if (!CGUIWindowFileManager::DeleteItem(m_vecItems[iItem]))
    return;
  Update(m_vecItems.m_strPath);
  m_viewControl.SetSelectedItem(iItem);
}

void CGUIWindowVideoBase::MarkUnWatched(int iItem)
{
  if ( iItem < 0 || iItem >= m_vecItems.Size() ) return ;
  CFileItem* pItem = m_vecItems[iItem];
  int iType=0;
  if (pItem->HasVideoInfoTag() && pItem->GetVideoInfoTag()->m_iSeason > 0) // episode
    iType = 1;
  m_database.MarkAsUnWatched(pItem->GetVideoInfoTag()->m_iDbId,iType>0);
  CUtil::DeleteVideoDatabaseDirectoryCache();
  m_viewControl.SetSelectedItem(iItem);
  Update(m_vecItems.m_strPath);
}

//Add Mark a Title as watched
void CGUIWindowVideoBase::MarkWatched(int iItem)
{
  if ( iItem < 0 || iItem >= m_vecItems.Size() ) return ;
  CFileItem* pItem = m_vecItems[iItem];
  int iType=0;
  if (pItem->HasVideoInfoTag() && pItem->GetVideoInfoTag()->m_iSeason > 0) // episode
    iType = 1;
  m_database.MarkAsWatched(pItem->GetVideoInfoTag()->m_iDbId,iType>0);
  CUtil::DeleteVideoDatabaseDirectoryCache();
  m_viewControl.SetSelectedItem(iItem);
  Update(m_vecItems.m_strPath);
}

//Add change a title's name
void CGUIWindowVideoBase::UpdateVideoTitle(int iItem)
{
  if ( iItem < 0 || iItem >= m_vecItems.Size() ) return ;
  CFileItem* pItem = m_vecItems[iItem];
  
  CVideoInfoTag detail;
  int iType=0;
  if (pItem->HasVideoInfoTag() && (!pItem->GetVideoInfoTag()->m_strShowTitle.IsEmpty() || pItem->GetVideoInfoTag()->m_iEpisode > 0)) // tvshow
    iType = 2;
  if (pItem->HasVideoInfoTag() && pItem->GetVideoInfoTag()->m_iSeason > 0) // episode
    iType = 1;

  if (iType == 0) // movies
    m_database.GetMovieInfo("", detail, pItem->GetVideoInfoTag()->m_iDbId);
  if (iType == 1) //  episodes
    m_database.GetEpisodeInfo(pItem->m_strPath,detail,pItem->GetVideoInfoTag()->m_iDbId);
  if (iType == 2) // tvshows
    m_database.GetTvShowInfo(pItem->GetVideoInfoTag()->m_strFileNameAndPath,detail,pItem->GetVideoInfoTag()->m_iDbId);

  CStdString strInput;
  strInput = detail.m_strTitle;

  //Get the new title
  if (!CGUIDialogKeyboard::ShowAndGetInput(strInput, g_localizeStrings.Get(16105), false)) return ;
  m_database.UpdateMovieTitle(pItem->GetVideoInfoTag()->m_iDbId, strInput, iType);
  CUtil::DeleteVideoDatabaseDirectoryCache();
  m_viewControl.SetSelectedItem(iItem);
  Update(m_vecItems.m_strPath);
}

void CGUIWindowVideoBase::LoadPlayList(const CStdString& strPlayList, int iPlayList /* = PLAYLIST_VIDEO */)
{
  // load a playlist like .m3u, .pls
  // first get correct factory to load playlist
  auto_ptr<CPlayList> pPlayList (CPlayListFactory::Create(strPlayList));
  if ( NULL != pPlayList.get())
  {
    // load it
    if (!pPlayList->Load(strPlayList))
    {
      CGUIDialogOK::ShowAndGetInput(6, 0, 477, 0);
      return ; //hmmm unable to load playlist?
    }
  }

  if (g_application.ProcessAndStartPlaylist(strPlayList, *pPlayList, iPlayList))
  {
    if (m_guiState.get())
      m_guiState->SetPlaylistDirectory("playlistvideo://");
  }
}

void CGUIWindowVideoBase::PlayItem(int iItem)
{
  // restrictions should be placed in the appropiate window code
  // only call the base code if the item passes since this clears
  // the currently playing temp playlist

  const CFileItem* pItem = m_vecItems[iItem];
  // if its a folder, build a temp playlist
  if (pItem->m_bIsFolder)
  {
    CFileItem item(*m_vecItems[iItem]);

    //  Allow queuing of unqueueable items
    //  when we try to queue them directly
    if (!item.CanQueue())
      item.SetCanQueue(true);

    // skip ".."
    if (item.IsParentFolder())
      return;

    // recursively add items to list
    CFileItemList queuedItems;
    AddItemToPlayList(&item, queuedItems);

    g_playlistPlayer.ClearPlaylist(PLAYLIST_VIDEO);
    g_playlistPlayer.Reset();
    g_playlistPlayer.Add(PLAYLIST_VIDEO, queuedItems);
    g_playlistPlayer.SetCurrentPlaylist(PLAYLIST_VIDEO);
    g_playlistPlayer.Play();
  }
  else if (pItem->IsPlayList())
  {
    // load the playlist the old way
    LoadPlayList(pItem->m_strPath, PLAYLIST_VIDEO);
  }
  else
  {
    // single item, play it
    OnClick(iItem);
  }
}

void CGUIWindowVideoBase::ApplyIMDBThumbToFolder(const CStdString &folder, const CStdString &imdbThumb)
{
  // copy icon to folder also;
  if (CFile::Exists(imdbThumb))
  {
    CFileItem folderItem(folder, true);
    CStdString strThumb(folderItem.GetCachedVideoThumb());
    CFile::Cache(imdbThumb.c_str(), strThumb.c_str(), NULL, NULL);
  }
}

bool CGUIWindowVideoBase::Update(const CStdString &strDirectory)
{
  if (m_thumbLoader.IsLoading())
    m_thumbLoader.StopThread();

  if (!CGUIMediaWindow::Update(strDirectory))
    return false;

  m_thumbLoader.Load(m_vecItems);

  return true;
}

void CGUIWindowVideoBase::OnPrepareFileItems(CFileItemList &items)
{
  items.SetCachedVideoThumbs();
}

void CGUIWindowVideoBase::AddToDatabase(int iItem)
{
  if (iItem < 0 || iItem >= m_vecItems.Size()) return;
  CFileItem* pItem;
  pItem = m_vecItems[iItem];
  if (pItem->IsParentFolder()) return;
  if (pItem->m_bIsFolder) return;

  bool bGotXml = false;
  CVideoInfoTag movie;
  movie.Reset();

  // look for matching xml file first
  CStdString strXml = pItem->m_strPath + ".xml";
  if (pItem->IsStack())
  {
    // for a stack, use the first file in the stack
    CStackDirectory stack;
    strXml = stack.GetFirstStackedFile(pItem->m_strPath) + ".xml";
  }
  CStdString strCache = "Z:\\" + CUtil::GetFileName(strXml);
  CUtil::GetFatXQualifiedPath(strCache);
  if (CFile::Exists(strXml))
  {
    bGotXml = true;
    CLog::Log(LOGDEBUG,__FUNCTION__": found matching xml file:[%s]", strXml.c_str());
    CFile::Cache(strXml, strCache);
    CIMDB imdb;
    if (!imdb.LoadXML(strCache, movie, false))
    {
      CLog::Log(LOGERROR,__FUNCTION__": Could not parse info in file:[%s]", strXml.c_str());
      bGotXml = false;
    }
  }

  // prompt for data
  if (!bGotXml)
  {
    // enter a new title
    CStdString strTitle = pItem->GetLabel();
    if (!CGUIDialogKeyboard::ShowAndGetInput(strTitle, g_localizeStrings.Get(528), false)) // Enter Title
      return;

    // pick genre
    CGUIDialogSelect* pSelect = (CGUIDialogSelect*)m_gWindowManager.GetWindow(WINDOW_DIALOG_SELECT);
    if (!pSelect)
      return;
    pSelect->SetHeading(530); // Select Genre
    pSelect->Reset();
    CFileItemList items;
    if (!CDirectory::GetDirectory("videodb://1/", items))
      return;
    for (int i = 0; i < items.Size(); ++i)
      pSelect->Add(items[i]->GetLabel());
    pSelect->EnableButton(true);
    pSelect->SetButtonLabel(531); // New Genre
    pSelect->DoModal();
    CStdString strGenre;
    int iSelected = pSelect->GetSelectedLabel();
    if (iSelected >= 0)
      strGenre = items[iSelected]->GetLabel();
    else if (!pSelect->IsButtonPressed())
      return;

    // enter new genre string
    if (strGenre.IsEmpty())
    {
      strGenre = g_localizeStrings.Get(532); // Manual Addition
      if (!CGUIDialogKeyboard::ShowAndGetInput(strGenre, g_localizeStrings.Get(533), false)) // Enter Genre
        return; // user backed out
      if (strGenre.IsEmpty())
        return; // no genre string
    }

    // set movie info
    movie.m_strTitle = strTitle;
    movie.m_strGenre = strGenre;
  }

  // Why should we double check title for uniqueness?  Who cares if 2 movies
  // have the same name in the db?

  /*
  // double check title for uniqueness
  items.ClearKeepPointer();
  if (!CDirectory::GetDirectory("videodb://2/", items))
    return;
  for (int i = 0; i < items.Size(); ++i)
  {
    if (items[i]->m_strTitle.Equals(movie.m_strTitle))
    {
      // uh oh, duplicate title
      CGUIDialogOK *pDialog = (CGUIDialogOK*)m_gWindowManager.GetWindow(WINDOW_DIALOG_OK);
      if (pDialog)
      {
        pDialog->SetHeading(529); // Duplicate Title
        pDialog->SetLine(0, movie.m_strTitle);
        pDialog->SetLine(1, "");
        pDialog->SetLine(2, "");
        pDialog->SetLine(3, "");
        pDialog->DoModal();
      }
      return;
    }
  }*/

  // everything is ok, so add to database
  m_database.Open();
  long lMovieId = m_database.AddMovie(pItem->m_strPath);
  movie.m_strIMDBNumber.Format("xx%08i", lMovieId);
  m_database.SetDetailsForMovie(pItem->m_strPath, movie);
  m_database.Close();

  // done...
  CGUIDialogOK *pDialog = (CGUIDialogOK*)m_gWindowManager.GetWindow(WINDOW_DIALOG_OK);
  if (pDialog)
  {
    pDialog->SetHeading(20177); // Done
    pDialog->SetLine(0, movie.m_strTitle);
    pDialog->SetLine(1, movie.m_strGenre);
    pDialog->SetLine(2, movie.m_strIMDBNumber);
    pDialog->SetLine(3, "");
    pDialog->DoModal();
  }

  // library view cache needs to be cleared
  CUtil::DeleteVideoDatabaseDirectoryCache();
}

/// \brief Search the current directory for a string got from the virtual keyboard
void CGUIWindowVideoBase::OnSearch()
{
  CStdString strSearch;
  if ( !CGUIDialogKeyboard::ShowAndGetInput(strSearch, g_localizeStrings.Get(16017), false) )
    return ;

  strSearch.ToLower();
  if (m_dlgProgress)
  {
    m_dlgProgress->SetHeading(194);
    m_dlgProgress->SetLine(0, strSearch);
    m_dlgProgress->SetLine(1, "");
    m_dlgProgress->SetLine(2, "");
    m_dlgProgress->StartModal();
    m_dlgProgress->Progress();
  }
  CFileItemList items;
  DoSearch(strSearch, items);

  if (items.Size())
  {
    CGUIDialogSelect* pDlgSelect = (CGUIDialogSelect*)m_gWindowManager.GetWindow(WINDOW_DIALOG_SELECT);
    pDlgSelect->Reset();
    pDlgSelect->SetHeading(283);
    items.Sort(SORT_METHOD_LABEL, SORT_ORDER_ASC);

    for (int i = 0; i < (int)items.Size(); i++)
    {
      CFileItem* pItem = items[i];
      pDlgSelect->Add(pItem->GetLabel());
    }

    pDlgSelect->DoModal();

    int iItem = pDlgSelect->GetSelectedLabel();
    if (iItem < 0)
    {
      if (m_dlgProgress) m_dlgProgress->Close();
      return ;
    }

    CFileItem* pSelItem = items[iItem];

    OnSearchItemFound(pSelItem);

    if (m_dlgProgress) m_dlgProgress->Close();
  }
  else
  {
    if (m_dlgProgress) m_dlgProgress->Close();
    CGUIDialogOK::ShowAndGetInput(194, 284, 0, 0);
  }
}

/// \brief React on the selected search item
/// \param pItem Search result item
void CGUIWindowVideoBase::OnSearchItemFound(const CFileItem* pSelItem)
{
  if (pSelItem->m_bIsFolder)
  {
    CStdString strPath = pSelItem->m_strPath;
    CStdString strParentPath;
    CUtil::GetParentPath(strPath, strParentPath);

    Update(strParentPath);

    SetHistoryForPath(strParentPath);

    strPath = pSelItem->m_strPath;
    CURL url(strPath);
    if (pSelItem->IsSmb() && !CUtil::HasSlashAtEnd(strPath))
      strPath += "/";

    for (int i = 0; i < m_vecItems.Size(); i++)
    {
      CFileItem* pItem = m_vecItems[i];
      if (pItem->m_strPath == strPath)
      {
        m_viewControl.SetSelectedItem(i);
        break;
      }
    }
  }
  else
  {
    CStdString strPath;
    CUtil::GetDirectory(pSelItem->m_strPath, strPath);

    Update(strPath);

    SetHistoryForPath(strPath);

    for (int i = 0; i < (int)m_vecItems.Size(); i++)
    {
      CFileItem* pItem = m_vecItems[i];
      if (pItem->m_strPath == pSelItem->m_strPath)
      {
        m_viewControl.SetSelectedItem(i);
        break;
      }
    }
  }
  m_viewControl.SetFocused();
}
