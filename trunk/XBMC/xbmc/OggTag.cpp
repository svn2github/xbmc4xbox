
#include "stdafx.h"
#include "oggtag.h"
#include "util.h"

using namespace MUSIC_INFO;

COggTag::COggTag()
{
  // dll stuff
  ZeroMemory(&m_dll, sizeof(OGGdll));
  m_bDllLoaded = false;
}

COggTag::~COggTag()
{
  if (m_bDllLoaded)
    CSectionLoader::UnloadDLL(OGG_DLL);
}

bool COggTag::ReadTag(const CStdString& strFile1)
{
  if (!LoadDLL())
    return false;

  CStdString strFile=strFile1;
  int currentStream=0;

  m_musicInfoTag.SetURL(strFile);

  CStdString strExtension;
  CUtil::GetExtension(strFile, strExtension);
  if (strExtension==".oggstream")
  {
    CStdString strFileName=CUtil::GetFileName(strFile);
    int iStart=strFileName.ReverseFind("-")+1;
    currentStream = atoi(strFileName.substr(iStart, strFileName.size()-iStart-10).c_str())-1;
    CStdString strPath=strFile;
    CUtil::GetDirectory(strPath, strFile);
    if (CUtil::HasSlashAtEnd(strFile))
      strFile.Delete(strFile.size()-1);
  }

  CFile file;
  if (!file.Open(strFile.c_str()))
    return false;

  //  setup ogg i/o callbacks
  ov_callbacks oggIOCallbacks;
  oggIOCallbacks.read_func=ReadCallback;
  oggIOCallbacks.seek_func=SeekCallback;
  oggIOCallbacks.tell_func=TellCallback;
  oggIOCallbacks.close_func=CloseCallback;

  OggVorbis_File vf;
  //  open ogg file with decoder
  if (m_dll.ov_open_callbacks(&file, &vf, NULL, 0, oggIOCallbacks)!=0)
    return false;

  int iStreams=m_dll.ov_streams(&vf);
  if (iStreams>1)
  {
    if (currentStream > iStreams)
    {
      m_dll.ov_clear(&vf);
      return false;
    }
  }

  m_musicInfoTag.SetDuration((int)m_dll.ov_time_total(&vf, currentStream));

  vorbis_comment* pComments=m_dll.ov_comment(&vf, currentStream);
  if (pComments)
  {
    for (int i=0; i<pComments->comments; ++i)
    {
      CStdString strEntry=pComments->user_comments[i];
      ParseTagEntry(strEntry);
    }
  }
  m_dll.ov_clear(&vf);
  return true;
}

bool COggTag::LoadDLL()
{
  if (m_bDllLoaded)
    return true;

  DllLoader* pDll=CSectionLoader::LoadDLL(OGG_DLL);

  pDll->ResolveExport("ov_clear", (void**)&m_dll.ov_clear);
  pDll->ResolveExport("ov_open", (void**)&m_dll.ov_open);
  pDll->ResolveExport("ov_open_callbacks", (void**)&m_dll.ov_open_callbacks);

  pDll->ResolveExport("ov_test", (void**)&m_dll.ov_test);
  pDll->ResolveExport("ov_test_callbacks", (void**)&m_dll.ov_test_callbacks);
  pDll->ResolveExport("ov_test_open", (void**)&m_dll.ov_test_open);

  pDll->ResolveExport("ov_bitrate", (void**)&m_dll.ov_bitrate);
  pDll->ResolveExport("ov_bitrate_instant", (void**)&m_dll.ov_bitrate_instant);
  pDll->ResolveExport("ov_streams", (void**)&m_dll.ov_streams);
  pDll->ResolveExport("ov_seekable", (void**)&m_dll.ov_seekable);
  pDll->ResolveExport("ov_serialnumber", (void**)&m_dll.ov_serialnumber);

  pDll->ResolveExport("ov_raw_total", (void**)&m_dll.ov_raw_total);
  pDll->ResolveExport("ov_pcm_total", (void**)&m_dll.ov_pcm_total);
  pDll->ResolveExport("ov_time_total", (void**)&m_dll.ov_time_total);

  pDll->ResolveExport("ov_raw_seek", (void**)&m_dll.ov_raw_seek);
  pDll->ResolveExport("ov_pcm_seek", (void**)&m_dll.ov_pcm_seek);
  pDll->ResolveExport("ov_pcm_seek_page", (void**)&m_dll.ov_pcm_seek_page);
  pDll->ResolveExport("ov_time_seek", (void**)&m_dll.ov_time_seek);
  pDll->ResolveExport("ov_time_seek_page", (void**)&m_dll.ov_time_seek_page);

  pDll->ResolveExport("ov_raw_tell", (void**)&m_dll.ov_raw_tell);
  pDll->ResolveExport("ov_pcm_tell", (void**)&m_dll.ov_pcm_tell);
  pDll->ResolveExport("ov_time_tell", (void**)&m_dll.ov_time_tell);

  pDll->ResolveExport("ov_info", (void**)&m_dll.ov_info);
  pDll->ResolveExport("ov_comment", (void**)&m_dll.ov_comment);

  pDll->ResolveExport("ov_read", (void**)&m_dll.ov_read);

  // Check resolves
  if (!m_dll.ov_clear || !m_dll.ov_open || !m_dll.ov_open_callbacks || 
      !m_dll.ov_test || !m_dll.ov_test_callbacks || !m_dll.ov_test_open || 
      !m_dll.ov_bitrate || !m_dll.ov_bitrate_instant || !m_dll.ov_streams || 
      !m_dll.ov_seekable ||   !m_dll.ov_serialnumber || !m_dll.ov_raw_total || 
      !m_dll.ov_pcm_total || !m_dll.ov_time_total || !m_dll.ov_raw_seek || 
      !m_dll.ov_pcm_seek || !m_dll.ov_pcm_seek_page || !m_dll.ov_time_seek || 
      !m_dll.ov_time_seek_page || !m_dll.ov_raw_tell || !m_dll.ov_pcm_tell || 
      !m_dll.ov_time_tell || !m_dll.ov_info || !m_dll.ov_comment || !m_dll.ov_read) 
  {
    CLog::Log(LOGERROR, "OGGCodec: Unable to load our dll %s", OGG_DLL);
    return false;
  }

  m_bDllLoaded = true;
  return true;
}

size_t COggTag::ReadCallback(void *ptr, size_t size, size_t nmemb, void *datasource)
{
  CFile* pFile=(CFile*)datasource;
  if (!pFile)
    return 0;

  return pFile->Read(ptr, size*nmemb);
}

int COggTag::SeekCallback(void *datasource, ogg_int64_t offset, int whence)
{
  CFile* pFile=(CFile*)datasource;
  if (!pFile)
    return 0;

  return (int)pFile->Seek(offset, whence);
}

int COggTag::CloseCallback(void *datasource)
{
  CFile* pFile=(CFile*)datasource;
  if (!pFile)
    return 0;

  pFile->Close();
  return 1;
}

long COggTag::TellCallback(void *datasource)
{
  CFile* pFile=(CFile*)datasource;
  if (!pFile)
    return 0;

  return (long)pFile->GetPosition();
}

int COggTag::GetStreamCount(const CStdString& strFile)
{
  if (!LoadDLL())
    return 0;

  CFile file;
  if (!file.Open(strFile.c_str()))
    return 0;

  //  setup ogg i/o callbacks
  ov_callbacks oggIOCallbacks;
  oggIOCallbacks.read_func=ReadCallback;
  oggIOCallbacks.seek_func=SeekCallback;
  oggIOCallbacks.tell_func=TellCallback;
  oggIOCallbacks.close_func=CloseCallback;

  OggVorbis_File vf;
  //  open ogg file with decoder
  if (m_dll.ov_open_callbacks(&file, &vf, NULL, 0, oggIOCallbacks)!=0)
    return 0;

  int iStreams=m_dll.ov_streams(&vf);

  m_dll.ov_clear(&vf);

  return iStreams;
}
