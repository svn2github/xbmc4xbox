/*!
 \file MusicDatabase.h
\brief
*/
#pragma once
#include "Database.h"

#include <set>

// return codes of Cleaning up the Database
// numbers are strings from strings.xml
#define ERROR_OK     317
#define ERROR_CANCEL    0
#define ERROR_DATABASE    315
#define ERROR_REORG_SONGS   319
#define ERROR_REORG_ARTIST   321
#define ERROR_REORG_GENRE   323
#define ERROR_REORG_PATH   325
#define ERROR_REORG_ALBUM   327
#define ERROR_WRITING_CHANGES  329
#define ERROR_COMPRESSING   332

#define NUM_SONGS_BEFORE_COMMIT 500

/*!
 \ingroup music
 \brief A set of CStdString objects, used for CMusicDatabase
 \sa ISETPATHES, CMusicDatabase
 */
typedef std::set<CStdString> SETPATHES;

/*!
 \ingroup music
 \brief The SETPATHES iterator
 \sa SETPATHES, CMusicDatabase
 */
typedef std::set<CStdString>::iterator ISETPATHES;

/*!
 \ingroup music
 \brief A vector of longs for iDs, used for CMusicDatabase's multiple artist/genre capability
 */
typedef std::vector<long> VECLONGS;

class CGUIDialogProgress;

/*!
 \ingroup music
 \brief Class to store and read tag information
 
 CMusicDatabase can be used to read and store
 tag information for faster access. It is based on
 sqlite (http://www.sqlite.org).
 
 Here is the database layout:
  \image html musicdatabase.png
 
 \sa CAlbum, CSong, VECSONGS, CMapSong, VECARTISTS, VECALBUMS, VECGENRES
 */
class CMusicDatabase : public CDatabase
{
  class CArtistCache
  {
  public:
    long idArtist;
    CStdString strArtist;
  };

  class CPathCache
  {
  public:
    long idPath;
    CStdString strPath;
  };

  class CGenreCache
  {
  public:
    long idGenre;
    CStdString strGenre;
  };

class CAlbumCache : public CAlbum
  {
  public:
    long idAlbum;
    long idArtist;
  };

public:
  CMusicDatabase(void);
  virtual ~CMusicDatabase(void);
  void EmptyCache();
  void Clean();
  int  Cleanup(CGUIDialogProgress *pDlgProgress);
  void DeleteAlbumInfo();
  bool LookupCDDBInfo(bool bRequery=false);
  void DeleteCDDBInfo();
  void AddSong(const CSong& song, bool bCheck = true);
  long AddAlbumInfo(const CAlbum& album, const VECSONGS& songs);
  long UpdateAlbumInfo(const CAlbum& album, const VECSONGS& songs);
  bool GetAlbumInfo(const CStdString& strAlbum, const CStdString& strPath, CAlbum& album, VECSONGS& songs);
  bool GetSong(const CStdString& strTitle, CSong& song);
  bool GetSongByFileName(const CStdString& strFileName, CSong& song);
  bool GetSongById(long idSong, CSong& song);
  bool GetSongsByPath(const CStdString& strPath, VECSONGS& songs);
  bool GetSongsByPath(const CStdString& strPath, CSongMap& songs, bool bAppendToMap = false);
//  bool GetSongsByAlbum(const CStdString& strAlbum, const CStdString& strPath, VECSONGS& songs);
  bool FindSongsByNameAndArtist(const CStdString& strSearch, VECSONGS& songs);
  bool Search(const CStdString& search, CFileItemList &items);

  bool GetGenresByName(const CStdString& strGenre1, VECGENRES& genres);

  bool GetAlbumsByPath(const CStdString& strPath, VECALBUMS& albums);
  bool GetArbitraryQuery(const CStdString& strQuery, const CStdString& strOpenRecordSet, const CStdString& strCloseRecordSet, 
	const CStdString& strOpenRecord, const CStdString& strCloseRecord, const CStdString& strOpenField, const CStdString& strCloseField, CStdString& strResult);
  bool FindAlbumsByName(const CStdString& strSearch, VECALBUMS& albums);
  bool GetTop100(const CStdString& strBaseDir, CFileItemList& items);
  bool GetTop100Albums(VECALBUMS& albums);
  bool GetTop100AlbumSongs(const CStdString& strBaseDir, CFileItemList& item);
  bool GetRecentlyAddedAlbums(VECALBUMS& albums);
  bool GetRecentlyAddedAlbumSongs(const CStdString& strBaseDir, CFileItemList& item);
  bool GetRecentlyPlayedAlbums(VECALBUMS& albums);
  bool GetRecentlyPlayedAlbumSongs(const CStdString& strBaseDir, CFileItemList& item);
  bool IncrTop100CounterByFileName(const CStdString& strFileName1);
  bool GetSubpathsFromPath(const CStdString &strPath, CStdString& strPathIds);
  bool RemoveSongsFromPaths(const CStdString &strPathIds);
  bool CleanupAlbumsArtistsGenres();
  bool GetGenresNav(const CStdString& strBaseDir, CFileItemList& items);
  bool GetArtistsNav(const CStdString& strBaseDir, CFileItemList& items, long idGenre);
  bool GetAlbumsNav(const CStdString& strBaseDir, CFileItemList& items, long idGenre, long idArtist);
  bool GetSongsNav(const CStdString& strBaseDir, CFileItemList& items, long idGenre, long idArtist,long idAlbum);
  bool GetSongsByWhere(const CStdString &whereClause, CFileItemList& items);
  bool GetRandomSong(CFileItem* item, long& lSongId, const CStdString& strWhere);
  int GetSongsCount();
  int GetSongsCount(const CStdString& strWhere);
  unsigned int GetSongIDs(const CStdString& strWhere, vector<long> &songIDs);

//  bool GetPathFromAlbumId(long idAlbum, CStdString& strPath);
//  bool GetPathFromSongId(long idSong, CStdString& strPath);
  bool SaveAlbumThumb(const CStdString& strAlbum, const CStdString& strPath, const CStdString& strThumb);
  bool RefreshMusicDbThumbs(CFileItem* pItem, CFileItemList &items);
  bool GetArtistPath(long idArtist, CStdString &path);
  bool InitialisePartyMode();
  bool UpdatePartyMode(long lSongId, bool bRelaxRestrictions);

  bool GetGenreById(long idGenre, CStdString& strGenre);
  bool GetArtistById(long idArtist, CStdString& strArtist);
  bool GetAlbumById(long idAlbum, CStdString& strAlbum);

  bool GetVariousArtistsAlbums(const CStdString& strBaseDir, CFileItemList& items);
  bool GetVariousArtistsAlbumsSongs(const CStdString& strBaseDir, CFileItemList& items);

protected:
  map<CStdString, int /*CArtistCache*/> m_artistCache;
  map<CStdString, int /*CGenreCache*/> m_genreCache;
  map<CStdString, int /*CPathCache*/> m_pathCache;
  map<CStdString, int /*CPathCache*/> m_thumbCache;
  map<CStdString, CAlbumCache> m_albumCache;
  virtual bool CreateTables();
  long AddAlbum(const CStdString& strAlbum, long lArtistId, int iNumArtists, const CStdString& strArtist, long idThumb, long idGenre, int numGenres, long year);
  long AddGenre(const CStdString& strGenre);
  long AddArtist(const CStdString& strArtist);
  long AddPath(const CStdString& strPath);
  long AddThumb(const CStdString& strThumb1);
  void AddExtraAlbumArtists(const CStdStringArray& vecArtists, long lAlbumId);
  void AddExtraSongArtists(const CStdStringArray& vecArtists, long lSongId, bool bCheck = true);
  void AddExtraGenres(const CStdStringArray& vecGenres, long lSongId, long lAlbumId, bool bCheck = true);
  bool AddAlbumInfoSongs(long idAlbumInfo, const VECSONGS& songs);
  bool GetAlbumInfoSongs(long idAlbumInfo, VECSONGS& songs);
  bool UpdateAlbumInfoSongs(long idAlbumInfo, const VECSONGS& songs);

private:
  bool GetExtraArtistsForAlbum(long lAlbumId, CStdString &strArtist);
  bool GetExtraArtistsForSong(long lSongId, CStdString &strArtist);
  bool GetExtraGenresForAlbum(long lAlbumId, CStdString &strGenre);
  bool GetExtraGenresForSong(long lSongId, CStdString &strGenre);
  CSong GetSongFromDataset(bool bWithMusicDbPath=false);
  CAlbum GetAlbumFromDataset();
  void GetFileItemFromDataset(CFileItem* item, const CStdString& strMusicDBbasePath);
  bool CleanupSongs();
  bool CleanupSongsByIds(const CStdString &strSongIds);
  bool CleanupPaths();
  bool CleanupThumbs();
  bool CleanupAlbums();
  bool CleanupArtists();
  bool CleanupGenres();
  virtual bool UpdateOldVersion(int version);
  bool SearchArtists(const CStdString& search, CFileItemList &artists);
  bool SearchAlbums(const CStdString& search, CFileItemList &albums);
  bool SearchSongs(const CStdString& strSearch, CFileItemList &songs);

  // Fields should be ordered as they 
  // appear in the songview
  enum _SongFields
  {
    song_idSong=0,
    song_iNumArtists,
    song_iNumGenres,
    song_strTitle,
    song_iTrack,
    song_iDuration,
    song_iYear,
    song_dwFileNameCRC,
    song_strFileName,
    song_strMusicBrainzTrackID,
    song_strMusicBrainzArtistID,
    song_strMusicBrainzAlbumID,
    song_strMusicBrainzAlbumArtistID,
    song_strMusicBrainzTRMID,
    song_iTimesPlayed,
    song_iStartOffset,
    song_iEndOffset,
    song_lastplayed,
    song_idAlbum,
    song_strAlbum,
    song_strPath,
    song_idArtist,
    song_strArtist,
    song_idGenre,
    song_strGenre,
    song_strThumb
  } SongFields;

  // Fields should be ordered as they 
  // appear in the albumview
  enum _AlbumFields
  {
    album_idAlbum=0,
    album_strAlbum, 
    album_iNumArtists, 
    album_idArtist, 
    album_iNumGenres,
    album_idGenre,
    album_strArtist,
    album_strGenre,
    album_iYear,
    album_strThumb
  } AlbumFields;

  int m_iSongsBeforeCommit;
};
