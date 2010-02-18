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

#include "XMLUtils.h"
#include "charsetconverter.h"
#include "Log.h"

#include "filters/xbmcfilesource.h"

#include "FileSystem/SpecialProtocol.h"
#include "XMLUtils.h"
#include "WINDirectShowEnumerator.h"
#include "GuiSettings.h"
#include "filters/VMR9AllocatorPresenter.h"
#include "filters/EVRAllocatorPresenter.h"

#include <ks.h>
#include <Codecapi.h>
#include "streamsmanager.h"
#include "FilterCoreFactory/FilterSelectionRule.h"

using namespace std;

bool CompareCFGFilterFileToString(CFGFilterFile * f, CStdString s)
{
  return f->GetXFilterName().Equals(s);
}

DIRECTSHOW_RENDERER CFGLoader::m_CurrentRenderer = DIRECTSHOW_RENDERER_UNDEF;

CFGLoader::CFGLoader():
  m_pGraphBuilder(NULL),
  m_pFGF(NULL)
{
  m_SourceF = NULL;
  m_SplitterF = NULL;
  m_VideoDecF = NULL;
  m_AudioDecF = NULL;
  m_AudioRendererF = NULL;
  m_VideoRendererF = NULL;
  m_UsingDXVADecoder = false;
}

CFGLoader::~CFGLoader()
{
  CAutoLock cAutoLock(this);
  while(!m_configFilter.empty())
  {
    if (m_configFilter.back())
      delete m_configFilter.back();
    m_configFilter.pop_back();
  }

  m_configFilter.clear();

  SAFE_DELETE(m_pFGF);
}




HRESULT CFGLoader::InsertSourceFilter(const CFileItem& pFileItem, const CStdString& filterName)
{

  HRESULT hr = E_FAIL;
  m_SourceF = NULL;

  if (CUtil::IsInArchive(pFileItem.m_strPath))
  {
    CLog::Log(LOGNOTICE,"%s File \"%s\" need a custom source filter", __FUNCTION__, pFileItem.m_strPath.c_str());
	  if(m_File.Open(pFileItem.m_strPath, READ_TRUNCATED | READ_BUFFERED))
	  {
	    CXBMCFileStream* pXBMCStream = new CXBMCFileStream(&m_File, &m_SourceF, &hr);

	    if (SUCCEEDED(hr = m_pGraphBuilder->AddFilter(m_SourceF, L"XBMC File Source")))
	    {
		    CLog::Log(LOGNOTICE, "%s Successfully added xbmc source filter to the graph", __FUNCTION__);
	    } else {
		    CLog::Log(LOGERROR, "%s Failted to add xbmc source filter to the graph", __FUNCTION__);
	    }

	    m_pStrSource = "XBMC File Source";
	    return hr;
    } else {
      CLog::Log(LOGERROR, "%s Failed to open \"%s\" with source filter!", __FUNCTION__, pFileItem.m_strPath.c_str());
      return E_FAIL;
    }
  } else {  
    if (SUCCEEDED(hr = InsertFilter(filterName, &m_SourceF, m_pStrSource)))
    {
      m_SplitterF = m_SourceF;
      CStdString pWinFilePath = pFileItem.m_strPath;
      if ( (pWinFilePath.Left(6)).Equals("smb://", false) )
        pWinFilePath.Replace("smb://", "\\\\");
      
      pWinFilePath.Replace("/", "\\");

      IFileSourceFilter *pFS = NULL;
	    m_SplitterF->QueryInterface(IID_IFileSourceFilter, (void**)&pFS);
      
      CStdStringW strFileW;    
      g_charsetConverter.utf8ToW(pWinFilePath, strFileW);

      if (SUCCEEDED(hr = pFS->Load(strFileW.c_str(), NULL)))
        CLog::Log(LOGNOTICE, "%s Successfully loaded file in the splitter", __FUNCTION__);
      else
        CLog::Log(LOGERROR, "%s Failed to load file in the splitter", __FUNCTION__);
    }
  }

  return hr;  
}
HRESULT CFGLoader::InsertSplitter(const CFileItem& pFileItem, const CStdString& filterName)
{
  HRESULT hr = InsertFilter(filterName, &m_SplitterF, m_pStrSplitter);

  if (SUCCEEDED(hr))
  {
    if (SUCCEEDED(hr = ConnectFilters(m_pGraphBuilder, m_SourceF, m_SplitterF)))
      CLog::Log(LOGNOTICE, "%s Successfully connected the source to the spillter", __FUNCTION__);
    else
      CLog::Log(LOGERROR, "%s Failed to connect the source to the spliter", __FUNCTION__);
  }
  
  return hr;
}

/*HRESULT CFGLoader::InsertAudioDecoder(const CStdString& filterName)
{
  list<CFGFilterFile *>::const_iterator it = std::find_if(m_configFilter.begin(),
    m_configFilter.end(),
    std::bind2nd(std::ptr_fun(CompareCFGFilterFileToString), filterName) );

  if (it == m_configFilter.end())
  {

    CLog::Log(LOGERROR, "%s Filter \"%s\" isn't loaded. Please check dsfilterconfig.xml", __FUNCTION__, filterName.c_str());
    return E_FAIL;

  } else {

    g_charsetConverter.wToUTF8((*it)->GetName(), m_pStrAudiodec);
    if(SUCCEEDED((*it)->Create(&m_AudioDecF)))
    {
      if (SUCCEEDED(m_pGraphBuilder->AddFilter(m_AudioDecF, (*it)->GetName().c_str())))
        CLog::Log(LOGNOTICE, "%s Successfully added \"%s\" to the graph", __FUNCTION__, m_pStrAudiodec.c_str());
      else {
        CLog::Log(LOGERROR, "%s Failed to add \"%s\" to the graph", __FUNCTION__, m_pStrAudiodec.c_str());
        m_pStrAudiodec = "";
        return E_FAIL;
      }
    }
    else {
      CLog::Log(LOGERROR, "%s Failed to create the audio decoder filter", __FUNCTION__);
      return E_FAIL;
    }
  }

  return S_OK;
}

HRESULT CFGLoader::InsertVideoDecoder(const CStdString& filterName)
{
  list<CFGFilterFile *>::const_iterator it = std::find_if(m_configFilter.begin(),
    m_configFilter.end(),
    std::bind2nd(std::ptr_fun(CompareCFGFilterFileToString), filterName) );

  if (it == m_configFilter.end())
  {

    CLog::Log(LOGERROR, "%s Filter \"%s\" isn't loaded. Please check dsfilterconfig.xml", __FUNCTION__, filterName.c_str());
    return E_FAIL;

  } else {
  
    g_charsetConverter.wToUTF8((*it)->GetName(), m_pStrVideodec);
    if(SUCCEEDED((*it)->Create(&m_VideoDecF)))
    {
      if (SUCCEEDED(m_pGraphBuilder->AddFilter(m_VideoDecF, (*it)->GetName().c_str())))
      {
        CLog::Log(LOGNOTICE, "%s Successfully added \"%s\" to the graph", __FUNCTION__, m_pStrVideodec.c_str());
      } else {
        CLog::Log(LOGERROR, "%s Failed to add \"%s\" to the graph", __FUNCTION__, m_pStrVideodec.c_str());
        return E_FAIL;
      }
    }
    else
    {
      CLog::Log(LOGERROR,"%s Failed to create video decoder filter \"%s\"", __FUNCTION__, m_pStrVideodec.c_str());
      m_pStrVideodec = "";
      return E_FAIL;
    }
  }
  
  return S_OK;
}*/

HRESULT CFGLoader::InsertAudioRenderer()
{
  HRESULT hr = S_OK;
  CFGFilterRegistry* pFGF;
  CStdString currentGuid,currentName;

  CDirectShowEnumerator p_dsound;
  std::vector<DSFilterInfo> deviceList = p_dsound.GetAudioRenderers();
  
  //see if there a config first 
  for (std::vector<DSFilterInfo>::const_iterator iter = deviceList.begin();
    iter != deviceList.end(); ++iter)
  {
    DSFilterInfo dev = *iter;
    if (g_guiSettings.GetString("dsplayer.audiorenderer").Equals(dev.lpstrName))
    {
      currentGuid = dev.lpstrGuid;
      currentName = dev.lpstrName;
      break;
    }
  }
  if (currentName.IsEmpty())
  {
    currentGuid = DShowUtil::CStringFromGUID(CLSID_DSoundRender);
    currentName.Format("Default DirectSound Device");
  }

  m_pStrAudioRenderer = currentName;
  pFGF = new CFGFilterRegistry(DShowUtil::GUIDFromCString(currentGuid));
  hr = pFGF->Create(&m_AudioRendererF);
  hr = m_pGraphBuilder->AddFilter(m_AudioRendererF, DShowUtil::AnsiToUTF16(currentName));

  if (SUCCEEDED(hr))
    CLog::Log(LOGNOTICE, "%s Successfully added \"%s\" to the graph", __FUNCTION__, m_pStrAudioRenderer.c_str());
  else
    CLog::Log(LOGNOTICE, "%s Failed to add \"%s\" to the graph (result: %X)", __FUNCTION__, m_pStrAudioRenderer.c_str(), hr);

  return hr;
}

HRESULT CFGLoader::InsertVideoRenderer()
{
  HRESULT hr = S_OK;
  
  if (g_sysinfo.IsVistaOrHigher())
  {
    if (g_guiSettings.GetBool("dsplayer.forcenondefaultrenderer"))
      m_CurrentRenderer = DIRECTSHOW_RENDERER_VMR9;
    else 
      m_CurrentRenderer = DIRECTSHOW_RENDERER_EVR;
  }
  else
  {
    if (g_guiSettings.GetBool("dsplayer.forcenondefaultrenderer"))
      m_CurrentRenderer = DIRECTSHOW_RENDERER_EVR;
    else 
      m_CurrentRenderer = DIRECTSHOW_RENDERER_VMR9;
  }
    

  // Renderers
  if (m_CurrentRenderer == DIRECTSHOW_RENDERER_EVR)
    m_pFGF = new CFGFilterVideoRenderer(__uuidof(CEVRAllocatorPresenter), L"Xbmc EVR");
  else
    m_pFGF = new CFGFilterVideoRenderer(__uuidof(CVMR9AllocatorPresenter), L"Xbmc VMR9 (Renderless)");
  hr = m_pFGF->Create(&m_VideoRendererF);
  hr = m_pGraphBuilder->AddFilter(m_VideoRendererF, m_pFGF->GetName());
  if (SUCCEEDED(hr))
  {
    CLog::Log(LOGDEBUG, "%s Allocator presenter successfully added to the graph (Renderer: %d)",  __FUNCTION__, m_CurrentRenderer);
  } else {
    CLog::Log(LOGERROR, "%s Failed to add allocator presenter to the graph (hr = %X)", __FUNCTION__, hr);
  }

  return hr; 
}

HRESULT CFGLoader::InsertAutoLoad()
{
  HRESULT hr = S_OK;
  IBaseFilter* ppBF;
  for (list<CFGFilterFile*>::iterator it = m_configFilter.begin(); it != m_configFilter.end(); it++)
  { 
    if ( (*it)->GetAutoLoad() )
    {
      if (SUCCEEDED((*it)->Create(&ppBF)))
	    {
        m_pGraphBuilder->AddFilter(ppBF,(*it)->GetName().c_str());
        ppBF = NULL;
	    }
	    else
	    {
        CLog::Log(LOGDEBUG,"DSPlayer %s Failed to create the auto loading filter called %s",__FUNCTION__,(*it)->GetXFilterName().c_str());
      }
    }
  }  
  SAFE_RELEASE(ppBF);
  CLog::Log(LOGDEBUG,"DSPlayer %s Is done adding the autoloading filters",__FUNCTION__);
  return hr;
}

/*
HRESULT CFGLoader::InsertExtraFilter( const CStdString& filterName )
{
  IBaseFilter* ppBF = NULL;
  CStdString extraName;

  list<CFGFilterFile *>::const_iterator it = std::find_if(m_configFilter.begin(),
    m_configFilter.end(),
    std::bind2nd(std::ptr_fun(CompareCFGFilterFileToString), filterName) );

  if (it == m_configFilter.end())
  {

    CLog::Log(LOGERROR, "%s Filter \"%s\" isn't loaded. Please check dsfilterconfig.xml", __FUNCTION__, filterName.c_str());
    return E_FAIL;

  } else {

    g_charsetConverter.wToUTF8((*it)->GetName(), extraName);
    if(SUCCEEDED((*it)->Create(&ppBF)))
    {
      if (SUCCEEDED(m_pGraphBuilder->AddFilter(ppBF,(*it)->GetName().c_str())))
      {
        CLog::Log(LOGNOTICE, "%s Successfully added \"%s\" to the graph", __FUNCTION__, extraName.c_str());
      } else {
        CLog::Log(LOGERROR, "%s Failed to add \"%s\" to the graph", __FUNCTION__, extraName.c_str());
        return E_FAIL;
      }
    }
    else
    {
      CLog::Log(LOGERROR,"%s Failed to create extra filter \"%s\"", __FUNCTION__, extraName.c_str());
      return E_FAIL;
    }
  }
  m_extraFilters.push_back(ppBF);
  return S_OK;
}
*/

HRESULT CFGLoader::LoadFilterRules(const CFileItem& pFileItem)
{
  //Load the rules from the xml
  TiXmlDocument graphConfigXml;
  if (!graphConfigXml.LoadFile(m_xbmcConfigFilePath))
    return E_FAIL;
  TiXmlElement* graphConfigRoot = graphConfigXml.RootElement();
  if ( !graphConfigRoot)
    return E_FAIL;
  
  TiXmlElement *pRules = graphConfigRoot->FirstChildElement("rules");
  pRules = pRules->FirstChildElement("rule");
  m_SplitterF = NULL;

  CStdString extension = pFileItem.GetAsUrl().GetFileType();
  //CStdString filterName;
  bool extensionNotFound = true;

  for (pRules; pRules; pRules = pRules->NextSiblingElement())
  {
    if (! pRules->Attribute("filetype"))
      continue;

    if (! ((CStdString)pRules->Attribute("filetype")).Equals(extension) )
      continue;

    extensionNotFound = false;

    /* Check validity */
    if (! pRules->FirstChild("source") ||
      ! pRules->FirstChild("splitter") ||
      ! pRules->FirstChild("video") ||
      ! pRules->FirstChild("audio"))
    {
      return E_FAIL;
    }
    InsertAudioRenderer(); // First added, last connected
    InsertVideoRenderer();

    CFilterSelectionRule *c = NULL;
    std::vector<CStdString> filters;

    c = new CFilterSelectionRule(pRules->FirstChildElement("source"), "source");
    c->GetFilters(pFileItem, filters);
    delete c;

    if (filters.empty())
    {
      //TODO: Error message
      return E_FAIL;
    }

    if (FAILED(InsertSourceFilter(pFileItem, filters[0])))
	  {
		  return E_FAIL;
	  }

    filters.clear();

    if (! m_SplitterF)
    {
      c = new CFilterSelectionRule(pRules->FirstChildElement("splitter"), "splitter");
      c->GetFilters(pFileItem, filters);
      delete c;

      if (filters.empty())
        return E_FAIL; //TODO: Error message
      
      if (FAILED(InsertSplitter(pFileItem, filters[0])))
      {
        return E_FAIL;
      }

      filters.clear();
    }

    /* Ok, the splitter is added to the graph. We need to detect the video stream codec in order to choose
    if we use or not the dxva filter */

    IBaseFilter *pBF = m_SplitterF;
    bool fourccfinded = false;
    BeginEnumPins(pBF, pEP, pPin)
    {
      if (fourccfinded)
        break;

      BeginEnumMediaTypes(pPin, pEMT, pMT)
      {
        if (pMT->majortype != MEDIATYPE_Video)
          break; // Next pin

        SVideoStreamInfos s;
        s.Clear();

        CStreamsManager::getSingleton()->GetStreamInfos(pMT, &s);

        fourccfinded = true;
        // CCV1 is a fake fourcc code generated by Haali in order to trick the Microsoft h264
        // decoder included in Windows 7. ffdshow handle that fourcc well, but it may not be
        // the case for others decoders
        if (s.fourcc == 'H264' || s.fourcc == 'AVC1' || s.fourcc == 'WVC1'
          || s.fourcc == 'WMV3' || s.fourcc == 'WMVA' || s.fourcc == 'CCV1')
          m_UsingDXVADecoder = true;

        if (s.fourcc == 'CCV1')
        {
          CLog::Log(LOGINFO, "Haali Media Splitter is configured for using a fake fourcc code 'CCV1'. If you've an error message, be sure your video decoder handle that fourcc well.");
        }

        break;
      }
      EndEnumMediaTypes(pEMT, pMT)
    }
    EndEnumPins(pEP, pBF)

    c = new CFilterSelectionRule(pRules->FirstChildElement("video"), "video");
    c->GetFilters(pFileItem, filters, m_UsingDXVADecoder);
    delete c;

    if (filters.empty())
      return E_FAIL; //TODO: Error message

    if (FAILED(InsertFilter(filters[0], &m_VideoDecF, m_pStrVideodec)))
    {
      return E_FAIL;
    }

    filters.clear();

    c = new CFilterSelectionRule(pRules->FirstChildElement("audio"), "audio");
    c->GetFilters(pFileItem, filters);
    delete c;

    if (filters.empty())
      return E_FAIL;

    if (FAILED(InsertFilter(filters[0], &m_AudioDecF, m_pStrAudiodec)))
    {
      return E_FAIL;
    }

    filters.clear();

    CStdString extraName = "";
    TiXmlElement *pSubTags = pRules->FirstChildElement("extra");
    for (pSubTags; pSubTags; pSubTags = pSubTags->NextSiblingElement())
    {
      c = new CFilterSelectionRule(pSubTags, "extra");
      c->GetFilters(pFileItem, filters, m_UsingDXVADecoder);
      delete c;

      if (SUCCEEDED(InsertFilter(filters[0], &pBF, extraName)))
        m_extraFilters.push_back(pBF);

      filters.clear();
    }

    break;
  }

  if (extensionNotFound)
  {
    CLog::Log(LOGERROR, "%s Extension \"%s\" not found. Please check dsfilterconfig.xml", __FUNCTION__, extension.c_str());
    return E_FAIL;
  }

  CLog::Log(LOGDEBUG,"%s All filters added to the graph", __FUNCTION__);
 
  return S_OK;
}

HRESULT CFGLoader::LoadConfig(IFilterGraph2* fg, CStdString configFile)
{

  m_pGraphBuilder = fg;
  fg = NULL;
  HRESULT hr = S_OK;
  m_xbmcConfigFilePath = configFile;

  if (!CFile::Exists(configFile))
    return false;

  TiXmlDocument graphConfigXml;
  if (!graphConfigXml.LoadFile(configFile))
    return false;

  TiXmlElement* graphConfigRoot = graphConfigXml.RootElement();
  if ( !graphConfigRoot)
    return false;

  CStdString strFPath, strFGuid, strFAutoLoad, strFileType;
  CStdString strTmpFilterName, strTmpFilterType,strTmpOsdName;
  CStdStringW strTmpOsdNameW;

  CFGFilterFile* pFGF;
  TiXmlElement *pFilters = graphConfigRoot->FirstChildElement("filters");
  pFilters = pFilters->FirstChildElement("filter");
  bool p_bGotThisFilter;

  while (pFilters)
  {
    strTmpFilterName = pFilters->Attribute("name");
    strTmpFilterType = pFilters->Attribute("type");
    XMLUtils::GetString(pFilters,"osdname",strTmpOsdName);
    g_charsetConverter.subtitleCharsetToW(strTmpOsdName,strTmpOsdNameW);
    p_bGotThisFilter = false;
    //first look if we got the path in the config file
    if (XMLUtils::GetString(pFilters, "path" , strFPath))
	  {
		  CStdString strPath2;
      XMLUtils::GetString(pFilters,"guid",strFGuid);
		  XMLUtils::GetString(pFilters,"filetype",strFileType);
		  strPath2.Format("special://xbmc/system/players/dsplayer/%s", strFPath.c_str());

      //First verify if we have the full path or just the name of the file
		  if (!CFile::Exists(strFPath))
      {
        if (CFile::Exists(strPath2))
        {
		      strFPath = _P(strPath2);
          p_bGotThisFilter = true;
        }
      }
      else
        p_bGotThisFilter = true;      

		  if (p_bGotThisFilter)
		    pFGF = new CFGFilterFile(DShowUtil::GUIDFromCString(strFGuid),strFPath,strTmpOsdNameW.c_str(),MERIT64_ABOVE_DSHOW+2,strTmpFilterName,strFileType);
	  }

	  if (! p_bGotThisFilter)
	  {
      XMLUtils::GetString(pFilters,"guid",strFGuid);
      XMLUtils::GetString(pFilters,"filetype",strFileType);
	    strFPath = DShowUtil::GetFilterPath(strFGuid);
	    pFGF = new CFGFilterFile(DShowUtil::GUIDFromCString(strFGuid),strFPath,strTmpOsdNameW.c_str(),MERIT64_ABOVE_DSHOW+2,strTmpFilterName,strFileType);
	  }

    if (XMLUtils::GetString(pFilters,"alwaysload",strFAutoLoad))
	  {
      if ( ( strFAutoLoad.Equals("1",false) ) || ( strFAutoLoad.Equals("true",false) ) )
        pFGF->SetAutoLoad(true);
    }
 
    if (pFGF)
      m_configFilter.push_back(pFGF);

    pFilters = pFilters->NextSiblingElement();

  }
  
  return true;
  //end while

  //m_pGraphBuilder = fg;
  //fg = NULL;
  //HRESULT hr = S_OK;
  //m_xbmcConfigFilePath = configFile;
  //if (!CFile::Exists(configFile))
  //  return false;
  //TiXmlDocument graphConfigXml;
  //if (!graphConfigXml.LoadFile(configFile))
  //  return false;
  //TiXmlElement* graphConfigRoot = graphConfigXml.RootElement();
  //if ( !graphConfigRoot)
  //  return false;
  //CStdString strFPath, strFGuid, strFAutoLoad, strFileType;
  //CStdString strTmpFilterName, strTmpFilterType,strTmpOsdName;
  //CStdStringW strTmpOsdNameW;

  //CFGFilterFile* pFGF;
  //TiXmlElement *pFilters = graphConfigRoot->FirstChildElement("filters");
  //pFilters = pFilters->FirstChildElement("filter");
  //while (pFilters)
  //{
  //  strTmpFilterName = pFilters->Attribute("name");
  //  strTmpFilterType = pFilters->Attribute("type");
  //  XMLUtils::GetString(pFilters,"osdname",strTmpOsdName);
  //  g_charsetConverter.subtitleCharsetToW(strTmpOsdName,strTmpOsdNameW);
  //  if (XMLUtils::GetString(pFilters,"path",strFPath) && !strFPath.empty())
	 // {
		//  CStdString strPath2;
		//  strPath2.Format("special://xbmc/system/players/dsplayer/%s", strFPath.c_str());
		//  if (!CFile::Exists(strFPath) && CFile::Exists(strPath2))
		//    strFPath = _P(strPath2);

		//  XMLUtils::GetString(pFilters,"guid",strFGuid);
		//  XMLUtils::GetString(pFilters,"filetype",strFileType);
		//  pFGF = new CFGFilterFile(DShowUtil::GUIDFromCString(strFGuid),strFPath,strTmpOsdNameW.c_str(),MERIT64_ABOVE_DSHOW+2,strTmpFilterName,strFileType);
	 // }
	 // else
	 // {
  //    XMLUtils::GetString(pFilters,"guid",strFGuid);
  //    XMLUtils::GetString(pFilters,"filetype",strFileType);
	 //   strFPath = DShowUtil::GetFilterPath(strFGuid);
	 //   pFGF = new CFGFilterFile(DShowUtil::GUIDFromCString(strFGuid),strFPath,strTmpOsdNameW.c_str(),MERIT64_ABOVE_DSHOW+2,strTmpFilterName,strFileType);
	 // }
  //  if (XMLUtils::GetString(pFilters,"alwaysload",strFAutoLoad))
	 // {
  //    if ( ( strFAutoLoad.Equals("1",false) ) || ( strFAutoLoad.Equals("true",false) ) )
  //      pFGF->SetAutoLoad(true);
  //  }
 
  //  if (pFGF)
  //    m_configFilter.push_back(pFGF);

  //  pFilters = pFilters->NextSiblingElement();
  //}//end while
}

HRESULT CFGLoader::InsertFilter(const CStdString& filterName, IBaseFilter** ppBF, CStdString& strBFName)
{
  HRESULT hr = S_OK;
  *ppBF = NULL;
  
  list<CFGFilterFile *>::const_iterator it = std::find_if(m_configFilter.begin(),
    m_configFilter.end(),
    std::bind2nd(std::ptr_fun(CompareCFGFilterFileToString), filterName) );

  if (it == m_configFilter.end())
  {
    CLog::Log(LOGERROR, "%s Filter \"%s\" isn't loaded. Please check dsfilterconfig.xml", __FUNCTION__, filterName.c_str());
    return E_FAIL;

  } else
  {
    if(SUCCEEDED(hr = (*it)->Create(ppBF)))
    {
      g_charsetConverter.wToUTF8((*it)->GetName(), strBFName);
      if (SUCCEEDED(hr = m_pGraphBuilder->AddFilter(*ppBF, (*it)->GetName().c_str())))
        CLog::Log(LOGNOTICE, "%s Successfully added \"%s\" to the graph", __FUNCTION__, strBFName.c_str());
      else
        CLog::Log(LOGERROR, "%s Failed to add \"%s\" to the graph", __FUNCTION__, strBFName.c_str());
    } else
    {
      CLog::Log(LOGERROR,"%s Failed to create filter \"%s\"", __FUNCTION__, filterName.c_str());
    }
  }

  return hr;
}

IBaseFilter*              CFGLoader::m_SourceF = NULL;
IBaseFilter*              CFGLoader::m_SplitterF = NULL;
IBaseFilter*              CFGLoader::m_VideoDecF = NULL;
IBaseFilter*              CFGLoader::m_AudioDecF = NULL;
std::vector<IBaseFilter *> CFGLoader::m_extraFilters;
IBaseFilter*              CFGLoader::m_AudioRendererF = NULL;
IBaseFilter*              CFGLoader::m_VideoRendererF = NULL;
bool                      CFGLoader::m_UsingDXVADecoder = false;
