/*!
\file GUIListContainer.h
\brief 
*/

#pragma once

#include "GUIControl.h"
#include "GUIListItemLayout.h"

enum VIEW_TYPE { VIEW_TYPE_NONE = 0,
                 VIEW_TYPE_LIST,
                 VIEW_TYPE_ICON,
                 VIEW_TYPE_BIG_LIST,
                 VIEW_TYPE_BIG_ICON,
                 VIEW_TYPE_WIDE,
                 VIEW_TYPE_BIG_WIDE,
                 VIEW_TYPE_WRAP,
                 VIEW_TYPE_BIG_WRAP,
                 VIEW_TYPE_AUTO,
                 VIEW_TYPE_MAX };
/*!
 \ingroup controls
 \brief 
 */

class CGUIBaseContainer : public CGUIControl
{
public:
  CGUIBaseContainer(DWORD dwParentID, DWORD dwControlId, float posX, float posY, float width, float height, ORIENTATION orientation, int scrollTime);
  virtual ~CGUIBaseContainer(void);

  virtual bool OnAction(const CAction &action);
  virtual void OnDown();
  virtual void OnUp();
  virtual void OnLeft();
  virtual void OnRight();
  virtual bool OnMouseOver(const CPoint &point);
  virtual bool OnMouseClick(DWORD dwButton, const CPoint &point);
  virtual bool OnMouseDoubleClick(DWORD dwButton, const CPoint &point);
  virtual bool OnMouseWheel(char wheel, const CPoint &point);
  virtual bool OnMessage(CGUIMessage& message);
  virtual void SetFocus(bool bOnOff);
  virtual void AllocResources();
  virtual void FreeResources();
  virtual void UpdateVisibility(const CGUIListItem *item = NULL);

  virtual unsigned int GetRows() const;
  
  virtual bool HasNextPage() const;
  virtual bool HasPreviousPage() const;

  void SetPageControl(DWORD id);

  virtual CStdString GetDescription() const;
  virtual void SaveStates(std::vector<CControlState> &states);
  virtual int GetSelectedItem() const;

  virtual void DoRender(DWORD currentTime);
  void LoadLayout(TiXmlElement *layout);
  void LoadContent(TiXmlElement *content);

  VIEW_TYPE GetType() const { return m_type; };
  const CStdString &GetLabel() const { return m_label; };
  void SetType(VIEW_TYPE type, const CStdString &label);

  virtual bool IsContainer() const { return true; };
  CGUIListItem *GetListItem(int offset, unsigned int flag = 0) const;

  virtual bool GetCondition(int condition, int data) const;
  CStdString GetLabel(int info) const;

#ifdef _DEBUG
  virtual void DumpTextureUse();
#endif
protected:
  bool OnClick(DWORD actionID);
  virtual bool SelectItemFromPoint(const CPoint &point);
  virtual void RenderItem(float posX, float posY, CGUIListItem *item, bool focused);
  virtual void Scroll(int amount);
  virtual bool MoveDown(bool wrapAround);
  virtual bool MoveUp(bool wrapAround);
  virtual void MoveToItem(int item);
  virtual void ValidateOffset();
  virtual int  CorrectOffset(int offset, int cursor) const;
  virtual void UpdateLayout();
  virtual void CalculateLayout();
  virtual void SelectItem(int item) {};
  virtual void Reset();
  virtual unsigned int GetNumItems() const { return m_items.size(); };
  virtual int GetCurrentPage() const;
  bool InsideLayout(const CGUIListItemLayout *layout, const CPoint &point);

  inline float Size() const;
  void MoveToRow(int row);
  void FreeMemory(int keepStart, int keepEnd);
  void GetCurrentLayouts();
  CGUIListItemLayout *GetFocusedLayout() const;

  int m_offset;
  int m_cursor;
  float m_analogScrollCount;

  ORIENTATION m_orientation;
  int m_itemsPerPage;

  std::vector<CGUIListItem*> m_items;
  typedef std::vector<CGUIListItem*> ::iterator iItems;
  CGUIListItem *m_lastItem;

  DWORD m_pageControl;

  DWORD m_renderTime;

  std::vector<CGUIListItemLayout> m_layouts;
  std::vector<CGUIListItemLayout> m_focusedLayouts;

  CGUIListItemLayout *m_layout;
  CGUIListItemLayout *m_focusedLayout;

  virtual void ScrollToOffset(int offset);
  DWORD m_scrollLastTime;
  int   m_scrollTime;
  float m_scrollSpeed;
  float m_scrollOffset;

  VIEW_TYPE m_type;
  CStdString m_label;

  bool m_staticContent;
  std::vector<CGUIListItem*> m_staticItems;
  bool m_wasReset;  // true if we've received a Reset message until we've rendered once.  Allows
                    // us to make sure we don't tell the infomanager that we've been moving when
                    // the "movement" was simply due to the list being repopulated (thus cursor position
                    // changing around)
};

