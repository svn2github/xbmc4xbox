/*
 *      Copyright (C) 2005-2008 Team XBMC
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
 
#include "emu_dummy.h"
#include "emu_ole32.h"
#include "utils/log.h"

static const CLSID CLSID_MemoryAllocator =  {0x1e651cc0, 0xb199, 0x11d0, {0x82, 0x12, 0x00, 0xc0, 0x4f, 0xc3, 0x2c, 0x45}};

extern "C" HRESULT WINAPI dllCoInitialize()
{
  return S_OK;
}

extern "C" void WINAPI dllCoUninitialize()
{}

extern "C" HRESULT WINAPI dllCoCreateInstance( REFCLSID rclsid, LPUNKNOWN pUnkOuter, DWORD dwClsContext, REFIID riid, LPVOID * ppv)
{
  if( IsEqualCLSID(rclsid, CLSID_MemoryAllocator) )
  {
    CLog::Log(LOGDEBUG, __FUNCTION__" - CLSID_MemoryAllocator");
    IUnknown *obj = NULL;
    if( obj == NULL )
    {
      CLog::Log(LOGDEBUG, __FUNCTION__" - MPlayer MemoryAllocator not available");
      return REGDB_E_CLASSNOTREG;
    }
    
    HRESULT result = obj->QueryInterface(riid, ppv);
    obj->Release();
    return result;
  }
  not_implement("ol32.dll fake function CoCreateInstance called\n"); //warning
  return REGDB_E_CLASSNOTREG;
}

extern "C" void WINAPI dllCoFreeUnusedLibraries()
{}
extern "C" int WINAPI dllStringFromGUID2(REFGUID rguid, LPOLESTR lpsz, int cchMax)
{
  not_implement("ol32.dll fake function StringFromGUID2 called\n"); //warning
  return 0;
}

extern "C" void WINAPI dllCoTaskMemFree(void * cb)
{
  if ( cb )
    free(cb);
  cb = 0;
}

extern "C" void * WINAPI dllCoTaskMemAlloc(unsigned long cb)
{
  return malloc(cb);
}
