#include "stdafx.h"
#include "GUIViewStateVideo.h"
#include "playlistplayer.h"
#include "FileSystem/VideoDatabaseDirectory.h"

using namespace DIRECTORY::VIDEODATABASEDIRECTORY;

CStdString CGUIViewStateWindowVideo::GetLockType()
{
  return "video";
}

bool CGUIViewStateWindowVideo::UnrollArchives()
{
  return g_guiSettings.GetBool("filelists.unrollarchives");
}

CStdString CGUIViewStateWindowVideo::GetExtensions()
{
  return g_stSettings.m_videoExtensions;
}

CGUIViewStateWindowVideoFiles::CGUIViewStateWindowVideoFiles(const CFileItemList& items) : CGUIViewStateWindowVideo(items)
{
  if (items.IsVirtualDirectoryRoot())
  {
    AddSortMethod(SORT_METHOD_LABEL, 551, LABEL_MASKS()); // Preformated
    AddSortMethod(SORT_METHOD_DRIVE_TYPE, 564, LABEL_MASKS()); // Preformated
    SetSortMethod(SORT_METHOD_LABEL);

    SetViewAsControl(DEFAULT_VIEW_LIST);

    SetSortOrder(SORT_ORDER_ASC);
  }
  else
  {
    // always use the label by default
    CStdString strFileMask = "%L";
    // when stacking is enabled, filenames are already cleaned so use the existing label
    if (g_stSettings.m_iMyVideoStack != STACK_NONE)
      strFileMask = "%L"; 
    if (g_guiSettings.GetBool("filelists.ignorethewhensorting"))
      AddSortMethod(SORT_METHOD_LABEL_IGNORE_THE, 551, LABEL_MASKS(strFileMask, "%I", "%L", ""));  // FileName, Size | Foldername, empty
    else
      AddSortMethod(SORT_METHOD_LABEL, 551, LABEL_MASKS(strFileMask, "%I", "%L", ""));  // FileName, Size | Foldername, empty
    AddSortMethod(SORT_METHOD_SIZE, 553, LABEL_MASKS(strFileMask, "%I", "%L", "%I"));  // FileName, Size | Foldername, Size
    AddSortMethod(SORT_METHOD_DATE, 552, LABEL_MASKS(strFileMask, "%J", "%L", "%J"));  // FileName, Date | Foldername, Date
    SetSortMethod((SORT_METHOD)g_guiSettings.GetInt("myvideos.sortmethod"));

    SetViewAsControl(g_guiSettings.GetInt("myvideos.viewmode"));

    SetSortOrder((SORT_ORDER)g_guiSettings.GetInt("myvideos.sortorder"));
  }
  LoadViewState(items.m_strPath, WINDOW_VIDEO_FILES);
}

void CGUIViewStateWindowVideoFiles::SaveViewState()
{
  SaveViewToDb(m_items.m_strPath, WINDOW_VIDEO_FILES);
}

VECSHARES& CGUIViewStateWindowVideoFiles::GetShares()
{
  return g_settings.m_vecMyVideoShares;
}

CGUIViewStateWindowVideoNav::CGUIViewStateWindowVideoNav(const CFileItemList& items) : CGUIViewStateWindowVideo(items)
{
  if (items.IsVirtualDirectoryRoot())
  {
    AddSortMethod(SORT_METHOD_NONE, 551, LABEL_MASKS("%F", "%I", "%L", ""));  // Filename, Size | Foldername, empty
    SetSortMethod(SORT_METHOD_NONE);

    SetViewAsControl(DEFAULT_VIEW_LIST);

    SetSortOrder(SORT_ORDER_NONE);
  }
  else if (items.IsVideoDb())
  {
    CVideoDatabaseDirectory dir;
    NODE_TYPE NodeType=dir.GetDirectoryChildType(items.m_strPath);
    NODE_TYPE ParentNodeType=dir.GetDirectoryType(items.m_strPath);
    switch (NodeType)
    {
    case NODE_TYPE_OVERVIEW:
      {
        AddSortMethod(SORT_METHOD_LABEL, 551, LABEL_MASKS("%T", "%R", "%L", ""));  // Filename, Duration | Foldername, empty
        SetSortMethod(SORT_METHOD_NONE);

        SetViewAsControl(DEFAULT_VIEW_LIST);

        SetSortOrder(SORT_ORDER_NONE);
      }
      break;
    case NODE_TYPE_ACTOR:
      {
        AddSortMethod(SORT_METHOD_LABEL, 551, LABEL_MASKS("%T", "%R", "%L", ""));  // Filename, Duration | Foldername, empty
        SetSortMethod(SORT_METHOD_LABEL);

        SetViewAsControl(g_stSettings.m_viewStateVideoNavActors.m_viewMode);

        SetSortOrder(g_stSettings.m_viewStateVideoNavActors.m_sortOrder);
      }
      break;
    case NODE_TYPE_YEAR:
      {
        AddSortMethod(SORT_METHOD_LABEL, 551, LABEL_MASKS("%T", "%R", "%L", ""));  // Filename, Duration | Foldername, empty
        SetSortMethod(SORT_METHOD_LABEL);

        SetViewAsControl(g_stSettings.m_viewStateVideoNavYears.m_viewMode);

        SetSortOrder(g_stSettings.m_viewStateVideoNavYears.m_sortOrder);
      }
      break;
    case NODE_TYPE_GENRE:
      {
        AddSortMethod(SORT_METHOD_LABEL, 551, LABEL_MASKS("%T", "%R", "%L", ""));  // Filename, Duration | Foldername, empty
        SetSortMethod(SORT_METHOD_LABEL);

        SetViewAsControl(g_stSettings.m_viewStateVideoNavGenres.m_viewMode);

        SetSortOrder(g_stSettings.m_viewStateVideoNavGenres.m_sortOrder);
      }
      break;
      case NODE_TYPE_TITLE:
      {
        if (g_guiSettings.GetBool("filelists.ignorethewhensorting"))
          AddSortMethod(SORT_METHOD_LABEL_IGNORE_THE, 551, LABEL_MASKS("%T", "%R", "%L", ""));  // Filename, Duration | Foldername, empty
        else
          AddSortMethod(SORT_METHOD_LABEL, 551, LABEL_MASKS("%T", "%R", "%L", ""));  // Filename, Duration | Foldername, empty
        AddSortMethod(SORT_METHOD_VIDEO_RATING, 563, LABEL_MASKS("%T", "%R", "%L", ""));  // Filename, Duration | Foldername, empty
        SetSortMethod(g_stSettings.m_viewStateVideoNavTitles.m_sortMethod);

        SetViewAsControl(g_stSettings.m_viewStateVideoNavTitles.m_viewMode);

        SetSortOrder(g_stSettings.m_viewStateVideoNavTitles.m_sortOrder);
      }
      break;
    }
  }
  else
  {
    AddSortMethod(SORT_METHOD_LABEL, 551, LABEL_MASKS("%F", "%D", "%L", ""));  // Filename, Duration | Foldername, empty
    SetSortMethod(SORT_METHOD_LABEL);

    SetViewAsControl(DEFAULT_VIEW_LIST);

  }
  LoadViewState(items.m_strPath, WINDOW_VIDEO_NAV);
}

void CGUIViewStateWindowVideoNav::SaveViewState()
{
  CVideoDatabaseDirectory dir;
  NODE_TYPE NodeType = dir.GetDirectoryChildType(m_items.m_strPath);
  switch (NodeType)
  {
  case NODE_TYPE_ACTOR:
    SaveViewToDb(m_items.m_strPath, WINDOW_VIDEO_NAV, g_stSettings.m_viewStateVideoNavActors);
    break;
  case NODE_TYPE_YEAR:
    SaveViewToDb(m_items.m_strPath, WINDOW_VIDEO_NAV, g_stSettings.m_viewStateVideoNavYears);
    break;
  case NODE_TYPE_GENRE:
    SaveViewToDb(m_items.m_strPath, WINDOW_VIDEO_NAV, g_stSettings.m_viewStateVideoNavGenres);
    break;
  case NODE_TYPE_TITLE:
    SaveViewToDb(m_items.m_strPath, WINDOW_VIDEO_NAV, g_stSettings.m_viewStateVideoNavTitles);
    break;
  default:
    SaveViewToDb(m_items.m_strPath, WINDOW_VIDEO_NAV);
    break;
  }
}

VECSHARES& CGUIViewStateWindowVideoNav::GetShares()
{
  //  Setup shares we want to have
  m_shares.clear();
  //  Musicdb shares
  CFileItemList items;
  CDirectory::GetDirectory("videodb://", items);
  for (int i=0; i<items.Size(); ++i)
  {
    CFileItem* item=items[i];
    CShare share;
    share.strName=item->GetLabel();
    share.strPath = item->m_strPath;
    share.m_strThumbnailImage="defaultFolderBig.png";
    share.m_iDriveType = SHARE_TYPE_LOCAL;
    m_shares.push_back(share);
  }

  //  Playlists share
  CShare share;
  share.strName=g_localizeStrings.Get(136); // Playlists
  share.strPath = "special://videoplaylists/";
  share.m_strThumbnailImage="defaultFolderBig.png";
  share.m_iDriveType = SHARE_TYPE_LOCAL;
  m_shares.push_back(share);

  return CGUIViewStateWindowVideo::GetShares();
}

CGUIViewStateWindowVideoPlaylist::CGUIViewStateWindowVideoPlaylist(const CFileItemList& items) : CGUIViewStateWindowVideo(items)
{
  AddSortMethod(SORT_METHOD_NONE, 551, LABEL_MASKS("%L", "", "%L", ""));  // Label, "" | Label, empty
  SetSortMethod(SORT_METHOD_NONE);

  SetViewAsControl(DEFAULT_VIEW_LIST);

  SetSortOrder(SORT_ORDER_NONE);

  LoadViewState(items.m_strPath, WINDOW_VIDEO_PLAYLIST);
}

void CGUIViewStateWindowVideoPlaylist::SaveViewState()
{
  SaveViewToDb(m_items.m_strPath, WINDOW_VIDEO_PLAYLIST);
}

int CGUIViewStateWindowVideoPlaylist::GetPlaylist()
{
  return PLAYLIST_VIDEO;
}

bool CGUIViewStateWindowVideoPlaylist::HideExtensions()
{
  return true;
}

bool CGUIViewStateWindowVideoPlaylist::HideParentDirItems()
{
  return true;
}

VECSHARES& CGUIViewStateWindowVideoPlaylist::GetShares()
{
  m_shares.clear();
  //  Playlist share
  CShare share;
  share.strName;
  share.strPath= "playlistvideo://";
  share.m_strThumbnailImage="defaultFolderBig.png";
  share.m_iDriveType = SHARE_TYPE_LOCAL;
  m_shares.push_back(share);

  return CGUIViewStateWindowVideo::GetShares();
}
