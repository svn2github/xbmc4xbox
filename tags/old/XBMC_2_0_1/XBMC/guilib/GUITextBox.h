/*!
\file GUITextBox.h
\brief 
*/

#ifndef GUILIB_GUITEXTBOX_H
#define GUILIB_GUITEXTBOX_H

#pragma once
#include "GUISpinControl.h"
#include "GUIButtonControl.h"
#include "GUIListItem.h"


/*!
 \ingroup controls
 \brief 
 */
class CGUITextBox : public CGUIControl
{
public:
  CGUITextBox(DWORD dwParentID, DWORD dwControlId, int iPosX, int iPosY, DWORD dwWidth, DWORD dwHeight,
              DWORD dwSpinWidth, DWORD dwSpinHeight,
              const CStdString& strUp, const CStdString& strDown,
              const CStdString& strUpFocus, const CStdString& strDownFocus,
              const CLabelInfo& spinInfo, int iSpinX, int iSpinY,
              const CLabelInfo &labelInfo);
  virtual ~CGUITextBox(void);
  virtual void Render();
  virtual bool OnAction(const CAction &action) ;
  virtual void OnRight();
  virtual void OnLeft();
  virtual bool OnMessage(CGUIMessage& message);

  virtual void PreAllocResources();
  virtual void AllocResources() ;
  virtual void FreeResources() ;
  virtual void DynamicResourceAlloc(bool bOnOff);
  virtual void SetPosition(int iPosX, int iPosY);
  virtual void SetWidth(int iWidth);
  virtual void SetHeight(int iHeight);
  virtual void SetPulseOnSelect(bool pulse);
  virtual void SetNavigation(DWORD up, DWORD down, DWORD left, DWORD right);
  const CLabelInfo& GetLabelInfo() const { return m_label; };
  DWORD GetSpinWidth() const { return m_upDown.GetWidth() / 2; };
  DWORD GetSpinHeight() const { return m_upDown.GetHeight(); };
  const CStdString& GetTextureUpName() const { return m_upDown.GetTextureUpName(); };
  const CStdString& GetTextureDownName() const { return m_upDown.GetTextureDownName(); };
  const CStdString& GetTextureUpFocusName() const { return m_upDown.GetTextureUpFocusName(); };
  const CStdString& GetTextureDownFocusName() const { return m_upDown.GetTextureDownFocusName(); };
  const CLabelInfo& GetSpinLabelInfo() const { return m_upDown.GetLabelInfo();};
  int GetSpinX() const { return m_iSpinPosX;};
  int GetSpinY() const { return m_iSpinPosY;};
  void SetText(const string &strText);
  virtual bool HitTest(int iPosX, int iPosY) const;
  virtual bool OnMouseOver();
  virtual bool OnMouseClick(DWORD dwButton);
  virtual bool OnMouseWheel();

protected:
  void OnPageUp();
  void OnPageDown();
  void UpdatePageControl();

  int m_iSpinPosX;
  int m_iSpinPosY;
  int m_iOffset;
  int m_iItemsPerPage;
  int m_iItemHeight;
  int m_iMaxPages;

  CStdString m_strText;
  CLabelInfo m_label;
  CGUISpinControl m_upDown;
  vector<CGUIListItem> m_vecItems;
  typedef vector<CGUIListItem> ::iterator ivecItems;

  bool m_wrapText;  // whether we need to wordwrap or not
};
#endif
