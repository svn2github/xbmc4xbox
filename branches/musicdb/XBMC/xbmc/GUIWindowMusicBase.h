/*!
\file GUIWindowMusicBase.h
\brief
*/

#pragma once
#include "GUIMediaWindow.h"
#include "MusicDatabase.h"
#include "PlayList.h"
#include "MusicInfoTagLoaderFactory.h"
#include "Utils/MusicInfoScraper.h"
#include "playlistplayer.h"
#include "musicinfoloader.h"

/*!
 \ingroup windows 
 \brief The base class for music windows
 
 CGUIWindowMusicBase is the base class for
 all music windows.
 */
class CGUIWindowMusicBase : public CGUIMediaWindow
{
public:
  CGUIWindowMusicBase(DWORD dwID, const CStdString &xmlFile);
  virtual ~CGUIWindowMusicBase(void);
  virtual bool OnMessage(CGUIMessage& message);
  virtual bool OnAction(const CAction& action);
  static void OnRipCD();

protected:
  /*!
  \brief Will be called when an popup context menu has been asked for
  \param iItem List/thumb control item that has been clicked on
  */
  virtual void OnPopupMenu(int iItem, bool bContextDriven = true);
  /*!
  \brief Overwrite to update your gui buttons (visible, enable,...)
  */
  virtual void UpdateButtons();
  
  virtual bool GetDirectory(const CStdString &strDirectory, CFileItemList &items);
  virtual void OnRetrieveMusicInfo(CFileItemList& items);
  void AddItemToPlayList(const CFileItem* pItem, CFileItemList &queuedItems);
  virtual void OnSearchItemFound(const CFileItem* pItem);
  virtual void DoSearch(const CStdString& strSearch, CFileItemList& items);
  virtual void OnScan(int iItem) {};

  // new methods
  virtual void PlayItem(int iItem);
  virtual bool OnPlayMedia(int iItem);

  void RetrieveMusicInfo();
  void OnInfo(int iItem, bool bShowInfo = true);
  void OnInfoAll(int iItem);
  virtual void OnQueueItem(int iItem);
  bool FindAlbumInfo(const CStdString& strAlbum, const CStdString& strArtist, CMusicAlbumInfo& album, bool bShowInfo = true);

  void ShowAlbumInfo(const CStdString& strAlbum, const CStdString& strPath, bool bSaveDb, bool bSaveDirThumb, bool bRefresh);
  void ShowAlbumInfo(const CStdString& strAlbum, const CStdString& strArtist, const CStdString& strPath, bool bSaveDb, bool bSaveDirThumb, bool bRefresh, bool bShowInfo = true);
  void UpdateThumb(const CMusicAlbumInfo &album, bool bSaveDb, bool bSaveDirThumb);

  void OnManualAlbumInfo();
  void OnRipTrack(int iItem);
  void OnSearch();
  virtual void LoadPlayList(const CStdString& strPlayList);
  
  typedef vector <CFileItem*>::iterator ivecItems; ///< CFileItem* vector Iterator
  CGUIDialogProgress* m_dlgProgress; ///< Progress dialog

  // member variables to save frequently used g_guiSettings (which is slow)
  bool m_hideExtensions;
  CMusicDatabase m_musicdatabase;
  CMusicInfoLoader m_musicInfoLoader;
};
