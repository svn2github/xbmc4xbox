#include "stdafx.h"
#include "emu_dx8.h"

extern "C" void OutputDebug(char* strDebug)
{
  OutputDebugString(strDebug);
}
extern "C" void d3dSetTextureStageState( int x, DWORD dwY, DWORD dwZ)
{
#ifndef HAS_SDL
  g_graphicsContext.Get3DDevice()->SetTextureStageState(x, (D3DTEXTURESTAGESTATETYPE)dwY, dwZ);
#endif
}

extern "C" void d3dSetRenderState(DWORD dwY, DWORD dwZ)
{
#ifndef HAS_SDL
  g_graphicsContext.Get3DDevice()->SetRenderState((D3DRENDERSTATETYPE)dwY, dwZ);
#endif
}
extern "C" void d3dGetRenderState(DWORD dwY, DWORD* dwZ)
{
#ifndef HAS_SDL
  g_graphicsContext.Get3DDevice()->GetRenderState((D3DRENDERSTATETYPE)dwY, dwZ);
#endif
}
extern "C" void d3dSetTransform( DWORD dwY, D3DMATRIX* dwZ )
{
#ifndef HAS_SDL
  g_graphicsContext.Get3DDevice()->SetTransform( (D3DTRANSFORMSTATETYPE)dwY, dwZ );
#endif
}

extern "C" bool d3dCreateTexture(unsigned int width, unsigned int height, LPDIRECT3DTEXTURE8 *pTexture)
{
#ifndef HAS_SDL
  return (D3D_OK == g_graphicsContext.Get3DDevice()->CreateTexture(width, height, 1, 0, D3DFMT_LIN_A8R8G8B8 , D3DPOOL_MANAGED, pTexture));
#else
  return false;
#endif
}

extern "C" void d3dDrawIndexedPrimitive(D3DPRIMITIVETYPE primType, unsigned int minIndex, unsigned int numVertices, unsigned int startIndex, unsigned int primCount)
{
#ifndef HAS_SDL
  g_graphicsContext.Get3DDevice()->DrawIndexedPrimitive(primType, minIndex, numVertices, startIndex, primCount);
#endif
}
