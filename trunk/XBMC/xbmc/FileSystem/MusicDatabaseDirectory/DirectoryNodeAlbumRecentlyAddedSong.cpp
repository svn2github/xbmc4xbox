#include "../../stdafx.h"
#include "DirectoryNodeAlbumRecentlyAddedSong.h"
#include "../../MusicDatabase.h"

using namespace DIRECTORY::MUSICDATABASEDIRECTORY;

CDirectoryNodeAlbumRecentlyAddedSong::CDirectoryNodeAlbumRecentlyAddedSong(const CStdString& strName, CDirectoryNode* pParent)
  : CDirectoryNode(NODE_TYPE_ALBUM_RECENTLY_ADDED_SONGS, strName, pParent)
{

}

bool CDirectoryNodeAlbumRecentlyAddedSong::GetContent(CFileItemList& items)
{
  CMusicDatabase musicdatabase;
  if (!musicdatabase.Open())
    return false;

  bool bSuccess=musicdatabase.GetRecentlyAddedAlbumSongs(BuildPath(), items);

  musicdatabase.Close();

  return bSuccess;
}
