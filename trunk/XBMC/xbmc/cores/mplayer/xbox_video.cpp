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
#include "stdafx.h"
#include "xbox_video.h"
#include "video.h"
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>

#include "video.h"
#include "mplayer.h"
#include "GraphicContext.h"
#include "../../settings.h"
#include "../../application.h"
#include "../../util.h"
#include "../../utils/log.h"

#define SUBTITLE_TEXTURE_WIDTH  720
#define SUBTITLE_TEXTURE_HEIGHT 120

#define NUM_BUFFERS (3)

static RECT                    rd;                             //rect of our stretched image
static RECT                    rs;                             //rect of our source image
static unsigned int            image_width, image_height;      //image width and height
static unsigned int            d_image_width, d_image_height;  //image width and height zoomed
static unsigned int            image_format=0;                 //image format
static unsigned int            primary_image_format;
static float                   fSourceFrameRatio=0;            //frame aspect ratio of video
static unsigned int            fs = 0;                         //display in window or fullscreen
static bool                    m_bPauseDrawing=false;          // whether we have paused drawing or not
static int                     iClearSubtitleRegion[2]={0};  // amount of subtitle region to clear
static LPDIRECT3DTEXTURE8      m_pSubtitleTexture[2]={NULL};
static unsigned char*          subtitleimage=NULL;             //image data
static unsigned int            subtitlestride;                 //surface stride
static LPDIRECT3DVERTEXBUFFER8 m_pSubtitleVB;                  // vertex buffer for subtitles
static int                     m_iBackBuffer=0;								// subtitles only
static int                     m_bFlip=0;
static bool                    m_bRenderGUI=false;
static RESOLUTION              m_iResolution=PAL_4x3;
static bool                    m_bFlipped;
static float                   m_fps;
static __int64                 m_lFrameCounter;

// Video size stuff
static float                    m_fOffsetX1;
static float                    m_fOffsetY1;
static float                    m_iScreenWidth;
static float                    m_iScreenHeight;
static bool                     m_bStretch;
static bool                     m_bZoom;

// YV12 decoder textures
static LPDIRECT3DVERTEXBUFFER8  m_pVideoVB;
static D3DTexture               m_YTexture[NUM_BUFFERS];
static D3DTexture               m_UTexture[NUM_BUFFERS];
static D3DTexture               m_VTexture[NUM_BUFFERS];
static void*                    m_TextureBuffer[NUM_BUFFERS];
static DWORD                    m_hPixelShader = 0;
static int                      ytexture_pitch;
static int                      uvtexture_pitch;
static int											ytexture_height; // aligned heights
static int											uvtexture_height;
static int                      m_iRenderBuffer;
static int                      m_iDecodeBuffer;
typedef struct directx_fourcc_caps
{
	char*             img_format_name;      //human readable name
	unsigned int      img_format;           //as MPlayer image format
	unsigned int      drv_caps;             //what hw supports with this format
} directx_fourcc_caps;

// we just support 1 format which is YV12
// TODO: add ARGB support

// 20-2-2004. 
// Removed the VFCAP_TIMER. (VFCAP_TIMER means that the video output driver handles all timing which we dont)
// this solves problems playing movies with no audio. With VFCAP_TIMER enabled they played much 2 fast
#define DIRECT3D8CAPS VFCAP_CSP_SUPPORTED_BY_HW |VFCAP_CSP_SUPPORTED |VFCAP_OSD |VFCAP_HWSCALE_UP|VFCAP_HWSCALE_DOWN//|VFCAP_TIMER
static directx_fourcc_caps g_ddpf[] =
{
	{"YV12 ",IMGFMT_YV12 ,DIRECT3D8CAPS},
};

struct VIDVERTEX 
{ 
	D3DXVECTOR4 p;
	FLOAT tu0, tv0;
	FLOAT tu1, tv1;
	FLOAT tu2, tv2;
};
static const DWORD FVF_VIDVERTEX = D3DFVF_XYZRHW|D3DFVF_TEX3;

struct SUBVERTEX 
{ 
	D3DXVECTOR4 p;
	FLOAT tu, tv;
};
static const DWORD FVF_SUBVERTEX = D3DFVF_XYZRHW|D3DFVF_TEX1;

#define NUM_FORMATS (sizeof(g_ddpf) / sizeof(g_ddpf[0]))

static void video_flip_page(void);

//********************************************************************************************************
void choose_best_resolution(float fps)
{
	DWORD dwStandard = XGetVideoStandard();
	DWORD dwFlags    = XGetVideoFlags();

	bool bUsingPAL        = (dwStandard==XC_VIDEO_STANDARD_PAL_I);    // current video standard:PAL or NTSC 
	bool bCanDoWidescreen = (dwFlags & XC_VIDEO_FLAGS_WIDESCREEN)!=0; // can widescreen be enabled?

	// check if widescreen is used by the GUI
	int iGUIResolution=g_stSettings.m_GUIResolution;
	if (  (g_settings.m_ResInfo[iGUIResolution].dwFlags&D3DPRESENTFLAG_WIDESCREEN)==0)
	{
		// NO, then if 'Choose Best Resolution' option is disabled
		// we dont wanna switch between 4:3 / 16:9 depending on the movie
		if (!g_stSettings.m_bAllowVideoSwitching)
		{
			bCanDoWidescreen =false;
		}
	}

	// Work out if the framerate suits PAL50 or PAL60
	bool bPal60=false;
	if (bUsingPAL && g_stSettings.m_bAllowPAL60 && (dwFlags&XC_VIDEO_FLAGS_PAL_60Hz))
	{
		// yes we're in PAL
		// yes PAL60 is allowed
		// yes dashboard PAL60 settings is enabled
		// Calculate the framerate difference from a divisor of 120fps and 100fps
		// (twice 60fps and 50fps to allow for 2:3 IVTC pulldown)
		float fFrameDifference60 = abs(120.0f/fps-floor(120.0f/fps+0.5f));
		float fFrameDifference50 = abs(100.0f/fps-floor(100.0f/fps+0.5f));
		// Make a decision based on the framerate difference
		if (fFrameDifference60 < fFrameDifference50)
			bPal60=true;
	}

	// Work out if framesize suits 4:3 or 16:9
	// Uses the frame aspect ratio of 8/(3*sqrt(3)) (=1.53960) which is the optimal point
	// where the percentage of black bars to screen area in 4:3 and 16:9 is equal
	bool bWidescreen = false;
	if (bCanDoWidescreen && ((float)d_image_width/d_image_height > 8.0f/(3.0f*sqrt(3.0f))))
	{
		bWidescreen = true;
	}

	// if video switching is not allowed then use current resolution (with pal 60 if needed)
	// if we're not in fullscreen mode then use current resolution 
	// if we're calibrating the video  then use current resolution 
	if  ( (!g_stSettings.m_bAllowVideoSwitching) ||
		(! ( g_graphicsContext.IsFullScreenVideo()|| g_graphicsContext.IsCalibrating())  )
		)
	{
		m_iResolution = g_graphicsContext.GetVideoResolution();
		// Check to see if we are using a PAL screen capable of PAL60
		if (bUsingPAL)
		{
			// FIXME - Fix for autochange of widescreen once GUI option is implemented
			bWidescreen = (g_settings.m_ResInfo[m_iResolution].dwFlags&D3DPRESENTFLAG_WIDESCREEN)!=0;
			if (bPal60)
			{
				if (bWidescreen)
					m_iResolution = PAL60_16x9;
				else
					m_iResolution = PAL60_4x3;
			}
			else
			{
				if (bWidescreen)
					m_iResolution = PAL_16x9;
				else
					m_iResolution = PAL_4x3;
			}
		}
		// Change our screen resolution
    Sleep(1000);
		g_graphicsContext.SetVideoResolution(m_iResolution);
		return;
	}

	// We are allowed to switch video resolutions, so we must
	// now decide which is the best resolution for the video we have
	if (bUsingPAL)  // PAL resolutions
	{
		// Currently does not allow HDTV solutions, as it is my beleif
		// that the XBox hardware only allows HDTV resolutions for NTSC systems.
		// this may need revising as more knowledge is obtained.
		if (bPal60)
		{
			if (bWidescreen)
				m_iResolution = PAL60_16x9;
			else
				m_iResolution = PAL60_4x3;
		}
		else    // PAL50
		{
			if (bWidescreen)
				m_iResolution = PAL_16x9;
			else
				m_iResolution = PAL_4x3;
		}
	}
	else      // NTSC resolutions
	{
		// Check if the picture warrants HDTV mode
		// And if HDTV modes (1080i and 720p) are available
		if ((image_height>540) && (dwFlags&XC_VIDEO_FLAGS_HDTV_720p))
		{ //image suits 720p if it's available
			m_iResolution = HDTV_720p;
		}
		else if ((image_height>480 || image_width>720) && (dwFlags&XC_VIDEO_FLAGS_HDTV_1080i))                  //1080i is available
		{ // image suits 1080i (540p) if it is available
			m_iResolution = HDTV_1080i;
		}
		else if ((image_height>480 || image_width>720) && (dwFlags&XC_VIDEO_FLAGS_HDTV_720p))
		{ // image suits 1080i or 720p and obviously 1080i is unavailable
			m_iResolution = HDTV_720p;
		}
		else  // either picture does not warrant HDTV, or HDTV modes are unavailable
		{
			if (dwFlags&XC_VIDEO_FLAGS_HDTV_480p)
			{
				if (bWidescreen)
					m_iResolution = HDTV_480p_16x9;
				else
					m_iResolution = HDTV_480p_4x3;
			}
			else
			{
				if (bWidescreen)
					m_iResolution = NTSC_16x9;
				else
					m_iResolution = NTSC_4x3;
			}
		}
	}

	// Finished - update our video resolution
  Sleep(1000);
	g_graphicsContext.SetVideoResolution(m_iResolution);
}

//**********************************************************************************************
// ClearSubtitleRegion()
//
// Clears our subtitle texture
//**********************************************************************************************
static void ClearSubtitleRegion(int iTexture)
{
	//  OutputDebugString("Clearing subs\n");
	for (int y=SUBTITLE_TEXTURE_HEIGHT-iClearSubtitleRegion[iTexture]; y < SUBTITLE_TEXTURE_HEIGHT; y++)
	{
		for (int x=0; x < (int)(subtitlestride); x+=2)
		{
			*(subtitleimage + subtitlestride*y+x   ) = 0x00;  // for black Y=0x10  U=0x80 V=0x80
			*(subtitleimage + subtitlestride*y+x+1 ) = 0x01;
		}
	}
	iClearSubtitleRegion[iTexture] = 0;
}

//********************************************************************************************************
void xbox_video_update_subtitle_position()
{
	if (!m_pSubtitleVB)
		return;
	SUBVERTEX* vertex=NULL;
	m_pSubtitleVB->Lock( 0, 0, (BYTE**)&vertex, 0L );
	float fSubtitleHeight = (float)g_settings.m_ResInfo[m_iResolution].iWidth/SUBTITLE_TEXTURE_WIDTH*SUBTITLE_TEXTURE_HEIGHT;
	if (g_stSettings.m_bEnlargeSubtitles)
		fSubtitleHeight *= 2;
	float fSubtitlePosition = g_settings.m_ResInfo[m_iResolution].iSubtitles - fSubtitleHeight;
	if (g_stSettings.m_bEnlargeSubtitles)
	{
		vertex[0].p = D3DXVECTOR4(-(float)g_settings.m_ResInfo[m_iResolution].iWidth / 2, fSubtitlePosition, 0, 0);
		vertex[1].p = D3DXVECTOR4((float)g_settings.m_ResInfo[m_iResolution].iWidth * 3 / 2, fSubtitlePosition, 0, 0);
		vertex[2].p = D3DXVECTOR4((float)g_settings.m_ResInfo[m_iResolution].iWidth * 3 / 2, fSubtitlePosition + fSubtitleHeight, 0, 0);
		vertex[3].p = D3DXVECTOR4(-(float)g_settings.m_ResInfo[m_iResolution].iWidth / 2, fSubtitlePosition + fSubtitleHeight, 0, 0);
	}
	else
	{
		vertex[0].p = D3DXVECTOR4(0, fSubtitlePosition, 0, 0);
		vertex[1].p = D3DXVECTOR4((float)g_settings.m_ResInfo[m_iResolution].iWidth, fSubtitlePosition, 0, 0);
		vertex[2].p = D3DXVECTOR4((float)g_settings.m_ResInfo[m_iResolution].iWidth, fSubtitlePosition + fSubtitleHeight, 0, 0);
		vertex[3].p = D3DXVECTOR4(0, fSubtitlePosition + fSubtitleHeight, 0, 0);
	}

	vertex[0].tu = 0;
	vertex[0].tv = 0;
	vertex[1].tu = SUBTITLE_TEXTURE_WIDTH;
	vertex[1].tv = 0;
	vertex[2].tu = SUBTITLE_TEXTURE_WIDTH;
	vertex[2].tv = SUBTITLE_TEXTURE_HEIGHT;
	vertex[3].tu = 0;
	vertex[3].tv = SUBTITLE_TEXTURE_HEIGHT;

	m_pSubtitleVB->Unlock();  
}

//********************************************************************************************************
static void Directx_CreateOverlay(unsigned int uiFormat)
{
	//  /*
	//      D3DFMT_YUY2 format (PC98 compliance). 
	//      Two pixels are stored in each YUY2 word. 
	//      Each data value (Y0, U, Y1, V) is 8 bits. This format is identical to the UYVY format described 
	//      except that the ordering of the bits is changed.
	//
	//      D3DFMT_UYVY 
	//      UYVY format (PC98 compliance). Two pixels are stored in each UYVY word. 
	//      Each data value (U, Y0, V, Y1) is 8 bits. The U and V specify the chrominance and are 
	//      shared by both pixels. The Y0 and Y1 values specify the luminance of the respective pixels. 
	//      The chrominance and luminance can be computed from RGB values using the following formulas: 
	//      Y = 0.299 * R + 0.587 * G + 0.114 * B
	//      U = -0.168736 * R + -0.331264 * G + 0.5 * B
	//      V = 0.5 * R + -0.418688 * G + -0.081312 * B 
	//  */

	g_graphicsContext.Lock();

	// Calculate texture pitches - must have a stride that is a multiple of 64 bytes
	ytexture_pitch = (image_width + 63) & ~63;
	uvtexture_pitch = (image_width/2 + 63) & ~63;

	for (int i = 0; i < NUM_BUFFERS; ++i)
	{
		int yw = (image_width + 1) & ~1; // make sure it's even
		int uvw = (image_width/2 + 1) & ~1; // make sure it's even
		ytexture_height = (image_height + 1) & ~1; // make sure it's even
		uvtexture_height = (image_height/2 + 1) & ~1; // make sure it's even

		// Create texture buffer
		if (m_TextureBuffer[i])
			XPhysicalFree(m_TextureBuffer[i]);
		m_TextureBuffer[i] = (BYTE*)XPhysicalAlloc(ytexture_pitch*ytexture_height + uvtexture_pitch*uvtexture_height*2, MAXULONG_PTR, 128, PAGE_READWRITE | PAGE_WRITECOMBINE);

		// blank the textures (just in case)
		memset(m_TextureBuffer[i], 0, ytexture_pitch*ytexture_height);
		memset((char*)m_TextureBuffer[i] + ytexture_pitch*ytexture_height, 128, uvtexture_pitch*uvtexture_height*2);

		// setup textures as linear luminance only textures, the pixel shader handles translation to RGB
		// YV12 is Y plane then V plane then U plane, U and V are half as wide and high
		XGSetTextureHeader(yw, ytexture_height, 1, 0, D3DFMT_LIN_L8, 0, &m_YTexture[i], 0, ytexture_pitch);
		XGSetTextureHeader(uvw, uvtexture_height, 1, 0, D3DFMT_LIN_L8, 0, &m_UTexture[i], ytexture_pitch*ytexture_height, uvtexture_pitch);
		XGSetTextureHeader(uvw, uvtexture_height, 1, 0, D3DFMT_LIN_L8, 0, &m_VTexture[i], ytexture_pitch*ytexture_height+uvtexture_pitch*uvtexture_height, uvtexture_pitch);
		m_YTexture[i].Register(m_TextureBuffer[i]);
		m_UTexture[i].Register(m_TextureBuffer[i]);
		m_VTexture[i].Register(m_TextureBuffer[i]);
	}

	// Create our video vertex buffer
	if (m_pVideoVB)
		m_pVideoVB->Release();
	g_graphicsContext.Get3DDevice()->CreateVertexBuffer(4*sizeof(VIDVERTEX), D3DUSAGE_WRITEONLY, 0L, D3DPOOL_DEFAULT, &m_pVideoVB );

	// Create subtitle textures
	for (int i = 0; i < 2; ++i)
	{
		if (m_pSubtitleTexture[i])  m_pSubtitleTexture[i]->Release();
		g_graphicsContext.Get3DDevice()->CreateTexture( SUBTITLE_TEXTURE_WIDTH,
																										SUBTITLE_TEXTURE_HEIGHT,
																										1,
																										0,
																										D3DFMT_LIN_A8R8G8B8,
																										0,
																										&m_pSubtitleTexture[i] ) ;

		// Clear the subtitle region of this overlay
		D3DLOCKED_RECT rectLocked;
		if ( D3D_OK == m_pSubtitleTexture[i]->LockRect(0,&rectLocked,NULL,0L) )
		{   
			subtitleimage  =(unsigned char*)rectLocked.pBits;
			subtitlestride = rectLocked.Pitch;
			iClearSubtitleRegion[i] = SUBTITLE_TEXTURE_HEIGHT;
			ClearSubtitleRegion(i);
		}
		m_pSubtitleTexture[i]->UnlockRect(0);
	}

	// Create our subtitle vertex buffer
	if (m_pSubtitleVB) m_pSubtitleVB->Release();
	g_graphicsContext.Get3DDevice()->CreateVertexBuffer( 4*sizeof(SUBVERTEX), D3DUSAGE_WRITEONLY, 0L, D3DPOOL_DEFAULT, &m_pSubtitleVB );
	xbox_video_update_subtitle_position();

	//g_graphicsContext.Get3DDevice()->EnableOverlay(TRUE);

	g_graphicsContext.Unlock();

	// Reset video size stuff so that the VB is filled
	m_fOffsetX1 = 0;
	m_fOffsetY1 = 0;
	m_iScreenWidth = 0;
	m_iScreenHeight = 0;
	m_bStretch = false;
	m_bZoom = false;
}

//***************************************************************************************
// CalculateSourceFrameRatio()
//
// Considers the source frame size and output frame size (as suggested by mplayer)
// to determine if the pixels in the source are not square.  It calculates the aspect
// ratio of the output frame.  We consider the cases of VCD, SVCD and DVD separately,
// as these are intended to be viewed on a non-square pixel TV set, so the pixels are
// defined to be the same ratio as the intended display pixels.
// These formats are determined by frame size.
//***************************************************************************************
static void CalculateFrameAspectRatio()
{
	fSourceFrameRatio = (float)d_image_width / d_image_height;

	// Check whether mplayer has decided that the size of the video file should be changed
	// This indicates either a scaling has taken place (which we didn't ask for) or it has
	// found an aspect ratio parameter from the file, and is changing the frame size based
	// on that.
	if (image_width == d_image_width && image_height == d_image_height)
		return;

	// mplayer is scaling in one or both directions.  We must alter our Source Pixel Ratio
	float fImageFrameRatio = (float)image_width / image_height;

	// OK, most sources will be correct now, except those that are intended
	// to be displayed on non-square pixel based output devices (ie PAL or NTSC TVs)
	// This includes VCD, SVCD, and DVD (and possibly others that we are not doing yet)
	// For this, we can base the pixel ratio on the pixel ratios of PAL and NTSC,
	// though we will need to adjust for anamorphic sources (ie those whose
	// output frame ratio is not 4:3) and for SVCDs which have 2/3rds the
	// horizontal resolution of the default NTSC or PAL frame sizes

	// The following are the defined standard ratios for PAL and NTSC pixels
	float fPALPixelRatio = 128.0f/117.0f;
	float fNTSCPixelRatio = 72.0f/79.0f;

	// Calculate the correction needed for anamorphic sources
	float fNon4by3Correction = (float)fSourceFrameRatio/(4.0f/3.0f);

	// Now use the helper functions to check for a VCD, SVCD or DVD frame size
	if (CUtil::IsNTSC_VCD(image_width,image_height))
		fSourceFrameRatio = fImageFrameRatio*fNTSCPixelRatio;
	if (CUtil::IsNTSC_SVCD(image_width,image_height))
		fSourceFrameRatio = fImageFrameRatio*3.0f/2.0f*fNTSCPixelRatio*fNon4by3Correction;
	if (CUtil::IsNTSC_DVD(image_width,image_height))
		fSourceFrameRatio = fImageFrameRatio*fNTSCPixelRatio*fNon4by3Correction;
	if (CUtil::IsPAL_VCD(image_width,image_height))
		fSourceFrameRatio = fImageFrameRatio*fPALPixelRatio;
	if (CUtil::IsPAL_SVCD(image_width,image_height))
		fSourceFrameRatio = fImageFrameRatio*3.0f/2.0f*fPALPixelRatio*fNon4by3Correction;
	if (CUtil::IsPAL_DVD(image_width,image_height))
		fSourceFrameRatio = fImageFrameRatio*fPALPixelRatio*fNon4by3Correction;
}

//********************************************************************************************************
unsigned int Directx_ManageDisplay()
{
	RESOLUTION iRes = g_graphicsContext.GetVideoResolution();
	float fOffsetX1 = (float)g_settings.m_ResInfo[iRes].Overscan.left;
	float fOffsetY1 = (float)g_settings.m_ResInfo[iRes].Overscan.top;
	float iScreenWidth = (float)g_settings.m_ResInfo[iRes].Overscan.width;
	float iScreenHeight = (float)g_settings.m_ResInfo[iRes].Overscan.height;

	if( !(g_graphicsContext.IsFullScreenVideo() || g_graphicsContext.IsCalibrating() ))
	{
		const RECT& rv = g_graphicsContext.GetViewWindow();
		iScreenWidth = (float)rv.right-rv.left;
		iScreenHeight= (float)rv.bottom-rv.top;
		fOffsetX1    = (float)rv.left;
		fOffsetY1    = (float)rv.top;
	}

	// Correct for HDTV_1080i -> 540p
	// don't think we need this if we're not using overlays
//	if (iRes == HDTV_1080i)
//	{
//		fOffsetY1/=2;
//		iScreenHeight/=2;
//	}

	// this function is pretty expensive now as it has to take a lock on the VB
	// need to check if it's really neccessary
	if (m_fOffsetX1 == fOffsetX1 && m_fOffsetY1 == fOffsetY1 &&
		m_iScreenWidth == iScreenWidth && m_iScreenHeight == iScreenHeight &&
		m_bStretch == g_stSettings.m_bStretch && m_bZoom == g_stSettings.m_bZoom)
		return 0;

	if (!m_pVideoVB)
		return 1;

	g_graphicsContext.Lock();

	VIDVERTEX* vertex=NULL;
	m_pVideoVB->Lock(0, 0, (BYTE**)&vertex, 0L);

	m_iScreenWidth = iScreenWidth;
	m_iScreenHeight = iScreenHeight;
	m_fOffsetX1 = fOffsetX1;
	m_fOffsetY1 = fOffsetY1;
	m_bStretch = g_stSettings.m_bStretch;
	m_bZoom = g_stSettings.m_bZoom;

	//if( g_graphicsContext.IsFullScreenVideo() || g_graphicsContext.IsCalibrating() )
	{
		if (g_stSettings.m_bStretch)
		{
			// stretch the movie so it occupies the entire screen (aspect ratio = gone)
			rs.left   = 0;
			rs.top    = 0;
			rs.right  = image_width;
			rs.bottom = image_height ;
			rd.left   = (int)fOffsetX1;
			rd.right  = (int)rd.left+(int)iScreenWidth;
			rd.top    = (int)fOffsetY1;
			rd.bottom = (int)rd.top+(int)iScreenHeight;

			vertex[0].p = D3DXVECTOR4(fOffsetX1,              fOffsetY1, 0, 0);
			vertex[1].p = D3DXVECTOR4(fOffsetX1+iScreenWidth, fOffsetY1, 0, 0);
			vertex[2].p = D3DXVECTOR4(fOffsetX1+iScreenWidth, fOffsetY1+iScreenHeight, 0, 0);
			vertex[3].p = D3DXVECTOR4(fOffsetX1,              fOffsetY1+iScreenHeight, 0, 0);
			vertex[0].tu0 = 0;                  vertex[0].tv0 = 0;
			vertex[1].tu0 = (float)image_width; vertex[1].tv0 = 0;
			vertex[2].tu0 = (float)image_width; vertex[2].tv0 = (float)image_height;
			vertex[3].tu0 = 0;                  vertex[3].tv0 = (float)image_height;

			for (int i = 0; i < 4; ++i)
			{
				vertex[i].tu1 = vertex[i].tu2 = vertex[i].tu0 / 2;
				vertex[i].tv1 = vertex[i].tv2 = vertex[i].tv0 / 2;
			}

			m_pVideoVB->Unlock();
			g_graphicsContext.Unlock();

			return 0;
		}

		if (g_stSettings.m_bZoom)
		{
			// zoom / panscan the movie so that it fills the entire screen
			// and keeps the Aspect ratio

			// calculate AR compensation (see http://www.iki.fi/znark/video/conversion)
			float fOutputFrameRatio = fSourceFrameRatio / g_settings.m_ResInfo[iRes].fPixelRatio; 
//			if (m_iResolution == HDTV_1080i) fOutputFrameRatio *= 2;

			// assume that the movie is widescreen first, so use full height
			float fVertBorder=0;
			float fNewHeight = (float)( iScreenHeight);
			float fNewWidth  =  fNewHeight*fOutputFrameRatio;
			float fHorzBorder= (fNewWidth-(float)iScreenWidth)/2.0f;
			float fFactor = fNewWidth / ((float)image_width);
			fHorzBorder = fHorzBorder/fFactor;

			if ( (int)fNewWidth < iScreenWidth )
			{
				fHorzBorder=0;
				fNewWidth  = (float)( iScreenWidth);
				fNewHeight = fNewWidth/fOutputFrameRatio;
				fVertBorder= (fNewHeight-(float)iScreenHeight)/2.0f;
				fFactor = fNewWidth / ((float)image_width);
				fVertBorder = fVertBorder/fFactor;
			}
			rs.left   = (int)fHorzBorder;
			rs.top    = (int)fVertBorder;
			rs.right  = (int)image_width  - (int)fHorzBorder;
			rs.bottom = (int)image_height - (int)fVertBorder;
			rd.left   = (int)fOffsetX1;
			rd.right  = (int)rd.left + (int)iScreenWidth;
			rd.top    = (int)fOffsetY1;
			rd.bottom = (int)rd.top + (int)iScreenHeight;

			vertex[0].p = D3DXVECTOR4(fOffsetX1,              fOffsetY1, 0, 0);
			vertex[1].p = D3DXVECTOR4(fOffsetX1+iScreenWidth, fOffsetY1, 0, 0);
			vertex[2].p = D3DXVECTOR4(fOffsetX1+iScreenWidth, fOffsetY1+iScreenHeight, 0, 0);
			vertex[3].p = D3DXVECTOR4(fOffsetX1,              fOffsetY1+iScreenHeight, 0, 0);
			vertex[0].tu0 = fHorzBorder;             vertex[0].tv0 = fVertBorder;
			vertex[1].tu0 = image_width-fHorzBorder; vertex[1].tv0 = fVertBorder;
			vertex[2].tu0 = image_width-fHorzBorder; vertex[2].tv0 = image_height-fVertBorder;
			vertex[3].tu0 = fHorzBorder;             vertex[3].tv0 = image_height-fVertBorder;

			for (int i = 0; i < 4; ++i)
			{
				vertex[i].tu1 = vertex[i].tu2 = vertex[i].tu0 / 2;
				vertex[i].tv1 = vertex[i].tv2 = vertex[i].tv0 / 2;
			}

			m_pVideoVB->Unlock();
			g_graphicsContext.Unlock();

			return 0;
		}

		// NORMAL
		// scale up image as much as possible
		// and keep the aspect ratio (introduces with black bars)

		float fOutputFrameRatio = fSourceFrameRatio / g_settings.m_ResInfo[iRes].fPixelRatio; 
//		if (m_iResolution == HDTV_1080i) fOutputFrameRatio *= 2;

		// maximize the movie width
		float fNewWidth  = iScreenWidth;
		float fNewHeight = fNewWidth/fOutputFrameRatio;

		if (fNewHeight > iScreenHeight)
		{
			fNewHeight = iScreenHeight;
			fNewWidth = fNewHeight*fOutputFrameRatio;
		}

		// this shouldnt happen, but just make sure that everything still fits onscreen
		if (fNewWidth > iScreenWidth || fNewHeight > iScreenHeight)
		{
			fNewWidth=(float)image_width;
			fNewHeight=(float)image_height;
		}

		// Centre the movie
		float iPosY = (iScreenHeight - fNewHeight)/2;
		float iPosX = (iScreenWidth  - fNewWidth)/2;

		// source rect
		rs.left   = 0;
		rs.top    = 0;
		rs.right  = image_width;
		rs.bottom = image_height;
		rd.left   = (int)(iPosX + fOffsetX1);
		rd.right  = (int)(rd.left + fNewWidth + 0.5f);
		rd.top    = (int)(iPosY + fOffsetY1);
		rd.bottom = (int)(rd.top + fNewHeight + 0.5f);

		vertex[0].p = D3DXVECTOR4(iPosX+fOffsetX1,                iPosY+fOffsetY1, 0, 0);
		vertex[1].p = D3DXVECTOR4(iPosX+fOffsetX1+fNewWidth+0.5f, iPosY+fOffsetY1, 0, 0);
		vertex[2].p = D3DXVECTOR4(iPosX+fOffsetX1+fNewWidth+0.5f, iPosY+fOffsetY1+fNewHeight+0.5f, 0, 0);
		vertex[3].p = D3DXVECTOR4(iPosX+fOffsetX1,                iPosY+fOffsetY1+fNewHeight+0.5f, 0, 0);
		vertex[0].tu0 = 0;                  vertex[0].tv0 = 0;
		vertex[1].tu0 = (float)image_width; vertex[1].tv0 = 0;
		vertex[2].tu0 = (float)image_width; vertex[2].tv0 = (float)image_height;
		vertex[3].tu0 = 0;                  vertex[3].tv0 = (float)image_height;

		for (int i = 0; i < 4; ++i)
		{
			vertex[i].tu1 = vertex[i].tu2 = vertex[i].tu0 / 2;
			vertex[i].tv1 = vertex[i].tv2 = vertex[i].tv0 / 2;
		}

		m_pVideoVB->Unlock();
		g_graphicsContext.Unlock();
	}
	return 0;
}

//***********************************************************************************************************
static void vo_draw_alpha_rgb32_shadow(int w,int h, unsigned char* src, unsigned char *srca, int srcstride, unsigned char* dstbase,int dststride)
{
  if (m_bPauseDrawing) return;
	register int y;
	for(y=0;y<h;y++)
	{
		register int x;

		for(x=0;x<w;x++)
		{
			if(srca[x])
			{
				dstbase[4*x+0]=((dstbase[4*x+0]*srca[x])>>8)+src[x];
				dstbase[4*x+1]=((dstbase[4*x+1]*srca[x])>>8)+src[x];
				dstbase[4*x+2]=((dstbase[4*x+2]*srca[x])>>8)+src[x];
				dstbase[4*x+3]=0xff;
			}
		}
		src+=srcstride;
		srca+=srcstride;
		dstbase+=dststride;
	}
}

//********************************************************************************************************
static void draw_alpha(int x0, int y0, int w, int h, unsigned char *src,unsigned char *srca, int stride)
{
  if (m_bPauseDrawing) return;
	// if we draw text on the bottom then it must b the subtitles
	// if we're not in stretch mode try to put the subtitles below the video
	if ( y0 > (int)(image_height/10)  )
	{
		if (w > SUBTITLE_TEXTURE_WIDTH) w = SUBTITLE_TEXTURE_WIDTH;
		if (h > SUBTITLE_TEXTURE_HEIGHT) h = SUBTITLE_TEXTURE_HEIGHT;
		int xpos = (SUBTITLE_TEXTURE_WIDTH-w)/2;
		//    OutputDebugString("Rendering Subs\n");
		vo_draw_alpha_rgb32_shadow(w,h,src,srca,stride,((unsigned char *) subtitleimage) + subtitlestride*(SUBTITLE_TEXTURE_HEIGHT-h) + 4*xpos,subtitlestride);
		//    vo_draw_alpha_rgb32(w,h,src,srca,stride,((unsigned char *) subtitleimage) + subtitlestride*(SUBTITLE_TEXTURE_HEIGHT-h) + 4*xpos,subtitlestride);
		iClearSubtitleRegion[m_iBackBuffer] = h;
		m_bRenderGUI=true;
		return;
	}
  
  // OSD is drawn after draw_slice / put_image
  // this means that m_iDecodeBuffer already points to the next buffer
  // so get previous buffer and draw OSD into it
  int iBuffer=m_iDecodeBuffer-1;
  if (iBuffer < 0) iBuffer=NUM_BUFFERS-1;
  byte* image=(byte*)m_TextureBuffer[iBuffer];
  DWORD dstride=ytexture_pitch;
	vo_draw_alpha_yv12(w,h,src,srca,stride,((unsigned char *) image) + dstride*y0 + 2*x0,dstride);
}

//********************************************************************************************************
static void video_draw_osd(void)
{
  
  if (m_bPauseDrawing) return;
	vo_draw_text(image_width, image_height, draw_alpha);
}

//********************************************************************************************************
static unsigned int query_format(unsigned int format)
{
	unsigned int i=0;
	while ( i < NUM_FORMATS )
	{
		if (g_ddpf[i].img_format == format)
			return g_ddpf[i].drv_caps;
		i++;
	}
	return 0;
}

//********************************************************************************************************
static void video_uninit(void)
{
	OutputDebugString("video_uninit\n");  

//	g_graphicsContext.Get3DDevice()->EnableOverlay(FALSE);
	for (int i=0; i < NUM_BUFFERS; i++)
	{
		if (m_TextureBuffer[i]) XPhysicalFree(m_TextureBuffer[i]);
		m_TextureBuffer[i] = NULL;
	}
	for (int i = 0; i < 2; ++i)
	{
		// subtitle stuff
		if ( m_pSubtitleTexture[i]) m_pSubtitleTexture[i]->Release();
		m_pSubtitleTexture[i]=NULL;
	}
	if (m_pVideoVB) m_pVideoVB->Release();
	m_pVideoVB=NULL;
	if (m_pSubtitleVB) m_pSubtitleVB->Release();
	m_pSubtitleVB=NULL;
	OutputDebugString("video_uninit done\n");
}

//***********************************************************************************************************
static void video_check_events(void)
{
}

//***********************************************************************************************************
static unsigned int video_preinit(const char *arg)
{
	m_iResolution=PAL_4x3;
	m_bPauseDrawing=false;
	for (int i=0; i<NUM_BUFFERS; i++) iClearSubtitleRegion[i] = 0;
	m_iBackBuffer=0;
	m_iRenderBuffer = -1;
  m_iDecodeBuffer = 0;
	m_bFlip=0;
	fs=1;

	// Create the pixel shader
	if (!m_hPixelShader)
	{
		D3DPIXELSHADERDEF* ShaderBuffer = (D3DPIXELSHADERDEF*)((char*)XLoadSection("PXLSHADER") + 4);
		g_graphicsContext.Get3DDevice()->CreatePixelShader(ShaderBuffer, &m_hPixelShader);
	}

	return 0;
}

//********************************************************************************************************
static unsigned int video_draw_slice(unsigned char *src[], int stride[], int w,int h,int x,int y )
{
  BYTE *s;
  BYTE *d;
  DWORD i=0;
  int   iBottom=y+h;
  int iOrgY=y;
  int iOrgH=h;

	while (m_YTexture[m_iDecodeBuffer].IsBusy()) Sleep(1);
	while (m_UTexture[m_iDecodeBuffer].IsBusy()) Sleep(1);
	while (m_VTexture[m_iDecodeBuffer].IsBusy()) Sleep(1);

  // copy Y
  d=(BYTE*)m_TextureBuffer[m_iDecodeBuffer]+ytexture_pitch*y+x;                
  s=src[0];                           
  for(i=0;i<(DWORD)h;i++){
    fast_memcpy(d,s,w);                  
    s+=stride[0];                  
    d+=ytexture_pitch;
  }
    
	w/=2;h/=2;x/=2;y/=2;
	
	// copy U
  d=(BYTE*)m_TextureBuffer[m_iDecodeBuffer] + ytexture_pitch*ytexture_height+ uvtexture_pitch*y+x;
  s=src[1];
  for(i=0;i<(DWORD)h;i++){
    fast_memcpy(d,s,w);
    s+=stride[1];
    d+=uvtexture_pitch;
  }
	// copy V
  d=(BYTE*)m_TextureBuffer[m_iDecodeBuffer] + ytexture_pitch*ytexture_height + uvtexture_pitch*uvtexture_height+ uvtexture_pitch*y+x;
  s=src[2];
  for(i=0;i<(DWORD)h;i++){
    fast_memcpy(d,s,w);
    s+=stride[2];
    d+=uvtexture_pitch;
  }

  if (iBottom+1>=(int)image_height)
  {
    ++m_iDecodeBuffer %= NUM_BUFFERS;
    m_bFlip++;
   // char szTmp[128];
   // sprintf(szTmp,"%i slice%i->%i decodebuf:%i renderbuf:%i flip:%i\n", 
   //     image_height,iOrgY,iOrgY+iOrgH,m_iDecodeBuffer ,m_iRenderBuffer, m_bFlip);
   // OutputDebugString(szTmp);
  }

  return 0;
}

//********************************************************************************************************
void xbox_video_render_subtitles(bool bUseBackBuffer)
{
	int iTexture = bUseBackBuffer ? m_iBackBuffer : 1-m_iBackBuffer;
  
	if (!m_pSubtitleTexture[iTexture])
		return;

	if (!m_pSubtitleVB)
		return;
	//  OutputDebugString("Rendering subs to screen\n");
	// Set state to render the image
	g_graphicsContext.Get3DDevice()->SetTexture( 0, m_pSubtitleTexture[iTexture]);
	g_graphicsContext.Get3DDevice()->SetTextureStageState( 0, D3DTSS_COLOROP,   D3DTOP_MODULATE );
	g_graphicsContext.Get3DDevice()->SetTextureStageState( 0, D3DTSS_COLORARG1, D3DTA_TEXTURE );
	g_graphicsContext.Get3DDevice()->SetTextureStageState( 0, D3DTSS_COLORARG2, D3DTA_DIFFUSE );
	g_graphicsContext.Get3DDevice()->SetTextureStageState( 0, D3DTSS_ALPHAOP,   D3DTOP_MODULATE );
	g_graphicsContext.Get3DDevice()->SetTextureStageState( 0, D3DTSS_ALPHAARG1, D3DTA_TEXTURE );
	g_graphicsContext.Get3DDevice()->SetTextureStageState( 0, D3DTSS_ALPHAARG2, D3DTA_DIFFUSE );
	g_graphicsContext.Get3DDevice()->SetTextureStageState( 1, D3DTSS_COLOROP,   D3DTOP_DISABLE );
	g_graphicsContext.Get3DDevice()->SetTextureStageState( 1, D3DTSS_ALPHAOP,   D3DTOP_DISABLE );
	g_graphicsContext.Get3DDevice()->SetTextureStageState( 0, D3DTSS_ADDRESSU,  D3DTADDRESS_CLAMP );
	g_graphicsContext.Get3DDevice()->SetTextureStageState( 0, D3DTSS_ADDRESSV,  D3DTADDRESS_CLAMP );

	g_graphicsContext.Get3DDevice()->SetRenderState( D3DRS_ZENABLE,      FALSE );
	g_graphicsContext.Get3DDevice()->SetRenderState( D3DRS_FOGENABLE,    FALSE );
	g_graphicsContext.Get3DDevice()->SetRenderState( D3DRS_FOGTABLEMODE, D3DFOG_NONE );
	g_graphicsContext.Get3DDevice()->SetRenderState( D3DRS_FILLMODE,     D3DFILL_SOLID );
	g_graphicsContext.Get3DDevice()->SetRenderState( D3DRS_CULLMODE,     D3DCULL_CCW );
	g_graphicsContext.Get3DDevice()->SetRenderState( D3DRS_ALPHABLENDENABLE, TRUE );
	g_graphicsContext.Get3DDevice()->SetRenderState( D3DRS_SRCBLEND,  D3DBLEND_SRCALPHA );
	g_graphicsContext.Get3DDevice()->SetRenderState( D3DRS_DESTBLEND, D3DBLEND_INVSRCALPHA );
	g_graphicsContext.Get3DDevice()->SetVertexShader( FVF_SUBVERTEX );
	// Render the image
	g_graphicsContext.Get3DDevice()->SetStreamSource( 0, m_pSubtitleVB, sizeof(SUBVERTEX) );
	g_graphicsContext.Get3DDevice()->DrawPrimitive( D3DPT_QUADLIST, 0, 1 );

}
//********************************************************************************************************
void RenderVideo()
{
  if (m_iRenderBuffer<0 || m_iRenderBuffer >= NUM_BUFFERS) return;
	if (!m_TextureBuffer[m_iRenderBuffer]) return;
  g_graphicsContext.Get3DDevice()->SetTexture( 0, &m_YTexture[m_iRenderBuffer]);
	g_graphicsContext.Get3DDevice()->SetTexture( 1, &m_UTexture[m_iRenderBuffer]);
	g_graphicsContext.Get3DDevice()->SetTexture( 2, &m_VTexture[m_iRenderBuffer]);
	for (int i = 0; i < 3; ++i)
	{
		g_graphicsContext.Get3DDevice()->SetTextureStageState( i, D3DTSS_ADDRESSU,  D3DTADDRESS_CLAMP );
		g_graphicsContext.Get3DDevice()->SetTextureStageState( i, D3DTSS_ADDRESSV,  D3DTADDRESS_CLAMP );
		g_graphicsContext.Get3DDevice()->SetTextureStageState( i, D3DTSS_MAGFILTER,  g_stSettings.m_minFilter );
		g_graphicsContext.Get3DDevice()->SetTextureStageState( i, D3DTSS_MINFILTER,  g_stSettings.m_maxFilter );
	}
	g_graphicsContext.Get3DDevice()->SetPixelShader(m_hPixelShader);

	g_graphicsContext.Get3DDevice()->SetRenderState( D3DRS_ZENABLE,      FALSE );
	g_graphicsContext.Get3DDevice()->SetRenderState( D3DRS_FOGENABLE,    FALSE );
	g_graphicsContext.Get3DDevice()->SetRenderState( D3DRS_FOGTABLEMODE, D3DFOG_NONE );
	g_graphicsContext.Get3DDevice()->SetRenderState( D3DRS_FILLMODE,     D3DFILL_SOLID );
	g_graphicsContext.Get3DDevice()->SetRenderState( D3DRS_CULLMODE,     D3DCULL_CCW );
	g_graphicsContext.Get3DDevice()->SetRenderState( D3DRS_ALPHABLENDENABLE, FALSE );
	g_graphicsContext.Get3DDevice()->SetRenderState( D3DRS_SRCBLEND,  D3DBLEND_ONE );
	g_graphicsContext.Get3DDevice()->SetRenderState( D3DRS_DESTBLEND, D3DBLEND_ZERO );
	g_graphicsContext.Get3DDevice()->SetVertexShader( FVF_VIDVERTEX );
	// Render the image
	g_graphicsContext.Get3DDevice()->SetStreamSource( 0, m_pVideoVB, sizeof(VIDVERTEX) );
	g_graphicsContext.Get3DDevice()->DrawPrimitive( D3DPT_QUADLIST, 0, 1 );

	g_graphicsContext.Get3DDevice()->SetPixelShader(NULL);
	g_graphicsContext.Get3DDevice()->SetTexture( 0, NULL);
	g_graphicsContext.Get3DDevice()->SetTexture( 1, NULL);
	g_graphicsContext.Get3DDevice()->SetTexture( 2, NULL);
}
//************************************************************************************
static void video_flip_page(void)
{
  if (m_bPauseDrawing) return;
  
	int nextbuf = (m_iRenderBuffer+1) % NUM_BUFFERS;
	if (m_bFlip<=0 || nextbuf == m_iDecodeBuffer)
  {
    return;
  }
	m_bFlip--;
  m_iRenderBuffer = nextbuf;
	m_pSubtitleTexture[m_iBackBuffer]->UnlockRect(0);
  

	Directx_ManageDisplay();
	if (g_graphicsContext.IsFullScreenVideo())
	{
		if (g_application.NeedRenderFullScreen())
			m_bRenderGUI = true;

		g_graphicsContext.Lock();
		g_graphicsContext.Get3DDevice()->Clear( 0L, NULL, D3DCLEAR_TARGET|D3DCLEAR_ZBUFFER|D3DCLEAR_STENCIL, 0x00010001, 1.0f, 0L );

		// render first so the subtitle overlay works
		RenderVideo();

		if (m_bRenderGUI)
		{
			// update our subtitle position
			xbox_video_update_subtitle_position();
			xbox_video_render_subtitles(true);
			g_application.RenderFullScreen();
		}

	  //g_graphicsContext.Get3DDevice()->BlockUntilVerticalBlank();      
		g_graphicsContext.Get3DDevice()->Present( NULL, NULL, NULL, NULL );
		g_graphicsContext.Unlock();
		m_bRenderGUI=false;
	}
	m_iBackBuffer = 1-m_iBackBuffer;
	g_graphicsContext.Lock();
	D3DLOCKED_RECT rectLocked2;
	if ( D3D_OK == m_pSubtitleTexture[m_iBackBuffer]->LockRect(0,&rectLocked2,NULL,0L))
	{
		subtitlestride=rectLocked2.Pitch;
		subtitleimage  =(unsigned char*)rectLocked2.pBits;
	}

	if (iClearSubtitleRegion[m_iBackBuffer])
	{
		ClearSubtitleRegion(m_iBackBuffer);
		m_bRenderGUI=true;
	}
	g_graphicsContext.Unlock();

	m_bFlipped=true;
/*
#if 0
	// when movie is running,
	// check the FPS again after 50 frames
	// after 50 frames mplayer has determined the REAL fps instead of just the one mentioned in the header
	// if its different we might need pal60
	m_lFrameCounter++;
	if (m_lFrameCounter==50)
	{
		char strFourCC[12];
		char strVideoCodec[256];
		unsigned int iWidth,iHeight;
		long tooearly, toolate;
		float fps;
		mplayer_GetVideoInfo(strFourCC,strVideoCodec, &fps, &iWidth,&iHeight, &tooearly, &toolate);
		if (fps != m_fps)
		{
			choose_best_resolution(fps);
		}
	}
#endif
*/
}

//********************************************************************************************************
void xbox_video_wait()
{
	int iTmp=0;
	m_bFlipped=false;
	while (!m_bFlipped && iTmp < 300)
	{
		Sleep(10);
		iTmp++;
	}
}
//********************************************************************************************************
void xbox_video_getRect(RECT& SrcRect, RECT& DestRect)
{
	SrcRect=rs;
	DestRect=rd;
}

//********************************************************************************************************
void xbox_video_CheckScreenSaver()
{
	// Called from CMPlayer::Process() (mplayer.cpp) when in 'pause' mode

	return;
}

//********************************************************************************************************
void xbox_video_getAR(float& fAR)
{
	float fOutputPixelRatio = g_settings.m_ResInfo[m_iResolution].fPixelRatio;
	float fWidth = (float)(rd.right - rd.left);
	float fHeight = (float)(rd.bottom - rd.top);
	fAR = fWidth/fHeight*fOutputPixelRatio;
}

//********************************************************************************************************
void xbox_video_render_update()
{
  if (m_iRenderBuffer < 0 || m_iRenderBuffer >=NUM_BUFFERS) return;
  bool bFullScreen = g_graphicsContext.IsFullScreenVideo() || g_graphicsContext.IsCalibrating();
	if (m_pVideoVB && m_TextureBuffer[m_iRenderBuffer])
	{
		g_graphicsContext.Lock();
		RenderVideo();
    if ( bFullScreen )
    {
		  xbox_video_update_subtitle_position();
		  xbox_video_render_subtitles(false);
    }

		g_graphicsContext.Unlock();
	}
}


//********************************************************************************************************
void xbox_video_update(bool bPauseDrawing)
{
	m_bRenderGUI=true;
  m_bPauseDrawing = bPauseDrawing;
	bool bFullScreen = g_graphicsContext.IsFullScreenVideo() || g_graphicsContext.IsCalibrating();
	g_graphicsContext.Lock();
	g_graphicsContext.SetVideoResolution(bFullScreen ? m_iResolution:g_stSettings.m_GUIResolution);
	g_graphicsContext.Unlock();
	Directx_ManageDisplay();
	xbox_video_render_update();
}


//********************************************************************************************************
static unsigned int video_draw_frame(unsigned char *src[])
{
	DebugBreak();
  byte* image=(BYTE*)m_TextureBuffer[m_iDecodeBuffer];
	fast_memcpy( image, *src, ytexture_pitch * image_height );
	return 0;
}

//********************************************************************************************************
static unsigned int get_image(mp_image_t *mpi)
{
	return VO_FALSE;
}
//********************************************************************************************************
static unsigned int put_image(mp_image_t *mpi)
{
	if((mpi->flags&MP_IMGFLAG_DIRECT) || (mpi->flags&MP_IMGFLAG_DRAW_CALLBACK))
	{
		return VO_FALSE;
	}
  DWORD  i = 0;
  BYTE   *d;
  BYTE   *s;
  DWORD x = mpi->x;
  DWORD y = mpi->y;
  DWORD w = mpi->w;
  DWORD h = mpi->h;

	while (m_YTexture[m_iDecodeBuffer].IsBusy()) Sleep(1);
	while (m_UTexture[m_iDecodeBuffer].IsBusy()) Sleep(1);
	while (m_VTexture[m_iDecodeBuffer].IsBusy()) Sleep(1);

	if (mpi->flags&MP_IMGFLAG_PLANAR)
	{
    //if(image_format!=IMGFMT_YVU9) video_draw_slice( mpi->planes,(int*)&mpi->stride[0],mpi->w,mpi->h,0,0);
    //else
    {
      // copy Y
      byte* image=(byte*)m_TextureBuffer[m_iDecodeBuffer];
      DWORD dstride=ytexture_pitch;
      d=image+dstride*y+x;
      s=mpi->planes[0];
      for(i=0;i<h;i++)
      {
        fast_memcpy(d,s,w);
        s+=mpi->stride[0];
        d+=dstride;
      }
      //w/=4;h/=4;x/=4;y/=4;
      w/=2;
      h/=2;
      x/=2;
      y/=2;
      
      // copy V
      //d=image+dstride*image_height + dstride*y/4+x;
      dstride=uvtexture_pitch;
      d=(BYTE*)m_TextureBuffer[m_iDecodeBuffer] + ytexture_pitch*ytexture_height + dstride*y+x;
      s=mpi->planes[1];
      for(i=0;i<h;i++){
        fast_memcpy(d,s,w);
        s+=mpi->stride[1];
        d+=dstride;
      }


      // copy U
      //d=image+dstride*image_height + dstride*image_height/16 + dstride/4*y+x;
      d=(BYTE*)m_TextureBuffer[m_iDecodeBuffer] + ytexture_pitch*ytexture_height + uvtexture_pitch*uvtexture_height + dstride*y+x;
      s=mpi->planes[2];
      for(i=0;i<h;i++){
        fast_memcpy(d,s,w);
        s+=mpi->stride[2];
        d+=dstride;
      }
    }
	}
	else
	{
		//packed - this isn't going to work!
		DebugBreak();
    fast_memcpy( m_TextureBuffer[m_iDecodeBuffer], mpi->planes[0], ytexture_height * ytexture_pitch);
	}

  ++m_iDecodeBuffer %= NUM_BUFFERS;
  m_bFlip++;

  //char szTmp[128];
  //sprintf(szTmp,"putimage decodebuf:%i renderbuf:%i flip:%i\n", m_iDecodeBuffer ,m_iRenderBuffer, m_bFlip);
  //OutputDebugString(szTmp);
	return VO_TRUE;
}

//********************************************************************************************************
static unsigned int video_config(unsigned int width, unsigned int height, unsigned int d_width, unsigned int d_height, unsigned int options, char *title, unsigned int format)
{
	char strFourCC[12];
	char strVideoCodec[256];

	unsigned int iWidth,iHeight;
	long tooearly, toolate;

	m_lFrameCounter=0;
	mplayer_GetVideoInfo(strFourCC,strVideoCodec, &m_fps, &iWidth,&iHeight, &tooearly, &toolate);
	g_graphicsContext.SetFullScreenVideo(true);
	OutputDebugString("video_config\n");
	fs = options & 0x01;
	image_format   =  format;
	image_width    = width;
	image_height   = height;
	d_image_width  = d_width;
	d_image_height = d_height;

	// calculate the input frame aspect ratio
	CalculateFrameAspectRatio();
	choose_best_resolution(m_fps);

	fs=1;//fullscreen

	Directx_ManageDisplay();
	Directx_CreateOverlay(image_format);
	OutputDebugString("video_config() done\n");
	return 0;
}

//********************************************************************************************************
static unsigned int video_control(unsigned int request, void *data, ...)
{
	switch (request)
	{
	case VOCTRL_GET_IMAGE:
		//libmpcodecs Direct Rendering interface
		//You need to update mpi (mp_image.h) structure, for example,
		//look at vo_x11, vo_sdl, vo_xv or mga_common.
		return get_image((mp_image_t *)data);

	case VOCTRL_QUERY_FORMAT:
		return query_format(*((unsigned int*)data));

	case VOCTRL_DRAW_IMAGE:
		/*  replacement for the current draw_slice/draw_frame way of
		passing video frames. by implementing SET_IMAGE, you'll get
		image in mp_image struct instead of by calling draw_*.
		unless you return VO_TRUE for VOCTRL_DRAW_IMAGE call, the
		old-style draw_* functils will be called!
		Note: draw_slice is still mandatory, for per-slice rendering!
		*/

		return put_image( (mp_image_t*)data );
		//return VO_FALSE;//NOTIMPL;

	case VOCTRL_FULLSCREEN:
		{
			//TODO
			return VO_NOTIMPL;
		}
	case VOCTRL_PAUSE:
	case VOCTRL_RESUME:
	case VOCTRL_RESET:
	case VOCTRL_GUISUPPORT:
	case VOCTRL_SET_EQUALIZER:
	case VOCTRL_GET_EQUALIZER:
		break;

		return VO_TRUE;
	};
	return VO_NOTIMPL;
}


//********************************************************************************************************
static vo_info_t video_info =
{
	"XBOX Direct3D8 YV12 renderer",
	"directx",
	"Frodo/JCMarshall/Butcher",
	""
};

//********************************************************************************************************
vo_functions_t video_functions =
{
	&video_info,
	video_preinit,
	video_config,
	video_control,
	video_draw_frame,
	video_draw_slice,
	video_draw_osd,
	video_flip_page,
	video_check_events,
	video_uninit
};
