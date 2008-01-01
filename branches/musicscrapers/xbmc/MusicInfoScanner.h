#pragma once
#include "utils/Thread.h"
#include "musicDatabase.h"

class CAlbum;
class CArtist;

namespace MUSIC_INFO
{
enum SCAN_STATE { PREPARING = 0, REMOVING_OLD, CLEANING_UP_DATABASE, READING_MUSIC_INFO, DOWNLOADING_ALBUM_INFO, DOWNLOADING_ARTIST_INFO, COMPRESSING_DATABASE, WRITING_CHANGES };

class IMusicInfoScannerObserver
{
public:
  virtual void OnStateChanged(SCAN_STATE state) = 0;
  virtual void OnDirectoryChanged(const CStdString& strDirectory) = 0;
  virtual void OnDirectoryScanned(const CStdString& strDirectory) = 0;
  virtual void OnSetProgress(int currentItem, int itemCount)=0;
  virtual void OnFinished() = 0;
};

class CMusicInfoScanner : CThread, public IRunnable
{
public:
  CMusicInfoScanner();
  virtual ~CMusicInfoScanner();

  void Start(const CStdString& strDirectory);
  void FetchAlbumInfo(const CStdString& strDirectory);
  void FetchArtistInfo(const CStdString& strDirectory);
  bool IsScanning();
  void Stop();
  void SetObserver(IMusicInfoScannerObserver* pObserver);

  static void CheckForVariousArtists(VECSONGS &songs);
  static bool HasSingleAlbum(const VECSONGS &songs, CStdString &album, CStdString &artist);

protected:
  virtual void Process();
  int RetrieveMusicInfo(CFileItemList& items, const CStdString& strDirectory);
  void UpdateFolderThumb(const VECSONGS &songs, const CStdString &folderPath);
  int GetPathHash(const CFileItemList &items, CStdString &hash);

  bool DoScan(const CStdString& strDirectory);

  virtual void Run();
  int CountFiles(const CFileItemList& items, bool recursive);
  int CountFilesRecursively(const CStdString& strPath);

protected:
  IMusicInfoScannerObserver* m_pObserver;
  int m_currentItem;
  int m_itemCount;
  bool m_bRunning;
  bool m_bCanInterrupt;
  bool m_needsCleanup;
  int m_scanType; // 0 - load from files, 1 - albums, 2 - artists
  CMusicDatabase m_musicDatabase;

  set<CStdString> m_pathsToScan;
  set<CAlbum> m_albumsToScan;
  set<CArtist> m_artistsToScan;
  set<CStdString> m_pathsToCount;
};
}
