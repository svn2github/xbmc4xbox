#pragma once
/*
 *      Copyright (C) 2005-2010 Team XBMC
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

#include "GUIDialog.h"
#include "GUIViewControl.h"

class CFileItemList;

class CGUIDialogPVRGroupManager : public CGUIDialog
{
public:
  CGUIDialogPVRGroupManager(void);
  virtual ~CGUIDialogPVRGroupManager(void);
  virtual bool OnMessage(CGUIMessage& message);
  virtual void OnWindowLoaded();
  virtual void OnWindowUnload();
  void SetRadio(bool IsRadio) { m_bIsRadio = IsRadio; }

protected:
  void Clear();
  void Update();

  CStdString      m_CurrentGroupName;
  bool            m_bIsRadio;

  unsigned int    m_iSelectedLeft;
  unsigned int    m_iSelectedRight;
  unsigned int    m_iSelectedGroup;

  CFileItemList  *m_channelLeftItems;
  CFileItemList  *m_channelRightItems;
  CFileItemList  *m_channelGroupItems;

  CGUIViewControl m_viewControlLeft;
  CGUIViewControl m_viewControlRight;
  CGUIViewControl m_viewControlGroup;
};
