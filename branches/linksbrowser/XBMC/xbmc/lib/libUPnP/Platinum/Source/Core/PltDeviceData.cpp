/*****************************************************************
|
|   Platinum - Device Data
|
|   Copyright (c) 2004-2006 Sylvain Rebaud
|   Author: Sylvain Rebaud (sylvain@rebaud.com)
|
****************************************************************/

/*----------------------------------------------------------------------
|   includes
+---------------------------------------------------------------------*/
#include "NptUtils.h"
#include "PltDeviceData.h"
#include "PltService.h"
#include "PltUPnP.h"
#include "PltXmlHelper.h"

/*----------------------------------------------------------------------
|   PLT_DeviceData::PLT_DeviceData
+---------------------------------------------------------------------*/
PLT_DeviceData::PLT_DeviceData(NPT_HttpUrl      description_url,
                               const char*      uuid,
                               NPT_TimeInterval lease_time,
                               const char*      device_type,
                               const char*      friendly_name) :
    m_ReferenceCount(1),
    m_Root(true),
    m_UUID(uuid),
    m_URLDescription(description_url),
    m_DeviceType(device_type),
    m_FriendlyName(friendly_name)
{
    if (uuid == NULL || strlen(uuid) == 0) {
        PLT_UPnPMessageHelper::GenerateGUID(m_UUID);
    }

    SetLeaseTime(lease_time);

    // remove trailing file according to RFC 2396
    m_URLBasePath = m_URLDescription.GetPath();
    if (!m_URLBasePath.EndsWith("/")) {
        int index = m_URLBasePath.ReverseFind('/');
        if (index >= 0) m_URLBasePath.SetLength(index+1);
    }
}

/*----------------------------------------------------------------------
|   PLT_DeviceData::~PLT_DeviceData
+---------------------------------------------------------------------*/
PLT_DeviceData::~PLT_DeviceData()
{
    m_Services.Apply(NPT_ObjectDeleter<PLT_Service>());
}

/*----------------------------------------------------------------------
|   PLT_DeviceData::SetURLBase
+---------------------------------------------------------------------*/
NPT_Result
PLT_DeviceData::SetURLBase(const char* url_base) 
{
    NPT_HttpUrl url(url_base);
    if (!url.IsValid()) {
        return NPT_FAILURE;
    }

    m_URLBasePath = url.GetPath();

    // remove trailing file according to RFC 2396
    if (!m_URLBasePath.EndsWith("/")) {
        int index = m_URLBasePath.ReverseFind('/');
        if (index < 0) return NPT_FAILURE;
        m_URLBasePath.SetLength(index+1);
    }

    return NPT_SUCCESS;
}    

/*----------------------------------------------------------------------
|   PLT_DeviceData::GetDescriptionUrl
+---------------------------------------------------------------------*/
NPT_String
PLT_DeviceData::GetDescriptionUrl(const char* bind_addr)
{
    NPT_HttpUrl url = m_URLDescription;

    // override host
    if (bind_addr) url.SetHost(bind_addr);
    return url.ToString();
}

/*----------------------------------------------------------------------
|   PLT_DeviceData::GetURLBase
+---------------------------------------------------------------------*/
NPT_HttpUrl
PLT_DeviceData::GetURLBase()
{
    NPT_HttpUrl url = m_URLDescription;

    // override path
    url.SetPath(m_URLBasePath);
    return url;
}

/*----------------------------------------------------------------------
|   PLT_DeviceData::SetLeaseTime
+---------------------------------------------------------------------*/
NPT_Result   
PLT_DeviceData::SetLeaseTime(NPT_TimeInterval lease_time) 
{
    m_LeaseTime = lease_time;
    NPT_System::GetCurrentTimeStamp(m_LeaseTimeLastUpdate);
    return NPT_SUCCESS;
}

/*----------------------------------------------------------------------
|   PLT_DeviceData::GetLeaseTimeLastUpdate
+---------------------------------------------------------------------*/
NPT_TimeStamp 
PLT_DeviceData::GetLeaseTimeLastUpdate() 
{
    return m_LeaseTimeLastUpdate;
}

/*----------------------------------------------------------------------
|   PLT_DeviceData::ToLog
+---------------------------------------------------------------------*/
NPT_Result
PLT_DeviceData::ToLog()
{
    PLT_Log(PLT_LOG_LEVEL_1, "Device GUID: %s\n", (const char*)m_UUID);
    PLT_Log(PLT_LOG_LEVEL_1, "Device Type: %s\n", (const char*)m_DeviceType);
    PLT_Log(PLT_LOG_LEVEL_1, "Device Base Url: %s\n", (const char*)GetURLBase().ToString());
    PLT_Log(PLT_LOG_LEVEL_1, "Device Friendly Name: %s\n\n", (const char*)m_FriendlyName);
    return NPT_SUCCESS;
}

/*----------------------------------------------------------------------
|   PLT_DeviceData::AddDevice
+---------------------------------------------------------------------*/
NPT_Result
PLT_DeviceData::AddDevice(PLT_DeviceDataReference& device)
{
    device->m_Root = false;
    m_EmbeddedDevices.Add(device);
    return NPT_SUCCESS;
}

/*----------------------------------------------------------------------
|   PLT_DeviceData::AddService
+---------------------------------------------------------------------*/
NPT_Result
PLT_DeviceData::AddService(PLT_Service* service)
{
    if (service->m_ServiceType == "" ||
        service->m_ServiceID   == "" ||
        service->m_SCPDURL     == "" ||
        service->m_ControlURL  == "" ||
        service->m_EventSubURL == "")
    {
        return NPT_ERROR_INVALID_PARAMETERS;
    }

    return m_Services.Add(service);
}

/*----------------------------------------------------------------------
|   PLT_GetDescriptionIterator class
+---------------------------------------------------------------------*/
template <class T>
class PLT_GetDescriptionIterator
{
public:
    PLT_GetDescriptionIterator<T>(NPT_XmlElementNode* parent) :
      m_Parent(parent) {}

      NPT_Result operator()(T& data) const {
          return data->GetDescription(m_Parent);
      }

private:
    NPT_XmlElementNode* m_Parent;
};

/*----------------------------------------------------------------------
|   PLT_DeviceData::ToXML
+---------------------------------------------------------------------*/
NPT_Result
PLT_DeviceData::GetDescription(NPT_XmlElementNode* root, NPT_XmlElementNode** device_out)
{
    NPT_XmlElementNode* device = new NPT_XmlElementNode("device");
    if (device_out) {
        *device_out = device;
    }

    NPT_CHECK(root->AddChild(device));

    // device properties
    NPT_CHECK(PLT_XmlHelper::AddChildText(device, "deviceType", m_DeviceType));
    NPT_CHECK(PLT_XmlHelper::AddChildText(device, "friendlyName", m_FriendlyName));
    NPT_CHECK(PLT_XmlHelper::AddChildText(device, "manufacturer", m_Manufacturer));
    NPT_CHECK(PLT_XmlHelper::AddChildText(device, "manufacturerURL", m_ManufacturerURL));
    NPT_CHECK(PLT_XmlHelper::AddChildText(device, "modelDescription", m_ModelDescription));
    NPT_CHECK(PLT_XmlHelper::AddChildText(device, "modelName", m_ModelName));
    NPT_CHECK(PLT_XmlHelper::AddChildText(device, "modelURL", m_ModelURL));
    NPT_CHECK(PLT_XmlHelper::AddChildText(device, "modelNumber", m_ModelNumber));
    NPT_CHECK(PLT_XmlHelper::AddChildText(device, "serialNumber", m_SerialNumber));
    NPT_CHECK(PLT_XmlHelper::AddChildText(device, "UDN", "uuid:" + m_UUID));
    NPT_CHECK(PLT_XmlHelper::AddChildText(device, "presentationURL", m_PresentationURL));

    // hack for now:
    NPT_XmlElementNode* icons = new NPT_XmlElementNode("iconList");
    NPT_CHECK(device->AddChild(icons));
    NPT_XmlElementNode* icon = new NPT_XmlElementNode("icon");
    NPT_CHECK(icons->AddChild(icon));
    NPT_CHECK(PLT_XmlHelper::AddChildText(icon, "mimetype", "image/png"));
    NPT_CHECK(PLT_XmlHelper::AddChildText(icon, "width", "48"));
    NPT_CHECK(PLT_XmlHelper::AddChildText(icon, "height", "48"));
    NPT_CHECK(PLT_XmlHelper::AddChildText(icon, "depth", "24"));
    NPT_CHECK(PLT_XmlHelper::AddChildText(icon, "url", "/logo48.png"));

    icon = new NPT_XmlElementNode("icon");
    NPT_CHECK(icons->AddChild(icon));
    NPT_CHECK(PLT_XmlHelper::AddChildText(icon, "mimetype", "image/png"));
    NPT_CHECK(PLT_XmlHelper::AddChildText(icon, "width", "32"));
    NPT_CHECK(PLT_XmlHelper::AddChildText(icon, "height", "32"));
    NPT_CHECK(PLT_XmlHelper::AddChildText(icon, "depth", "24"));
    NPT_CHECK(PLT_XmlHelper::AddChildText(icon, "url", "/logo32.png"));

    // services
    NPT_XmlElementNode* services = new NPT_XmlElementNode("serviceList");
    NPT_CHECK(device->AddChild(services));
    NPT_CHECK(m_Services.ApplyUntil(PLT_GetDescriptionIterator<PLT_Service*>(services), 
        NPT_UntilResultNotEquals(NPT_SUCCESS)));

    // embedded devices
    if (m_EmbeddedDevices.GetItemCount()) {
        NPT_XmlElementNode* deviceList = new NPT_XmlElementNode("deviceList");
        NPT_CHECK(device->AddChild(deviceList));

        NPT_CHECK(m_EmbeddedDevices.ApplyUntil(PLT_GetDescriptionIterator<PLT_DeviceDataReference>(deviceList), 
            NPT_UntilResultNotEquals(NPT_SUCCESS)));
    }

    return NPT_SUCCESS;
}

/*----------------------------------------------------------------------
|   PLT_DeviceData::GetDescription
+---------------------------------------------------------------------*/
NPT_Result
PLT_DeviceData::GetDescription(NPT_String& desc)
{
    NPT_XmlElementNode* root = new NPT_XmlElementNode("root");
    NPT_CHECK(root->SetNamespaceUri("", "urn:schemas-upnp-org:device-1-0"));

    // add spec version
    NPT_XmlElementNode* spec = new NPT_XmlElementNode("specVersion");
    NPT_CHECK(root->AddChild(spec));
    NPT_CHECK(PLT_XmlHelper::AddChildText(spec, "major", "1"));
    NPT_CHECK(PLT_XmlHelper::AddChildText(spec, "minor", "0"));

    // get device xml
    NPT_CHECK(GetDescription(root));

    // serialize node
    NPT_String tmp;
    NPT_CHECK(PLT_XmlHelper::Serialize(*root, tmp));
    delete root;

    // add preprocessor
    desc = "<?xml version=\"1.0\" encoding=\"utf-8\" ?>\n" + tmp;
    return NPT_SUCCESS;
}

/*----------------------------------------------------------------------
|   PLT_DeviceData::SetDescription
+---------------------------------------------------------------------*/
NPT_Result
PLT_DeviceData::SetDescription(const char* description)
{
    NPT_XmlParser parser;
    NPT_XmlNode*  tree = NULL;
    NPT_Result    res;

    res = parser.Parse(description, tree);
    if (NPT_FAILED(res)) {
        delete tree;
        return res;
    }

    NPT_XmlElementNode* root = tree->AsElementNode();
    if (!root || root->GetTag() != "root" || !root->GetNamespace() || *root->GetNamespace() != "urn:schemas-upnp-org:device-1-0") {
        delete tree;
        return NPT_FAILURE;
    }

    // look for optional URLBase element
    NPT_String URLBase;
    if (NPT_SUCCEEDED(PLT_XmlHelper::GetChildText(root, "URLBase", URLBase))) {
        SetURLBase(URLBase);
    }

    // at least one root device child element is required
    NPT_XmlElementNode* device;
    if (!(device = PLT_XmlHelper::GetChild(root, "device"))) {
        delete tree;
        return NPT_FAILURE;
    }

    res = SetDescriptionDevice(device);

    // delete the tree
    delete tree;
    return res;
}

/*----------------------------------------------------------------------
|   PLT_DeviceData::SetDescriptionDevice
+---------------------------------------------------------------------*/
NPT_Result
PLT_DeviceData::SetDescriptionDevice(NPT_XmlElementNode* device_node)
{
    NPT_Result res;

    NPT_CHECK(PLT_XmlHelper::GetChildText(device_node, "deviceType", m_DeviceType));
    NPT_CHECK(PLT_XmlHelper::GetChildText(device_node, "UDN", m_UUID));

    // jump uuid:, we should probably check first
    m_UUID = ((const char*)m_UUID)+5;

    // optional attributes
    PLT_XmlHelper::GetChildText(device_node, "friendlyName", m_FriendlyName);
    PLT_XmlHelper::GetChildText(device_node, "manufacturer", m_Manufacturer);
    PLT_XmlHelper::GetChildText(device_node, "manufacturerURL", m_ManufacturerURL);
    PLT_XmlHelper::GetChildText(device_node, "modelDescription", m_ModelDescription);
    PLT_XmlHelper::GetChildText(device_node, "modelName", m_ModelName);
    PLT_XmlHelper::GetChildText(device_node, "modelNumber", m_ModelNumber);
    PLT_XmlHelper::GetChildText(device_node, "serialNumber", m_SerialNumber);

    // enumerate device services
    NPT_XmlElementNode* serviceList = PLT_XmlHelper::GetChild(device_node, "serviceList");
    if (serviceList) {
        NPT_Array<NPT_XmlElementNode*> services;
        PLT_XmlHelper::GetChildren(serviceList, services, "service");
        for( int k = 0 ; k < (int)services.GetItemCount(); k++) {    
            PLT_Service* service = new PLT_Service(this);
            PLT_XmlHelper::GetChildText(services[k], "serviceType", service->m_ServiceType);
            PLT_XmlHelper::GetChildText(services[k], "serviceId", service->m_ServiceID);
            PLT_XmlHelper::GetChildText(services[k], "SCPDURL", service->m_SCPDURL);
            PLT_XmlHelper::GetChildText(services[k], "controlURL", service->m_ControlURL);
            PLT_XmlHelper::GetChildText(services[k], "eventSubURL", service->m_EventSubURL);
            if (NPT_FAILED(res = AddService(service))) {
                delete service;
                return res;
            }
        }
    }

    // enumerate embedded devices
    NPT_XmlElementNode* deviceList = PLT_XmlHelper::GetChild(device_node, "deviceList");
    if (deviceList) {
        NPT_Array<NPT_XmlElementNode*> devices;
        PLT_XmlHelper::GetChildren(deviceList, devices, "device");
        for( int k = 0 ; k < (int)devices.GetItemCount(); k++) {    
            PLT_DeviceDataReference device(new PLT_DeviceData());
            NPT_CHECK(device->SetDescriptionDevice(devices[k]));
            AddDevice(device);
        }
    }

    return NPT_SUCCESS;
}

/*----------------------------------------------------------------------
|   PLT_DeviceData::FindServiceById
+---------------------------------------------------------------------*/
NPT_Result
PLT_DeviceData::FindServiceById(const char* id, PLT_Service*& service)
{
    if (NPT_SUCCEEDED(NPT_ContainerFind(m_Services, PLT_ServiceIDFinder(id), service))) {
        return NPT_SUCCESS;
    }

    //    for (int i=0; i < (int)m_EmbeddedDevices.GetItemCount(); i++) {
    //        if (NPT_SUCCEEDED(NPT_ContainerFind(m_EmbeddedDevices[i]->m_Services, PLT_ServiceSCPDURLFinder(strURI), service)))
    //            return NPT_SUCCESS;
    //    }
    return NPT_FAILURE;
}

/*----------------------------------------------------------------------
|   PLT_DeviceData::FindServiceByType
+---------------------------------------------------------------------*/
NPT_Result
PLT_DeviceData::FindServiceByType(const char* type, PLT_Service*& service)
{
    if (NPT_SUCCEEDED(NPT_ContainerFind(m_Services, PLT_ServiceTypeFinder(type), service))) {
        return NPT_SUCCESS;
    }

    //    for (int i=0; i < (int)m_EmbeddedDevices.GetItemCount(); i++) {
    //        if (NPT_SUCCEEDED(NPT_ContainerFind(m_EmbeddedDevices[i]->m_Services, PLT_ServiceSCPDURLFinder(strURI), service)))
    //            return NPT_SUCCESS;
    //    }
    return NPT_FAILURE;
}

/*----------------------------------------------------------------------
|   PLT_DeviceData::FindServiceByDescriptionURI
+---------------------------------------------------------------------*/
NPT_Result
PLT_DeviceData::FindServiceByDescriptionURI(const char* uri, PLT_Service*& service)
{
    if (NPT_SUCCEEDED(NPT_ContainerFind(m_Services, PLT_ServiceSCPDURLFinder(uri), service))) {
        return NPT_SUCCESS;
    }

    for (int i=0; i < (int)m_EmbeddedDevices.GetItemCount(); i++) {
        if (NPT_SUCCEEDED(NPT_ContainerFind(m_EmbeddedDevices[i]->m_Services, PLT_ServiceSCPDURLFinder(uri), service)))
            return NPT_SUCCESS;
    }
    return NPT_FAILURE;
}

/*----------------------------------------------------------------------
|   PLT_DeviceData::FindServiceByControlURI
+---------------------------------------------------------------------*/
NPT_Result
PLT_DeviceData::FindServiceByControlURI(const char* uri, PLT_Service*& service)
{
    if (NPT_SUCCEEDED(NPT_ContainerFind(m_Services, PLT_ServiceControlURLFinder(uri), service))) {
        return NPT_SUCCESS;
    }

    for (int i=0; i < (int)m_EmbeddedDevices.GetItemCount(); i++) {
        if (NPT_SUCCEEDED(NPT_ContainerFind(m_EmbeddedDevices[i]->m_Services, PLT_ServiceControlURLFinder(uri), service)))
            return NPT_SUCCESS;
    }
    return NPT_FAILURE;
}

/*----------------------------------------------------------------------
|   PLT_DeviceData::FindServiceByEventSubURI
+---------------------------------------------------------------------*/
NPT_Result
PLT_DeviceData::FindServiceByEventSubURI(const char* uri, PLT_Service*& service)
{       
    if (NPT_SUCCEEDED(NPT_ContainerFind(m_Services, PLT_ServiceEventSubURLFinder(uri), service))) {
        return NPT_SUCCESS;
    }

    for (int i=0; i < (int)m_EmbeddedDevices.GetItemCount(); i++) {
        if (NPT_SUCCEEDED(NPT_ContainerFind(m_EmbeddedDevices[i]->m_Services, PLT_ServiceEventSubURLFinder(uri), service)))
            return NPT_SUCCESS;
    }
    return NPT_FAILURE;
}
