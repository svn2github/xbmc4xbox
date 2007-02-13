#include "../../stdafx.h"
#include "DirectoryNodeYear.h"
#include "QueryParams.h"
#include "../../VideoDatabase.h"

using namespace DIRECTORY::VIDEODATABASEDIRECTORY;

CDirectoryNodeYear::CDirectoryNodeYear(const CStdString& strName, CDirectoryNode* pParent)
  : CDirectoryNode(NODE_TYPE_YEAR, strName, pParent)
{

}

bool CDirectoryNodeYear::GetContent(CFileItemList& items)
{
  CVideoDatabase videodatabase;
  if (!videodatabase.Open())
    return false;

  CQueryParams params;
  CollectQueryParams(params);

  CStdString strBaseDir=BuildPath();
  bool bSuccess=videodatabase.GetYearsNav(strBaseDir, items, params.GetContentType());

  videodatabase.Close();

  return bSuccess;
}
