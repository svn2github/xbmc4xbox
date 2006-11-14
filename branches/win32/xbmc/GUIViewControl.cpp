#include "stdafx.h"
#include "GUIViewControl.h"
#include "GUIThumbnailPanel.h"
#include "Util.h"

CGUIViewControl::CGUIViewControl(void)
{
  m_viewAsControl = -1;
  m_parentWindow = WINDOW_INVALID;
  m_fileItems = NULL;
  Reset();
}

CGUIViewControl::~CGUIViewControl(void)
{
}

void CGUIViewControl::Reset()
{
  m_currentView = VIEW_METHOD_NONE;
  m_vecViews.clear();
}

void CGUIViewControl::AddView(VIEW_METHOD type, const CGUIControl *control)
{
  if (!control) return;
  m_vecViews.insert(pair<VIEW_METHOD, CGUIControl *>(type, (CGUIControl *)control));
}

void CGUIViewControl::SetViewControlID(int control)
{
  m_viewAsControl = control;
}

void CGUIViewControl::SetParentWindow(int window)
{
  m_parentWindow = window;
}

void CGUIViewControl::SetCurrentView(VIEW_METHOD viewMode)
{
  if (m_currentView == viewMode)
    return;
//  CLog::DebugLog("SetCurrentView: Oldview: %i, Newview :%i", m_currentView, viewMode);
  map_iter it = m_vecViews.find(viewMode);
  if (it == m_vecViews.end()) return;

  CGUIControl *pNewView = (*it).second;

  bool hasFocus(false);
  int item = -1;
  map_iter it_old = m_vecViews.find(m_currentView);
  if (it_old != m_vecViews.end())
  {
    // have an old view - let's clear it out and hide it.
    CGUIControl *pControl = (*it_old).second;
    hasFocus = pControl->HasFocus();
    item = GetSelectedItem(pControl);
    CGUIMessage msg(GUI_MSG_LABEL_RESET, m_parentWindow, pControl->GetID(), 0, 0, NULL);
    pControl->OnMessage(msg);
  }
  m_currentView = viewMode;
  // make only current control visible...
  for (map_iter view = m_vecViews.begin(); view != m_vecViews.end(); view++)
  {
    CGUIControl *control = (*view).second;
    control->SetVisible(false);
  }
  pNewView->SetVisible(true);
  // and focus if necessary
  if (hasFocus)
  {
    CGUIMessage msg(GUI_MSG_SETFOCUS, m_parentWindow, pNewView->GetID(), 0);
    g_graphicsContext.SendMessage(msg);
  }
  // if we have a thumbs view, make sure we have set the appropriate size...
  if (pNewView->GetControlType() == CGUIControl::GUICONTROL_THUMBNAIL)
  {
    if (viewMode == VIEW_METHOD_LARGE_ICONS)
      ((CGUIThumbnailPanel *)pNewView)->ShowBigIcons(true);
    else
      ((CGUIThumbnailPanel *)pNewView)->ShowBigIcons(false);
  }
  // Update it with the contents
  UpdateContents(pNewView);
  if (item > -1)
  {
    CGUIMessage msg(GUI_MSG_ITEM_SELECT, m_parentWindow, pNewView->GetID(), item);
    g_graphicsContext.SendMessage(msg);
  }
  // And update our "view as" button control
  UpdateViewAsControl();
}

void CGUIViewControl::SetItems(CFileItemList &items)
{
//  CLog::DebugLog("SetItems: %i", m_currentView);
  m_fileItems = &items;
  // update our current view control...
  UpdateView();
}

void CGUIViewControl::UpdateContents(const CGUIControl *control)
{
  if (!control || !m_fileItems) return;
  // reset the current view
  CGUIMessage msg1(GUI_MSG_LABEL_RESET, m_parentWindow, control->GetID(), 0, 0, NULL);
  g_graphicsContext.SendMessage(msg1);

  // add the items to the current view
  for (int i = 0; i < m_fileItems->Size(); i++)
  {
    CFileItem* pItem = (*m_fileItems)[i];
    // free it's memory, to make sure any icons etc. are loaded as needed.
    pItem->FreeMemory();
    CGUIMessage msg(GUI_MSG_LABEL_ADD, m_parentWindow, control->GetID(), 0, 0, (void*)pItem);
    g_graphicsContext.SendMessage(msg);
  }
}

void CGUIViewControl::UpdateView()
{
//  CLog::DebugLog("UpdateView: %i", m_currentView);
  map_iter it = m_vecViews.find(m_currentView);
  if (it == m_vecViews.end())
    return; // no valid current view!

  CGUIControl *pControl = (*it).second;
  // get the currently selected item
  int item = GetSelectedItem(pControl);
  UpdateContents(pControl);
  // set the current item
  if (item < 0) item = 0;
  CGUIMessage msg(GUI_MSG_ITEM_SELECT, m_parentWindow, pControl->GetID(), item);
  g_graphicsContext.SendMessage(msg);
}

int CGUIViewControl::GetSelectedItem(const CGUIControl *control) const
{
  if (!control || !m_fileItems) return -1;
  CGUIMessage msg(GUI_MSG_ITEM_SELECTED, m_parentWindow, control->GetID(), 0, 0, NULL);
  g_graphicsContext.SendMessage(msg);

  int iItem = msg.GetParam1();
  if (iItem >= m_fileItems->Size())
    return -1;
  return iItem;
}

int CGUIViewControl::GetSelectedItem() const
{
  map_iter it = m_vecViews.find(m_currentView);
  if (it == m_vecViews.end())
    return -1; // no valid current view!

  CGUIControl *pControl = (*it).second;
  return GetSelectedItem(pControl);
}

void CGUIViewControl::SetSelectedItem(int item)
{
  if (!m_fileItems || item < 0 || item >= m_fileItems->Size())
    return;

  map_iter it = m_vecViews.find(m_currentView);
  if (it == m_vecViews.end())
    return; // no valid current view!

  CGUIControl *pControl = (*it).second;
  CGUIMessage msg(GUI_MSG_ITEM_SELECT, m_parentWindow, pControl->GetID(), item);
  g_graphicsContext.SendMessage(msg);
}

void CGUIViewControl::SetSelectedItem(const CStdString &itemPath)
{
  if (!m_fileItems || itemPath == "")
    return;

  int item = -1;
  for (int i = 0; i < m_fileItems->Size(); ++i)
  {
    CStdString strPath =(*m_fileItems)[i]->m_strPath;
    CUtil::RemoveSlashAtEnd(strPath);
    if (strPath.CompareNoCase(itemPath) == 0)
    {
      item = i;
      break;
    }
  }
  SetSelectedItem(item);
}

void CGUIViewControl::SetFocused()
{
  map_iter it = m_vecViews.find(m_currentView);
  if (it == m_vecViews.end())
    return; // no valid current view!

  CGUIControl *pView = (*it).second;
  CGUIMessage msg(GUI_MSG_SETFOCUS, m_parentWindow, pView->GetID(), 0);
  g_graphicsContext.SendMessage(msg);
}

bool CGUIViewControl::HasControl(int viewControlID)
{
  // run through our controls, checking for the id
  for (map_iter it = m_vecViews.begin(); it != m_vecViews.end(); it++)
  {
    CGUIControl *pControl = (*it).second;
    if (pControl->GetID() == viewControlID)
      return true;
  }
  return false;
}

bool CGUIViewControl::HasViewMode(VIEW_METHOD viewMode)
{
  map_iter it = m_vecViews.find(viewMode);
  return (it!=m_vecViews.end());
}

int CGUIViewControl::GetCurrentControl()
{
  map_iter it = m_vecViews.find(m_currentView);
  if (it == m_vecViews.end())
    return -1; // no valid current view!

  return (*it).second->GetID();
}

void CGUIViewControl::Clear()
{
  map_iter it = m_vecViews.find(m_currentView);
  if (it == m_vecViews.end())
    return; // no valid current view!

  CGUIControl *pView = (*it).second;
  CGUIMessage msg(GUI_MSG_LABEL_RESET, m_parentWindow, pView->GetID(), 0);
  g_graphicsContext.SendMessage(msg);
}

void CGUIViewControl::UpdateViewAsControl()
{
  if (m_viewAsControl < 0)
    return;

  int iString;
  switch (m_currentView)
  {
  case VIEW_METHOD_LIST:
    iString = 101; // View: List
    break;
  case VIEW_METHOD_ICONS:
    iString = 100; // View: Icons
    break;
  case VIEW_METHOD_LARGE_ICONS:
    iString = 417; // View: Big Icons
    break;
  case VIEW_METHOD_LARGE_LIST:
    iString = 759; // View: Big List
    break;
  }
  CGUIMessage msg(GUI_MSG_LABEL_SET, m_parentWindow, m_viewAsControl);
  msg.SetLabel(iString);
  g_graphicsContext.SendMessage(msg);
}
