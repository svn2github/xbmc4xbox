#pragma once
#include "GUIDialog.h"
#include "GUIViewControl.h"

class CGUIWindowLoginScreen : public CGUIWindow
{
public:
  CGUIWindowLoginScreen(void);
  virtual ~CGUIWindowLoginScreen(void);
  virtual bool OnMessage(CGUIMessage& message);
  virtual void Render();
protected:
  virtual void OnInitWindow();
  virtual void OnWindowLoaded();
  void Update();
  void SetLabel(int iControl, const CStdString& strLabel);

  bool OnPopupMenu(int iItem);
  CGUIViewControl m_viewControl;
  CFileItemList m_vecItems;

  int m_iSelectedItem;
};
