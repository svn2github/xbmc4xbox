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

#include "stdafx.h"
#include "ButtonTranslator.h"
#include "Util.h"
#include "Settings.h"
#include "SkinInfo.h"
#include "Key.h"
#include "FileSystem/File.h"

using namespace std;
using namespace XFILE;

CButtonTranslator g_buttonTranslator;
extern CStdString g_LoadErrorStr;

CButtonTranslator::CButtonTranslator()
{}

CButtonTranslator::~CButtonTranslator()
{}

bool CButtonTranslator::Load()
{
  translatorMap.clear();

  // Load the config file
  CStdString keymapPath;
  bool success = false;

  keymapPath = "Q:\\system\\Keymap.xml";
  if(CFile::Exists(keymapPath))
    success |= LoadKeymap(keymapPath);
  else
    CLog::Log(LOGDEBUG, "CButtonTranslator::Load - no system keymap found, skipping");

  keymapPath = g_settings.GetUserDataItem("Keymap.xml");
  if(CFile::Exists(keymapPath))
    success |= LoadKeymap(keymapPath);
  else
    CLog::Log(LOGDEBUG, "CButtonTranslator::Load - no userdata keymap found, skipping");

  if (!success)
  {
    g_LoadErrorStr.Format("Error loading keymap: %s", keymapPath.c_str());
    return false;
  }

  // Done!
  return true;
}

bool CButtonTranslator::LoadKeymap(const CStdString &keymapPath)
{
  TiXmlDocument xmlDoc;

  CLog::Log(LOGINFO, "Loading %s", keymapPath.c_str());
  if (!xmlDoc.LoadFile(keymapPath))
  {
    CLog::Log(LOGERROR, "Error loading keymap: %s, Line %d\n%s", keymapPath.c_str(), xmlDoc.ErrorRow(), xmlDoc.ErrorDesc());
    return false;
  }
  TiXmlElement* pRoot = xmlDoc.RootElement();
  CStdString strValue = pRoot->Value();
  if ( strValue != "keymap")
  {
    CLog::Log(LOGERROR, "%s Doesn't contain <keymap>", keymapPath.c_str());
    return false;
  }
  // run through our window groups
  TiXmlNode* pWindow = pRoot->FirstChild();
  while (pWindow)
  {
    WORD wWindowID = WINDOW_INVALID;
    const char *szWindow = pWindow->Value();
    {
      if (szWindow)
      {
        if (strcmpi(szWindow, "global") == 0)
          wWindowID = (WORD) -1;
        else
          wWindowID = TranslateWindowString(szWindow);
      }
      MapWindowActions(pWindow, wWindowID);
      pWindow = pWindow->NextSibling();
    }
  }
  return true;
}

#if defined(HAS_SDL_JOYSTICK) || defined(HAS_EVENT_SERVER)
void CButtonTranslator::MapJoystickActions(WORD wWindowID, TiXmlNode *pJoystick)
{
  string joyname = "_xbmc_"; // default global map name
  vector<string> joynames;
  map<int, string> buttonMap;
  map<int, string> axisMap;

  TiXmlElement *pJoy = pJoystick->ToElement();
  if (pJoy && pJoy->Attribute("name"))
  {
    joyname = pJoy->Attribute("name");
  }
  else
  {
    CLog::Log(LOGNOTICE, "No Joystick name specified, loading default map");
  }

  joynames.push_back(joyname);

  // parse map
  TiXmlElement *pButton = pJoystick->FirstChildElement();
  int id = 0;
  //char* szId;
  const char* szType;
  const char *szAction;
  while (pButton)
  {
    szType = pButton->Value();
    szAction = pButton->GetText();
    if (szAction == NULL)
      szAction = "";
    if (szType)
    {
      if ((pButton->QueryIntAttribute("id", &id) == TIXML_SUCCESS) && id>=0 && id<=256)
      {
        if (strcmpi(szType, "button")==0)
        {
          buttonMap[id] = string(szAction);
        }
        else if (strcmpi(szType, "axis")==0)
        {
          int limit = 0;
          if (pButton->QueryIntAttribute("limit", &limit) == TIXML_SUCCESS)
          {
            if (limit==-1)
            {
              axisMap[-id] = string(szAction);
            }
            else if (limit==1)
            {
              axisMap[id] = string(szAction);
            }
            else if (limit==0)
            {
              axisMap[id|0xFFFF0000] = string(szAction);
            }
            else
            {
              axisMap[id] = string(szAction);
              axisMap[-id] = string(szAction);
              CLog::Log(LOGERROR, "Error in joystick map, invalid limit specified %d for axis %d", limit, id);
            }
          }
          else
          {
            axisMap[id] = string(szAction);
            axisMap[-id] = string(szAction);
          }
        }
        else
        {
          CLog::Log(LOGERROR, "Error reading joystick map element, unknown button type: %s", szType);
        }
      }
      else if (strcmpi(szType, "altname")==0)
      {
        joynames.push_back(string(szAction));
      }
      else
      {
        CLog::Log(LOGERROR, "Error reading joystick map element, Invalid id: %d", id);
      }
    }
    else
    {
      CLog::Log(LOGERROR, "Error reading joystick map element, skipping");
    }
    pButton = pButton->NextSiblingElement();
  }
  vector<string>::iterator it = joynames.begin();
  while (it!=joynames.end())
  {
    m_joystickButtonMap[*it][wWindowID] = buttonMap;
    m_joystickAxisMap[*it][wWindowID] = axisMap;
//    CLog::Log(LOGDEBUG, "Found Joystick map for window %d using %s", wWindowID, it->c_str());
    it++;
  }

  return;
}

bool CButtonTranslator::TranslateJoystickString(WORD wWindow, const char* szDevice, int id, bool axis, WORD& action, CStdString& strAction, bool &fullrange)
{
  bool found = false;

  map<string, JoystickMap>::iterator it;
  map<string, JoystickMap> *jmap;

  fullrange = false;

  if (axis)
  {
    jmap = &m_joystickAxisMap;
  }
  else
  {
    jmap = &m_joystickButtonMap;
  }

  it = jmap->find(szDevice);
  if (it==jmap->end())
    return false;

  JoystickMap wmap = it->second;
  JoystickMap::iterator it2;
  map<int, string> windowbmap;
  map<int, string> globalbmap;
  map<int, string>::iterator it3;

  it2 = wmap.find(wWindow);

  // first try local window map
  if (it2!=wmap.end())
  {
    windowbmap = it2->second;
    it3 = windowbmap.find(id);
    if (it3 != windowbmap.end())
    {
      strAction = (it3->second).c_str();
      found = true;
    }
    it3 = windowbmap.find(abs(id)|0xFFFF0000);
    if (it3 != windowbmap.end())
    {
      strAction = (it3->second).c_str();
      found = true;
      fullrange = true;
    }
  }

  // if not found, try global map
  if (!found)
  {
    it2 = wmap.find((WORD)-1);
    if (it2 != wmap.end())
    {
      globalbmap = it2->second;
      it3 = globalbmap.find(id);
      if (it3 != globalbmap.end())
      {
        strAction = (it3->second).c_str();
        found = true;
      }
      it3 = globalbmap.find(abs(id)|0xFFFF0000);
      if (it3 != globalbmap.end())
      {
        strAction = (it3->second).c_str();
        found = true;
        fullrange = true;
      }
    }
  }

  // translated found action
  if (found)
  {
    return TranslateActionString(strAction.c_str(), action);
  }

  return false;
}
#endif

void CButtonTranslator::GetAction(WORD wWindow, const CKey &key, CAction &action)
{
  CStdString strAction;
  // try to get the action from the current window
  WORD wAction = GetActionCode(wWindow, key, strAction);
  // if it's invalid, try to get it from the global map
  if (wAction == 0)
    wAction = GetActionCode( (WORD) -1, key, strAction);
  // Now fill our action structure
  action.wID = wAction;
  action.strAction = strAction;
  action.fAmount1 = 1; // digital button (could change this for repeat acceleration)
  action.fAmount2 = 0;
  action.fRepeat = key.GetRepeat();
  action.m_dwButtonCode = key.GetButtonCode();
  action.holdTime = key.GetHeld();
  // get the action amounts of the analog buttons
  if (key.GetButtonCode() == KEY_BUTTON_LEFT_ANALOG_TRIGGER)
  {
    action.fAmount1 = (float)key.GetLeftTrigger() / 255.0f;
  }
  else if (key.GetButtonCode() == KEY_BUTTON_RIGHT_ANALOG_TRIGGER)
  {
    action.fAmount1 = (float)key.GetRightTrigger() / 255.0f;
  }
  else if (key.GetButtonCode() == KEY_BUTTON_LEFT_THUMB_STICK)
  {
    action.fAmount1 = key.GetLeftThumbX();
    action.fAmount2 = key.GetLeftThumbY();
  }
  else if (key.GetButtonCode() == KEY_BUTTON_RIGHT_THUMB_STICK)
  {
    action.fAmount1 = key.GetRightThumbX();
    action.fAmount2 = key.GetRightThumbY();
  }
  else if (key.GetButtonCode() == KEY_BUTTON_LEFT_THUMB_STICK_UP)
    action.fAmount1 = key.GetLeftThumbY();
  else if (key.GetButtonCode() == KEY_BUTTON_LEFT_THUMB_STICK_DOWN)
    action.fAmount1 = -key.GetLeftThumbY();
  else if (key.GetButtonCode() == KEY_BUTTON_LEFT_THUMB_STICK_LEFT)
    action.fAmount1 = -key.GetLeftThumbX();
  else if (key.GetButtonCode() == KEY_BUTTON_LEFT_THUMB_STICK_RIGHT)
    action.fAmount1 = key.GetLeftThumbX();
  else if (key.GetButtonCode() == KEY_BUTTON_RIGHT_THUMB_STICK_UP)
    action.fAmount1 = key.GetRightThumbY();
  else if (key.GetButtonCode() == KEY_BUTTON_RIGHT_THUMB_STICK_DOWN)
    action.fAmount1 = -key.GetRightThumbY();
  else if (key.GetButtonCode() == KEY_BUTTON_RIGHT_THUMB_STICK_LEFT)
    action.fAmount1 = -key.GetRightThumbX();
  else if (key.GetButtonCode() == KEY_BUTTON_RIGHT_THUMB_STICK_RIGHT)
    action.fAmount1 = key.GetRightThumbX();
}

WORD CButtonTranslator::GetActionCode(WORD wWindow, const CKey &key, CStdString &strAction)
{
  WORD wKey = (WORD)key.GetButtonCode();
  map<WORD, buttonMap>::iterator it = translatorMap.find(wWindow);
  if (it == translatorMap.end())
    return 0;
  buttonMap::iterator it2 = (*it).second.find(wKey);
  WORD wAction = 0;
  while (it2 != (*it).second.end())
  {
    wAction = (*it2).second.wID;
    strAction = (*it2).second.strID;
    it2 = (*it).second.end();
  }
  return wAction;
}

void CButtonTranslator::MapAction(WORD wButtonCode, const char *szAction, buttonMap &map)
{
  WORD wAction = ACTION_NONE;
  if (!TranslateActionString(szAction, wAction) || !wButtonCode)
    return;   // no valid action, or an invalid buttoncode
  // have a valid action, and a valid button - map it.
  // check to see if we've already got this (button,action) pair defined
  buttonMap::iterator it = map.find(wButtonCode);
  if (it == map.end() || (*it).second.wID != wAction || (*it).second.strID != szAction)
  {
    // NOTE: This multimap is only being used as a normal map at this point (no support
    //       for multiple actions per key)
    if (it != map.end())
      map.erase(it);
    CButtonAction button;
    button.wID = wAction;
    button.strID = szAction;
    map.insert(pair<WORD, CButtonAction>(wButtonCode, button));
  }
}

void CButtonTranslator::MapWindowActions(TiXmlNode *pWindow, WORD wWindowID)
{
  if (!pWindow || wWindowID == WINDOW_INVALID) return;
  buttonMap map;
  std::map<WORD, buttonMap>::iterator it = translatorMap.find(wWindowID);
  if (it != translatorMap.end())
  {
    map = it->second;
    translatorMap.erase(it);
  }
  TiXmlNode* pDevice;
  if ((pDevice = pWindow->FirstChild("gamepad")) != NULL)
  { // map gamepad actions
    TiXmlElement *pButton = pDevice->FirstChildElement();
    while (pButton)
    {
      WORD wButtonCode = TranslateGamepadString(pButton->Value());
      if (pButton->FirstChild())
        MapAction(wButtonCode, pButton->FirstChild()->Value(), map);
      pButton = pButton->NextSiblingElement();
    }
  }
  if ((pDevice = pWindow->FirstChild("remote")) != NULL)
  { // map remote actions
    TiXmlElement *pButton = pDevice->FirstChildElement();
    while (pButton)
    {
      WORD wButtonCode = TranslateRemoteString(pButton->Value());
      if (pButton->FirstChild())
        MapAction(wButtonCode, pButton->FirstChild()->Value(), map);
      pButton = pButton->NextSiblingElement();
    }
  }
  if ((pDevice = pWindow->FirstChild("universalremote")) != NULL)
  { // map universal remote actions
    TiXmlElement *pButton = pDevice->FirstChildElement();
    while (pButton)
    {
      WORD wButtonCode = TranslateUniversalRemoteString(pButton->Value());
      if (pButton->FirstChild())
        MapAction(wButtonCode, pButton->FirstChild()->Value(), map);
      pButton = pButton->NextSiblingElement();
    }
  }
  if ((pDevice = pWindow->FirstChild("keyboard")) != NULL)
  { // map keyboard actions
    TiXmlElement *pButton = pDevice->FirstChildElement();
    while (pButton)
    {
      WORD wButtonCode = TranslateKeyboardButton(pButton);
      if (pButton->FirstChild())
        MapAction(wButtonCode, pButton->FirstChild()->Value(), map);
      pButton = pButton->NextSiblingElement();
    }
  }
#if defined(HAS_SDL_JOYSTICK) || defined(HAS_EVENT_SERVER)
  if ((pDevice = pWindow->FirstChild("joystick")) != NULL)
  {
    // map joystick actions
    while (pDevice)
    {
      MapJoystickActions(wWindowID, pDevice);
      pDevice = pDevice->NextSibling("joystick");
    }
  }
#endif
  // add our map to our table
  if (map.size() > 0)
    translatorMap.insert(pair<WORD, buttonMap>( wWindowID, map));
}

bool CButtonTranslator::TranslateActionString(const char *szAction, WORD &wAction)
{
  wAction = ACTION_NONE;
  CStdString strAction = szAction;
  strAction.ToLower();
  if (CUtil::IsBuiltIn(strAction)) wAction = ACTION_BUILT_IN_FUNCTION;
  else if (strAction.Equals("left")) wAction = ACTION_MOVE_LEFT;
  else if (strAction.Equals("right")) wAction = ACTION_MOVE_RIGHT;
  else if (strAction.Equals("up")) wAction = ACTION_MOVE_UP;
  else if (strAction.Equals("down")) wAction = ACTION_MOVE_DOWN;
  else if (strAction.Equals("pageup")) wAction = ACTION_PAGE_UP;
  else if (strAction.Equals("pagedown")) wAction = ACTION_PAGE_DOWN;
  else if (strAction.Equals("select")) wAction = ACTION_SELECT_ITEM;
  else if (strAction.Equals("highlight")) wAction = ACTION_HIGHLIGHT_ITEM;
  else if (strAction.Equals("parentdir")) wAction = ACTION_PARENT_DIR;
  else if (strAction.Equals("previousmenu")) wAction = ACTION_PREVIOUS_MENU;
  else if (strAction.Equals("info")) wAction = ACTION_SHOW_INFO;
  else if (strAction.Equals("pause")) wAction = ACTION_PAUSE;
  else if (strAction.Equals("stop")) wAction = ACTION_STOP;
  else if (strAction.Equals("skipnext")) wAction = ACTION_NEXT_ITEM;
  else if (strAction.Equals("skipprevious")) wAction = ACTION_PREV_ITEM;
//  else if (strAction.Equals("fastforward")) wAction = ACTION_FORWARD;
//  else if (strAction.Equals("rewind")) wAction = ACTION_REWIND;
  else if (strAction.Equals("fullscreen")) wAction = ACTION_SHOW_GUI;
  else if (strAction.Equals("aspectratio")) wAction = ACTION_ASPECT_RATIO;
  else if (strAction.Equals("stepforward")) wAction = ACTION_STEP_FORWARD;
  else if (strAction.Equals("stepback")) wAction = ACTION_STEP_BACK;
  else if (strAction.Equals("bigstepforward")) wAction = ACTION_BIG_STEP_FORWARD;
  else if (strAction.Equals("bigstepback")) wAction = ACTION_BIG_STEP_BACK;
  else if (strAction.Equals("osd")) wAction = ACTION_SHOW_OSD;

  else if (strAction.Equals("showsubtitles")) wAction = ACTION_SHOW_SUBTITLES;
  else if (strAction.Equals("nextsubtitle")) wAction = ACTION_NEXT_SUBTITLE;
  else if (strAction.Equals("codecinfo")) wAction = ACTION_SHOW_CODEC;
  else if (strAction.Equals("nextpicture")) wAction = ACTION_NEXT_PICTURE;
  else if (strAction.Equals("previouspicture")) wAction = ACTION_PREV_PICTURE;
  else if (strAction.Equals("zoomout")) wAction = ACTION_ZOOM_OUT;
  else if (strAction.Equals("zoomin")) wAction = ACTION_ZOOM_IN;
//  else if (strAction.Equals("togglesource")) wAction = ACTION_TOGGLE_SOURCE_DEST;
  else if (strAction.Equals("playlist")) wAction = ACTION_SHOW_PLAYLIST;
  else if (strAction.Equals("queue")) wAction = ACTION_QUEUE_ITEM;
//  else if (strAction.Equals("remove")) wAction = ACTION_REMOVE_ITEM;
//  else if (strAction.Equals("fullscreen")) wAction = ACTION_SHOW_FULLSCREEN;
  else if (strAction.Equals("zoomnormal")) wAction = ACTION_ZOOM_LEVEL_NORMAL;
  else if (strAction.Equals("zoomlevel1")) wAction = ACTION_ZOOM_LEVEL_1;
  else if (strAction.Equals("zoomlevel2")) wAction = ACTION_ZOOM_LEVEL_2;
  else if (strAction.Equals("zoomlevel3")) wAction = ACTION_ZOOM_LEVEL_3;
  else if (strAction.Equals("zoomlevel4")) wAction = ACTION_ZOOM_LEVEL_4;
  else if (strAction.Equals("zoomlevel5")) wAction = ACTION_ZOOM_LEVEL_5;
  else if (strAction.Equals("zoomlevel6")) wAction = ACTION_ZOOM_LEVEL_6;
  else if (strAction.Equals("zoomlevel7")) wAction = ACTION_ZOOM_LEVEL_7;
  else if (strAction.Equals("zoomlevel8")) wAction = ACTION_ZOOM_LEVEL_8;
  else if (strAction.Equals("zoomlevel9")) wAction = ACTION_ZOOM_LEVEL_9;

  else if (strAction.Equals("nextcalibration")) wAction = ACTION_CALIBRATE_SWAP_ARROWS;
  else if (strAction.Equals("resetcalibration")) wAction = ACTION_CALIBRATE_RESET;
  else if (strAction.Equals("analogmove")) wAction = ACTION_ANALOG_MOVE;
  else if (strAction.Equals("rotate")) wAction = ACTION_ROTATE_PICTURE;
  else if (strAction.Equals("close")) wAction = ACTION_CLOSE_DIALOG;
  else if (strAction.Equals("subtitledelayminus")) wAction = ACTION_SUBTITLE_DELAY_MIN;
  else if (strAction.Equals("subtitledelayplus")) wAction = ACTION_SUBTITLE_DELAY_PLUS;
  else if (strAction.Equals("audiodelayminus")) wAction = ACTION_AUDIO_DELAY_MIN;
  else if (strAction.Equals("audiodelayplus")) wAction = ACTION_AUDIO_DELAY_PLUS;
  else if (strAction.Equals("audionextlanguage")) wAction = ACTION_AUDIO_NEXT_LANGUAGE;
  else if (strAction.Equals("nextresolution")) wAction = ACTION_CHANGE_RESOLUTION;
  else if (strAction.Equals("audiotoggledigital")) wAction = ACTION_TOGGLE_DIGITAL_ANALOG;

  else if (strAction.Equals("number0")) wAction = REMOTE_0;
  else if (strAction.Equals("number1")) wAction = REMOTE_1;
  else if (strAction.Equals("number2")) wAction = REMOTE_2;
  else if (strAction.Equals("number3")) wAction = REMOTE_3;
  else if (strAction.Equals("number4")) wAction = REMOTE_4;
  else if (strAction.Equals("number5")) wAction = REMOTE_5;
  else if (strAction.Equals("number6")) wAction = REMOTE_6;
  else if (strAction.Equals("number7")) wAction = REMOTE_7;
  else if (strAction.Equals("number8")) wAction = REMOTE_8;
  else if (strAction.Equals("number9")) wAction = REMOTE_9;

//  else if (strAction.Equals("play")) wAction = ACTION_PLAY;
  else if (strAction.Equals("osdleft")) wAction = ACTION_OSD_SHOW_LEFT;
  else if (strAction.Equals("osdright")) wAction = ACTION_OSD_SHOW_RIGHT;
  else if (strAction.Equals("osdup")) wAction = ACTION_OSD_SHOW_UP;
  else if (strAction.Equals("osddown")) wAction = ACTION_OSD_SHOW_DOWN;
  else if (strAction.Equals("osdselect")) wAction = ACTION_OSD_SHOW_SELECT;
  else if (strAction.Equals("osdvalueplus")) wAction = ACTION_OSD_SHOW_VALUE_PLUS;
  else if (strAction.Equals("osdvalueminus")) wAction = ACTION_OSD_SHOW_VALUE_MIN;
  else if (strAction.Equals("smallstepback")) wAction = ACTION_SMALL_STEP_BACK;

  else if (strAction.Equals("fastforward")) wAction = ACTION_PLAYER_FORWARD;
  else if (strAction.Equals("rewind")) wAction = ACTION_PLAYER_REWIND;
  else if (strAction.Equals("play")) wAction = ACTION_PLAYER_PLAY;

  else if (strAction.Equals("delete")) wAction = ACTION_DELETE_ITEM;
  else if (strAction.Equals("copy")) wAction = ACTION_COPY_ITEM;
  else if (strAction.Equals("move")) wAction = ACTION_MOVE_ITEM;
  else if (strAction.Equals("mplayerosd")) wAction = ACTION_SHOW_MPLAYER_OSD;
  else if (strAction.Equals("hidesubmenu")) wAction = ACTION_OSD_HIDESUBMENU;
  else if (strAction.Equals("screenshot")) wAction = ACTION_TAKE_SCREENSHOT;
  else if (strAction.Equals("poweroff")) wAction = ACTION_POWERDOWN;
  else if (strAction.Equals("rename")) wAction = ACTION_RENAME_ITEM;
  else if (strAction.Equals("togglewatched")) wAction = ACTION_TOGGLE_WATCHED;
  else if (strAction.Equals("scanitem")) wAction = ACTION_SCAN_ITEM;

  else if (strAction.Equals("volumeup")) wAction = ACTION_VOLUME_UP;
  else if (strAction.Equals("volumedown")) wAction = ACTION_VOLUME_DOWN;
  else if (strAction.Equals("mute")) wAction = ACTION_MUTE;

  else if (strAction.Equals("backspace")) wAction = ACTION_BACKSPACE;
  else if (strAction.Equals("scrollup")) wAction = ACTION_SCROLL_UP;
  else if (strAction.Equals("scrolldown")) wAction = ACTION_SCROLL_DOWN;
  else if (strAction.Equals("analogfastforward")) wAction = ACTION_ANALOG_FORWARD;
  else if (strAction.Equals("analogrewind")) wAction = ACTION_ANALOG_REWIND;
  else if (strAction.Equals("moveitemup")) wAction = ACTION_MOVE_ITEM_UP;
  else if (strAction.Equals("moveitemdown")) wAction = ACTION_MOVE_ITEM_DOWN;
  else if (strAction.Equals("contextmenu")) wAction = ACTION_CONTEXT_MENU;

  else if (strAction.Equals("shift")) wAction = ACTION_SHIFT;
  else if (strAction.Equals("symbols")) wAction = ACTION_SYMBOLS;
  else if (strAction.Equals("cursorleft")) wAction = ACTION_CURSOR_LEFT;
  else if (strAction.Equals("cursorright")) wAction = ACTION_CURSOR_RIGHT;

  else if (strAction.Equals("showtime")) wAction = ACTION_SHOW_OSD_TIME;
  else if (strAction.Equals("analogseekforward")) wAction = ACTION_ANALOG_SEEK_FORWARD;
  else if (strAction.Equals("analogseekback")) wAction = ACTION_ANALOG_SEEK_BACK;

  else if (strAction.Equals("showpreset")) wAction = ACTION_VIS_PRESET_SHOW;
  else if (strAction.Equals("presetlist")) wAction = ACTION_VIS_PRESET_LIST;
  else if (strAction.Equals("nextpreset")) wAction = ACTION_VIS_PRESET_NEXT;
  else if (strAction.Equals("previouspreset")) wAction = ACTION_VIS_PRESET_PREV;
  else if (strAction.Equals("lockpreset")) wAction = ACTION_VIS_PRESET_LOCK;
  else if (strAction.Equals("randompreset")) wAction = ACTION_VIS_PRESET_RANDOM;
  else if (strAction.Equals("increasevisrating")) wAction = ACTION_VIS_RATE_PRESET_PLUS;
  else if (strAction.Equals("decreasevisrating")) wAction = ACTION_VIS_RATE_PRESET_MINUS;
  else if (strAction.Equals("showvideomenu")) wAction = ACTION_SHOW_VIDEOMENU;
  else if (strAction.Equals("enter")) wAction = ACTION_ENTER;
  else if (strAction.Equals("increaserating")) wAction = ACTION_INCREASE_RATING;
  else if (strAction.Equals("decreaserating")) wAction = ACTION_DECREASE_RATING;
  else if (strAction.Equals("togglefullscreen")) wAction = ACTION_TOGGLE_FULLSCREEN;
  else if (strAction.Equals("nextscene")) wAction = ACTION_NEXT_SCENE;
  else if (strAction.Equals("previousscene")) wAction = ACTION_PREV_SCENE;
  else if (strAction.Equals("nextletter")) wAction = ACTION_NEXT_LETTER;
  else if (strAction.Equals("prevletter")) wAction = ACTION_PREV_LETTER;
  // break if else block (MSVC 2k3 complains otherwise)
  if (strAction.Equals("jumpsms2")) wAction = ACTION_JUMP_SMS2;
  else if (strAction.Equals("jumpsms3")) wAction = ACTION_JUMP_SMS3;
  else if (strAction.Equals("jumpsms4")) wAction = ACTION_JUMP_SMS4;
  else if (strAction.Equals("jumpsms5")) wAction = ACTION_JUMP_SMS5;
  else if (strAction.Equals("jumpsms6")) wAction = ACTION_JUMP_SMS6;
  else if (strAction.Equals("jumpsms7")) wAction = ACTION_JUMP_SMS7;
  else if (strAction.Equals("jumpsms8")) wAction = ACTION_JUMP_SMS8;
  else if (strAction.Equals("jumpsms9")) wAction = ACTION_JUMP_SMS9;
  else if (strAction.Equals("filterclear")) wAction = ACTION_FILTER_CLEAR;
  else if (strAction.Equals("filtersms2")) wAction = ACTION_FILTER_SMS2;
  else if (strAction.Equals("filtersms3")) wAction = ACTION_FILTER_SMS3;
  else if (strAction.Equals("filtersms4")) wAction = ACTION_FILTER_SMS4;
  else if (strAction.Equals("filtersms5")) wAction = ACTION_FILTER_SMS5;
  else if (strAction.Equals("filtersms6")) wAction = ACTION_FILTER_SMS6;
  else if (strAction.Equals("filtersms7")) wAction = ACTION_FILTER_SMS7;
  else if (strAction.Equals("filtersms8")) wAction = ACTION_FILTER_SMS8;
  else if (strAction.Equals("filtersms9")) wAction = ACTION_FILTER_SMS9;
  else if (strAction.Equals("firstpage")) wAction = ACTION_FIRST_PAGE;
  else if (strAction.Equals("lastpage")) wAction = ACTION_LAST_PAGE;

  if (wAction == ACTION_NONE)
  {
    CLog::Log(LOGERROR, "Keymapping error: no such action '%s' defined", strAction.c_str());
    return false;
  }

  return true;
}

WORD CButtonTranslator::TranslateWindowString(const char *szWindow)
{
  WORD wWindowID = WINDOW_INVALID;
  CStdString strWindow = szWindow;
  if (strWindow.IsEmpty()) return wWindowID;
  strWindow.ToLower();
  // eliminate .xml
  if (strWindow.Mid(strWindow.GetLength() - 4) == ".xml" )
    strWindow = strWindow.Mid(0, strWindow.GetLength() - 4);

  // window12345, for custom window to be keymapped
  if (strWindow.length() > 6 && strWindow.Left(6).Equals("window"))
    strWindow = strWindow.Mid(6);
  if (strWindow.Left(2) == "my")  // drop "my" prefix
    strWindow = strWindow.Mid(2);
  if (StringUtils::IsNaturalNumber(strWindow))
  {
    // allow a full window id or a delta id
    int iWindow = atoi(strWindow.c_str());
    if (iWindow > WINDOW_INVALID)
      wWindowID = iWindow;
    else
      wWindowID = WINDOW_HOME + iWindow;
  }
  else if (strWindow.Equals("home")) wWindowID = WINDOW_HOME;
  else if (strWindow.Equals("programs")) wWindowID = WINDOW_PROGRAMS;
  else if (strWindow.Equals("pictures")) wWindowID = WINDOW_PICTURES;
  else if (strWindow.Equals("files") || strWindow.Equals("filemanager")) wWindowID = WINDOW_FILES;
  else if (strWindow.Equals("settings")) wWindowID = WINDOW_SETTINGS_MENU;
  else if (strWindow.Equals("music")) wWindowID = WINDOW_MUSIC;
  else if (strWindow.Equals("musicfiles")) wWindowID = WINDOW_MUSIC_FILES;
  else if (strWindow.Equals("musiclibrary")) wWindowID = WINDOW_MUSIC_NAV;
  else if (strWindow.Equals("musicplaylist")) wWindowID = WINDOW_MUSIC_PLAYLIST;
  else if (strWindow.Equals("musicplaylisteditor")) wWindowID = WINDOW_MUSIC_PLAYLIST_EDITOR;
  else if (strWindow.Equals("musicinformation")) wWindowID = WINDOW_MUSIC_INFO;
  else if (strWindow.Equals("video") || strWindow.Equals("videos")) wWindowID = WINDOW_VIDEOS;
  else if (strWindow.Equals("videofiles")) wWindowID = WINDOW_VIDEO_FILES;
  else if (strWindow.Equals("videolibrary")) wWindowID = WINDOW_VIDEO_NAV;
  else if (strWindow.Equals("videoplaylist")) wWindowID = WINDOW_VIDEO_PLAYLIST;
  else if (strWindow.Equals("systeminfo")) wWindowID = WINDOW_SYSTEM_INFORMATION;
  else if (strWindow.Equals("guicalibration")) wWindowID = WINDOW_SCREEN_CALIBRATION;
  else if (strWindow.Equals("screencalibration")) wWindowID = WINDOW_SCREEN_CALIBRATION;
  else if (strWindow.Equals("picturessettings")) wWindowID = WINDOW_SETTINGS_MYPICTURES;
  else if (strWindow.Equals("programssettings")) wWindowID = WINDOW_SETTINGS_MYPROGRAMS;
  else if (strWindow.Equals("weathersettings")) wWindowID = WINDOW_SETTINGS_MYWEATHER;
  else if (strWindow.Equals("musicsettings")) wWindowID = WINDOW_SETTINGS_MYMUSIC;
  else if (strWindow.Equals("systemsettings")) wWindowID = WINDOW_SETTINGS_SYSTEM;
  else if (strWindow.Equals("videossettings")) wWindowID = WINDOW_SETTINGS_MYVIDEOS;
  else if (strWindow.Equals("networksettings")) wWindowID = WINDOW_SETTINGS_NETWORK;
  else if (strWindow.Equals("appearancesettings")) wWindowID = WINDOW_SETTINGS_APPEARANCE;
  else if (strWindow.Equals("scripts")) wWindowID = WINDOW_SCRIPTS;
  else if (strWindow.Equals("gamesaves")) wWindowID = WINDOW_GAMESAVES;
  else if (strWindow.Equals("profiles")) wWindowID = WINDOW_SETTINGS_PROFILES;
  else if (strWindow.Equals("yesnodialog")) wWindowID = WINDOW_DIALOG_YES_NO;
  else if (strWindow.Equals("progressdialog")) wWindowID = WINDOW_DIALOG_PROGRESS;
  else if (strWindow.Equals("virtualkeyboard")) wWindowID = WINDOW_DIALOG_KEYBOARD;
  else if (strWindow.Equals("volumebar")) wWindowID = WINDOW_DIALOG_VOLUME_BAR;
  else if (strWindow.Equals("submenu")) wWindowID = WINDOW_DIALOG_SUB_MENU;
  else if (strWindow.Equals("favourites")) wWindowID = WINDOW_DIALOG_FAVOURITES;
  else if (strWindow.Equals("contextmenu")) wWindowID = WINDOW_DIALOG_CONTEXT_MENU;
  else if (strWindow.Equals("infodialog")) wWindowID = WINDOW_DIALOG_KAI_TOAST;
  else if (strWindow.Equals("numericinput")) wWindowID = WINDOW_DIALOG_NUMERIC;
  else if (strWindow.Equals("gamepadinput")) wWindowID = WINDOW_DIALOG_GAMEPAD;
  else if (strWindow.Equals("shutdownmenu")) wWindowID = WINDOW_DIALOG_BUTTON_MENU;
  else if (strWindow.Equals("scandialog")) wWindowID = WINDOW_DIALOG_MUSIC_SCAN;
  else if (strWindow.Equals("mutebug")) wWindowID = WINDOW_DIALOG_MUTE_BUG;
  else if (strWindow.Equals("playercontrols")) wWindowID = WINDOW_DIALOG_PLAYER_CONTROLS;
  else if (strWindow.Equals("seekbar")) wWindowID = WINDOW_DIALOG_SEEK_BAR;
  else if (strWindow.Equals("musicosd")) wWindowID = WINDOW_DIALOG_MUSIC_OSD;
  else if (strWindow.Equals("visualisationsettings")) wWindowID = WINDOW_DIALOG_VIS_SETTINGS;
  else if (strWindow.Equals("visualisationpresetlist")) wWindowID = WINDOW_DIALOG_VIS_PRESET_LIST;
  else if (strWindow.Equals("osdvideosettings")) wWindowID = WINDOW_DIALOG_VIDEO_OSD_SETTINGS;
  else if (strWindow.Equals("osdaudiosettings")) wWindowID = WINDOW_DIALOG_AUDIO_OSD_SETTINGS;
  else if (strWindow.Equals("videobookmarks")) wWindowID = WINDOW_DIALOG_VIDEO_BOOKMARKS;
  else if (strWindow.Equals("trainersettings")) wWindowID = WINDOW_DIALOG_TRAINER_SETTINGS;
  else if (strWindow.Equals("profilesettings")) wWindowID = WINDOW_DIALOG_PROFILE_SETTINGS;
  else if (strWindow.Equals("locksettings")) wWindowID = WINDOW_DIALOG_LOCK_SETTINGS;
  else if (strWindow.Equals("contentsettings")) wWindowID = WINDOW_DIALOG_CONTENT_SETTINGS;
  else if (strWindow.Equals("networksetup")) wWindowID = WINDOW_DIALOG_NETWORK_SETUP;
  else if (strWindow.Equals("mediasource")) wWindowID = WINDOW_DIALOG_MEDIA_SOURCE;
  else if (strWindow.Equals("smartplaylisteditor")) wWindowID = WINDOW_DIALOG_SMART_PLAYLIST_EDITOR;
  else if (strWindow.Equals("smartplaylistrule")) wWindowID = WINDOW_DIALOG_SMART_PLAYLIST_RULE;
  else if (strWindow.Equals("selectdialog")) wWindowID = WINDOW_DIALOG_SELECT;
  else if (strWindow.Equals("okdialog")) wWindowID = WINDOW_DIALOG_OK;
  else if (strWindow.Equals("movieinformation")) wWindowID = WINDOW_VIDEO_INFO;
  else if (strWindow.Equals("scriptsdebuginfo")) wWindowID = WINDOW_SCRIPTS_INFO;
  else if (strWindow.Equals("fullscreenvideo")) wWindowID = WINDOW_FULLSCREEN_VIDEO;
  else if (strWindow.Equals("visualisation")) wWindowID = WINDOW_VISUALISATION;
  else if (strWindow.Equals("slideshow")) wWindowID = WINDOW_SLIDESHOW;
  else if (strWindow.Equals("filestackingdialog")) wWindowID = WINDOW_DIALOG_FILESTACKING;
  else if (strWindow.Equals("weather")) wWindowID = WINDOW_WEATHER;
  else if (strWindow.Equals("screensaver")) wWindowID = WINDOW_SCREENSAVER;
  else if (strWindow.Equals("videoosd")) wWindowID = WINDOW_OSD;
  else if (strWindow.Equals("videomenu")) wWindowID = WINDOW_VIDEO_MENU;
  else if (strWindow.Equals("filebrowser")) wWindowID = WINDOW_DIALOG_FILE_BROWSER;
  else if (strWindow.Equals("startup")) wWindowID = WINDOW_STARTUP;
  else if (strWindow.Equals("startwindow")) wWindowID = g_SkinInfo.GetStartWindow();
  else if (strWindow.Equals("loginscreen")) wWindowID = WINDOW_LOGIN_SCREEN;
  else if (strWindow.Equals("musicoverlay")) wWindowID = WINDOW_MUSIC_OVERLAY;
  else if (strWindow.Equals("videooverlay")) wWindowID = WINDOW_VIDEO_OVERLAY;
  else if (strWindow.Equals("pictureinfo")) wWindowID = WINDOW_DIALOG_PICTURE_INFO;
  else if (strWindow.Equals("pluginsettings")) wWindowID = WINDOW_DIALOG_PLUGIN_SETTINGS;
  else if (strWindow.Equals("fullscreeninfo")) wWindowID = WINDOW_DIALOG_FULLSCREEN_INFO;
  else
    CLog::Log(LOGERROR, "Window Translator: Can't find window %s", strWindow.c_str());

  //CLog::Log(LOGDEBUG,"CButtonTranslator::TranslateWindowString(%s) returned Window ID (%i)", szWindow, wWindowID);
  return wWindowID;
}

WORD CButtonTranslator::TranslateGamepadString(const char *szButton)
{
  if (!szButton) return 0;
  WORD wButtonCode = 0;
  CStdString strButton = szButton;
  strButton.ToLower();
  if (strButton.Equals("a")) wButtonCode = KEY_BUTTON_A;
  else if (strButton.Equals("b")) wButtonCode = KEY_BUTTON_B;
  else if (strButton.Equals("x")) wButtonCode = KEY_BUTTON_X;
  else if (strButton.Equals("y")) wButtonCode = KEY_BUTTON_Y;
  else if (strButton.Equals("white")) wButtonCode = KEY_BUTTON_WHITE;
  else if (strButton.Equals("black")) wButtonCode = KEY_BUTTON_BLACK;
  else if (strButton.Equals("start")) wButtonCode = KEY_BUTTON_START;
  else if (strButton.Equals("back")) wButtonCode = KEY_BUTTON_BACK;
  else if (strButton.Equals("leftthumbbutton")) wButtonCode = KEY_BUTTON_LEFT_THUMB_BUTTON;
  else if (strButton.Equals("rightthumbbutton")) wButtonCode = KEY_BUTTON_RIGHT_THUMB_BUTTON;
  else if (strButton.Equals("leftthumbstick")) wButtonCode = KEY_BUTTON_LEFT_THUMB_STICK;
  else if (strButton.Equals("leftthumbstickup")) wButtonCode = KEY_BUTTON_LEFT_THUMB_STICK_UP;
  else if (strButton.Equals("leftthumbstickdown")) wButtonCode = KEY_BUTTON_LEFT_THUMB_STICK_DOWN;
  else if (strButton.Equals("leftthumbstickleft")) wButtonCode = KEY_BUTTON_LEFT_THUMB_STICK_LEFT;
  else if (strButton.Equals("leftthumbstickright")) wButtonCode = KEY_BUTTON_LEFT_THUMB_STICK_RIGHT;
  else if (strButton.Equals("rightthumbstick")) wButtonCode = KEY_BUTTON_RIGHT_THUMB_STICK;
  else if (strButton.Equals("rightthumbstickup")) wButtonCode = KEY_BUTTON_RIGHT_THUMB_STICK_UP;
  else if (strButton.Equals("rightthumbstickdown")) wButtonCode = KEY_BUTTON_RIGHT_THUMB_STICK_DOWN;
  else if (strButton.Equals("rightthumbstickleft")) wButtonCode = KEY_BUTTON_RIGHT_THUMB_STICK_LEFT;
  else if (strButton.Equals("rightthumbstickright")) wButtonCode = KEY_BUTTON_RIGHT_THUMB_STICK_RIGHT;
  else if (strButton.Equals("lefttrigger")) wButtonCode = KEY_BUTTON_LEFT_TRIGGER;
  else if (strButton.Equals("righttrigger")) wButtonCode = KEY_BUTTON_RIGHT_TRIGGER;
  else if (strButton.Equals("leftanalogtrigger")) wButtonCode = KEY_BUTTON_LEFT_ANALOG_TRIGGER;
  else if (strButton.Equals("rightanalogtrigger")) wButtonCode = KEY_BUTTON_RIGHT_ANALOG_TRIGGER;
  else if (strButton.Equals("dpadleft")) wButtonCode = KEY_BUTTON_DPAD_LEFT;
  else if (strButton.Equals("dpadright")) wButtonCode = KEY_BUTTON_DPAD_RIGHT;
  else if (strButton.Equals("dpadup")) wButtonCode = KEY_BUTTON_DPAD_UP;
  else if (strButton.Equals("dpaddown")) wButtonCode = KEY_BUTTON_DPAD_DOWN;
  else CLog::Log(LOGERROR, "Gamepad Translator: Can't find button %s", strButton.c_str());
  return wButtonCode;
}

WORD CButtonTranslator::TranslateRemoteString(const char *szButton)
{
  if (!szButton) return 0;
  WORD wButtonCode = 0;
  CStdString strButton = szButton;
  strButton.ToLower();
  if (strButton.Equals("left")) wButtonCode = XINPUT_IR_REMOTE_LEFT;
  else if (strButton.Equals("right")) wButtonCode = XINPUT_IR_REMOTE_RIGHT;
  else if (strButton.Equals("up")) wButtonCode = XINPUT_IR_REMOTE_UP;
  else if (strButton.Equals("down")) wButtonCode = XINPUT_IR_REMOTE_DOWN;
  else if (strButton.Equals("select")) wButtonCode = XINPUT_IR_REMOTE_SELECT;
  else if (strButton.Equals("back")) wButtonCode = XINPUT_IR_REMOTE_BACK;
  else if (strButton.Equals("menu")) wButtonCode = XINPUT_IR_REMOTE_MENU;
  else if (strButton.Equals("info")) wButtonCode = XINPUT_IR_REMOTE_INFO;
  else if (strButton.Equals("display")) wButtonCode = XINPUT_IR_REMOTE_DISPLAY;
  else if (strButton.Equals("title")) wButtonCode = XINPUT_IR_REMOTE_TITLE;
  else if (strButton.Equals("play")) wButtonCode = XINPUT_IR_REMOTE_PLAY;
  else if (strButton.Equals("pause")) wButtonCode = XINPUT_IR_REMOTE_PAUSE;
  else if (strButton.Equals("reverse")) wButtonCode = XINPUT_IR_REMOTE_REVERSE;
  else if (strButton.Equals("forward")) wButtonCode = XINPUT_IR_REMOTE_FORWARD;
  else if (strButton.Equals("skipplus")) wButtonCode = XINPUT_IR_REMOTE_SKIP_PLUS;
  else if (strButton.Equals("skipminus")) wButtonCode = XINPUT_IR_REMOTE_SKIP_MINUS;
  else if (strButton.Equals("stop")) wButtonCode = XINPUT_IR_REMOTE_STOP;
  else if (strButton.Equals("zero")) wButtonCode = XINPUT_IR_REMOTE_0;
  else if (strButton.Equals("one")) wButtonCode = XINPUT_IR_REMOTE_1;
  else if (strButton.Equals("two")) wButtonCode = XINPUT_IR_REMOTE_2;
  else if (strButton.Equals("three")) wButtonCode = XINPUT_IR_REMOTE_3;
  else if (strButton.Equals("four")) wButtonCode = XINPUT_IR_REMOTE_4;
  else if (strButton.Equals("five")) wButtonCode = XINPUT_IR_REMOTE_5;
  else if (strButton.Equals("six")) wButtonCode = XINPUT_IR_REMOTE_6;
  else if (strButton.Equals("seven")) wButtonCode = XINPUT_IR_REMOTE_7;
  else if (strButton.Equals("eight")) wButtonCode = XINPUT_IR_REMOTE_8;
  else if (strButton.Equals("nine")) wButtonCode = XINPUT_IR_REMOTE_9;
  // additional keys from the media center extender for xbox remote
  else if (strButton.Equals("power")) wButtonCode = XINPUT_IR_REMOTE_POWER;
  else if (strButton.Equals("mytv")) wButtonCode = XINPUT_IR_REMOTE_MY_TV;
  else if (strButton.Equals("mymusic")) wButtonCode = XINPUT_IR_REMOTE_MY_MUSIC;
  else if (strButton.Equals("mypictures")) wButtonCode = XINPUT_IR_REMOTE_MY_PICTURES;
  else if (strButton.Equals("myvideo")) wButtonCode = XINPUT_IR_REMOTE_MY_VIDEOS;
  else if (strButton.Equals("record")) wButtonCode = XINPUT_IR_REMOTE_RECORD;
  else if (strButton.Equals("start")) wButtonCode = XINPUT_IR_REMOTE_START;
  else if (strButton.Equals("volumeplus")) wButtonCode = XINPUT_IR_REMOTE_VOLUME_PLUS;
  else if (strButton.Equals("volumeminus")) wButtonCode = XINPUT_IR_REMOTE_VOLUME_MINUS;
  else if (strButton.Equals("channelplus")) wButtonCode = XINPUT_IR_REMOTE_CHANNEL_PLUS;
  else if (strButton.Equals("channelminus")) wButtonCode = XINPUT_IR_REMOTE_CHANNEL_MINUS;
  else if (strButton.Equals("pageplus")) wButtonCode = XINPUT_IR_REMOTE_CHANNEL_PLUS;
  else if (strButton.Equals("pageminus")) wButtonCode = XINPUT_IR_REMOTE_CHANNEL_MINUS;
  else if (strButton.Equals("mute")) wButtonCode = XINPUT_IR_REMOTE_MUTE;
  else if (strButton.Equals("recordedtv")) wButtonCode = XINPUT_IR_REMOTE_RECORDED_TV;
  else if (strButton.Equals("guide")) wButtonCode = XINPUT_IR_REMOTE_TITLE;   // same as title
  else if (strButton.Equals("livetv")) wButtonCode = XINPUT_IR_REMOTE_LIVE_TV;
  else if (strButton.Equals("star")) wButtonCode = XINPUT_IR_REMOTE_STAR;
  else if (strButton.Equals("hash")) wButtonCode = XINPUT_IR_REMOTE_HASH;
  else if (strButton.Equals("clear")) wButtonCode = XINPUT_IR_REMOTE_CLEAR;
  else if (strButton.Equals("enter")) wButtonCode = XINPUT_IR_REMOTE_SELECT;  // same as select
  else if (strButton.Equals("xbox")) wButtonCode = XINPUT_IR_REMOTE_DISPLAY; // same as display
  else CLog::Log(LOGERROR, "Remote Translator: Can't find button %s", strButton.c_str());
  return wButtonCode;
}

WORD CButtonTranslator::TranslateUniversalRemoteString(const char *szButton)
{
  if (!szButton || strlen(szButton) < 4 || strnicmp(szButton, "obc", 3)) return 0;
  const char *szCode = szButton + 3;
  // Button Code is 255 - OBC (Original Button Code) of the button
  WORD wButtonCode = 255 - (WORD)atol(szCode);
  if (wButtonCode > 255) wButtonCode = 0;
  return wButtonCode;
}

WORD CButtonTranslator::TranslateKeyboardString(const char *szButton)
{
  WORD wButtonCode = 0;
  if (strlen(szButton) == 1)
  { // single character
    wButtonCode = (WORD)toupper(szButton[0]) | KEY_VKEY;
    // FIXME It is a printable character, printable should be ASCII not VKEY! Till now it works, but how (long)?
    // FIXME support unicode: additional parameter necessary since unicode can not be embedded into key/action-ID.
  }
  else
  { // for keys such as return etc. etc.
    CStdString strKey = szButton;
    strKey.ToLower();
    if (strKey.Equals("return")) wButtonCode = 0xF00D;
    else if (strKey.Equals("enter")) wButtonCode = 0xF06C;
    else if (strKey.Equals("escape")) wButtonCode = 0xF01B;
    else if (strKey.Equals("esc")) wButtonCode = 0xF01B;
    else if (strKey.Equals("tab")) wButtonCode = 0xF009;
    else if (strKey.Equals("space")) wButtonCode = 0xF020;
    else if (strKey.Equals("left")) wButtonCode = 0xF025;
    else if (strKey.Equals("right")) wButtonCode = 0xF027;
    else if (strKey.Equals("up")) wButtonCode = 0xF026;
    else if (strKey.Equals("down")) wButtonCode = 0xF028;
    else if (strKey.Equals("insert")) wButtonCode = 0xF02D;
    else if (strKey.Equals("delete")) wButtonCode = 0xF02E;
    else if (strKey.Equals("home")) wButtonCode = 0xF024;
    else if (strKey.Equals("end")) wButtonCode = 0xF023;
    else if (strKey.Equals("f1")) wButtonCode = 0xF070;
    else if (strKey.Equals("f2")) wButtonCode = 0xF071;
    else if (strKey.Equals("f3")) wButtonCode = 0xF072;
    else if (strKey.Equals("f4")) wButtonCode = 0xF073;
    else if (strKey.Equals("f5")) wButtonCode = 0xF074;
    else if (strKey.Equals("f6")) wButtonCode = 0xF075;
    else if (strKey.Equals("f7")) wButtonCode = 0xF076;
    else if (strKey.Equals("f8")) wButtonCode = 0xF077;
    else if (strKey.Equals("f9")) wButtonCode = 0xF078;
    else if (strKey.Equals("f10")) wButtonCode = 0xF079;
    else if (strKey.Equals("f11")) wButtonCode = 0xF07A;
    else if (strKey.Equals("f12")) wButtonCode = 0xF07B;
    else if (strKey.Equals("numpadzero") || strKey.Equals("zero")) wButtonCode = 0xF060;
    else if (strKey.Equals("numpadone") || strKey.Equals("one")) wButtonCode = 0xF061;
    else if (strKey.Equals("numpadtwo") || strKey.Equals("two")) wButtonCode = 0xF062;
    else if (strKey.Equals("numpadthree") || strKey.Equals("three")) wButtonCode = 0xF063;
    else if (strKey.Equals("numpadfour") || strKey.Equals("four")) wButtonCode = 0xF064;
    else if (strKey.Equals("numpadfive") || strKey.Equals("five")) wButtonCode = 0xF065;
    else if (strKey.Equals("numpadsix") || strKey.Equals("six")) wButtonCode = 0xF066;
    else if (strKey.Equals("numpadseven") || strKey.Equals("seven")) wButtonCode = 0xF067;
    else if (strKey.Equals("numpadeight") || strKey.Equals("eight")) wButtonCode = 0xF068;
    else if (strKey.Equals("numpadnine") || strKey.Equals("nine")) wButtonCode = 0xF069;
    else if (strKey.Equals("numpadtimes")) wButtonCode = 0xF06A;
    else if (strKey.Equals("numpadplus")) wButtonCode = 0xF06B;
    else if (strKey.Equals("numpadminus")) wButtonCode = 0xF06D;
    else if (strKey.Equals("numpadperiod")) wButtonCode = 0xF06E;
    else if (strKey.Equals("numpaddivide")) wButtonCode = 0xF06F;
    else if (strKey.Equals("pageup")) wButtonCode = 0xF021;
    else if (strKey.Equals("pagedown")) wButtonCode = 0xF022;
    else if (strKey.Equals("printscreen")) wButtonCode = 0xF02A;
    else if (strKey.Equals("backspace")) wButtonCode = 0xF008;
    else if (strKey.Equals("menu")) wButtonCode = 0xF05D;
    else if (strKey.Equals("pause")) wButtonCode = 0xF013;
    else if (strKey.Equals("leftshift")) wButtonCode = 0xF0A0;
    else if (strKey.Equals("rightshift")) wButtonCode = 0xF0A1;
    else if (strKey.Equals("leftctrl")) wButtonCode = 0xF0A2;
    else if (strKey.Equals("rightctrl")) wButtonCode = 0xF0A3;
    else if (strKey.Equals("leftalt")) wButtonCode = 0xF0A4;
    else if (strKey.Equals("rightalt")) wButtonCode = 0xF0A5;
    else if (strKey.Equals("leftwindows")) wButtonCode = 0xF05B;
    else if (strKey.Equals("rightwindows")) wButtonCode = 0xF05C;
    else if (strKey.Equals("capslock")) wButtonCode = 0xF020;
    else if (strKey.Equals("numlock")) wButtonCode = 0xF090;
    else if (strKey.Equals("scrolllock")) wButtonCode = 0xF091;
    else if (strKey.Equals("semicolon") || strKey.Equals("colon")) wButtonCode = 0xF0BA;
    else if (strKey.Equals("equals") || strKey.Equals("plus")) wButtonCode = 0xF0BB;
    else if (strKey.Equals("comma") || strKey.Equals("lessthan")) wButtonCode = 0xF0BC;
    else if (strKey.Equals("minus") || strKey.Equals("underline")) wButtonCode = 0xF0BD;
    else if (strKey.Equals("period") || strKey.Equals("greaterthan")) wButtonCode = 0xF0BE;
    else if (strKey.Equals("forwardslash") || strKey.Equals("questionmark")) wButtonCode = 0xF0BF;
    else if (strKey.Equals("leftquote") || strKey.Equals("tilde")) wButtonCode = 0xF0C0;
    else if (strKey.Equals("opensquarebracket") || strKey.Equals("openbrace")) wButtonCode = 0xF0EB;
    else if (strKey.Equals("backslash") || strKey.Equals("pipe")) wButtonCode = 0xF0EC;
    else if (strKey.Equals("closesquarebracket") || strKey.Equals("closebrace")) wButtonCode = 0xF0ED;
    else if (strKey.Equals("quote") || strKey.Equals("doublequote")) wButtonCode = 0xF0EE;
    else CLog::Log(LOGERROR, "Keyboard Translator: Can't find button %s", strKey.c_str());
  }
  return wButtonCode;
}

WORD CButtonTranslator::TranslateKeyboardButton(TiXmlElement *pButton)
{
  const char *szButton = pButton->Value();

  if (!szButton) return 0;
  CStdString strKey = szButton;
  if (strKey.Equals("key"))
  {
    int id = 0;
    if (pButton->QueryIntAttribute("id", &id) == TIXML_SUCCESS)
      return id;
    else
      CLog::Log(LOGERROR, "Keyboard Translator: `key' button has no id");
  }
  else
  {
    return TranslateKeyboardString(szButton);
  }
  return 0;
}

void CButtonTranslator::Clear()
{
  translatorMap.clear();

}

