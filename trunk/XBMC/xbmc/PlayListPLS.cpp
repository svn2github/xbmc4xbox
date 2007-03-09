/*
 *      Copyright (C) 2005-2007 Team XboxMediaCenter
 *      http://www.xboxmediacenter.com
 *
 *  This Program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your option)
 *  any later version.
 *
 *  This Program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with GNU Make; see the file COPYING.  If not, write to
 *  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 *  http://www.gnu.org/copyleft/gpl.html
 *
 */

#include "stdafx.h"
#include "playlistpls.h"
#include "playlistfactory.h"
#include "util.h"
#include "utils\HTTP.h"

using namespace XFILE;
using namespace PLAYLIST;

#define START_PLAYLIST_MARKER "[playlist]"
#define PLAYLIST_NAME     "PlaylistName"

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
{}

CPlayListPLS::~CPlayListPLS(void)
{}

bool CPlayListPLS::Load(const CStdString &strFile)
{
  //read it from the file
  CStdString strBasePath;
  CStdString strFileName(strFile);
  m_strPlayListName = CUtil::GetFileName(strFileName);

  Clear();

  bool bShoutCast = false;
  if( strFileName.Left(8).Equals("shout://") )
  {
    strFileName.Delete(0, 8);
    strFileName.Insert(0, "http://");
    strBasePath = "";
    bShoutCast = true;
  }
  else
    CUtil::GetParentPath(strFileName, strBasePath);

  CFile file;
  if (!file.Open(strFileName, false) )
  {
    file.Close();
    return false;
  }

  char szLine[4096];
  CStdString strLine;

  // run through looking for the [playlist] marker.
  // if we find another http stream, then load it.
  while (1)
  {    
    if ( !file.ReadString(szLine, sizeof(szLine) ) )
    {
      file.Close();
      return size() > 0;
    }
    strLine = szLine;
    strLine.TrimLeft(" \t");
    strLine.TrimRight(" \n\r");
    if(strLine.Equals(START_PLAYLIST_MARKER))
      break;

    // try to parse as an url
    CURL url(strLine);
     
    // if it isn't an url, we are not interested
    if (url.GetProtocol().IsEmpty())
      continue;

    // if it's shoutcast, replace the http with shout
    if (bShoutCast && url.GetProtocol().Equals("http") )
      strLine.Replace("http:", "shout:");
    
    // add this to playlist
    CPlayListItem newItem(strLine, strLine, 0);
    Add(newItem);
  }

  int iMaxSize = 0;
  while (file.ReadString(szLine, sizeof(szLine) ) )
  {
    strLine = szLine;
    StringUtils::RemoveCRLF(strLine);
    int iPosEqual = strLine.Find("=");
    if (iPosEqual > 0)
    {
      CStdString strLeft = strLine.Left(iPosEqual);
      iPosEqual++;
      CStdString strValue = strLine.Right(strLine.size() - iPosEqual);
      strLeft.ToLower();
      while (strLeft[0] == ' ' || strLeft[0] == '\t')
        strLeft.erase(0,1);

      if (strLeft == "numberofentries")
      {
        m_vecItems.reserve(atoi(strValue.c_str()));
      }
      else if (strLeft.Left(4) == "file")
      {
        vector <int>::size_type idx = atoi(strLeft.c_str() + 4);
        if (idx > m_vecItems.size()) {
          iMaxSize = idx;
          m_vecItems.resize(idx);
        }
        if (m_vecItems[idx - 1].GetDescription().empty())
          m_vecItems[idx - 1].SetDescription(CUtil::GetFileName(strValue));
        CFileItem item(strValue, false);
        if (bShoutCast && !item.IsAudio())
          strValue.Replace("http:", "shout:");

        if (CUtil::IsRemote(strBasePath) && g_advancedSettings.m_pathSubstitutions.size() > 0)
         strValue = CUtil::SubstitutePath(strValue);
        CUtil::GetQualifiedFilename(strBasePath, strValue);
        g_charsetConverter.stringCharsetToUtf8(strValue);
        m_vecItems[idx - 1].SetFileName(strValue);
      }
      else if (strLeft.Left(5) == "title")
      {
        vector <int>::size_type idx = atoi(strLeft.c_str() + 5);
        if (idx > m_vecItems.size()) {
          iMaxSize = idx;
          m_vecItems.resize(idx);
        }
        g_charsetConverter.stringCharsetToUtf8(strValue);
        m_vecItems[idx - 1].SetDescription(strValue);
      }
      else if (strLeft.Left(6) == "length")
      {
        vector <int>::size_type idx = atoi(strLeft.c_str() + 6);
        if (idx > m_vecItems.size()) {
          iMaxSize = idx;
          m_vecItems.resize(idx);
        }
        m_vecItems[idx - 1].SetDuration(atol(strValue.c_str()));
      }
      else if (strLeft == "playlistname")
      {
        m_strPlayListName = strValue;
        g_charsetConverter.stringCharsetToUtf8(m_strPlayListName);
      }
    }
  }
  file.Close();
  m_vecItems.resize(iMaxSize);

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
  if (!m_vecItems.size()) return ;
  CStdString strPlaylist = strFileName;
  // force HD saved playlists into fatx compliance
  if (CUtil::IsHD(strPlaylist))
    CUtil::GetFatXQualifiedPath(strPlaylist);
  FILE *fd = fopen(strPlaylist.c_str(), "w+");
  if (!fd)
  {
    CLog::Log(LOGERROR, "Could not save PLS playlist: [%s]", strPlaylist.c_str());
    return ;
  }
  fprintf(fd, "%s\n", START_PLAYLIST_MARKER);
  CStdString strPlayListName=m_strPlayListName;
  g_charsetConverter.utf8ToStringCharset(strPlayListName);
  fprintf(fd, "PlaylistName=%s\n", strPlayListName.c_str() );

  for (int i = 0; i < (int)m_vecItems.size(); ++i)
  {
    const CPlayListItem& item = m_vecItems[i];
    CStdString strFileName=item.GetFileName();
    g_charsetConverter.utf8ToStringCharset(strFileName);
    CStdString strDescription=item.GetDescription();
    g_charsetConverter.utf8ToStringCharset(strDescription);
    fprintf(fd, "File%i=%s\n", i + 1, strFileName.c_str() );
    fprintf(fd, "Title%i=%s\n", i + 1, strDescription.c_str() );
    fprintf(fd, "Length%i=%i\n", i + 1, item.GetDuration() / 1000 );
  }

  fprintf(fd, "NumberOfEntries=%i\n", m_vecItems.size());
  fprintf(fd, "Version=2\n");
  fclose(fd);
}

bool CPlayListASX::LoadAsxIniInfo(const CStdString& strData)
{
  CLog::Log(LOGINFO, "Parsing INI style ASX");
  string::size_type equals = 0, end = 0;
  CStdString strMMS;

  while ((equals = strData.find('=', end)) != string::npos)
  {
    end = strData.find('\r', equals);
    if (end == string::npos)
    {
      strMMS = strData.substr(equals + 1);
    }
    else
    {
      strMMS = strData.substr(equals + 1, end - equals - 1);
    }
    CLog::Log(LOGINFO, "Adding element %s", strMMS.c_str());
    CPlayListItem newItem(strMMS, strMMS, 0);
    Add(newItem);
  }
  return true;
}

bool CPlayListASX::LoadData(const CStdString& strData)
{
  CLog::Log(LOGNOTICE, "Parsing ASX");

  // Check for [Reference] format first
  if (strData[0] == '[')
  {
    return LoadAsxIniInfo(strData);
  }
  else
  {
    // Parse XML format
    // Now load the XML file
    TiXmlDocument xmlDoc;
    xmlDoc.Parse(strData.c_str());
    if (xmlDoc.Error())
    {
      CLog::Log(LOGERROR, "Unable to parse ASX info from XML:\n%s\nError: %s", strData.c_str(), xmlDoc.ErrorDesc());
      return false;
    }

    TiXmlElement *pRootElement = xmlDoc.RootElement();

    // lowercase every element
    TiXmlNode *pNode = pRootElement;
    TiXmlNode *pChild = NULL;
    CStdString value;
    value = pNode->Value();
    value.ToLower();
    pNode->SetValue(value);
    while(pNode)
    {
      pChild = pNode->IterateChildren(pChild);
      if(pChild)
      {
        if (pChild->Type() == TiXmlNode::ELEMENT)
        {
          value = pChild->Value();
          value.ToLower();
          pChild->SetValue(value);

          TiXmlAttribute* pAttr = pChild->ToElement()->FirstAttribute();
          while(pAttr)
          {
            value = pAttr->Name();
            value.ToLower();
            pAttr->SetName(value);
            pAttr = pAttr->Next();
          }
        }

        pNode = pChild;
        pChild = NULL;
        continue;
      }

      pChild = pNode;
      pNode = pNode->Parent();
    }
    CStdString roottitle = "";
    TiXmlElement *pElement = pRootElement->FirstChildElement();
    while (pElement)
    {
      value = pElement->Value();
      if (value == "title")
      {
        roottitle = pElement->GetText();
      }
      else if (value == "entry")
      {
        CStdString title(roottitle);

        TiXmlElement *pRef = pElement->FirstChildElement("ref");
        TiXmlElement *pTitle = pElement->FirstChildElement("title");
        TiXmlElement *pDuration = pElement->FirstChildElement("duration"); /* <DURATION VALUE="hh:mm:ss.fract"/> */

        if(pTitle)
          title = pTitle->GetText();

        while (pRef)
        { // multiple references may apear for one entry
          // duration may exist on this level too
          value = pRef->Attribute("href");
          if (value != "")
          {
            if(title.IsEmpty())
              title = value;

            CLog::Log(LOGINFO, "Adding element %s, %s", title.c_str(), value.c_str());
            CPlayListItem newItem(title, value, 0);
            Add(newItem);
          }
          pRef = pRef->NextSiblingElement("ref");
        }
      }
      else if (value == "entryref")
      {
        value = pElement->Attribute("href");
        if (value != "")
        { // found an entryref, let's try loading that url
          auto_ptr<CPlayList> playlist(CPlayListFactory::Create(value));
          if (NULL != playlist.get())
            if (playlist->Load(value))
              Add(*playlist);
        }
      }
      pElement = pElement->NextSiblingElement();
    }
  }

  return true;
}


bool CPlayListRAM::LoadData(const CStdString& strData)
{
  CLog::Log(LOGINFO, "Parsing RAM");
  CLog::Log(LOGDEBUG, "%s", strData.c_str());
  CStdString strMMS;

  strMMS = strData.substr(0, strData.Find('\n'));
  CLog::Log(LOGINFO, "Adding element %s", strMMS.c_str());
  CPlayListItem newItem(strMMS, strMMS, 0);
  Add(newItem);
  return true;
}