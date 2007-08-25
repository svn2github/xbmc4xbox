#pragma once
#include "DirectoryNode.h"

namespace DIRECTORY
{
  namespace VIDEODATABASEDIRECTORY
  {
    class CDirectoryNodeStudio : public CDirectoryNode
    {
    public:
      CDirectoryNodeStudio(const CStdString& strName, CDirectoryNode* pParent);
    protected:
      virtual NODE_TYPE GetChildType();
      virtual bool GetContent(CFileItemList& items);
    };
  };
};

