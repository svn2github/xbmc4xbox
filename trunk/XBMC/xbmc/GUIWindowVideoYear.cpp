// Todo: 
//  - directory history
//  - if movie does not exists when play movie is called then show dialog asking to insert the correct CD
//  - show if movie has subs

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
#include "GUIDialogFileStacking.h"
#define CONTROL_BTNVIEWASICONS		 2
#define CONTROL_BTNSORTBY					 3
#define CONTROL_BTNSORTASC				 4
#define CONTROL_MOVIES             5
#define CONTROL_PLAY_DVD           6
#define CONTROL_STACK              7
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
		if (g_stSettings.m_bMyVideoYearSortAscending) bGreater=false;
    if ( rpStart.m_bIsFolder   == rpEnd.m_bIsFolder)
		{
			char szfilename1[1024];
			char szfilename2[1024];

			switch ( g_stSettings.m_iMyVideoYearSortMethod ) 
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

			if (g_stSettings.m_bMyVideoYearSortAscending)
				return (strcmp(szfilename1,szfilename2)<0);
			else
				return (strcmp(szfilename1,szfilename2)>=0);
		}
    if (!rpStart.m_bIsFolder) return false;
		return true;
	}
};

//****************************************************************************************************************************
CGUIWindowVideoYear::CGUIWindowVideoYear()
{
	m_strDirectory="";
  m_iItemSelected=-1;
}

//****************************************************************************************************************************
CGUIWindowVideoYear::~CGUIWindowVideoYear()
{
}

//****************************************************************************************************************************
void CGUIWindowVideoYear::OnAction(const CAction &action)
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

//****************************************************************************************************************************
bool CGUIWindowVideoYear::OnMessage(CGUIMessage& message)
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
				CONTROL_SELECT_ITEM(GetID(), CONTROL_LIST,iItem)
				CONTROL_SELECT_ITEM(GetID(), CONTROL_THUMBS,iItem)
			}
		}
		break;

		case GUI_MSG_DVDDRIVE_CHANGED_CD:
		{
			if ( m_strDirectory.IsEmpty() ) {
				int iItem = GetSelectedItem();
				Update( m_strDirectory );
				CONTROL_SELECT_ITEM(GetID(), CONTROL_LIST,iItem)
				CONTROL_SELECT_ITEM(GetID(), CONTROL_THUMBS,iItem)
			}
		}
		break;
		case GUI_MSG_WINDOW_DEINIT:
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
			Update(m_strDirectory);

      if (m_iItemSelected >=0)
      {
			  CONTROL_SELECT_ITEM(GetID(), CONTROL_LIST,m_iItemSelected)
			  CONTROL_SELECT_ITEM(GetID(), CONTROL_THUMBS,m_iItemSelected)
      }
		
			return true;
		}
		break;

    case GUI_MSG_CLICKED:
    {
      int iControl=message.GetSenderId();
      if (iControl==CONTROL_BTNVIEWASICONS)
      {
		  if ( m_strDirectory.IsEmpty() )
		    g_stSettings.m_bMyVideoYearRootViewAsIcons=!g_stSettings.m_bMyVideoYearRootViewAsIcons;
		  else
		    g_stSettings.m_bMyVideoYearViewAsIcons=!g_stSettings.m_bMyVideoYearViewAsIcons;

				g_settings.Save();
        UpdateButtons();
      }
      else if (iControl==CONTROL_BTNSORTBY) // sort by
      {
        if (m_strDirectory.size())
        {
        g_stSettings.m_iMyVideoYearSortMethod++;
        if (g_stSettings.m_iMyVideoYearSortMethod>=3) g_stSettings.m_iMyVideoYearSortMethod=0;
				g_settings.Save();
        }
        UpdateButtons();
        OnSort();
      }
      else if (iControl==CONTROL_BTNSORTASC) // sort asc
      {
        g_stSettings.m_bMyVideoYearSortAscending=!g_stSettings.m_bMyVideoYearSortAscending;
				g_settings.Save();
        UpdateButtons();
        OnSort();
      }
      else if (iControl==CONTROL_PLAY_DVD)
      {
          // play movie...
		    g_TextureManager.Flush();
		    g_graphicsContext.SetFullScreenVideo(true);
		    m_gWindowManager.ActivateWindow(WINDOW_FULLSCREEN_VIDEO);
        CIoSupport helper;
        helper.Remount("D:","Cdrom0");
        g_application.PlayFile("dvd://1");
      }
      else if (iControl==CONTROL_LIST||iControl==CONTROL_THUMBS)  // list/thumb control
      {
         // get selected item
        CGUIMessage msg(GUI_MSG_ITEM_SELECTED,GetID(),iControl,0,0,NULL);
        g_graphicsContext.SendMessage(msg);         
        int iItem=msg.GetParam1();
        int iAction=message.GetParam1();
        if (iAction == ACTION_SHOW_INFO) 
        {
          if (m_strDirectory!="")
					  OnInfo(iItem);
        }
        if (iAction == ACTION_SELECT_ITEM)
				{
					OnClick(iItem);
				}
      }
      else if (iControl==CONTROL_MOVIES)
      {
        m_gWindowManager.ActivateWindow(WINDOW_VIDEOS);
        return true;
      }
    }
	}
  return CGUIWindow::OnMessage(message);
}

//****************************************************************************************************************************
void CGUIWindowVideoYear::UpdateButtons()
{

	SET_CONTROL_HIDDEN(GetID(), CONTROL_LIST);
	SET_CONTROL_HIDDEN(GetID(), CONTROL_THUMBS);
	bool bViewIcon = false;
	if ( m_strDirectory.IsEmpty() ) {
		bViewIcon = g_stSettings.m_bMyVideoYearRootViewAsIcons;
	}
	else {
		bViewIcon = g_stSettings.m_bMyVideoYearViewAsIcons;
	}
   if (bViewIcon) 
    {
      SET_CONTROL_VISIBLE(GetID(), CONTROL_THUMBS);
    }
    else
    {
      SET_CONTROL_VISIBLE(GetID(), CONTROL_LIST);
    }

    int iString=101;
    if (!bViewIcon) 
    {
      iString=100;
    }
		SET_CONTROL_LABEL(GetID(), CONTROL_BTNVIEWASICONS,iString);
		SET_CONTROL_LABEL(GetID(), CONTROL_BTNSORTBY,g_stSettings.m_iMyVideoYearSortMethod+365);

    if ( g_stSettings.m_bMyVideoYearSortAscending)
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
		SET_CONTROL_LABEL(GetID(), CONTROL_LABELFILES,wszText);

}

//****************************************************************************************************************************
void CGUIWindowVideoYear::Clear()
{
	CFileItemList itemlist(m_vecItems); // will clean up everything
}

//****************************************************************************************************************************
void CGUIWindowVideoYear::OnSort()
{
 CGUIMessage msg(GUI_MSG_LABEL_RESET,GetID(),CONTROL_LIST,0,0,NULL);
  g_graphicsContext.SendMessage(msg);         

  
  CGUIMessage msg2(GUI_MSG_LABEL_RESET,GetID(),CONTROL_THUMBS,0,0,NULL);
  g_graphicsContext.SendMessage(msg2);         

  
  
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

  
  sort(m_vecItems.begin(), m_vecItems.end(), SSortVideoYearByName());

  for (int i=0; i < (int)m_vecItems.size(); i++)
  {
    CFileItem* pItem=m_vecItems[i];
    CGUIMessage msg(GUI_MSG_LABEL_ADD,GetID(),CONTROL_LIST,0,0,(void*)pItem);
    g_graphicsContext.SendMessage(msg);    
    CGUIMessage msg2(GUI_MSG_LABEL_ADD,GetID(),CONTROL_THUMBS,0,0,(void*)pItem);
    g_graphicsContext.SendMessage(msg2);         
  }
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
			m_history.Set(strSelectedItem,m_strDirectory);
		}
	}
  Clear();
  m_strDirectory=strDirectory;
  if (m_strDirectory=="")
  {
    VECMOVIEYEARS years;
    m_database.GetYears( years);
    for (int i=0; i < (int)years.size(); ++i)
    {
			CFileItem *pItem = new CFileItem(years[i]);
			pItem->m_strPath=years[i];
			pItem->m_bIsFolder=true;
      pItem->m_bIsShareOrDrive=false;
			m_vecItems.push_back(pItem);
    }
    SET_CONTROL_LABEL(GetID(), LABEL_YEAR,"");
  }
  else
  {
			CFileItem *pItem = new CFileItem("..");
			pItem->m_strPath="";
			pItem->m_bIsFolder=true;
      pItem->m_bIsShareOrDrive=false;
			m_vecItems.push_back(pItem);

      VECMOVIES movies;
      m_database.GetMoviesByYear(m_strDirectory, movies);
      for (int i=0; i < (int)movies.size(); ++i)
      {
        CIMDBMovie movie=movies[i];
        CFileItem *pItem = new CFileItem(movie.m_strTitle);
        pItem->m_strPath=movie.m_strSearchString;
        if (CUtil::IsVideo(pItem->m_strPath))
			    pItem->m_bIsFolder=false;
        else
          pItem->m_bIsFolder=true;
        pItem->m_bIsShareOrDrive=false;

        CStdString strThumb;
        CUtil::GetThumbnail(movie.m_strSearchString,strThumb);
        pItem->SetThumbnailImage(strThumb);
        pItem->m_fRating     = movie.m_fRating; 
        pItem->m_stTime.wYear= movie.m_iYear;
			  m_vecItems.push_back(pItem);
      }
      SET_CONTROL_LABEL(GetID(), LABEL_YEAR,m_strDirectory);
  }
	CUtil::SetThumbs(m_vecItems);
  SetIMDBThumbs(m_vecItems);
	CUtil::FillInDefaultIcons(m_vecItems);
  OnSort();
  UpdateButtons();
  strSelectedItem=m_history.Get(m_strDirectory);	

  bool bViewAsIcon = false;
	if ( m_strDirectory.IsEmpty() )
		bViewAsIcon = g_stSettings.m_bMyVideoYearRootViewAsIcons;
	else
		bViewAsIcon = g_stSettings.m_bMyVideoYearViewAsIcons;

	if ( bViewAsIcon ) {	
		SET_CONTROL_FOCUS(GetID(), CONTROL_THUMBS);
	}
	else {
		SET_CONTROL_FOCUS(GetID(), CONTROL_LIST);
	}
  for (int i=0; i < (int)m_vecItems.size(); ++i)
	{
		CFileItem* pItem=m_vecItems[i];
		if (pItem->m_strPath==strSelectedItem)
		{
			CONTROL_SELECT_ITEM(GetID(), CONTROL_LIST,i);
			CONTROL_SELECT_ITEM(GetID(), CONTROL_THUMBS,i);
			break;
		}
	}
}

//****************************************************************************************************************************
void CGUIWindowVideoYear::Render()
{
	CGUIWindow::Render();
}

//****************************************************************************************************************************
void CGUIWindowVideoYear::OnClick(int iItem)
{
  CFileItem* pItem=m_vecItems[iItem];
  CStdString strPath=pItem->m_strPath;

	CStdString strExtension;
	CUtil::GetExtension(pItem->m_strPath,strExtension);

  if (pItem->m_bIsFolder)
  {
    m_iItemSelected=-1;
		if ( pItem->m_bIsShareOrDrive ) 
		{
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
    g_playlistPlayer.SetCurrentPlaylist(PLAYLIST_VIDEO);
    CPlayList& playlist=g_playlistPlayer.GetPlaylist(PLAYLIST_VIDEO);
    playlist.Clear();
    for (int i=iSelectedFile-1; i < (int)movies.size(); ++i)
    {
      CStdString strFileName=movies[i];
      CPlayList::CPlayListItem item;
      item.SetFileName(strFileName);
      playlist.Add(item);
    }

    // play movie...
		g_TextureManager.Flush();
		g_graphicsContext.SetFullScreenVideo(true);
		m_gWindowManager.ActivateWindow(WINDOW_FULLSCREEN_VIDEO);
		g_playlistPlayer.PlayNext();
	}
}

void CGUIWindowVideoYear::OnInfo(int iItem)
{
  CFileItem* pItem=m_vecItems[iItem];
  if (pItem->m_bIsFolder) return;
	
  VECMOVIESFILES movies;
  m_database.GetFiles(atol(pItem->m_strPath),movies);
	if (movies.size() <=0) return;
  CStdString strFilePath=movies[0];
  CStdString strFile=CUtil::GetFileName(strFilePath);
  ShowIMDB(strFile,strFilePath, "" ,false);
}


int CGUIWindowVideoYear::GetSelectedItem()
{
	int iControl;
	bool bViewIcon = false;
	if ( m_strDirectory.IsEmpty() ) {
		bViewIcon = g_stSettings.m_bMyVideoYearRootViewAsIcons;
	}
	else {
		bViewIcon = g_stSettings.m_bMyVideoYearViewAsIcons;
	}
	if ( bViewIcon) 
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
void CGUIWindowVideoYear::SetIMDBThumbs(VECFILEITEMS& items)
{
}