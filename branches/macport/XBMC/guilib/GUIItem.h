/*!
\file GUIItem.h
\brief 
*/

#ifndef GUILIB_GUIItem_H
#define GUILIB_GUIItem_H

#pragma once

class CGUIItem
{
public:
  class RenderContext
  {
  public:
    RenderContext()
    {
      m_positionX = m_positionY = 0;
      m_bFocused = false;
    };
    virtual ~RenderContext(){};

    float m_positionX;
    float m_positionY;
    bool m_bFocused;
  };

  CGUIItem(CStdString& aItemName);
  virtual ~CGUIItem(void);
  virtual void AllocResources() {}
  virtual void FreeResources() {}
  virtual void OnPaint(CGUIItem::RenderContext* pContext) = 0;
  virtual void GetDisplayText(CStdString& aString)
  {
    aString = m_strName;
  };

  CStdString GetName();
  void SetCookie(DWORD aCookie);
  DWORD GetCookie();

protected:
  DWORD m_dwCookie;
  CStdString m_strName;
};
#endif
