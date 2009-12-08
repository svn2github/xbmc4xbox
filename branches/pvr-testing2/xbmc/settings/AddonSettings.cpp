/*
 *      Copyright (C) 2005-2009 Team XBMC
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

#include "AddonSettings.h"
#include "Util.h"
#include "FileSystem/File.h"
#include "FileSystem/Directory.h"
#include "FileSystem/PluginDirectory.h"
#include "utils/Addon.h"
#include "utils/log.h"

using namespace DIRECTORY;
using namespace ADDON;

bool CAddonSettings::SaveFromDefault(void)
{
  if (!GetAddonRoot()) //if scraper has no settings return false
    return false;

  TiXmlElement *setting = GetAddonRoot()->FirstChildElement("setting");
  while (setting)
  {
    CStdString id;
    if (setting->Attribute("id"))
      id = setting->Attribute("id");
    CStdString value;
    if (setting->Attribute("default"))
      value = setting->Attribute("default");
    Set(id,value);
    setting = setting->NextSiblingElement("setting");
  }
  return true;
}

void CAddonSettings::Clear()
{
  m_addonXmlDoc.Clear();
  m_userXmlDoc.Clear();
}

void CAddonSettings::Set(const CStdString& key, const CStdString& value)
{
  if (key == "") return;

  // Try to find the setting and change its value
  if (!m_userXmlDoc.RootElement())
  {
    TiXmlElement node("settings");
    m_userXmlDoc.InsertEndChild(node);
  }
  TiXmlElement *setting = m_userXmlDoc.RootElement()->FirstChildElement("setting");
  while (setting)
  {
    const char *id = setting->Attribute("id");
    if (id && strcmpi(id, key) == 0)
    {
      setting->SetAttribute("value", value.c_str());
      return;
    }

    setting = setting->NextSiblingElement("setting");
  }

  // Setting not found, add it
  TiXmlElement nodeSetting("setting");
  nodeSetting.SetAttribute("id", key.c_str());
  nodeSetting.SetAttribute("value", value.c_str());
  m_userXmlDoc.RootElement()->InsertEndChild(nodeSetting);
}

CStdString CAddonSettings::Get(const CStdString& key) const
{
  if (m_userXmlDoc.RootElement())
  {
    // Try to find the setting and return its value
    const TiXmlElement *setting = m_userXmlDoc.RootElement()->FirstChildElement("setting");
    while (setting)
    {
      const char *id = setting->Attribute("id");
      if (id && strcmpi(id, key) == 0)
        return setting->Attribute("value");

      setting = setting->NextSiblingElement("setting");
    }
  }

  if (m_addonXmlDoc.RootElement())
  {
    // Try to find the setting in the addon and return its default value
    const TiXmlElement* setting = m_addonXmlDoc.RootElement()->FirstChildElement("setting");
    while (setting)
    {
      const char *id = setting->Attribute("id");
      if (id && strcmpi(id, key) == 0 && setting->Attribute("default"))
        return setting->Attribute("default");

      setting = setting->NextSiblingElement("setting");
    }
  }

  // Otherwise return empty string
  return "";
}

bool CAddonSettings::Load(const CURL& url)
{
  m_url = url;

  // create the users filepath
  m_userFileName = GetUserDirectory(url);
  CUtil::AddFileToFolder(m_userFileName, "settings.xml", m_userFileName);

  CStdString addonFileName = url.Get();
  addonFileName = CPluginDirectory::TranslatePluginDirectory(addonFileName);
  CUtil::AddFileToFolder(addonFileName, "resources", addonFileName);
  CUtil::AddFileToFolder(addonFileName, "settings.xml", addonFileName);

  if (!m_addonXmlDoc.LoadFile(addonFileName))
  {
    CLog::Log(LOGERROR, "Unable to load: %s, Line %d\n%s", addonFileName.c_str(), m_addonXmlDoc.ErrorRow(), m_addonXmlDoc.ErrorDesc());
    return false;
  }

  // Make sure that the addon XML has the settings element
  TiXmlElement *setting = m_addonXmlDoc.RootElement();
  if (!setting || strcmpi(setting->Value(), "settings") != 0)
  {
    CLog::Log(LOGERROR, "Error loading Settings %s: cannot find root element 'settings'", addonFileName.c_str());
    return false;
  }

  // Load the user saved settings. If it does not exist, create it
  if (!m_userXmlDoc.LoadFile(m_userFileName))
  {
    TiXmlDocument doc;
    TiXmlDeclaration decl("1.0", "UTF-8", "yes");
    doc.InsertEndChild(decl);

    TiXmlElement xmlRootElement("settings");
    doc.InsertEndChild(xmlRootElement);

    m_userXmlDoc = doc;

    // Don't worry about the actual settings, they will be set when the user clicks "Ok"
    // in the settings dialog
  }

  return true;
}

bool CAddonSettings::Load(const CAddon& addon)
{
  // create the users filepath
  m_userFileName = GetUserDirectory(addon.Path());

  // If it is a virtual child add-on add the guid to the path
  if (!addon.Parent().IsEmpty())
    m_userFileName += "-" + addon.UUID();

  CUtil::AddFileToFolder(m_userFileName, "settings.xml", m_userFileName);

  CStdString addonFileName = addon.Path();
  CUtil::AddFileToFolder(addonFileName, "resources", addonFileName);
  CUtil::AddFileToFolder(addonFileName, "settings.xml", addonFileName);

  if (!m_addonXmlDoc.LoadFile(addonFileName))
  {
    CLog::Log(LOGERROR, "Unable to load: %s, Line %d\n%s", addonFileName.c_str(), m_addonXmlDoc.ErrorRow(), m_addonXmlDoc.ErrorDesc());
    return false;
  }

  // Make sure that the addon XML has the settings element
  TiXmlElement *setting = m_addonXmlDoc.RootElement();
  if (!setting || strcmpi(setting->Value(), "settings") != 0)
  {
    CLog::Log(LOGERROR, "Error loading Settings %s: cannot find root element 'settings'", addonFileName.c_str());
    return false;
  }

  // Load the user saved settings. If it does not exist, create it
  if (!m_userXmlDoc.LoadFile(m_userFileName))
  {
    TiXmlDocument doc;
    TiXmlDeclaration decl("1.0", "UTF-8", "yes");
    doc.InsertEndChild(decl);

    TiXmlElement xmlRootElement("settings");
    doc.InsertEndChild(xmlRootElement);

    m_userXmlDoc = doc;

    // Don't worry about the actual settings, they will be set when the user clicks "Ok"
    // in the settings dialog
  }
  return true;
}

bool CAddonSettings::Save(void)
{
  // break down the path into directories
  CStdString strRoot, strType, strAddon;
  CUtil::GetDirectory(m_userFileName, strAddon);
  CUtil::RemoveSlashAtEnd(strAddon);
  CUtil::GetDirectory(strAddon, strType);
  CUtil::RemoveSlashAtEnd(strType);
  CUtil::GetDirectory(strType, strRoot);
  CUtil::RemoveSlashAtEnd(strRoot);

  // create the individual folders
  if (!DIRECTORY::CDirectory::Exists(strRoot))
    DIRECTORY::CDirectory::Create(strRoot);
  if (!DIRECTORY::CDirectory::Exists(strType))
    DIRECTORY::CDirectory::Create(strType);
  if (!DIRECTORY::CDirectory::Exists(strAddon))
    DIRECTORY::CDirectory::Create(strAddon);

  return m_userXmlDoc.SaveFile(m_userFileName);
}

TiXmlElement* CAddonSettings::GetAddonRoot()
{
  return m_addonXmlDoc.RootElement();
}

bool CAddonSettings::SettingsExist(const CStdString& strPath)
{
  CStdString addonFileName = CPluginDirectory::TranslatePluginDirectory(strPath);
  CUtil::AddFileToFolder(addonFileName, "resources", addonFileName);
  CUtil::AddFileToFolder(addonFileName, "settings.xml", addonFileName);

  // Load the settings file to verify it's valid
  TiXmlDocument xmlDoc;
  if (!xmlDoc.LoadFile(addonFileName))
    return false;

  // Make sure that the addon XML has the settings element
  TiXmlElement *setting = xmlDoc.RootElement();
  if (!setting || strcmpi(setting->Value(), "settings") != 0)
    return false;

  return true;
}

CStdString CAddonSettings::GetUserDirectory(const CURL& url)
{
  CStdString addonUserName;
  if (url.GetProtocol() == "plugin")
  {
    addonUserName.Format("special://profile/plugin_data/%s/%s", url.GetHostName().c_str(), url.GetFileName().c_str());
  }
  else
  {
    addonUserName = url.Get();
    // Remove the special path
    addonUserName.Replace("special://home/addons/", "");
    addonUserName.Replace("special://xbmc/addons/", "");

    // and create the users filepath
    addonUserName.Format("special://profile/addon_data/%s", addonUserName.c_str());
  }
  CUtil::RemoveSlashAtEnd(addonUserName);
  return addonUserName;
}

CAddonSettings g_currentAddonSettings;
