#include "stdafx.h"
#include "GUIViewStateVideo.h"
#include "AutoSwitch.h"
#include "playlistplayer.h"

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
    AddSortMethod(SORT_METHOD_LABEL, 103, LABEL_MASKS()); // Preformated
    AddSortMethod(SORT_METHOD_DRIVE_TYPE, 498, LABEL_MASKS()); // Preformated
    SetSortMethod(g_stSettings.m_MyVideoRootSortMethod);

    AddViewAsControl(VIEW_METHOD_LIST, 101);
    AddViewAsControl(VIEW_METHOD_ICONS, 100);
    AddViewAsControl(VIEW_METHOD_LARGE_ICONS, 417);
    SetViewAsControl(g_stSettings.m_MyVideoRootViewMethod);

    SetSortOrder(g_stSettings.m_MyVideoRootSortOrder);
  }
  else
  {
    // always use the label by default
    CStdString strFileMask = "%L";
    // when stacking is enabled, filenames are already cleaned so use the existing label
    if (g_stSettings.m_iMyVideoStack != STACK_NONE)
      strFileMask = "%L"; 
    if (g_guiSettings.GetBool("filelists.ignorethewhensorting"))
      AddSortMethod(SORT_METHOD_LABEL_IGNORE_THE, 103, LABEL_MASKS(strFileMask, "%I", "%L", ""));  // FileName, Size | Foldername, empty
    else
      AddSortMethod(SORT_METHOD_LABEL, 103, LABEL_MASKS(strFileMask, "%I", "%L", ""));  // FileName, Size | Foldername, empty
    AddSortMethod(SORT_METHOD_SIZE, 105, LABEL_MASKS(strFileMask, "%I", "%L", "%I"));  // FileName, Size | Foldername, Size
    AddSortMethod(SORT_METHOD_DATE, 104, LABEL_MASKS(strFileMask, "%J", "%L", "%J"));  // FileName, Date | Foldername, Date
    SetSortMethod(g_stSettings.m_MyVideoSortMethod);

    AddViewAsControl(VIEW_METHOD_LIST, 101);
    AddViewAsControl(VIEW_METHOD_ICONS, 100);
    AddViewAsControl(VIEW_METHOD_LARGE_ICONS, 417);
    if (g_guiSettings.GetBool("myvideos.useautoswitching"))
    {
      SetViewAsControl(CAutoSwitch::GetView(items));
    }
    else
    {
      SetViewAsControl(g_stSettings.m_MyVideoViewMethod);
    }

    SetSortOrder(g_stSettings.m_MyVideoSortOrder);
  }
}

void CGUIViewStateWindowVideoFiles::SaveViewState()
{
  if (m_items.IsVirtualDirectoryRoot())
  {
    g_stSettings.m_MyVideoRootSortMethod=GetSortMethod();
    g_stSettings.m_MyVideoRootViewMethod=GetViewAsControl();
    g_stSettings.m_MyVideoRootSortOrder=GetSortOrder();
  }
  else
  {
    g_stSettings.m_MyVideoSortMethod=GetSortMethod();
    g_stSettings.m_MyVideoViewMethod=GetViewAsControl();
    g_stSettings.m_MyVideoSortOrder=GetSortOrder();
  }
  g_settings.Save();
}

VECSHARES& CGUIViewStateWindowVideoFiles::GetShares()
{
  return g_settings.m_vecMyVideoShares;
}

CGUIViewStateWindowVideoTitle::CGUIViewStateWindowVideoTitle(const CFileItemList& items) : CGUIViewStateWindowVideo(items)
{
  if (g_guiSettings.GetBool("filelists.ignorethewhensorting"))
    AddSortMethod(SORT_METHOD_LABEL_IGNORE_THE, 103, LABEL_MASKS("%K", "%R", "%L", ""));  // Titel, Rating | Foldername, empty
  else
    AddSortMethod(SORT_METHOD_LABEL, 103, LABEL_MASKS("%K", "%R", "%L", ""));  // Titel, Rating | Foldername, empty
  AddSortMethod(SORT_METHOD_VIDEO_YEAR, 366, LABEL_MASKS("%K", "%Y", "%L", ""));  // Titel, Year | Foldername, empty
  AddSortMethod(SORT_METHOD_VIDEO_RATING, 367, LABEL_MASKS("%K", "%R", "%L", ""));  // Titel, Rating | Foldername, empty
  SetSortMethod(g_stSettings.m_MyVideoTitleSortMethod);

  AddViewAsControl(VIEW_METHOD_LIST, 101);
  AddViewAsControl(VIEW_METHOD_ICONS, 100);
  AddViewAsControl(VIEW_METHOD_LARGE_ICONS, 417);
  AddViewAsControl(VIEW_METHOD_LARGE_LIST, 759);
  SetViewAsControl(g_stSettings.m_MyVideoTitleViewMethod);

  SetSortOrder((SORT_ORDER)g_stSettings.m_MyVideoTitleSortOrder);
}

void CGUIViewStateWindowVideoTitle::SaveViewState()
{
  g_stSettings.m_MyVideoTitleSortMethod=GetSortMethod();
  g_stSettings.m_MyVideoTitleViewMethod=GetViewAsControl();
  g_stSettings.m_MyVideoTitleSortOrder=GetSortOrder();
  g_settings.Save();
}

CGUIViewStateWindowVideoGenre::CGUIViewStateWindowVideoGenre(const CFileItemList& items) : CGUIViewStateWindowVideo(items)
{
  if (items.IsVirtualDirectoryRoot())
  {
    AddSortMethod(SORT_METHOD_LABEL, 103, LABEL_MASKS("%F", "", "%L", ""));  // Filename, Empty | Foldername, empty
    SetSortMethod(g_stSettings.m_MyVideoGenreRootSortMethod);

    AddViewAsControl(VIEW_METHOD_LIST, 101);
    AddViewAsControl(VIEW_METHOD_ICONS, 100);
    AddViewAsControl(VIEW_METHOD_LARGE_ICONS, 417);
    SetViewAsControl(g_stSettings.m_MyVideoGenreRootViewMethod);

    SetSortOrder(g_stSettings.m_MyVideoGenreRootSortOrder);
  }
  else
  {
    if (g_guiSettings.GetBool("filelists.ignorethewhensorting"))
      AddSortMethod(SORT_METHOD_LABEL_IGNORE_THE, 103, LABEL_MASKS("%K", "%R", "%L", ""));  // Titel, Rating | Foldername, empty
    else
      AddSortMethod(SORT_METHOD_LABEL, 103, LABEL_MASKS("%K", "%R", "%L", ""));  // Titel, Rating | Foldername, empty
    AddSortMethod(SORT_METHOD_VIDEO_YEAR, 366, LABEL_MASKS("%K", "%Y", "%L", ""));  // Titel, Year | Foldername, empty
    AddSortMethod(SORT_METHOD_VIDEO_RATING, 367, LABEL_MASKS("%K", "%R", "%L", ""));  // Titel, Rating | Foldername, empty
    SetSortMethod(g_stSettings.m_MyVideoGenreSortMethod);

    AddViewAsControl(VIEW_METHOD_LIST, 101);
    AddViewAsControl(VIEW_METHOD_ICONS, 100);
    AddViewAsControl(VIEW_METHOD_LARGE_ICONS, 417);
    SetViewAsControl(g_stSettings.m_MyVideoGenreViewMethod);

    SetSortOrder(g_stSettings.m_MyVideoGenreSortOrder);
  }
}

void CGUIViewStateWindowVideoGenre::SaveViewState()
{
  if (m_items.IsVirtualDirectoryRoot())
  {
    g_stSettings.m_MyVideoGenreRootSortMethod=GetSortMethod();
    g_stSettings.m_MyVideoGenreRootViewMethod=GetViewAsControl();
    g_stSettings.m_MyVideoGenreRootSortOrder=GetSortOrder();
  }
  else
  {
    g_stSettings.m_MyVideoGenreSortMethod=GetSortMethod();
    g_stSettings.m_MyVideoGenreViewMethod=GetViewAsControl();
    g_stSettings.m_MyVideoGenreSortOrder=GetSortOrder();
  }
  g_settings.Save();
}

CGUIViewStateWindowVideoActor::CGUIViewStateWindowVideoActor(const CFileItemList& items) : CGUIViewStateWindowVideo(items)
{
  if (items.IsVirtualDirectoryRoot())
  {
    AddSortMethod(SORT_METHOD_LABEL, 103, LABEL_MASKS("%F", "", "%L", ""));  // FileName, empty | Foldername, empty
    SetSortMethod(g_stSettings.m_MyVideoActorRootSortMethod);

    AddViewAsControl(VIEW_METHOD_LIST, 101);
    AddViewAsControl(VIEW_METHOD_ICONS, 100);
    AddViewAsControl(VIEW_METHOD_LARGE_ICONS, 417);
    SetViewAsControl(g_stSettings.m_MyVideoActorRootViewMethod);

    SetSortOrder(g_stSettings.m_MyVideoActorRootSortOrder);
  }
  else
  {
    if (g_guiSettings.GetBool("filelists.ignorethewhensorting"))
      AddSortMethod(SORT_METHOD_LABEL_IGNORE_THE, 103, LABEL_MASKS("%K", "%R", "%L", ""));  // Titel, Rating | Foldername, empty
    else
      AddSortMethod(SORT_METHOD_LABEL, 103, LABEL_MASKS("%K", "%R", "%L", ""));  // Titel, Rating | Foldername, empty
    AddSortMethod(SORT_METHOD_VIDEO_YEAR, 366, LABEL_MASKS("%K", "%Y", "%L", ""));  // Titel, Year | Foldername, empty
    AddSortMethod(SORT_METHOD_VIDEO_RATING, 367, LABEL_MASKS("%K", "%R", "%L", ""));  // Titel, Rating | Foldername, empty
    SetSortMethod(g_stSettings.m_MyVideoActorSortMethod);

    AddViewAsControl(VIEW_METHOD_LIST, 101);
    AddViewAsControl(VIEW_METHOD_ICONS, 100);
    AddViewAsControl(VIEW_METHOD_LARGE_ICONS, 417);
    SetViewAsControl(g_stSettings.m_MyVideoActorViewMethod);

    SetSortOrder(g_stSettings.m_MyVideoActorSortOrder);
  }
}

void CGUIViewStateWindowVideoActor::SaveViewState()
{
  if (m_items.IsVirtualDirectoryRoot())
  {
    g_stSettings.m_MyVideoActorRootSortMethod=GetSortMethod();
    g_stSettings.m_MyVideoActorRootViewMethod=GetViewAsControl();
    g_stSettings.m_MyVideoActorRootSortOrder=GetSortOrder();
  }
  else
  {
    g_stSettings.m_MyVideoActorSortMethod=GetSortMethod();
    g_stSettings.m_MyVideoActorViewMethod=GetViewAsControl();
    g_stSettings.m_MyVideoActorSortOrder=GetSortOrder();
  }
  g_settings.Save();
}

CGUIViewStateWindowVideoYear::CGUIViewStateWindowVideoYear(const CFileItemList& items) : CGUIViewStateWindowVideo(items)
{
  if (items.IsVirtualDirectoryRoot())
  {
    AddSortMethod(SORT_METHOD_LABEL, 103, LABEL_MASKS("%F", "", "%L", ""));  // Filename, empty | Foldername, empty
    SetSortMethod(g_stSettings.m_MyVideoYearRootSortMethod);

    AddViewAsControl(VIEW_METHOD_LIST, 101);
    AddViewAsControl(VIEW_METHOD_ICONS, 100);
    AddViewAsControl(VIEW_METHOD_LARGE_ICONS, 417);
    SetViewAsControl(g_stSettings.m_MyVideoYearRootViewMethod);

    SetSortOrder(g_stSettings.m_MyVideoYearRootSortOrder);
  }
  else
  {
    if (g_guiSettings.GetBool("filelists.ignorethewhensorting"))
      AddSortMethod(SORT_METHOD_LABEL_IGNORE_THE, 103, LABEL_MASKS("%K", "%R", "%L", ""));  // Titel, Rating | Foldername, empty
    else
      AddSortMethod(SORT_METHOD_LABEL, 103, LABEL_MASKS("%K", "%R", "%L", ""));  // Titel, Rating | Foldername, empty
    AddSortMethod(SORT_METHOD_VIDEO_YEAR, 366, LABEL_MASKS("%K", "%Y", "%L", ""));  // Titel, Year | Foldername, empty
    AddSortMethod(SORT_METHOD_VIDEO_RATING, 367, LABEL_MASKS("%K", "%R", "%L", ""));  // Titel, Rating | Foldername, empty
    SetSortMethod(g_stSettings.m_MyVideoYearSortMethod);

    AddViewAsControl(VIEW_METHOD_LIST, 101);
    AddViewAsControl(VIEW_METHOD_ICONS, 100);
    AddViewAsControl(VIEW_METHOD_LARGE_ICONS, 417);
    SetViewAsControl(g_stSettings.m_MyVideoYearViewMethod);

    SetSortOrder(g_stSettings.m_MyVideoYearSortOrder);
  }
}

void CGUIViewStateWindowVideoYear::SaveViewState()
{
  if (m_items.IsVirtualDirectoryRoot())
  {
    g_stSettings.m_MyVideoYearRootSortMethod=GetSortMethod();
    g_stSettings.m_MyVideoYearRootViewMethod=GetViewAsControl();
    g_stSettings.m_MyVideoYearRootSortOrder=GetSortOrder();
  }
  else
  {
    g_stSettings.m_MyVideoYearSortMethod=GetSortMethod();
    g_stSettings.m_MyVideoYearViewMethod=GetViewAsControl();
    g_stSettings.m_MyVideoYearSortOrder=GetSortOrder();
  }
  g_settings.Save();
}

CGUIViewStateWindowVideoPlaylist::CGUIViewStateWindowVideoPlaylist(const CFileItemList& items) : CGUIViewStateWindowVideo(items)
{
  AddSortMethod(SORT_METHOD_NONE, 103, LABEL_MASKS("%L", "", "%L", ""));  // Label, "" | Label, empty
  SetSortMethod(SORT_METHOD_NONE);

  AddViewAsControl(VIEW_METHOD_LIST, 101);
  AddViewAsControl(VIEW_METHOD_ICONS, 100);
  AddViewAsControl(VIEW_METHOD_LARGE_ICONS, 417);
  SetViewAsControl(g_stSettings.m_MyVideoPlaylistViewMethod);

  SetSortOrder(SORT_ORDER_NONE);
}

void CGUIViewStateWindowVideoPlaylist::SaveViewState()
{
  g_stSettings.m_MyVideoPlaylistViewMethod=GetViewAsControl();
  g_settings.Save();
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
  share.strPath="playlistvideo://";
  share.m_strThumbnailImage="defaultFolderBig.png";
  share.m_iDriveType = SHARE_TYPE_LOCAL;
  m_shares.push_back(share);

  return CGUIViewStateWindowVideo::GetShares();
}
