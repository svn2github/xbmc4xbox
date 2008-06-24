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
#include "PlayList.h"
#include "Util.h"
#include "PlayListFactory.h"
#include <sstream>
#include "VideoInfoTag.h"
#include "MusicInfoTag.h"
#include "FileSystem/File.h"

using namespace std;
using namespace MUSIC_INFO;
using namespace XFILE;
using namespace PLAYLIST;
/*
CPlayListItem::CPlayListItem() : m_lDuration(0)
{
  m_lStartOffset = 0;
  m_lEndOffset = 0;
  m_bUnPlayable = false;
	m_iprogramCount = 0;
}

CPlayListItem::CPlayListItem(const CStdString& strDescription, const CStdString& strFileName, long lDuration, long lStartOffset, long lEndOffset)
{
  m_strLabel = strDescription;
  m_strPath = strFileName;
  m_lDuration = lDuration;
  m_lStartOffset = lStartOffset;
  m_lEndOffset = lEndOffset;
  m_bUnPlayable = false;
	m_iprogramCount = 0;
}

CPlayListItem::~CPlayListItem()
{}

void CPlayListItem::SetFileName(const CStdString& strFileName)
{
  m_strPath = strFileName;
}

const CStdString& CPlayListItem::GetFileName() const
{
  return m_strPath;
}

void CPlayListItem::SetDescription(const CStdString& strDescription)
{
  m_strLabel = strDescription;
}

const CStdString& CPlayListItem::GetDescription() const
{
  return m_strLabel;
}

void CPlayListItem::SetDuration(long lDuration)
{
  m_lDuration = lDuration;
}

long CPlayListItem::GetDuration() const
{
  return m_lDuration;
}

void CPlayListItem::SetStartOffset(long lStartOffset)
{
  m_lStartOffset = lStartOffset;
}

long CPlayListItem::GetStartOffset() const
{
  return m_lStartOffset;
}

void CPlayListItem::SetEndOffset(long lEndOffset)
{
  m_lEndOffset = lEndOffset;
}

long CPlayListItem::GetEndOffset() const
{
  return m_lEndOffset;
}

void CPlayListItem::SetMusicTag(const CMusicInfoTag &tag)
{
  *GetMusicInfoTag() = tag;
}

void CPlayListItem::SetVideoTag(const CVideoInfoTag &tag)
{
  *GetVideoInfoTag() = tag;
}

bool CPlayListItem::LoadMusicTag()
{
  if (CFileItem::LoadMusicTag())
  {
    SetDuration(GetMusicInfoTag()->GetDuration());
    return true;
  }
  return false;
}

const CMusicInfoTag* CPlayListItem::GetMusicTag() const
{
  return GetMusicInfoTag();
}

const CVideoInfoTag* CPlayListItem::GetVideoTag() const
{
  return GetVideoInfoTag();
}*/

CPlayList::CPlayList(void)
{
  m_strPlayListName = "";
  m_iPlayableItems = -1;
  m_bShuffled = false;
  m_bWasPlayed = false;
}

CPlayList::~CPlayList(void)
{
  Clear();
}

void CPlayList::Add(const CFileItemPtr &item, int iPosition, int iOrder)
{
  int iOldSize = size();
  if (iPosition < 0 || iPosition >= iOldSize)
    iPosition = iOldSize;
  if (iOrder < 0 || iOrder >= iOldSize)
    item->m_iprogramCount = iOldSize;
	else
		item->m_iprogramCount = iOrder;

  // increment the playable counter
  item->ClearProperty("unplayable");
  if (m_iPlayableItems < 0)
    m_iPlayableItems = 1;
  else
    m_iPlayableItems++;

  //CLog::Log(LOGDEBUG,"%s item:(%02i/%02i)[%s]", __FUNCTION__, iPosition, item.m_iprogramCount, item.m_strPath.c_str());
  if (iPosition == iOldSize)
    m_vecItems.push_back(item);
  else
  {
    ivecItems it = m_vecItems.begin() + iPosition;
    m_vecItems.insert(it, 1, item);
    // correct any duplicate order values
    if (iOrder < iOldSize)
      IncrementOrder(iPosition + 1, iOrder);
  }
}

void CPlayList::Add(const CFileItemPtr &item)
{
  Add(item, -1, -1);
}

void CPlayList::Add(CPlayList& playlist)
{
	for (int i = 0; i < (int)playlist.size(); i++)
		Add(playlist[i], -1, -1);
}

void CPlayList::Add(CFileItemList& items)
{
	for (int i = 0; i < (int)items.Size(); i++)
		Add(items[i]);
}

void CPlayList::Insert(CPlayList& playlist, int iPosition /* = -1 */)
{
  // out of bounds so just add to the end
  int iSize = size();
  if (iPosition < 0 || iPosition >= iSize)
  {
    Add(playlist);
    return;
  }
	for (int i = 0; i < (int)playlist.size(); i++)
  {
    int iPos = iPosition + i;
		Add(playlist[i], iPos, iPos);
  }
}

void CPlayList::Insert(CFileItemList& items, int iPosition /* = -1 */)
{
  // out of bounds so just add to the end
  int iSize = size();
  if (iPosition < 0 || iPosition >= iSize)
  {
    Add(items);
    return;
  }
  for (int i = 0; i < (int)items.Size(); i++)
  {
		Add(items[i], iPosition + i, iPosition + i);
  }
}

void CPlayList::DecrementOrder(int iOrder)
{
  if (iOrder < 0) return;

  // it was the last item so do nothing
  if (iOrder == size()) return;

  // fix all items with an order greater than the removed iOrder
  ivecItems it;
  it = m_vecItems.begin();
  while (it != m_vecItems.end())
  {
    CFileItemPtr item = *it;
    if (item->m_iprogramCount > iOrder)
    {
      //CLog::Log(LOGDEBUG,"%s fixing item at order %i", __FUNCTION__, item.m_iprogramCount);
      item->m_iprogramCount--;
    }
    ++it;
  }
}

void CPlayList::IncrementOrder(int iPosition, int iOrder)
{
  if (iOrder < 0) return;

  // fix all items with an order equal or greater to the added iOrder at iPos
  ivecItems it;
  it = m_vecItems.begin() + iPosition;
  while (it != m_vecItems.end())
  {
    CFileItemPtr item = *it;
    if (item->m_iprogramCount >= iOrder)
    {
      //CLog::Log(LOGDEBUG,"%s fixing item at order %i", __FUNCTION__, item.m_iprogramCount);
      item->m_iprogramCount++;
    }
    ++it;
  }
}

void CPlayList::Clear()
{
  m_vecItems.erase(m_vecItems.begin(), m_vecItems.end());
  m_strPlayListName = "";
  m_iPlayableItems = -1;
  m_bWasPlayed = false;
}

int CPlayList::size() const
{
  return (int)m_vecItems.size();
}

const CFileItemPtr CPlayList::operator[] (int iItem) const
{
  return m_vecItems[iItem];
}

CFileItemPtr CPlayList::operator[] (int iItem)
{
  return m_vecItems[iItem];
}

void CPlayList::Shuffle(int iPosition)
{
  srand(timeGetTime());

  if (size() == 0)
    // nothing to shuffle, just set the flag for later
    m_bShuffled = true;
  else
  {
    if (iPosition >= size())
      return;
    if (iPosition < 0)
      iPosition = 0;
    CLog::Log(LOGDEBUG,"%s :shuffling at pos:%i", __FUNCTION__, iPosition);

    ivecItems it = m_vecItems.begin() + iPosition;
    random_shuffle(it, m_vecItems.end());

    // the list is now shuffled!
    m_bShuffled = true;
  }
}

struct SSortPlayListItem
{
  static bool PlaylistSort(const CFileItemPtr &left, const CFileItemPtr &right)
  {
    return (left->m_iprogramCount <= right->m_iprogramCount);
  }
};

void CPlayList::UnShuffle()
{
  sort(m_vecItems.begin(), m_vecItems.end(), SSortPlayListItem::PlaylistSort);
  // the list is now unshuffled!
  m_bShuffled = false;
}

const CStdString& CPlayList::GetName() const
{
  return m_strPlayListName;
}

void CPlayList::Remove(const CStdString& strFileName)
{
  int iOrder = -1;
  ivecItems it;
  it = m_vecItems.begin();
  while (it != m_vecItems.end() )
  {
    CFileItemPtr item = *it;
    if (item->m_strPath == strFileName)
    {
      iOrder = item->m_iprogramCount;
      it = m_vecItems.erase(it);
      //CLog::Log(LOGDEBUG,"PLAYLIST, removing item at order %i", iPos);
    }
    else
      ++it;
  }
  DecrementOrder(iOrder);
}

int CPlayList::FindOrder(int iOrder)
{
  for (int i = 0; i < size(); i++)
  {
    if (m_vecItems[i]->m_iprogramCount == iOrder)
      return i;
  }
  return -1;
}

// remove item from playlist by position
void CPlayList::Remove(int position)
{
  int iOrder = -1;
  if (position < (int)m_vecItems.size())
  {
    iOrder = m_vecItems[position]->m_iprogramCount;
    m_vecItems.erase(m_vecItems.begin() + position);
  }
  DecrementOrder(iOrder);
}

int CPlayList::RemoveDVDItems()
{
  vector <CStdString> vecFilenames;

  // Collect playlist items from DVD share
  ivecItems it;
  it = m_vecItems.begin();
  while (it != m_vecItems.end() )
  {
    CFileItemPtr item = *it;
    if ( item->IsCDDA() || item->IsOnDVD() )
    {
      vecFilenames.push_back( item->m_strPath );
    }
    it++;
  }

  // Delete them from playlist
  int nFileCount = vecFilenames.size();
  if ( nFileCount )
  {
    vector <CStdString>::iterator it;
    it = vecFilenames.begin();
    while (it != vecFilenames.end() )
    {
      CStdString& strFilename = *it;
      Remove( strFilename );
      it++;
    }
    vecFilenames.erase( vecFilenames.begin(), vecFilenames.end() );
  }
  return nFileCount;
}

bool CPlayList::Swap(int position1, int position2)
{
  if (
    (position1 < 0) ||
    (position2 < 0) ||
    (position1 >= size()) ||
    (position2 >= size())
  )
  {
    return false;
  }

  int iOrder = -1;
  if (!IsShuffled())
  {
    // swap the ordinals before swapping the items!
    //CLog::Log(LOGDEBUG,"PLAYLIST swapping items at orders (%i, %i)",m_vecItems[position1].m_iprogramCount,m_vecItems[position2].m_iprogramCount);
    swap(m_vecItems[position1]->m_iprogramCount, m_vecItems[position2]->m_iprogramCount);
  }

  // swap the items
  std::swap(m_vecItems[position1], m_vecItems[position2]);
  return true;
}

void CPlayList::SetUnPlayable(int iItem)
{
  CFileItemPtr item = m_vecItems[iItem];
  if (!item->GetPropertyBOOL("unplayable"))
  {
    item->SetProperty("unplayable", true);
    m_iPlayableItems--;
  }
}


bool CPlayList::Load(const CStdString& strFileName)
{
  Clear();
  CUtil::GetDirectory(strFileName, m_strBasePath);

  CFileStream file;
  if (!file.Open(strFileName))
    return false;

  if (file.GetLength() > 1024*1024)
  {
    CLog::Log(LOGWARNING, "%s - File is larger than 1 MB, most likely not a playlist", __FUNCTION__);
    return false;
  }

  return LoadData(file);
}

bool CPlayList::LoadData(std::istream &stream)
{
  // try to read as a string
  CStdString data;
  std::stringstream(data) << stream;
  return LoadData(data);
}

bool CPlayList::LoadData(const CStdString& strData)
{
  return false;
}


bool CPlayList::Expand(int position)
{
  CFileItemPtr item = m_vecItems[position];
  auto_ptr<CPlayList> playlist (CPlayListFactory::Create(*item.get()));
  if ( NULL == playlist.get())
    return false;

  if(!playlist->Load(item->m_strPath))
    return false;

  // remove any item that points back to itself
  for(int i = 0;i<playlist->size();i++)
  {
    if( (*playlist)[i]->m_strPath.Equals( item->m_strPath ) )
    {
      playlist->Remove(i);
      i--;
    }
  }

  if(playlist->size() <= 0)
    return false;

  Remove(position);
  Insert(*playlist, position);
  return true;
}

void CPlayList::UpdateItem(const CFileItem *item)
{
  if (!item) return;

  for (ivecItems it = m_vecItems.begin(); it != m_vecItems.end(); ++it)
  {
    CFileItemPtr playlistItem = *it;
    if (playlistItem->m_strPath == item->m_strPath)
    {
      *playlistItem = *item;
      break;
    }
  }
}

