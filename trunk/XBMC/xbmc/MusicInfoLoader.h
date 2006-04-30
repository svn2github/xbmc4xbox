#pragma once
#include "BackgroundInfoLoader.h"
#include "MusicDatabase.h"

namespace MUSIC_INFO
{
class CMusicInfoLoader : public CBackgroundInfoLoader
{
public:
  CMusicInfoLoader();
  virtual ~CMusicInfoLoader();

  void UseCacheOnHD(const CStdString& strFileName);
  virtual bool LoadItem(CFileItem* pItem);

protected:
  virtual void OnLoaderStart();
  virtual void OnLoaderFinish();
  void LoadCache(const CStdString& strFileName, CFileItemList& items);
  void SaveCache(const CStdString& strFileName, CFileItemList& items);
protected:
  CStdString m_strCacheFileName;
  CFileItemList m_mapFileItems;
  IMAPFILEITEMS it;
  CSongMap m_songsMap;
  CStdString m_strPrevPath;
  CMusicDatabase m_musicDatabase;
  int m_songsFromDb;
  int m_taglessItems;
};
};
