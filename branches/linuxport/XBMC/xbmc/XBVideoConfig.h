#pragma once

#define XC_VIDEO_FLAGS 8

class XBVideoConfig
{
public:
  XBVideoConfig();
  ~XBVideoConfig();

  bool HasPAL() const;
  bool HasPAL60() const;
  bool HasNTSC() const;
  bool HasWidescreen() const;
  bool HasLetterbox() const;
  bool Has480p() const;
  bool Has720p() const;
  bool Has1080i() const;
#ifndef HAS_SDL
  void GetModes(LPDIRECT3D8 pD3D);
  RESOLUTION GetInitialMode(LPDIRECT3D8 pD3D, D3DPRESENT_PARAMETERS *p3dParams);
#else
  void GetModes();
  RESOLUTION GetInitialMode();
#endif
  RESOLUTION GetSafeMode() const;
  RESOLUTION GetBestMode() const;
  bool IsValidResolution(RESOLUTION res) const;
  void PrintInfo() const;

  void Set480p(bool bEnable);
  void Set720p(bool bEnable);
  void Set1080i(bool bEnable);

  void SetNormal();
  void SetLetterbox(bool bEnable);
  void SetWidescreen(bool bEnable);

  bool NeedsSave();
  void Save();

private:
  bool bHasPAL;
  bool bHasNTSC;
  DWORD m_dwVideoFlags;
};

extern XBVideoConfig g_videoConfig;
