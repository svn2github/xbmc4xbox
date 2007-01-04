// IMDB1.h: interface for the CIMDB class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_IMDB1_H__562A722A_CD2A_4B4A_8A67_32DE8088A7D3__INCLUDED_)
#define AFX_IMDB1_H__562A722A_CD2A_4B4A_8A67_32DE8088A7D3__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "http.h"
#include "thread.h"
#include "ScraperParser.h"

// forward definitions
class TiXmlDocument;
class CGUIDialogProgress;

class CIMDBUrl
{
public:
  std::vector<CStdString> m_strURL;
  CStdString m_strID;  CStdString m_strTitle;
};
typedef vector<CIMDBUrl> IMDB_MOVIELIST;

class CIMDBMovie
{
public:
  void Reset();
  bool Load(const CStdString& strFileName);
  void Save(const CStdString& strFileName);
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
  CStdString m_strRuntime;
  CStdString m_strFile;
  CStdString m_strPath;
  CStdString m_strIMDBNumber;
  CStdString m_strMPAARating;
  CStdString m_strFileNameAndPath;
  bool m_bWatched;
  int m_iTop250;
  int m_iYear;
  float m_fRating;
private:
};

class CIMDB : public CThread
{
public:
  CIMDB();
  CIMDB(const CStdString& strProxyServer, int iProxyPort);
  virtual ~CIMDB();

  bool LoadDLL();
  bool InternalFindMovie(const CStdString& strMovie, IMDB_MOVIELIST& movielist);
  bool InternalGetDetails(const CIMDBUrl& url, CIMDBMovie& movieDetails);
  bool ParseDetails(TiXmlDocument &doc, CIMDBMovie &movieDetails);
  bool LoadDetails(const CStdString& strIMDB, CIMDBMovie &movieDetails);
  bool Download(const CStdString &strURL, const CStdString &strFileName);
  const CStdString GetURL(const CStdString& strMovie, CStdString& strURL, CStdString& strYear);

  // threaded lookup functions
  bool FindMovie(const CStdString& strMovie, IMDB_MOVIELIST& movielist, const CStdString& strScraper, CGUIDialogProgress *pProgress = NULL);
  bool GetDetails(const CIMDBUrl& url, CIMDBMovie &movieDetails, const CStdString& strScraper, CGUIDialogProgress *pProgress = NULL);

protected:
  void RemoveAllAfter(char* szMovie, const char* szSearch);
  CHTTP m_http;

  CScraperParser m_parser;
  CStdString m_strScraper;

  // threaded stuff
  void Process();
  void CloseThread();

  enum LOOKUP_STATE { DO_NOTHING = 0,
                      FIND_MOVIE = 1,
                      GET_DETAILS = 2 };
  CStdString      m_strMovie;
  IMDB_MOVIELIST  m_movieList;
  CIMDBMovie      m_movieDetails;
  CIMDBUrl        m_url;
  LOOKUP_STATE    m_state;
  bool            m_found;
};

#endif // !defined(AFX_IMDB1_H__562A722A_CD2A_4B4A_8A67_32DE8088A7D3__INCLUDED_)
