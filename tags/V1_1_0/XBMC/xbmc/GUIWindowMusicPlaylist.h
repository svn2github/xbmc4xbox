#pragma once
#include "GUIWindowMusicBase.h"
#include <map>
#include <set>

class CGUIWindowMusicPlayList : 	public CGUIWindowMusicBase
{
public:
	CGUIWindowMusicPlayList(void);
	virtual	~CGUIWindowMusicPlayList(void);

  virtual	bool				OnMessage(CGUIMessage& message);
  virtual	void				OnAction(const CAction &action);

protected:
	virtual void				GetDirectory(const CStdString &strDirectory, VECFILEITEMS &items);
  virtual	void				UpdateButtons();
  virtual void				OnClick(int iItem);
	virtual	void				OnQueueItem(int iItem);
	virtual void				OnFileItemFormatLabel(CFileItem* pItem);
	virtual	void				DoSort(VECFILEITEMS& items);
	virtual void				OnRetrieveMusicInfo(VECFILEITEMS& items);
					void				LoadItem(CFileItem* pItem);

					void				SavePlayList();
					void				ClearPlayList();
					void				ShufflePlayList();
					void				RemovePlayListItem(int iItem);
          void				MoveCurrentPlayListItem(int iAction); // up or down

					SETPATHES		m_Pathes;
					MAPSONGS		m_songsMap;
					CStdString	m_strPrevPath;
};
