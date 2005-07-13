
#include "../stdafx.h"
#include "iso9660directory.h"
#include "../xbox/iosupport.h"
#include "iso9660.h"
#include "../util.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

CISO9660Directory::CISO9660Directory(void)
{}

CISO9660Directory::~CISO9660Directory(void)
{}

bool CISO9660Directory::GetDirectory(const CStdString& strPath, CFileItemList &items)
{
  static char szTemp[1024];
  CStdString strRoot = strPath;
  if (!CUtil::HasSlashAtEnd(strPath) )
    strRoot += "/";

  // Scan active disc if not done before
  if (!m_isoReader.IsScanned())
    m_isoReader.Scan();

  CURL url(strPath);

  {
    WIN32_FIND_DATA wfd;
    HANDLE hFind;

    memset(&wfd, 0, sizeof(wfd));

    CStdString strSearchMask;
    CStdString strDirectory = url.GetFileName();
    if (strDirectory != "")
    {
      strSearchMask.Format("\\%s", strDirectory.c_str());
    }
    else
    {
      strSearchMask = "\\";
    }
    for (int i = 0; i < (int)strSearchMask.size(); ++i )
    {
      if (strSearchMask[i] == '/') strSearchMask[i] = '\\';
    }

    //      FILETIME localTime;
    hFind = m_isoReader.FindFirstFile((char*)strSearchMask.c_str(), &wfd);
    if (hFind != NULL)
    {
      do
      {
        if (wfd.cFileName[0] != 0)
        {
          if ( (wfd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) )
          {
            CStdString strDir = wfd.cFileName;
            if (strDir != "." && strDir != "..")
            {
              CFileItem *pItem = new CFileItem(wfd.cFileName);
              pItem->m_strPath = strRoot;
              pItem->m_strPath += wfd.cFileName;
              pItem->m_bIsFolder = true;
              //               FileTimeToLocalFileTime(&wfd.ftLastWriteTime,&localTime);
              //               FileTimeToSystemTime(&localTime, &pItem->m_stTime);

              items.Add(pItem);
            }
          }
          else
          {
            if ( IsAllowed( wfd.cFileName) )
            {
              CFileItem *pItem = new CFileItem(wfd.cFileName);
              pItem->m_strPath = strRoot;
              pItem->m_strPath += wfd.cFileName;
              pItem->m_bIsFolder = false;
              pItem->m_dwSize = CUtil::ToInt64(wfd.nFileSizeHigh, wfd.nFileSizeLow);
              //           FileTimeToLocalFileTime(&wfd.ftLastWriteTime,&localTime);
              //           FileTimeToSystemTime(&localTime, &pItem->m_stTime);
              items.Add(pItem);
            }
          }
        }
      }
      while (m_isoReader.FindNextFile(hFind, &wfd));
      m_isoReader.FindClose(hFind);
    }
  }

  return true;
}
