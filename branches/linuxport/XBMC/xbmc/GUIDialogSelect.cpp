/*
 *      Copyright (C) 2005-2007 Team XboxMediaCenter
 *      http://www.xboxmediacenter.com
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
 *  along with GNU Make; see the file COPYING.  If not, write to
 *  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 *  http://www.gnu.org/copyleft/gpl.html
 *
 */

#include "stdafx.h"
#include "GUIDialogSelect.h"
#include "Application.h"
#include "SortFileItem.h"

#define CONTROL_HEADING       1
#define CONTROL_LIST          3
#define CONTROL_NUMBEROFFILES 2
#define CONTROL_BUTTON        5

CGUIDialogSelect::CGUIDialogSelect(void)
    : CGUIDialogBoxBase(WINDOW_DIALOG_SELECT, "DialogSelect.xml")
{
  m_bButtonEnabled = false;
}

CGUIDialogSelect::~CGUIDialogSelect(void)
{}

bool CGUIDialogSelect::OnMessage(CGUIMessage& message)
{
  switch ( message.GetMessage() )
  {
  case GUI_MSG_WINDOW_DEINIT:
    {
      CGUIDialog::OnMessage(message);
      Reset();
      return true;
    }
    break;

  case GUI_MSG_WINDOW_INIT:
    {
      m_bButtonPressed = false;
      CGUIDialog::OnMessage(message);
      m_iSelected = -1;
      CGUIMessage msg(GUI_MSG_LABEL_RESET, GetID(), CONTROL_LIST, 0, 0, NULL);
      g_graphicsContext.SendMessage(msg);

      for (int i = 0; i < (int)m_vecList.size(); i++)
      {
        //CGUIListItem* pItem = m_vecList[i];
        CFileItem* pItem = m_vecList[i];
        CGUIMessage msg(GUI_MSG_LABEL_ADD, GetID(), CONTROL_LIST, 0, 0, (void*)pItem);
        g_graphicsContext.SendMessage(msg);
      }
      CStdString items;
      items.Format("%i %s", m_vecList.size(), g_localizeStrings.Get(127).c_str());
      SET_CONTROL_LABEL(CONTROL_NUMBEROFFILES, items);

      if (m_bButtonEnabled)
      {
        CGUIMessage msg2(GUI_MSG_VISIBLE, GetID(), CONTROL_BUTTON, 0, 0, NULL);
        g_graphicsContext.SendMessage(msg2);
      }
      else
      {
        CGUIMessage msg2(GUI_MSG_HIDDEN, GetID(), CONTROL_BUTTON, 0, 0, NULL);
        g_graphicsContext.SendMessage(msg2);
      }
      return true;
    }
    break;


  case GUI_MSG_CLICKED:
    {

      int iControl = message.GetSenderId();
      if (CONTROL_LIST == iControl)
      {
        int iAction = message.GetParam1();
        if (ACTION_SELECT_ITEM == iAction)
        {
          CGUIMessage msg(GUI_MSG_ITEM_SELECTED, GetID(), iControl, 0, 0, NULL);
          g_graphicsContext.SendMessage(msg);            
          m_iSelected = msg.GetParam1();
          if(m_iSelected >= 0 && m_iSelected < (int)m_vecList.size())
          {
            m_selectedItem = *m_vecList[m_iSelected];
            Close();
          }
          else
            m_iSelected = -1;
        }
      }
      if (CONTROL_BUTTON == iControl)
      {
        m_iSelected = -1;
        m_bButtonPressed = true;
        Close();
      }
    }
    break;
  }

  return CGUIDialog::OnMessage(message);
}

void CGUIDialogSelect::Close(bool forceClose)
{
  CGUIDialog::Close(forceClose);
}

void CGUIDialogSelect::Reset()
{
  m_bButtonEnabled = false;
  for (int i = 0; i < (int)m_vecList.size(); ++i)
  {
    //CGUIListItem* pItem = m_vecList[i];
    CFileItem* pItem = m_vecList[i];
    delete pItem;
  }
  m_vecList.erase(m_vecList.begin(), m_vecList.end());
}

void CGUIDialogSelect::Add(const CStdString& strLabel)
{
  //CGUIListItem* pItem = new CGUIListItem(strLabel);
  CFileItem* pItem = new CFileItem(strLabel);
  m_vecList.push_back(pItem);
}

void CGUIDialogSelect::Add(CFileItem* pItem)
{
  m_vecList.push_back(pItem);
}

int CGUIDialogSelect::GetSelectedLabel() const
{
  return m_iSelected;
}

const CFileItem& CGUIDialogSelect::GetSelectedItem()
{
  return m_selectedItem;
}

const CStdString& CGUIDialogSelect::GetSelectedLabelText()
{
  return m_selectedItem.GetLabel();
}

void CGUIDialogSelect::EnableButton(bool bOnOff)
{
  m_bButtonEnabled = bOnOff;
}

void CGUIDialogSelect::SetButtonLabel(int iString)
{
  CGUIMessage msg(GUI_MSG_LABEL_SET, GetID(), CONTROL_BUTTON);
  msg.SetLabel(iString);
  OnMessage(msg);
}

bool CGUIDialogSelect::IsButtonPressed()
{
  return m_bButtonPressed;
}

void CGUIDialogSelect::Sort(bool bSortOrder /*=true*/)
{
  sort(m_vecList.begin(), m_vecList.end(), bSortOrder ? SSortFileItem::LabelAscending : SSortFileItem::LabelDescending);
}

void CGUIDialogSelect::SetSelected(int iSelected)
{
  if (iSelected < 0 || iSelected >= (int)m_vecList.size()) return;
  m_iSelected = iSelected;
}

