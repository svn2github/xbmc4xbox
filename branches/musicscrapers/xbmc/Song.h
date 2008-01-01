/*!
 \file MusicDatabase.h
\brief
*/
#pragma once

#include <map>
#include <vector>

#include "MusicInfotag.h"
#include "utils/IMDB.h"

using namespace MUSIC_INFO;


/*!
 \ingroup music
 \brief Class to store and read album information from CMusicDatabase
 \sa CSong, CMusicDatabase
 */
class CAlbum
{
public:
  CAlbum() { idAlbum = 0; iRating = 0; iYear = 0; };
  bool operator<(const CAlbum &a) const
  {
    return strAlbum + strArtist < a.strAlbum + a.strArtist;
  }
  long idAlbum;
  CStdString strAlbum;
  CStdString strArtist;
  CStdString strGenre;
  CScraperUrl thumbURL;  
  CStdString strMoods;
  CStdString strStyles;
  CStdString strReview;
  int iRating;
  int iYear;
};

class CArtist
{
public:
  long idArtist;
  bool operator<(const CArtist& a) const
  {
    return strArtist < a.strArtist;
  }

  void Reset()
  {
    strArtist.Empty();
    strGenre.Empty();
    strBiography.Empty();
    strStyles.Empty();
    strMoods.Empty();
    strInstruments.Empty();
    strBorn.Empty();
    strFormed.Empty();
    thumbURL.Clear();
    discography.clear();
    idArtist = -1;
  }
  CStdString strArtist;
  CStdString strGenre;
  CStdString strBiography;
  CStdString strStyles;
  CStdString strMoods;
  CStdString strInstruments;
  CStdString strBorn;
  CStdString strFormed;
  CScraperUrl thumbURL;
  std::vector<std::pair<CStdString,CStdString> > discography;
};

class CGenre
{
public:
  long idGenre;
  CStdString strGenre;
};

/*!
 \ingroup music
 \brief Class to store and read song information from CMusicDatabase
 \sa CAlbum, CMusicDatabase
 */
class CSong
{
public:
  CSong() ;
  CSong(CMusicInfoTag& tag);
  virtual ~CSong(){};
  void Clear() ;

  bool operator<(const CSong &song) const
  {
    if (strFileName < song.strFileName) return true;
    if (strFileName > song.strFileName) return false;
    if (iTrack < song.iTrack) return true;
    return false;
  }
  long idSong;
  CStdString strFileName;
  CStdString strTitle;
  CStdString strArtist;
  CStdString strAlbum;
  CStdString strAlbumArtist;
  CStdString strGenre;
  CStdString strThumb;
  CStdString strMusicBrainzTrackID;
  CStdString strMusicBrainzArtistID;
  CStdString strMusicBrainzAlbumID;
  CStdString strMusicBrainzAlbumArtistID;
  CStdString strMusicBrainzTRMID;
  CStdString strComment;
  char rating;
  int iTrack;
  int iDuration;
  int iYear;
  int iTimesPlayed;
  CStdString lastPlayed;
  int iStartOffset;
  int iEndOffset;
};

/*!
 \ingroup music
 \brief A map of CSong objects, used for CMusicDatabase
 */
class CSongMap
{
public:
  CSongMap();

  CSong *Find(const CStdString &file);
  void Add(const CStdString &file, const CSong &song);
  void Clear();
  int Size();

private:
  std::map<CStdString, CSong> m_map;
};

/*!
 \ingroup music
 \brief A vector of CSong objects, used for CMusicDatabase
 \sa CMusicDatabase
 */
typedef std::vector<CSong> VECSONGS;

/*!
 \ingroup music
 \brief A vector of CStdString objects, used for CMusicDatabase
 */
typedef std::vector<CArtist> VECARTISTS;

/*!
 \ingroup music
 \brief A vector of CStdString objects, used for CMusicDatabase
 \sa CMusicDatabase
 */
typedef std::vector<CGenre> VECGENRES;

/*!
 \ingroup music
 \brief A vector of CAlbum objects, used for CMusicDatabase
 \sa CMusicDatabase
 */
typedef std::vector<CAlbum> VECALBUMS;
