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

#include "GUIDialogAddonSettings.h"
#include "utils/IAddon.h"
#include "Settings.h"
#include "GUIDialogNumeric.h"
#include "GUIDialogFileBrowser.h"
#include "GUIControlGroupList.h"
#include "GUILabelControl.h"
#include "GUIDialogOK.h"
#include "Util.h"
#include "MediaManager.h"
#include "GUIRadioButtonControl.h"
#include "GUISpinControlEx.h"
#include "GUIImage.h"
#include "FileSystem/Directory.h"
#include "FileSystem/PluginDirectory.h"
#include "VideoInfoScanner.h"
#include "Scraper.h"
#include "GUIWindowManager.h"
#include "Application.h"
#include "GUIDialogKeyboard.h"
#include "FileItem.h"

using namespace std;
using namespace ADDON;
using XFILE::CDirectory;

#define CONTROL_AREA                    2
#define CONTROL_DEFAULT_BUTTON          3
#define CONTROL_DEFAULT_RADIOBUTTON     4
#define CONTROL_DEFAULT_SPIN            5
#define CONTROL_DEFAULT_SEPARATOR       6
#define CONTROL_DEFAULT_LABEL_SEPARATOR 7
#define ID_BUTTON_OK                    10
#define ID_BUTTON_CANCEL                11
#define ID_BUTTON_DEFAULT               12
#define CONTROL_HEADING_LABEL           20
#define CONTROL_START_CONTROL           100

CGUIDialogAddonSettings::CGUIDialogAddonSettings()
   : CGUIDialogBoxBase(WINDOW_DIALOG_ADDON_SETTINGS, "DialogAddonSettings.xml")
{}

CGUIDialogAddonSettings::~CGUIDialogAddonSettings(void)
{}

bool CGUIDialogAddonSettings::OnMessage(CGUIMessage& message)
{
  switch (message.GetMessage())
  {
    case GUI_MSG_WINDOW_INIT:
    {
      CGUIDialogBoxBase::OnMessage(message);
      FreeControls();
      CreateControls();
      return true;
    }

    case GUI_MSG_CLICKED:
      {
        int iControl = message.GetSenderId();
        bool bCloseDialog = false;

        if (iControl == ID_BUTTON_OK)
          SaveSettings();
        else if (iControl == ID_BUTTON_DEFAULT)
          SetDefaults();
        else
          bCloseDialog = ShowVirtualKeyboard(iControl);

        if (iControl == ID_BUTTON_OK || iControl == ID_BUTTON_CANCEL || bCloseDialog)
        {
          m_bConfirmed = true;
          Close();
          return true;
        }
      }
      break;
  }
  return CGUIDialogBoxBase::OnMessage(message);
}

// \brief Show CGUIDialogOK dialog, then wait for user to dismiss it.
bool CGUIDialogAddonSettings::ShowAndGetInput(const AddonPtr &addon)
{
  bool result(false);

  if (addon->HasSettings())
  { 
    // Create the dialog
    CGUIDialogAddonSettings* pDialog = NULL;
    pDialog = (CGUIDialogAddonSettings*) g_windowManager.GetWindow(WINDOW_DIALOG_ADDON_SETTINGS);
    if (!pDialog)
      return false;

    // Set the heading
    CStdString heading;
    heading.Format("$LOCALIZE[10004] - %s", addon->Name().c_str()); // "Settings - AddonName"
    pDialog->SetHeading(heading);

    if (addon->LoadSettings())
    {
      pDialog->SetAddon(addon);
      pDialog->DoModal();

      if (pDialog->m_bConfirmed)
      { // settings have changed //TODO dialog could have been confirmed with no change
        result = true;
        addon->SaveSettings();
      }
    }
  }
  else
  { // addon cannot be configured, inform user
    CGUIDialogOK::ShowAndGetInput(24000,0,24030,0);
  }

  return result;
}

bool CGUIDialogAddonSettings::ShowVirtualKeyboard(int iControl)
{
  int controlId = CONTROL_START_CONTROL;
  bool bCloseDialog = false;

  const TiXmlElement *setting = m_addon->GetSettingsXML()->FirstChildElement("setting");
  while (setting)
  {
    if (controlId == iControl)
    {
      const CGUIControl* control = GetControl(controlId);
      if (control->GetControlType() == CGUIControl::GUICONTROL_BUTTON)
      {
        const char *id = setting->Attribute("id");
        const char *type = setting->Attribute("type");
        const char *option = setting->Attribute("option");
        const char *source = setting->Attribute("source");
        CStdString value = m_buttonValues[id];

        if (strcmp(type, "text") == 0)
        {
          // get any options
          bool bHidden = false;
          if (option)
            bHidden = (strcmp(option, "hidden") == 0);

          if (CGUIDialogKeyboard::ShowAndGetInput(value, ((CGUIButtonControl*) control)->GetLabel(), true, bHidden))
          {
            m_buttonValues[id] = value;
            // if hidden hide input
            if (bHidden)
            {
              CStdString hiddenText;
              hiddenText.append(value.size(), L'*');
              ((CGUIButtonControl *)control)->SetLabel2(hiddenText);
            }
            else
              ((CGUIButtonControl*) control)->SetLabel2(value);
          }
        }
        else if (strcmp(type, "integer") == 0 && CGUIDialogNumeric::ShowAndGetNumber(value, ((CGUIButtonControl*) control)->GetLabel()))
        {
          m_buttonValues[id] = value;
          ((CGUIButtonControl*) control)->SetLabel2(value);
        }
        else if (strcmp(type, "ipaddress") == 0 && CGUIDialogNumeric::ShowAndGetIPAddress(value, ((CGUIButtonControl*) control)->GetLabel()))
        {
          m_buttonValues[id] = value;
          ((CGUIButtonControl*) control)->SetLabel2(value);
        }
        else if (strcmpi(type, "video") == 0 || strcmpi(type, "music") == 0 ||
          strcmpi(type, "pictures") == 0 || strcmpi(type, "programs") == 0 ||
          strcmpi(type, "folder") == 0 || strcmpi(type, "files") == 0)
        {
          // setup the shares
          VECSOURCES *shares = NULL;
          if (!source || strcmpi(source, "") == 0)
            shares = g_settings.GetSourcesFromType(type);
          else
            shares = g_settings.GetSourcesFromType(source);

          if (!shares)
          {
            VECSOURCES localShares, networkShares;
            g_mediaManager.GetLocalDrives(localShares);
            if (!source || strcmpi(source, "local") != 0)
              g_mediaManager.GetNetworkLocations(networkShares);
            localShares.insert(localShares.end(), networkShares.begin(), networkShares.end());
            shares = &localShares;
          }

          if (strcmpi(type, "folder") == 0)
          {
            // get any options
            bool bWriteOnly = false;
            if (option)
              bWriteOnly = (strcmpi(option, "writeable") == 0);

            if (CGUIDialogFileBrowser::ShowAndGetDirectory(*shares, ((CGUIButtonControl*) control)->GetLabel(), value, bWriteOnly))
            {
              ((CGUIButtonControl*) control)->SetLabel2(value);
              m_buttonValues[id] = value;
            }
          }
          else if (strcmpi(type, "pictures") == 0)
          {
            if (CGUIDialogFileBrowser::ShowAndGetImage(*shares, ((CGUIButtonControl*) control)->GetLabel(), value))
            {
              ((CGUIButtonControl*) control)->SetLabel2(value);
              m_buttonValues[id] = value;
            }
          }
          else
          {
            // set the proper mask
            CStdString strMask;
            if (setting->Attribute("mask"))
              strMask = setting->Attribute("mask");
            else
            {
              if (strcmpi(type, "video") == 0)
                strMask = g_settings.m_videoExtensions;
              else if (strcmpi(type, "music") == 0)
                strMask = g_settings.m_musicExtensions;
              else if (strcmpi(type, "programs") == 0)
#if defined(_WIN32_WINNT)
                strMask = ".exe|.bat|.cmd|.py";
#else
                strMask = "";
#endif
            }

            // get any options
            bool bUseThumbs = false;
            bool bUseFileDirectories = false;
            if (option)
            {
              bUseThumbs = (strcmpi(option, "usethumbs") == 0 || strcmpi(option, "usethumbs|treatasfolder") == 0);
              bUseFileDirectories = (strcmpi(option, "treatasfolder") == 0 || strcmpi(option, "usethumbs|treatasfolder") == 0);
            }

            if (CGUIDialogFileBrowser::ShowAndGetFile(*shares, strMask, ((CGUIButtonControl*) control)->GetLabel(), value))
            {
              ((CGUIButtonControl*) control)->SetLabel2(value);
              m_buttonValues[id] = value;
            }
          }
        }
        else if (strcmpi(type, "action") == 0)
        {
          if (setting->Attribute("default"))
          {
            CStdString action = setting->Attribute("default");
            // replace $CWD with the url of plugin
            action.Replace("$CWD", m_addon->Path());
            if (option)
              bCloseDialog = (strcmpi(option, "close") == 0);
            g_application.getApplicationMessenger().ExecBuiltIn(action);
          }
        }
        break;
      }
    }
    setting = setting->NextSiblingElement("setting");
    controlId++;
  }
  EnableControls();
  return bCloseDialog;
}

void CGUIDialogAddonSettings::SetHeading(const CStdString& strHeading)
{
  m_strHeading = strHeading;
}

void CGUIDialogAddonSettings::SetAddon(const ADDON::AddonPtr &addon)
{
  m_addon = addon;
}

// Go over all the settings and set their values according to the values of the GUI components
bool CGUIDialogAddonSettings::SaveSettings(void)
{
  if (!m_addon)
    return false;

  // Retrieve all the values from the GUI components and put them in the model
  int controlId = CONTROL_START_CONTROL;
  const TiXmlElement *setting = m_addon->GetSettingsXML()->FirstChildElement("setting");
  while (setting)
  {
    CStdString id;
    if (setting->Attribute("id"))
      id = setting->Attribute("id");
    const char *type = setting->Attribute("type");

    // skip type "lsep", it is not a required control
    if (type && strcmpi(type, "lsep") != 0)
    {
      const CGUIControl* control = GetControl(controlId);

      CStdString value;
      switch (control->GetControlType())
      {
        case CGUIControl::GUICONTROL_BUTTON:
          value = m_buttonValues[id];
          break;
        case CGUIControl::GUICONTROL_RADIO:
          value = ((CGUIRadioButtonControl*) control)->IsSelected() ? "true" : "false";
          break;
        case CGUIControl::GUICONTROL_SPINEX:
          if (strcmpi(type, "fileenum") == 0 || strcmpi(type, "labelenum") == 0)
            value = ((CGUISpinControlEx*) control)->GetLabel();
          else
            value.Format("%i", ((CGUISpinControlEx*) control)->GetValue());
          break;
        default:
          break;
      }
      m_addon->UpdateSetting(id, value, CStdString(type));
    }
    setting = setting->NextSiblingElement("setting");
    controlId++;
  }
  return true;
}

void CGUIDialogAddonSettings::FreeControls()
{
  // clear the category group
  CGUIControlGroupList *control = (CGUIControlGroupList *)GetControl(CONTROL_AREA);
  if (control)
  {
    control->FreeResources();
    control->ClearAll();
  }
}

void CGUIDialogAddonSettings::CreateControls()
{
  CGUISpinControlEx *pOriginalSpin = (CGUISpinControlEx*)GetControl(CONTROL_DEFAULT_SPIN);
  CGUIRadioButtonControl *pOriginalRadioButton = (CGUIRadioButtonControl *)GetControl(CONTROL_DEFAULT_RADIOBUTTON);
  CGUIButtonControl *pOriginalButton = (CGUIButtonControl *)GetControl(CONTROL_DEFAULT_BUTTON);
  CGUIImage *pOriginalImage = (CGUIImage *)GetControl(CONTROL_DEFAULT_SEPARATOR);
  CGUILabelControl *pOriginalLabel = (CGUILabelControl *)GetControl(CONTROL_DEFAULT_LABEL_SEPARATOR);

  if (!m_addon || !pOriginalSpin || !pOriginalRadioButton || !pOriginalButton || !pOriginalImage)
    return;

  pOriginalSpin->SetVisible(false);
  pOriginalRadioButton->SetVisible(false);
  pOriginalButton->SetVisible(false);
  pOriginalImage->SetVisible(false);
  if (pOriginalLabel)
    pOriginalLabel->SetVisible(false);

  // clear the category group
  CGUIControlGroupList *group = (CGUIControlGroupList *)GetControl(CONTROL_AREA);
  if (!group)
    return;

  // set our dialog heading
  SET_CONTROL_LABEL(CONTROL_HEADING_LABEL, m_strHeading);

  // Create our base path, used for type "fileenum" settings
  CStdString basepath(m_addon->Path());

  CGUIControl* pControl = NULL;
  int controlId = CONTROL_START_CONTROL;
  const TiXmlElement *setting = m_addon->GetSettingsXML()->FirstChildElement("setting");
  while (setting)
  {
    const char *type = setting->Attribute("type");
    const char *id = setting->Attribute("id");
    CStdString values;
    if (setting->Attribute("values"))
      values = setting->Attribute("values");
    CStdString lvalues;
    if (setting->Attribute("lvalues"))
      lvalues = setting->Attribute("lvalues");
    CStdString entries;
    if (setting->Attribute("entries"))
      entries = setting->Attribute("entries");
    CStdString label;
    if (setting->Attribute("label") && atoi(setting->Attribute("label")) > 0)
    {
      if (m_addon->Parent())
        label.Format("$ADDON[%s %s]", m_addon->Parent()->UUID().c_str(), setting->Attribute("label"));
      else
        label.Format("$ADDON[%s %s]", m_addon->UUID().c_str(), setting->Attribute("label"));
    }
    else
      label = setting->Attribute("label");

    bool bSort=false;
    const char *sort = setting->Attribute("sort");
    if (sort && (strcmp(sort, "yes") == 0))
      bSort=true;

    if (type)
    {
      if (strcmpi(type, "text") == 0 || strcmpi(type, "ipaddress") == 0 ||
        strcmpi(type, "integer") == 0 || strcmpi(type, "video") == 0 ||
        strcmpi(type, "music") == 0 || strcmpi(type, "pictures") == 0 ||
        strcmpi(type, "folder") == 0 || strcmpi(type, "programs") == 0 ||
        strcmpi(type, "files") == 0 || strcmpi(type, "action") == 0)
      {
        pControl = new CGUIButtonControl(*pOriginalButton);
        if (!pControl) return;
        ((CGUIButtonControl *)pControl)->SettingsCategorySetTextAlign(XBFONT_CENTER_Y);
        ((CGUIButtonControl *)pControl)->SetLabel(label);
        if (id)
        {
          m_buttonValues[id] = m_addon->GetSetting(id);
          // get any option to test for hidden
          const char *option = setting->Attribute("option");
          if (option && (strcmp(option, "hidden") == 0))
          {
            CStdString hiddenText;
            hiddenText.append(m_addon->GetSetting(id).size(), L'*');
            ((CGUIButtonControl *)pControl)->SetLabel2(hiddenText);
          }
          else
            ((CGUIButtonControl *)pControl)->SetLabel2(m_addon->GetSetting(id));
        }
      }
      else if (strcmpi(type, "bool") == 0)
      {
        pControl = new CGUIRadioButtonControl(*pOriginalRadioButton);
        if (!pControl) return;
        ((CGUIRadioButtonControl *)pControl)->SetLabel(label);
        ((CGUIRadioButtonControl *)pControl)->SetSelected(m_addon->GetSetting(id) == "true");
      }
      else if (strcmpi(type, "enum") == 0 || strcmpi(type, "labelenum") == 0)
      {
        vector<CStdString> valuesVec;
        vector<CStdString> entryVec;

        pControl = new CGUISpinControlEx(*pOriginalSpin);
        if (!pControl) return;
        ((CGUISpinControlEx *)pControl)->SetText(label);

        if (!lvalues.IsEmpty())
          CUtil::Tokenize(lvalues, valuesVec, "|");
        else
          CUtil::Tokenize(values, valuesVec, "|");
        if (!entries.IsEmpty())
          CUtil::Tokenize(entries, entryVec, "|");

        if(bSort && strcmpi(type, "labelenum") == 0)
          std::sort(valuesVec.begin(), valuesVec.end(), sortstringbyname());

        for (unsigned int i = 0; i < valuesVec.size(); i++)
        {
          int iAdd = i;
          if (entryVec.size() > i)
            iAdd = atoi(entryVec[i]);
          if (!lvalues.IsEmpty())
          {
            CStdString replace = m_addon->GetString(atoi(valuesVec[i]));
            if (replace.IsEmpty())
              replace = g_localizeStrings.Get(atoi(valuesVec[i]));
            ((CGUISpinControlEx *)pControl)->AddLabel(replace, iAdd);
          }
          else
            ((CGUISpinControlEx *)pControl)->AddLabel(valuesVec[i], iAdd);
        }
        if (strcmpi(type, "labelenum") == 0)
        { // need to run through all our settings and find the one that matches
          ((CGUISpinControlEx*) pControl)->SetValueFromLabel(m_addon->GetSetting(id));
        }
        else
          ((CGUISpinControlEx*) pControl)->SetValue(atoi(m_addon->GetSetting(id)));

      }
      else if (strcmpi(type, "fileenum") == 0)
      {
        pControl = new CGUISpinControlEx(*pOriginalSpin);
        if (!pControl) return;
        ((CGUISpinControlEx *)pControl)->SetText(label);

        //find Folders...
        CFileItemList items;
        CStdString enumpath;
        CUtil::AddFileToFolder(basepath, values, enumpath);
        CStdString mask;
        if (setting->Attribute("mask"))
          mask = setting->Attribute("mask");
        if (!mask.IsEmpty())
          CDirectory::GetDirectory(enumpath, items, mask);
        else
          CDirectory::GetDirectory(enumpath, items);

        int iItem = 0;
        for (int i = 0; i < items.Size(); ++i)
        {
          CFileItemPtr pItem = items[i];
          if ((mask.Equals("/") && pItem->m_bIsFolder) || !pItem->m_bIsFolder)
          {
            ((CGUISpinControlEx *)pControl)->AddLabel(pItem->GetLabel(), iItem);
            if (pItem->GetLabel().Equals(m_addon->GetSetting(id)))
              ((CGUISpinControlEx *)pControl)->SetValue(iItem);
            iItem++;
          }
        }
      }
      else if (strcmpi(type, "lsep") == 0 && pOriginalLabel)
      {
        pControl = new CGUILabelControl(*pOriginalLabel);
        if (pControl)
          ((CGUILabelControl *)pControl)->SetLabel(label);
      }
      else if ((strcmpi(type, "sep") == 0 || strcmpi(type, "lsep") == 0) && pOriginalImage)
        pControl = new CGUIImage(*pOriginalImage);
    }

    if (pControl)
    {
      pControl->SetWidth(group->GetWidth());
      pControl->SetVisible(true);
      pControl->SetID(controlId);
      pControl->AllocResources();
      group->AddControl(pControl);
      pControl = NULL;
    }

    setting = setting->NextSiblingElement("setting");
    controlId++;
  }
  EnableControls();
}

// Go over all the settings and set their enabled condition according to the values of the enabled attribute
void CGUIDialogAddonSettings::EnableControls()
{
  int controlId = CONTROL_START_CONTROL;
  const TiXmlElement *setting = m_addon->GetSettingsXML()->FirstChildElement("setting");
  while (setting)
  {
    const CGUIControl* control = GetControl(controlId);
    if (control)
    {
      // set enable status
      if (setting->Attribute("enable"))
        ((CGUIControl*) control)->SetEnabled(GetCondition(setting->Attribute("enable"), controlId));
      else
        ((CGUIControl*) control)->SetEnabled(true);
      // set visible status
      if (setting->Attribute("visible"))
        ((CGUIControl*) control)->SetVisible(GetCondition(setting->Attribute("visible"), controlId));
      else
        ((CGUIControl*) control)->SetVisible(true);
    }
    setting = setting->NextSiblingElement("setting");
    controlId++;
  }
}

bool CGUIDialogAddonSettings::GetCondition(const CStdString &condition, const int controlId)
{
  if (condition.IsEmpty()) return true;

  bool bCondition = true;
  bool bCompare = true;
  vector<CStdString> conditionVec;
  if (condition.Find("+") >= 0)
    CUtil::Tokenize(condition, conditionVec, "+");
  else
  {
    bCondition = false;
    bCompare = false;
    CUtil::Tokenize(condition, conditionVec, "|");
  }

  for (unsigned int i = 0; i < conditionVec.size(); i++)
  {
    vector<CStdString> condVec;
    if (!TranslateSingleString(conditionVec[i], condVec)) continue;

    const CGUIControl* control2 = GetControl(controlId + atoi(condVec[1]));

    CStdString value;
    switch (control2->GetControlType())
    {
      case CGUIControl::GUICONTROL_BUTTON:
        value = ((CGUIButtonControl*) control2)->GetLabel2();
        break;
      case CGUIControl::GUICONTROL_RADIO:
        value = ((CGUIRadioButtonControl*) control2)->IsSelected() ? "true" : "false";
        break;
      case CGUIControl::GUICONTROL_SPINEX:
        value.Format("%i", ((CGUISpinControlEx*) control2)->GetValue());
        break;
      default:
        break;
    }

    if (condVec[0].Equals("eq"))
    {
      if (bCompare)
        bCondition &= value.Equals(condVec[2]);
      else
        bCondition |= value.Equals(condVec[2]);
    }
    else if (condVec[0].Equals("!eq"))
    {
      if (bCompare)
        bCondition &= !value.Equals(condVec[2]);
      else
        bCondition |= !value.Equals(condVec[2]);
    }
    else if (condVec[0].Equals("gt"))
    {
      if (bCompare)
        bCondition &= (atoi(value) > atoi(condVec[2]));
      else
        bCondition |= (atoi(value) > atoi(condVec[2]));
    }
    else if (condVec[0].Equals("lt"))
    {
      if (bCompare)
        bCondition &= (atoi(value) < atoi(condVec[2]));
      else
        bCondition |= (atoi(value) < atoi(condVec[2]));
    }
  }
  return bCondition;
}

bool CGUIDialogAddonSettings::TranslateSingleString(const CStdString &strCondition, vector<CStdString> &condVec)
{
  CStdString strTest = strCondition;
  strTest.ToLower();
  strTest.TrimLeft(" ");
  strTest.TrimRight(" ");

  int pos1 = strTest.Find("(");
  int pos2 = strTest.Find(",");
  int pos3 = strTest.Find(")");
  if (pos1 >= 0 && pos2 > pos1 && pos3 > pos2)
  {
    condVec.push_back(strTest.Left(pos1));
    condVec.push_back(strTest.Mid(pos1 + 1, pos2 - pos1 - 1));
    condVec.push_back(strTest.Mid(pos2 + 1, pos3 - pos2 - 1));
    return true;
  }
  return false;
}

// Go over all the settings and set their default values
void CGUIDialogAddonSettings::SetDefaults()
{
  int controlId = CONTROL_START_CONTROL;
  if (!m_addon)
    return;

  const TiXmlElement *setting = m_addon->GetSettingsXML()->FirstChildElement("setting");
  while (setting)
  {
    const CGUIControl* control = GetControl(controlId);
    if (control)
    {
      CStdString value;
      switch (control->GetControlType())
      {
        case CGUIControl::GUICONTROL_BUTTON:
          if (setting->Attribute("default") && setting->Attribute("id"))
            ((CGUIButtonControl*) control)->SetLabel2(setting->Attribute("default"));
          else
            ((CGUIButtonControl*) control)->SetLabel2("");
          break;
        case CGUIControl::GUICONTROL_RADIO:
          if (setting->Attribute("default"))
            ((CGUIRadioButtonControl*) control)->SetSelected(strcmpi(setting->Attribute("default"), "true") == 0);
          else
            ((CGUIRadioButtonControl*) control)->SetSelected(false);
          break;
        case CGUIControl::GUICONTROL_SPINEX:
          {
            if (setting->Attribute("default"))
            {
              if (strcmpi(setting->Attribute("type"), "fileenum") == 0 || strcmpi(setting->Attribute("type"), "labelenum") == 0)
              { // need to run through all our settings and find the one that matches
                  ((CGUISpinControlEx*) control)->SetValueFromLabel(setting->Attribute("default"));
              }
              else
                ((CGUISpinControlEx*) control)->SetValue(atoi(setting->Attribute("default")));
            }
            else
              ((CGUISpinControlEx*) control)->SetValue(0);
          }
          break;
        default:
          break;
      }
    }
    setting = setting->NextSiblingElement("setting");
    controlId++;
  }
  EnableControls();
}

