#include "stdafx.h"
#include "XBVideoConfig.h"


XBVideoConfig g_videoConfig;

XBVideoConfig::XBVideoConfig()
{
  bHasPAL = false;
  bHasNTSC = false;
#ifdef HAS_XBOX_D3D
  m_dwVideoFlags = XGetVideoFlags();
#else
  m_dwVideoFlags = 0;
#endif
}

XBVideoConfig::~XBVideoConfig()
{}

bool XBVideoConfig::HasPAL() const
{
#ifdef HAS_XBOX_D3D
  if (bHasPAL) return true;
  if (bHasNTSC) return false; // has NTSC (or PAL60) but not PAL
  return (XGetVideoStandard() == XC_VIDEO_STANDARD_PAL_I) != 0;
#else
  return true;
#endif
}

bool XBVideoConfig::HasPAL60() const
{
#ifdef HAS_XBOX_D3D
  return (m_dwVideoFlags & XC_VIDEO_FLAGS_PAL_60Hz) != 0;
#else
  return false; // no need for pal60 mode IMO
#endif
}

bool XBVideoConfig::HasNTSC() const
{
#ifdef HAS_XBOX_D3D
  return !HasPAL();
#else
  return true;
#endif
}

bool XBVideoConfig::HasWidescreen() const
{
#ifdef HAS_XBOX_D3D
  return (m_dwVideoFlags & XC_VIDEO_FLAGS_WIDESCREEN) != 0;
#else
  return true;
#endif
}

bool XBVideoConfig::Has480p() const
{
#ifdef HAS_XBOX_D3D
  return (m_dwVideoFlags & XC_VIDEO_FLAGS_HDTV_480p) != 0;
#else
  return true;
#endif
}

bool XBVideoConfig::Has720p() const
{
#ifdef HAS_XBOX_D3D
  return (m_dwVideoFlags & XC_VIDEO_FLAGS_HDTV_720p) != 0;
#else
  return true;
#endif
}

bool XBVideoConfig::Has1080i() const
{
#ifdef HAS_XBOX_D3D
  return (m_dwVideoFlags & XC_VIDEO_FLAGS_HDTV_1080i) != 0;
#else
  return true;
#endif
}

void XBVideoConfig::GetModes(LPDIRECT3D8 pD3D)
{
  bHasPAL = false;
  bHasNTSC = false;
  DWORD numModes = pD3D->GetAdapterModeCount(D3DADAPTER_DEFAULT);
  D3DDISPLAYMODE mode;
  CLog::Log(LOGINFO, "Available videomodes:");
  for ( DWORD i = 0; i < numModes; i++ )
  {
    pD3D->EnumAdapterModes( 0, i, &mode );

    // Skip modes we don't care about
    if ( mode.Format != D3DFMT_LIN_A8R8G8B8 )
      continue;
#ifdef HAS_XBOX_D3D
    if ( mode.Flags & D3DPRESENTFLAG_FIELD )
      continue;
    if ( mode.Flags & D3DPRESENTFLAG_10X11PIXELASPECTRATIO )
      continue;
    if ( mode.Flags & D3DPRESENTFLAG_EMULATE_REFRESH_RATE )
      continue;
#endif
    // ignore 640 wide modes
    if ( mode.Width < 720)
      continue;

    // If we get here, we found an acceptable mode
#ifdef HAS_XBOX_D3D
    CLog::Log(LOGINFO, "Found mode: %ix%i at %iHz, %s", mode.Width, mode.Height, mode.RefreshRate, mode.Flags & D3DPRESENTFLAG_WIDESCREEN ? "Widescreen" : "");
#else
    CLog::Log(LOGINFO, "Found mode: %ix%i at %iHz", mode.Width, mode.Height, mode.RefreshRate);
#endif
    if (mode.Width = 720 && mode.Height == 576 && mode.RefreshRate == 50)
      bHasPAL = true;
    if (mode.Width = 720 && mode.Height == 480 && mode.RefreshRate == 60)
      bHasNTSC = true;
  }
}

RESOLUTION XBVideoConfig::GetSafeMode() const
{
  if (HasPAL()) return PAL_4x3;
  return NTSC_4x3;
}

RESOLUTION XBVideoConfig::GetBestMode() const
{
  RESOLUTION bestRes;
  RESOLUTION resolutions[] = {HDTV_1080i, HDTV_720p, HDTV_480p_16x9, HDTV_480p_4x3, NTSC_16x9, NTSC_4x3, PAL_16x9, PAL_4x3, PAL60_16x9, PAL60_4x3, INVALID};
  UCHAR i = 0;
  while (resolutions[i] != INVALID)
  {
    if (IsValidResolution(resolutions[i]))
    {
      bestRes = resolutions[i];
      break;
    }
    i++;
  }
  return bestRes;
}

bool XBVideoConfig::IsValidResolution(RESOLUTION res) const
{
  bool bCanDoWidescreen = HasWidescreen();
  if (HasPAL())
  {
    bool bCanDoPAL60 = HasPAL60();
    if (res == PAL_4x3) return true;
    if (res == PAL_16x9 && bCanDoWidescreen) return true;
    if (res == PAL60_4x3 && bCanDoPAL60) return true;
    if (res == PAL60_16x9 && bCanDoPAL60 && bCanDoWidescreen) return true;
  }
  if (HasNTSC())
  {
    if (res == NTSC_4x3) return true;
    if (res == NTSC_16x9 && bCanDoWidescreen) return true;
    if (res == HDTV_480p_4x3 && Has480p()) return true;
    if (res == HDTV_480p_16x9 && Has480p() && bCanDoWidescreen) return true;
    if (res == HDTV_720p && Has720p()) return true;
    if (res == HDTV_1080i && Has1080i()) return true;
  }
  return false;
}

//pre: XBVideoConfig::GetModes has been called before this function
RESOLUTION XBVideoConfig::GetInitialMode(LPDIRECT3D8 pD3D, D3DPRESENT_PARAMETERS *p3dParams)
{
  bool bHasPal = HasPAL();
  DWORD numModes = pD3D->GetAdapterModeCount(D3DADAPTER_DEFAULT);
  D3DDISPLAYMODE mode;
  for ( DWORD i = 0; i < numModes; i++ )
  {
    pD3D->EnumAdapterModes( 0, i, &mode );

#ifdef HAS_XBOX_D3D
    // Skip modes we don't care about
    if ( mode.Format != D3DFMT_LIN_A8R8G8B8 )
      continue;
    if ( mode.Flags & D3DPRESENTFLAG_FIELD )
      continue;
    if ( mode.Flags & D3DPRESENTFLAG_10X11PIXELASPECTRATIO )
      continue;
    if ( mode.Flags & D3DPRESENTFLAG_EMULATE_REFRESH_RATE )
      continue;
#endif
    // ignore 640 wide modes
    if ( mode.Width < 720)
      continue;

    p3dParams->BackBufferWidth = mode.Width;
    p3dParams->BackBufferHeight = mode.Height;
    p3dParams->FullScreen_RefreshRateInHz = mode.RefreshRate;
#ifdef HAS_XBOX_D3D
    p3dParams->Flags = mode.Flags;
#endif
    if ((bHasPal) && ((mode.Height != 576) || (mode.RefreshRate != 50)))
    {
      continue;
    }
    //take the first available mode
#ifdef HAS_XBOX_D3D
    if (!HasWidescreen() && !(mode.Flags & D3DPRESENTFLAG_WIDESCREEN))
    {
      break;
    }
    if (HasWidescreen() && (mode.Flags & D3DPRESENTFLAG_WIDESCREEN))
    {
      break;
    }
#endif
  }

  if (HasPAL())
  {
    if (HasWidescreen() && (p3dParams->Flags & D3DPRESENTFLAG_WIDESCREEN))
    {
      return PAL_16x9;
    }
    else
    {
      return PAL_4x3;
    }
  }
  if (HasWidescreen() && (p3dParams->Flags & D3DPRESENTFLAG_WIDESCREEN))
  {
    return NTSC_16x9;
  }
  else
  {
    return NTSC_4x3;
  }
}

void XBVideoConfig::PrintInfo() const
{
#ifdef HAS_XBOX_D3D
  DWORD dwAVPack = XGetAVPack();
  CStdString strAVPack;
  if (dwAVPack == XC_AV_PACK_SCART) strAVPack = "Scart";
  else if (dwAVPack == XC_AV_PACK_HDTV) strAVPack = "HDTV";
  else if (dwAVPack == XC_AV_PACK_VGA) strAVPack = "VGA";
  else if (dwAVPack == XC_AV_PACK_RFU) strAVPack = "RF Unit";
  else if (dwAVPack == XC_AV_PACK_SVIDEO) strAVPack = "S-Video";
  else if (dwAVPack == XC_AV_PACK_STANDARD) strAVPack = "Standard";
  else strAVPack.Format("Unknown: %x", dwAVPack);
  CLog::Log(LOGINFO, "AV Pack: %s", strAVPack.c_str());
  CStdString strAVFlags;
  if (HasWidescreen()) strAVFlags += "Widescreen,";
  if (HasPAL60()) strAVFlags += "Pal60,";
  if (Has480p()) strAVFlags += "480p,";
  if (Has720p()) strAVFlags += "720p,";
  if (Has1080i()) strAVFlags += "1080i,";
  if (strAVFlags.size() > 1) strAVFlags = strAVFlags.Left(strAVFlags.size() - 1);
  CLog::Log(LOGINFO, "AV Flags: %s", strAVFlags.c_str());
#endif
}
