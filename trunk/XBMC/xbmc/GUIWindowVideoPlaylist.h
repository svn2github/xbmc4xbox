#pragma once
#include "GUIWindowVideoBase.h"

class CGUIWindowVideoPlaylist : public CGUIWindowVideoBase
{
public:
  CGUIWindowVideoPlaylist(void);
  virtual ~CGUIWindowVideoPlaylist(void);

  virtual bool OnMessage(CGUIMessage& message);
  virtual bool OnAction(const CAction &action);

protected:
  virtual bool OnPlayMedia(int iItem);
  virtual void UpdateButtons();

  virtual void OnPopupMenu(int iItem);
  void OnMove(int iItem, int iAction);

  virtual bool GetDirectory(const CStdString &strDirectory, CFileItemList &items);
  void ClearPlayList();
  void RemovePlayListItem(int iItem);
  bool MoveCurrentPlayListItem(int iItem, int iAction, bool bUpdate = true);
  void MoveItem(int iStart, int iDest);

  virtual void OnPrepareFileItems(CFileItemList &items);
  void ShufflePlayList();
  void SavePlayList();

  int iPos;
};
