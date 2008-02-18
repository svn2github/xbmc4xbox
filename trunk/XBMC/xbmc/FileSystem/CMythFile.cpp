#include "stdafx.h"
#include "CMythFile.h"
#include "Util.h"
#include "DllLibCMyth.h"

extern "C" {
#include "../lib/libcmyth/cmyth.h"
#include "../lib/libcmyth/mvp_refmem.h"
}

using namespace XFILE;

static void prog_update_callback(cmyth_proginfo_t prog)
{
  CLog::Log(LOGDEBUG, "%s - prog_update_callback", __FUNCTION__);
}


bool CCMythFile::HandleEvents()
{
  char buf[128];
  cmyth_event_t next;

  struct timeval to;
  to.tv_sec = 0;
  to.tv_usec = 0;

  while(true)
  {
    /* check if there are any new events */
    if(m_dll->event_select(m_event, &to) <= 0)
      return false;

    next = m_dll->event_get(m_event, buf, 128);
    switch (next) {
    case CMYTH_EVENT_UNKNOWN:
      CLog::Log(LOGDEBUG, "%s - MythTV unknown event (error?)", __FUNCTION__);
      break;
    case CMYTH_EVENT_CLOSE:
      CLog::Log(LOGDEBUG, "%s - Event socket closed, shutting down myth", __FUNCTION__);
      break;
    case CMYTH_EVENT_RECORDING_LIST_CHANGE:
      CLog::Log(LOGDEBUG, "%s - MythTV event RECORDING_LIST_CHANGE", __FUNCTION__);
      if (m_programlist)
        m_dll->ref_release(m_programlist);
//      m_programlist = m_dll->proglist_get_all_recorded(m_control);
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
    {
      CLog::Log(LOGDEBUG, "%s - MythTV event %s", __FUNCTION__,buf);
      char * pfx = "LIVETV_CHAIN UPDATE ";
      char * chainid = buf + strlen(pfx);
      m_dll->livetv_chain_update(m_recorder, chainid, 4096);
      break;
    }
    case CMYTH_EVENT_SIGNAL:
      CLog::Log(LOGDEBUG, "%s - MythTV event SIGNAL", __FUNCTION__);
      break;
    }
  }
}

bool CCMythFile::Open(const CURL& url, bool binary)
{
  if(!binary)
    return false;
  
  if(!m_dll->IsLoaded())
    return false;

  Close();

  m_dll->dbg_level(CMYTH_DBG_DETAIL);

  int port = url.GetPort();
  if(port == 0)
    port = 6543;

  m_control = m_dll->conn_connect_ctrl((char*)url.GetHostName().c_str(), port, 16*1024, 4096);
  if(!m_control)
  {
    CLog::Log(LOGERROR, "%s - unable to connect to server %s, port %d", __FUNCTION__, url.GetHostName().c_str(), port);
    return false;
  }

  m_event = m_dll->conn_connect_event((char*)url.GetHostName().c_str(), port, 16*1024, 4096);
  if(!m_event)
  {
    CLog::Log(LOGERROR, "%s - unable to connect to server %s, port %d", __FUNCTION__, url.GetHostName().c_str(), port);
    return false;
  }

  CStdString path(url.GetFileName());

  if(path.Left(11) == "recordings/")
  {
    CStdString file = path.Mid(11);

    m_programlist = m_dll->proglist_get_all_recorded(m_control);
    if(!m_programlist)
    {
      CLog::Log(LOGERROR, "%s - unable to get list of recordings", __FUNCTION__);
      return false;
    }
    int count = m_dll->proglist_get_count(m_programlist);
    for(int i=0; i<count; i++)
    {
      cmyth_proginfo_t program = m_dll->proglist_get_item(m_programlist, i);
      if(program)
      {
        char* path = m_dll->proginfo_pathname(program);
        if(path)
        {
          CStdString recording = CUtil::GetFileName(path);
          if(file == recording)
            m_program = (cmyth_proginfo_t)m_dll->ref_hold(program);
          m_dll->ref_release(path);
        }
        m_dll->ref_release(program);
      }
      if(m_program)
        break;
    }

    if(!m_program)
    {
      CLog::Log(LOGERROR, "%s - unable to get find selected file", __FUNCTION__);
      return false;
    }

    m_file = m_dll->conn_connect_file(m_program, m_control, 16*1024, 4096);
    if(!m_file)
    {
      CLog::Log(LOGERROR, "%s - unable to connect to file", __FUNCTION__);
      return false;
    }
    CLog::Log(LOGDEBUG, "%s - file: size %lld, start %lld, ", __FUNCTION__,  m_dll->file_length(m_file), m_dll->file_start(m_file));
  } 
  else if (path.Left(9) == "channels/")
  {
    CStdString channel = path.Mid(9);
    if(!CUtil::GetExtension(channel).Equals(".ts"))
    {
      CLog::Log(LOGERROR, "%s - invalid channel url %s", __FUNCTION__, channel.c_str());
      return false;
    }
    CUtil::RemoveExtension(channel);

    m_recorder = m_dll->conn_get_free_recorder(m_control);
    if(!m_recorder)
    {
      CLog::Log(LOGERROR, "%s - unable to get recorder", __FUNCTION__);
      return false;
    }

    if(!m_dll->recorder_is_recording(m_recorder))
    {
      CLog::Log(LOGDEBUG, "%s - recorder isn't running, let's start it", __FUNCTION__);
      char* msg = NULL;
      if(!(m_recorder = m_dll->spawn_live_tv(m_recorder, 16*1024, 4096, prog_update_callback, &msg)))
      {
        CLog::Log(LOGERROR, "%s - unable to spawn live tv: %s", __FUNCTION__, msg ? msg : "");
        return false;
      }
    }

    if(!ChangeChannel(CHANNEL_DIRECTION_SAME, channel.c_str()))
      return false;

    if(!m_dll->recorder_is_recording(m_recorder))
    {
      CLog::Log(LOGERROR, "%s - recorder hasn't started", __FUNCTION__);
      return false;
    }
    m_filename = m_dll->recorder_get_filename(m_recorder);

    CLog::Log(LOGDEBUG, "%s - recorder has started on filename %s", __FUNCTION__, m_filename.c_str());

    m_program = m_dll->recorder_get_cur_proginfo(m_recorder);
    if(!m_program)
      CLog::Log(LOGWARNING, "%s - failed to get current program info", __FUNCTION__);
  }
  else
  {
    CLog::Log(LOGERROR, "%s - invalid path specified %s", __FUNCTION__, path.c_str());
    return false;
  }

  /* check for any events */
  HandleEvents();

  return true;
}


void CCMythFile::Close()
{
  if(m_program)
  {
    m_dll->ref_release(m_program);
    m_program = NULL;
  }
  if(m_programlist)
  {
    m_dll->ref_release(m_programlist);
    m_programlist = NULL;
  }
  if(m_recorder)
  {
    m_dll->recorder_stop_livetv(m_recorder);
    m_dll->ref_release(m_recorder);
    m_recorder = NULL;
  }
  if(m_file)
  {
    m_dll->ref_release(m_file);
    m_file = NULL;
  }
  if(m_control)
  {
    m_dll->ref_release(m_control);
    m_control = NULL;
  }
}

CCMythFile::CCMythFile()
{
  m_dll         = new DllLibCMyth();
  m_program     = NULL;
  m_programlist = NULL;
  m_recorder    = NULL;
  m_control     = NULL;
  m_file        = NULL;
  m_remain      = 0;
  m_dll->Load();
}

CCMythFile::~CCMythFile()
{
  Close();
  delete m_dll;
}

bool CCMythFile::Exists(const CURL& url)
{
  CStdString path(url.GetFileName());

  if(path.Left(11) == "recordings/")
  {

  }
  return false;
}

bool CCMythFile::Delete(const CURL& url) 
{ 
  CStdString path(url.GetFileName());

  if(path.Left(11) == "recordings/")
  {
  }
  return false;
}

__int64 CCMythFile::Seek(__int64 pos, int whence)
{
  CLog::Log(LOGDEBUG, "%s - seek to pos %lld, whence %d", __FUNCTION__, pos, whence);

  if(whence == SEEK_POSSIBLE)
  {
    if(m_recorder)
      return false;
    else
      return true;
  }

  if(m_recorder)
    return false; //m_dll->livetv_seek(m_recorder, pos, whence);
  else
    return m_dll->file_seek(m_file, pos, whence);

  return -1;
}

__int64 CCMythFile::GetPosition()
{
  if(m_recorder)
    return m_dll->livetv_seek(m_recorder, 0, SEEK_CUR);
  else
    return m_dll->file_seek(m_file, 0, SEEK_CUR);
  return -1;
}

__int64 CCMythFile::GetLength()
{
  if(m_file)
    return m_dll->file_length(m_file);
  return -1;
}

unsigned int CCMythFile::Read(void* buffer, __int64 size)
{ 
  /* check for any events */
  HandleEvents();

  struct timeval to;
  to.tv_sec = 10;
  to.tv_usec = 0;
  int ret;

  if(m_remain == 0)
  {
    if(m_recorder)
      ret = m_dll->livetv_request_block(m_recorder, (unsigned long)size);
    else
      ret = m_dll->file_request_block(m_file, (unsigned long)size);
    if(ret <= 0)
    {
      CLog::Log(LOGERROR, "%s - error requesting block of data (%d)", __FUNCTION__, ret);
      return 0;
    }
    m_remain = ret;
  }

  if(m_recorder)
    ret = m_dll->livetv_select(m_recorder, &to);
  else
    ret = m_dll->file_select(m_file, &to);
  if(ret <= 0)
  {
    CLog::Log(LOGERROR, "%s - timeout waiting for data (%d)", __FUNCTION__, ret);
    return 0;
  }

  size = min(m_remain, (int)size);
  if(m_recorder)
    ret = m_dll->livetv_get_block(m_recorder, (char*)buffer, (unsigned long)size);
  else
    ret = m_dll->file_get_block(m_file, (char*)buffer, (unsigned long)size);
  if(ret <= 0)
  {
    CLog::Log(LOGERROR, "%s - failed to retrieve block (%d)", __FUNCTION__, ret);
    return 0;
  }
  if(ret > m_remain)
  {
    CLog::Log(LOGERROR, "%s - received more data than we requested, probably a buffer overrun", __FUNCTION__);
    m_remain = 0;
    return 0;
  }
  m_remain -= ret;
  return ret;
}

bool CCMythFile::SkipNext()
{
  if(m_recorder)
    return m_dll->recorder_is_recording(m_recorder) > 0;

  return false;
}

CVideoInfoTag* CCMythFile::GetVideoInfoTag()
{
  if(m_program)
  {
    char *str;

    if((str = m_dll->proginfo_chanstr(m_program)))
    {
      m_infotag.m_strTitle = str;
      m_dll->ref_release(str);
    }

    if((str = m_dll->proginfo_title(m_program)))
    {
      m_infotag.m_strTitle    += " : ";
      m_infotag.m_strTitle     = str;
      m_infotag.m_strShowTitle = str;
      m_dll->ref_release(str);
    }

    if((str = m_dll->proginfo_description(m_program)))
    {
      m_infotag.m_strPlotOutline = str;
      m_infotag.m_strPlot        = str;
      m_dll->ref_release(str);
    }

    m_infotag.m_iSeason  = 1; /* set this so xbmc knows it's a tv show */
    m_infotag.m_iEpisode = 1;
    return &m_infotag;
  }
  return NULL;
}

int CCMythFile::GetTotalTime()
{
  if(m_program && m_recorder)
    return m_dll->proginfo_length_sec(m_program) * 1000;

  return -1;
}

int CCMythFile::GetStartTime()
{
  if(m_program && m_recorder)
  {
    cmyth_timestamp_t start = m_dll->proginfo_start(m_program);
    cmyth_timestamp_t end = m_dll->proginfo_rec_start(m_program);

    double diff = difftime(m_dll->timestamp_to_unixtime(end), m_dll->timestamp_to_unixtime(start));

    m_dll->ref_release(start);
    m_dll->ref_release(end);
    return (int)(diff * 1000);
  }
  return 0;
}

bool CCMythFile::ChangeChannel(int direction, const char* channel)
{
  if(m_dll->recorder_pause(m_recorder) < 0)
  {
    CLog::Log(LOGDEBUG, "%s - failed to pause recorder", __FUNCTION__);
    return false;
  }

  if(channel)
  {
    if(m_dll->recorder_set_channel(m_recorder, (char*)channel) < 0)
    {
      CLog::Log(LOGDEBUG, "%s - failed to change channel", __FUNCTION__);
      return false;
    }
  }
  else
  {
    if(m_dll->recorder_change_channel(m_recorder, (cmyth_channeldir_t)direction) < 0)
    {
      CLog::Log(LOGDEBUG, "%s - failed to change channel", __FUNCTION__);
      return false;
    }
  }

  if(m_program)
    m_dll->ref_release(m_program);
  m_program = m_dll->recorder_get_cur_proginfo(m_recorder);

  return true;
}

bool CCMythFile::NextChannel()
{
  return ChangeChannel(CHANNEL_DIRECTION_UP, NULL);
}

bool CCMythFile::PrevChannel()
{
  return ChangeChannel(CHANNEL_DIRECTION_DOWN, NULL);
}
