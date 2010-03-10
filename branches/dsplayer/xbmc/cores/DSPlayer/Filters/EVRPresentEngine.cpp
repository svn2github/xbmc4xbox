#include "EVRPresentEngine.h"
#include "EVRAllocatorPresenter.h"
#include <evr.h>
#include <d3dx9tex.h>
#include "utils/log.h"
#include "DShowUtil/dshowutil.h"
#include "WindowingFactory.h" //d3d device and d3d interface
#include "cores/VideoRenderers/RenderManager.h"

#include "application.h"
#include "IPinHook.h"

//-----------------------------------------------------------------------------
// Constructor
//-----------------------------------------------------------------------------
//static const GUID GUID_SURFACE_INDEX = { 0x30c8e9f6, 0x415, 0x4b81, { 0xa3, 0x15, 0x1, 0xa, 0xc6, 0xa9, 0xda, 0x19 } };

D3DPresentEngine::D3DPresentEngine(CEVRAllocatorPresenter *presenter, HRESULT& hr) : 
  m_DeviceResetToken(0),
  m_pDeviceManager(NULL),
  m_pCallback(NULL),
  m_bufferCount(4),
  m_pAllocatorPresenter(presenter),
  m_bExiting(false),
  m_pVideoSurface(NULL),
  m_pVideoTexture(NULL),
  m_bNeedNewDevice(false)
{
  SetRectEmpty(&m_rcDestRect);

  pDXVA2HLib = LoadLibrary ("dxva2.dll");
  pfDXVA2CreateDirect3DDeviceManager9  = pDXVA2HLib ? (PTR_DXVA2CreateDirect3DDeviceManager9) GetProcAddress (pDXVA2HLib, "DXVA2CreateDirect3DDeviceManager9") : NULL;
  pEVRHLib = LoadLibrary ("evr.dll");
  pfMFCreateVideoSampleFromSurface  = pEVRHLib ? (PTR_MFCreateVideoSampleFromSurface)  GetProcAddress (pEVRHLib, "MFCreateVideoSampleFromSurface") : NULL;

  if (!pfMFCreateVideoSampleFromSurface)
    CLog::Log(LOGERROR,"Could not find MFCreateVideoSampleFromSurface (evr.dll)");

  ZeroMemory(&m_DisplayMode, sizeof(m_DisplayMode));
  ZeroMemory(m_pInternalVideoTexture, 7 * sizeof(IDirect3DTexture9*));
  ZeroMemory(m_pInternalVideoSurface, 7 * sizeof(IDirect3DSurface9*));

  m_pVideoTexture = NULL;

  hr = InitializeDXVA();

  g_Windowing.Register(this);
  g_renderManager.PreInit(true);
}


//-----------------------------------------------------------------------------
// Destructor
//-----------------------------------------------------------------------------

D3DPresentEngine::~D3DPresentEngine()
{
  m_pDeviceManager = NULL;
  m_pCallback = NULL;
  // Unload DLL
  if (pDXVA2HLib)
    FreeLibrary(pDXVA2HLib);
  if (pEVRHLib)
    FreeLibrary(pEVRHLib);

  g_renderManager.UnInit();
  g_Windowing.Unregister(this);
}


//-----------------------------------------------------------------------------
// GetService
//
// Returns a service interface from the presenter engine.
// The presenter calls this method from inside it's implementation of 
// IMFGetService::GetService.
//
// Classes that derive from D3DPresentEngine can override this method to return 
// other interfaces. If you override this method, call the base method from the 
// derived class.
//-----------------------------------------------------------------------------

HRESULT D3DPresentEngine::GetService(REFGUID guidService, REFIID riid, void** ppv)
{
  if (guidService == MR_VIDEO_ACCELERATION_SERVICE)
    return m_pDeviceManager->QueryInterface (__uuidof(IDirect3DDeviceManager9), (void**) ppv);

  return E_NOINTERFACE;
}


//-----------------------------------------------------------------------------
// CheckFormat
//
// Queries whether the D3DPresentEngine can use a specified Direct3D format.
//-----------------------------------------------------------------------------

HRESULT D3DPresentEngine::CheckFormat(D3DFORMAT format)
{
  HRESULT hr = S_OK;

  UINT uAdapter = D3DADAPTER_DEFAULT;
  D3DDEVTYPE type = D3DDEVTYPE_HAL;

  D3DDISPLAYMODE mode;
  D3DDEVICE_CREATION_PARAMETERS params;

  if (g_Windowing.Get3DDevice())
  {
    hr = g_Windowing.Get3DDevice()->GetCreationParameters(&params);
    if(FAILED(hr))
      CLog::Log(LOGERROR,"%s",__FUNCTION__);

    uAdapter = params.AdapterOrdinal;
    type = params.DeviceType;

  }
  hr = g_Windowing.Get3DObject()->GetAdapterDisplayMode(uAdapter, &mode);
  if(FAILED(hr))
    CLog::Log(LOGERROR,"%s",__FUNCTION__);

  hr = g_Windowing.Get3DObject()->CheckDeviceType(uAdapter, type, mode.Format, format, TRUE); 
  if(FAILED(hr))
    CLog::Log(LOGERROR,"%s",__FUNCTION__);

  return hr;
}

//-----------------------------------------------------------------------------
// SetDestinationRect
// 
// Sets the region within the video window where the video is drawn.
//-----------------------------------------------------------------------------

HRESULT D3DPresentEngine::SetDestinationRect(const RECT& rcDest)
{
  if (EqualRect(&rcDest, &m_rcDestRect))
    return S_OK; // No change.
  CAutoLock lock(&m_ObjectLock);
  m_rcDestRect = rcDest;
  return S_OK;
}

//-----------------------------------------------------------------------------
// CreateVideoSamples
// 
// Creates video samples based on a specified media type.
// 
// pFormat: Media type that describes the video format.
// videoSampleQueue: List that will contain the video samples.
//
// Note: For each video sample, the method creates a swap chain with a
// single back buffer. The video sample object holds a pointer to the swap
// chain's back buffer surface. The mixer renders to this surface, and the
// D3DPresentEngine renders the video frame by presenting the swap chain.
//-----------------------------------------------------------------------------

HRESULT D3DPresentEngine::CreateVideoSamples(IMFMediaType *pFormat ,VideoSampleList& videoSampleQueue)
{
  if (!pFormat)
    return MF_E_UNEXPECTED;

  HRESULT hr = S_OK;
  D3DPRESENT_PARAMETERS pp;

  IMFSample *pVideoSample = NULL;
  
  CAutoLock lock(&m_ObjectLock);

  ReleaseResources();

  //get the video frame size for the current IMFMediaType
  hr = MFGetAttributeSize(pFormat, MF_MT_FRAME_SIZE, &m_iVideoWidth, &m_iVideoHeight);
  if (FAILED(hr))
  {
    CLog::Log(LOGERROR,"%s Getting the frame size returned an error",__FUNCTION__);
    return hr;
  }

  D3DFORMAT d3dFormat;
  GUID subtype = GUID_NULL;
  /* Get D3DFORMAT from MEDIA_SubType*/

  pFormat->GetGUID(MF_MT_SUBTYPE, & subtype);

  if (subtype == MFVideoFormat_RGB565)
    d3dFormat = D3DFMT_R5G6B5;
  else if (subtype == MFVideoFormat_RGB555)
    d3dFormat = D3DFMT_X1R5G5B5;
  else if (subtype == MFVideoFormat_RGB24)
    d3dFormat = D3DFMT_R8G8B8;
  else if (subtype == MFVideoFormat_ARGB32)
    d3dFormat = D3DFMT_A8R8G8B8;
  else
    d3dFormat = D3DFMT_X8R8G8B8;

  if (FAILED(g_Windowing.Get3DDevice()->CreateTexture(m_iVideoWidth ,
                                                      m_iVideoHeight ,
                                                      1 , 
                                                      D3DUSAGE_RENDERTARGET,
                                                      d3dFormat,
                                                      D3DPOOL_DEFAULT,
                                                      &m_pVideoTexture,
                                                      NULL)))
  {
    CLog::Log(LOGERROR,"%s Error while creating the video texture",__FUNCTION__);
    return E_FAIL;
  }

  hr = m_pVideoTexture->GetSurfaceLevel(0, &m_pVideoSurface);  
  
  // Create the video samples.
  for (int i = 0; i < m_bufferCount; i++)
  {
    hr = g_Windowing.Get3DDevice()->CreateTexture(m_iVideoWidth ,
                                                  m_iVideoHeight ,
                                                  1 ,
                                                  D3DUSAGE_RENDERTARGET ,
                                                  d3dFormat ,
                                                  D3DPOOL_DEFAULT ,
                                                  &m_pInternalVideoTexture[i] ,
                                                  NULL);
    CHECK_HR(hr);
    hr  = m_pInternalVideoTexture[i]->GetSurfaceLevel(0, &m_pInternalVideoSurface[i]);
    if (FAILED(hr))
      m_pInternalVideoSurface[i] = NULL;

    hr = CreateD3DSample(m_pInternalVideoSurface[i], &pVideoSample, i);
    
    // Add it to the samples list.
    CHECK_HR(hr = videoSampleQueue.InsertBack(pVideoSample));
    S_RELEASE(pVideoSample);
  }

  // Let the derived class create any additional D3D resources that it needs.
  CHECK_HR(hr = OnCreateVideoSamples(pp));

done:
  if (FAILED(hr))
    ReleaseResources();
    
  S_RELEASE(pVideoSample);
  return hr;
}



//-----------------------------------------------------------------------------
// ReleaseResources
// 
// Released Direct3D resources used by this object. 
//-----------------------------------------------------------------------------

void D3DPresentEngine::ReleaseResources()
{
  // Let the derived class release any resources it created.
  OnReleaseResources();

  CAutoLock lock(&m_ObjectLock);

  SAFE_RELEASE(m_pVideoSurface);

  //Releasing video surface
  for (int i = 0; i < 7; i++) 
  { 
    SAFE_RELEASE(m_pInternalVideoTexture[i]);
    m_pInternalVideoSurface[i] = NULL;
  } 
}

//-----------------------------------------------------------------------------
// PresentSample
//
// Presents a video frame.
//
// pSample:  Pointer to the sample that contains the surface to present. If 
//           this parameter is NULL, the method paints a black rectangle.
// llTarget: Target presentation time.
//
// This method is called by the scheduler and/or the presenter.
//-----------------------------------------------------------------------------
class CEVRAllocatorPresenter;

HRESULT D3DPresentEngine::PresentSample(IMFSample* pSample, LONGLONG llTarget)
{
  HRESULT hr = S_OK;

  IMFMediaBuffer* pBuffer = NULL;
  IDirect3DSurface9* pSurface = NULL;

  if (m_pAllocatorPresenter->CheckShutdown() != S_OK)
    return S_OK;
    
  if (!g_renderManager.IsConfigured())
    return S_OK;

  if (m_bNeedNewDevice)
    return S_OK;

  if (! m_pVideoTexture)
    return S_OK;
  
  if (pSample)
  {
    CHECK_HR(hr = pSample->GetBufferByIndex(0, &pBuffer));
    IMFGetService* pServ;
    CHECK_HR(hr = pBuffer->QueryInterface(__uuidof(IMFGetService),(void**)&pServ));
    CHECK_HR(hr = pServ->GetService(MR_BUFFER_SERVICE, __uuidof(IDirect3DSurface9), (void**)&pSurface))
    SAFE_RELEASE(pServ);
  }
  if (m_bNeedNewDevice || !g_Windowing.Get3DDevice())
    return S_OK;

  if (pSurface)
  {
    hr = g_Windowing.Get3DDevice()->StretchRect(pSurface, NULL, m_pVideoSurface, NULL, D3DTEXF_NONE);    
    g_renderManager.PaintVideoTexture(m_pVideoTexture, m_pVideoSurface);
    g_application.NewFrame();
    g_application.WaitFrame(100);
  }

done:
  S_RELEASE(pSurface);
  S_RELEASE(pBuffer);

  if (FAILED(hr))
  {
    if (hr == D3DERR_DEVICELOST || hr == D3DERR_DEVICENOTRESET)
    {
      CLog::Log(LOGDEBUG,"D3DDevice was lost!");
      // Ignore. We need to reset or re-create the device, but this method
      // is probably being called from the scheduler thread, which is not the
      // same thread that created the device. The Reset(Ex) method must be
      // called from the thread that created the device.

      // The presenter will detect the state when it calls CheckDeviceState() 
      // on the next sample.
      hr = S_OK;
    }
  }
  return hr;
}

//-----------------------------------------------------------------------------
// private/protected methods
//-----------------------------------------------------------------------------


//-----------------------------------------------------------------------------
// InitializeDXVA
// 
// Initializes Direct3D and the Direct3D device manager.
//-----------------------------------------------------------------------------

HRESULT D3DPresentEngine::InitializeDXVA()
{
  HRESULT hr = S_OK;
    
  hr = pfDXVA2CreateDirect3DDeviceManager9(&m_DeviceResetToken, &m_pDeviceManager);

  if (SUCCEEDED(hr))
  {
    CLog::Log(LOGNOTICE, "%s Successfully created IDirect3DDeviceManager9", __FUNCTION__);

    hr = m_pDeviceManager->ResetDevice(g_Windowing.Get3DDevice(), m_DeviceResetToken);
  }

  Com::SmartPtr<IDirectXVideoDecoderService>	pDecoderService;
  HANDLE							hDevice;
  if (SUCCEEDED (m_pDeviceManager->OpenDeviceHandle(&hDevice)) &&
    SUCCEEDED (m_pDeviceManager->GetVideoService (hDevice, __uuidof(IDirectXVideoDecoderService), (void**)&pDecoderService)))
  {
    HookDirectXVideoDecoderService (pDecoderService);
    m_pDeviceManager->CloseDeviceHandle (hDevice);
  }

  return hr;
}

//-----------------------------------------------------------------------------
// CreateD3DSample
//
// Creates an sample object (IMFSample) to hold a Direct3D swap chain.
//-----------------------------------------------------------------------------

HRESULT D3DPresentEngine::CreateD3DSample(IDirect3DSurface9 *pSurface, IMFSample **ppVideoSample, int surfaceIndex)
{
  // Caller holds the object lock.

  HRESULT hr = S_OK;
  D3DCOLOR clrBlack = D3DCOLOR_ARGB(0xFF, 0x00, 0x00, 0x00);

  IMFSample* pSample = NULL;
  // Create the sample.
  CHECK_HR(hr = pfMFCreateVideoSampleFromSurface(pSurface, &pSample));
  CHECK_HR(hr = pSample->SetUINT32(GUID_SURFACE_INDEX,surfaceIndex));

  // Return the pointer to the caller.
  *ppVideoSample = pSample;
  (*ppVideoSample)->AddRef();

done:
    S_RELEASE(pSurface);
    S_RELEASE(pSample);
  return hr;
}

//-----------------------------------------------------------------------------
// RegisterCallback
//
// Registers a callback sink for getting the D3D surface and
// is called for every video frame that needs to be rendered
//-----------------------------------------------------------------------------
HRESULT D3DPresentEngine::RegisterCallback(IEVRPresenterCallback *pCallback)
{
  if(m_pCallback)
    m_pCallback = NULL;

  m_pCallback = pCallback;

  return S_OK;
}

//-----------------------------------------------------------------------------
// SetBufferCount
//
// Sets the total number of buffers to use when the EVR
// custom presenter is running.
//-----------------------------------------------------------------------------
HRESULT D3DPresentEngine::SetBufferCount(int bufferCount)
{
  m_bufferCount = bufferCount;
  return S_OK;
}

void D3DPresentEngine::OnDestroyDevice()
{

}

void D3DPresentEngine::OnCreateDevice()
{

}

void D3DPresentEngine::OnLostDevice()
{
  ReleaseResources();
}

void D3DPresentEngine::OnResetDevice()
{
  HRESULT		hr;

  // Reset DXVA Manager, and get new buffers
  hr = m_pDeviceManager->ResetDevice(g_Windowing.Get3DDevice(), m_DeviceResetToken);

  m_pAllocatorPresenter->NotifyEvent(EC_DISPLAY_CHANGED, S_OK, 0);
}
