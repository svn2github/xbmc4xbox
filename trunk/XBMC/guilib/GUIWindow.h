#ifndef GUILIB_GUIWINDOW_H
#define GUILIB_GUIWINDOW_H

#pragma once
#include <vector>
using namespace std;
#include "guicontrol.h" 


class CGUIWindow
{
public:
  CGUIWindow(DWORD dwID);
  virtual ~CGUIWindow(void);
  bool            		Load(const CStdString& strFileName);
  virtual void    		Render();
  virtual void    		OnKey(const CKey& key);
  virtual bool    		OnMessage(CGUIMessage& message);
  void            		Add(CGUIControl* pControl);
  int             		GetFocusControl();
  void            		SelectNextControl();
  DWORD           		GetID(void) const;
	const CGUIControl*	GetControl(int iControl) const;
	void								ClearAll();
	int									GetFocusedControl() const;
	void								AllocResources();
  void								FreeResources();
	void								ResetAllControls();  
protected:
  vector<CGUIControl*> m_vecControls;
  typedef vector<CGUIControl*>::iterator ivecControls;
  DWORD  m_dwWindowId;
  DWORD  m_dwDefaultFocusControlID;
};

#endif