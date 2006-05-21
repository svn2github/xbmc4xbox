#pragma once
#include "GUIMediaWindow.h"
#include "programdatabase.h"
#include "GUIDialogProgress.h"
#include "ThumbLoader.h"

class CGUIWindowPrograms :
      public CGUIMediaWindow, public IBackgroundLoaderObserver
{
public:
  CGUIWindowPrograms(void);
  virtual ~CGUIWindowPrograms(void);
  virtual bool OnMessage(CGUIMessage& message);
  virtual bool OnPopupMenu(int iItem);

protected:
  virtual void OnItemLoaded(CFileItem* pItem) {};
  virtual void GoParentFolder();
  virtual bool OnClick(int iItem);
  virtual bool Update(const CStdString& strDirectory);
  virtual bool OnPlayMedia(int iItem);
  virtual bool GetDirectory(const CStdString &strDirectory, CFileItemList &items);
  virtual void OnWindowLoaded();
  void SetOverlayIcons();

  void OnScan(CFileItemList& items, int& iTotalAppsFound) ;
  void LoadDirectory(const CStdString& strDirectory, int depth);
  void LoadDirectory2(const CStdString& strDirectory, int depth);
  void DeleteThumbs(CFileItemList& items);
  int GetRegion(int iItem, bool bReload=false);
  void PopulateTrainersList();
  CGUIDialogProgress* m_dlgProgress;

  CStdString m_shareDirectory;

  int m_iDepth;
  CStdString m_strBookmarkName;
  vector<CStdString> m_vecPaths, m_vecPaths1;
  CProgramDatabase m_database;
  CStdString m_strParentPath;
  bool m_isRoot;
  int m_iRegionSet; // for cd stuff

  CProgramThumbLoader m_thumbLoader;
};
