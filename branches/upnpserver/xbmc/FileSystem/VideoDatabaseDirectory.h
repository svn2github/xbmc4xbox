#pragma once
#include "idirectory.h"
#include "videodatabasedirectory/DirectoryNode.h"
#include "videodatabasedirectory/QueryParams.h"

using namespace DIRECTORY;

namespace DIRECTORY
{
  class CVideoDatabaseDirectory : public IDirectory
  {
  public:
    CVideoDatabaseDirectory(void);
    virtual ~CVideoDatabaseDirectory(void);
    virtual bool GetDirectory(const CStdString& strPath, CFileItemList &items);
    virtual bool Exists(const char* strPath);
    VIDEODATABASEDIRECTORY::NODE_TYPE GetDirectoryChildType(const CStdString& strPath);
    VIDEODATABASEDIRECTORY::NODE_TYPE GetDirectoryType(const CStdString& strPath);
    VIDEODATABASEDIRECTORY::NODE_TYPE GetDirectoryParentType(const CStdString& strPath);
    void ClearDirectoryCache(const CStdString& strDirectory);
    bool IsAllItem(const CStdString& strDirectory);
    bool GetLabel(const CStdString& strDirectory, CStdString& strLabel);
    bool ContainsMovies(const CStdString &path);
  };
};
