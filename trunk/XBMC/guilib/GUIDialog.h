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
  virtual bool    OnMessage(CGUIMessage& message);
  virtual void    Render();
	void						DoModal(DWORD dwParentId);
	virtual void		Close();
	virtual bool    Load(const CStdString& strFileName, bool bContainsPath = false);
	virtual bool		IsRunning() const { return m_bRunning; }
	virtual bool		IsDialog() { return true;};

protected:
	DWORD						m_dwParentWindowID;
	CGUIWindow* 		m_pParentWindow;
	DWORD						m_dwPrevRouteWindow;
	CGUIWindow* 		m_pPrevRouteWindow;
	bool						m_bRunning;
private:
  bool            m_bPrevOverlayAllowed;
};
