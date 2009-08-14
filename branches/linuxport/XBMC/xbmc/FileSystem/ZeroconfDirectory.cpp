/*
 *      Copyright (C) 2005-2009 Team XBMC
 *      http://www.xbmc.org
 *
 *  This Program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your option)
 *  any later version.
 *
 *  This Program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with XBMC; see the file COPYING.  If not, write to
 *  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 *  http://www.gnu.org/copyleft/gpl.html
 *
 */

#include "stdafx.h"
#include "ZeroconfDirectory.h"
#include "URL.h"
#include "Util.h"
#include "FileItem.h"
#include "ZeroconfBrowser.h"
#include "Directory.h"

using namespace XFILE;
using namespace DIRECTORY;

CZeroconfDirectory::CZeroconfDirectory()
{
  static bool initialized_browser = false;
  if(!initialized_browser)
  {
    CZeroconfBrowser* browser = CZeroconfBrowser::GetInstance();
    browser->AddServiceType("_smb._tcp.");
    browser->AddServiceType("_ftp._tcp.");
    browser->AddServiceType("_htsp._tcp.");
    browser->Start();
    initialized_browser = true;
  }
}

CZeroconfDirectory::~CZeroconfDirectory()
{
}

namespace
{
  CStdString GetHumanReadableProtocol(std::string const& fcr_service_type)
  {
    if(fcr_service_type == "_smb._tcp.")
      return "SAMBA";
    else if(fcr_service_type == "_ftp._tcp.")
      return "FTP";
    else if(fcr_service_type == "_htsp._tcp.")
      return "HTS";
    //fallback, just return the received type
    return fcr_service_type;
  }
  bool GetXBMCProtocol(std::string const& fcr_service_type, CStdString& fr_protocol)
  {
    if(fcr_service_type == "_smb._tcp.")
      fr_protocol = "smb";
    else if(fcr_service_type == "_ftp._tcp.")
      fr_protocol = "ftp";
    else if(fcr_service_type == "_htsp._tcp.")
      fr_protocol = "htsp";
    else
      return false;
    return true;
  }
}

bool CZeroconfDirectory::GetDirectory(const CStdString& strPath, CFileItemList &items)
{
  assert(strPath.substr(0, 11) == "zeroconf://");
  CStdString path = strPath.substr(11, strPath.length());
  CUtil::RemoveSlashAtEnd(path);
  if(path.empty())
  {
    std::vector<CZeroconfBrowser::ZeroconfService> found_services = CZeroconfBrowser::GetInstance()->GetFoundServices();
    for(std::vector<CZeroconfBrowser::ZeroconfService>::iterator it = found_services.begin(); it != found_services.end(); ++it)
    {
      CStdString path = CStdString("zeroconf://" + CZeroconfBrowser::ZeroconfService::toPath(*it));
      CFileItemPtr item(new CFileItem(path, true));
      CStdString protocol = GetHumanReadableProtocol(it->type);
      item->SetLabel(it->name + "(" + protocol  + ")");
      item->SetLabelPreformated(true);
      items.Add(item);
    }
    return true;
  } else
  {
    //decode the path first
    CStdString decoded = path;
    CUtil::UrlDecode(decoded);
    CZeroconfBrowser::ZeroconfService zeroconf_service = CZeroconfBrowser::ZeroconfService::fromPath(decoded);
    if(!CZeroconfBrowser::GetInstance()->ResolveService(zeroconf_service))
      return false;
    else
    {
      assert(!zeroconf_service.ip.empty());
      CURL service;
      service.SetPort(zeroconf_service.port);
      service.SetHostName(zeroconf_service.ip);
      //do protocol conversion (_smb._tcp -> smb)
      //ToDo: try automatic conversion -> remove leading '_' and '._tcp'?
      CStdString protocol;
      if(!GetXBMCProtocol(zeroconf_service.type, protocol))
      {
        CLog::Log(LOGERROR, "CZeroconfDirectory::GetDirectory Unknown service type: %s.", service.GetProtocol().c_str());
        return false;
      }
      service.SetProtocol(protocol);
      CStdString resolved_path;
      service.GetURL(resolved_path);
      return CDirectory::GetDirectory(resolved_path, items);
    }
  }
}
