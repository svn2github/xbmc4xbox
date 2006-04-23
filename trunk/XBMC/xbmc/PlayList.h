#pragma once

#include "fileitem.h"

namespace PLAYLIST
{
class CPlayList
{
public:
class CPlayListItem : public CFileItem
  {
  public:
    CPlayListItem();
    CPlayListItem(const CStdString& strDescription, const CStdString& strFileName, long lDuration = 0, long lStartOffset = 0, long lEndOffset = 0);

    virtual ~CPlayListItem();

    void SetFileName(const CStdString& strFileName);
    const CStdString& GetFileName() const;

    void SetDescription(const CStdString& strDescription);
    const CStdString& GetDescription() const;

    void SetDuration(long lDuration);
    long GetDuration() const;

    void SetStartOffset(long lStartOffset);
    long GetStartOffset() const;

    void SetEndOffset(long lEndOffset);
    long GetEndOffset() const;

    void SetMusicTag(const CMusicInfoTag &tag);
    CMusicInfoTag GetMusicTag() const;

    // keep track of the order items were added to the playlist
    int m_iOrder;

    bool WasPlayed();
    void SetPlayed() { m_bPlayed = true; };
    void ClearPlayed() { m_bPlayed = false; };

    bool IsUnPlayable();
    void SetUnPlayable() { m_bUnPlayable = true; };
    void ClearUnPlayable() { m_bUnPlayable = false; };

  protected:
    long m_lDuration;
    bool m_bPlayed;
    bool m_bUnPlayable;
  };
  CPlayList(void);
  virtual ~CPlayList(void);
  virtual bool Load(const CStdString& strFileName, bool bDeep = true){ return false;};
  virtual void Save(const CStdString& strFileName) const {};
  void Add(CPlayListItem& item);
  const CStdString& GetName() const;
  void Remove(const CStdString& strFileName);
  void Remove(int position);
  bool Swap(int position1, int position2);
  void Clear();
  int size() const;
  int RemoveDVDItems();
  const CPlayList::CPlayListItem& operator[] (int iItem) const;
  CPlayList::CPlayListItem& operator[] (int iItem);

  virtual void Shuffle();
  virtual void UnShuffle();
  virtual bool IsShuffled() { return m_bShuffled; }
  void FixOrder(int iOrder);

  void SetPlayed(int iItem);
  void SetUnPlayable(int iItem);
  void ClearPlayed();
  int GetUnplayed() { return m_iUnplayedItems; };
  int GetPlayable() { return m_iPlayableItems; };

protected:
  CStdString m_strPlayListName;
  int m_iUnplayedItems;
  int m_iPlayableItems;
  bool m_bShuffled;
  vector <CPlayListItem> m_vecItems;
  typedef vector <CPlayListItem>::iterator ivecItems;
};
};
