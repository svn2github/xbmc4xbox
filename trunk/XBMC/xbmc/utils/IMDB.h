// IMDB1.h: interface for the CIMDB class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_IMDB1_H__562A722A_CD2A_4B4A_8A67_32DE8088A7D3__INCLUDED_)
#define AFX_IMDB1_H__562A722A_CD2A_4B4A_8A67_32DE8088A7D3__INCLUDED_

#include "HTTP.h"	// Added by ClassView
#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "http.h"
#include "StdString.h"
#include <vector>


class CIMDBUrl
{
	public:
		CStdString m_strURL;
		CStdString m_strTitle;
};
typedef vector<CIMDBUrl> IMDB_MOVIELIST;

class CIMDBMovie
{
	public:
		bool 				Load(const CStdString& strFileName);
		void 				Save(const CStdString& strFileName);
		CStdString m_strDirector;
		CStdString m_strWritingCredits;
		CStdString m_strGenre;
		CStdString m_strTagLine;
		CStdString m_strPlotOutline;
		CStdString m_strPlot;
		CStdString m_strPictureURL;
		CStdString m_strTitle;
		CStdString m_strVotes;
		CStdString m_strCast;
		CStdString m_strSearchString;
    CStdString m_strFile;
    CStdString m_strPath;
    CStdString m_strDVDLabel;
		int				 m_iTop250;
		int    		 m_iYear;
		float  		 m_fRating;
private:
};

class CIMDB  
{
public:
	CIMDB();
	CIMDB(const CStdString& strProxyServer, int iProxyPort);
	virtual ~CIMDB();

	bool FindMovie(const CStdString& strMovie,	IMDB_MOVIELIST& movielist);
	bool GetDetails(const CIMDBUrl& url, CIMDBMovie& movieDetails);
	bool Download(const CStdString &strURL, const CStdString &strFileName);
	void GetURL(const CStdString& strMovie,CStdString& strURL);
protected:
	void ParseAHREF(const char* ahref, CStdString& strURL, CStdString& strTitle);
	void ParseGenres(const char* ahref, CStdString& strURL, CStdString& strTitle);
	void RemoveAllAfter(char* szMovie,const char* szSearch);
  CHTTP m_http;
};

#endif // !defined(AFX_IMDB1_H__562A722A_CD2A_4B4A_8A67_32DE8088A7D3__INCLUDED_)
