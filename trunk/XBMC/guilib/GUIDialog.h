/*!
	\file GUIDialog.h
	\brief 
	*/

#pragma once
#include "guiwindow.h"

/*!
	\ingroup winmsg
	\brief 
	*/
class CGUIDialog :
	public CGUIWindow
{
public:
	CGUIDialog(DWORD dwID);
	virtual ~CGUIDialog(void);
  virtual void    Render();
	void						DoModal(DWORD dwParentId);
	virtual void		Close();
	
protected:
	DWORD						m_dwParentWindowID;
	CGUIWindow* 		m_pParentWindow;
	bool						m_bRunning;
};
