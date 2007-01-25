/*
* UPnP Support for XBox Media Center
* Copyright (c) 2006 c0diq (Sylvain Rebaud)
* Portions Copyright (c) by the authors of libPlatinum
*
* http://www.plutinosoft.com/blog/category/platinum/
*
* This program is free software; you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation; either version 2 of the License, or
* (at your option) any later version.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with this program; if not, write to the Free Software
* Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
*/


#include "../stdafx.h"
#include "../util.h"
#include "UPnPDirectory.h"
#include "../UPnP.h"
#include "Platinum.h"
#include "PltSyncMediaBrowser.h"

using namespace DIRECTORY;

namespace DIRECTORY
{
/*----------------------------------------------------------------------
|   CUPnPDirectory::GetFriendlyName
+---------------------------------------------------------------------*/
const char* 
CUPnPDirectory::GetFriendlyName(const char* url)
{
    NPT_String path = url;
    if (!path.EndsWith("/")) path += "/";

    if (path.Left(7).Compare("upnp://", true) != 0) {
        return NULL;
    } else if (path.Compare("upnp://", true) == 0) {
        return "UPnP Media Servers (Auto-Discover)";
    } 

    // look for nextslash 
    int next_slash = path.Find('/', 7);
    if (next_slash == -1) 
        return NULL;

    NPT_String uuid = path.SubString(7, next_slash-7);
    NPT_String object_id = path.SubString(next_slash+1, path.GetLength()-next_slash-2);

    // look for device 
    PLT_DeviceDataReference* device;
    const NPT_Lock<PLT_DeviceMap>& devices = CUPnP::GetInstance()->m_MediaBrowser->GetMediaServers();
    if (NPT_FAILED(devices.Get(uuid, device)) || device == NULL) 
        return NULL;

    return (const char*)(*device)->GetFriendlyName();
}

/*----------------------------------------------------------------------
|   CUPnPDirectory::GetDirectory
+---------------------------------------------------------------------*/
bool 
CUPnPDirectory::GetDirectory(const CStdString& strPath, CFileItemList &items)
{
    CFileItemList vecCacheItems;
    CUPnP* upnp = CUPnP::GetInstance();

    // start client if it hasn't been done yet
    upnp->StartClient();
                     

    // We accept upnp://devuuid[/item_id]
    // make sure we have a slash to look for at the end
    CStdString strRoot = strPath;
    if (!CUtil::HasSlashAtEnd(strRoot)) strRoot += "/";

    NPT_String path = strPath.c_str();

    if (path.Left(7).Compare("upnp://", true) != 0) {
        return false;
    } 
    
    if (path.Compare("upnp://", true) == 0) {
        // root -> get list of devices 
        const NPT_Lock<PLT_DeviceMap>& devices = upnp->m_MediaBrowser->GetMediaServers();
        const NPT_List<PLT_DeviceMapEntry*>& entries = devices.GetEntries();
        NPT_List<PLT_DeviceMapEntry*>::Iterator entry = entries.GetFirstItem();
        while (entry) {
            PLT_DeviceDataReference device = (*entry)->GetValue();
            NPT_String name   = device->GetFriendlyName();
            NPT_String uuid = (*entry)->GetKey();

            CFileItem *pItem = new CFileItem((const char*)name);
            pItem->m_strPath = (const char*) path + uuid;
            pItem->m_bIsFolder = true;

            if (!CUtil::HasSlashAtEnd(pItem->m_strPath)) pItem->m_strPath += '/';

            vecCacheItems.Add(pItem);
            items.Add(new CFileItem(*pItem));

            ++entry;
        }
    } else {
        // look for nextslash 
        int next_slash = path.Find('/', 7);
        if (next_slash == -1) 
            return false;

        NPT_String uuid = path.SubString(7, next_slash-7);
        NPT_String object_id = path.SubString(next_slash+1, path.GetLength()-next_slash-2);

        // look for device 
        PLT_DeviceDataReference* device;
        const NPT_Lock<PLT_DeviceMap>& devices = upnp->m_MediaBrowser->GetMediaServers();
        if (NPT_FAILED(devices.Get(uuid, device)) || device == NULL) 
            return false;

        // issue a browse request with object_id
        // if object_id is empty use "0" for root 
        NPT_String root_id = object_id.IsEmpty()?"0":object_id;

        // just a guess as to what types of files we want */
        bool video = true;
        bool audio = true;
        bool image = true;
        if( !m_strFileMask.IsEmpty() ) {
          video = m_strFileMask.Find(".wmv") >= 0;
          audio = m_strFileMask.Find(".wma") >= 0;
          image = m_strFileMask.Find(".jpg") >= 0;
        }

        // special case for Windows Media Connect and WMP11 when looking for root
        // We can target which root subfolder we want based on directory mask
        if (root_id == "0" 
         && ((*device)->GetFriendlyName().Find("Windows Media Connect", 0, true) >= 0
           ||(*device)->m_ModelName == "Windows Media Player Sharing")) {

            // look for a specific type to differentiate which folder we want
            if (audio && !video && !image) {
                // music
                root_id = "1";
            } else if (!audio && video && !image) {
                // video
                root_id = "2";
            } else if (!audio && !video && image) {
                // pictures
                root_id = "3";
            }
        }

        // same thing but special case for Xbox Media Center
        if (root_id == "0" && ((*device)->m_ModelName.Find("Xbox Media Center", 0, true) >= 0)) {

            // look for a specific type to differentiate which folder we want
            if (audio && !video && !image) {
                // music
                root_id = "virtualpath://upnpmusic";
            } else if (!audio && video && !image) {
                // video
                root_id = "virtualpath://upnpvideo";
            } else if (!audio && !video && image) {
                // pictures
                root_id = "virtualpath://upnppictures";
            }
        }

        // if error, the list could be partial and that's ok
        // we still want to process it
        PLT_MediaObjectListReference list;
        upnp->m_MediaBrowser->Browse(*device, root_id, list);
        if (list.IsNull()) return false;

        PLT_MediaObjectList::Iterator entry = list->GetFirstItem();
        while (entry) {
            // disregard items with wrong class/type
            if( (!video && (*entry)->m_ObjectClass.type.CompareN("object.item.videoitem", 21,true) == 0)
             || (!audio && (*entry)->m_ObjectClass.type.CompareN("object.item.audioitem", 21,true) == 0)
             || (!image && (*entry)->m_ObjectClass.type.CompareN("object.item.imageitem", 21,true) == 0) )
            {
                ++entry;
                continue;
            }

            CFileItem *pItem = new CFileItem((const char*)(*entry)->m_Title);
            pItem->m_bIsFolder = (*entry)->IsContainer();

            // if it's a container, format a string as upnp://uuid/object_id/ 
            if (pItem->m_bIsFolder) {
                pItem->m_strPath = (const char*) NPT_String("upnp://") + uuid + "/" + (*entry)->m_ObjectID;
                if (!CUtil::HasSlashAtEnd(pItem->m_strPath)) pItem->m_strPath += '/';

            } else {
                if ((*entry)->m_Resources.GetItemCount()) {
                    // if it's an item, path is the first url to the item
                    // we hope the server made the first one reachable for us
                    // (it could be a format we dont know how to play however)
                    pItem->m_strPath = (const char*) (*entry)->m_Resources[0].m_Uri;

                    // set metadata
                    if ((*entry)->m_Resources[0].m_Size > 0) {
                        pItem->m_dwSize  = (*entry)->m_Resources[0].m_Size;
                    }
                    pItem->m_musicInfoTag.SetDuration((*entry)->m_Resources[0].m_Duration);
                    pItem->m_musicInfoTag.SetGenre((const char*) (*entry)->m_Affiliation.genre);
                    pItem->m_musicInfoTag.SetAlbum((const char*) (*entry)->m_Affiliation.album);
                    
                    // some servers (like WMC) use upnp:artist instead of dc:creator
                    if ((*entry)->m_Creator.GetLength() == 0) {
                        pItem->m_musicInfoTag.SetArtist((const char*) (*entry)->m_People.artist);
                    } else {
                        pItem->m_musicInfoTag.SetArtist((const char*) (*entry)->m_Creator);
                    }
                    pItem->m_musicInfoTag.SetTitle((const char*) (*entry)->m_Title);                    
                    pItem->m_musicInfoTag.SetLoaded();

                    // look for content type in protocol info
                    if ((*entry)->m_Resources[0].m_ProtocolInfo.GetLength()) {
                        char proto[1024];
                        char dummy1[1024];
                        char ct[1204];
                        char dummy2[1024];
                        int fields = sscanf((*entry)->m_Resources[0].m_ProtocolInfo, "%[^:]:%[^:]:%[^:]:%[^:]", proto, dummy1, ct, dummy2);
                        if (fields == 4) {
                            pItem->SetContentType(ct);
                        }
                    }

                    //TODO, current thumbnail and icon of CFileItem is expected to be a local
                    //      image file in the normal thumbnail directories, we have no way to
                    //      specify an external filename on the filelayer
                    //if((*entry)->m_ExtraInfo.album_art_uri.GetLength())
                    //  pItem->SetThumbnailImage((const char*) (*entry)->m_ExtraInfo.album_art_uri);
                    //else
                    //  pItem->SetThumbnailImage((const char*) (*entry)->m_Description.icon_uri);

                    // look for date?
                    if((*entry)->m_Description.date.GetLength()) {
                      SYSTEMTIME time = {};
                      int count = sscanf((*entry)->m_Description.date, "%hu-%hu-%huT%hu:%hu:%hu",
                                          &time.wYear, &time.wMonth, &time.wDay, &time.wHour, &time.wMinute, &time.wSecond);
                      pItem->m_dateTime = time;
                    }
                    
                }
            }

            pItem->SetLabelPreformated(true);
            vecCacheItems.Add(pItem);
            items.Add(new CFileItem(*pItem));

            ++entry;
        }
    }

    return true;
}
}