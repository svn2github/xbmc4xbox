/*!
\file GraphicContext.h
\brief 
*/

#ifndef GUILIB_GRAPHICCONTEXT_H
#define GUILIB_GRAPHICCONTEXT_H

#pragma once

#include <vector>
#include <stack>
#include "../xbmc/utils/CriticalSection.h"  // base class
#include "TransformMatrix.h"                // for the members m_guiTransform etc.

// forward definitions
class IMsgSenderCallback;
class CGUIMessage;

#ifdef _XBOX
#include "common/XBoxMouse.h"
#else
#include "common/DirectInputMouse.h"
#endif

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
  int iWidth;
  int iHeight;
  int iSubtitles;
  DWORD dwFlags;
  float fPixelRatio;
  char strMode[11];
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
  int GetFPS() const;
  bool SendMessage(CGUIMessage& message);
  void setMessageSender(IMsgSenderCallback* pCallback);
  DWORD GetNewID();
  const CStdString& GetMediaDir() const { return m_strMediaDir; }
  void SetMediaDir(const CStdString& strMediaDir) { m_strMediaDir = strMediaDir; }
  bool IsWidescreen() const { return m_bWidescreen; }
  bool SetViewPort(float fx, float fy , float fwidth, float fheight, bool intersectPrevious = false);
  void RestoreViewPort();
  const RECT& GetViewWindow() const;
  void SetViewWindow(float left, float top, float right, float bottom);
  void SetFullScreenViewWindow(RESOLUTION &res);
  void ClipToViewWindow();
  void SetFullScreenVideo(bool bOnOff);
  bool IsFullScreenVideo() const;
  bool IsCalibrating() const;
  void SetCalibrating(bool bOnOff);
  void SetGUIResolution(RESOLUTION &res, bool forceClear = false);
  void GetAllowedResolutions(std::vector<RESOLUTION> &res, bool bAllowPAL60 = false);
  bool IsValidResolution(RESOLUTION res);
  void SetVideoResolution(RESOLUTION &res, BOOL NeedZ = FALSE, bool forceClear = false);
  RESOLUTION GetVideoResolution() const;
  void SetScreenFilters(bool useFullScreenFilters);
  void ResetOverscan(RESOLUTION res, OVERSCAN &overscan);
  void ResetScreenParameters(RESOLUTION res);
  void Lock() { EnterCriticalSection(*this); }
  void Unlock() { LeaveCriticalSection(*this); }
  void EnablePreviewWindow(bool bEnable);
  float GetPixelRatio(RESOLUTION iRes) const;
  void CaptureStateBlock();
  void ApplyStateBlock();
  void Clear();

  // output scaling
  void SetScalingResolution(RESOLUTION res, float posX, float posY, bool needsScaling);  // sets the input skin resolution.
  float GetScalingPixelRatio() const;

  inline float ScaleFinalXCoord(float x, float y) const;
  inline float ScaleFinalYCoord(float x, float y) const;
  inline void ScaleFinalCoords(float &x, float &y) const;
  inline float ScaleFinalX() const { return m_windowScaleX; };
  inline float ScaleFinalY() const { return m_windowScaleY; };
  inline DWORD MergeAlpha(DWORD color) const;
  inline void SetWindowTransform(const TransformMatrix &matrix)
  { // reset the group transform stack
    while (m_groupTransform.size())
      m_groupTransform.pop();
    m_groupTransform.push(m_guiTransform * matrix);
    m_finalTransform = m_groupTransform.top();
  }
  inline void SetControlTransform(const TransformMatrix &matrix)
  {
    m_finalTransform = m_groupTransform.top() * matrix;
  };
  inline void AddGroupTransform(const TransformMatrix &matrix)
  { // add to the stack
    m_groupTransform.push(m_groupTransform.top() * matrix);
    m_finalTransform = m_groupTransform.top();
  };
  inline void RemoveGroupTransform()
  { // remove from stack
    if (m_groupTransform.size()) m_groupTransform.pop();
  };

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
  RESOLUTION m_windowResolution;
  float m_windowScaleX;
  float m_windowScaleY;

  TransformMatrix m_guiTransform;
  TransformMatrix m_finalTransform;
  stack<TransformMatrix> m_groupTransform;
};

/*!
 \ingroup graphics
 \brief 
 */
extern CGraphicContext g_graphicsContext;
#endif
