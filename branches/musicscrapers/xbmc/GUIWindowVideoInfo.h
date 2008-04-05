#pragma once
#include "GUIDialog.h"
#include "GUIListItem.h"
#include "VideoDatabase.h"
#include "GUIWindowVideoBase.h"
#include "GUIWindowVideoFiles.h"

class CFileItem;

class CGUIWindowVideoInfo :
      public CGUIDialog
{
public:
  CGUIWindowVideoInfo(void);
  virtual ~CGUIWindowVideoInfo(void);
  virtual bool OnMessage(CGUIMessage& message);
  virtual void Render();
  void SetMovie(const CFileItem *item);
  bool NeedRefresh() const;
  bool RefreshAll() const;
  virtual bool OnAction(const CAction& action);

  const CStdString &GetThumbnail() const;
  virtual CFileItem* GetCurrentListItem(int offset = 0) { return m_movieItem; }
  virtual bool HasListItems() const { return true; };
protected:
  virtual void OnInitWindow();
  void Refresh();
  void Update();
  void SetLabel(int iControl, const CStdString& strLabel);

  // link cast to movies
  void AddItemsToList(const std::vector<std::pair<CStdString,CStdString> > &vecStr);
  void OnSearch(CStdString& strSearch);
  void DoSearch(CStdString& strSearch, CFileItemList& items);
  void OnSearchItemFound(const CFileItem* pItem);
  void Play(bool resume = false);
  void OnGetThumb();
  void OnGetFanart();
  void PlayTrailer();
  CFileItem* m_movieItem;
  bool m_bViewReview;
  bool m_bRefresh;
  bool m_bRefreshAll;
  std::vector<std::pair<CStdString,CStdString> > m_vecStrCast;
  CGUIDialogProgress* m_dlgProgress;
  CVideoDatabase m_database;
};
