#pragma once
#include "IDirectory.h"

using namespace DIRECTORY;
namespace DIRECTORY
{
class CVirtualPathDirectory :
      public IDirectory
{
public:
  CVirtualPathDirectory(void);
  virtual ~CVirtualPathDirectory(void);
  virtual bool GetDirectory(const CStdString& strPath, CFileItemList &items);
  virtual bool Exists(const CStdString& strPath);

  bool GetPathes(const CStdString& strPath, vector<CStdString>& vecPaths);

protected:
  virtual bool GetTypeAndBookmark(const CStdString& strPath, CStdString& strType, CStdString& strBookmark);
  virtual bool GetMatchingShare(const CStdString &strPath, CShare& share);
};
}
