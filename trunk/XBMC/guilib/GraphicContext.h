/*!
\file GraphicContext.h
\brief 
*/

#ifndef GUILIB_GRAPHICCONTEXT_H
#define GUILIB_GRAPHICCONTEXT_H

#pragma once

#include <vector>
#include <stack>

#include "IMsgSenderCallback.h"
#include "common/mouse.h"
#include "../xbmc/utils/CriticalSection.h"
#include "../xbmc/utils/SingleLock.h"

/*!
 \ingroup graphics
 \brief 
 */
enum RESOLUTION {
  INVALID = -1,
  HDTV_1080i = 0,
  HDTV_720p = 1,
  HDTV_480p_4x3 = 2,
  HDTV_480p_16x9 = 3,
  NTSC_4x3 = 4,
  NTSC_16x9 = 5,
  PAL_4x3 = 6,
  PAL_16x9 = 7,
  PAL60_4x3 = 8,
  PAL60_16x9 = 9,
  AUTORES = 10
};

/*!
 \ingroup graphics
 \brief 
 */
struct OVERSCAN
{
  int left;
  int top;
  int right;
  int bottom;
};

/*!
 \ingroup graphics
 \brief 
 */
struct RESOLUTION_INFO
{
  OVERSCAN Overscan;
  OVERSCAN GUIOverscan;
  int iWidth;
  int iHeight;
  int iSubtitles;
  int iOSDYOffset; // Y offset for OSD (applied to all Y pos in skin)
  DWORD dwFlags;
  float fPixelRatio;
  char strMode[11];
};

class CAttribute
{
public:
  CAttribute()
  {
    Reset();
  };
  void Reset()
  {
    alpha = 255;
    offsetX = offsetY = 0;
  };
  DWORD alpha;
  int offsetX;
  int offsetY;
};

/*!
 \ingroup graphics
 \brief 
 */
class CGraphicContext : public CCriticalSection
{
public:
  CGraphicContext(void);
  virtual ~CGraphicContext(void);
  LPDIRECT3DDEVICE8 Get3DDevice() { return m_pd3dDevice; }
  void SetD3DDevice(LPDIRECT3DDEVICE8 p3dDevice);
  //  void         GetD3DParameters(D3DPRESENT_PARAMETERS &params);
  void SetD3DParameters(D3DPRESENT_PARAMETERS *p3dParams);
  int GetWidth() const { return m_iScreenWidth; }
  int GetHeight() const { return m_iScreenHeight; }
  bool SendMessage(CGUIMessage& message);
  void setMessageSender(IMsgSenderCallback* pCallback);
  DWORD GetNewID();
  const CStdString& GetMediaDir() const { return m_strMediaDir; }
  void SetMediaDir(const CStdString& strMediaDir) { m_strMediaDir = strMediaDir; }
  bool IsWidescreen() const { return m_bWidescreen; }
  bool SetViewPort(float fx, float fy , float fwidth, float fheight, bool intersectPrevious = false);
  void RestoreViewPort();
  const RECT& GetViewWindow() const;
  void SetViewWindow(const RECT& rc) ;
  void SetFullScreenViewWindow(RESOLUTION &res);
  void ClipToViewWindow();
  void SetFullScreenVideo(bool bOnOff);
  bool IsFullScreenVideo() const;
  bool IsCalibrating() const;
  void SetCalibrating(bool bOnOff);
  void SetGUIResolution(RESOLUTION &res);
  void GetAllowedResolutions(std::vector<RESOLUTION> &res, bool bAllowPAL60 = false);
  bool IsValidResolution(RESOLUTION res);
  void SetVideoResolution(RESOLUTION &res, BOOL NeedZ = FALSE);
  RESOLUTION GetVideoResolution() const;
  void ResetOverscan(RESOLUTION res, OVERSCAN &overscan);
  void ResetScreenParameters(RESOLUTION res);
  void Lock() { EnterCriticalSection(*this); }
  void Unlock() { LeaveCriticalSection(*this); }
  void EnablePreviewWindow(bool bEnable);
  void ScalePosToScreenResolution(DWORD& x, DWORD& y, RESOLUTION res);
  void ScaleRectToScreenResolution(DWORD& left, DWORD& top, DWORD& right, DWORD& bottom, RESOLUTION res);
  void ScaleXCoord(DWORD &x, RESOLUTION res);
  void ScaleXCoord(int &x, RESOLUTION res);
  void ScaleXCoord(long &x, RESOLUTION res);
  void ScaleYCoord(DWORD &x, RESOLUTION res);
  void ScaleYCoord(int &x, RESOLUTION res);
  void ScaleYCoord(long &x, RESOLUTION res);
  float GetPixelRatio(RESOLUTION iRes) const;
  void CaptureStateBlock();
  void ApplyStateBlock();
  void Clear();

  // output scaling
  void SetScalingResolution(RESOLUTION res, int posX, int posY, bool needsScaling);  // sets the input skin resolution.
  inline float ScaleFinalXCoord(float x) const;
  inline float ScaleFinalYCoord(float y) const;
  inline float ScaleFinalX() const { return m_windowScaleX; };
  inline float ScaleFinalY() const { return m_windowScaleY; };
  inline DWORD MergeAlpha(DWORD color) const;
  void ResetControlAttributes() { m_controlAttribute.Reset(); };
  void SetControlAlpha(DWORD alpha) { m_controlAttribute.alpha = alpha; };
  void SetControlOffset(float offsetX, float offsetY) { m_controlAttribute.offsetX = (int)offsetX; m_controlAttribute.offsetY = (int)offsetY; };
  void SetWindowAttributes(CAttribute &attribute) { m_windowAttribute = attribute; };

protected:
  IMsgSenderCallback* m_pCallback;
  LPDIRECT3DDEVICE8 m_pd3dDevice;
  D3DPRESENT_PARAMETERS* m_pd3dParams;
  int m_iScreenHeight;
  int m_iScreenWidth;
  DWORD m_dwID;
  bool m_bWidescreen;
  CStdString m_strMediaDir;
  stack<D3DVIEWPORT8*> m_viewStack;
  RECT m_videoRect;
  bool m_bFullScreenVideo;
  bool m_bShowPreviewWindow;
  bool m_bCalibrating;
  RESOLUTION m_Resolution;
  DWORD m_stateBlock;

private:
  float m_windowScaleX;
  float m_windowScaleY;
  float m_windowPosX;
  float m_windowPosY;
  CAttribute m_controlAttribute;
  CAttribute m_windowAttribute;
};

/*!
 \ingroup graphics
 \brief 
 */
extern CGraphicContext g_graphicsContext;
#endif
