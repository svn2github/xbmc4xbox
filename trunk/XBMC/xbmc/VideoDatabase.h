#pragma once
#include "Database.h"
#include "utils\IMDB.h"
#include "settings/VideoSettings.h"

typedef vector<CStdString> VECMOVIEYEARS;
typedef vector<CStdString> VECMOVIEACTORS;
typedef vector<CStdString> VECMOVIEGENRES;
typedef vector<CIMDBMovie> VECMOVIES;
typedef vector<CStdString> VECMOVIESFILES;

#define VIDEO_SHOW_ALL 0
#define VIDEO_SHOW_UNWATCHED 1
#define VIDEO_SHOW_WATCHED 2

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
  VIDEODB_ID_MAX
} VIDEODB_IDS;

// these defines are based on how many columns we have and which column certain data is going to be in
// when we do GetDetailsForMovie()
#define VIDEODB_MAX_COLUMNS 21
#define VIDEODB_DETAILS_FILE VIDEODB_MAX_COLUMNS + 1
#define VIDEODB_DETAILS_PATH VIDEODB_MAX_COLUMNS + 2

#define VIDEODB_TYPE_STRING 1
#define VIDEODB_TYPE_INT 2
#define VIDEODB_TYPE_FLOAT 3
#define VIDEODB_TYPE_BOOL 4

const struct SDbMovieOffsets
{
  int type;
  size_t offset;
} DbMovieOffsets[] = 
{
  { VIDEODB_TYPE_STRING, offsetof(CIMDBMovie,m_strTitle) },
  { VIDEODB_TYPE_STRING, offsetof(CIMDBMovie,m_strPlot) },
  { VIDEODB_TYPE_STRING, offsetof(CIMDBMovie,m_strPlotOutline) },
  { VIDEODB_TYPE_STRING, offsetof(CIMDBMovie,m_strTagLine) },
  { VIDEODB_TYPE_STRING, offsetof(CIMDBMovie,m_strVotes) },
  { VIDEODB_TYPE_FLOAT, offsetof(CIMDBMovie,m_fRating) },
  { VIDEODB_TYPE_STRING, offsetof(CIMDBMovie,m_strWritingCredits) },
  { VIDEODB_TYPE_INT, offsetof(CIMDBMovie,m_iYear) },
  { VIDEODB_TYPE_STRING, offsetof(CIMDBMovie,m_strPictureURL) },
  { VIDEODB_TYPE_STRING, offsetof(CIMDBMovie,m_strIMDBNumber) },
  { VIDEODB_TYPE_BOOL, offsetof(CIMDBMovie,m_bWatched) },
  { VIDEODB_TYPE_STRING, offsetof(CIMDBMovie,m_strRuntime) },
  { VIDEODB_TYPE_STRING, offsetof(CIMDBMovie,m_strMPAARating) },
  { VIDEODB_TYPE_INT, offsetof(CIMDBMovie,m_iTop250) },
  { VIDEODB_TYPE_STRING, offsetof(CIMDBMovie,m_strGenre) },
  { VIDEODB_TYPE_STRING, offsetof(CIMDBMovie,m_strDirector) }
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

  void MarkAsWatched(const CFileItem &item);
  void MarkAsWatched(long lMovieId);

  void MarkAsUnWatched(long lMovieId);
  void UpdateMovieTitle(long lMovieId, const CStdString& strNewMovieTitle);

  bool HasMovieInfo(const CStdString& strFilenameAndPath);
  bool HasSubtitle(const CStdString& strFilenameAndPath);
  void DeleteMovieInfo(const CStdString& strFileNameAndPath);

  void GetFilePath(long lMovieId, CStdString &filePath);
  void GetMovieInfo(const CStdString& strFilenameAndPath, CIMDBMovie& details, long lMovieId = -1);
  long GetMovieInfo(const CStdString& strFilenameAndPath);
  void SetMovieInfo(const CStdString& strFilenameAndPath, CIMDBMovie& details);

  void GetMoviesByPath(const CStdString& strPath1, VECMOVIES& movies);
  void GetMoviesByActor(const CStdString& strActor, VECMOVIES& movies);

  void GetGenresByName(const CStdString& strSearch, CFileItemList& items);
  void GetActorsByName(const CStdString& strSearch, CFileItemList& items);
  void GetDirectorsByName(const CStdString& strSearch, CFileItemList& items);
  void GetTitlesByName(const CStdString& strSearch, CFileItemList& items);

  void GetBookMarksForFile(const CStdString& strFilenameAndPath, VECBOOKMARKS& bookmarks, CBookmark::EType type = CBookmark::STANDARD);
  void AddBookMarkToFile(const CStdString& strFilenameAndPath, const CBookmark &bookmark, CBookmark::EType type = CBookmark::STANDARD);
  bool GetResumeBookMark(const CStdString& strFilenameAndPath, CBookmark &bookmark);
  void ClearBookMarkOfFile(const CStdString& strFilenameAndPath, CBookmark& bookmark, CBookmark::EType type = CBookmark::STANDARD);
  void ClearBookMarksOfFile(const CStdString& strFilenameAndPath, CBookmark::EType type = CBookmark::STANDARD);

  void RemoveContentForPath(const CStdString& strPath);
  void DeleteMovie(const CStdString& strFilenameAndPath);
  int GetRecentMovies(long* pMovieIdArray, int nSize);

  bool GetVideoSettings(const CStdString &strFilenameAndPath, CVideoSettings &settings);
  void SetVideoSettings(const CStdString &strFilenameAndPath, const CVideoSettings &settings);
  void EraseVideoSettings();

  bool GetStackTimes(const CStdString &filePath, vector<long> &times);
  void SetStackTimes(const CStdString &filePath, vector<long> &times);
  void SetScraperForPath(const CStdString& filePath, const CStdString& strScraper, const CStdString& strContent);

  bool GetGenresNav(const CStdString& strBaseDir, CFileItemList& items);
  bool GetActorsNav(const CStdString& strBaseDir, CFileItemList& items);
  bool GetDirectorsNav(const CStdString& strBaseDir, CFileItemList& items);
  bool GetTitlesNav(const CStdString& strBaseDir, CFileItemList& items, long idGenre=-1, long idYear=-1, long idActor=-1, long idDirector=-1);
  bool GetYearsNav(const CStdString& strBaseDir, CFileItemList& items);
  bool GetGenreById(long lIdGenre, CStdString& strGenre);
  int GetMovieCount();
  bool GetScraperForPath(const CStdString& strPath, CStdString& strScraper, CStdString& strContent);
  bool GetScraperForPath(const CStdString& strPath, CStdString& strScraper, CStdString& strContent, int& iFound);

  void CleanDatabase();
  
  long AddFile(const CStdString& strFileName);
  void ExportToXML(const CStdString &xmlFile);
  void ImportFromXML(const CStdString &xmlFile);

protected:
  long GetPath(const CStdString& strPath);
  long GetFile(const CStdString& strFilenameAndPath, long& lMovieId, long& lEpisodeId, bool bExact = false);

  long GetMovie(const CStdString& strFilenameAndPath);

  long AddPath(const CStdString& strPath);
  long AddGenre(const CStdString& strGenre1);
  long AddActor(const CStdString& strActor);

  void AddActorToMovie(long lMovieId, long lActorId, const CStdString& strRole);
  void AddDirectorToMovie(long lMovieId, long lDirectorId);
  void AddGenreToMovie(long lMovieId, long lGenreId);

  CIMDBMovie GetDetailsForMovie(auto_ptr<Dataset> &pDS, bool needsCast = false);

private:
  virtual bool CreateTables();
  virtual bool UpdateOldVersion(int version);

};
