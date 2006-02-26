#pragma once
#include "GUIViewState.h"

class CGUIViewStateWindowPictures : public CGUIViewState
{
public:
  CGUIViewStateWindowPictures(const CFileItemList& items);

protected:
  virtual bool HideParentDirItems();
  virtual void SaveViewState();
  virtual CStdString GetLockType();
  virtual bool UnrollArchives();
};

class CGUIViewStateWindowPrograms : public CGUIViewState
{
public:
  CGUIViewStateWindowPrograms(const CFileItemList& items);

protected:
  virtual bool HideParentDirItems();
  virtual void SaveViewState();
  virtual CStdString GetLockType();
};

class CGUIViewStateWindowScripts : public CGUIViewState
{
public:
  CGUIViewStateWindowScripts(const CFileItemList& items);

protected:
  virtual void SaveViewState();
};
