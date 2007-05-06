#pragma once

#include "GUIWindow.h"

class CGUIWindowVisualisation :
      public CGUIWindow
{
public:
  CGUIWindowVisualisation(void);
  virtual ~CGUIWindowVisualisation(void);
  virtual void AllocResources(bool forceLoad = false);
  virtual void FreeResources(bool forceUnLoad = false);
  virtual bool OnMessage(CGUIMessage& message);
  virtual bool OnAction(const CAction &action);
  virtual bool OnMouse(float x, float y);
  virtual void Render();
protected:
  DWORD m_dwInitTimer;
  DWORD m_dwLockedTimer;
  bool m_bShowPreset;
  CMusicInfoTag m_tag;    // current tag info, for finding when the info manager updates
};
