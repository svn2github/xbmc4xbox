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

#pragma once

#include "idirectory.h"

/* forward declarations */
class PLT_UPnP;
class PLT_SyncMediaBrowser;

using namespace DIRECTORY;

namespace DIRECTORY
{
class CUPnPDirectory;

class CUPnP
{
public:
    CUPnP();
    ~CUPnP();

    // methods
    static CUPnP* GetInstance();
    static void   ReleaseInstance();
    static bool   IsInstantiated() { return upnp != NULL; }

private:
    friend class CUPnPDirectory;

    PLT_UPnP*              m_UPnP;
    PLT_SyncMediaBrowser*  m_MediaBrowser;

    static CUPnP* upnp;
};

class CUPnPDirectory :  public IDirectory
{
public:
    CUPnPDirectory(void) {}
    virtual ~CUPnPDirectory(void) {}

    virtual bool GetDirectory(const CStdString& strPath, CFileItemList &items);

    static const char* GetFriendlyName(const char* url);

private:
};
}
