#pragma once
#include "guiwindow.h"
#include "guiwindowmanager.h"
#include "osd/osdmenu.h"
#include "osd/iexecutor.h"
#include "utils/CriticalSection.h"
using namespace OSD;
class CGUIWindowFullScreen :
  public CGUIWindow,
  public IExecutor
{
public:
  CGUIWindowFullScreen(void);
  virtual ~CGUIWindowFullScreen(void);
  virtual bool    OnMessage(CGUIMessage& message);
  virtual void    OnAction(const CAction &action);
	virtual void		Render();
	void				    RenderFullScreen();
  bool            OSDVisible() const;
  
   virtual void   OnExecute(int iAction, const IOSDOption* option);
private:
  void            ShowOSD();
  void            HideOSD();

	bool						m_bShowInfo;
	bool						m_bShowStatus;
	DWORD						m_dwLastTime;
	DWORD						m_dwFPSTime;
	float						m_fFrameCounter;
	FLOAT						m_fFPS;
  COSDMenu        m_osdMenu;
  bool            m_bOSDVisible;
  CCriticalSection m_section;
};
