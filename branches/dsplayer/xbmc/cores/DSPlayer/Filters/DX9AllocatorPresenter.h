/* 
 *  Copyright (C) 2003-2006 Gabest
 *  http://www.gabest.org
 *
 *  Copyright (C) 2005-2010 Team XBMC
 *  http://www.xbmc.org
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

#pragma once

#include "AllocatorCommon.h"
#include "RendererSettings.h"
#include "cores/VideoRenderers/RenderManager.h"
#include "D3DResource.h"
#include "IPaintCallback.h"
#include "DShowUtil/smartlist.h"
#include "utils/Event.h"
// Support ffdshow queueing.
// This interface is used to check version of Media Player Classic.
// {A273C7F6-25D4-46b0-B2C8-4F7FADC44E37}
//DEFINE_GUID(IID_IVMRffdshow9,
//0xa273c7f6, 0x25d4, 0x46b0, 0xb2, 0xc8, 0x4f, 0x7f, 0xad, 0xc4, 0x4e, 0x37);

MIDL_INTERFACE("A273C7F6-25D4-46b0-B2C8-4F7FADC44E37")
IVMRffdshow9 : public IUnknown
{
public:
  virtual STDMETHODIMP support_ffdshow(void) = 0;
};

#define VMRBITMAP_UPDATE            0x80000000
#define MAX_PICTURE_SLOTS      (60+2)        // Last 2 for pixels shader!

#define NB_JITTER          126

  class CDX9AllocatorPresenter
    : public ISubPicAllocatorPresenterImpl ,
      public ID3DResource,
      public IPaintCallback

  {
  public:
    CCritSec        m_VMR9AlphaBitmapLock;
    void          UpdateAlphaBitmap();
  protected:
    Com::SmartSize  m_ScreenSize;
    Com::SmartRect  m_pScreenSize;
    UINT  m_RefreshRate;

//    bool  m_fVMRSyncFix;
    bool  m_bAlternativeVSync;
    bool  m_bHighColorResolution;
    bool  m_bCompositionEnabled;
    bool  m_bIsEVR;
    int    m_OrderedPaint;
    int    m_VSyncMode;
    bool  m_bDesktopCompositionDisabled;
    bool  m_bIsFullscreen;
    bool  m_bNeedCheckSample;
    DWORD  m_MainThreadId;
    CDsSettings::CRendererSettingsEVR m_LastRendererSettings;

    HRESULT (__stdcall * m_pDwmIsCompositionEnabled)(__out BOOL* pfEnabled);
    HRESULT (__stdcall * m_pDwmEnableComposition)(UINT uCompositionAction);

    HMODULE m_hDWMAPI;

    HMODULE m_hD3D9;

    CCritSec          m_RenderLock;

    Com::SmartPtr<IDirect3D9>      m_pD3D;

    void LockD3DDevice()
    {
      if (m_pD3DDev)
      {
        _RTL_CRITICAL_SECTION *pCritSec = (_RTL_CRITICAL_SECTION *)((size_t)m_pD3DDev.m_ptr + sizeof(size_t));

        if (!IsBadReadPtr(pCritSec, sizeof(*pCritSec)) && !IsBadWritePtr(pCritSec, sizeof(*pCritSec))
          && !IsBadReadPtr(pCritSec->DebugInfo, sizeof(*(pCritSec->DebugInfo))) && !IsBadWritePtr(pCritSec->DebugInfo, sizeof(*(pCritSec->DebugInfo))))      
        {
          if (pCritSec->DebugInfo->CriticalSection == pCritSec)
            EnterCriticalSection(pCritSec);
        }
      }
    }

    void UnlockD3DDevice()
    {
      if (m_pD3DDev)
      {
        _RTL_CRITICAL_SECTION *pCritSec = (_RTL_CRITICAL_SECTION *)((size_t)m_pD3DDev.m_ptr + sizeof(size_t));

        if (!IsBadReadPtr(pCritSec, sizeof(*pCritSec)) && !IsBadWritePtr(pCritSec, sizeof(*pCritSec))
          && !IsBadReadPtr(pCritSec->DebugInfo, sizeof(*(pCritSec->DebugInfo))) && !IsBadWritePtr(pCritSec->DebugInfo, sizeof(*(pCritSec->DebugInfo))))      
        {
          if (pCritSec->DebugInfo->CriticalSection == pCritSec)
            LeaveCriticalSection(pCritSec);
        }
      }
    }
    CStdString m_D3DDevExError;
    Com::SmartPtr<IDirect3DDevice9>    m_pD3DDev;
    Com::SmartPtr<IDirect3DTexture9>    m_pVideoTexture[MAX_PICTURE_SLOTS];
    Com::SmartPtr<IDirect3DSurface9>    m_pVideoSurface[MAX_PICTURE_SLOTS];
    Com::SmartPtr<IDirect3DTexture9>    m_pOSDTexture;
    Com::SmartPtr<IDirect3DSurface9>    m_pOSDSurface;
    Com::SmartPtr<ID3DXLine>      m_pLine;
    Com::SmartPtr<ID3DXFont>      m_pFont;
    Com::SmartPtr<ID3DXSprite>    m_pSprite;
    class CExternalPixelShader
    {
    public:
      Com::SmartPtr<IDirect3DPixelShader9> m_pPixelShader;
      CStdStringA m_SourceData;
      CStdStringA m_SourceTarget;
      HRESULT Compile(CPixelShaderCompiler *pCompiler)
      {
        HRESULT hr = pCompiler->CompileShader(m_SourceData, "main", m_SourceTarget, 0, &m_pPixelShader);
        if(FAILED(hr)) 
          return hr;

        return S_OK;
      }
    };
    std::vector<CExternalPixelShader>  m_pPixelShaders;//CAtlList
    std::vector<CExternalPixelShader>  m_pPixelShadersScreenSpace;//CAtlList
    Com::SmartPtr<IDirect3DPixelShader9>    m_pResizerPixelShader[4]; // bl, bc1, bc2_1, bc2_2
    Com::SmartPtr<IDirect3DTexture9>    m_pScreenSizeTemporaryTexture[2];
    D3DFORMAT            m_SurfaceType;
    D3DFORMAT            m_BackbufferType;
    D3DFORMAT            m_DisplayType;
    D3DTEXTUREFILTERTYPE      m_filter;
    D3DCAPS9        m_caps;
    
    std::auto_ptr<CPixelShaderCompiler> m_pPSC;
    

    bool SettingsNeedResetDevice();

    STDMETHODIMP_(void) SetTime(REFERENCE_TIME rtNow);
    virtual HRESULT CreateDevice(CStdString &_Error);
    virtual HRESULT AllocSurfaces(D3DFORMAT Format = D3DFMT_A8R8G8B8);
    virtual void DeleteSurfaces();

    // Thread stuff
    HANDLE      m_hEvtQuit;      // Stop rendering thread event
    HANDLE      m_hVSyncThread;
    static DWORD WINAPI VSyncThreadStatic(LPVOID lpParam);
    void VSyncThread();
    void StartWorkerThreads();
    void StopWorkerThreads();

    UINT GetAdapter(IDirect3D9 *pD3D, bool GetAdapter = false);

    float m_bicubicA;
    HRESULT InitResizers(float bicubicA, bool bNeedScreenSizeTexture);

    bool GetVBlank(int &_ScanLine, int &_bInVBlank, bool _bMeasureTime);
    bool WaitForVBlankRange(int &_RasterStart, int _RasterEnd, bool _bWaitIfInside, bool _bNeedAccurate, bool _bMeasure, bool &_bTakenLock);
    bool WaitForVBlank(bool &_Waited, bool &_bTakenLock);
    int GetVBlackPos();
    void CalculateJitter(LONGLONG PerformanceCounter);
    virtual void OnVBlankFinished(bool fAll, LONGLONG PerformanceCounter){}

    HRESULT DrawRect(DWORD _Color, DWORD _Alpha, const Com::SmartRect &_Rect);
    HRESULT TextureCopy(Com::SmartPtr<IDirect3DTexture9> pTexture);
    HRESULT TextureResize(Com::SmartPtr<IDirect3DTexture9> pTexture, Vector dst[4], D3DTEXTUREFILTERTYPE filter, const Com::SmartRect &SrcRect);
    HRESULT TextureResizeBilinear(Com::SmartPtr<IDirect3DTexture9> pTexture, Vector dst[4], const Com::SmartRect &SrcRect);
    HRESULT TextureResizeBicubic1pass(Com::SmartPtr<IDirect3DTexture9> pTexture, Vector dst[4], const Com::SmartRect &SrcRect);
    HRESULT TextureResizeBicubic2pass(Com::SmartPtr<IDirect3DTexture9> pTexture, Vector dst[4], const Com::SmartRect &SrcRect);

    // Casimir666
    typedef HRESULT (WINAPI * D3DXLoadSurfaceFromMemoryPtr)(
        LPDIRECT3DSURFACE9  pDestSurface,
        CONST PALETTEENTRY*  pDestPalette,
        CONST RECT*    pDestRect,
        LPCVOID      pSrcMemory,
        D3DFORMAT    SrcFormat,
        UINT      SrcPitch,
        CONST PALETTEENTRY*  pSrcPalette,
        CONST RECT*    pSrcRect,
        DWORD      Filter,
        D3DCOLOR    ColorKey);

    typedef HRESULT (WINAPI* D3DXCreateLinePtr) (LPDIRECT3DDEVICE9   pDevice, LPD3DXLINE* ppLine);

    typedef HRESULT (WINAPI* D3DXCreateFontPtr)(
                    LPDIRECT3DDEVICE9  pDevice,  
                    int      Height,
                    UINT      Width,
                    UINT      Weight,
                    UINT      MipLevels,
                    bool      Italic,
                    DWORD      CharSet,
                    DWORD      OutputPrecision,
                    DWORD      Quality,
                    DWORD      PitchAndFamily,
                    LPCWSTR      pFaceName,
                    LPD3DXFONT*    ppFont);


    void        DrawText(const RECT &rc, const CStdString &strText, int _Priority);
    void        DrawStats();
    HRESULT        AlphaBlt(RECT* pSrc, RECT* pDst, Com::SmartPtr<IDirect3DTexture9> pTexture);

    // D3D Reset
    virtual void BeforeDeviceReset() {};
    virtual void AfterDeviceReset() {};
    virtual bool ResetDevice();

    double GetFrameTime();
    double GetFrameRate();


    int            m_nTearingPos;
    VMR9AlphaBitmap      m_VMR9AlphaBitmap;
    Com::SmartAutoVectorPtr<BYTE>  m_VMR9AlphaBitmapData;
    Com::SmartRect          m_VMR9AlphaBitmapRect;
    int            m_VMR9AlphaBitmapWidthBytes;

    D3DXLoadSurfaceFromMemoryPtr  m_pD3DXLoadSurfaceFromMemory;
    D3DXCreateLinePtr    m_pD3DXCreateLine;
    D3DXCreateFontPtr    m_pD3DXCreateFont;
    HRESULT (__stdcall *m_pD3DXCreateSprite)(LPDIRECT3DDEVICE9 pDevice, LPD3DXSPRITE * ppSprite);



    int            m_nNbDXSurface;          // Total number of DX Surfaces
    int            m_nVMR9Surfaces;          // Total number of DX Surfaces
    int            m_iVMR9Surface;
    int            m_nCurSurface;          // Surface currently displayed
    long          m_nUsedBuffer;
    bool          m_bNeedPendingResetDevice;
    bool          m_bPendingResetDevice;
    bool          m_bNeedNewDevice;

    double          m_fAvrFps;            // Estimate the real FPS
    double          m_fJitterStdDev;        // Estimate the Jitter std dev
    double          m_fJitterMean;
    double          m_fSyncOffsetStdDev;
    double          m_fSyncOffsetAvr;
    double          m_DetectedRefreshRate;

    CCritSec        m_RefreshRateLock;
    double          m_DetectedRefreshTime;
    double          m_DetectedRefreshTimePrim;
    double          m_DetectedScanlineTime;
    double          m_DetectedScanlineTimePrim;
    double          m_DetectedScanlinesPerFrame;

    double GetRefreshRate()
    {
      if (m_DetectedRefreshRate)
        return m_DetectedRefreshRate;
      return m_RefreshRate;
    }

    long GetScanLines()
    {
      if (m_DetectedRefreshRate)
        return (long) m_DetectedScanlinesPerFrame;
      return m_ScreenSize.cy;
    }

    double         m_ldDetectedRefreshRateList[100];
    double         m_ldDetectedScanlineRateList[100];
    int            m_DetectedRefreshRatePos;
    bool           m_bSyncStatsAvailable;            
    __int64        m_pllJitter [NB_JITTER];    // Jitter buffer for stats
    __int64        m_pllSyncOffset [NB_JITTER];    // Jitter buffer for stats
    __int64        m_llLastPerf;
    __int64        m_JitterStdDev;
    __int64        m_MaxJitter;
    __int64        m_MinJitter;
    __int64        m_MaxSyncOffset;
    __int64        m_MinSyncOffset;
    int            m_nNextJitter;
    int            m_nNextSyncOffset;
    __int64        m_rtTimePerFrame;
    double         m_DetectedFrameRate;
    double         m_DetectedFrameTime;
    double         m_DetectedFrameTimeStdDev;
    bool           m_DetectedLock;
    __int64        m_DetectedFrameTimeHistory[60];
    double         m_DetectedFrameTimeHistoryHistory[500];
    int            m_DetectedFrameTimePos;
    int            m_bInterlaced;

    double         m_TextScale;

    int            m_VBlankEndWait;
    int            m_VBlankStartWait;
    __int64        m_VBlankWaitTime;
    __int64        m_VBlankLockTime;
    int            m_VBlankMin;
    int            m_VBlankMinCalc;
    int            m_VBlankMax;
    int            m_VBlankEndPresent;
    __int64        m_VBlankStartMeasureTime;
    int            m_VBlankStartMeasure;

    __int64        m_PresentWaitTime;
    __int64        m_PresentWaitTimeMin;
    __int64        m_PresentWaitTimeMax;

    __int64        m_PaintTime;
    __int64        m_PaintTimeMin;
    __int64        m_PaintTimeMax;

    __int64        m_WaitForGPUTime;

    __int64        m_RasterStatusWaitTime;
    __int64        m_RasterStatusWaitTimeMin;
    __int64        m_RasterStatusWaitTimeMax;
    __int64        m_RasterStatusWaitTimeMaxCalc;

    double          m_ClockDiffCalc;
    double          m_ClockDiffPrim;
    double          m_ClockDiff;

    double          m_TimeChangeHistory[100];
    double          m_ClockChangeHistory[100];
    int            m_ClockTimeChangeHistoryPos;
    double          m_ModeratedTimeSpeed;
    double          m_ModeratedTimeSpeedPrim;
    double          m_ModeratedTimeSpeedDiff;

    bool          m_bCorrectedFrameTime;
    int           m_FrameTimeCorrection;
    __int64       m_LastFrameDuration;
    __int64       m_LastSampleTime;

    CStdString          m_strStatsMsg[10];
    
    CStdString          m_D3D9Device;

  public:
    CDX9AllocatorPresenter(HWND hWnd, HRESULT& hr, bool bIsEVR, CStdString &_Error);
    ~CDX9AllocatorPresenter();

    // ISubPicAllocatorPresenter
    STDMETHODIMP CreateRenderer(IUnknown** ppRenderer);
    STDMETHODIMP_(bool) Paint(bool fAll);
    STDMETHODIMP GetDIB(BYTE* lpDib, DWORD* size);
    
    // ID3DResource
    virtual void OnLostDevice();
    virtual void OnDestroyDevice();
    virtual void OnCreateDevice();
    virtual void OnResetDevice();

    // IPainCallback
    virtual void OnPaint(CRect destRect);
  };