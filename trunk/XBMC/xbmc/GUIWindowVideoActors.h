#pragma once
#include "GUIWindowVideoBase.h"

class CGUIWindowVideoActors : public CGUIWindowVideoBase
{
public:
  CGUIWindowVideoActors(void);
  virtual ~CGUIWindowVideoActors(void);
  virtual bool OnMessage(CGUIMessage& message);

protected:
  virtual void SaveViewMode();
  virtual void LoadViewMode();
  virtual int SortMethod();
  virtual bool SortAscending();

  virtual void FormatItemLabels();
  virtual void SortItems(CFileItemList& items);
  virtual bool Update(const CStdString &strDirectory);
  virtual void OnClick(int iItem);
  virtual void OnInfo(int iItem);
  virtual void OnDeleteItem(int iItem) {return;};
};
