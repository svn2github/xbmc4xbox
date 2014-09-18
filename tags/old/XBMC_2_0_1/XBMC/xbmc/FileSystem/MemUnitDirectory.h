#pragma once

#include "idirectory.h"
#include "MemoryUnits/IFileSystem.h"

using namespace DIRECTORY;
namespace DIRECTORY
{
  class CMemUnitDirectory : public IDirectory
  {
  public:
    CMemUnitDirectory(void);
    virtual ~CMemUnitDirectory(void);
    virtual bool GetDirectory(const CStdString& strPath, CFileItemList &items);
    virtual bool Create(const char* strPath);
    virtual bool Exists(const char* strPath);
    virtual bool Remove(const char* strPath);
  protected:
    IFileSystem *GetFileSystem(const CStdString &path);
  };
};
