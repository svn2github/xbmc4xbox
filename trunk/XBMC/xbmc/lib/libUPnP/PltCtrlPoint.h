/*****************************************************************
|
|   Platinum - Control Point
|
|   Copyright (c) 2004-2006 Sylvain Rebaud
|   Author: Sylvain Rebaud (sylvain@rebaud.com)
|
 ****************************************************************/

#ifndef _PLT_CONTROL_POINT_H_
#define _PLT_CONTROL_POINT_H_

/*----------------------------------------------------------------------
|   includes
+---------------------------------------------------------------------*/
#include "NptDefs.h"
#include "NptStrings.h"
#include "PltService.h"
#include "PltHttpServerListener.h"
#include "PltSsdpListener.h"
#include "PltDeviceData.h"

/*----------------------------------------------------------------------
|   forward declarations
+---------------------------------------------------------------------*/
class PLT_HttpServer;
class PLT_CtrlPointHouseKeepingTask;
class PLT_SsdpSearchTask;

/*----------------------------------------------------------------------
|   PLT_CtrlPointListener class
+---------------------------------------------------------------------*/
class PLT_CtrlPointListener
{
public:
    virtual ~PLT_CtrlPointListener() {}

    virtual NPT_Result OnDeviceAdded(PLT_DeviceDataReference& device) = 0;
    virtual NPT_Result OnDeviceRemoved(PLT_DeviceDataReference& device) = 0;
    virtual NPT_Result OnActionResponse(NPT_Result res, PLT_Action* action, void* userdata) = 0;
    virtual NPT_Result OnEventNotify(PLT_Service* service, NPT_List<PLT_StateVariable*>* vars) = 0;
};

typedef NPT_List<PLT_CtrlPointListener*> PLT_CtrlPointListenerList;

/*----------------------------------------------------------------------
|   PLT_CtrlPoint class
+---------------------------------------------------------------------*/
class PLT_CtrlPoint : public PLT_HttpServerListener,
                      public PLT_SsdpPacketListener,
                      public PLT_SsdpSearchResponseListener
{
public:
    PLT_CtrlPoint(const char* autosearch = "upnp:rootdevice"); // pass NULL to bypass the multicast search

    NPT_Result   AddListener(PLT_CtrlPointListener* listener);
    NPT_Result   RemoveListener(PLT_CtrlPointListener* listener);

    NPT_Result   SetUUIDToIgnore(const char* uuid) {
        m_UUIDToIgnore = uuid;
        return NPT_SUCCESS;
    }

    NPT_Result   Start(PLT_TaskManager* task_manager);
    NPT_Result   Stop();

    NPT_Result   Search(const NPT_HttpUrl& url = NPT_HttpUrl("239.255.255.250", 1900, "*"), 
                        const char*        target = "upnp:rootdevice", 
                        const NPT_Cardinal MX = 5);
    NPT_Result   Discover(const NPT_HttpUrl& url = NPT_HttpUrl("239.255.255.250", 1900, "*"), 
                          const char*        target = "ssdp:all", 
                          const NPT_Cardinal MX = 5,
                          NPT_Timeout        repeat = 50000);
    
    NPT_Result   InvokeAction(PLT_Action* action, PLT_Arguments& arguments, void* userdata = NULL);
    NPT_Result   Subscribe(PLT_Service* service, bool renew = false, void* userdata = NULL);

    // PLT_HttpServerListener methods
    virtual NPT_Result ProcessHttpRequest(NPT_HttpRequest* request, NPT_SocketInfo info, NPT_HttpResponse*& response);

    // PLT_SsdpSearchResponseListener methods
    NPT_Result   ProcessSsdpSearchResponse(NPT_Result        res, 
                                           NPT_SocketInfo&   info, 
                                           NPT_HttpResponse* response);

    // PLT_SsdpPacketListener method
    virtual NPT_Result OnSsdpPacket(NPT_HttpRequest* request, NPT_SocketInfo info);

    // helper methods
    NPT_Result FindDevice(NPT_String& uuid, PLT_DeviceDataReference& device) {
        NPT_AutoLock lock(m_Devices);
        NPT_List<PLT_DeviceDataReference>::Iterator it = m_Devices.Find(PLT_DeviceDataFinder(uuid));
        if (it) {
            device = (*it);
            return NPT_SUCCESS;
        }
        return NPT_FAILURE;
    }

protected:
    virtual ~PLT_CtrlPoint();

    // methods
    NPT_Result   ProcessSsdpNotify(
        NPT_HttpRequest*   request, 
        NPT_SocketInfo     info);

    NPT_Result   ProcessSsdpMessage(
        NPT_HttpMessage*   message, 
        NPT_SocketInfo&    info, 
        NPT_String&        uuid);

    NPT_Result   ProcessGetDescriptionResponse(
        NPT_Result               res, 
        NPT_HttpResponse*        response,
        PLT_DeviceDataReference& device);

    NPT_Result   ProcessGetSCPDResponse(
        NPT_Result               res, 
        NPT_HttpRequest*         request,
        NPT_HttpResponse*        response,
        PLT_DeviceDataReference& device);

    NPT_Result   ProcessActionResponse(
        NPT_Result         res, 
        NPT_HttpResponse*  response,
        PLT_Action*        action,
        void*              userdata);

    NPT_Result   ProcessSubscribeResponse(
        NPT_Result         res, 
        NPT_HttpResponse*  response,
        PLT_Service*       service,
        void*              userdata);

    NPT_Result   DoHouseKeeping();

private:
    // methods
    NPT_Result   ParseFault(PLT_Action* action, NPT_XmlElementNode* fault);

private:
    friend class NPT_Reference<PLT_CtrlPoint>;
	friend class PLT_CtrlPointGetDescriptionTask;
    friend class PLT_CtrlPointGetSCPDTask;
    friend class PLT_CtrlPointInvokeActionTask;
    friend class PLT_CtrlPointHouseKeepingTask;
    friend class PLT_CtrlPointSubscribeEventTask;

    NPT_AtomicVariable                              m_ReferenceCount;
    NPT_String                                      m_UUIDToIgnore;
    PLT_CtrlPointHouseKeepingTask*                  m_HouseKeepingTask;
    NPT_List<PLT_SsdpSearchTask*>                   m_SsdpSearchTasks;
    NPT_Lock<PLT_CtrlPointListenerList>             m_ListenerList;
    PLT_HttpServer*                                 m_EventHttpServer;
    PLT_TaskManager*                                m_TaskManager;
    NPT_Lock<NPT_List<PLT_DeviceDataReference> >    m_Devices;
    NPT_List<PLT_EventSubscriber*>                  m_Subscribers;
    NPT_String                                      m_AutoSearch;
};

typedef NPT_Reference<PLT_CtrlPoint> PLT_CtrlPointReference;

#endif /* _PLT_CONTROL_POINT_H_ */
