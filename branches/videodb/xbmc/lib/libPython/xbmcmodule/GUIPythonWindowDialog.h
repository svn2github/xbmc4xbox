#pragma once
#include "GUIPythonWindow.h"

class CGUIPythonWindowDialog : public CGUIPythonWindow
{
public:
  CGUIPythonWindowDialog(DWORD dwId);
  virtual ~CGUIPythonWindowDialog(void);
	virtual bool    OnMessage(CGUIMessage& message);
	void            Activate(DWORD dwParentId);
	virtual void		Close();
	virtual bool		IsRunning() const { return m_bRunning; }
  virtual bool    IsDialog() const { return true;};
  virtual bool    IsModalDialog() const { return true; };

protected:
	DWORD						m_dwParentWindowID;
	CGUIWindow* 		m_pParentWindow;
	bool						m_bRunning;
};
