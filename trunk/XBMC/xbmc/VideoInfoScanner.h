#pragma once
#include "utils/Thread.h"
#include "VideoDatabase.h"
#include "utils/IMDB.h"

namespace VIDEO
{
  typedef struct SScanSettings
  {
    bool parent_name;       /* use the parent dirname as name of lookup */
    bool parent_name_root;  /* use the name of directory where scan started as name for files in that dir */
    int  recurse;           /* recurse into sub folders (indicate levels) */
  } SScanSettings;

  enum SCAN_STATE { PREPARING = 0, REMOVING_OLD, CLEANING_UP_DATABASE, FETCHING_VIDEO_INFO, COMPRESSING_DATABASE, WRITING_CHANGES };

  class IVideoInfoScannerObserver
  {
  public:
    virtual void OnStateChanged(SCAN_STATE state) = 0;
    virtual void OnDirectoryChanged(const CStdString& strDirectory) = 0;
    virtual void OnDirectoryScanned(const CStdString& strDirectory) = 0;
    virtual void OnSetProgress(int currentItem, int itemCount)=0;
    virtual void OnSetCurrentProgress(int currentItem, int itemCount)=0;
    virtual void OnFinished() = 0;
  };

  class CVideoInfoScanner : CThread, public IRunnable
  {
  public:
    CVideoInfoScanner();
    virtual ~CVideoInfoScanner();
    void Start(const CStdString& strDirectory, const SScraperInfo& info, const SScanSettings& settings, bool bUpdateAll);
    bool IsScanning();
    void Stop();
    void SetObserver(IVideoInfoScannerObserver* pObserver);

    void EnumerateSeriesFolder(const CFileItem* item, IMDB_EPISODELIST& episodeList);
    long AddMovieAndGetThumb(CFileItem *pItem, const CStdString &content, const CVideoInfoTag &movieDetails, long idShow, bool bApplyToDir=false, CGUIDialogProgress* pDialog = NULL);
    void OnProcessSeriesFolder(IMDB_EPISODELIST& episodes, IMDB_EPISODELIST& files, long lShowId, CIMDB& IMDB, CGUIDialogProgress* pDlgProgress = NULL);
    static CStdString GetnfoFile(CFileItem *item);
    long GetIMDBDetails(CFileItem *pItem, CIMDBUrl &url, const SScraperInfo& info, bool bUseDirNames=false, CGUIDialogProgress* pDialog=NULL);
    bool RetrieveVideoInfo(CFileItemList& items, bool bDirNames, const SScraperInfo& info, bool bRefresh=false, CIMDBUrl *pURL=NULL, CGUIDialogProgress* m_dlgProgress  = NULL);
    static void ApplyIMDBThumbToFolder(const CStdString &folder, const CStdString &imdbThumb);

  protected:
    virtual void Process();
    bool DoScan(const CStdString& strDirectory, SScanSettings settings);

    virtual void Run();
    int CountFiles(const CStdString& strPath);
    static int GetPathHash(const CFileItemList &items, CStdString &hash);

  protected:
    IVideoInfoScannerObserver* m_pObserver;
    int m_currentItem;
    int m_itemCount;
    bool m_bRunning;
    bool m_bUpdateAll;
    bool m_bCanInterrupt;
    CStdString m_strStartDir;
    CVideoDatabase m_database;
    SScraperInfo m_info;
    map<CStdString,SScanSettings> m_pathsToScan;
    set<CStdString> m_pathsToCount;
  };
};