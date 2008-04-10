#pragma once

#include "ImusicInfoTagLoader.h"

#include "APEv2Tag.h"

namespace MUSIC_INFO
{

class CMusicInfoTagLoaderApe: public IMusicInfoTagLoader
{
public:
  CMusicInfoTagLoaderApe(void);
  virtual ~CMusicInfoTagLoaderApe();

  virtual bool Load(const CStdString& strFileName, CMusicInfoTag& tag);
private:
  virtual int ReadDuration(const CStdString& strFileName);
};
}
