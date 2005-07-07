#pragma once

#include "guiwindow.h"

class CGUIWindowOSD : public CGUIDialog
{
public:

  CGUIWindowOSD(void);
  virtual ~CGUIWindowOSD(void);

  virtual bool OnMessage(CGUIMessage& message);
  virtual bool OnAction(const CAction &action);
  virtual void Render();
  virtual bool SubMenuVisible();

  friend class CGUIWindowSettingsScreenCalibration;

protected:
  virtual void Get_TimeInfo();
  virtual void SetVideoProgress();
  virtual void ToggleButton(DWORD iButtonID, bool bSelected);
  virtual void ToggleSubMenu(DWORD iButtonID, DWORD iBackID);
  virtual void SetSliderValue(float fMin, float fMax, float fValue, DWORD iControlID, float fInterval = 0);
  virtual void SetCheckmarkValue(BOOL bValue, DWORD iControlID);
  virtual void Handle_ControlSetting(DWORD iControlID, DWORD wID);
  virtual void PopulateBookmarks();
  void ClearBookmarkItems();
  virtual void PopulateAudioStreams();
  void ClearAudioStreamItems();
  virtual void PopulateSubTitles();
  void ClearSubTitleItems();
  void Reset();
  virtual void OnWindowLoaded();
  bool m_bSubMenuOn;
  int m_iActiveMenu;
  DWORD m_iActiveMenuButtonID;
  int m_iCurrentBookmark;
  DWORD m_dwOSDTimeOut;

  vector<CGUIListItem*> m_vecBookmarksItems;
  vector<CGUIListItem*> m_vecAudioStreamItems;
  vector<CGUIListItem*> m_vecSubTitlesItems;
};
