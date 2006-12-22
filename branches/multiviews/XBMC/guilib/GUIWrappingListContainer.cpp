#include "include.h"
#include "GUIWrappingListContainer.h"
#include "GUIListItem.h"

CGUIWrappingListContainer::CGUIWrappingListContainer(DWORD dwParentID, DWORD dwControlId, float posX, float posY, float width, float height, ORIENTATION orientation, int scrollTime, int fixedPosition)
    : CGUIBaseContainer(dwParentID, dwControlId, posX, posY, width, height, orientation, scrollTime)
{
  m_cursor = fixedPosition;
  ControlType = GUICONTAINER_FIXEDLIST;
  m_type = VIEW_TYPE_LIST;
}

CGUIWrappingListContainer::~CGUIWrappingListContainer(void)
{
}

void CGUIWrappingListContainer::Render()
{
  if (!IsVisible()) return CGUIBaseContainer::Render();

  ValidateOffset();

  if (m_bInvalidated)
    UpdateLayout();

  m_scrollOffset += m_scrollSpeed * (m_renderTime - m_scrollLastTime);
  if ((m_scrollSpeed < 0 && m_scrollOffset < m_offset * m_layout.Size(m_orientation)) ||
      (m_scrollSpeed > 0 && m_scrollOffset > m_offset * m_layout.Size(m_orientation)))
  {
    m_scrollOffset = m_offset * m_layout.Size(m_orientation);
    m_scrollSpeed = 0;
  }
  m_scrollLastTime = m_renderTime;

  int offset = (int)(m_scrollOffset / m_layout.Size(m_orientation));
  // Free memory not used on screen at the moment, do this first so there's more memory for the new items.
  FreeMemory(CorrectOffset(offset), CorrectOffset(offset + m_itemsPerPage));

  g_graphicsContext.SetViewPort(m_posX, m_posY, m_width, m_height);
  float posX = m_posX;
  float posY = m_posY;
  if (m_orientation == VERTICAL)
    posY += (offset * m_layout.Size(m_orientation) - m_scrollOffset);
  else
    posX += (offset * m_layout.Size(m_orientation) - m_scrollOffset);;

  float focusedPosX = 0;
  float focusedPosY = 0;
  CGUIListItem *focusedItem = NULL;
  int current = offset;
  while (posX < m_posX + m_width && posY < m_posY + m_height && m_items.size())
  {
    CGUIListItem *item = m_items[CorrectOffset(current)];
    bool focused = (current == m_offset + m_cursor) && m_bHasFocus;
    // render our item
    if (focused)
    {
      focusedPosX = posX;
      focusedPosY = posY;
      focusedItem = item;
    }
    else
      RenderItem(posX, posY, item, focused);

    // increment our position
    if (m_orientation == VERTICAL)
      posY += focused ? m_focusedLayout.Size(m_orientation) : m_layout.Size(m_orientation);
    else
      posX += focused ? m_focusedLayout.Size(m_orientation) : m_layout.Size(m_orientation);

    current++;
  }
  // render focused item last so it can overlap other items
  if (focusedItem)
    RenderItem(focusedPosX, focusedPosY, focusedItem, true);

  g_graphicsContext.RestoreViewPort();

  if (m_pageControl)
  { // tell our pagecontrol (scrollbar or whatever) to update
    CGUIMessage msg(GUI_MSG_ITEM_SELECT, GetID(), m_pageControl, CorrectOffset(offset));
    SendWindowMessage(msg);
  }
  CGUIBaseContainer::Render();
}

bool CGUIWrappingListContainer::OnAction(const CAction &action)
{
  switch (action.wID)
  {
  case ACTION_PAGE_UP:
    Scroll(-m_itemsPerPage);
    return true;
  case ACTION_PAGE_DOWN:
    Scroll(m_itemsPerPage);
    return true;
    // smooth scrolling (for analog controls)
  case ACTION_SCROLL_UP:
    {
      m_analogScrollCount += action.fAmount1 * action.fAmount1;
      bool handled = false;
      while (m_analogScrollCount > 0.4)
      {
        handled = true;
        m_analogScrollCount -= 0.4f;
        Scroll(-1);
      }
      return handled;
    }
    break;
  case ACTION_SCROLL_DOWN:
    {
      m_analogScrollCount += action.fAmount1 * action.fAmount1;
      bool handled = false;
      while (m_analogScrollCount > 0.4)
      {
        handled = true;
        m_analogScrollCount -= 0.4f;
        Scroll(1);
      }
      return handled;
    }
    break;
  }
  return CGUIBaseContainer::OnAction(action);
}

bool CGUIWrappingListContainer::OnMessage(CGUIMessage& message)
{
  if (message.GetControlId() == GetID() )
  {
    if (message.GetMessage() == GUI_MSG_ITEM_SELECT)
    {
      int item = message.GetParam1();
      if (item >= 0 && item < (int)m_items.size())
        ScrollToOffset(item - m_cursor);
      return true;
    }
  }
  return CGUIBaseContainer::OnMessage(message);
}

bool CGUIWrappingListContainer::MoveUp(DWORD control)
{
  Scroll(-1);
  return true;
}

bool CGUIWrappingListContainer::MoveDown(DWORD control)
{
  Scroll(+1);
  return true;
}

// scrolls the said amount
void CGUIWrappingListContainer::Scroll(int amount)
{
  ScrollToOffset(m_offset + amount);
}

void CGUIWrappingListContainer::ValidateOffset()
{
  // no need to check the range here
}

int CGUIWrappingListContainer::CorrectOffset(int offset) const
{
  if (m_items.size())
  {
    int correctOffset = offset % (int)m_items.size();
    if (correctOffset < 0) correctOffset += m_items.size();
    return correctOffset;
  }
  return 0;
}

void CGUIWrappingListContainer::MoveToItem(int item)
{
  // TODO: Implement this...
  ScrollToOffset(item - m_cursor);
}
