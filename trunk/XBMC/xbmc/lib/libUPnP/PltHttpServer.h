/*****************************************************************
|
|   Platinum - HTTP Server
|
|   Copyright (c) 2004-2006 Sylvain Rebaud
|   Author: Sylvain Rebaud (sylvain@rebaud.com)
|
 ****************************************************************/

#ifndef _PLT_HTTP_SERVER_H_
#define _PLT_HTTP_SERVER_H_

/*----------------------------------------------------------------------
|   includes
+---------------------------------------------------------------------*/
#include "NptDefs.h"
#include "NptStrings.h"
#include "NptSockets.h"
#include "NptHttp.h"
#include "NptFile.h"
#include "PltHttpServerListener.h"
#include "PltHttpServerTask.h"

/*----------------------------------------------------------------------
|   forward declarations
+---------------------------------------------------------------------*/
class PLT_HttpServerStartIterator;

/*----------------------------------------------------------------------
|   PLT_HttpServer class
+---------------------------------------------------------------------*/
class PLT_HttpServer
{
    friend class PLT_HttpServerTask<class PLT_HttpServer>;
    friend class PLT_HttpServerStartIterator;

 public:
    PLT_HttpServer(PLT_HttpServerListener* listener, 
                   unsigned int            port = 0,
                   NPT_Cardinal            max_clients = 0);
    virtual ~PLT_HttpServer();

    virtual NPT_Result   Start();
    virtual NPT_Result   Stop();
    virtual unsigned int GetPort() { return m_Port; }

 private:
    PLT_TaskManager*          m_TaskManager;
    PLT_HttpServerListener*   m_Listener;
    unsigned int              m_Port;
    PLT_HttpServerListenTask* m_HttpListenTask;
};

/*----------------------------------------------------------------------
|   PLT_FileServer class
+---------------------------------------------------------------------*/
class PLT_FileServer : public PLT_HttpServerListener
{
public:
    // class methods
    static NPT_Result ServeFile(NPT_String        filename, 
                                NPT_HttpResponse* response, 
                                NPT_Integer       start = -1, 
                                NPT_Integer       end = -1,
                                bool              request_is_head = false);

    // constructor & destructor
    PLT_FileServer(const char* local_path) : m_LocalPath(local_path) {}
    virtual ~PLT_FileServer() {}

    // PLT_HttpServerListener method
    NPT_Result ProcessHttpRequest(NPT_HttpRequest*   request, 
                                  NPT_SocketInfo     info, 
                                  NPT_HttpResponse*& response);

protected:
    NPT_String m_LocalPath;
};

#endif /* _PLT_HTTP_SERVER_H_ */
