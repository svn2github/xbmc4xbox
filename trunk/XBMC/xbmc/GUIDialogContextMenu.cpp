#include "stdafx.h"
#include "GUIDialogContextMenu.h"
#include "GUIButtonControl.h"
#include "GUIDialogNumeric.h"
#include "GUIDialogGamepad.h"
#include "application.h"
#include "GUIPassword.h"
#include "util.h"
#include "GUIDialogMediaSource.h"

#define BACKGROUND_IMAGE 999
#define BACKGROUND_BOTTOM 998
#define BUTTON_TEMPLATE 1000
#define SPACE_BETWEEN_BUTTONS 2

CGUIDialogContextMenu::CGUIDialogContextMenu(void):CGUIDialog(WINDOW_DIALOG_CONTEXT_MENU, "DialogContextMenu.xml")
{
  m_iClickedButton = -1;
  m_iNumButtons = 0;
}
CGUIDialogContextMenu::~CGUIDialogContextMenu(void)
{}
bool CGUIDialogContextMenu::OnMessage(CGUIMessage &message)
{
  if (message.GetMessage() == GUI_MSG_CLICKED)
  { // someone has been clicked - deinit...
    m_iClickedButton = message.GetSenderId() - BUTTON_TEMPLATE;
    Close();
    return true;
  }
  return CGUIDialog::OnMessage(message);
}

void CGUIDialogContextMenu::OnInitWindow()
{ // disable the template button control
  CGUIControl *pControl = (CGUIControl *)GetControl(BUTTON_TEMPLATE);
  if (pControl)
  {
    pControl->SetVisible(false);
  }
  m_iClickedButton = -1;
  // set initial control focus
  m_lastControlID = BUTTON_TEMPLATE + 1;
  CGUIDialog::OnInitWindow();
}

void CGUIDialogContextMenu::ClearButtons()
{ // destroy our buttons (if we have them from a previous viewing)
  for (int i = 1; i <= m_iNumButtons; i++)
  {
    // get the button to remove...
    CGUIControl *pControl = (CGUIControl *)GetControl(BUTTON_TEMPLATE + i);
    if (pControl)
    {
      // remove the control from our list
      Remove(BUTTON_TEMPLATE + i);
      // kill the button
      pControl->FreeResources();
      delete pControl;
    }
  }
  m_iNumButtons = 0;
}

int CGUIDialogContextMenu::AddButton(int iLabel)
{
  return AddButton(g_localizeStrings.Get(iLabel));
}

int CGUIDialogContextMenu::AddButton(const CStdString &strLabel)
{ // add a button to our control
  CGUIButtonControl *pButtonTemplate = (CGUIButtonControl *)GetControl(BUTTON_TEMPLATE);
  if (!pButtonTemplate) return 0;
  CGUIButtonControl *pButton = new CGUIButtonControl(*pButtonTemplate);
  if (!pButton) return 0;
  // set the button's ID and position
  m_iNumButtons++;
  DWORD dwID = BUTTON_TEMPLATE + m_iNumButtons;
  pButton->SetID(dwID);
  pButton->SetPosition(pButtonTemplate->GetXPosition(), (m_iNumButtons - 1)*(pButtonTemplate->GetHeight() + SPACE_BETWEEN_BUTTONS));
  pButton->SetVisible(true);
  pButton->SetNavigation(dwID - 1, dwID + 1, dwID, dwID);
  pButton->SetLabel(strLabel);
  Add(pButton);
  // and update the size of our menu
  CGUIControl *pControl = (CGUIControl *)GetControl(BACKGROUND_IMAGE);
  if (pControl)
  {
    pControl->SetHeight(m_iNumButtons*(pButtonTemplate->GetHeight() + SPACE_BETWEEN_BUTTONS));
    CGUIControl *pControl2 = (CGUIControl *)GetControl(BACKGROUND_BOTTOM);
    if (pControl2)
      pControl2->SetPosition(pControl2->GetXPosition(), pControl->GetYPosition() + pControl->GetHeight());
  }
  return m_iNumButtons;
}
void CGUIDialogContextMenu::DoModal(DWORD dwParentId, int iWindowID /*= WINDOW_INVALID */)
{
  // update the navigation of the first and last buttons
  CGUIControl *pControl = (CGUIControl *)GetControl(BUTTON_TEMPLATE + 1);
  if (pControl)
    pControl->SetNavigation(BUTTON_TEMPLATE + m_iNumButtons, pControl->GetControlIdDown(), pControl->GetControlIdLeft(), pControl->GetControlIdRight());
  pControl = (CGUIControl *)GetControl(BUTTON_TEMPLATE + m_iNumButtons);
  if (pControl)
    pControl->SetNavigation(pControl->GetControlIdUp(), BUTTON_TEMPLATE + 1, pControl->GetControlIdLeft(), pControl->GetControlIdRight());
  // update our default control
  if (m_dwDefaultFocusControlID <= BUTTON_TEMPLATE || m_dwDefaultFocusControlID > (DWORD)(BUTTON_TEMPLATE + m_iNumButtons))
    m_dwDefaultFocusControlID = BUTTON_TEMPLATE + 1;
  // check the default control has focus...
  while (m_dwDefaultFocusControlID <= (DWORD)(BUTTON_TEMPLATE + m_iNumButtons) && !(GetControl(m_dwDefaultFocusControlID)->CanFocus()))
    m_dwDefaultFocusControlID++;
  CGUIDialog::DoModal(dwParentId);
}
int CGUIDialogContextMenu::GetButton()
{
  return m_iClickedButton;
}

DWORD CGUIDialogContextMenu::GetHeight()
{
  CGUIControl *pControl = (CGUIControl *)GetControl(BACKGROUND_IMAGE);
  if (pControl)
    return pControl->GetHeight();
  else
    return CGUIDialog::GetHeight();
}
DWORD CGUIDialogContextMenu::GetWidth()
{
  CGUIControl *pControl = (CGUIControl *)GetControl(BACKGROUND_IMAGE);
  if (pControl)
    return pControl->GetWidth();
  else
    return CGUIDialog::GetWidth();
}
int CGUIDialogContextMenu::GetNumButtons()
{
  return m_iNumButtons;
}
void CGUIDialogContextMenu::EnableButton(int iButton, bool bEnable)
{
  CGUIControl *pControl = (CGUIControl *)GetControl(BUTTON_TEMPLATE + iButton);
  if (pControl) pControl->SetEnabled(bEnable);
}
bool CGUIDialogContextMenu::BookmarksMenu(const CStdString &strType, const CStdString &strLabel, const CStdString &strPath, int iLockMode, bool bMaxRetryExceeded, int iPosX, int iPosY)
{
  // popup the context menu
  CGUIDialogContextMenu *pMenu = (CGUIDialogContextMenu *)m_gWindowManager.GetWindow(WINDOW_DIALOG_CONTEXT_MENU);
  if (pMenu)
  {
    // check where we are
    bool bMyProgramsMenu = ("myprograms" == strType);
    // For DVD Drive Context menu stuff, we can also check where we are, so playin dvd can be dependet to the section!
    bool bIsDVDContextMenu  = false;
    bool bIsDVDMediaPresent = false;
    
    // load our menu
    pMenu->Initialize();

    CStdString strDefault = GetDefaultShareNameByType(strType);
    
    // add the needed buttons
    int btn_Rename = 0;
#ifdef SKIN_VERSION_PRE_2_0_COMPATIBILITY
    if (!m_gWindowManager.GetWindow(WINDOW_DIALOG_MEDIA_SOURCE))
      btn_Rename = pMenu->AddButton(118); // Rename
#endif
    int btn_EditPath = 0;
#ifdef SKIN_VERSION_PRE_2_0_COMPATIBILITY
    if (!CUtil::IsVirtualPath(strPath))
      btn_EditPath = pMenu->AddButton(748); // Edit Path
    int btn_AddShare = pMenu->AddButton(bMyProgramsMenu ? 754 : 749); // 4: Add Program Link / Add Share
#else
    if (!CUtil::IsVirtualPath(strPath))
      btn_EditPath = pMenu->AddButton(1027); // Edit Source
    int btn_AddShare = pMenu->AddButton(1026); // Add Source
#endif
    int btn_Delete = pMenu->AddButton(522); // Remove Source

    int btn_Default = pMenu->AddButton(13335); // Set as Default
    int btn_ClearDefault = 0;
    if (!strDefault.IsEmpty())
      btn_ClearDefault = pMenu->AddButton(13403); // Clear Default

    // GeminiServer: DVD Drive Context menu stuff
    int btn_PlayDisc = 0;
    int btn_Eject = 0;
    CIoSupport TrayIO;
    if (strPath == "D:\\" || strPath == "iso9660://" || strPath == "UDF://" || strPath == "cdda://" || strPath == "cdda://local/" )
    { 
      // We need to check if there is a detected is inserted! 
      int iTrayState = TrayIO.GetTrayState();
      if ( iTrayState == DRIVE_CLOSED_MEDIA_PRESENT || iTrayState == TRAY_CLOSED_MEDIA_PRESENT )
      {
        btn_PlayDisc = pMenu->AddButton(341); // Play CD/DVD!
        bIsDVDMediaPresent = true;
      }
      btn_Eject = pMenu->AddButton(13391);  // Eject/Load CD/DVD!
      bIsDVDContextMenu = true;
    }

    // This if statement should always be the *last* one to add buttons
    // Only show share lock stuff if masterlock isn't disabled
    int btn_LockShare = 0; // Lock Share
    int btn_ResetLock = 0; // Reset Share Lock
    int btn_RemoveLock = 0; // Remove Share Lock
    int btn_ReactivateLock = 0; // Reactivate Share Lock
    int btn_ChangeLock = 0; // Change Share Lock;
    if (LOCK_MODE_EVERYONE != g_stSettings.m_iMasterLockMode)
    {
      if (LOCK_MODE_EVERYONE == iLockMode) 
        btn_LockShare = pMenu->AddButton(12332);
      else if (LOCK_MODE_EVERYONE < iLockMode && bMaxRetryExceeded) 
        btn_ResetLock = pMenu->AddButton(12334);
      else if (LOCK_MODE_EVERYONE > iLockMode && !bMaxRetryExceeded)
      {
        btn_RemoveLock = pMenu->AddButton(12335);
        // don't show next button if folder locks are being overridden
        if (!g_application.m_bMasterLockOverridesLocalPasswords) 
        {
          btn_ReactivateLock = pMenu->AddButton(12353);
		      btn_ChangeLock = pMenu->AddButton(12356);
        }
      }
    }
    // set the correct position
    pMenu->SetPosition(iPosX - pMenu->GetWidth() / 2, iPosY - pMenu->GetHeight() / 2);
    pMenu->DoModal(m_gWindowManager.GetActiveWindow());
    
    int btn = pMenu->GetButton();
    if (btn > 0)
    {
      if (btn == btn_Rename)
      {
        if (!CheckMasterCode(iLockMode)) return false;
        // rename share
        CStdString strNewLabel = strLabel;
        CStdString strHeading = g_localizeStrings.Get(bMyProgramsMenu ? 756 : 753);
        if (CGUIDialogKeyboard::ShowAndGetInput(strNewLabel, strHeading, false))
        {
          g_settings.UpdateBookmark(strType, strLabel, "name", strNewLabel);
          return true;
        }
      }
      else if (btn == btn_EditPath)
      {
        if (!CheckMasterCode(iLockMode)) return false;
#ifdef PRE_SKIN_VERSION_2_0_COMPATIBILITY
        if (!m_gWindowManager.GetWindow(WINDOW_DIALOG_MEDIA_SOURCE))
        { // edit path
          CStdString strNewPath = strPath;
          CStdString strHeading = g_localizeStrings.Get(bMyProgramsMenu ? 755 : 752);
          if (CGUIDialogKeyboard::ShowAndGetInput(strNewPath, strHeading, false))
          {
            if (bMyProgramsMenu)
            { // get a path depth
              CStdString strNewDepth = "";
              strHeading = g_localizeStrings.Get(757);
              if (!(CGUIDialogKeyboard::ShowAndGetInput(strNewDepth, strHeading, false)))
                return false;
              if (!(CUtil::IsNaturalNumber(strNewDepth) && 0 < atoi(strNewDepth.c_str()) && 10 > atoi(strNewDepth.c_str())))
              {
                CGUIDialogOK::ShowAndGetInput(257, 759, 760, 0);
                return false;
              }
              g_settings.UpdateBookmark(strType, strLabel, "depth", strNewDepth);
            }
            g_settings.UpdateBookmark(strType, strLabel, "path", strNewPath);
            return true;
          }
        }
        else
#endif
        // TODO: No support here for <depth> parameter in My Programs.
        // Ideally, we should simply search for xbe's more intelligently
        // and display the results.  They can then be characterised by the user.
        return CGUIDialogMediaSource::ShowAndEditMediaSource(strType, strLabel, strPath);
      }
      else if (btn == btn_Delete)
      {
        if (!CheckMasterCode(iLockMode)) return false;
        // prompt user if they want to really delete the bookmark
        if (CGUIDialogYesNo::ShowAndGetInput(bMyProgramsMenu ? 758 : 751, 0, 750, 0))
        {
          // delete this share
          g_settings.DeleteBookmark(strType, strLabel, strPath);

          // check default
          if (!strDefault.IsEmpty())
          {
            // need compare left anchored due to DVD's adding status and name
            if (strLabel.Left(strDefault.size()).Equals(strDefault))
              ClearDefault(strType);
          }

          return true;
        }
      }
      else if (btn == btn_AddShare)
      {
        if (!CheckMasterCode(iLockMode)) return false;

#ifdef SKIN_VERSION_PRE_2_0_COMPATIBILITY
        if (!m_gWindowManager.GetWindow(WINDOW_DIALOG_MEDIA_SOURCE))
        {
          // Add new share
          CStdString strNewPath;
          CStdString strHeading = g_localizeStrings.Get(bMyProgramsMenu ? 755 : 752);
          if (CGUIDialogKeyboard::ShowAndGetInput(strNewPath, strHeading, false))
          { // got a valid path
            CStdString strNewDepth = "";
            if (bMyProgramsMenu)
            { // get a path depth
              CStdString strHeading = g_localizeStrings.Get(757);
              if (!(CGUIDialogKeyboard::ShowAndGetInput(strNewDepth, strHeading, false)))
                return false;
            }
            CStdString strNewName;
            CStdString strHeading = g_localizeStrings.Get(bMyProgramsMenu ? 756 : 753);
            if (CGUIDialogKeyboard::ShowAndGetInput(strNewName, strHeading, false))
            {
              // got a valid name, save the bookmark
              g_settings.AddBookmark(strType, strNewName, strNewPath, atoi(strNewDepth.c_str()));
              return true;
            }
          }
          return false;
        }
        else
#endif
        return CGUIDialogMediaSource::ShowAndAddMediaSource(strType);
      }
      else if (btn == btn_Default)
      {
        if (!CheckMasterCode(iLockMode)) return false;
        // make share default
        SetDefault(strType, strLabel);
        return true;
      }
      else if (btn == btn_ClearDefault)
      {
        if (!CheckMasterCode(iLockMode)) return false;
        // remove share default
        ClearDefault(strType);
        return true;
      }
      else if (btn == btn_PlayDisc)
      {
        // Ok Play now the Media CD/DVD!
        return CAutorun::PlayDisc();
      }
      else if (btn == btn_Eject)
      {
        if (TrayIO.GetTrayState() == TRAY_OPEN)
        {
          TrayIO.CloseTray();
          return true;
        }
        else
        {
          TrayIO.EjectTray();
          return true;
        }
      }
      else if (btn == btn_LockShare)
      {
        // prompt user for mastercode when changing lock settings
        if (!g_passwordManager.IsMasterLockUnlocked(true))
          return false;

        // popup the context menu
        CGUIDialogContextMenu *pMenu = (CGUIDialogContextMenu *)m_gWindowManager.GetWindow(WINDOW_DIALOG_CONTEXT_MENU);
        if (pMenu)
        {
          // load our menu
          pMenu->Initialize();
          // add the needed buttons
          pMenu->AddButton(12337); // 1: Numeric Password
          pMenu->AddButton(12338); // 2: XBOX gamepad button combo
          pMenu->AddButton(12339); // 3: Full-text Password

          // set the correct position
          pMenu->SetPosition(iPosX - pMenu->GetWidth() / 2, iPosY - pMenu->GetHeight() / 2);
          pMenu->DoModal(m_gWindowManager.GetActiveWindow());

          char strLockMode[33];  // holds 32 places plus sign character
          itoa(pMenu->GetButton(), strLockMode, 10);
          CStdString strNewPassword = "";
          switch (pMenu->GetButton())
          {
          case 1:  // 1: Numeric Password
            if (!CGUIDialogNumeric::ShowAndVerifyNewPassword(strNewPassword))
              return false;
            break;
          case 2:  // 2: Gamepad Password
            if (!CGUIDialogGamepad::ShowAndVerifyNewPassword(strNewPassword))
              return false;
            break;
          case 3:  // 3: Fulltext Password
            if (!CGUIDialogKeyboard::ShowAndVerifyNewPassword(strNewPassword))
              return false;
            break;
          default:  // Not supported, abort
            return false;
            break;
          }
          // password entry and re-entry succeeded, write out the lock data
          g_settings.UpdateBookmark(strType, strLabel, "lockmode", strLockMode);
          g_settings.UpdateBookmark(strType, strLabel, "lockcode", strNewPassword);
          g_settings.UpdateBookmark(strType, strLabel, "badpwdcount", "0");
          return true;
        }
      }
      else if (btn == btn_ResetLock)
      {
        // prompt user for mastercode when changing lock settings
        if (!g_passwordManager.IsMasterLockUnlocked(true))
          return false;

        g_settings.UpdateBookmark(strType, strLabel, "badpwdcount", "0");
        return true;
      }
      else if (btn == btn_RemoveLock)
      {
        // prompt user for mastercode when changing lock settings
        if (!g_passwordManager.IsMasterLockUnlocked(true))
          return false;

        if (!CGUIDialogYesNo::ShowAndGetInput(0, 12335, 750, 0))
          return false;

        g_settings.UpdateBookmark(strType, strLabel, "lockmode", "0");
        g_settings.UpdateBookmark(strType, strLabel, "lockcode", "-");
        g_settings.UpdateBookmark(strType, strLabel, "badpwdcount", "0");
        return true;
      }
      else if (btn == btn_ReactivateLock)
      {
        if (LOCK_MODE_EVERYONE > iLockMode && !bMaxRetryExceeded && !g_application.m_bMasterLockOverridesLocalPasswords)
        {
          // don't prompt user for mastercode when reactivating a lock
          CStdString strInvertedLockmode = "";
          strInvertedLockmode.Format("%d", iLockMode * -1);
          g_settings.UpdateBookmark(strType, strLabel, "lockmode", strInvertedLockmode);
          return true;
        }
        else  // this should never happen, but if it does, don't perform any action
          return false;
      }
      else if (btn == btn_ChangeLock)
      {
	      // prompt user for mastercode when changing lock settings
	      //if (!g_passwordManager.IsMasterLockUnlocked(true))
	      //   return false;
	      //else
	      //{
	      CStdString strNewPW;
	      CStdString strNewLockMode;
		      switch (iLockMode)
		      {
		      case -1:  // 1: Numeric Password
			      if (!CGUIDialogNumeric::ShowAndVerifyNewPassword(strNewPW))
			      return false;
			      else strNewLockMode = "1";
			      break;
		      case -2:  // 2: Gamepad Password
			      if (!CGUIDialogGamepad::ShowAndVerifyNewPassword(strNewPW))
			      return false;
			      else strNewLockMode = "2";
			      break;
		      case -3:  // 3: Fulltext Password
			      if (!CGUIDialogKeyboard::ShowAndVerifyNewPassword(strNewPW))
			      return false;
			      else strNewLockMode = "3";
			      break;
		      default:  // Not supported, abort
			      return false;
			      break;
		      }
		      // password ReSet and re-entry succeeded, write out the lock data
		      g_settings.UpdateBookmark(strType, strLabel, "lockcode", strNewPW);
		      g_settings.UpdateBookmark(strType, strLabel, "lockmode", strNewLockMode);
		      g_settings.UpdateBookmark(strType, strLabel, "badpwdcount", "0");
		      return true;
      //}
      }
    }
  }
  return false;
}

bool CGUIDialogContextMenu::CheckMasterCode(int iLockMode)
{
  // prompt user for mastercode if the bookmark is locked
  // or if m_iMasterLockProtectShares is enabled
  if (LOCK_MODE_EVERYONE < iLockMode || 0 != g_stSettings.m_iMasterLockProtectShares)
  {
    // Prompt user for mastercode
    return g_passwordManager.IsMasterLockUnlocked(true);
  }
  else
  {
    // we don't need to prompt for mastercode
    return true;
  }
}

void CGUIDialogContextMenu::OnWindowUnload()
{
  ClearButtons();
}

CStdString CGUIDialogContextMenu::GetDefaultShareNameByType(const CStdString &strType)
{
  VECSHARES *pShares = NULL;
  CStdString strDefault;

  if (strType == "myprograms")
  {
    pShares = &g_settings.m_vecMyProgramsBookmarks;
    strDefault = g_stSettings.m_szDefaultPrograms;
  }
  else if (strType == "files")
  {
    pShares = &g_settings.m_vecMyFilesShares;
    strDefault = g_stSettings.m_szDefaultFiles;
  }
  else if (strType == "music")
  {
    pShares = &g_settings.m_vecMyMusicShares;
    strDefault = g_stSettings.m_szDefaultMusic;
  }
  else if (strType == "video")
  {
    pShares = &g_settings.m_vecMyVideoShares;
    strDefault = g_stSettings.m_szDefaultVideos;
  }
  else if (strType == "pictures")
  {
    pShares = &g_settings.m_vecMyPictureShares;
    strDefault = g_stSettings.m_szDefaultPictures;
  }

  if (!pShares) return "";

  VECSHARES vecShares = *pShares;
  bool bIsBookmarkName = false;
  int iIndex = CUtil::GetMatchingShare(strDefault, vecShares, bIsBookmarkName);
  if (iIndex < 0)
    return "";

  return vecShares.at(iIndex).strName;
}

void CGUIDialogContextMenu::SetDefault(const CStdString &strType, const CStdString &strDefault)
{
  if (g_settings.UpDateXbmcXML(strType, "default", strDefault))
  {
    if (strType == "myprograms")
      strcpy(g_stSettings.m_szDefaultPrograms, strDefault.c_str());
    else if (strType == "files")
      strcpy(g_stSettings.m_szDefaultFiles, strDefault.c_str());
    else if (strType == "music")
      strcpy(g_stSettings.m_szDefaultMusic, strDefault.c_str());
    else if (strType == "video")
      strcpy(g_stSettings.m_szDefaultVideos, strDefault.c_str());
    else if (strType == "pictures")
      strcpy(g_stSettings.m_szDefaultPictures, strDefault.c_str());
  }
  else
    CLog::Log(LOGERROR, "Could not change default bookmark for %s", strType.c_str());
}

void CGUIDialogContextMenu::ClearDefault(const CStdString &strType)
{
  SetDefault(strType, "");
}