#include "../../stdafx.h"
#include "DirectoryNodeAlbumRecentlyPlayed.h"
#include "../../MusicDatabase.h"

using namespace DIRECTORY::MUSICDATABASEDIRECTORY;

CDirectoryNodeAlbumRecentlyPlayed::CDirectoryNodeAlbumRecentlyPlayed(const CStdString& strName, CDirectoryNode* pParent)
  : CDirectoryNode(NODE_TYPE_ALBUM_RECENTLY_PLAYED, strName, pParent)
{

}

NODE_TYPE CDirectoryNodeAlbumRecentlyPlayed::GetChildType()
{
  if (GetName()=="-1")
    return NODE_TYPE_ALBUM_RECENTLY_PLAYED_SONGS;

  return NODE_TYPE_SONG; 
}

bool CDirectoryNodeAlbumRecentlyPlayed::GetContent(CFileItemList& items)
{
  CMusicDatabase musicdatabase;
  if (!musicdatabase.Open())
    return false;

  VECALBUMS albums;
  if (!musicdatabase.GetRecentlyPlayedAlbums(albums))
  {
    musicdatabase.Close();
    return false;
  }

  // add "All Albums"
  CFileItem* pItem = new CFileItem(g_localizeStrings.Get(15102));
  pItem->m_strPath=BuildPath() + "-1/";
  pItem->m_bIsFolder = true;
  items.Add(pItem);

  for (int i=0; i<(int)albums.size(); ++i)
  {
    CAlbum& album=albums[i];
    CFileItem* pItem=new CFileItem(album);
    pItem->m_strPath=BuildPath();
    CStdString strDir;
    strDir.Format("%ld/", album.idAlbum);
    pItem->m_strPath+=strDir;
    pItem->SetCanQueue(false);
    items.Add(pItem);
  }


  musicdatabase.Close();

  return true;
}
