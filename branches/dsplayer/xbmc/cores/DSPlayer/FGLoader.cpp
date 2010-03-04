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
#include "GUIDialogOK.h"
#include "GUIWindowManager.h"
#include "filtercorefactory/filtercorefactory.h"
#include "Settings.h"
using namespace std;

DIRECTSHOW_RENDERER CFGLoader::m_CurrentRenderer = DIRECTSHOW_RENDERER_UNDEF;

CFGLoader::CFGLoader():
  m_pGraphBuilder(NULL),
  m_pFGF(NULL)
{
  Filters.Clear();
  m_UsingDXVADecoder = false;
}

CFGLoader::~CFGLoader()
{
  CAutoLock cAutoLock(this);
  
  CFilterCoreFactory::Destroy();
  SAFE_DELETE(m_pFGF);

  CLog::Log(LOGDEBUG, "%s Ressources released", __FUNCTION__);
}


HRESULT CFGLoader::InsertSourceFilter(const CFileItem& pFileItem, const CStdString& filterName)
{

  HRESULT hr = E_FAIL;
  
  if (pFileItem.IsInternetStream())
  {
    //CLSID_URLReader for httpstream
  //IAMOpenProgress will be need for requesting status of stream
    IFileSourceFilter *pSourceUrl = NULL;
    IUnknown *pUnk = NULL;
    IBaseFilter *pBFSrc = NULL;
    hr = CoCreateInstance(CLSID_URLReader,NULL, CLSCTX_ALL, IID_IUnknown,(void**) &pUnk);
    pUnk->QueryInterface(IID_IBaseFilter,(void**)&Filters.Source.pBF);
    if (Filters.Source.pBF)
    {
      hr = m_pGraphBuilder->AddFilter(Filters.Source.pBF, L"URLReader");
      Filters.Source.osdname.Format("URLReader");
      CStdStringW strUrlW;
      g_charsetConverter.utf8ToW(pFileItem.m_strPath,strUrlW);
      hr = pUnk->QueryInterface(IID_IFileSourceFilter,(void**) &pSourceUrl);
      if (SUCCEEDED(hr))
        hr = pSourceUrl->Load(strUrlW.c_str(), NULL);
      //if(FAILED(hr) || !(pSourceUrl = pUnk) || FAILED(hr = pSourceUrl->Load(strUrlW.c_str(), NULL)))
      if(FAILED(hr))
      {
        SAFE_RELEASE(pSourceUrl);
        
        m_pGraphBuilder->RemoveFilter(pBFSrc);
      }
      else
      {
        CLog::Log(LOGNOTICE, "%s Successfully added url source filter to the graph", __FUNCTION__);
        return hr;
      }
    
    }
    //NULL, CLSCTX_ALL, IID_IUnknown,(void**) &m_pUnkInner);
    
  }
  if (CUtil::IsInArchive(pFileItem.m_strPath))
  {
    Filters.PlayingArchive = true;
    CLog::Log(LOGNOTICE,"%s File \"%s\" need a custom source filter", __FUNCTION__, pFileItem.m_strPath.c_str());
	  if(m_File.Open(pFileItem.m_strPath, READ_TRUNCATED | READ_BUFFERED))
	  {
	    CXBMCFileStream* pXBMCStream = new CXBMCFileStream(&m_File, &Filters.Source.pBF, &hr);

	    if (SUCCEEDED(hr = m_pGraphBuilder->AddFilter(Filters.Source.pBF, L"XBMC File Source")))
		    CLog::Log(LOGNOTICE, "%s Successfully added xbmc source filter to the graph", __FUNCTION__);
      else
		    CLog::Log(LOGERROR, "%s Failted to add xbmc source filter to the graph", __FUNCTION__);

	    Filters.Source.osdname = "XBMC File Source";
	    return hr;
    } 
    else 
    {
      CLog::Log(LOGERROR, "%s Failed to open \"%s\" with source filter!", __FUNCTION__, pFileItem.m_strPath.c_str());
      return E_FAIL;
    }
  } 
  else 
  {  
    if (SUCCEEDED(hr = InsertFilter(filterName, Filters.Source)))
    {
      Filters.Splitter.pBF = Filters.Source.pBF;
      Filters.Splitter.osdname = Filters.Source.osdname;
      Filters.Splitter.guid = Filters.Source.guid;

      CStdString pWinFilePath = pFileItem.m_strPath;
      if ( (pWinFilePath.Left(6)).Equals("smb://", false) )
        pWinFilePath.Replace("smb://", "\\\\");
      
      pWinFilePath.Replace("/", "\\");

      IFileSourceFilter *pFS = NULL;
	    Filters.Splitter.pBF->QueryInterface(IID_IFileSourceFilter, (void**)&pFS);
      
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
  HRESULT hr = InsertFilter(filterName, Filters.Splitter);

  if (SUCCEEDED(hr))
  {
    if (SUCCEEDED(hr = ConnectFilters(m_pGraphBuilder, Filters.Source.pBF, Filters.Splitter.pBF)))
      CLog::Log(LOGNOTICE, "%s Successfully connected the source to the spillter", __FUNCTION__);
    else
    {
      CLog::Log(LOGERROR, "%s Failed to connect the source to the spliter", __FUNCTION__);
      CLog::Log(LOGNOTICE, "%s Trying to just render the source output pin", __FUNCTION__);
      BeginEnumPins(Filters.Source.pBF,pEP,pPin)
      {
        if (!DShowUtil::IsPinConnected(pPin))
        {
          hr = m_pGraphBuilder->Render(pPin);
        }
      }
      EndEnumPins
      if (FAILED(hr))
      {
        CLog::Log(LOGERROR, "%s Failed the just rendering the output pin", __FUNCTION__);
      }

    }
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

HRESULT CFGLoader::InsertAudioRenderer(const CStdString& filterName)
{
  HRESULT hr = S_OK;
  CFGFilterRegistry* pFGF;
  CStdString currentGuid, currentName;

  if (! filterName.empty())
  {
    if (SUCCEEDED(InsertFilter(filterName, Filters.AudioRenderer)))
      return S_OK;
    else
      CLog::Log(LOGERROR, "%s Failed to insert custom audio renderer, fallback to default", __FUNCTION__);
  }

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

  pFGF = new CFGFilterRegistry(DShowUtil::GUIDFromCString(currentGuid));
  hr = pFGF->Create(&Filters.AudioRenderer.pBF);
  delete pFGF;

  if (FAILED(hr))
  {
    CLog::Log(LOGERROR, "%s Failed to create the audio renderer (%X)", __FUNCTION__, hr);
    return hr;
  }

  Filters.AudioRenderer.osdname = currentName;
  Filters.AudioRenderer.guid = DShowUtil::GUIDFromCString(currentGuid);

  hr = m_pGraphBuilder->AddFilter(Filters.AudioRenderer.pBF, DShowUtil::AnsiToUTF16(currentName));

  if (SUCCEEDED(hr))
    CLog::Log(LOGNOTICE, "%s Successfully added \"%s\" to the graph", __FUNCTION__, Filters.AudioRenderer.osdname.c_str());
  else
    CLog::Log(LOGNOTICE, "%s Failed to add \"%s\" to the graph (result: %X)", __FUNCTION__, Filters.AudioRenderer.osdname.c_str(), hr);

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
  {
    m_pFGF = new CFGFilterVideoRenderer(__uuidof(CEVRAllocatorPresenter), L"Xbmc EVR");
    Filters.VideoRenderer.osdname = _T("Enhanced Video Renderer");
  }
  else
  {
    m_pFGF = new CFGFilterVideoRenderer(__uuidof(CVMR9AllocatorPresenter), L"Xbmc VMR9");
    Filters.VideoRenderer.osdname = _T("VMR9 (Renderless)");
  }

  
  hr = m_pFGF->Create(&Filters.VideoRenderer.pBF);
  hr = m_pGraphBuilder->AddFilter(Filters.VideoRenderer.pBF, m_pFGF->GetName());
  if (SUCCEEDED(hr))
  {
    CLog::Log(LOGDEBUG, "%s Allocator presenter successfully added to the graph (Renderer: %d)",  __FUNCTION__, m_CurrentRenderer);
  } else {
    CLog::Log(LOGERROR, "%s Failed to add allocator presenter to the graph (hr = %X)", __FUNCTION__, hr);
  }

  return hr; 
}

HRESULT CFGLoader::LoadFilterRules(const CFileItem& pFileItem)
{
   
  if (! CFilterCoreFactory::SomethingMatch(pFileItem))
  {

    CLog::Log(LOGERROR, "%s Extension \"%s\" not found. Please check dsfilterconfig.xml", __FUNCTION__, CURL(pFileItem.m_strPath).GetFileType().c_str());
    CGUIDialogOK *dialog = (CGUIDialogOK *)g_windowManager.GetWindow(WINDOW_DIALOG_OK);
    if (dialog)
    {
      dialog->SetHeading("Extension not found");
      dialog->SetLine(0, "Impossible to play the media file : the media");
      dialog->SetLine(1, "extension \"" + CURL(pFileItem.m_strPath).GetFileType() + "\" isn't");
      dialog->SetLine(2, "declared in dsfilterconfig.xml.");
      dialog->DoModal();
    }
    return E_FAIL;
  }

  CStdString filter = "";

  CFilterCoreFactory::GetAudioRendererFilter(pFileItem, filter);
  InsertAudioRenderer(filter); // First added, last connected
  InsertVideoRenderer();

  if (! CFilterCoreFactory::GetSourceFilter(pFileItem, filter))
    return E_FAIL;

  if (FAILED(InsertSourceFilter(pFileItem, filter)))
  {
	  return E_FAIL;
  }

  if (! Filters.Splitter.pBF)
  {
    if (! CFilterCoreFactory::GetSplitterFilter(pFileItem, filter))
      return E_FAIL;
    
    if (FAILED(InsertSplitter(pFileItem, filter)))
    {
      return E_FAIL;
    }
  }

  /* Ok, the splitter is added to the graph. We need to detect the video stream codec in order to choose
  if we use or not the dxva filter */

  IBaseFilter *pBF = Filters.Splitter.pBF;
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

      CLog::Log(LOGINFO, "Video stream fourcc : %4.4hs", &s.fourcc);

      fourccfinded = true;
      // CCV1 is a fake fourcc code generated by Haali in order to trick the Microsoft h264
      // decoder included in Windows 7. ffdshow handle that fourcc well, but it may not be
      // the case for others decoders
      if (s.fourcc == 'H264' || s.fourcc == 'AVC1' || s.fourcc == 'WVC1'
        || s.fourcc == 'WMV3' || s.fourcc == 'WMVA' || s.fourcc == 'CCV1')
        m_UsingDXVADecoder = true;

      if (s.fourcc == 'CCV1')
      {
        CLog::Log(LOGINFO, "Haali Media Splitter is configured for using a fake fourcc code 'CCV1'. If you've got an error message, be sure your video decoder handle that fourcc well.");
      }

      break;
    }
    EndEnumMediaTypes(pMT)
  }
  EndEnumPins

  std::vector<CStdString> extras;
  if (! CFilterCoreFactory::GetExtraFilters(pFileItem, extras, m_UsingDXVADecoder))
    return E_FAIL;

  // Insert extra first because first added, last connected!
  for (int i = 0; i < extras.size(); i++)
  {
    SFilterInfos f;
    if (SUCCEEDED(InsertFilter(extras[i], f)))
      Filters.Extras.push_back(f);

  }
  extras.clear();

  if (! CFilterCoreFactory::GetVideoFilter(pFileItem, filter, m_UsingDXVADecoder))
    return E_FAIL;

  if (FAILED(InsertFilter(filter, Filters.Video)))
  {
    return E_FAIL;
  }

  if (! CFilterCoreFactory::GetAudioFilter(pFileItem, filter, m_UsingDXVADecoder))
    return E_FAIL;

  if (FAILED(InsertFilter(filter, Filters.Audio)))
  {
    return E_FAIL;
  }

  CLog::Log(LOGDEBUG,"%s All filters added to the graph", __FUNCTION__);
 
  return S_OK;
}

HRESULT CFGLoader::LoadConfig(IFilterGraph2* fg)
{

  m_pGraphBuilder = fg;

  LoadFilterCoreFactorySettings(g_settings.GetUserDataItem("dsfilterconfig.xml"), true);
  LoadFilterCoreFactorySettings("special://xbmc/system/players/dsplayer/dsfilterconfig.xml", false);

  return S_OK;
}

HRESULT CFGLoader::InsertFilter(const CStdString& filterName, SFilterInfos& f)
{
  HRESULT hr = S_OK;
  f.pBF = NULL;

  CFGFilterFile *filter = NULL;
  if (! (filter = CFilterCoreFactory::GetFilterFromName(filterName)))
    return E_FAIL;

  if(SUCCEEDED(hr = filter->Create(&f.pBF)))
  {
    g_charsetConverter.wToUTF8(filter->GetName(), f.osdname);
    if (SUCCEEDED(hr = m_pGraphBuilder->AddFilter(f.pBF, filter->GetName().c_str())))
      CLog::Log(LOGNOTICE, "%s Successfully added \"%s\" to the graph", __FUNCTION__, f.osdname.c_str());
    else
      CLog::Log(LOGERROR, "%s Failed to add \"%s\" to the graph", __FUNCTION__, f.osdname.c_str());

    f.guid = filter->GetCLSID();
  } 
  else
  {
    CLog::Log(LOGERROR,"%s Failed to create filter \"%s\"", __FUNCTION__, filterName.c_str());
  }

  return hr;
}

bool CFGLoader::LoadFilterCoreFactorySettings( const CStdString& fileStr, bool clear )
{
  CLog::Log(LOGNOTICE, "Loading filter core factory settings from %s.", fileStr.c_str());
  if (!CFile::Exists(fileStr))
  { // tell the user it doesn't exist
    CLog::Log(LOGNOTICE, "%s does not exist. Skipping.", fileStr.c_str());
    return false;
  }

  TiXmlDocument filterCoreFactoryXML;
  if (!filterCoreFactoryXML.LoadFile(fileStr))
  {
    CLog::Log(LOGERROR, "Error loading %s, Line %d (%s)", fileStr.c_str(), filterCoreFactoryXML.ErrorRow(), filterCoreFactoryXML.ErrorDesc());
    return false;
  }

  return CFilterCoreFactory::LoadConfiguration(filterCoreFactoryXML.RootElement(), clear);
}

bool                      CFGLoader::m_UsingDXVADecoder = false;
SFilters                  CFGLoader::Filters;
