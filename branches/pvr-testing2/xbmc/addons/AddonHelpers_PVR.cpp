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

#include "Application.h"
#include "AddonHelpers_PVR.h"
#include "PVREpg.h"
#include "PVRChannels.h"
#include "PVRTimers.h"
#include "PVRRecordings.h"
#include "PVRClient.h"
#include "log.h"
#include "PVRManager.h"

namespace ADDON
{

CAddonHelpers_PVR::CAddonHelpers_PVR(CAddon* addon)
{
  m_addon     = addon;
  m_callbacks = new CB_PVRLib;

  /* Write XBMC PVR specific Add-on function addresses to callback table */
  m_callbacks->TransferEpgEntry       = PVRTransferEpgEntry;
  m_callbacks->TransferChannelEntry   = PVRTransferChannelEntry;
  m_callbacks->TransferTimerEntry     = PVRTransferTimerEntry;
  m_callbacks->TransferRecordingEntry = PVRTransferRecordingEntry;
  m_callbacks->AddMenuHook            = PVRAddMenuHook;
  m_callbacks->Recording              = PVRRecording;
  m_callbacks->TriggerTimerUpdate     = PVRTriggerTimerUpdate;
  m_callbacks->TriggerRecordingUpdate = PVRTriggerRecordingUpdate;
  m_callbacks->FreeDemuxPacket        = PVRFreeDemuxPacket;
  m_callbacks->AllocateDemuxPacket    = PVRAllocateDemuxPacket;
};

CAddonHelpers_PVR::~CAddonHelpers_PVR()
{
  delete m_callbacks;
};

void CAddonHelpers_PVR::PVRTransferEpgEntry(void *addonData, const PVRHANDLE handle, const PVR_PROGINFO *epgentry)
{
  CAddonHelpers* addon = (CAddonHelpers*) addonData;
  if (addon == NULL || handle == NULL || epgentry == NULL)
  {
    CLog::Log(LOGERROR, "PVR: PVRTransferEpgEntry is called with NULL-Pointer!!!");
    return;
  }

  cPVREpg *xbmcEpg        = (cPVREpg*) handle->DATA_ADDRESS;
  PVR_PROGINFO *epgentry2 = (PVR_PROGINFO*) epgentry;
  CPVRClient* client      = (CPVRClient*) handle->CALLER_ADDRESS;
  epgentry2->starttime   += client->GetTimeCorrection();
  epgentry2->endtime     += client->GetTimeCorrection();
  if (handle->DATA_IDENTIFIER == 1)
    cPVREpg::AddDB(epgentry2, xbmcEpg);
  else
    cPVREpg::Add(epgentry2, xbmcEpg);

  return;
}

void CAddonHelpers_PVR::PVRTransferChannelEntry(void *addonData, const PVRHANDLE handle, const PVR_CHANNEL *channel)
{
  CAddonHelpers* addon = (CAddonHelpers*) addonData;
  if (addon == NULL || handle == NULL || channel == NULL)
  {
    CLog::Log(LOGERROR, "PVR: PVRTransferChannelEntry is called with NULL-Pointer!!!");
    return;
  }

  CPVRClient* client         = (CPVRClient*) handle->CALLER_ADDRESS;
  cPVRChannels *xbmcChannels = (cPVRChannels*) handle->DATA_ADDRESS;
  cPVRChannelInfoTag tag;

  tag.SetChannelID(-1);
  tag.SetNumber(-1);
  tag.SetClientNumber(channel->number);
  tag.SetGroupID(0);
  tag.SetClientID(client->GetClientID());
  tag.SetUniqueID(channel->uid);
  tag.SetName(channel->name);
  tag.SetClientName(channel->callsign);
  tag.SetIcon(channel->iconpath);
  tag.SetEncryptionSystem(channel->encryption);
  tag.SetRadio(channel->radio);
  tag.SetHidden(channel->hide);
  tag.SetRecording(channel->recording);
  tag.SetInputFormat(channel->input_format);
  tag.SetStreamURL(channel->stream_url);

  xbmcChannels->push_back(tag);
  return;
}

void CAddonHelpers_PVR::PVRTransferRecordingEntry(void *addonData, const PVRHANDLE handle, const PVR_RECORDINGINFO *recording)
{
  CAddonHelpers* addon = (CAddonHelpers*) addonData;
  if (addon == NULL || handle == NULL || recording == NULL)
  {
    CLog::Log(LOGERROR, "PVR: PVRTransferRecordingEntry is called with NULL-Pointer!!!");
    return;
  }

  CPVRClient* client = (CPVRClient*) handle->CALLER_ADDRESS;
  cPVRRecordings *xbmcRecordings = (cPVRRecordings*) handle->DATA_ADDRESS;

  cPVRRecordingInfoTag tag;

  tag.SetClientIndex(recording->index);
  tag.SetClientID(client->GetClientID());
  tag.SetTitle(recording->title);
  tag.SetRecordingTime(recording->recording_time);
  tag.SetDuration(CDateTimeSpan(0, 0, recording->duration / 60, recording->duration % 60));
  tag.SetPriority(recording->priority);
  tag.SetLifetime(recording->lifetime);
  tag.SetDirectory(recording->directory);
  tag.SetPlot(recording->description);
  tag.SetPlotOutline(recording->subtitle);
  tag.SetStreamURL(recording->stream_url);
  tag.SetChannelName(recording->channel_name);

  xbmcRecordings->push_back(tag);
  return;
}

void CAddonHelpers_PVR::PVRTransferTimerEntry(void *addonData, const PVRHANDLE handle, const PVR_TIMERINFO *timer)
{
  CAddonHelpers* addon = (CAddonHelpers*) addonData;
  if (addon == NULL || handle == NULL || timer == NULL)
  {
    CLog::Log(LOGERROR, "PVR: PVRTransferTimerEntry is called with NULL-Pointer!!!");
    return;
  }

  cPVRTimers *xbmcTimers      = (cPVRTimers*) handle->DATA_ADDRESS;
  CPVRClient* client          = (CPVRClient*) handle->CALLER_ADDRESS;
  cPVRChannelInfoTag *channel = cPVRChannels::GetByClientFromAll(timer->channelNum, client->GetClientID());

  if (channel == NULL)
  {
    CLog::Log(LOGERROR, "PVR: PVRTransferTimerEntry is called with not present channel");
    return;
  }

  cPVRTimerInfoTag tag;
  tag.SetClientID(client->GetClientID());
  tag.SetClientIndex(timer->index);
  tag.SetActive(timer->active);
  tag.SetTitle(timer->title);
  tag.SetDir(timer->directory);
  tag.SetClientNumber(timer->channelNum);
  tag.SetStart((time_t) (timer->starttime+client->GetTimeCorrection()));
  tag.SetStop((time_t) (timer->endtime+client->GetTimeCorrection()));
  tag.SetFirstDay((time_t) (timer->firstday+client->GetTimeCorrection()));
  tag.SetPriority(timer->priority);
  tag.SetLifetime(timer->lifetime);
  tag.SetRecording(timer->recording);
  tag.SetRepeating(timer->repeat);
  tag.SetWeekdays(timer->repeatflags);
  tag.SetNumber(channel->Number());
  tag.SetRadio(channel->IsRadio());
  CStdString path;
  path.Format("pvr://client%i/timers/%i", tag.ClientID(), tag.ClientIndex());
  tag.SetPath(path);

  CStdString summary;
  if (!tag.IsRepeating())
  {
    summary.Format("%s %s %s %s %s", tag.Start().GetAsLocalizedDate()
                   , g_localizeStrings.Get(19159)
                   , tag.Start().GetAsLocalizedTime("", false)
                   , g_localizeStrings.Get(19160)
                   , tag.Stop().GetAsLocalizedTime("", false));
  }
  else if (tag.FirstDay() != NULL)
  {
    summary.Format("%s-%s-%s-%s-%s-%s-%s %s %s %s %s %s %s"
                   , timer->repeatflags & 0x01 ? g_localizeStrings.Get(19149) : "__"
                   , timer->repeatflags & 0x02 ? g_localizeStrings.Get(19150) : "__"
                   , timer->repeatflags & 0x04 ? g_localizeStrings.Get(19151) : "__"
                   , timer->repeatflags & 0x08 ? g_localizeStrings.Get(19152) : "__"
                   , timer->repeatflags & 0x10 ? g_localizeStrings.Get(19153) : "__"
                   , timer->repeatflags & 0x20 ? g_localizeStrings.Get(19154) : "__"
                   , timer->repeatflags & 0x40 ? g_localizeStrings.Get(19155) : "__"
                   , g_localizeStrings.Get(19156)
                   , tag.FirstDay().GetAsLocalizedDate(false)
                   , g_localizeStrings.Get(19159)
                   , tag.Start().GetAsLocalizedTime("", false)
                   , g_localizeStrings.Get(19160)
                   , tag.Stop().GetAsLocalizedTime("", false));
  }
  else
  {
    summary.Format("%s-%s-%s-%s-%s-%s-%s %s %s %s %s"
                   , timer->repeatflags & 0x01 ? g_localizeStrings.Get(19149) : "__"
                   , timer->repeatflags & 0x02 ? g_localizeStrings.Get(19150) : "__"
                   , timer->repeatflags & 0x04 ? g_localizeStrings.Get(19151) : "__"
                   , timer->repeatflags & 0x08 ? g_localizeStrings.Get(19152) : "__"
                   , timer->repeatflags & 0x10 ? g_localizeStrings.Get(19153) : "__"
                   , timer->repeatflags & 0x20 ? g_localizeStrings.Get(19154) : "__"
                   , timer->repeatflags & 0x40 ? g_localizeStrings.Get(19155) : "__"
                   , g_localizeStrings.Get(19159)
                   , tag.Start().GetAsLocalizedTime("", false)
                   , g_localizeStrings.Get(19160)
                   , tag.Stop().GetAsLocalizedTime("", false));
  }
  tag.SetSummary(summary);

  xbmcTimers->push_back(tag);
  return;
}

void CAddonHelpers_PVR::PVRAddMenuHook(void *addonData, PVR_MENUHOOK *hook)
{
  CAddonHelpers* addon = (CAddonHelpers*) addonData;
  if (addon == NULL || hook == NULL)
  {
    CLog::Log(LOGERROR, "PVR: PVRAddMenuHook is called with NULL-Pointer!!!");
    return;
  }

  CAddonHelpers_PVR* addonHelper = addon->GetHelperPVR();
  CPVRClient* client  = (CPVRClient*) addonHelper->m_addon;
  PVR_MENUHOOKS *hooks = client->GetMenuHooks();

  PVR_MENUHOOK hookInt;
  hookInt.hook_id   = hook->hook_id;
  hookInt.string_id = hook->string_id;
  hooks->push_back(hookInt);
}

void CAddonHelpers_PVR::PVRRecording(void *addonData, const char *Name, const char *FileName, bool On)
{
  CAddonHelpers* addon = (CAddonHelpers*) addonData;
  if (addon == NULL)
  {
    CLog::Log(LOGERROR, "PVR: PVRRecording is called with NULL-Pointer!!!");
    return;
  }

  CAddonHelpers_PVR* addonHelper = addon->GetHelperPVR();

  CStdString line1;
  CStdString line2;
  if (On)
    line1.Format(g_localizeStrings.Get(19197), addonHelper->m_addon->Name());
  else
    line1.Format(g_localizeStrings.Get(19198), addonHelper->m_addon->Name());

  if (Name)
    line2 = Name;
  else if (FileName)
    line2 = FileName;
  else
    line2 = "";

  g_application.m_guiDialogKaiToast.QueueNotification(CGUIDialogKaiToast::Info, line1, line2, 5000, false);
  CLog::Log(LOGDEBUG, "%s: %s-%s - Recording %s : %s %s", __FUNCTION__, TranslateType(addonHelper->m_addon->Type()).c_str(), addonHelper->m_addon->Name().c_str(), On ? "started" : "finished", Name, FileName);
}

void CAddonHelpers_PVR::PVRTriggerTimerUpdate(void *addonData)
{
  CAddonHelpers* addon = (CAddonHelpers*) addonData;
  if (addon == NULL)
  {
    CLog::Log(LOGERROR, "PVR: PVRTriggerTimerUpdate is called with NULL-Pointer!!!");
    return;
  }

  CAddonHelpers_PVR* addonHelper = addon->GetHelperPVR();

  g_PVRManager.TriggerTimersUpdate(false);
  CLog::Log(LOGDEBUG, "%s: %s-%s - Triggered Timer Update", __FUNCTION__, TranslateType(addonHelper->m_addon->Type()).c_str(), addonHelper->m_addon->Name().c_str());
}

void CAddonHelpers_PVR::PVRTriggerRecordingUpdate(void *addonData)
{
  CAddonHelpers* addon = (CAddonHelpers*) addonData;
  if (addon == NULL)
  {
    CLog::Log(LOGERROR, "PVR: PVRTriggerRecordingUpdate is called with NULL-Pointer!!!");
    return;
  }

  CAddonHelpers_PVR* addonHelper = addon->GetHelperPVR();

  g_PVRManager.TriggerRecordingsUpdate(false);
  CLog::Log(LOGDEBUG, "%s: %s-%s - Triggered Recording Update", __FUNCTION__, TranslateType(addonHelper->m_addon->Type()).c_str(), addonHelper->m_addon->Name().c_str());
}

void CAddonHelpers_PVR::PVRFreeDemuxPacket(void *addonData, DemuxPacket* pPacket)
{
  CDVDDemuxUtils::FreeDemuxPacket(pPacket);
}

DemuxPacket* CAddonHelpers_PVR::PVRAllocateDemuxPacket(void *addonData, int iDataSize)
{
  return CDVDDemuxUtils::AllocateDemuxPacket(iDataSize);
}

}; /* namespace ADDON */
