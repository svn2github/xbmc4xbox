#include "../../stdafx.h"
#include "DirectoryNode.h"
#include "../../util.h"
#include "QueryParams.h"
#include "DirectoryNodeRoot.h"
#include "DirectoryNodeOverview.h"
#include "DirectoryNodeGenre.h"
#include "DirectoryNodeTitle.h"
#include "DirectoryNodeYear.h"
#include "DirectoryNodeActor.h"

using namespace DIRECTORY::VIDEODATABASEDIRECTORY;

//  Constructor is protected use ParseURL()
CDirectoryNode::CDirectoryNode(NODE_TYPE Type, const CStdString& strName, CDirectoryNode* pParent)
{
  m_Type=Type;
  m_strName=strName;
  m_pParent=pParent;
}

CDirectoryNode::~CDirectoryNode()
{
  if (m_pParent)
    delete m_pParent;
}

//  Parses a given path and returns the current node of the path
CDirectoryNode* CDirectoryNode::ParseURL(const CStdString& strPath)
{
  CURL url(strPath);

  CStdString strDirectory=url.GetFileName();
  if (CUtil::HasSlashAtEnd(strDirectory))
    strDirectory.Delete(strDirectory.size()-1);

  CStdStringArray Path;
  StringUtils::SplitString(strDirectory, "/", Path);
  if (!strDirectory.IsEmpty())
    Path.insert(Path.begin(), "");

  CDirectoryNode* pNode=NULL;
  CDirectoryNode* pParent=NULL;
  NODE_TYPE NodeType=NODE_TYPE_ROOT;

  for (int i=0; i<(int)Path.size(); ++i)
  {
    pNode=CDirectoryNode::CreateNode(NodeType, Path[i], pParent);
    NodeType= pNode ? pNode->GetChildType() : NODE_TYPE_NONE;
    pParent=pNode;
  }

  return pNode;
}

//  returns the database ids of the path,
void CDirectoryNode::GetDatabaseInfo(const CStdString& strPath, CQueryParams& params)
{
  auto_ptr<CDirectoryNode> pNode(CDirectoryNode::ParseURL(strPath));

  if (!pNode.get())
    return;

  pNode->CollectQueryParams(params);
}

//  Create a node object
CDirectoryNode* CDirectoryNode::CreateNode(NODE_TYPE Type, const CStdString& strName, CDirectoryNode* pParent)
{
  switch (Type)
  {
  case NODE_TYPE_ROOT:
    return new CDirectoryNodeRoot(strName, pParent);
  case NODE_TYPE_OVERVIEW:
    return new CDirectoryNodeOverview(strName, pParent);
  case NODE_TYPE_GENRE:
    return new CDirectoryNodeGenre(strName, pParent);
  case NODE_TYPE_YEAR:
    return new CDirectoryNodeYear(strName, pParent);
  case NODE_TYPE_ACTOR:
    return new CDirectoryNodeActor(strName, pParent);
  case NODE_TYPE_TITLE:
    return new CDirectoryNodeTitle(strName, pParent);
  }

  return NULL;
}

//  Current node name
const CStdString& CDirectoryNode::GetName()
{
  return m_strName;
}

//  Current node type
NODE_TYPE CDirectoryNode::GetType()
{
  return m_Type;
}

//  Return the parent directory node or NULL, if there is no
CDirectoryNode* CDirectoryNode::GetParent()
{
  return m_pParent;
}

void CDirectoryNode::RemoveParent()
{
  m_pParent=NULL;
}

//  should be overloaded by a derived class
//  to get the content of a node. Will be called
//  by GetChilds() of a parent node
bool CDirectoryNode::GetContent(CFileItemList& items)
{
  return false;
}

//  Creates a videodb url
CStdString CDirectoryNode::BuildPath()
{
  CStdStringArray array;

  if (!m_strName.IsEmpty())
    array.insert(array.begin(), m_strName);
  
  CDirectoryNode* pParent=m_pParent;
  while (pParent!=NULL)
  {
    const CStdString& strNodeName=pParent->GetName();
    if (!strNodeName.IsEmpty())
      array.insert(array.begin(), strNodeName);

    pParent=pParent->GetParent();
  }

  CStdString strPath="videodb://";
  for (int i=0; i<(int)array.size(); ++i)
    strPath+=array[i]+"/";

  return strPath;
}

//  Collects Query params from this and all parent nodes. If a NODE_TYPE can
//  be used as a database parameter, it will be added to the
//  params object.
void CDirectoryNode::CollectQueryParams(CQueryParams& params)
{
  params.SetQueryParam(m_Type, m_strName);

  CDirectoryNode* pParent=m_pParent;
  while (pParent!=NULL)
  {
    params.SetQueryParam(pParent->GetType(), pParent->GetName());
    pParent=pParent->GetParent();
  }
}

//  Should be overloaded by a derived class.
//  Returns the NODE_TYPE of the child nodes.
NODE_TYPE CDirectoryNode::GetChildType()
{
  return NODE_TYPE_NONE;
}

//  Get the child fileitems of this node
bool CDirectoryNode::GetChilds(CFileItemList& items)
{
  if (CanCache() && items.Load())
    return true;

  auto_ptr<CDirectoryNode> pNode(CDirectoryNode::CreateNode(GetChildType(), "", this));

  bool bSuccess=false;
  if (pNode.get())
  {
    bSuccess=pNode->GetContent(items);
    if (bSuccess)
    {
      AddQueuingFolder(items);
      if (CanCache())
        items.SetCacheToDisc(true);
    }
    else
      items.Clear();

    pNode->RemoveParent();
  }

  return bSuccess;
}

//  Add an "* All ..." folder to the CFileItemList
//  depending on the child node
void CDirectoryNode::AddQueuingFolder(CFileItemList& items)
{
  CFileItem* pItem=NULL;

  // always hide "all" items
  //if (g_guiSettings.GetBool("musicfiles.hideallitems"))
  //  return;

  // no need for "all" item when only one item
  if (items.Size() == 1)
    return;

  switch (GetChildType())
  {
    //  Have no queuing folder
  case NODE_TYPE_ROOT:
  case NODE_TYPE_OVERVIEW:
    break;

  /* no need for all genres
  case NODE_TYPE_GENRE:
    pItem = new CFileItem(g_localizeStrings.Get(15105));  // "All Genres"
    pItem->m_strPath = BuildPath() + "-1/";
    break;
  */

  case NODE_TYPE_GENRE:
  case NODE_TYPE_YEAR:
  case NODE_TYPE_TITLE:
  case NODE_TYPE_ACTOR:
/*    pItem = new CFileItem(g_localizeStrings.Get(15103));  // "All Artists"
    pItem->m_strPath = BuildPath() + "-1/";*/
    break;

/*    //  All album related nodes
  case NODE_TYPE_ALBUM:
    if (GetType() == NODE_TYPE_OVERVIEW) return;
  case NODE_TYPE_ALBUM_RECENTLY_PLAYED:
  case NODE_TYPE_ALBUM_RECENTLY_ADDED:
  case NODE_TYPE_ALBUM_COMPILATIONS:
  case NODE_TYPE_ALBUM_TOP100:
    pItem = new CFileItem(g_localizeStrings.Get(15102));  // "All Albums"
    pItem->m_strPath = BuildPath() + "-1/";
    break;
*/
    //  All song related nodes
/*  case NODE_TYPE_ALBUM_RECENTLY_PLAYED_SONGS:
  case NODE_TYPE_ALBUM_RECENTLY_ADDED_SONGS:
  case NODE_TYPE_ALBUM_COMPILATIONS_SONGS:
  case NODE_TYPE_ALBUM_TOP100_SONGS:
  case NODE_TYPE_SONG_TOP100:
  case NODE_TYPE_SONG:
    pItem = new CFileItem(g_localizeStrings.Get(15104));  // "All Songs"
    pItem->m_strPath = BuildPath() + "-1/";
    break;*/
  }

  if (pItem)
  {
    pItem->m_bIsFolder = true;
    CStdString strFake;
    //  HACK: This item will stay on top of a list
    strFake.Format("%c", 0x01);
    if (g_guiSettings.GetBool("musicfiles.allitemsonbottom"))
      //  HACK: This item will stay on bottom of a list
      strFake.Format("%c", 0xff);
    pItem->m_musicInfoTag.SetAlbum(strFake);
    pItem->m_musicInfoTag.SetArtist(strFake);
    pItem->m_musicInfoTag.SetTitle(strFake);
    pItem->m_musicInfoTag.SetGenre(strFake);
    pItem->SetCanQueue(false);
    pItem->SetLabelPreformated(true);
    items.Add(pItem);
  }
}

bool CDirectoryNode::CanCache()
{
  //  Only cache the directorys in the root
  NODE_TYPE childnode=GetChildType();
  NODE_TYPE node=GetType();

  // something should probably be cached
  return (childnode==NODE_TYPE_TITLE);
}
