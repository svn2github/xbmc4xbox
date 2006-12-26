#pragma once
#include "Database.h"

#define VIEW_DATABASE_NAME "ViewModes.db"

class CViewDatabase : public CDatabase
{
public:
  CViewDatabase(void);
  virtual ~CViewDatabase(void);

  bool GetViewState(const CStdString &path, int windowID, CViewState &state);
  bool SetViewState(const CStdString &path, int windowID, const CViewState &state);

protected:
  virtual bool CreateTables();
  virtual bool UpdateOldVersion(int version);
};
