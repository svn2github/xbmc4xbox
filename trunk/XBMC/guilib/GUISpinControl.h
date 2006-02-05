/*!
\file GUISpinControl.h
\brief 
*/

#ifndef GUILIB_SPINCONTROL_H
#define GUILIB_SPINCONTROL_H

#pragma once

#include "GUIImage.h"

#define SPIN_CONTROL_TYPE_INT    1
#define SPIN_CONTROL_TYPE_FLOAT  2
#define SPIN_CONTROL_TYPE_TEXT   3

class CRect
{
public:
  CRect() { top = left = width = height = 0;};
  ~CRect() {};

  void SetRect(int l, int t, int w, int h)
  {
    left = l;
    top = t;
    width = w;
    height = h;
  };

  bool PtInRect(int x, int y) const
  {
    if (left <= x && x <= left + width && top <= y && y <= top + height)
      return true;
    return false;
  };

private:
  int top;
  int left;
  int width;
  int height;
};
/*!
 \ingroup controls
 \brief 
 */
class CGUISpinControl : public CGUIControl
{
public:
  CGUISpinControl(DWORD dwParentID, DWORD dwControlId, int iPosX, int iPosY, DWORD dwWidth, DWORD dwHeight, const CStdString& strUp, const CStdString& strDown, const CStdString& strUpFocus, const CStdString& strDownFocus, const CLabelInfo& labelInfo, int iType);
  virtual ~CGUISpinControl(void);
  virtual void Render();
  virtual bool OnAction(const CAction &action);
  virtual void OnLeft();
  virtual void OnRight();
  virtual bool OnMouseOver();
  virtual bool OnMouseClick(DWORD dwButton);
  virtual bool OnMouseWheel();
  virtual bool OnMessage(CGUIMessage& message);
  virtual void PreAllocResources();
  virtual void AllocResources();
  virtual void FreeResources();
  virtual void DynamicResourceAlloc(bool bOnOff);
  virtual void SetPosition(int iPosX, int iPosY);
  virtual bool HitTest(int iPosX, int iPosY) const;
  virtual DWORD GetWidth() const;
  void SetRange(int iStart, int iEnd);
  void SetFloatRange(float fStart, float fEnd);
  void SetValue(int iValue);
  void SetFloatValue(float fValue);
  int GetValue() const;
  float GetFloatValue() const;
  void AddLabel(const wstring& strLabel, int iValue);
  void AddLabel(CStdString aLabel, int iValue);
  const wstring GetLabel() const;
  virtual void SetFocus(bool bOnOff);
  virtual void SetVisible(bool bVisible);
  void SetReverse(bool bOnOff);
  int GetMaximum() const;
  int GetMinimum() const;
  const CStdString& GetTextureUpName() const { return m_imgspinUp.GetFileName(); };
  const CStdString& GetTextureDownName() const { return m_imgspinDown.GetFileName(); };
  const CStdString& GetTextureUpFocusName() const { return m_imgspinUpFocus.GetFileName(); };
  const CStdString& GetTextureDownFocusName() const { return m_imgspinDownFocus.GetFileName(); };
  const CLabelInfo& GetLabelInfo() const { return m_label; };
  void SetSpinAlign(DWORD align, int offsetX) { m_label.align = align; m_label.offsetX = offsetX; };
  int GetType() const { return m_iType;};
  void SetType(int iType) { m_iType = iType; };
  DWORD GetSpinWidth() const { return m_imgspinUp.GetWidth(); };
  DWORD GetSpinHeight() const { return m_imgspinUp.GetHeight(); };
  void SetFloatInterval(float fInterval);
  float GetFloatInterval() const;
  bool GetShowRange() const;
  void SetShowRange(bool bOnoff) ;
  void SetBuddyControlID(DWORD dwBuddyControlID);
  void SetNonProportional(bool bOnOff);
  void Clear();
  virtual CStdString GetDescription() const;

protected:
  void PageUp();
  void PageDown();
  bool CanMoveDown(bool bTestReverse = true);
  bool CanMoveUp(bool bTestReverse = true);
  void MoveUp(bool bTestReverse = true);
  void MoveDown(bool bTestReverse = true);
  int m_iStart;
  int m_iEnd;
  float m_fStart;
  float m_fEnd;
  int m_iValue;
  float m_fValue;
  int m_iType;
  int m_iSelect;
  bool m_bReverse;
  float m_fInterval;
  vector<wstring> m_vecLabels;
  vector<int> m_vecValues;
  CGUIImage m_imgspinUp;
  CGUIImage m_imgspinDown;
  CGUIImage m_imgspinUpFocus;
  CGUIImage m_imgspinDownFocus;
  CLabelInfo m_label;
  bool m_bShowRange;
  char m_szTyped[10];
  int m_iTypedPos;
  DWORD m_dwBuddyControlID;
  bool m_bBuddyDisabled;
  float m_fMaxTextWidth;
  CRect m_rectHit;   // rect for hit test on the Text
};
#endif
