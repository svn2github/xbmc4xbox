#include "stdafx.h"
#include "VTPDirectory.h"
#include "VTPSession.h"
#include "VideoInfoTag.h"
#include "URL.h"
#include "Util.h"
#include "FileItem.h"


using namespace std;
using namespace DIRECTORY;

CVTPDirectory::CVTPDirectory()
{
  m_session = new CVTPSession();
}

CVTPDirectory::~CVTPDirectory()
{
  if(m_session)
    delete m_session;
}

bool CVTPDirectory::GetChannels(const CStdString& base, CFileItemList &items)
{
  vector<CVTPSession::Channel> channels;
  if(!m_session->GetChannels(channels))
    return false;

  vector<CVTPSession::Channel>::iterator it;
  for(it = channels.begin(); it != channels.end(); it++)
  {
    CStdString buffer;
    CFileItemPtr item(new CFileItem("", false));

    item->m_strPath.Format("%s/%d.ts", base.c_str(), it->index);
    item->m_strTitle = it->name;
    buffer.Format("%d - %s", it->index, it->name);
    item->SetLabel(buffer);

    items.Add(item);
  }
  return true;
}

bool CVTPDirectory::GetDirectory(const CStdString& strPath, CFileItemList &items)
{
  CURL url(strPath);

  if(url.GetHostName() == "")
    url.SetHostName("localhost");

  if(url.GetPort() == 0)
    url.SetPort(2004);

  if(!m_session->Open(url.GetHostName(), url.GetPort()))
    return false;

  CStdString base;
  url.GetURL(base);
  CUtil::RemoveSlashAtEnd(base);

  if(url.GetFileName().IsEmpty())
  {
    CFileItemPtr item;

    item.reset(new CFileItem(base + "/channels/", true));
    item->SetLabel("Live Channels");
    item->SetLabelPreformated(true);
    items.Add(item);
    return true;
  }
  else if(url.GetFileName() == "channels/")
    return GetChannels(base, items);
  else
    return false;
}

