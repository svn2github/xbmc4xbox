#include ".\musicalbuminfo.h"
#include ".\htmltable.h"
#include ".\htmlutil.h"
#include ".\http.h"
#include "../util.h"



using namespace MUSIC_GRABBER;
using namespace HTML;
CMusicAlbumInfo::CMusicAlbumInfo(void)
{
	m_strArtist="";
	m_strTitle="";
	m_strDateOfRelease="";
	m_strGenre="";
	m_strTones="";
	m_strStyles="";
	m_strReview="";
	m_strImageURL="";
	m_strTitle2="";
	m_iRating=0;
	m_bLoaded=false;
}

CMusicAlbumInfo::~CMusicAlbumInfo(void)
{
}


CMusicAlbumInfo::CMusicAlbumInfo(const CStdString& strAlbumInfo, const CStdString& strAlbumURL)
{
	m_strArtist="";
	m_strDateOfRelease="";
	m_strGenre="";
	m_strTones="";
	m_strStyles="";
	m_strReview="";
	m_strTitle2="";
	m_iRating=0;
	m_bLoaded=false;
	m_strTitle2		= strAlbumInfo;
	m_strAlbumURL	= strAlbumURL;
}

const CStdString& CMusicAlbumInfo::GetAlbumURL() const
{
	return m_strAlbumURL;
}
const CStdString& CMusicAlbumInfo::GetArtist() const
{
	return m_strArtist;
}

const CStdString& CMusicAlbumInfo::GetTitle() const
{
	return m_strTitle;
}
const CStdString& CMusicAlbumInfo::GetTitle2() const
{
	return m_strTitle2;
}


const CStdString& CMusicAlbumInfo::GetDateOfRelease() const
{
	return m_strDateOfRelease;
}


const CStdString& CMusicAlbumInfo::GetGenre() const
{
	return m_strGenre;
}

const CStdString& CMusicAlbumInfo::GetTones() const
{
	return m_strTones;
}

const CStdString& CMusicAlbumInfo::GetStyles() const
{
	return m_strStyles;
}

const CStdString& CMusicAlbumInfo::GetReview() const
{
	return m_strReview;
}

const CStdString& CMusicAlbumInfo::GetImageURL() const
{
	return m_strImageURL;
}

int	CMusicAlbumInfo::GetRating() const
{
	return m_iRating;
}

int	CMusicAlbumInfo::GetNumberOfSongs() const
{
	return (int)m_vecSongs.size();
}

const CMusicSong& CMusicAlbumInfo::GetSong(int iSong)
{
	return m_vecSongs[iSong];
}

bool	CMusicAlbumInfo::Parse(const CStdString& strHTML)
{
	m_vecSongs.erase(m_vecSongs.begin(),m_vecSongs.end());
	CStdString strHTMLLow=strHTML;
	strHTMLLow.MakeLower();
	int iStartOfTable=strHTMLLow.Find("artist");
	if (iStartOfTable< 0) return false;
	iStartOfTable=strHTMLLow.ReverseFind("<table",iStartOfTable);
	if (iStartOfTable < 0) return false;

	CHTMLUtil  util;
	CHTMLTable table;
	CStdString strTable=strHTML.Right((int)strHTML.size()-iStartOfTable);
	table.Parse(strTable);
	for (int iRow=0; iRow < table.GetRows(); iRow++)
	{
		const CHTMLRow& row=table.GetRow(iRow);
		int iColums=row.GetColumns();
		if (iColums>1)
		{
			const CStdString strColum1=row.GetColumValue(0);
			const CStdString strValue =row.GetColumValue(1);
			if (strColum1.Find("Artist") >=0)
			{
				util.getValueOfTag(	strValue,m_strArtist);			
			}
			if (strColum1.Find("Date of Release") >=0)
			{
				util.getValueOfTag(	strValue,m_strDateOfRelease);			
			}
			if (strColum1.Find("Genre") >=0)
			{
				util.getValueOfTag(	strValue,m_strGenre);			
			}
			if (strColum1.Find("Tones") >=0)
			{
				util.getValueOfTag(	strValue,m_strTones);			
			}
			if (strColum1.Find("Styles") >=0)
			{
				util.getValueOfTag(	strValue,m_strStyles);			
			}
			if (strColum1.Find("AMG Rating") >=0)
			{
				CStdString strRating;
				util.getValueOfTag(	strValue,strRating);			
			}
			if (strColum1.Find("Album Title") >=0)
			{
				util.getValueOfTag(	strValue,m_strTitle);			
			}
		}
	}

	// parse review/image
	iStartOfTable=strHTMLLow.Find("review",0);
	if (iStartOfTable < 0) return false;
	strTable=strHTML.Right((int)strHTML.size()-iStartOfTable);
	table.Parse(strTable);
	int iRows=table.GetRows();
	if (iRows > 0)
	{
		const CHTMLRow& row=table.GetRow(0);
		int iColums=row.GetColumns();
		if (iColums>=1)
		{
			const CStdString strReviewAndImage=row.GetColumValue(0);
			
			util.getAttributeOfTag(strReviewAndImage,"src=",m_strImageURL);
			m_strReview=strReviewAndImage;
			util.RemoveTags(m_strReview);
			util.ConvertHTMLToAnsi(m_strReview, m_strReview);
		}
	}

	// parse songs...
	iStartOfTable=strHTMLLow.Find("htrk1.gif",0);
	if (iStartOfTable < 0) return false;
	iStartOfTable=strHTMLLow.ReverseFind("<table",iStartOfTable);
	if (iStartOfTable < 0) return false;
	strTable=strHTML.Right((int)strHTML.size()-iStartOfTable);
	table.Parse(strTable);
	for (int iRow=0; iRow < table.GetRows(); iRow++)
	{
		const CHTMLRow& row=table.GetRow(iRow);
		int iCols=row.GetColumns();
		if (iCols >=5)
		{
			CStdString strTrack;
			for (int i=0; i < iCols;i++)
			{
				strTrack=row.GetColumValue(i);
				int ipos=strTrack.Find(".");
				if (ipos>=0)
				{
					bool bok=true;
					for (int x=0; x <ipos; x++)
					{
						if (!isdigit( strTrack[x] ) )
						{
							bok=false;
							break;
						}
					}
					if (bok) break;
				}
			}
			CStdString strNameAndDuration=row.GetColumValue(4);

			
			CStdString strName,strDuration="0";
			int iDuration=0;
			util.getValueOfTag(strNameAndDuration,strName);
			int iPos=strNameAndDuration.ReverseFind("-");
			if (iPos > 0)
			{
				iPos+=2;
				strDuration=strNameAndDuration.Right( (int)strNameAndDuration.size()-iPos);
				CStdString strMin="0", strSec="0";
				iPos=strDuration.Find(":");
				if (iPos>=0)
				{
					strMin=strDuration.Left(iPos);
					iPos++;
					strSec=strDuration.Right((int)strDuration.size()-iPos);
					int iMin=atoi(strMin.c_str());
					int iSec=atoi(strSec.c_str());
					iDuration=iMin*60+iSec;
				}
			}
			iPos=strTrack.Find(".");
			if (iPos > 0) strTrack = strTrack.Left(iPos);

			int iTrack=atoi(strTrack.c_str());
			CStdString strStripped;
			util.ConvertHTMLToAnsi(strName, strStripped);
			CMusicSong newSong(iTrack, strStripped, iDuration);
			m_vecSongs.push_back(newSong);
		}
	}
	if (m_strTitle2="") m_strTitle2=m_strTitle;
	SetLoaded(true);
	return true;
}


bool CMusicAlbumInfo::Load()
{
	CStdString strHTML;
	CHTTP http;
	if ( !http.Get(m_strAlbumURL, strHTML)) return false;
	return Parse(strHTML);
}



void CMusicAlbumInfo::Save(CStdString& strFileName)
{
	
	FILE* fd = fopen(strFileName.c_str(), "wb+");
	if (fd)
	{
		CUtil::SaveString(m_strArtist,fd);
		CUtil::SaveString(m_strTitle,fd);
		CUtil::SaveString(m_strDateOfRelease,fd);
		CUtil::SaveString(m_strGenre,fd);
		CUtil::SaveString(m_strTones,fd);
		CUtil::SaveString(m_strStyles,fd);
		CUtil::SaveString(m_strReview,fd);
		CUtil::SaveString(m_strImageURL,fd);
		CUtil::SaveString(m_strTitle2,fd);
		CUtil::SaveInt(m_iRating,fd);
		CUtil::SaveInt(m_vecSongs.size(),fd);
		for (int iSong=0; iSong < (int)m_vecSongs.size(); iSong++)
		{
			CMusicSong& song=m_vecSongs[iSong];
			song.Save(fd);
		}
		fclose(fd);
	}
}

bool CMusicAlbumInfo::Load(CStdString& strFileName)
{
	m_vecSongs.erase(m_vecSongs.begin(),m_vecSongs.end());
	FILE* fd = fopen(strFileName.c_str(), "rb");
	if (fd)
	{
		CUtil::LoadString(m_strArtist,fd);
		CUtil::LoadString(m_strTitle,fd);
		CUtil::LoadString(m_strDateOfRelease,fd);
		CUtil::LoadString(m_strGenre,fd);
		CUtil::LoadString(m_strTones,fd);
		CUtil::LoadString(m_strStyles,fd);
		CUtil::LoadString(m_strReview,fd);
		CUtil::LoadString(m_strImageURL,fd);
		CUtil::LoadString(m_strTitle2,fd);
		m_iRating=CUtil::LoadInt(fd);
		int iSongs=CUtil::LoadInt(fd);
		for (int iSong=0; iSong < iSongs; iSong++)
		{
			CMusicSong song;
			song.Load(fd);
			m_vecSongs.push_back(song);
		}
		
		SetLoaded(true);
		fclose(fd);
		return true;
	}
	return false;
}

void CMusicAlbumInfo::SetLoaded(bool bOnOff)
{
	m_bLoaded=bOnOff;
}
bool CMusicAlbumInfo::Loaded() const
{
	return m_bLoaded;
}
