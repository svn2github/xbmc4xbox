/*
 *      Copyright (C) 2005-2010 Team XBMC
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

#include "FileOperations.h"
#include "Settings.h"
#include "MediaSource.h"
#include "../FileSystem/Directory.h"
#include "FileItem.h"
#include "AdvancedSettings.h"
#include "../Util.h"

using namespace XFILE;
using namespace Json;
using namespace JSONRPC;

JSON_STATUS CFileOperations::GetRootDirectory(const CStdString &method, ITransportLayer *transport, IClient *client, const Value &parameterObject, Value &result)
{
  const Value param = parameterObject.isObject() ? parameterObject : Value(objectValue);
  CStdString type = param.get("type", "null").asString();
  type = type.ToLower();

  if (type.Equals("video") || type.Equals("music") || type.Equals("pictures") || type.Equals("files") || type.Equals("programs"))
  {
    VECSOURCES *sources = g_settings.GetSourcesFromType(type);
    if (sources)
    {
      CFileItemList items;
      for (unsigned int i = 0; i < (unsigned int)sources->size(); i++)
        items.Add(CFileItemPtr(new CFileItem(sources->at(i))));

      HandleFileItemList(NULL, "shares", items, parameterObject, result);
    }

    return OK;
  }
  else
    return InvalidParams;
}

JSON_STATUS CFileOperations::GetDirectory(const CStdString &method, ITransportLayer *transport, IClient *client, const Value &parameterObject, Value &result)
{
  if (parameterObject.isObject() && parameterObject.isMember("directory"))
  {
    CStdString type = "files";
    if (parameterObject.isMember("type"))
    {
      if (parameterObject["type"].isString())
        type = parameterObject["type"].asString();
      else
        return InvalidParams;
    }

    type = type.ToLower();

    if (type.Equals("video") || type.Equals("music") || type.Equals("pictures") || type.Equals("files") || type.Equals("programs"))
    {
      CDirectory directory;
      CFileItemList items;
      CStdString strPath = parameterObject["directory"].asString();

      if (directory.GetDirectory(strPath, items))
      {
        CStdStringArray regexps;

        if (type.Equals("video"))
          regexps = g_advancedSettings.m_videoExcludeFromListingRegExps;
        else if (type.Equals("music"))
          regexps = g_advancedSettings.m_audioExcludeFromListingRegExps;
        else if (type.Equals("pictures"))
          regexps = g_advancedSettings.m_pictureExcludeFromListingRegExps;

        CFileItemList filteredDirectories, filteredFiles;
        for (unsigned int i = 0; i < (unsigned int)items.Size(); i++)
        {
          if (regexps.size() == 0 || !CUtil::ExcludeFileOrFolder(items[i]->m_strPath, regexps))
          {
            if (items[i]->m_bIsFolder)
              filteredDirectories.Add(items[i]);
            else
              filteredFiles.Add(items[i]);
          }
        }

        HandleFileItemList(NULL, "directories", filteredDirectories, parameterObject, result);
        HandleFileItemList(NULL, "files", filteredFiles, parameterObject, result);

        return OK;
      }
    }
  }

  return InvalidParams;
}

JSON_STATUS CFileOperations::Download(const CStdString &method, ITransportLayer *transport, IClient *client, const Value &parameterObject, Value &result)
{
  if (!parameterObject.isString())
    return InvalidParams;

  return transport->Download(parameterObject.asString().c_str(), &result) ? OK : BadPermission;
}
