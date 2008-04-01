#pragma once

#include "IFile.h"
#include "ILiveTV.h"
#include "CMythSession.h"
#include "VideoInfoTag.h"

class DllLibCMyth;

namespace XFILE
{


class CCMythFile 
  : public  IFile
  ,         ILiveTVInterface
  ,         IRecordable
  , private CCMythSession::IEventListener
{
public:
  CCMythFile();
  virtual ~CCMythFile();
  virtual bool          Open(const CURL& url, bool binary = true);
  virtual __int64       Seek(__int64 pos, int whence=SEEK_SET);
  virtual __int64       GetPosition();
  virtual __int64       GetLength();
  virtual int           Stat(const CURL& url, struct __stat64* buffer) { return -1; }
  virtual void          Close();
  virtual unsigned int  Read(void* buffer, __int64 size);
  virtual CStdString    GetContent() { return ""; }
  virtual bool          SkipNext();

  virtual bool          Delete(const CURL& url);
  virtual bool          Exists(const CURL& url);

  virtual ILiveTVInterface* GetLiveTV() {return (ILiveTVInterface*)this;}

  virtual bool           NextChannel();
  virtual bool           PrevChannel();

  virtual int            GetTotalTime();
  virtual int            GetStartTime();

  virtual bool           UpdateItem(CFileItem& item);

  virtual IRecordable*   GetRecordable() {return (IRecordable*)this;}

  virtual bool           CanRecord();
  virtual bool           IsRecording();
  virtual bool           Record(bool bOnOff);

protected:
  virtual void OnEvent(int event, const std::string& data);

  bool HandleEvents();
  bool ChangeChannel(int direction, const char* channel);

  bool SetupConnection(const CURL& url, bool control, bool event, bool database);
  bool SetupRecording(const CURL& url);
  bool SetupLiveTV(const CURL& url);
  bool SetupFile(const CURL& url);

  CStdString GetValue(char* str) { return m_session->GetValue(str); }

  CCMythSession*    m_session;
  DllLibCMyth*      m_dll;
  cmyth_conn_t      m_control;
  cmyth_database_t  m_database;
  cmyth_recorder_t  m_recorder;
  cmyth_proginfo_t  m_program;
  cmyth_file_t      m_file;
  CStdString        m_filename;
  CVideoInfoTag     m_infotag;

  CCriticalSection  m_section;
  std::queue<std::pair<int, std::string> > m_events;

  bool              m_recording;
};

}
