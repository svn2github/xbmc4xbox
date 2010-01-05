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

#include "system.h"
#include "WebServer.h"
#include "../lib/libjsonrpc/JSONRPC.h"
#include "../lib/libhttpapi/HttpApi.h"
#include "../FileSystem/File.h"
#include "../Util.h"
#include "log.h"

using namespace XFILE;
using namespace std;

CWebServer::CWebServer()
{
  m_running = false;
  m_daemon = NULL;
}


int PrintOutKey(void *cls, enum MHD_ValueKind kind, const char *key, const char *value) 
{
  printf("%s %s\n", key, value);

  map<CStdString, CStdString> *arguments = (map<CStdString, CStdString> *)cls;
  arguments->insert( pair<CStdString,CStdString>(key,value) );

  return MHD_YES; 
} 

int CWebServer::answer_to_connection (void *cls, struct MHD_Connection *connection,
                      const char *url, const char *method,
                      const char *version, const char *upload_data,
                      size_t *upload_data_size, void **con_cls)
{
  printf("%s | %s\n", method, url);

  CStdString strURL = url;
#ifdef HAS_JSONRPC
  if (strURL.Equals("/jsonrpc") && strcmp (method, "POST") == 0)
  {
    char jsoncall[*upload_data_size + 1];
    memcpy(jsoncall, upload_data, *upload_data_size);
    jsoncall[*upload_data_size] = '\0';
    if (*upload_data_size > 204800)
      CLog::Log(LOGINFO, "JSONRPC: Recieved a jsonrpc call wich is bigger than 200KiB, skipping logging it");
    else
      CLog::Log(LOGINFO, "JSONRPC: Recieved a jsonrpc call - %s", jsoncall);
    printf("%s\n", jsoncall);
    CStdString jsonresponse = CJSONRPC::MethodCall(jsoncall);

    struct MHD_Response *response = MHD_create_response_from_data(jsonresponse.length(), (void *) jsonresponse.c_str(), MHD_NO, MHD_YES);
    int ret = MHD_queue_response(connection, MHD_HTTP_OK, response);
    MHD_destroy_response(response);

    return ret;
  }
#endif
  if (strcmp(method, "GET") == 0)
  {
    if (strURL.Left(6).Equals("/thumb"))
    {
      strURL = strURL.Right(strURL.length() - 7);
      strURL = strURL.Left(strURL.length() - 4);
    }
    else if (strURL.Left(4).Equals("/vfs"))
    {
      strURL = strURL.Right(strURL.length() - 5);
      CUtil::UrlDecode(strURL);
    }
#ifdef HAS_HTTPAPI
    else if (strURL.Left(18).Equals("/xbmcCmds/xbmcHttp"))
    {
      printf("getting argumentkinds\n");
      map<CStdString, CStdString> arguments;
      if (MHD_get_connection_values(connection, MHD_GET_ARGUMENT_KIND, PrintOutKey, &arguments) > 0)
      {
        CStdString httpapiresponse = CHttpApi::MethodCall(arguments["command"], arguments["parameter"]);

        struct MHD_Response *response = MHD_create_response_from_data(httpapiresponse.length(), (void *) httpapiresponse.c_str(), MHD_NO, MHD_YES);
        int ret = MHD_queue_response(connection, MHD_HTTP_OK, response);
        MHD_destroy_response(response);

        return ret;
      }
    }
#endif
#ifdef HAS_WEB_INTERFACE
    else if (strURL.Equals("/"))
      strURL = "special://home/web/index.html";
    else
      strURL.Format("special://home/web%s", strURL.c_str());
#endif
    CFile *file = new CFile();
    if (file->Open(strURL))
    {
      struct MHD_Response *response = MHD_create_response_from_callback ( file->GetLength(),
                                                                          2048,
                                                                          &CWebServer::ContentReaderCallback, file,
                                                                          &CWebServer::ContentReaderFreeCallback);
      int ret = MHD_queue_response(connection, MHD_HTTP_OK, response);
      MHD_destroy_response(response);
      return ret;
    }
    else
      delete file;
  }

  return MHD_NO;
}

int CWebServer::ContentReaderCallback(void *cls, uint64_t pos, char *buf, int max)
{
  CFile *file = (CFile *)cls;
  file->Seek(pos);
  return file->Read(buf, max);
}
void CWebServer::ContentReaderFreeCallback(void *cls)
{
  CFile *file = (CFile *)cls;
  file->Close();

  delete file;
}

bool CWebServer::Initialize()
{
#ifdef HAS_JSONRPC
  CJSONRPC::Initialize();
#endif
  return true;
}

bool CWebServer::Start(const char *ip, int port)
{
  if (!m_running)
  {
    Initialize();

    // To stream perfectly we should probably have MHD_USE_THREAD_PER_CONNECTION instead of MHD_USE_SELECT_INTERNALLY as it provides multiple clients concurrently
    m_daemon = MHD_start_daemon(MHD_USE_SELECT_INTERNALLY | MHD_USE_SSL, port, NULL, NULL, &CWebServer::answer_to_connection, NULL, MHD_OPTION_END);
    m_running = m_daemon != NULL;
  }
  return m_running;
}

bool CWebServer::Stop()
{
  if (m_running)
  {
    MHD_stop_daemon(m_daemon);
    m_running = false;
  }

  return !m_running;
}
