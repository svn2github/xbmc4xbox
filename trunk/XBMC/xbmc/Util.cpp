#include "stdafx.h"
#include "application.h"
#include "util.h"
#include "xbox/iosupport.h"
#include "xbox/xbeheader.h"
#include "xbox/undocumented.h"
#include "xbresource.h"
#include "DetectDVDType.h"
#include "ThumbnailCache.h"
#include "filesystem/hddirectory.h"
#include "filesystem/DirectoryCache.h"
#include "Credits.h"
#include "shortcut.h"
#include "playlistplayer.h"
#include "lib/libPython/XBPython.h"
#include "utils/RegExp.h"
#include "utils/AlarmClock.h" 
#include "cores/VideoRenderers/RenderManager.h"
#include "ButtonTranslator.h"
#include "Picture.h"
#include "GUIDialogNumeric.h"
#include "autorun.h"
#include "utils/fstrcmp.h"
#include "utils/GUIInfoManager.h"
#include "utils/trainer.h"
#include "FileSystem/ZipManager.h"
#include "FileSystem/RarManager.h"
#include "FileSystem/StackDirectory.h"
#include "FileSystem/VirtualPathDirectory.h"
#include "GUIDialogFileBrowser.h"
#include "MediaManager.h"
#include <xbdm.h>
#include "xbox/network.h"
#include "utils/KaiClient.h"
#include "GUIPassword.h"
#include "FileSystem/UPnPDirectory.h"
#include "lib/libfilezilla/xbfilezilla.h"
#include "lib/libscrobbler/scrobbler.h"
#include "partymodemanager.h"
#include "filesystem/MultiPathDirectory.h"
#include "utils/LED.h"
#include "MusicInfoLoader.h"
#include "utils/FilterFlickerPatch.h"


#define clamp(x) (x) > 255.f ? 255 : ((x) < 0 ? 0 : (BYTE)(x+0.5f)) // Valid ranges: brightness[-1 -> 1 (0 is default)] contrast[0 -> 2 (1 is default)]  gamma[0.5 -> 3.5 (1 is default)] default[ramp is linear]
static const __int64 SECS_BETWEEN_EPOCHS = 11644473600;
static const __int64 SECS_TO_100NS = 10000000;

HANDLE CUtil::m_hCurrentCpuUsage = NULL;

CStdString strHasClientIP="",strHasClientInfo="",strNewClientIP,strNewClientInfo; 
using namespace AUTOPTR;
using namespace MEDIA_DETECT;
using namespace XFILE;
using namespace PLAYLIST;
static D3DGAMMARAMP oldramp, flashramp;


extern "C"
{
	extern bool WINAPI NtSetSystemTime(LPFILETIME SystemTime , LPFILETIME PreviousTime );
};

CUtil::CUtil(void)
{
}

CUtil::~CUtil(void)
{}


/* returns filename extension including period of filename */
const CStdString CUtil::GetExtension(const CStdString& strFileName)
{   
  int period = strFileName.find_last_of('.');
  if(period >= 0)
  {
    if( strFileName.find_first_of('/', period+1) != -1 ) return "";
    if( strFileName.find_first_of('\\', period+1) != -1 ) return "";

    /* url options could be at the end of a url */
    const int options = strFileName.find_first_of('?', period+1);

    if(options >= 0)
      return strFileName.substr(period, options-period);
    else
      return strFileName.substr(period);
  }
  else
    return "";
}

void CUtil::GetExtension(const CStdString& strFile, CStdString& strExtension)
{
  strExtension = GetExtension(strFile);
}

/* returns a filename given an url */
/* handles both / and \, and options in urls*/
const CStdString CUtil::GetFileName(const CStdString& strFileNameAndPath)
{
  /* find any slashes */
  const int slash1 = strFileNameAndPath.find_last_of('/');
  const int slash2 = strFileNameAndPath.find_last_of('\\');

  /* select the last one */
  int slash;
  if(slash2>slash1)
    slash = slash2;
  else
    slash = slash1;

  /* check if there is any options in the url */
  const int options = strFileNameAndPath.find_first_of('?', slash+1);
  if(options < 0)
    return strFileNameAndPath.substr(slash+1);
  else
    return strFileNameAndPath.substr(slash+1, options-(slash+1));
}

CStdString CUtil::GetTitleFromPath(const CStdString& strFileNameAndPath, bool bIsFolder /* = false */)
{
  // use above to get the filename
  CStdString strFilename = GetFileName(strFileNameAndPath);

  // if upnp:// we can ask for the friendlyname
  if (strFileNameAndPath.Left(7).Compare("upnp://") == 0) {
      strFilename = CUPnPDirectory::GetFriendlyName(strFileNameAndPath.c_str());
  }

  // now remove the extension if needed
  if (g_guiSettings.GetBool("filelists.hideextensions") && !bIsFolder)
  {
    RemoveExtension(strFilename);
    return strFilename;
  }
  return strFilename;
}

bool CUtil::GetVolumeFromFileName(const CStdString& strFileName, CStdString& strFileTitle, CStdString& strVolumeNumber)
{
  const CStdStringArray &regexps = g_advancedSettings.m_videoStackRegExps;

  CStdString strFileNameTemp = strFileName;
  CStdString strFileNameLower = strFileName;
  strFileNameLower.MakeLower();

  CStdString strVolume;
  CStdString strTestString;
  CRegExp reg;

  //CLog::Log(LOGNOTICE, "GetVolumeFromFileName : 1 : " + strFileName);

  for (unsigned int i = 0; i < regexps.size(); i++)
  {
    CStdString strRegExp = regexps[i];
    if (!reg.RegComp(strRegExp.c_str()))
    { // invalid regexp - complain in logs
      CLog::Log(LOGERROR, "Invalid RegExp: %s.", regexps[i].c_str());
      continue;
    }
    int iFoundToken = reg.RegFind(strFileNameLower.c_str());
    if (iFoundToken >= 0)
    { // found this token
      int iRegLength = reg.GetFindLen();
      int iCount = reg.GetSubCount();
      //CLog::Log(LOGNOTICE, "GetVolumeFromFileName : 2 : " + strFileName + " : " + strRegExp + " : iRegLength=%i : iCount=%i", iRegLength, iCount);
      if( 1 == iCount )
      {
        char *pReplace = reg.GetReplaceString("\\1");

        if (pReplace)
        {
          strVolumeNumber = pReplace;
          free(pReplace);

          // remove the extension (if any).  We do this on the base filename, as the regexp
          // match may include some of the extension (eg the "." in particular).

          // the extension will then be added back on at the end - there is no reason 
          // to clean it off here. It will be cleaned off during the display routine, if 
          // the settings to hide extensions are turned on.
          CStdString strFileNoExt = strFileNameTemp;
          RemoveExtension(strFileNoExt);
          CStdString strFileExt = strFileNameTemp.Right(strFileNameTemp.length() - strFileNoExt.length());
          CStdString strFileRight = strFileNoExt.Mid(iFoundToken + iRegLength);
          strFileTitle = strFileName.Left(iFoundToken) + strFileRight + strFileExt;
          //CLog::Log(LOGNOTICE, "GetVolumeFromFileName : 3 : " + strFileName + " : " + strVolumeNumber + " : " + strFileTitle + " : " + strFileExt + " : " + strFileRight + " : " + strFileTitle);
          return true;
        }

      }
      else if( iCount > 1 )
      {        
        //Second Sub value contains the stacking
        strVolumeNumber = strFileName.Mid(iFoundToken + reg.GetSubStart(2), reg.GetSubLenght(2));

        strFileTitle = strFileName.Left(iFoundToken);

        //First Sub value contains prefix
        strFileTitle += strFileName.Mid(iFoundToken + reg.GetSubStart(1), reg.GetSubLenght(1));

        //Third Sub value contains suffix
        strFileTitle += strFileName.Mid(iFoundToken + reg.GetSubStart(3), reg.GetSubLenght(3));
        strFileTitle += strFileNameTemp.Mid(iFoundToken + iRegLength);
        //CLog::Log(LOGNOTICE, "GetVolumeFromFileName : 4 : " + strFileName + " : " + strVolumeNumber + " : " + strFileTitle);
        return true;
      }

    }
  }
  //CLog::Log(LOGNOTICE, "GetVolumeFromFileName : 5 : " + strFileName);
  return false;
}

void CUtil::RemoveExtension(CStdString& strFileName)
{
  int iPos = strFileName.ReverseFind(".");
  // Extension found
  if (iPos > 0)
  {
    CStdString strExtension;
    CUtil::GetExtension(strFileName, strExtension);
    strExtension.ToLower();

    CStdString strFileMask;
    strFileMask = g_stSettings.m_pictureExtensions;
    strFileMask += g_stSettings.m_musicExtensions;
    strFileMask += g_stSettings.m_videoExtensions;
    strFileMask += ".py|.xml|.milk|.xpr|.cdg";

    // Only remove if its a valid media extension
    if (strFileMask.Find(strExtension.c_str()) >= 0)
      strFileName = strFileName.Left(iPos);
  }
}

void CUtil::CleanFileName(CStdString& strFileName)
{
  bool result = false;

  if (g_guiSettings.GetBool("filelists.hideextensions"))
  {
    RemoveExtension(strFileName);
  }

  //CLog::Log(LOGNOTICE, "CleanFileName : 3 : " + strFileName);

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
    //m_szMyVideoCleanSeparatorsString = " -_.[]()+";

    //m_szMyVideoCleanTokensArray = "divx|xvid|3ivx|ac3|ac351|mp3|wma|m4a|mp4|ogg|scr|ts|sharereactor";

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
        if (pos >= maxPos && pos > 0)
        {
          tokenFound = tokenFound | true;
          char separator = strFileName.GetAt(pos - 1);
          char buffer[10];
          itoa(pos, buffer, 10);
          char buffer2[10];
          itoa(maxPos, buffer2, 10);
          //CLog::Log(LOGNOTICE, "CleanFileName : 4 : " + strFileName + " : " + token + " : " + buffer + " : " + separator + " : " + buffer2 + " : " + separatorsString);
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
              //CLog::Log(LOGNOTICE, "CleanFileName : 5 : " + strFileName + " : " + token + " : " + buffer + " : " + separator + " : " + buffer2);
            //else
              //CLog::Log(LOGNOTICE, "CleanFileName : 6 : " + strFileName + " : " + token + " : " + buffer + " : " + separator + " : " + buffer2);
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
  // if the extension is shown, the '.' before the extension should be 
  // left as is.

  strFileName = strFileName.Trim();
  //CLog::Log(LOGNOTICE, "CleanFileName : 7 : " + strFileName);

  int extPos = (int)strFileName.size();
  if (!g_guiSettings.GetBool("filelists.hideextensions"))
  {
    CStdString strFileNameTemp = strFileName;
    RemoveExtension(strFileNameTemp);
    //CLog::Log(LOGNOTICE, "CleanFileName : 8 : " + strFileName + " : " + strFileNameTemp);
    extPos = strFileNameTemp.size();
  }

  {
    bool alreadyContainsSpace = (strFileName.Find(' ') >= 0);

    for (int i = 0; i < extPos; i++)
    {
      char c = strFileName.GetAt(i);
      if ((c == '_') || ((!alreadyContainsSpace) && (c == '.')))
      {
        strFileName.SetAt(i, ' ');
      }
    }
  }

  strFileName = strFileName.Trim();
  //CLog::Log(LOGNOTICE, "CleanFileName : 9 : " + strFileName);
}

bool CUtil::GetParentPath(const CStdString& strPath, CStdString& strParent)
{
  strParent = "";

  CURL url(strPath);
  CStdString strFile = url.GetFileName();
  if ( ((url.GetProtocol() == "rar") || (url.GetProtocol() == "zip")) && strFile.IsEmpty())
  { 
    strFile = url.GetHostName();
    return GetParentPath(strFile, strParent);
  }
  else if (strFile.size() == 0)
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
    strFile = strFile.Left(strFile.size() - 1);
  }

  int iPos = strFile.ReverseFind('/');
  if (iPos < 0)
  {
    iPos = strFile.ReverseFind('\\');
  }
  if (iPos < 0)
  {
    url.SetFileName("");
    url.GetURL(strParent);
    return true;
  }

  strFile = strFile.Left(iPos);
  url.SetFileName(strFile);
  url.GetURL(strParent);
  return true;
}


void CUtil::GetQualifiedFilename(const CStdString &strBasePath, CStdString &strFilename)
{
  //Make sure you have a full path in the filename, otherwise adds the base path before.
  CURL plItemUrl(strFilename);
  CURL plBaseUrl(strBasePath);
  int iDotDotLoc, iBeginCut, iEndCut;

  if (plBaseUrl.IsLocal()) //Base in local directory
  {
    if (plItemUrl.IsLocal() ) //Filename is local or not qualified
    {
      if (!( strFilename.c_str()[1] == ':')) //Filename not fully qualified
      {
        if (strFilename.c_str()[0] == '/' || strFilename.c_str()[0] == '\\' || HasSlashAtEnd(strBasePath))
        {
          strFilename = strBasePath + strFilename;
          strFilename.Replace('/', '\\');
        }
        else
        {
          strFilename = strBasePath + '\\' + strFilename;
          strFilename.Replace('/', '\\');
        }
      }
    }
    strFilename.Replace("\\.\\", "\\");
    while ((iDotDotLoc = strFilename.Find("\\..\\")) > 0)
    {
      iEndCut = iDotDotLoc + 4;
      iBeginCut = strFilename.Left(iDotDotLoc).ReverseFind('\\') + 1;
      strFilename.Delete(iBeginCut, iEndCut - iBeginCut);
    }

    // This routine is only called from the playlist loaders,
    // where the filepath is in UTF-8 anyway, so we don't need
    // to do checking for FatX characters.
    //if (g_guiSettings.GetBool("servers.ftpautofatx") && (CUtil::IsHD(strFilename)))
    //  CUtil::GetFatXQualifiedPath(strFilename);
  }
  else //Base is remote
  {
    if (plItemUrl.IsLocal()) //Filename is local
    {
      if (strFilename[1] == ':') // already fully qualified
        return;
      if (strFilename.c_str()[0] == '/' || strFilename.c_str()[0] == '\\' || HasSlashAtEnd(strBasePath)) //Begins with a slash.. not good.. but we try to make the best of it..

      {
        strFilename = strBasePath + strFilename;
        strFilename.Replace('\\', '/');
      }
      else
      {
        strFilename = strBasePath + '/' + strFilename;
        strFilename.Replace('\\', '/');
      }
    }
    strFilename.Replace("/./", "/");
    while ((iDotDotLoc = strFilename.Find("/../")) > 0)
    {
      iEndCut = iDotDotLoc + 4;
      iBeginCut = strFilename.Left(iDotDotLoc).ReverseFind('/') + 1;
      strFilename.Delete(iBeginCut, iEndCut - iBeginCut);
    }
  }
}

bool CUtil::PatchCountryVideo(F_COUNTRY Country, F_VIDEO Video)
{
  BYTE	*Kernel=(BYTE *)0x80010000;
  DWORD	i, j = 0, k;
  DWORD	*CountryPtr;
  BYTE	CountryValues[4]={0, 1, 2, 4};
  BYTE	VideoTyValues[5]={0, 1, 2, 3, 3};
  BYTE	VideoFrValues[5]={0x00, 0x40, 0x40, 0x80, 0x40};

  switch (Country) 
  {
	  case COUNTRY_EUR:
		  if (!Video)
		    Video = VIDEO_PAL50;
		  break;
	  case COUNTRY_USA:
	    Video = VIDEO_NTSCM;
	  Country = COUNTRY_USA;
		  break;
	  case COUNTRY_JAP:
	    Video = VIDEO_NTSCJ;
	  Country = COUNTRY_JAP;
		  break;
	  default:
	  Country = COUNTRY_EUR;
	    Video = VIDEO_PAL50;
  };

  // Search for the original code in the Kernel.
  // Searching from 0x80011000 to 0x80024000 in order that this will work on as many Kernels
  // as possible.

  for(i=0x1000; i<0x14000; i++)
  {
    if(Kernel[i]!=OriginalData[0])	
	    continue;

    for(j=0; j<57; j++)
    {
	    if(Kernel[i+j]!=OriginalData[j])	
		    break;
    }
    if(j==57)	
	    break;
  }

  if(j==57)
  {
    // Ok, found the code to patch. Get pointer to original Country setting.
    // This may not be strictly neccessary, but lets do it anyway for completeness.

    j=(Kernel[i+57])+(Kernel[i+58]<<8)+(Kernel[i+59]<<16)+(Kernel[i+60]<<24);
    CountryPtr=(DWORD *)j;
  }
  else
  {
    // Did not find code in the Kernel. Check if my patch is already there.

    for(i=0x1000; i<0x14000; i++)
    {
	    if(Kernel[i]!=PatchData[0])	
		    continue;

	    for(j=0; j<25; j++)
	    {
		    if(Kernel[i+j]!=PatchData[j])	
			    break;
	    }
	    if(j==25)	
		    break;
    }

    if(j==25)
    {
	    // Ok, found my patch. Get pointer to original Country setting.
	    // This may not be strictly neccessary, but lets do it anyway for completeness.

	    j=(Kernel[i+66])+(Kernel[i+67]<<8)+(Kernel[i+68]<<16)+(Kernel[i+69]<<24);
	    CountryPtr=(DWORD *)j;
    }
    else
    {
	    // Did not find my patch - so I can't work with this BIOS. Exit.
	    return( false );
    }
  }

  // Patch in new code.

  j=MmQueryAddressProtect(&Kernel[i]);
  MmSetAddressProtect(&Kernel[i], 70, PAGE_READWRITE);

  memcpy(&Kernel[i], &PatchData[0], 70);

  // Patch Success. Fix up values.

  *CountryPtr=(DWORD)CountryValues[Country];
  Kernel[i+0x1f]=CountryValues[Country];
  Kernel[i+0x19]=VideoTyValues[Video];
  Kernel[i+0x1a]=VideoFrValues[Video];

  k=(DWORD)CountryPtr;
  Kernel[i+66]=(BYTE)(k&0xff);
  Kernel[i+67]=(BYTE)((k>>8)&0xff);
  Kernel[i+68]=(BYTE)((k>>16)&0xff);
  Kernel[i+69]=(BYTE)((k>>24)&0xff);

  MmSetAddressProtect(&Kernel[i], 70, j);

  // All Done!
  return( true );
} 

bool CUtil::InstallTrainer(CTrainer& trainer)
{
	unsigned char *xboxkrnl = (unsigned char *)KERNEL_START_ADDRESS;
	unsigned char *hackptr = (unsigned char *)KERNEL_STORE_ADDRESS;
  void *ourmemaddr = NULL; // pointer used to allocated trainer mem
	unsigned int i = 0;
	bool Found = false;
	DWORD memsize;

  CLog::Log(LOGDEBUG,"installing trainer %s",trainer.GetPath().c_str());
	
  if (trainer.IsXBTF()) // size of our allocation buffer for trainer
		memsize = XBTF_HEAP_SIZE;
	else
		memsize = ETM_HEAP_SIZE;

	unsigned char xbe_entry_point[] = {0xff,0x15,0x28,0x01,0x01,0x00}; // xbe entry point bytes in kernel
	unsigned char evox_tsr_hook[] = {0xff,0x15,0x10,0x00,0x00,0x80}; // check for evox's evil tsr hook

	for(i = 0; i < KERNEL_SEARCH_RANGE; i++)
	{
		if (memcmp(&xboxkrnl[i], xbe_entry_point, sizeof(xbe_entry_point)) == 0 ||
			memcmp(&xboxkrnl[i], evox_tsr_hook, sizeof(evox_tsr_hook)) == 0)
		{
			Found = true;
			break;
		}
	}

	if(Found)
	{
		unsigned char *patchlocation = xboxkrnl;

		patchlocation += i + 2; // adjust to xbe entry point bytes in kernel (skipping actual call opcodes)
		_asm
		{
			pushad

			mov eax, cr0
			push eax
			and eax, 0FFFEFFFFh
			mov cr0, eax // disable memory write prot

			mov	edi, patchlocation // address of call to xbe entry point in kernel
			mov	dword ptr [edi], KERNEL_STORE_ADDRESS // patch with address of where we store loaderdata+trainer buffer address

			pop eax
			mov cr0, eax // restore memory write prot

			popad
		}
	}
	else
	{
		__asm // recycle check
		{
			pushad

			mov edx, KERNEL_STORE_ADDRESS
			mov ecx, DWORD ptr [edx]
			cmp ecx, 0 // just in case :)
			jz cleanup

			cmp word ptr [ecx], 0BA60h // address point to valid loaderdata?
			jnz cleanup

			mov Found, 1 // yes! flag it found

			push ecx
			call MmFreeContiguousMemory // release old memory
cleanup:
			popad
		}
	}

	// allocate our memory space BELOW the kernel (so we can access buffer from game's scope)
	// if you allocate above kernel our buffer is out of scope and only debug bio will allow
	// game to access it
	ourmemaddr = MmAllocateContiguousMemoryEx(memsize, 0, -1, KERNEL_ALLOCATE_ADDRESS, PAGE_NOCACHE | PAGE_READWRITE);
	if ((DWORD)ourmemaddr > 0)
	{
		MmPersistContiguousMemory(ourmemaddr, memsize, true); // so we survive soft boots
		memcpy(hackptr, &ourmemaddr, 4); // store location of ourmemaddr in kernel

		memset(ourmemaddr, 0xFF, memsize); // init trainer buffer
		memcpy(ourmemaddr, trainerloaderdata, sizeof(trainerloaderdata)); // copy loader data (actual kernel hack)

		// patch loaderdata with trainer base address
		_asm
		{
			pushad

			mov eax, ourmemaddr
			mov ebx, eax
			add ebx, SIZEOFLOADERDATA
			mov dword ptr [eax+2], ebx

			popad
		}

		// adjust ourmemaddr pointer past loaderdata
		ourmemaddr=(PVOID *)(((unsigned int) ourmemaddr) + sizeof(trainerloaderdata));

		// copy our trainer data into allocated mem
		memcpy(ourmemaddr, trainer.data(), trainer.Size());

    if (trainer.IsXBTF())
    {
      DWORD dwSection = 0;

      // get address of XBTF_Section
      _asm
      {
        pushad

        mov eax, ourmemaddr

        cmp dword ptr [eax+0x1A], 0 // real xbtf or just a converted etm? - XBTF_ENTRYPOINT
        je converted_etm

        push eax
        mov ebx, 0x16
        add eax, ebx
        mov ecx, DWORD PTR [eax]
        pop	eax
        add eax, ecx
        mov dwSection, eax // get address of xbtf_section
      
      converted_etm:
        popad
      }

      if (dwSection == 0)
        return Found; // its a converted etm so we do not have toys section :)

      // adjust past trainer
      ourmemaddr=(PVOID *)(((unsigned int) ourmemaddr) + trainer.Size());

      // inject SMBus code
      memcpy(ourmemaddr, sm_bus, sizeof(sm_bus));
      _asm
      {
        pushad

        mov eax, dwSection
        mov ebx, ourmemaddr
        cmp dword ptr [eax], 0
        jne nosmbus
        mov DWORD PTR [eax], ebx
      nosmbus:
        popad
      }
      // adjust past SMBus
      ourmemaddr=(PVOID *)(((unsigned int) ourmemaddr) + sizeof(sm_bus));

      // PatchIt
      memcpy(ourmemaddr, patch_it_toy, sizeof(patch_it_toy));
      _asm
      {
        pushad

        mov eax, dwSection
        add eax, 4 // 2nd dword in XBTF_Section
        mov ebx, ourmemaddr
        cmp dword PTR [eax], 0
        jne nopatchit
        mov DWORD PTR [eax], ebx
      nopatchit:
        popad
      }

      // adjust past PatchIt
      ourmemaddr=(PVOID *)(((unsigned int) ourmemaddr) + sizeof(patch_it_toy));

      // HookIt
      memcpy(ourmemaddr, hookit_toy, sizeof(hookit_toy));
      _asm
      {
        pushad

        mov eax, dwSection
        add eax, 8 // 3rd dword in XBTF_Section
        mov ebx, ourmemaddr
        cmp dword PTR [eax], 0
        jne nohookit
        mov DWORD PTR [eax], ebx
      nohookit:
        popad
      }

      // adjust past HookIt
      ourmemaddr=(PVOID *)(((unsigned int) ourmemaddr) + sizeof(hookit_toy));

      // igk_main_toy
      memcpy(ourmemaddr, igk_main_toy, sizeof(igk_main_toy));
      _asm
      {
        // patch hook_igk_toy w/ address
        pushad

        mov edx, offset hook_igk_toy
        add edx, 5
        mov ecx, ourmemaddr
        mov dword PTR [edx], ecx

        popad
      }

      // adjust past igk_main_toy
      ourmemaddr=(PVOID *)(((unsigned int) ourmemaddr) + sizeof(igk_main_toy));

      // hook_igk_toy
      memcpy(ourmemaddr, hook_igk_toy, sizeof(hook_igk_toy));
      _asm
      {
        pushad

        mov eax, dwSection
        add eax, 0ch // 4th dword in XBTF_Section
        mov ebx, ourmemaddr
        cmp dword PTR [eax], 0
        jne nohookigk
        mov DWORD PTR [eax], ebx
      nohookigk:
        popad
      }
      ourmemaddr=(PVOID *)(((unsigned int) ourmemaddr) + sizeof(igk_main_toy));

      if (g_guiSettings.GetInt("lcd.mode") > 0 && g_guiSettings.GetInt("lcd.type") == MODCHIP_SMARTXX)
      {
        memcpy(ourmemaddr, lcd_toy_xx, sizeof(lcd_toy_xx));
        _asm
        {
          pushad

          mov ecx, ourmemaddr
          add ecx, 0141h // lcd clear

          mov eax, dwSection
          add eax, 010h // 5th dword

          cmp dword PTR [eax], 0
          jne nolcdxx

          mov dword PTR [eax], ecx
          add ecx, 0ah // lcd writestring
          add eax, 4 // 6th dword

          cmp dword ptr [eax], 0
          jne nolcd

          mov dword ptr [eax], ecx
        nolcdxx:
          popad
        }
        ourmemaddr=(PVOID *)(((unsigned int) ourmemaddr) + sizeof(lcd_toy_xx));
      }
      else
      {
        // lcd toy
        memcpy(ourmemaddr, lcd_toy_x3, sizeof(lcd_toy_x3));
        _asm
        {
          pushad

          mov ecx, ourmemaddr
          add ecx, 0bdh // lcd clear

          mov eax, dwSection
          add eax, 010h // 5th dword

          cmp dword PTR [eax], 0
          jne nolcd

          mov dword PTR [eax], ecx
          add ecx, 0ah // lcd writestring
          add eax, 4 // 6th dword

          cmp dword ptr [eax], 0
          jne nolcd

          mov dword ptr [eax], ecx
        nolcd:
          popad
        }
        ourmemaddr=(PVOID *)(((unsigned int) ourmemaddr) + sizeof(lcd_toy_x3));
      }
    }
  }

	return Found;
}

bool CUtil::RemoveTrainer()
{
  unsigned char *xboxkrnl = (unsigned char *)KERNEL_START_ADDRESS;
	unsigned char *hackptr = (unsigned char *)KERNEL_STORE_ADDRESS;
  void *ourmemaddr = NULL; // pointer used to allocated trainer mem
	unsigned int i = 0;
	bool Found = false;
  
  unsigned char xbe_entry_point[] = {0xff,0x15,0x80,0x00,0x00,0x0c}; // xbe entry point bytes in kernel
  *((DWORD*)(xbe_entry_point+2)) = KERNEL_STORE_ADDRESS;

	for(i = 0; i < KERNEL_SEARCH_RANGE; i++)
	{
		if (memcmp(&xboxkrnl[i], xbe_entry_point, 6) == 0)
		{
			Found = true;
			break;
		}
	}

	if(Found)
  {
    unsigned char *patchlocation = xboxkrnl;
    patchlocation += i + 2; // adjust to xbe entry point bytes in kernel (skipping actual call opcodes)
    __asm // recycle check
    {
        pushad

        mov eax, cr0
        push eax
        and eax, 0FFFEFFFFh
        mov cr0, eax // disable memory write prot

        mov	edi, patchlocation // address of call to xbe entry point in kernel
        mov	dword ptr [edi], 0x00010128 // patch with address of where we store loaderdata+trainer buffer address

        pop eax
        mov cr0, eax // restore memory write prot

        popad
    }
  }
  
  return Found;
}

void CUtil::RunShortcut(const char* szShortcutPath)
{
  CShortcut shortcut;
  char szPath[1024];
  char szParameters[1024];
  if ( shortcut.Create(szShortcutPath))
  {
    CFileItem item(shortcut.m_strPath, false);
    // if another shortcut is specified, load this up and use it
    if (item.IsShortCut())
    {
      CHAR szNewPath[1024];
      strcpy(szNewPath, szShortcutPath);
      CHAR* szFile = strrchr(szNewPath, '\\');
      strcpy(&szFile[1], shortcut.m_strPath.c_str());

      CShortcut targetShortcut;
      if (FAILED(targetShortcut.Create(szNewPath)))
        return;

      shortcut.m_strPath = targetShortcut.m_strPath;
    }

    strcpy( szPath, shortcut.m_strPath.c_str() );

    CHAR szMode[16];
    strcpy( szMode, shortcut.m_strVideo.c_str() );
    strlwr( szMode );

    strcpy(szParameters, shortcut.m_strParameters.c_str());

    BOOL bRow = strstr(szMode, "pal") != NULL;
    BOOL bJap = strstr(szMode, "ntsc-j") != NULL;
    BOOL bUsa = strstr(szMode, "ntsc") != NULL;

    F_VIDEO video = VIDEO_NULL;
    if (bRow)
      video = VIDEO_PAL50;
    if (bJap)
      video = (F_VIDEO)CXBE::FilterRegion(VIDEO_NTSCJ);
    if (bUsa)
      video = (F_VIDEO)CXBE::FilterRegion(VIDEO_NTSCM);

    CUSTOM_LAUNCH_DATA data;
    if (!shortcut.m_strCustomGame.IsEmpty())
    {
      memset(&data,0,sizeof(CUSTOM_LAUNCH_DATA));
      strcpy(data.szFilename,shortcut.m_strCustomGame.c_str());
      CIoSupport support;
      support.GetPartition("C:",data.szRemap_D_As);
      strcpy(data.szLaunchXBEOnExit,CUtil::GetFileName(g_guiSettings.GetString("myprograms.dashboard")).c_str());
      data.executionType = 0;
      data.magic = GetXbeID(szPath);
    }
    
    CUtil::RunXBE(szPath,strcmp(szParameters,"")?szParameters:NULL,video,COUNTRY_NULL,shortcut.m_strCustomGame.IsEmpty()?NULL:&data);
  }
}

bool CUtil::RunFFPatchedXBE(CStdString szPath1, CStdString& szNewPath)
{
  if (!g_guiSettings.GetBool("myprograms.autoffpatch"))
  {
    CLog::Log(LOGDEBUG, __FUNCTION__" - Auto Filter Flicker is off. Skipping Filter Flicker Patching.");
    return false;
  }
  CStdString strIsPMode = g_settings.m_ResInfo[g_guiSettings.m_LookAndFeelResolution].strMode;
  if ( strIsPMode.Equals("480p 16:9") || strIsPMode.Equals("480p 4:3") || strIsPMode.Equals("720p 16:9"))
  {
    CLog::Log(LOGDEBUG, __FUNCTION__" - Progressive Mode detected: Skipping Auto Filter Flicker Patching!");
    return false;
  }
  if (strncmp(szPath1, "D:", 2) == 0)
  {
    CLog::Log(LOGDEBUG, __FUNCTION__" - Source is DVD-ROM! Skipping Filter Flicker Patching.");
    return false;
  }
  
  CLog::Log(LOGDEBUG, __FUNCTION__" - Auto Filter Flicker is ON. Starting Filter Flicker Patching.");
  CXBE m_xbe;
  if((int)m_xbe.ExtractGameRegion(szPath1.c_str()) <= 0) // Reading the GameRegion is enought to detect a Patchable xbe!
  {
    CLog::Log(LOGDEBUG, __FUNCTION__" - %s",szPath1.c_str());
    CLog::Log(LOGDEBUG, __FUNCTION__" - Not Patchable xbe detected (Homebrew?)! Skipping Filter Flicker Patching.");
    return false;
  }
  CGFFPatch m_ffp;
  if (!m_ffp.FFPatch(szPath1, szNewPath))
  {
    CLog::Log(LOGDEBUG, __FUNCTION__" - ERROR during Filter Flicker Patching. Falling back to the original source.");
    return false;
  }
  if(szNewPath.IsEmpty())
  {
    CLog::Log(LOGDEBUG, __FUNCTION__" - ERROR NO Patchfile Path is empty! Falling back to the original source.");
    return false;
  }
  CLog::Log(LOGDEBUG, __FUNCTION__" - Filter Flicker Patching done. Starting %s.",szNewPath.c_str());
  return true;
}
void CUtil::RunXBE(const char* szPath1, char* szParameters, F_VIDEO ForceVideo, F_COUNTRY ForceCountry, CUSTOM_LAUNCH_DATA* pData) 
{
  /// \brief Runs an executable file
  /// \param szPath1 Path of executeable to run
  /// \param szParameters Any parameters to pass to the executeable being run
  g_application.PrintXBEToLCD(szPath1); //write to LCD
  Sleep(600);        //and wait a little bit to execute

  CStdString szNewPath;
  if(RunFFPatchedXBE(szPath1, szNewPath))
  {
    szPath1 = szNewPath;
  }
  char szDevicePath[1024];
  char szPath[1024];
  char szXbePath[1024];
  strcpy(szPath, szPath1);
  if (strncmp(szPath1, "Q:", 2) == 0)
  { // mayaswell support the virtual drive as well...
    CStdString strPath;
    // home dir is xbe dir
    GetHomePath(strPath);
    if (!HasSlashAtEnd(strPath))
      strPath += "\\";
    if (szPath1[2] == '\\')
      strPath += szPath1 + 3;
    else
      strPath += szPath1 + 2;
    strcpy(szPath, strPath.c_str());
  }
  char* szBackslash = strrchr(szPath, '\\');
  if (szBackslash)
  {
    *szBackslash = 0x00;
    char* szXbe = &szBackslash[1];

    char* szColon = strrchr(szPath, ':');
    if (szColon)
    {
      *szColon = 0x00;
      char* szDrive = szPath;
      char* szDirectory = &szColon[1];

      CIoSupport helper;
      helper.GetPartition( (LPCSTR) szDrive, szDevicePath);

      strcat(szDevicePath, szDirectory);
      wsprintf(szXbePath, "d:\\%s", szXbe);

      g_application.Stop();

      CUtil::LaunchXbe(szDevicePath, szXbePath, szParameters, ForceVideo, ForceCountry, pData);
    }
  }
  
  CLog::Log(LOGERROR, "Unable to run xbe : %s", szPath);
}

void CUtil::LaunchXbe(const char* szPath, const char* szXbe, const char* szParameters, F_VIDEO ForceVideo, F_COUNTRY ForceCountry, CUSTOM_LAUNCH_DATA* pData)
{
  CLog::Log(LOGINFO, "launch xbe:%s %s", szPath, szXbe);
  CLog::Log(LOGINFO, " mount %s as D:", szPath);

  CIoSupport helper;
  helper.Unmount("D:");
  helper.Mount("D:", const_cast<char*>(szPath));

  CLog::Log(LOGINFO, "launch xbe:%s", szXbe);

  if (ForceVideo != VIDEO_NULL)
  {
    if (!ForceCountry)
    {
      if (ForceVideo == VIDEO_NTSCM)
        ForceCountry = COUNTRY_USA;
      if (ForceVideo == VIDEO_NTSCJ)
        ForceCountry = COUNTRY_JAP;
      if (ForceVideo == VIDEO_PAL50)
        ForceCountry = COUNTRY_EUR;
    }
    CLog::Log(LOGDEBUG,"forcing video mode: %i",ForceVideo);
    bool bSuccessful = PatchCountryVideo(ForceCountry, ForceVideo);
    if( !bSuccessful )
      CLog::Log(LOGINFO,"AutoSwitch: Failed to set mode");
  }
  if (pData)
  {
    DWORD dwRegion = pData->magic;
    pData->magic = CUSTOM_LAUNCH_MAGIC;
    const char* xbe = szXbe+3;
    CLog::Log(LOGINFO,"launching game %s from path %s",pData->szFilename,szPath);
    helper.Unmount("D:");
    XWriteTitleInfoAndRebootA( (char*)xbe, (char*)(CStdString("\\Device\\")+szPath).c_str(), LDT_TITLE, dwRegion, pData);
  }
  else
  {
    if (szParameters == NULL)
    {
      XLaunchNewImage(szXbe, NULL );
    }
    else
    {
      LAUNCH_DATA LaunchData;
      strcpy((char*)LaunchData.Data, szParameters);

      XLaunchNewImage(szXbe, &LaunchData );
    }
  }
}

void CUtil::GetHomePath(CStdString& strPath)
{
  char szXBEFileName[1024];
  CIoSupport helper;
  helper.GetXbePath(szXBEFileName);
  char *szFileName = strrchr(szXBEFileName, '\\');
  *szFileName = 0;
  strPath = szXBEFileName;
}

/* WARNING, this function can easily fail on full urls, since they might have options at the end */
void CUtil::ReplaceExtension(const CStdString& strFile, const CStdString& strNewExtension, CStdString& strChangedFile)
{
  CStdString strExtension;
  GetExtension(strFile, strExtension);
  if ( strExtension.size() )
  {
    strChangedFile = strFile.substr(0, strFile.size() - strExtension.size()) ;
    strChangedFile += strNewExtension;
  }
  else
  {
    strChangedFile = strFile;
    strChangedFile += strNewExtension;
  }
}

bool CUtil::HasSlashAtEnd(const CStdString& strFile)
{
  if (strFile.size() == 0) return false;
  char kar = strFile.c_str()[strFile.size() - 1];
  if (kar == '/' || kar == '\\') 
    return true;

  return false;
}

bool CUtil::IsRemote(const CStdString& strFile)
{
  CURL url(strFile);
  CStdString strProtocol = url.GetProtocol();
  strProtocol.ToLower();
  if (strProtocol == "cdda" || strProtocol == "iso9660") return false;
  if (strProtocol.Left(3) == "mem") return false;   // memory cards
  if (strProtocol == "virtualpath")
  { // virtual paths need to be checked separately
    CVirtualPathDirectory dir;
    vector<CStdString> paths;
    if (dir.GetPathes(strFile, paths))
    {
      for (unsigned int i = 0; i < paths.size(); i++)
        if (IsRemote(paths[i])) return true;
    }
    return false;
  }
  if ( !url.IsLocal() ) return true;
  return false;
}

bool CUtil::IsOnDVD(const CStdString& strFile)
{
  if (strFile.Left(4) == "DVD:" || strFile.Left(4) == "dvd:")
    return true;

  if (strFile.Left(2) == "D:" || strFile.Left(2) == "d:")
    return true;

  if (strFile.Left(4) == "UDF:" || strFile.Left(4) == "udf:")
    return true;

  if (strFile.Left(8) == "ISO9660:" || strFile.Left(8) == "iso9660:")
    return true;

  return false;
}

bool CUtil::IsMultiPath(const CStdString& strPath)
{
  if (strPath.Left(12).Equals("multipath://")) return true;
  return false;
}

bool CUtil::IsDVD(const CStdString& strFile)
{
  CStdString strFileLow = strFile; strFileLow.MakeLower();
  if (strFileLow == "d:\\" || strFileLow == "iso9660://" || strFileLow == "udf://" || strFileLow == "dvd://1" )
    return true;

  return false;
}

bool CUtil::IsVirtualPath(const CStdString& strFile)
{
  if (strFile.Left(14).Equals("virtualpath://")) return true;
  return false;
}

bool CUtil::IsStack(const CStdString& strFile)
{
  if (strFile.Left(8).Equals("stack://")) return true;
  return false;
}

bool CUtil::IsRAR(const CStdString& strFile)
{
  CStdString strExtension;
  CUtil::GetExtension(strFile,strExtension);
  if (strExtension.Equals(".001") && strFile.Mid(strFile.length()-7,7).CompareNoCase(".ts.001")) return true;
  if (strExtension.CompareNoCase(".cbr") == 0) return true;
  if (strExtension.CompareNoCase(".rar") == 0)
      return true;

  return false;
}

bool CUtil::IsInZIP(const CStdString& strFile)
{
  if( strFile.substr(0,6) == "zip://" )
  {
    CURL url(strFile);
    return url.GetFileName() != "";
  }
  return false;
}

bool CUtil::IsInRAR(const CStdString& strFile)
{
  if( strFile.substr(0,6) == "rar://" )
  {
    CURL url(strFile);
    return url.GetFileName() != "";
  }
  return false;
}

bool CUtil::IsZIP(const CStdString& strFile) // also checks for comic books!
{
  CStdString strExtension;
  CUtil::GetExtension(strFile,strExtension);
  if (strExtension.CompareNoCase(".zip") == 0) return true;
  if (strExtension.CompareNoCase(".cbz") == 0) return true;
  return false;
}

bool CUtil::IsCDDA(const CStdString& strFile)
{
  return strFile.Left(5).Equals("cdda:");
}

bool CUtil::IsISO9660(const CStdString& strFile)
{
  return strFile.Left(8).Equals("iso9660:");
}

bool CUtil::IsSmb(const CStdString& strFile)
{
  return strFile.Left(4).Equals("smb:");
}

bool CUtil::IsDAAP(const CStdString& strFile)
{
  return strFile.Left(5).Equals("daap:");  
}

bool CUtil::IsMemCard(const CStdString& strFile)
{
  return strFile.Left(3).Equals("mem");
}

void CUtil::GetFileAndProtocol(const CStdString& strURL, CStdString& strDir)
{
  strDir = strURL;
  if (!IsRemote(strURL)) return ;
  if (IsDVD(strURL)) return ;

  CURL url(strURL);
  strDir.Format("%s://%s", url.GetProtocol().c_str(), url.GetFileName().c_str());
}

int CUtil::GetDVDIfoTitle(const CStdString& strFile)
{
  CStdString strFilename = GetFileName(strFile);
  if (strFilename.Equals("video_ts.ifo")) return 0;
  //VTS_[TITLE]_0.IFO
  return atoi(strFilename.Mid(4, 2).c_str());
}

void CUtil::UrlDecode(CStdString& strURLData)
{
  CStdString strResult;
  
  /* resulet will always be less than source */
  strResult.reserve( strURLData.length() );

  for (unsigned int i = 0; i < (int)strURLData.size(); ++i)
  {
    int kar = (unsigned char)strURLData[i];
    if (kar == '+') strResult += ' ';
    else if (kar == '%')
    {
      if (i < strURLData.size() - 2)
      {
        CStdString strTmp;
        strTmp.assign(strURLData.substr(i + 1, 2));
        int dec_num;
        sscanf(strTmp,"%x",&dec_num);
        strResult += (char)dec_num;
        i += 2;
      }
      else
        strResult += kar;
    }
    else strResult += kar;
  }
  strURLData = strResult;
}

void CUtil::URLEncode(CStdString& strURLData)
{
  CStdString strResult;

  /* wonder what a good value is here is, depends on how often it occurs */  
  strResult.reserve( strURLData.length() * 2 );

  for (int i = 0; i < (int)strURLData.size(); ++i)
  {
    int kar = (unsigned char)strURLData[i];
    if (kar == ' ') strResult += '+';
    else if (isalnum(kar)) strResult += kar;
    else
    {
      CStdString strTmp;
      strTmp.Format("%%%02.2x", kar);
      strResult += strTmp;
    }
  }
  strURLData = strResult;
}

bool CUtil::CacheXBEIcon(const CStdString& strFilePath, const CStdString& strIcon)
{
  // extract icon from .xbe
  CStdString localFile;
  g_charsetConverter.utf8ToStringCharset(strFilePath, localFile);
  CXBE xbeReader;
  CStdString strTempFile;
  CUtil::AddFileToFolder(g_advancedSettings.m_cachePath,"1.xpr",strTempFile);

  if (!xbeReader.ExtractIcon(localFile, strTempFile.c_str()))
    return false;

  bool success(false);
  CXBPackedResource* pPackedResource = new CXBPackedResource();
  if ( SUCCEEDED( pPackedResource->Create( strTempFile.c_str(), 1, NULL ) ) )
  {
    LPDIRECT3DTEXTURE8 pTexture;
    LPDIRECT3DTEXTURE8 m_pTexture;
    D3DSURFACE_DESC descSurface;

    pTexture = pPackedResource->GetTexture((DWORD)0);

    if ( pTexture )
    {
      if ( SUCCEEDED( pTexture->GetLevelDesc( 0, &descSurface ) ) )
      {
        int iHeight = descSurface.Height;
        int iWidth = descSurface.Width;
        DWORD dwFormat = descSurface.Format;
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
        if ( D3D_OK == m_pTexture->LockRect(0, &rectLocked, NULL, 0L ) )
        {
          BYTE *pBuff = (BYTE*)rectLocked.pBits;
          if (pBuff)
          {
            DWORD strideScreen = rectLocked.Pitch;
            //mp_msg(0,0," strideScreen=%i\n", strideScreen);
            CPicture pic;
            success = pic.CreateThumbnailFromSurface(pBuff, iHeight, iWidth, strideScreen, strIcon.c_str());
          }
          m_pTexture->UnlockRect(0);
        }
        pSrcSurface->Release();
        pDestSurface->Release();
        m_pTexture->Release();
      }
      pTexture->Release();
    }
  }
  delete pPackedResource;
  CFile::Delete(strTempFile);
  return success;
}


bool CUtil::GetDirectoryName(const CStdString& strFileName, CStdString& strDescription)
{
  CStdString strFName = CUtil::GetFileName(strFileName);
  strDescription = strFileName.Left(strFileName.size() - strFName.size());
  if (CUtil::HasSlashAtEnd(strDescription) )
  {
    strDescription = strDescription.Left(strDescription.size() - 1);
  }
  int iPos = strDescription.ReverseFind("\\");
  if (iPos < 0)
    iPos = strDescription.ReverseFind("/");
  if (iPos >= 0)
  {
    CStdString strTmp = strDescription.Right(strDescription.size()-iPos-1);
    strDescription = strTmp;//strDescription.Right(strDescription.size() - iPos - 1);
  }
  else if (strDescription.size() <= 0)    
    strDescription = strFName;
  return true;
}

bool CUtil::GetXBEDescription(const CStdString& strFileName, CStdString& strDescription)
{
  _XBE_CERTIFICATE HC;
  _XBE_HEADER HS;

  FILE* hFile = fopen(strFileName.c_str(), "rb");
  if (!hFile)
  {
    strDescription = CUtil::GetFileName(strFileName);
    return false;
  }
  fread(&HS, 1, sizeof(HS), hFile);
  fseek(hFile, HS.XbeHeaderSize, SEEK_SET);
  fread(&HC, 1, sizeof(HC), hFile);
  fclose(hFile);

  // The XBE title is stored in WCHAR (UTF16) format

  // XBE titles can in fact use all 40 characters available to them,
  // and thus are not necessarily NULL terminated
  WCHAR TitleName[41];
  wcsncpy(TitleName, HC.TitleName, 40);
  TitleName[40] = 0;
  if (wcslen(TitleName) > 0)
  {
    g_charsetConverter.utf16toUTF8(TitleName, strDescription);
    return true;
  }
  strDescription = CUtil::GetFileName(strFileName);
  return false;
}

bool CUtil::SetXBEDescription(const CStdString& strFileName, const CStdString& strDescription)
{
  _XBE_CERTIFICATE HC;
  _XBE_HEADER HS;

  FILE* hFile = fopen(strFileName.c_str(), "r+b");
  fread(&HS, 1, sizeof(HS), hFile);
  fseek(hFile, HS.XbeHeaderSize, SEEK_SET);
  fread(&HC, 1, sizeof(HC), hFile);
  fseek(hFile,HS.XbeHeaderSize, SEEK_SET);

  // The XBE title is stored in WCHAR (UTF16)

  CStdStringW shortDescription;
  g_charsetConverter.utf8ToUTF16(strDescription, shortDescription);
  if (shortDescription.size() > 40)
    shortDescription = shortDescription.Left(40); 
  wcsncpy(HC.TitleName, shortDescription.c_str(), 40);  // only allow 40 chars*/
  fwrite(&HC,1,sizeof(HC),hFile);
  fclose(hFile);
  
  return true;
}

DWORD CUtil::GetXbeID( const CStdString& strFilePath)
{
  DWORD dwReturn = 0;

  DWORD dwCertificateLocation;
  DWORD dwLoadAddress;
  DWORD dwRead;
  //  WCHAR wcTitle[41];

  CAutoPtrHandle hFile( CreateFile( strFilePath.c_str(),
                                    GENERIC_READ,
                                    FILE_SHARE_READ,
                                    NULL,
                                    OPEN_EXISTING,
                                    FILE_ATTRIBUTE_NORMAL,
                                    NULL ));
  if ( hFile.isValid() )
  {
    if ( SetFilePointer( (HANDLE)hFile, 0x104, NULL, FILE_BEGIN ) == 0x104 )
    {
      if ( ReadFile( (HANDLE)hFile, &dwLoadAddress, 4, &dwRead, NULL ) )
      {
        if ( SetFilePointer( (HANDLE)hFile, 0x118, NULL, FILE_BEGIN ) == 0x118 )
        {
          if ( ReadFile( (HANDLE)hFile, &dwCertificateLocation, 4, &dwRead, NULL ) )
          {
            dwCertificateLocation -= dwLoadAddress;
            // Add offset into file
            dwCertificateLocation += 8;
            if ( SetFilePointer( (HANDLE)hFile, dwCertificateLocation, NULL, FILE_BEGIN ) == dwCertificateLocation )
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

void CUtil::CreateShortcut(CFileItem* pItem)
{
  if ( pItem->IsXBE() )
  {
    // xbe
    pItem->SetIconImage("defaultProgram.png");
    if ( !pItem->IsOnDVD() )
    {
      CStdString strDescription;
      if (! CUtil::GetXBEDescription(pItem->m_strPath, strDescription))
      {
        CUtil::GetDirectoryName(pItem->m_strPath, strDescription);
      }
      if (strDescription.size())
      {
        CStdString strFname;
        strFname = CUtil::GetFileName(pItem->m_strPath);
        strFname.ToLower();
        if (strFname != "dashupdate.xbe" && strFname != "downloader.xbe" && strFname != "update.xbe")
        {
          CShortcut cut;
          cut.m_strPath = pItem->m_strPath;
          cut.Save(strDescription);
        }
      }
    }
  }
}

void CUtil::GetFatXQualifiedPath(CStdString& strFileNameAndPath)
{
  // This routine gets rid of any "\\"'s at the start of the path.
  // Should this be the case?
  vector<CStdString> tokens;
  CStdString strBasePath;
  strFileNameAndPath.Replace("/","\\");
  CUtil::GetDirectory(strFileNameAndPath,strBasePath);
  CStdString strFileName = CUtil::GetFileName(strFileNameAndPath);
  CUtil::Tokenize(strBasePath,tokens,"\\");
  if (tokens.empty())
    return; // nothing to do here (invalid path)
  strFileNameAndPath = tokens.front();
  for (vector<CStdString>::iterator token=tokens.begin()+1;token != tokens.end();++token)
  {
    CStdString strToken = token->Left(42);
    if (token->size() > 42)
    { // remove any spaces as a result of truncation only
      while (strToken[strToken.size()-1] == ' ')
        strToken.erase(strToken.size()-1);
    }
    CUtil::RemoveIllegalChars(strToken);
    strFileNameAndPath += "\\"+strToken;
  }
  if (strFileName != "")
  {
    CUtil::ShortenFileName(strFileName);
    if (strFileName[0] == '\\') 
      strFileName.erase(0,1);
    CUtil::RemoveIllegalChars(strFileName);
    CStdString strExtension;
    CStdString strNoExt;
    CUtil::GetExtension(strFileName,strExtension);
    CUtil::ReplaceExtension(strFileName,"",strNoExt);
    while (strNoExt[strNoExt.size()-1] == ' ')
      strNoExt.erase(strNoExt.size()-1);
    strFileNameAndPath += "\\"+strNoExt+strExtension;
  }
}

void CUtil::ShortenFileName(CStdString& strFileNameAndPath)
{
  CStdString strFile = CUtil::GetFileName(strFileNameAndPath);
  if (strFile.size() > 42)
  {
    CStdString strExtension;
    CUtil::GetExtension(strFileNameAndPath, strExtension);
    CStdString strPath = strFileNameAndPath.Left( strFileNameAndPath.size() - strFile.size() );

    CRegExp reg;
    CStdString strSearch=strFile; strSearch.ToLower();
    reg.RegComp("([_\\-\\. ](cd|part)[0-9]*)[_\\-\\. ]");          // this is to ensure that cd1, cd2 or partXXX. do not
    int matchPos = reg.RegFind(strSearch.c_str());                 // get cut from filenames when they are shortened.

    CStdString strPartNumber;
    char* szPart = reg.GetReplaceString("\\1");
    strPartNumber = szPart;
    free(szPart);

    int partPos = 42 - strPartNumber.size() - strExtension.size();

    if (matchPos > partPos )
    {
       strFile = strFile.Left(partPos);
       strFile += strPartNumber;
    } else
    {
       strFile = strFile.Left(42 - strExtension.size());
    }
    strFile += strExtension;

    CStdString strNewFile = strPath;
    if (!CUtil::HasSlashAtEnd(strPath))
      strNewFile += "\\";

    strNewFile += strFile;
    strFileNameAndPath = strNewFile;
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
  if ( !CDetectDVDMedia::IsDiscInDrive() )
  {
    strIcon = "defaultDVDEmpty.png";
    return ;
  }

  if ( IsDVD(strPath) )
  {
    CCdInfo* pInfo = CDetectDVDMedia::GetCdInfo();
    //  xbox DVD
    if ( pInfo != NULL && pInfo->IsUDFX( 1 ) )
    {
      strIcon = "defaultXBOXDVD.png";
      return ;
    }
    strIcon = "defaultDVDRom.png";
    return ;
  }

  if ( IsISO9660(strPath) )
  {
    CCdInfo* pInfo = CDetectDVDMedia::GetCdInfo();
    if ( pInfo != NULL && pInfo->IsVideoCd( 1 ) )
    {
      strIcon = "defaultVCD.png";
      return ;
    }
    strIcon = "defaultDVDRom.png";
    return ;
  }

  if ( IsCDDA(strPath) )
  {
    strIcon = "defaultCDDA.png";
    return ;
  }
}

void CUtil::RemoveTempFiles()
{
  WIN32_FIND_DATA wfd;

  CStdString strAlbumDir;
  strAlbumDir.Format("%s\\*.tmp", g_settings.GetDatabaseFolder().c_str());
  memset(&wfd, 0, sizeof(wfd));

  CAutoPtrFind hFind( FindFirstFile(strAlbumDir.c_str(), &wfd));
  if (!hFind.isValid())
    return ;
  do
  {
    if ( !(wfd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) )
    {
      string strFile = g_settings.GetDatabaseFolder();
      strFile += "\\";
      strFile += wfd.cFileName;
      DeleteFile(strFile.c_str());
    }
  }
  while (FindNextFile(hFind, &wfd));
}

void CUtil::DeleteGUISettings()
{
  // Load in master code first to ensure it's setting isn't reset
  TiXmlDocument doc;
  if (doc.LoadFile(g_settings.GetSettingsFile()))
  {
    g_guiSettings.LoadMasterLock(doc.RootElement());
  }
  // delete T:\\settings.xml only
  CLog::Log(LOGINFO, "  DeleteFile(%s)", g_settings.GetSettingsFile().c_str());
  ::DeleteFile(g_settings.GetSettingsFile().c_str());
}

bool CUtil::IsHD(const CStdString& strFileName)
{
  if (strFileName.size() <= 2) return false;
  char szDriveletter = tolower(strFileName.GetAt(0));
  if ( (szDriveletter >= 'c' && szDriveletter <= 'g' && szDriveletter != 'd') || (szDriveletter == 'q') || (szDriveletter == 'z') || (szDriveletter == 'y') || (szDriveletter == 'x') )
  {
    if (strFileName.GetAt(1) == ':') return true;
  }
  return false;
}

void CUtil::RemoveIllegalChars( CStdString& strText)
{
  char szRemoveIllegal [1024];
  strcpy(szRemoveIllegal , strText.c_str());
  static char legalChars[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz1234567890!#$%&'()-@[]^_`{}~.���������������������������������� ";
  char *cursor;
  for (cursor = szRemoveIllegal; *(cursor += strspn(cursor, legalChars)); /**/ )
    *cursor = '_';
  strText = szRemoveIllegal;
}

void CUtil::ClearSubtitles()
{
  //delete cached subs
  WIN32_FIND_DATA wfd;
  CAutoPtrFind hFind ( FindFirstFile("Z:\\*.*", &wfd));

  if (hFind.isValid())
  {
    do
    {
      if (wfd.cFileName[0] != 0)
      {
        if ( (wfd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) == 0 )
        {
          CStdString strFile;
          strFile.Format("Z:\\%s", wfd.cFileName);
          if (strFile.Find("subtitle") >= 0 )
          {
            if (strFile.Find(".keep") != strFile.size()-5) // do not remove files ending with .keep
              ::DeleteFile(strFile.c_str());
          }
          else if (strFile.Find("vobsub_queue") >= 0 )
          {
            ::DeleteFile(strFile.c_str());
          }
        }
      }
    }
    while (FindNextFile((HANDLE)hFind, &wfd));
  }
}

static char * sub_exts[] = { ".utf", ".utf8", ".utf-8", ".sub", ".srt", ".smi", ".rt", ".txt", ".ssa", ".aqt", ".jss", ".ass", ".idx", ".ifo", NULL};

void CUtil::CacheSubtitles(const CStdString& strMovie, CStdString& strExtensionCached, XFILE::IFileCallback *pCallback )
{
  static bool bAlternateChecked=false;

  // new array for commons sub dirs
  char * common_sub_dirs[] = {"subs",
                              "Subs",
                              "subtitles",
                              "Subtitles",
                              "vobsubs",
                              "Vobsubs",
                              "sub",
                              "Sub",
                              "vobsub",
                              "Vobsub",
                              "subtitle",
                              "Subtitle",
                              NULL};

  std::vector<CStdString> vecExtensionsCached;
  strExtensionCached = "";

  ClearSubtitles();

  CFileItem item(strMovie, false);
  if (item.IsInternetStream()) return ;
  if (item.IsPlayList()) return ;
  if (!item.IsVideo()) return ;

  std::vector<CStdString> strLookInPaths;

  CStdString strFileName;
  CStdString strFileNameNoExt;
  CStdString strPath;

  CUtil::Split(strMovie, strPath, strFileName);
  ReplaceExtension(strFileName, "", strFileNameNoExt);
  strLookInPaths.push_back(strPath);

  if (!bAlternateChecked && strlen(g_stSettings.m_szAlternateSubtitleDirectory)) // to avoid checking non-existent directories (network) every time..
  {
    if (!g_network.IsAvailable() && !IsHD(g_stSettings.m_szAlternateSubtitleDirectory))
    {
      CLog::Log(LOGINFO,"CUtil::CacheSubtitles: disabling alternate subtitle directory for this session, it's nonaccessible");
      g_stSettings.m_szAlternateSubtitleDirectory[0] = '\0';
    }
    else
    if (!CDirectory::Exists(g_stSettings.m_szAlternateSubtitleDirectory))
    {
      CLog::Log(LOGINFO,"CUtil::CacheSubtitles: disabling alternate subtitle directory for this session, it's nonexistant");
      g_stSettings.m_szAlternateSubtitleDirectory[0] = '\0';
    }

    bAlternateChecked = true;
  }

  if (strMovie.substr(0,6) == "rar://") // <--- if this is found in main path then ignore it!
  {
    CURL url(strMovie);
    CStdString strArchive = url.GetHostName();
    CUtil::Split(strArchive, strPath, strFileName);
    strLookInPaths.push_back(strPath);
  }

  // checking if any of the common subdirs exist ..
  int iSize = strLookInPaths.size();
  for (int i=0;i<iSize;++i)
  {
    CStdString strParent;
    CUtil::GetParentPath(strLookInPaths[i],strParent);
    if (CURL(strParent).GetFileName() == "")
      strParent = "";
    for (int j=0; common_sub_dirs[j]; j+=2)
    {
      CStdString strPath2;
      CUtil::AddFileToFolder(strLookInPaths[i],common_sub_dirs[j],strPath2);
      if (CDirectory::Exists(strPath2))
        strLookInPaths.push_back(strPath2);
      else
      {
        CURL url(strLookInPaths[i]);
        if (url.GetProtocol() == "smb" || url.GetProtocol() == "xbms")
        {
          CUtil::AddFileToFolder(strLookInPaths[i],common_sub_dirs[j+1],strPath2);
          if (CDirectory::Exists(strPath2))
            strLookInPaths.push_back(strPath2);
        }
      }
        
      // ../common dirs aswell
      if (strParent != "")
      {
        CUtil::AddFileToFolder(strParent,common_sub_dirs[j],strPath2);
        if (CDirectory::Exists(strPath2))
          strLookInPaths.push_back(strPath2);
        else
        {
          CURL url(strParent);

          if (url.GetProtocol() == "smb" || url.GetProtocol() == "xbms")
          {
            CUtil::AddFileToFolder(strParent,common_sub_dirs[j+1],strPath2);
            if (CDirectory::Exists(strPath2))
              strLookInPaths.push_back(strPath2);
          }
        }
      }
    }
  }
  // .. done checking for common subdirs

  // check if there any cd-directories in the paths we have added so far
  char temp[6];
  iSize = strLookInPaths.size();
  for (int i=0;i<9;++i) // 9 cd's
  {
    sprintf(temp,"cd%i",i+1);
    for (int i=0;i<iSize;++i)
    {
      CStdString strPath2;
      CUtil::AddFileToFolder(strLookInPaths[i],temp,strPath2);
      if (CDirectory::Exists(strPath2))
        strLookInPaths.push_back(strPath2);
    }
  }
  // .. done checking for cd-dirs

  // this is last because we dont want to check any common subdirs or cd-dirs in the alternate <subtitles> dir.
  if (strlen(g_stSettings.m_szAlternateSubtitleDirectory) != 0)
  {
    strPath = g_stSettings.m_szAlternateSubtitleDirectory;
    if (!HasSlashAtEnd(strPath))
      strPath += "/"; //Should work for both remote and local files
    strLookInPaths.push_back(strPath);
  }

  CStdString strLExt;
  CStdString strDest;
  CStdString strItem;

  // 2 steps for movie directory and alternate subtitles directory
  for (unsigned int step = 0; step < strLookInPaths.size(); step++)
  {
    if (strLookInPaths[step].length() != 0)
    {
      CFileItemList items;

      CDirectory::GetDirectory(strLookInPaths[step], items,".utf|.utf8|.utf-8|.sub|.srt|.smi|.rt|.txt|.ssa|.text|.ssa|.aqt|.jss|.ass|.idx|.ifo|.rar",false);
      int fnl = strFileNameNoExt.size();

      CStdString strFileNameNoExtNoCase(strFileNameNoExt);
      strFileNameNoExtNoCase.MakeLower();
      for (int j = 0; j < (int)items.Size(); j++)
      {
        Split(items[j]->m_strPath, strPath, strItem);

        // is this a rar-file .. 
        if (CUtil::IsRAR(strItem) && g_guiSettings.GetBool("subtitles.searchrars"))
        {
          CStdString strRar, strItemWithPath;
          CUtil::AddFileToFolder(strLookInPaths[step],strFileNameNoExt+".rar",strRar);
          CUtil::AddFileToFolder(strLookInPaths[step],strItem,strItemWithPath);
          
          if (step != (strMovie.substr(0,6)=="rar://"?1:0) || (strFileNameNoExtNoCase+".rar").Equals(strItem))
            CacheRarSubtitles( vecExtensionsCached, items[j]->m_strPath, strFileNameNoExtNoCase);
        }
        else
        {
          for (int i = 0; sub_exts[i]; i++)
          {
            int l = strlen(sub_exts[i]);

            //Cache any alternate subtitles.
            if (strItem.Left(9).ToLower() == "subtitle." && strItem.Right(l).ToLower() == sub_exts[i])
            {
              strLExt = strItem.Right(strItem.GetLength() - 9);
              strDest.Format("Z:\\subtitle.alt-%s", strLExt);
              if (CFile::Cache(items[j]->m_strPath, strDest.c_str(), pCallback, NULL))
              {
                CLog::Log(LOGINFO, " cached subtitle %s->%s\n", strItem.c_str(), strDest.c_str());
                strExtensionCached = strLExt;
              }
            }

            //Cache subtitle with same name as movie
            if (strItem.Right(l).ToLower() == sub_exts[i] && strItem.Left(fnl).ToLower() == strFileNameNoExt.ToLower())
            {
              strLExt = strItem.Right(strItem.size() - fnl - 1); //Disregard separator char
              strDest.Format("Z:\\subtitle.%s", strLExt);
              if (std::find(vecExtensionsCached.begin(),vecExtensionsCached.end(),strLExt) == vecExtensionsCached.end())
              {
                if (CFile::Cache(items[j]->m_strPath, strDest.c_str(), pCallback, NULL))
                {
                  vecExtensionsCached.push_back(strLExt);
                  CLog::Log(LOGINFO, " cached subtitle %s->%s\n", strItem.c_str(), strDest.c_str());
                }
              }
            }
          }
        }
      }

      g_directoryCache.ClearDirectory(strLookInPaths[step]);
    }
  }

  // rename any keep subtitles
  CFileItemList items;
  CDirectory::GetDirectory("z:\\",items,".keep");
  for (int i=0;i<items.Size();++i)
  {
    CFile::Delete(items[i]->m_strPath.Left(items[i]->m_strPath.size()-5));
    CFile::Rename(items[i]->m_strPath,items[i]->m_strPath.Left(items[i]->m_strPath.size()-5));
  }
  
  // construct string of added exts?
  for (std::vector<CStdString>::iterator it=vecExtensionsCached.begin(); it != vecExtensionsCached.end(); ++it)
    strExtensionCached += *it+" ";
}

bool CUtil::CacheRarSubtitles(std::vector<CStdString>& vecExtensionsCached, const CStdString& strRarPath, const CStdString& strCompare, const CStdString& strExtExt)
{
  bool bFoundSubs = false;
  CFileItemList ItemList;

  // get _ALL_files in the rar, even those located in subdirectories because we set the bMask to false.
  // so now we dont have to find any subdirs anymore, all files in the rar is checked.
  if( !g_RarManager.GetFilesInRar(ItemList, strRarPath, false, "") ) return false;
  for (int it= 0 ; it <ItemList.Size();++it)
  {
    CStdString strPathInRar = ItemList[it]->m_strPath;
    CLog::Log(LOGDEBUG, "CacheRarSubs:: Found file %s", strPathInRar.c_str());
    // always check any embedded rar archives
    // checking for embedded rars, I moved this outside the sub_ext[] loop. We only need to check this once for each file.
    if (CUtil::IsRAR(strPathInRar))
    {
      CStdString strExtAdded;
      CStdString strRarInRar;
      CUtil::CreateRarPath(strRarInRar, strRarPath, strPathInRar);
      CacheRarSubtitles(vecExtensionsCached,strRarInRar,strCompare, strExtExt);
    }
    // done checking if this is a rar-in-rar

    int iPos=0;
    CStdString strExt = CUtil::GetExtension(strPathInRar);
    CStdString strFileName = CUtil::GetFileName(strPathInRar);
    CStdString strFileNameNoCase(strFileName);
    strFileNameNoCase.MakeLower();
    if (strFileNameNoCase.Find(strCompare) >= 0)
      while (sub_exts[iPos])
      {      
        if (strExt.CompareNoCase(sub_exts[iPos]) == 0)
        {
          CStdString strSourceUrl, strDestUrl;
          CUtil::CreateRarPath(strSourceUrl, strRarPath, strPathInRar);

          CStdString strDestFile;
          strDestFile.Format("z:\\subtitle%s%s", sub_exts[iPos],strExtExt.c_str());

          if (CFile::Cache(strSourceUrl,strDestFile))
          {
            vecExtensionsCached.push_back(CStdString(sub_exts[iPos]));
            CLog::Log(LOGINFO, " cached subtitle %s->%s", strPathInRar.c_str(), strDestFile.c_str());
            bFoundSubs = true;
            break;
          }
        }

        iPos++;
      }
  }
  return bFoundSubs;
}

void CUtil::PrepareSubtitleFonts()
{
  if (g_guiSettings.GetString("subtitles.font").size() == 0) return ;
  if (g_guiSettings.GetInt("subtitles.height") == 0) return ;

  CStdString strPath, strHomePath, strSearchMask;
  //  if(!g_guiSettings.GetBool("MyVideos.AlternateMPlayer"))
  strHomePath = "Q:\\system\\players";
  //  else
  //    strHomePath = "Q:";

  strPath.Format("%s\\mplayer\\font\\%s\\%i\\",
                 strHomePath.c_str(),
                 g_guiSettings.GetString("Subtitles.Font").c_str(), g_guiSettings.GetInt("Subtitles.Height"));

  strSearchMask = strPath + "*.*";
  WIN32_FIND_DATA wfd;
  CAutoPtrFind hFind ( FindFirstFile(strSearchMask.c_str(), &wfd));
  if (hFind.isValid())
  {
    do
    {
      if (wfd.cFileName[0] != 0)
      {
        if ( (wfd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) == 0 )
        {
          CStdString strSource, strDest;
          strSource.Format("%s%s", strPath.c_str(), wfd.cFileName);
          strDest.Format("%s\\mplayer\\font\\%s", strHomePath.c_str(), wfd.cFileName);
          ::CopyFile(strSource.c_str(), strDest.c_str(), FALSE);
        }
      }
    }
    while (FindNextFile((HANDLE)hFind, &wfd));
  }
}

__int64 CUtil::ToInt64(DWORD dwHigh, DWORD dwLow)
{
  __int64 n;
  n = dwHigh;
  n <<= 32;
  n += dwLow;
  return n;
}

void CUtil::AddFileToFolder(const CStdString& strFolder, const CStdString& strFile, CStdString& strResult)
{
  strResult = strFolder;
  // remove the stack:// as it screws up the logic below
  if (IsStack(strFolder))
    strResult = strResult.Mid(8);

  // Add a slash to the end of the path if necessary
  if (!CUtil::HasSlashAtEnd(strResult))
  {
    if (strResult.Find("//") >= 0 )
      strResult += "/";
    else
      strResult += "\\";
  }
  // Remove any slash at the start of the file
  if (strFile.size() && strFile[0] == '/' || strFile[0] == '\\')
    strResult += strFile.Mid(1);
  else
    strResult += strFile;
	// re-add the stack:// protocol
  if (IsStack(strFolder))
    strResult = "stack://" + strResult;
}

void CUtil::AddSlashAtEnd(CStdString& strFolder)
{
  // correct check for base url like smb://
  if (strFolder.Right(3).Equals("://"))
    return;

  if (!CUtil::HasSlashAtEnd(strFolder))
  {
    if (strFolder.Find("//") >= 0)
      strFolder += "/";
    else
      strFolder += "\\";
  }
}

void CUtil::RemoveSlashAtEnd(CStdString& strFolder)
{
  // correct check for base url like smb://
  if (strFolder.Right(3).Equals("://"))
    return;

  if (CUtil::HasSlashAtEnd(strFolder))
    strFolder.Delete(strFolder.size() - 1);
}

void CUtil::GetPath(const CStdString& strFileName, CStdString& strPath)
{
  int iPos1 = strFileName.Find("/");
  int iPos2 = strFileName.Find("\\");
  int iPos3 = strFileName.Find(":");
  if (iPos2 > iPos1) iPos1 = iPos2;
  if (iPos3 > iPos1) iPos1 = iPos3;

  strPath = strFileName.Left(iPos1 - 1);
}
void CUtil::GetDirectory(const CStdString& strFilePath, CStdString& strDirectoryPath)
{
  //Will from a full filename return the directory the file resides in.
  //has no trailing slash on result. Could lead to problems when reading from root on cd
  //ISO9660://filename.bla will result in path ISO9660:/
  //This behaviour should probably be changed, but it would break other things
  int iPos1 = strFilePath.ReverseFind('/');
  int iPos2 = strFilePath.ReverseFind('\\');

  if (iPos2 > iPos1)
  {
    iPos1 = iPos2;
  }

  if (iPos1 > 0)
  {
    strDirectoryPath = strFilePath.Left(iPos1);
  }
}
void CUtil::Split(const CStdString& strFileNameAndPath, CStdString& strPath, CStdString& strFileName)
{
  //Splits a full filename in path and file.
  //ex. smb://computer/share/directory/filename.ext -> strPath:smb://computer/share/directory/ and strFileName:filename.ext
  //Trailing slash will be preserved
  strFileName = "";
  strPath = "";
  int i = strFileNameAndPath.size() - 1;
  while (i > 0)
  {
    char ch = strFileNameAndPath[i];
    if (ch == ':' || ch == '/' || ch == '\\') break;
    else i--;
  }
  strPath = strFileNameAndPath.Left(i + 1);
  strFileName = strFileNameAndPath.Right(strFileNameAndPath.size() - i - 1);
}

void CUtil::CreateZipPath(CStdString& strUrlPath, const CStdString& strRarPath, const CStdString& strFilePathInRar,const WORD wOptions,  const CStdString& strPwd, const CStdString& strCachePath)
{
  //The possibilties for wOptions are
  //RAR_AUTODELETE : the cached version of the rar (strRarPath) will be deleted in file's dtor.
  //EXFILE_AUTODELETE : the extracted file (strFilePathInRar) will be deleted in file's dtor.
  //RAR_OVERWRITE : if the rar is already cached, overwrite the local copy.
  //EXFILE_OVERWRITE : if the extracted file is already cached, overwrite the local copy.
  CStdString strBuffer;

  strUrlPath = "zip://";

  if( !strPwd.IsEmpty() )
  {
    strBuffer = strPwd;
    CUtil::URLEncode(strBuffer);
    strUrlPath += strBuffer;
    strUrlPath += "@";
  }    

  strBuffer = strRarPath;
  CUtil::URLEncode(strBuffer);

  strUrlPath += strBuffer;

  strBuffer = strFilePathInRar;
  strBuffer.Replace('\\', '/');
  strBuffer.TrimLeft('/');

  strUrlPath += "/";
  strUrlPath += strBuffer;

#if 0  // options are not used
  strBuffer = strCachePath;
  CUtil::URLEncode(strBuffer);

  strUrlPath += "?cache=";
  strUrlPath += strBuffer;

  strBuffer.Format("%i", wOptions);
  strUrlPath += "&flags=";
  strUrlPath += strBuffer;
#endif
}


void CUtil::CreateRarPath(CStdString& strUrlPath, const CStdString& strRarPath, const CStdString& strFilePathInRar,const WORD wOptions,  const CStdString& strPwd, const CStdString& strCachePath)
{
  //The possibilties for wOptions are
  //RAR_AUTODELETE : the cached version of the rar (strRarPath) will be deleted in file's dtor.
  //EXFILE_AUTODELETE : the extracted file (strFilePathInRar) will be deleted in file's dtor.
  //RAR_OVERWRITE : if the rar is already cached, overwrite the local copy.
  //EXFILE_OVERWRITE : if the extracted file is already cached, overwrite the local copy.
  CStdString strBuffer;

  strUrlPath = "rar://";

  if( !strPwd.IsEmpty() )
  {
    strBuffer = strPwd;
    CUtil::URLEncode(strBuffer);
    strUrlPath += strBuffer;
    strUrlPath += "@";
  }    

  strBuffer = strRarPath;
  CUtil::URLEncode(strBuffer);

  strUrlPath += strBuffer;

  strBuffer = strFilePathInRar;
  strBuffer.Replace('\\', '/');
  strBuffer.TrimLeft('/');

  strUrlPath += "/";
  strUrlPath += strBuffer;

#if 0 // options are not used
  strBuffer = strCachePath;
  CUtil::URLEncode(strBuffer);

  strUrlPath += "?cache=";
  strUrlPath += strBuffer;

  strBuffer.Format("%i", wOptions);
  strUrlPath += "&flags=";
  strUrlPath += strBuffer;
#endif
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
  if (g_guiSettings.GetBool("myvideos.useexternaldvdplayer") && !g_guiSettings.GetString("myvideos.externaldvdplayer").IsEmpty())
  {
    RunXBE(g_guiSettings.GetString("myvideos.externaldvdplayer").c_str());
  }
  else
  {
    CIoSupport helper;
    helper.Remount("D:", "Cdrom0");
    CFileItem item("dvd://1", false);
    item.SetLabel(CDetectDVDMedia::GetDVDLabel());
    g_application.PlayFile(item);
  }
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
    for (i = 0; i <= max; i++)
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
  if (iBrightNess < 0) iBrightNess = 0;
  if (iBrightNess > 100) iBrightNess = 100;
  if (iContrast < 0) iContrast = 0;
  if (iContrast > 100) iContrast = 100;
  if (iGamma < 0) iGamma = 0;
  if (iGamma > 100) iGamma = 100;

  float fBrightNess = (((float)iBrightNess) / 50.0f) - 1.0f; // -1..1 Default: 0
  float fContrast = (((float)iContrast) / 50.0f);      // 0..2  Default: 1
  float fGamma = (((float)iGamma) / 40.0f) + 0.5f;      // 0.5..3.0 Default: 1
  CUtil::SetBrightnessContrastGamma(fBrightNess, fContrast, fGamma, bImmediate);
}

void CUtil::SetBrightnessContrastGamma(float Brightness, float Contrast, float Gamma, bool bImmediate)
{
  // calculate ramp
  D3DGAMMARAMP ramp;

  Gamma = 1.0f / Gamma;
  for (int i = 0; i < 256; ++i)
  {
    float f = (powf((float)i / 255.f, Gamma) * Contrast + Brightness) * 255.f;
    ramp.blue[i] = ramp.green[i] = ramp.red[i] = clamp(f);
  }

  // set ramp next v sync
  g_graphicsContext.Lock();
  g_graphicsContext.Get3DDevice()->SetGammaRamp(bImmediate ? D3DSGR_IMMEDIATE : 0, &ramp);
  g_graphicsContext.Unlock();
}


void CUtil::Tokenize(const CStdString& path, vector<CStdString>& tokens, const string& delimiters)
{
  // Tokenize ripped from http://www.linuxselfhelp.com/HOWTO/C++Programming-HOWTO-7.html
  string str = path;
  // Skip delimiters at beginning.
  string::size_type lastPos = str.find_first_not_of(delimiters, 0);
  // Find first "non-delimiter".
  string::size_type pos = str.find_first_of(delimiters, lastPos);

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
    return ;
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

void CUtil::TakeScreenshot(const char* fn, bool flashScreen)
{
    LPDIRECT3DSURFACE8 lpSurface = NULL;

    g_graphicsContext.Lock();
    if (g_application.IsPlayingVideo())
    {
      g_renderManager.SetupScreenshot();
    }
    if (0)
    { // reset calibration to defaults
      OVERSCAN oscan;
      memcpy(&oscan, &g_settings.m_ResInfo[g_graphicsContext.GetVideoResolution()].Overscan, sizeof(OVERSCAN));
      g_graphicsContext.ResetOverscan(g_graphicsContext.GetVideoResolution(), g_settings.m_ResInfo[g_graphicsContext.GetVideoResolution()].Overscan);
      g_application.Render();
      memcpy(&g_settings.m_ResInfo[g_graphicsContext.GetVideoResolution()].Overscan, &oscan, sizeof(OVERSCAN));
    }
    // now take screenshot
    g_graphicsContext.Get3DDevice()->BlockUntilVerticalBlank();
    if (SUCCEEDED(g_graphicsContext.Get3DDevice()->GetBackBuffer( -1, D3DBACKBUFFER_TYPE_MONO, &lpSurface)))
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
    if (flashScreen)
    {
      g_graphicsContext.Get3DDevice()->BlockUntilVerticalBlank();
      FlashScreen(true, true);
      Sleep(10);
      g_graphicsContext.Get3DDevice()->BlockUntilVerticalBlank();
      FlashScreen(true, false);
    }
}

void CUtil::TakeScreenshot()
{
  char fn[1024];
  static bool savingScreenshots = false;
  static vector<CStdString> screenShots;

  bool promptUser = false;
  // check to see if we have a screenshot folder yet
  CStdString strDir = g_guiSettings.GetString("pictures.screenshotpath", false);
  if (strDir.IsEmpty())
  {
    strDir = "Z:\\";
    if (!savingScreenshots)
    {
      promptUser = true;
      savingScreenshots = true;
      screenShots.clear();
    }
  }
  CUtil::RemoveSlashAtEnd(strDir);

  if (!strDir.IsEmpty())
  {
    sprintf(fn, "%s\\screenshot%%03d.bmp", strDir.c_str());
    strcpy(fn, CUtil::GetNextFilename(fn, 999).c_str());

    if (strlen(fn))
    {
      TakeScreenshot(fn, true);
      if (savingScreenshots)
        screenShots.push_back(fn);
      if (promptUser)
      { // grab the real directory
        CStdString newDir = g_guiSettings.GetString("pictures.screenshotpath");
        if (!newDir.IsEmpty())
        {
          for (unsigned int i = 0; i < screenShots.size(); i++)
          {
            char dest[1024];
            sprintf(dest, "%s\\screenshot%%03d.bmp", newDir.c_str());
            strcpy(dest, CUtil::GetNextFilename(dest, 999).c_str());
            CFile::Cache(screenShots[i], dest);
          }
          screenShots.clear();
        }
        savingScreenshots = false;
      }
    }
    else
    {
      CLog::Log(LOGWARNING, "Too many screen shots or invalid folder");
    }
  }

}

void CUtil::ClearCache()
{
  g_directoryCache.ClearDirectory(g_settings.GetMusicThumbFolder());
/*
  g_directoryCache.ClearDirectory(g_settings.GetThumbnailsFolder());

  for (unsigned int hex=0; hex < 16; hex++)
  {
    CStdString strHex;
    strHex.Format("\\%x",hex);
    g_directoryCache.ClearDirectory(g_settings.GetThumbnailsFolder() + strHex);
  }*/
}

void CUtil::StatToStatI64(struct _stati64 *result, struct stat *stat)
{
  result->st_dev = stat->st_dev;
  result->st_ino = stat->st_ino;
  result->st_mode = stat->st_mode;
  result->st_nlink = stat->st_nlink;
  result->st_uid = stat->st_uid;
  result->st_gid = stat->st_gid;
  result->st_rdev = stat->st_rdev;
  result->st_size = (__int64)stat->st_size;
  result->st_atime = (long)(stat->st_atime & 0xFFFFFFFF);
  result->st_mtime = (long)(stat->st_mtime & 0xFFFFFFFF);
  result->st_ctime = (long)(stat->st_ctime & 0xFFFFFFFF);
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
  result->st_atime = (long)(stat->st_atime & 0xFFFFFFFF);
  result->st_mtime = (long)(stat->st_mtime & 0xFFFFFFFF);
  result->st_ctime = (long)(stat->st_ctime & 0xFFFFFFFF);
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
  if (stat->st_size <= LONG_MAX)
    result->st_size = (_off_t)stat->st_size;
  else
  {
    result->st_size = 0;
    CLog::Log(LOGWARNING, "WARNING: File is larger than 32bit stat can handle, file size will be reported as 0 bytes");
  }
  result->st_atime = (time_t)stat->st_atime;
  result->st_mtime = (time_t)stat->st_mtime;
  result->st_ctime = (time_t)stat->st_ctime;
}

bool CUtil::CreateDirectoryEx(const CStdString& strPath)
{
  // Function to create all directories at once instead
  // of calling CreateDirectory for every subdir.
  // Creates the directory and subdirectories if needed.
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

  int i;
  CFileItem item(strPath,true);
  if (item.IsSmb())
  {
    i = 0;
  }
  else if (item.IsHD())
  {
    i = 2; // remove the "E:" from the filename
  }
  else
  {
    CLog::Log(LOGERROR,"CUtil::CreateDirectoryEx called with an unsupported path: %s",strPath.c_str());
    return false;
  }

  int s = i;
  while (i < iSize)
  {
    i = path.find(cSep, i + 1);
    if (i < 0) i = iSize; // get remaining chars
    strArray.push_back(path.substr(s, i - s));
  }

  // create the directories
  url.GetURLWithoutFilename(strTemp);
  for (unsigned int i = 0; i < strArray.size(); i++)
  {
    CStdString strTemp1 = strTemp + strArray[i];
    CDirectory::Create(strTemp1);
  }
  strArray.clear();

  // was the final destination directory successfully created ?
  if (!CDirectory::Exists(strPath)) return false;
  return true;
}
CStdString CUtil::MakeLegalFileName(const char* strFile, bool bKeepExtension, bool isFATX)
{
  // check if the filename is a legal FATX one.
  // this means illegal chars will be removed from the string,
  // and the remaining string is stripped back to 42 chars if needed
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
  strPath += GetDirectorySeperator(strPath);
}

char CUtil::GetDirectorySeperator(const CStdString &strFilename)
{
  CURL url(strFilename);
  return url.GetDirectorySeparator();
}

void CUtil::ConvertFileItemToPlayListItem(const CFileItem *pItem, CPlayList::CPlayListItem &playlistitem)
{
  playlistitem.SetDescription(pItem->GetLabel());
  playlistitem.SetFileName(pItem->m_strPath);
  playlistitem.SetDuration(pItem->m_musicInfoTag.GetDuration());
  playlistitem.SetStartOffset(pItem->m_lStartOffset);
  playlistitem.SetEndOffset(pItem->m_lEndOffset);
  playlistitem.SetMusicTag(pItem->m_musicInfoTag);
  playlistitem.SetThumbnailImage(pItem->GetThumbnailImage());
}

bool CUtil::IsUsingTTFSubtitles()
{  
  return CUtil::GetExtension(g_guiSettings.GetString("subtitles.font")).Equals(".ttf");
}

typedef struct
{
  char command[20];
  char description[128];
} BUILT_IN;

const BUILT_IN commands[] = {
  "Help", "This help message",
  "Reboot", "Reboot the xbox (power cycle)",
  "Restart", "Restart the xbox (power cycle)",
  "ShutDown", "Shutdown the xbox",
  "Dashboard", "Run your dashboard",
  "RestartApp", "Restart XBMC",
  "Credits", "Run XBMCs Credits",
  "Reset", "Reset the xbox (warm reboot)",
  "Mastermode","Control master mode",
  "ActivateWindow", "Activate the specified window",
  "ReplaceWindow", "Replaces the current window with the new one",
  "TakeScreenshot", "Takes a Screenshot",
  "RunScript", "Run the specified script",
  "RunXBE", "Run the specified executeable",
  "Extract", "Extracts the specified archive",
  "PlayMedia", "Play the specified media file (or playlist)",
  "SlideShow", "Run a slideshow from the specified directory",
  "RecursiveSlideShow", "Run a slideshow from the specified directory, including all subdirs",
  "ReloadSkin", "Reload XBMC's skin",
  "PlayerControl", "Control the music or video player",
  "EjectTray", "Close or open the DVD tray",
  "AlarmClock", "Prompt for a length of time and start an alarm clock",
  "CancelAlarm","Cancels an alarm",
  "KaiConnection","Change kai connection status (connect/disconnect)",
  "Action", "Executes an action for the active window (same as in keymap)",
  "Notification", "Shows a notification on screen, specify header, then message, and optionally time in milliseconds and a icon.",
  "PlayDVD"," Plays the inserted CD or DVD media from the DVD-ROM Drive!",
  "Skin.ToggleSetting"," Toggles a skin setting on or off",
  "Skin.SetString"," Prompts and sets skin string",
  "Skin.SetNumeric"," Prompts and sets numeric input",
  "Skin.SetPath"," Prompts and sets a skin path",
  "Skin.Theme"," Control skin theme",
  "Skin.SetImage"," Prompts and sets a skin image",
  "Skin.SetBool"," Sets a skin setting on",
  "Skin.Reset"," Resets a skin setting to default",
  "Skin.ResetSettings"," Resets all skin settings",
  "Mute","Mute the player",
  "SetVolume","Set the current volume",
  "Dialog.Close","Close a dialog",
  "System.LogOff","Log off current user",
  "System.PWMControl","Control PWM RGB LEDs"
};

bool CUtil::IsBuiltIn(const CStdString& execString)
{
  CStdString function, param;
  SplitExecFunction(execString, function, param);
  for (int i = 0; i < sizeof(commands)/sizeof(BUILT_IN); i++)
  {
    if (function.CompareNoCase(commands[i].command) == 0)
      return true;
  }
  return false;
}

void CUtil::SplitExecFunction(const CStdString &execString, CStdString &strFunction, CStdString &strParam)
{
  strParam = "";

  int iPos = execString.Find("(");
  int iPos2 = execString.ReverseFind(")");
  if (iPos > 0 && iPos2 > 0)
  {
    strParam = execString.Mid(iPos + 1, iPos2 - iPos - 1);
    strFunction = execString.Left(iPos);
  }
  else
    strFunction = execString;

  //xbmc is the standard prefix.. so allways remove this
  //all other commands with go through in full
  if( strFunction.Left(5).Equals("xbmc.", false) )
    strFunction.Delete(0, 5);
}

void CUtil::GetBuiltInHelp(CStdString &help)
{
  help.Empty();
  for (int i = 0; i < sizeof(commands)/sizeof(BUILT_IN); i++)
  {
    help += commands[i].command;
    help += "\t";
//    for (int i = 0; i < 20 - strlen(commands[i].command); i++)
//      help += " ";
    help += commands[i].description;
    help += "\n";
  }
}

int CUtil::ExecBuiltIn(const CStdString& execString)
{
  // Get the text after the "XBMC."
  CStdString execute, parameter;
  SplitExecFunction(execString, execute, parameter);
  CStdString strParameterCaseIntact = parameter;
  parameter.ToLower();
  execute.ToLower();

  if (execute.Equals("reboot") || execute.Equals("restart"))  //Will reboot the xbox, aka cold reboot
  {
    g_applicationMessenger.Restart();
  }
  else if (execute.Equals("shutdown"))
  {
    g_applicationMessenger.Shutdown();
  }
  else if (execute.Equals("dashboard"))
  {
    RunXBE(g_guiSettings.GetString("myprograms.dashboard").c_str());
  }
  else if (execute.Equals("restartapp"))
  {
    g_applicationMessenger.RestartApp();
  }
  else if (execute.Equals("mastermode"))
  {
    if (g_passwordManager.bMasterUser)
    {
      g_passwordManager.bMasterUser = false;
      g_passwordManager.LockBookmarks();
      g_application.m_guiDialogKaiToast.QueueNotification(g_localizeStrings.Get(20052),g_localizeStrings.Get(20053));
    }
    else if (g_passwordManager.IsMasterLockUnlocked(true))
    {
      g_passwordManager.UnlockBookmarks();
      g_passwordManager.bMasterUser = true;
      g_application.m_guiDialogKaiToast.QueueNotification(g_localizeStrings.Get(20052),g_localizeStrings.Get(20054));
    }
  }
  else if (execute.Equals("takescreenshot"))
  {
    TakeScreenshot();
  }
  else if (execute.Equals("credits"))
  {
    RunCredits();
  }
  else if (execute.Equals("reset")) //Will reset the xbox, aka soft reset
  {
    g_applicationMessenger.Reset();
  }
  else if (execute.Equals("activatewindow") || execute.Equals("replacewindow"))
  {
    // get the parameters
    CStdString strWindow;
    CStdString strPath;

    // split the parameter on first comma
    int iPos = parameter.Find(",");
    if (iPos == 0)
    {
      // error condition missing path
      // XMBC.ActivateWindow(1,)
      CLog::Log(LOGERROR, "Activate/ReplaceWindow called with invalid parameter: %s", parameter.c_str());
      return -7;
    }
    else if (iPos < 0)
    {
      // no path parameter
      // XBMC.ActivateWindow(5001)
      strWindow = parameter;
    }
    else
    {
      // path parameter included
      // XBMC.ActivateWindow(5001,F:\Music\)
      strWindow = parameter.Left(iPos);
      strPath = parameter.Mid(iPos + 1);
    }
    if (strPath.Equals("autodetection"))
    {
      //GeminiServer: Open the AutoDetect XBOX FTP in filemanager
      if (g_guiSettings.GetBool("autodetect.onoff"))
      {
        //Autodetection String: NickName;FTP_USER;FTP_Password;FTP_PORT;BOOST_MODE
        CStdString strFTPPath, strNickName, strFtpUserName, strFtpPassword, strFtpPort, strBoosMode;
        CStdStringArray arSplit; 
        StringUtils::SplitString(strNewClientInfo,";", arSplit);
        if ((int)arSplit.size() > 1)
        {
          strNickName     = arSplit[0].c_str();
          strFtpUserName  = arSplit[1].c_str();
          strFtpPassword  = arSplit[2].c_str();
          strFtpPort      = arSplit[3].c_str();
          strBoosMode     = arSplit[4].c_str();
          strFTPPath.Format("ftp://%s:%s@%s:%s/",strFtpUserName.c_str(),strFtpPassword.c_str(),strHasClientIP.c_str(),strFtpPort.c_str());
          
          strPath  = strFTPPath;
        }else{
          CLog::Log(LOGERROR, "ActivateWindow: Autodetection returned with invalid parameter : %s", strNewClientInfo.c_str());
          return -7;
        }
      }
    }

    // confirm the window destination is actually a number
    // before switching
    int iWindow = g_buttonTranslator.TranslateWindowString(strWindow.c_str());
    if (iWindow != WINDOW_INVALID)
    {
      // disable the screensaver
      g_application.ResetScreenSaverWindow();
      if (execute.Equals("activatewindow"))
        m_gWindowManager.ActivateWindow(iWindow, strPath);
      else  // ReplaceWindow
        m_gWindowManager.ChangeActiveWindow(iWindow, strPath);
    }
    else
    {
      CLog::Log(LOGERROR, "Activate/ReplaceWindow called with invalid destination window: %s", strWindow.c_str());
      return false;
    }
  }
  else if (execute.Equals("runscript"))
  {
    g_pythonParser.evalFile(parameter.c_str());
  }
  else if (execute.Equals("extract"))
  {
    // Detects if file is zip or zip then extracts
    CStdString strDestDirect = "";
    std::vector<CStdString> params;
    StringUtils::SplitString(strParameterCaseIntact,",",params);
    if (params.size() < 2)
      CUtil::GetDirectory(params[0],strDestDirect);
    else
      strDestDirect = params[1];
  
    if (!HasSlashAtEnd(strDestDirect))
        strDestDirect += "\\";
        
    if (params.size() < 1)
      return -1; // No File Selected
    
    if (CUtil::IsZIP(params[0]))
      g_ZipManager.ExtractArchive(params[0],strDestDirect);     
    else if (CUtil::IsRAR(params[0]))
      g_RarManager.ExtractArchive(params[0],strDestDirect);
    else
      CLog::Log(LOGERROR, "CUtil::ExecuteBuiltin: No archive given");     
  }
  else if (execute.Equals("runxbe"))
  {
    // only usefull if there is actualy a xbe to execute
    if (parameter.size() > 0)
    {
      CFileItem item(parameter);
      item.m_strPath = parameter;
      if (item.IsShortCut())
        CUtil::RunShortcut(parameter);
      else if (item.IsXBE())
      {
        int iRegion;
        if (g_guiSettings.GetBool("myprograms.gameautoregion"))
        {
          CXBE xbe;
          iRegion = xbe.ExtractGameRegion(parameter);
          if (iRegion < 1 || iRegion > 7)
            iRegion = 0;
          iRegion = xbe.FilterRegion(iRegion);
        }
        else
          iRegion = 0;

        CUtil::RunXBE(parameter.c_str(),NULL,F_VIDEO(iRegion));
      }
    }
    else
    {
      CLog::Log(LOGERROR, "CUtil::ExecBuiltIn, runxbe called with no arguments.");
    }
  }
  else if (execute.Equals("playmedia"))
  {
    if (parameter.IsEmpty())
    {
      CLog::Log(LOGERROR, "XBMC.PlayMedia called with empty parameter");
      return -3;
    }
    CFileItem item(parameter, false);
    if (!g_application.PlayMedia(item, PLAYLIST_MUSIC_TEMP))
    {
      CLog::Log(LOGERROR, "XBMC.PlayMedia could not play media: %s", parameter.c_str());
      return false;
    }
  }
  else if (execute.Equals("slideShow") || execute.Equals("recursiveslideShow"))
  {
    if (parameter.IsEmpty())
    {
      CLog::Log(LOGERROR, "XBMC.SlideShow called with empty parameter");
      return -2;
    }
    CGUIMessage msg( GUI_MSG_START_SLIDESHOW, 0, 0, execute.Equals("SlideShow") ? 0 : 1, 0, 0);
    msg.SetStringParam(parameter);
    CGUIWindow *pWindow = m_gWindowManager.GetWindow(WINDOW_SLIDESHOW);
    if (pWindow) pWindow->OnMessage(msg);
  }
  else if (execute.Equals("reloadskin"))
  {
    //	Reload the skin
    CGUIWindow* pWindow=m_gWindowManager.GetWindow(m_gWindowManager.GetActiveWindow());
    DWORD dwFocusedControlID=pWindow->GetFocusedControl();

    g_application.LoadSkin(g_guiSettings.GetString("lookandfeel.skin"));

    CGUIMessage msg(GUI_MSG_SETFOCUS, m_gWindowManager.GetActiveWindow(), dwFocusedControlID, 0);
    pWindow->OnMessage(msg);
  }
  else if (execute.Equals("playercontrol"))
  {
    g_application.ResetScreenSaver();
    g_application.ResetScreenSaverWindow();
    if (parameter.IsEmpty())
    {
      CLog::Log(LOGERROR, "XBMC.PlayerControl called with empty parameter");
      return -3;
    }
    if (parameter.Equals("play"))
    { // play/pause
      // either resume playing, or pause
      if (g_application.IsPlaying())
      {
        if (g_application.GetPlaySpeed() != 1)
          g_application.SetPlaySpeed(1);
        else          
          g_application.m_pPlayer->Pause();
      }
    } 
    else if (parameter.Equals("stop"))
    {
      g_application.StopPlaying();
    }
    else if (parameter.Equals("rewind") || parameter.Equals("forward"))
    {
      if (g_application.IsPlaying() && !g_application.m_pPlayer->IsPaused())
      {
        int iPlaySpeed = g_application.GetPlaySpeed();
        if (parameter.Equals("rewind") && iPlaySpeed == 1) // Enables Rewinding
          iPlaySpeed *= -2;
        else if (parameter.Equals("rewind") && iPlaySpeed > 1) //goes down a notch if you're FFing
          iPlaySpeed /= 2;
        else if (parameter.Equals("forward") && iPlaySpeed < 1) //goes up a notch if you're RWing
        {
          iPlaySpeed /= 2;
          if (iPlaySpeed == -1) iPlaySpeed = 1;
        }
        else
          iPlaySpeed *= 2;

        if (iPlaySpeed > 32 || iPlaySpeed < -32)
          iPlaySpeed = 1;

        g_application.SetPlaySpeed(iPlaySpeed);
      }
    }
    else if (parameter.Equals("next"))
    {
      CAction action;
      action.wID = ACTION_NEXT_ITEM;
      g_application.OnAction(action);
    }
    else if (parameter.Equals("previous"))
    {
      CAction action;
      action.wID = ACTION_PREV_ITEM;
      g_application.OnAction(action);
    }
    else if (parameter.Equals("bigskipbackward"))
    {
      if (g_application.IsPlaying())
        g_application.m_pPlayer->Seek(false, true);
    }
    else if (parameter.Equals("bigskipforward"))
    {
      if (g_application.IsPlaying())
        g_application.m_pPlayer->Seek(true, true);
    }
    else if (parameter.Equals("smallskipbackward"))
    {
      if (g_application.IsPlaying())
        g_application.m_pPlayer->Seek(false, false);
    }
    else if (parameter.Equals("smallskipforward"))
    {
      if (g_application.IsPlaying())
        g_application.m_pPlayer->Seek(true, false);
    }
    else if( parameter.Equals("showvideomenu") )
    {
      if( g_application.IsPlaying() && g_application.m_pPlayer )
      {
        CAction action;
        memset(&action, 0, 0);
        action.wID = ACTION_SHOW_VIDEOMENU;
        g_application.m_pPlayer->OnAction(action);
      }
    }
    else if( parameter.Equals("record") )
    {
      if( g_application.IsPlaying() && g_application.m_pPlayer && g_application.m_pPlayer->CanRecord())
      {
        g_application.m_pPlayer->Record(!g_application.m_pPlayer->IsRecording());
      }
    }
    else if (parameter.Equals("partymode"))
    {
      if (g_partyModeManager.IsEnabled())
        g_partyModeManager.Disable();
      else
        g_partyModeManager.Enable();
    }
    else if (parameter.Equals("random"))
    {
      // get current playlist
      int iPlaylist = g_playlistPlayer.GetCurrentPlaylist();

      // reverse the current setting
      g_playlistPlayer.SetShuffle(iPlaylist, !(g_playlistPlayer.IsShuffled(iPlaylist)));

      // save settings for now playing windows
      switch (iPlaylist)
      {
      case PLAYLIST_MUSIC:
        g_stSettings.m_bMyMusicPlaylistShuffle = g_playlistPlayer.IsShuffled(iPlaylist);
        g_settings.Save();
        break;
      case PLAYLIST_VIDEO:
        g_stSettings.m_bMyVideoPlaylistShuffle = g_playlistPlayer.IsShuffled(iPlaylist);
        g_settings.Save();
      }

      // send message
      CGUIMessage msg(GUI_MSG_PLAYLISTPLAYER_RANDOM, 0, 0, iPlaylist, g_playlistPlayer.IsShuffled(iPlaylist));
      m_gWindowManager.SendThreadMessage(msg);

    }
    else if (parameter.Left(6).Equals("repeat"))
    {
      // get current playlist
      int iPlaylist = g_playlistPlayer.GetCurrentPlaylist();
      PLAYLIST::REPEAT_STATE state = g_playlistPlayer.GetRepeat(iPlaylist);

      if (parameter.Equals("repeatall"))
        state = PLAYLIST::REPEAT_ALL;
      else if (parameter.Equals("repeatone"))
        state = PLAYLIST::REPEAT_ONE;
      else if (parameter.Equals("repeatoff"))
        state = PLAYLIST::REPEAT_NONE;
      else if (state == PLAYLIST::REPEAT_NONE)
        state = PLAYLIST::REPEAT_ALL;
      else if (state == PLAYLIST::REPEAT_ALL)
        state = PLAYLIST::REPEAT_ONE;
      else
        state = PLAYLIST::REPEAT_NONE;

      g_playlistPlayer.SetRepeat(iPlaylist, state);

      // save settings for now playing windows
      switch (iPlaylist)
      {
      case PLAYLIST_MUSIC:
        g_stSettings.m_bMyMusicPlaylistRepeat = (state == PLAYLIST::REPEAT_ALL);
        g_settings.Save();
        break;
      case PLAYLIST_VIDEO:
        g_stSettings.m_bMyVideoPlaylistRepeat = (state == PLAYLIST::REPEAT_ALL);
        g_settings.Save();
      }

      // send messages so now playing window can get updated
      CGUIMessage msg(GUI_MSG_PLAYLISTPLAYER_REPEAT, 0, 0, iPlaylist, (int)state);
      m_gWindowManager.SendThreadMessage(msg);
    }
  }
  else if (execute.Equals("mute"))
  {
    g_application.Mute();
  }
  else if (execute.Equals("setvolume"))
  {
    g_application.SetVolume(atoi(parameter.c_str()));
  }
  else if (execute.Equals("ejecttray"))
  {
    CIoSupport io;
    if (io.GetTrayState() == TRAY_OPEN)
      io.CloseTray();
    else
      io.EjectTray();
  }
  else if( execute.Equals("alarmclock") ) 
  {
    float fSecs = -1.f;
    CStdString strCommand;
    CStdString strName;
    if (!parameter.IsEmpty())
    {
      CRegExp reg;
      if (!reg.RegComp("([^,]*),([^\\(,]*)(\\([^\\)]*\\)?)?,?(.*)?$"))
        return -1; // whatever
      if (reg.RegFind(strParameterCaseIntact.c_str()) > -1)
      {
        char* szParam = reg.GetReplaceString("\\2\\3");
        if (szParam)
        {
          strCommand = szParam;
          free(szParam);
        }
        szParam = reg.GetReplaceString("\\4");
        if (szParam)
        {
          if (strlen(szParam))
            fSecs = fSecs = static_cast<float>(atoi(szParam)*60);
          free(szParam);
        }
        szParam = reg.GetReplaceString("\\1");
        if (szParam)
        {
          strName = szParam;
          free(szParam);
        }
      }
    }
    
    if (fSecs == -1.f)
    {
      CStdString strTime;
      CStdString strHeading;
      if (strCommand.Equals("xbmc.shutdown") || strCommand.Equals("xbmc.shutdown()"))
        strHeading = g_localizeStrings.Get(20145);
      else
        strHeading = g_localizeStrings.Get(13209);
      if( CGUIDialogNumeric::ShowAndGetNumber(strTime, strHeading) )
        fSecs = static_cast<float>(atoi(strTime.c_str())*60);
      else
        return -4;
    }
    if( g_alarmClock.isRunning() )
      g_alarmClock.stop(strName);
    
    g_alarmClock.start(strName,fSecs,strCommand);
  }
  else if (execute.Equals("notification"))
  {
    std::vector<CStdString> params;
    StringUtils::SplitString(strParameterCaseIntact,",",params);
    if (params.size() < 2)
      return -1;
    if (params.size() == 4)
      g_application.m_guiDialogKaiToast.QueueNotification(params[3],params[0],params[1],atoi(params[2].c_str()));
    else if (params.size() == 3)
      g_application.m_guiDialogKaiToast.QueueNotification("",params[0],params[1],atoi(params[2].c_str()));
    else
      g_application.m_guiDialogKaiToast.QueueNotification(params[0],params[1]);
  }
  else if (execute.Equals("cancelalarm"))
  {
    g_alarmClock.stop(parameter);
  }
  else if (execute.Equals("kaiconnection"))
  {
    if (CKaiClient::GetInstance())	
    {
      if (!CKaiClient::GetInstance()->IsEngineConnected())
      {
        while (!CKaiClient::GetInstance()->IsEngineConnected())
        {
          CKaiClient::GetInstance()->Reattach();
          Sleep(3000);
        }
      }
      else
      {
        CKaiClient::GetInstance()->Detach();
      }
    }
    else
    {
      CGUIDialogOK::ShowAndGetInput(15000, 0, 14073, 0);
    }
  }
  else if (execute.Equals("playdvd"))
  {
    CAutorun::PlayDisc();
  }
  else if (execute.Equals("skin.togglesetting"))
  {
    int setting = g_settings.TranslateSkinBool(parameter);
    g_settings.SetSkinBool(setting, !g_settings.GetSkinBool(setting));
    g_settings.Save();
  }
  else if (execute.Equals("skin.setbool"))
  {
    int pos = parameter.Find(",");
    if (pos >= 0)
    {
      int string = g_settings.TranslateSkinBool(parameter.Left(pos));
      g_settings.SetSkinBool(string, parameter.Mid(pos+1).Equals("true"));
      g_settings.Save();
      return 0;
    }
    // default is to set it to true
    int setting = g_settings.TranslateSkinBool(parameter);
    g_settings.SetSkinBool(setting, true);
    g_settings.Save();
  }
  else if (execute.Equals("skin.reset"))
  {
    g_settings.ResetSkinSetting(parameter);
    g_settings.Save();
  }
  else if (execute.Equals("skin.resetsettings"))
  {
    g_settings.ResetSkinSettings();
    g_settings.Save();
  }
  else if (execute.Equals("skin.theme"))
  {
    // enumerate themes
    std::vector<CStdString> vecTheme;
    GetSkinThemes(vecTheme);

    int iTheme = -1;
    int iThemes = vecTheme.size();
    // find current theme
    if (!g_guiSettings.GetString("lookandfeel.skintheme").Equals("skindefault"))
      for (unsigned int i=0;i<vecTheme.size();++i)
        if (vecTheme[i].Equals(g_guiSettings.GetString("lookandfeel.skintheme")))
        {
          iTheme=i;
          break;
        }

    int iParam = atoi(parameter.c_str());
    if (iParam == 0)
      iTheme++;
    if (iParam == -1)
      iTheme--;
    if (iTheme > (int)vecTheme.size()-1)
      iTheme = -1;
    if (iTheme < -1)
      iTheme = vecTheme.size()-1; 

    if (iTheme==-1)
      g_guiSettings.SetString("lookandfeel.skintheme","skindefault");
    else
      g_guiSettings.SetString("lookandfeel.skintheme",vecTheme[iTheme]);
    
    g_application.DelayLoadSkin();
  }
  else if (execute.Equals("skin.setstring") || execute.Equals("skin.setimage") || execute.Equals("skin.setpath") || execute.Equals("skin.setnumeric"))
  {
    // break the parameter up if necessary
    // only search for the first "," and use that to break the string up
    int pos = strParameterCaseIntact.Find(",");
    if (pos >= 0)
    {
      int string = g_settings.TranslateSkinString(strParameterCaseIntact.Left(pos));
      g_settings.SetSkinString(string, strParameterCaseIntact.Mid(pos+1));
      g_settings.Save();
      return 0;
    }
    int string = g_settings.TranslateSkinString(strParameterCaseIntact);
    CStdString value = g_settings.GetSkinString(string);
    VECSHARES shares;
    g_mediaManager.GetLocalDrives(shares);
    if (execute.Equals("skin.setstring"))
    {
      if (CGUIDialogKeyboard::ShowAndGetInput(value, g_localizeStrings.Get(1029), true))
        g_settings.SetSkinString(string, value);
    }
    else if (execute.Equals("skin.setnumeric"))
    {
      if (CGUIDialogNumeric::ShowAndGetNumber(value, g_localizeStrings.Get(20074)))
        g_settings.SetSkinString(string, value);
    }
    else if (execute.Equals("skin.setimage"))
    {
      if (CGUIDialogFileBrowser::ShowAndGetImage(shares, g_localizeStrings.Get(1030), value))
        g_settings.SetSkinString(string, value);
    }
    else // execute.Equals("skin.setpath"))
    {
      if (CGUIDialogFileBrowser::ShowAndGetDirectory(shares, g_localizeStrings.Get(1031), value))
        g_settings.SetSkinString(string, value);
    }
    g_settings.Save();
  }
  else if (execute.Equals("dialog.close"))
  {
    CStdStringArray arSplit; 
    StringUtils::SplitString(parameter,",", arSplit);
    DWORD id = g_buttonTranslator.TranslateWindowString(arSplit[0]);
    bool bForce = false;
    if (arSplit.size() > 1)
      if (arSplit[1].Equals("true"))
        bForce = true;
    CGUIWindow *window = (CGUIWindow *)m_gWindowManager.GetWindow(id);
    if (window && window->IsDialog())
      ((CGUIDialog *)window)->Close(bForce);
  }
  else if (execute.Equals("system.logoff"))
  {
    if (m_gWindowManager.GetActiveWindow() == WINDOW_LOGIN_SCREEN || !g_settings.bUseLoginScreen)
      return -1;

    g_settings.m_iLastUsedProfileIndex = g_settings.m_iLastLoadedProfileIndex;
    g_application.StopPlaying();
    g_network.NetworkMessage(CNetwork::SERVICES_DOWN,1);
    g_network.Deinitialize();
    g_settings.LoadProfile(0); // login screen always runs as default user
    g_passwordManager.m_mapSMBPasswordCache.clear();
    g_passwordManager.bMasterUser = false;
    m_gWindowManager.ActivateWindow(WINDOW_LOGIN_SCREEN);
    g_network.Initialize(g_guiSettings.GetInt("network.assignment"),
      g_guiSettings.GetString("network.ipaddress").c_str(),
      g_guiSettings.GetString("network.subnet").c_str(),
      g_guiSettings.GetString("network.gateway").c_str(),
      g_guiSettings.GetString("network.dns").c_str());
  }
  else if (execute.Left(18).Equals("system.pwmcontrol"))
  {
    CStdString strTemp ,strRgbA, strRgbB, strTran; 
    CStdStringArray arSplit; 
    int iTrTime;
    StringUtils::SplitString(parameter,",", arSplit);
    
    if ((int)arSplit.size() > 1)
    {
      strRgbA  = arSplit[0].c_str();
      strRgbB  = arSplit[1].c_str();
      strTran  = arSplit[2].c_str();
      iTrTime  = atoi(arSplit[3].c_str());
    }
      PWMControl(strRgbA,strRgbB,strTran, iTrTime);
  }
  else
    return -1;
  return 0;
}
int CUtil::GetMatchingShare(const CStdString& strPath1, VECSHARES& vecShares, bool& bIsBookmarkName)
{
  if (strPath1.IsEmpty())
    return -1;

  //CLog::Log(LOGDEBUG,"CUtil::GetMatchingShare, testing original path/name [%s]", strPath1.c_str());

  // copy as we may change strPath
  CStdString strPath = strPath1;  

  // Check for special protocols
  CURL checkURL(strPath);

  // stack://
  if (checkURL.GetProtocol() == "stack")
    strPath.Delete(0, 8); // remove the stack protocol

  if (checkURL.GetProtocol() == "shout")
    strPath = checkURL.GetHostName();

  if (checkURL.GetProtocol() == "multipath")
  {
    vector<CStdString> vecTest;
    CMultiPathDirectory dir;
    dir.GetPaths(strPath, vecTest);
    strPath = vecTest[0]; // just test the first path
  }

  //CLog::Log(LOGDEBUG,"CUtil::GetMatchingShare, testing for matching name [%s]", strPath.c_str());
  bIsBookmarkName = false;
  int iIndex = -1;
  int iLength = -1;
  // we first test the NAME of a bookmark
  for (int i = 0; i < (int)vecShares.size(); ++i)
  {
    CShare share = vecShares.at(i);
    CStdString strName = share.strName;

    // special cases for dvds
    if (IsOnDVD(share.strPath))
    {
      if (IsOnDVD(strPath))
        return i;

      // not a path, so we need to modify the bookmark name
      // since we add the drive status and disc name to the bookmark
      // "Name (Drive Status/Disc Name)"
      int iPos = strName.ReverseFind('(');
      if (iPos > 1)
        strName = strName.Mid(0, iPos - 1);
    }
    //CLog::Log(LOGDEBUG,"CUtil::GetMatchingShare, comparing name [%s]", strName.c_str());
    if (strPath.Equals(strName))
    {
      bIsBookmarkName = true;
      return i;
    }
  }

  // now test the paths

  // remove user details, and ensure path only uses forward slashes
  // and ends with a trailing slash so as not to match a substring
  CURL urlDest(strPath);
  CStdString strDest;
  urlDest.GetURLWithoutUserDetails(strDest);
  ForceForwardSlashes(strDest);
  if (!HasSlashAtEnd(strDest))
    strDest += "/";
  int iLenPath = strDest.size();

  //CLog::Log(LOGDEBUG,"CUtil::GetMatchingShare, testing url [%s]", strDest.c_str());

  for (int i = 0; i < (int)vecShares.size(); ++i)
  {
    CShare share = vecShares.at(i);

    // does it match a bookmark name?
    if (share.strPath.substr(0,8) == "shout://")
    {
      CURL url(share.strPath);
      if (strPath.Equals(url.GetHostName()))
        return i;
    }
    
    // doesnt match a name, so try the bookmark path
    vector<CStdString> vecPaths;

    // add any concatenated paths if they exist
    if (share.vecPaths.size() > 0)
      vecPaths = share.vecPaths;

    // add the actual share path at the front of the vector
    vecPaths.insert(vecPaths.begin(), share.strPath);
    
    // test each path
    for (int j = 0; j < (int)vecPaths.size(); ++j)
    {
      // remove user details, and ensure path only uses forward slashes
      // and ends with a trailing slash so as not to match a substring
      CURL urlShare(vecPaths[j]);
      CStdString strShare;
      urlShare.GetURLWithoutUserDetails(strShare);
      ForceForwardSlashes(strShare);
      if (!HasSlashAtEnd(strShare))
        strShare += "/";
      int iLenShare = strShare.size();
      //CLog::Log(LOGDEBUG,"CUtil::GetMatchingShare, comparing url [%s]", strShare.c_str());

      if ((iLenPath >= iLenShare) && (strDest.Left(iLenShare).Equals(strShare)) && (iLenShare > iLength))
      {
        //CLog::Log(LOGDEBUG,"Found matching bookmark at index %i: [%s], Len = [%i]", i, strShare.c_str(), iLenShare);

        // if exact match, return it immediately
        if (iLenPath == iLenShare)
        {
          // if the path EXACTLY matches an item in a concatentated path
          // set bookmark name to true to load the full virtualpath 
          bIsBookmarkName = false;
          if (vecPaths.size() > 1)
            bIsBookmarkName = true;
          return i;
        }
        iIndex = i;
        iLength = iLenShare;
      }
    }
  }

  // return the index of the share with the longest match
  if (iIndex == -1)
  {

    // rar:// and zip://
    // if archive wasn't mounted, look for a matching share for the archive instead    
    if( strPath.Left(6).Equals("rar://") || strPath.Left(6).Equals("zip://") )
    {
      // get the hostname portion of the url since it contains the archive file 
      strPath = checkURL.GetHostName();
      
      bIsBookmarkName = false;
      bool bDummy;
      return GetMatchingShare(strPath, vecShares, bDummy);
    }

    CLog::Log(LOGWARNING,"CUtil::GetMatchingShare... no matching bookmark found for [%s]", strPath1.c_str());
  }
  return iIndex;
}

CStdString CUtil::TranslateSpecialDir(const CStdString &strSpecial)
{
  CStdString strReturn=strSpecial;
  if (strSpecial[0] == '$')
  {
    if (strSpecial.Left(5).Equals("$HOME"))
      CUtil::AddFileToFolder("Q:", strSpecial.Mid(5), strReturn);
    else if (strSpecial.Left(10).Equals("$SUBTITLES"))
      CUtil::AddFileToFolder(g_stSettings.m_szAlternateSubtitleDirectory, strSpecial.Mid(10), strReturn);
    else if (strSpecial.Left(9).Equals("$USERDATA"))
      CUtil::AddFileToFolder(g_settings.GetUserDataFolder(), strSpecial.Mid(9), strReturn);
    else if (strSpecial.Left(9).Equals("$DATABASE"))
      CUtil::AddFileToFolder(g_settings.GetDatabaseFolder(), strSpecial.Mid(9), strReturn);
    else if (strSpecial.Left(11).Equals("$THUMBNAILS"))
      CUtil::AddFileToFolder(g_settings.GetThumbnailsFolder(), strSpecial.Mid(11), strReturn);
    else if (strSpecial.Left(11).Equals("$RECORDINGS"))
      CUtil::AddFileToFolder(g_guiSettings.GetString("musicfiles.recordingpath",false), strSpecial.Mid(11), strReturn);
    else if (strSpecial.Left(12).Equals("$SCREENSHOTS"))
      CUtil::AddFileToFolder(g_guiSettings.GetString("pictures.screenshotpath",false), strSpecial.Mid(12), strReturn);
    else if (strSpecial.Left(10).Equals("$PLAYLISTS"))
      CUtil::AddFileToFolder(g_guiSettings.GetString("system.playlistspath",false), strSpecial.Mid(10), strReturn);
    else if (strSpecial.Left(15).Equals("$MUSICPLAYLISTS"))
    {
      if (g_guiSettings.GetString("system.playlistspath",false) == "")
        strReturn = "";
      else
        CUtil::AddFileToFolder(MusicPlaylistsLocation(), strSpecial.Mid(15), strReturn);
    }
    else if (strSpecial.Left(15).Equals("$VIDEOPLAYLISTS"))
    {
      if (g_guiSettings.GetString("system.playlistspath",false) == "")
        strReturn = "";
      else
        CUtil::AddFileToFolder(VideoPlaylistsLocation(), strSpecial.Mid(15), strReturn);
    }
    else if (strSpecial.Left(7).Equals("$CDRIPS"))
      CUtil::AddFileToFolder(g_guiSettings.GetString("cddaripper.path"), strSpecial.Mid(7), strReturn);
  }
  /*
  if (strReturn.IsEmpty())
    CLog::Log(LOGERROR,"Invalid special directory token: %s",strSpecial.c_str());
  */
  return strReturn;
}

CStdString CUtil::MusicPlaylistsLocation()
{
  CStdString strReturn;
  CUtil::AddFileToFolder(g_guiSettings.GetString("system.playlistspath"), "music", strReturn);
  return strReturn;
}

CStdString CUtil::VideoPlaylistsLocation()
{
  CStdString strReturn;
  CUtil::AddFileToFolder(g_guiSettings.GetString("system.playlistspath"), "video", strReturn);
  return strReturn;
}

void CUtil::DeleteDatabaseDirectoryCache()
{
  WIN32_FIND_DATA wfd;
  memset(&wfd, 0, sizeof(wfd));

  CAutoPtrFind hFind( FindFirstFile("Z:\\db-*.fi", &wfd));
  if (!hFind.isValid())
    return;
  do
  {
    if (!(wfd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY))
    {
      CStdString strFile = "Z:\\";
      strFile += wfd.cFileName;
      DeleteFile(strFile.c_str());
    }
  }
  while (FindNextFile(hFind, &wfd));
}
bool CUtil::IsLeapYear(int iLYear, int iLMonth, int iLTag, int &iMonMax, int &iWeekDay)  // GeminiServer
{
	// R�ckgabewert: FALSE, wenn kein Schaltjahr,   TRUE, wenn Schaltjahr
	bool ret_value;
	int iIsLeapYear;
	if(iLYear%4 == 0 && (iLYear%100 != 0 || iLYear %400 == 0))
	{
		ret_value=TRUE;
		iIsLeapYear = 1;
	}
	else
	{
		ret_value=FALSE;
		iIsLeapYear = 0;
	}
	switch(iLMonth)
		{
			case 1:	iMonMax = 31 ;	
				break;
			case 2:	iMonMax = 28+iIsLeapYear; 
				break;
			case 3:	iMonMax = 31;
				break;
			case 4:	iMonMax = 30;
				break;
			case 5:	iMonMax = 31;
				break;
			case 6:	iMonMax = 30;
				break;
			case 7:	iMonMax = 31;
				break;
			case 8:	iMonMax = 31;
				break;
			case 9:	iMonMax = 30;
				break;
			case 10: iMonMax = 31;
				break;
			case 11: iMonMax = 30;
				break;
			case 12: iMonMax = 31;
				break;
			default: iMonMax = 31;
		}
	// monat: von 1... 12, tag von 1... 31.
	if(iLMonth <= 2)
	{
		iLMonth += 10;
		--iLYear;
	}
	else iLMonth -= 2;
	iWeekDay = (iLTag+(13*iLMonth-1)/5+iLYear+iLYear/4-iLYear/100+iLYear/400)%7;
	return(ret_value);
}

bool CUtil::SetSysDateTimeYear(int iYear, int iMonth, int iDay, int iHour, int iMinute)
{
 	TIME_ZONE_INFORMATION tziNew;
	SYSTEMTIME CurTime;
	SYSTEMTIME NewTime;
	GetLocalTime(&CurTime);
	GetLocalTime(&NewTime);
	int iRescBiases, iHourUTC;
  int iMinuteNew;
	
  DWORD dwRet = GetTimeZoneInformation(&tziNew);  // Get TimeZone Informations 
	float iGMTZone = (float(tziNew.Bias)/(60));     // Calc's the GMT Time
  
  CLog::Log(LOGDEBUG, "------------ TimeZone -------------");
  CLog::Log(LOGDEBUG, "-      GMT Zone: GMT %.1f",iGMTZone);
	CLog::Log(LOGDEBUG, "-          Bias: %i minutes",tziNew.Bias);
  CLog::Log(LOGDEBUG, "-  DaylightBias: %i",tziNew.DaylightBias);
	CLog::Log(LOGDEBUG, "-  StandardBias: %i",tziNew.StandardBias);
  
  switch (dwRet)
  {
    case TIME_ZONE_ID_STANDARD:
      {
        iRescBiases   = tziNew.Bias + tziNew.StandardBias;
        CLog::Log(LOGDEBUG, "-   Timezone ID: 1, Standart");
      }
      break;
    case TIME_ZONE_ID_DAYLIGHT:
      {
        iRescBiases   = tziNew.Bias + tziNew.StandardBias + tziNew.DaylightBias;
        CLog::Log(LOGDEBUG, "-   Timezone ID: 2, Daylight");
      }
      break;
    case TIME_ZONE_ID_UNKNOWN:
      {
        iRescBiases   = tziNew.Bias + tziNew.StandardBias;
        CLog::Log(LOGDEBUG, "-   Timezone ID: 0, Unknown");
      }
      break;
    case TIME_ZONE_ID_INVALID:
      {
        iRescBiases   = tziNew.Bias + tziNew.StandardBias;
        CLog::Log(LOGDEBUG, "-   Timezone ID: Invalid");
      }
      break;
    default: 
      iRescBiases   = tziNew.Bias + tziNew.StandardBias;
  }
    CLog::Log(LOGDEBUG, "--------------- END ---------------");
	
  // Calculation
  iHourUTC = GMTZoneCalc(iRescBiases, iHour, iMinute, iMinuteNew);
  iMinute = iMinuteNew;
  if(iHourUTC <0)
  {
    iDay = iDay - 1;
    iHourUTC =iHourUTC + 24;
  }
  if(iHourUTC >23)
  {
    iDay = iDay + 1;
    iHourUTC =iHourUTC - 24;
  }
  
  // Set the New-,Detected Time Values to System Time!
  NewTime.wYear		= (WORD)iYear;    
	NewTime.wMonth	= (WORD)iMonth;
	NewTime.wDay		= (WORD)iDay;	
	NewTime.wHour		= (WORD)iHourUTC;
  NewTime.wMinute	= (WORD)iMinute;

	FILETIME stNewTime, stCurTime;
	SystemTimeToFileTime(&NewTime, &stNewTime);
	SystemTimeToFileTime(&CurTime, &stCurTime);
	bool bReturn=NtSetSystemTime(&stNewTime, &stCurTime); //NtSetSystemTime(IN PLARGE_INTEGER SystemTime, OUT PLARGE_INTEGER PreviousTime OPTIONAL );
  return bReturn;
}
int CUtil::GMTZoneCalc(int iRescBiases, int iHour, int iMinute, int &iMinuteNew)
{
  int iHourUTC, iTemp;
  iMinuteNew = iMinute;
  iTemp = iRescBiases/60;
  
  if (iRescBiases == 0 )return iHour;   // GMT Zone 0, no need calculate
  if (iRescBiases > 0)
    iHourUTC = iHour + abs(iTemp);
  else 
    iHourUTC = iHour - abs(iTemp);

  if ((iTemp*60) != iRescBiases)
  { 
    if (iRescBiases > 0)
      iMinuteNew = iMinute + abs(iTemp*60 - iRescBiases);
    else
      iMinuteNew = iMinute - abs(iTemp*60 - iRescBiases);
    
    if (iMinuteNew >= 60)
    {
      iMinuteNew = iMinuteNew -60;
      iHourUTC = iHourUTC + 1;
    }
    else if (iMinuteNew < 0)
    {
      iMinuteNew = iMinuteNew +60;
      iHourUTC = iHourUTC - 1;
    }
  }
  return iHourUTC;
}
bool CUtil::XboxAutoDetectionPing(bool bRefresh, CStdString strFTPUserName, CStdString strFTPPass, CStdString strNickName, int iFTPPort, CStdString &strHasClientIP, CStdString &strHasClientInfo, CStdString &strNewClientIP, CStdString &strNewClientInfo )
{
  //GeminiServer
  CStdString strWorkTemp;
  CStdString strSendMessage = "ping\0";
  CStdString strReceiveMessage = "ping";
  int iUDPPort = 4905;
  char  sztmp[512], szTemp[512];
	static int	udp_server_socket, inited=0;
	int  cliLen, t1,t2,t3,t4, init_counter=0, life=0;
  struct sockaddr_in	server;
  struct sockaddr_in	cliAddr;
  struct timeval timeout={0,500};
  XNADDR xna;
	DWORD dwState;
  fd_set readfds; 
  bool bState= false;
	if( ( !inited )  || ( bRefresh ) )
	{
		dwState = XNetGetTitleXnAddr(&xna);
		XNetInAddrToString(xna.ina,(char *)strWorkTemp.c_str(),64);

		// Get IP address
		sscanf( (char *)strWorkTemp.c_str(), "%d.%d.%d.%d", &t1, &t2, &t3, &t4 );
    if( !t1 ) return false;
    cliLen = sizeof( cliAddr);
    if( !inited ) 
    {
      int tUDPsocket  = socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP);
	    char value      = 1;
	    setsockopt( tUDPsocket, SOL_SOCKET, SO_BROADCAST, &value, value );
	    struct sockaddr_in addr;
	    memset(&(addr),0,sizeof(addr));
	    addr.sin_family       = AF_INET; 
	    addr.sin_addr.s_addr  = INADDR_ANY;
	    addr.sin_port         = htons(iUDPPort);
	    bind(tUDPsocket,(struct sockaddr *)(&addr),sizeof(addr));
      udp_server_socket = tUDPsocket;
      inited = 1;
    }
    FD_ZERO(&readfds);
		FD_SET(udp_server_socket, &readfds);
		life = select( 0,&readfds, NULL, NULL, &timeout ); 
		if (life == -1 )  return false;
    memset(&(server),0,sizeof(server));
		server.sin_family           = AF_INET;
		server.sin_addr.S_un.S_addr = INADDR_BROADCAST;
		server.sin_port             = htons(iUDPPort);	
    sendto(udp_server_socket,(char *)strSendMessage.c_str(),5,0,(struct sockaddr *)(&server),sizeof(server));
	}
	FD_ZERO(&readfds);
	FD_SET(udp_server_socket, &readfds);
	life = select( 0,&readfds, NULL, NULL, &timeout );
  if (life == 0 ) // Do we have a Ping able xbox ? 0:no 1:yes
  {
    strNewClientIP =""; 
    strNewClientInfo ="";
    g_infoManager.SetAutodetectedXbox(false);
  }
  while( life )
	{
    recvfrom(udp_server_socket, sztmp, 512, 0,(struct sockaddr *) &cliAddr, &cliLen); 
    strWorkTemp.Format("%s",sztmp);
    if( strWorkTemp == strReceiveMessage )
		{
      strWorkTemp.Format("%s;%s;%s;%d;%d\r\n\0",strNickName.c_str(),strFTPUserName.c_str(),strFTPPass.c_str(),iFTPPort,0 );
      sendto(udp_server_socket,(char *)strWorkTemp.c_str(),strlen((char *)strWorkTemp.c_str())+1,0,(struct sockaddr *)(&cliAddr),sizeof(cliAddr));
      strWorkTemp.Format("%d.%d.%d.%d",cliAddr.sin_addr.S_un.S_un_b.s_b1,cliAddr.sin_addr.S_un.S_un_b.s_b2,cliAddr.sin_addr.S_un.S_un_b.s_b3,cliAddr.sin_addr.S_un.S_un_b.s_b4 );

			bool bPing = ( bool )false; // Check if we have this client in our list already, and if not respond with a ping // todo: a code to check the list of other clients
			if( bPing ) sendto(udp_server_socket,strSendMessage.c_str(),5,0,(struct sockaddr *)(&cliAddr),sizeof(cliAddr));  
		}
		else
		{
      sprintf( szTemp, "%d.%d.%d.%d", cliAddr.sin_addr.S_un.S_un_b.s_b1,cliAddr.sin_addr.S_un.S_un_b.s_b2,cliAddr.sin_addr.S_un.S_un_b.s_b3,cliAddr.sin_addr.S_un.S_un_b.s_b4 );
      if (strHasClientIP != szTemp && strHasClientInfo != strWorkTemp)
      {
        strHasClientIP = szTemp, strHasClientInfo  = strWorkTemp;
        if (!strHasClientIP.IsEmpty()&& !strHasClientInfo.IsEmpty())
        {
          strNewClientIP = szTemp;        //This is the Client IP Adress!
          strNewClientInfo = strWorkTemp; // This is the Client Informations!
          bState = true;
        } //todo: add it to a list of clients after parsing out user id, password, port, boost capable, etc.
      }
      else
        g_infoManager.SetAutodetectedXbox(true);
		}
		timeout.tv_sec=0;
		timeout.tv_usec = 5000;
		FD_ZERO(&readfds);
		FD_SET(udp_server_socket, &readfds);
		life = select( 0,&readfds, NULL, NULL, &timeout );
	}
  return bState;
}
bool CUtil::XboxAutoDetection() // GeminiServer: Xbox Autodetection!
{
  if (g_guiSettings.GetBool("autodetect.onoff"))
  {
    static DWORD pingTimer = 0;
    if( timeGetTime() - pingTimer < (DWORD)g_advancedSettings.m_autoDetectPingTime * 1000)
      return false;
    pingTimer = timeGetTime();

    CStdString strLabel      = g_localizeStrings.Get(1251); // lbl Xbox Autodetection
    CStdString strNickName   = g_guiSettings.GetString("autodetect.nickname");
    CStdString strSysFtpName = g_guiSettings.GetString("servers.ftpserveruser");
    CStdString strSysFtpPw   = g_guiSettings.GetString("servers.ftpserverpassword");

    if(!g_guiSettings.GetBool("Autodetect.senduserpw")) //Send anon login names!
    {
      strSysFtpName = "anonymous"; strSysFtpPw = "anonymous";
    }
    int iSysFtpPort = 21;
    if ( CUtil::XboxAutoDetectionPing(true, strSysFtpName, strSysFtpPw, strNickName, iSysFtpPort,strHasClientIP,strHasClientInfo, strNewClientIP , strNewClientInfo ) )
    {
      //Autodetection String: NickName;FTP_USER;FTP_Password;FTP_PORT;BOOST_MODE
      CStdString strFTPPath, strNickName, strFtpUserName, strFtpPassword, strFtpPort, strBoosMode;
      CStdStringArray arSplit; 
      StringUtils::SplitString(strNewClientInfo,";", arSplit);
      if ((int)arSplit.size() > 1)
      {
        strNickName     = arSplit[0].c_str();
        strFtpUserName  = arSplit[1].c_str();
        strFtpPassword  = arSplit[2].c_str();
        strFtpPort      = arSplit[3].c_str();
        strBoosMode     = arSplit[4].c_str();
        strFTPPath.Format("ftp://%s:%s@%s:%s/",strFtpUserName.c_str(),strFtpPassword.c_str(),strHasClientIP.c_str(),strFtpPort.c_str());

        //PopUp Notification (notify antime if a box is found! no switch needed)
        CStdString strtemplbl;
        strtemplbl.Format("%s %s",strNickName, strNewClientIP);
        g_application.m_guiDialogKaiToast.QueueNotification(strLabel, strtemplbl);
        CLog::Log(LOGDEBUG,"%s: %s FTP-Link: %s", strLabel.c_str(), strNickName.c_str(), strFTPPath.c_str());
        
        if (g_guiSettings.GetBool("autodetect.popupinfo")) //PopUP Ask window to connect to the detected XBOX via Filemanger!
        {
          if (CGUIDialogYesNo::ShowAndGetInput(1251, 0, 1257, 0))
          {
            g_infoManager.SetAutodetectedXbox(true);
            CStdString strTemp;
            strNickName.TrimRight(' ');
            strTemp.Format("FTP XBOX (%s)", strNickName.c_str());
            m_gWindowManager.ActivateWindow(WINDOW_FILES, strFTPPath); //Open in MyFiles
          }
        }
      }
    }
    strHasClientIP = strNewClientIP, strHasClientInfo = strNewClientInfo;
  }
  else
  {
    strHasClientIP ="", strHasClientInfo = ""; 
    g_infoManager.SetAutodetectedXbox(false);
  }
  return true;
}
bool CUtil::XboxAutoDetectionGetShare(CShare& share)
{
  if( !strNewClientIP.IsEmpty() && !strNewClientInfo.IsEmpty())
  {
    //Autodetection String: NickName;FTP_USER;FTP_Password;FTP_PORT;BOOST_MODE
    CStdString strFTPPath, strNickName, strFtpUserName, strFtpPassword, strFtpPort, strBoosMode;
    CStdStringArray arSplit; 
    StringUtils::SplitString(strNewClientInfo,";", arSplit);
    if ((int)arSplit.size() > 1)
    {
      strNickName     = arSplit[0].c_str();
      strFtpUserName  = arSplit[1].c_str();
      strFtpPassword  = arSplit[2].c_str();
      strFtpPort      = arSplit[3].c_str();
      strBoosMode     = arSplit[4].c_str();
      strFTPPath.Format("ftp://%s:%s@%s:%s/",strFtpUserName.c_str(),strFtpPassword.c_str(),strNewClientIP.c_str(),strFtpPort.c_str());

      strNickName.TrimRight(' ');
      share.strName.Format("FTP XBOX (%s)", strNickName.c_str());
      share.strPath.Format("%s",strFTPPath.c_str());
    }
    return true;
  }
  return false;
}
bool CUtil::IsFTP(const CStdString& strFile)
{
  if( strFile.Left(6).Equals("ftp://") ) return true;
  else if( strFile.Left(7).Equals("ftpx://") ) return true;
  else return false;
}

bool CUtil::GetFTPServerUserName(int iFTPUserID, CStdString &strFtpUser1, int &iUserMax )
{
  if( !g_application.m_pFileZilla )
    return false;

  class CXFUser*	m_pUser;
  std::vector<CXFUser*> users;
  g_application.m_pFileZilla->GetAllUsers(users);
  iUserMax = users.size();
	if (iUserMax > 0)
	{
		//for (int i = 1 ; i < iUserSize; i++){ delete users[i]; }
    m_pUser = users[iFTPUserID];
    strFtpUser1 = m_pUser->GetName();
    if (strFtpUser1.size() != 0) return true;
    else return false;
	}
  else 
    return false;
}
bool CUtil::SetFTPServerUserPassword(CStdString strFtpUserName, CStdString strFtpUserPassword)
{
  if( !g_application.m_pFileZilla )
    return false;
  
  CStdString strTempUserName;
  class CXFUser*	p_ftpUser;
  std::vector<CXFUser*> v_ftpusers;
  bool bFoundUser = false;
  g_application.m_pFileZilla->GetAllUsers(v_ftpusers);
  int iUserSize = v_ftpusers.size();
	if (iUserSize > 0)
	{
		for (int i = 1 ; i < iUserSize; i++)
    {
      p_ftpUser = v_ftpusers[i-1];
      strTempUserName = p_ftpUser->GetName();
      if (strTempUserName == strFtpUserName) 
      { 
        if (p_ftpUser->SetPassword(strFtpUserPassword.c_str()) != XFS_INVALID_PARAMETERS)
        {
          p_ftpUser->CommitChanges();
          g_guiSettings.SetString("servers.ftpserverpassword",strFtpUserPassword.c_str());
          return true;
        }
        break;
      }
    }
  }
  return false;
}
//strXboxNickNameIn: New NickName to write
//strXboxNickNameOut: Same if it is in NICKNAME Cache
bool CUtil::SetXBOXNickName(CStdString strXboxNickNameIn, CStdString &strXboxNickNameOut)
{
  WCHAR pszNickName[MAX_NICKNAME];
  unsigned int uiSize = MAX_NICKNAME;
  bool bfound= false;
  HANDLE hNickName = XFindFirstNickname(false,pszNickName,MAX_NICKNAME);
  if (hNickName != INVALID_HANDLE_VALUE)
  { do
      {
        strXboxNickNameOut.Format("%ls",pszNickName );
        if (strXboxNickNameIn.Equals(strXboxNickNameOut))
        { 
          bfound = true; 
          break; 
        }
        else if (strXboxNickNameIn.IsEmpty()) strXboxNickNameOut.Format("GeminiServer");
      }while(XFindNextNickname(hNickName,pszNickName,uiSize) != false);
    XFindClose(hNickName);
  }
  if(!bfound) 
  {
    CStdStringW wstrName = strXboxNickNameIn.c_str();
    XSetNickname(wstrName.c_str(), false);
  }
  return true;
}
//strXboxNickNameOut: Will fast receive the last XBOX NICKNAME from Cache
bool CUtil::GetXBOXNickName(CStdString &strXboxNickNameOut)
{
  WCHAR wszXboxNickname[MAX_NICKNAME];
  HANDLE hNickName = XFindFirstNickname( FALSE, wszXboxNickname, MAX_NICKNAME );
	if ( hNickName != INVALID_HANDLE_VALUE )
	{
    strXboxNickNameOut.Format("%ls",wszXboxNickname);
		XFindClose( hNickName ); 
    return true;
	}
  else
  { 
    // it seems to be empty? should we create one? or the user
    strXboxNickNameOut.Format("");
    return false;
  }
  
}

void CUtil::GetRecursiveListing(const CStdString& strPath, CFileItemList& items, const CStdString& strMask)
{
  CFileItemList myItems;
  CDirectory::GetDirectory(strPath,myItems,strMask);
  for (int i=0;i<myItems.Size();++i)
  {
    if (myItems[i]->m_bIsFolder)
      CUtil::GetRecursiveListing(myItems[i]->m_strPath,items,strMask);
    else if (!myItems[i]->IsRAR() && !myItems[i]->IsZIP())
      items.Add(new CFileItem(*myItems[i]));
  }
}

void CUtil::GetRecursiveDirsListing(const CStdString& strPath, CFileItemList& item)
{
  CFileItemList myItems;
  CDirectory::GetDirectory(strPath,myItems,"");
  for (int i=0;i<myItems.Size();++i)
  {
    if (myItems[i]->m_bIsFolder && !myItems[i]->m_strPath.Equals(".."))
    {
      CFileItem* pItem = new CFileItem(*myItems[i]);
      item.Add(pItem);
      CUtil::GetRecursiveListing(myItems[i]->m_strPath,item,"");
    }   
  }
  CLog::Log(LOGDEBUG,"done listing!");
}

void CUtil::ForceForwardSlashes(CStdString& strPath)
{
  int iPos = strPath.ReverseFind('\\');
  while (iPos > 0) 
  {
    strPath.at(iPos) = '/';
    iPos = strPath.ReverseFind('\\');
  }
}

double CUtil::AlbumRelevance(const CStdString& strAlbumTemp1, const CStdString& strAlbum1, const CStdString& strArtistTemp1, const CStdString& strArtist1)
{
  // case-insensitive fuzzy string comparison on the album and artist for relevance
  // weighting is identical, both album and artist are 50% of the total relevance
  // a missing artist means the maximum relevance can only be 0.50
  CStdString strAlbumTemp = strAlbumTemp1;
  strAlbumTemp.MakeLower();
  CStdString strAlbum = strAlbum1;
  strAlbum.MakeLower();
  double fAlbumPercentage = fstrcmp(strAlbumTemp, strAlbum, 0.0f);
  double fArtistPercentage = 0.0f;
  if (!strArtist1.IsEmpty())
  {
    CStdString strArtistTemp = strArtistTemp1;
    strArtistTemp.MakeLower();
    CStdString strArtist = strArtist1;
    strArtist.MakeLower();
    fArtistPercentage = fstrcmp(strArtistTemp, strArtist, 0.0f);
  }
  double fRelevance = fAlbumPercentage * 0.5f + fArtistPercentage * 0.5f;
  return fRelevance;
}

CStdString CUtil::SubstitutePath(const CStdString& strFileName)
{
  // substitutes paths to correct issues with remote playlists containing full paths
  for (unsigned int i = 0; i < g_advancedSettings.m_pathSubstitutions.size(); i++)
  {
    vector<CStdString> vecSplit;
    StringUtils::SplitString(g_advancedSettings.m_pathSubstitutions[i], " , ", vecSplit);

    // something is wrong, go to next substitution
    if (vecSplit.size() != 2)
      continue;

    CStdString strSearch = vecSplit[0];
    CStdString strReplace = vecSplit[1];
    strSearch.Replace(",,",",");
    strReplace.Replace(",,",",");

    if (!CUtil::HasSlashAtEnd(strSearch))
      CUtil::AddSlashAtEnd(strSearch);
    if (!CUtil::HasSlashAtEnd(strReplace))
      CUtil::AddSlashAtEnd(strReplace);

    // if left most characters match the search, replace them
    int iLen = strSearch.size();
    if (strFileName.Left(iLen).Equals(strSearch))
    {
      // fix slashes
      CStdString strTemp = strFileName.Mid(iLen);
      strTemp.Replace("\\", strReplace.Right(1));
      CStdString strFileNameNew = strReplace + strTemp;
      //CLog::Log(LOGDEBUG,"CUtil::SubstitutePath, new filename: %s", strFileNameNew.c_str());
      return strFileNameNew;
    }
  }

  // nothing matches, return original string
  return strFileName;
}

bool CUtil::MakeShortenPath(CStdString StrInput, CStdString& StrOutput, int iTextMaxLength)
{
  int iStrInputSize = StrInput.size(); 
  if((iStrInputSize <= 0) || (iTextMaxLength >= iStrInputSize)) 
    return false;
  
  char cDelim = '\0';
  int nGreaterDelim, nPos;

  nPos = StrInput.find_last_of( '\\' );
  if ( nPos >= 0 ) 
    cDelim = '\\';
  else
  {
    nPos = StrInput.find_last_of( '/' );
    if ( nPos >= 0 )
      cDelim = '/';
  }
  if ( cDelim == '\0' )
    return false;
  
  if (nPos == StrInput.size() - 1)
  {
    StrInput.erase(StrInput.size() - 1);
    nPos = StrInput.find_last_of( cDelim );
  }
  while( iTextMaxLength < iStrInputSize )
  {
    nPos = StrInput.find_last_of( cDelim, nPos );
    nGreaterDelim = nPos;
    if ( nPos >= 0 ) nPos = StrInput.find_last_of( cDelim, nPos - 1 );
    else break;
    if ( nPos < 0 ) break;
    if ( nGreaterDelim > nPos ) StrInput.replace( nPos + 1, nGreaterDelim - nPos - 1, ".." );
    iStrInputSize = StrInput.size();
  }
  // replace any additional /../../ with just /../ if necessary
  CStdString replaceDots;
  replaceDots.Format("..%c..", cDelim);
  while (StrInput.size() > (unsigned int)iTextMaxLength)
    if (!StrInput.Replace(replaceDots, ".."))
      break;
  // finally, truncate our string to force inside our max text length,
  // replacing the last 2 characters with ".."

  // eg end up with:
  // "smb://../Playboy Swimsuit Cal.."
  if (iTextMaxLength > 2 && StrInput.size() > (unsigned int)iTextMaxLength)
  {
    StrInput = StrInput.Left(iTextMaxLength - 2);
    StrInput += "..";
  }
  StrOutput = StrInput;
  return true;
}

float CUtil::CurrentCpuUsage()
{
  return 0.0f;
/*
  float fCpuUsage = -1.0;
  
  if (!m_hCurrentCpuUsage)
  {
    DmOpenPerformanceCounter("% CPU:total", &m_hCurrentCpuUsage);
  }
  
  if (m_hCurrentCpuUsage)
  {
    DM_COUNTDATA data;

    DmQueryPerformanceCounterHandle(m_hCurrentCpuUsage, DMCOUNT_PRATIO, &data);
    fCpuUsage = (float)data.CountValue.LowPart / (float)data.RateValue.LowPart * 100.0f;
  }
  
  return fCpuUsage;
*/
}

bool CUtil::SupportsFileOperations(const CStdString& strPath)
{
  // currently only hd and smb support delete and rename
  if (IsHD(strPath))
    return true;
  if (IsSmb(strPath))
    return true;
  if (IsStack(strPath))
  {
    CStackDirectory dir;
    return SupportsFileOperations(dir.GetFirstStackedFile(strPath));
  }
  if (IsMemCard(strPath))
    return true;
  return false;
}

CStdString CUtil::GetCachedAlbumThumb(const CStdString& album, const CStdString& path)
{
  if (album.IsEmpty())
    return GetCachedMusicThumb("unknown"+path);
  return GetCachedMusicThumb(album+path);
}

CStdString CUtil::GetCachedMusicThumb(const CStdString& path)
{
  Crc32 crc;
  CStdString noSlashPath(path);
  RemoveSlashAtEnd(noSlashPath);
  crc.ComputeFromLowerCase(noSlashPath);
  CStdString thumb;
  thumb.Format("%s\\%x.tbn", g_settings.GetMusicThumbFolder().c_str(), crc);
  return thumb;
}

void CUtil::GetSkinThemes(std::vector<CStdString>& vecTheme)
{ 
  CStdString strPath;
  CUtil::AddFileToFolder(g_graphicsContext.GetMediaDir(),"media",strPath);
  CHDDirectory directory;
  CFileItemList items;
  directory.GetDirectory(strPath, items);
  // Search for Themes in the Current skin!
  for (int i = 0; i < items.Size(); ++i)
  {
    CFileItem* pItem = items[i];
    if (!pItem->m_bIsFolder)
    {
      CStdString strExtension;
      CUtil::GetExtension(pItem->m_strPath, strExtension);
      if (strExtension == ".xpr" && pItem->GetLabel().CompareNoCase("Textures.xpr"))
      {
        CStdString strLabel = pItem->GetLabel();
        vecTheme.push_back(strLabel.Mid(0, strLabel.size() - 4));
      }
    }
  }
  sort(vecTheme.begin(), vecTheme.end(), sortstringbyname());
}
bool CUtil::PWMControl(CStdString strRGBa, CStdString strRGBb, CStdString strTransition, int iTrTime)
{
  ILEDSmartxxRGB* s_XXrbg = new ILEDSmartxxRGB();
   if(!s_XXrbg->IsRunning())
     s_XXrbg->Start();
   s_XXrbg->SetRGBState(strRGBa,strRGBb, strTransition, iTrTime);
   return true;
}


bool CUtil::LoadMusicTag(CFileItem *pItem)
{
  if (!pItem->IsAudio()) return false;
  CMusicInfoLoader musicInfoLoader;
  return musicInfoLoader.LoadItem(pItem);
}

bool CUtil::LoadMusicTag(CPlayList::CPlayListItem *playListItem)
{
  CFileItem* pItem = new CFileItem(playListItem->m_strPath, false);
  if (CUtil::LoadMusicTag(pItem))
  {
    // apply missing infomration - tag, duration, and thumb
    playListItem->SetMusicTag(pItem->m_musicInfoTag);
    playListItem->SetDuration(pItem->m_musicInfoTag.GetDuration());
    playListItem->SetThumbnailImage(pItem->GetThumbnailImage());
    return true;
  }
  return false;
}