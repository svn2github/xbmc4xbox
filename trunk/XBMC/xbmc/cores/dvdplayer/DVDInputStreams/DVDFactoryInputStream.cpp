
#include "stdafx.h"
#include "DVDFactoryInputStream.h"
#include "DVDInputStream.h"
#include "DVDInputStreamFile.h"
#include "DVDInputStreamNavigator.h"
#include "..\..\..\fileitem.h"

CDVDInputStream* CDVDFactoryInputStream::CreateInputStream(IDVDPlayer* pPlayer, const char* strFile)
{
  CFileItem item(strFile, false);
  if (item.IsDVDFile(false, true) || item.IsDVDImage() ||
      strncmp(strFile, "\\Device\\Cdrom0", 14) == 0)
  {
    return (new CDVDInputStreamNavigator(pPlayer));
  }
  
  return (new CDVDInputStreamFile());
}