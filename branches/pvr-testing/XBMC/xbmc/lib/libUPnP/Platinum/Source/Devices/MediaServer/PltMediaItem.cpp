/*****************************************************************
|
|   Platinum - AV Media Item
|
| Copyright (c) 2004-2008, Plutinosoft, LLC.
| All rights reserved.
| http://www.plutinosoft.com
|
| This program is free software; you can redistribute it and/or
| modify it under the terms of the GNU General Public License
| as published by the Free Software Foundation; either version 2
| of the License, or (at your option) any later version.
|
| OEMs, ISVs, VARs and other distributors that combine and 
| distribute commercially licensed software with Platinum software
| and do not wish to distribute the source code for the commercially
| licensed software under version 2, or (at your option) any later
| version, of the GNU General Public License (the "GPL") must enter
| into a commercial license agreement with Plutinosoft, LLC.
| 
| This program is distributed in the hope that it will be useful,
| but WITHOUT ANY WARRANTY; without even the implied warranty of
| MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
| GNU General Public License for more details.
|
| You should have received a copy of the GNU General Public License
| along with this program; see the file LICENSE.txt. If not, write to
| the Free Software Foundation, Inc., 
| 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
| http://www.gnu.org/licenses/gpl-2.0.html
|
****************************************************************/

/*----------------------------------------------------------------------
|   includes
+---------------------------------------------------------------------*/
#include "PltMediaItem.h"
#include "PltMediaServer.h"
#include "PltMetadataHandler.h"
#include "PltDidl.h"
#include "PltXmlHelper.h"
#include "PltService.h"

NPT_SET_LOCAL_LOGGER("platinum.media.server.item")

extern const char* didl_namespace_dc;
extern const char* didl_namespace_upnp;

/*----------------------------------------------------------------------
|   PLT_PersonRoles::AddPerson
+---------------------------------------------------------------------*/
NPT_Result
PLT_PersonRoles::Add(const NPT_String& name, const NPT_String& role /* = "" */)
{
    PLT_PersonRole person;
    person.name = name;
    person.role = role;

    return NPT_List<PLT_PersonRole>::Add(person);
}

/*----------------------------------------------------------------------
|   PLT_PersonRoles::ToDidl
+---------------------------------------------------------------------*/
NPT_Result
PLT_PersonRoles::ToDidl(NPT_String& didl, const NPT_String& tag)
{
    NPT_String tmp;
    for (NPT_List<PLT_PersonRole>::Iterator it = 
         NPT_List<PLT_PersonRole>::GetFirstItem(); it; it++) {
        // if there's an empty artist, allow it only if there's nothing else
        if (it->name.IsEmpty() && m_ItemCount>1 && !tmp.IsEmpty()) continue;

        tmp += "<upnp:" + tag;
        if (!it->role.IsEmpty()) {
            tmp += " role=\"";
            PLT_Didl::AppendXmlEscape(tmp, it->role);
            tmp += "\"";
        }
        tmp += ">";
        PLT_Didl::AppendXmlEscape(tmp, it->name);
        tmp += "</upnp:" + tag + ">";
    }

    didl += tmp;
    return NPT_SUCCESS;
}

/*----------------------------------------------------------------------
|   PLT_PersonRoles::ToDidl
+---------------------------------------------------------------------*/
NPT_Result
PLT_PersonRoles::FromDidl(const NPT_Array<NPT_XmlElementNode*>& nodes)
{
    for (NPT_Cardinal i=0; i<nodes.GetItemCount(); i++) {
        PLT_PersonRole person;
        const NPT_String* name = nodes[i]->GetText();
        const NPT_String* role = nodes[i]->GetAttribute("role");
        if (name) person.name = *name;
        if (role) person.role = *role;
        NPT_CHECK(NPT_List<PLT_PersonRole>::Add(person));
    }
    return NPT_SUCCESS;
}

/*----------------------------------------------------------------------
|   PLT_MediaItemResource::PLT_MediaItemResource
+---------------------------------------------------------------------*/
PLT_MediaItemResource::PLT_MediaItemResource()
{
    m_Uri             = "";
    m_ProtocolInfo    = "";
    m_Duration        = (NPT_UInt32)-1;
    m_Size            = (NPT_Size)-1;
    m_Protection      = "";
    m_Bitrate         = (NPT_UInt32)-1;
    m_BitsPerSample   = (NPT_UInt32)-1;
    m_SampleFrequency = (NPT_UInt32)-1;
    m_NbAudioChannels = (NPT_UInt32)-1;
    m_Resolution      = "";
    m_ColorDepth      = (NPT_UInt32)-1;
}

const NPT_HttpFileRequestHandler_FileTypeMapEntry 
PLT_HttpFileRequestHandler_360FileTypeMap[] = {
    {"avi",  "video/avi"},
    {"divx", "video/avi"},
    {"xvid", "video/avi"}
};

const NPT_HttpFileRequestHandler_FileTypeMapEntry 
PLT_HttpFileRequestHandler_PS3FileTypeMap[] = {
    {"avi",  "video/x-msvideo"},
    {"divx", "video/x-msvideo"},
    {"xvid", "video/x-msvideo"}
};

/*----------------------------------------------------------------------
|   PLT_HttpFileRequestHandler_DefaultDlnaExtMapEntry
+---------------------------------------------------------------------*/
typedef struct PLT_HttpFileRequestHandler_DefaultDlnaExtMapEntry {
    const char* mime_type;
    const char* dlna_ext;
} PLT_HttpFileRequestHandler_DefaultDlnaExtMapEntry ;

static const PLT_HttpFileRequestHandler_DefaultDlnaExtMapEntry 
PLT_HttpFileRequestHandler_DefaultDlnaMap[] = {
    {"image/gif",      "DLNA.ORG_PN=GIF_LRG"},
    {"image/jpeg",     "DLNA.ORG_PN=JPEG_LRG"},
    {"image/jp2",      "DLNA.ORG_PN=JPEG_LRG"},
    {"image/png",      "DLNA.ORG_PN=PNG_LRG"},
    {"image/bmp",      "DLNA.ORG_PN=BMP_LRG"},
    {"image/tiff",     "DLNA.ORG_PN=TIFF_LRG"},
    {"audio/mpeg",     "DLNA.ORG_PN=MP3;DLNA.ORG_OP=01;DLNA.ORG_CI=0"},
    {"audio/mp4",      "DLNA.ORG_PN=AAC_ISO_320;DLNA.ORG_OP=01;DLNA.ORG_CI=0"},
    {"audio/x-ms-wma", "DLNA.ORG_PN=WMABASE;DLNA.ORG_OP=01;DLNA.ORG_CI=0"},
    {"audio/x-wav",    "DLNA.ORG_PN=WAV;DLNA.ORG_OP=01;DLNA.ORG_CI=0"},
    {"video/avi",      "DLNA.ORG_PN=AVI;DLNA.ORG_OP=01;DLNA.ORG_CI=0"},
    {"video/mp4",      "DLNA.ORG_PN=MPEG4_P2_SP_AAC;DLNA.ORG_CI=0"},
    {"video/mpeg",     "DLNA.ORG_PN=MPEG_PS_PAL;DLNA.ORG_OP=01;DLNA.ORG_CI=0"},
    {"video/x-ms-wmv", "DLNA.ORG_PN=WMVMED_FULL;DLNA.ORG_OP=01;DLNA.ORG_CI=0"},
    {"video/x-msvideo","DLNA.ORG_PN=AVI;DLNA.ORG_OP=01;DLNA.ORG_CI=0"},
    {"video/x-ms-asf", "DLNA.ORG_OP=01;DLNA.ORG_CI=0"}
};

static const PLT_HttpFileRequestHandler_DefaultDlnaExtMapEntry 
PLT_HttpFileRequestHandler_360DlnaMap[] = {
    {"video/mp4",  "DLNA.ORG_OP=01;DLNA.ORG_CI=0"}
};

static const PLT_HttpFileRequestHandler_DefaultDlnaExtMapEntry 
PLT_HttpFileRequestHandler_PS3DlnaMap[] = {
    {"image/jpg",  "DLNA.ORG_OP=01"},
    {"image/png",  "DLNA.ORG_OP=01"}
};

/*----------------------------------------------------------------------
|   PLT_MediaObject::GetMimeType
+---------------------------------------------------------------------*/
const char* 
PLT_MediaObject::GetMimeType(const NPT_String& filename,
                             const PLT_HttpRequestContext* context /* = NULL */)
{
    int last_dot = filename.ReverseFind('.');
    if (last_dot > 0) {
        NPT_String extension = filename.GetChars()+last_dot+1;
        extension.MakeLowercase();
        
        return GetMimeTypeFromExtension(extension, context);
    }

    return "application/unknown";
}

/*----------------------------------------------------------------------
|   PLT_MediaObject::GetMimeTypeFromExtension
+---------------------------------------------------------------------*/
const char* 
PLT_MediaObject::GetMimeTypeFromExtension(const NPT_String& extension,
                                          const PLT_HttpRequestContext* context /* = NULL */)
{
    if (context) {
        NPT_HttpRequest& request = (NPT_HttpRequest&)context->GetRequest();
        const NPT_String* agent = request.GetHeaders().GetHeaderValue(NPT_HTTP_HEADER_USER_AGENT);
        const NPT_String* hdr = request.GetHeaders().GetHeaderValue("X-AV-Client-Info");

        // look for special case for 360
        if (agent && (agent->Find("XBox", 0, true) >= 0 || agent->Find("Xenon", 0, true) >= 0)) {
            for (unsigned int i=0; i<NPT_ARRAY_SIZE(PLT_HttpFileRequestHandler_360FileTypeMap); i++) {
                if (extension == PLT_HttpFileRequestHandler_360FileTypeMap[i].extension) {
                    return PLT_HttpFileRequestHandler_360FileTypeMap[i].mime_type;
                }
            }

            // fallback to default if not found
        } else if (hdr && hdr->Find("PLAYSTATION 3", 0, true) >= 0) {
            for (unsigned int i=0; i<NPT_ARRAY_SIZE(PLT_HttpFileRequestHandler_PS3FileTypeMap); i++) {
                if (extension == PLT_HttpFileRequestHandler_PS3FileTypeMap[i].extension) {
                    return PLT_HttpFileRequestHandler_PS3FileTypeMap[i].mime_type;
                }
            }

            // fallback to default if not found
        }
    }

    for (unsigned int i=0; i<NPT_ARRAY_SIZE(NPT_HttpFileRequestHandler_DefaultFileTypeMap); i++) {
        if (extension == NPT_HttpFileRequestHandler_DefaultFileTypeMap[i].extension) {
            return NPT_HttpFileRequestHandler_DefaultFileTypeMap[i].mime_type;
        }
    }

    return "application/unknown";
}

/*----------------------------------------------------------------------
|   PLT_MediaObject::GetDlnaExtension
+---------------------------------------------------------------------*/
const char* 
PLT_MediaObject::GetDlnaExtension(const char*                   mime_type,
                                  const PLT_HttpRequestContext* context /* = NULL */)
{
    NPT_String _mime_type = mime_type;
    _mime_type.MakeLowercase();
    
    if (context) {
        NPT_HttpRequest& request = (NPT_HttpRequest&)context->GetRequest();
        const NPT_String* agent = request.GetHeaders().GetHeaderValue(NPT_HTTP_HEADER_USER_AGENT);
        const NPT_String* hdr = request.GetHeaders().GetHeaderValue("X-AV-Client-Info");

        // look for special case for 360
        if (agent && (agent->Find("XBox", 0, true) >= 0 || agent->Find("Xenon", 0, true) >= 0)) {
            for (unsigned int i=0; i<NPT_ARRAY_SIZE(PLT_HttpFileRequestHandler_360DlnaMap); i++) {
                if (_mime_type == PLT_HttpFileRequestHandler_360DlnaMap[i].mime_type) {
                    return PLT_HttpFileRequestHandler_360DlnaMap[i].dlna_ext;
                }
            }
            
            return "*"; // Should we try default dlna instead?
        } else if (hdr && hdr->Find("PLAYSTATION 3", 0, true) >= 0) {
            for (unsigned int i=0; i<NPT_ARRAY_SIZE(PLT_HttpFileRequestHandler_PS3DlnaMap); i++) {
                if (_mime_type == PLT_HttpFileRequestHandler_PS3DlnaMap[i].mime_type) {
                    return PLT_HttpFileRequestHandler_PS3DlnaMap[i].dlna_ext;
                }
            }
            
            return "DLNA.ORG_OP=01"; // Should we try default dlna instead?
        }
    }

    for (unsigned int i=0; i<NPT_ARRAY_SIZE(PLT_HttpFileRequestHandler_DefaultDlnaMap); i++) {
        if (_mime_type == PLT_HttpFileRequestHandler_DefaultDlnaMap[i].mime_type) {
            return PLT_HttpFileRequestHandler_DefaultDlnaMap[i].dlna_ext;
        }
    }

    return "*";
}

/*----------------------------------------------------------------------
|   PLT_MediaObject::GetProtInfo
+---------------------------------------------------------------------*/
NPT_String
PLT_MediaObject::GetProtInfo(const char*                   filepath, 
                             const PLT_HttpRequestContext* context /* = NULL */)
{
    NPT_String mime_type = GetMimeType(filepath, context);
    return "http-get:*:"+mime_type+":"+GetDlnaExtension(mime_type, context);
}

/*----------------------------------------------------------------------
|   PLT_MediaObject::GetUPnPClass
+---------------------------------------------------------------------*/
const char*
PLT_MediaObject::GetUPnPClass(const char*                   filepath, 
                              const PLT_HttpRequestContext* context /* = NULL */)
{
    NPT_COMPILER_UNUSED(context);

    const char* ret = NULL;
    NPT_String mime_type = GetMimeType(filepath, context);

    if (mime_type.StartsWith("audio")) {
        ret = "object.item.audioItem.musicTrack";
    } else if (mime_type.StartsWith("video")) {
        ret = "object.item.videoItem"; //Note: 360 wants "object.item.videoItem" and not "object.item.videoItem.Movie"
    } else if (mime_type.StartsWith("image")) {
        ret = "object.item.imageItem.photo";
    } else {
        ret = "object.item";
    }

    return ret;
}

/*----------------------------------------------------------------------
|   PLT_MediaObject::Reset
+---------------------------------------------------------------------*/
NPT_Result
PLT_MediaObject::Reset() 
{
    m_ObjectClass.type = "";
    m_ObjectClass.friendly_name = "";
    m_ObjectID = "";
    m_ParentID = "";

    m_Title = "";
    m_Creator = "";
    m_Date = "";
    m_Restricted = true;

    m_People.actors.Clear();
    m_People.artists.Clear();    
    m_People.authors.Clear();

    m_Affiliation.album     = "";
    m_Affiliation.genre.Clear();
    m_Affiliation.playlist  = "";

    m_Description.description = "";
    m_Description.long_description = "";
    m_ExtraInfo.album_art_uri = "";
    m_ExtraInfo.artist_discography_uri = "";

    m_MiscInfo.original_track_number = 0;
    m_MiscInfo.dvdregioncode = 0;
    m_MiscInfo.toc = "";
    m_MiscInfo.user_annotation = "";

    m_Resources.Clear();

    m_Didl = "";

    return NPT_SUCCESS;
}

/*----------------------------------------------------------------------
|   PLT_MediaObject::ToDidl
+---------------------------------------------------------------------*/
NPT_Result
PLT_MediaObject::ToDidl(NPT_UInt32 mask, NPT_String& didl)
{
    // title is required
    didl += "<dc:title>";
    PLT_Didl::AppendXmlEscape(didl, m_Title);
    didl += "</dc:title>";

    // creator
    if (mask & PLT_FILTER_MASK_CREATOR) {
        didl += "<dc:creator>";
        if (m_Creator.IsEmpty()) m_Creator = "Unknown";
        PLT_Didl::AppendXmlEscape(didl, m_Creator);
        didl += "</dc:creator>";
    }

    // date
    if (mask & PLT_FILTER_MASK_DATE && !m_Date.IsEmpty()) {
        didl += "<dc:date>";
        PLT_Didl::AppendXmlEscape(didl, m_Date);
        didl += "</dc:date>";
    } 

    // artist
    if (mask & PLT_FILTER_MASK_ARTIST) {
        // force an empty artist just in case (not DLNA Compliant though)
        //if (m_People.artists.GetItemCount() == 0) m_People.artists.Add("");
        m_People.artists.ToDidl(didl, "artist");
    }

    // actor
    if (mask & PLT_FILTER_MASK_ACTOR) {
        m_People.actors.ToDidl(didl, "actor");
    }

    // actor
    if (mask & PLT_FILTER_MASK_AUTHOR) {
        m_People.authors.ToDidl(didl, "author");
    }
    
    // album
    if (mask & PLT_FILTER_MASK_ALBUM && !m_Affiliation.album.IsEmpty()) {
        didl += "<upnp:album>";
        PLT_Didl::AppendXmlEscape(didl, m_Affiliation.album);
        didl += "</upnp:album>";
    }

    // genre
    if (mask & PLT_FILTER_MASK_GENRE) {
        // Add unknown genre
        if (m_Affiliation.genre.GetItemCount() == 0) 
            m_Affiliation.genre.Add("Unknown");

        for (NPT_List<NPT_String>::Iterator it = 
             m_Affiliation.genre.GetFirstItem(); it; ++it) {
            didl += "<upnp:genre>";
            PLT_Didl::AppendXmlEscape(didl, (*it));
            didl += "</upnp:genre>";        
        }
    }

    // album art URI
    if (mask & PLT_FILTER_MASK_ALBUMARTURI && !m_ExtraInfo.album_art_uri.IsEmpty()) {
        didl += "<upnp:albumArtURI";
        if (!m_ExtraInfo.album_art_uri_dlna_profile.IsEmpty()) {
            didl += " dlna:profileID=\"";
            PLT_Didl::AppendXmlEscape(didl, m_ExtraInfo.album_art_uri_dlna_profile);
            didl += "\"";
        }
        didl += ">";
        PLT_Didl::AppendXmlEscape(didl, m_ExtraInfo.album_art_uri);
        didl += "</upnp:albumArtURI>";
    }

    // description
    if (mask & PLT_FILTER_MASK_DESCRIPTION && !m_Description.long_description.IsEmpty()) {
        didl += "<upnp:longDescription>";
        PLT_Didl::AppendXmlEscape(didl, m_Description.long_description);
        didl += "</upnp:longDescription>";
    }

    // original track number
    if (mask & PLT_FILTER_MASK_ORIGINALTRACK && m_MiscInfo.original_track_number > 0) {
        didl += "<upnp:originalTrackNumber>";
        didl += NPT_String::FromInteger(m_MiscInfo.original_track_number);
        didl += "</upnp:originalTrackNumber>";
    }

    // resource
    if (mask & PLT_FILTER_MASK_RES) {
        for (NPT_Cardinal i=0; i<m_Resources.GetItemCount(); i++) {
            if (m_Resources[i].m_ProtocolInfo.GetLength() > 0) {
                // protocol info is required
                didl += "<res";
                
                if (mask & PLT_FILTER_MASK_RES_DURATION && m_Resources[i].m_Duration != (NPT_UInt32)-1) {
                    didl += " duration=\"";
                    PLT_Didl::FormatTimeStamp(didl, m_Resources[i].m_Duration);
                    didl += "\"";
                }

                if (mask & PLT_FILTER_MASK_RES_SIZE && m_Resources[i].m_Size != (NPT_Size)-1) {
                    didl += " size=\"";
                    didl += NPT_String::FromIntegerU(m_Resources[i].m_Size);
                    didl += "\"";
                }

                if (mask & PLT_FILTER_MASK_RES_PROTECTION && !m_Resources[i].m_Protection.IsEmpty()) {
                    didl += " protection=\"";
                    PLT_Didl::AppendXmlEscape(didl, m_Resources[i].m_Protection);
                    didl += "\"";
                }
                
                if (mask & PLT_FILTER_MASK_RES_RESOLUTION && !m_Resources[i].m_Resolution.IsEmpty()) {
                    didl += " resolution=\"";
                    PLT_Didl::AppendXmlEscape(didl, m_Resources[i].m_Resolution);
                    didl += "\"";
                }
                
                if (mask & PLT_FILTER_MASK_RES_BITRATE && m_Resources[i].m_Bitrate != (NPT_Size)-1) {                    
                    didl += " bitrate=\"";
                    didl += NPT_String::FromIntegerU(m_Resources[i].m_Bitrate);
                    didl += "\"";
                }
                
                didl += " protocolInfo=\"";
                PLT_Didl::AppendXmlEscape(didl, m_Resources[i].m_ProtocolInfo);
                didl += "\">";
                PLT_Didl::AppendXmlEscape(didl, m_Resources[i].m_Uri);
                didl += "</res>";
            }
        }
    }

    // class is required
    didl += "<upnp:class>";
    PLT_Didl::AppendXmlEscape(didl, m_ObjectClass.type);
    didl += "</upnp:class>";

    return NPT_SUCCESS;
}

/*----------------------------------------------------------------------
|   PLT_MediaObject::FromDidl
+---------------------------------------------------------------------*/
NPT_Result
PLT_MediaObject::FromDidl(NPT_XmlElementNode* entry)
{
    NPT_String str, xml;
    NPT_Array<NPT_XmlElementNode*> children;
    NPT_Result res;

    // check if item is restricted (is default true?)
    if (NPT_SUCCEEDED(PLT_XmlHelper::GetAttribute(entry, "restricted", str))) {
        m_Restricted = PLT_Service::IsTrue(str);
    }

    res = PLT_XmlHelper::GetAttribute(entry, "id", m_ObjectID);
    NPT_CHECK_LABEL_SEVERE(res, cleanup);

    res = PLT_XmlHelper::GetAttribute(entry, "parentID", m_ParentID);
    NPT_CHECK_LABEL_SEVERE(res, cleanup);

    res = PLT_XmlHelper::GetChildText(entry, "title", m_Title, didl_namespace_dc);
    NPT_CHECK_LABEL_SEVERE(res, cleanup);

    res = PLT_XmlHelper::GetChildText(entry, "class", m_ObjectClass.type, didl_namespace_upnp);
    NPT_CHECK_LABEL_SEVERE(res, cleanup);

    // read non-required elements
    PLT_XmlHelper::GetChildText(entry, "creator", m_Creator, didl_namespace_dc);
    PLT_XmlHelper::GetChildText(entry, "date", m_Date, didl_namespace_dc);

    PLT_XmlHelper::GetChildren(entry, children, "artist", didl_namespace_upnp);
    m_People.artists.FromDidl(children);

    PLT_XmlHelper::GetChildText(entry, "album", m_Affiliation.album, didl_namespace_upnp);

    children.Clear();
    PLT_XmlHelper::GetChildren(entry, children, "genre", didl_namespace_upnp);
    for (NPT_Cardinal i=0; i<children.GetItemCount(); i++) {
        if (children[i]->GetText()) {
            m_Affiliation.genre.Add(*children[i]->GetText());
        }
    }

    PLT_XmlHelper::GetChildText(entry, "albumArtURI", m_ExtraInfo.album_art_uri, didl_namespace_upnp);
    PLT_XmlHelper::GetChildText(entry, "longDescription", m_Description.long_description, didl_namespace_upnp);
    PLT_XmlHelper::GetChildText(entry, "originalTrackNumber", str, didl_namespace_upnp);
    NPT_UInt32 value;
    if (NPT_FAILED(str.ToInteger(value))) value = 0;
    m_MiscInfo.original_track_number = value;

    children.Clear();
    PLT_XmlHelper::GetChildren(entry, children, "res");
    if (children.GetItemCount() > 0) {
        for (NPT_Cardinal i=0; i<children.GetItemCount(); i++) {
            PLT_MediaItemResource resource;
            if (children[i]->GetText() == NULL) {
                res = NPT_FAILURE;
                NPT_CHECK_LABEL_SEVERE(res, cleanup);
            }

            resource.m_Uri = *children[i]->GetText();
            
            // validate uri
            NPT_HttpUrl url(resource.m_Uri);
            NPT_IpAddress ip;
            if (!url.IsValid() || NPT_FAILED(ip.Parse(url.GetHost()))) {
                res = NPT_FAILURE;
                NPT_CHECK_LABEL_SEVERE(res, cleanup);
            }
            res = PLT_XmlHelper::GetAttribute(children[i], "protocolInfo", resource.m_ProtocolInfo);
            NPT_CHECK_LABEL_SEVERE(res, cleanup);
            
            PLT_XmlHelper::GetAttribute(children[i], "protection", resource.m_Protection);
            PLT_XmlHelper::GetAttribute(children[i], "resolution", resource.m_Resolution);

            if (NPT_SUCCEEDED(PLT_XmlHelper::GetAttribute(children[i], "size", str))) {
                if (NPT_FAILED(str.ToInteger64(resource.m_Size))) resource.m_Size = (NPT_Size)-1;
            }

            if (NPT_SUCCEEDED(PLT_XmlHelper::GetAttribute(children[i], "duration", str))) {
                if (NPT_FAILED(PLT_Didl::ParseTimeStamp(str, resource.m_Duration))) {
                    // if error while converting, ignore and set to -1 to indicate we don't know the duration
                    resource.m_Duration = (NPT_UInt32)-1;
                }

                // DLNA: reformat duration in case it was wrong
                str = "";
                PLT_Didl::FormatTimeStamp(str, resource.m_Duration);
                PLT_XmlHelper::SetAttribute(children[i], "duration", str); 
            }    
            m_Resources.Add(resource);
        }
    }

    
    // serialize the entry Didl as a we might need to pass it to a renderer
    res = PLT_XmlHelper::Serialize(*entry, xml);
    NPT_CHECK_LABEL_SEVERE(res, cleanup);
    
    m_Didl = didl_header + xml + didl_footer;    
    return NPT_SUCCESS;

cleanup:
    return res;
}

/*----------------------------------------------------------------------
|   PLT_MediaObjectList::PLT_MediaObjectList
+---------------------------------------------------------------------*/
PLT_MediaObjectList::PLT_MediaObjectList()
{
}

/*----------------------------------------------------------------------
|   PLT_MediaObjectList::~PLT_MediaObjectList
+---------------------------------------------------------------------*/
PLT_MediaObjectList::~PLT_MediaObjectList()
{
    Apply(NPT_ObjectDeleter<PLT_MediaObject>());
}

/*----------------------------------------------------------------------
|   PLT_MediaItem::PLT_MediaItem
+---------------------------------------------------------------------*/
PLT_MediaItem::PLT_MediaItem()
{
    Reset();
}

/*----------------------------------------------------------------------
|   PLT_MediaItem::~PLT_MediaItem
+---------------------------------------------------------------------*/
PLT_MediaItem::~PLT_MediaItem()
{
}

/*----------------------------------------------------------------------
|   PLT_MediaItem::ToDidl
+---------------------------------------------------------------------*/
NPT_Result
PLT_MediaItem::ToDidl(NPT_UInt32 mask, NPT_String& didl)
{
    NPT_String tmp;
    // Allocate enough space for a big string we're going to concatenate in
    tmp.Reserve(2048);

    tmp = "<item id=\"";

    PLT_Didl::AppendXmlEscape(tmp, m_ObjectID);
    tmp += "\" parentID=\"";
    PLT_Didl::AppendXmlEscape(tmp, m_ParentID);
    tmp += "\" restricted=\"";
    tmp += m_Restricted?"1\"":"0\"";

    tmp += ">";

    NPT_CHECK_SEVERE(PLT_MediaObject::ToDidl(mask, tmp));

    /* close tag */
    tmp += "</item>";

    didl += tmp;
    return NPT_SUCCESS;
}

/*----------------------------------------------------------------------
|   PLT_MediaItem::FromDidl
+---------------------------------------------------------------------*/
NPT_Result
PLT_MediaItem::FromDidl(NPT_XmlElementNode* entry)
{
    /* reset first */
    Reset();

    if (entry->GetTag().Compare("item", true) != 0)
        return NPT_ERROR_INTERNAL;

    return PLT_MediaObject::FromDidl(entry);
}

/*----------------------------------------------------------------------
|   PLT_MediaContainer::PLT_MediaContainer
+---------------------------------------------------------------------*/
PLT_MediaContainer::PLT_MediaContainer()
{
    Reset();
}

/*----------------------------------------------------------------------
|   PLT_MediaContainer::~PLT_MediaContainer
+---------------------------------------------------------------------*/
PLT_MediaContainer::~PLT_MediaContainer(void)
{
}

/*----------------------------------------------------------------------
|   PLT_MediaContainer::Reset
+---------------------------------------------------------------------*/
NPT_Result
PLT_MediaContainer::Reset() 
{
    m_SearchClasses.Clear();
    m_Searchable = true;
    m_ChildrenCount = -1;

    return PLT_MediaObject::Reset();
}

/*----------------------------------------------------------------------
|   PLT_MediaContainer::ToDidl
+---------------------------------------------------------------------*/
NPT_Result
PLT_MediaContainer::ToDidl(NPT_UInt32 mask, NPT_String& didl)
{
    NPT_String tmp;
    // Allocate enough space for a big string we're going to concatenate in
    tmp.Reserve(2048);

    tmp = "<container id=\"";

    PLT_Didl::AppendXmlEscape(tmp, m_ObjectID);
    tmp += "\" parentID=\"";
    PLT_Didl::AppendXmlEscape(tmp, m_ParentID);
    tmp += "\" restricted=\"";
    tmp += m_Restricted?"1\"":"0\"";

    if (mask & PLT_FILTER_MASK_SEARCHABLE) {
        tmp += " searchable=\"";
        tmp += m_Searchable?"1\"":"0\"";
    }
    
    if (mask & PLT_FILTER_MASK_CHILDCOUNT && m_ChildrenCount != -1) {
        tmp += " childCount=\"";
        tmp += NPT_String::FromInteger(m_ChildrenCount);
        tmp += "\"";
    }

    tmp += ">";

    NPT_CHECK_SEVERE(PLT_MediaObject::ToDidl(mask, tmp));

    /* close tag */
    tmp += "</container>";

    didl += tmp;
    return NPT_SUCCESS;
}

/*----------------------------------------------------------------------
|   PLT_MediaContainer::FromDidl
+---------------------------------------------------------------------*/
NPT_Result
PLT_MediaContainer::FromDidl(NPT_XmlElementNode* entry)
{
    NPT_String str;

    /* reset first */
    Reset();

    // check entry type
    if (entry->GetTag().Compare("Container", true) != 0) 
        return NPT_ERROR_INTERNAL;

    // check if item is searchable (is default true?)
    if (NPT_SUCCEEDED(PLT_XmlHelper::GetAttribute(entry, "searchable", str))) {
        m_Searchable = PLT_Service::IsTrue(str);
    }

    // look for childCount
    if (NPT_SUCCEEDED(PLT_XmlHelper::GetAttribute(entry, "childCount", str))) {
        NPT_UInt32 count;
        NPT_CHECK_SEVERE(str.ToInteger(count));
        m_ChildrenCount = count;
    }

    return PLT_MediaObject::FromDidl(entry);
}
