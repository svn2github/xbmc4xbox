#ifndef GUILIB_GUIWINDOW_H
#define GUILIB_GUIWINDOW_H

#pragma once

#include <map>
#include <string>
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
  virtual void    		OnAction(const CAction &action);
  virtual bool    		OnMessage(CGUIMessage& message);
  void            		Add(CGUIControl* pControl);
  int             		GetFocusControl();
  void            		SelectNextControl();
  DWORD           		GetID(void) const;
	void								SetID(DWORD dwID);
	const CGUIControl*	GetControl(int iControl) const;
	void								ClearAll();
	int									GetFocusedControl() const;
	void								AllocResources();
  void								FreeResources();
	void								ResetAllControls();  
protected:
	bool LoadReference(const CStdString& strFileName, map<string,CGUIControl*>& controls);
  vector<CGUIControl*> m_vecControls;
  typedef vector<CGUIControl*>::iterator ivecControls;
  DWORD  m_dwWindowId;
  DWORD  m_dwDefaultFocusControlID;
};

#endif