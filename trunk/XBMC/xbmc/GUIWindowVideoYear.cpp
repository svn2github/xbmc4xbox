// Todo: 
//  - directory history
//  - if movie does not exists when play movie is called then show dialog asking to insert the correct CD
//  - show if movie has subs

#include "stdafx.h"
#include "GUIWindowVideoYear.h"
#include "settings.h"
#include "guiWindowManager.h"
#include "localizestrings.h"
#include "texturemanager.h"
#include "util.h"
#include "url.h"
#include "utils/imdb.h"
#include "GUIDialogOK.h"
#include "GUIDialogprogress.h"
#include "GUIDialogSelect.h" 
#include "GUIWindowVideoInfo.h" 
#include "application.h" 
#include <algorithm>
#include "DetectDVDType.h"
#include "nfofile.h"
#include "filesystem/file.h"
#include "xbox/iosupport.h"
#include "playlistplayer.h"
#include "GUIThumbnailPanel.h"
#include "GUIPassword.h"

#define VIEW_AS_LIST           0
#define VIEW_AS_ICONS          1
#define VIEW_AS_LARGEICONS     2

#define CONTROL_BTNVIEWASICONS		 2
#define CONTROL_BTNSORTBY					 3
#define CONTROL_BTNSORTASC				 4
#define CONTROL_BTNTYPE            5
#define CONTROL_PLAY_DVD           6
#define CONTROL_STACK              7
#define CONTROL_IMDB							 9
#define CONTROL_LIST							10
#define CONTROL_THUMBS						11
#define CONTROL_LABELFILES        12
#define LABEL_YEAR              100

//****************************************************************************************************************************
struct SSortVideoYearByName
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
				case 0:	//	Sort by name
					strcpy(szfilename1, rpStart.GetLabel().c_str());
					strcpy(szfilename2, rpEnd.GetLabel().c_str());
					break;

				case 1: // Sort by year
          if ( rpStart.m_stTime.wYear > rpEnd.m_stTime.wYear ) return bGreater;
					if ( rpStart.m_stTime.wYear < rpEnd.m_stTime.wYear ) return !bGreater;
					return true;
				break;

        case 2: // sort by rating
          if ( rpStart.m_fRating < rpEnd.m_fRating) return bGreater;
					if ( rpStart.m_fRating > rpEnd.m_fRating) return !bGreater;
					return true;
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

//****************************************************************************************************************************
CGUIWindowVideoYear::CGUIWindowVideoYear()
{
	m_Directory.m_strPath="";
	m_Directory.m_bIsFolder=true;
	m_iItemSelected=-1;
	m_iLastControl=-1;
}

//****************************************************************************************************************************
CGUIWindowVideoYear::~CGUIWindowVideoYear()
{
}

//****************************************************************************************************************************
void CGUIWindowVideoYear::OnAction(const CAction &action)
{
	CGUIWindowVideoBase::OnAction(action);
}

//****************************************************************************************************************************
bool CGUIWindowVideoYear::OnMessage(CGUIMessage& message)
{
  switch ( message.GetMessage() )
  {
		case GUI_MSG_DVDDRIVE_EJECTED_CD:
		case GUI_MSG_DVDDRIVE_CHANGED_CD:
		case GUI_MSG_WINDOW_DEINIT:
    case GUI_MSG_WINDOW_INIT:
			return CGUIWindowVideoBase::OnMessage(message);
		break;

    case GUI_MSG_CLICKED:
    {
      int iControl=message.GetSenderId();
      if (iControl==CONTROL_BTNSORTBY) // sort by
      {
        if (!m_Directory.IsVirtualDirectoryRoot())
        {
					g_stSettings.m_iMyVideoYearSortMethod++;
					if (g_stSettings.m_iMyVideoYearSortMethod>=3)
						g_stSettings.m_iMyVideoYearSortMethod=0;
					g_settings.Save();
        }
        UpdateButtons();
        OnSort();
      }
      else if (iControl==CONTROL_BTNSORTASC) // sort asc
      {
				if (m_Directory.IsVirtualDirectoryRoot())
	        g_stSettings.m_bMyVideoYearRootSortAscending=!g_stSettings.m_bMyVideoYearRootSortAscending;
				else
	        g_stSettings.m_bMyVideoYearSortAscending=!g_stSettings.m_bMyVideoYearSortAscending;

				g_settings.Save();
        UpdateButtons();
        OnSort();
      }
      else
 				return CGUIWindowVideoBase::OnMessage(message);
		}
	}
  return CGUIWindow::OnMessage(message);
}

//****************************************************************************************************************************
void CGUIWindowVideoYear::FormatItemLabels()
{
  for (int i=0; i < (int)m_vecItems.size(); i++)
  {
    CFileItem* pItem=m_vecItems[i];
    if (g_stSettings.m_iMyVideoYearSortMethod==0||g_stSettings.m_iMyVideoYearSortMethod==2)
    {
			if (pItem->m_bIsFolder) pItem->SetLabel2("");
      else 
			{
        CStdString strRating;
        strRating.Format("%2.2f", pItem->m_fRating);
				pItem->SetLabel2(strRating);
			}
    }
    else
    {
      if (pItem->m_stTime.wYear)
			{
				CStdString strDateTime;
        strDateTime.Format("%i",pItem->m_stTime.wYear); 
        pItem->SetLabel2(strDateTime);
			}
      else
        pItem->SetLabel2("");
    }
  }
}

void CGUIWindowVideoYear::SortItems(VECFILEITEMS& items)
{
  SSortVideoYearByName sortmethod;
	if (m_Directory.IsVirtualDirectoryRoot())
	{
		sortmethod.m_iSortMethod=g_stSettings.m_iMyVideoYearRootSortMethod;
		sortmethod.m_bSortAscending=g_stSettings.m_bMyVideoYearRootSortAscending;
	}
	else
	{
		sortmethod.m_iSortMethod=g_stSettings.m_iMyVideoYearSortMethod;
		sortmethod.m_bSortAscending=g_stSettings.m_bMyVideoYearSortAscending;
	}
  sort(items.begin(), items.end(), sortmethod);
}

//****************************************************************************************************************************
void CGUIWindowVideoYear::Update(const CStdString &strDirectory)
{
  // get selected item
	int iItem=GetSelectedItem();
	CStdString strSelectedItem="";
	if (iItem >=0 && iItem < (int)m_vecItems.size())
	{
		CFileItem* pItem=m_vecItems[iItem];
		if (pItem->m_bIsFolder && pItem->GetLabel() != "..")
		{
			strSelectedItem=pItem->m_strPath;
			m_history.Set(strSelectedItem,m_Directory.m_strPath);
		}
	}
  ClearFileItems();
  m_Directory.m_strPath=strDirectory;
  if (m_Directory.IsVirtualDirectoryRoot())
  {
    VECMOVIEYEARS years;
    m_database.GetYears( years);
		// Display an error message if the database doesn't contain any years
		DisplayEmptyDatabaseMessage(years.empty());
    for (int i=0; i < (int)years.size(); ++i)
    {
			CFileItem *pItem = new CFileItem(years[i]);
			pItem->m_strPath=years[i];
			pItem->m_bIsFolder=true;
      pItem->m_bIsShareOrDrive=false;
			m_vecItems.push_back(pItem);
    }
    SET_CONTROL_LABEL(LABEL_YEAR,"");
  }
  else
  {
		if (!g_guiSettings.GetBool("VideoLists.HideParentDirItems"))
		{
			CFileItem *pItem = new CFileItem("..");
			pItem->m_strPath="";
			pItem->m_bIsFolder=true;
      pItem->m_bIsShareOrDrive=false;
			m_vecItems.push_back(pItem);
		}
		m_strParentPath = "";
		VECMOVIES movies;
		m_database.GetMoviesByYear(m_Directory.m_strPath, movies);
		for (int i=0; i < (int)movies.size(); ++i)
		{
			CIMDBMovie movie=movies[i];
			CFileItem *pItem = new CFileItem(movie.m_strTitle);
			pItem->m_strPath=movie.m_strSearchString;
			pItem->m_bIsFolder=false;
			pItem->m_bIsShareOrDrive=false;

			CStdString strThumb;
			CUtil::GetVideoThumbnail(movie.m_strIMDBNumber,strThumb);
			if (CUtil::FileExists(strThumb))
				pItem->SetThumbnailImage(strThumb);
			pItem->m_fRating     = movie.m_fRating; 
			pItem->m_stTime.wYear= movie.m_iYear;
			m_vecItems.push_back(pItem);
		}
		SET_CONTROL_LABEL(LABEL_YEAR,m_Directory.m_strPath);
  }
	CUtil::SetThumbs(m_vecItems);
  SetIMDBThumbs(m_vecItems);

	// Fill in default icons
	CStdString strPath;
	for (int i=0; i<(int)m_vecItems.size(); i++)
	{
		CFileItem* pItem=m_vecItems[i];
		strPath=pItem->m_strPath;
		//	Fake a videofile
		pItem->m_strPath=pItem->m_strPath+".avi";

		pItem->FillInDefaultIcon();
		pItem->m_strPath=strPath;
	}

  OnSort();
  UpdateButtons();
  strSelectedItem=m_history.Get(m_Directory.m_strPath);	

	m_iLastControl=GetFocusedControl();

	if (m_iLastControl==CONTROL_THUMBS || m_iLastControl==CONTROL_LIST)
	{
		if ( ViewByIcon() ) {	
			SET_CONTROL_FOCUS(CONTROL_THUMBS, 0);
		}
		else {
			SET_CONTROL_FOCUS(CONTROL_LIST, 0);
		}
	}

  for (int i=0; i < (int)m_vecItems.size(); ++i)
	{
		CFileItem* pItem=m_vecItems[i];
		if (pItem->m_strPath==strSelectedItem)
		{
			CONTROL_SELECT_ITEM(CONTROL_LIST,i);
			CONTROL_SELECT_ITEM(CONTROL_THUMBS,i);
			break;
		}
	}
}

//****************************************************************************************************************************
void CGUIWindowVideoYear::OnClick(int iItem)
{
	if ( iItem < 0 || iItem >= (int)m_vecItems.size() ) return;
  CFileItem* pItem=m_vecItems[iItem];
  CStdString strPath=pItem->m_strPath;

	CStdString strExtension;
	CUtil::GetExtension(pItem->m_strPath,strExtension);

  if (pItem->m_bIsFolder)
  {
    m_iItemSelected=-1;
		if ( pItem->m_bIsShareOrDrive ) 
		{
      if ( !CGUIPassword::IsItemUnlocked( pItem, "video" ) )
        return;

			if ( !HaveDiscOrConnection( pItem->m_strPath, pItem->m_iDriveType ) )
				return;
		}
    Update(strPath);
  }
  else
	{
    m_iItemSelected=GetSelectedItem();
    int iSelectedFile=1;
		VECMOVIESFILES movies;
    m_database.GetFiles(atol(pItem->m_strPath),movies);
		if (movies.size() <=0) return;
    if (!CheckMovie(movies[0])) return;
    if (movies.size() >1)
    {
      CGUIDialogFileStacking* dlg = (CGUIDialogFileStacking*)m_gWindowManager.GetWindow(WINDOW_DIALOG_FILESTACKING);
		  if (dlg)
      {
        dlg->SetNumberOfFiles(movies.size());
		    dlg->DoModal(GetID());
		    iSelectedFile = dlg->GetSelectedFile();
      }
      if (iSelectedFile < 1) return;
    }

    g_playlistPlayer.Reset();
    g_playlistPlayer.SetCurrentPlaylist(PLAYLIST_VIDEO_TEMP);
    CPlayList& playlist=g_playlistPlayer.GetPlaylist(PLAYLIST_VIDEO_TEMP);
    playlist.Clear();
    for (int i=iSelectedFile-1; i < (int)movies.size(); ++i)
    {
      CStdString strFileName=movies[i];
      CPlayList::CPlayListItem item;
      item.SetFileName(strFileName);
      playlist.Add(item);
    }

    // play movie...
		g_playlistPlayer.PlayNext();
	}
}

void CGUIWindowVideoYear::OnInfo(int iItem)
{
	if ( iItem < 0 || iItem >= (int)m_vecItems.size() ) return;
  if ( m_Directory.IsVirtualDirectoryRoot() ) return;
	CGUIWindowVideoBase::OnInfo(iItem);
}

bool CGUIWindowVideoYear::ViewByIcon()
{
  if ( m_Directory.IsVirtualDirectoryRoot() )
  {
    if (g_stSettings.m_iMyVideoYearRootViewAsIcons != VIEW_AS_LIST) return true;
  }
  else
  {
    if (g_stSettings.m_iMyVideoYearViewAsIcons != VIEW_AS_LIST) return true;
  }
  return false;
}

bool CGUIWindowVideoYear::ViewByLargeIcon()
{
  if ( m_Directory.IsVirtualDirectoryRoot() )
  {
    if (g_stSettings.m_iMyVideoYearRootViewAsIcons == VIEW_AS_LARGEICONS) return true;
  }
  else
  {
    if (g_stSettings.m_iMyVideoYearViewAsIcons== VIEW_AS_LARGEICONS) return true;
  }
  return false;
}

void CGUIWindowVideoYear::SetViewMode(int iViewMode)
{
  if ( m_Directory.IsVirtualDirectoryRoot() )
    g_stSettings.m_iMyVideoYearRootViewAsIcons = iViewMode;
  else
    g_stSettings.m_iMyVideoYearViewAsIcons = iViewMode;
}

int CGUIWindowVideoYear::SortMethod()
{
	if (m_Directory.IsVirtualDirectoryRoot())
		return g_stSettings.m_iMyVideoYearRootSortMethod+365;
	else
		return g_stSettings.m_iMyVideoYearSortMethod+365;
}

bool CGUIWindowVideoYear::SortAscending()
{
	if (m_Directory.IsVirtualDirectoryRoot())
		return g_stSettings.m_bMyVideoYearRootSortAscending;
	else
		return g_stSettings.m_bMyVideoYearSortAscending;
}
