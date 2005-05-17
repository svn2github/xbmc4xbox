#pragma once
#include "MusicAlbumInfo.h"
#include "http.h"

using namespace MUSIC_GRABBER;

namespace MUSIC_GRABBER
{
class CMusicInfoScraper : public CThread
{
public:
  CMusicInfoScraper(void);
  virtual ~CMusicInfoScraper(void);
  void FindAlbuminfo(const CStdString& strAlbum);
  void LoadAlbuminfo(int iAlbum);
  bool Completed();
  bool Successfull();
  void Cancel();
  bool IsCanceled();
  int GetAlbumCount() const;
  CMusicAlbumInfo& GetAlbum(int iAlbum);
protected:
  void FindAlbuminfo();
  void LoadAlbuminfo();
  virtual void OnStartup();
  virtual void Process();
  vector<CMusicAlbumInfo> m_vecAlbums;
  CStdString m_strAlbum;
  int m_iAlbum;
  bool m_bSuccessfull;
  bool m_bCanceled;
  CHTTP m_http;
};

};
