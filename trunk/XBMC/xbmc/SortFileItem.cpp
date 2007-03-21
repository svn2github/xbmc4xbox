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
#include "SortFileItem.h"

inline int StartsWithToken(const CStdString& strLabel)
{
  for (unsigned int i=0;i<g_advancedSettings.m_vecTokens.size();++i)
  {
    if (g_advancedSettings.m_vecTokens[i].size() < strLabel.size() && 
        strnicmp(g_advancedSettings.m_vecTokens[i].c_str(), strLabel.c_str(), g_advancedSettings.m_vecTokens[i].size()) == 0)
      return g_advancedSettings.m_vecTokens[i].size();
  }
  return 0;
}

// TODO:
// 1. See if the special case stuff can be moved out.  Problems are that you
//    have to keep the parent folder item separate from all other items in order
//    to guarantee correct sort order.
//
bool SSortFileItem::FileAscending(CFileItem *left, CFileItem *right)
{
  // ignore the ".." item - that should always be on top
  if (left->IsParentFolder()) return true;
  if (right->IsParentFolder()) return false;
  // only if they're both folders or both files do we do the full comparison
  if (left->m_bIsFolder == right->m_bIsFolder)
  {
    int result = StringUtils::AlphaNumericCompare(left->m_strPath.c_str(), right->m_strPath.c_str());
    if (result < 0) return true;
    if (result > 0) return false;
    return left->m_lStartOffset <= right->m_lStartOffset;
  }
  return left->m_bIsFolder;
}

bool SSortFileItem::FileDescending(CFileItem *left, CFileItem *right)
{
  // ignore the ".." item - that should always be on top
  if (left->IsParentFolder()) return true;
  if (right->IsParentFolder()) return false;
  // only if they're both folders or both files do we do the full comparison
  if (left->m_bIsFolder == right->m_bIsFolder)
  {
    int result = StringUtils::AlphaNumericCompare(left->m_strPath.c_str(), right->m_strPath.c_str());
    if (result < 0) return false;
    if (result > 0) return true;
    return left->m_lStartOffset >= right->m_lStartOffset;
  }
  return left->m_bIsFolder;
}

bool SSortFileItem::SizeAscending(CFileItem *left, CFileItem *right)
{
  // ignore the ".." item - that should always be on top
  if (left->IsParentFolder()) return true;
  if (right->IsParentFolder()) return false;
  // only if they're both folders or both files do we do the full comparison
  if (left->m_bIsFolder == right->m_bIsFolder)
    return left->m_dwSize <= right->m_dwSize;
  return left->m_bIsFolder;
}

bool SSortFileItem::SizeDescending(CFileItem *left, CFileItem *right)
{
  // ignore the ".." item - that should always be on top
  if (left->IsParentFolder()) return true;
  if (right->IsParentFolder()) return false;
  // only if they're both folders or both files do we do the full comparison
  if (left->m_bIsFolder == right->m_bIsFolder)
    return left->m_dwSize >= right->m_dwSize;
  return left->m_bIsFolder;
}

bool SSortFileItem::DateAscending(CFileItem *left, CFileItem *right)
{
  // ignore the ".." item - that should always be on top
  if (left->IsParentFolder()) return true;
  if (right->IsParentFolder()) return false;
  if (left->m_bIsFolder == right->m_bIsFolder)
  { // same category
    if ( left->m_dateTime < right->m_dateTime ) return true;
    if ( left->m_dateTime > right->m_dateTime ) return false;
    // dates are the same, sort by label in reverse (as default sort
    // method is descending for date, and ascending for label)
    return StringUtils::AlphaNumericCompare(left->GetLabel().c_str(), right->GetLabel().c_str()) > 0;
  }
  return left->m_bIsFolder;
}

bool SSortFileItem::DateDescending(CFileItem *left, CFileItem *right)
{
  // ignore the ".." item - that should always be on top
  if (left->IsParentFolder()) return true;
  if (right->IsParentFolder()) return false;
  if (left->m_bIsFolder == right->m_bIsFolder)
  { // same category
    if ( left->m_dateTime < right->m_dateTime ) return false;
    if ( left->m_dateTime > right->m_dateTime ) return true;
    // dates are the same, sort by label in reverse (as default sort
    // method is descending for date, and ascending for label)
    return StringUtils::AlphaNumericCompare(left->GetLabel().c_str(), right->GetLabel().c_str()) < 0;
  }
  return left->m_bIsFolder;
}

bool SSortFileItem::DriveTypeAscending(CFileItem *left, CFileItem *right)
{
  // ignore the ".." item - that should always be on top
  if (left->IsParentFolder()) return true;
  if (right->IsParentFolder()) return false;
  if (left->m_bIsFolder == right->m_bIsFolder)
  { // same category
    if ( left->m_iDriveType < right->m_iDriveType ) return true;
    if ( left->m_iDriveType > right->m_iDriveType ) return false;
    return StringUtils::AlphaNumericCompare(left->GetLabel().c_str(), right->GetLabel().c_str()) <= 0;
  }
  return left->m_bIsFolder;
}

bool SSortFileItem::DriveTypeDescending(CFileItem *left, CFileItem *right)
{
  // ignore the ".." item - that should always be on top
  if (left->IsParentFolder()) return true;
  if (right->IsParentFolder()) return false;
  if (left->m_bIsFolder == right->m_bIsFolder)
  { // same category
    if ( left->m_iDriveType < right->m_iDriveType ) return false;
    if ( left->m_iDriveType > right->m_iDriveType ) return true;
    return StringUtils::AlphaNumericCompare(left->GetLabel().c_str(), right->GetLabel().c_str()) >= 0;
  }
  return left->m_bIsFolder;
}

bool SSortFileItem::LabelAscending(CFileItem *left, CFileItem *right)
{
  // special items
  if (left->IsParentFolder()) return true;
  if (right->IsParentFolder()) return false;
  // only if they're both folders or both files do we do the full comparison
  if (left->m_bIsFolder == right->m_bIsFolder)
    return StringUtils::AlphaNumericCompare(left->GetLabel().c_str(),right->GetLabel().c_str()) <= 0;
  return left->m_bIsFolder;
}

bool SSortFileItem::LabelDescending(CFileItem *left, CFileItem *right)
{
  // special items
  if (left->IsParentFolder()) return true;
  if (right->IsParentFolder()) return false;
  // only if they're both folders or both files do we do the full comparison
  if (left->m_bIsFolder == right->m_bIsFolder)
    return StringUtils::AlphaNumericCompare(left->GetLabel().c_str(),right->GetLabel().c_str()) >= 0;
  return left->m_bIsFolder;
}

bool SSortFileItem::LabelAscendingNoThe(CFileItem *left, CFileItem *right)
{
  // special items
  if (left->IsParentFolder()) return true;
  if (right->IsParentFolder()) return false;
  // only if they're both folders or both files do we do the full comparison
  if (left->m_bIsFolder == right->m_bIsFolder)
  {
    char *l = (char *)left->GetLabel().c_str();
    char *r = (char *)right->GetLabel().c_str();
    l += StartsWithToken(left->GetLabel());
    r += StartsWithToken(right->GetLabel());

    return StringUtils::AlphaNumericCompare(l, r) <= 0;
  }
  return left->m_bIsFolder;
}

bool SSortFileItem::LabelDescendingNoThe(CFileItem *left, CFileItem *right)
{
  // special items
  if (left->IsParentFolder()) return true;
  if (right->IsParentFolder()) return false;
  // only if they're both folders or both files do we do the full comparison
  if (left->m_bIsFolder == right->m_bIsFolder)
  {
    char *l = (char *)left->GetLabel().c_str();
    char *r = (char *)right->GetLabel().c_str();
    l += StartsWithToken(left->GetLabel());
    r += StartsWithToken(right->GetLabel());

    return StringUtils::AlphaNumericCompare(l, r) >= 0;
  }
  return left->m_bIsFolder;
}

bool SSortFileItem::SongTrackNumAscending(CFileItem *left, CFileItem *right)
{
  // special items
  if (left->IsParentFolder()) return true;
  if (right->IsParentFolder()) return false;
  // only if they're both folders or both files do we do the full comparison
  if (left->m_bIsFolder == right->m_bIsFolder)
    return left->GetMusicInfoTag()->GetTrackAndDiskNumber() <= right->GetMusicInfoTag()->GetTrackAndDiskNumber();
  return left->m_bIsFolder;
}

bool SSortFileItem::SongTrackNumDescending(CFileItem *left, CFileItem *right)
{
  // special items
  if (left->IsParentFolder()) return true;
  if (right->IsParentFolder()) return false;
  // only if they're both folders or both files do we do the full comparison
  if (left->m_bIsFolder == right->m_bIsFolder)
    return left->GetMusicInfoTag()->GetTrackAndDiskNumber() >= right->GetMusicInfoTag()->GetTrackAndDiskNumber();
  return left->m_bIsFolder;
}

bool SSortFileItem::EpisodeNumAscending(CFileItem *left, CFileItem *right)
{
  // special items
  if (left->IsParentFolder()) return true;
  if (right->IsParentFolder()) return false;
  // only if they're both folders or both files do we do the full comparison
  if (left->m_bIsFolder == right->m_bIsFolder)
    return left->GetVideoInfoTag()->m_iEpisode <= right->GetVideoInfoTag()->m_iEpisode;
  return left->m_bIsFolder;
}

bool SSortFileItem::EpisodeNumDescending(CFileItem *left, CFileItem *right)
{
  // special items
  if (left->IsParentFolder()) return true;
  if (right->IsParentFolder()) return false;
  // only if they're both folders or both files do we do the full comparison
  if (left->m_bIsFolder == right->m_bIsFolder)
    return left->GetVideoInfoTag()->m_iEpisode >= right->GetVideoInfoTag()->m_iEpisode;
  return left->m_bIsFolder;
}

bool SSortFileItem::SongDurationAscending(CFileItem *left, CFileItem *right)
{
  // special items
  if (left->IsParentFolder()) return true;
  if (right->IsParentFolder()) return false;
  // only if they're both folders or both files do we do the full comparison
  if (left->m_bIsFolder == right->m_bIsFolder)
    return left->GetMusicInfoTag()->GetDuration() <= right->GetMusicInfoTag()->GetDuration();
  return left->m_bIsFolder;
}

bool SSortFileItem::SongDurationDescending(CFileItem *left, CFileItem *right)
{
  // special items
  if (left->IsParentFolder()) return true;
  if (right->IsParentFolder()) return false;
  // only if they're both folders or both files do we do the full comparison
  if (left->m_bIsFolder == right->m_bIsFolder)
    return left->GetMusicInfoTag()->GetDuration() >= right->GetMusicInfoTag()->GetDuration();
  return left->m_bIsFolder;
}

bool SSortFileItem::SongTitleAscending(CFileItem *left, CFileItem *right)
{
  // special cases
  if (left->IsParentFolder()) return true;
  if (right->IsParentFolder()) return false;
  // only if they're both folders or both files do we do the full comparison
  if (left->m_bIsFolder == right->m_bIsFolder)
  {
    char *l = (char *)left->GetMusicInfoTag()->GetTitle().c_str();
    char *r = (char *)right->GetMusicInfoTag()->GetTitle().c_str();
    return StringUtils::AlphaNumericCompare(l, r) <= 0;
  }
  return left->m_bIsFolder;
}

bool SSortFileItem::SongTitleDescending(CFileItem *left, CFileItem *right)
{
  // special cases
  if (left->IsParentFolder()) return true;
  if (right->IsParentFolder()) return false;
  // only if they're both folders or both files do we do the full comparison
  if (left->m_bIsFolder == right->m_bIsFolder)
  {
    char *l = (char *)left->GetMusicInfoTag()->GetTitle().c_str();
    char *r = (char *)right->GetMusicInfoTag()->GetTitle().c_str();
    return StringUtils::AlphaNumericCompare(l, r) >= 0;
  }
  return left->m_bIsFolder;
}

bool SSortFileItem::MovieTitleAscending(CFileItem *left, CFileItem *right)
{
  // special cases
  if (left->IsParentFolder()) return true;
  if (right->IsParentFolder()) return false;
  // only if they're both folders or both files do we do the full comparison
  if (left->m_bIsFolder == right->m_bIsFolder)
  {
    char *l = (char *)left->GetVideoInfoTag()->m_strTitle.c_str();
    char *r = (char *)right->GetVideoInfoTag()->m_strTitle.c_str();
    return StringUtils::AlphaNumericCompare(l, r) <= 0;
  }
  return left->m_bIsFolder;
}

bool SSortFileItem::MovieTitleDescending(CFileItem *left, CFileItem *right)
{
  // special cases
  if (left->IsParentFolder()) return true;
  if (right->IsParentFolder()) return false;
  // only if they're both folders or both files do we do the full comparison
  if (left->m_bIsFolder == right->m_bIsFolder)
  {
    char *l = (char *)left->GetVideoInfoTag()->m_strTitle.c_str();
    char *r = (char *)right->GetVideoInfoTag()->m_strTitle.c_str();
    return StringUtils::AlphaNumericCompare(l, r) >= 0;
  }
  return left->m_bIsFolder;
}

bool SSortFileItem::SongTitleAscendingNoThe(CFileItem *left, CFileItem *right)
{
  // special cases
  if (left->IsParentFolder()) return true;
  if (right->IsParentFolder()) return false;
  // only if they're both folders or both files do we do the full comparison
  if (left->m_bIsFolder == right->m_bIsFolder)
  {
    char *l = (char *)left->GetMusicInfoTag()->GetTitle().c_str();
    char *r = (char *)right->GetMusicInfoTag()->GetTitle().c_str();
    l += StartsWithToken(left->GetMusicInfoTag()->GetTitle());
    r += StartsWithToken(right->GetMusicInfoTag()->GetTitle());
    
    return StringUtils::AlphaNumericCompare(l, r) <= 0;
  }
  return left->m_bIsFolder;
}

bool SSortFileItem::SongTitleDescendingNoThe(CFileItem *left, CFileItem *right)
{
  // special cases
  if (left->IsParentFolder()) return true;
  if (right->IsParentFolder()) return false;
  // only if they're both folders or both files do we do the full comparison
  if (left->m_bIsFolder == right->m_bIsFolder)
  {
    char *l = (char *)left->GetMusicInfoTag()->GetTitle().c_str();
    char *r = (char *)right->GetMusicInfoTag()->GetTitle().c_str();
    l += StartsWithToken(left->GetMusicInfoTag()->GetTitle());
    r += StartsWithToken(right->GetMusicInfoTag()->GetTitle());
    
    return StringUtils::AlphaNumericCompare(l, r) >= 0;
  }
  return left->m_bIsFolder;
}

bool SSortFileItem::SongArtistAscending(CFileItem *left, CFileItem *right)
{
  // special cases
  if (left->IsParentFolder()) return true;
  if (right->IsParentFolder()) return false;
  // only if they're both folders or both files do we do the full comparison
  if (left->m_bIsFolder == right->m_bIsFolder)
  {
    char *l = (char *)left->GetMusicInfoTag()->GetArtist().c_str();
    char *r = (char *)right->GetMusicInfoTag()->GetArtist().c_str();
    int result = StringUtils::AlphaNumericCompare(l, r);
    if (result < 0) return true;
    if (result > 0) return false;
    // test year
    if (g_advancedSettings.m_bMusicLibraryAlbumsSortByArtistThenYear)
    {
      result = StringUtils::AlphaNumericCompare(left->GetMusicInfoTag()->GetYear().c_str(), right->GetMusicInfoTag()->GetYear().c_str());
      if (result < 0) return true;
      if (result > 0) return false;
    }
    // artists agree, test the album
    l = (char *)left->GetMusicInfoTag()->GetAlbum().c_str();
    r = (char *)right->GetMusicInfoTag()->GetAlbum().c_str();
    result = StringUtils::AlphaNumericCompare(l, r);
    if (result < 0) return true;
    if (result > 0) return false;
    // artist and album agree, test the track number
    return left->GetMusicInfoTag()->GetTrackAndDiskNumber() <= right->GetMusicInfoTag()->GetTrackAndDiskNumber();
  }
  return left->m_bIsFolder;
}

bool SSortFileItem::SongArtistDescending(CFileItem *left, CFileItem *right)
{
  // special cases
  if (left->IsParentFolder()) return true;
  if (right->IsParentFolder()) return false;
  // only if they're both folders or both files do we do the full comparison
  if (left->m_bIsFolder == right->m_bIsFolder)
  {
    char *l = (char *)left->GetMusicInfoTag()->GetArtist().c_str();
    char *r = (char *)right->GetMusicInfoTag()->GetArtist().c_str();
    int result = StringUtils::AlphaNumericCompare(l, r);
    if (result < 0) return false;
    if (result > 0) return true;
    // test year
    if (g_advancedSettings.m_bMusicLibraryAlbumsSortByArtistThenYear)
    {
      result = StringUtils::AlphaNumericCompare(left->GetMusicInfoTag()->GetYear().c_str(), right->GetMusicInfoTag()->GetYear().c_str());
      if (result < 0) return false;
      if (result > 0) return true;
    }
    // artists agree, test the album
    l = (char *)left->GetMusicInfoTag()->GetAlbum().c_str();
    r = (char *)right->GetMusicInfoTag()->GetAlbum().c_str();
    result = StringUtils::AlphaNumericCompare(l, r);
    if (result < 0) return false;
    if (result > 0) return true;
    // artist and album agree, test the track number
    return left->GetMusicInfoTag()->GetTrackAndDiskNumber() >= right->GetMusicInfoTag()->GetTrackAndDiskNumber();
  }
  return left->m_bIsFolder;
}

bool SSortFileItem::SongArtistAscendingNoThe(CFileItem *left, CFileItem *right)
{
  // special cases
  if (left->IsParentFolder()) return true;
  if (right->IsParentFolder()) return false;
  // only if they're both folders or both files do we do the full comparison
  if (left->m_bIsFolder == right->m_bIsFolder)
  {
    char *l = (char *)left->GetMusicInfoTag()->GetArtist().c_str();
    char *r = (char *)right->GetMusicInfoTag()->GetArtist().c_str();
    l += StartsWithToken(left->GetMusicInfoTag()->GetArtist());
    r += StartsWithToken(right->GetMusicInfoTag()->GetArtist());

    int result = StringUtils::AlphaNumericCompare(l, r);
    if (result < 0) return true;
    if (result > 0) return false;
    // test year
    if (g_advancedSettings.m_bMusicLibraryAlbumsSortByArtistThenYear)
    {
      result = StringUtils::AlphaNumericCompare(left->GetMusicInfoTag()->GetYear().c_str(), right->GetMusicInfoTag()->GetYear().c_str());
      if (result < 0) return true;
      if (result > 0) return false;
    }
    // artists agree, test the album
    l = (char *)left->GetMusicInfoTag()->GetAlbum().c_str();
    r = (char *)right->GetMusicInfoTag()->GetAlbum().c_str();
    l += StartsWithToken(left->GetMusicInfoTag()->GetAlbum());
    r += StartsWithToken(right->GetMusicInfoTag()->GetAlbum());

    result = StringUtils::AlphaNumericCompare(l, r);
    if (result < 0) return true;
    if (result > 0) return false;
    // artist and album agree, test the track number
    return left->GetMusicInfoTag()->GetTrackAndDiskNumber() <= right->GetMusicInfoTag()->GetTrackAndDiskNumber();
  }
  return left->m_bIsFolder;
}

bool SSortFileItem::SongArtistDescendingNoThe(CFileItem *left, CFileItem *right)
{
  // special cases
  if (left->IsParentFolder()) return true;
  if (right->IsParentFolder()) return false;
  // only if they're both folders or both files do we do the full comparison
  if (left->m_bIsFolder == right->m_bIsFolder)
  {
    char *l = (char *)left->GetMusicInfoTag()->GetArtist().c_str();
    char *r = (char *)right->GetMusicInfoTag()->GetArtist().c_str();
    l += StartsWithToken(left->GetMusicInfoTag()->GetArtist());
    r += StartsWithToken(right->GetMusicInfoTag()->GetArtist());

    int result = StringUtils::AlphaNumericCompare(l, r);
    if (result < 0) return false;
    if (result > 0) return true;
    // test year
    if (g_advancedSettings.m_bMusicLibraryAlbumsSortByArtistThenYear)
    {
      result = StringUtils::AlphaNumericCompare(left->GetMusicInfoTag()->GetYear().c_str(), right->GetMusicInfoTag()->GetYear().c_str());
      if (result < 0) return false;
      if (result > 0) return true;
    }
    // artists agree, test the album
    l = (char *)left->GetMusicInfoTag()->GetAlbum().c_str();
    r = (char *)right->GetMusicInfoTag()->GetAlbum().c_str();
    l += StartsWithToken(left->GetMusicInfoTag()->GetAlbum());
    r += StartsWithToken(right->GetMusicInfoTag()->GetAlbum());

    result = StringUtils::AlphaNumericCompare(l, r);
    if (result < 0) return false;
    if (result > 0) return true;
    // artist and album agree, test the track number
    return left->GetMusicInfoTag()->GetTrackAndDiskNumber() >= right->GetMusicInfoTag()->GetTrackAndDiskNumber();
  }
  return left->m_bIsFolder;
}

bool SSortFileItem::SongAlbumAscending(CFileItem *left, CFileItem *right)
{
  // special cases
  if (left->IsParentFolder()) return true;
  if (right->IsParentFolder()) return false;
  // only if they're both folders or both files do we do the full comparison
  if (left->m_bIsFolder == right->m_bIsFolder)
  {
    char *l = (char *)left->GetMusicInfoTag()->GetAlbum().c_str();
    char *r = (char *)right->GetMusicInfoTag()->GetAlbum().c_str();
    int result = StringUtils::AlphaNumericCompare(l, r);
    if (result < 0) return true;
    if (result > 0) return false;
    // album names match, try the artist
    l = (char *)left->GetMusicInfoTag()->GetArtist().c_str();
    r = (char *)right->GetMusicInfoTag()->GetArtist().c_str();
    result = StringUtils::AlphaNumericCompare(l, r);
    if (result < 0) return true;
    if (result > 0) return false;
    // album and artist match - sort by track
    return left->GetMusicInfoTag()->GetTrackAndDiskNumber() <= right->GetMusicInfoTag()->GetTrackAndDiskNumber();
  }
  return left->m_bIsFolder;
}

bool SSortFileItem::SongAlbumDescending(CFileItem *left, CFileItem *right)
{
  // special cases
  if (left->IsParentFolder()) return true;
  if (right->IsParentFolder()) return false;
  // only if they're both folders or both files do we do the full comparison
  if (left->m_bIsFolder == right->m_bIsFolder)
  {
    char *l = (char *)left->GetMusicInfoTag()->GetAlbum().c_str();
    char *r = (char *)right->GetMusicInfoTag()->GetAlbum().c_str();
    int result = StringUtils::AlphaNumericCompare(l, r);
    if (result < 0) return false;
    if (result > 0) return true;
    // album names match, try the artist
    l = (char *)left->GetMusicInfoTag()->GetArtist().c_str();
    r = (char *)right->GetMusicInfoTag()->GetArtist().c_str();
    result = StringUtils::AlphaNumericCompare(l, r);
    if (result < 0) return false;
    if (result > 0) return true;
    // album and artist match - sort by track
    return left->GetMusicInfoTag()->GetTrackAndDiskNumber() >= right->GetMusicInfoTag()->GetTrackAndDiskNumber();
  }
  return left->m_bIsFolder;
}

bool SSortFileItem::SongAlbumAscendingNoThe(CFileItem *left, CFileItem *right)
{
  // special cases
  if (left->IsParentFolder()) return true;
  if (right->IsParentFolder()) return false;
  // only if they're both folders or both files do we do the full comparison
  if (left->m_bIsFolder == right->m_bIsFolder)
  {
    char *l = (char *)left->GetMusicInfoTag()->GetAlbum().c_str();
    char *r = (char *)right->GetMusicInfoTag()->GetAlbum().c_str();
    l += StartsWithToken(left->GetMusicInfoTag()->GetAlbum());
    r += StartsWithToken(right->GetMusicInfoTag()->GetAlbum());

    int result = StringUtils::AlphaNumericCompare(l, r);
    if (result < 0) return true;
    if (result > 0) return false;
    // album names match, try the artist
    l = (char *)left->GetMusicInfoTag()->GetArtist().c_str();
    r = (char *)right->GetMusicInfoTag()->GetArtist().c_str();
    l += StartsWithToken(left->GetMusicInfoTag()->GetArtist());
    r += StartsWithToken(right->GetMusicInfoTag()->GetArtist());

    result = StringUtils::AlphaNumericCompare(l, r);
    if (result < 0) return true;
    if (result > 0) return false;
    // album and artist match - sort by track
    return left->GetMusicInfoTag()->GetTrackAndDiskNumber() <= right->GetMusicInfoTag()->GetTrackAndDiskNumber();
  }
  return left->m_bIsFolder;
}

bool SSortFileItem::SongAlbumDescendingNoThe(CFileItem *left, CFileItem *right)
{
  // special cases
  if (left->IsParentFolder()) return true;
  if (right->IsParentFolder()) return false;
  // only if they're both folders or both files do we do the full comparison
  if (left->m_bIsFolder == right->m_bIsFolder)
  {
    char *l = (char *)left->GetMusicInfoTag()->GetAlbum().c_str();
    char *r = (char *)right->GetMusicInfoTag()->GetAlbum().c_str();
    l += StartsWithToken(left->GetMusicInfoTag()->GetAlbum());
    r += StartsWithToken(right->GetMusicInfoTag()->GetAlbum());

    int result = StringUtils::AlphaNumericCompare(l, r);
    if (result < 0) return false;
    if (result > 0) return true;
    // album names match, try the artist
    l = (char *)left->GetMusicInfoTag()->GetArtist().c_str();
    r = (char *)right->GetMusicInfoTag()->GetArtist().c_str();
    l += StartsWithToken(left->GetMusicInfoTag()->GetArtist());
    r += StartsWithToken(right->GetMusicInfoTag()->GetArtist());

    result = StringUtils::AlphaNumericCompare(l, r);
    if (result < 0) return false;
    if (result > 0) return true;
    // album and artist match - sort by track
    return left->GetMusicInfoTag()->GetTrackAndDiskNumber() >= right->GetMusicInfoTag()->GetTrackAndDiskNumber();
  }
  return left->m_bIsFolder;
}

bool SSortFileItem::SongGenreAscending(CFileItem *left, CFileItem *right)
{
  // special items
  if (left->IsParentFolder()) return true;
  if (right->IsParentFolder()) return false;
  // only if they're both folders or both files do we do the full comparison
  if (left->m_bIsFolder == right->m_bIsFolder)
    return StringUtils::AlphaNumericCompare(left->GetMusicInfoTag()->GetGenre().c_str(),right->GetMusicInfoTag()->GetGenre().c_str()) <= 0;
  return left->m_bIsFolder;
}

bool SSortFileItem::SongGenreDescending(CFileItem *left, CFileItem *right)
{
  // special items
  if (left->IsParentFolder()) return true;
  if (right->IsParentFolder()) return false;
  // only if they're both folders or both files do we do the full comparison
  if (left->m_bIsFolder == right->m_bIsFolder)
    return StringUtils::AlphaNumericCompare(left->GetMusicInfoTag()->GetGenre().c_str(),right->GetMusicInfoTag()->GetGenre().c_str()) >= 0;
  return left->m_bIsFolder;
}

bool SSortFileItem::ProgramCountAscending(CFileItem *left, CFileItem *right)
{
  // ignore the ".." item - that should always be on top
  if (left->IsParentFolder()) return true;
  if (right->IsParentFolder()) return false;
  if (left->m_bIsFolder == right->m_bIsFolder)
    return left->m_iprogramCount <= right->m_iprogramCount;
  return left->m_bIsFolder;
}

bool SSortFileItem::ProgramCountDescending(CFileItem *left, CFileItem *right)
{
  // ignore the ".." item - that should always be on top
  if (left->IsParentFolder()) return true;
  if (right->IsParentFolder()) return false;
  if (left->m_bIsFolder == right->m_bIsFolder)
    return left->m_iprogramCount >= right->m_iprogramCount;
  return left->m_bIsFolder;
}

bool SSortFileItem::MovieYearAscending(CFileItem *left, CFileItem *right)
{
  // ignore the ".." item - that should always be on top
  if (left->IsParentFolder()) return true;
  if (right->IsParentFolder()) return false;
  if (left->m_bIsFolder == right->m_bIsFolder)
  {
    if (left->GetVideoInfoTag()->m_iYear > 0)
    {
      int result = left->GetVideoInfoTag()->m_iYear-right->GetVideoInfoTag()->m_iYear;
      if (result < 0) return true;
      if (result > 0) return false;
      return StringUtils::AlphaNumericCompare(left->GetLabel().c_str(), right->GetLabel().c_str()) <= 0;
    }
    if (!left->GetVideoInfoTag()->m_strPremiered.IsEmpty())
      return StringUtils::AlphaNumericCompare(left->GetVideoInfoTag()->m_strPremiered.c_str(), right->GetVideoInfoTag()->m_strPremiered.c_str()) <= 0;
    if (!left->GetVideoInfoTag()->m_strFirstAired.IsEmpty())
      return StringUtils::AlphaNumericCompare(left->GetVideoInfoTag()->m_strFirstAired.c_str(), right->GetVideoInfoTag()->m_strFirstAired.c_str()) <= 0;
  }
  return left->m_bIsFolder;
}

bool SSortFileItem::MovieYearDescending(CFileItem *left, CFileItem *right)
{
  // ignore the ".." item - that should always be on top
  if (left->IsParentFolder()) return true;
  if (right->IsParentFolder()) return false;
  if (left->m_bIsFolder == right->m_bIsFolder)
  {
    if (left->GetVideoInfoTag()->m_iYear > 0)
    {
     int result = left->GetVideoInfoTag()->m_iYear-right->GetVideoInfoTag()->m_iYear;
     if (result < 0) return false;
     if (result > 0) return true;
     return StringUtils::AlphaNumericCompare(left->GetLabel().c_str(), right->GetLabel().c_str()) >= 0;
    }
    if (!left->GetVideoInfoTag()->m_strPremiered.IsEmpty())
      return StringUtils::AlphaNumericCompare(left->GetVideoInfoTag()->m_strPremiered.c_str(), right->GetVideoInfoTag()->m_strPremiered.c_str()) >= 0;
    if (!left->GetVideoInfoTag()->m_strFirstAired.IsEmpty())
      return StringUtils::AlphaNumericCompare(left->GetVideoInfoTag()->m_strFirstAired.c_str(), right->GetVideoInfoTag()->m_strFirstAired.c_str()) >= 0;
  }
  return left->m_bIsFolder;
}

bool SSortFileItem::ProductionCodeAscending(CFileItem *left, CFileItem *right)
{
  // ignore the ".." item - that should always be on top
  if (left->IsParentFolder()) return true;
  if (right->IsParentFolder()) return false;
  if (left->m_bIsFolder == right->m_bIsFolder)
  {
    return StringUtils::AlphaNumericCompare(left->GetVideoInfoTag()->m_strProductionCode.c_str(),right->GetVideoInfoTag()->m_strProductionCode.c_str()) <= 0;
  }
  return left->m_bIsFolder;
}

bool SSortFileItem::ProductionCodeDescending(CFileItem *left, CFileItem *right)
{
  // ignore the ".." item - that should always be on top
  if (left->IsParentFolder()) return true;
  if (right->IsParentFolder()) return false;
  if (left->m_bIsFolder == right->m_bIsFolder)
  {
    return StringUtils::AlphaNumericCompare(left->GetVideoInfoTag()->m_strProductionCode.c_str(),right->GetVideoInfoTag()->m_strProductionCode.c_str()) >= 0;
  }
  return left->m_bIsFolder;
}

bool SSortFileItem::MovieRatingAscending(CFileItem *left, CFileItem *right)
{
  // ignore the ".." item - that should always be on top
  if (left->IsParentFolder()) return true;
  if (right->IsParentFolder()) return false;
  if (left->m_bIsFolder == right->m_bIsFolder)
  {
    if (left->m_fRating < right->m_fRating) return true;
    if (left->m_fRating > right->m_fRating) return false;
    return StringUtils::AlphaNumericCompare(left->GetLabel().c_str(), right->GetLabel().c_str()) <= 0;
  }
  return left->m_bIsFolder;
}

bool SSortFileItem::MovieRatingDescending(CFileItem *left, CFileItem *right)
{
  // ignore the ".." item - that should always be on top
  if (left->IsParentFolder()) return true;
  if (right->IsParentFolder()) return false;
  if (left->m_bIsFolder == right->m_bIsFolder)
  {
    if (left->m_fRating < right->m_fRating) return false;
    if (left->m_fRating > right->m_fRating) return true;
    return StringUtils::AlphaNumericCompare(left->GetLabel().c_str(), right->GetLabel().c_str()) >= 0;
  }
  return left->m_bIsFolder;
}
