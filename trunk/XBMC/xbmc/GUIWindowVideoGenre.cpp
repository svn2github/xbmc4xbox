// Todo:
//  - directory history
//  - if movie does not exists when play movie is called then show dialog asking to insert the correct CD
//  - show if movie has subs

#include "stdafx.h"
#include "GUIWindowVideoGenre.h"
#include "Util.h"
#include "Utils/imdb.h"
#include "GUIWindowVideoInfo.h"
#include "NFOFile.h"
#include "PlayListPlayer.h"
#include "GUIThumbnailPanel.h"
#include "GUIPassword.h"

#define CONTROL_PLAY_DVD           6
#define CONTROL_STACK              7
#define CONTROL_IMDB        9
#define CONTROL_BTNSHOWMODE       10
#define LABEL_GENRE              100


//****************************************************************************************************************************
CGUIWindowVideoGenre::CGUIWindowVideoGenre()
: CGUIWindowVideoBase(WINDOW_VIDEO_GENRE, "MyVideoGenre.xml")
{
  m_vecItems.m_strPath = "";
}

//****************************************************************************************************************************
CGUIWindowVideoGenre::~CGUIWindowVideoGenre()
{
}

//****************************************************************************************************************************
bool CGUIWindowVideoGenre::OnMessage(CGUIMessage& message)
{
  switch ( message.GetMessage() )
  {
  case GUI_MSG_WINDOW_INIT:
    {
      m_iShowMode = g_stSettings.m_iMyVideoGenreShowMode;
    }
    break;
  case GUI_MSG_CLICKED:
    {
      int iControl = message.GetSenderId();
      if (iControl == CONTROL_BTNSHOWMODE)
	    {
        m_iShowMode++;
		    if (m_iShowMode > VIDEO_SHOW_WATCHED) m_iShowMode = VIDEO_SHOW_ALL;
		    g_stSettings.m_iMyVideoGenreShowMode = m_iShowMode;
        g_settings.Save();
		    Update(m_vecItems.m_strPath);
        return true;
      }
      else
        return CGUIWindowVideoBase::OnMessage(message);
    }
  }
  return CGUIWindowVideoBase::OnMessage(message);
}

bool CGUIWindowVideoGenre::GetDirectory(const CStdString &strDirectory, CFileItemList &items)
{
  items.m_strPath = strDirectory;
  if (items.IsVirtualDirectoryRoot())
  {
    VECMOVIEGENRES genres;
    m_database.GetGenres(genres, m_iShowMode);
    // Display an error message if the database doesn't contain any genres
    DisplayEmptyDatabaseMessage(genres.empty());
    for (int i = 0; i < (int)genres.size(); ++i)
    {
      CFileItem *pItem = new CFileItem(genres[i]);
      pItem->m_strPath = genres[i];
      pItem->m_bIsFolder = true;
      pItem->m_bIsShareOrDrive = false;
      pItem->FillInDefaultIcon();
      items.Add(pItem);
    }
  }
  else
  {
    if (m_guiState.get() && !m_guiState->HideParentDirItems())
    {
      CFileItem *pItem = new CFileItem("..");
      pItem->m_strPath = "";
      pItem->m_bIsFolder = true;
      pItem->m_bIsShareOrDrive = false;
      items.Add(pItem);
    }

    VECMOVIES movies;
    m_database.GetMoviesByGenre(items.m_strPath, movies);
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
        CFileItem *pItem = new CFileItem(strTitle);
        pItem->m_strTitle=strTitle;
        pItem->m_strPath = movie.m_strSearchString;
        pItem->m_bIsFolder = false;
        pItem->m_bIsShareOrDrive = false;

        CStdString strThumb;
        CUtil::GetVideoThumbnail(movie.m_strIMDBNumber, strThumb);
        if (CFile::Exists(strThumb))
          pItem->SetThumbnailImage(strThumb);
        pItem->m_fRating = movie.m_fRating;
        pItem->m_stTime.wYear = movie.m_iYear;
        if (!movie.m_bWatched)
          pItem->SetOverlayImage(CGUIListItem::ICON_OVERLAY_UNWATCHED);
        items.Add(pItem);
      }
    }
  }

  return true;
}

void CGUIWindowVideoGenre::OnPrepareFileItems(CFileItemList &items)
{
  items.SetThumbs();

  // Fill in default icons
  // normally this is done by the base class for us
  // but the m_strPath only contains the database id
  // of the movie, so we must fake a videofile to fill
  // in default icons
  CStdString strPath;
  for (int i = 0; i < (int)items.Size(); i++)
  {
    CFileItem* pItem = items[i];

    if (pItem->m_bIsFolder)
      continue;

    strPath = pItem->m_strPath;
    // Fake a videofile
    pItem->m_strPath = pItem->m_strPath + ".avi";
    pItem->FillInDefaultIcon();
    pItem->m_strPath = strPath;
  }
}

void CGUIWindowVideoGenre::UpdateButtons()
{
  CGUIWindowVideoBase::UpdateButtons();
  SET_CONTROL_LABEL(LABEL_GENRE, m_vecItems.m_strPath);
}

void CGUIWindowVideoGenre::OnInfo(int iItem)
{
  if ( m_vecItems.IsVirtualDirectoryRoot() ) return ;
  CGUIWindowVideoBase::OnInfo(iItem);
}
