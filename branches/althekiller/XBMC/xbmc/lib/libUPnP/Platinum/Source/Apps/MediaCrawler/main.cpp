/*****************************************************************
|
|   Platinum - main
|
|   Copyright (c) 2004-2008, Plutinosoft, LLC.
|   Author: Sylvain Rebaud (sylvain@plutinosoft.com)
|
****************************************************************/

#include "MediaCrawler.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

/*----------------------------------------------------------------------
|   main
+---------------------------------------------------------------------*/
int main(void)
{
    PLT_UPnP upnp;
    PLT_CtrlPointReference ctrlPoint(new PLT_CtrlPoint());
    upnp.AddCtrlPoint(ctrlPoint);

    CMediaCrawler* crawler = new CMediaCrawler(ctrlPoint, "Sylvain: Platinum: Crawler");
    CPassThroughStreamHandler* handler = new CPassThroughStreamHandler(crawler);
    crawler->AddStreamHandler(handler);
    PLT_DeviceHostReference device(crawler);
    upnp.AddDevice(device);
    
    // make sure we ignore ourselves
    ctrlPoint->IgnoreUUID(device->GetUUID());

    upnp.Start();

    // extra broadcast discover 
    ctrlPoint->Discover(NPT_HttpUrl("255.255.255.255", 1900, "*"), "upnp:rootdevice", 1);

    char buf[256];
    while (gets(buf)) {
        if (*buf == 'q')
            break;
    }

    upnp.Stop();

    delete handler;
    return 0;
}
