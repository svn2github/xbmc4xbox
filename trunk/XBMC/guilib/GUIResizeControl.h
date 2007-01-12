/*!
\file GUIRESIZEControl.h
\brief 
*/

#ifndef GUILIB_GUIRESIZECONTROL_H
#define GUILIB_GUIRESIZECONTROL_H

#pragma once

#include "GUIImage.h"

#define DIRECTION_NONE 0
#define DIRECTION_UP 1
#define DIRECTION_DOWN 2
#define DIRECTION_LEFT 3
#define DIRECTION_RIGHT 4

/*!
 \ingroup controls
 \brief 
 */
class CGUIResizeControl : public CGUIControl
{
public:
  CGUIResizeControl(DWORD dwParentID, DWORD dwControlId,
                    float posX, float posY, float width, float height,
                    const CImage& textureFocus, const CImage& textureNoFocus);

  virtual ~CGUIResizeControl(void);

  virtual void Render();
  virtual bool OnAction(const CAction &action);
  virtual void OnUp();
  virtual void OnDown();
  virtual void OnLeft();
  virtual void OnRight();
  virtual bool OnMouseDrag();
  virtual bool OnMouseClick(DWORD dwButton);
  virtual void PreAllocResources();
  virtual void AllocResources();
  virtual void FreeResources();
  virtual void DynamicResourceAlloc(bool bOnOff);
  virtual void SetPosition(float posX, float posY);
  virtual void SetColorDiffuse(D3DCOLOR color);
  void SetLimits(float x1, float y1, float x2, float y2);

protected:
  virtual void Update() ;
  void SetAlpha(unsigned char alpha);
  void UpdateSpeed(int nDirection);
  void Resize(float x, float y);
  CGUIImage m_imgFocus;
  CGUIImage m_imgNoFocus;
  DWORD m_dwFrameCounter;
  DWORD m_dwLastMoveTime;
  int m_nDirection;
  float m_fSpeed;
  float m_fAnalogSpeed;
  float m_fMaxSpeed;
  float m_fAcceleration;
  float m_x1, m_x2, m_y1, m_y2;
};
#endif
