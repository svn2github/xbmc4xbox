
#include "stdafx.h"
#include "application.h"
#include "util.h"
#include "xbox/iosupport.h"
#include "xbox/xbeheader.h"
#include "crc32.h"
#include "settings.h"
#include "utils/log.h"
#include "xbox/undocumented.h"
#include "url.h"
#include "xbresource.h"
#include "graphiccontext.h"
#include "sectionloader.h"
#include "lib/cximage/ximage.h"
#include "filesystem/file.h"
#include "DetectDVDType.h"
#include "autoptrhandle.h"
#include "ThumbnailCache.h"
#include "filesystem/hddirectory.h"
#include "filesystem/DirectoryCache.h"
#include "Credits.h"
#include "utils/CharsetConverter.h"
#include "cores/mplayer/xbox_video.h"
#include "shortcut.h"

bool CUtil::m_bNetworkUp = false;

struct SSortByLabel
{
	bool operator()(CFileItem* pStart, CFileItem* pEnd)
	{
    CGUIListItem& rpStart=*pStart;
    CGUIListItem& rpEnd=*pEnd;

		CStdString strLabel1=rpStart.GetLabel();
		strLabel1.ToLower();

		CStdString strLabel2=rpEnd.GetLabel();
		strLabel2.ToLower();

		if (m_bSortAscending)
			return (strcmp(strLabel1.c_str(),strLabel2.c_str())<0);
		else
			return (strcmp(strLabel1.c_str(),strLabel2.c_str())>=0);
	}

	bool m_bSortAscending;
};

using namespace AUTOPTR;
using namespace MEDIA_DETECT;
using namespace XFILE;
using namespace PLAYLIST;
char g_szTitleIP[32];
static D3DGAMMARAMP oldramp, flashramp;

CUtil::CUtil(void)
{
  memset(g_szTitleIP,0,sizeof(g_szTitleIP));
}

CUtil::~CUtil(void)
{
}

char* CUtil::GetExtension(const CStdString& strFileName)
{
  char* extension = strrchr(strFileName.c_str(),'.');
  return extension ;
}

int CUtil::cmpnocase(const char* str1,const char* str2)
{
  int iLen;
  if ( strlen(str1) != strlen(str2) ) return 1;

  iLen=strlen(str1);
  for (int i=0; i < iLen;i++ )
  {
    if (tolower((unsigned char)str1[i]) != tolower((unsigned char)str2[i]) ) return 1;
  }
  return 0;
}


char* CUtil::GetFileName(const CStdString& strFileNameAndPath)
{

  char* extension = strrchr(strFileNameAndPath.c_str(),'\\');
  if (!extension)
  {
    extension = strrchr(strFileNameAndPath.c_str(),'/');
    if (!extension) return (char*)strFileNameAndPath.c_str();
  }

  extension++;
  return extension;

}

CStdString CUtil::GetTitleFromPath(const CStdString& strFileNameAndPath)
{
	// use above to get the filename
	CStdString strFilename = GetFileName(strFileNameAndPath);
	// now remove the extension if needed
	if (IsSmb(strFileNameAndPath)) {
		CStdString strTempFilename;
		g_charsetConverter.utf8ToStringCharset(strFilename,strTempFilename);
		strFilename = strTempFilename;
	}
	if (g_guiSettings.GetBool("FileLists.HideExtensions"))
	{
		RemoveExtension(strFilename);
		return strFilename;
	}
	return strFilename;
}

bool CUtil::GetVolumeFromFileName(const CStdString& strFileName, CStdString& strFileTitle, CStdString& strVolumePrefix, int& volumeNumber)
{
  bool result = false;
  CStdString strFileNameTemp = strFileName;

  // remove any extension
  int pos = strFileNameTemp.ReverseFind('.');
  if (pos > 0)
    strFileNameTemp = strFileNameTemp.Left(pos);

  const CStdString & separatorsString = g_settings.m_szMyVideoStackSeparatorsString;
  const CStdStringArray & tokens = g_settings.m_szMyVideoStackTokensArray;

  CStdString strFileNameTempLower = strFileNameTemp;
  strFileNameTempLower.MakeLower();

  for (int i = 0; i < (int)tokens.size(); i++)
  {
    const CStdString & token = tokens[i];
    pos = strFileNameTempLower.ReverseFind(token);
    //CLog::Log(LOGERROR, "GetVolumeFromFileName : " + strFileNameTemp + " : " + token + " : 1");
    if ((pos > 0) && (pos < (int)strFileNameTemp.size()))
    {
      //CLog::Log(LOGERROR, "GetVolumeFromFileName : " + strFileNameTemp + " : " + token + " : 2");
      // token found at end
      if (separatorsString.Find(strFileNameTemp.GetAt(pos - 1)) > -1)
      {
        // appropriate separator found between file title and token

        CStdString volumeNumberString = strFileNameTemp.Mid(pos + token.size()).Trim();
        //CLog::Log(LOGERROR, "GetVolumeFromFileName : " + strFileNameTemp + " : " + token + " : " + volumeNumberString + " : 3");
        volumeNumber = atoi(volumeNumberString.c_str());
        // the parsed result must be > 0
        if (volumeNumber > 0)
        {
          //CLog::Log(LOGERROR, "GetVolumeFromFileName : " + strFileNameTemp + " : " + token + " : " + volumeNumberString + " : 4");
          bool volumeNumberValid = true;
          // make sure only numbers follow the token
          for (int j = 1; j < (int)volumeNumberString.size(); j++)
          {
            char c = volumeNumberString.GetAt(j);
            if ((c < '0') || (c > '9'))
            {
              volumeNumberValid = false;
              break;
            }
          }

          if (volumeNumberValid)
          {
            //CLog::Log(LOGERROR, "GetVolumeFromFileName : " + strFileNameTemp + " : " + token + " : " + volumeNumberString + " : 5");

            result = true;
            strFileTitle = strFileNameTemp.Left(pos-1);
            // trim all additional separator characters
            strFileTitle.TrimRight(separatorsString.c_str());
            strVolumePrefix = token;
            break;
          }
        }
      }
    }
  }

  return result;
}

void CUtil::RemoveExtension(CFileItem *pItem)
{
	if (pItem->m_bIsFolder)
		return;
	CStdString strLabel = pItem->GetLabel();
	RemoveExtension(strLabel);
	pItem->SetLabel(strLabel);
}

void CUtil::RemoveExtension(CStdString& strFileName)
{
	int iPos = strFileName.ReverseFind(".");
	//	Extension found
	if (iPos>0)
	{
		CStdString strExtension;
		CUtil::GetExtension(strFileName,strExtension);
		CUtil::Lower(strExtension);

		CStdString strFileMask;
		strFileMask=g_stSettings.m_szMyMusicExtensions;
		strFileMask+=g_stSettings.m_szMyPicturesExtensions;
		strFileMask+=g_stSettings.m_szMyVideoExtensions;

		//	Only remove if its a valid media extension
		if (strFileMask.Find(strExtension.c_str())>=0)
			strFileName=strFileName.Left(iPos);
	}
}

// Remove the extensions from the filenames
void CUtil::RemoveExtensions(VECFILEITEMS &items)
{
	for (int i=0; i < (int)items.size(); ++i)
		RemoveExtension(items[i]);
}

void CUtil::CleanFileName(CFileItem *pItem)
{
	if (pItem->m_bIsFolder)
		return;
	CStdString strLabel = pItem->GetLabel();
	CleanFileName(strLabel);
	pItem->SetLabel(strLabel);
}

void CUtil::CleanFileName(CStdString& strFileName)
{
  bool result = false;

  // assume extension has already been removed

  // remove volume indicator

  {
    const CStdString & separatorsString = g_settings.m_szMyVideoStackSeparatorsString;
    const CStdStringArray & tokens = g_settings.m_szMyVideoStackTokensArray;

    CStdString strFileNameTempLower = strFileName;
    strFileNameTempLower.MakeLower();

    for (int i = 0; i < (int)tokens.size(); i++)
    {
      const CStdString & token = tokens[i];
      int pos = strFileNameTempLower.ReverseFind(token);
      if ((pos > 0) && (pos < (int)strFileName.size()))
      {
        // token found at end
        if (separatorsString.Find(strFileName.GetAt(pos - 1)) > -1)
        {
          // appropriate separator found between file title and token

          CStdString volumeNumberString = strFileName.Mid(pos + token.size()).Trim();
          int volumeNumber = atoi(volumeNumberString.c_str());
          // the parsed result must be > 0
          if (volumeNumber > 0)
          {
            bool volumeNumberValid = true;
            // make sure only numbers follow the token
            for (int j = 1; j < (int)volumeNumberString.size(); j++)
            {
              char c = volumeNumberString.GetAt(j);
              if ((c < '0') || (c > '9'))
              {
                volumeNumberValid = false;
                break;
              }
            }

            if (volumeNumberValid)
            {
              strFileName = strFileName.Left(pos-1);
              // trim all additional separator characters
              strFileName.TrimRight(separatorsString.c_str());
              break;
            }
          }
        }
      }
    }
  }


  // remove known tokens:      { "divx", "xvid", "3ivx", "ac3", "ac351", "mp3", "wma", "m4a", "mp4", "ogg", "SCR", "TS", "sharereactor" }
  // including any separators: { ' ', '-', '_', '.', '[', ']', '(', ')' }
  // logic is as follows:
  //   - multiple tokens can be listed, separated by any combination of separators
  //   - first token must follow a '-' token, potentially in addition to other separator tokens
  //   - thus, something like "video_XviD_AC3" will not be parsed, but something like "video_-_XviD_AC3" will be parsed

  // special logic - if a known token is found, try to group it with any 
  // other tokens up to the dash separating the token group from the title. 
  // thus, something like "The Godfather-DivX_503_2p_VBR-HQ_480x640_16x9" should 
  // be fully cleaned up to "The Godfather"
  // the problem with this logic is that it may clean out things we still 
  // want to see, such as language codes.

  {
    //const CStdString separatorsString = " -_.[]()+";

    //const CStdString tokensString = "divx|xvid|3ivx|ac3|ac351|mp3|wma|m4a|mp4|ogg|scr|ts|sharereactor";
    //CStdStringArray tokens;
    //StringUtils::SplitString(tokensString, "|", tokens);

    const CStdString & separatorsString = g_settings.m_szMyVideoCleanSeparatorsString;
    const CStdStringArray & tokens = g_settings.m_szMyVideoCleanTokensArray;

    CStdString strFileNameTempLower = strFileName;
    strFileNameTempLower.MakeLower();

    int maxPos = 0;
    bool tokenFoundWithSeparator = false;

    while ((maxPos < (int)strFileName.size()) && (!tokenFoundWithSeparator))
    {
      bool tokenFound = false;

      for (int i = 0; i < (int)tokens.size(); i++)
      {
        CStdString token = tokens[i];
        int pos = strFileNameTempLower.Find(token, maxPos);
        if (pos >= maxPos && pos>0)
        {
          tokenFound = tokenFound | true;
          char separator = strFileName.GetAt(pos - 1);
          char buffer[10];
          itoa(pos, buffer, 10);
          char buffer2[10];
          itoa(maxPos, buffer2, 10);
          //CLog::Log(LOGERROR, "CleanFileName : 1 : " + strFileName + " : " + token + " : " + buffer + " : " + separator + " : " + buffer2 + " : " + separatorsString);
          if (separatorsString.Find(separator) > -1)
          {
            // token has some separator before it - now look for the 
            // specific '-' separator, and trim any additional separators.

            int pos2 = pos;
            while (pos2 > 0)
            {
              separator = strFileName.GetAt(pos2 - 1);
              if (separator == '-')
                tokenFoundWithSeparator = true;
              else if (separatorsString.Find(separator) == -1)
                break;
              pos2--;
            }
            if (tokenFoundWithSeparator)
              pos = pos2;
            //if (tokenFoundWithSeparator)
            //  CLog::Log(LOGERROR, "CleanFileName : 2 : " + strFileName + " : " + token + " : " + buffer + " : " + separator + " : " + buffer2);
            //else
            //  CLog::Log(LOGERROR, "CleanFileName : 3 : " + strFileName + " : " + token + " : " + buffer + " : " + separator + " : " + buffer2);
          }

          if (tokenFoundWithSeparator)
          {
            if (pos > 0)
              strFileName = strFileName.Left(pos);
            break;
          }
          else
          {
            maxPos = max(maxPos, pos + 1);
          }
        }
      }

      if (!tokenFound)
        break;

      maxPos++;
    }
    
  }
  

  // TODO: would be nice if we could remove years (i.e. "(1999)") from the 
  // title, and put the year in a separate column instead

  // TODO: would also be nice if we could do something with 
  // languages (i.e. "[ITA]") - need to consider files with 
  // multiple audio tracks


  // final cleanup - special characters used instead of spaces:
  // all '_' tokens should be replaced by spaces
  // if the file contains no spaces, all '.' tokens should be replaced by 
  // spaces - one possibility of a mistake here could be something like:
  // "Dr..StrangeLove" - hopefully no one would have anything like this.

  strFileName = strFileName.Trim();

  {
    bool alreadyContainsSpace = (strFileName.Find(' ') >= 0);

    for (int i = 0; i < (int)strFileName.size(); i++)
    {
      char c = strFileName.GetAt(i);
      if ((c == '_') || ((!alreadyContainsSpace) && (c == '.')))
      {
        strFileName.SetAt(i, ' ');
      }
    }
  }

  strFileName = strFileName.Trim();
}

void CUtil::CleanFileNames(VECFILEITEMS &items)
{
	for (int i=0; i < (int)items.size(); ++i)
		CleanFileName(items[i]);
}

bool CUtil::GetParentPath(const CStdString& strPath, CStdString& strParent)
{
  strParent="";

  CURL url(strPath);
  CStdString strFile=url.GetFileName();
  if (strFile.size()==0)
  {
    if (url.GetProtocol() == "smb" && (url.GetHostName().size() > 0))
    {
      // we have an smb share with only server or workgroup name
      // set hostname to "" and return true.
      url.SetHostName("");
      url.GetURL(strParent);
      return true;
    }
    else if (url.GetProtocol() == "xbms" && (url.GetHostName().size() > 0))
    {
      // we have an xbms share with only server name
      // set hostname to "" and return true.
      url.SetHostName("");
      url.GetURL(strParent);
      return true;
    }
    return false;
  }

  if (HasSlashAtEnd(strFile) )
  {
    strFile=strFile.Left(strFile.size()-1);
  }

  int iPos=strFile.ReverseFind('/');
  if (iPos < 0)
  {
    iPos=strFile.ReverseFind('\\');
  }
  if (iPos < 0)
  {
    url.SetFileName("");
    url.GetURL(strParent);
    return true;
  }

  strFile=strFile.Left(iPos);
  url.SetFileName(strFile);
  url.GetURL(strParent);
  return true;
}

//Make sure you have a full path in the filename, otherwise adds the base path before.
void CUtil::GetQualifiedFilename(const CStdString &strBasePath, CStdString &strFilename)
{
  CURL plItemUrl(strFilename);
  CURL plBaseUrl(strBasePath);
  int iDotDotLoc, iBeginCut, iEndCut;

  if(plBaseUrl.GetProtocol().length()==0) //Base in local directory
  {
    if(plItemUrl.GetProtocol().length()==0 ) //Filename is local or not qualified
    {
      if (!( strFilename.c_str()[1] == ':')) //Filename not fully qualified
      {
        if (strFilename.c_str()[0] == '/' || strFilename.c_str()[0] == '\\')
        {
          strFilename = strBasePath + strFilename;
          strFilename.Replace('/','\\');
        }
        else
        {
          strFilename = strBasePath + '\\' + strFilename;
          strFilename.Replace('/','\\');
        }
      }
    }
    strFilename.Replace("\\.\\","\\");
    while((iDotDotLoc = strFilename.Find("\\..\\")) > 0)
    {
        iEndCut = iDotDotLoc + 4;
        iBeginCut = strFilename.Left(iDotDotLoc).ReverseFind('\\')+1;
        strFilename.Delete(iBeginCut, iEndCut - iBeginCut);
    }
  }
  else //Base is remote
  {
    if(plItemUrl.GetProtocol().length()==0 ) //Filename is local
    {
      if (strFilename.c_str()[0] == '/' || strFilename.c_str()[0] == '\\' ) //Begins with a slash.. not good.. but we try to make the best of it..
      {
        strFilename = strBasePath + strFilename;
        strFilename.Replace('\\','/');
      }
      else
      {
        strFilename = strBasePath + '/' + strFilename;
        strFilename.Replace('\\','/');
      }
    }
    strFilename.Replace("/./","/");
    while((iDotDotLoc = strFilename.Find("/../")) > 0)
    {
        iEndCut = iDotDotLoc + 4;
        iBeginCut = strFilename.Left(iDotDotLoc).ReverseFind('/')+1;
        strFilename.Delete(iBeginCut, iEndCut - iBeginCut);
    }
  }
}

/// \brief Runs an executable file
/// \param szPath1 Path of executeable to run
/// \param szParameters Any parameters to pass to the executeable being run
void CUtil::RunXBE(const char* szPath1, char* szParameters)
{
	g_application.PrintXBEToLCD(szPath1);	//write to LCD
	Sleep(600);								//and wait a little bit to execute

	char szDevicePath[1024];
	char szPath[1024];
	char szXbePath[1024];
	strcpy(szPath,szPath1);
	char* szBackslash = strrchr(szPath,'\\');
	*szBackslash=0x00;
	char* szXbe = &szBackslash[1];

	char* szColon = strrchr(szPath,':');
	*szColon=0x00;
	char* szDrive = szPath;
	char* szDirectory = &szColon[1];

	CIoSupport helper;
	helper.GetPartition( (LPCSTR) szDrive, szDevicePath);

	strcat(szDevicePath,szDirectory);
	wsprintf(szXbePath,"d:\\%s",szXbe);

	g_application.Stop();

	CUtil::LaunchXbe(szDevicePath,szXbePath,szParameters);
}

//*********************************************************************************************
void CUtil::LaunchXbe(char* szPath, char* szXbe, char* szParameters)
{
  CLog::Log(LOGINFO, "launch xbe:%s %s", szPath,szXbe);
  CLog::Log(LOGINFO, " mount %s as D:", szPath);

  CIoSupport helper;
  helper.Unmount("D:");
  helper.Mount("D:",szPath);

  // detach if connected to kai
  CKaiClient::GetInstance()->RemoveObserver();

  CLog::Log(LOGINFO, "launch xbe:%s", szXbe);

  if (szParameters==NULL)
  {
    XLaunchNewImage(szXbe, NULL );
  }
  else
  {
    LAUNCH_DATA LaunchData;
    strcpy((char*)LaunchData.Data,szParameters);

    XLaunchNewImage(szXbe, &LaunchData );
  }
}

bool CUtil::FileExists(const CStdString& strFileName)
{
  if (strFileName.size()==0) return false;
  CFile file;
  if (!file.Exists(strFileName))
    return false;

  return true;
}

void CUtil::GetThumbnail(const CStdString& strFileName, CStdString& strThumb)
{
  strThumb="";
	CFileItem item(strFileName, false);

  CStdString strFile;
  CUtil::ReplaceExtension(strFileName,".tbn",strFile);
	if (CUtil::FileExists(strFile))
	{
    strThumb=strFile;
    return;
  }

  if (item.IsXBE())
  {
    if (CUtil::GetXBEIcon(strFileName,strThumb) ) return ;
    strThumb="defaultProgamIcon.png";
    return;
  }

  if (item.IsShortCut() )
  {
    CShortcut shortcut;
    if ( shortcut.Create( strFileName ) )
    {
      CStdString strFile=shortcut.m_strPath;

      GetThumbnail(strFile,strThumb);
      return;
    }
  }

  Crc32 crc;
  crc.ComputeFromLowerCase(strFileName);

  strThumb.Format("%s\\%x.tbn",g_stSettings.szThumbnailsDirectory,crc);
}

void CUtil::GetFileSize(__int64 dwFileSize, CStdString& strFileSize)
{
  char szTemp[128];
  // file < 1 kbyte?
  if (dwFileSize < 1024)
  {
    //  substract the integer part of the float value
    float fRemainder=(((float)dwFileSize)/1024.0f)-floor(((float)dwFileSize)/1024.0f);
    float fToAdd=0.0f;
    if (fRemainder < 0.01f)
      fToAdd=0.1f;
    sprintf(szTemp,"%2.1f KB", (((float)dwFileSize)/1024.0f)+fToAdd);
    strFileSize=szTemp;
    return;
  }
  __int64 iOneMeg=1024*1024;

  // file < 1 megabyte?
  if (dwFileSize < iOneMeg)
  {
    sprintf(szTemp,"%02.1f KB", ((float)dwFileSize)/1024.0f);
    strFileSize=szTemp;
    return;
  }

  // file < 1 GByte?
  __int64 iOneGigabyte=iOneMeg;
  iOneGigabyte *= (__int64)1000;
  if (dwFileSize < iOneGigabyte)
  {
    sprintf(szTemp,"%02.1f MB", ((float)dwFileSize)/((float)iOneMeg));
    strFileSize=szTemp;
    return;
  }
  //file > 1 GByte
  int iGigs=0;
  while (dwFileSize >= iOneGigabyte)
  {
    dwFileSize -=iOneGigabyte;
    iGigs++;
  }
  float fMegs=((float)dwFileSize)/((float)iOneMeg);
  fMegs /=1000.0f;
  fMegs+=iGigs;
  sprintf(szTemp,"%02.1f GB", fMegs);
  strFileSize=szTemp;
  return;
}

void CUtil::GetDate(SYSTEMTIME stTime, CStdString& strDateTime)
{
  char szTmp[128];
  sprintf(szTmp,"%i-%i-%i %02.2i:%02.2i",
          stTime.wDay,stTime.wMonth,stTime.wYear,
          stTime.wHour,stTime.wMinute);
  strDateTime=szTmp;
}

void CUtil::GetHomePath(CStdString& strPath)
{
  char szXBEFileName[1024];
  CIoSupport helper;
  helper.GetXbePath(szXBEFileName);
  char *szFileName = strrchr(szXBEFileName,'\\');
  *szFileName=0;
  strPath=szXBEFileName;
}

bool CUtil::IsEthernetConnected()
{
  if (!(XNetGetEthernetLinkStatus() & XNET_ETHERNET_LINK_ACTIVE))
    return false;

  return true;
}

void CUtil::GetTitleIP(CStdString& ip)
{
  ip = g_szTitleIP;
}

bool CUtil::InitializeNetwork(int iAssignment, const char* szLocalAddress, const char* szLocalSubnet, const char* szLocalGateway, const char* szNameServer)
{
  if (!IsEthernetConnected())
  {
    CLog::Log(LOGWARNING, "network cable unplugged");
    return false;
  }

  struct network_info networkinfo ;
  memset(&networkinfo ,0,sizeof(networkinfo ));
  bool bSetup(false);
  if (iAssignment == NETWORK_DHCP)
  {
    bSetup=true;
    CLog::Log(LOGNOTICE, "use DHCP");
    networkinfo.DHCP=true;
  }
  else if (iAssignment == NETWORK_STATIC)
  {
    bSetup=true;
    CLog::Log(LOGNOTICE, "use static ip");
    networkinfo.DHCP=false;
    strcpy(networkinfo.ip,szLocalAddress);
    strcpy(networkinfo.subnet,szLocalSubnet);
    strcpy(networkinfo.gateway,szLocalGateway);
    strcpy(networkinfo.DNS1,szNameServer);
  }
  else
  {
    CLog::Log(LOGWARNING, "Not initializing network, using settings as they are setup by dashboard");
  }

  if (bSetup)
  {
    CLog::Log(LOGINFO, "setting up network...");
    int iCount=0;
    while (CUtil::SetUpNetwork( false, networkinfo )==1 && iCount < 100)
    {
      Sleep(50);
      iCount++;
    }
  }
  else
  {
    CLog::Log(LOGINFO, "init network");
    XNetStartupParams xnsp;
    memset(&xnsp, 0, sizeof(xnsp));
    xnsp.cfgSizeOfStruct = sizeof(XNetStartupParams);

    // Bypass security so that we may connect to 'untrusted' hosts
    xnsp.cfgFlags = XNET_STARTUP_BYPASS_SECURITY;
    // create more memory for networking
		xnsp.cfgPrivatePoolSizeInPages = 64; // == 256kb, default = 12 (48kb)
		xnsp.cfgEnetReceiveQueueLength = 16; // == 32kb, default = 8 (16kb)
		xnsp.cfgIpFragMaxSimultaneous = 16; // default = 4
		xnsp.cfgIpFragMaxPacketDiv256 = 32; // == 8kb, default = 8 (2kb)
		xnsp.cfgSockMaxSockets = 64; // default = 64
		xnsp.cfgSockDefaultRecvBufsizeInK = 128; // default = 16
		xnsp.cfgSockDefaultSendBufsizeInK = 128; // default = 16
    INT err = XNetStartup(&xnsp);
  }

  CLog::Log(LOGINFO, "get local ip address:");
  XNADDR xna;
  DWORD dwState;
  do
  {
    dwState = XNetGetTitleXnAddr(&xna);
    Sleep(50);
  } while (dwState==XNET_GET_XNADDR_PENDING);

  XNetInAddrToString(xna.ina,g_szTitleIP,32);

  CLog::Log(LOGINFO, "ip adres:%s",g_szTitleIP);
  WSADATA WsaData;
  int err = WSAStartup( MAKEWORD(2,2), &WsaData );

	if (err == NO_ERROR)
	{
		m_bNetworkUp = true;
		return true;
	}
	return false;
}

static const __int64 SECS_BETWEEN_EPOCHS = 11644473600;
static const __int64 SECS_TO_100NS       = 10000000;

void CUtil::ConvertTimeTToFileTime(__int64 sec, long nsec, FILETIME &ftTime)
{
  __int64 l64Result =((__int64)sec + SECS_BETWEEN_EPOCHS) + SECS_TO_100NS + (nsec / 100);
  ftTime.dwLowDateTime = (DWORD)l64Result;
  ftTime.dwHighDateTime = (DWORD)(l64Result>>32);
}


void CUtil::ReplaceExtension(const CStdString& strFile, const CStdString& strNewExtension, CStdString& strChangedFile)
{
  CStdString strExtension;
  GetExtension(strFile,strExtension);
  if ( strExtension.size() )
  {

    strChangedFile=strFile.substr(0, strFile.size()-strExtension.size()) ;
    strChangedFile+=strNewExtension;
  }
  else
  {
    strChangedFile=strFile;
    strChangedFile+=strNewExtension;
  }
}

void CUtil::GetExtension(const CStdString& strFile, CStdString& strExtension)
{
  int iPos=strFile.ReverseFind(".");
  if (iPos <0)
  {
    strExtension="";
    return;
  }
  strExtension=strFile.Right( strFile.size()-iPos);
}

void CUtil::Lower(CStdString& strText)
{
  char szText[1024];
  strcpy(szText, strText.c_str());
  //for multi-byte language, the strText.size() is not correct
  for (unsigned int i=0; i < strlen(szText);++i)
    szText[i]=tolower(szText[i]);
  strText=szText;
};

void CUtil::Unicode2Ansi(const wstring& wstrText,CStdString& strName)
{
  strName="";
  char *pstr=(char*)wstrText.c_str();
  for (int i=0; i < (int)wstrText.size();++i )
  {
    strName += pstr[i*2];
  }
}

bool CUtil::HasSlashAtEnd(const CStdString& strFile)
{
  if (strFile.size()==0) return false;
  char kar=strFile.c_str()[strFile.size()-1];
  if (kar=='/' || kar=='\\') return true;
  return false;
}

bool CUtil::IsRemote(const CStdString& strFile)
{
  CURL url(strFile);
  CStdString strProtocol=url.GetProtocol();
  strProtocol.ToLower();
  if (strProtocol=="cdda" || strProtocol=="iso9660") return false;
  if ( url.GetProtocol().size() ) return true;
  return false;
}

bool CUtil::IsDVD(const CStdString& strFile)
{
  if (strFile.Left(4)=="DVD:" || strFile.Left(4)=="dvd:")
    return true;
    
  if (strFile.Left(2)=="D:" || strFile.Left(2)=="d:")
    return true;

  if (strFile.Left(4)=="UDF:" || strFile.Left(4)=="udf:")
    return true;

  return false;
}

bool CUtil::IsCDDA(const CStdString& strFile)
{
	CURL url(strFile);
	if (url.GetProtocol()=="cdda")
		return true;
	return false;
}

bool CUtil::IsISO9660(const CStdString& strFile)
{
  CStdString strLeft=strFile.Left(8);
  strLeft.ToLower();
  if (strLeft=="iso9660:")
    return true;
  return false;
}

bool CUtil::IsSmb(const CStdString& strFile)
{
  CStdString strLeft = strFile.Left(4);
  return (strLeft.CompareNoCase("smb:") == 0);
}

void CUtil::GetFileAndProtocol(const CStdString& strURL, CStdString& strDir)
{
  strDir=strURL;
  if (!IsRemote(strURL)) return;
  if (IsDVD(strURL)) return;

  CURL url(strURL);
  strDir.Format("%s://%s", url.GetProtocol().c_str(), url.GetFileName().c_str());
}

void CUtil::RemoveCRLF(CStdString& strLine)
{
  while ( strLine.size() && (strLine.Right(1)=="\n" || strLine.Right(1)=="\r") )
  {
    strLine=strLine.Left((int)strLine.size()-1);
  }

}

int CUtil::GetDVDIfoTitle(const CStdString& strFile)
{
	CStdString strFilename = GetFileName(strFile);
	if (strFilename.Equals("video_ts.ifo")) return 0;
	//VTS_[TITLE]_0.IFO
	return atoi(strFilename.Mid(4,2).c_str());
}

 void CUtil::URLEncode(CStdString& strURLData)
{
  CStdString strResult;
  for (int i=0; i < (int)strURLData.size(); ++i)
  {
      int kar=(unsigned char)strURLData[i];
      if (kar==' ') strResult+='+';
      else if (isalnum(kar) || kar=='&'  || kar=='=' ) strResult+=kar;
      else {
        CStdString strTmp;
        strTmp.Format("%%%02.2x", kar);
        strResult+=strTmp;
      }
  }
  strURLData=strResult;
}

void CUtil::SaveString(const CStdString &strTxt, FILE *fd)
{
  int iSize=strTxt.size();
  fwrite(&iSize,1,sizeof(int),fd);
  if (iSize > 0)
  {
    fwrite(&strTxt.c_str()[0],1,iSize,fd);
  }
}
int  CUtil::LoadString(CStdString &strTxt, byte* pBuffer)
{
  strTxt="";
  int iSize;
  int iCount=sizeof(int);
  memcpy(&iSize,pBuffer,sizeof(int));
  if (iSize==0) return iCount;
  char *szTmp = new char [iSize+2];
  memcpy(szTmp ,&pBuffer[iCount], iSize);
  szTmp[iSize]=0;
  iCount+=iSize;
  strTxt=szTmp;
  delete [] szTmp;
  return iCount;
}
bool CUtil::LoadString(string &strTxt, FILE *fd)
{
  strTxt="";
  int iSize;
  int iRead=fread(&iSize,1,sizeof(int),fd);
  if (iRead != sizeof(int) ) return false;
  if (feof(fd)) return false;
  if (iSize==0) return true;
  if (iSize > 0 && iSize < 16384)
  {
    char *szTmp = new char [iSize+2];
    iRead=fread(szTmp,1,iSize,fd);
    if (iRead != iSize)
    {
      delete [] szTmp;
      return false;
    }
    szTmp[iSize]=0;
    strTxt=szTmp;
    delete [] szTmp;
    return true;
  }
  return false;
}

void CUtil::SaveInt(int iValue, FILE *fd)
{
  fwrite(&iValue,1,sizeof(int),fd);
}

int CUtil::LoadInt( FILE *fd)
{
  int iValue;
  fread(&iValue,1,sizeof(int),fd);
  return iValue;
}

void CUtil::LoadDateTime(SYSTEMTIME& dateTime, FILE *fd)
{
  fread(&dateTime,1,sizeof(dateTime),fd);
}

void CUtil::SaveDateTime(SYSTEMTIME& dateTime, FILE *fd)
{
  fwrite(&dateTime,1,sizeof(dateTime),fd);
}


void CUtil::GetSongInfo(const CStdString& strFileName, CStdString& strSongCacheName)
{
  Crc32 crc;
  crc.Compute(strFileName);
  strSongCacheName.Format("%s\\songinfo\\%x.si",g_stSettings.m_szAlbumDirectory,crc);
}

void CUtil::GetAlbumFolderThumb(const CStdString& strFileName, CStdString& strThumb, bool bTempDir /*=false*/)
{
  Crc32 crc;
  crc.ComputeFromLowerCase(strFileName);
  if (bTempDir)
    strThumb.Format("%s\\thumbs\\temp\\%x.tbn",g_stSettings.m_szAlbumDirectory,crc);
  else
    strThumb.Format("%s\\thumbs\\%x.tbn",g_stSettings.m_szAlbumDirectory,crc);
}

void CUtil::GetAlbumThumb(const CStdString& strAlbumName, const CStdString& strFileName, CStdString& strThumb, bool bTempDir /*=false*/)
{
  CStdString str;
  if (strAlbumName.IsEmpty())
  {
	  str = "unknown" + strFileName;
  }
  else
  {
	  str = strAlbumName + strFileName;
  }

  GetAlbumFolderThumb(str, strThumb, bTempDir);
}


bool CUtil::GetXBEIcon(const CStdString& strFilePath, CStdString& strIcon)
{
  // check if thumbnail already exists
	if (CUtil::IsDVD(strFilePath) && !CDetectDVDMedia::IsDiscInDrive() )
	{
	  strIcon="defaultDVDEmpty.png";
      return true;
	}


	if (CUtil::IsDVD(strFilePath) || g_guiSettings.GetBool("MyPrograms.CacheProgramThumbs") )		// create CRC for DVD as we can't store default.tbn on DVD
	{
	  Crc32 crc;
 	  crc.Compute(strFilePath);
	  strIcon.Format("%s\\%x.tbn",g_stSettings.szThumbnailsDirectory,crc);
	}
	else
	{
		CStdString strPath="";
		CStdString strFileName="";
		CStdString defaultTbn;
		CUtil::Split(strFilePath, strPath, strFileName);
		CUtil::ReplaceExtension(strFileName, ".tbn", defaultTbn);
		if (CUtil::HasSlashAtEnd(strPath))
			strPath.Delete(strPath.size()-1);
		
		strIcon.Format("%s\\%s", strPath.c_str(), defaultTbn.c_str());
	}

	if (CUtil::FileExists(strIcon) && !CUtil::IsDVD(strFilePath))   // always create thumbnail for DVD.
  {
    //yes, just return
    return true;
  }

  // no, then create a new thumb
  // Locate file ID and get TitleImage.xbx E:\UDATA\<ID>\TitleImage.xbx

  bool bFoundThumbnail=false;
  CStdString szFileName;
  szFileName.Format("E:\\UDATA\\%08x\\TitleImage.xbx", GetXbeID( strFilePath ) );
  if (!CUtil::FileExists(szFileName))
  {
    // extract icon from .xbe
    CXBE xbeReader;
    ::DeleteFile("T:\\1.xpr");
    if ( !xbeReader.ExtractIcon(strFilePath, "T:\\1.xpr"))
    {
      return false;
    }
    szFileName="T:\\1.xpr";
  }

  CXBPackedResource* pPackedResource = new CXBPackedResource();
  if( SUCCEEDED( pPackedResource->Create( szFileName.c_str(), 1, NULL ) ) )
  {
    LPDIRECT3DTEXTURE8 pTexture;
    LPDIRECT3DTEXTURE8 m_pTexture;
    D3DSURFACE_DESC descSurface;

    pTexture = pPackedResource->GetTexture((DWORD)0);

    if ( pTexture )
    {
      if ( SUCCEEDED( pTexture->GetLevelDesc( 0, &descSurface ) ) )
      {
        int iHeight=descSurface.Height;
        int iWidth=descSurface.Width;
        DWORD dwFormat=descSurface.Format;
        g_graphicsContext.Get3DDevice()->CreateTexture( 128,
                                  128,
                                  1,
                                  0,
                                  D3DFMT_LIN_A8R8G8B8,
                                  0,
                                  &m_pTexture);
        LPDIRECT3DSURFACE8 pSrcSurface = NULL;
        LPDIRECT3DSURFACE8 pDestSurface = NULL;

        pTexture->GetSurfaceLevel( 0, &pSrcSurface );
        m_pTexture->GetSurfaceLevel( 0, &pDestSurface );

        D3DXLoadSurfaceFromSurface( pDestSurface, NULL, NULL,
                                    pSrcSurface, NULL, NULL,
                                    D3DX_DEFAULT, D3DCOLOR( 0 ) );
        D3DLOCKED_RECT rectLocked;
        if ( D3D_OK == m_pTexture->LockRect(0,&rectLocked,NULL,0L  ) )
        {
            BYTE *pBuff   = (BYTE*)rectLocked.pBits;
            if (pBuff)
            {
              DWORD strideScreen=rectLocked.Pitch;
              //mp_msg(0,0," strideScreen=%i\n", strideScreen);

              CSectionLoader::Load("CXIMAGE");
              CxImage* pImage = new CxImage(iWidth, iHeight, 24, CXIMAGE_FORMAT_JPG);
              for (int y=0; y < iHeight; y++)
              {
                byte *pPtr = pBuff+(y*(strideScreen));
                for (int x=0; x < iWidth;x++)
                {
                  byte Alpha=*(pPtr+3);
                  byte b=*(pPtr+0);
                  byte g=*(pPtr+1);
                  byte r=*(pPtr+2);
                  pPtr+=4;
                  pImage->SetPixelColor(x,y,RGB(r,g,b));
                }
              }

              m_pTexture->UnlockRect(0);
              pImage->Flip();
              pImage->Save(strIcon.c_str(),CXIMAGE_FORMAT_JPG);
              delete pImage;
              bFoundThumbnail=true;
              CSectionLoader::Unload("CXIMAGE");
            }
            else m_pTexture->UnlockRect(0);
        }
        pSrcSurface->Release();
        pDestSurface->Release();
        m_pTexture->Release();
      }
      pTexture->Release();
    }
  }
  delete pPackedResource;
  if (bFoundThumbnail) CUtil::ClearCache();
  return bFoundThumbnail;
}


bool CUtil::GetDirectoryName(const CStdString& strFileName, CStdString& strDescription)
{
  CStdString strFName=CUtil::GetFileName(strFileName);
  strDescription=strFileName.Left(strFileName.size()-strFName.size());
  if (CUtil::HasSlashAtEnd(strDescription) )
  {
    strDescription=strDescription.Left(strDescription.size()-1);
  }
  int iPos=strDescription.ReverseFind("\\");
  if (iPos < 0)
    iPos=strDescription.ReverseFind("/");
  if (iPos >=0)
  {
    strDescription=strDescription.Right(strDescription.size()-iPos-1);
  }

  else strDescription=strFName;
  return true;
}
bool CUtil::GetXBEDescription(const CStdString& strFileName, CStdString& strDescription)
{

    _XBE_CERTIFICATE HC;
    _XBE_HEADER HS;

    FILE* hFile  = fopen(strFileName.c_str(),"rb");
    if (!hFile)
    {
      strDescription=CUtil::GetFileName(strFileName);
      return false;
    }
    fread(&HS,1,sizeof(HS),hFile);
    fseek(hFile,HS.XbeHeaderSize,SEEK_SET);
    fread(&HC,1,sizeof(HC),hFile);
    fclose(hFile);

    CHAR TitleName[40];
    WideCharToMultiByte(CP_ACP,0,HC.TitleName,-1,TitleName,40,NULL,NULL);
    if (strlen(TitleName) > 0)
    {
      strDescription=TitleName;
      return true;
    }
    strDescription=CUtil::GetFileName(strFileName);
    return false;
}

DWORD CUtil::GetXbeID( const CStdString& strFilePath)
{
  DWORD dwReturn = 0;

  DWORD dwCertificateLocation;
  DWORD dwLoadAddress;
  DWORD dwRead;
//  WCHAR wcTitle[41];

  CAutoPtrHandle  hFile( CreateFile( strFilePath.c_str(),
            GENERIC_READ,
            FILE_SHARE_READ,
            NULL,
            OPEN_EXISTING,
            FILE_ATTRIBUTE_NORMAL,
            NULL ));
  if ( hFile.isValid() )
  {
    if ( SetFilePointer(  (HANDLE)hFile,  0x104, NULL, FILE_BEGIN ) == 0x104 )
    {
      if ( ReadFile( (HANDLE)hFile, &dwLoadAddress, 4, &dwRead, NULL ) )
      {
        if ( SetFilePointer(  (HANDLE)hFile,  0x118, NULL, FILE_BEGIN ) == 0x118 )
        {
          if ( ReadFile( (HANDLE)hFile, &dwCertificateLocation, 4, &dwRead, NULL ) )
          {
            dwCertificateLocation -= dwLoadAddress;
            // Add offset into file
            dwCertificateLocation += 8;
            if ( SetFilePointer(  (HANDLE)hFile,  dwCertificateLocation, NULL, FILE_BEGIN ) == dwCertificateLocation )
            {
              dwReturn = 0;
              ReadFile( (HANDLE)hFile, &dwReturn, sizeof(DWORD), &dwRead, NULL );
              if ( dwRead != sizeof(DWORD) )
              {
                dwReturn = 0;
              }
            }

          }
        }
      }
    }
  }
  return dwReturn;
}

void CUtil::FillInDefaultIcons(VECFILEITEMS &items)
{
  for (int i=0; i < (int)items.size(); ++i)
  {
    CFileItem* pItem=items[i];
    pItem->FillInDefaultIcon();

  }
}

void CUtil::CreateShortcuts(VECFILEITEMS &items)
{
	for (int i=0; i < (int)items.size(); ++i)
	{
		CFileItem* pItem=items[i];
		CreateShortcut(pItem);

	}
}

void CUtil::CreateShortcut(CFileItem* pItem)
{
	bool bOnlyDefaultXBE=g_guiSettings.GetBool("MyPrograms.DefaultXBEOnly");
	if ( bOnlyDefaultXBE ? pItem->IsDefaultXBE() : pItem->IsXBE() )
	{
		// xbe
		pItem->SetIconImage("defaultProgram.png");
		if ( !pItem->IsDVD() )
		{
			CStdString strDescription;
			if (! CUtil::GetXBEDescription(pItem->m_strPath,strDescription))
			{
				CUtil::GetDirectoryName(pItem->m_strPath, strDescription);
			}
			if (strDescription.size())
			{
				CStdString strFname;
				strFname=CUtil::GetFileName(pItem->m_strPath);
				strFname.ToLower();
				if (strFname!="dashupdate.xbe" && strFname!="downloader.xbe" && strFname != "update.xbe")
				{
					CShortcut cut;
					cut.m_strPath=pItem->m_strPath;
					cut.Save(strDescription);
				}
			}
		}
	}
}


void CUtil::SetThumbs(VECFILEITEMS &items)
{
  //cache thumbnails directory
	g_directoryCache.InitThumbCache();

  for (int i=0; i < (int)items.size(); ++i)
  {
    CFileItem* pItem=items[i];
    pItem->SetThumb();
  }

  g_directoryCache.ClearThumbCache();
}

bool CUtil::GetFolderThumb(const CStdString& strFolder, CStdString& strThumb)
{
  // get the thumbnail for the folder contained in strFolder
  // and return the filename of the thumbnail in strThumb
  //
  // if folder contains folder.jpg and is local on xbox HD then use it as the thumbnail
  // if folder contains folder.jpg but is located on a share then cache the folder.jpg
  // to q:\thumbs and return the cached image as a thumbnail
  CStdString strFolderImage;
  strThumb="";
	CUtil::AddFileToFolder(strFolder, "folder.jpg", strFolderImage);

	CFileItem item(strFolder, true);
  // remote or local file?
  if (item.IsRemote() || item.IsDVD() || item.IsISO9660() )
  {
    // dont try to locate a folder.jpg for streams &  shoutcast
    if (item.IsInternetStream())
      return false;

    CUtil::GetThumbnail( strFolderImage,strThumb);
    // if local cache of thumb doesnt exists yet
    if (!CUtil::FileExists( strThumb) )
    {
      CFile file;
      // then cache folder.jpg to xbox HD
			if (g_guiSettings.GetBool("VideoLibrary.FindRemoteThumbs") && (file.Exists(strFolderImage)))
			{
				if ( file.Cache(strFolderImage.c_str(), strThumb.c_str(),NULL,NULL))
				{
					return true;
				}
			}
    }
    else
    {
      // else used the cached version
      return true;
    }
  }
  else if (CUtil::FileExists(strFolderImage) )
  {
    // is local, and folder.jpg exists. Use it
    strThumb=strFolderImage;
    return true;
  }

  // no thumb found
  strThumb="";
  return false;
}

void CUtil::ShortenFileName(CStdString& strFileNameAndPath)
{
  CStdString strFile=CUtil::GetFileName(strFileNameAndPath);
  if (strFile.size() > 42)
  {
    CStdString strExtension;
    CUtil::GetExtension(strFileNameAndPath, strExtension);
    CStdString strPath=strFileNameAndPath.Left( strFileNameAndPath.size() - strFile.size() );

    strFile=strFile.Left(42-strExtension.size());
    strFile+=strExtension;

    CStdString strNewFile=strPath;
    if (!CUtil::HasSlashAtEnd(strPath))
      strNewFile+="\\";

    strNewFile+=strFile;
    strFileNameAndPath=strNewFile;
  }
}

void CUtil::ConvertPathToUrl( const CStdString& strPath, const CStdString& strProtocol, CStdString& strOutUrl )
{
  strOutUrl = strProtocol;
  CStdString temp = strPath;
  temp.Replace( '\\', '/' );
  temp.Delete( 0, 3 );
  strOutUrl += temp;
}

void CUtil::GetDVDDriveIcon( const CStdString& strPath, CStdString& strIcon )
{
  if ( !CDetectDVDMedia::IsDiscInDrive() ) {
    strIcon="defaultDVDEmpty.png";
    return;
  }

  if ( IsDVD(strPath) ) {
    CCdInfo* pInfo = CDetectDVDMedia::GetCdInfo();
    //  xbox DVD
    if ( pInfo != NULL && pInfo->IsUDFX( 1 ) ) {
      strIcon="defaultXBOXDVD.png";
      return;
    }
    strIcon="defaultDVDRom.png";
    return;
  }

  if ( IsISO9660(strPath) ) {
    CCdInfo* pInfo = CDetectDVDMedia::GetCdInfo();
    if ( pInfo != NULL && pInfo->IsVideoCd( 1 ) ) {
      strIcon="defaultVCD.png";
      return;
    }
    strIcon="defaultDVDRom.png";
    return;
  }

  if ( IsCDDA(strPath) ) {
    strIcon="defaultCDDA.png";
    return;
  }
}

void CUtil::RemoveTempFiles()
{
  WIN32_FIND_DATA wfd;

  CStdString strAlbumDir;
  strAlbumDir.Format("%s\\*.tmp",g_stSettings.m_szAlbumDirectory);
  memset(&wfd,0,sizeof(wfd));

  CAutoPtrFind hFind( FindFirstFile(strAlbumDir.c_str(),&wfd));
  if (!hFind.isValid())
    return ;
  do
  {
    if ( !(wfd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) )
    {
      string strFile=g_stSettings.m_szAlbumDirectory;
	  strFile += "\\";
      strFile += wfd.cFileName;
      DeleteFile(strFile.c_str());
    }
  } while (FindNextFile(hFind, &wfd));

  //CStdString strTempThumbDir;
  //strTempThumbDir.Format("%s\\thumbs\\temp\\*.tbn",g_stSettings.m_szAlbumDirectory);
  //memset(&wfd,0,sizeof(wfd));

  //CAutoPtrFind hFind1( FindFirstFile(strTempThumbDir.c_str(),&wfd));
  //if (!hFind1.isValid())
  //  return ;
  //do
  //{
  //  if ( !(wfd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) )
  //  {
  //    CStdString strFile;
  //    strFile.Format("%s\\thumbs\\temp\\",g_stSettings.m_szAlbumDirectory);
  //    strFile += wfd.cFileName;
  //    DeleteFile(strFile.c_str());
  //  }
  //} while (FindNextFile(hFind1, &wfd));
}

void CUtil::DeleteTDATA()
{
  WIN32_FIND_DATA wfd;
  CStdString strTDATADir;
  strTDATADir = "T:\\*.*";
  memset(&wfd,0,sizeof(wfd));

  CAutoPtrFind hFind( FindFirstFile(strTDATADir.c_str(),&wfd));
  if (!hFind.isValid())
    return ;
  do
  {
    if ( !(wfd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) )
    {
 	  string strFile="T:\\";
      strFile += wfd.cFileName;
	  CLog::Log(LOGINFO, "  DeleteFile(%s)", strFile.c_str());
	  DeleteFile(strFile.c_str());
    }
  } while (FindNextFile(hFind, &wfd));
}

bool CUtil::IsHD(const CStdString& strFileName)
{
  if (strFileName.size()<=2) return false;
  char szDriveletter=tolower(strFileName.GetAt(0));
  if ( (szDriveletter >= 'c'&& szDriveletter <= 'g' && szDriveletter != 'd') || (szDriveletter=='q')  )
  {
    if (strFileName.GetAt(1)==':') return true;
  }
  return false;
}

// Following 6 routines added by JM to determine (possible) source type based
// on frame size
bool CUtil::IsNTSC_VCD(int iWidth, int iHeight)
{
  return (iWidth==352 && iHeight==240);
}

bool CUtil::IsNTSC_SVCD(int iWidth, int iHeight)
{
  return (iWidth==480 && iHeight==480);
}

bool CUtil::IsNTSC_DVD(int iWidth, int iHeight)
{
  return (iWidth==720 && iHeight==480);
}

bool CUtil::IsPAL_VCD(int iWidth, int iHeight)
{
  return (iWidth==352 && iHeight==488);
}

bool CUtil::IsPAL_SVCD(int iWidth, int iHeight)
{
  return (iWidth==480 && iHeight==576);
}

bool CUtil::IsPAL_DVD(int iWidth, int iHeight)
{
  return (iWidth==720 && iHeight==576);
}


void CUtil::RemoveIllegalChars( CStdString& strText)
{
  char szRemoveIllegal [1024];
  strcpy(szRemoveIllegal ,strText.c_str());
  static char legalChars[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz1234567890!#$%&'()-@[]^_`{}~ ";
  char *cursor;
  for (cursor = szRemoveIllegal; *(cursor += strspn(cursor, legalChars)); /**/ )
    *cursor = '_';
  strText=szRemoveIllegal;
}

void CUtil::ClearSubtitles()
{
	//delete cached subs
	WIN32_FIND_DATA wfd;
	CAutoPtrFind hFind ( FindFirstFile("Z:\\*.*",&wfd));

	if (hFind.isValid())
	{
		do
		{
			if (wfd.cFileName[0]!=0)
			{
				if ( (wfd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)==0 )
				{
					CStdString strFile;
					strFile.Format("Z:\\%s", wfd.cFileName);
					if (strFile.Find("subtitle")>=0 )
					{
						::DeleteFile(strFile.c_str());
					}
					else if (strFile.Find("vobsub_queue")>=0 )
					{
						::DeleteFile(strFile.c_str());
					}
				}
			}
		} while (FindNextFile((HANDLE)hFind, &wfd));
	}
}

void CUtil::CacheSubtitles(const CStdString& strMovie, CStdString& strExtensionCached )
{
  char * sub_exts[] = {  ".utf", ".utf8", ".utf-8", ".sub", ".srt", ".smi", ".rt", ".txt", ".ssa", ".aqt", ".jss", ".ass", ".idx",".ifo", NULL};
  strExtensionCached = "";

	ClearSubtitles();

  CFileItem item(strMovie, false);
  if (item.IsInternetStream()) return;
  if (item.IsPlayList()) return;
  if (!item.IsVideo()) return;

  const int iPaths = 2;
  CStdString strLookInPaths[2];
  
  CStdString strFileName;
  CStdString strFileNameNoExt;
  
  CUtil::Split(strMovie,strLookInPaths[0],strFileName);
  ReplaceExtension(strFileName, "", strFileNameNoExt);

	if (strlen(g_stSettings.m_szAlternateSubtitleDirectory) != 0)
	{
		strLookInPaths[1] = g_stSettings.m_szAlternateSubtitleDirectory;
    if(!HasSlashAtEnd(strLookInPaths[1]))
      strLookInPaths[1] += "/"; //Should work for both remote and local files
	}
  else
    strLookInPaths[1] = "";

  CStdString strLExt;
  CStdString strDest;
  CStdString strItem, strPath;
  CFile file;

	// 2 steps for movie directory and alternate subtitles directory
	for(int step=0; step<iPaths; step++)
	{
    if(strLookInPaths[step].length() != 0)
    {

      g_directoryCache.ClearDirectory(strLookInPaths[step]);


			VECFILEITEMS items;
      CFileItemList temp(items); //Releases memory when it goes out of scope

      CDirectory::GetDirectory(strLookInPaths[step],items);
			int fnl = strFileNameNoExt.size();

			for (int j=0; j < (int)items.size(); j++)
			{
			  for(int i=0; sub_exts[i]; i++)
			  {
          int l = strlen(sub_exts[i]);
          Split(items[j]->m_strPath, strPath, strItem);
           
          //Cache any alternate subtitles.
          if (strItem.Left(9) == "subtitle." && strItem.Right(l).ToLower() == sub_exts[i])
          {
						strLExt = strItem.Right(strItem.GetLength() - 9);
						strDest.Format("Z:\\subtitle.alt-%s", strLExt);
						if (file.Cache(items[j]->m_strPath, strDest.c_str(),NULL,NULL))
						{
              CLog::Log(LOGINFO, " cached subtitle %s->%s\n", strItem.c_str(), strDest.c_str());
							strExtensionCached = strLExt;
						}            
          }

          //Cache subtitle with same name as movie
					if (strItem.Right(l).ToLower() == sub_exts[i] && strItem.Left(fnl) == strFileNameNoExt)
					{
						strLExt = strItem.Right(strItem.size() - fnl - 1); //Disregard separator char
            strDest.Format("Z:\\subtitle.%s", strLExt);
						if (file.Cache(items[j]->m_strPath, strDest.c_str(),NULL,NULL))
						{
              CLog::Log(LOGINFO, " cached subtitle %s->%s\n", strItem.c_str(), strDest.c_str());
							strExtensionCached = strLExt;
						}
					}
				}
			}			
		}
	}
}

void CUtil::SecondsToHMSString(long lSeconds, CStdString& strHMS, bool bMustUseHHMMSS)
{
  int hh = lSeconds / 3600;
  lSeconds = lSeconds%3600;
  int mm = lSeconds / 60;
  int ss = lSeconds % 60;

  if (hh>=1 || bMustUseHHMMSS)
    strHMS.Format("%2.2i:%02.2i:%02.2i",hh,mm,ss);
  else
    strHMS.Format("%i:%02.2i",mm,ss);

}
void CUtil::PrepareSubtitleFonts()
{
  if (g_guiSettings.GetString("Subtitles.Font").size()==0) return;
  if (g_guiSettings.GetInt("Subtitles.Height")==0) return;

  CStdString strPath,strHomePath,strSearchMask;
//  if(!g_guiSettings.GetBool("MyVideos.AlternateMPlayer"))
    strHomePath = "Q:\\system\\players";
//  else
//    strHomePath = "Q:";

  strPath.Format("%s\\mplayer\\font\\%s\\%i\\",
          strHomePath.c_str(),
          g_guiSettings.GetString("Subtitles.Font").c_str(),g_guiSettings.GetInt("Subtitles.Height"));

  strSearchMask=strPath+"*.*";
  WIN32_FIND_DATA wfd;
  CAutoPtrFind hFind ( FindFirstFile(strSearchMask.c_str(),&wfd));
  if (hFind.isValid())
  {
    do
    {
      if (wfd.cFileName[0]!=0)
      {
        if ( (wfd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)==0 )
        {
          CStdString strSource,strDest;
          strSource.Format("%s%s",strPath.c_str(),wfd.cFileName);
          strDest.Format("%s\\mplayer\\font\\%s",strHomePath.c_str(),  wfd.cFileName);
          ::CopyFile(strSource.c_str(),strDest.c_str(),FALSE);
        }
      }
    } while (FindNextFile((HANDLE)hFind, &wfd));
  }
}

__int64 CUtil::ToInt64(DWORD dwHigh, DWORD dwLow)
{
  __int64 n;
  n=dwHigh;
  n <<=32;
  n += dwLow;
  return n;
}

void CUtil::AddFileToFolder(const CStdString& strFolder, const CStdString& strFile, CStdString& strResult)
{
  strResult=strFolder;
  if (!CUtil::HasSlashAtEnd(strResult))
  {
    if (strResult.Find("//")>=0 )
      strResult+="/";
    else
      strResult+="\\";
  }
  strResult += strFile;
}

void CUtil::GetPath(const CStdString& strFileName, CStdString& strPath)
{
  int iPos1=strFileName.Find("/");
  int iPos2=strFileName.Find("\\");
  int iPos3=strFileName.Find(":");
  if (iPos2>iPos1) iPos1=iPos2;
  if (iPos3>iPos1) iPos1=iPos3;

  strPath=strFileName.Left(iPos1-1);
}
//Will from a full filename return the directory the file resides in.
//has no trailing slash on result. Could lead to problems when reading from root on cd
//ISO9660://filename.bla will result in path ISO9660:/ 
//This behaviour should probably be changed, but it would break other things
void CUtil::GetDirectory(const CStdString& strFilePath, CStdString& strDirectoryPath)
{
	int iPos1=strFilePath.ReverseFind('/');
	int iPos2=strFilePath.ReverseFind('\\');

	if (iPos2>iPos1)
	{
		iPos1=iPos2;
	}

	if (iPos1>0)
	{
		strDirectoryPath = strFilePath.Left(iPos1);
	}
}
//Splits a full filename in path and file.
//ex. smb://computer/share/directory/filename.ext -> strPath:smb://computer/share/directory/ and strFileName:filename.ext
//Trailing slash will be preserved
void CUtil::Split(const CStdString& strFileNameAndPath, CStdString& strPath, CStdString& strFileName)
{
  strFileName="";
  strPath="";
  int i=strFileNameAndPath.size()-1;
  while (i > 0)
  {
    char ch=strFileNameAndPath[i];
    if (ch==':' || ch=='/' || ch=='\\') break;
    else i--;
  }
  strPath     = strFileNameAndPath.Left(i + 1);
  strFileName = strFileNameAndPath.Right(strFileNameAndPath.size() - i - 1);
}

int CUtil::GetFolderCount(VECFILEITEMS &items)
{
  int nFolderCount=0;
  for (int i = 0; i < (int)items.size(); i++)
  {
    CFileItem* pItem = items[i];
    if (pItem->m_bIsFolder)
      nFolderCount++;
  }

  return nFolderCount;
}

int CUtil::GetFileCount(VECFILEITEMS &items)
{
  int nFileCount=0;
  for (int i = 0; i < (int)items.size(); i++)
  {
    CFileItem* pItem = items[i];
    if (! pItem->m_bIsFolder)
      nFileCount++;
  }

  return nFileCount;
}

bool CUtil::ThumbExists(const CStdString& strFileName, bool bAddCache)
{
  return CThumbnailCache::GetThumbnailCache()->ThumbExists(strFileName, bAddCache);
}

void CUtil::ThumbCacheAdd(const CStdString& strFileName, bool bFileExists)
{
  CThumbnailCache::GetThumbnailCache()->Add(strFileName, bFileExists);
}

void CUtil::ThumbCacheClear()
{
  CThumbnailCache::GetThumbnailCache()->Clear();
}

bool CUtil::ThumbCached(const CStdString& strFileName)
{
  return CThumbnailCache::GetThumbnailCache()->IsCached(strFileName);
}

void CUtil::PlayDVD()
{
  if (g_stSettings.m_szExternalDVDPlayer[0] && strcmp(g_stSettings.m_szExternalDVDPlayer, "dvdplayerbeta") != 0)
  {
    char szPath[1024];
    char szDevicePath[1024];
    char szXbePath[1024];
    char szParameters[1024];
    memset(szParameters,0,sizeof(szParameters));
    strcpy(szPath,g_stSettings.m_szExternalDVDPlayer);
    char* szBackslash = strrchr(szPath,'\\');
    *szBackslash=0x00;
    char* szXbe = &szBackslash[1];
    char* szColon = strrchr(szPath,':');
    *szColon=0x00;
    char* szDrive = szPath;
    char* szDirectory = &szColon[1];
    CIoSupport helper;
    helper.GetPartition( (LPCSTR) szDrive, szDevicePath);
    strcat(szDevicePath,szDirectory);
    wsprintf(szXbePath,"d:\\%s",szXbe);

    g_application.Stop();
    CUtil::LaunchXbe(szDevicePath,szXbePath,NULL);
  }
  else
  {
    CIoSupport helper;
    helper.Remount("D:","Cdrom0");
    CFileItem item("dvd://1", false);
    g_application.PlayFile(item);
  }
}

DWORD CUtil::SetUpNetwork( bool resetmode, struct network_info& networkinfo )
{
  static unsigned char params[512];
  static DWORD        vReturn;
  static XNADDR       sAddress;
  char temp_str[64];
  static char mode = 0;
  XNADDR xna;
  DWORD dwState;

  if( resetmode )
  {
    resetmode = 0;
    mode = 0;
  }

  if( !XNetGetEthernetLinkStatus() )
  {
    return 1;
  }

  if( mode == 100 )
  {
    CLog::Log(LOGDEBUG, "  pending...");
    dwState = XNetGetTitleXnAddr(&xna);
    if(dwState==XNET_GET_XNADDR_PENDING)
      return 1;
    mode = 5;
    char  azIPAdd[256];
    memset( azIPAdd,0,256);
    XNetInAddrToString(xna.ina,azIPAdd,32);
    CLog::Log(LOGINFO, "ip address:%s",azIPAdd);
  }

  // if local address is specified
  if( mode == 0 )
  {
    if ( !networkinfo.DHCP )
    {
      TXNetConfigParams configParams;

      XNetLoadConfigParams( (LPBYTE) &configParams );
      BOOL bXboxVersion2 = (configParams.V2_Tag == 0x58425632 );  // "XBV2"
      BOOL bDirty = FALSE;

      if (bXboxVersion2)
      {
        if (configParams.V2_IP != inet_addr(networkinfo.ip))
        {
          configParams.V2_IP = inet_addr(networkinfo.ip);
          bDirty = TRUE;
        }
      }
      else
      {
        if (configParams.V1_IP != inet_addr(networkinfo.ip))
        {
          configParams.V1_IP = inet_addr(networkinfo.ip);
          bDirty = TRUE;
        }
      }

      if (bXboxVersion2)
      {
        if (configParams.V2_Subnetmask != inet_addr(networkinfo.subnet))
        {
          configParams.V2_Subnetmask = inet_addr(networkinfo.subnet);
          bDirty = TRUE;
        }
      }
      else
      {
        if (configParams.V1_Subnetmask != inet_addr(networkinfo.subnet))
        {
          configParams.V1_Subnetmask = inet_addr(networkinfo.subnet);
          bDirty = TRUE;
        }
      }

      if (bXboxVersion2)
      {
        if (configParams.V2_Defaultgateway != inet_addr(networkinfo.gateway))
        {
          configParams.V2_Defaultgateway = inet_addr(networkinfo.gateway);
          bDirty = TRUE;
        }
      }
      else
      {
        if (configParams.V1_Defaultgateway != inet_addr(networkinfo.gateway))
        {
          configParams.V1_Defaultgateway = inet_addr(networkinfo.gateway);
          bDirty = TRUE;
        }
      }

      if (bXboxVersion2)
      {
        if (configParams.V2_DNS1 != inet_addr(networkinfo.DNS1))
        {
          configParams.V2_DNS1 = inet_addr(networkinfo.DNS1);
          bDirty = TRUE;
        }
      }
      else
      {
        if (configParams.V1_DNS1 != inet_addr(networkinfo.DNS1))
        {
          configParams.V1_DNS1 = inet_addr(networkinfo.DNS1);
          bDirty = TRUE;
        }
      }

      if (bXboxVersion2)
      {
        if (configParams.V2_DNS2 != inet_addr(networkinfo.DNS2))
        {
          configParams.V2_DNS2 = inet_addr(networkinfo.DNS2);
          bDirty = TRUE;
        }
      }
      else
      {
        if (configParams.V1_DNS2 != inet_addr(networkinfo.DNS2))
        {
          configParams.V1_DNS2 = inet_addr(networkinfo.DNS2);
          bDirty = TRUE;
        }
      }

      if (configParams.Flag != (0x04|0x08) )
      {
        configParams.Flag = 0x04 | 0x08;
        bDirty = TRUE;
      }

      XNetSaveConfigParams( (LPBYTE) &configParams );

      XNetStartupParams xnsp;
      memset(&xnsp, 0, sizeof(xnsp));
      xnsp.cfgSizeOfStruct = sizeof(XNetStartupParams);

      // Bypass security so that we may connect to 'untrusted' hosts
      xnsp.cfgFlags = XNET_STARTUP_BYPASS_SECURITY;
    // create more memory for networking
			xnsp.cfgPrivatePoolSizeInPages = 64; // == 256kb, default = 12 (48kb)
			xnsp.cfgEnetReceiveQueueLength = 16; // == 32kb, default = 8 (16kb)
			xnsp.cfgIpFragMaxSimultaneous = 16; // default = 4
			xnsp.cfgIpFragMaxPacketDiv256 = 32; // == 8kb, default = 8 (2kb)
			xnsp.cfgSockMaxSockets = 64; // default = 64
			xnsp.cfgSockDefaultRecvBufsizeInK = 128; // default = 16
			xnsp.cfgSockDefaultSendBufsizeInK = 128; // default = 16
      CLog::Log(LOGINFO, "requesting local ip adres");
      int err = XNetStartup(&xnsp);
      mode = 100;
      return 1;
    }
    else
    {
      /**     Set DHCP-flags from a known DHCP mode  (maybe some day we will fix this)  **/
      XNetLoadConfigParams(params);
      memset( params, 0, (sizeof(IN_ADDR) * 5) + 20 );
      params[40]=33;  params[41]=223; params[42]=196; params[43]=67;  params[44]=6;
      params[45]=145; params[46]=157; params[47]=118; params[48]=182; params[49]=239;
      params[50]=68;  params[51]=197; params[52]=133; params[53]=150; params[54]=118;
      params[55]=211; params[56]=38;  params[57]=87;  params[58]=222; params[59]=119;
      params[64]=0; params[72]=0; params[73]=0; params[74]=0; params[75]=0;
      params[340]=160;    params[341]=93;       params[342]=131;      params[343]=191;      params[344]=46;

      XNetStartupParams xnsp;

      memset(&xnsp, 0, sizeof(xnsp));
      xnsp.cfgSizeOfStruct = sizeof(XNetStartupParams);
      // Bypass security so that we may connect to 'untrusted' hosts
      xnsp.cfgFlags = XNET_STARTUP_BYPASS_SECURITY;

			xnsp.cfgPrivatePoolSizeInPages = 64; // == 256kb, default = 12 (48kb)
			xnsp.cfgEnetReceiveQueueLength = 16; // == 32kb, default = 8 (16kb)
			xnsp.cfgIpFragMaxSimultaneous = 16; // default = 4
			xnsp.cfgIpFragMaxPacketDiv256 = 32; // == 8kb, default = 8 (2kb)
			xnsp.cfgSockMaxSockets = 64; // default = 64
			xnsp.cfgSockDefaultRecvBufsizeInK = 128; // default = 16
			xnsp.cfgSockDefaultSendBufsizeInK = 128; // default = 16

      XNetSaveConfigParams(params);
      CLog::Log(LOGINFO, "requesting DHCP");
      int err = XNetStartup(&xnsp);
      mode = 5;
    }


  }

  char g_szTitleIP[32];

  WSADATA WsaData;
  int err;

  char ftploop;
  if( mode == 5 )
  {
    XNADDR xna;
    DWORD dwState;

    dwState = XNetGetTitleXnAddr(&xna);

    if( dwState==XNET_GET_XNADDR_PENDING)
      return 1;

    XNetInAddrToString(xna.ina,g_szTitleIP,32);
    err = WSAStartup( MAKEWORD(2,2), &WsaData );
    ftploop = 1;
    mode ++;
  }

  if( mode == 6 )
  {
    vReturn = XNetGetTitleXnAddr(&sAddress);


    ftploop = 1;

    if( vReturn != XNET_GET_XNADDR_PENDING )
    {
      char  azIPAdd[256];
      //char  azMessage[256];

      memset(azIPAdd,0,sizeof(azIPAdd));

      XNetInAddrToString(sAddress.ina,azIPAdd,sizeof(azIPAdd));
      //strcpy(NetworkStatus,azIPAdd);
      //strcpy( NetworkStatusInternal, NetworkStatus );
			if (sAddress.ina.S_un.S_addr != 0)
      {
        DWORD temp = XNetGetEthernetLinkStatus();
        if(  temp & XNET_ETHERNET_LINK_ACTIVE )
        {

          if ( temp & XNET_ETHERNET_LINK_FULL_DUPLEX )
          {
            // full duplex
            CLog::Log(LOGINFO, "  full duplex");
          }

          if ( temp & XNET_ETHERNET_LINK_HALF_DUPLEX )
          {
            // half duplex
            CLog::Log(LOGINFO, "  half duplex");
          }

          if ( temp & XNET_ETHERNET_LINK_100MBPS )
          {
            CLog::Log(LOGINFO, "  100 mbps");
          }

          if ( temp & XNET_ETHERNET_LINK_10MBPS )
          {
            CLog::Log(LOGINFO, "  10bmps");
          }

          if ( vReturn &  XNET_GET_XNADDR_STATIC )
          {
            CLog::Log(LOGINFO, "  static ip");
          }

          if ( vReturn &  XNET_GET_XNADDR_DHCP )
          {
            CLog::Log(LOGINFO, "  Dynamic IP");
          }

          if ( vReturn & XNET_GET_XNADDR_DNS )
          {
            CLog::Log(LOGINFO, "  DNS");
          }

          if ( vReturn & XNET_GET_XNADDR_ETHERNET )
          {
            CLog::Log(LOGINFO, "  ethernet");
          }

          if ( vReturn & XNET_GET_XNADDR_NONE )
          {
            CLog::Log(LOGINFO, "  none");
          }

          if ( vReturn & XNET_GET_XNADDR_ONLINE )
          {
            CLog::Log(LOGINFO, "  online");
          }

          if ( vReturn & XNET_GET_XNADDR_PENDING )
          {
            CLog::Log(LOGINFO, "  pending");
          }

          if ( vReturn & XNET_GET_XNADDR_TROUBLESHOOT )
          {
            CLog::Log(LOGINFO, "  error");
          }

          if ( vReturn & XNET_GET_XNADDR_PPPOE )
          {
            CLog::Log(LOGINFO, "  ppoe");
          }

          sprintf(temp_str,"  IP: %s",azIPAdd);
          CLog::Log(LOGINFO, temp_str);
        }
        ftploop = 0;
        mode ++;
        return 0;
      }
			return 2;
    }

    Sleep(50);
    return 1;
  }
  return 1;
}

void CUtil::GetVideoThumbnail(const CStdString& strIMDBID, CStdString& strThumb)
{
  strThumb.Format("%s\\imdb\\imdb%s.jpg",g_stSettings.szThumbnailsDirectory,strIMDBID.c_str());
}

void CUtil::SetMusicThumbs(VECFILEITEMS &items)
{
  //cache thumbnails directory
	g_directoryCache.InitMusicThumbCache();

  for (int i=0; i < (int)items.size(); ++i)
  {
    CFileItem* pItem=items[i];
    pItem->SetMusicThumb();
  }

  g_directoryCache.ClearMusicThumbCache();
}

CStdString CUtil::GetNextFilename(const char* fn_template, int max)
{
	// Open the file.
	char szName[1024];

	INT i;

	WIN32_FIND_DATA wfd;
	HANDLE hFind;


	if (NULL != strstr(fn_template, "%03d"))
	{
		for(i = 0; i <= max; i++)
		{

			wsprintf(szName, fn_template, i);

			memset(&wfd, 0, sizeof(wfd));
			if ((hFind = FindFirstFile(szName, &wfd)) != INVALID_HANDLE_VALUE)
				FindClose(hFind);
			else
			{
				// FindFirstFile didn't find the file 'szName', return it
				return szName;
			}
		}
	}

	return ""; // no fn generated
}

void CUtil::InitGamma()
{
  g_graphicsContext.Get3DDevice()->GetGammaRamp(&oldramp);
}
void CUtil::RestoreBrightnessContrastGamma()
{
  g_graphicsContext.Lock();
  g_graphicsContext.Get3DDevice()->SetGammaRamp(D3DSGR_IMMEDIATE , &oldramp);
  g_graphicsContext.Unlock();
}

void CUtil::SetBrightnessContrastGammaPercent(int iBrightNess, int iContrast, int iGamma, bool bImmediate)
{
  if (iBrightNess < 0) iBrightNess=0;
  if (iBrightNess >100) iBrightNess=100;
  if (iContrast < 0) iContrast=0;
  if (iContrast >100) iContrast=100;
  if (iGamma < 0) iGamma=0;
  if (iGamma >100) iGamma=100;

  float fBrightNess=(((float)iBrightNess)/50.0f) -1.0f;	// -1..1	Default: 0
  float fContrast=(((float)iContrast)/50.0f);						// 0..2		Default: 1
  float fGamma=(((float)iGamma)/40.0f)+0.5f;						// 0.5..3.0	Default: 1
  CUtil::SetBrightnessContrastGamma(fBrightNess, fContrast, fGamma, bImmediate);
}

#define clamp(x) (x) > 255.f ? 255 : ((x) < 0 ? 0 : (BYTE)(x+0.5f))
// Valid ranges:
//  brightness -1 -> 1 (0 is default)
//  contrast    0 -> 2 (1 is default)
//  gamma     0.5 -> 3.5 (1 is default)
//  default ramp is linear.
void CUtil::SetBrightnessContrastGamma(float Brightness, float Contrast, float Gamma, bool bImmediate)
{
	// calculate ramp
	D3DGAMMARAMP ramp;

	Gamma = 1.0f / Gamma;
	for (int i = 0; i < 256; ++i)
	{
		float f = (powf((float)i / 255.f, Gamma) * Contrast + Brightness)*255.f;
		ramp.blue[i] = ramp.green[i] = ramp.red[i] = clamp(f);
	}

	// set ramp next v sync
	g_graphicsContext.Lock();
	g_graphicsContext.Get3DDevice()->SetGammaRamp(bImmediate ? D3DSGR_IMMEDIATE : 0, &ramp);
	g_graphicsContext.Unlock();
}


// Tokenize ripped from http://www.linuxselfhelp.com/HOWTO/C++Programming-HOWTO-7.html
void CUtil::Tokenize(const CStdString& path, vector<CStdString>& tokens, const string& delimiters)
{
	string str = path;
	// Skip delimiters at beginning.
	string::size_type lastPos = str.find_first_not_of(delimiters, 0);
	// Find first "non-delimiter".
	string::size_type pos     = str.find_first_of(delimiters, lastPos);

	while (string::npos != pos || string::npos != lastPos)
	{
		// Found a token, add it to the vector.
		tokens.push_back(str.substr(lastPos, pos - lastPos));
		// Skip delimiters.  Note the "not_of"
		lastPos = str.find_first_not_of(delimiters, pos);
		// Find next "non-delimiter"
		pos = str.find_first_of(delimiters, lastPos);
	}
}


void CUtil::FlashScreen(bool bImmediate, bool bOn)
{
	static bool bInFlash = false;

	if (bInFlash == bOn)
		return;
	bInFlash = bOn;
	g_graphicsContext.Lock();
	if (bOn)
	{
		g_graphicsContext.Get3DDevice()->GetGammaRamp(&flashramp);
		SetBrightnessContrastGamma(0.5f, 1.2f, 2.0f, bImmediate);
	}
	else
		g_graphicsContext.Get3DDevice()->SetGammaRamp(bImmediate ? D3DSGR_IMMEDIATE : 0, &flashramp);
	g_graphicsContext.Unlock();
}

void CUtil::TakeScreenshot()
{
	LPDIRECT3DSURFACE8 lpSurface = NULL;
	char fn[1024];
	CStdString strDir = g_stSettings.m_szScreenshotsDirectory;

	if (strlen(g_stSettings.m_szScreenshotsDirectory))
	{
		sprintf(fn, "%s\\screenshot%%03d.bmp", strDir.c_str());
		strcpy(fn, CUtil::GetNextFilename(fn, 999).c_str());

		if (strlen(fn))
		{
			g_graphicsContext.Lock();
			if (g_application.IsPlayingVideo())
			{
				g_renderManager.SetupScreenshot();
			}
			g_graphicsContext.Get3DDevice()->BlockUntilVerticalBlank();
			if (SUCCEEDED(g_graphicsContext.Get3DDevice()->GetBackBuffer(-1, D3DBACKBUFFER_TYPE_MONO, &lpSurface)))
			{
				if (FAILED(XGWriteSurfaceToFile(lpSurface, fn)))
				{
					CLog::Log(LOGERROR, "Failed to Generate Screenshot");
				}
				else
				{
					CLog::Log(LOGINFO, "Screen shot saved as %s", fn);
				}
				lpSurface->Release();
			}
			g_graphicsContext.Unlock();
			g_graphicsContext.Get3DDevice()->BlockUntilVerticalBlank();
			FlashScreen(true, true);
			Sleep(10);
			g_graphicsContext.Get3DDevice()->BlockUntilVerticalBlank();
			FlashScreen(true, false);
		}
		else
		{
			CLog::Log(LOGWARNING, "Too many screen shots or invalid folder");
		}
	}
 }

void CUtil::ClearCache()
{
  CStdString strThumb=g_stSettings.m_szAlbumDirectory;
  strThumb+="\\thumbs";
  g_directoryCache.ClearDirectory(strThumb);
  g_directoryCache.ClearDirectory(strThumb+"\\temp");

  strThumb=g_stSettings.szThumbnailsDirectory;
  g_directoryCache.ClearDirectory(strThumb);
  g_directoryCache.ClearDirectory(strThumb+"\\imdb");
}

void CUtil::SortFileItemsByName(VECFILEITEMS& items, bool bSortAscending/*=true*/)
{
	SSortByLabel sortmethod;
	sortmethod.m_bSortAscending=bSortAscending;
	sort(items.begin(), items.end(), sortmethod);
}

void CUtil::Stat64ToStatI64(struct _stati64 *result, struct __stat64 *stat)
{
	result->st_dev = stat->st_dev;
	result->st_ino = stat->st_ino;
	result->st_mode = stat->st_mode;
	result->st_nlink = stat->st_nlink;
	result->st_uid = stat->st_uid;
	result->st_gid = stat->st_gid;
	result->st_rdev = stat->st_rdev;
	result->st_size = stat->st_size;
	result->st_atime = (long)stat->st_atime;
	result->st_mtime = (long)stat->st_mtime;
	result->st_ctime = (long)stat->st_ctime;
}

void CUtil::StatI64ToStat64(struct __stat64 *result, struct _stati64 *stat)
{
	result->st_dev = stat->st_dev;
	result->st_ino = stat->st_ino;
	result->st_mode = stat->st_mode;
	result->st_nlink = stat->st_nlink;
	result->st_uid = stat->st_uid;
	result->st_gid = stat->st_gid;
	result->st_rdev = stat->st_rdev;
	result->st_size = stat->st_size;
	result->st_atime = stat->st_atime;
	result->st_mtime = stat->st_mtime;
	result->st_ctime = stat->st_ctime;
}

void CUtil::Stat64ToStat(struct _stat *result, struct __stat64 *stat)
{
	result->st_dev = stat->st_dev;
	result->st_ino = stat->st_ino;
	result->st_mode = stat->st_mode;
	result->st_nlink = stat->st_nlink;
	result->st_uid = stat->st_uid;
	result->st_gid = stat->st_gid;
	result->st_rdev = stat->st_rdev;
	result->st_size = (_off_t)stat->st_size;
	result->st_atime = (time_t)stat->st_atime;
	result->st_mtime = (time_t)stat->st_mtime;
	result->st_ctime = (time_t)stat->st_ctime;
}

// around 50% faster than memcpy
// only worthwhile if the destination buffer is not likely to be read back immediately
// and the number of bytes copied is >16
// somewhat faster if the source and destination are a multiple of 16 bytes apart
void fast_memcpy(void* d, const void* s, unsigned n)
{
	__asm {
		mov edx,n
		mov esi,s
		prefetchnta [esi]
		prefetchnta [esi+32]
		mov edi,d

		// pre align
		mov eax,edi
		mov ecx,16
		and eax,15
		sub ecx,eax
		and ecx,15
		cmp edx,ecx
		jb fmc_exit_main
		sub edx,ecx

		test ecx,ecx
fmc_start_pre:
		jz fmc_exit_pre

		mov al,[esi]
		mov [edi],al

		inc esi
		inc edi
		dec ecx
		jmp fmc_start_pre

fmc_exit_pre:
		mov eax,esi
		and eax,15
		jnz fmc_notaligned

		// main copy, aligned
		mov ecx,edx
		shr ecx,4
fmc_start_main_a:
		jz fmc_exit_main

		prefetchnta [esi+32]
		movaps xmm0,[esi]
		movntps [edi],xmm0

		add esi,16
		add edi,16
		dec ecx
		jmp fmc_start_main_a

fmc_notaligned:
		// main copy, unaligned
		mov ecx,edx
		shr ecx,4
fmc_start_main_u:
		jz fmc_exit_main

		prefetchnta [esi+32]
		movups xmm0,[esi]
		movntps [edi],xmm0

		add esi,16
		add edi,16
		dec ecx
		jmp fmc_start_main_u

fmc_exit_main:

		// post align
		mov ecx,edx
		and ecx,15
fmc_start_post:
		jz fmc_exit_post

		mov al,[esi]
		mov [edi],al

		inc esi
		inc edi
		dec ecx
		jmp fmc_start_post

fmc_exit_post:
	}
}

void fast_memset(void* d, int c, unsigned n)
{
	char __declspec(align(16)) buf[16];

	__asm {
		mov edx,n
		mov edi,d

		// pre align
		mov eax,edi
		mov ecx,16
		and eax,15
		sub ecx,eax
		and ecx,15
		cmp edx,ecx
		jb fms_exit_main
		sub edx,ecx
		mov eax,c

		test ecx,ecx
fms_start_pre:
		jz fms_exit_pre

		mov [edi],al

		inc edi
		dec ecx
		jmp fms_start_pre

fms_exit_pre:
		test al,al
		jz fms_initzero

		// duplicate the value 16 times
		lea esi,buf
		mov [esi],al
		mov [esi+1],al
		mov [esi+2],al
		mov [esi+3],al
		mov eax,[esi]
		mov [esi+4],eax
		mov [esi+8],eax
		mov [esi+12],eax
		movaps xmm0,[esi]
		jmp fms_init_loop

fms_initzero:
		// optimzed set zero
		xorps xmm0,xmm0

fms_init_loop:
		mov ecx,edx
		shr ecx,4

fms_start_main:
		jz fms_exit_main

		movntps [edi],xmm0

		add edi,16
		dec ecx
		jmp fms_start_main

fms_exit_main:

		// post align
		mov ecx,edx
		and ecx,15
fms_start_post:
		jz fms_exit_post

		mov [edi],al

		inc edi
		dec ecx
		jmp fms_start_post

fms_exit_post:
	}
}

// Function to create all directories at once instead
// of calling CreateDirectory for every subdir.
// Creates the directory and subdirectories if needed.
bool CUtil::CreateDirectoryEx(const CStdString& strPath)
{
	std::vector<string> strArray;

  CURL url(strPath);
	string path = url.GetFileName().c_str();
	int iSize = path.size();
	char cSep = CUtil::GetDirectorySeperator(strPath);
	if (path.at(iSize - 1) == cSep) path.erase(iSize - 1, iSize - 1); // remove slash at end
	CStdString strTemp;

	// return true if directory already exist
	if (CDirectory::Exists(strPath)) return true;

	/* split strPath up into an array
	 * music\\album\\ will result in
	 * music
	 * music\\album
	 */
	int i, s = 0;
	if (path.at(1) == ':') i = 2; // to skip 'e:'
	s = i;
	while(i < iSize)
	{
		i = path.find(cSep, i + 1);
		if (i < 0) i = iSize; // get remaining chars
		strArray.push_back(path.substr(s, i - s));
	}

	// create the directories
	url.GetURLWithoutFilename(strTemp);
	for(unsigned int i = 0; i < strArray.size(); i++)
	{
		CDirectory::Create(strTemp + strArray[i].c_str());
	}

  strArray.clear();

	// is the directory successfully created ?
  if (!CDirectory::Exists(strPath)) return false;
	return true;
}

// check if the filename is a legal FATX one.
// this means illegal chars will be removed from the string,
// and the remaining string is stripped back to 42 chars if needed
CStdString CUtil::MakeLegalFileName(const char* strFile, bool bKeepExtension, bool isFATX)
{
  if (NULL == strFile) return "";
  char cIllegalChars[] = "<>=?:;\"*+,/\\|";
  unsigned int iIllegalCharSize = strlen(cIllegalChars);
  bool isIllegalChar;
	unsigned int iSize = strlen(strFile);
	unsigned int iNewStringSize = 0;
	char* strNewString = new char[iSize + 1];

  // only copy the legal characters to the new filename
  for (unsigned int i = 0; i < iSize; i++)
  {
	  isIllegalChar = false;
	  // check for illigal chars
	  for (unsigned j = 0; j < iIllegalCharSize; j++)
		  if (strFile[i] == cIllegalChars[j]) isIllegalChar = true;
	  // FATX only allows chars from 32 till 127
	  if (isIllegalChar == false &&
			  strFile[i] > 31 && strFile[i] < 127) strNewString[iNewStringSize++] = strFile[i];
  }
  strNewString[iNewStringSize] = '\0';

  if (isFATX)
  {
    // since we can only write to samba shares and hd, we assume this has to be a fatx filename
    // thus we have to strip it down to 42 chars (samba doesn't have this limitation)

	  // no need to keep the extension, just strip it down to 42 characters
	  if (iNewStringSize > 42 && bKeepExtension == false) strNewString[42] = '\0';

	  // we want to keep the extension
	  else if (iNewStringSize > 42 && bKeepExtension == true)
	  {
		  char strExtension[42];
		  unsigned int iExtensionLenght = iNewStringSize - (strrchr(strNewString, '.') - strNewString);
		  strcpy(strExtension, (strNewString + iNewStringSize - iExtensionLenght));

		  strcpy(strNewString + (42 - iExtensionLenght), strExtension);
	  }
	}

	CStdString result(strNewString);
  delete[] strNewString;
	return result;
}

void CUtil::AddDirectorySeperator(CStdString& strPath)
{
  CURL url(strPath);
  if (url.GetProtocol().size()>0) strPath += "/";
  else strPath += "\\";
}

char CUtil::GetDirectorySeperator(const CStdString& strPath)
{
  CURL url(strPath);
  if (url.GetProtocol().size()>0) return '/';
  return '\\';
}

void CUtil::ConvertFileItemToPlayListItem(const CFileItem *pItem, CPlayList::CPlayListItem &playlistitem)
{
	playlistitem.SetDescription(pItem->GetLabel());
	playlistitem.SetFileName(pItem->m_strPath);
	playlistitem.SetDuration(pItem->m_musicInfoTag.GetDuration());
	playlistitem.SetStartOffset(pItem->m_lStartOffset);
	playlistitem.SetEndOffset(pItem->m_lEndOffset);
	playlistitem.SetMusicTag(pItem->m_musicInfoTag);
}


bool CUtil::IsNaturalNumber(const CStdString& str)
{
	return IsNaturalNumber((CStdStringW)str);
}

bool CUtil::IsNaturalNumber(const CStdStringW& str)
{
  if (0 == (int)str.size())
    return false;
	for (int i = 0; i < (int)str.size(); i++)
	{
		if ((str[i] < '0') || (str[i] > '9')) return false;
	}
	return true;
}

bool CUtil::IsUsingTTFSubtitles()
{
  char* ext = strrchr(g_guiSettings.GetString("Subtitles.Font").c_str(), '.');
	if (ext && stricmp(ext, ".ttf") == 0)
		return true;
	else
		return false;
}

bool CUtil::IsBuiltIn(const CStdString& execString)
{
	if (strncmp(execString.c_str(), "XBMC.", 5) == 0)
		return true;

	return false;
}

void CUtil::SplitExecFunction(const CStdString &execString, CStdString &strFunction, CStdString &strParam)
{
	strParam = "";
	strFunction = execString.Mid(5);
	int iPos = strFunction.Find("(");
	int iPos2 = strFunction.Find(")", iPos);
	if (iPos > 0 && iPos2 > 0)
	{	// got a parameter
		strParam = strFunction.Mid(iPos+1, iPos2-iPos-1);
		strFunction = strFunction.Left(iPos).c_str();
	}
}

void CUtil::ExecBuiltIn(const CStdString& execString)
{
	// Get the text after the "XBMC."
	CStdString execute, parameter;
	SplitExecFunction(execString, execute, parameter);

	if ((execute == "Reboot") || (execute == "Restart"))  //Will reboot the xbox, aka cold reboot
	{
		g_applicationMessenger.Restart();
	}
	else if (execute == "ShutDown")
	{
		g_applicationMessenger.Shutdown();
	}
	else if (execute == "Dashboard")
	{
		RunXBE(g_stSettings.szDashboard);
	}
  else if (execute == "RestartApp")
	{
		g_applicationMessenger.RestartApp();
	}
	else if (execute == "Credits")
	{
		RunCredits();
	}
	else if (execute == "Reset") //Will reset the xbox, aka soft reset
	{
		g_applicationMessenger.Reset();
	}
	else if (execute == "ActivateWindow")
	{	// get the parameter
		int iWindow = WINDOW_HOME + atoi(parameter.c_str());
		// check what type of window we have (it could be a dialog)
		CGUIWindow *pWindow = m_gWindowManager.GetWindow(iWindow);
		if (pWindow && pWindow->IsDialog())
		{	// Dialog
			CGUIDialog *pDialog = (CGUIDialog *)pWindow;
			if (!pDialog->IsRunning())
				pDialog->DoModal(m_gWindowManager.GetActiveWindow());
			return;
		}
		m_gWindowManager.ActivateWindow(iWindow);
	}
	else if (execute == "RunScript")
	{
		g_pythonParser.evalFile(parameter.c_str());
	}
	else if (execute == "RunXBE")
	{
		CUtil::RunXBE(parameter.c_str());
	}
}

void usleep(int t)
{
  LARGE_INTEGER li;
  
  li.QuadPart = (LONGLONG)t * -10;

  // Where possible, Alertable should be set to FALSE and WaitMode should be set to KernelMode,
  // in order to reduce driver complexity. The principal exception to this is when the wait is a long term wait.
  KeDelayExecutionThread(KernelMode, false, &li);
}
