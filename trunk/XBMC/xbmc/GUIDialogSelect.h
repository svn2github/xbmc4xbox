#pragma once
#include "GUIDialog.h"
#include "guiwindowmanager.h"
#include "guilistitem.h"
#include "stdstring.h"
#include <vector>
#include <string>
using namespace std;

class CGUIDialogSelect :
  public CGUIDialog
{
public:
  CGUIDialogSelect(void);
  virtual ~CGUIDialogSelect(void);
  virtual bool    OnMessage(CGUIMessage& message);
  virtual void    OnAction(const CAction &action);
  
	void						Reset();
	void						Add(const CStdString& strLabel);
	int							GetSelectedLabel() const;
	void						SetHeading(const wstring& strLine);
	void						SetHeading(const string& strLine);
	void						SetHeading(int iString);
protected:
	int							m_iSelected;
	vector<CGUIListItem*> m_vecList;
};
