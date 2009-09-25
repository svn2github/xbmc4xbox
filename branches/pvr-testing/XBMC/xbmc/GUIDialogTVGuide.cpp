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

#include "GUIDialogTVGuide.h"
#include "PVRManager.h"
#include "Application.h"
#include "Util.h"
#include "Picture.h"
#include "GUIDialogContextMenu.h"
#include "GUIDialogOK.h"
#include "GUIDialogTVEPGProgInfo.h"
#include "GUIWindowManager.h"
#include "ViewState.h"
#include "Settings.h"
#include "FileItem.h"

using namespace std;

#define CONTROL_LIST                  11

CGUIDialogTVGuide::CGUIDialogTVGuide()
    : CGUIDialog(WINDOW_DIALOG_TV_OSD_GUIDE, "VideoOSDTVGuide.xml")
{
  m_vecItems = new CFileItemList;
}

CGUIDialogTVGuide::~CGUIDialogTVGuide()
{
  delete m_vecItems;
}

bool CGUIDialogTVGuide::OnMessage(CGUIMessage& message)
{
  switch (message.GetMessage())
  {
  case GUI_MSG_WINDOW_DEINIT:
    {
      Clear();
    }
    break;

  case GUI_MSG_WINDOW_INIT:
    {
      CGUIWindow::OnMessage(message);
      Update();
      return true;
    }
    break;

  case GUI_MSG_CLICKED:
    {
      int iControl = message.GetSenderId();

      if (m_viewControl.HasControl(iControl))   // list/thumb control
      {
        int iItem = m_viewControl.GetSelectedItem();
        int iAction = message.GetParam1();

        if (iAction == ACTION_SELECT_ITEM || iAction == ACTION_MOUSE_LEFT_CLICK)
        {
          ShowInfo(iItem);
          return true;
        }
      }
    }
    break;

  case GUI_MSG_SETFOCUS:
    {
      if (m_viewControl.HasControl(message.GetControlId()) && (DWORD) m_viewControl.GetCurrentControl() != message.GetControlId())
      {
        m_viewControl.SetFocused();
        return true;
      }
    }
    break;
  }

  return CGUIDialog::OnMessage(message);
}

void CGUIDialogTVGuide::Update()
{
  // lock our display, as this window is rendered from the player thread
  g_graphicsContext.Lock();
  m_viewControl.SetCurrentView(DEFAULT_VIEW_LIST);

  // empty the list ready for population
  Clear();

  bool RadioPlaying;
  int CurrentChannel;
  g_PVRManager.GetCurrentChannel(&CurrentChannel, &RadioPlaying);
  cPVREpgs::GetEPGChannel(CurrentChannel, m_vecItems, RadioPlaying);

  m_viewControl.SetItems(*m_vecItems);
  g_graphicsContext.Unlock();
}

void CGUIDialogTVGuide::Clear()
{
  m_viewControl.Clear();
  m_vecItems->Clear();
}

void CGUIDialogTVGuide::ShowInfo(int item)
{
  /* Check file item is in list range and get his pointer */
  if (item < 0 || item >= (int)m_vecItems->Size()) return;

  CFileItemPtr pItem = m_vecItems->Get(item);

  /* Load programme info dialog */
  CGUIDialogTVEPGProgInfo* pDlgInfo = (CGUIDialogTVEPGProgInfo*)m_gWindowManager.GetWindow(WINDOW_DIALOG_TV_GUIDE_INFO);

  if (!pDlgInfo)
    return;

  /* inform dialog about the file item */
  pDlgInfo->SetProgInfo(pItem.get());

  /* Open dialog window */
  pDlgInfo->DoModal();
}

void CGUIDialogTVGuide::OnWindowLoaded()
{
  CGUIDialog::OnWindowLoaded();
  m_viewControl.Reset();
  m_viewControl.SetParentWindow(GetID());
  m_viewControl.AddView(GetControl(CONTROL_LIST));
}

void CGUIDialogTVGuide::OnWindowUnload()
{
  CGUIDialog::OnWindowUnload();
  m_viewControl.Reset();
}

CGUIControl *CGUIDialogTVGuide::GetFirstFocusableControl(int id)
{
  if (m_viewControl.HasControl(id))
    id = m_viewControl.GetCurrentControl();

  return CGUIWindow::GetFirstFocusableControl(id);
}
