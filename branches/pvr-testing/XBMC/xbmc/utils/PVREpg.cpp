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

#include "FileItem.h"
#include "PVREpg.h"
#include "PVRChannels.h"
#include "PVRManager.h"
#include "GUISettings.h"
#include "GUIDialogEpgScan.h"
#include "GUIWindowManager.h"
#include "utils/log.h"
#include "utils/TimeUtils.h"

using namespace std;

void EPGSearchFilter::SetDefaults()
{
  m_SearchString      = "";
  m_CaseSensitive     = false;
  m_SearchDescription = false;
  m_GenreType         = -1;
  m_GenreSubType      = -1;
  m_minDuration       = -1;
  m_maxDuration       = -1;
  cPVREpgs::GetFirstEPGDate().GetAsSystemTime(m_startDate);
  m_startDate.wHour   = 0;
  m_startDate.wMinute = 0;
  cPVREpgs::GetLastEPGDate().GetAsSystemTime(m_endDate);
  m_endDate.wHour     = 23;
  m_endDate.wMinute   = 59;
  m_startTime         = m_startDate;
  m_startTime.wHour   = 0;
  m_startTime.wMinute = 0;
  m_endTime           = m_endDate;
  m_endTime.wHour     = 23;
  m_endTime.wMinute   = 59;
  m_ChannelNumber     = -1;
  m_FTAOnly           = false;
  m_IncUnknGenres     = true;
  m_Group             = -1;
  m_IgnPresentTimers  = true;
  m_IgnPresentRecords = true;
  m_PreventRepeats    = true;
}

CTVEPGInfoTag::CTVEPGInfoTag(long uniqueBroadcastID)
{
  Reset();
  m_uniqueBroadcastID = uniqueBroadcastID;
}

void CTVEPGInfoTag::Reset()
{
  m_idChannel = -1;
  m_IconPath = "";
  m_strSource = "";
  m_strBouquet = "";
  m_strChannel = "";
  m_strFileNameAndPath = "";
  m_repeat = false;
  m_commFree = false;
  m_isRecording = false;
  m_bAutoSwitch = false;
  m_isRadio = false;

  CVideoInfoTag::Reset();
}

bool CTVEPGInfoTag::HasTimer(void) const
{
  for (unsigned int i = 0; i < PVRTimers.size(); ++i)
  {
    if (PVRTimers[i].Epg() == this)
      return true;
  }
  return false;
}

int CTVEPGInfoTag::DurationSeconds() const
{
  int duration;
  duration =  m_duration.GetDays()*60*60*24;
  duration += m_duration.GetHours()*60*60;
  duration += m_duration.GetMinutes()*60;
  duration += m_duration.GetSeconds();
  return duration;
}


cPVREpg::cPVREpg(long ChannelID)
{
  m_channelID = ChannelID;
  m_Channel = cPVRChannels::GetByChannelIDFromAll(ChannelID);
}

CTVEPGInfoTag *cPVREpg::AddInfoTag(CTVEPGInfoTag *Tag)
{
  tags.push_back(*Tag);
  Tag->Epg = this;
  return Tag;
}

void cPVREpg::DelInfoTag(CTVEPGInfoTag *tag)
{
  if (tag->Epg == this)
  {
    for (unsigned int i = 0; i < tags.size(); i++)
    {
      CTVEPGInfoTag *entry = &tags[i];
      if (entry == tag)
      {
//      if (hasRunning && Event->IsRunning())
//        ClrRunningStatus();
        //delete entry;
        tags.erase(tags.begin()+i);
        return;
      }
    }
  }
}

void cPVREpg::Cleanup(void)
{
  Cleanup(CDateTime::GetCurrentDateTime());
}

void cPVREpg::Cleanup(CDateTime Time)
{
  CTVEPGInfoTag *tag;
  for (unsigned int i = 0; i < tags.size(); i++)
  {
    if (!tags[i].HasTimer() && tags[i].End()+CDateTimeSpan(0, g_guiSettings.GetInt("pvrmenu.lingertime") / 60 + 1, g_guiSettings.GetInt("pvrmenu.lingertime") % 60, 0) < Time) // adding one hour for safety
      DelInfoTag(tag);
    else
      break;
  }
}

const CTVEPGInfoTag *cPVREpg::GetInfoTagNow(void) const
{
  CDateTime now = CDateTime::GetCurrentDateTime();

  if (tags.size() == 0)
    return NULL;

  for (unsigned int i = 0; i < tags.size(); i++)
  {
    if ((tags[i].Start() <= now) && (tags[i].End() > now))
      return &tags[i];
  }
  return NULL;
}

const CTVEPGInfoTag *cPVREpg::GetInfoTagNext(void) const
{
  const CTVEPGInfoTag *TagNext = NULL;
  CDateTime now = CDateTime::GetCurrentDateTime();

  if (tags.size() == 0)
    return false;

  for (unsigned int i = 0; i < tags.size(); i++)
  {
    if ((tags[i].Start() <= now) && (tags[i].End() > now))
    {
      CDateTime next = tags[i].End();

      for (unsigned int j = 0; j < tags.size(); j++)
      {
        if (tags[j].Start() >= next)
        {
          return &tags[j];
        }
      }
    }
  }

  return NULL;
}

const CTVEPGInfoTag *cPVREpg::GetInfoTag(long uniqueID, CDateTime StartTime) const
{

  return NULL;
}

const CTVEPGInfoTag *cPVREpg::GetInfoTagAround(CDateTime Time) const
{

  return NULL;
}

bool cPVREpg::Add(const PVR_PROGINFO *data, cPVREpg *Epg)
{
  if (Epg && data)
  {
    CTVEPGInfoTag *InfoTag = NULL;

    long      uniqueBroadcastID = data->uid;
    CDateTime StartTime         = CDateTime((time_t)data->starttime);

    InfoTag = (CTVEPGInfoTag *)Epg->GetInfoTag(uniqueBroadcastID, StartTime);
    CTVEPGInfoTag *newInfoTag = NULL;
    if (!InfoTag)
    {
      InfoTag = newInfoTag = new CTVEPGInfoTag(uniqueBroadcastID);
    }
    if (InfoTag)
    {
      InfoTag->SetStart(CDateTime((time_t)data->starttime));
      InfoTag->SetEnd(CDateTime((time_t)data->endtime));
      InfoTag->SetTitle(data->title);
      InfoTag->SetPlotOutline(data->subtitle);
      InfoTag->SetPlot(data->description);
      InfoTag->SetGenreType(data->genre_type);
      InfoTag->SetGenreSubType(data->genre_sub_type);
      InfoTag->SetGenre(data->genre);
      int duration = data->endtime - data->starttime;
      InfoTag->SetDuration(CDateTimeSpan(0, 0, duration / 60, duration % 60));
      InfoTag->SetChannelID(Epg->ChannelID());
      InfoTag->SetChannel(Epg->m_Channel);

      InfoTag->m_strChannel = Epg->m_Channel->Name();
      InfoTag->m_channelNum = Epg->m_Channel->Number();
      InfoTag->m_isRadio    = Epg->m_Channel->IsRadio();
      InfoTag->m_IconPath   = Epg->m_Channel->Icon();

      if (newInfoTag)
        Epg->AddInfoTag(newInfoTag);

      return true;
    }
  }
  return false;
}




// --- cPVREpgsLock --------------------------------------------------------

cPVREpgsLock::cPVREpgsLock(bool WriteLock)
{
  cPVREpgs::m_epgs.m_locked++;
  m_WriteLock = WriteLock;
  if (m_WriteLock)
    EnterCriticalSection(&cPVREpgs::m_epgs.m_critSection);
}

cPVREpgsLock::~cPVREpgsLock()
{
  if (m_WriteLock)
    LeaveCriticalSection(&cPVREpgs::m_epgs.m_critSection);
  cPVREpgs::m_epgs.m_locked--;
}

bool cPVREpgsLock::Locked(void)
{
  return cPVREpgs::m_epgs.m_locked > 1 ? true : false;
}

// --- cPVREpgs ---------------------------------------------------------------

cPVREpgs          cPVREpgs::m_epgs;
DWORD             cPVREpgs::m_lastCleanup = time(NULL);

const cPVREpgs *cPVREpgs::EPGs(cPVREpgsLock &PVREpgsLock)
{
  return PVREpgsLock.Locked() ? NULL : &m_epgs;
}

void cPVREpgs::Cleanup(void)
{
  DWORD now = CTimeUtils::GetTimeMS();
  if (now - m_lastCleanup > 3600)
  {
    CLog::Log(LOGNOTICE, "cPVREpgs: cleaning up epg data");
    cPVREpgsLock EpgsLock(true);
    cPVREpgs *s = (cPVREpgs *)EPGs(EpgsLock);
    if (s)
    {
      for (unsigned int i = 0; i < s->size(); i++)
        s->at(i).Cleanup(CDateTime::GetCurrentDateTime());
    }
    m_lastCleanup = now;
  }
}

bool cPVREpgs::ClearAll(void)
{
  cPVREpgsLock EpgsLock(true);
  cPVREpgs *s = (cPVREpgs *)EPGs(EpgsLock);
  if (s)
  {
    for (unsigned int i = 0; i < PVRTimers.size(); ++i)
      PVRTimers[i].SetEpg(NULL);
    for (unsigned int i = 0; i < s->size(); i++)
      s->at(i).Cleanup(-1);
    return true;
  }
  return false;
}

bool cPVREpgs::ClearChannel(long ChannelID)
{
  cPVREpgsLock EpgsLock(true);
  cPVREpgs *s = (cPVREpgs *)EPGs(EpgsLock);
  if (s)
  {
    for (unsigned int i = 0; i < s->size(); i++)
    {
      if (ChannelID == s->at(i).ChannelID())
      {
        s->at(i).Cleanup(-1);
        return true;
      }
    }
  }
  return false;
}

bool cPVREpgs::Load()
{
  InitializeCriticalSection(&m_epgs.m_critSection);
  m_epgs.m_locked = 0;
  return Update(false);
}

void cPVREpgs::Process()
{
  CLIENTMAP   *clients  = g_PVRManager.Clients();
  cPVREpgsLock EpgsLock(true);
  cPVREpgs *s = (cPVREpgs *)EPGs(EpgsLock);
  if (s)
  {
    CGUIDialogEpgScan *scanner = (CGUIDialogEpgScan *)g_windowManager.GetWindow(WINDOW_DIALOG_EPG_SCAN);
    scanner->Show();
    int channelcount = PVRChannelsTV.size() + PVRChannelsRadio.size();

    time_t start;
    time_t end;
    CDateTime::GetCurrentDateTime().GetAsTime(start);
    CDateTime::GetCurrentDateTime().GetAsTime(end);
    start -= g_guiSettings.GetInt("pvrmenu.lingertime")*60;
    end   += g_guiSettings.GetInt("pvrmenu.daystodisplay")*24*60*60;

    for (unsigned int i = 0; i < PVRChannelsTV.size(); i++)
    {
      scanner->SetProgress(i, channelcount);
      scanner->SetTitle(PVRChannelsTV[i].Name());
      scanner->UpdateState();
      cPVREpg *p = s->AddEPG(PVRChannelsTV[i].ChannelID());
      if (p)
      {
        PVR_ERROR err = clients->find(PVRChannelsTV[i].ClientID())->second->GetEPGForChannel(PVRChannelsTV[i].ClientNumber(), p, start, end);
        if (err == PVR_ERROR_NO_ERROR)
        {
          // Initialize the channels' schedule pointers, so that the first WhatsOn menu will come up faster:
          s->GetEPG(&PVRChannelsTV[i]);
        }
      }
    }

    for (unsigned int i = 0; i < PVRChannelsRadio.size(); i++)
    {
      scanner->SetProgress(PVRChannelsTV.size()+i, channelcount);
      scanner->SetTitle(PVRChannelsRadio[i].Name());
      scanner->UpdateState();
      cPVREpg *p = s->AddEPG(PVRChannelsRadio[i].ChannelID());
      if (p)
      {
        PVR_ERROR err = clients->find(PVRChannelsRadio[i].ClientID())->second->GetEPGForChannel(PVRChannelsRadio[i].ClientNumber(), p, start, end);
        if (err == PVR_ERROR_NO_ERROR)
        {
          // Initialize the channels' schedule pointers, so that the first WhatsOn menu will come up faster:
          s->GetEPG(&PVRChannelsRadio[i]);
        }
      }
    }
    scanner->Close();
  }
}

bool cPVREpgs::Update(bool Wait)
{
  if (Wait)
  {
    m_epgs.Process();
    return true;
  }
  else
  {
    m_epgs.Create();
    m_epgs.SetName("PVR EPG Update");
    m_epgs.SetPriority(-5);
  }
  return false;
}

int cPVREpgs::GetEPGAll(CFileItemList* results, bool radio)
{
  int cnt = 0;

  cPVRChannels *ch = !radio ? &PVRChannelsTV : &PVRChannelsRadio;
  cPVREpgsLock EpgsLock;
  cPVREpgs *s = (cPVREpgs *)EPGs(EpgsLock);
  if (s)
  {
    for (unsigned int i = 0; i < ch->size(); i++)
    {
      if (ch->at(i).m_hide)
        continue;

      const cPVREpg *Epg = s->GetEPG(&ch->at(i), true);

      const vector<CTVEPGInfoTag> *ch_epg = Epg->InfoTags();


      for (unsigned int i = 0; i < ch_epg->size(); i++)
      {
        CFileItemPtr channel(new CFileItem(ch_epg->at(i)));
        results->Add(channel);
        cnt++;
      }
    }
  }
  return cnt;
}

bool cPVREpgs::FilterEntry(const CTVEPGInfoTag &tag, const EPGSearchFilter &filter)
{
  if (filter.m_GenreType != -1)
  {
    if (tag.GenreType() != filter.m_GenreType &&
        (!filter.m_IncUnknGenres &&
        ((tag.GenreType() < EVCONTENTMASK_USERDEFINED || tag.GenreType() >= EVCONTENTMASK_MOVIEDRAMA))))
    {
      return false;
    }

    if (filter.m_GenreSubType != -1)
    {
      if (tag.GenreSubType() != filter.m_GenreSubType &&
          (!filter.m_IncUnknGenres &&
          ((tag.GenreSubType() < EVCONTENTMASK_USERDEFINED || tag.GenreSubType() >= EVCONTENTMASK_MOVIEDRAMA))))
      {
        return false;
      }
    }
  }
  if (filter.m_minDuration != -1)
  {
    if (tag.DurationSeconds() < (filter.m_minDuration*60))
      return false;
  }
  if (filter.m_maxDuration != -1)
  {
    if (tag.DurationSeconds() > (filter.m_maxDuration*60))
      return false;
  }
  if (filter.m_ChannelNumber != -1)
  {
    if (filter.m_ChannelNumber == -2)
    {
      if (tag.IsRadio())
        return false;
    }
    else if (filter.m_ChannelNumber == -3)
    {
      if (!tag.IsRadio())
        return false;
    }
    else if (tag.Channel() != filter.m_ChannelNumber)
      return false;
  }
  if (filter.m_FTAOnly && tag.IsEncrypted())
  {
    return false;
  }
  if (filter.m_Group != -1)
  {
    if (tag.GroupID() != filter.m_Group)
      return false;
  }

  int timeTag = (tag.Start().GetHour()*60 + tag.Start().GetMinute());

  if (timeTag < (filter.m_startTime.wHour*60 + filter.m_startTime.wMinute))
    return false;

  if (timeTag > (filter.m_endTime.wHour*60 + filter.m_endTime.wMinute))
    return false;

  if (tag.Start() < filter.m_startDate)
    return false;

  if (tag.Start() > filter.m_endDate)
    return false;

  if (filter.m_SearchString != "")
  {
    CStdString title = tag.Title();
    CStdString searchStr = filter.m_SearchString;
    if (!filter.m_CaseSensitive)
    {
      title.ToLower();
      searchStr.ToLower();
    }

    if (title.Find(searchStr) < 0)
    {
      if (filter.m_SearchDescription)
      {
        title = tag.PlotOutline();
        if (!filter.m_CaseSensitive)
          title.ToLower();

        if (title.Find(searchStr) < 0)
        {
          title = tag.Plot();
          if (!filter.m_CaseSensitive)
            title.ToLower();

          if (title.Find(searchStr) < 0)
          {
            return false;
          }
        }
      }
      else
      {
        return false;
      }
    }

  }
  return true;
}

int cPVREpgs::GetEPGSearch(CFileItemList* results, const EPGSearchFilter &filter)
{
  cPVREpgsLock EpgsLock;
  cPVREpgs *s = (cPVREpgs *)EPGs(EpgsLock);
  if (s)
  {
    for (unsigned int i = 0; i < PVRChannelsTV.size(); i++)
    {
      if (PVRChannelsTV[i].m_hide)
        continue;

      const cPVREpg *Epg = s->GetEPG(&PVRChannelsTV[i], true);

      const vector<CTVEPGInfoTag> *ch_epg = Epg->InfoTags();


      for (unsigned int i = 0; i < ch_epg->size(); i++)
      {
        if (FilterEntry(ch_epg->at(i), filter))
        {
          CFileItemPtr channel(new CFileItem(ch_epg->at(i)));
          results->Add(channel);
        }
      }
    }

    for (unsigned int i = 0; i < PVRChannelsRadio.size(); i++)
    {
      if (PVRChannelsRadio[i].m_hide)
        continue;

      const cPVREpg *Epg = s->GetEPG(&PVRChannelsRadio[i], true);

      const vector<CTVEPGInfoTag> *ch_epg = Epg->InfoTags();


      for (unsigned int i = 0; i < ch_epg->size(); i++)
      {
        if (FilterEntry(ch_epg->at(i), filter))
        {
          CFileItemPtr channel(new CFileItem(ch_epg->at(i)));
          results->Add(channel);
        }
      }
    }

    if (filter.m_IgnPresentRecords && PVRRecordings.size() > 0)
    {
      for (unsigned int i = 0; i < PVRRecordings.size(); i++)
      {
        unsigned int size = results->Size();
        for (unsigned int j = 0; j < size; j++)
        {
          const CTVEPGInfoTag *epgentry = results->Get(j)->GetTVEPGInfoTag();
          if (epgentry)
          {
            if (epgentry->Title() != PVRRecordings[i].Title())
              continue;
            if (epgentry->PlotOutline() != PVRRecordings[i].PlotOutline())
              continue;
            if (epgentry->Plot() != PVRRecordings[i].Plot())
              continue;

            results->Remove(j);
            j--;
          }
        }
      }
    }

    if (filter.m_IgnPresentTimers && PVRTimers.size() > 0)
    {
      for (unsigned int i = 0; i < PVRTimers.size(); i++)
      {
        unsigned int size = results->Size();
        for (unsigned int j = 0; j < size; j++)
        {
          const CTVEPGInfoTag *epgentry = results->Get(j)->GetTVEPGInfoTag();
          if (epgentry)
          {
            if (epgentry->Channel() != PVRTimers[i].Number())
              continue;
            if (epgentry->Start() < PVRTimers[i].Start())
              continue;
            if (epgentry->End() > PVRTimers[i].Stop())
              continue;

            results->Remove(j);
            j--;
          }
        }
      }
    }
  }
  return results->Size();
}

int cPVREpgs::GetEPGChannel(unsigned int number, CFileItemList* results, bool radio)
{
  int cnt = 0;

  cPVRChannels *ch = !radio ? &PVRChannelsTV : &PVRChannelsRadio;
  cPVREpgsLock EpgsLock;
  cPVREpgs *s = (cPVREpgs *)EPGs(EpgsLock);
  if (s)
  {
    const vector<CTVEPGInfoTag> *ch_epg = s->GetEPG(&ch->at(number-1), true)->InfoTags();
    for (unsigned int i = 0; i < ch_epg->size(); i++)
    {
      CFileItemPtr channel(new CFileItem(ch_epg->at(i)));
      channel->SetLabel2(ch_epg->at(i).Start().GetAsLocalizedDateTime(false, false));
      results->Add(channel);
      cnt++;
    }
  }
  return cnt;
}

int cPVREpgs::GetEPGNow(CFileItemList* results, bool radio)
{
  int cnt = 0;

  cPVRChannels *ch = !radio ? &PVRChannelsTV : &PVRChannelsRadio;
  cPVREpgsLock EpgsLock;
  cPVREpgs *s = (cPVREpgs *)EPGs(EpgsLock);
  if (s)
  {
    for (unsigned int i = 0; i < ch->size(); i++)
    {
      if (ch->at(i).m_hide)
        continue;

      const CTVEPGInfoTag *epgnow = s->GetEPG(&ch->at(i), true)->GetInfoTagNow();

      if (!epgnow)
        continue;

      CFileItemPtr entry(new CFileItem(*epgnow));
      entry->SetLabel2(epgnow->Start().GetAsLocalizedTime("", false));
      entry->m_strPath = ch->at(i).Name();
      entry->SetThumbnailImage(ch->at(i).Icon());
      results->Add(entry);
      cnt++;
    }
  }
  return cnt;
}

int cPVREpgs::GetEPGNext(CFileItemList* results, bool radio)
{
  int cnt = 0;

  cPVRChannels *ch = !radio ? &PVRChannelsTV : &PVRChannelsRadio;
  cPVREpgsLock EpgsLock;
  cPVREpgs *s = (cPVREpgs *)EPGs(EpgsLock);
  if (s)
  {
    for (unsigned int i = 0; i < ch->size(); i++)
    {
      if (ch->at(i).m_hide)
        continue;

      const CTVEPGInfoTag *epgnext = s->GetEPG(&ch->at(i), true)->GetInfoTagNext();

      if (!epgnext)
        continue;

      CFileItemPtr entry(new CFileItem(*epgnext));
      entry->SetLabel2(epgnext->Start().GetAsLocalizedTime("", false));
      entry->m_strPath = ch->at(i).Name();
      entry->SetThumbnailImage(ch->at(i).Icon());
      results->Add(entry);
      cnt++;
    }
  }
  return cnt;
}

cPVREpg *cPVREpgs::AddEPG(long ChannelID)
{
  cPVREpg *p = (cPVREpg *)GetEPG(ChannelID);
  if (!p)
  {
    p = new cPVREpg(ChannelID);
    Add(p);
    cPVRChannelInfoTag *channel = cPVRChannels::GetByChannelIDFromAll(ChannelID);
    if (channel)
      channel->m_Epg = p;
  }
  return p;
}

const cPVREpg *cPVREpgs::GetEPG(long ChannelID) const
{
  for (unsigned int i = 0; i < size(); i++)
  {
    if (at(i).ChannelID() == ChannelID)
      return &at(i);
  }
  return NULL;
}

const cPVREpg *cPVREpgs::GetEPG(const cPVRChannelInfoTag *Channel, bool AddIfMissing) const
{
  // This is not very beautiful, but it dramatically speeds up the
  // "What's on now/next?" menus.
  static cPVREpg DummyEPG(-1);

  if (!Channel->m_Epg)
     Channel->m_Epg = GetEPG(Channel->ChannelID());

  if (!Channel->m_Epg)
     Channel->m_Epg = &DummyEPG;

  if (Channel->m_Epg == &DummyEPG && AddIfMissing)
  {
    cPVREpg *Epg = new cPVREpg(Channel->ChannelID());
    ((cPVREpgs *)this)->Add(Epg);
    Channel->m_Epg = Epg;
  }
  return Channel->m_Epg != &DummyEPG ? Channel->m_Epg: NULL;
}

CDateTime cPVREpgs::GetFirstEPGDate(bool radio/* = false*/)
{
  CDateTime first = CDateTime::GetCurrentDateTime();
  cPVRChannels *ch = !radio ? &PVRChannelsTV : &PVRChannelsRadio;
  cPVREpgsLock EpgsLock;
  cPVREpgs *s = (cPVREpgs *)EPGs(EpgsLock);
  if (s)
  {
    for (unsigned int i = 0; i < ch->size(); i++)
    {
      if (ch->at(i).m_hide)
        continue;

      const cPVREpg *Epg = s->GetEPG(&ch->at(i), true);
      const vector<CTVEPGInfoTag> *ch_epg = Epg->InfoTags();

      for (unsigned int j = 0; j < ch_epg->size(); j++)
      {
        if (ch_epg->at(j).Start() < first)
          first = ch_epg->at(j).Start();
      }
    }
  }
  return first;
}

CDateTime cPVREpgs::GetLastEPGDate(bool radio/* = false*/)
{
  CDateTime last = CDateTime::GetCurrentDateTime();
  cPVRChannels *ch = !radio ? &PVRChannelsTV : &PVRChannelsRadio;
  cPVREpgsLock EpgsLock;
  cPVREpgs *s = (cPVREpgs *)EPGs(EpgsLock);
  if (s)
  {
    for (unsigned int i = 0; i < ch->size(); i++)
    {
      if (ch->at(i).m_hide)
        continue;

      const cPVREpg *Epg = s->GetEPG(&ch->at(i), true);
      const vector<CTVEPGInfoTag> *ch_epg = Epg->InfoTags();

      for (unsigned int j = 0; j < ch_epg->size(); j++)
      {
        if (ch_epg->at(j).End() >= last)
          last = ch_epg->at(j).End();
      }
    }
  }
  return last;
}

void cPVREpgs::Add(cPVREpg *entry)
{
  push_back(*entry);
}

