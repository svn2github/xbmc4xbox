/*
 *      Copyright (C) 2005-2009 Team XBMC
 *      http://www.xbmc.org
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
 *  along with XBMC; see the file COPYING.  If not, write to
 *  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 *  http://www.gnu.org/copyleft/gpl.html
 *
 */

#include "FGLoader.h"
#include "dshowutil/dshowutil.h"
#include "tinyXML/tinyxml.h"
#include "XMLUtils.h"
#include "charsetconverter.h"
#include "Log.h"
#include "FileSystem/SpecialProtocol.h"
using namespace std;
CFGLoader::CFGLoader(IGraphBuilder2* gb,CStdString xbmcPath)
:m_pGraphBuilder(gb)
,m_xbmcPath(xbmcPath)
{
}

CFGLoader::~CFGLoader()
{
}

HRESULT CFGLoader::LoadConfig(CStdString configFile)
{
  HRESULT hr = S_OK;
  m_xbmcConfigFilePath = configFile;
  m_xbmcPath.Replace("\\","\\\\");
  if (!CFile::Exists(configFile))
    return false;
  TiXmlDocument graphConfigXml;
  if (!graphConfigXml.LoadFile(configFile))
    return false;
  TiXmlElement* graphConfigRoot = graphConfigXml.RootElement();
  if ( !graphConfigRoot)
    return false;
  CStdString strFPath;
  CStdString strFGuid;
  CStdString strFileType;
  CStdString strTrimedPath;
  CStdString strTmpFilterName;
  CStdString strTmpFilterType;
  CFGFilterFile* pFGF;
  TiXmlElement *pFilters = graphConfigRoot->FirstChildElement("filters");
  pFilters = pFilters->FirstChildElement("filter");
  while (pFilters)
  {
    strTmpFilterName = pFilters->Attribute("name");
    strTmpFilterType = pFilters->Attribute("type");
    XMLUtils::GetString(pFilters,"path",strFPath);

    CStdString strPath2;
    strPath2.Format("special://xbmc/system/players/dsplayer/%s", strFPath.c_str());
    if (!CFile::Exists(strFPath) && CFile::Exists(strPath2))
      strFPath = _P(strPath2);

	XMLUtils::GetString(pFilters,"guid",strFGuid);
    XMLUtils::GetString(pFilters,"filetype",strFileType);
    pFGF = new CFGFilterFile(DShowUtil::GUIDFromCString(strFGuid),strFPath,L"",MERIT64_ABOVE_DSHOW+2,strTmpFilterName,strFileType);
    if (strTmpFilterName.Equals("mpcvideodec",false))
      m_mpcVideoDecGuid = DShowUtil::GUIDFromCString(strFGuid);

  if (pFGF)
    m_configFilter.AddTail(pFGF);

  pFilters = pFilters->NextSiblingElement();
  
  }
}

HRESULT CFGLoader::LoadFilterRules(CStdString fileType , CComPtr<IBaseFilter> fileSource ,IBaseFilter **pBFSplitter)
{
  HRESULT hr;
  CheckPointer(pBFSplitter,E_POINTER);
  IBaseFilter *ppSBF = NULL;
  //Load the rules from the xml
  TiXmlDocument graphConfigXml;
  if (!graphConfigXml.LoadFile(m_xbmcConfigFilePath))
    return false;
  TiXmlElement* graphConfigRoot = graphConfigXml.RootElement();
  if ( !graphConfigRoot)
    return false;
  CInterfaceList<IUnknown, &IID_IUnknown> pUnk;
  TiXmlElement *pRules = graphConfigRoot->FirstChildElement("rules");
  pRules = pRules->FirstChildElement("rule");
  while (pRules)
  {
    if (((CStdString)pRules->Attribute("filetypes")).Equals(fileType.c_str(),false))
    {
      POSITION pos = m_configFilter.GetHeadPosition();
      while(pos)
      {
        CFGFilterFile* pFGF = m_configFilter.GetNext(pos);
        if ( ((CStdString)pRules->Attribute("splitter")).Equals(pFGF->GetXFilterName().c_str(),false) )
        {
          if(SUCCEEDED(pFGF->Create(&ppSBF, pUnk)))
          {
            m_pGraphBuilder->AddFilter(ppSBF,DShowUtil::AnsiToUTF16(pFGF->GetXFilterName().c_str()).c_str());//L"XBMC SPLITTER");
            hr = ::ConnectFilters(m_pGraphBuilder,fileSource,ppSBF);
            if ( SUCCEEDED( hr ) )
              //Now the source filter is connected to the splitter lets load the rules from the xml
              CLog::Log(LOGDEBUG,"%s Connected the source to the spillter",__FUNCTION__);
            else
            {
              CLog::Log(LOGERROR,"%s Failed to connect the source to the spliter",__FUNCTION__);
              return hr;
            }
		      }
    		}
        else if ( ((CStdString)pRules->Attribute("videodec")).Equals(pFGF->GetXFilterName().c_str(),false) )
        {
        CComPtr<IBaseFilter> ppBF;
        if(SUCCEEDED(pFGF->Create(&ppBF, pUnk)))
          m_pGraphBuilder->AddFilter(ppBF,DShowUtil::AnsiToUTF16(pFGF->GetXFilterName().c_str()).c_str());
        ppBF.Release();
      }
      else if ( ((CStdString)pRules->Attribute("audiodec")).Equals(pFGF->GetXFilterName().c_str(),false) )
      {
        CComPtr<IBaseFilter> ppBF;
        if(SUCCEEDED(pFGF->Create(&ppBF, pUnk)))
        {
          m_pGraphBuilder->AddFilter(ppBF,DShowUtil::AnsiToUTF16(pFGF->GetXFilterName().c_str()).c_str());
        }
        ppBF.Release();
      }
    }
  }
    pRules = pRules->NextSiblingElement();
  }
  if(!ppSBF)
  {
    CLog::Log(LOGERROR,"%s Failed to create ppSBF",__FUNCTION__);
    return E_FAIL;
  }
  *pBFSplitter = ppSBF;
  (*pBFSplitter)->AddRef();
  SAFE_RELEASE(ppSBF);
  return hr;
}