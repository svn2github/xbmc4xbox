#pragma once
#include "IDirectory.h"

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
  bool GetTypeAndBookmark(const CStdString& strPath, CStdString& strType, CStdString& strBookmark);
  bool GetMatchingShare(const CStdString &strPath, CShare& share);
};
}
