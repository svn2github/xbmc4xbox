#pragma once
#include "GUIWindow.h"

class CGUIWindowSettingsProfile :
      public CGUIWindow
{
public:
  CGUIWindowSettingsProfile(void);
  virtual ~CGUIWindowSettingsProfile(void);
  virtual bool OnMessage(CGUIMessage& message);
  virtual void OnAction(const CAction &action);

protected:
  int m_iLastControl;
  vector<CGUIListItem*> m_vecListItems;

  void OnPopupMenu(int iItem);
  void DoRename(int iItem);
  void DoDelete(int iItem);
  void DoOverwrite(int iItem);
  bool GetKeyboard(CStdString& strInput);
  int GetSelectedItem();
  void LoadList();
  void SetLastLoaded();
  void ClearListItems();
};
