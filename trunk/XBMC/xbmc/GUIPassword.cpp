#include "stdafx.h"
#include "GUIPassword.h"
#include "Application.h"
#include "GUIDialogNumeric.h"
#include "GUIDialogGamepad.h"
#include "GUIDialogContextMenu.h"
#include "GUIDialogKeyboard.h"
#include "GUIDialogLockSettings.h"
#include "util.h"
#include "settings.h"
#include "xbox/xkutils.h"

CGUIPassword g_passwordManager;

CGUIPassword::CGUIPassword(void)
{
  iMasterLockRetriesLeft = -1;
  bMasterUser = false;
  m_SMBShare = "";
}
CGUIPassword::~CGUIPassword(void)
{}

bool CGUIPassword::IsItemUnlocked(CFileItem* pItem, const CStdString &strType)
{
  // \brief Tests if the user is allowed to access the share folder
  // \param pItem The share folder item to access
  // \param strType The type of share being accessed, e.g. "music", "video", etc. See CSettings::UpdateBookmark
  // \return If access is granted, returns \e true
  if (g_settings.m_vecProfiles[0].getLockMode() == LOCK_MODE_EVERYONE)
    return true;

  while (pItem->m_iHasLock > 1)
  {
    int iMode = pItem->m_iLockMode;
    CStdString strLockCode = pItem->m_strLockCode;
    CStdString strLabel = pItem->GetLabel();
    bool bConfirmed = false;
    bool bCanceled = false;
    int iResult = 0;  // init to user succeeded state, doing this to optimize switch statement below
    char buffer[33]; // holds 32 places plus sign character
    int iRetries = 0;
    if(g_passwordManager.bMasterUser)// Check if we are the MasterUser!
    {
      iResult = 0; 
    }
    else
    {
      if (!(1 > pItem->m_iBadPwdCount))
        iRetries = g_guiSettings.GetInt("masterlock.maxretries") - pItem->m_iBadPwdCount;
      if (0 != g_guiSettings.GetInt("masterlock.maxretries") && pItem->m_iBadPwdCount >= g_guiSettings.GetInt("masterlock.maxretries"))
      { // user previously exhausted all retries, show access denied error
        CGUIDialogOK::ShowAndGetInput(12345, 12346, 0, 0);
        return false;
      }
      // show the appropriate lock dialog
      CStdString strHeading = "";
      if (pItem->m_bIsFolder)
        strHeading = g_localizeStrings.Get(12325);
      else
        strHeading = g_localizeStrings.Get(12348);
      
      switch (iMode)
      {
      case LOCK_MODE_NUMERIC:
        iResult = CGUIDialogNumeric::ShowAndVerifyPassword(strLockCode, strHeading, iRetries);
        break;
      case LOCK_MODE_GAMEPAD:
        iResult = CGUIDialogGamepad::ShowAndVerifyPassword(strLockCode, strHeading, iRetries);
        break;
      case LOCK_MODE_QWERTY:
        iResult = CGUIDialogKeyboard::ShowAndVerifyPassword(strLockCode, strHeading, iRetries);
        break;
      default:
        // pItem->m_iLockMode isn't set to an implemented lock mode, so treat as unlocked
        return true;
        break;
      }
    }
    switch (iResult)
    {
    case -1:
      { // user canceled out
        return false; 
        break;
      }
    case 0:
      {
        // password entry succeeded
        pItem->m_iBadPwdCount = 0;
        pItem->m_iHasLock = 1;
        g_passwordManager.LockBookmark(strType,strLabel,false);
        g_settings.UpdateBookmark(strType, strLabel, "badpwdcount", itoa(pItem->m_iBadPwdCount, buffer, 10));
        break;
      }
    case 1:
      {
        // password entry failed
        if (0 != g_guiSettings.GetInt("masterlock.maxretries"))
          pItem->m_iBadPwdCount++;
        g_settings.UpdateBookmark(strType, strLabel, "badpwdcount", itoa(pItem->m_iBadPwdCount, buffer, 10));
        break;
      }
    default:
      {
        // this should never happen, but if it does, do nothing
        return false; break;
      }
    }
  }
  return true;
}

void CGUIPassword::SetSMBShare(const CStdString &strPath)
{
  m_SMBShare = strPath;
}

CStdString CGUIPassword::GetSMBShare()
{ 
  return m_SMBShare;  
}

bool CGUIPassword::GetSMBShareUserPassword()
{
  CURL url(m_SMBShare);
  CStdString passcode;
  CStdString username = url.GetUserName();
  CStdString outusername = username;
  CStdString share;
  url.GetURLWithoutUserDetails(share);

  CStdString header;
  header.Format("%s %s", g_localizeStrings.Get(14062).c_str(), share.c_str());

  if (!CGUIDialogKeyboard::ShowAndGetInput(outusername, header, false))
  {
    if (outusername.IsEmpty() || outusername != username) // just because the routine returns false when enter is hit and nothing was changed
      return false;
  }
  if (!CGUIDialogKeyboard::ShowAndGetInput(passcode, g_localizeStrings.Get(12326), true))
    return false;
  
  url.SetPassword(passcode);
  url.SetUserName(outusername);
  url.GetURL(m_SMBShare);

  return true;
}

bool CGUIPassword::CheckStartUpLock()   // GeminiServer
{
  // prompt user for mastercode if the mastercode was set b4 or by xml
  int iVerifyPasswordResult = -1;
  CStdString strHeader = g_localizeStrings.Get(20075);
  if (iMasterLockRetriesLeft == -1)
    iMasterLockRetriesLeft = g_guiSettings.GetInt("masterlock.maxretries");
  if (g_passwordManager.iMasterLockRetriesLeft == 0) g_passwordManager.iMasterLockRetriesLeft = 1;
  CStdString strPassword = g_settings.m_vecProfiles[0].getLockCode();
  for (int i=1; i <= g_passwordManager.iMasterLockRetriesLeft; i++)
  {
    switch (g_settings.m_vecProfiles[0].getLockMode())
    { // Prompt user for mastercode
      case LOCK_MODE_NUMERIC:
        iVerifyPasswordResult = CGUIDialogNumeric::ShowAndVerifyPassword(strPassword, strHeader, 0);
        break;
      case LOCK_MODE_GAMEPAD:
        iVerifyPasswordResult = CGUIDialogGamepad::ShowAndVerifyPassword(strPassword, strHeader, 0);
        break;
      case LOCK_MODE_QWERTY:
        iVerifyPasswordResult = CGUIDialogKeyboard::ShowAndVerifyPassword(strPassword, strHeader, 0);
        break;
    }
    if (iVerifyPasswordResult != 0 )
    {
      CStdString strLabel,strLabel1;
      strLabel1 = g_localizeStrings.Get(12343);
      int iLeft = g_passwordManager.iMasterLockRetriesLeft-i;
      strLabel.Format("%i %s",iLeft,strLabel1.c_str());
      
      // PopUp OK and Display: MasterLock mode has changed but no no Mastercode has been set!
      CGUIDialogOK *dlg = (CGUIDialogOK *)m_gWindowManager.GetWindow(WINDOW_DIALOG_OK);
      if (!dlg) return false;
      dlg->SetHeading( g_localizeStrings.Get(20076) );
      dlg->SetLine( 0, g_localizeStrings.Get(12367) );
      dlg->SetLine( 1, g_localizeStrings.Get(12368) );
      dlg->SetLine( 2, strLabel);
      dlg->DoModal();
    }
    else 
      i=g_passwordManager.iMasterLockRetriesLeft;
  }

  if (iVerifyPasswordResult == 0)
  {
    g_passwordManager.iMasterLockRetriesLeft = g_guiSettings.GetInt("masterlock.maxretries");
    return true;  // OK The MasterCode Accepted! XBMC Can Run!
  }
  else 
  {
    g_applicationMessenger.Shutdown(); // Turn off the box
    return false;
  }
}

bool CGUIPassword::SetMasterLockMode()
{
  CGUIDialogLockSettings* pDialog = (CGUIDialogLockSettings*)m_gWindowManager.GetWindow(WINDOW_DIALOG_LOCK_SETTINGS);
  if (pDialog && g_advancedSettings.m_profilesupport)
  {
    CProfile& profile=g_settings.m_vecProfiles.at(0);
    if (pDialog->ShowAndGetLock(profile._iLockMode,profile._strLockCode,profile._bLockMusic,profile._bLockVideo,profile._bLockPictures,profile._bLockPrograms,profile._bLockFiles,profile._bLockSettings,12360))
      return true;

    return false;
  }
  else // DEPRECATED BUT LEFT FOR COMPATIBILITY
  {
    // load a context menu with the options for the mastercode...
    CGUIDialogContextMenu *menu = (CGUIDialogContextMenu *)m_gWindowManager.GetWindow(WINDOW_DIALOG_CONTEXT_MENU);
    if (menu)
    {
      menu->Initialize();
      menu->AddButton(1223);
      menu->AddButton(12337);
      menu->AddButton(12338);
      menu->AddButton(12339);
      menu->SetPosition((g_graphicsContext.GetWidth() - menu->GetWidth()) / 2, (g_graphicsContext.GetHeight() - menu->GetHeight()) / 2);
      menu->DoModal();

      CStdString newPassword;
      int iLockMode = -1;
      switch(menu->GetButton())
      {
      case 1:
        iLockMode = LOCK_MODE_EVERYONE; //Disabled! Need check routine!!!
        break;
      case 2:
        iLockMode = LOCK_MODE_NUMERIC;
        CGUIDialogNumeric::ShowAndVerifyNewPassword(newPassword);
        break;
      case 3:
        iLockMode = LOCK_MODE_GAMEPAD;
        CGUIDialogGamepad::ShowAndVerifyNewPassword(newPassword);
        break;
      case 4:
        iLockMode = LOCK_MODE_QWERTY;
        CGUIDialogKeyboard::ShowAndVerifyNewPassword(newPassword);
        break;
      }

      if (iLockMode == LOCK_MODE_EVERYONE) // disable
      {
        g_settings.m_vecProfiles[0].setLockMode(LOCK_MODE_EVERYONE);
        return true;
      }

      if (!newPassword.IsEmpty() && newPassword.c_str() != "-" )
      {
        g_settings.m_vecProfiles[0].setLockCode(newPassword);
      }
      else         
        return false;

      g_settings.m_vecProfiles[0].setLockMode(iLockMode);
    }
    else 
      return false;
  }
  return true;
}

bool CGUIPassword::IsProfileLockUnlocked()
{
  if (g_passwordManager.bMasterUser || g_settings.m_vecProfiles[0].getLockMode() == LOCK_MODE_EVERYONE)
    return true;

  if (g_settings.m_iLastLoadedProfileIndex == 0)
    return CheckLock(g_settings.m_vecProfiles[g_settings.m_iLastLoadedProfileIndex].getLockMode(),g_settings.m_vecProfiles[g_settings.m_iLastLoadedProfileIndex].getLockCode(),20075);
  else
    return CheckLock(g_settings.m_vecProfiles[g_settings.m_iLastLoadedProfileIndex].getLockMode(),g_settings.m_vecProfiles[g_settings.m_iLastLoadedProfileIndex].getLockCode(),20095);
}

bool CGUIPassword::IsMasterLockUnlocked(bool bPromptUser)
{
  if (iMasterLockRetriesLeft == -1)
    iMasterLockRetriesLeft = g_guiSettings.GetInt("masterlock.maxretries");
  if ((LOCK_MODE_EVERYONE < g_settings.m_vecProfiles[g_settings.m_iLastLoadedProfileIndex].getLockMode()) && !bPromptUser)
    // not unlocked, but calling code doesn't want to prompt user
    return false;

  if (g_passwordManager.bMasterUser)
    return true;

  if (iMasterLockRetriesLeft == 0)
  {
    UpdateMasterLockRetryCount(false);
    return false;
  }

  // no, unlock since we are allowed to prompt
  int iVerifyPasswordResult = -1;
  CStdString strHeading = g_localizeStrings.Get(20075);
  CStdString strPassword = g_settings.m_vecProfiles[0].getLockCode();
  switch (g_settings.m_vecProfiles[0].getLockMode())
  {
  case LOCK_MODE_NUMERIC:
    iVerifyPasswordResult = CGUIDialogNumeric::ShowAndVerifyPassword(strPassword, strHeading, 0);
    break;
  case LOCK_MODE_GAMEPAD:
    iVerifyPasswordResult = CGUIDialogGamepad::ShowAndVerifyPassword(strPassword, strHeading, 0);
    break;
  case LOCK_MODE_QWERTY:
    iVerifyPasswordResult = CGUIDialogKeyboard::ShowAndVerifyPassword(strPassword, strHeading, 0);
    break;
  default:   // must not be supported, treat as unlocked
    iVerifyPasswordResult = 0;
    break;
  }
  if (1 == iVerifyPasswordResult) 
    UpdateMasterLockRetryCount(false);
  
  if (0 != iVerifyPasswordResult) 
    return false;

  // user successfully entered mastercode
  UpdateMasterLockRetryCount(true);
  if (g_guiSettings.GetBool("masterlock.automastermode"))
  {
      UnlockBookmarks();
      bMasterUser = true;
      g_application.m_guiDialogKaiToast.QueueNotification(g_localizeStrings.Get(20052),g_localizeStrings.Get(20054));
  }
  return true;
}

void CGUIPassword::UpdateMasterLockRetryCount(bool bResetCount)
{
  // \brief Updates Master Lock status.
  // \param bResetCount masterlock retry counter is zeroed if true, or incremented and displays an Access Denied dialog if false.
  if (!bResetCount)
  {
    // Bad mastercode entered
    if (0 < g_guiSettings.GetInt("masterlock.maxretries"))
    {
      // We're keeping track of how many bad passwords are entered
      if (1 < g_passwordManager.iMasterLockRetriesLeft)
      {
        // user still has at least one retry after decrementing
        g_passwordManager.iMasterLockRetriesLeft--;
      }
      else
      {
        // user has run out of retry attempts
        g_passwordManager.iMasterLockRetriesLeft = 0;
        if (g_guiSettings.GetBool("masterlock.enableshutdown"))
        {
          // Shutdown enabled, tell the user we're shutting off
          CGUIDialogOK::ShowAndGetInput(12345, 12346, 12347, 0);
          g_applicationMessenger.Shutdown();
          return ;
        }
        // Tell the user they ran out of retry attempts
        CGUIDialogOK::ShowAndGetInput(12345, 12346, 0, 0);
        return ;
      }
    }
    CStdString dlgLine1 = "";
    if (0 < g_passwordManager.iMasterLockRetriesLeft)
      dlgLine1.Format("%d %s", g_passwordManager.iMasterLockRetriesLeft, g_localizeStrings.Get(12343));
    CGUIDialogOK *dialog = (CGUIDialogOK *)m_gWindowManager.GetWindow(WINDOW_DIALOG_OK); // Tell user they entered a bad password
    if (dialog) 
    {
      dialog->SetHeading(20075);
      dialog->SetLine(0, 12345);
      dialog->SetLine(1, dlgLine1);
      dialog->SetLine(2, 0);
      dialog->DoModal();
    }
  }
  else 
    g_passwordManager.iMasterLockRetriesLeft = g_guiSettings.GetInt("masterlock.maxretries"); // user entered correct mastercode, reset retries to max allowed
}

bool CGUIPassword::CheckLock(int btnType, const CStdString& strPassword, int iHeading)
{
  if (btnType == 0 || strPassword.Equals("-") || g_settings.m_vecProfiles[0].getLockMode() == LOCK_MODE_EVERYONE || g_passwordManager.bMasterUser)
    return true;

  int iVerifyPasswordResult = -1;
  CStdString strHeading = g_localizeStrings.Get(iHeading);
  switch (g_settings.m_vecProfiles[0].getLockMode())
  {
  case LOCK_MODE_NUMERIC:
    iVerifyPasswordResult = CGUIDialogNumeric::ShowAndVerifyPassword(const_cast<CStdString&>(strPassword), strHeading, 0);
    break;
  case LOCK_MODE_GAMEPAD:
    iVerifyPasswordResult = CGUIDialogGamepad::ShowAndVerifyPassword(const_cast<CStdString&>(strPassword), strHeading, 0);
    break;
  case LOCK_MODE_QWERTY:
    iVerifyPasswordResult = CGUIDialogKeyboard::ShowAndVerifyPassword(const_cast<CStdString&>(strPassword), strHeading, 0);
    break;
  default:   // must not be supported, treat as unlocked
    iVerifyPasswordResult = 0;
    break;
  }
  
  return (iVerifyPasswordResult==0);
}

bool CGUIPassword::CheckMenuLock(int iWindowID)
{
  bool bCheckPW         = false;
  int iSwitch = iWindowID;
  
  // check if a settings subcategory was called from other than settings window
  if (iWindowID >= WINDOW_UI_CALIBRATION && iWindowID <= WINDOW_SETTINGS_APPEARANCE)
  {
    int iCWindowID = m_gWindowManager.GetActiveWindow();
    if (iCWindowID != WINDOW_SETTINGS_MENU && (iCWindowID < WINDOW_UI_CALIBRATION || iCWindowID > WINDOW_SETTINGS_APPEARANCE))
      iSwitch = WINDOW_SETTINGS_MENU;
  }
  CLog::Log(LOGDEBUG, "Checking if window ID %i is locked.", iSwitch);

  switch (iSwitch)
  {
    case WINDOW_SETTINGS_MENU:  // Settings 
      bCheckPW = g_settings.m_vecProfiles[g_settings.m_iLastLoadedProfileIndex].settingsLocked();
      break;
    case WINDOW_FILES:          // Files
      bCheckPW = g_settings.m_vecProfiles[g_settings.m_iLastLoadedProfileIndex].filesLocked();
      break;
    case WINDOW_PROGRAMS:       // Programs
    case WINDOW_SCRIPTS:
      bCheckPW = g_settings.m_vecProfiles[g_settings.m_iLastLoadedProfileIndex].programsLocked();
      break;
    case WINDOW_MUSIC_FILES:    // Music
      bCheckPW = g_settings.m_vecProfiles[g_settings.m_iLastLoadedProfileIndex].musicLocked();
      break;
    case WINDOW_VIDEO_FILES:    // Video
      bCheckPW = g_settings.m_vecProfiles[g_settings.m_iLastLoadedProfileIndex].videoLocked();
      break;
    case WINDOW_PICTURES:       // Pictures
      bCheckPW = g_settings.m_vecProfiles[g_settings.m_iLastLoadedProfileIndex].picturesLocked();
      break;
    case WINDOW_SETTINGS_PROFILES:
      bCheckPW = true;
      break;
    default:
      bCheckPW = false;
      break;
  }
  if (bCheckPW) 
    return IsMasterLockUnlocked(true); //Now let's check the PW if we need!
  else 
    return true;
}
CStdString CGUIPassword::GetSMBAuthFilename(const CStdString& strAuth)
{
  /// \brief Gets the path of an authenticated share
  /// \param strAuth The SMB style path
  /// \return Path to share with proper username/password, or same as imput if none found in db
  CURL urlIn(strAuth);
  CStdString strPath(strAuth);

  CStdString strShare = urlIn.GetShareName();	// it's only the server\share we're interested in authenticating
  IMAPPASSWORDS it = m_mapSMBPasswordCache.find(strShare);
  if(it != m_mapSMBPasswordCache.end())
  {
    // if share found in cache use it to supply username and password
    CURL url(it->second);		// map value contains the full url of the originally authenticated share. map key is just the share
    CStdString strPassword = url.GetPassWord();
   
    CStdString strUserName = url.GetUserName();
    urlIn.SetPassword(strPassword);
    urlIn.SetUserName(strUserName);
    urlIn.GetURL(strPath);
  }  
  return strPath;
}

bool CGUIPassword::LockBookmark(const CStdString& strType, const CStdString& strName, bool bState)
{
  VECSHARES* pShares = g_settings.GetSharesFromType(strType);
  bool bResult = false;
  for (IVECSHARES it=pShares->begin();it != pShares->end();++it)
  {
    if (it->strName == strName)
    {
      if (it->m_iHasLock > 0)
      {
        it->m_iHasLock = bState?2:1;      
        bResult = true;
      }
      break;
    }
  }
  CGUIMessage msg(GUI_MSG_NOTIFY_ALL,0,0,GUI_MSG_DVDDRIVE_CHANGED_CD);
  m_gWindowManager.SendThreadMessage(msg);        

  return bResult;
}

void CGUIPassword::LockBookmarks()
{
  // lock all bookmarks (those with locks)
  const char* strType[5] = {"myprograms","music","video","pictures","files"};
  VECSHARES* pShares[5];
  pShares[0] = g_settings.GetSharesFromType("myprograms");
  pShares[1] = g_settings.GetSharesFromType("music");
  pShares[2] = g_settings.GetSharesFromType("video");
  pShares[3] = g_settings.GetSharesFromType("pictures");
  pShares[4] = g_settings.GetSharesFromType("files");
  for (int i=0;i<5;++i)
  {
    for (IVECSHARES it=pShares[i]->begin();it != pShares[i]->end();++it)    
      if (it->m_iLockMode != LOCK_MODE_EVERYONE)
        it->m_iHasLock = 2;
  }
  CGUIMessage msg(GUI_MSG_NOTIFY_ALL,0,0,GUI_MSG_DVDDRIVE_CHANGED_CD);
  m_gWindowManager.SendThreadMessage(msg);
}

void CGUIPassword::UnlockBookmarks()
{
   // open lock for all bookmarks
  const char* strType[5] = {"myprograms","music","video","pictures","files"};
  VECSHARES* pShares[5];
  pShares[0] = g_settings.GetSharesFromType("myprograms");
  pShares[1] = g_settings.GetSharesFromType("music");
  pShares[2] = g_settings.GetSharesFromType("video");
  pShares[3] = g_settings.GetSharesFromType("pictures");
  pShares[4] = g_settings.GetSharesFromType("files");
  for (int i=0;i<5;++i)
  {
    for (IVECSHARES it=pShares[i]->begin();it != pShares[i]->end();++it)    
      if (it->m_iLockMode != LOCK_MODE_EVERYONE)
        it->m_iHasLock = 1;
  }
  CGUIMessage msg(GUI_MSG_NOTIFY_ALL,0,0,GUI_MSG_DVDDRIVE_CHANGED_CD);
  m_gWindowManager.SendThreadMessage(msg);
}

void CGUIPassword::RemoveBookmarkLocks()
{
  // remove lock from all bookmarks
  const char* strType[5] = {"myprograms","music","video","pictures","files"};
  VECSHARES* pShares[5];
  pShares[0] = g_settings.GetSharesFromType("myprograms");
  pShares[1] = g_settings.GetSharesFromType("music");
  pShares[2] = g_settings.GetSharesFromType("video");
  pShares[3] = g_settings.GetSharesFromType("pictures");
  pShares[4] = g_settings.GetSharesFromType("files");
  g_settings.BeginBookmarkTransaction();
  for (int i=0;i<5;++i)
  {
    for (IVECSHARES it=pShares[i]->begin();it != pShares[i]->end();++it)    
      if (it->m_iLockMode != LOCK_MODE_EVERYONE) // remove old info
      {
        it->m_iHasLock = 0;
        it->m_iLockMode = LOCK_MODE_EVERYONE;
        g_settings.UpdateBookmark(strType[i],it->strName,"lockmode","0"); // removes locks from xml
      }
  }
  g_settings.CommitBookmarkTransaction();
  CGUIMessage msg(GUI_MSG_NOTIFY_ALL,0,0, GUI_MSG_DVDDRIVE_CHANGED_CD);
  m_gWindowManager.SendThreadMessage(msg);
}

bool CGUIPassword::IsDatabasePathUnlocked(CStdString& strPath, VECSHARES& vecShares)
{
  if (g_passwordManager.bMasterUser || g_settings.m_vecProfiles[0].getLockMode() == LOCK_MODE_EVERYONE)
    return true;

  // try to find the best matching bookmark
  bool bName = false;
  int iIndex = CUtil::GetMatchingShare(strPath, vecShares, bName);

  if (iIndex > -1)
    if (vecShares[iIndex].m_iHasLock < 2)
      return true;

  return false;
}