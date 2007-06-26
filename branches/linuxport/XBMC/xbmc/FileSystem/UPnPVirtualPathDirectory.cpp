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


#include "stdafx.h"
#include "../Util.h"
#include "NptUtils.h"
#include "UPnPVirtualPathDirectory.h"

/*----------------------------------------------------------------------
|   CUPnPVirtualPathDirectory::FindSharePath
+---------------------------------------------------------------------*/
bool 
CUPnPVirtualPathDirectory::FindSharePath(const char* share_name, const char* path, bool begin /* = false */) 
{
    // look for all the paths given a share name
    CShare share;
    vector<CStdString> paths;
    CUPnPVirtualPathDirectory dir;
    if (!dir.GetMatchingShare((const char*)share_name, share, paths)) 
        return false;

    for (unsigned int i = 0; i < paths.size(); i++) {
        if (begin) {
            if (NPT_StringsEqualN(path, paths[i].c_str(), NPT_StringLength(paths[i].c_str()))) {
                if (NPT_StringLength(path) <= NPT_StringLength(paths[i].c_str()) || path[NPT_StringLength(paths[i].c_str())] == '\\')
                    return true;
            }
        } else if (NPT_StringsEqual(path, paths[i].c_str())) {
            return true;
        }
    }

    return false;
}

/*----------------------------------------------------------------------
|   CUPnPVirtualPathDirectory::SplitPath
+---------------------------------------------------------------------*/
bool 
CUPnPVirtualPathDirectory::SplitPath(const char* object_id, NPT_String& share_name, NPT_String& path) 
{
    int index = 0;
    NPT_String id = object_id;

    // reset output params first
    share_name = "";
    path = "";

    if (id.StartsWith("virtualpath://upnpmusic")) {
        index = 23;
    } else if (id.StartsWith("virtualpath://upnpvideo")) {
        index = 23;
    } else if (id.StartsWith("virtualpath://upnppictures")) {
        index = 26;
    } else {
        return false;
    }

    // nothing to split
    if (id.GetLength() <= (NPT_Cardinal)index) {
        return true;
    }

    // invalid id!
    if (id[index] != '/') {
        return false;
    }

    // look for share
    index = id.Find('/', index+1);
    share_name = id.SubString(0, (index==-1)?id.GetLength():index);

    if (index >= 0) {
        path = id.SubString(index+1);
    }

    return true;
}

/*----------------------------------------------------------------------
|   CUPnPVirtualPathDirectory::GetDirectory
+---------------------------------------------------------------------*/
bool
CUPnPVirtualPathDirectory::GetDirectory(const CStdString& strPath, CFileItemList &items) 
{
    NPT_String path = strPath.c_str();
    CShare     share;
    CFileItem* item;
    vector<CStdString> paths;

    if (path == "0") {
        // music
        item = new CFileItem("virtualpath://upnpmusic", true);
        item->SetLabel("Music");
        items.Add(item);

        // video
        item = new CFileItem("virtualpath://upnpvideo", true);
        item->SetLabel("Video");
        items.Add(item);

        // pictures
        item = new CFileItem("virtualpath://upnppictures", true);
        item->SetLabel("Pictures");
        items.Add(item);

        return true;
    } else if (path == "virtualpath://upnpmusic" || 
               path == "virtualpath://upnpvideo" || 
               path == "virtualpath://upnppictures") {
        // look for all shares given a container
        VECSHARES *shares = NULL;
        if (path == "virtualpath://upnpmusic") {
            shares = g_settings.GetSharesFromType("upnpmusic");
        } else if (path == "virtualpath://upnpvideo") {
            shares = g_settings.GetSharesFromType("upnpvideo");
        } else if (path == "virtualpath://upnppictures") {
            shares = g_settings.GetSharesFromType("upnppictures");
        }
        if (shares) {
            for (unsigned int i = 0; i < shares->size(); i++) {
                // Does this share contains any local paths?
                CShare &share = shares->at(i);
                // reconstruct share name as it could have been replaced by
                // a path if there was just one entry
                NPT_String share_name = path + "/";
                share_name += share.strName;
                if (GetMatchingShare((const char*)share_name, share, paths) && paths.size()) {
                    item = new CFileItem((const char*)share_name, true);
                    item->SetLabel(share.strName);
                    items.Add(item);
                }
            }
        }
        
        return true;
    } else if (!GetMatchingShare((const char*)path, share, paths)) {
        // split to remove share name from path
        NPT_String share_name;
        NPT_String file_path;
        bool bret = SplitPath(path, share_name, file_path);
        if (!bret || share_name.GetLength() == 0 || file_path.GetLength() == 0) {
            return false;
        }

        // make sure the file_path is the beginning of a share paths
        if (!FindSharePath(share_name, file_path, true)) return false;

        // use the share name to figure out what extensions to use
        if (share_name.StartsWith("virtualpath://upnpmusic")) {
            return CDirectory::GetDirectory(
                (const char*)file_path, 
                items, 
                g_stSettings.m_musicExtensions);
        } else if (share_name.StartsWith("virtualpath://upnpvideo")) {
            return CDirectory::GetDirectory(
                (const char*)file_path, 
                items, 
                g_stSettings.m_videoExtensions);
        } else if (share_name.StartsWith("virtualpath://upnppictures")) {
            return CDirectory::GetDirectory(
                (const char*)file_path, 
                items, 
                g_stSettings.m_pictureExtensions);
        }

        // weird!
        return false;
    }

    // use the path to figure out what extensions to use
    if (path.StartsWith("virtualpath://upnpmusic")) {
        SetMask(g_stSettings.m_musicExtensions);
    } else if (path.StartsWith("virtualpath://upnpvideo")) {
        SetMask(g_stSettings.m_videoExtensions);
    } else if (path.StartsWith("virtualpath://upnppictures")) {
        SetMask(g_stSettings.m_pictureExtensions);
    }

    // dont allow prompting, this is a background task
    // although I don't think it matters here
    SetAllowPrompting(false);

    // it's a virtual path, get all items
    CVirtualPathDirectory::GetDirectory(strPath, items);

    // always return true
    // this will make sure that even if nothing is found in a share
    // it doesn't fail upnp
    return true;
}

/*----------------------------------------------------------------------
|   CUPnPVirtualPathDirectory::GetMatchingShare
+---------------------------------------------------------------------*/
bool 
CUPnPVirtualPathDirectory::GetMatchingShare(const CStdString &strPath, CShare& share, vector<CStdString>& paths) 
{
    paths.clear();

    CStdString strType, strBookmark;
    if (!GetTypeAndBookmark(strPath, strType, strBookmark))
        return false;

    VECSHARES *vecShares = g_settings.GetSharesFromType(strType);
    if (!vecShares) return false;

    // look for share
    int i;
    for (i = 0; i < (int)vecShares->size(); ++i) {
        CShare share = vecShares->at(i);
        CStdString strName = share.strName;
        if (strBookmark.Equals(strName))
            break;
    }

    if (i == vecShares->size()) return false;

    share = (*vecShares)[i];

    // filter out non local shares
    for (unsigned int j = 0; j < share.vecPaths.size(); j++) {
        if (!CUtil::IsRemote(share.vecPaths[j])) {
            paths.push_back(share.vecPaths[j]);
        }
    }
    return true;
}

