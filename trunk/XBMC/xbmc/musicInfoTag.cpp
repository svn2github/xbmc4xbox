#include "stdafx.h"
#include "musicInfoTag.h"
#include "util.h"
#include "musicdatabase.h"

using namespace MUSIC_INFO;

CMusicInfoTag::CMusicInfoTag(void)
{
  Clear();
}

CMusicInfoTag::CMusicInfoTag(const CMusicInfoTag& tag)
{
  *this = tag;
}

CMusicInfoTag::~CMusicInfoTag()
{}

const CMusicInfoTag& CMusicInfoTag::operator =(const CMusicInfoTag& tag)
{
  if (this == &tag) return * this;

  m_strURL = tag.m_strURL;
  m_strArtist = tag.m_strArtist;
  m_strAlbum = tag.m_strAlbum;
  m_strGenre = tag.m_strGenre;
  m_strTitle = tag.m_strTitle;
  m_iDuration = tag.m_iDuration;
  m_iTrack = tag.m_iTrack;
  m_bLoaded = tag.m_bLoaded;
  memcpy(&m_dwReleaseDate, &tag.m_dwReleaseDate, sizeof(m_dwReleaseDate) );
  return *this;
}

int CMusicInfoTag::GetTrackNumber() const
{
  return m_iTrack;
}

int CMusicInfoTag::GetDuration() const
{
  return m_iDuration;
}

const CStdString& CMusicInfoTag::GetTitle() const
{
  return m_strTitle;
}

const CStdString& CMusicInfoTag::GetURL() const
{
  return m_strURL;
}

const CStdString& CMusicInfoTag::GetArtist() const
{
  return m_strArtist;
}

const CStdString& CMusicInfoTag::GetAlbum() const
{
  return m_strAlbum;
}

const CStdString& CMusicInfoTag::GetGenre() const
{
  return m_strGenre;
}

void CMusicInfoTag::GetReleaseDate(SYSTEMTIME& dateTime) const
{
  memcpy(&dateTime, &m_dwReleaseDate, sizeof(m_dwReleaseDate) );
}

CStdString CMusicInfoTag::GetYear() const
{
  CStdString strReturn;
  strReturn.Format("%i", m_dwReleaseDate.wYear);
  return m_dwReleaseDate.wYear > 1900 ? strReturn : "";
}

void CMusicInfoTag::SetTitle(const CStdString& strTitle)
{
  m_strTitle = strTitle;
  m_strTitle.TrimLeft(" ");
  m_strTitle.TrimRight(" ");
  m_strTitle.TrimRight("\r\n");
}

void CMusicInfoTag::SetURL(const CStdString& strURL)
{
  m_strURL = strURL;
}

void CMusicInfoTag::SetArtist(const CStdString& strArtist)
{
  m_strArtist = strArtist;
  m_strArtist.TrimLeft(" ");
  m_strArtist.TrimRight(" ");
  m_strArtist.TrimRight("\r\n");
}

void CMusicInfoTag::SetAlbum(const CStdString& strAlbum)
{
  m_strAlbum = strAlbum;
  m_strAlbum.TrimLeft(" ");
  m_strAlbum.TrimRight(" ");
}

void CMusicInfoTag::SetGenre(const CStdString& strGenre)
{
  m_strGenre = strGenre;
  m_strGenre.TrimLeft(" ");
  m_strGenre.TrimRight(" ");
  m_strGenre.TrimRight("\r\n");
}

void CMusicInfoTag::SetReleaseDate(SYSTEMTIME& dateTime)
{
  memcpy(&m_dwReleaseDate, &dateTime, sizeof(m_dwReleaseDate) );
}

void CMusicInfoTag::SetTrackNumber(int iTrack)
{
  m_iTrack = iTrack;
}

void CMusicInfoTag::SetDuration(int iSec)
{
  m_iDuration = iSec;
}

void CMusicInfoTag::SetLoaded(bool bOnOff)
{
  m_bLoaded = bOnOff;
}

bool CMusicInfoTag::Loaded() const
{
  return m_bLoaded;
}

bool CMusicInfoTag::Load(const CStdString& strFileName)
{
  FILE* fd = fopen(strFileName.c_str(), "rb");
  if (fd)
  {
    CUtil::LoadString(m_strURL, fd);
    CUtil::LoadString(m_strTitle, fd);
    CUtil::LoadString(m_strArtist, fd);
    CUtil::LoadString(m_strAlbum, fd);
    CUtil::LoadString(m_strGenre, fd);
    m_iDuration = CUtil::LoadInt(fd);
    m_iTrack = CUtil::LoadInt(fd);
    CUtil::LoadDateTime(m_dwReleaseDate, fd);
    SetLoaded(true);
    fclose(fd);
    return true;
  }
  return false;
}

void CMusicInfoTag::Save(const CStdString& strFileName)
{
  FILE* fd = fopen(strFileName.c_str(), "wb+");
  if (fd)
  {
    CUtil::SaveString(m_strURL, fd);
    CUtil::SaveString(m_strTitle, fd);
    CUtil::SaveString(m_strArtist, fd);
    CUtil::SaveString(m_strAlbum, fd);
    CUtil::SaveString(m_strGenre, fd);
    CUtil::SaveInt(m_iDuration, fd);
    CUtil::SaveInt(m_iTrack, fd);
    CUtil::SaveDateTime(m_dwReleaseDate, fd);
    fclose(fd);
  }
}

void CMusicInfoTag::SetAlbum(const CAlbum& album)
{
  SetArtist(album.strArtist);
  SetAlbum(album.strAlbum);
  m_strURL = album.strPath;
  m_bLoaded = true;
}

void CMusicInfoTag::SetSong(const CSong& song)
{
  SetTitle(song.strTitle);
  SetGenre(song.strGenre);
  SetArtist(song.strArtist);
  SetAlbum(song.strAlbum);
  m_strURL = song.strFileName;
  SYSTEMTIME stTime;
  stTime.wYear = song.iYear;
  SetReleaseDate(stTime);
  m_iTrack = song.iTrack;
  m_iDuration = song.iDuration;
  m_bLoaded = true;
}

void CMusicInfoTag::Serialize(CArchive& ar)
{
  if (ar.IsStoring())
  {
    ar << m_strURL;
    ar << m_strTitle;
    ar << m_strArtist;
    ar << m_strAlbum;
    ar << m_strGenre;
    ar << m_iDuration;
    ar << m_iTrack;
    ar << m_bLoaded;
    ar << m_dwReleaseDate;
  }
  else
  {
    ar >> m_strURL;
    ar >> m_strTitle;
    ar >> m_strArtist;
    ar >> m_strAlbum;
    ar >> m_strGenre;
    ar >> m_iDuration;
    ar >> m_iTrack;
    ar >> m_bLoaded;
    ar >> m_dwReleaseDate;
  }
}

void CMusicInfoTag::Clear()
{
  m_strURL = "";
  m_strArtist = "";
  m_strAlbum = "";
  m_strGenre = "";
  m_strTitle = "";
  m_iDuration = 0;
  m_iTrack = 0;
  m_bLoaded = false;
  memset(&m_dwReleaseDate, 0, sizeof(m_dwReleaseDate) );
}
