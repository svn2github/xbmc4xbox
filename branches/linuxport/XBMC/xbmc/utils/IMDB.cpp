// IMDB1.cpp: implementation of the CIMDB class.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "IMDB.h"
#include "../Util.h"
#include "HTMLUtil.h"
#include "XMLUtils.h"
#include "RegExp.h"
#include "ScraperParser.h"

using namespace HTML;

#pragma warning (disable:4018) 
//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

bool CIMDBUrl::Parse(CStdString strUrls)
{
  if (strUrls.IsEmpty())
    return false;
  
  // ok, now parse the xml file
  if (strUrls.Find("encoding=\"utf-8\"") < 0)
    g_charsetConverter.stringCharsetToUtf8(strUrls);
  
  TiXmlDocument doc;
  doc.Parse(strUrls.c_str(),0,TIXML_ENCODING_UTF8);
  if (doc.RootElement())
  {
    TiXmlHandle docHandle( &doc );
    TiXmlElement *link = docHandle.FirstChild( "episodeguide" ).FirstChild( "url" ).Element();
    if(link)
      for ( link; link; link = link->NextSiblingElement("url") )
        m_scrURL.push_back(CScraperUrl(link));      
    else if( link = docHandle.FirstChild( "episodeguide" ).Element() )
      m_scrURL.push_back(CScraperUrl(link));

  } 
  else
    return false;
  return true;
}


CIMDB::CIMDB()
{
}

CIMDB::CIMDB(const CStdString& strProxyServer, int iProxyPort)
    : m_http(strProxyServer, iProxyPort)
{
}

CIMDB::~CIMDB()
{
}
bool CIMDB::Get(CScraperUrl& scrURL, string& strHTML)
{
  CURL url(scrURL.m_url[0].m_url);
  m_http.SetReferer(scrURL.m_url[0].m_spoof);

  if(scrURL.m_url[0].m_post)
  {
    CStdString strOptions = url.GetOptions();
    strOptions = strOptions.substr(1);
    url.SetOptions("");
    CStdString strUrl;
    url.GetURL(strUrl);

    if (!m_http.Post(strUrl, strOptions, strHTML))
      return false;
  }
  else 
    if (!m_http.Get(scrURL.m_url[0].m_url, strHTML))
      return false;
  
  return true;
}
bool CIMDB::InternalFindMovie(const CStdString &strMovie, IMDB_MOVIELIST& movielist)
{
  // load our scraper xml
  if (!m_parser.Load("Q:\\system\\scrapers\\video\\"+m_info.strPath))
    return false;

  CIMDBUrl url;
  movielist.clear();

  CStdString strHTML, strYear;
  CScraperUrl scrURL;
  
  GetURL(strMovie, scrURL, strYear);

  if (!Get(scrURL, strHTML) || strHTML.size() == 0)
  {
    CLog::Log(LOGERROR, "IMDB: Unable to retrieve web site");
    return false;
  }
  
  m_parser.m_param[0] = strHTML;
  m_parser.m_param[1] = scrURL.m_url[0].m_url;
  CStdString strXML = m_parser.Parse("GetSearchResults");
  if (strXML.IsEmpty())
  {
    CLog::Log(LOGERROR, "IMDB: Unable to parse web site");
    return false;
  }

  if (strXML.Find("encoding=\"utf-8\"") < 0)
    g_charsetConverter.stringCharsetToUtf8(strXML);

  // ok, now parse the xml file
  TiXmlDocument doc;
  doc.Parse(strXML.c_str(),0,TIXML_ENCODING_UTF8);
  if (!doc.RootElement())
  {
    CLog::Log(LOGERROR, "IMDB: Unable to parse xml");
    return false;
  }
  TiXmlHandle docHandle( &doc );
  TiXmlElement *movie = docHandle.FirstChild( "results" ).FirstChild( "entity" ).Element();

  int iYear = atoi(strYear);

  for ( movie; movie; movie = movie->NextSiblingElement() )
  {
    url.m_scrURL.clear();
    TiXmlNode *title = movie->FirstChild("title");
    TiXmlElement *link = movie->FirstChildElement("url");
    TiXmlNode *year = movie->FirstChild("year");
    TiXmlNode* id = movie->FirstChild("id");
    if (title && title->FirstChild() && link && link->FirstChild())
    {
      url.m_strTitle = title->FirstChild()->Value();
      CScraperUrl scrURL(link);
      url.m_scrURL.push_back(scrURL);
      //url.m_strURL.push_back(link->FirstChild()->Value());
      while ((link = link->NextSiblingElement("url")))
      {
        CScraperUrl scrURL(link);
        //url.m_strURL.push_back(link->FirstChild()->Value());
        url.m_scrURL.push_back(scrURL);
      }
      if (id && id->FirstChild())
        url.m_strID = id->FirstChild()->Value();
      // if source contained a distinct year, only allow those
      if(iYear != 0)
      {
        if(year && year->FirstChild())
        { // sweet scraper provided a year
          if(iYear != atoi(year->FirstChild()->Value()))
            continue;
        }
        else if(url.m_strTitle.length() >= 6)
        { // imdb normally puts year at end of title within ()
          if(url.m_strTitle.at(url.m_strTitle.length()-1) == ')'
          && url.m_strTitle.at(url.m_strTitle.length()-6) == '(')
          {
            int iYear2 = atoi(url.m_strTitle.Right(5).Left(4).c_str());
            if( iYear2 != 0 && iYear != iYear2)
              continue;
          }
        }
      }

      movielist.push_back(url);
    }
  }
  return true;
}

bool CIMDB::InternalGetEpisodeList(const CIMDBUrl& url, IMDB_EPISODELIST& details)
{
  // load our scraper xml
  if (!m_parser.Load("Q:\\system\\scrapers\\video\\"+m_info.strPath))
    return false;
  IMDB_EPISODELIST temp;
  for(int i=0; i < url.m_scrURL.size(); i++)
  {
    CStdString strHTML;
    CScraperUrl scrUrl;
    scrUrl = url.m_scrURL[i];
    if (!Get(scrUrl,strHTML) || strHTML.size() == 0)
    {
      CLog::Log(LOGERROR, "IMDB: Unable to retrieve web site");
      if (temp.size() > 0 || (i == 0 && url.m_scrURL.size() > 1)) // use what was fetched
        continue;

      return false;
    }
    m_parser.m_param[0] = strHTML;
    m_parser.m_param[1] = scrUrl.m_url[0].m_url;

    CStdString strXML = m_parser.Parse("GetEpisodeList");
    if (strXML.IsEmpty())
    {
      CLog::Log(LOGERROR, "IMDB: Unable to parse web site");
      if (temp.size() > 0 || (i == 0 && url.m_scrURL.size() > 1)) // use what was fetched
        continue;

      return false;
    }
    // ok, now parse the xml file
    TiXmlDocument doc;
    doc.Parse(strXML.c_str());
    if (!doc.RootElement())
    {
      CLog::Log(LOGERROR, "IMDB: Unable to parse xml");
      return false;
    }
    TiXmlHandle docHandle( &doc );
    TiXmlElement *movie = docHandle.FirstChild( "episodeguide" ).FirstChild( "episode" ).Element();

    for ( movie; movie; movie = movie->NextSiblingElement() )
    {
      TiXmlNode *title = movie->FirstChild("title");
      TiXmlElement *link = movie->FirstChildElement("url");
      TiXmlNode *epnum = movie->FirstChild("epnum");
      TiXmlNode *season = movie->FirstChild("season");
      TiXmlNode* id = movie->FirstChild("id");
      if (title && title->FirstChild() && link && link->FirstChild() && epnum && epnum->FirstChild() && season && season->FirstChild())
      {
        CIMDBUrl url2;
        g_charsetConverter.stringCharsetToUtf8(title->FirstChild()->Value(),url2.m_strTitle);
        url2.m_scrURL.push_back(CScraperUrl(link) );

        while ((link = link->NextSiblingElement("url")))
        {
          url2.m_scrURL.push_back(CScraperUrl(link));
        }
        if (id && id->FirstChild())
          url2.m_strID = id->FirstChild()->Value();
        // if source contained a distinct year, only allow those
        std::pair<int,int> key(atoi(season->FirstChild()->Value()),atoi(epnum->FirstChild()->Value()));
        temp.insert(std::make_pair<std::pair<int,int>,CIMDBUrl>(key,url2));
      }
    }
  }

  // find minimum in each season
  std::map<int,int> min; 
  for (IMDB_EPISODELIST::iterator iter=temp.begin(); iter != temp.end(); ++iter ) 
  { 
    if (min.size() == (iter->first.first -1))
      min.insert(iter->first);
    else if (iter->first.second < min[iter->first.first])
      min[iter->first.first] = iter->first.second;
  }
  // correct episode numbers
  for (IMDB_EPISODELIST::iterator iter=temp.begin(); iter != temp.end(); ++iter ) 
  {
    int episode=iter->first.second - min[iter->first.first];
    if (min[iter->first.first] > 0)
      episode++;
    std::pair<int,int> key(iter->first.first,episode);
    details.insert(std::make_pair<std::pair<int,int>,CIMDBUrl>(key,iter->second));
  }

  return true;
}

bool CIMDB::InternalGetDetails(const CIMDBUrl& url, CVideoInfoTag& movieDetails, const CStdString& strFunction)
{
  // load our scraper xml
  if (!m_parser.Load("q:\\system\\scrapers\\video\\"+m_info.strPath))
    return false;

  std::vector<CStdString> strHTML;

  for (unsigned int i=0;i<url.m_scrURL.size();++i)
  {
    CStdString strCurrHTML;
    CScraperUrl strU = url.m_scrURL[i];
    if (!Get( strU,strCurrHTML) || strCurrHTML.size() == 0)
      return false;
    strHTML.push_back(strCurrHTML);
  }

  // now grab our details using the scraper
  for (int i=0;i<strHTML.size();++i)
    m_parser.m_param[i] = strHTML[i];

  m_parser.m_param[strHTML.size()] = url.m_strID;

  CStdString strXML = m_parser.Parse(strFunction);
  if (strXML.IsEmpty())
  {
    CLog::Log(LOGERROR, "IMDB: Unable to parse web site");
    return false;
  }

  // abit uggly, but should work. would have been better if parset
  // set the charset of the xml, and we made use of that
  if (strXML.Find("encoding=\"utf-8\"") < 0)
    g_charsetConverter.stringCharsetToUtf8(strXML);

    // ok, now parse the xml file
  TiXmlBase::SetCondenseWhiteSpace(false);
  TiXmlDocument doc;
  doc.Parse(strXML.c_str(),0,TIXML_ENCODING_UTF8);
  if (!doc.RootElement())
  {
    CLog::Log(LOGERROR, "IMDB: Unable to parse xml");
    return false;
  }

  bool ret = ParseDetails(doc, movieDetails);
  TiXmlElement* pRoot = doc.RootElement();
  TiXmlElement* xurl = pRoot->FirstChildElement("url");
  while (xurl && xurl->FirstChild())
  {
    const char* szFunction = xurl->Attribute("function");
    if (szFunction)
    {
      CIMDBUrl url2;
      
      CScraperUrl scrURL(xurl);
      url2.m_scrURL.push_back(scrURL);
      InternalGetDetails(url2,movieDetails,szFunction);
    }
    xurl = xurl->NextSiblingElement("url");
  }
  TiXmlBase::SetCondenseWhiteSpace(true);
  
  return ret;
}

bool CIMDB::ParseDetails(TiXmlDocument &doc, CVideoInfoTag &movieDetails)
{
  TiXmlHandle docHandle( &doc );
  TiXmlElement *details = docHandle.FirstChild( "details" ).Element();

  if (!details)
  {
    CLog::Log(LOGERROR, "IMDB: Invalid xml file");
    return false;
  }

  // set chaining to true here as this is called by our scrapers
  movieDetails.Load(details, true);
  
  CHTMLUtil::RemoveTags(movieDetails.m_strPlot);

  return true;
}

bool CIMDB::Download(const CStdString &strURL, const CStdString &strFileName)
{
  CStdString strHTML;
  if (!m_http.Download(strURL, strFileName))
  {
    CLog::Log(LOGERROR, "failed to download %s -> %s", strURL.c_str(), strFileName.c_str());
    return false;
  }

  return true;
}

bool CIMDB::LoadXML(const CStdString& strXMLFile, CVideoInfoTag &movieDetails, bool bDownload /* = true */)
{
  TiXmlBase::SetCondenseWhiteSpace(false);
  TiXmlDocument doc;

  movieDetails.Reset();
  if (doc.LoadFile(strXMLFile) && ParseDetails(doc, movieDetails))
  { // excellent!
    return true;
  }
  if (!bDownload)
    return true;

  return false;
}

void CIMDB::RemoveAllAfter(char* szMovie, const char* szSearch)
{
  char* pPtr = strstr(szMovie, szSearch);
  if (pPtr) *pPtr = 0;
}

// TODO: Make this user-configurable?
void CIMDB::GetURL(const CStdString &strMovie, CScraperUrl& scrURL, CStdString& strYear)
{
  char szMovie[1024];
  char szYear[5];

  CStdString strMovieNoExtension = strMovie;
  //don't assume movie name is a file with an extension
  //CUtil::RemoveExtension(strMovieNoExtension);

  // replace whitespace with +
  strMovieNoExtension.Replace(".","+");
  strMovieNoExtension.Replace("-","+");
  strMovieNoExtension.Replace(" ","+");

  // lowercase
  strMovieNoExtension = strMovieNoExtension.ToLower();

  // default to movie name begin complete filename, no year
  strcpy(szMovie, strMovieNoExtension.c_str());
  strcpy(szYear,"");

  CRegExp reYear;
  reYear.RegComp("(.+)\\+\\(?(19[0-9][0-9]|200[0-9])\\)?(\\+.*)?");
  if (reYear.RegFind(szMovie) >= 0)
  {
    char *pMovie = reYear.GetReplaceString("\\1");
    char *pYear = reYear.GetReplaceString("\\2");
    strcpy(szMovie,pMovie);
    strcpy(szYear,pYear);

    if (pMovie) free(pMovie);
    if (pYear) free(pYear);
  }

  CRegExp reTags;
  reTags.RegComp("\\+(ac3|custom|dc|divx|dsr|dsrip|dutch|dvd|dvdrip|dvdscr|fragment|fs|hdtv|internal|limited|multisubs|ntsc|ogg|ogm|pal|pdtv|proper|repack|rerip|retail|se|svcd|swedish|unrated|ws|xvid|xxx|cd[1-9]|\\[.*\\])(\\+|$)");

  CStdString strTemp;
  int i=0;
  CStdString strSearch = szMovie;
  if ((i=reTags.RegFind(strSearch.c_str())) >= 0) // new logic - select the crap then drop anything to the right of it
  {
    m_parser.m_param[0] = strSearch.Mid(0,i);
 }
  else
    m_parser.m_param[0] = szMovie;

  scrURL.ParseString(m_parser.Parse("CreateSearchUrl"));
  strYear = szYear;
}

// threaded functions
void CIMDB::Process()
{
  // note here that we're calling our external functions but we're calling them with
  // no progress bar set, so they're effectively calling our internal functions directly.
  m_found = false;
  if (m_state == FIND_MOVIE)
  {
    if (!FindMovie(m_strMovie, m_movieList))
      CLog::Log(LOGERROR, "IMDb::Error looking up movie %s", m_strMovie.c_str());
  }
  else if (m_state == GET_DETAILS)
  {
    if (!GetDetails(m_url, m_movieDetails))
      CLog::Log(LOGERROR, "IMDb::Error getting movie details from %s", m_url.m_scrURL[0].m_url[0].m_url.c_str());
  }
  else if (m_state == GET_EPISODE_DETAILS)
  {
    if (!GetEpisodeDetails(m_url, m_movieDetails))
      CLog::Log(LOGERROR, "IMDb::Error getting movie details from %s", m_url.m_scrURL[0].m_url[0].m_url.c_str());
  }
  else if (m_state == GET_EPISODE_LIST)
  {
    if (!GetEpisodeList(m_url, m_episode))
      CLog::Log(LOGERROR, "IMDb::Error getting episode details from %s", m_url.m_scrURL[0].m_url[0].m_url.c_str());
  }
  m_found = true;
}

bool CIMDB::FindMovie(const CStdString &strMovie, IMDB_MOVIELIST& movieList, CGUIDialogProgress *pProgress /* = NULL */)
{
  //CLog::Log(LOGDEBUG,"CIMDB::FindMovie(%s)", strMovie.c_str());
  g_charsetConverter.utf8ToStringCharset(strMovie,m_strMovie); // make sure searches is done using string chars
  if (pProgress)
  { // threaded version
    m_state = FIND_MOVIE;
    m_found = false;
    if (ThreadHandle())
      StopThread();
    Create();
    while (!m_found)
    {
      pProgress->Progress();
      if (pProgress->IsCanceled())
      {
        CloseThread();
        return false;
      }
      Sleep(1);
    }
    // transfer to our movielist
    movieList.clear();
    for (unsigned int i=0; i < m_movieList.size(); i++)
      movieList.push_back(m_movieList[i]);
    m_movieList.clear();
    CloseThread();
    return true;
  }
  else  // unthreaded
    return InternalFindMovie(strMovie, movieList);
}

bool CIMDB::GetDetails(const CIMDBUrl &url, CVideoInfoTag &movieDetails, CGUIDialogProgress *pProgress /* = NULL */)
{
  //CLog::Log(LOGDEBUG,"CIMDB::GetDetails(%s)", url.m_strURL.c_str());
  m_url = url;
  m_movieDetails = movieDetails;

  // fill in the defaults
  movieDetails.Reset();
  if (pProgress)
  { // threaded version
    m_state = GET_DETAILS;
    m_found = false;
    if (ThreadHandle())
      StopThread();
    Create();
    while (!m_found)
    {
      pProgress->Progress();
      if (pProgress->IsCanceled())
      {
        CloseThread();
        return false;
      }
      Sleep(1);
    }
    movieDetails = m_movieDetails;
    CloseThread();
    return true;
  }
  else  // unthreaded
    return InternalGetDetails(url, movieDetails);
}

bool CIMDB::GetEpisodeDetails(const CIMDBUrl &url, CVideoInfoTag &movieDetails, CGUIDialogProgress *pProgress /* = NULL */)
{
  //CLog::Log(LOGDEBUG,"CIMDB::GetDetails(%s)", url.m_strURL.c_str());
  m_url = url;
  m_movieDetails = movieDetails;

  // fill in the defaults
  movieDetails.Reset();
  if (pProgress)
  { // threaded version
    m_state = GET_EPISODE_DETAILS;
    m_found = false;
    if (ThreadHandle())
      StopThread();
    Create();
    while (!m_found)
    {
      pProgress->Progress();
      if (pProgress->IsCanceled())
      {
        CloseThread();
        return false;
      }
      Sleep(1);
    }
    movieDetails = m_movieDetails;
    CloseThread();
    return true;
  }
  else  // unthreaded
    return InternalGetDetails(url, movieDetails, "GetEpisodeDetails");
}

bool CIMDB::GetEpisodeList(const CIMDBUrl &url, IMDB_EPISODELIST& movieDetails, CGUIDialogProgress *pProgress /* = NULL */)
{
  //CLog::Log(LOGDEBUG,"CIMDB::GetDetails(%s)", url.m_strURL.c_str());
  m_url = url;
  m_episode = movieDetails;

  // fill in the defaults
  movieDetails.clear();
  if (pProgress)
  { // threaded version
    m_state = GET_EPISODE_LIST;
    m_found = false;
    if (ThreadHandle())
      StopThread();
    Create();
    while (!m_found)
    {
      pProgress->Progress();
      if (pProgress->IsCanceled())
      {
        CloseThread();
        return false;
      }
      Sleep(1);
    }
    movieDetails = m_episode;
    CloseThread();
    return true;
  }
  else  // unthreaded
    return InternalGetEpisodeList(url, movieDetails);  
}

void CIMDB::CloseThread()
{
  m_http.Cancel();
  StopThread();
  m_state = DO_NOTHING;
  m_found = false;
}
