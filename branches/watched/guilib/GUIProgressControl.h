/*!
\file GUIProgressControl.h
\brief 
*/

#ifndef GUILIB_GUIPROGRESSCONTROL_H
#define GUILIB_GUIPROGRESSCONTROL_H

#pragma once

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

#include "guiImage.h"

/*!
 \ingroup controls
 \brief 
 */
class CGUIProgressControl :
      public CGUIControl
{
public:
  CGUIProgressControl(DWORD dwParentID, DWORD dwControlId, float posX, float posY, 
                      float width, float height, const CImage& backGroundTexture, 
                      const CImage& leftTexture, const CImage& midTexture, 
                      const CImage& rightTexture, const CImage& overlayTexture, 
                      float min, float max, bool reveal=false);
  virtual ~CGUIProgressControl(void);
  virtual void Render();
  virtual bool CanFocus() const;
  virtual void PreAllocResources();
  virtual void AllocResources();
  virtual void FreeResources();
  virtual void DynamicResourceAlloc(bool bOnOff);
  virtual bool OnMessage(CGUIMessage& message);
  virtual void SetPosition(float posX, float posY);
  virtual void SetColorDiffuse(const CGUIInfoColor &color);
  void SetPercentage(float fPercent);
  void SetInfo(int iInfo);
  int GetInfo() const {return m_iInfoCode;};

  float GetPercentage() const;
protected:
  CGUIImage m_guiBackground;
  CGUIImage m_guiLeft;
  CGUIImage m_guiMid;
  CGUIImage m_guiRight;
  CGUIImage m_guiOverlay;
  float m_RangeMin;
  float m_RangeMax;
  int m_iInfoCode;
  float m_fPercent;
  bool m_bReveal;
};
#endif
