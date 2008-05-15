#pragma once

#include "GUIDialogSettings.h"
#include "ScraperSettings.h"

namespace VIDEO 
{
  struct SScanSettings;
}
class CFileItemList;

class CGUIDialogContentSettings : public CGUIDialogSettings
{
public:
  CGUIDialogContentSettings(void);
  virtual ~CGUIDialogContentSettings(void);
  virtual bool OnMessage(CGUIMessage &message);

  static bool Show(SScraperInfo& scraper, bool& bRunScan, int iLabel=-1);
  static bool Show(SScraperInfo& scraper, VIDEO::SScanSettings& settings, bool& bRunScan, int iLabel=-1);
  static bool ShowForDirectory(const CStdString& strDirectory, SScraperInfo& scraper, VIDEO::SScanSettings& settings, bool& bRunScan);
  virtual bool HasListItems() const { return true; };
  virtual CFileItem* GetCurrentListItem(int offset = 0);
protected:
  virtual void OnCancel();
  virtual void OnWindowLoaded();
  virtual void OnInitWindow();
  virtual void SetupPage();
  virtual void CreateSettings();
  void FillListControl();
  void OnSettingChanged(unsigned int setting);
  SScraperInfo FindDefault(const CStdString& strType, const CStdString& strDefault);

  bool m_bNeedSave;

  bool m_bRunScan;
  bool m_bScanRecursive;
  bool m_bUseDirNames;
  bool m_bSingleItem;
  bool m_bExclude;
  std::map<CStdString,std::vector<SScraperInfo> > m_scrapers; // key = content type
  CFileItemList* m_vecItems;

  SScraperInfo m_info;
  CScraperSettings m_scraperSettings; // needed so we have a basis
  CStdString m_strContentType; // used for artist/albums
};

