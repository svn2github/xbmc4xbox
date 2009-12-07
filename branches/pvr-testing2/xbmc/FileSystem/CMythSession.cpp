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

#include "DllLibCMyth.h"
#include "CMythSession.h"
#include "VideoInfoTag.h"
#include "AdvancedSettings.h"
#include "DateTime.h"
#include "FileItem.h"
#include "URL.h"
#include "StringUtils.h"
#include "utils/SingleLock.h"
#include "utils/log.h"
#include "utils/TimeUtils.h"

extern "C"
{
#include "cmyth/include/cmyth/cmyth.h"
#include "cmyth/include/refmem/refmem.h"
}

using namespace XFILE;
using namespace std;

#define MYTH_DEFAULT_PORT     6543
#define MYTH_DEFAULT_USERNAME "mythtv"
#define MYTH_DEFAULT_PASSWORD "mythtv"
#define MYTH_DEFAULT_DATABASE "mythconverg"

CCriticalSection       CCMythSession::m_section_session;
vector<CCMythSession*> CCMythSession::m_sessions;

void CCMythSession::CheckIdle()
{
  CSingleLock lock(m_section_session);

  vector<CCMythSession*>::iterator it;
  for (it = m_sessions.begin(); it != m_sessions.end(); )
  {
    CCMythSession* session = *it;
    if (session->m_timestamp + 5000 < CTimeUtils::GetTimeMS())
    {
      CLog::Log(LOGINFO, "%s - closing idle connection to MythTV backend: %s", __FUNCTION__, session->m_hostname.c_str());
      delete session;
      it = m_sessions.erase(it);
    }
    else
    {
      it++;
    }
  }
}

CCMythSession* CCMythSession::AquireSession(const CURL& url)
{
  CSingleLock lock(m_section_session);

  vector<CCMythSession*>::iterator it;
  for (it = m_sessions.begin(); it != m_sessions.end(); it++)
  {
    CCMythSession* session = *it;
    if (session->CanSupport(url))
    {
      m_sessions.erase(it);
      return session;
    }
  }
  return new CCMythSession(url);
}

void CCMythSession::ReleaseSession(CCMythSession* session)
{
  session->SetListener(NULL);
  session->m_timestamp = CTimeUtils::GetTimeMS();
  CSingleLock lock(m_section_session);
  m_sessions.push_back(session);
}

CDateTime CCMythSession::GetValue(cmyth_timestamp_t t)
{
  CDateTime result;
  if (t)
  {
    time_t time = m_dll->timestamp_to_unixtime(t);
    if (time)
      result = *localtime(&time); // Convert to local time
    m_dll->ref_release(t);
  }
  return result;
}

CStdString CCMythSession::GetValue(char *str)
{
  CStdString result;
  if (str)
  {
    result = str;
    m_dll->ref_release(str);
    result.Trim();
  }
  return result;
}

int CCMythSession::GetValue(int integer)
{
  int result;
  if(integer)
  {
    result = integer;
  }
  return result;
}

bool CCMythSession::UpdateItem(CFileItem &item, cmyth_proginfo_t info)
{
  if (!info)
    return false;

  CVideoInfoTag* tag = item.GetVideoInfoTag();

  tag->m_strAlbum       = GetValue(m_dll->proginfo_chansign(info));
  tag->m_strShowTitle   = GetValue(m_dll->proginfo_title(info));
  tag->m_strPlotOutline = GetValue(m_dll->proginfo_subtitle(info));
  tag->m_strPlot        = GetValue(m_dll->proginfo_description(info));
  tag->m_strGenre       = GetValue(m_dll->proginfo_category(info));

  if (tag->m_strPlot.Left(tag->m_strPlotOutline.length()) != tag->m_strPlotOutline && !tag->m_strPlotOutline.IsEmpty())
    tag->m_strPlot = tag->m_strPlotOutline + '\n' + tag->m_strPlot;

  tag->m_strOriginalTitle = tag->m_strShowTitle;

  tag->m_strTitle = tag->m_strAlbum;
  if (tag->m_strShowTitle.length() > 0)
    tag->m_strTitle += " : " + tag->m_strShowTitle;

  CDateTimeSpan span = GetValue(m_dll->proginfo_rec_end(info)) - GetValue(m_dll->proginfo_rec_start(info));
  StringUtils::SecondsToTimeString(span.GetSeconds() +
                                   span.GetMinutes() * 60 +
                                   span.GetHours() * 3600, tag->m_strRuntime, TIME_FORMAT_GUESS);

  tag->m_iSeason  = 0; /* set this so xbmc knows it's a tv show */
  tag->m_iEpisode = 0;

  item.m_strTitle = GetValue(m_dll->proginfo_chanstr(info));
  item.m_dateTime = GetValue(m_dll->proginfo_rec_start(info));
  item.m_dwSize   = m_dll->proginfo_length(info);

  if (m_dll->proginfo_rec_status(info) == RS_RECORDING)
  {
    tag->m_strStatus = "livetv";

    CStdString temp;

    temp = GetValue(m_dll->proginfo_chanicon(info));
    if (temp.length() > 0)
    {
      CURL url(item.m_strPath);
      url.SetFileName("files/channels/" + temp);
      item.SetThumbnailImage(url.Get());
    }

    temp = GetValue(m_dll->proginfo_chanstr(info));
    if (temp.length() > 0)
    {
      CURL url(item.m_strPath);
      url.SetFileName("channels/" + temp + ".ts");
      item.m_strPath = url.Get();
    }
    item.SetCachedVideoThumb();
  }

  return true;
}

CCMythSession::CCMythSession(const CURL& url)
{
  m_control   = NULL;
  m_event     = NULL;
  m_database  = NULL;
  m_hostname  = url.GetHostName();
  m_username  = url.GetUserName() == "" ? MYTH_DEFAULT_USERNAME : url.GetUserName();
  m_password  = url.GetPassWord() == "" ? MYTH_DEFAULT_PASSWORD : url.GetPassWord();
  m_port      = url.HasPort() ? url.GetPort() : MYTH_DEFAULT_PORT;
  m_timestamp = CTimeUtils::GetTimeMS();
  m_dll = new DllLibCMyth;
  m_dll->Load();
  if (m_dll->IsLoaded())
  {
    if (g_advancedSettings.m_logLevel >= LOG_LEVEL_DEBUG_SAMBA)
      m_dll->dbg_level(CMYTH_DBG_ALL);
    else if (g_advancedSettings.m_logLevel >= LOG_LEVEL_DEBUG)
      m_dll->dbg_level(CMYTH_DBG_DETAIL);
    else
      m_dll->dbg_level(CMYTH_DBG_ERROR);
  }
}

CCMythSession::~CCMythSession()
{
  Disconnect();
  delete m_dll;
  CLog::Log(LOGDEBUG, "CCMythSession destroyed");
}

bool CCMythSession::CanSupport(const CURL& url)
{
  if (m_hostname != url.GetHostName())
    return false;

  if (m_port != (url.HasPort() ? url.GetPort() : MYTH_DEFAULT_PORT))
    return false;

  if (m_username != (url.GetUserName() == "" ? MYTH_DEFAULT_USERNAME : url.GetUserName()))
    return false;

  if (m_password != (url.GetPassWord() == "" ? MYTH_DEFAULT_PASSWORD : url.GetPassWord()))
    return false;

  return true;
}

void CCMythSession::Process()
{
  char buf[128];
  int  next;

  struct timeval to;

  while (!m_bStop)
  {
    /* check if there are any new events */
    to.tv_sec = 0;
    to.tv_usec = 100000;
    if (m_dll->event_select(m_event, &to) <= 0)
      continue;

    next = m_dll->event_get(m_event, buf, sizeof(buf));
    buf[sizeof(buf) - 1] = 0;

    switch (next)
    {
    case CMYTH_EVENT_UNKNOWN:
      CLog::Log(LOGDEBUG, "%s - MythTV event UNKNOWN (error?)", __FUNCTION__);
      break;
    case CMYTH_EVENT_CLOSE:
      CLog::Log(LOGDEBUG, "%s - MythTV event EVENT_CLOSE", __FUNCTION__);
      break;
    case CMYTH_EVENT_RECORDING_LIST_CHANGE:
      CLog::Log(LOGDEBUG, "%s - MythTV event RECORDING_LIST_CHANGE", __FUNCTION__);
      break;
    case CMYTH_EVENT_SCHEDULE_CHANGE:
      CLog::Log(LOGDEBUG, "%s - MythTV event SCHEDULE_CHANGE", __FUNCTION__);
      break;
    case CMYTH_EVENT_DONE_RECORDING:
      CLog::Log(LOGDEBUG, "%s - MythTV event DONE_RECORDING", __FUNCTION__);
      break;
    case CMYTH_EVENT_QUIT_LIVETV:
      CLog::Log(LOGDEBUG, "%s - MythTV event QUIT_LIVETV", __FUNCTION__);
      break;
    case CMYTH_EVENT_LIVETV_CHAIN_UPDATE:
      CLog::Log(LOGDEBUG, "%s - MythTV event LIVETV_CHAIN_UPDATE: %s", __FUNCTION__, buf);
      break;
    case CMYTH_EVENT_SIGNAL:
      CLog::Log(LOGDEBUG, "%s - MythTV event SIGNAL", __FUNCTION__);
      break;
    case CMYTH_EVENT_ASK_RECORDING:
      CLog::Log(LOGDEBUG, "%s - MythTV event ASK_RECORDING", __FUNCTION__);
      break;
    }

    {
      CSingleLock lock(m_section);
      if (m_listener)
        m_listener->OnEvent(next, buf);
    }
  }
}

void CCMythSession::Disconnect()
{
  if (!m_dll || !m_dll->IsLoaded())
    return;

  StopThread();

  if (m_control)
    m_dll->ref_release(m_control);
  if (m_event)
    m_dll->ref_release(m_event);
  if (m_database)
    m_dll->ref_release(m_database);
}

cmyth_conn_t CCMythSession::GetControl()
{
  if (!m_control)
  {
    if (!m_dll->IsLoaded())
      return false;

    m_control = m_dll->conn_connect_ctrl((char*)m_hostname.c_str(), m_port, 16*1024, 4096);
    if (!m_control)
      CLog::Log(LOGERROR, "%s - unable to connect to server on %s:%d", __FUNCTION__, m_hostname.c_str(), m_port);
  }
  return m_control;
}

cmyth_database_t CCMythSession::GetDatabase()
{
  if (!m_database)
  {
    if (!m_dll->IsLoaded())
      return false;

    m_database = m_dll->database_init((char*)m_hostname.c_str(), (char*)MYTH_DEFAULT_DATABASE,
                                      (char*)m_username.c_str(), (char*)m_password.c_str());
    if (!m_database)
      CLog::Log(LOGERROR, "%s - unable to connect to database on %s:%d", __FUNCTION__, m_hostname.c_str(), m_port);
  }
  return m_database;
}

bool CCMythSession::SetListener(IEventListener *listener)
{
  if (!m_event && listener)
  {
    if (!m_dll->IsLoaded())
      return false;

    m_event = m_dll->conn_connect_event((char*)m_hostname.c_str(), m_port, 16*1024 , 4096);
    if (!m_event)
    {
      CLog::Log(LOGERROR, "%s - unable to connect to server on %s:%d", __FUNCTION__, m_hostname.c_str(), m_port);
      return false;
    }
    /* start event handler thread */
    CThread::Create(false, THREAD_MINSTACKSIZE);
  }
  CSingleLock lock(m_section);
  m_listener = listener;
  return true;
}

DllLibCMyth* CCMythSession::GetLibrary()
{
  if (m_dll->IsLoaded())
    return m_dll;
  return NULL;
}
