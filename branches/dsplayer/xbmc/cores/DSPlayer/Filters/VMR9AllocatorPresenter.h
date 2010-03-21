/*
 *      Copyright (C) 2005-2010 Team XBMC
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

#ifndef _DXALLOCATORPRESENTER_H
#define _DXALLOCATORPRESNETER_H
#pragma once

#include <vmr9.h>
#include <wxdebug.h>
#include <Wxutil.h>

#pragma warning(push, 2)

#pragma warning(disable : 4995)

#include <vector>
#pragma warning(pop)


#include <d3d9.h>

#include "DsRenderer.h"
#include "geometry.h"

class CMacrovisionKicker;
class COuterVMR9;

[uuid("A2636B41-5E3C-4426-B6BC-CD8616600912")]
class CVMR9AllocatorPresenter  : public CDsRenderer,
                                        IVMRSurfaceAllocator9,
                                        IVMRImagePresenter9
                    
{
public:
  CVMR9AllocatorPresenter(HRESULT& hr, CStdString &_Error);
  ~CVMR9AllocatorPresenter();

  DECLARE_IUNKNOWN
  STDMETHODIMP NonDelegatingQueryInterface(REFIID riid, void** ppv);

  // IVMRSurfaceAllocator9
  virtual STDMETHODIMP InitializeDevice(DWORD_PTR dwUserID, VMR9AllocationInfo *lpAllocInfo, DWORD *lpNumBuffers);
  virtual STDMETHODIMP TerminateDevice(DWORD_PTR dwID);  
  virtual STDMETHODIMP GetSurface(DWORD_PTR dwUserID, DWORD SurfaceIndex, DWORD SurfaceFlags, IDirect3DSurface9 **lplpSurface);
  virtual STDMETHODIMP AdviseNotify(IVMRSurfaceAllocatorNotify9 *lpIVMRSurfAllocNotify);

  // IVMRImagePresenter9
  virtual STDMETHODIMP StartPresenting( DWORD_PTR dwUserID);
  virtual STDMETHODIMP StopPresenting( DWORD_PTR dwUserID);
  virtual STDMETHODIMP PresentImage( DWORD_PTR dwUserID, VMR9PresentationInfo *lpPresInfo);

  // IDSRenderer
  STDMETHODIMP CreateRenderer(IUnknown** ppRenderer);

  virtual void OnLostDevice();
  virtual void OnDestroyDevice();
  virtual void OnCreateDevice();
  virtual void OnResetDevice();
protected:
  void DeleteSurfaces();
  void GetCurrentVideoSize();
  HRESULT ChangeD3dDev();

  D3DFORMAT m_DisplayType;
  
  bool m_fUseInternalTimer;
  //Clock stuff
  double         m_rtTimePerFrame;
  float          m_fps;
  int            m_fFrameRate;
  double         m_FlipTimeStamp; // time stamp of last flippage. used to play at a forced framerate
  double         m_iCurrentPts; // last pts displayed

  bool           m_bRenderCreated;
  bool           m_bNeedNewDevice;
  volatile bool m_bVmrStop;
private:
  //long        m_refCount;
  IVMRSurfaceAllocatorNotify9*        m_pIVMRSurfAllocNotify;
  std::vector<IDirect3DSurface9*>     m_pSurfaces;
  int                                 m_pNbrSurface;
  int                                 m_pCurSurface;
  bool          m_bNeedCheckSample;
  
};

#endif // _DXALLOCATORPRESENTER_H
