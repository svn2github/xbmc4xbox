#pragma once
#include "GUIWindowMusicBase.h"


class CGUIWindowMusicNav : public CGUIWindowMusicBase
{
public:

  CGUIWindowMusicNav(void);
  virtual ~CGUIWindowMusicNav(void);

  virtual bool OnMessage(CGUIMessage& message);
  virtual void Render();

protected:
  // override base class methods
  virtual bool GetDirectory(const CStdString &strDirectory, CFileItemList &items);
  virtual void UpdateButtons();
  virtual void DoSearch(const CStdString& strSearch, CFileItemList& items);
  virtual void PlayItem(int iItem);
  virtual void OnWindowLoaded();
  virtual void OnPopupMenu(int iItem);

  void SetArtistImage(int iItem);
  bool GetSongsFromPlayList(const CStdString& strPlayList, CFileItemList &items);
  void DisplayEmptyDatabaseMessage(bool bDisplay);

  VECSHARES m_shares;

  bool m_bDisplayEmptyDatabaseMessage;  ///< If true we display a message informing the user to switch back to the Files view.
};
