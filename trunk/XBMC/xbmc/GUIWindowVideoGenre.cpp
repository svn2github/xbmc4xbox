// Todo:
//  - directory history
//  - if movie does not exists when play movie is called then show dialog asking to insert the correct CD
//  - show if movie has subs

#include "stdafx.h"
#include "GUIWindowVideoGenre.h"
#include "Util.h"
#include "Utils/imdb.h"
#include "GUIWindowVideoInfo.h"
#include "Application.h"
#include "NFOFile.h"
#include "PlayListPlayer.h"
#include "GUIThumbnailPanel.h"
#include "GUIPassword.h"

#define CONTROL_BTNVIEWASICONS   2
#define CONTROL_BTNSORTBY      3
#define CONTROL_BTNSORTASC     4
#define CONTROL_BTNTYPE            5
#define CONTROL_PLAY_DVD           6
#define CONTROL_STACK              7
#define CONTROL_IMDB        9
#define CONTROL_LIST       10
#define CONTROL_THUMBS      11
#define CONTROL_LABELFILES        12
#define LABEL_GENRE              100

//****************************************************************************************************************************
struct SSortVideoGenreByName
{
  static bool Sort(CFileItem* pStart, CFileItem* pEnd)
  {
    CFileItem& rpStart = *pStart;
    CFileItem& rpEnd = *pEnd;
    if (rpStart.GetLabel() == "..") return true;
    if (rpEnd.GetLabel() == "..") return false;
    bool bGreater = true;
    if (m_bSortAscending) bGreater = false;
    if ( rpStart.m_bIsFolder == rpEnd.m_bIsFolder)
    {
      char szfilename1[1024];
      char szfilename2[1024];

      switch ( m_iSortMethod )
      {
      case 0:  // Sort by name
        strcpy(szfilename1, rpStart.GetLabel().c_str());
        strcpy(szfilename2, rpEnd.GetLabel().c_str());
        break;

      case 1:  // Sort by year
        if ( rpStart.m_stTime.wYear > rpEnd.m_stTime.wYear ) return bGreater;
        if ( rpStart.m_stTime.wYear < rpEnd.m_stTime.wYear ) return !bGreater;
        return true;
        break;

      case 2:  // sort by rating
        if ( rpStart.m_fRating < rpEnd.m_fRating) return bGreater;
        if ( rpStart.m_fRating > rpEnd.m_fRating) return !bGreater;
        return true;
        break;

      default:  // Sort by Filename by default
        strcpy(szfilename1, rpStart.GetLabel().c_str());
        strcpy(szfilename2, rpEnd.GetLabel().c_str());
        break;
      }


      for (int i = 0; i < (int)strlen(szfilename1); i++)
        szfilename1[i] = tolower((unsigned char)szfilename1[i]);

      for (i = 0; i < (int)strlen(szfilename2); i++)
        szfilename2[i] = tolower((unsigned char)szfilename2[i]);
      //return (rpStart.strPath.compare( rpEnd.strPath )<0);

      if (m_bSortAscending)
        return (strcmp(szfilename1, szfilename2) < 0);
      else
        return (strcmp(szfilename1, szfilename2) >= 0);
    }
    if (!rpStart.m_bIsFolder) return false;
    return true;
  }

  static bool m_bSortAscending;
  static int m_iSortMethod;
};
bool SSortVideoGenreByName::m_bSortAscending;
int SSortVideoGenreByName::m_iSortMethod;

//****************************************************************************************************************************
CGUIWindowVideoGenre::CGUIWindowVideoGenre()
{
  m_Directory.m_strPath = "";
  m_Directory.m_bIsFolder = true;
  m_iItemSelected = -1;
  m_iLastControl = -1;
}

//****************************************************************************************************************************
CGUIWindowVideoGenre::~CGUIWindowVideoGenre()
{}

//****************************************************************************************************************************
void CGUIWindowVideoGenre::OnAction(const CAction &action)
{
  CGUIWindowVideoBase::OnAction(action);
}

//****************************************************************************************************************************
bool CGUIWindowVideoGenre::OnMessage(CGUIMessage& message)
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
      int iControl = message.GetSenderId();
      if (iControl == CONTROL_BTNSORTBY) // sort by
      {
        if (!m_Directory.IsVirtualDirectoryRoot())
        {
          g_stSettings.m_iMyVideoGenreSortMethod++;
          if (g_stSettings.m_iMyVideoGenreSortMethod >= 3)
            g_stSettings.m_iMyVideoGenreSortMethod = 0;
          g_settings.Save();
        }
        UpdateButtons();
        OnSort();
      }
      else if (iControl == CONTROL_BTNSORTASC) // sort asc
      {
        if (m_Directory.IsVirtualDirectoryRoot())
          g_stSettings.m_bMyVideoGenreRootSortAscending = !g_stSettings.m_bMyVideoGenreRootSortAscending;
        else
          g_stSettings.m_bMyVideoGenreSortAscending = !g_stSettings.m_bMyVideoGenreSortAscending;

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
void CGUIWindowVideoGenre::FormatItemLabels()
{
  for (int i = 0; i < (int)m_vecItems.Size(); i++)
  {
    CFileItem* pItem = m_vecItems[i];
    if (g_stSettings.m_iMyVideoGenreSortMethod == 0 || g_stSettings.m_iMyVideoGenreSortMethod == 2)
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

void CGUIWindowVideoGenre::SortItems(CFileItemList& items)
{
  if (m_Directory.IsVirtualDirectoryRoot())
  {
    SSortVideoGenreByName::m_iSortMethod = g_stSettings.m_iMyVideoGenreRootSortMethod;
    SSortVideoGenreByName::m_bSortAscending = g_stSettings.m_bMyVideoGenreRootSortAscending;
  }
  else
  {
    SSortVideoGenreByName::m_iSortMethod = g_stSettings.m_iMyVideoGenreSortMethod;
    SSortVideoGenreByName::m_bSortAscending = g_stSettings.m_bMyVideoGenreSortAscending;
  }
  items.Sort(SSortVideoGenreByName::Sort);
}

//****************************************************************************************************************************
void CGUIWindowVideoGenre::Update(const CStdString &strDirectory)
{
  // get selected item
  int iItem = m_viewControl.GetSelectedItem();
  CStdString strSelectedItem = "";
  if (iItem >= 0 && iItem < (int)m_vecItems.Size())
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
    VECMOVIEGENRES genres;
    m_database.GetGenres( genres);
    // Display an error message if the database doesn't contain any genres
    DisplayEmptyDatabaseMessage(genres.empty());
    for (int i = 0; i < (int)genres.size(); ++i)
    {
      CFileItem *pItem = new CFileItem(genres[i]);
      pItem->m_strPath = genres[i];
      pItem->m_bIsFolder = true;
      pItem->m_bIsShareOrDrive = false;
      m_vecItems.Add(pItem);
    }
    SET_CONTROL_LABEL(LABEL_GENRE, "");
  }
  else
  {
    if (!g_guiSettings.GetBool("VideoLists.HideParentDirItems"))
    {
      CFileItem *pItem = new CFileItem("..");
      pItem->m_strPath = "";
      pItem->m_bIsFolder = true;
      pItem->m_bIsShareOrDrive = false;
      m_vecItems.Add(pItem);
    }
    m_strParentPath = "";
    VECMOVIES movies;
    m_database.GetMoviesByGenre(m_Directory.m_strPath, movies);
    for (int i = 0; i < (int)movies.size(); ++i)
    {
      CIMDBMovie movie = movies[i];
      CFileItem *pItem = new CFileItem(movie.m_strTitle);
      pItem->m_strPath = movie.m_strSearchString;
      pItem->m_bIsFolder = false;
      pItem->m_bIsShareOrDrive = false;

      CStdString strThumb;
      CUtil::GetVideoThumbnail(movie.m_strIMDBNumber, strThumb);
      if (CUtil::FileExists(strThumb))
        pItem->SetThumbnailImage(strThumb);
      pItem->m_fRating = movie.m_fRating;
      pItem->m_stTime.wYear = movie.m_iYear;
      m_vecItems.Add(pItem);
    }
    SET_CONTROL_LABEL(LABEL_GENRE, m_Directory.m_strPath);
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
}

//****************************************************************************************************************************
void CGUIWindowVideoGenre::OnClick(int iItem)
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
    int iSelectedFile = 1;
    VECMOVIESFILES movies;
    m_database.GetFiles(atol(pItem->m_strPath), movies);
    PlayMovies(movies, pItem->m_lStartOffset);
  }
}

void CGUIWindowVideoGenre::OnInfo(int iItem)
{
  if ( m_Directory.IsVirtualDirectoryRoot() ) return ;
  CGUIWindowVideoBase::OnInfo(iItem);
}

void CGUIWindowVideoGenre::LoadViewMode()
{
  m_iViewAsIconsRoot = g_stSettings.m_iMyVideoGenreRootViewAsIcons;
  m_iViewAsIcons = g_stSettings.m_iMyVideoGenreViewAsIcons;
}

void CGUIWindowVideoGenre::SaveViewMode()
{
  g_stSettings.m_iMyVideoGenreRootViewAsIcons = m_iViewAsIconsRoot;
  g_stSettings.m_iMyVideoGenreViewAsIcons = m_iViewAsIcons;
  g_settings.Save();
}

int CGUIWindowVideoGenre::SortMethod()
{
  if (m_Directory.IsVirtualDirectoryRoot())
    return g_stSettings.m_iMyVideoGenreRootSortMethod + 365;
  else
    return g_stSettings.m_iMyVideoGenreSortMethod + 365;
}

bool CGUIWindowVideoGenre::SortAscending()
{
  if (m_Directory.IsVirtualDirectoryRoot())
    return g_stSettings.m_bMyVideoGenreRootSortAscending;
  else
    return g_stSettings.m_bMyVideoGenreSortAscending;
}
