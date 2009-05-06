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

#include "stdafx.h"
#include "HTSPDirectory.h"
#include "URL.h"
#include "Util.h"
#include "FileItem.h"
#include "Settings.h"
#include "cores/dvdplayer/DVDInputStreams/DVDInputStreamHTSP.h"

extern "C" {
#include "lib/libhts/htsmsg.h"
#include "lib/libhts/htsmsg_binary.h"
}

using namespace XFILE;
using namespace DIRECTORY;
using namespace HTSP;

struct SSession
{
  SSession()
  {
    session = NULL;
    refs    = 0;
  }

  std::string            hostname;
  int                    port;
  std::string            username;
  std::string            password;
  CHTSPDirectorySession* session;
  int                    refs;
  DWORD                  last;
};

struct STimedOut
{
  STimedOut(DWORD idle) : m_idle(idle) 
  { 
    m_time = GetTickCount(); 
  }
  bool operator()(SSession& data)
  {
    return data.refs == 0 && data.last + m_idle < m_time;
  }
  DWORD m_idle;
  DWORD m_time;
};

typedef std::vector<SSession> SSessions;


static SSessions         g_sessions;
static CCriticalSection  g_section;


CHTSPDirectorySession::CHTSPDirectorySession()
{
}

CHTSPDirectorySession::~CHTSPDirectorySession()
{
  Close();
}

CHTSPDirectorySession* CHTSPDirectorySession::Acquire(const CURL& url)
{
  CSingleLock lock(g_section);

  for(SSessions::iterator it = g_sessions.begin(); it != g_sessions.end(); it++)
  {
    if(it->hostname == url.GetHostName()
    && it->port     == url.GetPort()
    && it->username == url.GetUserName()
    && it->password == url.GetPassWord())
    {
      it->refs++;
      return it->session;
    }
  }

  CHTSPDirectorySession* session = new CHTSPDirectorySession();
  if(session->Open(url))
  {
    SSession data;
    data.hostname = url.GetHostName();
    data.port     = url.GetPort();
    data.username = url.GetUserName();
    data.password = url.GetPassWord();
    data.session  = session;
    data.refs     = 1;
    g_sessions.push_back(data);
    return session;
  }

  delete session;
  return NULL;
}

void CHTSPDirectorySession::Release(CHTSPDirectorySession* &session)
{
  if(session == NULL)
    return;

  CSingleLock lock(g_section);
  for(SSessions::iterator it = g_sessions.begin(); it != g_sessions.end(); it++)
  {
    if(it->session == session)
    {
      it->refs--;
      it->last = GetTickCount();
      return;
    }
  }
  CLog::Log(LOGERROR, "CHTSPDirectorySession::Release - release of invalid session");
  ASSERT(0);
}

void CHTSPDirectorySession::CheckIdle(DWORD idle)
{
  CSingleLock lock(g_section);
  SSessions::iterator it2 = remove_if(g_sessions.begin(), g_sessions.end(), STimedOut(idle));
  for(SSessions::iterator it = it2; it != g_sessions.end(); it++)
  {
    CLog::Log(LOGINFO, "CheckIdle - Closing session to htsp://%s:%i", it->hostname.c_str(), it->port);
    delete it->session;
  }
  g_sessions.erase(it2, g_sessions.end());
}

bool CHTSPDirectorySession::Open(const CURL& url)
{
  if(!m_session.Connect(url.GetHostName(), url.GetPort()))
    return false;

  if(m_session.GetProtocol() < 2)
  {
    CLog::Log(LOGERROR, "CHTSPDirectory::GetDirectory - incompatible protocol version %d", m_session.GetProtocol());
    return false;
  }

  if(!url.GetUserName().IsEmpty())
    m_session.Auth(url.GetUserName(), url.GetPassWord());

  Create();

  m_started.WaitMSec(30000);
  return true;
}

void CHTSPDirectorySession::Close()
{
  m_bStop = true;
  m_session.Abort();
  StopThread();
}

htsmsg_t* CHTSPDirectorySession::ReadResult(htsmsg_t* m)
{
  CSingleLock lock(m_section);
  unsigned    seq (m_session.AddSequence());

  SMessage &message(m_queue[seq]);
  message.event = new CEvent();
  message.msg   = NULL;

  lock.Leave();
  htsmsg_add_u32(m, "seq", seq);
  if(!m_session.SendMessage(m))
  {
    m_queue.erase(seq);
    return NULL;
  }

  if(!message.event->WaitMSec(2000))
    CLog::Log(LOGERROR, "CHTSPDirectorySession::ReadResult - Timeout waiting for response");
  lock.Enter();

  m =    message.msg;
  delete message.event;

  m_queue.erase(seq);

  return m;
}

bool CHTSPDirectorySession::GetEvent(SEvent& event, uint32_t id)
{
  if(id == 0)
  {
    event.Clear();
    return false;
  }

  SEvents::iterator it = m_events.find(id);
  if(it != m_events.end())
  {
    event = it->second;
    return true;
  }

  htsmsg_t *msg = htsmsg_create_map();
  htsmsg_add_str(msg, "method", "getEvent");
  htsmsg_add_u32(msg, "eventId", id);
  if((msg = ReadResult(msg)) == NULL)
  {
    CLog::Log(LOGDEBUG, "CHTSPSession::GetEvent - failed to get event %u", id);
    return false;
  }
  if(!CHTSPSession::ParseEvent(msg, id, event))
    return false;

  m_events[id] = event;
  return true;
}

void CHTSPDirectorySession::Process()
{
  CLog::Log(LOGDEBUG, "CHTSPDirectorySession::Process() - Starting");
  m_session.SendEnableAsync();

  htsmsg_t* msg;

  while(!m_bStop)
  {
    if((msg = m_session.ReadMessage()) == NULL)
      continue;

    uint32_t seq;
    if(htsmsg_get_u32(msg, "seq", &seq) == 0)
    {
      CSingleLock lock(m_section);
      SMessages::iterator it = m_queue.find(seq);
      if(it != m_queue.end())
      {
        it->second.msg = msg;
        it->second.event->Set();
        continue;
      }
    }

    const char* method;
    if((method = htsmsg_get_str(msg, "method")) == NULL)
    {
      htsmsg_destroy(msg);
      continue;
    }

    if     (strstr(method, "channelAdd"))
      CHTSPSession::ParseChannelUpdate(msg, m_channels);
    else if(strstr(method, "channelUpdate"))
      CHTSPSession::ParseChannelUpdate(msg, m_channels);
    else if(strstr(method, "channelRemove"))
      CHTSPSession::ParseChannelRemove(msg, m_channels);
    if     (strstr(method, "tagAdd"))
      CHTSPSession::ParseTagUpdate(msg, m_tags);
    else if(strstr(method, "tagUpdate"))
      CHTSPSession::ParseTagUpdate(msg, m_tags);
    else if(strstr(method, "tagRemove"))
      CHTSPSession::ParseTagRemove(msg, m_tags);
    else if(strstr(method, "initialSyncCompleted"))
      m_started.Set();

    htsmsg_destroy(msg);
  }

  m_session.Close();
  CLog::Log(LOGDEBUG, "CHTSPDirectorySession::Process() - Exiting");
}

SChannels CHTSPDirectorySession::GetChannels()
{
  return GetChannels(0);
}

SChannels CHTSPDirectorySession::GetChannels(int tag)
{
  CSingleLock lock(m_section);
  if(tag == 0)
    return m_channels;

  STags::iterator it = m_tags.find(tag);
  if(it == m_tags.end())
  {
    SChannels channels;
    return channels;
  }
  return GetChannels(it->second);
}

SChannels CHTSPDirectorySession::GetChannels(STag& tag)
{
  CSingleLock lock(m_section);
  SChannels channels;

  std::vector<int>::iterator it;
  for(it = tag.channels.begin(); it != tag.channels.end(); it++)
  {
    SChannels::iterator it2 = m_channels.find(*it);
    if(it2 == m_channels.end())
    {
      CLog::Log(LOGERROR, "CHTSPDirectorySession::GetChannels - tag points to unknown channel %d", *it);
      continue;
    }
    channels[*it] = it2->second;
  }
  return channels;
}


STags CHTSPDirectorySession::GetTags()
{
  CSingleLock lock(m_section);
  return m_tags;
}

CHTSPDirectory::CHTSPDirectory(void)
{
  m_session = NULL;
}

CHTSPDirectory::~CHTSPDirectory(void)
{
  CHTSPDirectorySession::Release(m_session);
}


bool CHTSPDirectory::GetChannels(const CURL &base, CFileItemList &items)
{
  SChannels channels = m_session->GetChannels();
  return GetChannels(base, items, channels, 0);
}

bool CHTSPDirectory::GetChannels( const CURL &base
                                , CFileItemList &items
                                , SChannels channels
                                , int tag)
{
  CURL url(base);

  SEvent event;

  for(SChannels::iterator it = channels.begin(); it != channels.end(); it++)
  {
    if(!m_session->GetEvent(event, it->second.event))
      event.Clear();

    CFileItemPtr item(new CFileItem("", true));

    url.SetFileName("");
    url.GetURL(item->m_strPath);
    CHTSPSession::ParseItem(it->second, tag, event, *item);
    item->m_bIsFolder = false;
    item->m_strTitle.Format("%d", it->second.id);

    items.Add(item);
  }

  if (g_guiSettings.GetBool("filelists.ignorethewhensorting"))
    items.AddSortMethod(SORT_METHOD_LABEL_IGNORE_THE, 551, LABEL_MASKS("%K[ - %B]", "%Z", "%L", ""));
  else
    items.AddSortMethod(SORT_METHOD_LABEL, 551, LABEL_MASKS("%K[ - %B]", "%Z", "%L", ""));

  if (g_guiSettings.GetBool("filelists.ignorethewhensorting"))
    items.AddSortMethod(SORT_METHOD_LABEL_IGNORE_THE, 20364, LABEL_MASKS("%Z", "%B", "%L", ""));
  else
    items.AddSortMethod(SORT_METHOD_LABEL, 20364, LABEL_MASKS("%Z", "%B", "%L", ""));

  return channels.size() > 0;

}

bool CHTSPDirectory::GetTag(const CURL &base, CFileItemList &items)
{
  CURL url(base);

  int id = atoi(url.GetFileName().Mid(5));

  SChannels channels = m_session->GetChannels(id);
  if(channels.size() == 0)
    return false;

  return GetChannels(base, items, channels, id);
}


bool CHTSPDirectory::GetDirectory(const CStdString& strPath, CFileItemList &items)
{
  CURL                    url(strPath);

  CHTSPDirectorySession::Release(m_session);
  if(!(m_session = CHTSPDirectorySession::Acquire(url)))
    return false;


  if(url.GetFileName().IsEmpty())
  {
    CFileItemPtr item;

    item.reset(new CFileItem("", true));
    url.SetFileName("tags/0/");
    url.GetURL(item->m_strPath);
    item->SetLabel(g_localizeStrings.Get(22018));
    item->SetLabelPreformated(true);
    items.Add(item);

    STags tags = m_session->GetTags();
    CStdString filename, label;
    for(STags::iterator it = tags.begin(); it != tags.end(); it++)
    {
      filename.Format("tags/%d/", it->second.id);
      label.Format("Tag: %s", it->second.name);

      item.reset(new CFileItem("", true));
      url.SetFileName(filename);
      url.GetURL(item->m_strPath);
      item->SetLabel(label);
      item->SetLabelPreformated(true);
      item->SetThumbnailImage(it->second.icon);
      items.Add(item);
    }

    return true;
  }
  else if(url.GetFileName().Left(5) == "tags/")
    return GetTag(url, items);
  return false;
}
