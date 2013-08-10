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

#include "addons/Addon.h"
#include "windows/GUIMediaWindow.h"
#include "utils/CriticalSection.h"
#include "utils/Job.h"
#include "pictures/PictureThumbLoader.h"

class CFileItem;
class CFileItemList;
class CFileOperationJob;

class CGUIWindowAddonBrowser :
      public CGUIMediaWindow,
      public IJobCallback
{
public:
  CGUIWindowAddonBrowser(void);
  virtual ~CGUIWindowAddonBrowser(void);
  virtual bool OnMessage(CGUIMessage& message);
//  virtual bool OnAction(const CAction &action);

  void RegisterJob(const CStdString& id, CFileOperationJob* job,
                   unsigned int jobid);

  // job callback
  void OnJobComplete(unsigned int jobID, bool success, CJob* job);

  static std::pair<CFileOperationJob*,unsigned int> AddJob(const CStdString& path);

  /*! \brief Popup a selection dialog with a list of addons of the given type
   \param type the type of addon wanted
   \param addonID [out] the addon ID of the selected item
   \param showNone whether there should be a "None" item in the list (defaults to false)
   \return true if an addon was selected, false if an error occurred or if the selection process was cancelled
   */
  static bool SelectAddonID(ADDON::TYPE type, CStdString &addonID, bool showNone = false);

  /*! \brief Install an addon if it is available in a repository
   \param addonID the addon ID of the item to install
   \param force whether to force the install even if the addon is already installed (eg for updating). Defaults to false.
   */
  static void InstallAddon(const CStdString &addonID, bool force = false);

  /*! \brief Install a set of addons from the official repository (if needed)
   \param addonIDs a set of addon IDs to install
   */
  static void InstallAddonsFromXBMCRepo(const std::set<CStdString> &addonIDs);

protected:
  void UnRegisterJob(CFileOperationJob* job);
  virtual void GetContextButtons(int itemNumber, CContextButtons &buttons);
  virtual bool OnContextButton(int itemNumber, CONTEXT_BUTTON button);
  virtual bool OnClick(int iItem);
  virtual void UpdateButtons();
  virtual bool GetDirectory(const CStdString &strDirectory, CFileItemList &items);
  virtual bool Update(const CStdString &strDirectory);
  std::map<CStdString,CFileOperationJob*> m_idtojob;
  std::map<CStdString,unsigned int> m_idtojobid;
  std::map<CFileOperationJob*,CStdString> m_jobtoid;
  CCriticalSection m_critSection;
  CPictureThumbLoader m_thumbLoader;
  CStdString m_startDirectory;
};

