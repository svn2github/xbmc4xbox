#pragma once
#include "GUIWindowVideoBase.h"

class CGUIWindowVideoPlaylist : public CGUIWindow
{
public:
  CGUIWindowVideoPlaylist(void);
  virtual ~CGUIWindowVideoPlaylist(void);

  virtual bool OnMessage(CGUIMessage& message);
  virtual void OnAction(const CAction &action);
  virtual void OnWindowLoaded();

protected:
  void GetDirectoryHistoryString(const CFileItem* pItem, CStdString& strHistoryString);
  void GetDirectory(const CStdString &strDirectory, CFileItemList &items);
  void Update(const CStdString &strDirectory);
  void ClearPlayList();
  void ClearFileItems();
  void UpdateListControl();
  void RemovePlayListItem(int iItem);
  void MoveCurrentPlayListItem(int iAction); // up or down
  void OnFileItemFormatLabel(CFileItem* pItem);
  void DoSort(CFileItemList& items);
  bool GetKeyboard(CStdString& strInput);
  void UpdateButtons();
  void ShufflePlayList();
  void SavePlayList();

  void OnClick(int iItem);
  void OnQueueItem(int iItem);
  int m_iItemSelected;
  int m_iLastControl;
  CFileItemList m_vecItems;
  CDirectoryHistory m_history;
  CFileItem m_Directory;
  CGUIViewControl m_viewControl;
  typedef vector <CFileItem*>::iterator ivecItems;
};
