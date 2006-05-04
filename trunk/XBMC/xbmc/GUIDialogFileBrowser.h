#pragma once
#include "GUIDialog.h"
#include "FileSystem/VirtualDirectory.h"
#include "FileSystem/DirectoryHistory.h"
#include "GUIViewControl.h"
#include "PictureThumbLoader.h"

using namespace DIRECTORY;

class CGUIDialogFileBrowser : public CGUIDialog, public IBackgroundLoaderObserver
{
public:
  CGUIDialogFileBrowser(void);
  virtual ~CGUIDialogFileBrowser(void);
  virtual bool OnMessage(CGUIMessage& message);
  virtual bool OnAction(const CAction &action);
  virtual void Render();
  virtual void OnWindowLoaded();
  virtual void OnWindowUnload();
  bool IsConfirmed() { return m_bConfirmed; };
  void SetHeading(const CStdString &heading);

  static bool ShowAndGetDirectory(VECSHARES &shares, const CStdString &heading, CStdString &path, bool bWriteOnly=false);
  static bool ShowAndGetFile(VECSHARES &shares, const CStdString &mask, const CStdString &heading, CStdString &path);
  static bool ShowAndGetShare(CStdString &path, bool allowNetworkShares, CShare *additionalShare = NULL);
  static bool ShowAndGetImage(VECSHARES &shares, const CStdString &heading, CStdString &path);
  static bool ShowAndGetImage(const CFileItemList &items, const CStdString &heading, CStdString &path);

  void SetShares(VECSHARES &shares);

  virtual void OnItemLoaded(CFileItem *item) {};
  const CFileItem *GetCurrentListItem() const;
protected:
  void GoParentFolder();
  void OnClick(int iItem);
  void OnSort();
  void ClearFileItems();
  void Update(const CStdString &strDirectory);
  bool HaveDiscOrConnection( CStdString& strPath, int iDriveType );
  void OnAddNetworkLocation();

  VECSHARES m_shares;
  CVirtualDirectory m_rootDir;
  CFileItemList m_vecItems;
  CFileItem m_Directory;
  CStdString m_strParentPath;
  CStdString m_selectedPath;
  CDirectoryHistory m_history;
  int m_browsingForFolders; // 0 - no, 1 - yes, 2 - yes, only writable
  bool m_bConfirmed;
  bool m_addNetworkShareEnabled;
  bool m_browsingForImages;
  bool m_singleList;              // if true, we have no shares or anything

  CPictureThumbLoader m_thumbLoader;
  CGUIViewControl m_viewControl;
};
