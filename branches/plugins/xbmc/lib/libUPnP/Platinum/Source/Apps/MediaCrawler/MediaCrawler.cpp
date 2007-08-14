/*****************************************************************
|
|   Platinum - Media Crawler
|
|   Copyright (c) 2004-2006 Sylvain Rebaud
|   Author: Sylvain Rebaud (sylvain@rebaud.com)
|
****************************************************************/
#include "PltVersion.h"
#include "MediaCrawler.h"
#include "PltLeaks.h"
#include "PltMediaServer.h"
#include "PltHttpServer.h"
#include "PltDidl.h"
#include "PltXmlHelper.h"

/*----------------------------------------------------------------------
|   CMediaCrawler::CMediaCrawler
+---------------------------------------------------------------------*/
CMediaCrawler::CMediaCrawler(PLT_CtrlPointReference& ctrlPoint,
                             const char*             friendly_name,
                             bool                    show_ip,
                             const char*             udn /* = NULL */,
                             unsigned int            port /* = 0 */,
                             unsigned int            fileserver_port /* = 0 */) :
    PLT_MediaBrowser(ctrlPoint, NULL),
    PLT_MediaServer(friendly_name, show_ip, udn, port, fileserver_port)
{
}

/*----------------------------------------------------------------------
|   CMediaCrawler::~CMediaCrawler
+---------------------------------------------------------------------*/
CMediaCrawler::~CMediaCrawler()
{
}

/*----------------------------------------------------------------------
|   CMediaCrawler::Start
+---------------------------------------------------------------------*/
NPT_Result
CMediaCrawler::Start(PLT_TaskManager* task_manager)
{
    NPT_CHECK(PLT_MediaServer::Start(task_manager));

    NPT_String ip;
    NPT_List<NPT_NetworkInterface*> if_list;
    NPT_NetworkInterface::GetNetworkInterfaces(if_list);
    NPT_List<NPT_NetworkInterface*>::Iterator iface = if_list.GetFirstItem();
    while (iface) {
        NPT_String tmp = (*(*iface)->GetAddresses().GetFirstItem()).GetPrimaryAddress().ToString();
        if (tmp.Compare("0.0.0.0") && tmp.Compare("127.0.0.1")) {
            ip = tmp;
            PLT_Log(PLT_LOG_LEVEL_1, "IP addr: %s\n", (const char*)ip);
            break;
        }
        ++iface;
    }
    if_list.Apply(NPT_ObjectDeleter<NPT_NetworkInterface>());

    // use localhost if no ip found
    if (ip.GetLength() == 0) {
        ip = "127.0.0.1";
    }

    // set the file urls for content and album arts according to fileserver port
    m_FileBaseUri = "http://" + ip + ":" + NPT_String::FromInteger(m_FileServer->GetPort()) + "/foo";
    return NPT_SUCCESS;
}

/*----------------------------------------------------------------------
|   CMediaCrawler::Stop
+---------------------------------------------------------------------*/
NPT_Result
CMediaCrawler::Stop()
{
    return PLT_MediaServer::Stop();
}

/*----------------------------------------------------------------------
|   CMediaCrawler::SplitObjectId
+---------------------------------------------------------------------*/
NPT_Result
CMediaCrawler::SplitObjectId(const NPT_String& object_id, NPT_String& server_uuid, NPT_String& server_object_id)
{
    // reset output params
    server_uuid = "";
    server_object_id = "";

    if (object_id.GetLength() == 0 || object_id[0] != '0')
        return NPT_ERROR_INVALID_FORMAT;

    if (object_id.GetLength() > 1) {
        if (object_id[1] != '/') return NPT_ERROR_INVALID_FORMAT;
    
        server_uuid = object_id.SubString(2);

        // look for next delimiter
        int index = server_uuid.Find('/');
        if (index >= 0) {
            server_object_id = server_uuid.SubString(index+1);
            server_uuid.SetLength(index);
        }
    }

    return NPT_SUCCESS;
}

/*----------------------------------------------------------------------
|   CMediaCrawler::FormatObjectId
+---------------------------------------------------------------------*/
NPT_String
CMediaCrawler::FormatObjectId(const NPT_String& server_uuid, const NPT_String& server_object_id)
{
    NPT_String object_id = NPT_String("0/") + server_uuid;
    if (server_object_id.GetLength())
        object_id += NPT_String("/") + server_object_id;

    return object_id;
}

/*----------------------------------------------------------------------
|   PLT_MediaServer::OnBrowse
+---------------------------------------------------------------------*/
NPT_Result
CMediaCrawler::OnBrowse(PLT_ActionReference& action, NPT_SocketInfo* info /* = NULL */)
{
    NPT_Result res;

    NPT_String object_id;
    if (NPT_FAILED(action->GetArgumentValue("ObjectId", object_id))) {
        PLT_Log(PLT_LOG_LEVEL_1, "PLT_FileMediaServer::OnBrowse - invalid arguments.");
        return NPT_FAILURE;
    }

    PLT_Log(PLT_LOG_LEVEL_1, "PLT_FileMediaServer::OnOnBrowse - id = %s\r\n", (const char*)object_id);

    // extract server uuid and server object id from our object id
    NPT_String server_uuid;
    NPT_String server_object_id;
    if (NPT_FAILED(SplitObjectId(object_id, server_uuid, server_object_id))) {
        /* error */
        PLT_Log(PLT_LOG_LEVEL_1, "CMediaCrawler::OnBrowse - ObjectID not found.");
        action->SetError(701, "No Such Object.");
        return NPT_FAILURE;
    }

    // root ?
    if (server_uuid.GetLength() == 0) {
        res = OnBrowseRoot(action);
    } else {
        // we have a device to browse
        // is it device root?
        if (server_object_id.GetLength() == 0) {
            server_object_id = "0";
        }
        res = OnBrowseDevice(action, server_uuid, server_object_id, info);
    }

    if (NPT_FAILED(res) && (action->GetErrorCode() == 0)) {
        action->SetError(800, "Internal error");
    }

    return res;
}

/*----------------------------------------------------------------------
|   CMediaCrawler::OnBrowseRoot
+---------------------------------------------------------------------*/
NPT_Result
CMediaCrawler::OnBrowseRoot(PLT_ActionReference& action)
{
    NPT_String browseFlagValue;
    if (NPT_FAILED(action->GetArgumentValue("BrowseFlag", browseFlagValue))) {
        PLT_Log(PLT_LOG_LEVEL_1, "PLT_FileMediaServer::OnBrowse - invalid arguments.");
        return NPT_FAILURE;
    }

    /* extract browseFlag */
    BrowseFlags browseFlag;
    if (NPT_FAILED(GetBrowseFlag(browseFlagValue, browseFlag))) {
        /* error */
        PLT_Log(PLT_LOG_LEVEL_1, "PLT_FileMediaServer::OnBrowseRoot - BrowseFlag value not allowed.");
        action->SetError(402,"Invalid BrowseFlag arg.");
        return NPT_FAILURE;
    }

    if (browseFlag == BROWSEMETADATA) {
        PLT_MediaContainer item;
        item.m_Title = "Root";
        item.m_ParentID = "-1";
        item.m_ObjectID = "0";
        item.m_ChildrenCount = GetMediaServers().GetItemCount();
        item.m_ObjectClass.type = "object.container";

        /* convert item to didl */
        NPT_String filter;
        action->GetArgumentValue("Filter", filter);
        NPT_String tmp;
        NPT_CHECK(PLT_Didl::ToDidl(item, filter, tmp));

        /* add didl header and footer */
        NPT_String didl = didl_header + tmp + didl_footer;

        action->SetArgumentValue("Result", didl);
        action->SetArgumentValue("NumberReturned", "1");
        action->SetArgumentValue("TotalMatches", "1");

        // update ID may be wrong here, it should be the one of the container?
        action->SetArgumentValue("UpdateId", "1");
        // TODO: We need to keep track of the overall updateID of the CDS
    } else {
        NPT_String startingInd;
        NPT_String reqCount;
        NPT_String filter;

        NPT_CHECK(action->GetArgumentValue("StartingIndex", startingInd));
        NPT_CHECK(action->GetArgumentValue("RequestedCount", reqCount));   
        NPT_CHECK(action->GetArgumentValue("Filter", filter));

        unsigned long start_index, req_count;
        if (NPT_FAILED(startingInd.ToInteger(start_index)) ||
            NPT_FAILED(reqCount.ToInteger(req_count))) {
            return NPT_FAILURE;
        }   
                    
        unsigned long cur_index = 0;
        unsigned long num_returned = 0;
        unsigned long total_matches = 0;
        //unsigned long update_id = 0;
        PLT_MediaContainer item;
        NPT_String tmp;
        NPT_String didl = didl_header;

        // populate a list of containers (one container per known servers)
        const NPT_Lock<PLT_DeviceDataReferenceList>& devices = GetMediaServers();
        NPT_Lock<PLT_DeviceDataReferenceList>::Iterator entry = devices.GetFirstItem();
        while (entry) {
            PLT_DeviceDataReference device = (*entry);
            item.m_Title = device->GetFriendlyName();
            item.m_ObjectID = FormatObjectId(device->GetUUID(), "0");
            item.m_ParentID = "0";
            item.m_ObjectClass.type = "object.container";

            if ((cur_index >= start_index) && ((num_returned < req_count) || (req_count == 0))) {
                NPT_CHECK(PLT_Didl::ToDidl(item, filter, tmp));

                didl += tmp;
                num_returned++;
            }
            cur_index++;
            total_matches++;  

            ++entry;
        }

        didl += didl_footer;

        action->SetArgumentValue("Result", didl);
        action->SetArgumentValue("NumberReturned", NPT_String::FromInteger(num_returned));
        action->SetArgumentValue("TotalMatches", NPT_String::FromInteger(total_matches));
        action->SetArgumentValue("UpdateId", "1");
    }

    return NPT_SUCCESS;
}

/*----------------------------------------------------------------------
|   CMediaCrawler::OnBrowseDevice
+---------------------------------------------------------------------*/
NPT_Result
CMediaCrawler::OnBrowseDevice(PLT_ActionReference& action, 
                              const char*          server_uuid, 
                              const char*          server_object_id, 
                              NPT_SocketInfo*      info /* = NULL */)
{
    NPT_Result res;
    PLT_DeviceDataReference device;

    {
        // look for device first
        const NPT_Lock<PLT_DeviceDataReferenceList>& devices = GetMediaServers();
        //NPT_AutoLock lock(devices);

        if (NPT_FAILED(NPT_ContainerFind(devices, PLT_DeviceDataFinder(server_uuid), device))) {
            /* error */
            PLT_Log(PLT_LOG_LEVEL_1, "CMediaCrawler::OnBrowseDevice - device not found.");
            action->SetError(701, "No Such Object.");
            return NPT_FAILURE;
        } 
    }

    // look for args and convert them
    NPT_String browseFlagValue;
    NPT_String startingInd;
    NPT_String reqCount;
    NPT_String filter;
    NPT_String sort;

    NPT_CHECK(action->GetArgumentValue("BrowseFlag", browseFlagValue));
    NPT_CHECK(action->GetArgumentValue("StartingIndex", startingInd));
    NPT_CHECK(action->GetArgumentValue("RequestedCount", reqCount));   
    NPT_CHECK(action->GetArgumentValue("Filter", filter));
    NPT_CHECK(action->GetArgumentValue("SortCriteria", sort));

    unsigned long start_index, req_count;
    if (NPT_FAILED(startingInd.ToInteger(start_index)) ||
        NPT_FAILED(reqCount.ToInteger(req_count))) {
        return NPT_FAILURE;
    } 

    // create a container for our result
    // this will be filled in by OnBrowseResponse
    CMediaCrawlerBrowseInfoReference browse_info(new CMediaCrawlerBrowseInfo());
    browse_info->shared_var.SetValue(0);

    // send off the browse packet.  Note that this will
    // not block.  The shared variable is used to block
    // until the response has been received.
    res = Browse(device,
        server_object_id,
        browseFlagValue,
        start_index,
        req_count,
        filter,
        sort,
        new CMediaCrawlerBrowseInfoReference(browse_info));		
    NPT_CHECK(res);

    // wait 30 secs for response
    res = browse_info->shared_var.WaitUntilEquals(1, 30000);
    NPT_CHECK(res);

    // did the browse fail?
    if (NPT_FAILED(browse_info->res)) {
        action->SetError(browse_info->code, "");
        return NPT_FAILURE;
    }    
   
    action->SetArgumentValue("Result", UpdateDidl(server_uuid, browse_info->didl, info));
    action->SetArgumentValue("NumberReturned", browse_info->nr);
    action->SetArgumentValue("TotalMatches", browse_info->tm);
    action->SetArgumentValue("UpdateId", browse_info->uid);
    action->SetArgumentValue("ObjectID", FormatObjectId(server_uuid, browse_info->object_id));
    
    return NPT_SUCCESS;
}

/*----------------------------------------------------------------------
|   CMediaCrawler::OnBrowseResponse
+---------------------------------------------------------------------*/
NPT_Result
CMediaCrawler::OnBrowseResponse(NPT_Result res, PLT_DeviceDataReference& device, PLT_ActionReference& action, void* userdata)
{
    NPT_COMPILER_UNUSED(device);

    if (!userdata) return NPT_FAILURE;

    CMediaCrawlerBrowseInfoReference* info = (CMediaCrawlerBrowseInfoReference*) userdata;
    (*info)->res = res;
    (*info)->code = action->GetErrorCode();

    if (NPT_FAILED(res) || action->GetErrorCode() != 0) {
        goto bad_action;
    }

    if (NPT_FAILED(action->GetArgumentValue("ObjectID", (*info)->object_id)))  {
        goto bad_action;
    }
    if (NPT_FAILED(action->GetArgumentValue("UpdateID", (*info)->uid))) {
        goto bad_action;
    }
    if (NPT_FAILED(action->GetArgumentValue("NumberReturned", (*info)->nr))) {
        goto bad_action;
    }
    if (NPT_FAILED(action->GetArgumentValue("TotalMatches", (*info)->tm))) {
        goto bad_action;
    }
    if (NPT_FAILED(action->GetArgumentValue("Result", (*info)->didl))) {
        goto bad_action;
    }
    goto done;

bad_action:
    if (NPT_SUCCEEDED((*info)->res)) (*info)->res = NPT_FAILURE;

done:
    (*info)->shared_var.SetValue(1);
    delete info;

    return NPT_SUCCESS;
}

/*----------------------------------------------------------------------
|   CMediaCrawler::UpdateDidl
+---------------------------------------------------------------------*/
NPT_String
CMediaCrawler::UpdateDidl(const char* server_uuid, const NPT_String& didl, NPT_SocketInfo* info)
{
    NPT_String     new_didl;
    NPT_String     str;
    NPT_XmlNode*   node = NULL;
    NPT_XmlWriter  writer;
    NPT_OutputStreamReference stream(new NPT_StringOutputStream(&new_didl));

    PLT_Log(PLT_LOG_LEVEL_1, "Parsing Didl...\r\n");

    NPT_XmlElementNode* tree = NULL;
    NPT_XmlParser parser;
    if (NPT_FAILED(parser.Parse(didl, node)) || !node || !node->AsElementNode()) {
        goto cleanup;
    }

    tree = node->AsElementNode();

    PLT_Log(PLT_LOG_LEVEL_1, "Processing Didl xml...\r\n");
    if (tree->GetTag().Compare("DIDL-Lite", true)) {
        goto cleanup;
    }

    // iterate through children
    NPT_Result res;
    for (NPT_List<NPT_XmlNode*>::Iterator children = tree->GetChildren().GetFirstItem(); children; children++) {
        NPT_XmlElementNode* child = (*children)->AsElementNode();
        if (!child) continue;

        // object id remapping
        NPT_XmlAttribute* attribute_id;
        res = PLT_XmlHelper::GetAttribute(child, "id", attribute_id);
        if (NPT_SUCCEEDED(res) && attribute_id) {
            attribute_id->SetValue(FormatObjectId(server_uuid, attribute_id->GetValue()));
        }

        // parent ID remapping
        NPT_XmlAttribute* attribute_parent_id;
        res = PLT_XmlHelper::GetAttribute(child, "parentID", attribute_parent_id);
        if (NPT_SUCCEEDED(res)) {
            attribute_parent_id->SetValue(attribute_parent_id->GetValue().Compare("-1")?FormatObjectId(server_uuid, attribute_parent_id->GetValue()):"0");
        }

        // resources remapping
        NPT_Array<NPT_XmlElementNode*> res;
        PLT_XmlHelper::GetChildren(child, res, "res");
        if (res.GetItemCount() > 0) {
            for (unsigned int i=0; i<res.GetItemCount(); i++) {
                NPT_XmlElementNode* resource = res[i];
                NPT_XmlAttribute*   attribute_prot;
                const NPT_String*   url;
                if (NPT_SUCCEEDED(PLT_XmlHelper::GetAttribute(resource, "protocolInfo", attribute_prot)) && (url = resource->GetText()) != NULL) {
                    CStreamHandler* handler = NULL;
                    NPT_Result res = NPT_ContainerFind(m_StreamHandlers, CStreamHandlerFinder(attribute_prot->GetValue(), *url), handler);
                    if (NPT_SUCCEEDED(res)) {
                        handler->ModifyResource(resource);
                    } else {
                        // special case for Windows Media Connect
                        // When a browse is done on the same machine, WMC uses localhost 
                        // instead of the IP for all resources urls which means we cannot advertise that 
                        // since it would be useless for a remote device 
                        // so we try to replace it with the right IP address by looking at which interface we received the
                        // initial browse request on to make sure the remote device will be able to access the modified resource
                        // urls (in case the local PC has more than 1 NICs)

                        // replace the url
                        NPT_List<NPT_XmlNode*>& children = resource->GetChildren();
                        if (children.GetItemCount() != 1 || !(*children.GetFirstItem())->AsTextNode())
                            continue;

                        const NPT_String* text = resource->GetText();
                        if (text) {
                            NPT_HttpUrl url(NPT_Uri::Decode(*text));
                            if ((url.GetHost() == "localhost" || url.GetHost() == "127.0.0.1") && info) {
                                if (info->local_address.GetIpAddress().AsLong()) {
                                    url.SetHost(info->local_address.GetIpAddress().ToString());

                                    // replace text
                                    children.Apply(NPT_ObjectDeleter<NPT_XmlNode>());
                                    children.Clear();
                                    resource->AddText(url.ToString());
                                }
                            }
                        }
                    }
                }
            }
        }
    }

    // serialize modified node into new didl
    writer.Serialize(*node, *stream);
    delete node;
    return new_didl;

cleanup:
    delete node;
    return didl;
}

/*----------------------------------------------------------------------
|   CMediaCrawler::ProcessFileRequest
+---------------------------------------------------------------------*/
NPT_Result 
CMediaCrawler::ProcessFileRequest(NPT_HttpRequest* request, NPT_SocketInfo info, NPT_HttpResponse*& response)
{
    NPT_COMPILER_UNUSED(info);

    PLT_Log(PLT_LOG_LEVEL_3, "CMediaCrawler::ProcessFileRequest Received Request:\r\n");
    PLT_HttpHelper::ToLog(request, PLT_LOG_LEVEL_3);

    if (request->GetMethod().Compare("GET") && request->GetMethod().Compare("HEAD")) {
        response->SetStatus(500, "Internal Server Error");
        return NPT_SUCCESS;
    }

    // add the user agent header, some stupid media servers like YME needs it
    if (!request->GetHeaders().GetHeader(NPT_HTTP_HEADER_USER_AGENT)) {
        request->GetHeaders().SetHeader(NPT_HTTP_HEADER_USER_AGENT, 
            "Platinum/" PLT_PLATINUM_VERSION_STRING);
    }

    // File requested
    NPT_HttpUrlQuery query(request->GetUrl().GetQuery());
    const char* url = query.GetField("url");
    if (url) {
        // look for handler
        CStreamHandler* handler = NULL;
        NPT_Result res = NPT_ContainerFind(m_StreamHandlers, CStreamHandlerFinder(NULL, url), handler);
        if (NPT_SUCCEEDED(res)) {
            return handler->ProcessFileRequest(request, response);
        }
    }

    response = new NPT_HttpResponse(404, "File Not Found");
    response->GetHeaders().SetHeader("Server", "Platinum/" PLT_PLATINUM_VERSION_STRING);
    return NPT_SUCCESS;
}

/*----------------------------------------------------------------------
|   CMediaCrawler::AddStreamHandler
+---------------------------------------------------------------------*/
NPT_Result 
CMediaCrawler::AddStreamHandler(CStreamHandler* handler) 
{
    // make sure we don't have a metadatahandler registered for the same extension
    //    PLT_StreamHandler* prev_handler;
    //    NPT_Result ret = NPT_ContainerFind(m_StreamHandlers, PLT_StreamHandlerFinder(handler->GetExtension()), prev_handler);
    //    if (NPT_SUCCEEDED(ret)) {
    //        return NPT_ERROR_INVALID_PARAMETERS;
    //    }

    m_StreamHandlers.Add(handler);
    return NPT_SUCCESS;
}
