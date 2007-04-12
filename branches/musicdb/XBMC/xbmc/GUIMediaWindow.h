#pragma once
#include "GUIWindow.h"
#include "FileSystem/VirtualDirectory.h"
#include "FileSystem/DirectoryHistory.h"
#include "GUIViewControl.h"

// base class for all media windows
class CGUIMediaWindow : public CGUIWindow
{
public:
  CGUIMediaWindow(DWORD id, const char *xmlFile);
  virtual ~CGUIMediaWindow(void);
  virtual bool IsMediaWindow() const { return true; };
  virtual bool OnMessage(CGUIMessage& message);
  virtual bool OnAction(const CAction &action);
  virtual void OnWindowLoaded();
  virtual void OnWindowUnload();
  virtual void OnInitWindow();
  virtual CFileItem *GetCurrentListItem();
  const CFileItem &CurrentDirectory() const { return m_vecItems;};

protected:
  CGUIControl *GetFirstFocusableControl(int id);
  void SetupShares();
  virtual void GoParentFolder();
  virtual bool OnClick(int iItem);
  virtual void FormatItemLabels(CFileItemList &items, const CGUIViewState::LABEL_MASKS &labelMasks);
  virtual void UpdateButtons();
  virtual bool GetDirectory(const CStdString &strDirectory, CFileItemList &items);
  virtual bool Update(const CStdString &strDirectory);
  virtual void OnSort();
  virtual void OnPrepareFileItems(CFileItemList &items);
  virtual void OnFinalizeFileItems(CFileItemList &items);

  virtual void ClearFileItems();
  virtual void SortItems(CFileItemList &items);

  // check for a disc or connection
  virtual bool HaveDiscOrConnection(CStdString& strPath, int iDriveType);
  void ShowShareErrorMessage(CFileItem* pItem);

  void GetDirectoryHistoryString(const CFileItem* pItem, CStdString& strHistoryString);
  void SetHistoryForPath(const CStdString& strDirectory);
  virtual void LoadPlayList(const CStdString& strFileName) {}
  virtual bool OnPlayMedia(int iItem);
  void UpdateFileList();
  virtual void OnDeleteItem(int iItem);
  void OnRenameItem(int iItem);

protected:
  DIRECTORY::CVirtualDirectory m_rootDir;
  CGUIViewControl m_viewControl;

  // current path and history
  CFileItemList m_vecItems;
  CDirectoryHistory m_history;
  auto_ptr<CGUIViewState> m_guiState;

  // save control state on window exit
  int m_iLastControl;
  int m_iSelectedItem;
};
