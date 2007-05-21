#pragma once
#include "Database.h"
#include "utils/IMDB.h"
#include "settings/VideoSettings.h"
#include <set>

#ifndef my_offsetof
#ifndef _LINUX
#define my_offsetof(TYPE, MEMBER) offsetof(TYPE, MEMBER)
#else
/*
   Custom version of standard offsetof() macro which can be used to get
   offsets of members in class for non-POD types (according to the current
   version of C++ standard offsetof() macro can't be used in such cases and
   attempt to do so causes warnings to be emitted, OTOH in many cases it is
   still OK to assume that all instances of the class has the same offsets
   for the same members).
 */

#define my_offsetof(TYPE, MEMBER) \
               ((size_t)((char *)&(((TYPE *)0x10)->MEMBER) - (char*)0x10))
#endif
#endif

typedef vector<CStdString> VECMOVIEYEARS;
typedef vector<CStdString> VECMOVIEACTORS;
typedef vector<CStdString> VECMOVIEGENRES;
typedef vector<CVideoInfoTag> VECMOVIES;
typedef vector<CStdString> VECMOVIESFILES;

#define VIDEO_SHOW_ALL 0
#define VIDEO_SHOW_UNWATCHED 1
#define VIDEO_SHOW_WATCHED 2

namespace VIDEO
{
  struct SScanSettings;
}

// these defines are based on how many columns we have and which column certain data is going to be in
// when we do GetDetailsForMovie()
#define VIDEODB_MAX_COLUMNS 21 // leave room for the fileid
#define VIDEODB_DETAILS_FILE VIDEODB_MAX_COLUMNS + 2
#define VIDEODB_DETAILS_PATH VIDEODB_MAX_COLUMNS + 3

#define VIDEODB_TYPE_STRING 1
#define VIDEODB_TYPE_INT 2
#define VIDEODB_TYPE_FLOAT 3
#define VIDEODB_TYPE_BOOL 4

typedef enum 
{
  VIDEODB_CONTENT_MOVIES = 1,
  VIDEODB_CONTENT_TVSHOWS = 2
} VIDEODB_CONTENT_IDS;

typedef enum // this enum MUST match the offset struct further down!! and make sure to keep min and max at -1 and sizeof(offsets)
{
  VIDEODB_ID_MIN = -1,
  VIDEODB_ID_TITLE = 0,
  VIDEODB_ID_PLOT = 1,
  VIDEODB_ID_PLOTOUTLINE = 2,
  VIDEODB_ID_TAGLINE = 3,
  VIDEODB_ID_VOTES = 4,
  VIDEODB_ID_RATING = 5,
  VIDEODB_ID_CREDITS = 6,
  VIDEODB_ID_YEAR = 7,
  VIDEODB_ID_THUMBURL = 8,
  VIDEODB_ID_IDENT = 9,
  VIDEODB_ID_WATCHED = 10,
  VIDEODB_ID_RUNTIME = 11,
  VIDEODB_ID_MPAA = 12,
  VIDEODB_ID_TOP250 = 13,
  VIDEODB_ID_GENRE = 14,
  VIDEODB_ID_DIRECTOR = 15,
  VIDEODB_ID_ORIGINALTITLE = 16,
  VIDEODB_ID_THUMBURL_SPOOF = 17,
  VIDEODB_ID_MAX
} VIDEODB_IDS;

const struct SDbTableOffsets
{
  int type;
  size_t offset;
} DbMovieOffsets[] = 
{
  { VIDEODB_TYPE_STRING, my_offsetof(CVideoInfoTag,m_strTitle) },
  { VIDEODB_TYPE_STRING, my_offsetof(CVideoInfoTag,m_strPlot) },
  { VIDEODB_TYPE_STRING, my_offsetof(CVideoInfoTag,m_strPlotOutline) },
  { VIDEODB_TYPE_STRING, my_offsetof(CVideoInfoTag,m_strTagLine) },
  { VIDEODB_TYPE_STRING, my_offsetof(CVideoInfoTag,m_strVotes) },
  { VIDEODB_TYPE_FLOAT, my_offsetof(CVideoInfoTag,m_fRating) },
  { VIDEODB_TYPE_STRING, my_offsetof(CVideoInfoTag,m_strWritingCredits) },
  { VIDEODB_TYPE_INT, my_offsetof(CVideoInfoTag,m_iYear) },
  { VIDEODB_TYPE_STRING, my_offsetof(CVideoInfoTag,m_strPictureURL.m_url) },
  { VIDEODB_TYPE_STRING, my_offsetof(CVideoInfoTag,m_strIMDBNumber) },
  { VIDEODB_TYPE_BOOL, my_offsetof(CVideoInfoTag,m_bWatched) },
  { VIDEODB_TYPE_STRING, my_offsetof(CVideoInfoTag,m_strRuntime) },
  { VIDEODB_TYPE_STRING, my_offsetof(CVideoInfoTag,m_strMPAARating) },
  { VIDEODB_TYPE_INT, my_offsetof(CVideoInfoTag,m_iTop250) },
  { VIDEODB_TYPE_STRING, my_offsetof(CVideoInfoTag,m_strGenre) },
  { VIDEODB_TYPE_STRING, my_offsetof(CVideoInfoTag,m_strDirector) },
  { VIDEODB_TYPE_STRING, my_offsetof(CVideoInfoTag,m_strOriginalTitle)},
  { VIDEODB_TYPE_STRING, my_offsetof(CVideoInfoTag,m_strPictureURL.m_spoof) }
};

typedef enum // this enum MUST match the offset struct further down!! and make sure to keep min and max at -1 and sizeof(offsets)
{
  VIDEODB_ID_TV_MIN = -1,
  VIDEODB_ID_TV_TITLE = 0,
  VIDEODB_ID_TV_PLOT = 1,
  VIDEODB_ID_TV_STATUS = 2,
  VIDEODB_ID_TV_VOTES = 3,
  VIDEODB_ID_TV_RATING = 4,
  VIDEODB_ID_TV_PREMIERED = 5,
  VIDEODB_ID_TV_THUMBURL = 6,
  VIDEODB_ID_TV_THUMBURL_SPOOF = 7,
  VIDEODB_ID_TV_GENRE = 8,
  VIDEODB_ID_TV_ORIGINALTITLE = 9,
  VIDEODB_ID_TV_EPISODEGUIDE = 10,
  VIDEODB_ID_TV_EPISODES = 11,
  VIDEODB_ID_TV_MAX
} VIDEODB_TV_IDS;

const struct SDbTableOffsets DbTvShowOffsets[] = 
{
  { VIDEODB_TYPE_STRING, my_offsetof(CVideoInfoTag,m_strTitle) },
  { VIDEODB_TYPE_STRING, my_offsetof(CVideoInfoTag,m_strPlot) },
  { VIDEODB_TYPE_STRING, my_offsetof(CVideoInfoTag,m_strStatus) },
  { VIDEODB_TYPE_STRING, my_offsetof(CVideoInfoTag,m_strVotes) },
  { VIDEODB_TYPE_FLOAT, my_offsetof(CVideoInfoTag,m_fRating) },
  { VIDEODB_TYPE_STRING, my_offsetof(CVideoInfoTag,m_strPremiered) },
  { VIDEODB_TYPE_STRING, my_offsetof(CVideoInfoTag,m_strPictureURL.m_url) },
  { VIDEODB_TYPE_STRING, my_offsetof(CVideoInfoTag,m_strPictureURL.m_spoof) },
  { VIDEODB_TYPE_STRING, my_offsetof(CVideoInfoTag,m_strGenre) },
  { VIDEODB_TYPE_STRING, my_offsetof(CVideoInfoTag,m_strOriginalTitle)},
  { VIDEODB_TYPE_STRING, my_offsetof(CVideoInfoTag,m_strEpisodeGuide)},
  { VIDEODB_TYPE_INT, my_offsetof(CVideoInfoTag,m_iEpisode) },
};

typedef enum // this enum MUST match the offset struct further down!! and make sure to keep min and max at -1 and sizeof(offsets)
{
  VIDEODB_ID_EPISODE_MIN = -1,
  VIDEODB_ID_EPISODE_TITLE = 0,
  VIDEODB_ID_EPISODE_PLOT = 1,
  VIDEODB_ID_EPISODE_VOTES = 2,
  VIDEODB_ID_EPISODE_RATING = 3,
  VIDEODB_ID_EPISODE_CREDITS = 4,
  VIDEODB_ID_EPISODE_AIRED = 5,
  VIDEODB_ID_EPISODE_THUMBURL = 6,
  VIDEODB_ID_EPISODE_THUMBURL_SPOOF = 7,
  VIDEODB_ID_EPISODE_WATCHED = 8,
  VIDEODB_ID_EPISODE_RUNTIME = 9,
  VIDEODB_ID_EPISODE_DIRECTOR = 10,
  VIDEODB_ID_EPISODE_IDENT = 11,
  VIDEODB_ID_EPISODE_SEASON = 12,
  VIDEODB_ID_EPISODE_EPISODE = 13,
  VIDEODB_ID_EPISODE_ORIGINALTITLE = 14,
  VIDEODB_ID_EPISODE_MAX
} VIDEODB_EPISODE_IDS;

const struct SDbTableOffsets DbEpisodeOffsets[] = 
{
  { VIDEODB_TYPE_STRING, my_offsetof(CVideoInfoTag,m_strTitle) },
  { VIDEODB_TYPE_STRING, my_offsetof(CVideoInfoTag,m_strPlot) },
  { VIDEODB_TYPE_STRING, my_offsetof(CVideoInfoTag,m_strVotes) },
  { VIDEODB_TYPE_FLOAT, my_offsetof(CVideoInfoTag,m_fRating) },
  { VIDEODB_TYPE_STRING, my_offsetof(CVideoInfoTag,m_strWritingCredits) },
  { VIDEODB_TYPE_STRING, my_offsetof(CVideoInfoTag,m_strFirstAired) },
  { VIDEODB_TYPE_STRING, my_offsetof(CVideoInfoTag,m_strPictureURL.m_url) },
  { VIDEODB_TYPE_STRING, my_offsetof(CVideoInfoTag,m_strPictureURL.m_spoof) },
  { VIDEODB_TYPE_BOOL, my_offsetof(CVideoInfoTag,m_bWatched) },
  { VIDEODB_TYPE_STRING, my_offsetof(CVideoInfoTag,m_strRuntime) },
  { VIDEODB_TYPE_STRING, my_offsetof(CVideoInfoTag,m_strDirector) },
  { VIDEODB_TYPE_STRING, my_offsetof(CVideoInfoTag,m_strProductionCode) },
  { VIDEODB_TYPE_INT, my_offsetof(CVideoInfoTag,m_iSeason) },
  { VIDEODB_TYPE_INT, my_offsetof(CVideoInfoTag,m_iEpisode) },
  { VIDEODB_TYPE_STRING, my_offsetof(CVideoInfoTag,m_strOriginalTitle)},
};

class CBookmark
{
public:
  CBookmark();
  double timeInSeconds;
  CStdString thumbNailImage;
  CStdString playerState;
  CStdString player;

  enum EType
  {
    STANDARD = 0,
    RESUME = 1,
  } type;
};

typedef vector<CBookmark> VECBOOKMARKS;

#define COMPARE_PERCENTAGE     0.90f // 90%
#define COMPARE_PERCENTAGE_MIN 0.50f // 50%


class CVideoDatabase : public CDatabase
{
public:
  CVideoDatabase(void);
  virtual ~CVideoDatabase(void);

  long AddMovie(const CStdString& strFilenameAndPath);
  long AddTvShow(const CStdString& strPath);
  long AddEpisode(long idShow, const CStdString& strFilenameAndPath);

  void MarkAsWatched(const CFileItem &item);
  void MarkAsWatched(long lMovieId, bool bEpisode=false);

  void MarkAsUnWatched(long lMovieId, bool bEpisode=false);
  void UpdateMovieTitle(long lMovieId, const CStdString& strNewMovieTitle, int iType=0); // 0=movie,1=episode,2=tvshow

  bool HasMovieInfo(const CStdString& strFilenameAndPath);
  bool HasTvShowInfo(const CStdString& strFilenameAndPath);
  bool HasEpisodeInfo(const CStdString& strFilenameAndPath);
  
  void DeleteDetailsForTvShow(const CStdString& strPath);

  void GetFilePath(long lMovieId, CStdString &filePath, int iType=0); // 0=movies, 1=episodes, 2=tvshows
  bool GetPathHash(const CStdString &path, CStdString &hash);
  bool GetPaths(map<CStdString,VIDEO::SScanSettings> &paths);

  void GetMovieInfo(const CStdString& strFilenameAndPath, CVideoInfoTag& details, long lMovieId = -1);
  void GetTvShowInfo(const CStdString& strPath, CVideoInfoTag& details, long lTvShowId = -1);
  bool GetEpisodeInfo(const CStdString& strFilenameAndPath, CVideoInfoTag& details, long lEpisodeId = -1);
  
  long GetMovieInfo(const CStdString& strFilenameAndPath);
  long GetTvShowInfo(const CStdString& strPath);
  long GetEpisodeInfo(const CStdString& strFilenameAndPath, long lEpisodeId=-1); // lEpisodeId is used for hinting due to two parters...

  void SetDetailsForMovie(const CStdString& strFilenameAndPath, const CVideoInfoTag& details);
  long SetDetailsForTvShow(const CStdString& strPath, const CVideoInfoTag& details);
  long SetDetailsForEpisode(const CStdString& strFilenameAndPath, const CVideoInfoTag& details, long idShow, long lEpisodeId=-1);

  void GetMoviesByActor(const CStdString& strActor, VECMOVIES& movies);
  void GetTvShowsByActor(const CStdString& strActor, VECMOVIES& movies);
  void GetEpisodesByActor(const CStdString& strActor, VECMOVIES& movies);

  void GetMovieGenresByName(const CStdString& strSearch, CFileItemList& items);
  void GetTvShowGenresByName(const CStdString& strSearch, CFileItemList& items);

  void GetMovieActorsByName(const CStdString& strSearch, CFileItemList& items);
  void GetTvShowsActorsByName(const CStdString& strSearch, CFileItemList& items);

  void GetMovieDirectorsByName(const CStdString& strSearch, CFileItemList& items);
  void GetTvShowsDirectorsByName(const CStdString& strSearch, CFileItemList& items);

  void GetMoviesByName(const CStdString& strSearch, CFileItemList& items);
  void GetTvShowsByName(const CStdString& strSearch, CFileItemList& items);
  void GetEpisodesByName(const CStdString& strSearch, CFileItemList& items);

  void GetEpisodesByPlot(const CStdString& strSearch, CFileItemList& items);

  void GetBookMarksForFile(const CStdString& strFilenameAndPath, VECBOOKMARKS& bookmarks, CBookmark::EType type = CBookmark::STANDARD);
  void AddBookMarkToFile(const CStdString& strFilenameAndPath, const CBookmark &bookmark, CBookmark::EType type = CBookmark::STANDARD);
  bool GetResumeBookMark(const CStdString& strFilenameAndPath, CBookmark &bookmark);
  void ClearBookMarkOfFile(const CStdString& strFilenameAndPath, CBookmark& bookmark, CBookmark::EType type = CBookmark::STANDARD);
  void ClearBookMarksOfFile(const CStdString& strFilenameAndPath, CBookmark::EType type = CBookmark::STANDARD);

  void RemoveContentForPath(const CStdString& strPath,CGUIDialogProgress *progress = NULL);

  void DeleteMovie(const CStdString& strFilenameAndPath);
  void DeleteTvShow(const CStdString& strPath);
  void DeleteEpisode(const CStdString& strFilenameAndPath, long lEpisodeId=-1);

  int GetRecentMovies(long* pMovieIdArray, int nSize);

  bool GetVideoSettings(const CStdString &strFilenameAndPath, CVideoSettings &settings);
  void SetVideoSettings(const CStdString &strFilenameAndPath, const CVideoSettings &settings);
  void EraseVideoSettings();

  bool GetStackTimes(const CStdString &filePath, vector<long> &times);
  void SetStackTimes(const CStdString &filePath, vector<long> &times);
  void SetScraperForPath(const CStdString& filePath, const CStdString& strScraper, const CStdString& strContent, bool bUseFolderNames=false, bool bScanRecursive=false);
  bool SetPathHash(const CStdString &path, const CStdString &hash);

  bool GetArbitraryQuery(const CStdString& strQuery, const CStdString& strOpenRecordSet, const CStdString& strCloseRecordSet, 
                         const CStdString& strOpenRecord, const CStdString& strCloseRecord, const CStdString& strOpenField, const CStdString& strCloseField, CStdString& strResult);

  bool GetGenresNav(const CStdString& strBaseDir, CFileItemList& items, long idContent=-1);
  bool GetActorsNav(const CStdString& strBaseDir, CFileItemList& items, long idContent=-1);
  bool GetDirectorsNav(const CStdString& strBaseDir, CFileItemList& items, long idContent=-1);
  bool GetTitlesNav(const CStdString& strBaseDir, CFileItemList& items, long idGenre=-1, long idYear=-1, long idActor=-1, long idDirector=-1);
  bool GetTvShowsNav(const CStdString& strBaseDir, CFileItemList& items, long idGenre=-1, long idYear=-1, long idActor=-1, long idDirector=-1);
  bool GetYearsNav(const CStdString& strBaseDir, CFileItemList& items, long idContent=-1);
  bool GetSeasonsNav(const CStdString& strBaseDir, CFileItemList& items, long idActor=-1, long idDirector=-1, long idGenre=-1, long idYear=-1, long idShow=-1);
  bool GetEpisodesNav(const CStdString& strBaseDir, CFileItemList& items, long idGenre=-1, long idYear=-1, long idActor=-1, long idDirector=-1, long idShow=-1, long idSeason=-1);
  
  int GetMovieCount();
  int GetTvShowCount();

  bool GetGenreById(long lIdGenre, CStdString& strGenre);

  bool GetScraperForPath(const CStdString& strPath, CStdString& strScraper, CStdString& strContent);
  bool GetScraperForPath(const CStdString& strPath, CStdString& strScraper, CStdString& strContent, int& iFound);
  bool GetScraperForPath(const CStdString& strPath, CStdString& strScraper, CStdString& strContent, bool& bUseFolderNames, bool& bScanRecursive, int& iFound);

  void CleanDatabase();
  
  long AddFile(const CStdString& strFileName);
  void ExportToXML(const CStdString &xmlFile);
  void ImportFromXML(const CStdString &xmlFile);

protected:
  long GetPath(const CStdString& strPath);
  long GetFile(const CStdString& strFilenameAndPath);

  long AddPath(const CStdString& strPath);
  long AddGenre(const CStdString& strGenre1);
  long AddActor(const CStdString& strActor);

  void AddActorToMovie(long lMovieId, long lActorId, const CStdString& strRole);
  void AddActorToTvShow(long lTvShowId, long lActorId, const CStdString& strRole);
  void AddActorToEpisode(long lEpisode, long lActorId, const CStdString& strRole);

  void AddDirectorToMovie(long lMovieId, long lDirectorId);
  void AddDirectorToTvShow(long lTvShowId, long lDirectorId);
  void AddDirectorToEpisode(long lEpisodeId, long lDirectorId);

  void AddGenreToMovie(long lMovieId, long lGenreId);
  void AddGenreToTvShow(long lTvShowId, long lGenreId);
  void AddGenreToEpisode(long lEpisodeId, long lGenreId);

  void AddGenreAndDirectors(const CVideoInfoTag& details, vector<long>& vecDirectors, vector<long>& vecGenres);

  CVideoInfoTag GetDetailsForMovie(auto_ptr<Dataset> &pDS, bool needsCast = false);
  CVideoInfoTag GetDetailsForTvShow(auto_ptr<Dataset> &pDS, bool needsCast = false);
  CVideoInfoTag GetDetailsForEpisode(auto_ptr<Dataset> &pDS, bool needsCast = false);

private:
  virtual bool CreateTables();
  virtual bool UpdateOldVersion(int version);
};
