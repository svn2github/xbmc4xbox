/*****************************************************************
|
|   Platinum - UPnP Engine
|
|   Copyright (c) 2004-2006 Sylvain Rebaud
|   Author: Sylvain Rebaud (sylvain@rebaud.com)
|
 ****************************************************************/

#ifndef _PLT_UPNP_H_
#define _PLT_UPNP_H_

/*----------------------------------------------------------------------
|   includes
+---------------------------------------------------------------------*/
#include "PltTaskManager.h"
#include "PltCtrlPoint.h"
#include "PltDeviceHost.h"

/*----------------------------------------------------------------------
|   forward definitions
+---------------------------------------------------------------------*/
class PLT_SsdpListenTask;

/*----------------------------------------------------------------------
|   PLT_UPnP class
+---------------------------------------------------------------------*/
class PLT_UPnP : public PLT_TaskManager
{
public:
    PLT_UPnP(NPT_UInt32 ssdp_port = 1900, bool multicast = true);
    ~PLT_UPnP();

    NPT_Result AddDevice(PLT_DeviceHostReference& device);
    NPT_Result AddCtrlPoint(PLT_CtrlPointReference& ctrlpoint);

    NPT_Result Start();
    NPT_Result Stop();

private:
    // members
    NPT_Mutex                           m_Lock;
    NPT_List<PLT_DeviceHostReference>   m_Devices;
    NPT_List<PLT_CtrlPointReference>    m_CtrlPoints;

    // since we can only have one socket listening on port 1900, 
    // we create it in here and we will attach every control points
    // and devices to it when they're added
    bool                                m_Started;
    NPT_Socket*                         m_SsdpListensocket;
    bool                                m_Multicast;
    PLT_SsdpListenTask*                 m_SsdpListenTask; 
};

#endif /* _PLT_UPNP_H_ */
