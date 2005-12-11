// Todo:
//  - directory history
//  - if movie does not exists when play movie is called then show dialog asking to insert the correct CD
//  - show if movie has subs

#include "stdafx.h"
#include "GUIWindowVideoActors.h"
#include "Util.h"
#include "GUIWindowVideoInfo.h"
#include "nfofile.h"
#include "GUIPassword.h"
#include "SortFileItem.h"

#define CONTROL_BTNVIEWASICONS     2 
#define CONTROL_BTNSORTBY          3
#define CONTROL_BTNSORTASC         4
#define CONTROL_BTNTYPE            5
#define CONTROL_LIST              50
#define CONTROL_THUMBS            51
#define CONTROL_LABELFILES        12

#define LABEL_ACTOR              100
#define CONTROL_PLAY_DVD           6
#define CONTROL_STACK              7
#define CONTROL_IMDB        9
#define CONTROL_BTNSHOWMODE       10

//****************************************************************************************************************************
CGUIWindowVideoActors::CGUIWindowVideoActors()
: CGUIWindowVideoBase(WINDOW_VIDEO_ACTOR, "MyVideoActors.xml")
{
  m_Directory.m_strPath = "";
}

//****************************************************************************************************************************
CGUIWindowVideoActors::~CGUIWindowVideoActors()
{
}

//****************************************************************************************************************************
bool CGUIWindowVideoActors::OnMessage(CGUIMessage& message)
{
  switch ( message.GetMessage() )
  {
  case GUI_MSG_WINDOW_INIT:
    {
      m_iShowMode = g_stSettings.m_iMyVideoActorShowMode;
    }
    break;

  case GUI_MSG_CLICKED:
    {
      int iControl = message.GetSenderId();
      if (iControl == CONTROL_BTNSORTBY) // sort by
      {
        if (!m_Directory.IsVirtualDirectoryRoot())
        {
          g_stSettings.m_iMyVideoActorSortMethod++;
          if (g_stSettings.m_iMyVideoActorSortMethod >= 3)
            g_stSettings.m_iMyVideoActorSortMethod = 0;
          g_settings.Save();
        }
        UpdateButtons();
        OnSort();
      }
      else if (iControl == CONTROL_BTNSORTASC) // sort asc
      {
        if (m_Directory.IsVirtualDirectoryRoot())
          g_stSettings.m_bMyVideoActorRootSortAscending = !g_stSettings.m_bMyVideoActorRootSortAscending;
        else
          g_stSettings.m_bMyVideoActorSortAscending = !g_stSettings.m_bMyVideoActorSortAscending;

        g_settings.Save();

        UpdateButtons();
        OnSort();
      }
      else if (iControl == CONTROL_BTNSHOWMODE)
	    {
        m_iShowMode++;
		    if (m_iShowMode > VIDEO_SHOW_WATCHED) m_iShowMode = VIDEO_SHOW_ALL;
		    g_stSettings.m_iMyVideoActorShowMode = m_iShowMode;
        g_settings.Save();
		    Update(m_Directory.m_strPath);
        return true;
      }
      else
        return CGUIWindowVideoBase::OnMessage(message);
    }
  }
  return CGUIWindowVideoBase::OnMessage(message);
}

//****************************************************************************************************************************
void CGUIWindowVideoActors::FormatItemLabels()
{
  for (int i = 0; i < m_vecItems.Size(); i++)
  {
    CFileItem* pItem = m_vecItems[i];
    if (g_stSettings.m_iMyVideoActorSortMethod == 0 || g_stSettings.m_iMyVideoActorSortMethod == 2)
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
        strDateTime.Format("%i", pItem->m_stTime.wYear);
        pItem->SetLabel2(strDateTime);
      }
      else
        pItem->SetLabel2("");
    }
  }
}

void CGUIWindowVideoActors::SortItems(CFileItemList& items)
{
  int sortMethod;
  bool sortAscending;
  if (m_Directory.IsVirtualDirectoryRoot())
  {
    sortMethod = g_stSettings.m_iMyVideoActorRootSortMethod;
    sortAscending = g_stSettings.m_bMyVideoActorRootSortAscending;
  }
  else
  {
    sortMethod = g_stSettings.m_iMyVideoActorSortMethod;
    sortAscending = g_stSettings.m_bMyVideoActorSortAscending;
    if (g_stSettings.m_iMyVideoActorSortMethod == 1)
      sortAscending = !sortAscending;
  }
  switch (sortMethod)
  {
  case 1:
    items.Sort(sortAscending ? SSortFileItem::MovieYearAscending : SSortFileItem::MovieYearDescending); break;
  case 2:
    items.Sort(sortAscending ? SSortFileItem::MovieRatingAscending : SSortFileItem::MovieRatingDescending); break;
  default:
    if (g_guiSettings.GetBool("MyVideos.IgnoreTheWhenSorting"))
      items.Sort(sortAscending ? SSortFileItem::LabelAscendingNoThe : SSortFileItem::LabelDescendingNoThe);
    else
      items.Sort(sortAscending ? SSortFileItem::LabelAscending : SSortFileItem::LabelDescending);
    break;
  }
}

//****************************************************************************************************************************
bool CGUIWindowVideoActors::Update(const CStdString &strDirectory)
{
  // get selected item
  int iItem = m_viewControl.GetSelectedItem();
  CStdString strSelectedItem = "";
  if (iItem >= 0 && iItem < m_vecItems.Size())
  {
    CFileItem* pItem = m_vecItems[iItem];
    if (pItem->GetLabel() != "..")
    {
      strSelectedItem = pItem->m_strPath;
      m_history.Set(strSelectedItem, m_Directory.m_strPath);
    }
  }
  ClearFileItems();
  m_Directory.m_strPath = strDirectory;
  if (m_Directory.IsVirtualDirectoryRoot())
  {
    VECMOVIEACTORS actors;
    m_database.GetActors(actors, m_iShowMode);
    // Display an error message if the database doesn't contain any actors
    DisplayEmptyDatabaseMessage(actors.empty());
    for (int i = 0; i < (int)actors.size(); ++i)
    {
      CFileItem *pItem = new CFileItem(actors[i]);
      pItem->m_strPath = actors[i];
      pItem->m_bIsFolder = true;
      pItem->m_bIsShareOrDrive = false;
      m_vecItems.Add(pItem);
    }
    SET_CONTROL_LABEL(LABEL_ACTOR, "");
  }
  else
  {
    if (!g_guiSettings.GetBool("MyVideos.HideParentDirItems"))
    {
      CFileItem *pItem = new CFileItem("..");
      pItem->m_strPath = "";
      pItem->m_bIsFolder = true;
      pItem->m_bIsShareOrDrive = false;
      m_vecItems.Add(pItem);
    }
    m_strParentPath = "";
    VECMOVIES movies;
    m_database.GetMoviesByActor(m_Directory.m_strPath, movies);
    for (int i = 0; i < (int)movies.size(); ++i)
    {
      CIMDBMovie movie = movies[i];
      // add the appropiate movies to m_vecItems based on the showmode
      if (
        (m_iShowMode == VIDEO_SHOW_ALL) ||
        (m_iShowMode == VIDEO_SHOW_WATCHED && movie.m_bWatched == true) ||
        (m_iShowMode == VIDEO_SHOW_UNWATCHED && movie.m_bWatched == false)
        )
      {
        // mark watched movies when showing all
        CStdString strTitle = movie.m_strTitle;
        if (m_iShowMode == VIDEO_SHOW_ALL && movie.m_bWatched == true)
          strTitle += " [W]";
        CFileItem *pItem = new CFileItem(strTitle);
        pItem->m_strPath = movie.m_strSearchString;
        pItem->m_bIsFolder = false;
        pItem->m_bIsShareOrDrive = false;

        CStdString strThumb;
        CUtil::GetVideoThumbnail(movie.m_strIMDBNumber, strThumb);
        if (CFile::Exists(strThumb))
          pItem->SetThumbnailImage(strThumb);
        pItem->m_fRating = movie.m_fRating;
        pItem->m_stTime.wYear = movie.m_iYear;
        m_vecItems.Add(pItem);
      }
    }
    SET_CONTROL_LABEL(LABEL_ACTOR, m_Directory.m_strPath);
  }
  m_vecItems.SetThumbs();
  SetIMDBThumbs(m_vecItems);

  // Fill in default icons
  CStdString strPath;
  for (int i = 0; i < (int)m_vecItems.Size(); i++)
  {
    CFileItem* pItem = m_vecItems[i];
    strPath = pItem->m_strPath;
    // Fake a videofile
    pItem->m_strPath = pItem->m_strPath + ".avi";

    pItem->FillInDefaultIcon();
    pItem->m_strPath = strPath;
  }

  OnSort();
  UpdateButtons();
  // update selected item
  strSelectedItem = m_history.Get(m_Directory.m_strPath);
  for (int i = 0; i < (int)m_vecItems.Size(); ++i)
  {
    CFileItem* pItem = m_vecItems[i];
    if (pItem->m_strPath == strSelectedItem)
    {
      m_viewControl.SetSelectedItem(i);
      break;
    }
  }

  return true;
}

//****************************************************************************************************************************
void CGUIWindowVideoActors::OnClick(int iItem)
{
  if ( iItem < 0 || iItem >= (int)m_vecItems.Size() ) return ;
  CFileItem* pItem = m_vecItems[iItem];
  CStdString strPath = pItem->m_strPath;

  CStdString strExtension;
  CUtil::GetExtension(pItem->m_strPath, strExtension);

  if (pItem->m_bIsFolder)
  {
    m_iItemSelected = -1;
    if ( pItem->m_bIsShareOrDrive )
    {
      if ( !g_passwordManager.IsItemUnlocked( pItem, "video" ) )
        return ;

      if ( !HaveDiscOrConnection( pItem->m_strPath, pItem->m_iDriveType ) )
        return ;
    }
    Update(strPath);
  }
  else
  {
    m_iItemSelected = m_viewControl.GetSelectedItem();
    PlayMovie(pItem);
  }
}

void CGUIWindowVideoActors::OnInfo(int iItem)
{
  if ( m_Directory.IsVirtualDirectoryRoot() ) return ;
  CGUIWindowVideoBase::OnInfo(iItem);
}

void CGUIWindowVideoActors::LoadViewMode()
{
  m_iViewAsIconsRoot = g_stSettings.m_iMyVideoActorRootViewAsIcons;
  m_iViewAsIcons = g_stSettings.m_iMyVideoActorViewAsIcons;
}

void CGUIWindowVideoActors::SaveViewMode()
{
  g_stSettings.m_iMyVideoActorRootViewAsIcons = m_iViewAsIconsRoot;
  g_stSettings.m_iMyVideoActorViewAsIcons = m_iViewAsIcons;
  g_settings.Save();
}

int CGUIWindowVideoActors::SortMethod()
{
  if (m_Directory.IsVirtualDirectoryRoot())
    return g_stSettings.m_iMyVideoActorRootSortMethod + 365;
  else
    return g_stSettings.m_iMyVideoActorSortMethod + 365;
}

bool CGUIWindowVideoActors::SortAscending()
{
  if (m_Directory.IsVirtualDirectoryRoot())
    return g_stSettings.m_bMyVideoActorRootSortAscending;
  else
    return g_stSettings.m_bMyVideoActorSortAscending;
}
