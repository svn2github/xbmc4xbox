#include "guiwindowpictures.h"
#include "settings.h"
#include "guiWindowManager.h"
#include "localizestrings.h"
#include "util.h"
#include "url.h"
#include "picture.h"
#include "utils/log.h"
#include "application.h"
#include <algorithm>
#include "GUIDialogOK.h"
#include "DetectDVDType.h"
#include "sectionloader.h"
#include "GUIThumbnailPanel.h"
#define VIEW_AS_LIST           0
#define VIEW_AS_ICONS          1
#define VIEW_AS_LARGEICONS     2

#define CONTROL_BTNVIEWASICONS		2
#define CONTROL_BTNSORTBY					3
#define CONTROL_BTNSORTASC				4

#define CONTROL_BTNSLIDESHOW			6
#define CONTROL_BTNSLIDESHOW_RECURSIVE			7

#define CONTROL_BTNCREATETHUMBS		8
#define CONTROL_LIST							10
#define CONTROL_THUMBS						11
#define CONTROL_LABELFILES         12

struct SSortPicturesByName
{
	bool operator()(CFileItem* pStart, CFileItem* pEnd)
	{
    CFileItem& rpStart=*pStart;
    CFileItem& rpEnd=*pEnd;
		if (rpStart.GetLabel()=="..") return true;
		if (rpEnd.GetLabel()=="..") return false;
		bool bGreater=true;
		if (g_stSettings.m_bMyPicturesSortAscending) bGreater=false;
    if ( rpStart.m_bIsFolder   == rpEnd.m_bIsFolder)
		{
			char szfilename1[1024];
			char szfilename2[1024];

			switch ( g_stSettings.m_iMyPicturesSortMethod ) 
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

			if (g_stSettings.m_bMyPicturesSortAscending)
				return (strcmp(szfilename1,szfilename2)<0);
			else
				return (strcmp(szfilename1,szfilename2)>=0);
		}
    if (!rpStart.m_bIsFolder) return false;
		return true;
	}
};

CGUIWindowPictures::CGUIWindowPictures(void)
:CGUIWindow(0)
{
	m_strDirectory="?";
  m_iItemSelected=-1;
}

CGUIWindowPictures::~CGUIWindowPictures(void)
{
}

void CGUIWindowPictures::OnAction(const CAction &action)
{
	if (action.wID == ACTION_PARENT_DIR)
	{
		GoParentFolder();
		return;
	}
	if (action.wID == ACTION_PREVIOUS_MENU)
	{
		m_gWindowManager.PreviousWindow();
		return;
	}
	CGUIWindow::OnAction(action);
}

bool CGUIWindowPictures::OnMessage(CGUIMessage& message)
{
  switch ( message.GetMessage() )
  {
		case GUI_MSG_START_SLIDESHOW:
		{
			CStdString* pUrl = (CStdString*) message.GetLPVOID();
			Update( *pUrl );
			OnSlideShow();
		}
		break;
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
		  	if (message.GetParam1() != WINDOW_SLIDESHOW)
			{
				CSectionLoader::Unload("CXIMAGE");
			}
    break;

    case GUI_MSG_WINDOW_INIT:
		{
			CGUIWindow::OnMessage(message);
		  	if (message.GetParam1() != WINDOW_SLIDESHOW)
			{
				CSectionLoader::Load("CXIMAGE");
			}
			if (m_strDirectory=="?")
				m_strDirectory=g_stSettings.m_szDefaultPictures;

			m_dlgProgress = (CGUIDialogProgress*)m_gWindowManager.GetWindow(WINDOW_DIALOG_PROGRESS);
			m_rootDir.SetMask(g_stSettings.m_szMyPicturesExtensions);
			int iControl=CONTROL_LIST;

      if (!ViewByIcon()) iControl=CONTROL_THUMBS;
			SET_CONTROL_HIDDEN(GetID(), iControl);

      if ( g_stSettings.m_bMyPicturesSortAscending)
      {
        CGUIMessage msg(GUI_MSG_DESELECTED,GetID(), CONTROL_BTNSORTASC);
        g_graphicsContext.SendMessage(msg);
      }
      else
      {
        CGUIMessage msg(GUI_MSG_SELECTED,GetID(), CONTROL_BTNSORTASC);
        g_graphicsContext.SendMessage(msg);
      }

			m_rootDir.SetShares(g_settings.m_vecMyPictureShares);
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

        bool bLargeIcons(false);
		    if ( m_strDirectory.IsEmpty() )
        {
		      g_stSettings.m_iMyPicturesRootViewAsIcons++;
          if (g_stSettings.m_iMyPicturesRootViewAsIcons > VIEW_AS_LARGEICONS) g_stSettings.m_iMyPicturesRootViewAsIcons=VIEW_AS_LIST;
        }
		    else
        {
		      g_stSettings.m_iMyPicturesViewAsIcons++;
          if (g_stSettings.m_iMyPicturesViewAsIcons > VIEW_AS_LARGEICONS) g_stSettings.m_iMyPicturesViewAsIcons=VIEW_AS_LIST;
        }

				g_settings.Save();
        ShowThumbPanel();
				
        UpdateButtons();
      }
      else if (iControl==CONTROL_BTNSORTBY) // sort by
      {
        g_stSettings.m_iMyPicturesSortMethod++;
        if (g_stSettings.m_iMyPicturesSortMethod >=3) g_stSettings.m_iMyPicturesSortMethod=0;
				g_settings.Save();
        UpdateButtons();
        OnSort();
      }
      else if (iControl==CONTROL_BTNSORTASC) // sort asc
      {
        g_stSettings.m_bMyPicturesSortAscending=!g_stSettings.m_bMyPicturesSortAscending;
				g_settings.Save();
        UpdateButtons();
        OnSort();
      }
      else if (iControl==CONTROL_BTNSLIDESHOW) // Slide Show
      {
				OnSlideShow();
      }
      else if (iControl==CONTROL_BTNSLIDESHOW_RECURSIVE) // Recursive Slide Show
      {
        OnSlideShowRecursive();
      }
      else if (iControl==CONTROL_BTNCREATETHUMBS) // Create Thumbs
      {
				OnCreateThumbs();
      }
      else if (iControl==CONTROL_LIST||iControl==CONTROL_THUMBS)  // list/thumb control
      {
         // get selected item
        CGUIMessage msg(GUI_MSG_ITEM_SELECTED,GetID(),iControl,0,0,NULL);
        g_graphicsContext.SendMessage(msg);         
        int iItem=msg.GetParam1();
				int iAction=message.GetParam1();
				if (iAction==ACTION_SELECT_ITEM)
				{
					OnClick(iItem);
				}
      }
    }
    break;
	}
  return CGUIWindow::OnMessage(message);
}

void CGUIWindowPictures::OnSort()
{
  CGUIMessage msg(GUI_MSG_LABEL_RESET,GetID(),CONTROL_LIST,0,0,NULL);
  g_graphicsContext.SendMessage(msg);         

  
  CGUIMessage msg2(GUI_MSG_LABEL_RESET,GetID(),CONTROL_THUMBS,0,0,NULL);
  g_graphicsContext.SendMessage(msg2);         

  
  
  for (int i=0; i < (int)m_vecItems.size(); i++)
  {
    CFileItem* pItem=m_vecItems[i];
    if (g_stSettings.m_iMyPicturesSortMethod==0||g_stSettings.m_iMyPicturesSortMethod==2)
    {
			if (pItem->m_bIsFolder) pItem->SetLabel2("");
      else 
			{
				CStdString strFileSize;
				CUtil::GetFileSize(pItem->m_dwSize, strFileSize);
				pItem->SetLabel2(strFileSize);
			}
    }
    else
    {
      if (pItem->m_stTime.wYear)
			{
				CStdString strDateTime;
        CUtil::GetDate(pItem->m_stTime, strDateTime);
        pItem->SetLabel2(strDateTime);
			}
      else
        pItem->SetLabel2("");
    }
  }

  
  sort(m_vecItems.begin(), m_vecItems.end(), SSortPicturesByName());

  for (int i=0; i < (int)m_vecItems.size(); i++)
  {
    CFileItem* pItem=m_vecItems[i];
    CGUIMessage msg(GUI_MSG_LABEL_ADD,GetID(),CONTROL_LIST,0,0,(void*)pItem);
    g_graphicsContext.SendMessage(msg);    
    CGUIMessage msg2(GUI_MSG_LABEL_ADD,GetID(),CONTROL_THUMBS,0,0,(void*)pItem);
    g_graphicsContext.SendMessage(msg2);         
  }
}

void CGUIWindowPictures::Clear()
{
	CFileItemList itemlist(m_vecItems); // will clean up everything
}

void CGUIWindowPictures::UpdateButtons()
{

	SET_CONTROL_HIDDEN(GetID(), CONTROL_THUMBS);
	SET_CONTROL_HIDDEN(GetID(), CONTROL_LIST);
	bool bViewIcon = false;
  int iString;
	if ( m_strDirectory.IsEmpty() ) 
  {
		switch (g_stSettings.m_iMyPicturesRootViewAsIcons)
    {
      case VIEW_AS_LIST:
        iString=100; // view as icons
      break;
      
      case VIEW_AS_ICONS:
        iString=417;  // view as large icons
        bViewIcon=true;
      break;
      case VIEW_AS_LARGEICONS:
        iString=101; // view as list
        bViewIcon=true;
      break;

      default:
        iString=100; // view as icons
      break;
    }
	}
	else 
  {
		switch (g_stSettings.m_iMyPicturesViewAsIcons)
    {
      case VIEW_AS_LIST:
        iString=100; // view as icons
      break;
      
      case VIEW_AS_ICONS:
        iString=417;  // view as large icons
        bViewIcon=true;
      break;
      case VIEW_AS_LARGEICONS:
        iString=101; // view as list
        bViewIcon=true;
      break;
      default:
        iString=100; // view as icons
      break;

    }		
	}
  if (bViewIcon) 
  {
    SET_CONTROL_VISIBLE(GetID(), CONTROL_THUMBS);
  }
  else
  {
    SET_CONTROL_VISIBLE(GetID(), CONTROL_LIST);
  }

		SET_CONTROL_LABEL(GetID(), CONTROL_BTNVIEWASICONS,iString);
		SET_CONTROL_LABEL(GetID(), CONTROL_BTNSORTBY,g_stSettings.m_iMyPicturesSortMethod+103);

    if ( g_stSettings.m_bMyPicturesSortAscending)
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



void CGUIWindowPictures::Update(const CStdString &strDirectory)
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

	CStdString strParentPath;
	bool bParentExists=CUtil::GetParentPath(strDirectory, strParentPath);

	// check if current directory is a root share
  if ( !m_rootDir.IsShare(strDirectory) )
  {
		// no, do we got a parent dir?
		if ( bParentExists )
		{
			// yes
			CFileItem *pItem = new CFileItem("..");
			pItem->m_strPath=strParentPath;
			pItem->m_bIsFolder=true;
      pItem->m_bIsShareOrDrive=false;
			m_vecItems.push_back(pItem);
		}
  }
  else
  {
		// yes, this is the root of a share
		// add parent path to the virtual directory
	  CFileItem *pItem = new CFileItem("..");
	  pItem->m_strPath="";
    pItem->m_bIsShareOrDrive=false;
	  pItem->m_bIsFolder=true;
	  m_vecItems.push_back(pItem);
  }

  m_strDirectory=strDirectory;
	m_rootDir.GetDirectory(strDirectory,m_vecItems);
  OnSort();
  UpdateButtons();

	strSelectedItem=m_history.Get(m_strDirectory);	

	if ( ViewByIcon() ) 
  {	
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


void CGUIWindowPictures::OnClick(int iItem)
{
  CFileItem* pItem=m_vecItems[iItem];
  CStdString strPath=pItem->m_strPath;
	if (pItem->m_bIsFolder)
	{
    m_iItemSelected=-1;
		if ( pItem->m_bIsShareOrDrive ) {
			if ( !HaveDiscOrConnection( pItem->m_strPath, pItem->m_iDriveType ) )
				return;
		}
		Update(strPath);
	}
	else
	{
		// show picture
    m_iItemSelected=GetSelectedItem();
		OnShowPicture(strPath);
	}
}

bool CGUIWindowPictures::HaveDiscOrConnection( CStdString& strPath, int iDriveType )
{
	if ( iDriveType == SHARE_TYPE_DVD ) {
		CDetectDVDMedia::WaitMediaReady();
		if ( !CDetectDVDMedia::IsDiscInDrive() ) {
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
			CONTROL_SELECT_ITEM(GetID(), CONTROL_LIST,iItem)
			CONTROL_SELECT_ITEM(GetID(), CONTROL_THUMBS,iItem)
			return false;
		}
	}
	else if ( iDriveType == SHARE_TYPE_REMOTE ) {
			// TODO: Handle not connected to a remote share
		if ( !CUtil::IsEthernetConnected() ) {
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

void CGUIWindowPictures::OnShowPicture(const CStdString& strPicture)
{
	CGUIWindowSlideShow *pSlideShow = (CGUIWindowSlideShow *)m_gWindowManager.GetWindow(WINDOW_SLIDESHOW);
	if (!pSlideShow)
		return;
  if (g_application.IsPlayingVideo())
    g_application.StopPlaying();

	pSlideShow->Reset();
  for (int i=0; i < (int)m_vecItems.size();++i)
  {
    CFileItem* pItem=m_vecItems[i];
    if (!pItem->m_bIsFolder)
    {
		  pSlideShow->Add(pItem->m_strPath);
    }
  }
	pSlideShow->Select(strPicture);
	m_gWindowManager.ActivateWindow(WINDOW_SLIDESHOW);
}

void CGUIWindowPictures::AddDir(CGUIWindowSlideShow *pSlideShow,const CStdString& strPath)
{
  if (!pSlideShow) return;
  VECFILEITEMS items;
  m_rootDir.GetDirectory(strPath,items);
  for (int i=0; i < (int)items.size();++i)
  {
    CFileItem* pItem=items[i];
    if (!pItem->m_bIsFolder)
    {
		  pSlideShow->Add(pItem->m_strPath);
    }
    else
    {
      AddDir(pSlideShow,pItem->m_strPath);
    }
  }
  CFileItemList itemlist(items); // will clean up everything
}

void  CGUIWindowPictures::OnSlideShowRecursive()
{
	CGUIWindowSlideShow *pSlideShow = (CGUIWindowSlideShow *)m_gWindowManager.GetWindow(WINDOW_SLIDESHOW);
	if (!pSlideShow)
		return;
	
  if (g_application.IsPlayingVideo())
    g_application.StopPlaying();

  pSlideShow->Reset();
  AddDir(pSlideShow,m_strDirectory);
  pSlideShow->StartSlideShow();
	m_gWindowManager.ActivateWindow(WINDOW_SLIDESHOW);
}

void CGUIWindowPictures::OnSlideShow()
{
	CGUIWindowSlideShow *pSlideShow = (CGUIWindowSlideShow *)m_gWindowManager.GetWindow(WINDOW_SLIDESHOW);
	if (!pSlideShow)
		return;
  if (g_application.IsPlayingVideo())
      g_application.StopPlaying();

	pSlideShow->Reset();
  for (int i=0; i < (int)m_vecItems.size();++i)
  {
    CFileItem* pItem=m_vecItems[i];
    if (!pItem->m_bIsFolder)
    {
		  pSlideShow->Add(pItem->m_strPath);
    }
  }
	pSlideShow->StartSlideShow();
	m_gWindowManager.ActivateWindow(WINDOW_SLIDESHOW);
}

void CGUIWindowPictures::OnCreateThumbs()
{
  if (m_dlgProgress) 
  {
    m_dlgProgress->SetHeading(110);
	  m_dlgProgress->StartModal(GetID());
  }
  CSectionLoader::Load("CXIMAGE");
  for (int i=0; i < (int)m_vecItems.size();++i)
  {
    CFileItem* pItem=m_vecItems[i];
    if (!pItem->m_bIsFolder)
    {
			WCHAR wstrProgress[128];
			WCHAR wstrFile[128];
      swprintf(wstrProgress,L"   progress:%i/%i", i, m_vecItems.size() );
      swprintf(wstrFile,L"   picture:%S", pItem->GetLabel().c_str() );

			if (m_dlgProgress)
      {
        m_dlgProgress->SetLine(0, wstrFile);
			  m_dlgProgress->SetLine(1, wstrProgress);
			  m_dlgProgress->SetLine(2, L"");
			  m_dlgProgress->Progress();
			  if ( m_dlgProgress->IsCanceled() ) break;
      }
			CPicture picture;
      picture.CreateThumnail(pItem->m_strPath);
    }
  }
  CSectionLoader::Unload("CXIMAGE");
	if (m_dlgProgress) m_dlgProgress->Close();
  Update(m_strDirectory);
}

void CGUIWindowPictures::Render()
{
	CGUIWindow::Render();
}


int CGUIWindowPictures::GetSelectedItem()
{
	int iControl;
	bool bViewAsIcon=false;

  if ( ViewByIcon()) 
		iControl=CONTROL_THUMBS;
  else
		iControl=CONTROL_LIST;

  CGUIMessage msg(GUI_MSG_ITEM_SELECTED,GetID(),iControl,0,0,NULL);
  g_graphicsContext.SendMessage(msg);         
  int iItem=msg.GetParam1();
	return iItem;
}


void CGUIWindowPictures::GoParentFolder()
{
	if (m_vecItems.size()==0) return;
	CFileItem* pItem=m_vecItems[0];
	if (pItem->m_bIsFolder)
	{
		if (pItem->GetLabel()=="..")
		{
			CStdString strPath=pItem->m_strPath;
			Update(strPath);
		}
	}
}

bool CGUIWindowPictures::ViewByIcon()
{
  if ( m_strDirectory.IsEmpty() )
  {
    if (g_stSettings.m_iMyPicturesRootViewAsIcons != VIEW_AS_LIST) return true;
  }
  else
  {
    if (g_stSettings.m_iMyPicturesViewAsIcons != VIEW_AS_LIST) return true;
  }
  return false;
}

bool CGUIWindowPictures::ViewByLargeIcon()
{
  if ( m_strDirectory.IsEmpty() )
  {
    if (g_stSettings.m_iMyPicturesRootViewAsIcons == VIEW_AS_LARGEICONS) return true;
  }
  else
  {
    if (g_stSettings.m_iMyPicturesViewAsIcons== VIEW_AS_LARGEICONS) return true;
  }
  return false;
}

void CGUIWindowPictures::ShowThumbPanel()
{
 
  if ( ViewByLargeIcon() )
  {
    CGUIThumbnailPanel* pControl=(CGUIThumbnailPanel*)GetControl(CONTROL_THUMBS);
    pControl->SetThumbDimensions(10,16,100,100);
    pControl->SetTextureDimensions(128,128);
    pControl->SetItemHeight(150);
    pControl->SetItemWidth(150);
  }
  else
  {
    CGUIThumbnailPanel* pControl=(CGUIThumbnailPanel*)GetControl(CONTROL_THUMBS);
    pControl->SetThumbDimensions(4,10,64,64);
    pControl->SetTextureDimensions(80,80);
    pControl->SetItemHeight(128);
    pControl->SetItemWidth(128);
  }
}