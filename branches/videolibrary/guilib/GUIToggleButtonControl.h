/*!
\file GUIToggleButtonControl.h
\brief 
*/

#ifndef GUILIB_GUITOGGLEBUTTONCONTROL_H
#define GUILIB_GUITOGGLEBUTTONCONTROL_H

#pragma once

#include "GUIButtonControl.h"

/*!
 \ingroup controls
 \brief 
 */
class CGUIToggleButtonControl : public CGUIButtonControl
{
public:
  CGUIToggleButtonControl(DWORD dwParentID, DWORD dwControlId, int iPosX, int iPosY, DWORD dwWidth, DWORD dwHeight, const CStdString& strTextureFocus, const CStdString& strTextureNoFocus, const CStdString& strAltTextureFocus, const CStdString& strAltTextureNoFocus, const CLabelInfo &labelInfo);
  virtual ~CGUIToggleButtonControl(void);

  virtual void Render();
  virtual bool OnAction(const CAction &action);
  virtual void PreAllocResources();
  virtual void AllocResources();
  virtual void FreeResources();
  virtual void DynamicResourceAlloc(bool bOnOff);
  virtual void SetPosition(int iPosX, int iPosY);
  virtual void SetWidth(int iWidth);
  virtual void SetHeight(int iHeight);
  virtual void SetColourDiffuse(D3DCOLOR colour);
  void SetLabel(const string& strLabel);
  void SetAltLabel(const string& label);
  const string& GetAltLabel() const { return m_selectButton.GetLabel(); };
  virtual const string& GetLabel() const;
  const CStdString& GetTextureAltFocusName() const { return m_selectButton.GetTextureFocusName(); };
  const CStdString& GetTextureAltNoFocusName() const { return m_selectButton.GetTextureNoFocusName(); };
  int GetToggleSelect() const { return m_toggleSelect; };
  void SetToggleSelect(int toggleSelect) { m_toggleSelect = toggleSelect; };

protected:
  virtual void Update();
  CGUIButtonControl m_selectButton;
  int m_toggleSelect;
};
#endif
