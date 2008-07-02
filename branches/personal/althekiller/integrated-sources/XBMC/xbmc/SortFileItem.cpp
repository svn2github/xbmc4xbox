/*
 *      Copyright (C) 2005-2008 Team XBMC
 *      http://www.xbmc.org
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
 *  along with XBMC; see the file COPYING.  If not, write to
 *  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 *  http://www.gnu.org/copyleft/gpl.html
 *
 */

#include "stdafx.h"
#include "Settings.h"
#include "SortFileItem.h"
#include "VideoInfoTag.h"
#include "MusicInfoTag.h"
#include "FileItem.h"
#include "URL.h"
#include "utils/log.h"

#define RETURN_IF_NULL(x,y) if ((x) == NULL) { CLog::Log(LOGWARNING, "%s, sort item is null", __FUNCTION__); return y; }

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
bool SSortFileItem::FileAscending(const CFileItemPtr &left, const CFileItemPtr &right)
{
  // sanity
  RETURN_IF_NULL(left,false); RETURN_IF_NULL(right,false);

  // ignore the ".." item - that should always be on top
  if (left->IsParentFolder()) return true;
  if (right->IsParentFolder()) return false;
  CURL lurl(left->m_strPath); CURL rurl(right->m_strPath);
  int result = StringUtils::AlphaNumericCompare(lurl.GetFileNameWithoutPath().c_str(), rurl.GetFileNameWithoutPath().c_str());
  if (result < 0) return true;
  if (result > 0) return false;
  return left->m_lStartOffset < right->m_lStartOffset; // useful for .cue's in my music
}

bool SSortFileItem::FileDescending(const CFileItemPtr &left, const CFileItemPtr &right)
{
  // sanity
  RETURN_IF_NULL(left,false); RETURN_IF_NULL(right,false);

  // ignore the ".." item - that should always be on top
  if (left->IsParentFolder()) return true;
  if (right->IsParentFolder()) return false;
  CURL lurl(left->m_strPath); CURL rurl(right->m_strPath);
  int result = StringUtils::AlphaNumericCompare(lurl.GetFileNameWithoutPath().c_str(), rurl.GetFileNameWithoutPath().c_str());
  if (result < 0) return false;
  if (result > 0) return true;
  return left->m_lStartOffset > right->m_lStartOffset;
}

bool SSortFileItem::SizeAscending(const CFileItemPtr &left, const CFileItemPtr &right)
{
  // sanity
  RETURN_IF_NULL(left,false); RETURN_IF_NULL(right,false);

  // ignore the ".." item - that should always be on top
  if (left->IsParentFolder()) return true;
  if (right->IsParentFolder()) return false;
  // only if they're both folders or both files do we do the full comparison
  if (left->m_bIsFolder == right->m_bIsFolder)
    return left->m_dwSize < right->m_dwSize;
  return left->m_bIsFolder;
}

bool SSortFileItem::SizeDescending(const CFileItemPtr &left, const CFileItemPtr &right)
{
  // sanity
  RETURN_IF_NULL(left,false); RETURN_IF_NULL(right,false);

  // ignore the ".." item - that should always be on top
  if (left->IsParentFolder()) return true;
  if (right->IsParentFolder()) return false;
  // only if they're both folders or both files do we do the full comparison
  if (left->m_bIsFolder == right->m_bIsFolder)
    return left->m_dwSize > right->m_dwSize;
  return left->m_bIsFolder;
}

bool SSortFileItem::DateAscending(const CFileItemPtr &left, const CFileItemPtr &right)
{
  // sanity
  RETURN_IF_NULL(left,false); RETURN_IF_NULL(right,false);

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

bool SSortFileItem::DateDescending(const CFileItemPtr &left, const CFileItemPtr &right)
{
  // sanity
  RETURN_IF_NULL(left,false); RETURN_IF_NULL(right,false);

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

bool SSortFileItem::DriveTypeAscending(const CFileItemPtr &left, const CFileItemPtr &right)
{
  // sanity
  RETURN_IF_NULL(left,false); RETURN_IF_NULL(right,false);

  // ignore the ".." item - that should always be on top
  if (left->IsParentFolder()) return true;
  if (right->IsParentFolder()) return false;
  if (left->m_bIsFolder == right->m_bIsFolder)
  { // same category
    if ( left->m_iDriveType < right->m_iDriveType ) return true;
    if ( left->m_iDriveType > right->m_iDriveType ) return false;
    return StringUtils::AlphaNumericCompare(left->GetLabel().c_str(), right->GetLabel().c_str()) < 0;
  }
  return left->m_bIsFolder;
}

bool SSortFileItem::DriveTypeDescending(const CFileItemPtr &left, const CFileItemPtr &right)
{
  // sanity
  RETURN_IF_NULL(left,false); RETURN_IF_NULL(right,false);

  // ignore the ".." item - that should always be on top
  if (left->IsParentFolder()) return true;
  if (right->IsParentFolder()) return false;
  if (left->m_bIsFolder == right->m_bIsFolder)
  { // same category
    if ( left->m_iDriveType < right->m_iDriveType ) return false;
    if ( left->m_iDriveType > right->m_iDriveType ) return true;
    return StringUtils::AlphaNumericCompare(left->GetLabel().c_str(), right->GetLabel().c_str()) > 0;
  }
  return left->m_bIsFolder;
}

bool SSortFileItem::LabelAscending(const CFileItemPtr &left, const CFileItemPtr &right)
{
  // sanity
  RETURN_IF_NULL(left,false); RETURN_IF_NULL(right,false);

  // special items
  if (left->IsParentFolder()) return true;
  if (right->IsParentFolder()) return false;
  // only if they're both folders or both files do we do the full comparison
  if (left->m_bIsFolder == right->m_bIsFolder)
    return StringUtils::AlphaNumericCompare(left->GetLabel().c_str(),right->GetLabel().c_str()) < 0;
  return left->m_bIsFolder;
}

bool SSortFileItem::LabelDescending(const CFileItemPtr &left, const CFileItemPtr &right)
{
  // sanity
  RETURN_IF_NULL(left,false); RETURN_IF_NULL(right,false);

  // special items
  if (left->IsParentFolder()) return true;
  if (right->IsParentFolder()) return false;
  // only if they're both folders or both files do we do the full comparison
  if (left->m_bIsFolder == right->m_bIsFolder)
    return StringUtils::AlphaNumericCompare(left->GetLabel().c_str(),right->GetLabel().c_str()) > 0;
  return left->m_bIsFolder;
}

bool SSortFileItem::LabelAscendingNoThe(const CFileItemPtr &left, const CFileItemPtr &right)
{
  // sanity
  RETURN_IF_NULL(left,false); RETURN_IF_NULL(right,false);

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

    return StringUtils::AlphaNumericCompare(l, r) < 0;
  }
  return left->m_bIsFolder;
}

bool SSortFileItem::LabelDescendingNoThe(const CFileItemPtr &left, const CFileItemPtr &right)
{
  // sanity
  RETURN_IF_NULL(left,false); RETURN_IF_NULL(right,false);

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

    return StringUtils::AlphaNumericCompare(l, r) > 0;
  }
  return left->m_bIsFolder;
}

bool SSortFileItem::SongTrackNumAscending(const CFileItemPtr &left, const CFileItemPtr &right)
{
  // sanity
  RETURN_IF_NULL(left,false); RETURN_IF_NULL(right,false);

  // special items
  if (left->IsParentFolder()) return true;
  if (right->IsParentFolder()) return false;
  // only if they're both folders or both files do we do the full comparison
  if (left->m_bIsFolder == right->m_bIsFolder)
    return left->GetMusicInfoTag()->GetTrackAndDiskNumber() < right->GetMusicInfoTag()->GetTrackAndDiskNumber();
  return left->m_bIsFolder;
}

bool SSortFileItem::SongTrackNumDescending(const CFileItemPtr &left, const CFileItemPtr &right)
{
  // sanity
  RETURN_IF_NULL(left,false); RETURN_IF_NULL(right,false);

  // special items
  if (left->IsParentFolder()) return true;
  if (right->IsParentFolder()) return false;
  // only if they're both folders or both files do we do the full comparison
  if (left->m_bIsFolder == right->m_bIsFolder)
    return left->GetMusicInfoTag()->GetTrackAndDiskNumber() > right->GetMusicInfoTag()->GetTrackAndDiskNumber();
  return left->m_bIsFolder;
}

bool SSortFileItem::EpisodeNumAscending(const CFileItemPtr &left, const CFileItemPtr &right)
{
  // sanity
  RETURN_IF_NULL(left,false); RETURN_IF_NULL(right,false);

  // special items
  if (left->IsParentFolder()) return true;
  if (right->IsParentFolder()) return false;
  // only if they're both folders or both files do we do the full comparison
  if (left->m_bIsFolder == right->m_bIsFolder)
  {
    // we calculate an offset number based on the episode's
    // sort season and episode values. in addition
    // we include specials 'episode' numbers to get proper
    // sorting of multiple specials in a row. each
    // of these are given their particular ranges to semi-ensure uniqueness.
    // theoretical problem: if a show has > 128 specials and two of these are placed
    // after each other they will sort backwards. if a show has > 2^8-1 seasons
    // or if a season has > 2^16-1 episodes strange things will happen (overflow)
    unsigned int il;
    unsigned int ir;
    if (left->GetVideoInfoTag()->m_iSpecialSortEpisode > 0)
      il = (left->GetVideoInfoTag()->m_iSpecialSortSeason<<24)+(left->GetVideoInfoTag()->m_iSpecialSortEpisode<<8)-(128-left->GetVideoInfoTag()->m_iEpisode);
    else
      il = (left->GetVideoInfoTag()->m_iSeason<<24)+(left->GetVideoInfoTag()->m_iEpisode<<8);
    if (right->GetVideoInfoTag()->m_iSpecialSortEpisode > 0)
      ir = (right->GetVideoInfoTag()->m_iSpecialSortSeason<<24)+(right->GetVideoInfoTag()->m_iSpecialSortEpisode<<8)-(128-right->GetVideoInfoTag()->m_iEpisode);
    else
      ir = (right->GetVideoInfoTag()->m_iSeason<<24)+(right->GetVideoInfoTag()->m_iEpisode<<8);
    return il < ir;
  }
  return left->m_bIsFolder;
}

bool SSortFileItem::EpisodeNumDescending(const CFileItemPtr &left, const CFileItemPtr &right)
{
  // sanity
  RETURN_IF_NULL(left,false); RETURN_IF_NULL(right,false);

  // special items
  if (left->IsParentFolder()) return true;
  if (right->IsParentFolder()) return false;
  // only if they're both folders or both files do we do the full comparison
  if (left->m_bIsFolder == right->m_bIsFolder)
  {
    unsigned int il;
    unsigned int ir;
    if (left->GetVideoInfoTag()->m_iSpecialSortEpisode > 0)
      il = (left->GetVideoInfoTag()->m_iSpecialSortSeason<<24)+(left->GetVideoInfoTag()->m_iSpecialSortEpisode<<8)-(128-left->GetVideoInfoTag()->m_iEpisode);
    else
      il = (left->GetVideoInfoTag()->m_iSeason<<24)+(left->GetVideoInfoTag()->m_iEpisode<<8);
    if (right->GetVideoInfoTag()->m_iSpecialSortEpisode > 0)
      ir = (right->GetVideoInfoTag()->m_iSpecialSortSeason<<24)+(right->GetVideoInfoTag()->m_iSpecialSortEpisode<<8)-(128-right->GetVideoInfoTag()->m_iEpisode);
    else
      ir = (right->GetVideoInfoTag()->m_iSeason<<24)+(right->GetVideoInfoTag()->m_iEpisode<<8);
    return il > ir;
  }
  return left->m_bIsFolder;
}

bool SSortFileItem::SongDurationAscending(const CFileItemPtr &left, const CFileItemPtr &right)
{
  // sanity
  RETURN_IF_NULL(left,false); RETURN_IF_NULL(right,false);

  // special items
  if (left->IsParentFolder()) return true;
  if (right->IsParentFolder()) return false;
  // only if they're both folders or both files do we do the full comparison
  if (left->m_bIsFolder == right->m_bIsFolder)
    return left->GetMusicInfoTag()->GetDuration() < right->GetMusicInfoTag()->GetDuration();
  return left->m_bIsFolder;
}

bool SSortFileItem::SongDurationDescending(const CFileItemPtr &left, const CFileItemPtr &right)
{
  // sanity
  RETURN_IF_NULL(left,false); RETURN_IF_NULL(right,false);

  // special items
  if (left->IsParentFolder()) return true;
  if (right->IsParentFolder()) return false;
  // only if they're both folders or both files do we do the full comparison
  if (left->m_bIsFolder == right->m_bIsFolder)
    return left->GetMusicInfoTag()->GetDuration() > right->GetMusicInfoTag()->GetDuration();
  return left->m_bIsFolder;
}

bool SSortFileItem::SongTitleAscending(const CFileItemPtr &left, const CFileItemPtr &right)
{
  // sanity
  RETURN_IF_NULL(left,false); RETURN_IF_NULL(right,false);

  // special cases
  if (left->IsParentFolder()) return true;
  if (right->IsParentFolder()) return false;
  // only if they're both folders or both files do we do the full comparison
  if (left->m_bIsFolder == right->m_bIsFolder)
  {
    char *l = (char *)left->GetMusicInfoTag()->GetTitle().c_str();
    char *r = (char *)right->GetMusicInfoTag()->GetTitle().c_str();
    return StringUtils::AlphaNumericCompare(l, r) < 0;
  }
  return left->m_bIsFolder;
}

bool SSortFileItem::SongTitleDescending(const CFileItemPtr &left, const CFileItemPtr &right)
{
  // sanity
  RETURN_IF_NULL(left,false); RETURN_IF_NULL(right,false);

  // special cases
  if (left->IsParentFolder()) return true;
  if (right->IsParentFolder()) return false;
  // only if they're both folders or both files do we do the full comparison
  if (left->m_bIsFolder == right->m_bIsFolder)
  {
    char *l = (char *)left->GetMusicInfoTag()->GetTitle().c_str();
    char *r = (char *)right->GetMusicInfoTag()->GetTitle().c_str();
    return StringUtils::AlphaNumericCompare(l, r) > 0;
  }
  return left->m_bIsFolder;
}

bool SSortFileItem::MovieTitleAscending(const CFileItemPtr &left, const CFileItemPtr &right)
{
  // sanity
  RETURN_IF_NULL(left,false); RETURN_IF_NULL(right,false);

  // special cases
  if (left->IsParentFolder()) return true;
  if (right->IsParentFolder()) return false;
  // only if they're both folders or both files do we do the full comparison
  if (left->m_bIsFolder == right->m_bIsFolder)
  {
    char *l = (char *)left->GetVideoInfoTag()->m_strTitle.c_str();
    char *r = (char *)right->GetVideoInfoTag()->m_strTitle.c_str();
    return StringUtils::AlphaNumericCompare(l, r) < 0;
  }
  return left->m_bIsFolder;
}

bool SSortFileItem::MovieTitleDescending(const CFileItemPtr &left, const CFileItemPtr &right)
{
  // sanity
  RETURN_IF_NULL(left,false); RETURN_IF_NULL(right,false);

  // special cases
  if (left->IsParentFolder()) return true;
  if (right->IsParentFolder()) return false;
  // only if they're both folders or both files do we do the full comparison
  if (left->m_bIsFolder == right->m_bIsFolder)
  {
    char *l = (char *)left->GetVideoInfoTag()->m_strTitle.c_str();
    char *r = (char *)right->GetVideoInfoTag()->m_strTitle.c_str();
    return StringUtils::AlphaNumericCompare(l, r) > 0;
  }
  return left->m_bIsFolder;
}

bool SSortFileItem::SongTitleAscendingNoThe(const CFileItemPtr &left, const CFileItemPtr &right)
{
  // sanity
  RETURN_IF_NULL(left,false); RETURN_IF_NULL(right,false);

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

    return StringUtils::AlphaNumericCompare(l, r) < 0;
  }
  return left->m_bIsFolder;
}

bool SSortFileItem::SongTitleDescendingNoThe(const CFileItemPtr &left, const CFileItemPtr &right)
{
  // sanity
  RETURN_IF_NULL(left,false); RETURN_IF_NULL(right,false);

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

    return StringUtils::AlphaNumericCompare(l, r) > 0;
  }
  return left->m_bIsFolder;
}

bool SSortFileItem::SongArtistAscending(const CFileItemPtr &left, const CFileItemPtr &right)
{
  // sanity
  RETURN_IF_NULL(left,false); RETURN_IF_NULL(right,false);

  // special cases
  if (left->IsParentFolder()) return true;
  if (right->IsParentFolder()) return false;
  // only if they're both folders or both files do we do the full comparison
  if (left->m_bIsFolder == right->m_bIsFolder)
  {
    char *l;
    CStdString strL;
    if (left->HasMusicInfoTag())
      l = (char *)left->GetMusicInfoTag()->GetArtist().c_str();
    else if (left->HasVideoInfoTag())
    {
      strL = left->GetVideoInfoTag()->m_strArtist;
      l = (char*)strL.c_str();
    }
    else
      l = (char*)strL.c_str();
    char *r;
    CStdString strR;
    if (right->HasMusicInfoTag())
      r = (char *)right->GetMusicInfoTag()->GetArtist().c_str();
    else if (right->HasVideoInfoTag())
    {
      strR = right->GetVideoInfoTag()->m_strArtist;
      r = (char*)strR.c_str();
    }
    else
      r = (char*)strR.c_str();
    int result = StringUtils::AlphaNumericCompare(l, r);
    if (result < 0) return true;
    if (result > 0) return false;
    // test year
    if (g_advancedSettings.m_bMusicLibraryAlbumsSortByArtistThenYear)
    {
      int yL = 0, yR = 0;
      if (left->HasMusicInfoTag())
        yL = left->GetMusicInfoTag()->GetYear();
      else if (left->HasVideoInfoTag())
        yL = left->GetVideoInfoTag()->m_iYear;
      if (right->HasMusicInfoTag())
        yR = right->GetMusicInfoTag()->GetYear();
      else if (right->HasVideoInfoTag())
        yR = right->GetVideoInfoTag()->m_iYear;
      if (yL < yR) return true;
      if (yL > yR) return false;
    }
    // artists agree, test the album
    if (left->HasMusicInfoTag())
      l = (char *)left->GetMusicInfoTag()->GetAlbum().c_str();
    else if (left->HasVideoInfoTag())
    {
      l = (char*)left->GetVideoInfoTag()->m_strAlbum.c_str();
    }
    if (right->HasMusicInfoTag())
      r = (char *)right->GetMusicInfoTag()->GetAlbum().c_str();
    else if (right->HasVideoInfoTag())
    {
      r = (char*)right->GetVideoInfoTag()->m_strAlbum.c_str();
    }

    result = StringUtils::AlphaNumericCompare(l, r);
    if (result < 0) return true;
    if (result > 0) return false;
    // artist and album agree, test the track number
    int lt = 0, rt=0;
    if (left->HasMusicInfoTag())
      lt = left->GetMusicInfoTag()->GetTrackAndDiskNumber();
    if (right->HasMusicInfoTag())
      rt = right->GetMusicInfoTag()->GetTrackAndDiskNumber();

    return lt < rt;
  }
  return left->m_bIsFolder;
}

bool SSortFileItem::SongArtistDescending(const CFileItemPtr &left, const CFileItemPtr &right)
{
  // sanity
  RETURN_IF_NULL(left,false); RETURN_IF_NULL(right,false);

  // special cases
  if (left->IsParentFolder()) return true;
  if (right->IsParentFolder()) return false;
  // only if they're both folders or both files do we do the full comparison
  if (left->m_bIsFolder == right->m_bIsFolder)
  {
    char *l;
    CStdString strL, strR;
    if (left->HasMusicInfoTag())
      l = (char *)left->GetMusicInfoTag()->GetArtist().c_str();
    else if (left->HasVideoInfoTag())
      l = (char *)left->GetVideoInfoTag()->m_strArtist.c_str();
    else
      l = (char*)strL.c_str();
    char *r;
    if (right->HasMusicInfoTag())
      r = (char *)right->GetMusicInfoTag()->GetArtist().c_str();
    else if (right->HasVideoInfoTag())
      r = (char*)right->GetVideoInfoTag()->m_strArtist.c_str();
    else
      r = (char*)strR.c_str();

    int result = StringUtils::AlphaNumericCompare(l, r);
    if (result > 0) return true;
    if (result < 0) return false;
    // test year
    if (g_advancedSettings.m_bMusicLibraryAlbumsSortByArtistThenYear)
    {
      int yL = 0, yR = 0;
      if (left->HasMusicInfoTag())
        yL = left->GetMusicInfoTag()->GetYear();
      else if (left->HasVideoInfoTag())
        yL = left->GetVideoInfoTag()->m_iYear;
      if (right->HasMusicInfoTag())
        yR = right->GetMusicInfoTag()->GetYear();
      else if (right->HasVideoInfoTag())
        yR = right->GetVideoInfoTag()->m_iYear;
      if (yL > yR) return true;
      if (yL < yR) return false;
    }
    // artists agree, test the album
    if (left->HasMusicInfoTag())
      l = (char *)left->GetMusicInfoTag()->GetAlbum().c_str();
    else if (left->HasVideoInfoTag())
    {
      l = (char*)left->GetVideoInfoTag()->m_strAlbum.c_str();
    }
    if (right->HasMusicInfoTag())
      r = (char *)right->GetMusicInfoTag()->GetAlbum().c_str();
    else if (right->HasVideoInfoTag())
    {
      r = (char*)right->GetVideoInfoTag()->m_strAlbum.c_str();
    }

    result = StringUtils::AlphaNumericCompare(l, r);
    if (result > 0) return true;
    if (result < 0) return false;
    // artist and album agree, test the track number
    int lt = 0, rt=0;
    if (left->HasMusicInfoTag())
      lt = left->GetMusicInfoTag()->GetTrackAndDiskNumber();
    if (right->HasMusicInfoTag())
      rt = right->GetMusicInfoTag()->GetTrackAndDiskNumber();

    return rt < lt;
  }
  return left->m_bIsFolder;
}

bool SSortFileItem::SongArtistAscendingNoThe(const CFileItemPtr &left, const CFileItemPtr &right)
{
  // sanity
  RETURN_IF_NULL(left,false); RETURN_IF_NULL(right,false);

  // special cases
  if (left->IsParentFolder()) return true;
  if (right->IsParentFolder()) return false;
  // only if they're both folders or both files do we do the full comparison
  if (left->m_bIsFolder == right->m_bIsFolder)
  {
    char *l;
    CStdString strL;
    if (left->HasMusicInfoTag())
      l = (char *)left->GetMusicInfoTag()->GetArtist().c_str();
    else if (left->HasVideoInfoTag())
    {
      strL = left->GetVideoInfoTag()->m_strArtist;
      l = (char*)strL.c_str();
    }
    else
      l = (char*)strL.c_str();
    char *r;
    CStdString strR;
    if (right->HasMusicInfoTag())
      r = (char *)right->GetMusicInfoTag()->GetArtist().c_str();
    else if (right->HasVideoInfoTag())
    {
      strR = right->GetVideoInfoTag()->m_strArtist;
      r = (char*)strR.c_str();
    }
    else
      r = (char*)strR.c_str();
    l += StartsWithToken(l);
    r += StartsWithToken(r);

    int result = StringUtils::AlphaNumericCompare(l, r);
    if (result < 0) return true;
    if (result > 0) return false;
    // test year
    if (g_advancedSettings.m_bMusicLibraryAlbumsSortByArtistThenYear)
    {
      int yL = 0, yR = 0;
      if (left->HasMusicInfoTag())
        yL = left->GetMusicInfoTag()->GetYear();
      else if (left->HasVideoInfoTag())
        yL = left->GetVideoInfoTag()->m_iYear;
      if (right->HasMusicInfoTag())
        yR = right->GetMusicInfoTag()->GetYear();
      else if (right->HasVideoInfoTag())
        yR = right->GetVideoInfoTag()->m_iYear;
      if (yL < yR) return true;
      if (yL > yR) return false;
    }
    // artists agree, test the album
    if (left->HasMusicInfoTag())
      l = (char *)left->GetMusicInfoTag()->GetAlbum().c_str();
    else if (left->HasVideoInfoTag())
      l = (char*)left->GetVideoInfoTag()->m_strAlbum.c_str();
    if (right->HasMusicInfoTag())
      r = (char *)right->GetMusicInfoTag()->GetAlbum().c_str();
    else if (right->HasVideoInfoTag())
      r = (char*)right->GetVideoInfoTag()->m_strAlbum.c_str();

    l += StartsWithToken(l);
    r += StartsWithToken(r);

    result = StringUtils::AlphaNumericCompare(l, r);
    if (result < 0) return true;
    if (result > 0) return false;
    // artist and album agree, test the track number
    int lt = 0, rt=0;
    if (left->HasMusicInfoTag())
      lt = left->GetMusicInfoTag()->GetTrackAndDiskNumber();
    if (right->HasMusicInfoTag())
      rt = right->GetMusicInfoTag()->GetTrackAndDiskNumber();
    return lt < rt;
  }
  return left->m_bIsFolder;
}

bool SSortFileItem::SongArtistDescendingNoThe(const CFileItemPtr &left, const CFileItemPtr &right)
{
  // sanity
  RETURN_IF_NULL(left,false); RETURN_IF_NULL(right,false);

  // special cases
  if (left->IsParentFolder()) return true;
  if (right->IsParentFolder()) return false;
  // only if they're both folders or both files do we do the full comparison
  if (left->m_bIsFolder == right->m_bIsFolder)
  {
    char *l;
    CStdString strL;
    if (left->HasMusicInfoTag())
      l = (char *)left->GetMusicInfoTag()->GetArtist().c_str();
    else if (left->HasVideoInfoTag())
    {
      strL = left->GetVideoInfoTag()->m_strArtist;
      l = (char*)strL.c_str();
    }
    else
      l = (char*)strL.c_str();
    char *r;
    CStdString strR;
    if (right->HasMusicInfoTag())
      r = (char *)right->GetMusicInfoTag()->GetArtist().c_str();
    else if (right->HasVideoInfoTag())
    {
      strR = right->GetVideoInfoTag()->m_strArtist;
      r = (char*)strR.c_str();
    }
    else
      r = (char*)strR.c_str();
    l += StartsWithToken(l);
    r += StartsWithToken(r);

    int result = StringUtils::AlphaNumericCompare(l, r);
    if (result > 0) return true;
    if (result < 0) return false;
    // test year
    if (g_advancedSettings.m_bMusicLibraryAlbumsSortByArtistThenYear)
    {
      int yL = 0, yR = 0;
      if (left->HasMusicInfoTag())
        yL = left->GetMusicInfoTag()->GetYear();
      else if (left->HasVideoInfoTag())
        yL = left->GetVideoInfoTag()->m_iYear;
      if (right->HasMusicInfoTag())
        yR = right->GetMusicInfoTag()->GetYear();
      else if (right->HasVideoInfoTag())
        yR = right->GetVideoInfoTag()->m_iYear;
      if (yL > yR) return true;
      if (yL < yR) return false;
    }
    // artists agree, test the album
    if (left->HasMusicInfoTag())
      l = (char *)left->GetMusicInfoTag()->GetAlbum().c_str();
    else if (left->HasVideoInfoTag())
      l = (char*)left->GetVideoInfoTag()->m_strAlbum.c_str();
    if (right->HasMusicInfoTag())
      r = (char *)right->GetMusicInfoTag()->GetAlbum().c_str();
    else if (right->HasVideoInfoTag())
      r = (char*)right->GetVideoInfoTag()->m_strAlbum.c_str();

    l += StartsWithToken(l);
    r += StartsWithToken(r);

    result = StringUtils::AlphaNumericCompare(l, r);
    if (result > 0) return true;
    if (result < 0) return false;
    // artist and album agree, test the track number
    int lt = 0, rt=0;
    if (left->HasMusicInfoTag())
      lt = left->GetMusicInfoTag()->GetTrackAndDiskNumber();
    if (right->HasMusicInfoTag())
      rt = right->GetMusicInfoTag()->GetTrackAndDiskNumber();
    return rt < lt;
  }
  return left->m_bIsFolder;}

bool SSortFileItem::SongAlbumAscending(const CFileItemPtr &left, const CFileItemPtr &right)
{
  // sanity
  RETURN_IF_NULL(left,false); RETURN_IF_NULL(right,false);

  // special cases
  if (left->IsParentFolder()) return true;
  if (right->IsParentFolder()) return false;
  // only if they're both folders or both files do we do the full comparison
  if (left->m_bIsFolder == right->m_bIsFolder)
  {
    char *l;
    CStdString strL;
    if (left->HasMusicInfoTag())
      l = (char *)left->GetMusicInfoTag()->GetAlbum().c_str();
    else if (left->HasVideoInfoTag())
      l = (char*)left->GetVideoInfoTag()->m_strAlbum.c_str();
    else
      l = (char*)strL.c_str();
    char *r;
    CStdString strR;
    if (right->HasMusicInfoTag())
      r = (char *)right->GetMusicInfoTag()->GetAlbum().c_str();
    else if (right->HasVideoInfoTag())
      r = (char*)right->GetVideoInfoTag()->m_strAlbum.c_str();
    else
      r = (char*)strR.c_str();
    int result = StringUtils::AlphaNumericCompare(l, r);
    if (result < 0) return true;
    if (result > 0) return false;
    // album names match, try the artist
    if (left->HasMusicInfoTag())
      l = (char *)left->GetMusicInfoTag()->GetArtist().c_str();
    else if (left->HasVideoInfoTag())
    {
      strL = left->GetVideoInfoTag()->m_strArtist;
      l = (char*)strL.c_str();
    }
    if (right->HasMusicInfoTag())
      r = (char *)right->GetMusicInfoTag()->GetArtist().c_str();
    else if (right->HasVideoInfoTag())
    {
      strR = right->GetVideoInfoTag()->m_strArtist;
      r = (char*)strR.c_str();
    }
    result = StringUtils::AlphaNumericCompare(l, r);
    if (result < 0) return true;
    if (result > 0) return false;
    // album and artist match - sort by track
    int lt = 0, rt=0;
    if (left->HasMusicInfoTag())
      lt = left->GetMusicInfoTag()->GetTrackAndDiskNumber();
    if (right->HasMusicInfoTag())
      rt = right->GetMusicInfoTag()->GetTrackAndDiskNumber();
    return lt < rt;
  }
  return left->m_bIsFolder;
}

bool SSortFileItem::SongAlbumDescending(const CFileItemPtr &left, const CFileItemPtr &right)
{
  // sanity
  RETURN_IF_NULL(left,false); RETURN_IF_NULL(right,false);

  // special cases
  if (left->IsParentFolder()) return true;
  if (right->IsParentFolder()) return false;
  // only if they're both folders or both files do we do the full comparison
  if (left->m_bIsFolder == right->m_bIsFolder)
  {
    char *l;
    CStdString strL;
    if (left->HasMusicInfoTag())
      l = (char *)left->GetMusicInfoTag()->GetAlbum().c_str();
    else if (left->HasVideoInfoTag())
      l = (char*)left->GetVideoInfoTag()->m_strAlbum.c_str();
    else
      l = (char*)strL.c_str();
    char *r;
    CStdString strR;
    if (right->HasMusicInfoTag())
      r = (char *)right->GetMusicInfoTag()->GetAlbum().c_str();
    else if (right->HasVideoInfoTag())
      r = (char*)right->GetVideoInfoTag()->m_strAlbum.c_str();
    else
      r = (char*)strR.c_str();
    int result = StringUtils::AlphaNumericCompare(l, r);
    if (result > 0) return true;
    if (result < 0) return false;
    // album names match, try the artist
    if (left->HasMusicInfoTag())
      l = (char *)left->GetMusicInfoTag()->GetArtist().c_str();
    else if (left->HasVideoInfoTag())
    {
      strL = left->GetVideoInfoTag()->m_strArtist;
      l = (char*)strL.c_str();
    }
    if (right->HasMusicInfoTag())
      r = (char *)right->GetMusicInfoTag()->GetArtist().c_str();
    else if (right->HasVideoInfoTag())
    {
      strR = right->GetVideoInfoTag()->m_strArtist;
      r = (char*)strR.c_str();
    }
    result = StringUtils::AlphaNumericCompare(l, r);
    if (result > 0) return true;
    if (result < 0) return false;
    // album and artist match - sort by track
    int lt = 0, rt=0;
    if (left->HasMusicInfoTag())
      lt = left->GetMusicInfoTag()->GetTrackAndDiskNumber();
    if (right->HasMusicInfoTag())
      rt = right->GetMusicInfoTag()->GetTrackAndDiskNumber();
    return rt < lt;
  }
  return left->m_bIsFolder;
}

bool SSortFileItem::SongAlbumAscendingNoThe(const CFileItemPtr &left, const CFileItemPtr &right)
{
  // sanity
  RETURN_IF_NULL(left,false); RETURN_IF_NULL(right,false);

  // special cases
  if (left->IsParentFolder()) return true;
  if (right->IsParentFolder()) return false;
  // only if they're both folders or both files do we do the full comparison
  if (left->m_bIsFolder == right->m_bIsFolder)
  {
    char *l;
    CStdString strL;
    if (left->HasMusicInfoTag())
      l = (char *)left->GetMusicInfoTag()->GetAlbum().c_str();
    else if (left->HasVideoInfoTag())
      l = (char*)left->GetVideoInfoTag()->m_strAlbum.c_str();
    else
      l = (char*)strL.c_str();
    char *r;
    CStdString strR;
    if (right->HasMusicInfoTag())
      r = (char *)right->GetMusicInfoTag()->GetAlbum().c_str();
    else if (right->HasVideoInfoTag())
      r = (char*)right->GetVideoInfoTag()->m_strAlbum.c_str();
    else
      r = (char*)strR.c_str();
    l += StartsWithToken(l);
    r += StartsWithToken(r);

    int result = StringUtils::AlphaNumericCompare(l, r);
    if (result < 0) return true;
    if (result > 0) return false;
    // album names match, try the artist
    if (left->HasMusicInfoTag())
      l = (char *)left->GetMusicInfoTag()->GetArtist().c_str();
    else if (left->HasVideoInfoTag())
    {
      strL = left->GetVideoInfoTag()->m_strArtist;
      l = (char*)strL.c_str();
    }
    if (right->HasMusicInfoTag())
      r = (char *)right->GetMusicInfoTag()->GetArtist().c_str();
    else if (right->HasVideoInfoTag())
    {
      strR = right->GetVideoInfoTag()->m_strArtist;
      r = (char*)strR.c_str();
    }
    l += StartsWithToken(l);
    r += StartsWithToken(r);

    result = StringUtils::AlphaNumericCompare(l, r);
    if (result < 0) return true;
    if (result > 0) return false;
    // album and artist match - sort by track
    int lt = 0, rt=0;
    if (left->HasMusicInfoTag())
      lt = left->GetMusicInfoTag()->GetTrackAndDiskNumber();
    if (right->HasMusicInfoTag())
      rt = right->GetMusicInfoTag()->GetTrackAndDiskNumber();

    return lt < rt;
  }
  return left->m_bIsFolder;
}

bool SSortFileItem::SongAlbumDescendingNoThe(const CFileItemPtr &left, const CFileItemPtr &right)
{
  // sanity
  RETURN_IF_NULL(left,false); RETURN_IF_NULL(right,false);

  // special cases
  if (left->IsParentFolder()) return true;
  if (right->IsParentFolder()) return false;
  // only if they're both folders or both files do we do the full comparison
  if (left->m_bIsFolder == right->m_bIsFolder)
  {
    char *l;
    CStdString strL;
    if (left->HasMusicInfoTag())
      l = (char *)left->GetMusicInfoTag()->GetAlbum().c_str();
    else if (left->HasVideoInfoTag())
      l = (char*)left->GetVideoInfoTag()->m_strAlbum.c_str();
    else
      l = (char*)strL.c_str();
    char *r;
    CStdString strR;
    if (right->HasMusicInfoTag())
      r = (char *)right->GetMusicInfoTag()->GetAlbum().c_str();
    else if (right->HasVideoInfoTag())
      r = (char*)right->GetVideoInfoTag()->m_strAlbum.c_str();
    else
      r = (char*)strR.c_str();
    l += StartsWithToken(l);
    r += StartsWithToken(r);

    int result = StringUtils::AlphaNumericCompare(l, r);
    if (result > 0) return true;
    if (result < 0) return false;
    // album names match, try the artist
    if (left->HasMusicInfoTag())
      l = (char *)left->GetMusicInfoTag()->GetArtist().c_str();
    else if (left->HasVideoInfoTag())
    {
      strL = left->GetVideoInfoTag()->m_strArtist;
      l = (char*)strL.c_str();
    }
    if (right->HasMusicInfoTag())
      r = (char *)right->GetMusicInfoTag()->GetArtist().c_str();
    else if (right->HasVideoInfoTag())
    {
      strR = right->GetVideoInfoTag()->m_strArtist;
      r = (char*)strR.c_str();
    }
    l += StartsWithToken(l);
    r += StartsWithToken(r);

    result = StringUtils::AlphaNumericCompare(l, r);
    if (result > 0) return true;
    if (result < 0) return false;
    // album and artist match - sort by track
    int lt = 0, rt=0;
    if (left->HasMusicInfoTag())
      lt = left->GetMusicInfoTag()->GetTrackAndDiskNumber();
    if (right->HasMusicInfoTag())
      rt = right->GetMusicInfoTag()->GetTrackAndDiskNumber();

    return rt < lt;
  }
  return left->m_bIsFolder;
}

bool SSortFileItem::GenreAscending(const CFileItemPtr &left, const CFileItemPtr &right)
{
  // sanity
  RETURN_IF_NULL(left,false); RETURN_IF_NULL(right,false);

  // special items
  if (left->IsParentFolder()) return true;
  if (right->IsParentFolder()) return false;
  // only if they're both folders or both files do we do the full comparison
  if (left->m_bIsFolder == right->m_bIsFolder)
  {
    const char* lGenre;
    if (left->HasMusicInfoTag())
      lGenre = left->GetMusicInfoTag()->GetGenre().c_str();
    else
      lGenre = left->GetVideoInfoTag()->m_strGenre.c_str();
    const char* rGenre;
    if (right->HasMusicInfoTag())
      rGenre = right->GetMusicInfoTag()->GetGenre().c_str();
    else
      rGenre = right->GetVideoInfoTag()->m_strGenre.c_str();

    return StringUtils::AlphaNumericCompare(lGenre,rGenre) < 0;
  }
  return left->m_bIsFolder;
}

bool SSortFileItem::GenreDescending(const CFileItemPtr &left, const CFileItemPtr &right)
{
  // sanity
  RETURN_IF_NULL(left,false); RETURN_IF_NULL(right,false);

  // special items
  if (left->IsParentFolder()) return true;
  if (right->IsParentFolder()) return false;
  // only if they're both folders or both files do we do the full comparison
  if (left->m_bIsFolder == right->m_bIsFolder)
  {
    const char* lGenre;
    if (left->HasMusicInfoTag())
      lGenre = left->GetMusicInfoTag()->GetGenre().c_str();
    else
      lGenre = left->GetVideoInfoTag()->m_strGenre.c_str();
    const char* rGenre;
    if (right->HasMusicInfoTag())
      rGenre = right->GetMusicInfoTag()->GetGenre().c_str();
    else
      rGenre = right->GetVideoInfoTag()->m_strGenre.c_str();

    return StringUtils::AlphaNumericCompare(lGenre,rGenre) > 0;
  }
  return left->m_bIsFolder;
}

bool SSortFileItem::ProgramCountAscending(const CFileItemPtr &left, const CFileItemPtr &right)
{
  // sanity
  RETURN_IF_NULL(left,false); RETURN_IF_NULL(right,false);

  // ignore the ".." item - that should always be on top
  if (left->IsParentFolder()) return true;
  if (right->IsParentFolder()) return false;
  if (left->m_bIsFolder == right->m_bIsFolder)
    return left->m_iprogramCount < right->m_iprogramCount;
  return left->m_bIsFolder;
}

bool SSortFileItem::ProgramCountDescending(const CFileItemPtr &left, const CFileItemPtr &right)
{
  // sanity
  RETURN_IF_NULL(left,false); RETURN_IF_NULL(right,false);

  // ignore the ".." item - that should always be on top
  if (left->IsParentFolder()) return true;
  if (right->IsParentFolder()) return false;
  if (left->m_bIsFolder == right->m_bIsFolder)
    return left->m_iprogramCount > right->m_iprogramCount;
  return left->m_bIsFolder;
}

bool SSortFileItem::MovieYearAscending(const CFileItemPtr &left, const CFileItemPtr &right)
{
  // sanity
  RETURN_IF_NULL(left,false); RETURN_IF_NULL(right,false);

  // ignore the ".." item - that should always be on top
  if (left->IsParentFolder()) return true;
  if (right->IsParentFolder()) return false;
  if (left->m_bIsFolder == right->m_bIsFolder)
  {
    if (left->GetVideoInfoTag()->m_strPremiered.IsEmpty() && left->GetVideoInfoTag()->m_strFirstAired.IsEmpty())
    {
      int result = left->GetVideoInfoTag()->m_iYear-right->GetVideoInfoTag()->m_iYear;
      if (result < 0) return true;
      if (result > 0) return false;
      return StringUtils::AlphaNumericCompare(left->GetLabel().c_str(), right->GetLabel().c_str()) < 0;
    }
    if (!left->GetVideoInfoTag()->m_strPremiered.IsEmpty())
      return StringUtils::AlphaNumericCompare(left->GetVideoInfoTag()->m_strPremiered.c_str(), right->GetVideoInfoTag()->m_strPremiered.c_str()) < 0;
    if (!left->GetVideoInfoTag()->m_strFirstAired.IsEmpty())
      return StringUtils::AlphaNumericCompare(left->GetVideoInfoTag()->m_strFirstAired.c_str(), right->GetVideoInfoTag()->m_strFirstAired.c_str()) < 0;
  }
  return left->m_bIsFolder;
}

bool SSortFileItem::MovieYearDescending(const CFileItemPtr &left, const CFileItemPtr &right)
{
  // sanity
  RETURN_IF_NULL(left,false); RETURN_IF_NULL(right,false);

  // ignore the ".." item - that should always be on top
  if (left->IsParentFolder()) return true;
  if (right->IsParentFolder()) return false;
  if (left->m_bIsFolder == right->m_bIsFolder)
  {
    if (left->GetVideoInfoTag()->m_strPremiered.IsEmpty() && left->GetVideoInfoTag()->m_strFirstAired.IsEmpty())
    {
     int result = left->GetVideoInfoTag()->m_iYear-right->GetVideoInfoTag()->m_iYear;
     if (result < 0) return false;
     if (result > 0) return true;
     return StringUtils::AlphaNumericCompare(left->GetLabel().c_str(), right->GetLabel().c_str()) > 0;
    }
    if (!left->GetVideoInfoTag()->m_strPremiered.IsEmpty())
      return StringUtils::AlphaNumericCompare(left->GetVideoInfoTag()->m_strPremiered.c_str(), right->GetVideoInfoTag()->m_strPremiered.c_str()) > 0;
    if (!left->GetVideoInfoTag()->m_strFirstAired.IsEmpty())
      return StringUtils::AlphaNumericCompare(left->GetVideoInfoTag()->m_strFirstAired.c_str(), right->GetVideoInfoTag()->m_strFirstAired.c_str()) > 0;
  }
  return left->m_bIsFolder;
}

bool SSortFileItem::ProductionCodeAscending(const CFileItemPtr &left, const CFileItemPtr &right)
{
  // sanity
  RETURN_IF_NULL(left,false); RETURN_IF_NULL(right,false);

  // ignore the ".." item - that should always be on top
  if (left->IsParentFolder()) return true;
  if (right->IsParentFolder()) return false;
  if (left->m_bIsFolder == right->m_bIsFolder)
  {
    return StringUtils::AlphaNumericCompare(left->GetVideoInfoTag()->m_strProductionCode.c_str(),right->GetVideoInfoTag()->m_strProductionCode.c_str()) < 0;
  }
  return left->m_bIsFolder;
}

bool SSortFileItem::ProductionCodeDescending(const CFileItemPtr &left, const CFileItemPtr &right)
{
  // sanity
  RETURN_IF_NULL(left,false); RETURN_IF_NULL(right,false);

  // ignore the ".." item - that should always be on top
  if (left->IsParentFolder()) return true;
  if (right->IsParentFolder()) return false;
  if (left->m_bIsFolder == right->m_bIsFolder)
  {
    return StringUtils::AlphaNumericCompare(left->GetVideoInfoTag()->m_strProductionCode.c_str(),right->GetVideoInfoTag()->m_strProductionCode.c_str()) > 0;
  }
  return left->m_bIsFolder;
}

bool SSortFileItem::MovieRatingAscending(const CFileItemPtr &left, const CFileItemPtr &right)
{
  // sanity
  RETURN_IF_NULL(left,false); RETURN_IF_NULL(right,false);

  // ignore the ".." item - that should always be on top
  if (left->IsParentFolder()) return true;
  if (right->IsParentFolder()) return false;
  if (left->m_bIsFolder == right->m_bIsFolder)
  {
    if (left->GetVideoInfoTag()->m_fRating < right->GetVideoInfoTag()->m_fRating) return true;
    if (left->GetVideoInfoTag()->m_fRating > right->GetVideoInfoTag()->m_fRating) return false;
    return StringUtils::AlphaNumericCompare(left->GetLabel().c_str(), right->GetLabel().c_str()) < 0;
  }
  return left->m_bIsFolder;
}

bool SSortFileItem::MovieRatingDescending(const CFileItemPtr &left, const CFileItemPtr &right)
{
  // sanity
  RETURN_IF_NULL(left,false); RETURN_IF_NULL(right,false);

  // ignore the ".." item - that should always be on top
  if (left->IsParentFolder()) return true;
  if (right->IsParentFolder()) return false;
  if (left->m_bIsFolder == right->m_bIsFolder)
  {
    if (left->GetVideoInfoTag()->m_fRating < right->GetVideoInfoTag()->m_fRating) return false;
    if (left->GetVideoInfoTag()->m_fRating > right->GetVideoInfoTag()->m_fRating) return true;
    return StringUtils::AlphaNumericCompare(left->GetLabel().c_str(), right->GetLabel().c_str()) > 0;
  }
  return left->m_bIsFolder;
}

bool SSortFileItem::MPAARatingAscending(const CFileItemPtr &left, const CFileItemPtr &right)
{
  // sanity
  RETURN_IF_NULL(left,false); RETURN_IF_NULL(right,false);

  // ignore the ".." item - that should always be on top
  if (left->IsParentFolder()) return true;
  if (right->IsParentFolder()) return false;
  if (left->m_bIsFolder == right->m_bIsFolder)
  {
    if (left->GetVideoInfoTag()->m_strMPAARating < right->GetVideoInfoTag()->m_strMPAARating) return true;
    if (left->GetVideoInfoTag()->m_strMPAARating > right->GetVideoInfoTag()->m_strMPAARating) return false;
    return StringUtils::AlphaNumericCompare(left->GetLabel().c_str(), right->GetLabel().c_str()) < 0;
  }
  return left->m_bIsFolder;
}

bool SSortFileItem::MPAARatingDescending(const CFileItemPtr &left, const CFileItemPtr &right)
{
  // sanity
  RETURN_IF_NULL(left,false); RETURN_IF_NULL(right,false);

  // ignore the ".." item - that should always be on top
  if (left->IsParentFolder()) return true;
  if (right->IsParentFolder()) return false;
  if (left->m_bIsFolder == right->m_bIsFolder)
  {
    if (left->GetVideoInfoTag()->m_strMPAARating < right->GetVideoInfoTag()->m_strMPAARating) return false;
    if (left->GetVideoInfoTag()->m_strMPAARating > right->GetVideoInfoTag()->m_strMPAARating) return true;
    return StringUtils::AlphaNumericCompare(left->GetLabel().c_str(), right->GetLabel().c_str()) > 0;
  }
  return left->m_bIsFolder;
}

bool SSortFileItem::SongRatingAscending(const CFileItemPtr &left, const CFileItemPtr &right)
{
  // sanity
  RETURN_IF_NULL(left,false); RETURN_IF_NULL(right,false);

  // ignore the ".." item - that should always be on top
  if (left->IsParentFolder()) return true;
  if (right->IsParentFolder()) return false;
  if (left->m_bIsFolder == right->m_bIsFolder)
  {
    if (left->GetMusicInfoTag()->GetRating() < right->GetMusicInfoTag()->GetRating()) return true;
    if (left->GetMusicInfoTag()->GetRating() > right->GetMusicInfoTag()->GetRating()) return false;
    return StringUtils::AlphaNumericCompare(left->GetMusicInfoTag()->GetTitle().c_str(), right->GetMusicInfoTag()->GetTitle().c_str()) > 0;
  }
  return left->m_bIsFolder;
}

bool SSortFileItem::SongRatingDescending(const CFileItemPtr &left, const CFileItemPtr &right)
{
  // sanity
  RETURN_IF_NULL(left,false); RETURN_IF_NULL(right,false);

  // ignore the ".." item - that should always be on top
  if (left->IsParentFolder()) return true;
  if (right->IsParentFolder()) return false;
  if (left->m_bIsFolder == right->m_bIsFolder)
  {
    // currently we just compare rating, then title
    if (left->GetMusicInfoTag()->GetRating() < right->GetMusicInfoTag()->GetRating()) return false;
    if (left->GetMusicInfoTag()->GetRating() > right->GetMusicInfoTag()->GetRating()) return true;
    return StringUtils::AlphaNumericCompare(left->GetMusicInfoTag()->GetTitle().c_str(), right->GetMusicInfoTag()->GetTitle().c_str()) < 0;
  }
  return left->m_bIsFolder;
}

bool SSortFileItem::MovieRuntimeAscending(const CFileItemPtr &left, const CFileItemPtr &right)
{
  // sanity
  RETURN_IF_NULL(left,false); RETURN_IF_NULL(right,false);

  // special cases
  if (left->IsParentFolder()) return true;
  if (right->IsParentFolder()) return false;
  // only if they're both folders or both files do we do the full comparison
  if (left->m_bIsFolder == right->m_bIsFolder)
  {
    char *l = (char *)left->GetVideoInfoTag()->m_strRuntime.c_str();
    char *r = (char *)right->GetVideoInfoTag()->m_strRuntime.c_str();
    return StringUtils::AlphaNumericCompare(l, r) < 0;
  }
  return left->m_bIsFolder;
}

bool SSortFileItem::MovieRuntimeDescending(const CFileItemPtr &left, const CFileItemPtr &right)
{
  // sanity
  RETURN_IF_NULL(left,false); RETURN_IF_NULL(right,false);

  // special cases
  if (left->IsParentFolder()) return true;
  if (right->IsParentFolder()) return false;
  // only if they're both folders or both files do we do the full comparison
  if (left->m_bIsFolder == right->m_bIsFolder)
  {
    char *l = (char *)left->GetVideoInfoTag()->m_strRuntime.c_str();
    char *r = (char *)right->GetVideoInfoTag()->m_strRuntime.c_str();
    return StringUtils::AlphaNumericCompare(l, r) > 0;
  }
  return left->m_bIsFolder;
}

bool SSortFileItem::StudioAscending(const CFileItemPtr &left, const CFileItemPtr &right)
{
  // sanity
  RETURN_IF_NULL(left,false); RETURN_IF_NULL(right,false);

  // special cases
  if (left->IsParentFolder()) return true;
  if (right->IsParentFolder()) return false;
  // only if they're both folders or both files do we do the full comparison
  if (left->m_bIsFolder == right->m_bIsFolder)
  {
    char *l = (char *)left->GetVideoInfoTag()->m_strStudio.c_str();
    char *r = (char *)right->GetVideoInfoTag()->m_strStudio.c_str();
    return StringUtils::AlphaNumericCompare(l, r) < 0;
  }
  return left->m_bIsFolder;
}

bool SSortFileItem::StudioDescending(const CFileItemPtr &left, const CFileItemPtr &right)
{
  // sanity
  RETURN_IF_NULL(left,false); RETURN_IF_NULL(right,false);

  // special cases
  if (left->IsParentFolder()) return true;
  if (right->IsParentFolder()) return false;
  // only if they're both folders or both files do we do the full comparison
  if (left->m_bIsFolder == right->m_bIsFolder)
  {
    char *l = (char *)left->GetVideoInfoTag()->m_strStudio.c_str();
    char *r = (char *)right->GetVideoInfoTag()->m_strStudio.c_str();
    return StringUtils::AlphaNumericCompare(l, r) > 0;
  }
  return left->m_bIsFolder;
}

bool SSortFileItem::StudioAscendingNoThe(const CFileItemPtr &left, const CFileItemPtr &right)
{
  // sanity
  RETURN_IF_NULL(left,false); RETURN_IF_NULL(right,false);

  // special cases
  if (left->IsParentFolder()) return true;
  if (right->IsParentFolder()) return false;
  // only if they're both folders or both files do we do the full comparison
  if (left->m_bIsFolder == right->m_bIsFolder)
  {
    char *l = (char *)left->GetVideoInfoTag()->m_strStudio.c_str();
    char *r = (char *)right->GetVideoInfoTag()->m_strStudio.c_str();
    l += StartsWithToken(left->GetVideoInfoTag()->m_strStudio);
    r += StartsWithToken(right->GetVideoInfoTag()->m_strStudio);

    return StringUtils::AlphaNumericCompare(l, r) < 0;
  }
  return left->m_bIsFolder;
}

bool SSortFileItem::StudioDescendingNoThe(const CFileItemPtr &left, const CFileItemPtr &right)
{
  // sanity
  RETURN_IF_NULL(left,false); RETURN_IF_NULL(right,false);

  // special cases
  if (left->IsParentFolder()) return true;
  if (right->IsParentFolder()) return false;
  // only if they're both folders or both files do we do the full comparison
  if (left->m_bIsFolder == right->m_bIsFolder)
  {
    char *l = (char *)left->GetVideoInfoTag()->m_strStudio.c_str();
    char *r = (char *)right->GetVideoInfoTag()->m_strStudio.c_str();
    l += StartsWithToken(left->GetVideoInfoTag()->m_strStudio);
    r += StartsWithToken(right->GetVideoInfoTag()->m_strStudio);

    return StringUtils::AlphaNumericCompare(l, r) > 0;
  }
  return left->m_bIsFolder;
}
