#pragma once
#include "filesystem/idirectory.h"
#include "fileitem.h"
#include "settings.h"
#include "filesystem/directory.h"
#include <string>
#include "stdstring.h"
#include <vector>
using namespace std;

namespace DIRECTORY
{

class CPluginDirectory : public IDirectory
{
public:
  CPluginDirectory(void);
  ~CPluginDirectory(void);
  virtual bool GetDirectory(const CStdString& strPath, CFileItemList& items);
  static bool RunScriptWithParams(const CStdString& strPath);
  static bool HasPlugins(const CStdString &type);
  bool GetPluginsDirectory(const CStdString &type, CFileItemList &items);

  // callbacks from python
  static bool AddItem(int handle, const CFileItem *item, int totalItems);
  static void EndOfDirectory(int handle, bool success);
  static void AddSortMethod(int handle, SORT_METHOD sortMethod);

private:
  bool WaitOnScriptResult(const CStdString &scriptPath, const CStdString &scriptName);

  static vector<CPluginDirectory*> globalHandles;
  static int getNewHandle(CPluginDirectory *cp);
  static void removeHandle(int handle);

  CFileItemList m_listItems;
  HANDLE        m_directoryFetched;

  bool          m_cancelled;    // set to true when we are cancelled
  bool          m_success;      // set by script in EndOfDirectory
  int    m_totalItems;   // set by script in AddDirectoryItem
};
};
