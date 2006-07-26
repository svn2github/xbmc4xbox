/*
* XBoxMediaCenter
* Copyright (c) 2003 Frodo/jcmarshall
* Portions Copyright (c) by the authors of ffmpeg / xvid /mplayer
*
* This program is free software; you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation; either version 2 of the License, or
* (at your option) any later version.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with this program; if not, write to the Free Software
* Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
*/
#include "../../stdafx.h"
#include "ComboRenderer.h"
#include "../../application.h"
#include "../../util.h"


CComboRenderer::CComboRenderer(LPDIRECT3DDEVICE8 pDevice)
    : CXBoxRenderer(pDevice)
{
  m_bHasDimView = false;
  m_RGBTexture = NULL;
  m_hPixelShader = 0;
}

void CComboRenderer::DeleteYUY2Texture()
{
  CSingleLock lock(g_graphicsContext);
  if (m_RGBTexture)
    SAFE_RELEASE(m_RGBTexture);
}

void CComboRenderer::ClearYUY2Texture()
{
  D3DLOCKED_RECT lr;
  // Clear our RGB/YUY2 texture
  m_RGBTexture->LockRect(0, &lr, NULL, 0);
  fast_memset(lr.pBits, 0x00800080, lr.Pitch*m_iSourceHeight);
  m_RGBTexture->UnlockRect(0);
}

bool CComboRenderer::CreateYUY2Texture()
{
  CSingleLock lock(g_graphicsContext);
  DeleteYUY2Texture();
  // Create our textures...
  if (D3D_OK != m_pD3DDevice->CreateTexture(m_iSourceWidth / 2, m_iSourceHeight, 1, 0, D3DFMT_LIN_A8R8G8B8, 0, &m_RGBTexture))
  {
    return false;
  }

  // setup the RGB texture as both a YUY2 texture AND a RGB texture
  // (We can only render into an RGB texture, so we do the YUV->YUY2 conversion by)
  // rendering into the RGB texture which shares memory with the YUY2 texture.

  D3DLOCKED_RECT lr;
  m_RGBTexture->LockRect(0, &lr, NULL, 0);
  XGSetTextureHeader(m_iSourceWidth, m_iSourceHeight, 1, 0, D3DFMT_YUY2, 0, &m_YUY2Texture, 0, lr.Pitch);
  m_YUY2Texture.Register(lr.pBits);
  m_RGBTexture->UnlockRect(0);

  ClearYUY2Texture();
  CLog::Log(LOGDEBUG, "Created yuy2 textures");
  return true;
}

void CComboRenderer::ManageTextures()
{
  //use 1 buffer in fullscreen mode and 0 buffers in windowed mode
  if (g_graphicsContext.IsFullScreenVideo())
  {
    if (!m_RGBTexture)
    { // we need to create the YUV texture
      CreateYUY2Texture();
    }
  }
  else
  {
    if (m_RGBTexture)
    { // don't need the YUV texture in the GUI
      DeleteYUY2Texture();
    }
  }
  CXBoxRenderer::ManageTextures();
}

void CComboRenderer::ManageDisplay()
{
  const RECT& rv = g_graphicsContext.GetViewWindow();
  float fScreenWidth = (float)rv.right - rv.left;
  float fScreenHeight = (float)rv.bottom - rv.top;
  float fOffsetX1 = (float)rv.left;
  float fOffsetY1 = (float)rv.top;
  float fPixelRatio = g_stSettings.m_fPixelRatio;
  float fMaxScreenWidth = (float)g_settings.m_ResInfo[g_graphicsContext.GetVideoResolution()].iWidth;
  float fMaxScreenHeight = (float)g_settings.m_ResInfo[g_graphicsContext.GetVideoResolution()].iHeight;
  if (fOffsetX1 < 0) fOffsetX1 = 0;
  if (fOffsetY1 < 0) fOffsetY1 = 0;
  if (fScreenWidth + fOffsetX1 > fMaxScreenWidth) fScreenWidth = fMaxScreenWidth - fOffsetX1;
  if (fScreenHeight + fOffsetY1 > fMaxScreenHeight) fScreenHeight = fMaxScreenHeight - fOffsetY1;

  // Correct for HDTV_1080i -> 540p
  if (GetResolution() == HDTV_1080i)
  {
    fOffsetY1 /= 2;
    fScreenHeight /= 2;
    fPixelRatio *= 2;
  }

  ManageTextures();

  // source rect
  rs.left = g_stSettings.m_currentVideoSettings.m_CropLeft;
  rs.top = g_stSettings.m_currentVideoSettings.m_CropTop;
  rs.right = m_iSourceWidth - g_stSettings.m_currentVideoSettings.m_CropRight;
  rs.bottom = m_iSourceHeight - g_stSettings.m_currentVideoSettings.m_CropBottom;

  CalcNormalDisplayRect(fOffsetX1, fOffsetY1, fScreenWidth, fScreenHeight, GetAspectRatio() * fPixelRatio, g_stSettings.m_fZoomAmount);

  // check whether we need to alter our source rect
  if (rd.left < fOffsetX1 || rd.right > fOffsetX1 + fScreenWidth)
  {
    // wants to be wider than we allow, so fix
    float fRequiredWidth = (float)rd.right - rd.left;
    if (rs.right <= rs.left) rs.right = rs.left+1;
    float fHorizScale = fRequiredWidth / (float)(rs.right - rs.left);
    float fNewWidth = fScreenWidth / fHorizScale;
    rs.left = (rs.right - rs.left - (int)fNewWidth) / 2;
    rs.right = rs.left + (int)fNewWidth;
    rd.left = (int)fOffsetX1;
    rd.right = (int)(fOffsetX1 + fScreenWidth);
  }
  if (rd.top < fOffsetY1 || rd.bottom > fOffsetY1 + fScreenHeight)
  {
    // wants to be wider than we allow, so fix
    float fRequiredHeight = (float)rd.bottom - rd.top;
    if (rs.bottom <= rs.top) rs.bottom = rs.top+1;
    float fVertScale = fRequiredHeight / (float)(rs.bottom - rs.top);
    float fNewHeight = fScreenHeight / fVertScale;
    rs.top = (rs.bottom - rs.top - (int)fNewHeight) / 2;
    rs.bottom = rs.top + (int)fNewHeight;
    rd.top = (int)fOffsetY1;
    rd.bottom = (int)(fOffsetY1 + fScreenHeight);
  }
}

unsigned int CComboRenderer::Configure(unsigned int width, unsigned int height, unsigned int d_width, unsigned int d_height, float fps)
{
  CXBoxRenderer::Configure(width, height, d_width, d_height, fps);
  m_pD3DDevice->EnableOverlay(TRUE);
  m_bConfigured = true;
  return 0;
}

void CComboRenderer::Update(bool bPauseDrawing)
{
  CSingleLock lock(g_graphicsContext);
  m_pD3DDevice->EnableOverlay(!bPauseDrawing);
  if (bPauseDrawing) return ;
  CXBoxRenderer::Update(bPauseDrawing);
}

void CComboRenderer::YV12toYUY2()
{
  if (!m_RGBTexture) return;
  int index = m_iYV12RenderBuffer;

  ResetEvent(m_eventTexturesDone[index]);

  // Do the YV12 -> YUY2 conversion.
  // ALWAYS use buffer 0 in this case (saves 12 bits/pixel)
  m_pD3DDevice->SetTexture( 0, m_YUVTexture[index][FIELD_FULL][PLANE_Y] );
  m_pD3DDevice->SetTexture( 1, m_YUVTexture[index][FIELD_FULL][PLANE_U] );
  m_pD3DDevice->SetTexture( 2, m_YUVTexture[index][FIELD_FULL][PLANE_Y] );
  m_pD3DDevice->SetTexture( 3, m_YUVTexture[index][FIELD_FULL][PLANE_V] );

  for (int i = 0; i < 4; ++i)
  {
    m_pD3DDevice->SetTextureStageState( i, D3DTSS_ADDRESSU, D3DTADDRESS_CLAMP );
    m_pD3DDevice->SetTextureStageState( i, D3DTSS_ADDRESSV, D3DTADDRESS_CLAMP );
    m_pD3DDevice->SetTextureStageState( i, D3DTSS_MAGFILTER, D3DTEXF_POINT );
    m_pD3DDevice->SetTextureStageState( i, D3DTSS_MINFILTER, D3DTEXF_POINT );
  }
  // U and V need to use linear filtering, as they're being doubled vertically
  m_pD3DDevice->SetTextureStageState( 1, D3DTSS_MINFILTER, D3DTEXF_LINEAR );
  m_pD3DDevice->SetTextureStageState( 1, D3DTSS_MAGFILTER, D3DTEXF_LINEAR );
  m_pD3DDevice->SetTextureStageState( 3, D3DTSS_MINFILTER, D3DTEXF_LINEAR );
  m_pD3DDevice->SetTextureStageState( 3, D3DTSS_MAGFILTER, D3DTEXF_LINEAR );

  m_pD3DDevice->SetRenderState( D3DRS_ZENABLE, FALSE );
  m_pD3DDevice->SetRenderState( D3DRS_FOGENABLE, FALSE );
  m_pD3DDevice->SetRenderState( D3DRS_FOGTABLEMODE, D3DFOG_NONE );
  m_pD3DDevice->SetRenderState( D3DRS_FILLMODE, D3DFILL_SOLID );
  m_pD3DDevice->SetRenderState( D3DRS_CULLMODE, D3DCULL_CCW );
  m_pD3DDevice->SetRenderState( D3DRS_ALPHABLENDENABLE, FALSE );
  m_pD3DDevice->SetRenderState( D3DRS_SRCBLEND, D3DBLEND_ONE );
  m_pD3DDevice->SetRenderState( D3DRS_DESTBLEND, D3DBLEND_ZERO );
  m_pD3DDevice->SetRenderState( D3DRS_YUVENABLE, FALSE );
  m_pD3DDevice->SetVertexShader( FVF_YUYVVERTEX );
  m_pD3DDevice->SetPixelShader( m_hPixelShader );
  // Render the image
  LPDIRECT3DSURFACE8 pOldRT, pNewRT;
  m_RGBTexture->GetSurfaceLevel(0, &pNewRT);
  m_pD3DDevice->GetRenderTarget(&pOldRT);
  m_pD3DDevice->SetRenderTarget(pNewRT, NULL);

  m_pD3DDevice->Begin(D3DPT_QUADLIST);
  m_pD3DDevice->SetVertexData2f( D3DVSDE_TEXCOORD0, (float)1.5f, (float)0.5f );
  m_pD3DDevice->SetVertexData2f( D3DVSDE_TEXCOORD1, (float)0.5f, (float)0.5f );
  m_pD3DDevice->SetVertexData2f( D3DVSDE_TEXCOORD2, (float)0.5f, (float)0.5f );
  m_pD3DDevice->SetVertexData2f( D3DVSDE_TEXCOORD3, (float)0.5f, (float)0.5f);
  m_pD3DDevice->SetVertexData4f( D3DVSDE_VERTEX, (float)0.0f, (float)0.0f, 0, 1.0f );

  m_pD3DDevice->SetVertexData2f( D3DVSDE_TEXCOORD0, (float)m_iSourceWidth + 1.5f, (float)0.5f );
  m_pD3DDevice->SetVertexData2f( D3DVSDE_TEXCOORD1, (float)m_iSourceWidth / 2.0f + 0.5f, (float)0.5f );
  m_pD3DDevice->SetVertexData2f( D3DVSDE_TEXCOORD2, (float)m_iSourceWidth + 0.5f, (float)0.5f );
  m_pD3DDevice->SetVertexData2f( D3DVSDE_TEXCOORD3, (float)m_iSourceWidth / 2.0f + 0.5f, (float)0.5f );
  m_pD3DDevice->SetVertexData4f( D3DVSDE_VERTEX, (float)m_iSourceWidth / 2.0f, (float)0.0f, 0, 1.0f );

  m_pD3DDevice->SetVertexData2f( D3DVSDE_TEXCOORD0, (float)m_iSourceWidth + 1.5f, (float)m_iSourceHeight + 0.5f );
  m_pD3DDevice->SetVertexData2f( D3DVSDE_TEXCOORD1, (float)m_iSourceWidth / 2.0f + 0.5f, (float)m_iSourceHeight / 2.0f + 0.5f );
  m_pD3DDevice->SetVertexData2f( D3DVSDE_TEXCOORD2, (float)m_iSourceWidth + 0.5f, (float)m_iSourceHeight + 0.5f);
  m_pD3DDevice->SetVertexData2f( D3DVSDE_TEXCOORD3, (float)m_iSourceWidth / 2.0f + 0.5f, (float)m_iSourceHeight / 2.0f + 0.5f );
  m_pD3DDevice->SetVertexData4f( D3DVSDE_VERTEX, (float)m_iSourceWidth / 2.0f, (float)m_iSourceHeight, 0, 1.0f );

  m_pD3DDevice->SetVertexData2f( D3DVSDE_TEXCOORD0, (float)1.5f, (float)m_iSourceHeight + 0.5f );
  m_pD3DDevice->SetVertexData2f( D3DVSDE_TEXCOORD1, (float)0.5f, (float)m_iSourceHeight / 2.0f + 0.5f );
  m_pD3DDevice->SetVertexData2f( D3DVSDE_TEXCOORD2, (float)0.5f, (float)m_iSourceHeight + 0.5f );
  m_pD3DDevice->SetVertexData2f( D3DVSDE_TEXCOORD3, (float)0.5f, (float)m_iSourceHeight / 2.0f + 0.5f );
  m_pD3DDevice->SetVertexData4f( D3DVSDE_VERTEX, (float)0.0f, (float)m_iSourceHeight, 0, 1.0f );
  m_pD3DDevice->End();

  m_pD3DDevice->SetTexture(0, NULL);
  m_pD3DDevice->SetTexture(1, NULL);
  m_pD3DDevice->SetTexture(2, NULL);
  m_pD3DDevice->SetTexture(3, NULL);

  m_pD3DDevice->SetRenderState( D3DRS_YUVENABLE, FALSE );
  m_pD3DDevice->SetPixelShader( NULL );
  m_pD3DDevice->SetRenderTarget(pOldRT, NULL);

  m_pD3DDevice->SetTextureStageState( 0, D3DTSS_MAGFILTER, D3DTEXF_LINEAR );
  m_pD3DDevice->SetTextureStageState( 0, D3DTSS_MINFILTER, D3DTEXF_LINEAR );
  m_pD3DDevice->SetTextureStageState( 2, D3DTSS_MAGFILTER, D3DTEXF_LINEAR );
  m_pD3DDevice->SetTextureStageState( 2, D3DTSS_MINFILTER, D3DTEXF_LINEAR );

  pOldRT->Release();
  pNewRT->Release();

  m_pD3DDevice->KickPushBuffer();

  //Okey, when the gpu is done with the textures here, they are free to be modified again
  m_pD3DDevice->InsertCallback(D3DCALLBACK_WRITE,&TextureCallback, (DWORD)m_eventTexturesDone[index]);

  m_bHasDimView = false;
}

void CComboRenderer::Render(DWORD flags)
{
  if ( m_RGBTexture == NULL )
  {
    RenderLowMem(flags);
  }
  else
  {
    YV12toYUY2();

    // Don't render if we are waiting an overlay event
    while (!m_pD3DDevice->GetOverlayUpdateStatus()) Sleep(1);

    /* clear target area, otherwise we won't get any picture */
    D3DRECT target;
    target.x1 = rd.left;
    target.x2 = rd.right;
    target.y1 = rd.top;
    target.y2 = rd.bottom;
    m_pD3DDevice->Clear( 1L, &target, D3DCLEAR_TARGET, m_clearColour, 1.0f, 0L );

    LPDIRECT3DSURFACE8 pSurface;
    m_YUY2Texture.GetSurfaceLevel(0, &pSurface);
    m_pD3DDevice->UpdateOverlay( pSurface, &rs, &rd, TRUE, m_clearColour );
    pSurface->Release();
  }

  CXBoxRenderer::Render(flags | RENDER_FLAG_NOOSDALPHA);
}

unsigned int CComboRenderer::PreInit()
{
  CXBoxRenderer::PreInit();
  // May have to set clearColour non-zero in future for HW overlays method?
//  if (!m_clearColour)
//    m_clearColour = 0x010001;

  m_bHasDimView = false;
  // Create the pixel shader
  if (!m_hPixelShader)
  {
    // shader to interleave separate Y U and V planes into a single YUY2 output
    const char* shader =
      "xps.1.1\n"
      "def c0,1,0,0,0\n"
      "def c1,0,1,0,0\n"
      "def c2,0,0,1,0\n"
      "def c3,0,0,0,1\n"
      "tex t0\n" // Y1 plane (Y plane)
      "tex t1\n" // U plane
      "tex t2\n" // Y2 plane (Y plane shifted 1 pixel to the left)
      "tex t3\n" // V plane
      "xmma discard,discard,r0, t0, c0, t1, c1\n"
      "xmma discard,discard,r1, t2, c2, t3.b, c3\n"
      "add r0, r0, r1\n";
    XGBuffer* pShader;
    XGAssembleShader("XBMCShader", shader, strlen(shader), 0, NULL, &pShader, NULL, NULL, NULL, NULL, NULL);
    m_pD3DDevice->CreatePixelShader((D3DPIXELSHADERDEF*)pShader->GetBufferPointer(), &m_hPixelShader);
    pShader->Release();
  }
  return 0;
}

void CComboRenderer::UnInit()
{
  CSingleLock lock(g_graphicsContext);

  m_pD3DDevice->EnableOverlay(FALSE);
  DeleteYUY2Texture();

  if (m_hPixelShader)
  {
    m_pD3DDevice->DeletePixelShader(m_hPixelShader);
    m_hPixelShader = 0;
  }

  CXBoxRenderer::UnInit();
}

void CComboRenderer::CheckScreenSaver()
{
  // Called from CMPlayer::Process() (mplayer.cpp) when in 'pause' mode
  D3DLOCKED_RECT lr;

  float fAmount = (float)g_guiSettings.GetInt("screensaver.dimlevel") / 100.0f;

  if (g_application.m_bScreenSave && !m_bHasDimView)
  {
    if ( D3D_OK == m_YUY2Texture.LockRect(0, &lr, NULL, 0 ))
    {
      // Drop brightness of current surface to 20%
      DWORD strideScreen = lr.Pitch;
      for (DWORD y = 0; y < UINT (rs.top + rs.bottom); y++)
      {
        BYTE *pDest = (BYTE*)lr.pBits + strideScreen * y;
        for (DWORD x = 0; x < UINT ((rs.left + rs.right) >> 1); x++)
        {
          pDest[0] = BYTE (pDest[0] * fAmount); // Y1
          pDest[1] = BYTE ((pDest[1] - 128) * fAmount + 128); // U (with 128 shift!)
          pDest[2] = BYTE (pDest[2] * fAmount); // Y2
          pDest[3] = BYTE ((pDest[3] - 128) * fAmount + 128); // V (with 128 shift!)
          pDest += 4;
        }
      }
      m_YUY2Texture.UnlockRect(0);

      // Commit to screen
      CSingleLock lock(g_graphicsContext);

      while (!m_pD3DDevice->GetOverlayUpdateStatus()) Sleep(1);
      LPDIRECT3DSURFACE8 pSurface;
      m_YUY2Texture.GetSurfaceLevel(0, &pSurface);
      m_pD3DDevice->UpdateOverlay( pSurface, &rs, &rd, TRUE, m_clearColour );
      pSurface->Release();
    }

    m_bHasDimView = true;
  }
}

void CComboRenderer::SetupScreenshot()
{
  if (!g_graphicsContext.IsFullScreenVideo())
    return;
  CSingleLock lock(g_graphicsContext);
  // first, grab the current overlay texture and convert it to RGB
  LPDIRECT3DTEXTURE8 pRGB = NULL;
  if (D3D_OK != m_pD3DDevice->CreateTexture(m_iSourceWidth, m_iSourceHeight, 1, 0, D3DFMT_LIN_A8R8G8B8, 0, &pRGB))
  {
    return ;
  }
  D3DLOCKED_RECT lr, lr2;
  m_YUY2Texture.LockRect(0, &lr, NULL, 0);
  pRGB->LockRect(0, &lr2, NULL, 0);
  // convert to RGB via software converter
  BYTE *s = (BYTE *)lr.pBits;
  LONG *d = (LONG *)lr2.pBits;
  LONG dpitch = lr2.Pitch / 4;
  for (unsigned int y = 0; y < m_iSourceHeight; y++)
  {
    for (unsigned int x = 0; x < m_iSourceWidth; x += 2)
    {
      d[x] = YUV2RGB(s[2 * x], s[2 * x + 1], s[2 * x + 3]);
      d[x + 1] = YUV2RGB(s[2 * x + 2], s[2 * x + 1], s[2 * x + 3]);
    }
    s += lr.Pitch;
    d += dpitch;
  }
  m_YUY2Texture.UnlockRect(0);
  pRGB->UnlockRect(0);
  // ok - now lets dump the RGB texture to a file to test
  // ok, now this needs to be rendered to the screen
  m_pD3DDevice->SetTexture( 0, pRGB);

  m_pD3DDevice->SetTextureStageState( 0, D3DTSS_ADDRESSU, D3DTADDRESS_CLAMP );
  m_pD3DDevice->SetTextureStageState( 0, D3DTSS_ADDRESSV, D3DTADDRESS_CLAMP );
  m_pD3DDevice->SetTextureStageState( 0, D3DTSS_MAGFILTER, D3DTEXF_LINEAR );
  m_pD3DDevice->SetTextureStageState( 0, D3DTSS_MINFILTER, D3DTEXF_LINEAR );

  // set scissors if we are not in fullscreen video
  if ( !(g_graphicsContext.IsFullScreenVideo() || g_graphicsContext.IsCalibrating() ))
  {
    g_graphicsContext.ClipToViewWindow();
  }

  m_pD3DDevice->SetRenderState( D3DRS_ZENABLE, FALSE );
  m_pD3DDevice->SetRenderState( D3DRS_FOGENABLE, FALSE );
  m_pD3DDevice->SetRenderState( D3DRS_FOGTABLEMODE, D3DFOG_NONE );
  m_pD3DDevice->SetRenderState( D3DRS_FILLMODE, D3DFILL_SOLID );
  m_pD3DDevice->SetRenderState( D3DRS_CULLMODE, D3DCULL_CCW );
  m_pD3DDevice->SetRenderState( D3DRS_ALPHABLENDENABLE, FALSE );
  m_pD3DDevice->SetRenderState( D3DRS_SRCBLEND, D3DBLEND_ONE );
  m_pD3DDevice->SetRenderState( D3DRS_DESTBLEND, D3DBLEND_ZERO );
  m_pD3DDevice->SetRenderState( D3DRS_YUVENABLE, FALSE /*TRUE*/ );
  m_pD3DDevice->SetVertexShader( FVF_RGBVERTEX );
  // Render the image
  m_pD3DDevice->SetScreenSpaceOffset( -0.5f, -0.5f); // fix texel align
  m_pD3DDevice->Begin(D3DPT_QUADLIST);
  m_pD3DDevice->SetVertexData2f( D3DVSDE_TEXCOORD0, (float)rs.left, (float)rs.top );
  m_pD3DDevice->SetVertexData4f( D3DVSDE_VERTEX, (float)rd.left, (float)rd.top, 0, 1.0f );

  m_pD3DDevice->SetVertexData2f( D3DVSDE_TEXCOORD0, (float)rs.right, (float)rs.top );
  m_pD3DDevice->SetVertexData4f( D3DVSDE_VERTEX, (float)rd.right, (float)rd.top, 0, 1.0f );

  m_pD3DDevice->SetVertexData2f( D3DVSDE_TEXCOORD0, (float)rs.right, (float)rs.bottom );
  m_pD3DDevice->SetVertexData4f( D3DVSDE_VERTEX, (float)rd.right, (float)rd.bottom, 0, 1.0f );

  m_pD3DDevice->SetVertexData2f( D3DVSDE_TEXCOORD0, (float)rs.left, (float)rs.bottom );
  m_pD3DDevice->SetVertexData4f( D3DVSDE_VERTEX, (float)rd.left, (float)rd.bottom, 0, 1.0f );
  m_pD3DDevice->End();
  m_pD3DDevice->SetScreenSpaceOffset(0, 0);

  m_pD3DDevice->SetTexture(0, NULL);
  m_pD3DDevice->SetScissors(0, FALSE, NULL );

  RenderOSD();

  if (g_application.NeedRenderFullScreen())
  { // render our subtitles and osd
    g_application.RenderFullScreen();
  }

  m_pD3DDevice->Present( NULL, NULL, NULL, NULL );

  while (pRGB->IsBusy()) Sleep(1);
  pRGB->Release();

  return ;
}

#define Y_SCALE 1.164383561643835616438356164383f
#define UV_SCALE 1.1434977578475336322869955156951f

#define R_Vp 1.403f
#define G_Up -0.344f
#define G_Vp -0.714f
#define B_Up 1.770f

LONG CComboRenderer::YUV2RGB(BYTE y, BYTE u, BYTE v)
{
  // normalize
  float Yp = (y - 16) * Y_SCALE;
  float Up = (u - 128) * UV_SCALE;
  float Vp = (v - 128) * UV_SCALE;

  float R = Yp + R_Vp * Vp;
  float G = Yp + G_Up * Up + G_Vp * Vp;
  float B = Yp + B_Up * Up;

  // clamp
  if (R < 0) R = 0;
  if (R > 255) R = 255;
  if (G < 0) G = 0;
  if (G > 255) G = 255;
  if (B < 0) B = 0;
  if (B > 255) B = 255;

  return ((int)R << 16) + ((int)G << 8) + (int)B;
}
