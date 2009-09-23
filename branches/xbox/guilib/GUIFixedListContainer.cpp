/*
 *      Copyright (C) 2005-2008 Team XBMC
 *      http://www.xbmc.org
 *
 *  This Program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your option)
 *  any later version.
 *
 *  This Program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with XBMC; see the file COPYING.  If not, write to
 *  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 *  http://www.gnu.org/copyleft/gpl.html
 *
 */

#include "include.h"
#include "GUIFixedListContainer.h"
#include "GUIListItem.h"
#include "utils/GUIInfoManager.h"

CGUIFixedListContainer::CGUIFixedListContainer(DWORD dwParentID, DWORD dwControlId, float posX, float posY, float width, float height, ORIENTATION orientation, int scrollTime, int preloadItems, int fixedPosition)
    : CGUIBaseContainer(dwParentID, dwControlId, posX, posY, width, height, orientation, scrollTime, preloadItems)
{
  ControlType = GUICONTAINER_FIXEDLIST;
  m_type = VIEW_TYPE_LIST;
  m_cursor = fixedPosition;
}

CGUIFixedListContainer::~CGUIFixedListContainer(void)
{
}

bool CGUIFixedListContainer::OnAction(const CAction &action)
{
  switch (action.wID)
  {
  case ACTION_PAGE_UP:
    {
      if (m_offset + m_cursor < m_itemsPerPage)
      { // already on the first page, so move to the first item
        Scroll(m_offset + m_cursor - m_itemsPerPage);
      }
      else
      { // scroll up to the previous page
        Scroll(-m_itemsPerPage);
      }
      return true;
    }
    break;
  case ACTION_PAGE_DOWN:
    {
      if (m_offset + m_cursor >= (int)m_items.size() || (int)m_items.size() < m_itemsPerPage)
      { // already at the last page, so move to the last item.
        Scroll(m_items.size() - 1 - m_offset + m_cursor);
      }
      else
      { // scroll down to the next page
        Scroll(m_itemsPerPage);
      }
      return true;
    }
    break;
    // smooth scrolling (for analog controls)
  case ACTION_SCROLL_UP:
    {
      m_analogScrollCount += action.fAmount1 * action.fAmount1;
      bool handled = false;
      while (m_analogScrollCount > 0.4)
      {
        handled = true;
        m_analogScrollCount -= 0.4f;
        if (m_offset > -m_cursor)
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
        if (m_offset < (int)m_items.size() - 1)
          Scroll(1);
      }
      return handled;
    }
    break;
  }
  return CGUIBaseContainer::OnAction(action);
}

bool CGUIFixedListContainer::OnMessage(CGUIMessage& message)
{
  if (message.GetControlId() == GetID() )
  {
    if (message.GetMessage() == GUI_MSG_ITEM_SELECT)
    {
      SelectItem(message.GetParam1());
      return true;
    }
  }
  return CGUIBaseContainer::OnMessage(message);
}

bool CGUIFixedListContainer::MoveUp(bool wrapAround)
{
  if (m_offset > -m_cursor)
    ScrollToOffset(m_offset - 1);
  else if (wrapAround)
  {
    if (m_items.size() > 0)
    { // move 2 last item in list
      int offset = m_items.size() - m_cursor - 1;
      if (offset < -m_cursor) offset = -m_cursor;
      ScrollToOffset(offset);
      g_infoManager.SetContainerMoving(GetID(), -1);
    }
  }
  else
    return false;
  return true;
}

bool CGUIFixedListContainer::MoveDown(bool wrapAround)
{
  if (m_offset + m_cursor + 1 < (int)m_items.size())
  {
    ScrollToOffset(m_offset + 1);
  }
  else if (wrapAround)
  { // move first item in list
    ScrollToOffset(-m_cursor);
    g_infoManager.SetContainerMoving(GetID(), 1);
  }
  else
    return false;
  return true;
}

// scrolls the said amount
void CGUIFixedListContainer::Scroll(int amount)
{
  // increase or decrease the offset
  int offset = m_offset + amount;
  if (offset >= (int)m_items.size() - m_cursor)
  {
    offset = m_items.size() - m_cursor - 1;
  }
  if (offset < -m_cursor) offset = -m_cursor;
  ScrollToOffset(offset);
}

void CGUIFixedListContainer::ValidateOffset()
{ // first thing is we check the range of m_offset
  if (!m_layout) return;
  if (m_offset > (int)m_items.size() - m_cursor)
  {
    m_offset = m_items.size() - m_cursor;
    m_scrollOffset = m_offset * m_layout->Size(m_orientation);
  }
  if (m_offset < -m_cursor)
  {
    m_offset = -m_cursor;
    m_scrollOffset = m_offset * m_layout->Size(m_orientation);
  }
}

bool CGUIFixedListContainer::SelectItemFromPoint(const CPoint &point)
{
  if (!m_focusedLayout || !m_layout)
    return false;

  const float mouse_scroll_speed = 0.05f;
  const float mouse_max_amount = 1.5f;   // max speed: 1 item per frame
  float sizeOfItem = m_layout->Size(m_orientation);
  // see if the point is either side of our focused item
  float start = m_cursor * sizeOfItem;
  float end = start + m_focusedLayout->Size(m_orientation);
  float pos = (m_orientation == VERTICAL) ? point.y : point.x;
  if (pos < start - 0.5f * sizeOfItem)
  { // scroll backward
    if (!InsideLayout(m_layout, point))
      return false;
    float amount = min((start - pos) / sizeOfItem, mouse_max_amount);
    m_analogScrollCount += amount * amount * mouse_scroll_speed;
    if (m_analogScrollCount > 1)
    {
      Scroll(-1);
      m_analogScrollCount-=1.0f;
    }
    return true;
  }
  else if (pos > end + 0.5f * sizeOfItem)
  {
    if (!InsideLayout(m_layout, point))
      return false;
    // scroll forward
    float amount = min((pos - end) / sizeOfItem, mouse_max_amount);
    m_analogScrollCount += amount * amount * mouse_scroll_speed;
    if (m_analogScrollCount > 1)
    {
      Scroll(1);
      m_analogScrollCount-=1.0f;
    }
    return true;
  }
  return InsideLayout(m_focusedLayout, point);
}

void CGUIFixedListContainer::SelectItem(int item)
{
  // Check that m_offset is valid
  ValidateOffset();
  // only select an item if it's in a valid range
  if (item >= 0 && item < (int)m_items.size())
  {
    // Select the item requested
    ScrollToOffset(item - m_cursor);
  }
}

bool CGUIFixedListContainer::HasPreviousPage() const
{
  return (m_offset > 0);
}

bool CGUIFixedListContainer::HasNextPage() const
{
  return (m_offset != (int)m_items.size() - m_itemsPerPage && (int)m_items.size() >= m_itemsPerPage);
}

int CGUIFixedListContainer::GetCurrentPage() const
{
  int offset = CorrectOffset(m_offset, m_cursor);
  if (offset + m_itemsPerPage - m_cursor >= (int)GetRows())  // last page
    return (GetRows() + m_itemsPerPage - 1) / m_itemsPerPage;
  return offset / m_itemsPerPage + 1;
}

