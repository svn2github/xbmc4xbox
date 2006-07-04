#include "stdafx.h"
#include "GUIDialogProfileSettings.h"
#include "GUIDialogFileBrowser.h"
#include "GUIDialogContextMenu.h"
#include "GUIListControl.h"
#include "GUIDialogNumeric.h"
#include "GUIDialogKeyboard.h"
#include "GUIDialogGamepad.h"
#include "GUIDialogLockSettings.h"
#include "MediaManager.h"
#include "Util.h"
#include "picture.h"

#define CONTROL_PROFILE_IMAGE      2
#define CONTROL_START              30

CGUIDialogProfileSettings::CGUIDialogProfileSettings(void)
    : CGUIDialogSettings(WINDOW_DIALOG_PROFILE_SETTINGS, "ProfileSettings.xml")
{
  m_bNeedSave = false;
  m_loadOnDemand = true;
}

CGUIDialogProfileSettings::~CGUIDialogProfileSettings(void)
{
}

bool CGUIDialogProfileSettings::OnMessage(CGUIMessage &message)
{
  switch (message.GetMessage())
  {
  case GUI_MSG_WINDOW_DEINIT:
    {
      CGUIDialogSettings::OnMessage(message);
    }
    break;

  case GUI_MSG_CLICKED:
    int iControl = message.GetSenderId();
    if (iControl == 500)
      Close();
    if (iControl == 501)
    {
      m_bNeedSave = false;
      Close();
    }
    break;
  }
  return CGUIDialogSettings::OnMessage(message);
}

void CGUIDialogProfileSettings::OnWindowLoaded()
{
  CGUIImage *pImage = (CGUIImage*)GetControl(2);
  m_strDefaultImage = pImage->GetFileName();
}

void CGUIDialogProfileSettings::SetupPage()
{
  CGUIDialogSettings::SetupPage();
  SET_CONTROL_LABEL(1000,m_strName);
  SET_CONTROL_LABEL(1001,m_strDirectory);
  CGUIImage *pImage = (CGUIImage*)GetControl(2);
  if (!m_strThumb.IsEmpty())
    pImage->SetFileName(m_strThumb);
  else
    pImage->SetFileName(m_strDefaultImage);
}

void CGUIDialogProfileSettings::CreateSettings()
{
  // clear out any old settings
  m_settings.clear();
  
  AddButton(1,20093);
  AddButton(2,20065);
  if (!m_bIsDefault)
    AddButton(3,20070);
  if (g_settings.m_vecProfiles[0].getLockMode() != LOCK_MODE_EVERYONE || m_bIsDefault)
    AddButton(4,20066);

  if (!m_bIsDefault)
  {
    SettingInfo setting;
    setting.id = 5;
    setting.name = g_localizeStrings.Get(20060);
    setting.data = &m_iDbMode;
    setting.type = SettingInfo::SPIN;
    setting.entry.push_back(g_localizeStrings.Get(20061));
    setting.entry.push_back(g_localizeStrings.Get(20062));
    setting.entry.push_back(g_localizeStrings.Get(20063));
    m_settings.push_back(setting);

    SettingInfo setting2;
    setting2.id = 6;
    setting2.name = g_localizeStrings.Get(20094);
    setting2.data = &m_iSourcesMode;
    setting2.type = SettingInfo::SPIN;
    setting2.entry.push_back(g_localizeStrings.Get(20061));
    setting2.entry.push_back(g_localizeStrings.Get(20062));
    setting2.entry.push_back(g_localizeStrings.Get(20063));
    m_settings.push_back(setting2);
  }
  if (m_bIsNewUser)
  {
    SetupPage();
    OnSettingChanged(0); // id=1
    OnSettingChanged(2); // id=3
  }
}

void CGUIDialogProfileSettings::OnSettingChanged(unsigned int num)
{
  
  // setting has changed - update anything that needs it
  if (num >= m_settings.size()) return;
  SettingInfo &setting = m_settings.at(num);
  // check and update anything that needs it
  if (setting.id == 1)
  {
    if (CGUIDialogKeyboard::ShowAndGetInput(m_strName,"Profile name",false))
    {
      m_bNeedSave = true;
      SET_CONTROL_LABEL(1000,m_strName);
    }
  }
  if (setting.id == 2)
  {
    CStdString strThumb;
    if (CGUIDialogFileBrowser::ShowAndGetImage(*g_settings.GetSharesFromType("files"),g_localizeStrings.Get(20065),strThumb))
    {
      m_bNeedSave = true;
      CGUIImage *pImage = (CGUIImage*)GetControl(2);
      m_strThumb = CFileItem(strThumb).GetCachedProfileThumb();
      if (!CFile::Exists(m_strThumb))
      {
        CPicture pic;
        pic.DoCreateThumbnail(strThumb, m_strThumb);
      }
      pImage->SetFileName(m_strThumb);
    }
  }
  if (setting.id == 3)
  {
    VECSHARES shares;
    CShare share;
    share.strName = "Profiles";
    share.strPath = g_settings.m_vecProfiles[0].getDirectory()+"\\profiles";
    shares.push_back(share);
    if (m_strDirectory == "")
      m_strDirectory = share.strPath;
    
    if (CGUIDialogFileBrowser::ShowAndGetDirectory(shares,g_localizeStrings.Get(20070),m_strDirectory,true))
    {
      if (!m_bIsDefault)
        m_strDirectory.erase(0,g_settings.m_vecProfiles[0].getDirectory().size()+1);       
      m_bNeedSave = true;
      SET_CONTROL_LABEL(1001,m_strDirectory);
    }
  }

  if (setting.id == 4)
  {
    if (CGUIDialogLockSettings::ShowAndGetLock(m_iLockMode,m_strLockCode,m_bLockMusic,m_bLockVideo,m_bLockPictures,m_bLockPrograms,m_bLockFiles,m_bLockSettings,m_bIsDefault?12360:20068))
    {
      m_bNeedSave = true;
    }
  }
  if (setting.id > 4)
    m_bNeedSave = true;
}

bool CGUIDialogProfileSettings::ShowForProfile(unsigned int iProfile)
{
  CGUIDialogProfileSettings *dialog = (CGUIDialogProfileSettings *)m_gWindowManager.GetWindow(WINDOW_DIALOG_PROFILE_SETTINGS);
  if (!dialog) return false;
  if (iProfile == 0)
    dialog->m_bIsDefault = true;
  else
    dialog->m_bIsDefault = false;

  if (iProfile >= g_settings.m_vecProfiles.size())
  {
    dialog->m_strName.Empty();
    dialog->m_iDbMode = 0;
    dialog->m_iLockMode = LOCK_MODE_EVERYONE;
    dialog->m_iSourcesMode = 0;
    dialog->m_bLockSettings = true;
    dialog->m_bLockMusic = false;
    dialog->m_bLockVideo = false;
    dialog->m_bLockFiles = true;
    dialog->m_bLockPictures = false;
    dialog->m_bLockPrograms = false;

    dialog->m_strDirectory.Empty();
    dialog->m_strThumb.Empty();
    dialog->m_strName = "";
    dialog->m_bIsNewUser = true;
  }
  else
  {
    dialog->m_strName = g_settings.m_vecProfiles[iProfile].getName();
    dialog->m_strThumb = g_settings.m_vecProfiles[iProfile].getThumb();
    dialog->m_strDirectory = g_settings.m_vecProfiles[iProfile].getDirectory();
    dialog->m_iDbMode = g_settings.m_vecProfiles[iProfile].hasDatabases()?0:1;
    dialog->m_iSourcesMode = g_settings.m_vecProfiles[iProfile].hasSources()?0:1;
    if (!g_settings.m_vecProfiles[iProfile].canWriteDatabases() && !g_settings.m_vecProfiles[iProfile].hasDatabases())
      dialog->m_iDbMode = 2;
    if (!g_settings.m_vecProfiles[iProfile].canWriteSources() && !g_settings.m_vecProfiles[iProfile].hasSources())
      dialog->m_iSourcesMode = 2;
    
    dialog->m_iLockMode = g_settings.m_vecProfiles[iProfile].getLockMode();
    dialog->m_strLockCode = g_settings.m_vecProfiles[iProfile].getLockCode();
    dialog->m_bLockFiles = g_settings.m_vecProfiles[iProfile].filesLocked();
    dialog->m_bLockMusic = g_settings.m_vecProfiles[iProfile].musicLocked();
    dialog->m_bLockVideo = g_settings.m_vecProfiles[iProfile].videoLocked();
    dialog->m_bLockPrograms = g_settings.m_vecProfiles[iProfile].programsLocked();
    dialog->m_bLockPictures = g_settings.m_vecProfiles[iProfile].picturesLocked();
    dialog->m_bLockSettings = g_settings.m_vecProfiles[iProfile].settingsLocked();
    dialog->m_bIsNewUser = false;
  }
  dialog->DoModal();
  if (dialog->m_bNeedSave)
  {
    if (iProfile >= g_settings.m_vecProfiles.size())
    {
      if (dialog->m_strName.IsEmpty() || dialog->m_strDirectory.IsEmpty())
        return false;
      CStdString strLabel;
      strLabel.Format(g_localizeStrings.Get(20047),dialog->m_strName);
      if (!CGUIDialogYesNo::ShowAndGetInput(g_localizeStrings.Get(20058),strLabel,dialog->m_strDirectory,""))
        return false;

      // check for old profile settings
      CProfile profile;
      g_settings.m_vecProfiles.push_back(profile);      
      bool bExists = CFile::Exists(g_settings.GetUserDataFolder()+"\\"+dialog->m_strDirectory+"\\guisettings.xml");

      if (bExists)
        if (!CGUIDialogYesNo::ShowAndGetInput(20058,20103,20104,20022))
          bExists = false;

      if (!bExists)
      {
        // save new profile guisettings
        if (CGUIDialogYesNo::ShowAndGetInput(20058,20048,20102,20022,20044,20064))
          CFile::Cache(g_settings.GetUserDataFolder()+"\\guisettings.xml",g_settings.GetUserDataFolder()+"\\"+dialog->m_strDirectory+"\\guisettings.xml");
        else
        {
          CGUISettings settings = g_guiSettings;
          CGUISettings settings2;
          g_guiSettings = settings2;
          g_settings.SaveSettings(g_settings.GetUserDataFolder()+"\\"+dialog->m_strDirectory+"\\guisettings.xml");
          g_guiSettings = settings;
        }
      }

      bExists = CFile::Exists(g_settings.GetUserDataFolder()+"\\"+dialog->m_strDirectory+"\\sources.xml");
      if (bExists)
        if (!CGUIDialogYesNo::ShowAndGetInput(20058,20105,20104,20022))
          bExists = false;

      if (!bExists)
      {
        if (dialog->m_iSourcesMode == 0)
          if (CGUIDialogYesNo::ShowAndGetInput(20058,20071,20102,20022,20044,20064))
            CFile::Cache(g_settings.GetUserDataFolder()+"\\sources.xml",g_settings.GetUserDataFolder()+"\\"+dialog->m_strDirectory+"\\sources.xml");
          else
          {
            TiXmlDocument xmlDoc;
            TiXmlElement xmlRootElement("sources");
            TiXmlNode *pRoot = xmlDoc.InsertEndChild(xmlRootElement);
            TiXmlElement xmlProgramElement("myprograms");
            pRoot->InsertEndChild(xmlProgramElement);
            TiXmlElement xmlVideoElement("video");
            pRoot->InsertEndChild(xmlVideoElement);
            TiXmlElement xmlMusicElement("music");
            pRoot->InsertEndChild(xmlMusicElement);
            TiXmlElement xmlPicturesElement("pictures");
            pRoot->InsertEndChild(xmlPicturesElement);
            TiXmlElement xmlFilesElement("files");
            pRoot->InsertEndChild(xmlFilesElement); 
            xmlDoc.SaveFile(g_settings.GetUserDataFolder()+"\\"+dialog->m_strDirectory+"\\sources.xml");
          }
      }
    }

    if (!dialog->m_bIsNewUser)
      if (!CGUIDialogYesNo::ShowAndGetInput(20067,20103,20022,20022))
        return false;

    g_settings.m_vecProfiles[iProfile].setName(dialog->m_strName);
    g_settings.m_vecProfiles[iProfile].setDirectory(dialog->m_strDirectory);
    g_settings.m_vecProfiles[iProfile].setThumb(dialog->m_strThumb);
    g_settings.m_vecProfiles[iProfile].setWriteDatabases(dialog->m_iDbMode < 2);
    g_settings.m_vecProfiles[iProfile].setWriteSources(dialog->m_iSourcesMode < 2);
    g_settings.m_vecProfiles[iProfile].setDatabases(dialog->m_iDbMode == 0);
    g_settings.m_vecProfiles[iProfile].setSources(dialog->m_iSourcesMode == 0);
    if (dialog->m_strLockCode == "-")
      g_settings.m_vecProfiles[iProfile].setLockMode(LOCK_MODE_EVERYONE);
    else
      g_settings.m_vecProfiles[iProfile].setLockMode(dialog->m_iLockMode);
    if (dialog->m_iLockMode == LOCK_MODE_EVERYONE)
      g_settings.m_vecProfiles[iProfile].setLockCode("-");
    else
      g_settings.m_vecProfiles[iProfile].setLockCode(dialog->m_strLockCode);
    g_settings.m_vecProfiles[iProfile].setMusicLocked(dialog->m_bLockMusic);
    g_settings.m_vecProfiles[iProfile].setVideoLocked(dialog->m_bLockVideo);
    g_settings.m_vecProfiles[iProfile].setSettingsLocked(dialog->m_bLockSettings);
    g_settings.m_vecProfiles[iProfile].setFilesLocked(dialog->m_bLockFiles);
    g_settings.m_vecProfiles[iProfile].setPicturesLocked(dialog->m_bLockPictures);
    g_settings.m_vecProfiles[iProfile].setProgramsLocked(dialog->m_bLockPrograms);

    g_settings.SaveProfiles("q:\\system\\profiles.xml");
    if (iProfile == g_settings.m_iLastLoadedProfileIndex)
      g_settings.LoadProfile(iProfile); // to activate changes
    return true;
  }

  return false;
}

void CGUIDialogProfileSettings::OnInitWindow()
{
  m_bNeedSave = false;

  CGUIDialogSettings::OnInitWindow();
}