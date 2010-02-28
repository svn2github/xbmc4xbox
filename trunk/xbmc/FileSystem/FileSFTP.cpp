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


#include "FileSFTP.h"
#ifdef HAS_FILESYSTEM_SFTP
#include "utils/SingleLock.h"
#include "utils/log.h"
#include "utils/TimeUtils.h"
#include "Util.h"

using namespace XFILE;
using namespace std;

CSFTPSession::CSFTPSession(string host, string username, string password)
{
  CLog::Log(LOGINFO, "SFTPSession: Creating new session on host '%s' with user '%s'", host.c_str(), username.c_str());
  CSingleLock lock(m_critSect);
  if (!Connect(host, username, password))
    Disconnect();

  m_LastActive = CTimeUtils::GetTimeMS();
}

CSFTPSession::~CSFTPSession()
{
  CSingleLock lock(m_critSect);
  Disconnect();
}

SFTP_FILE *CSFTPSession::CreateFileHande(string file)
{
  if (m_connected)
  {
    CSingleLock lock(m_critSect);
    m_LastActive = CTimeUtils::GetTimeMS();
    SFTP_FILE *handle = sftp_open(m_sftp_session, file.c_str(), O_RDONLY, 0);
    if (handle)
    {
      sftp_file_set_blocking(handle);
      return handle;
    }
    else
      CLog::Log(LOGERROR, "SFTPSession: Was connected but couldn't create filehandle\n");
  }
  else
    CLog::Log(LOGERROR, "SFTPSession: Not connected and can't create file handle");

  return NULL;
}

void CSFTPSession::CloseFileHandle(SFTP_FILE *handle)
{
  CSingleLock lock(m_critSect);
  sftp_close(handle);
}

bool CSFTPSession::GetDirectory(string base, string folder, CFileItemList &items)
{
  if (m_connected)
  {
    SFTP_DIR *dir = NULL;

    {
      CSingleLock lock(m_critSect);
      m_LastActive = CTimeUtils::GetTimeMS();
      dir = sftp_opendir(m_sftp_session, folder.size() == 0 ? "./" : folder.c_str());
    }

    if (dir)
    {
      bool read = true;
      while (read)
      {
        SFTP_ATTRIBUTES *attributes = NULL;

        {
          CSingleLock lock(m_critSect);
          read = sftp_dir_eof(dir) == 0;
          attributes = sftp_readdir(m_sftp_session, dir);
        }

        if (attributes && (strcmp(attributes->name, "..") == 0 || strcmp(attributes->name, ".") == 0))
          continue;
        if (attributes)
        {
          CStdString localPath = folder;
          localPath.append(attributes->name);

          if (attributes->type == SSH_FILEXFER_TYPE_SYMLINK)
          {
            CSingleLock lock(m_critSect);
            sftp_attributes_free(attributes);
            char *realpath = sftp_readlink(m_sftp_session, localPath.c_str());
            attributes = sftp_stat(m_sftp_session, realpath);
            free(realpath);
            if (attributes == NULL)
              continue;
          }

          CFileItemPtr pItem(new CFileItem);
          pItem->SetLabel(attributes->name);

          if (attributes->type & SSH_FILEXFER_TYPE_DIRECTORY)
          {
            localPath.append("/");
            pItem->m_bIsFolder = true;
          }

          pItem->m_strPath = base;
          pItem->m_strPath += localPath;
          items.Add(pItem);

          {
            CSingleLock lock(m_critSect);
            sftp_attributes_free(attributes);
          }
        }
        else
          read = false;
      }

      {
        CSingleLock lock(m_critSect);
        sftp_closedir(dir);
      }

      return true;
    }
  }
  else
    CLog::Log(LOGERROR, "SFTPSession: Not connected, can't list directory");

  return false;
}

bool CSFTPSession::Exists(const char *path)
{
  CSingleLock lock(m_critSect);

  SFTP_ATTRIBUTES *attributes = sftp_stat(m_sftp_session, path);
  bool exists = attributes != NULL;

  if (attributes)
    sftp_attributes_free(attributes);

  return exists;
}

int CSFTPSession::Stat(const char *path, struct __stat64* buffer)
{
  CSingleLock lock(m_critSect);
  memset(buffer, 0, sizeof (buffer));
  m_LastActive = CTimeUtils::GetTimeMS();
  SFTP_ATTRIBUTES *attributes = sftp_stat(m_sftp_session, path);

  if (attributes)
  {
    buffer->st_size = attributes->size;
    buffer->st_mtime = attributes->mtime;
    buffer->st_atime = attributes->atime;

    sftp_attributes_free(attributes);
    return 0;
  }
  else
  {
    CLog::Log(LOGERROR, "SFTPSession: STAT - Failed to get attributes");
    return -1;
  }
}

void CSFTPSession::Seek(SFTP_FILE *handle, u64 position)
{
  CSingleLock lock(m_critSect);
  m_LastActive = CTimeUtils::GetTimeMS();
  sftp_seek64(handle, position);
}

int CSFTPSession::Read(SFTP_FILE *handle, void *buffer, int64_t length)
{
  CSingleLock lock(m_critSect);
  m_LastActive = CTimeUtils::GetTimeMS();
  return sftp_read(handle, buffer, length);
}

int64_t CSFTPSession::GetPosition(SFTP_FILE *handle)
{
  CSingleLock lock(m_critSect);
  m_LastActive = CTimeUtils::GetTimeMS();
  return sftp_tell64(handle);
}

bool CSFTPSession::IsIdle()
{
  return (CTimeUtils::GetTimeMS() - m_LastActive) > 90000;
}

bool CSFTPSession::VerifyKnownHost(ssh_session *session)
{
  switch (ssh_is_server_known(session))
  {
    case SSH_SERVER_KNOWN_OK:
      return true;
    case SSH_SERVER_KNOWN_CHANGED:
      CLog::Log(LOGERROR, "SFTPSession: Server that was known has changed");
      return false;
    case SSH_SERVER_FOUND_OTHER:
      CLog::Log(LOGERROR, "SFTPSession: The host key for this server was not found but an other type of key exists. An attacker might change the default server key to confuse your client into thinking the key does not exist");
      return false;
    case SSH_SERVER_FILE_NOT_FOUND:
      CLog::Log(LOGINFO, "SFTPSession: Server file was not found, creating a new one");
    case SSH_SERVER_NOT_KNOWN:
      CLog::Log(LOGINFO, "SFTPSession: Server unkown, we trust it for now");
      if (ssh_write_knownhost(session) < 0)
      {
        CLog::Log(LOGERROR, "CSFTPSession: Failed to save host '%s'", strerror(errno));
        return false;
      }

      return true;
    case SSH_SERVER_ERROR:
      CLog::Log(LOGERROR, "SFTPSession: Failed to verify host '%s'", ssh_get_error(session));
      return false;
  }

  return false;
}

bool CSFTPSession::Connect(string host, string username, string password)
{
  m_connected     = false;
  m_session       = NULL;
  m_sftp_session  = NULL;

  m_session=ssh_new();
  if (m_session == NULL)
  {
    CLog::Log(LOGERROR, "SFTPSession: Failed to initialize session");
    return false;
  }

  SSH_OPTIONS *options = ssh_options_new();

  if (ssh_options_set_username(options, username.c_str()) < 0)
  {
    CLog::Log(LOGERROR, "SFTPSession: Failed to set username '%s' for session", username.c_str());
    return false;
  }

  if (ssh_options_set_host(options, host.c_str()) < 0)
  {
    CLog::Log(LOGERROR, "SFTPSession: Failed to set host '%s' for session", host.c_str());
    return false;
  }

  ssh_options_set_log_verbosity(options, 0);

  ssh_set_options(m_session, options);

  if(ssh_connect(m_session))
  {
    CLog::Log(LOGERROR, "SFTPSession: Failed to connect '%s'", ssh_get_error(m_session));
    return false;
  }

  if (!VerifyKnownHost(m_session))
    return false;

  int noAuth = SSH_AUTH_DENIED;
  if ((noAuth = ssh_userauth_none(m_session, NULL)) == SSH_AUTH_ERROR)
  {
    CLog::Log(LOGERROR, "SFTPSession: Failed to authenticate via guest '%s'", ssh_get_error(m_session));
    return false;
  }

  int method = ssh_auth_list(m_session);

  // Try to authenticate with public key first
  int publicKeyAuth = SSH_AUTH_DENIED;
  if (method & SSH_AUTH_METHOD_PUBLICKEY && (publicKeyAuth = ssh_userauth_autopubkey(m_session, NULL)) == SSH_AUTH_ERROR)
  {
    CLog::Log(LOGERROR, "SFTPSession: Failed to authenticate via publickey '%s'", ssh_get_error(m_session));
    return false;
  }

  // Try to authenticate with password
  int passwordAuth = SSH_AUTH_DENIED;
  if (method & SSH_AUTH_METHOD_PASSWORD && (passwordAuth = ssh_userauth_password(m_session, username.c_str(), password.c_str())) == SSH_AUTH_ERROR)
  {
    CLog::Log(LOGERROR, "SFTPSession: Failed to authenticate via password '%s'", ssh_get_error(m_session));
    return false;
  }

  if (noAuth == SSH_AUTH_SUCCESS || publicKeyAuth == SSH_AUTH_SUCCESS || passwordAuth == SSH_AUTH_SUCCESS)
  {
    m_sftp_session = sftp_new(m_session);

    if (m_sftp_session == NULL)
    {
      CLog::Log(LOGERROR, "SFTPSession: Failed to initialize channel '%s'", ssh_get_error(m_session));
      return false;
    }

    if (sftp_init(m_sftp_session))
    {
      CLog::Log(LOGERROR, "SFTPSession: Failed to initialize sftp '%s'", ssh_get_error(m_session));
      return false;
    }

    m_connected = true;
  }

  return m_connected;
}

void CSFTPSession::Disconnect()
{
  if (m_sftp_session)
    sftp_free(m_sftp_session);

  if (m_session)
    ssh_disconnect(m_session);

  m_sftp_session = NULL;
  m_session = NULL;
}

CCriticalSection CSFTPSessionManager::m_critSect;
map<string, CSFTPSessionPtr> CSFTPSessionManager::sessions;

CSFTPSessionPtr CSFTPSessionManager::CreateSession(string host, string username, string password)
{
  CSingleLock lock(m_critSect);
  string key = username + ":" + password + "@" + host;
  CSFTPSessionPtr ptr = sessions[key];
  if (ptr == NULL)
  {
    ptr = CSFTPSessionPtr(new CSFTPSession(host, username, password));
    sessions[key] = ptr;
  }

  return ptr;
}

void CSFTPSessionManager::ClearOutIdleSessions()
{
  CSingleLock lock(m_critSect);
  for(map<string, CSFTPSessionPtr>::iterator iter = sessions.begin(); iter != sessions.end(); ++iter)
  {
    if (iter->second->IsIdle())
      sessions.erase(iter);
  }
}

void CSFTPSessionManager::DisconnectAllSessions()
{
  CSingleLock lock(m_critSect);
  sessions.clear();
}

CFileSFTP::CFileSFTP()
{
  m_sftp_handle = NULL;
}

CFileSFTP::~CFileSFTP()
{
  Close();
}

bool CFileSFTP::Open(const CURL& url)
{
  string username = url.GetUserName().c_str();
  string password = url.GetPassWord().c_str();
  string filename = url.GetFileName().c_str();
  string hostname = url.GetHostName().c_str();

  m_session = CSFTPSessionManager::CreateSession(hostname, username, password);
  if (m_session)
  {
    m_file = filename.c_str();
    m_sftp_handle = m_session->CreateFileHande(m_file);

    return (m_sftp_handle != NULL);
  }
  else
  {
    CLog::Log(LOGERROR, "SFTPFile: Failed to allocate session");
    return false;
  }
}

void CFileSFTP::Close()
{
  if (m_session && m_sftp_handle)
  {
    m_session->CloseFileHandle(m_sftp_handle);
    m_sftp_handle = NULL;
    m_session = CSFTPSessionPtr();
  }
}

int64_t CFileSFTP::Seek(int64_t iFilePosition, int iWhence)
{
  if(iWhence == SEEK_POSSIBLE)
    return 1;

  if (m_session && m_sftp_handle)
  {
    u64 position = 0;
    if (iWhence == SEEK_SET)
      position = iFilePosition;
    else if (iWhence == SEEK_CUR)
      position = GetPosition() + iFilePosition;
    else if (iWhence == SEEK_END)
      position = GetLength() + iFilePosition;

    m_session->Seek(m_sftp_handle, position);
    return GetPosition();
  }
  else
  {
    CLog::Log(LOGERROR, "SFTPFile: Can't seek without a filehandle");
    return -1;
  }
}

unsigned int CFileSFTP::Read(void* lpBuf, int64_t uiBufSize)
{
  if (m_session && m_sftp_handle)
  {
    int rc = m_session->Read(m_sftp_handle, lpBuf, uiBufSize);

    if (rc > 0)
      return rc;
    else
      CLog::Log(LOGERROR, "SFTPFile: Failed to read %i", rc);
  }
  else
    CLog::Log(LOGERROR, "SFTPFile: Can't read without a filehandle");

  return 0;
}

bool CFileSFTP::Exists(const CURL& url)
{
  CSFTPSessionPtr session = CSFTPSessionManager::CreateSession(url.GetHostName().c_str(), url.GetUserName().c_str(), url.GetPassWord().c_str());
  if (session)
    return session->Exists(url.GetFileName().c_str());
  else
  {
    CLog::Log(LOGERROR, "SFTPFile: Failed to create session to check exists");
    return false;
  }
}

int CFileSFTP::Stat(const CURL& url, struct __stat64* buffer)
{
  CSFTPSessionPtr session = CSFTPSessionManager::CreateSession(url.GetHostName().c_str(), url.GetUserName().c_str(), url.GetPassWord().c_str());
  if (session)
    return session->Stat(url.GetFileName().c_str(), buffer);
  else
  {
    CLog::Log(LOGERROR, "SFTPFile: Failed to create session to stat");
    return -1;
  }
}

int CFileSFTP::Stat(struct __stat64* buffer)
{
  if (m_session)
    return m_session->Stat(m_file.c_str(), buffer);

  CLog::Log(LOGERROR, "SFTPFile: Can't stat without a session");
  return -1;
}

int64_t CFileSFTP::GetLength()
{
  struct __stat64 buffer;
  if (Stat(&buffer) < 0)
    return 0;
  else
  {
    int64_t length = buffer.st_size;
    return length;
  }
}

int64_t CFileSFTP::GetPosition()
{
  if (m_session && m_sftp_handle)
    return m_session->GetPosition(m_sftp_handle);

  CLog::Log(LOGERROR, "SFTPFile: Can't get position without a filehandle");
  return 0;
}
#endif
