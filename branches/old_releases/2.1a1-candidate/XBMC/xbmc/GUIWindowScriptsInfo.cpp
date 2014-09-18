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
#include "GUIWindowScriptsInfo.h"


#define CONTROL_TEXTAREA 5

CGUIWindowScriptsInfo::CGUIWindowScriptsInfo(void)
    : CGUIDialog(WINDOW_SCRIPTS_INFO, "DialogScriptInfo.xml")
{}

CGUIWindowScriptsInfo::~CGUIWindowScriptsInfo(void)
{}

bool CGUIWindowScriptsInfo::OnAction(const CAction &action)
{
  if (action.wID == ACTION_SHOW_INFO)
  {
    // erase debug screen
    strInfo = "";
    CGUIMessage msg(GUI_MSG_LABEL_SET, GetID(), CONTROL_TEXTAREA);
    msg.SetLabel(strInfo);
    OnMessage(msg);
    return true;
  }
  return CGUIDialog::OnAction(action);
}

bool CGUIWindowScriptsInfo::OnMessage(CGUIMessage& message)
{
  switch ( message.GetMessage() )
  {
  case GUI_MSG_WINDOW_INIT:
    {
      CGUIDialog::OnMessage(message);
      CGUIMessage msg(GUI_MSG_LABEL_SET, GetID(), CONTROL_TEXTAREA);
      msg.SetLabel(strInfo);
      msg.SetParam1(5); // 5 pages max
      OnMessage(msg);
      return true;
    }
    break;

  case GUI_MSG_USER:
    {
      strInfo += message.GetLabel();
      CGUIMessage msg(GUI_MSG_LABEL_SET, GetID(), CONTROL_TEXTAREA);
      msg.SetLabel(strInfo);
      msg.SetParam1(5); // 5 pages max
      OnMessage(msg);
    }
    break;
  }
  return CGUIDialog::OnMessage(message);
}
