#pragma once
#include "IDirectory.h"

class CMediaSource;

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

  bool GetPathes(const CStdString& strPath, std::vector<CStdString>& vecPaths);

protected:
  bool GetTypeAndSource(const CStdString& strPath, CStdString& strType, CStdString& strSource);
  bool GetMatchingSource(const CStdString &strPath, CMediaSource& share);
};
}
