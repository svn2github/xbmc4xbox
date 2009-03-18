#pragma once

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

#include "GUIDialog.h"
#include "GUIViewControl.h"
#include "Addon.h"

class CFileItem;
class CFileItemList;

namespace ADDON
{

class CGUIDialogAddonBrowser : public CGUIDialog
{
public:
  CGUIDialogAddonBrowser(void);
  virtual ~CGUIDialogAddonBrowser(void);
  virtual bool OnMessage(CGUIMessage& message);
  virtual bool OnAction(const CAction &action);
  virtual void Render();
  virtual void OnWindowLoaded();
  virtual void OnWindowUnload();
  bool IsConfirmed() { return m_confirmed; };
  void SetHeading(const CStdString &heading);

  static bool ShowAndGetAddons(const AddonType &type, const bool activeOnly);

  void SetAddonType(const AddonType &type);

  virtual bool HasListItems() const { return true; };
  virtual CFileItemPtr GetCurrentListItem(int offset = 0);

protected:
  void OnClick(int iItem);
  void OnSort();
  void ClearFileItems();
  void Update();
  bool OnContextMenu(int iItem);
  void OnGetAddons(const AddonType &type);
  
  AddonType m_type;
  CFileItemList* m_vecItems;

  bool m_confirmed;
  bool m_changed;
  inline void SetAddNew(bool activeOnly) { m_getAddons = !activeOnly; };
  bool m_getAddons;
  CGUIViewControl m_viewControl;
};

};
