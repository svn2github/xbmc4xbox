#pragma once
#include "GUIWindowVideoBase.h"

class CGUIWindowVideoGenre : 	public CGUIWindowVideoBase
{
public:
	CGUIWindowVideoGenre(void);
	virtual ~CGUIWindowVideoGenre(void);
  virtual bool    OnMessage(CGUIMessage& message);
  virtual void    OnAction(const CAction &action);

protected:
  virtual bool      ViewByLargeIcon();
  virtual bool      ViewByIcon();
	virtual void			SetViewMode(int iMode);
	virtual int				SortMethod();
	virtual bool			SortAscending();

	virtual void			FormatItemLabels();
	virtual void			SortItems(VECFILEITEMS& items);
	virtual void			Update(const CStdString &strDirectory);
  virtual void			OnClick(int iItem);
  virtual void			OnInfo(int iItem);
};
