#include "graphiccontext.h"
#include "../xbmc/utils/log.h"

#define WIDE_SCREEN_COMPENSATIONY (FLOAT)1.2
#define WIDE_SCREEN_COMPENSATIONX (FLOAT)0.85

CGraphicContext g_graphicsContext;

CGraphicContext::CGraphicContext(void)
{
  m_bOSDOn=false;
	InitializeCriticalSection(&m_critSection);
  m_iScreenWidth=720;
  m_iScreenHeight=576;
	m_iScreenOffsetX=0;
	m_iScreenOffsetY=0;
  m_pd3dDevice=NULL;
  m_pd3dParams=NULL;
  m_pResInfo=NULL;
  m_dwID=0;
  m_strMediaDir="D:\\media";
	m_bShowPreviewWindow=false;
	m_bCalibrating=false;
	m_Resolution=INVALID;
	m_pCallback=NULL;
}

CGraphicContext::~CGraphicContext(void)
{
	DeleteCriticalSection(&m_critSection);
}

void CGraphicContext::SetOSDOn(bool bOnOff)
{
  m_bOSDOn=bOnOff;
}
void CGraphicContext::SetD3DDevice(LPDIRECT3DDEVICE8 p3dDevice)
{
	m_pd3dDevice=p3dDevice;
}

void CGraphicContext::SetD3DParameters(D3DPRESENT_PARAMETERS *p3dParams, RESOLUTION_INFO *pResInfo)
{
	m_pd3dParams=p3dParams;
	m_pResInfo=pResInfo;
}

void CGraphicContext::SendMessage(CGUIMessage& message)
{
  if (!m_pCallback) return;
  m_pCallback->SendMessage(message);
}

void CGraphicContext::setMessageSender(IMsgSenderCallback* pCallback)
{
  m_pCallback=pCallback;
}

DWORD CGraphicContext::GetNewID()
{
  m_dwID++;
  return  m_dwID;
}

bool CGraphicContext::IsWidescreen() const
{
	return m_bWidescreen;
}

void CGraphicContext::Correct(float& fCoordinateX, float& fCoordinateY)  const
{
	int	iOSDYOffset;

	if (m_bOSDOn)	// full screen video, so adjust with OSD offset instead
	{
		iOSDYOffset = m_pResInfo[m_Resolution].iOSDYOffset;
		fCoordinateX  += (float)m_iScreenOffsetX;
		fCoordinateY  += (float)iOSDYOffset;
	}
	else					// normal UI, adjust with UI offsets
	{
		fCoordinateX  += (float)m_iScreenOffsetX;
		fCoordinateY  += (float)m_iScreenOffsetY;
	}
}

void CGraphicContext::Scale(float& fCoordinateX, float& fCoordinateY, float& fWidth, float& fHeight) const
{
	if (!m_bWidescreen) return;

	// for widescreen images need 2 b rescaled to 66% width, 100% height
}


void CGraphicContext::SetViewPort(float fx, float fy , float fwidth, float fheight)
{
	Correct(fx,fy);
	D3DVIEWPORT8 newviewport;
	Get3DDevice()->GetViewport(&m_oldviewport);
	newviewport.X      = (DWORD)fx;
	newviewport.Y			 = (DWORD)fy;
	newviewport.Width  = (DWORD)(fwidth);
	newviewport.Height = (DWORD)(fheight);
	newviewport.MinZ   = 0.0f;
	newviewport.MaxZ   = 1.0f;
	Get3DDevice()->SetViewport(&newviewport);
}

void CGraphicContext::RestoreViewPort()
{
	Get3DDevice()->SetViewport(&m_oldviewport);
}

const RECT&	CGraphicContext::GetViewWindow() const
{
	return m_videoRect;
}
void CGraphicContext::SetViewWindow(const RECT&	rc) 
{
	m_videoRect.left  = rc.left;
	m_videoRect.top   = rc.top;
	m_videoRect.right = rc.right;
	m_videoRect.bottom= rc.bottom;
	if (m_bShowPreviewWindow && !m_bFullScreenVideo)
	{
		D3DRECT d3dRC;
		d3dRC.x1=rc.left;
		d3dRC.x2=rc.right;
		d3dRC.y1=rc.top;
		d3dRC.y2=rc.bottom;
		Get3DDevice()->Clear( 1, &d3dRC, D3DCLEAR_TARGET|D3DCLEAR_ZBUFFER|D3DCLEAR_STENCIL, 0x00010001, 1.0f, 0L );
	}
}

void CGraphicContext::SetFullScreenVideo(bool bOnOff)
{
	Lock();
	m_bFullScreenVideo=bOnOff;
	Unlock();
}

bool CGraphicContext::IsFullScreenVideo() const
{
	return m_bFullScreenVideo;
}

void CGraphicContext::SetOffset(int iXoffset, int iYoffset)
{
	m_iScreenOffsetX=iXoffset;
	m_iScreenOffsetY=iYoffset;
}
void CGraphicContext::Lock()
{
	EnterCriticalSection(&m_critSection);
}

void CGraphicContext::Unlock()
{
	LeaveCriticalSection(&m_critSection);
}

void CGraphicContext::EnablePreviewWindow(bool bEnable)
{
	m_bShowPreviewWindow=bEnable;
}


bool CGraphicContext::IsCalibrating() const
{
	return m_bCalibrating;
}

void CGraphicContext::SetCalibrating(bool bOnOff)
{
	m_bCalibrating=bOnOff;
}

bool CGraphicContext::IsValidResolution(RESOLUTION res)
{
	DWORD dwStandard=XGetVideoStandard();
	DWORD dwFlags=XGetVideoFlags();
	bool bCanDoWidescreen = (dwFlags & XC_VIDEO_FLAGS_WIDESCREEN)!=0;
	if (dwStandard==XC_VIDEO_STANDARD_PAL_I)
	{
		bool bCanDoPAL60 = (dwFlags & XC_VIDEO_FLAGS_PAL_60Hz)!=0;
		if (res == PAL_4x3) return true;
		if (res == PAL_16x9 && bCanDoWidescreen) return true;
		if (res == PAL60_4x3 && bCanDoPAL60) return true;
		if (res == PAL60_16x9 && bCanDoPAL60 && bCanDoWidescreen) return true;
	}
	else	// NTSC Screenmodes
	{
		if (res == NTSC_4x3) return true;
		if (res == NTSC_16x9 && bCanDoWidescreen) return true;
		if (res == HDTV_480p_4x3 && (dwFlags&XC_VIDEO_FLAGS_HDTV_480p)) return true;
		if (res == HDTV_480p_16x9 && (dwFlags&XC_VIDEO_FLAGS_HDTV_480p) && bCanDoWidescreen) return true;
		if (res == HDTV_720p && (dwFlags&XC_VIDEO_FLAGS_HDTV_720p)) return true;
		if (res == HDTV_1080i && (dwFlags&XC_VIDEO_FLAGS_HDTV_1080i)) return true;
	}
	return false;
}

void CGraphicContext::GetAllowedResolutions(vector<RESOLUTION> &res, bool bAllowPAL60)
{
	DWORD dwStandard=XGetVideoStandard();
	DWORD dwFlags=XGetVideoFlags();
	bool bCanDoWidescreen = (dwFlags & XC_VIDEO_FLAGS_WIDESCREEN)!=0;
	res.clear();
	if (dwStandard==XC_VIDEO_STANDARD_PAL_I)
	{
		res.push_back(PAL_4x3);
		if (bCanDoWidescreen) res.push_back(PAL_16x9);
		if (bAllowPAL60 && (dwFlags & XC_VIDEO_FLAGS_PAL_60Hz))
		{
			res.push_back(PAL60_4x3);
			if (bCanDoWidescreen) res.push_back(PAL60_16x9);
		}
	}
	else
	{
		res.push_back(NTSC_4x3);
		if (bCanDoWidescreen) res.push_back(NTSC_16x9);
		if (dwFlags & XC_VIDEO_FLAGS_HDTV_480p)
		{
			res.push_back(HDTV_480p_4x3);
			if (bCanDoWidescreen) res.push_back(HDTV_480p_16x9);
		}
		if (dwFlags & XC_VIDEO_FLAGS_HDTV_720p)
			res.push_back(HDTV_720p);
		if (dwFlags & XC_VIDEO_FLAGS_HDTV_1080i)
			res.push_back(HDTV_1080i);
	}
}

void CGraphicContext::SetGUIResolution(RESOLUTION &res)
{
	SetVideoResolution(res);
	m_iScreenWidth=m_pd3dParams->BackBufferWidth ;
	m_iScreenHeight=m_pd3dParams->BackBufferHeight;
	m_bWidescreen=(m_pd3dParams->Flags & D3DPRESENTFLAG_WIDESCREEN)!=0;
}

void CGraphicContext::SetVideoResolution(RESOLUTION &res)
{
	if (!IsValidResolution(res))
	{	// Choose a failsafe resolution that we can actually display
		if (XGetVideoStandard()==XC_VIDEO_STANDARD_PAL_I)
			res = PAL_4x3;
		else
			res = NTSC_4x3;
	}
	bool NeedReset = false;
	if (m_bFullScreenVideo)
	{
		if (m_pd3dParams->FullScreen_PresentationInterval != D3DPRESENT_INTERVAL_ONE)
		{
			m_pd3dParams->FullScreen_PresentationInterval = D3DPRESENT_INTERVAL_ONE;
			NeedReset = true;
		}
	}
	else
	{
		if (m_pd3dParams->FullScreen_PresentationInterval != D3DPRESENT_INTERVAL_IMMEDIATE)
		{
			m_pd3dParams->FullScreen_PresentationInterval = D3DPRESENT_INTERVAL_IMMEDIATE;
			NeedReset = true;
		}
	}
	if (m_Resolution != res)
	{	
		NeedReset = true;
		m_pd3dParams->BackBufferWidth = m_pResInfo[res].iWidth;
		m_pd3dParams->BackBufferHeight = m_pResInfo[res].iHeight;
		m_pd3dParams->Flags = m_pResInfo[res].dwFlags;
		if (res == PAL60_4x3 || res == PAL60_16x9)
		{
      if (m_pd3dParams->BackBufferWidth <= 720 && m_pd3dParams->BackBufferHeight <=480)
      {
			  m_pd3dParams->FullScreen_RefreshRateInHz = 60;
      }
      else
        m_pd3dParams->FullScreen_RefreshRateInHz = 0;
      {
      }
		}
		else
		{
			m_pd3dParams->FullScreen_RefreshRateInHz = 0;
		}
  }
	if (NeedReset && m_pd3dDevice)
	{
		m_pd3dDevice->Reset(m_pd3dParams);
	}
  if (m_pd3dDevice)
  {
	    m_pd3dDevice->Clear( 0L, NULL, D3DCLEAR_TARGET|D3DCLEAR_ZBUFFER|D3DCLEAR_STENCIL, 0x00010001, 1.0f, 0L );
	    m_pd3dDevice->Present( NULL, NULL, NULL, NULL );
	}
	m_Resolution=res;
}

RESOLUTION CGraphicContext::GetVideoResolution() const
{
	return m_Resolution;
}

void CGraphicContext::ScaleRectToScreenResolution(DWORD& left, DWORD&  top, DWORD& right, DWORD& bottom)
{
	float fPercentX = ((float)m_iScreenWidth ) / 720.0f;
	float fPercentY = ((float)m_iScreenHeight) / 576.0f;
	left   = (DWORD) ( (float(left))	 * fPercentX); 
	top    = (DWORD) ( (float(top))		 * fPercentY); 
	right  = (DWORD) ( (float(right))	 * fPercentX); 
	bottom = (DWORD) ( (float(bottom)) * fPercentY); 
}
void CGraphicContext::ScalePosToScreenResolution(DWORD& x, DWORD&  y)
{
	float fPercentX = ((float)m_iScreenWidth ) / 720.0f;
	float fPercentY = ((float)m_iScreenHeight) / 576.0f;
	x  = (DWORD) ( (float(x))		 * fPercentX); 
	y  = (DWORD) ( (float(y))		 * fPercentY); 
}

void CGraphicContext::ResetScreenParameters(RESOLUTION res)
{
	// 1080i (540p for overlays)
	switch (res)
	{
		case HDTV_1080i:
			m_pResInfo[res].Overscan.left = 0;
			m_pResInfo[res].Overscan.top = 0;
			m_pResInfo[res].Overscan.width = 1920;
			m_pResInfo[res].Overscan.height = 1080;
			m_pResInfo[res].iSubtitles = 1080;
			m_pResInfo[res].iOSDYOffset = 0;
			m_pResInfo[res].iWidth = 1920;
			m_pResInfo[res].iHeight = 1080;
			m_pResInfo[res].dwFlags = D3DPRESENTFLAG_INTERLACED|D3DPRESENTFLAG_WIDESCREEN;
			m_pResInfo[res].fPixelRatio = 1.0f;
			strcpy(m_pResInfo[res].strMode,"1080i 16:9");
		break;
		case HDTV_720p:
			m_pResInfo[res].Overscan.left = 0;
			m_pResInfo[res].Overscan.top = 0;
			m_pResInfo[res].Overscan.width = 1280;
			m_pResInfo[res].Overscan.height = 720;
			m_pResInfo[res].iSubtitles = 720;
			m_pResInfo[res].iOSDYOffset = 0;
			m_pResInfo[res].iWidth = 1280;
			m_pResInfo[res].iHeight = 720;
			m_pResInfo[res].dwFlags = D3DPRESENTFLAG_PROGRESSIVE|D3DPRESENTFLAG_WIDESCREEN;
			m_pResInfo[res].fPixelRatio = 1.0f;
			strcpy(m_pResInfo[res].strMode,"720p 16:9");
		break;
		case HDTV_480p_4x3:
			m_pResInfo[res].Overscan.left = 0;
			m_pResInfo[res].Overscan.top = 0;
			m_pResInfo[res].Overscan.width = 720;
			m_pResInfo[res].Overscan.height = 480;
			m_pResInfo[res].iSubtitles = 480;
			m_pResInfo[res].iOSDYOffset = 0;
			m_pResInfo[res].iWidth = 720;
			m_pResInfo[res].iHeight = 480;
			m_pResInfo[res].dwFlags = D3DPRESENTFLAG_PROGRESSIVE;
			m_pResInfo[res].fPixelRatio = 72.0f/79.0f;
			strcpy(m_pResInfo[res].strMode,"480p 4:3");
		break;
		case HDTV_480p_16x9:
			m_pResInfo[res].Overscan.left = 0;
			m_pResInfo[res].Overscan.top = 0;
			m_pResInfo[res].Overscan.width = 720;
			m_pResInfo[res].Overscan.height = 480;
			m_pResInfo[res].iSubtitles = 480;
			m_pResInfo[res].iOSDYOffset = 0;
			m_pResInfo[res].iWidth = 720;
			m_pResInfo[res].iHeight = 480;
			m_pResInfo[res].dwFlags = D3DPRESENTFLAG_PROGRESSIVE|D3DPRESENTFLAG_WIDESCREEN;
			m_pResInfo[res].fPixelRatio = 72.0f/79.0f*4.0f/3.0f;
			strcpy(m_pResInfo[res].strMode,"480p 16:9");
		break;
		case NTSC_4x3:
			m_pResInfo[res].Overscan.left = 0;
			m_pResInfo[res].Overscan.top = 0;
			m_pResInfo[res].Overscan.width = 720;
			m_pResInfo[res].Overscan.height = 480;
			m_pResInfo[res].iSubtitles = 480;
			m_pResInfo[res].iOSDYOffset = 0;
			m_pResInfo[res].iWidth = 720;
			m_pResInfo[res].iHeight = 480;
			m_pResInfo[res].dwFlags = 0;
			m_pResInfo[res].fPixelRatio = 72.0f/79.0f;
			strcpy(m_pResInfo[res].strMode,"NTSC 4:3");
		break;
		case NTSC_16x9:
			m_pResInfo[res].Overscan.left = 0;
			m_pResInfo[res].Overscan.top = 0;
			m_pResInfo[res].Overscan.width = 720;
			m_pResInfo[res].Overscan.height = 480;
			m_pResInfo[res].iSubtitles = 480;
			m_pResInfo[res].iOSDYOffset = 0;
			m_pResInfo[res].iWidth = 720;
			m_pResInfo[res].iHeight = 480;
			m_pResInfo[res].dwFlags = D3DPRESENTFLAG_WIDESCREEN;
			m_pResInfo[res].fPixelRatio = 72.0f/79.0f*4.0f/3.0f;
			strcpy(m_pResInfo[res].strMode,"NTSC 16:9");
		break;
		case PAL_4x3:
			m_pResInfo[res].Overscan.left = 0;
			m_pResInfo[res].Overscan.top = 0;
			m_pResInfo[res].Overscan.width = 720;
			m_pResInfo[res].Overscan.height = 576;
			m_pResInfo[res].iSubtitles = 576;
			m_pResInfo[res].iOSDYOffset = 0;
			m_pResInfo[res].iWidth = 720;
			m_pResInfo[res].iHeight = 576;
			m_pResInfo[res].dwFlags = 0;
			m_pResInfo[res].fPixelRatio = 128.0f/117.0f;
			strcpy(m_pResInfo[res].strMode,"PAL 4:3");
		break;
		case PAL_16x9:
			m_pResInfo[res].Overscan.left = 0;
			m_pResInfo[res].Overscan.top = 0;
			m_pResInfo[res].Overscan.width = 720;
			m_pResInfo[res].Overscan.height = 576;
			m_pResInfo[res].iSubtitles = 576;
			m_pResInfo[res].iOSDYOffset = 0;
			m_pResInfo[res].iWidth = 720;
			m_pResInfo[res].iHeight = 576;
			m_pResInfo[res].dwFlags = D3DPRESENTFLAG_WIDESCREEN;
			m_pResInfo[res].fPixelRatio = 128.0f/117.0f*4.0f/3.0f;
			strcpy(m_pResInfo[res].strMode,"PAL 16:9");
		break;
		case PAL60_4x3:
			m_pResInfo[res].Overscan.left = 0;
			m_pResInfo[res].Overscan.top = 0;
			m_pResInfo[res].Overscan.width = 720;
			m_pResInfo[res].Overscan.height = 480;
			m_pResInfo[res].iSubtitles = 480;
			m_pResInfo[res].iOSDYOffset = -75;
			m_pResInfo[res].iWidth = 720;
			m_pResInfo[res].iHeight = 480;
			m_pResInfo[res].dwFlags = 0;
			m_pResInfo[res].fPixelRatio = 72.0f/79.0f;
			strcpy(m_pResInfo[res].strMode,"PAL60 4:3");
		break;
		case PAL60_16x9:
			m_pResInfo[res].Overscan.left = 0;
			m_pResInfo[res].Overscan.top = 0;
			m_pResInfo[res].Overscan.width = 720;
			m_pResInfo[res].Overscan.height = 480;
			m_pResInfo[res].iSubtitles = 480;
			m_pResInfo[res].iOSDYOffset = -75;
			m_pResInfo[res].iWidth = 720;
			m_pResInfo[res].iHeight = 480;
			m_pResInfo[res].dwFlags = D3DPRESENTFLAG_WIDESCREEN;
			m_pResInfo[res].fPixelRatio = 72.0f/79.0f*4.0f/3.0f;
			strcpy(m_pResInfo[res].strMode,"PAL60 16:9");
		break;
	}
}
