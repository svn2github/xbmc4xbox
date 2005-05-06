#pragma once
#include "GUIWindowVideoBase.h"

class CGUIWindowVideoFiles : public CGUIWindowVideoBase
{
public:
  CGUIWindowVideoFiles(void);
  virtual ~CGUIWindowVideoFiles(void);
  virtual bool OnMessage(CGUIMessage& message);
  virtual bool OnAction(const CAction &action);

private:
  virtual void SetIMDBThumbs(CFileItemList& items);
protected:
  virtual void SaveViewMode();
  virtual void LoadViewMode();
  virtual int SortMethod();
  virtual bool SortAscending();
  virtual void AddFileToDatabase(const CFileItem* pItem);
  virtual void FormatItemLabels();
  virtual void SortItems(CFileItemList& items);
  virtual void UpdateButtons();
  virtual void Update(const CStdString &strDirectory);
  void UpdateDir(const CStdString &strDirectory);
  virtual void OnClick(int iItem);

  virtual void OnPopupMenu(int iItem);
  virtual void OnInfo(int iItem);

  virtual void OnScan();
  bool DoScan(CFileItemList& items);
  void OnRetrieveVideoInfo(CFileItemList& items);
  void LoadPlayList(const CStdString& strFileName);
  virtual void GetDirectory(const CStdString &strDirectory, CFileItemList &items);
  virtual void GetDirectoryHistoryString(const CFileItem* pItem, CStdString& strHistoryString);
  void SetHistoryForPath(const CStdString& strDirectory);
};
