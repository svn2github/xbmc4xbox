#pragma once
#include "GUIDialog.h"

class CGUIDialogContextMenu :
      public CGUIDialog
{
public:
  CGUIDialogContextMenu(void);
  virtual ~CGUIDialogContextMenu(void);
  virtual bool OnMessage(CGUIMessage &message);
  virtual void DoModal(DWORD dwParentId, int iWindowID = WINDOW_INVALID);
  virtual void OnWindowUnload();
  void ClearButtons();
  int AddButton(int iLabel);
  int AddButton(const CStdString &strButton);
  int GetNumButtons();
  void EnableButton(int iButton, bool bEnable);
  int GetButton();
  DWORD GetWidth();
  DWORD GetHeight();

  static bool BookmarksMenu(const CStdString &strType, const CStdString &strLabel, const CStdString &strPath, int iLockMode, bool bMaxRetryExceeded, int iPosX, int iPosY);

protected:
  virtual void OnInitWindow();
  static bool CheckMasterCode(int iLockMode);
  static CStdString GetDefaultShareNameByType(const CStdString &strType);
  static void SetDefault(const CStdString &strType, const CStdString &strDefault);
  static void ClearDefault(const CStdString &strType);

private:
  int m_iNumButtons;
  int m_iClickedButton;
};
