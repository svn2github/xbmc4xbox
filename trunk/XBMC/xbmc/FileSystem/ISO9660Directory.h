#pragma once
#include "idirectory.h"

namespace DIRECTORY
{
class CISO9660Directory :
      public IDirectory
{
public:
  CISO9660Directory(void);
  virtual ~CISO9660Directory(void);
  virtual bool GetDirectory(const CStdString& strPath, CFileItemList &items);
  virtual bool Exists(const char* strPath);
};
}
