#pragma once
#include "guiwindow.h"
#include "filesystem/VirtualDirectory.h"
#include "filesystem/DirectoryHistory.h"
#include "FileItem.h"
#include "GUIDialogProgress.h"

#include "stdstring.h"
#include <vector>
using namespace std;
using namespace DIRECTORY;

class CGUIWindowPictures : 	public CGUIWindow
{
public:
	CGUIWindowPictures(void);
	virtual ~CGUIWindowPictures(void);
  virtual bool			OnMessage(CGUIMessage& message);
  virtual void			OnAction(const CAction &action);
  virtual void			Render();

protected:
	void							GoParentFolder();
  void							OnClick(int iItem);
  void							OnSort();
  void							UpdateButtons();
  void							Clear();
	void							Update(const CStdString &strDirectory);
	void							OnShowPicture(const CStdString& strPicture);
	void							OnSlideShow();
	void							OnCreateThumbs();
	int								GetSelectedItem();
  bool							HaveDiscOrConnection( CStdString& strPath, int iDriveType );

	CVirtualDirectory		m_rootDir;
  VECFILEITEMS				m_vecItems;
	CStdString					m_strDirectory;
	CGUIDialogProgress*	m_dlgProgress;
	CDirectoryHistory		m_history;
  int                 m_iItemSelected;
};
