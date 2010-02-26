/* 
 *  Copyright (C) 2003-2006 Gabest
 *  http://www.gabest.org
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

#include "FGManager.h"

#include <mpconfig.h> //IMixerConfig

#include "WinSystemWin32.h" //g_hwnd
#include "WindowingFactory.h"
#include "CharsetConverter.h"
#include "DShowUtil/dshowutil.h"
#include "DShowUtil/DshowCommon.h"
#include "SystemInfo.h" //g_sysinfo
#include "GUISettings.h"//g_guiSettings


#include "filters/VMR9AllocatorPresenter.h"
#include "filters/EVRAllocatorPresenter.h"

#include <initguid.h>
#include "moreuuids.h"
#include <dmodshow.h>
#include <D3d9.h>
#include <Vmr9.h>
#include "Filters/IMpaDecFilter.h"
#include "Log.h"
#include "FileSystem/SpecialProtocol.h"

//XML CONFIG HEADERS
#include "tinyXML/tinyxml.h"
#include "XMLUtils.h"
#include "GUIDialogOK.h"
#include "GUIWindowManager.h"
#include "GUIDialogYesNo.h"
//END XML CONFIG HEADERS

//Headers and definition for windows registry
#include "DShowUtil/RegKey.h"

using namespace std;

//
// CFGManager
//

CFGManager::CFGManager():
  m_dwRegister(0),
  m_audioPinConnected(false),
  m_videoPinConnected(false),
  m_subtitlePinConnected(false)
{
  HRESULT hr;
  hr = CoCreateInstance(CLSID_FilterGraph, NULL, CLSCTX_ALL, IID_IUnknown,(void**) &m_pUnkInner);
  hr = m_pUnkInner->QueryInterface(IID_IFilterGraph2,(void**)&m_pFG);
  hr = CoCreateInstance(CLSID_FilterMapper2,NULL,CLSCTX_ALL,__uuidof(m_pFM),(void**) &m_pFM);
}

CFGManager::~CFGManager()
{
  CAutoLock cAutoLock(this);

  while(!m_source.empty()) 
    m_source.pop_back();
  while(!m_transform.empty())
  {
	  if (m_transform.back())
		  delete m_transform.back();
	  
	  m_transform.pop_back();
  }
  while(!m_override.empty()) 
    m_override.pop_back();

  SAFE_DELETE(m_CfgLoader);

  SAFE_RELEASE(m_pFM);
  SAFE_RELEASE(m_pFG);

  CLog::Log(LOGDEBUG, "%s Ressources released", __FUNCTION__);
}

HRESULT CFGManager::QueryInterface(const IID &iid, void** ppv)
{
  HRESULT hr;
  CheckPointer(ppv,E_POINTER);
  hr = m_pUnkInner->QueryInterface(iid,ppv);
  if (SUCCEEDED(hr))
    return hr;
  BeginEnumFilters(m_pFG, pEF, pBF)
  {
    hr = pBF->QueryInterface(iid, ppv);
    if (SUCCEEDED(hr))
    {
      SAFE_RELEASE(pBF);
      SAFE_RELEASE(pEF);
      break;
    }

  }
  EndEnumFilters(pEF, pBF)
  
  return hr;
}

void CFGManager::CStreamPath::Append(IBaseFilter* pBF, IPin* pPin)
{
  path_t p;
  p.clsid = DShowUtil::GetCLSID(pBF);
  p.filter = DShowUtil::GetFilterName(pBF);
  p.pin = DShowUtil::GetPinName(pPin);
  push_back(p);
}

bool CFGManager::CStreamPath::Compare(const CStreamPath& path)
{
  PathListIter pos1 = begin();
  PathListConstIter pos2 = path.begin();
  while((pos1 != end()) && (pos2 != path.end()))
  {
    const path_t& p1 = *pos1;
    const path_t& p2 = *pos2;
    if(p1.filter != p2.filter) return true;
    else if(p1.pin != p2.pin) return false;
    pos1++;
    pos2++;
  }

  return true;
}

HRESULT CFGManager::CreateFilter(CFGFilter* pFGF, IBaseFilter** ppBF)
{
  CheckPointer(pFGF, E_POINTER);
  CheckPointer(ppBF, E_POINTER);

  IBaseFilter* pBF;
  if(FAILED(pFGF->Create(&pBF)))
    return E_FAIL;

  *ppBF = pBF;
  pBF = NULL;

  return S_OK;
}

// IFilterGraph

STDMETHODIMP CFGManager::AddFilter(IBaseFilter* pFilter, LPCWSTR pName)
{
  CAutoLock cAutoLock(this);

  HRESULT hr;

  if (m_pFG)
  {
	  hr = m_pFG->AddFilter(pFilter, pName);
	  if(FAILED(hr))
		return hr;
  }

  // TODO
  hr = pFilter->JoinFilterGraph(NULL, NULL);
  hr = pFilter->JoinFilterGraph(m_pFG, pName);
  
  //SAFE_RELEASE(pIFG);
  return hr;
}

STDMETHODIMP CFGManager::RemoveFilter(IBaseFilter* pFilter)
{
  if(!m_pFG) 
    return E_UNEXPECTED;

  CAutoLock cAutoLock(this);

  HRESULT hr = E_FAIL;

  hr = m_pFG->RemoveFilter(pFilter);
  if (SUCCEEDED(hr))
    pFilter->JoinFilterGraph(NULL, NULL);

  return hr;
}

STDMETHODIMP CFGManager::EnumFilters(IEnumFilters** ppEnum)
{
  if(!m_pFG) 
    return E_UNEXPECTED;

  return m_pFG->EnumFilters(ppEnum);
}

STDMETHODIMP CFGManager::FindFilterByName(LPCWSTR pName, IBaseFilter** ppFilter)
{
  if(!m_pFG) 
    return E_UNEXPECTED;
  CAutoLock cAutoLock(this);

  return m_pFG->FindFilterByName(pName, ppFilter);
}

STDMETHODIMP CFGManager::ConnectDirect(IPin* pPinOut, IPin* pPinIn, const AM_MEDIA_TYPE* pmt)
{
  if(! m_pFG) 
    return E_UNEXPECTED;

	CAutoLock cAutoLock(this);

  IBaseFilter* pBFIn = DShowUtil::GetFilterFromPin(pPinIn);
	CLSID clsidIn = DShowUtil::GetCLSID(pBFIn);

  IBaseFilter* pBFOut = DShowUtil::GetFilterFromPin(pPinOut);
  CLSID clsidOut = DShowUtil::GetCLSID(pBFOut);

	// TODO: GetUpStreamFilter goes up on the first input pin only
	for(IBaseFilter* pBFUS = DShowUtil::GetFilterFromPin(pPinOut); pBFUS; pBFUS = DShowUtil::GetUpStreamFilter(pBFUS))
	{
		if(pBFUS == pBFIn) 
      return VFW_E_CIRCULAR_GRAPH;
    if(DShowUtil::GetCLSID(pBFUS) == clsidIn) 
      return VFW_E_CANNOT_CONNECT;
	}
  
  HRESULT hr = m_pFG->ConnectDirect(pPinOut, pPinIn, pmt);

#ifdef _DSPLAYER_DEBUG
  CStdString filterNameIn, filterNameOut;
  CStdString pinNameIn, pinNameOut;

  g_charsetConverter.wToUTF8(DShowUtil::GetFilterName(pBFOut), filterNameOut);
  g_charsetConverter.wToUTF8(DShowUtil::GetPinName(pPinOut), pinNameOut);
  g_charsetConverter.wToUTF8(DShowUtil::GetFilterName(pBFIn), filterNameIn);
  g_charsetConverter.wToUTF8(DShowUtil::GetPinName(pPinIn), pinNameIn);

  CLog::Log(LOGDEBUG, "%s: %s connecting %s.%s pin to %s.%s", __FUNCTION__, 
                                                                    (SUCCEEDED(hr) ? "Succeeded": "Failed"),
                                                                    filterNameOut.c_str(), 
                                                                    pinNameOut.c_str(), 
                                                                    filterNameIn.c_str(), 
                                                                    pinNameIn.c_str());
#endif

  return hr;
}

STDMETHODIMP CFGManager::Reconnect(IPin* ppin)
{
  if(! m_pFG) 
    return E_UNEXPECTED;
  CAutoLock cAutoLock(this);

  return m_pFG->Reconnect(ppin);
}

STDMETHODIMP CFGManager::Disconnect(IPin* ppin)
{
  if(! m_pFG) 
    return E_UNEXPECTED;
  CAutoLock cAutoLock(this);

  return m_pFG->Disconnect(ppin);
}

STDMETHODIMP CFGManager::SetDefaultSyncSource()
{
  if (! m_pFG)
    return E_UNEXPECTED;
  CAutoLock cAutoLock(this);

  return m_pFG->SetDefaultSyncSource();
}

// IGraphBuilder

STDMETHODIMP CFGManager::Connect(IPin* pPinOut, IPin* pPinIn)
{
  CAutoLock cAutoLock(this);

  CheckPointer(pPinOut, E_POINTER);

  HRESULT hr;

  {
    PIN_DIRECTION dir;
    if(FAILED(hr = pPinOut->QueryDirection(&dir)) || dir != PINDIR_OUTPUT
    || pPinIn && (FAILED(hr = pPinIn->QueryDirection(&dir)) || dir != PINDIR_INPUT))
      return VFW_E_INVALID_DIRECTION;

    IPin* pPinTo;
    if(SUCCEEDED(hr = pPinOut->ConnectedTo(&pPinTo)) || pPinTo
    || pPinIn && (SUCCEEDED(hr = pPinIn->ConnectedTo(&pPinTo)) || pPinTo))
	{
		if (pPinTo)
			pPinTo->Release();
		return VFW_E_ALREADY_CONNECTED;
	}
  }

  bool fDeadEnd = true;

  if(pPinIn)
  {
    // 1. Try a direct connection between the filters, with no intermediate filters

    if(SUCCEEDED(hr = ConnectDirect(pPinOut, pPinIn, NULL)))
      return hr;

    // Since pPinIn is our target we must not allow:
    // - intermediate filters with no output pins 
    // - input pins being on by the same filter, that would lead to circular graph
  }
/*  else
  {
    // 1. Use IStreamBuilder
    IStreamBuilder* pSB = NULL;
    if (SUCCEEDED(pPinOut->QueryInterface(__uuidof(pSB),(void**)&pSB)))
    {
      if(SUCCEEDED(hr = pSB->Render(pPinOut, m_pFG)))
      {
        SAFE_RELEASE(pSB);
        return hr;
      }

      pSB->Backout(pPinOut, m_pFG);
      SAFE_RELEASE(pSB);
    }
  }*/

  /* 2. Try cached filters
  IGraphConfig* pGC = NULL;
  if (SUCCEEDED(m_pFG->QueryInterface(__uuidof(pGC),(void**)&pGC)))
  {
    BeginEnumCachedFilters(pGC, pEF, pBF)
    {
      if(pPinIn && (DShowUtil::IsStreamEnd(pBF) || DShowUtil::GetFilterFromPin(pPinIn) == pBF))
        continue;

      hr = pGC->RemoveFilterFromCache(pBF);

      if(SUCCEEDED(hr = ConnectFilterDirect(pPinOut, pBF, NULL)))
      {
        if(!DShowUtil::IsStreamEnd(pBF)) fDeadEnd = false;

        if(SUCCEEDED(hr = ConnectFilter(pBF, pPinIn)))
        {
          SAFE_RELEASE(pBF);
          SAFE_RELEASE(pEF);
          SAFE_RELEASE(pGC);
          return hr;
        }
      }

      hr = pGC->AddFilterToCache(pBF);
    }
    EndEnumCachedFilters(pEF, pBF)
    SAFE_RELEASE(pGC);
  }*/

  // 3. Try filters in the graph

  {
    std::list<IBaseFilter*> pBFs; // Don't connect the audio and video renderer yet
    BeginEnumFilters(m_pFG, pEF, pBF)
    {
      if(pPinIn && (DShowUtil::IsStreamEnd(pBF) || DShowUtil::GetFilterFromPin(pPinIn) == pBF)
      || DShowUtil::GetFilterFromPin(pPinOut) == pBF)// || pBF == m_CfgLoader->GetAudioRenderer() || pBF == m_CfgLoader->GetVideoRenderer())
        continue;

      // HACK: ffdshow - audio capture filter
      if(DShowUtil::GetCLSID(pPinOut) == DShowUtil::GUIDFromCString(_T("{04FE9017-F873-410E-871E-AB91661A4EF7}"))
      && DShowUtil::GetCLSID(pBF) == DShowUtil::GUIDFromCString(_T("{E30629D2-27E5-11CE-875D-00608CB78066}")))
        continue;
      pBFs.push_back(pBF);
    }
    EndEnumFilters(pEF, pBF)

    IBaseFilter* pBF;
    for (list<IBaseFilter*>::iterator it = pBFs.begin() ; it != pBFs.end(); it++)
    {
      pBF = *it;// ((IBaseFilter*)it);
      if(SUCCEEDED(hr = ConnectFilterDirect(pPinOut, pBF, NULL)))
      {
        if(! DShowUtil::IsStreamEnd(pBF)) 
          fDeadEnd = false;

        if(SUCCEEDED(hr = ConnectFilter(pBF, pPinIn)))
          return hr;
      }

      //EXECUTE_ASSERT(Disconnect(pPinOut));
      Disconnect(pPinOut);
    }
  }

  // 4. Look up filters in the registry
  
  /*{
    CFGFilterList* fl = new CFGFilterList();

    std::vector<GUID> types;
    DShowUtil::ExtractMediaTypes(pPinOut, types);

    
    CFGFilter* pFGF;
    for(FilterListIter it = m_transform.begin() ; it != m_transform.end(); it++ )
    {
      pFGF = *it;
      if(pFGF->GetMerit() < MERIT64_DO_USE || pFGF->CheckTypes(types, false)) 
        fl->Insert(pFGF, 0, pFGF->CheckTypes(types, true), false);
    }

    
    for(FilterListIter it = m_override.begin() ; it != m_override.end() ; it++)
    {
      pFGF = *it;
      if(pFGF->GetMerit() < MERIT64_DO_USE || pFGF->CheckTypes(types, false)) 
        fl->Insert(pFGF, 0, pFGF->CheckTypes(types, true), false);
    }

    IEnumMoniker* pEM;
    //Not sure its the best way to do it but at least its not throwing unhandled exception
    std::vector<GUID>::iterator it = types.begin();
    if((types.size() > 0 )
    && SUCCEEDED(m_pFM->EnumMatchingFilters(
      &pEM, 0, false, MERIT_DO_NOT_USE+1,
      true, types.size()/2, &(*it), NULL, NULL, false, 
      !!pPinIn, 0, NULL, NULL, NULL)))
    {
      for(IMoniker* pMoniker; S_OK == pEM->Next(1, &pMoniker, NULL); pMoniker = NULL)
      {
        CFGFilterRegistry* pFGF = new CFGFilterRegistry(pMoniker);
        fl->Insert(pFGF, 0, pFGF->CheckTypes(types, true));
      }
    }
//crashing right there on mkv
//The subtitle might be the reason
    std::list<CFGFilter*> theList;
    theList = fl->GetSortedList();
    for (list<CFGFilter*>::iterator it = theList.begin() ; it != theList.end() ; it++)
    {
      //CFGFilter* pFGF = fl.GetNext(pos);

      IBaseFilter* pBF;
      if(FAILED(CreateFilter(*it, &pBF)))
        continue;

      if(pPinIn &&  DShowUtil::IsStreamEnd(pBF))
        continue;
      if(FAILED(hr = AddFilter(pBF, (LPCWSTR)(*it)->GetName().c_str())))
        continue;

      hr = E_FAIL;

      if(types.size() == 2 && types[0] == MEDIATYPE_Stream && types[1] != MEDIATYPE_NULL)
      {
        CMediaType mt;
        
        mt.majortype = types[0];
        mt.subtype = types[1];
        mt.formattype = FORMAT_None;
        if(FAILED(hr)) hr = ConnectFilterDirect(pPinOut, pBF, &mt);

        mt.formattype = GUID_NULL;
        if(FAILED(hr)) hr = ConnectFilterDirect(pPinOut, pBF, &mt);
      }

      if(FAILED(hr)) hr = ConnectFilterDirect(pPinOut, pBF, NULL);

      if(SUCCEEDED(hr))
      {
        if(! DShowUtil::IsStreamEnd(pBF)) 
          fDeadEnd = false;

        hr = ConnectFilter(pBF, pPinIn);

        if(SUCCEEDED(hr))
        {
          IMixerPinConfig* pMPC;
          if (SUCCEEDED(pBF->QueryInterface(IID_IMixerPinConfig,(void **)&pMPC)))
          {  
            pMPC->SetAspectRatioMode(AM_ARMODE_STRETCHED);
            SAFE_RELEASE(pMPC);
          }
          IVMRAspectRatioControl* pARC;
          if (SUCCEEDED(pBF->QueryInterface(__uuidof(IVMRAspectRatioControl),(void **)&pARC)))
          {  
            pARC->SetAspectRatioMode(VMR_ARMODE_NONE);
            SAFE_RELEASE(pARC);
          }

          IVMRAspectRatioControl9* pARC9;
          if (SUCCEEDED(pBF->QueryInterface(__uuidof(IVMRAspectRatioControl9),(void **)&pARC9)))
          {  
            pARC9->SetAspectRatioMode(VMR_ARMODE_NONE);
            SAFE_RELEASE(pARC9);
          }
          return hr;
        }
      }

      EXECUTE_ASSERT(SUCCEEDED(RemoveFilter(pBF)));
    }
  }*/
  
  
  /*if(fDeadEnd)
  {
    CStreamDeadEnd* psde;
    psde = new CStreamDeadEnd();
    //CAutoPtr<CStreamDeadEnd> psde(new CStreamDeadEnd());
    //The one under wasnt used already commented
    //psde->AddTailList(&m_streampath);

    path_t pth;
    for (PathListIter it = m_streampath.begin();it != m_streampath.end(); it++)
    {
      psde->push_back(*it);
    }
    BeginEnumMediaTypes(pPinOut, pEM, pmt)
      psde->mts.push_back(CMediaType(*pmt));
    EndEnumMediaTypes(pmt)
    m_deadends.push_back(psde);
  }*/

  return pPinIn ? VFW_E_CANNOT_CONNECT : VFW_E_CANNOT_RENDER;
}

STDMETHODIMP CFGManager::Render(IPin* pPinOut)
{
  return m_pFG->Render(pPinOut);
}

HRESULT CFGManager::RenderFileXbmc(const CFileItem& pFileItem)
{
  CAutoLock cAutoLock(this);
  HRESULT hr = S_OK;
  //update ffdshow registry to avoid stupid connection problem
  UpdateRegistry();
  if (FAILED(m_CfgLoader->LoadFilterRules(pFileItem)))
  {
    CLog::Log(LOGERROR, "%s Failed to load filters rules", __FUNCTION__);
    return E_FAIL;
  } else
    CLog::Log(LOGDEBUG, "%s Successfully loaded filters rules", __FUNCTION__);

  hr = ConnectFilter(CFGLoader::Filters.Splitter.pBF , NULL);

  if (hr != S_OK)
  {
    IBaseFilter *pBF = CFGLoader::Filters.Splitter.pBF;
    int nVideoPin = 0, nAudioPin = 0;
    int nConnectedVideoPin = 0, nConnectedAudioPin = 0;
    bool videoError = false, audioError = false;
    BeginEnumPins(pBF, pEP, pPin)
    {
      BeginEnumMediaTypes(pPin, pEMT, pMT)
      {
        if (pMT->majortype == MEDIATYPE_Video)
        {
          nVideoPin++;
          if (DShowUtil::IsPinConnected(pPin) == S_OK)
            nConnectedVideoPin++;
        }
        if (pMT->majortype == MEDIATYPE_Audio)
        {
          nAudioPin++;
          if (DShowUtil::IsPinConnected(pPin) == S_OK)
            nConnectedAudioPin++;
        }
        break;
      }
      EndEnumMediaTypes(pMT)
    }
    EndEnumPins
    
    videoError = (nVideoPin >= 1 && nConnectedVideoPin == 0); // No error if no video stream
    audioError = (nAudioPin >= 1 && nConnectedAudioPin == 0); // No error if no audio stream
    //Work around to fix the audio pipeline
    //I think this should be the best way to render a pin if there an error the audio stream
    //The only problem is the graph is getting locked after that
    if (audioError)//
    {
      IBaseFilter *pBF = CFGLoader::Filters.Splitter.pBF;

      BeginEnumPins(pBF, pEP, pPin)
      {
        BeginEnumMediaTypes(pPin, pEMT, pMT)
        {
          if (pMT->majortype == MEDIATYPE_Audio)
          {
            if (SUCCEEDED(Render(pPin)))
            {
              audioError = false;
            }
          }
        }
        EndEnumMediaTypes(pMT)
      }
      EndEnumPins
    }

    

    if ( videoError || audioError)
    {
      CGUIDialogOK *dialog = (CGUIDialogOK *)g_windowManager.GetWindow(WINDOW_DIALOG_OK);
      if (dialog)
      {
        CStdString strError;
        
        if (videoError && audioError)
          strError = CStdString("Error in both audio and video rendering chain.");
        else if (videoError)
          strError = CStdString("Error in the video rendering chain.");
        else if (audioError)
          strError = CStdString("Error in the audio rendering chain.");
        CLog::Log(LOGERROR, "%s Audio / Video error \n %s \n Ensure that the audio/video stream is supported by your selected decoder and ensure that the decoder is properly configured.", __FUNCTION__, strError.c_str());
        dialog->SetHeading("Audio / Video error");
        dialog->SetLine(0, strError.c_str());
        dialog->SetLine(1, "more details or see XBMC Wiki - 'DSPlayer codecs'");
        dialog->SetLine(2, "section for more informations.");
        dialog->DoModal();

        return E_FAIL;
      }
    }
  }

  

  //Apparently the graph dont start with unconnected filters in the graph for wmv files
  if (pFileItem.GetAsUrl().GetFileType().Equals("wmv",false))
    RemoveUnconnectedFilters(m_pFG);

  g_dsconfig.ConfigureFilters(m_pFG);
#ifdef _DSPLAYER_DEBUG
  LogFilterGraph();
#endif

  return hr;  
}

HRESULT CFGManager::GetFileInfo(CStdString* sourceInfo, CStdString* splitterInfo, CStdString* audioInfo,
                                CStdString* videoInfo, CStdString* audioRenderer)
{
  *sourceInfo = CFGLoader::Filters.Source.osdname;
  *splitterInfo = CFGLoader::Filters.Splitter.osdname;
  *audioInfo = CFGLoader::Filters.Audio.osdname;
  *videoInfo = CFGLoader::Filters.Video.osdname;
  *audioRenderer = CFGLoader::Filters.AudioRenderer.osdname;
  return S_OK;
}

STDMETHODIMP CFGManager::Abort()
{
  if (!m_pFG)
    return E_UNEXPECTED;
  CAutoLock cAutoLock(this);
  
  return m_pFG->Abort();
}

STDMETHODIMP CFGManager::ShouldOperationContinue()
{
  if (! m_pFG)
    return E_UNEXPECTED;
  CAutoLock cAutoLock(this);

  return m_pFG->ShouldOperationContinue();
}

// IFilterGraph2

STDMETHODIMP CFGManager::AddSourceFilterForMoniker(IMoniker* pMoniker, IBindCtx* pCtx, LPCWSTR lpcwstrFilterName, IBaseFilter** ppFilter)
{
  if (! m_pFG)
    return E_UNEXPECTED;
  CAutoLock cAutoLock(this);
  
  return m_pFG->AddSourceFilterForMoniker(pMoniker, pCtx, lpcwstrFilterName, ppFilter);
}

STDMETHODIMP CFGManager::ReconnectEx(IPin* ppin, const AM_MEDIA_TYPE* pmt)
{
  if (!m_pFG)
    return E_UNEXPECTED;
  CAutoLock cAutoLock(this);
  
  return m_pFG->ReconnectEx(ppin, pmt);
}

// IGraphBuilder2

HRESULT CFGManager::IsPinDirection(IPin* pPin, PIN_DIRECTION dir1)
{
  CAutoLock cAutoLock(this);

  CheckPointer(pPin, E_POINTER);

  PIN_DIRECTION dir2;
  if(FAILED(pPin->QueryDirection(&dir2)))
    return E_FAIL;

  return dir1 == dir2 ? S_OK : S_FALSE;
}

HRESULT CFGManager::IsPinConnected(IPin* pPin)
{
  CAutoLock cAutoLock(this);

  CheckPointer(pPin, E_POINTER);

  IPin* pPinTo;
  return SUCCEEDED(pPin->ConnectedTo(&pPinTo)) && pPinTo ? S_OK : S_FALSE;
}

HRESULT CFGManager::ConnectFilter(IBaseFilter* pBF, IPin* pPinIn)
{
  CAutoLock cAutoLock(this);

  CheckPointer(pBF, E_POINTER);

  if(pPinIn && S_OK != IsPinDirection(pPinIn, PINDIR_INPUT))
    return VFW_E_INVALID_DIRECTION;

  int nTotal = 0, nRendered = 0;
  GUID mediaType = GUID_NULL;

  /* Only the video pin and one audio pin can be connected
  if the filter is the splitter */
  if (pBF == CFGLoader::Filters.Splitter.pBF
    && m_audioPinConnected
    && m_videoPinConnected
    && m_subtitlePinConnected)
    return S_OK;

  BeginEnumPins(pBF, pEP, pPin)
  {
    if(DShowUtil::GetPinName(pPin)[0] != '~'
    && S_OK == IsPinDirection(pPin, PINDIR_OUTPUT)
    && S_OK != IsPinConnected(pPin))
    {
      BeginEnumMediaTypes(pPin, pEnum, pMediaType)
      {
        mediaType = pMediaType->majortype;
      }
      EndEnumMediaTypes(pMediaType)

      if (pBF == CFGLoader::Filters.Splitter.pBF && (mediaType == MEDIATYPE_Video) 
          && m_videoPinConnected)
        continue;                               // A video pin is already connected, continue !
      else if (pBF == CFGLoader::Filters.Splitter.pBF && (mediaType == MEDIATYPE_Audio)
          && m_audioPinConnected)
        continue;                               // An audio pin is already connected, continue !
      else if (pBF == CFGLoader::Filters.Splitter.pBF && mediaType == MEDIATYPE_Subtitle
          && m_subtitlePinConnected)
        continue;                               // We don't connect subtitle pin yet, continue !*/

      m_streampath.Append(pBF, pPin);

      HRESULT hr = Connect(pPin, pPinIn);

      if(SUCCEEDED(hr))
      {
        for (vector<CStreamDeadEnd>::iterator it = m_deadends.end() ; it != m_deadends.begin() ;it-- )
        {
          if((*it).Compare(m_streampath))
            m_deadends.erase(it);
        }
        //for(int i = m_deadends.size()-1; i >= 0; i--)
        //  if(m_deadends[i]->Compare(m_streampath))
        //    m_deadends.RemoveAt(i);

        nRendered++;
      }

      nTotal++;

      m_streampath.pop_back();

      if (pBF == CFGLoader::Filters.Splitter.pBF && (mediaType == MEDIATYPE_Video) 
          && SUCCEEDED(hr))
        m_videoPinConnected = true;
      else if (pBF == CFGLoader::Filters.Splitter.pBF && (mediaType == MEDIATYPE_Audio)
          && SUCCEEDED(hr))
        m_audioPinConnected = true;
      else if (pBF == CFGLoader::Filters.Splitter.pBF && mediaType == MEDIATYPE_Subtitle
          && SUCCEEDED(hr))
        m_subtitlePinConnected = true;

      if(SUCCEEDED(hr) && pPinIn)
      {
        SAFE_RELEASE(pEP);
        SAFE_RELEASE(pPin);
        return S_OK;
      }
    }
  }
  EndEnumPins

  return 
    nRendered == nTotal ? (nRendered > 0 ? S_OK : S_FALSE) :
    nRendered > 0 ? VFW_S_PARTIAL_RENDER :
    VFW_E_CANNOT_RENDER;
}

HRESULT CFGManager::ConnectFilter(IPin* pPinOut, IBaseFilter* pBF)
{
  CAutoLock cAutoLock(this);

  CheckPointer(pPinOut, E_POINTER);
  CheckPointer(pBF, E_POINTER);

  HRESULT hr;

  BeginEnumPins(pBF, pEP, pPin)
  {
    PIN_DIRECTION dir;
    if(DShowUtil::GetPinName(pPin)[0] != '~'
    && SUCCEEDED(hr = pPin->QueryDirection(&dir)) && dir == PINDIR_INPUT
    && SUCCEEDED(hr = Connect(pPinOut, pPin)))
    {
      SAFE_RELEASE(pEP);
      SAFE_RELEASE(pPin);
      return hr;
    }
  }
  EndEnumPins

  return VFW_E_CANNOT_CONNECT;
}

HRESULT CFGManager::ConnectFilterDirect(IPin* pPinOut, IBaseFilter* pBF, const AM_MEDIA_TYPE* pmt)
{
  CAutoLock cAutoLock(this);

  CheckPointer(pPinOut, E_POINTER);
  CheckPointer(pBF, E_POINTER);

  HRESULT hr;

  BeginEnumPins(pBF, pEP, pPin)
  {
    PIN_DIRECTION dir;
    if(DShowUtil::GetPinName(pPin)[0] != '~'
    && SUCCEEDED(hr = pPin->QueryDirection(&dir)) && dir == PINDIR_INPUT
    && SUCCEEDED(hr = ConnectDirect(pPinOut, pPin, pmt)))
    {
      SAFE_RELEASE(pEP);
      SAFE_RELEASE(pPin);
      return hr;
    }
  }
  EndEnumPins

  return VFW_E_CANNOT_CONNECT;
}

HRESULT CFGManager::NukeDownstream(IUnknown* pUnk)
{
  CAutoLock cAutoLock(this);

  IBaseFilter* pBF;
  IPin* pPin;
  if (SUCCEEDED(pUnk->QueryInterface(__uuidof(pBF),(void**)&pBF)))
  {
    BeginEnumPins(pBF, pEP, pPin)
    {
      NukeDownstream(pPin);
    }
    EndEnumPins
  }
  else if (SUCCEEDED(pUnk->QueryInterface(__uuidof(pPin),(void**)&pPin)))
  {
    IPin* pPinTo;
    if(S_OK == IsPinDirection(pPin, PINDIR_OUTPUT)
    && SUCCEEDED(pPin->ConnectedTo(&pPinTo)) && pPinTo)
    {
      if(IBaseFilter* pBF = DShowUtil::GetFilterFromPin(pPinTo))
      {
        NukeDownstream(pBF);
        Disconnect(pPinTo);
        Disconnect(pPin);
        RemoveFilter(pBF);
      }
    }
  }
  else
  {
    return E_INVALIDARG;
  }

  return S_OK;
}

HRESULT CFGManager::AddToROT()
{
  CAutoLock cAutoLock(this);

  HRESULT hr = E_FAIL;

  if(m_dwRegister)
    return S_FALSE;

  IRunningObjectTable* pROT = NULL;
  IMoniker* pMoniker = NULL;
  WCHAR wsz[256] = {0};
  swprintf(wsz, L"FilterGraph %08p pid %08x (XBMC)", (DWORD_PTR)this, GetCurrentProcessId());

  if(SUCCEEDED(hr = GetRunningObjectTable(0, &pROT))
  && SUCCEEDED(hr = CreateItemMoniker(L"!", wsz, &pMoniker)))
        hr = pROT->Register(ROTFLAGS_REGISTRATIONKEEPSALIVE, m_pFG, pMoniker, &m_dwRegister);

  SAFE_RELEASE(pMoniker);
  SAFE_RELEASE(pROT);
  return hr;
}

HRESULT CFGManager::RemoveFromROT()
{
  CAutoLock cAutoLock(this);

  HRESULT hr;

  if(!m_dwRegister) return S_FALSE;

  IRunningObjectTable* pROT;
  if(SUCCEEDED(hr = GetRunningObjectTable(0, &pROT))
    && SUCCEEDED(hr = pROT->Revoke(m_dwRegister)))
    m_dwRegister = 0;

  return hr;
}

// IGraphBuilderDeadEnd

STDMETHODIMP_(size_t) CFGManager::GetCount()
{
  return m_deadends.size();
}

STDMETHODIMP CFGManager::GetDeadEnd(int iIndex, std::list<CStdStringW>& path, std::list<CMediaType>& mts)
{
  CAutoLock cAutoLock(this);

  if(iIndex < 0 || iIndex >= m_deadends.size()) 
    return E_FAIL;

  while (!path.empty())
    path.pop_back();
  while (!mts.empty())
    mts.pop_back();

  path_t p;
  
  for (list<path_t>::iterator it = m_deadends.at(iIndex).begin(); it != m_deadends.at(iIndex).end(); it++)
  {
    p = *it;
    CStdStringW str;
    str.Format(L"%s::%s", p.filter, p.pin);
    path.push_back(str);
  }

  for (list<CMediaType>::iterator it = m_deadends.at(iIndex).mts.begin(); it != m_deadends.at(iIndex).mts.begin(); it++)
    mts.push_back(*it);

  return S_OK;
}

//
//   CFGManagerCustom
//
#ifdef _DSPLAYER_DEBUG
void CFGManager::LogFilterGraph(void)
{
  CStdString buffer;
  CLog::Log(LOGDEBUG, "Starting filters listing ...");
  BeginEnumFilters(m_pFG, pEF, pBF)
  {
    g_charsetConverter.wToUTF8(DShowUtil::GetFilterName(pBF), buffer);
    CLog::Log(LOGDEBUG, "%s", buffer.c_str());
  }
  EndEnumFilters(pEF, pBF)
  CLog::Log(LOGDEBUG, "End of filters listing");
}
#endif

void CFGManager::InitManager()
{
  WORD merit_low = 1;
  ULONGLONG merit;
  merit = MERIT64_ABOVE_DSHOW + merit_low++;


// Blocked filters
  
// "Subtitle Mixer" makes an access violation around the 
// 11-12th media type when enumerating them on its output.
  m_transform.push_back(new CFGFilterRegistry(DShowUtil::GUIDFromCString(_T("{00A95963-3BE5-48C0-AD9F-3356D67EA09D}")), MERIT64_DO_NOT_USE));

// DiracSplitter.ax is crashing MPC-HC when opening invalid files...
  m_transform.push_back(new CFGFilterRegistry(DShowUtil::GUIDFromCString(_T("{09E7F58E-71A1-419D-B0A0-E524AE1454A9}")), MERIT64_DO_NOT_USE));
  m_transform.push_back(new CFGFilterRegistry(DShowUtil::GUIDFromCString(_T("{5899CFB9-948F-4869-A999-5544ECB38BA5}")), MERIT64_DO_NOT_USE));
  m_transform.push_back(new CFGFilterRegistry(DShowUtil::GUIDFromCString(_T("{F78CF248-180E-4713-B107-B13F7B5C31E1}")), MERIT64_DO_NOT_USE));

// ISCR suxx
  m_transform.push_back(new CFGFilterRegistry(DShowUtil::GUIDFromCString(_T("{48025243-2D39-11CE-875D-00608CB78066}")), MERIT64_DO_NOT_USE));

// Samsung's "mpeg-4 demultiplexor" can even open matroska files, amazing...
  m_transform.push_back(new CFGFilterRegistry(DShowUtil::GUIDFromCString(_T("{99EC0C72-4D1B-411B-AB1F-D561EE049D94}")), MERIT64_DO_NOT_USE));

// LG Video Renderer (lgvid.ax) just crashes when trying to connect it
  m_transform.push_back(new CFGFilterRegistry(DShowUtil::GUIDFromCString(_T("{9F711C60-0668-11D0-94D4-0000C02BA972}")), MERIT64_DO_NOT_USE));

// palm demuxer crashes (even crashes graphedit when dropping an .ac3 onto it)
  m_transform.push_back(new CFGFilterRegistry(DShowUtil::GUIDFromCString(_T("{BE2CF8A7-08CE-4A2C-9A25-FD726A999196}")), MERIT64_DO_NOT_USE));

// mainconcept color space converter
  m_transform.push_back(new CFGFilterRegistry(DShowUtil::GUIDFromCString(_T("{272D77A0-A852-4851-ADA4-9091FEAD4C86}")), MERIT64_DO_NOT_USE));

  //Block if vmr9 is used
  if (g_sysinfo.IsVistaOrHigher())
    if (g_guiSettings.GetBool("dsplayer.forcenondefaultrenderer"))
      m_transform.push_back(new CFGFilterRegistry(DShowUtil::GUIDFromCString(_T("{9852A670-F845-491B-9BE6-EBD841B8A613}")), MERIT64_DO_NOT_USE));
  else
    if (!g_guiSettings.GetBool("dsplayer.forcenondefaultrenderer"))
      m_transform.push_back(new CFGFilterRegistry(DShowUtil::GUIDFromCString(_T("{9852A670-F845-491B-9BE6-EBD841B8A613}")), MERIT64_DO_NOT_USE));

    
  
  if(m_pFM)
  {
    IEnumMoniker* pEM;

    GUID guids[] = {MEDIATYPE_Video, MEDIASUBTYPE_NULL};

    if(SUCCEEDED(m_pFM->EnumMatchingFilters(&pEM, 0, FALSE, MERIT_DO_NOT_USE+1,
      TRUE, 1, guids, NULL, NULL, TRUE, FALSE, 0, NULL, NULL, NULL)))
    {
      for(IMoniker* pMoniker; S_OK == pEM->Next(1, &pMoniker, NULL); pMoniker = NULL)
      {
        CFGFilterRegistry f(pMoniker);
        m_vrmerit = max(m_vrmerit, f.GetMerit());
      }
    }

    m_vrmerit += 0x100;
  }

  if(m_pFM)
  {
    IEnumMoniker* pEM;

    GUID guids[] = {MEDIATYPE_Audio, MEDIASUBTYPE_NULL};

    if(SUCCEEDED(m_pFM->EnumMatchingFilters(&pEM, 0, FALSE, MERIT_DO_NOT_USE+1,
      TRUE, 1, guids, NULL, NULL, TRUE, FALSE, 0, NULL, NULL, NULL)))
    {
      for(IMoniker* pMoniker; S_OK == pEM->Next(1, &pMoniker, NULL); pMoniker = NULL)
      {
        CFGFilterRegistry f(pMoniker);
        m_armerit = max(m_armerit, f.GetMerit());
      }
    }

    BeginEnumSysDev(CLSID_AudioRendererCategory, pMoniker)
    {
      CFGFilterRegistry f(pMoniker);
      m_armerit = max(m_armerit, f.GetMerit());
    }
    EndEnumSysDev

    m_armerit += 0x100;
  }

  CStdString fileconfigtmp;
  fileconfigtmp = _P("special://xbmc/system/players/dsplayer/dsfilterconfig.xml");
  //Load the config for the xml
  m_CfgLoader = new CFGLoader();

  if (SUCCEEDED(m_CfgLoader->LoadConfig(m_pFG)))
    CLog::Log(LOGNOTICE,"Successfully loaded %s",fileconfigtmp.c_str());
  else
    CLog::Log(LOGERROR,"Failed loading %s",fileconfigtmp.c_str());
}

void CFGManager::UpdateRegistry()
{
  return;
  CStdString strRegKey;
  strRegKey.Format("Software\\Gnu\\ffdshow_dxva\\default");
  RegKey ffReg(HKEY_CURRENT_USER ,"Software\\Gnu\\ffdshow" ,true);
  RegKey ffRegDxva(HKEY_CURRENT_USER ,strRegKey.c_str() ,true);
  //Adding dxva config to ffdshow just in case its off
  ffRegDxva.setValue("dec_DXVA_H264",DWORD(1));
  ffRegDxva.setValue("dec_DXVA_VC1",DWORD(1));
  //Needed to process subtitles
  ffRegDxva.setValue("dec_dxva_postProcessingMode",DWORD(1));
    
  //this reg var is to show the dialog when unknown application connect
  //set to 0 to remove the dialog
  //ffdshow\\isCompMgr
  ffReg.setValue("isCompMgr",DWORD(0));
  //this one is to use only the prog in the white list
  //ffdshow\\isWhitelist
  //set to 0 if you want to enable every app to use ffdshow
  ffReg.setValue("isWhitelist",DWORD(0));
}
