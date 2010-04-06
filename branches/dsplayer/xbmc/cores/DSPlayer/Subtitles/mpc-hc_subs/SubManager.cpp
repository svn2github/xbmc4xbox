#include "stdafx.h"
#include <d3d9.h>
#include "..\subpic\ISubPic.h"
#include "..\subpic\DX9SubPic.h"
#include <moreuuids.h>
#include "..\subtitles\VobSubFile.h"
#include "..\subtitles\RTS.h"
#include "..\DSUtil\NullRenderers.h"
#include "SubManager.h"
#include "TextPassThruFilter.h"

BOOL g_overrideUserStyles;
int g_subPicsBufferAhead(3);
bool g_pow2tex(true);
BOOL g_disableAnim(TRUE);

CSubManager::CSubManager(IDirect3DDevice9* d3DDev, SIZE size, HRESULT& hr)
	: m_d3DDev(d3DDev), m_iSubtitleSel(-1), m_rtNow(-1), m_lastSize(size),
  m_textureSize(1024, 768), m_rtTimePerFrame(0), m_useDefaultStyle(true)//Set on XBMC Side
{

	//ATLTRACE("CSubManager constructor: texture size %dx%d, buffer ahead: %d, pow2tex: %d", g_textureSize.cx, g_textureSize.cy, g_subPicsBufferAhead, g_pow2tex);
	m_pAllocator = (new CDX9SubPicAllocator(d3DDev, m_textureSize, g_pow2tex));
	hr = S_OK;
	if (g_subPicsBufferAhead > 0)
		m_pSubPicQueue.reset(new CSubPicQueue(g_subPicsBufferAhead, g_disableAnim, m_pAllocator, &hr));
	else
		m_pSubPicQueue.reset(new CSubPicQueueNoThread(m_pAllocator, &hr));
	if (FAILED(hr))
	{
    //ATLTRACE("CSubPicQueue creation error: %x", hr);
	}
}

CSubManager::~CSubManager(void)
{
}

void CSubManager::SetStyle(STSStyle style)
{
  m_style = style; m_useDefaultStyle = false;
  UpdateSubtitle();
}
void CSubManager::ApplyStyle(CRenderedTextSubtitle* pRTS)
{

}

void CSubManager::ApplyStyleSubStream(ISubStream* pSubStream)
{
  if (m_useDefaultStyle || !pSubStream)
    return;

  CLSID clsid;
  if (FAILED(pSubStream->GetClassID(&clsid)))
    return;

	if(clsid == __uuidof(CRenderedTextSubtitle))
	{
		CRenderedTextSubtitle* pRTS = (CRenderedTextSubtitle*)(ISubStream*)pSubStream;

    pRTS->SetDefaultStyle(m_style);
		//pRTS->SetOverride(true, &m_style); // Don't override, or maybe set an option on gui

		pRTS->Deinit();
	}
}

void CSubManager::SetSubPicProvider(ISubStream* pSubStream)
{
  ApplyStyleSubStream(pSubStream);

	m_pSubPicQueue->SetSubPicProvider(Com::SmartQIPtr<ISubPicProvider>(pSubStream));
	m_subresync.RemoveAll();
}

void CSubManager::SetTextPassThruSubStream(ISubStream* pSubStreamOld, ISubStream* pSubStreamNew)
{
	ApplyStyleSubStream(pSubStreamNew);
	m_pInternalSubStream = pSubStreamNew;
  SetSubPicProvider(m_pInternalSubStream);
}

void CSubManager::InvalidateSubtitle(DWORD_PTR nSubtitleId, REFERENCE_TIME rtInvalidate)
{
  m_pSubPicQueue->Invalidate(rtInvalidate);
}

void CSubManager::UpdateSubtitle()
{
  ISubStream* pSubPic = NULL;
  if (SUCCEEDED(m_pSubPicQueue->GetSubPicProvider((ISubPicProvider**) &pSubPic)) && pSubPic)
  {
    ApplyStyleSubStream(pSubPic);
  }
}

void CSubManager::SetEnable(bool enable)
{
	if ((enable && m_iSubtitleSel < 0) || (!enable && m_iSubtitleSel >= 0))
	{
		m_iSubtitleSel ^= 0x80000000;
		UpdateSubtitle();
	}
}

void CSubManager::SetTime(REFERENCE_TIME rtNow)
{
  m_rtNow = rtNow;
  m_pSubPicQueue->SetTime(m_rtNow);
}

HRESULT CSubManager::GetTexture(Com::SmartPtr<IDirect3DTexture9>& pTexture, Com::SmartRect& pSrc, Com::SmartRect& pDest, Com::SmartRect& renderRect)
{
  if (m_iSubtitleSel < 0)
    return E_INVALIDARG;

  m_fps = 10000000.0 / m_rtTimePerFrame;
  m_pSubPicQueue->SetFPS(m_fps);

  Com::SmartSize renderSize(renderRect.right, renderRect.bottom);
  if (m_lastSize != renderSize && renderRect.right > 0 && renderRect.bottom > 0)
  { 
    m_pAllocator->ChangeDevice(m_d3DDev);
    m_pAllocator->SetCurSize(renderSize);
    m_pAllocator->SetCurVidRect(renderRect);
    m_pSubPicQueue->Invalidate(m_rtNow + 1000000);
    m_lastSize = renderSize;
  }

  Com::SmartPtr<ISubPic> pSubPic;
  if(m_pSubPicQueue->LookupSubPic(m_rtNow, pSubPic))
  {
    if (SUCCEEDED (pSubPic->GetSourceAndDest(&renderSize, pSrc, pDest)))
    {
      return pSubPic->GetTexture(pTexture);      
    }
  }

  return E_FAIL;
}

static bool IsTextPin(IPin* pPin)
{
	bool isText = false;
	BeginEnumMediaTypes(pPin, pEMT, pmt)
	{
		if (pmt->majortype == MEDIATYPE_Text || pmt->majortype == MEDIATYPE_Subtitle)
		{
			isText = true;
			break;
		}
	}
	EndEnumMediaTypes(pmt)
	return isText;
}

static bool isTextConnection(IPin* pPin)
{
	AM_MEDIA_TYPE mt;
	if (FAILED(pPin->ConnectionMediaType(&mt)))
		return false;
	bool isText = (mt.majortype == MEDIATYPE_Text || mt.majortype == MEDIATYPE_Subtitle);
	FreeMediaType(mt);
	return isText;
}

//load internal subtitles through TextPassThruFilter
HRESULT CSubManager::InsertPassThruFilter(IGraphBuilder* pGB)
{
	BeginEnumFilters(pGB, pEF, pBF)
	{
		if(!IsSplitter(pBF)) continue;

    BeginEnumPins(pBF, pEP, pPin)
		{
			PIN_DIRECTION pindir;
			pPin->QueryDirection(&pindir);
			if (pindir != PINDIR_OUTPUT)
				continue;
			Com::SmartPtr<IPin> pPinTo;
			pPin->ConnectedTo(&pPinTo);
			if (pPinTo)
			{
				if (!isTextConnection(pPin))
					continue;
				pGB->Disconnect(pPin);
				pGB->Disconnect(pPinTo);
			}
			else if (!IsTextPin(pPin))
				continue;
			
			Com::SmartQIPtr<IBaseFilter> pTPTF = new CTextPassThruFilter(this);
			CStdStringW name = L"XBMC Subtitles Pass Thru";
			if(FAILED(pGB->AddFilter(pTPTF, name)))
				continue;

			Com::SmartQIPtr<ISubStream> pSubStream;
			HRESULT hr;
			do
			{
				if (FAILED(hr = pGB->ConnectDirect(pPin, GetFirstPin(pTPTF, PINDIR_INPUT), NULL)))
				{
					break;
				}
				Com::SmartQIPtr<IBaseFilter> pNTR = new CNullTextRenderer(NULL, &hr);
        name = L"XBMC Null Renderer";
				if (FAILED(hr) || FAILED(pGB->AddFilter(pNTR, name)))
					break;

				if FAILED(hr = pGB->ConnectDirect(GetFirstPin(pTPTF, PINDIR_OUTPUT), GetFirstPin(pNTR, PINDIR_INPUT), NULL))
					break;
				pSubStream = pTPTF;
			} while(0);

			if (pSubStream)
			{
				ApplyStyleSubStream(pSubStream);
				return S_OK;
			}
			else
			{
				pGB->RemoveFilter(pTPTF);
			}
		}
		EndEnumPins
	}
	EndEnumFilters
  
  return E_FAIL;
}

CStdString GetExtension(CStdString&  filename)
{
  const size_t i = filename.rfind('.');
  return filename.substr(i+1, filename.size());
}

HRESULT CSubManager::LoadExternalSubtitle( const wchar_t* subPath, ISubStream** pSubPic )
{
  if (!pSubPic)
    return E_POINTER;

  CStdStringW path(subPath);
  *pSubPic = NULL;
  try
  {
    Com::SmartPtr<ISubStream> pSubStream;

    if(!pSubStream)
    {
      std::auto_ptr<CVobSubFile> pVSF(new CVobSubFile(&m_csSubLock));
      if(CStdString(GetExtension(path).MakeLower()) == _T(".idx") && pVSF.get() && pVSF->Open(path) && pVSF->GetStreamCount() > 0)
        pSubStream = pVSF.release();
    }

    if(!pSubStream)
    {
      std::auto_ptr<CRenderedTextSubtitle> pRTS(new CRenderedTextSubtitle(&m_csSubLock));
      if(pRTS.get() && pRTS->Open(path, DEFAULT_CHARSET) && pRTS->GetStreamCount() > 0) {
        ApplyStyleSubStream(pRTS.get());
        pSubStream = pRTS.release();
      }
    }
    if (pSubStream)
    {
      *pSubPic = pSubStream.Detach();
      return S_OK;
    }
  }
  catch(... /*CException* e*/)
  {
    //e->Delete();
  }

  return E_FAIL;
}

void CSubManager::SetTimePerFrame( REFERENCE_TIME timePerFrame )
{
  m_rtTimePerFrame = timePerFrame;
}

void CSubManager::Free()
{
  m_pSubPicQueue.reset();
  m_pAllocator = NULL;
}

HRESULT CSubManager::SetSubPicProviderToInternal()
{
  if (! m_pInternalSubStream)
    return E_FAIL;
  
  SetSubPicProvider(m_pInternalSubStream);
  return S_OK;
}

void CSubManager::SetTextureSize( Com::SmartSize& pSize )
{
  m_textureSize = pSize;
  if (m_pAllocator)
  {
    m_pAllocator->SetMaxTextureSize(m_textureSize);
    m_pSubPicQueue->Invalidate(m_rtNow + 1000000);
  }
}