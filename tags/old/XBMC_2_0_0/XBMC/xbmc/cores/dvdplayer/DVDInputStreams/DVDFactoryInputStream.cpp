
#include "../../../stdafx.h"
#include "DVDFactoryInputStream.h"
#include "DVDInputStream.h"
#include "DVDInputStreamFile.h"
#include "DVDInputStreamNavigator.h"
#include "DVDInputStreamHttp.h"

CDVDInputStream* CDVDFactoryInputStream::CreateInputStream(IDVDPlayer* pPlayer, const std::string& file, const std::string& content)
{
  CFileItem item(file.c_str(), false);
  if (item.IsDVDFile(false, true) || item.IsDVDImage() ||
      file.compare("\\Device\\Cdrom0") == 0)
  {
    return (new CDVDInputStreamNavigator(pPlayer));
  }  
  //else if (item.IsShoutCast())
  //  /* this should be replaced with standard file as soon as ffmpeg can handle raw aac */
  //  /* currently ffmpeg isn't able to detect that */
  //  return (new CDVDInputStreamHttp());
  //else if (item.IsInternetStream() )  
  //  return (new CDVDInputStreamHttp());
  
  // our file interface handles all these types of streams
  return (new CDVDInputStreamFile());
}
