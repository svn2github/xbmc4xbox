/*!
	\file GUIProgressControl.h
	\brief 
	*/

#ifndef GUILIB_GUIPROGRESSCONTROL_H
#define GUILIB_GUIPROGRESSCONTROL_H

#pragma once
#include "gui3d.h"
#include "guicontrol.h"
#include "guimessage.h"
#include "guiImage.h"
#include "stdstring.h"
using namespace std;

/*!
	\ingroup controls
	\brief 
	*/
class CGUIProgressControl :
  public CGUIControl
{
public:
  CGUIProgressControl(DWORD dwParentID, DWORD dwControlId, int iPosX, int iPosY, DWORD dwWidth, DWORD dwHeight, CStdString& strBackGroundTexture,CStdString& strLeftTexture,CStdString& strMidTexture,CStdString& strRightTexture, CStdString& strOverlayTexture);
  virtual ~CGUIProgressControl(void);
  virtual void Render();
  virtual bool CanFocus() const;  
	virtual void PreAllocResources();
	virtual void AllocResources();
  virtual void FreeResources();
  virtual bool OnMessage(CGUIMessage& message);
	void				 SetPercentage(int iPercent);
	int 				 GetPercentage() const;
	const CStdString& GetBackGroundTextureName() const { return m_guiBackground.GetFileName();};
	const CStdString& GetBackTextureLeftName() const { return m_guiLeft.GetFileName();};
	const CStdString& GetBackTextureRightName() const { return m_guiRight.GetFileName();};
	const CStdString& GetBackTextureMidName() const { return m_guiMid.GetFileName();};
	const CStdString& GetBackTextureOverlayName() const { return m_guiOverlay.GetFileName();};
protected:
	CGUIImage				m_guiBackground;
	CGUIImage				m_guiLeft;
	CGUIImage				m_guiRight;
	CGUIImage				m_guiMid;
	CGUIImage				m_guiOverlay;
	int						m_iPercent;
};
#endif
