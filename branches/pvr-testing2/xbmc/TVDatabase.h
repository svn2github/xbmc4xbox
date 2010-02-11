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

#include "Database.h"
#include "DateTime.h"
#include "FileItem.h"
#include "settings/VideoSettings.h"
#include "utils/PVREpg.h"
#include "utils/PVRChannels.h"

class CTVDatabase : public CDatabase
{
public:
  CTVDatabase(void);
  virtual ~CTVDatabase(void);

  bool EraseClients();
  long AddClient(const CStdString &client, const CStdString &guid);
  CDateTime GetLastEPGScanTime();
  bool UpdateLastEPGScan(const CDateTime lastScan);
  int GetLastChannel();
  bool UpdateLastChannel(const cPVRChannelInfoTag &info);

  /* Database Epg handling */
  bool EraseEPG();
  bool EraseChannelEPG(long channelID);
  bool EraseChannelEPGAfterTime(long channelID, CDateTime after);
  bool EraseOldEPG();
  long AddEPGEntry(const cPVREPGInfoTag &info);
  bool UpdateEPGEntry(const cPVREPGInfoTag &info);
  bool RemoveEPGEntry(const cPVREPGInfoTag &info);
  bool RemoveEPGEntries(unsigned int channelID, const CDateTime &start, const CDateTime &end);
  bool GetEPGForChannel(const cPVRChannelInfoTag &channelinfo, cPVREpg *epg, const CDateTime &start, const CDateTime &end);
  CDateTime GetEPGDataStart(int channelID);
  CDateTime GetEPGDataEnd(int channelID);

  /* Database Channel handling */
  bool EraseChannels();
  bool EraseClientChannels(long clientID);
  long AddDBChannel(const cPVRChannelInfoTag &info);
  bool RemoveDBChannel(const cPVRChannelInfoTag &info);
  long UpdateDBChannel(const cPVRChannelInfoTag &info);
  int  GetDBNumChannels(bool radio);
  int  GetNumHiddenChannels();
  bool HasChannel(const cPVRChannelInfoTag &info);
  bool GetDBChannelList(cPVRChannels &results, bool radio);

  /* Database Channel Portal Linkage */
  bool EraseChannelLinkageMap();
  long AddChannelLinkage(int PortalChannel, int LinkedChannel);
  bool DeleteChannelLinkage(unsigned int channelId);
  bool GetChannelLinkageMap(cPVRChannelInfoTag &channel);

  /* Database Channel Group handling */
  bool EraseChannelGroups();
  long AddChannelGroup(const CStdString &groupName, int sortOrder);
  bool DeleteChannelGroup(unsigned int GroupId);
  bool GetChannelGroupList(cPVRChannelGroups &results);
  bool SetChannelGroupName(unsigned int GroupId, const CStdString &newname);
  bool SetChannelGroupSortOrder(unsigned int GroupId, int sortOrder);

  /* Database Radio Group handling */
  bool EraseRadioChannelGroups();
  long AddRadioChannelGroup(const CStdString &groupName, int sortOrder);
  bool DeleteRadioChannelGroup(unsigned int GroupId);
  bool GetRadioChannelGroupList(cPVRChannelGroups &results);
  bool SetRadioChannelGroupName(unsigned int GroupId, const CStdString &newname);
  bool SetRadioChannelGroupSortOrder(unsigned int GroupId, int sortOrder);

  /* Database channel settings storage */
  bool EraseChannelSettings();
  bool GetChannelSettings(unsigned int channelID, CVideoSettings &settings);
  bool SetChannelSettings(unsigned int channelID, const CVideoSettings &settings);

protected:
  long GetClientId(const CStdString &guid);
  long GetChannelGroupId(const CStdString &groupname);
  long GetRadioChannelGroupId(const CStdString &groupname);

private:
  virtual bool CreateTables();
  virtual bool UpdateOldVersion(int version);
};
