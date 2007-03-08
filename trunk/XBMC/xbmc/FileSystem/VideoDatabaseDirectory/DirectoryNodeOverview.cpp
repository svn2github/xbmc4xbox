#include "../../stdafx.h"
#include "DirectoryNodeOverview.h"

using namespace DIRECTORY::VIDEODATABASEDIRECTORY;

CDirectoryNodeOverview::CDirectoryNodeOverview(const CStdString& strName, CDirectoryNode* pParent)
  : CDirectoryNode(NODE_TYPE_OVERVIEW, strName, pParent)
{

}

NODE_TYPE CDirectoryNodeOverview::GetChildType()
{
  if (GetName()=="1")
    return NODE_TYPE_MOVIES_OVERVIEW;
  else if (GetName()=="2")
    return NODE_TYPE_TVSHOWS_OVERVIEW;

  return NODE_TYPE_NONE;
}

bool CDirectoryNodeOverview::GetContent(CFileItemList& items)
{
  CStdStringArray vecRoot;
  vecRoot.push_back(g_localizeStrings.Get(342));   // Movies
  vecRoot.push_back(g_localizeStrings.Get(20343)); // TV Shows

  for (int i = 0; i < (int)vecRoot.size(); ++i)
  {
    CFileItem* pItem = new CFileItem(vecRoot[i]);
    CStdString strDir;
    strDir.Format("%i/", i+1);
    pItem->m_strPath = BuildPath() + strDir;
    pItem->m_bIsFolder = true;
    pItem->SetCanQueue(false);
    items.Add(pItem);
  }

  return true;
}
