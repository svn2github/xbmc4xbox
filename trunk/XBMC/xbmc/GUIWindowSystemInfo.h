#pragma once
#include "guiwindow.h"

class CGUIWindowSystemInfo :
	public CGUIWindow
{
public:
	CGUIWindowSystemInfo(void);
	virtual ~CGUIWindowSystemInfo(void);
  virtual bool    OnMessage(CGUIMessage& message);
  virtual void    OnAction(const CAction &action);
	virtual void		Render();
protected:
	void						GetValues();
	DWORD						m_dwlastTime ;

	unsigned short cputemp;
	unsigned short cpudec;
	unsigned short mbtemp;


};
