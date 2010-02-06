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

#include "dsconfig.h"
#include "Utils/log.h"
#include "DShowUtil/DShowUtil.h"
#include "CharsetConverter.h"

//Required for the gui buttons
#include "XMLUtils.h"
#include "FileSystem/SpecialProtocol.h"

#include "GuiSettings.h"


using namespace std;

class CDSConfig g_dsconfig;
CDSConfig::CDSConfig(void)
{
  m_pGraphBuilder = NULL;
  m_pIMpcDecFilter = NULL;
  m_pIMpaDecFilter = NULL;
  m_pSplitter = NULL;
}

CDSConfig::~CDSConfig(void)
{
  while (! m_pPropertiesFilters.empty())
    m_pPropertiesFilters.pop_back();
}

HRESULT CDSConfig::ConfigureFilters(IFilterGraph2* pGB, IBaseFilter * splitter)
{
  HRESULT hr = S_OK;
  m_pGraphBuilder = pGB;
  m_pSplitter = splitter;
  pGB = NULL;

  while (! m_pPropertiesFilters.empty())
    m_pPropertiesFilters.pop_back();

  ConfigureFilters();

  return hr;
}

void CDSConfig::ConfigureFilters()
{
  BeginEnumFilters(m_pGraphBuilder, pEF, pBF)
  {
	  GetMpcVideoDec(pBF);
	  GetMpaDec(pBF);
    LoadPropertiesPage(pBF);
  }
  EndEnumFilters(pEF, pBF)
  CreatePropertiesXml();
}

bool CDSConfig::LoadPropertiesPage(IBaseFilter *pBF)
{
  ISpecifyPropertyPages *pProp = NULL;
  CAUUID pPages;
  if ( SUCCEEDED( pBF->QueryInterface(IID_ISpecifyPropertyPages, (void **) &pProp) ) )
  {
    pProp->GetPages(&pPages);
    if (pPages.cElems > 0)
    {
      m_pPropertiesFilters.push_back(pBF);
    
      CStdString filterName;
      g_charsetConverter.wToUTF8(DShowUtil::GetFilterName(pBF), filterName);
      CLog::Log(LOGNOTICE, "%s \"%s\" expose ISpecifyPropertyPages", __FUNCTION__, filterName.c_str());
    }
	  SAFE_RELEASE(pProp);
    CoTaskMemFree(pPages.pElems);
    return true;
    
  } 
  else
    return false;
}

void CDSConfig::CreatePropertiesXml()
{
  //verify we have at least one property page
  if (m_pPropertiesFilters.empty())
    return;

  CStdString pStrName;
  CStdString pStrId;
  int pIntId = 0;
  TiXmlDocument xmlDoc;
  TiXmlElement xmlRootElement("strings");
  TiXmlNode *pRoot = xmlDoc.InsertEndChild(xmlRootElement);
  if (!pRoot) 
    return;
  
  for (std::vector<IBaseFilter*>::const_iterator it = m_pPropertiesFilters.begin() ; it != m_pPropertiesFilters.end(); it++)
  {
    g_charsetConverter.wToUTF8(DShowUtil::GetFilterName(*it), pStrName);
    TiXmlElement newFilterElement("string");
    
    //Set the id of the lang
    pStrId.Format("%i", pIntId);
    newFilterElement.SetAttribute("id", pStrId.c_str());

    //set the name of the filter in the element
    //newFilterElement.setValue(pStrName.c_str());
    TiXmlNode *pNewNode = pRoot->InsertEndChild(newFilterElement);
    if (! pNewNode)
      break;
    
    TiXmlText value(pStrName.c_str());
    pNewNode->InsertEndChild(value);
    pIntId++;
  }
  xmlDoc.SaveFile("special://temp//dslang.xml");
}

void CDSConfig::ShowPropertyPage(IBaseFilter *pBF)
{
  m_pCurrentProperty = new CDSPropertyPage(pBF);
  m_pCurrentProperty->Initialize();
}

bool CDSConfig::GetMpcVideoDec(IBaseFilter* pBF)
{
  if (m_pIMpcDecFilter)
    return false;
  pBF->QueryInterface(__uuidof(m_pIMpcDecFilter), (void **)&m_pIMpcDecFilter);
  if (!m_pIMpcDecFilter)
    return false;
  m_pStdDxva.Format("");
  if (g_guiSettings.GetBool("dsplayer.forcenondefaultrenderer"))
  {
    m_pStdDxva = CStdString("");
    CLog::Log(LOGERROR,"%s DXVA WILL NEVER WORK WITH NON DEFAULT RENDERER ON",__FUNCTION__);
  }
  else
  {
    if ( m_pIMpcDecFilter )
    {
      m_pStdDxva = DShowUtil::GetDXVAMode(m_pIMpcDecFilter->GetDXVADecoderGuid());
      CLog::Log(LOGDEBUG,"Got IMPCVideoDecFilter");
    }
  }

  return true;
}

bool CDSConfig::GetMpaDec(IBaseFilter* pBF)
{
  if (m_pIMpaDecFilter)
    return false;
  pBF->QueryInterface(__uuidof(m_pIMpaDecFilter), (void **)&m_pIMpaDecFilter);

//definition of AC3 VALUE DEFINITION
//A52_CHANNEL 0
//A52_MONO 1
//A52_STEREO 2
//A52_3F 3
//A52_2F1R 4
//A52_3F1R 5
//A52_2F2R 6
//A52_3F2R 7
//A52_CHANNEL1 8
//A52_CHANNEL2 9
//A52_DOLBY 10
//A52_CHANNEL_MASK 15
//LFE checked is 16

//DTS VALUE DEFINITION
//DCA_MONO 0
//DCA_CHANNEL 1
//DCA_STEREO 2
//DCA_STEREO_SUMDIFF 3
//DCA_STEREO_TOTAL 4
//DCA_3F 5
//DCA_2F1R 6
//DCA_3F1R 7
//DCA_2F2R 8
//DCA_3F2R 9
//DCA_4F2R 10
//LFE checked DCA_LFE 0x80-->128 decimal
  if (m_pIMpaDecFilter)
  {
    int pSpkConfig;
    bool audioisdigital,audiodowntostereo;
    //0 analog
    audiodowntostereo = g_guiSettings.GetSetting("audiooutput.downmixmultichannel")->ToString().Equals("true",false);
    audioisdigital = g_guiSettings.GetSetting("audiooutput.mode")->ToString().Equals("1",false);
    if (audioisdigital)
    {
      //1 digital
      //Well didnt really searched on exactly how the spdif config is setted mathematically
      //So just took the lazy way and took the value in the windows registry for spdif
      //ac3 stereo        -->ffffffee
      //ac3 3front 2 rear -->ffffffe9
      if (audiodowntostereo)
        pSpkConfig = 0xFFFFFFEE;
      else
        pSpkConfig = 0xFFFFFFE9;
      //ac3 = 0 for setting speaker
      m_pIMpaDecFilter->SetSpeakerConfig((IMpaDecFilter::enctype)0, pSpkConfig);
      
      //dts stereo        -->ffffff7e
      //dts 3front 2 rear -->ffffff77 
      
      if (audiodowntostereo)
        pSpkConfig = 0xFFFFFF7E;
      else
        pSpkConfig = 0xFFFFFF77;
      
      //dts = 1 for setting speaker
      m_pIMpaDecFilter->SetSpeakerConfig((IMpaDecFilter::enctype)1, pSpkConfig);
      //m_pIMpaDecFilter->SaveSettings();
    }
    else
    {
      //0 analog
      if (audiodowntostereo)
        pSpkConfig = 18;
      else
        pSpkConfig = 23;
      //ac3 = 0 for setting speaker
      m_pIMpaDecFilter->SetSpeakerConfig((IMpaDecFilter::enctype)0, pSpkConfig);

      if (audiodowntostereo)
        pSpkConfig = 130;
      else
        pSpkConfig = 137;

      //dts = 1 for setting speaker
      m_pIMpaDecFilter->SetSpeakerConfig((IMpaDecFilter::enctype)1, pSpkConfig);
      
      //aac to stereo
      m_pIMpaDecFilter->SetSpeakerConfig((IMpaDecFilter::enctype)2, audiodowntostereo ? 1 : 0);
      //m_pIMpaDecFilter->SaveSettings();
    }
    
    CLog::Log(LOGNOTICE,"%s %s",__FUNCTION__,audiodowntostereo ? "dts and ac3 is now on stereo" : "dts and ac3 is now on 3 front, 2 rear");
    CLog::Log(LOGNOTICE,"%s %s",__FUNCTION__,audioisdigital ? "SPDIF" : "not SPDIF");
                                                
  }
  return true;
}