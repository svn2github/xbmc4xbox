
#include "../stdafx.h"
// IMDB1.cpp: implementation of the CIMDB class.
//
//////////////////////////////////////////////////////////////////////

#include "IMDB.h"
#include "../util.h"
#include "log.h"
#include "localizestrings.h"
#include "../settings.h"
#include "HTMLUtil.h"
using namespace HTML;
#pragma warning (disable:4018)
//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CIMDB::CIMDB()
{
}
CIMDB::CIMDB(const CStdString& strProxyServer, int iProxyPort)
:m_http(strProxyServer,iProxyPort)
{

}

CIMDB::~CIMDB()
{

}

bool CIMDB::FindMovie(const CStdString &strMovie,IMDB_MOVIELIST& movielist)
{
	char szTitle[1024];
	char szURL[1024];
	CIMDBUrl url;
	movielist.clear();
	bool bSkip=false;
	int ipos=0;

	CStdString strURL,strHTML;
	GetURL(strMovie,strURL);
	if (!m_http.Get(strURL,strHTML))
		return false;

	if (strHTML.size()==0) 
  {
    CLog::Log(LOGWARNING, "empty document returned");
    return false;
  }

	char *szBuffer= new char [strHTML.size()+1];
	if (!szBuffer) return false;
	strcpy(szBuffer,strHTML.c_str());
	
	/*  AS OF 20-12-2004
	<h2>Popular Results</h2>

<b>Popular Titles</b> (Displaying 1 Result)<ol><li><a href="/title/tt0056869/?fr=c2l0ZT1kZnxzZz0xfHR0PTF8cG49MHxxPUZvciBUaGUgQmlyZHN8bXg9MjB8bG09MjAwfGh0bWw9MQ__;fc=1;ft=11;fm=1">The Birds</a> (1963)<br>&#160;aka <em>"Alfred Hitchcock's The Birds"</em> - UK <em>(complete title)</em></li></ol>
</p>

<h2>Other Results</h2>

<p>
<b>Titles (Exact Matches)</b> (Displaying 1 Result)<ol><li><a href="/title/tt0248808/?fr=c2l0ZT1kZnxzZz0xfHR0PTF8cG49MHxxPUZvciBUaGUgQmlyZHN8bXg9MjB8bG09MjAwfGh0bWw9MQ__;fc=2;ft=11;fm=1">For the Birds</a> (2000)</li></ol><b>Titles (Partial Matches)</b> (Displaying 4 Results)<ol><li><a href="/title/tt0045173/?fr=c2l0ZT1kZnxzZz0xfHR0PTF8cG49MHxxPUZvciBUaGUgQmlyZHN8bXg9MjB8bG09MjAwfGh0bWw9MQ__;fc=3;ft=11;fm=1">Something for the Birds</a> (1952)</li><li><a href="/title/tt0151087/?fr=c2l0ZT1kZnxzZz0xfHR0PTF8cG49MHxxPUZvciBUaGUgQmlyZHN8bXg9MjB8bG09MjAwfGh0bWw9MQ__;fc=4;ft=11;fm=1">It's for the Birds</a> (1967)</li><li><a href="/title/tt0284522/?fr=c2l0ZT1kZnxzZz0xfHR0PTF8cG49MHxxPUZvciBUaGUgQmlyZHN8bXg9MjB8bG09MjAwfGh0bWw9MQ__;fc=5;ft=11;fm=1">Strictly for the Birds</a> (1963)</li><li><a href="/title/tt0066281/?fr=c2l0ZT1kZnxzZz0xfHR0PTF8cG49MHxxPUZvciBUaGUgQmlyZHN8bXg9MjB8bG09MjAwfGh0bWw9MQ__;fc=6;ft=11;fm=1">The Rainbirds</a> (1971) (TV)<br>&#160;aka <em>"Play for Today: The Rainbirds"</em> - UK <em>(series title)</em></li></ol><b>Titles (Approx Matches)</b> (Displaying 5 Results)<ol><li><a href="/title/tt0366252/?fr=c2l0ZT1kZnxzZz0xfHR0PTF8cG49MHxxPUZvciBUaGUgQmlyZHN8bXg9MjB8bG09MjAwfGh0bWw9MQ__;fc=7;ft=11;fm=1">Borrowed Clothes; or, Fine Feathers Make Fine Birds</a> (1909)</li><li><a href="/title/tt0055934/?fr=c2l0ZT1kZnxzZz0xfHR0PTF8cG49MHxxPUZvciBUaGUgQmlyZHN8bXg9MjB8bG09MjAwfGh0bWw9MQ__;fc=8;ft=11;fm=1">Du mouron pour les petits oiseaux</a> (1962)<br>&#160;aka <em>"Chicken Feed for Little Birds"</em> - <em>(English title)</em></li><li><a href="/title/tt0151086/?fr=c2l0ZT1kZnxzZz0xfHR0PTF8cG49MHxxPUZvciBUaGUgQmlyZHN8bXg9MjB8bG09MjAwfGh0bWw9MQ__;fc=9;ft=11;fm=1">It's for the Birdies</a> (1962)</li><li><a href="/title/tt0032426/?fr=c2l0ZT1kZnxzZz0xfHR0PTF8cG49MHxxPUZvciBUaGUgQmlyZHN8bXg9MjB8bG09MjAwfGh0bWw9MQ__;fc=10;ft=11;fm=1">The Early Worm Gets the Bird</a> (1940)</li><li><a href="/title/tt0306741/?fr=c2l0ZT1kZnxzZz0xfHR0PTF8cG49MHxxPUZvciBUaGUgQmlyZHN8bXg9MjB8bG09MjAwfGh0bWw9MQ__;fc=11;ft=11;fm=1">Doraemon, Nobita to tsubasa no yuusacchi</a> (2001) (TV)<br>&#160;aka <em>"Doraemon, Nobita, and the Bird Kingdom"</em></li></ol>

</p>
<p>
  */
	/* OLD METHOD
	<b>Exact Matches</b> (11 matches, by popularity)
    </p>
    <p>
    <table>
    <tr><td valign="top" align="right">1.&#160;</td><td valign="top" width="100%"><a href="/title/tt0313443/">Out of Time (2003/I)</a></td></tr>
	///old way
	<H2><A NAME="mov">Movies</A></H2>
	<OL><LI><A HREF="/Title?0167261">Lord of the Rings: The Two Towers, The (2002)</A><BR>...aka <I>Two Towers, The (2002) (USA: short title)</I>
	</LI>
	*/

	CHTMLUtil html;

	char *pStartOfMovieList=strstr(szBuffer," Titles</b>");
	if (!pStartOfMovieList)
	{	// We don't have the "Titles</b>" tag, try "Matches)</b>
		pStartOfMovieList=strstr(szBuffer," Matches)</b>");
	}
	if (!pStartOfMovieList)
	{
		char* pMovieTitle	 = strstr(szBuffer,"\"title\">");
		char* pMovieDirector = strstr(szBuffer,"Directed");
		char* pMovieGenre	 = strstr(szBuffer,"Genre:");
		char* pMoviePlot	 = strstr(szBuffer,"Plot");

		// oh i say, it appears we've been redirected to the actual movie details...

		// NOTE: pMovieDirector not always populated for TV series
		if (pMovieTitle && pMovieGenre && pMoviePlot)
		{
			pMovieTitle+=8;
			char *pEnd = strstr(pMovieTitle,"<");
			if (pEnd) *pEnd=0;

      OutputDebugString("Got movie:");
      OutputDebugString(pMovieTitle);
      OutputDebugString("url:" );
      OutputDebugString(m_http.m_redirectedURL.c_str());
      OutputDebugString("\n" );
			html.ConvertHTMLToAnsi(pMovieTitle, url.m_strTitle);
			url.m_strURL   = m_http.m_redirectedURL;
			movielist.push_back(url);

			delete [] szBuffer;
			return true;
		}

		OutputDebugString("Unable to locate start of movie list.\n");
		delete [] szBuffer;
		return false;
	}

	pStartOfMovieList+=strlen("<table>");
	char *pEndOfMovieList=strstr(pStartOfMovieList,"</table>");
	if (!pEndOfMovieList)
	{
		pEndOfMovieList=pStartOfMovieList+strlen(pStartOfMovieList);
		OutputDebugString("Unable to locate end of movie list.\n");
//		delete [] szBuffer;
	//	return false;
	}

	*pEndOfMovieList=0;
	while(1)
	{
		char* pAHREF=strstr(pStartOfMovieList,"<a href=\"/title");
		if (pAHREF)
		{
			//<a href="/title/tt0313443/">Out of Time</a>(2003/I)</tr>...
			//<a href="/title/tt0313443/">Out of Time (2003/I)</a>
			//old way
			//<A HREF="/Title?0167261">Lord of the Rings: The Two Towers, The (2002)</A>
			char* pendAHREF=strstr(pStartOfMovieList,"</a>");
			if (pendAHREF)
			{
				char* pYear=pendAHREF+4;
				char* pendYear = strstr(pYear, "<");
				*pendAHREF	=0;
				pAHREF+=strlen("<a href=.");
				// get url
				char *pURL=strstr(pAHREF,">");
				if (pURL)
				{
					pURL--;
					*pURL=0;
					pURL++;
					*pURL=0;
          char* pURLEnd=strstr(pURL+1,"<");
					char *pURLQuestion=strstr(pAHREF, "?fr");
					if (pURLQuestion)
					{
						*pURLQuestion=0;
						pURLQuestion++;
						*pURLQuestion=0;
					}
          strcpy(szURL, pAHREF);
					if (pURLEnd)  
          {
            *pURLEnd=0;
            strcpy(szTitle, pURL+1);
            *pURLEnd='<';
          }
          else
            strcpy(szTitle, pURL+1);
					if (pendYear)
					{
						*pendYear = 0;
						strcat(szTitle, pYear);
						pendAHREF = pendYear;
					}
					html.ConvertHTMLToAnsi(szTitle, url.m_strTitle);
		
					sprintf(szURL,"http://us.imdb.com/%s", &pAHREF[1]);
					url.m_strURL=szURL;
					movielist.push_back(url);
				}
				pStartOfMovieList = pendAHREF+1;
				if (pStartOfMovieList >=pEndOfMovieList) break;
			}
			else
			{
				break;
			}
		}
		else
		{
			break;
		}
	}

	delete [] szBuffer;
	return true;
}


bool CIMDB::GetDetails(const CIMDBUrl& url, CIMDBMovie& movieDetails)
{
	CStdString strHTML;
	CStdString strURL = url.m_strURL;

	CStdString strLocNotAvail=g_localizeStrings.Get(416);	// Not available

	movieDetails.m_strTitle=url.m_strTitle;
	movieDetails.m_strDirector=strLocNotAvail;
	movieDetails.m_strWritingCredits=strLocNotAvail;
	movieDetails.m_strGenre=strLocNotAvail;
	movieDetails.m_strTagLine=strLocNotAvail;
	movieDetails.m_strPlotOutline=strLocNotAvail;
	movieDetails.m_strPlot=strLocNotAvail;
	movieDetails.m_strPictureURL="";
	movieDetails.m_strRuntime=strLocNotAvail;
	movieDetails.m_iYear=0;
	movieDetails.m_fRating=0.0;
	movieDetails.m_strVotes=strLocNotAvail;
	movieDetails.m_strCast=strLocNotAvail;
	movieDetails.m_iTop250=0;

	if (!m_http.Get(strURL,strHTML))
		return false;
	if (strHTML.size()==0) 
		return false;
	char *szBuffer= new char[strHTML.size()+1];
	strcpy(szBuffer,strHTML.c_str());
	
  int idxUrlParams = strURL.Find('?');
  if (idxUrlParams != -1) {
    strURL = strURL.Left(idxUrlParams);
  }
  char szURL[1024];
  strcpy(szURL, strURL.c_str());
  if (CUtil::HasSlashAtEnd(strURL)) szURL[ strURL.size()-1 ]=0;
  int ipos=strlen(szURL)-1;
  while (szURL[ipos] != '/' && ipos>0) ipos--;

  movieDetails.m_strIMDBNumber=&szURL[ipos+1];
  CLog::Log(LOGINFO, "imdb number:%s\n", movieDetails.m_strIMDBNumber.c_str());
	char *pDirectedBy=strstr(szBuffer,"Directed by");
	char *pCredits=strstr(szBuffer,"Writing credits");
	char* pGenre=strstr(szBuffer,"Genre:");
	char* pTagLine=strstr(szBuffer,"Tagline:</b>");	
	char* pPlotOutline=strstr(szBuffer,"Plot Outline:</b>");	
	char* pPlotSummary=strstr(szBuffer,"Plot Summary:</b>");	
	char* pPlot=strstr(szBuffer,"<a href=\"plotsummary");
	char* pImage=strstr(szBuffer,"<img border=\"0\" alt=\"cover\" src=\"");
	char* pRating=strstr(szBuffer,"User Rating:</b>");
	char* pRuntime=strstr(szBuffer,"Runtime:</b>");
	char* pCast=strstr(szBuffer,"first billed only: </b></td></tr>");
	char* pCred=strstr(szBuffer,"redited cast:"); // Complete credited cast or Credited cast
	char* pTop=strstr(szBuffer, "top 250:");

	char *pYear=strstr(szBuffer,"/Sections/Years/");
	if (pYear)
	{
		char szYear[5];
		pYear+=strlen("/Sections/Years/");
		strncpy(szYear,pYear,4);
		szYear[4]=0;
		movieDetails.m_iYear=atoi(szYear);
	}

	if (pDirectedBy) 
		ParseAHREF(pDirectedBy, strURL, movieDetails.m_strDirector);

	if (pCredits) 
		ParseAHREF(pCredits, strURL, movieDetails.m_strWritingCredits);

	if (pGenre)  
		ParseGenres(pGenre, strURL, movieDetails.m_strGenre);

	if (pRating) // and votes
	{
		char *pStart = strstr(pRating, "<b>"); 
		if(pStart) 
		{
			char *pEnd = strstr(pStart, "/");
			*pEnd = 0;
			// set rating
			movieDetails.m_fRating = (float)atof(pStart+3);

			if(movieDetails.m_fRating != 0.0) {
				// now, votes
				pStart = strstr(pEnd+2, "(");
				if(pStart)
					pEnd = strstr(pStart, " votes)");

				if(pEnd) {
					*pEnd = 0;
					pStart++; // skip the parantese before votes
					movieDetails.m_strVotes = pStart; // save
				} else {
					movieDetails.m_strVotes = "0";
				}
			}
		}
	}

	if(pTop) // top rated movie :)
	{
		pTop += strlen("top 250:") + 2; // jump space and #
		char *pEnd = strstr(pTop, "</a>");
		*pEnd = 0;
		movieDetails.m_iTop250 = atoi(pTop);
	}

	if(!pCast) 
	{
		pCast=pCred;
	}

	CHTMLUtil html;
	if(pCast) 
	{
		char *pRealEnd = strstr(pCast, "&nbsp;");
		char *pStart = strstr(pCast, "<a href");
		char *pEnd = pCast;

		if(pRealEnd &&  pStart && pEnd)
		{
			 movieDetails.m_strCast = "Cast overview:\n";

			 while(pRealEnd > pStart) 
			 {
				CStdString url = "";
				CStdString actor = "";
				CStdString role = "";
		
				// actor
					
				pEnd = strstr(pStart, "</a>");

				pEnd += 4;
				*pEnd = 0;

				pEnd += 1;

				ParseAHREF(pStart, url, actor);


				// role

				pStart = strstr(pEnd, "<td valign=\"top\">");
				pStart += strlen("<td valign=\"top\">");
			
				pEnd = strstr(pStart, "</td>");
				*pEnd = 0;
				
				html.ConvertHTMLToAnsi(pStart, role);

				pEnd += 1;

				// add to cast
				movieDetails.m_strCast += actor;
				if(!role.empty()) // Role not always listed
					movieDetails.m_strCast += " as " + role;
				
				movieDetails.m_strCast += "\n";
				
				// next actor
				pStart = strstr(pEnd, "<a href");
			}
		}
			
	}

	if (pTagLine)
	{
		pTagLine += strlen("Tagline:</b>");
		char *pEnd = strstr(pTagLine,"<");
		if (pEnd) *pEnd=0;
		html.ConvertHTMLToAnsi(pTagLine,movieDetails.m_strTagLine);
	}

	if (pRuntime)
	{
		pRuntime += strlen("Runtime:</b>");
		char *pEnd = strstr(pRuntime,"<");
		if (pEnd) *pEnd=0;
		html.ConvertHTMLToAnsi(pRuntime,movieDetails.m_strRuntime);
	}

	if (!pPlotOutline)
	{
		if (pPlotSummary)
		{
			pPlotSummary += strlen("Plot Summary:</b>");
			char *pEnd = strstr(pPlotSummary,"<");
			if (pEnd) *pEnd=0;
			html.ConvertHTMLToAnsi(pPlotSummary,movieDetails.m_strPlotOutline);			
		}
	}
	else
	{
		pPlotOutline += strlen("Plot Outline:</b>");
		char *pEnd = strstr(pPlotOutline,"<");
		if (pEnd) *pEnd=0;
		html.ConvertHTMLToAnsi(pPlotOutline,movieDetails.m_strPlotOutline);
		movieDetails.m_strPlot=movieDetails.m_strPlotOutline;
	}

	if (pImage)
	{
		pImage += strlen("<img border=\"0\" alt=\"cover\" src=\"");
		char *pEnd = strstr(pImage,"\"");
		if (pEnd) *pEnd=0;
		movieDetails.m_strPictureURL=pImage;
		movieDetails.m_strPictureURL.Replace('\r',(char)0);
	}

	if (pPlot)
	{
		CStdString strPlotURL = url.m_strURL;
    if (idxUrlParams != -1)
    {
      strPlotURL = strPlotURL.Left(idxUrlParams);
    }
    strPlotURL = strPlotURL + "plotsummary";
		CStdString strPlotHTML;
		if ( m_http.Get(strPlotURL,strPlotHTML))
		{
			if (0!=strPlotHTML.size())
			{
				char* szPlotHTML = new char[1+strPlotHTML.size()];
				strcpy(szPlotHTML ,strPlotHTML.c_str());
				char *strPlotStart=strstr(szPlotHTML,"<p class=\"plotpar\">");
				if (strPlotStart)
				{
					strPlotStart += strlen("<p class=\"plotpar\">");
					char *strPlotEnd=strstr(strPlotStart,"</p>");
					if (strPlotEnd) *strPlotEnd=0;

					// Remove any tags that may appear in the text
					bool inTag = false;
					int iPlot = 0;
					char* strPlot = new char[strlen(strPlotStart)+1];
					for (int i = 0; i < strlen(strPlotStart); i++)
					{
						if (strPlotStart[i] == '<')
							inTag = true;
						else if (strPlotStart[i] == '>')
							inTag = false;
						else if (!inTag)
							strPlot[iPlot++] = strPlotStart[i];
					}
					strPlot[iPlot] = '\0';

					html.ConvertHTMLToAnsi(strPlot, movieDetails.m_strPlot);

					delete [] strPlot;
				}
				delete [] szPlotHTML;
			}
		}
	}
	delete [] szBuffer;

	return true;
}

void CIMDB::ParseAHREF(const char *ahref, CStdString &strURL, CStdString &strTitle)
{
	char* szAHRef;
	szAHRef=new char[strlen(ahref)+1];
	strncpy(szAHRef,ahref,strlen(ahref));
	szAHRef[strlen(ahref)]=0;
	strURL="";
	strTitle="";
	char *pStart=strstr(szAHRef,"<a href=\"");
	if (!pStart) pStart=strstr(szAHRef,"<A HREF=\"");
	if (!pStart) 
	{
		delete [] szAHRef;
		return;
	}
	char* pEnd = strstr(szAHRef,"</a>"); 
	if (!pEnd) pEnd = strstr(szAHRef,"</A>"); 

	if (!pEnd)
	{
		delete [] szAHRef;
		return;
	}
	*pEnd=0;
	pEnd+=2;
	pStart += strlen("<a href=\"");
	
	char *pSep=strstr(pStart,">");
	if (pSep) *pSep=0;
	strURL=pStart;
	pSep++;

	CHTMLUtil html;
	//strTitle=pSep;
	html.ConvertHTMLToAnsi(pSep,strTitle);
	
	delete [] szAHRef;
}

void CIMDB::ParseGenres(const char *ahref, CStdString &strURL, CStdString &strTitle)
{
	char* szAHRef;
	szAHRef=new char[strlen(ahref)+1];
	strncpy(szAHRef,ahref,strlen(ahref));
	szAHRef[strlen(ahref)]=0;

	CStdString strGenre = "";

	char *pStart;
	char *pEnd=szAHRef;
	
	char *pSlash=strstr(szAHRef," / ");

	strTitle = "";

	if(pSlash) 
	{
		char *pRealEnd = strstr(szAHRef, "(more)");
		if(!pRealEnd) pRealEnd=strstr(szAHRef, "<br><br>");
    if (!pRealEnd)
    {
      OutputDebugString("1");
    }
		while(pSlash<pRealEnd)
		{
			pStart = pEnd+2;
			pEnd = pSlash;
			*pEnd = 0; // terminate CStdString after current genre
      int iLen=pEnd-pStart;
      if (iLen < 0) break;
			char *szTemp = new char[iLen+1];
			strncpy(szTemp, pStart, iLen);
			szTemp[iLen] = 0;
	
			ParseAHREF(szTemp, strURL, strGenre);
      delete [] szTemp;
			strTitle = strTitle + strGenre + " / ";
			pSlash=strstr(pEnd+2," / ");
			if(!pSlash) pSlash = pRealEnd;
		}
	}
	// last genre
	pEnd+=2;
	ParseAHREF(pEnd, strURL, strGenre);
	strTitle = strTitle + strGenre;

	delete [] szAHRef;
}

bool CIMDB::Download(const CStdString &strURL, const CStdString &strFileName)
{
	CStdString strHTML;
	if (!m_http.Download(strURL,strFileName)) 
  {
    CLog::Log(LOGERROR, "failed to download %s -> %s", strURL.c_str(), strFileName.c_str());
    return false;
  }

	return true;
}

void CIMDBMovie::Reset()
{
  m_strDirector="";
	m_strWritingCredits="";
	m_strGenre="";
	m_strTagLine="";
	m_strPlotOutline="";
	m_strPlot="";
	m_strPictureURL="";
	m_strTitle="";
	m_strVotes="";
	m_strCast="";
	m_strSearchString="";
  m_strFile="";
  m_strPath="";
  m_strDVDLabel="";
  m_strIMDBNumber="";
	m_iTop250=0;
	m_iYear=0;
	m_fRating=0.0f;
}
void CIMDBMovie::Save(const CStdString &strFileName)
{
}

bool CIMDBMovie::Load(const CStdString& strFileName)
{

	return true;
}


void CIMDB::RemoveAllAfter(char* szMovie,const char* szSearch)
{
  char* pPtr=strstr(szMovie,szSearch);
  if (pPtr) *pPtr=0;
}

void CIMDB::GetURL(const CStdString &strMovie, CStdString& strURL)
{
//	char szURL[1024];
	char szMovie[1024];
	CIMDBUrl url;
	bool bSkip=false;
	int ipos=0;

	// skip extension
	int imax=0;
	for (int i=0; i < strMovie.size();i++)
	{
		if (strMovie[i]=='.') imax=i;
		if (i+2 < strMovie.size())
		{
			// skip -CDx. also
			if (strMovie[i]=='-')
			{
				if (strMovie[i+1]=='C' || strMovie[i+1]=='c')
				{
					if (strMovie[i+2]=='D' || strMovie[i+2]=='d')
					{
						imax=i;
						break;
					}
				}
			}
		}
	}
	if (!imax) imax=strMovie.size();
	for (int i=0; i < imax;i++)
	{
		for (int c=0;isdigit(strMovie[i+c]);c++)
		{
			if (c==3)
			{
				i+=4;
				break;
			}
		}
    char kar=strMovie[i];
		if (kar =='.') kar=' ';
    if (kar ==32) kar = '+';
		if (kar == '[' || kar=='(' ) bSkip=true;			//skip everthing between () and []
		else if (kar == ']' || kar==')' ) bSkip=false;
		else if (!bSkip)
		{
			if (ipos > 0)
			{
				if (!isalnum(kar)) 
				{
					if (szMovie[ipos-1] != '+')
					kar='+';
					else 
					kar='.';
				}
			}
			if (isalnum(kar) ||kar==' ' || kar=='+')
			{
				szMovie[ipos]=kar;
				szMovie[ipos+1]=0;
				ipos++;
			}
		}
	}

	CStdString strTmp=szMovie;
	strTmp=strTmp.ToLower();
	strTmp=strTmp.Trim();
	strcpy(szMovie,strTmp.c_str());

	RemoveAllAfter(szMovie," divx ");
	RemoveAllAfter(szMovie," xvid ");
	RemoveAllAfter(szMovie," dvd ");
	RemoveAllAfter(szMovie," svcd ");
	RemoveAllAfter(szMovie," ac3 ");
	RemoveAllAfter(szMovie," ogg ");
	RemoveAllAfter(szMovie," ogm ");
	RemoveAllAfter(szMovie," internal ");
	RemoveAllAfter(szMovie," fragment ");
	RemoveAllAfter(szMovie," dvdrip ");
	RemoveAllAfter(szMovie," proper ");
	RemoveAllAfter(szMovie," limited ");
	RemoveAllAfter(szMovie," rerip ");

	RemoveAllAfter(szMovie,"+divx+");
	RemoveAllAfter(szMovie,"+xvid+");
	RemoveAllAfter(szMovie,"+dvd+");
	RemoveAllAfter(szMovie,"+svcd+");
	RemoveAllAfter(szMovie,"+ac3+");
	RemoveAllAfter(szMovie,"+ogg+");
	RemoveAllAfter(szMovie,"+ogm+");
	RemoveAllAfter(szMovie,"+internal+");
	RemoveAllAfter(szMovie,"+fragment+");
	RemoveAllAfter(szMovie,"+dvdrip+");
	RemoveAllAfter(szMovie,"+proper+");
	RemoveAllAfter(szMovie,"+limited+");
	RemoveAllAfter(szMovie,"+rerip+");
  
//	sprintf(szURL,"http://us.imdb.com/Tsearch?title=%s", szMovie);
//	strURL = szURL;
	strURL.Format("http://%s/Tsearch?title=%s", g_stSettings.m_szIMDBurl, szMovie);
}
