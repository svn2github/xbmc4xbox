#pragma once
#include "GUIWindowVideo.h"
#include "filesystem/VirtualDirectory.h"
#include "filesystem/DirectoryHistory.h"
#include "FileItem.h"
#include "GUIDialogProgress.h"
#include "videodatabase.h"

#include "stdstring.h"
#include <vector>
using namespace std;
using namespace DIRECTORY;

class CGUIWindowVideoGenre : 	public CGUIWindowVideo
{
public:
	CGUIWindowVideoGenre(void);
	virtual ~CGUIWindowVideoGenre(void);
  virtual bool    OnMessage(CGUIMessage& message);
  virtual void    OnAction(const CAction &action);
  virtual void    Render();

protected:
  void							Clear();
  virtual void			OnSort();
  virtual void			UpdateButtons();
	virtual void			Update(const CStdString &strDirectory);

};
