
#include "stdafx.h"
#include "playlistpls.h"
#include "util.h"
#include "filesystem/file.h"
#include "url.h"

#define START_PLAYLIST_MARKER "[playlist]"
#define PLAYLIST_NAME					"PlaylistName"
using namespace PLAYLIST;
/*----------------------------------------------------------------------
[playlist]
PlaylistName=Playlist 001
File1=E:\Program Files\Winamp3\demo.mp3
Title1=demo
Length1=5
File2=E:\Program Files\Winamp3\demo.mp3
Title2=demo
Length2=5
NumberOfEntries=2
Version=2
----------------------------------------------------------------------*/
CPlayListPLS::CPlayListPLS(void)
{
}

CPlayListPLS::~CPlayListPLS(void)
{
}


bool CPlayListPLS::Load(const CStdString& strFileName)
{
	CStdString strBasePath;
  bool bShoutCast=false;
  CStdString strExt=CUtil::GetExtension(strFileName);
  strExt.ToLower();
  if ( CUtil::cmpnocase(strExt,".pls")==0) bShoutCast=true;

	Clear();
	m_strPlayListName=CUtil::GetFileName(strFileName);
	CUtil::GetParentPath(strFileName,strBasePath);
	CFile file;
	if (!file.Open(strFileName,false) ) 
	{
		file.Close();
		return false;
	}

	char szLine[4096];
	if ( !file.ReadString(szLine, sizeof(szLine) ) )
	{
		file.Close();
		return false;
	}
	CStdString strLine=szLine;
	CUtil::RemoveCRLF(strLine);
	if (strLine != START_PLAYLIST_MARKER)
	{
    CURL url(strLine);
    if (url.GetProtocol() == "http" ||url.GetProtocol() == "mms"||url.GetProtocol() == "rtp")
    {
			if (bShoutCast) strLine.Replace("http:","shout:");
			CPlayListItem newItem(strLine,strLine,0);
			Add(newItem);
      file.Close();
      return true;
    }
		file.Close();
		return false;
	}

	while (file.ReadString(szLine,sizeof(szLine) ) )
	{
		strLine=szLine;
		CUtil::RemoveCRLF(strLine);
		int iPosEqual=strLine.Find("=");
		if (iPosEqual>0)
		{
			CStdString strLeft =strLine.Left(iPosEqual);
			iPosEqual++;
			CStdString strValue=strLine.Right(strLine.size()-iPosEqual);
			strLeft.ToLower();

			if (strLeft == "numberofentries")
			{
				m_vecItems.reserve(atoi(strValue.c_str()));
			}
			else if (strLeft.Left(4) == "file")
			{	
				int idx = atoi(strLeft.c_str() + 4);
				m_vecItems.resize(idx);
				if (m_vecItems[idx-1].GetDescription().empty())
					m_vecItems[idx-1].SetDescription(CUtil::GetFileName(strValue));
				if (bShoutCast)
					strValue.Replace("http:","shout:");
				CUtil::GetQualifiedFilename(strBasePath,strValue);
				m_vecItems[idx-1].SetFileName(strValue);
			}
			else if (strLeft.Left(5) == "title")
			{	
				int idx = atoi(strLeft.c_str() + 5);
				m_vecItems.resize(idx);
				m_vecItems[idx-1].SetDescription(strValue);
			}
			else if (strLeft.Left(6) == "length")
			{	
				int idx = atoi(strLeft.c_str() + 6);
				m_vecItems.resize(idx);
				m_vecItems[idx-1].SetDuration(atol(strValue.c_str()));
			}
			else if (strLeft=="playlistname")
			{
				m_strPlayListName=strValue;
			}
		}		
	}
	file.Close();

	// check for missing entries
	for (ivecItems p = m_vecItems.begin(); p != m_vecItems.end(); ++p)
	{
		while (p->GetFileName().empty() && p != m_vecItems.end())
		{
			p = m_vecItems.erase(p);
		}
	}

	return true;
}

void CPlayListPLS::Save(const CStdString& strFileName) const
{
	if (!m_vecItems.size()) return;
	FILE *fd = fopen(strFileName.c_str(),"w+");
	if (!fd) return;
	fprintf(fd,"%s\n",START_PLAYLIST_MARKER);	
	fprintf(fd,"PlaylistName=%s\n",m_strPlayListName.c_str() );	

	for (int i=0; i < (int)m_vecItems.size(); ++i)
	{
		const CPlayListItem& item = m_vecItems[i];
		fprintf(fd,"File%i=%s\n",i+1, item.GetFileName().c_str() );
		fprintf(fd,"Title%i=%s\n",i+1, item.GetDescription().c_str() );
		fprintf(fd,"Length%i=%i\n",i+1, item.GetDuration()/1000 );
	}
	
	fprintf(fd,"NumberOfEntries=%i\n",m_vecItems.size());
	fprintf(fd,"Version=2\n");
	fclose(fd);
}
