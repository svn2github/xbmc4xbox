//

#include "stdafx.h"
#include "guiwindowVideoBase.h"
#include "settings.h"
#include "guiWindowManager.h"
#include "localizestrings.h"
#include "texturemanager.h"
#include "util.h"
#include "picture.h"
#include "url.h"
#include "utils/imdb.h"
#include "utils/http.h"
#include "GUIDialogOK.h"
#include "GUIDialogprogress.h"
#include "GUIDialogSelect.h" 
#include "GUIWindowVideoInfo.h" 
#include "PlayListFactory.h"
#include "application.h" 
#include <algorithm>
#include "DetectDVDType.h"
#include "nfofile.h"
#include "utils/fstrcmp.h"
#include "utils/log.h"
#include "filesystem/file.h"
#include "playlistplayer.h"
#include "xbox/iosupport.h"
#include "GUIThumbnailPanel.h"
#include "GUIListControl.h"
#include "filesystem/directorycache.h"
#include "GUIDialogOK.h"

#define VIEW_AS_LIST           0
#define VIEW_AS_ICONS          1
#define VIEW_AS_LARGEICONS     2

#define CONTROL_BTNVIEWASICONS		2
#define CONTROL_BTNSORTBY					3
#define CONTROL_BTNSORTASC				4
#define CONTROL_BTNTYPE           5
#define CONTROL_PLAY_DVD          6
#define CONTROL_STACK             7
#define CONTROL_BTNSCAN           8
#define CONTROL_IMDB              9
#define CONTROL_LIST							10
#define CONTROL_THUMBS						11
#define CONTROL_LABELFILES         12

struct SSortVideoListByName
{
	bool operator()(CStdString& strItem1, CStdString& strItem2)
	{
    return strcmp(strItem1.c_str(),strItem2.c_str())<0;
  }
};

struct SSortVideoByName
{
	bool operator()(CFileItem* pStart, CFileItem* pEnd)
	{
    CFileItem& rpStart=*pStart;
    CFileItem& rpEnd=*pEnd;
		if (rpStart.GetLabel()=="..") return true;
		if (rpEnd.GetLabel()=="..") return false;
		bool bGreater=true;
		if (m_bSortAscending) bGreater=false;
    if ( rpStart.m_bIsFolder   == rpEnd.m_bIsFolder)
		{
			char szfilename1[1024];
			char szfilename2[1024];

			switch ( m_iSortMethod ) 
			{
				case 0:	//	Sort by Filename
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

        case 2:
          if ( rpStart.m_dwSize > rpEnd.m_dwSize) return bGreater;
					if ( rpStart.m_dwSize < rpEnd.m_dwSize) return !bGreater;
					return true;
        break;

        case 3:	//	Sort by share type
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
			
			for (i=0; i < (int)strlen(szfilename2); i++)
				szfilename2[i]=tolower((unsigned char)szfilename2[i]);
			//return (rpStart.strPath.compare( rpEnd.strPath )<0);

			if (m_bSortAscending)
				return (strcmp(szfilename1,szfilename2)<0);
			else
				return (strcmp(szfilename1,szfilename2)>=0);
		}
    if (!rpStart.m_bIsFolder) return false;
		return true;
	}
	bool m_bSortAscending;
	int m_iSortMethod;
};


CGUIWindowVideoBase::CGUIWindowVideoBase()
:CGUIWindow(0)
{
	m_strDirectory="?";
  m_iItemSelected=-1;
	m_iLastControl=-1;
	m_bDisplayEmptyDatabaseMessage = false;
}

CGUIWindowVideoBase::~CGUIWindowVideoBase()
{
}


void CGUIWindowVideoBase::OnAction(const CAction &action)
{
  if (action.wID == ACTION_PARENT_DIR)
	{
		GoParentFolder();
		return;
	}

 	if (action.wID == ACTION_PREVIOUS_MENU)
	{
    m_gWindowManager.ActivateWindow(WINDOW_HOME);
		return;
	}
	CGUIWindow::OnAction(action);
}

bool CGUIWindowVideoBase::OnMessage(CGUIMessage& message)
{
  switch ( message.GetMessage() )
  {
		case GUI_MSG_DVDDRIVE_EJECTED_CD:
		{
			if ( !m_strDirectory.IsEmpty() ) {
				if ( CUtil::IsCDDA( m_strDirectory ) || CUtil::IsDVD( m_strDirectory ) || CUtil::IsISO9660( m_strDirectory ) ) {
					//	Disc has changed and we are inside a DVD Drive share, get out of here :)
					m_strDirectory = "";
					Update( m_strDirectory );
				}
			}
			else {
				int iItem = GetSelectedItem();
				Update( m_strDirectory );
				CONTROL_SELECT_ITEM(CONTROL_LIST,iItem)
				CONTROL_SELECT_ITEM(CONTROL_THUMBS,iItem)
			}
		}
		break;

		case GUI_MSG_DVDDRIVE_CHANGED_CD:
		{
			if ( m_strDirectory.IsEmpty() ) {
				int iItem = GetSelectedItem();
				Update( m_strDirectory );
				CONTROL_SELECT_ITEM(CONTROL_LIST,iItem)
				CONTROL_SELECT_ITEM(CONTROL_THUMBS,iItem)
			}
		}
		break;
		case GUI_MSG_WINDOW_DEINIT:
			m_iLastControl=GetFocusedControl();
			m_iItemSelected=GetSelectedItem();
			Clear();
      m_database.Close();
		break;

    case GUI_MSG_WINDOW_INIT:
		{
			CGUIWindow::OnMessage(message);

      m_database.Open();
      m_dlgProgress = (CGUIDialogProgress*)m_gWindowManager.GetWindow(WINDOW_DIALOG_PROGRESS);
			m_rootDir.SetMask(g_stSettings.m_szMyVideoExtensions);
			m_rootDir.SetShares(g_settings.m_vecMyVideoShares);
			
			if (m_iLastControl>-1)
				SET_CONTROL_FOCUS(m_iLastControl, 0);

			Update(m_strDirectory);

      UpdateThumbPanel();
      if (m_iItemSelected >=0)
      {
			  CONTROL_SELECT_ITEM(CONTROL_LIST,m_iItemSelected)
			  CONTROL_SELECT_ITEM(CONTROL_THUMBS,m_iItemSelected)
      }
			return true;
		}
		break;

    case GUI_MSG_CLICKED:
    {
      int iControl=message.GetSenderId();
      if (iControl==CONTROL_BTNVIEWASICONS)
      {
				// cycle LIST->ICONS->LARGEICONS
				int iViewMode = VIEW_AS_ICONS;
				if (ViewByIcon()) iViewMode = VIEW_AS_LARGEICONS;
				if (ViewByLargeIcon()) iViewMode = VIEW_AS_LIST;
				SetViewMode(iViewMode);
				g_settings.Save();
				UpdateThumbPanel();
        UpdateButtons();
      }
      else if (iControl==CONTROL_PLAY_DVD)
      {
          // play movie...
        CUtil::PlayDVD();
      }
      else if (iControl==CONTROL_BTNTYPE)
      {
				CGUIMessage msg(GUI_MSG_ITEM_SELECTED, GetID(),CONTROL_BTNTYPE);
				m_gWindowManager.SendMessage(msg);

				int nSelected=msg.GetParam1();
				int nNewWindow=WINDOW_VIDEOS;
				switch (nSelected)
				{
				case 0:	//	Movies
					nNewWindow=WINDOW_VIDEOS;
					break;
				case 1:	//	Genre
					nNewWindow=WINDOW_VIDEO_GENRE;
					break;
				case 2:	//	Actors
					nNewWindow=WINDOW_VIDEO_ACTOR;
					break;
				case 3:	//	Year
					nNewWindow=WINDOW_VIDEO_YEAR;
					break;
				case 4:	//	Titel
					nNewWindow=WINDOW_VIDEO_TITLE;
					break;
				}

				if (nNewWindow!=GetID())
				{
					g_stSettings.m_iVideoStartWindow=nNewWindow;
					g_settings.Save();
					m_gWindowManager.ActivateWindow(nNewWindow);
					CGUIMessage msg2(GUI_MSG_SETFOCUS, nNewWindow, CONTROL_BTNTYPE);
					g_graphicsContext.SendMessage(msg2);
				}

        return true;
      }
      else if (iControl==CONTROL_LIST||iControl==CONTROL_THUMBS)  // list/thumb control
      {
         // get selected item
        CGUIMessage msg(GUI_MSG_ITEM_SELECTED,GetID(),iControl,0,0,NULL);
        g_graphicsContext.SendMessage(msg);         
        int iItem=msg.GetParam1();
        int iAction=message.GetParam1();
        if (iAction == ACTION_CONTEXT_MENU || iAction == ACTION_MOUSE_RIGHT_CLICK) 
        {
					OnPopupMenu(iItem);
        }
        else if (iAction == ACTION_SELECT_ITEM || iAction == ACTION_MOUSE_LEFT_CLICK)
				{
					OnClick(iItem);
				}
        else if (iAction == ACTION_QUEUE_ITEM || iAction == ACTION_MOUSE_MIDDLE_CLICK)
        {
					OnQueueItem(iItem);
        }
 				else if (iAction==ACTION_SHOW_INFO)
				{
					OnInfo(iItem);
				}
			}
 			else if (iControl==CONTROL_IMDB)
			{
				OnManualIMDB();
			}
    }
	}
  return CGUIWindow::OnMessage(message);
}

void CGUIWindowVideoBase::UpdateButtons()
{
	//	Remove labels from the window selection
	CGUIMessage msg(GUI_MSG_LABEL_RESET,GetID(),CONTROL_BTNTYPE);
	g_graphicsContext.SendMessage(msg);

	//	Add labels to the window selection
	CStdString strItem=g_localizeStrings.Get(744);	//	Files
	CGUIMessage msg2(GUI_MSG_LABEL_ADD,GetID(),CONTROL_BTNTYPE);
	msg2.SetLabel(strItem);
	g_graphicsContext.SendMessage(msg2);

	strItem=g_localizeStrings.Get(135);	//	Genre
	msg2.SetLabel(strItem);
	g_graphicsContext.SendMessage(msg2);

	strItem=g_localizeStrings.Get(344);	//	Actors
	msg2.SetLabel(strItem);
	g_graphicsContext.SendMessage(msg2);

	strItem=g_localizeStrings.Get(345);	//	Year
	msg2.SetLabel(strItem);
	g_graphicsContext.SendMessage(msg2);

	strItem=g_localizeStrings.Get(369);	//	Titel
	msg2.SetLabel(strItem);
	g_graphicsContext.SendMessage(msg2);

	//	Select the current window as default item 
	int nWindow = 0;
	if (g_stSettings.m_iVideoStartWindow == WINDOW_VIDEO_GENRE)	nWindow = 1;
	if (g_stSettings.m_iVideoStartWindow == WINDOW_VIDEO_ACTOR) nWindow = 2;
	if (g_stSettings.m_iVideoStartWindow == WINDOW_VIDEO_YEAR) nWindow = 3;
	if (g_stSettings.m_iVideoStartWindow == WINDOW_VIDEO_TITLE) nWindow = 4;
	CONTROL_SELECT_ITEM(CONTROL_BTNTYPE, nWindow);

	int iString = 101;	// view as list
	if (ViewByIcon())
		iString = 100;		// view as icon
	if (ViewByLargeIcon())
		iString = 417;		// view as large icon

	SET_CONTROL_LABEL(CONTROL_BTNVIEWASICONS,iString);
	UpdateThumbPanel();

	// disable scan and manual imdb controls if internet lookups are disabled
	if (g_guiSettings.GetBool("Network.EnableInternet"))
	{
		CONTROL_ENABLE(CONTROL_BTNSCAN);
		CONTROL_ENABLE(CONTROL_IMDB);
	}
	else
	{
		CONTROL_DISABLE(CONTROL_BTNSCAN);
		CONTROL_DISABLE(CONTROL_IMDB);
	}

	if (ViewByIcon()) 
	{
		SET_CONTROL_VISIBLE(CONTROL_THUMBS);
		SET_CONTROL_HIDDEN(CONTROL_LIST);
	}
	else
	{
		SET_CONTROL_VISIBLE(CONTROL_LIST);
		SET_CONTROL_HIDDEN(CONTROL_THUMBS);
	}

	SET_CONTROL_LABEL(CONTROL_BTNSORTBY, SortMethod());

	if (SortAscending())
	{
		CGUIMessage msg(GUI_MSG_DESELECTED,GetID(), CONTROL_BTNSORTASC);
		g_graphicsContext.SendMessage(msg);
	}
	else
	{
		CGUIMessage msg(GUI_MSG_SELECTED,GetID(), CONTROL_BTNSORTASC);
		g_graphicsContext.SendMessage(msg);
	}

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
}

void CGUIWindowVideoBase::OnSort()
{
	CGUIMessage msg(GUI_MSG_LABEL_RESET,GetID(),CONTROL_LIST,0,0,NULL);
	g_graphicsContext.SendMessage(msg);         
  
	CGUIMessage msg2(GUI_MSG_LABEL_RESET,GetID(),CONTROL_THUMBS,0,0,NULL);
	g_graphicsContext.SendMessage(msg2);         

	FormatItemLabels();
	SortItems(m_vecItems);

	for (int i=0; i < (int)m_vecItems.size(); i++)
	{
		CFileItem* pItem=m_vecItems[i];
		CGUIMessage msg(GUI_MSG_LABEL_ADD,GetID(),CONTROL_LIST,0,0,(void*)pItem);
		g_graphicsContext.SendMessage(msg);    
		CGUIMessage msg2(GUI_MSG_LABEL_ADD,GetID(),CONTROL_THUMBS,0,0,(void*)pItem);
		g_graphicsContext.SendMessage(msg2);         
	}
}

void CGUIWindowVideoBase::Clear()
{
	CFileItemList itemlist(m_vecItems); // will clean up everything
}

int CGUIWindowVideoBase::GetSelectedItem()
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

bool CGUIWindowVideoBase::HaveDiscOrConnection( CStdString& strPath, int iDriveType )
{
	if ( iDriveType == SHARE_TYPE_DVD )
	{
		CDetectDVDMedia::WaitMediaReady();
		if ( !CDetectDVDMedia::IsDiscInDrive() )
		{
			CGUIDialogOK* dlg = (CGUIDialogOK*)m_gWindowManager.GetWindow(WINDOW_DIALOG_OK);
      if (dlg)
      {
			  dlg->SetHeading( 218 );
			  dlg->SetLine( 0, 219 );
			  dlg->SetLine( 1, L"" );
			  dlg->SetLine( 2, L"" );
			  dlg->DoModal( GetID() );
      }
			int iItem = GetSelectedItem();
			Update( m_strDirectory );
			CONTROL_SELECT_ITEM(CONTROL_LIST,iItem)
			CONTROL_SELECT_ITEM(CONTROL_THUMBS,iItem)
			return false;
		}
	}
	else if ( iDriveType == SHARE_TYPE_REMOTE )
	{
			// TODO: Handle not connected to a remote share
		if ( !CUtil::IsEthernetConnected() )
		{
			CGUIDialogOK* dlg = (CGUIDialogOK*)m_gWindowManager.GetWindow(WINDOW_DIALOG_OK);
      if (dlg)
      {
			  dlg->SetHeading( 220 );
			  dlg->SetLine( 0, 221 );
			  dlg->SetLine( 1, L"" );
			  dlg->SetLine( 2, L"" );
			  dlg->DoModal( GetID() );
      }
			return false;
		}
	}
	else
		return true;
  return true;
}

/// \brief Tests if the user is allowed to access the share folder
/// \param pItem The share folder item to access
/// \param strType The type of share being accessed, e.g. "music", "videos", etc. See CSettings::UpdateBookmark
/// \return If access is granted, returns \e true
bool CGUIWindowVideoBase::HaveBookmarkPermissions(CFileItem* pItem, const CStdString &strType)
{
  while (0 < pItem->m_iLockMode)
  {
    CStdString strLabel = pItem->GetLabel();
    bool bConfirmed = false;
    bool bCanceled = false;
    char buffer[33]; // holds 32 places plus sign character

    if (0 != g_stSettings.m_iMasterLockMaxRetry && pItem->m_iBadPwdCount >= g_stSettings.m_iMasterLockMaxRetry)
    {
      // user previously exhausted all retries, show access denied error
      CGUIDialogOK* pDialogOK = (CGUIDialogOK*)m_gWindowManager.GetWindow(WINDOW_DIALOG_OK);
      pDialogOK->SetHeading( 12345 );
      pDialogOK->SetLine( 0, 12346 );
      pDialogOK->SetLine( 1, L"" );
      pDialogOK->SetLine( 2, L"" );
      pDialogOK->DoModal( GetID() );
      //XKUtils::XBOXPowerOff();
      return false;
    }

    // show the appropriate lock dialog, populate bConfirmed and bCanceled
    if (LOCK_MODE_NUMERIC == pItem->m_iLockMode)
    {
      CGUIDialogPasswordNumeric *pDialog = &(g_application.m_guiDialogPasswordNumeric);
      pDialog->m_strPassword = pItem->m_strLockCode;

      if (1 > pItem->m_iBadPwdCount)
      {
        pDialog->m_iRetries = 0;
      }
      else
      {
        pDialog->m_iRetries = g_stSettings.m_iMasterLockMaxRetry - pItem->m_iBadPwdCount;
      }
      pDialog->SetHeading( 12325 );
      pDialog->SetLine( 0, 12328 );
      pDialog->SetLine( 1, 12329 );
      pDialog->SetLine( 2, L"" );
      pDialog->DoModal(GetID());
      bConfirmed = pDialog->IsConfirmed();
      bCanceled = pDialog->IsCanceled();
    }
    else if (LOCK_MODE_GAMEPAD == pItem->m_iLockMode)
    {
      CGUIDialogPasswordGamepad *pDialog = &(g_application.m_guiDialogPasswordGamepad);
      pDialog->m_strPassword = pItem->m_strLockCode;

      if (1 > pItem->m_iBadPwdCount)
      {
        pDialog->m_iRetries = 0;
      }
      else
      {
        pDialog->m_iRetries = g_stSettings.m_iMasterLockMaxRetry - pItem->m_iBadPwdCount;
      }
      pDialog->SetHeading( 12325 );
      pDialog->SetLine( 0, 12330 );
      pDialog->SetLine( 1, 12331 );
      pDialog->DoModal(GetID());
      bConfirmed = pDialog->IsConfirmed();
      bCanceled = pDialog->IsCanceled();
    }
    else if (LOCK_MODE_QWERTY == pItem->m_iLockMode)
    {   
      CStdString strTextInput = "";
      CStdStringW strHeading = g_localizeStrings.Get(12326);
      if (!(1 > pItem->m_iBadPwdCount))
      {
				strHeading.Format(L"%s %i %s", g_localizeStrings.Get(12342).c_str(), g_stSettings.m_iMasterLockMaxRetry - pItem->m_iBadPwdCount, g_localizeStrings.Get(12343).c_str()); 
      }
      bCanceled = !CGUIDialogKeyboard::ShowAndGetInput(strTextInput, strHeading, false);
      bConfirmed = (!bCanceled && (strTextInput == pItem->m_strLockCode));
    }
    else
    {
      // pItem->m_iLockMode isn't set to an implemented lock mode, so treat as unlocked
      return true;
    }

    if (bConfirmed && !bCanceled)
    {
      // password entry succeeded
      pItem->m_iLockMode = pItem->m_iLockMode * -1;
      itoa(pItem->m_iLockMode, buffer, 10);
      g_settings.UpdateBookmark(strType, strLabel, "lockmode", itoa(pItem->m_iLockMode, buffer, 10));
      pItem->m_iBadPwdCount = 0;
      g_settings.UpdateBookmark(strType, strLabel, "badpwdcount", itoa(pItem->m_iBadPwdCount, buffer, 10));
    }
    else if (!bConfirmed && bCanceled)
    {
      // user canceled out
      return false;
    }
    else
    {
      // password entry failed
      if (0 != g_stSettings.m_iMasterLockMaxRetry)
        pItem->m_iBadPwdCount++;
      g_settings.UpdateBookmark(strType, strLabel, "badpwdcount", itoa(pItem->m_iBadPwdCount, buffer, 10));
    }
  }
  return true;
}

void CGUIWindowVideoBase::OnInfo(int iItem)
{
	CFileItem* pItem=m_vecItems[iItem];
  VECMOVIESFILES movies;
  m_database.GetFiles(atol(pItem->m_strPath),movies);
	if (movies.size() <=0) return;
  CStdString strFilePath=movies[0];
  CStdString strFile=CUtil::GetFileName(strFilePath);
  ShowIMDB(strFile,strFilePath, "" ,false);
}

void CGUIWindowVideoBase::ShowIMDB(const CStdString& strMovie, const CStdString& strFile, const CStdString& strFolder, bool bFolder)
{
 CGUIDialogOK* 		 		pDlgOK			 = (CGUIDialogOK*)m_gWindowManager.GetWindow(WINDOW_DIALOG_OK);
 CGUIDialogProgress* 	pDlgProgress= (CGUIDialogProgress*)m_gWindowManager.GetWindow(WINDOW_DIALOG_PROGRESS);
 CGUIDialogSelect* 		pDlgSelect= (CGUIDialogSelect*)m_gWindowManager.GetWindow(WINDOW_DIALOG_SELECT);
 CGUIWindowVideoInfo* pDlgInfo= (CGUIWindowVideoInfo*)m_gWindowManager.GetWindow(WINDOW_VIDEO_INFO);
 
 CIMDB								IMDB;
 bool									bUpdate(false); 
 bool									bFound=false;
 
 if (!pDlgOK) return;
 if (!pDlgProgress) return;
 if (!pDlgSelect) return;
 if (!pDlgInfo) return;
 CUtil::ClearCache();
 CStdString strMovieName=strMovie;

 if (m_database.HasMovieInfo(strFile) )
 {
    CIMDBMovie movieDetails;
    m_database.GetMovieInfo(strFile,movieDetails);
	  pDlgInfo->SetMovie(movieDetails);
	  pDlgInfo->DoModal(GetID());
    if ( !pDlgInfo->NeedRefresh() ) return;

		// quietly return if Internet lookups are disabled
		if (!g_guiSettings.GetBool("Network.EnableInternet")) return;

    m_database.DeleteMovieInfo(strFile);
 }

	// quietly return if Internet lookups are disabled
	if (!g_guiSettings.GetBool("Network.EnableInternet")) return;

  // handle .nfo files
	CStdString strExtension;
	CUtil::GetExtension(strFile,strExtension);
	if ( CUtil::cmpnocase(strExtension.c_str(),".nfo") ==0)
	{
		CFile file;
		if ( file.Cache(strFile,"Z:\\movie.nfo",NULL,NULL))
		{
			CNfoFile nfoReader;
			if ( nfoReader.Create("Z:\\movie.nfo") == S_OK)
			{
				CIMDBUrl url;
				CIMDBMovie movieDetails;
				url.m_strURL=nfoReader.m_strImDbUrl;
				if ( IMDB.GetDetails(url, movieDetails) )
				{
					// now show the imdb info
					pDlgInfo->SetMovie(movieDetails);
					pDlgInfo->DoModal(GetID());
					bFound=true;
					bUpdate=true;
          if ( !pDlgInfo->NeedRefresh() ) return;
          m_database.DeleteMovieInfo(strFile);
				}
			}
		}
	}

	if (!g_guiSettings.GetBool("FileLists.HideExtensions") && !bFolder)
		CUtil::RemoveExtension(strMovieName);

  bool bContinue;
  do
  {
    bContinue=false;
	  if (!bFound)
	  {
		  // show dialog that we're busy querying www.imdb.com
		  pDlgProgress->SetHeading(197);
		  pDlgProgress->SetLine(0,strMovieName);
		  pDlgProgress->SetLine(1,"");
		  pDlgProgress->SetLine(2,"");
		  pDlgProgress->StartModal(GetID());
		  pDlgProgress->Progress();

		  bool						bError=true;
  		
  		
      OutputDebugString("query imdb\n");
      IMDB_MOVIELIST	movielist;
		  if (IMDB.FindMovie(strMovieName,	movielist) ) 
		  {
			  pDlgProgress->Close();

			  int iMoviesFound=movielist.size();
			  if (iMoviesFound > 0)
			  {
          OutputDebugString("found 1 or more movies\n");
				  int iSelectedMovie=0;
				  if ( iMoviesFound > 1)
				  {
					  // more then 1 movie found
					  // ask user to select 1
            OutputDebugString("found more then 1 movie\n");
					  pDlgSelect->SetHeading(196);
					  pDlgSelect->Reset();
					  for (int i=0; i < iMoviesFound; ++i)
					  {
						  CIMDBUrl url = movielist[i];
						  pDlgSelect->Add(url.m_strTitle);
					  }
            pDlgSelect->EnableButton(true);
            pDlgSelect->SetButtonLabel(413); // manual

					  pDlgSelect->DoModal(GetID());

					  // and wait till user selects one
					  iSelectedMovie= pDlgSelect->GetSelectedLabel();
            if (iSelectedMovie< 0)
            {
              if (!pDlgSelect->IsButtonPressed()) return;
							if (!CGUIDialogKeyboard::ShowAndGetInput(strMovieName, false)) return;
              bContinue=true;
              bError=false;
            }
				  }
          
          if (iSelectedMovie>=0)
          {
            OutputDebugString("get details\n");
				    CIMDBMovie movieDetails;
            movieDetails.m_strSearchString=strFile;
				    CIMDBUrl& url=movielist[iSelectedMovie];
    				
				    // show dialog that we're downloading the movie info
				    pDlgProgress->SetHeading(198);
				    pDlgProgress->SetLine(0,strMovieName);
				    pDlgProgress->SetLine(1,url.m_strTitle);
				    pDlgProgress->SetLine(2,"");
				    pDlgProgress->StartModal(GetID());
				    pDlgProgress->Progress();
				    if ( IMDB.GetDetails(url, movieDetails) )
				    {
					    // got all movie details :-)
              OutputDebugString("got details\n");
					    pDlgProgress->Close();
					    bError=false;

					    // now show the imdb info
              OutputDebugString("show info\n");
              m_database.SetMovieInfo(strFile,movieDetails);
					    pDlgInfo->SetMovie(movieDetails);
					    pDlgInfo->DoModal(GetID());
              if (!pDlgInfo->NeedRefresh())
              { 
					      bUpdate=true;
                if (bFolder)
                {
                  // copy icon to folder also;
                  CStdString strThumbOrg;
                  CUtil::GetVideoThumbnail(movieDetails.m_strIMDBNumber,strThumbOrg);
                  if (CUtil::FileExists(strThumbOrg))
                  {
                    CStdString strFolderImage;
                    CUtil::AddFileToFolder(strFolder, "folder.jpg", strFolderImage);                  
                    if (CUtil::IsRemote(strFolder) || CUtil::IsDVD(strFolder) || CUtil::IsISO9660(strFolder) )
                    {
                      CStdString strThumb;
                      CUtil::GetThumbnail( strFolderImage,strThumb);
                      CFile file;
                      file.Cache(strThumbOrg.c_str(), strThumb.c_str(),NULL,NULL);
                    }
                    else 
                    {
                      CFile file;
                      file.Cache(strThumbOrg.c_str(), strFolderImage.c_str(),NULL,NULL);
                    }
                  }
                }
              }
              else
              {
                bContinue=true;
                strMovieName=strMovie;
              }
				    }
				    else
				    {
              OutputDebugString("failed to get details\n");
					    pDlgProgress->Close();
				    }
          }
			  }
        else
        {
          pDlgProgress->Close();
					if (!CGUIDialogKeyboard::ShowAndGetInput(strMovieName, false)) return;
          bContinue=true;
          bError=false;
        }
		  }
		  else
		  {
			  pDlgProgress->Close();
				if (!CGUIDialogKeyboard::ShowAndGetInput(strMovieName, false)) return;
        bContinue=true;
        bError=false;
		  }

		  if (bError)
		  {
			  // show dialog...
			  pDlgOK->SetHeading(195);
			  pDlgOK->SetLine(0,strMovieName);
			  pDlgOK->SetLine(2,L"");
			  pDlgOK->DoModal(GetID());
		  }
	  }
  } while (bContinue);

	if (bUpdate)
	{
		Update(m_strDirectory);
	}
}

void CGUIWindowVideoBase::Render()
{
	CGUIWindow::Render();
	if (m_bDisplayEmptyDatabaseMessage)
	{
		CGUIListControl *pControl = (CGUIListControl *)GetControl(CONTROL_LIST);
		int iX = pControl->GetXPosition()+pControl->GetWidth()/2;
		int iY = pControl->GetYPosition()+pControl->GetHeight()/2;
		CGUIFont *pFont = g_fontManager.GetFont(pControl->GetFontName());
		if (pFont)
		{
			float fWidth, fHeight;
			CStdStringW wszText = g_localizeStrings.Get(745);	// "No scanned information for this view"
			CStdStringW wszText2 = g_localizeStrings.Get(746); // "Switch back to Files view"
			pFont->GetTextExtent(wszText, &fWidth, &fHeight);
			pFont->DrawText((float)iX, (float)iY-fHeight, 0xffffffff, wszText.c_str(), XBFONT_CENTER_X|XBFONT_CENTER_Y);
			pFont->DrawText((float)iX, (float)iY+fHeight, 0xffffffff, wszText2.c_str(), XBFONT_CENTER_X|XBFONT_CENTER_Y);
		}
	}
}

void CGUIWindowVideoBase::GoParentFolder()
{
	// don't call Update() with m_strParentPath, as we update m_strParentPath before the
	// directory is retrieved.
	CStdString strPath=m_strParentPath;
	Update(strPath);
  UpdateButtons();
}

void CGUIWindowVideoBase::OnManualIMDB()
{
  CStdString strInput;
  if (!CGUIDialogKeyboard::ShowAndGetInput(strInput, false)) return;

  CStdString strThumb;
  CUtil::GetThumbnail("Z:\\",strThumb);
  ::DeleteFile(strThumb.c_str());

  ShowIMDB(strInput,"Z:\\", "Z:\\", false);
	return ;
}

bool CGUIWindowVideoBase::ViewByIcon()
{
  if ( m_strDirectory.IsEmpty() )
  {
    if (g_stSettings.m_iMyVideoRootViewAsIcons != VIEW_AS_LIST) return true;
  }
  else
  {
    if (g_stSettings.m_iMyVideoViewAsIcons != VIEW_AS_LIST) return true;
  }
  return false;
}

void CGUIWindowVideoBase::UpdateThumbPanel()
{
//	int iItem=GetSelectedItem(); 
	CGUIThumbnailPanel* pControl=(CGUIThumbnailPanel*)GetControl(CONTROL_THUMBS);
	if (pControl)
    pControl->ShowBigIcons(ViewByLargeIcon());
/*  if (iItem>-1)
  {
    CONTROL_SELECT_ITEM(CONTROL_LIST, iItem);
    CONTROL_SELECT_ITEM(CONTROL_THUMBS, iItem);
  }*/
}

bool CGUIWindowVideoBase::IsCorrectDiskInDrive(const CStdString& strFileName, const CStdString& strDVDLabel) 
{
  CDetectDVDMedia::WaitMediaReady();
  CCdInfo* pCdInfo = CDetectDVDMedia::GetCdInfo();
  if (pCdInfo == NULL) return false;
  if (!CUtil::FileExists(strFileName)) return false;
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
 
  CIMDBMovie movieDetails;
  m_database.GetMovieInfo(strFileName,movieDetails);
  if ( !CUtil::IsDVD(movieDetails.m_strPath) && !CUtil::IsISO9660(movieDetails.m_strPath)) return true;
  CGUIDialogOK *pDlgOK= (CGUIDialogOK*)m_gWindowManager.GetWindow(WINDOW_DIALOG_OK);
  if (!pDlgOK) return true;
  while (1)
  {
    if (IsCorrectDiskInDrive(strFileName, movieDetails.m_strDVDLabel)) {
      return true;
    }
    pDlgOK->SetHeading( 428);
    pDlgOK->SetLine( 0, 429 );
    pDlgOK->SetLine( 1, movieDetails.m_strDVDLabel );
    pDlgOK->SetLine( 2, L"" );
    pDlgOK->DoModal( GetID() );
    if (!pDlgOK->IsConfirmed()) 
    {
      break;
    }
  }
  return false; 
}

void CGUIWindowVideoBase::OnQueueItem(int iItem)
{
	// add item 2 playlist
	const CFileItem* pItem=m_vecItems[iItem];
	AddItemToPlayList(pItem);
	
	//move to next item
	CONTROL_SELECT_ITEM(CONTROL_LIST,iItem+1);
	CONTROL_SELECT_ITEM(CONTROL_THUMBS,iItem+1);
}

void CGUIWindowVideoBase::AddItemToPlayList(const CFileItem* pItem) 
{
	if (pItem->m_bIsFolder)
	{
		// recursive
		if (pItem->GetLabel() == "..") return;
		CStdString strDirectory=m_strDirectory;
		m_strDirectory=pItem->m_strPath;
		VECFILEITEMS items;
		CFileItemList itemlist(items);
		GetDirectory(m_strDirectory, items);
    
    SortItems(items);

		for (int i=0; i < (int) items.size(); ++i)
		{
			AddItemToPlayList(items[i]);
		}
		m_strDirectory=strDirectory;
	}
	else
	{
    if (!CUtil::IsNFO(pItem->m_strPath) && CUtil::IsVideo(pItem->m_strPath) && !CUtil::IsPlayList(pItem->m_strPath))
		{
			CPlayList::CPlayListItem playlistItem ;
			playlistItem.SetFileName(pItem->m_strPath);
			playlistItem.SetDescription(pItem->GetLabel());
			playlistItem.SetDuration(pItem->m_musicInfoTag.GetDuration());
			g_playlistPlayer.GetPlaylist( PLAYLIST_VIDEO ).Add(playlistItem);
		}
	}
}

void CGUIWindowVideoBase::DisplayEmptyDatabaseMessage(bool bDisplay)
{
	m_bDisplayEmptyDatabaseMessage = bDisplay;
}

void CGUIWindowVideoBase::OnPopupMenu(int iItem)
{
	// calculate our position
	int iPosX=200;
	int iPosY=100;
	CGUIListControl *pList = (CGUIListControl *)GetControl(CONTROL_LIST);
	if (pList)
	{
		iPosX = pList->GetXPosition()+pList->GetWidth()/2;
		iPosY = pList->GetYPosition()+pList->GetHeight()/2;
	}	
	// mark the item
	m_vecItems[iItem]->Select(true);
	// popup the context menu
	CGUIDialogContextMenu *pMenu = (CGUIDialogContextMenu *)m_gWindowManager.GetWindow(WINDOW_DIALOG_CONTEXT_MENU);
	if (!pMenu) return;
	// clean any buttons not needed
	pMenu->ClearButtons();
	// add the needed buttons
	pMenu->AddButton(13346);	// Show Video Information
	pMenu->AddButton(13347);	// Add to Playlist
	pMenu->AddButton(13350);	// Now Playing...
	pMenu->AddButton(13349);	// Query Info For All Files
	pMenu->AddButton(13348);	// Search IMDb...
	pMenu->AddButton(5);			// Settings

	// turn off the now playing button if nothing is playing
	if (!g_application.IsPlayingVideo())
		pMenu->EnableButton(3, false);
	// turn off the query info button if we aren't in files view
	if (GetID() != WINDOW_VIDEOS)
		pMenu->EnableButton(4, false);
	// position it correctly
	pMenu->SetPosition(iPosX-pMenu->GetWidth()/2, iPosY-pMenu->GetHeight()/2);
	pMenu->DoModal(GetID());
	switch (pMenu->GetButton())
	{
	case 1:	// Show Video Information
		OnInfo(iItem);
		break;
	case 2:	// Add to Playlist
		OnQueueItem(iItem);
		break;
	case 3:	// Now Playing...
		m_gWindowManager.ActivateWindow(WINDOW_VIDEO_PLAYLIST);
		return;
		break;
	case 4:	// Query Info For All Files
		OnScan();
		break;
	case 5:	// Search IMDb...
		OnManualIMDB();
		break;
	case 6:	// Settings
		m_gWindowManager.ActivateWindow(WINDOW_SETTINGS_MYVIDEOS);
		return;
		break;
	}
	m_vecItems[iItem]->Select(false);
}