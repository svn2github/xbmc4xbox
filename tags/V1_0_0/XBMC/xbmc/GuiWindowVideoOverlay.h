#pragma once
#include "guiwindow.h"

#include "stdstring.h"
#include <vector>
using namespace std;

class CGUIWindowVideoOverlay: 	public CGUIWindow
{
public:
	CGUIWindowVideoOverlay(void);
	virtual ~CGUIWindowVideoOverlay(void);
  virtual bool				OnMessage(CGUIMessage& message);
  virtual void				OnAction(const CAction &action);
	virtual void				Render();
	void								SetCurrentFile(const CStdString& strFile);
protected:
  void                HideControl(int iControl);
  void                ShowControl(int iControl);
};
