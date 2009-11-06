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

/*
 * DESCRIPTION:
 *
 */

#include "FileItem.h"
#include "PVREpg.h"
#include "PVRChannels.h"
#include "GUISettings.h"
#include "TVDatabase.h"
#include "PVRManager.h"
#include "GUIWindowManager.h"
#include "GUIDialogYesNo.h"
#include "GUIDialogOK.h"
#include "LocalizeStrings.h"
#include "utils/log.h"
#include "FileSystem/File.h"

using namespace XFILE;

/**
 * Create a blank unmodified channel tag
 */
cPVRChannelInfoTag::cPVRChannelInfoTag()
{
  Reset();
}

bool cPVRChannelInfoTag::operator==(const cPVRChannelInfoTag& right) const
{
  if (this == &right) return true;

  return (m_iIdChannel            == right.m_iIdChannel &&
          m_iChannelNum           == right.m_iChannelNum &&
          m_iClientNum            == right.m_iClientNum &&
          m_strChannel            == right.m_strChannel &&
          m_IconPath              == right.m_IconPath &&
          m_encrypted             == right.m_encrypted &&
          m_radio                 == right.m_radio &&
          m_hide                  == right.m_hide &&
          m_isRecording           == right.m_isRecording &&
          m_strFileNameAndPath    == right.m_strFileNameAndPath);
}

bool cPVRChannelInfoTag::operator!=(const cPVRChannelInfoTag &right) const
{
  if (m_iIdChannel            != right.m_iIdChannel) return true;
  if (m_iChannelNum           != right.m_iChannelNum) return true;
  if (m_iClientNum            != right.m_iClientNum) return true;
  if (m_strChannel            != right.m_strChannel) return true;
  if (m_IconPath              != right.m_IconPath) return true;
  if (m_encrypted             != right.m_encrypted) return true;
  if (m_radio                 != right.m_radio) return true;
  if (m_hide                  != right.m_hide) return true;
  if (m_isRecording           != right.m_isRecording) return true;
  if (m_strFileNameAndPath    != right.m_strFileNameAndPath) return true;

  return false;
}

/**
 * Initialize blank cPVRChannelInfoTag
 */
void cPVRChannelInfoTag::Reset()
{
  m_iIdChannel            = -1;
  m_iChannelNum           = -1;
  m_iClientNum            = -1;
  m_iGroupID              = 0;
  m_strChannel            = "";
  m_strClientName         = "";
  m_IconPath              = "";
  m_radio                 = false;
  m_encrypted             = false;
  m_hide                  = false;
  m_isRecording           = false;
  m_startTime             = NULL;
  m_endTime               = NULL;
  m_strFileNameAndPath    = "";
  m_strNextTitle          = "";
  m_clientID              = -1;
  m_Epg                   = NULL;

  CVideoInfoTag::Reset();
}

int cPVRChannelInfoTag::GetDuration() const
{
  time_t start, end;
  m_startTime.GetAsTime(start);
  m_endTime.GetAsTime(end);
  return end - start;
}

int cPVRChannelInfoTag::GetTime() const
{
  CDateTimeSpan time = CDateTime::GetCurrentDateTime() - StartTime();
  return time.GetDays()    * 60 * 60 * 24
       + time.GetHours()   * 60 * 60
       + time.GetMinutes() * 60
       + time.GetSeconds();
}


// --- cPVRChannels ---------------------------------------------------------------

cPVRChannels PVRChannelsTV;
cPVRChannels PVRChannelsRadio;

cPVRChannels::cPVRChannels(void)
{
  m_bRadio = false;
  m_iHiddenChannels = 0;
}

bool cPVRChannels::Load(bool radio)
{
  m_bRadio = radio;
  CTVDatabase *database = g_PVRManager.GetTVDatabase();
  CLIENTMAP   *clients  = g_PVRManager.Clients();

  Clear();

  database->Open();

  if (database->GetDBNumChannels(m_bRadio) > 0)
  {
    database->GetDBChannelList(*this, m_bRadio);
    database->Close();
    Update();
  }
  else
  {
    CLog::Log(LOGNOTICE, "cPVRChannels: TV Database holds no %s channels, reading channels from clients", m_bRadio ? "Radio" : "TV");

    CLIENTMAPITR itr = clients->begin();
    while (itr != clients->end())
    {
      IPVRClient* client = (*itr).second;
      if (client->GetNumChannels() > 0)
      {
        client->GetChannelList(*this, m_bRadio);
      }
      itr++;
    }

    ReNumberAndCheck();
    SearchAndSetChannelIcons();

    for (unsigned int i = 0; i < size(); i++)
      at(i).SetChannelID(database->AddDBChannel(at(i)));

    database->Compress(true);
    database->Close();
  }

  return false;
}

bool cPVRChannels::Update()
{
  CTVDatabase *database = g_PVRManager.GetTVDatabase();
  CLIENTMAP   *clients  = g_PVRManager.Clients();
  cPVRChannels PVRChannels_tmp;

  database->Open();

  CLIENTMAPITR itr = clients->begin();
  while (itr != clients->end())
  {
    IPVRClient* client = (*itr).second;
    if (client->GetNumChannels() > 0)
    {
      client->GetChannelList(PVRChannels_tmp, m_bRadio);
    }
    itr++;
  }

  PVRChannels_tmp.ReNumberAndCheck();

  /*
   * First whe look for moved channels on backend (other backend number)
   * and delete no more present channels inside database.
   * Problem:
   * If a channel on client is renamed, it is deleted from Database
   * and later added as new channel and loose his Group Information
   */
  for (unsigned int i = 0; i < size(); i++)
  {
    bool found = false;
    bool changed = false;

    for (unsigned int j = 0; j < PVRChannels_tmp.size(); j++)
    {
      if (at(i).UniqueID() == PVRChannels_tmp[j].UniqueID() &&
          at(i).ClientID() == PVRChannels_tmp[j].ClientID())
      {
        if (at(i).ClientNumber() != PVRChannels_tmp[j].ClientNumber())
        {
          at(i).SetClientNumber(PVRChannels_tmp[j].ClientNumber());
          changed = true;
        }

        if (at(i).ClientName() != PVRChannels_tmp[j].ClientName())
        {
          at(i).SetClientName(PVRChannels_tmp[j].ClientName());
          at(i).SetName(PVRChannels_tmp[j].ClientName());
          changed = true;
        }

        found = true;
        PVRChannels_tmp.erase(PVRChannels_tmp.begin()+j);
        break;
      }
    }

    if (changed)
    {
      database->UpdateDBChannel(at(i));
      CLog::Log(LOGINFO,"PVRManager: Updated %s channel %s", m_bRadio?"Radio":"TV", at(i).Name().c_str());
    }

    if (!found)
    {
      CLog::Log(LOGINFO,"PVRManager: Removing %s channel %s (no more present)", m_bRadio?"Radio":"TV", at(i).Name().c_str());
      database->RemoveDBChannel(at(i));
      erase(begin()+i);
      i--;
    }
  }

  /*
   * Now whe add new channels to frontend
   * All entries now present in the temp lists, are new entries
   */
  for (unsigned int i = 0; i < PVRChannels_tmp.size(); i++)
  {
    PVRChannels_tmp[i].SetChannelID(database->AddDBChannel(PVRChannels_tmp[i]));
    push_back(PVRChannels_tmp[i]);
    CLog::Log(LOGINFO,"PVRManager: Added %s channel %s", m_bRadio?"Radio":"TV", PVRChannels_tmp[i].Name().c_str());
  }

  database->Close();

  m_iHiddenChannels = 0;
  for (unsigned int i = 0; i < size(); i++)
  {
    if (at(i).IsHidden())
      m_iHiddenChannels++;



  }

  return false;
}

void cPVRChannels::SearchAndSetChannelIcons(bool writeDB)
{
  CTVDatabase *database = g_PVRManager.GetTVDatabase();
  database->Open();

  for (unsigned int i = 0; i < size(); i++)
  {
    CStdString iconpath;

    /* If the Icon is already set continue with next channel */
    if (at(i).Icon() != "")
      continue;

    if (g_guiSettings.GetString("pvrmenu.iconpath") != "")
    {
      /* Search icon by channel name */
      iconpath = g_guiSettings.GetString("pvrmenu.iconpath") + at(i).ClientName();
      if (CFile::Exists(iconpath + ".tbn"))
      {
        at(i).SetIcon(iconpath + ".tbn");
        if (writeDB) database->UpdateDBChannel(at(i));
        continue;
      }
      else if (CFile::Exists(iconpath + ".jpg"))
      {
        at(i).SetIcon(iconpath + ".jpg");
        if (writeDB) database->UpdateDBChannel(at(i));
        continue;
      }
      else if (CFile::Exists(iconpath + ".png"))
      {
        at(i).SetIcon(iconpath + ".png");
        if (writeDB) database->UpdateDBChannel(at(i));
        continue;
      }

      /* Search icon by channel name in lower case */
      iconpath = g_guiSettings.GetString("pvrmenu.iconpath") + at(i).ClientName().ToLower();
      if (CFile::Exists(iconpath + ".tbn"))
      {
        at(i).SetIcon(iconpath + ".tbn");
        if (writeDB) database->UpdateDBChannel(at(i));
        continue;
      }
      else if (CFile::Exists(iconpath + ".jpg"))
      {
        at(i).SetIcon(iconpath + ".jpg");
        if (writeDB) database->UpdateDBChannel(at(i));
        continue;
      }
      else if (CFile::Exists(iconpath + ".png"))
      {
        at(i).SetIcon(iconpath + ".png");
        if (writeDB) database->UpdateDBChannel(at(i));
        continue;
      }

      /* Search Icon by Unique Id */
      iconpath.Format("%s/%08d",g_guiSettings.GetString("pvrmenu.iconpath"), at(i).UniqueID());
      if (CFile::Exists(iconpath + ".tbn"))
      {
        at(i).SetIcon(iconpath + ".tbn");
        if (writeDB) database->UpdateDBChannel(at(i));
        continue;
      }
      else if (CFile::Exists(iconpath + ".jpg"))
      {
        at(i).SetIcon(iconpath + ".jpg");
        if (writeDB) database->UpdateDBChannel(at(i));
        continue;
      }
      else if (CFile::Exists(iconpath + ".png"))
      {
        at(i).SetIcon(iconpath + ".png");
        if (writeDB) database->UpdateDBChannel(at(i));
        continue;
      }
    }

    /* Start channel icon scraper here if nothing was found*/
    /// TODO


    CLog::Log(LOGNOTICE,"PVRManager: No channel icon found for %s, use '%s' or '%08d' with extension 'tbn', 'jpg' or 'png'", at(i).Name().c_str(), at(i).ClientName().c_str(), at(i).UniqueID());
  }

  database->Close();
}

void cPVRChannels::ReNumberAndCheck(void)
{
  int Number = 1;
  m_iHiddenChannels = 0;
  for (unsigned int i = 0; i < size(); i++)
  {
    if (at(i).ClientNumber() <= 0)
    {
      CLog::Log(LOGERROR, "cPVRChannels: Channel '%s' from client '%i' is invalid, removing from list", at(i).Name().c_str(), at(i).ClientID());
      erase(begin()+i);
      i--;
      break;
    }

    if (at(i).UniqueID() <= 0)
      CLog::Log(LOGNOTICE, "cPVRChannels: Channel '%s' from client '%i' have no unique ID. Contact PVR Client developer.", at(i).Name().c_str(), at(i).ClientID());

    if (at(i).Name().IsEmpty())
    {
      CStdString name;
      CLog::Log(LOGERROR, "cPVRChannels: Client channel '%i' from client '%i' have no channel name", at(i).ClientNumber(), at(i).ClientID());
      name.Format(g_localizeStrings.Get(18029), at(i).ClientNumber());
      at(i).SetName(name);
    }

    if (at(i).IsHidden())
      m_iHiddenChannels++;

    CStdString path;
    at(i).SetNumber(Number);

    if (!m_bRadio)
      path.Format("pvr://channelstv/%i.ts", Number);
    else
      path.Format("pvr://channelsradio/%i.ts", Number);

    at(i).SetPath(path);
    at(i).m_strStatus = "livetv";
    Number++;
  }
}

int cPVRChannels::GetChannels(CFileItemList* results, int group_id)
{
  int cnt = 0;

  for (unsigned int i = 0; i < size(); i++)
  {
    if (at(i).IsHidden())
      continue;

    if ((group_id != -1) && (at(i).GroupID() != group_id))
      continue;

    CFileItemPtr channel(new CFileItem(at(i)));
    g_PVRManager.SetCurrentPlayingProgram(*channel);

    results->Add(channel);
    cnt++;
  }
  return cnt;
}

int cPVRChannels::GetHiddenChannels(CFileItemList* results)
{
  int cnt = 0;

  for (unsigned int i = 0; i < size(); i++)
  {
    if (!at(i).IsHidden())
      continue;

    CFileItemPtr channel(new CFileItem(at(i)));
    results->Add(channel);
    cnt++;
  }
  return cnt;
}

void cPVRChannels::MoveChannel(unsigned int oldindex, unsigned int newindex)
{
  cPVRChannels m_channels_temp;

  if ((newindex == oldindex) || (newindex == 0))
    return;

  CTVDatabase *database = g_PVRManager.GetTVDatabase();
  database->Open();

  m_channels_temp.push_back(at(oldindex-1));
  erase(begin()+oldindex-1);
  if (newindex < size())
    insert(begin()+newindex-1, m_channels_temp[0]);
  else
    push_back(m_channels_temp[0]);

  for (unsigned int i = 0; i < size(); i++)
  {
    if (at(i).Number() != i+1)
    {
      CStdString path;
      at(i).SetNumber(i+1);

      if (!m_bRadio)
        path.Format("pvr://channelstv/%i.ts", at(i).Number());
      else
        path.Format("pvr://channelsradio/%i.ts", at(i).Number());

      at(i).SetPath(path);
      database->UpdateDBChannel(at(i));
    }
  }

  CLog::Log(LOGNOTICE, "cPVRChannels: TV Channel %d moved to %d", oldindex, newindex);
  database->Close();

  /* Synchronize channel numbers inside timers */
  for (unsigned int i = 0; i < PVRTimers.size(); i++)
  {
    cPVRChannelInfoTag *tag = GetByClient(PVRTimers[i].ClientNumber(), PVRTimers[i].ClientID());
    if (tag)
      PVRTimers[i].SetNumber(tag->Number());
  }

  return;
}

void cPVRChannels::HideChannel(unsigned int number)
{
  CTVDatabase *database = g_PVRManager.GetTVDatabase();

  for (unsigned int i = 0; i < PVRTimers.size(); i++)
  {
    if ((PVRTimers[i].Number() == number) && (PVRTimers[i].IsRadio() == m_bRadio))
    {
      CGUIDialogYesNo* pDialog = (CGUIDialogYesNo*)g_windowManager.GetWindow(WINDOW_DIALOG_YES_NO);
      if (!pDialog)
        return;

      pDialog->SetHeading(18090);
      pDialog->SetLine(0, 18095);
      pDialog->SetLine(1, "");
      pDialog->SetLine(2, 18096);
      pDialog->DoModal();

      if (!pDialog->IsConfirmed())
        return;

      PVRTimers.DeleteTimer(PVRTimers[i], true);
    }
  }

  if (g_PVRManager.IsPlayingTV() || g_PVRManager.IsPlayingRadio() && g_PVRManager.GetCurrentPlayingItem()->GetTVChannelInfoTag()->Number() == number)
  {
    CGUIDialogOK::ShowAndGetInput(18090,18097,0,18098);
    return;
  }

  if (at(number-1).IsHidden())
  {
    at(number-1).SetHidden(false);
    database->Open();
    database->UpdateDBChannel(at(number-1));
    m_iHiddenChannels = database->GetNumHiddenChannels();
    database->Close();
  }
  else
  {
    at(number-1).SetHidden(true);
    cPVREpgs::ClearChannel(at(number-1).ChannelID());
    database->Open();
    database->UpdateDBChannel(at(number-1));
    m_iHiddenChannels = database->GetNumHiddenChannels();
    database->Close();
    MoveChannel(number, size());
  }
}

cPVRChannelInfoTag *cPVRChannels::GetByNumber(int Number)
{
  for (unsigned int i = 0; i < size(); i++)
  {
    if (at(i).Number() == Number)
      return &at(i);
  }
  return NULL;
}

cPVRChannelInfoTag *cPVRChannels::GetByClient(int Number, int ClientID)
{
  for (unsigned int i = 0; i < size(); i++)
  {
    if (at(i).ClientNumber() == Number && at(i).ClientID() == ClientID)
      return &at(i);
  }
  return NULL;
}

cPVRChannelInfoTag *cPVRChannels::GetByChannelID(long ChannelID)
{
  for (unsigned int i = 0; i < size(); i++)
  {
    if (at(i).ChannelID() == ChannelID)
      return &at(i);
  }
  return NULL;
}

cPVRChannelInfoTag *cPVRChannels::GetByUniqueID(long UniqueID)
{
  for (unsigned int i = 0; i < size(); i++)
  {
    if (at(i).UniqueID() != 0)
    {
      if (at(i).UniqueID() == UniqueID)
        return &at(i);
    }
    else
    {
      if (at(i).ChannelID() == UniqueID)
        return &at(i);
    }
  }
  return NULL;
}

CStdString cPVRChannels::GetNameForChannel(unsigned int Number)
{
  if ((Number <= size()+1) && (Number > 0))
  {
    if (at(Number-1).Name() != NULL)
      return at(Number-1).Name();
    else
      return g_localizeStrings.Get(13205);
  }
  return "";
}

CStdString cPVRChannels::GetChannelIcon(unsigned int Number)
{
  if (Number > 0 && Number <= size()+1)
    return "";

  return at(Number-1).Icon();
}

void cPVRChannels::SetChannelIcon(unsigned int Number, CStdString Icon)
{
  CTVDatabase *database = g_PVRManager.GetTVDatabase();

  if (Number > size()+1)
    return;

  if (at(Number-1).Icon() != Icon)
  {
    CTVDatabase *database = g_PVRManager.GetTVDatabase();
    database->Open();
    at(Number-1).SetIcon(Icon);
    database->UpdateDBChannel(at(Number-1));
    database->Close();
  }
}

void cPVRChannels::Clear()
{
  /* Clear all current present Channels inside list */
  erase(begin(), end());
  return;
}

int cPVRChannels::GetNumChannelsFromAll()
{
  return PVRChannelsTV.GetNumChannels()+PVRChannelsRadio.GetNumChannels();
}

void cPVRChannels::SearchMissingChannelIcons()
{
  CLog::Log(LOGINFO,"PVRManager: Manual Channel Icon search started...");
  PVRChannelsTV.SearchAndSetChannelIcons(true);
  PVRChannelsRadio.SearchAndSetChannelIcons(true);
  /// TODO: Add Process dialog here
  CGUIDialogOK::ShowAndGetInput(18033,0,20177,0);
}

cPVRChannelInfoTag *cPVRChannels::GetByClientFromAll(int Number, int ClientID)
{
  cPVRChannelInfoTag *channel;

  channel = PVRChannelsTV.GetByClient(Number, ClientID);
  if (channel != NULL)
    return channel;

  channel = PVRChannelsRadio.GetByClient(Number, ClientID);
  if (channel != NULL)
    return channel;

  return NULL;
}

cPVRChannelInfoTag *cPVRChannels::GetByChannelIDFromAll(long ChannelID)
{
  cPVRChannelInfoTag *channel;

  channel = PVRChannelsTV.GetByChannelID(ChannelID);
  if (channel != NULL)
    return channel;

  channel = PVRChannelsRadio.GetByChannelID(ChannelID);
  if (channel != NULL)
    return channel;

  return NULL;
}

cPVRChannelInfoTag *cPVRChannels::GetByUniqueIDFromAll(long UniqueID)
{
  cPVRChannelInfoTag *channel;

  channel = PVRChannelsTV.GetByUniqueID(UniqueID);
  if (channel != NULL)
    return channel;

  channel = PVRChannelsRadio.GetByUniqueID(UniqueID);
  if (channel != NULL)
    return channel;

  return NULL;
}

// --- cPVRChannelGroup -----------------------------------------------------------

cPVRChannelGroup::cPVRChannelGroup(void)
{
  m_iGroupID = 0;
  m_GroupName = "";
}

// --- cPVRChannelGroups ----------------------------------------------------------

cPVRChannelGroups PVRChannelGroups;

cPVRChannelGroups::cPVRChannelGroups(void)
{
}

bool cPVRChannelGroups::Load()
{
  CTVDatabase *database = g_PVRManager.GetTVDatabase();
  database->Open();

  Clear();
  database->GetGroupList(*this);

  database->Close();
  return true;
}

int cPVRChannelGroups::GetGroupList(CFileItemList* results)
{
  for (unsigned int i = 0; i < size(); i++)
  {
    CFileItemPtr group(new CFileItem(at(i).GroupName()));
    group->m_strTitle = at(i).GroupName();
    group->m_strPath.Format("%i", at(i).GroupID());
    results->Add(group);
  }
  return size();
}

int cPVRChannelGroups::GetFirstChannelForGroupID(int GroupId, bool radio)
{
  if (GroupId == -1)
    return 1;

  cPVRChannels *channels;
  if (!radio)
    channels = &PVRChannelsTV;
  else
    channels = &PVRChannelsRadio;

  for (unsigned int i = 0; i < channels->size(); i++)
  {
    if (channels->at(i).GroupID() == GroupId)
      return i+1;
  }
  return 1;
}

int cPVRChannelGroups::GetPrevGroupID(int current_group_id)
{
  if (size() == 0)
    return -1;

  if ((current_group_id == -1) || (current_group_id == 0))
    return at(size()-1).GroupID();

  for (unsigned int i = 0; i < size(); i++)
  {
    if (current_group_id == at(i).GroupID())
    {
      if (i != 0)
        return at(i-1).GroupID();
      else
        return -1;
    }
  }
  return -1;
}

int cPVRChannelGroups::GetNextGroupID(int current_group_id)
{
  unsigned int i = 0;

  if (size() == 0)
    return -1;

  if ((current_group_id == 0) || (current_group_id == -1))
    return at(0).GroupID();

  if (size() == 0)
    return -1;

  for (; i < size(); i++)
  {
    if (current_group_id == at(i).GroupID())
      break;
  }

  if (i >= size()-1)
    return -1;
  else
    return at(i+1).GroupID();
}

void cPVRChannelGroups::AddGroup(const CStdString &name)
{
  CTVDatabase *database = g_PVRManager.GetTVDatabase();
  database->Open();

  Clear();
  database->AddGroup(name);
  database->GetGroupList(*this);

  database->Close();
}

bool cPVRChannelGroups::RenameGroup(unsigned int GroupId, const CStdString &newname)
{
  CTVDatabase *database = g_PVRManager.GetTVDatabase();
  database->Open();

  Clear();
  database->RenameGroup(GroupId, newname);
  database->GetGroupList(*this);

  database->Close();
  return true;
}

bool cPVRChannelGroups::DeleteGroup(unsigned int GroupId)
{
  CTVDatabase *database = g_PVRManager.GetTVDatabase();
  database->Open();

  /* Delete the group inside Database */
  database->DeleteGroup(GroupId);

  /* Set all channels with this group to undefined */
  for (unsigned int i = 0; i < PVRChannelsTV.size(); i++)
  {
    if (PVRChannelsTV[i].GroupID() == GroupId)
    {
      PVRChannelsTV[i].SetGroupID(0);
      database->UpdateDBChannel(PVRChannelsTV[i]);
    }
  }
  for (unsigned int i = 0; i < PVRChannelsRadio.size(); i++)
  {
    if (PVRChannelsRadio[i].GroupID() == GroupId)
    {
      PVRChannelsRadio[i].SetGroupID(0);
      database->UpdateDBChannel(PVRChannelsRadio[i]);
    }
  }

  /* Reload the group list */
  Clear();
  database->GetGroupList(*this);

  database->Close();
  return true;
}

CStdString cPVRChannelGroups::GetGroupName(int GroupId)
{
  if (GroupId != -1)
  {
    for (unsigned int i = 0; i < size(); i++)
    {
      if (GroupId == at(i).GroupID())
        return at(i).GroupName();
    }
  }

  return g_localizeStrings.Get(593);
}

bool cPVRChannelGroups::ChannelToGroup(const cPVRChannelInfoTag &channel, unsigned int GroupId)
{
  CTVDatabase *database = g_PVRManager.GetTVDatabase();
  database->Open();

  cPVRChannels *channels;
  if (!channel.IsRadio())
    channels = &PVRChannelsTV;
  else
    channels = &PVRChannelsRadio;

  channels->at(channel.Number()-1).SetGroupID(GroupId);
  database->UpdateDBChannel(channels->at(channel.Number()-1));

  database->Close();
  return true;
}

void cPVRChannelGroups::Clear()
{
  /* Clear all current present Channel groups inside list */
  erase(begin(), end());
  return;
}
