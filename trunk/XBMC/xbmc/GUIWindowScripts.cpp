
#include "stdafx.h"
#include "guiwindowscripts.h"
#include "util.h"
#include "detectdvdtype.h"
#include "lib/libPython/XBPython.h"
#include "GUIWindowScriptsInfo.h"
#include "SortFileItem.h"
#ifdef PRE_SKIN_VERSION_2_0_COMPATIBILITY
#include "SkinInfo.h"
#endif

#define CONTROL_BTNVIEWASICONS     2
#define CONTROL_BTNSORTBY          3
#define CONTROL_BTNSORTASC         4
#define CONTROL_LIST              50
#define CONTROL_THUMBS            51
#define CONTROL_LABELFILES        12


CGUIWindowScripts::CGUIWindowScripts()
    : CGUIWindow(WINDOW_SCRIPTS, "MyScripts.xml")
{
  m_bDVDDiscChanged = false;
  m_bDVDDiscEjected = false;
  m_bViewOutput = false;
  m_Directory.m_strPath = "?";
  m_Directory.m_bIsFolder = true;
  scriptSize = 0;
  m_iLastControl = -1;
  m_iSelectedItem = -1;
}

CGUIWindowScripts::~CGUIWindowScripts()
{}


bool CGUIWindowScripts::OnAction(const CAction &action)
{
  if (action.wID == ACTION_PARENT_DIR)
  {
    GoParentFolder();
    return true;
  }

  if (action.wID == ACTION_PREVIOUS_MENU)
  {
    m_gWindowManager.PreviousWindow();
    return true;
  }

  if (action.wID == ACTION_SHOW_INFO)
  {
    OnInfo();
    return true;
  }
  return CGUIWindow::OnAction(action);
}

bool CGUIWindowScripts::OnMessage(CGUIMessage& message)
{
  switch ( message.GetMessage() )
  {
  case GUI_MSG_WINDOW_DEINIT:
    {
      m_iSelectedItem = m_viewControl.GetSelectedItem();
      m_iLastControl = GetFocusedControl();
      ClearFileItems();
    }
    break;

  case GUI_MSG_WINDOW_INIT:
    {
      int iLastControl = m_iLastControl;
      shares.erase(shares.begin(), shares.end());
      CGUIWindow::OnMessage(message);
      if (m_Directory.m_strPath == "?") m_Directory.m_strPath = "Q:\\scripts"; //g_stSettings.m_szDefaultScripts;

      m_rootDir.SetMask("*.py");

      CShare share;
      share.strName = "Q Drive";
      share.strPath = "Q:\\";
      share.m_iBufferSize = 0;
      share.m_iDriveType = SHARE_TYPE_LOCAL;
      shares.push_back(share);

      m_rootDir.SetShares(shares); //g_settings.m_vecScriptShares);

      Update(m_Directory.m_strPath);

      if (iLastControl > -1)
      {
        SET_CONTROL_FOCUS(iLastControl, 0);
      }
      else
      {
        SET_CONTROL_FOCUS(m_dwDefaultFocusControlID, 0);
      }

      if (m_iSelectedItem > -1)
        m_viewControl.SetSelectedItem(m_iSelectedItem);

      return true;
    }
    break;

  case GUI_MSG_CLICKED:
    {
      int iControl = message.GetSenderId();
      if (iControl == CONTROL_BTNVIEWASICONS)
      {
        if ( m_Directory.IsVirtualDirectoryRoot() )
          g_stSettings.m_bScriptsRootViewAsIcons = !g_stSettings.m_bScriptsRootViewAsIcons;
        else
          g_stSettings.m_bScriptsViewAsIcons = !g_stSettings.m_bScriptsViewAsIcons;
        g_settings.Save();
        UpdateButtons();
      }
      else if (iControl == CONTROL_BTNSORTBY) // sort by
      {
        g_stSettings.m_iScriptsSortMethod++;
        if (g_stSettings.m_iScriptsSortMethod >= 3) g_stSettings.m_iScriptsSortMethod = 0;
        g_settings.Save();
        UpdateButtons();
        OnSort();
      }
      else if (iControl == CONTROL_BTNSORTASC) // sort asc
      {
        g_stSettings.m_bScriptsSortAscending = !g_stSettings.m_bScriptsSortAscending;
        g_settings.Save();
        UpdateButtons();
        OnSort();
      }

      else if (m_viewControl.HasControl(iControl))  // list/thumb control
      {
        int iItem = m_viewControl.GetSelectedItem();
        int iAction = message.GetParam1();
        if (iItem < 0) break;
        if (iAction == ACTION_SELECT_ITEM || iAction == ACTION_MOUSE_LEFT_CLICK)
        {
          OnClick(iItem);
        }
      }
    }
    break;
  case GUI_MSG_SETFOCUS:
    {
      if (m_viewControl.HasControl(message.GetControlId()) && m_viewControl.GetCurrentControl() != message.GetControlId())
      {
        m_viewControl.SetFocused();
        return true;
      }
    }
    break;
  }
  return CGUIWindow::OnMessage(message);
}

void CGUIWindowScripts::UpdateButtons()
{
  if (m_Directory.IsVirtualDirectoryRoot())
    m_viewControl.SetCurrentView(g_stSettings.m_bScriptsRootViewAsIcons);
  else
    m_viewControl.SetCurrentView(g_stSettings.m_bScriptsViewAsIcons);

  SET_CONTROL_LABEL(CONTROL_BTNSORTBY, g_stSettings.m_iScriptsSortMethod + 103);

  if ( g_stSettings.m_bScriptsSortAscending)
  {
    CGUIMessage msg(GUI_MSG_DESELECTED, GetID(), CONTROL_BTNSORTASC);
    g_graphicsContext.SendMessage(msg);
  }
  else
  {
    CGUIMessage msg(GUI_MSG_SELECTED, GetID(), CONTROL_BTNSORTASC);
    g_graphicsContext.SendMessage(msg);
  }

  int iItems = m_vecItems.Size();
  if (iItems)
  {
    CFileItem* pItem = m_vecItems[0];
    if (pItem->GetLabel() == "..") iItems--;
  }
  WCHAR wszText[20];
  const WCHAR* szText = g_localizeStrings.Get(127).c_str();
  swprintf(wszText, L"%i %s", iItems, szText);
  SET_CONTROL_LABEL(CONTROL_LABELFILES, wszText);

}

void CGUIWindowScripts::ClearFileItems()
{
  m_viewControl.Clear();
  m_vecItems.Clear(); // will clean up everything
}

void CGUIWindowScripts::OnSort()
{
  for (int i = 0; i < m_vecItems.Size(); i++)
  {
    CFileItem* pItem = m_vecItems[i];
    if (g_stSettings.m_iScriptsSortMethod == 0 || g_stSettings.m_iScriptsSortMethod == 2)
    {
      if (pItem->m_bIsFolder)
        pItem->SetLabel2("");
      else
        pItem->SetFileSizeLabel();
    }
    else
    {
      if (pItem->m_stTime.wYear)
      {
        CStdString strDateTime;
        CUtil::GetDate(pItem->m_stTime, strDateTime);
        pItem->SetLabel2(strDateTime);
      }
      else
        pItem->SetLabel2("");
    }
  }

  int sortMethod = g_stSettings.m_iScriptsSortMethod;
  bool sortAscending = g_stSettings.m_bScriptsSortAscending;
  switch (sortMethod)
  {
  case 1:
    m_vecItems.Sort(sortAscending ? SSortFileItem::DateAscending : SSortFileItem::DateDescending); break;
  case 2:
    m_vecItems.Sort(sortAscending ? SSortFileItem::SizeAscending : SSortFileItem::SizeDescending); break;
  default:
    m_vecItems.Sort(sortAscending ? SSortFileItem::LabelAscending : SSortFileItem::LabelDescending); break;
  }

  m_viewControl.SetItems(m_vecItems);
}

void CGUIWindowScripts::Update(const CStdString &strDirectory)
{
  // get selected item
  int iItem = m_viewControl.GetSelectedItem();
  CStdString strSelectedItem = "";
  if (iItem >= 0 && iItem < m_vecItems.Size())
  {
    CFileItem* pItem = m_vecItems[iItem];
    if (pItem->GetLabel() != "..")
    {
      strSelectedItem = pItem->m_strPath;
      m_history.Set(strSelectedItem, m_Directory.m_strPath);
    }
  }
  ClearFileItems();

  CStdString strParentPath;
  bool bParentExists = false;
  if (strDirectory != "Q:\\scripts")
    bParentExists = CUtil::GetParentPath(strDirectory, strParentPath);

  // check if current directory is a root share
  if ( !m_rootDir.IsShare(strDirectory) )
  {
    // no, do we got a parent dir?
    if ( bParentExists )
    {
      // yes
      if (!g_guiSettings.GetBool("FileLists.HideParentDirItems"))
      {
        CFileItem *pItem = new CFileItem("..");
        pItem->m_strPath = strParentPath;
        pItem->m_bIsFolder = true;
        pItem->m_bIsShareOrDrive = false;
        m_vecItems.Add(pItem);
      }
      m_strParentPath = strParentPath;
    }
  }
  else
  {
    // yes, this is the root of a share
    // add parent path to the virtual directory
    if (!g_guiSettings.GetBool("FileLists.HideParentDirItems"))
    {
      CFileItem *pItem = new CFileItem("..");
      pItem->m_strPath = "";
      pItem->m_bIsShareOrDrive = false;
      pItem->m_bIsFolder = true;
      m_vecItems.Add(pItem);
    }
    m_strParentPath = "";
  }

  m_Directory.m_strPath = strDirectory;
  m_rootDir.GetDirectory(strDirectory, m_vecItems);
  m_vecItems.SetThumbs();
  if (g_guiSettings.GetBool("FileLists.HideExtensions"))
    m_vecItems.RemoveExtensions();

  m_vecItems.FillInDefaultIcons();

  /* check if any python scripts are running. If true, place "(Running)" after the item.
   * since stopping a script can take up to 10 seconds or more,we display 'stopping'
   * after the filename for now.
   */
  int iSize = g_pythonParser.ScriptsSize();
  for (int i = 0; i < iSize; i++)
  {
    int id = g_pythonParser.GetPythonScriptId(i);
    if (g_pythonParser.isRunning(id))
    {
      const char* filename = g_pythonParser.getFileName(id);

      for (int i = 0; i < m_vecItems.Size(); i++)
      {
        CFileItem* pItem = m_vecItems[i];
        if (pItem->m_strPath == filename)
        {
          char tstr[1024];
          strcpy(tstr, pItem->GetLabel());
          if (g_pythonParser.isStopping(id))
            strcat(tstr, " (Stopping)");
          else
            strcat(tstr, " (Running)");
          pItem->SetLabel(tstr);
        }
      }
    }
  }

  m_iLastControl = GetFocusedControl();
  OnSort();
  UpdateButtons();

  strSelectedItem = m_history.Get(m_Directory.m_strPath);

  for (int i = 0; i < (int)m_vecItems.Size(); ++i)
  {
    CFileItem* pItem = m_vecItems[i];
    if (pItem->m_strPath == strSelectedItem)
    {
      m_viewControl.SetSelectedItem(i);
      break;
    }
  }
}

void CGUIWindowScripts::OnClick(int iItem)
{
  if ( iItem < 0 || iItem >= (int)m_vecItems.Size() ) return ;
  CFileItem* pItem = m_vecItems[iItem];
  CStdString strPath = pItem->m_strPath;

  CStdString strExtension;
  CUtil::GetExtension(pItem->m_strPath, strExtension);

  if (pItem->m_bIsFolder)
  {
    if ( pItem->m_bIsShareOrDrive )
    {
      if ( !HaveDiscOrConnection( pItem->m_strPath, pItem->m_iDriveType ) )
        return ;
    }
    Update(strPath);
  }
  else
  {
    /* execute script...
     * if script is already running do not run it again but stop it.
     */
    int id = g_pythonParser.getScriptId(strPath);
    if (id != -1)
    {
      /* if we are here we already know that this script is running.
       * But we will check it again to be sure :)
       */
      if (g_pythonParser.isRunning(id))
      {
        g_pythonParser.stopScript(id);

        // update items
        int selectedItem = m_viewControl.GetSelectedItem();
        Update(m_Directory.m_strPath);
        m_viewControl.SetSelectedItem(selectedItem);
        return ;
      }
    }
    g_pythonParser.evalFile(strPath);
  }
}

bool CGUIWindowScripts::HaveDiscOrConnection( CStdString& strPath, int iDriveType )
{
  if ( iDriveType == SHARE_TYPE_DVD )
  {
    MEDIA_DETECT::CDetectDVDMedia::WaitMediaReady();
    if ( !MEDIA_DETECT::CDetectDVDMedia::IsDiscInDrive() )
    {
      CGUIDialogOK::ShowAndGetInput(218, 219, 0, 0);
      return false;
    }
  }
  else if ( iDriveType == SHARE_TYPE_REMOTE )
  {
    // TODO: Handle not connected to a remote share
    if ( !CUtil::IsEthernetConnected() )
    {
      CGUIDialogOK::ShowAndGetInput(220, 221, 0, 0);
      return false;
    }
  }
  else
    return true;
  return true;
}

void CGUIWindowScripts::OnInfo()
{
  CGUIWindowScriptsInfo* pDlgInfo = (CGUIWindowScriptsInfo*)m_gWindowManager.GetWindow(WINDOW_SCRIPTS_INFO);
  if (pDlgInfo) pDlgInfo->DoModal(GetID());
}

void CGUIWindowScripts::Render()
{
  // update control_list / control_thumbs if one or more scripts have stopped / started
  if (g_pythonParser.ScriptsSize() != scriptSize)
  {
    int selectedItem = m_viewControl.GetSelectedItem();
    Update(m_Directory.m_strPath);
    m_viewControl.SetSelectedItem(selectedItem);
    scriptSize = g_pythonParser.ScriptsSize();
  }
  CGUIWindow::Render();
}

void CGUIWindowScripts::GoParentFolder()
{
  CStdString strPath(m_strParentPath), strOldPath(m_Directory.m_strPath);
  Update(strPath);
  UpdateButtons();

  if (!g_guiSettings.GetBool("LookAndFeel.FullDirectoryHistory"))
    m_history.Remove(strOldPath); //Delete current path

}

void CGUIWindowScripts::OnWindowLoaded()
{
  CGUIWindow::OnWindowLoaded();
#ifdef PRE_SKIN_VERSION_2_0_COMPATIBILITY
  if (g_SkinInfo.GetVersion() < 1.8)
  {
    ChangeControlID(10, CONTROL_LIST, CGUIControl::GUICONTROL_LIST);
    ChangeControlID(11, CONTROL_THUMBS, CGUIControl::GUICONTROL_THUMBNAIL);
  }
#endif
  m_viewControl.Reset();
  m_viewControl.SetParentWindow(GetID());
  m_viewControl.AddView(VIEW_AS_LIST, GetControl(CONTROL_LIST));
  m_viewControl.AddView(VIEW_AS_ICONS, GetControl(CONTROL_THUMBS));
  m_viewControl.AddView(VIEW_AS_LARGE_ICONS, GetControl(CONTROL_THUMBS));
  m_viewControl.SetViewControlID(CONTROL_BTNVIEWASICONS);
}

void CGUIWindowScripts::OnWindowUnload()
{
  CGUIWindow::OnWindowUnload();
  m_viewControl.Reset();
}