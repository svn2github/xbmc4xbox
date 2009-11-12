/*
 *      Copyright (C) 2005-2008 Team XBMC
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

#include "system.h"
#include "SystemInfo.h"
#ifndef _LINUX
#include <conio.h>
#else
#include <sys/utsname.h>
#endif
#include "utils/GUIInfoManager.h"
#include "FileSystem/FileCurl.h"
#include "Network.h"
#include "Application.h"
#include "GraphicContext.h"
#include "WindowingFactory.h"
#include "Settings.h"
#include "LocalizeStrings.h"
#include "CPUInfo.h"
#include "utils/TimeUtils.h"

CSysInfo g_sysinfo;

bool CSysInfo::DoWork()
{
  //Request always
  m_systemuptime      = GetSystemUpTime(false);
  m_systemtotaluptime = GetSystemUpTime(true);
  m_InternetState     = GetInternetState();
  if (!m_bRequestDone)
  { // request once
    m_videoencoder      = GetVideoEncoder();
    m_xboxversion       = GetXBVerInfo();
    m_cpufrequency      = GetCPUFreqInfo();
    m_kernelversion     = GetKernelVersion();
    m_macadress         = GetMACAddress();
  }
  m_bRequestDone = true;
  return true;
}

CStdString CSysInfo::TranslateInfo(int info) const
{
  switch(info)
  {
  case SYSTEM_VIDEO_ENCODER_INFO:
    if (m_bRequestDone) return m_videoencoder;
    else return CInfoLoader::BusyInfo(info);
    break;
  case NETWORK_MAC_ADDRESS:
    if (m_bRequestDone) return m_macadress;
    else return CInfoLoader::BusyInfo(info);
    break;
  case SYSTEM_KERNEL_VERSION:
    if (m_bRequestDone) return m_kernelversion;
    else return CInfoLoader::BusyInfo(info);
    break;
  case SYSTEM_CPUFREQUENCY:
    if (m_bRequestDone) return m_cpufrequency;
    else return CInfoLoader::BusyInfo(info);
    break;
  case SYSTEM_UPTIME:
    if (!m_systemuptime.IsEmpty()) return m_systemuptime;
    else return CInfoLoader::BusyInfo(info);
  case SYSTEM_TOTALUPTIME:
     if (!m_systemtotaluptime.IsEmpty()) return m_systemtotaluptime;
    else return CInfoLoader::BusyInfo(info);
  case SYSTEM_INTERNET_STATE:
    if (!m_InternetState.IsEmpty())return m_InternetState;
    else return g_localizeStrings.Get(503); //Busy text

  default:
    return g_localizeStrings.Get(503); //Busy text
  }
}

void CSysInfo::Reset()
{
  m_bInternetState = false;
  m_InternetState = "";
}

CSysInfo::CSysInfo(void) : CInfoLoader(15 * 1000)
{
}

CSysInfo::~CSysInfo()
{
}

bool CSysInfo::GetDiskSpace(const CStdString drive,int& iTotal, int& iTotalFree, int& iTotalUsed, int& iPercentFree, int& iPercentUsed)
{
  bool bRet= false;
  ULARGE_INTEGER ULTotal= { { 0 } };
  ULARGE_INTEGER ULTotalFree= { { 0 } };

  if( !drive.IsEmpty() && !drive.Equals("*") ) 
  { 
#ifdef _WIN32
    UINT uidriveType = GetDriveType(( drive + ":\\" ));
    if(uidriveType != DRIVE_UNKNOWN && uidriveType != DRIVE_NO_ROOT_DIR)
#endif
      bRet= ( 0 != GetDiskFreeSpaceEx( ( drive + ":\\" ), NULL, &ULTotal, &ULTotalFree) );
  }
  else 
  {
    ULARGE_INTEGER ULTotalTmp= { { 0 } };
    ULARGE_INTEGER ULTotalFreeTmp= { { 0 } };
#ifdef _WIN32 
    char* pcBuffer= NULL;
    DWORD dwStrLength= GetLogicalDriveStrings( 0, pcBuffer );
    if( dwStrLength != 0 )
    {
      dwStrLength+= 1;
      pcBuffer= new char [dwStrLength];
      GetLogicalDriveStrings( dwStrLength, pcBuffer );
      int iPos= 0;
      do {
        if( DRIVE_FIXED == GetDriveType( pcBuffer + iPos  ) &&
            GetDiskFreeSpaceEx( ( pcBuffer + iPos ), NULL, &ULTotal, &ULTotalFree ) )
        {
          ULTotalTmp.QuadPart+= ULTotal.QuadPart;
          ULTotalFreeTmp.QuadPart+= ULTotalFree.QuadPart;
        }
        iPos += (strlen( pcBuffer + iPos) + 1 );
      }while( strlen( pcBuffer + iPos ) > 0 );
    }
    delete[] pcBuffer;
#else // for linux and osx
    static const char *drv_letter[] = { "C:\\", "E:\\", "F:\\", "G:\\", "X:\\", "Y:\\", "Z:\\", NULL };
    for( int i = 0; drv_letter[i]; i++)
    {
      if( GetDiskFreeSpaceEx( drv_letter[i], NULL, &ULTotal, &ULTotalFree ) )
      {
        ULTotalTmp.QuadPart+= ULTotal.QuadPart;
        ULTotalFreeTmp.QuadPart+= ULTotalFree.QuadPart;
      }
    }
#endif
    if( ULTotalTmp.QuadPart || ULTotalFreeTmp.QuadPart )
    {
      ULTotal.QuadPart= ULTotalTmp.QuadPart;
      ULTotalFree.QuadPart= ULTotalFreeTmp.QuadPart;
      bRet= true;
    }
  }

  if( bRet )
  {
    iTotal = (int)( ULTotal.QuadPart / MB );
    iTotalFree = (int)( ULTotalFree.QuadPart / MB );
    iTotalUsed = iTotal - iTotalFree;
    iPercentUsed = (int)( 100.0f * ( ULTotal.QuadPart - ULTotalFree.QuadPart ) / ULTotal.QuadPart + 0.5f );
    iPercentFree = 100 - iPercentUsed;
  }

  return bRet;
}

double CSysInfo::GetCPUFrequency()
{
#if defined (_LINUX) || defined(_WIN32)
  return double (g_cpuInfo.getCPUFrequency());
#else
  return 0;
#endif
}

CStdString CSysInfo::GetMACAddress()
{
#if defined(HAS_LINUX_NETWORK)
  CNetworkInterface* iface = g_application.getNetwork().GetFirstConnectedInterface();
  if (iface)
    return iface->GetMacAddress();
#endif
  return "";
}

CStdString CSysInfo::GetVideoEncoder()
{
  return "GPU: " + g_Windowing.GetRenderRenderer();
}

CStdString CSysInfo::GetXBVerInfo()
{
  return "CPU: " + g_cpuInfo.getCPUModel();
}

bool CSysInfo::IsWindowsXP()
{
#ifdef _WIN32
  OSVERSIONINFOEX osvi;
  ZeroMemory(&osvi, sizeof(OSVERSIONINFOEX));
  osvi.dwOSVersionInfoSize = sizeof(osvi);

  if (GetVersionEx((OSVERSIONINFO *)&osvi))
  {
    if (osvi.dwMajorVersion == 5)
      return true;
  }
#endif
  return false;
}

CStdString CSysInfo::GetKernelVersion()
{
#if defined (_LINUX)
  struct utsname un;
  if (uname(&un)==0)
  {
    CStdString strKernel;
    strKernel.Format("%s %s %s %s", un.sysname, un.release, un.version, un.machine);
    return strKernel;
  }

  return "";
#else
  OSVERSIONINFOEX osvi;
  SYSTEM_INFO si;

  ZeroMemory(&si, sizeof(SYSTEM_INFO));
  ZeroMemory(&osvi, sizeof(OSVERSIONINFOEX));

  GetSystemInfo(&si);

  osvi.dwOSVersionInfoSize = sizeof(osvi);
  CStdString strKernel = "Windows ";

  if (GetVersionEx((OSVERSIONINFO *)&osvi))
  {
    if ( osvi.dwMajorVersion == 6 && osvi.dwMinorVersion == 0 )
    {
      if( osvi.wProductType == VER_NT_WORKSTATION )
        strKernel.append("Vista");
      else
        strKernel.append("Server 2008");

      if ( si.wProcessorArchitecture==PROCESSOR_ARCHITECTURE_AMD64 || si.wProcessorArchitecture==PROCESSOR_ARCHITECTURE_IA64)
        strKernel.append(", 64-bit Native");
      else if (si.wProcessorArchitecture==PROCESSOR_ARCHITECTURE_INTEL )
      {
        BOOL bIsWow = FALSE;;
        if(IsWow64Process(GetCurrentProcess(), &bIsWow))
        {
          if (bIsWow)
          {
            GetNativeSystemInfo(&si);
            if (si.wProcessorArchitecture == PROCESSOR_ARCHITECTURE_AMD64 || si.wProcessorArchitecture == PROCESSOR_ARCHITECTURE_IA64)
             strKernel.append(", 64-bit (WoW)");
          }
          else 
          {
            strKernel.append(", 32-bit");
          }
        }
        else
          strKernel.append(", 32-bit");
      }
    }
    else if ( osvi.dwMajorVersion == 5 && osvi.dwMinorVersion == 2 )
    {
      if( osvi.wProductType == VER_NT_WORKSTATION && si.wProcessorArchitecture==PROCESSOR_ARCHITECTURE_AMD64)
      {
        strKernel.append("XP Professional x64 Edition");
      }
    }
    else if ( osvi.dwMajorVersion == 5 && osvi.dwMinorVersion == 1 )
    {
      strKernel.append("XP ");
      if( osvi.wSuiteMask & VER_SUITE_PERSONAL )
        strKernel.append("Home Edition" );
      else 
        strKernel.append("Professional" );
    }
    else if ( osvi.dwMajorVersion == 5 && osvi.dwMinorVersion == 0 )
    {
      strKernel.append("2000");
    }

    if( _tcslen(osvi.szCSDVersion) > 0 )
    {
      strKernel.append(" ");
      strKernel.append(osvi.szCSDVersion);
    }
    CStdString strBuild;
    strBuild.Format(" build %d",osvi.dwBuildNumber);
    strKernel += strBuild;
  }
  return strKernel;
#endif
}

CStdString CSysInfo::GetCPUFreqInfo()
{
  CStdString strCPUFreq;
  double CPUFreq = GetCPUFrequency();
  strCPUFreq.Format("%4.2fMHz", CPUFreq);
  return strCPUFreq;
}

CStdString CSysInfo::GetHddSpaceInfo(int drive, bool shortText)
{
 int percent;
 return GetHddSpaceInfo( percent, drive, shortText);
}

CStdString CSysInfo::GetHddSpaceInfo(int& percent, int drive, bool shortText)
{
  int total, totalFree, totalUsed, percentFree, percentused;
  CStdString strDrive;
  bool bRet=false;
  percent = 0;
  CStdString strRet;
  switch (drive)
  {
    case SYSTEM_FREE_SPACE:
    case SYSTEM_USED_SPACE:
    case SYSTEM_TOTAL_SPACE:
    case SYSTEM_FREE_SPACE_PERCENT:
    case SYSTEM_USED_SPACE_PERCENT:
      bRet = g_sysinfo.GetDiskSpace("",total, totalFree, totalUsed, percentFree, percentused);
      break;
    case LCD_FREE_SPACE_C:
    case SYSTEM_FREE_SPACE_C:
    case SYSTEM_USED_SPACE_C:
    case SYSTEM_TOTAL_SPACE_C:
    case SYSTEM_FREE_SPACE_PERCENT_C:
    case SYSTEM_USED_SPACE_PERCENT_C:
      strDrive = "C";
      bRet = g_sysinfo.GetDiskSpace("C",total, totalFree, totalUsed, percentFree, percentused);
      break;
    case LCD_FREE_SPACE_E:
    case SYSTEM_FREE_SPACE_E:
    case SYSTEM_USED_SPACE_E:
    case SYSTEM_TOTAL_SPACE_E:
    case SYSTEM_FREE_SPACE_PERCENT_E:
    case SYSTEM_USED_SPACE_PERCENT_E:
      strDrive = "E";
      bRet = g_sysinfo.GetDiskSpace("E",total, totalFree, totalUsed, percentFree, percentused);
      break;
    case LCD_FREE_SPACE_F:
    case SYSTEM_FREE_SPACE_F:
    case SYSTEM_USED_SPACE_F:
    case SYSTEM_TOTAL_SPACE_F:
    case SYSTEM_FREE_SPACE_PERCENT_F:
    case SYSTEM_USED_SPACE_PERCENT_F:
      strDrive = "F";
      bRet = g_sysinfo.GetDiskSpace("F",total, totalFree, totalUsed, percentFree, percentused);
      break;
    case LCD_FREE_SPACE_G:
    case SYSTEM_FREE_SPACE_G:
    case SYSTEM_USED_SPACE_G:
    case SYSTEM_TOTAL_SPACE_G:
    case SYSTEM_FREE_SPACE_PERCENT_G:
    case SYSTEM_USED_SPACE_PERCENT_G:
      strDrive = "G";
      bRet = g_sysinfo.GetDiskSpace("G",total, totalFree, totalUsed, percentFree, percentused);
      break;
    case SYSTEM_USED_SPACE_X:
    case SYSTEM_FREE_SPACE_X:
    case SYSTEM_TOTAL_SPACE_X:
      strDrive = "X";
      bRet = g_sysinfo.GetDiskSpace("X",total, totalFree, totalUsed, percentFree, percentused);
      break;
    case SYSTEM_USED_SPACE_Y:
    case SYSTEM_FREE_SPACE_Y:
    case SYSTEM_TOTAL_SPACE_Y:
      strDrive = "Y";
      bRet = g_sysinfo.GetDiskSpace("Y",total, totalFree, totalUsed, percentFree, percentused);
      break;
    case SYSTEM_USED_SPACE_Z:
    case SYSTEM_FREE_SPACE_Z:
    case SYSTEM_TOTAL_SPACE_Z:
      strDrive = "Z";
      bRet = g_sysinfo.GetDiskSpace("Z",total, totalFree, totalUsed, percentFree, percentused);
      break;
  }
  if (bRet)
  {
    if (shortText)
    {
      switch(drive)
      {
        case LCD_FREE_SPACE_C:
        case LCD_FREE_SPACE_E:
        case LCD_FREE_SPACE_F:
        case LCD_FREE_SPACE_G:
          strRet.Format("%iMB", totalFree);
          break;
        case SYSTEM_FREE_SPACE:
        case SYSTEM_FREE_SPACE_C:
        case SYSTEM_FREE_SPACE_E:
        case SYSTEM_FREE_SPACE_F:
        case SYSTEM_FREE_SPACE_G:
        case SYSTEM_FREE_SPACE_X:
        case SYSTEM_FREE_SPACE_Y:
        case SYSTEM_FREE_SPACE_Z:
          percent = percentFree;
          break;
        case SYSTEM_USED_SPACE:
        case SYSTEM_USED_SPACE_C:
        case SYSTEM_USED_SPACE_E:
        case SYSTEM_USED_SPACE_F:
        case SYSTEM_USED_SPACE_G:
        case SYSTEM_USED_SPACE_X:
        case SYSTEM_USED_SPACE_Y:
        case SYSTEM_USED_SPACE_Z:
          percent = percentused;
          break;
      }
    }
    else
    {
      switch(drive)
      {
      case SYSTEM_FREE_SPACE:
      case SYSTEM_FREE_SPACE_C:
      case SYSTEM_FREE_SPACE_E:
      case SYSTEM_FREE_SPACE_F:
      case SYSTEM_FREE_SPACE_G:
      case SYSTEM_FREE_SPACE_X:
      case SYSTEM_FREE_SPACE_Y:
      case SYSTEM_FREE_SPACE_Z:
        if (strDrive.IsEmpty())
          strRet.Format("%i MB %s", totalFree, g_localizeStrings.Get(160));
        else
          strRet.Format("%s: %i MB %s", strDrive, totalFree, g_localizeStrings.Get(160));
        break;
      case SYSTEM_USED_SPACE:
      case SYSTEM_USED_SPACE_C:
      case SYSTEM_USED_SPACE_E:
      case SYSTEM_USED_SPACE_F:
      case SYSTEM_USED_SPACE_G:
      case SYSTEM_USED_SPACE_X:
      case SYSTEM_USED_SPACE_Y:
      case SYSTEM_USED_SPACE_Z:
        if (strDrive.IsEmpty())
          strRet.Format("%i MB %s", totalUsed, g_localizeStrings.Get(20162));
        else
          strRet.Format("%s: %i MB %s", strDrive, totalUsed, g_localizeStrings.Get(20162));
        break;
      case SYSTEM_TOTAL_SPACE:
      case SYSTEM_TOTAL_SPACE_C:
      case SYSTEM_TOTAL_SPACE_E:
      case SYSTEM_TOTAL_SPACE_F:
      case SYSTEM_TOTAL_SPACE_G:
        if (strDrive.IsEmpty())
          strRet.Format("%i MB %s", total, g_localizeStrings.Get(20161));
        else
          strRet.Format("%s: %i MB %s", strDrive, total, g_localizeStrings.Get(20161));
        break;
      case SYSTEM_FREE_SPACE_PERCENT:
      case SYSTEM_FREE_SPACE_PERCENT_C:
      case SYSTEM_FREE_SPACE_PERCENT_E:
      case SYSTEM_FREE_SPACE_PERCENT_F:
      case SYSTEM_FREE_SPACE_PERCENT_G:
        if (strDrive.IsEmpty())
          strRet.Format("%i MB %s", percentFree, g_localizeStrings.Get(160));
        else
          strRet.Format("%s: %i MB %s", strDrive, percentFree, g_localizeStrings.Get(160));
        break;
      case SYSTEM_USED_SPACE_PERCENT:
      case SYSTEM_USED_SPACE_PERCENT_C:
      case SYSTEM_USED_SPACE_PERCENT_E:
      case SYSTEM_USED_SPACE_PERCENT_F:
      case SYSTEM_USED_SPACE_PERCENT_G:
        if (strDrive.IsEmpty())
          strRet.Format("%i MB %s", percentused, g_localizeStrings.Get(20162));
        else
          strRet.Format("%s: %i MB %s", strDrive, percentused, g_localizeStrings.Get(20162));
        break;
      }
    }
  }
  else
  {
    if (shortText)
      strRet = "N/A";
    else if (strDrive.IsEmpty())
      strRet = g_localizeStrings.Get(161);
    else
      strRet.Format("%s: %s", strDrive, g_localizeStrings.Get(161));
  }
  return strRet;
}

bool CSysInfo::SystemUpTime(int iInputMinutes, int &iMinutes, int &iHours, int &iDays)
{
  iMinutes=0;iHours=0;iDays=0;
  iMinutes = iInputMinutes;
  if (iMinutes >= 60) // Hour's
  {
    iHours = iMinutes / 60;
    iMinutes = iMinutes - (iHours *60);
  }
  if (iHours >= 24) // Days
  {
    iDays = iHours / 24;
    iHours = iHours - (iDays * 24);
  }
  return true;
}

CStdString CSysInfo::GetSystemUpTime(bool bTotalUptime)
{
  CStdString strSystemUptime;
  int iInputMinutes, iMinutes,iHours,iDays;

  if(bTotalUptime)
  {
    //Total Uptime
    iInputMinutes = g_stSettings.m_iSystemTimeTotalUp + ((int)(CTimeUtils::GetTimeMS() / 60000));
  }
  else
  {
    //Current UpTime
    iInputMinutes = (int)(CTimeUtils::GetTimeMS() / 60000);
  }

  SystemUpTime(iInputMinutes,iMinutes, iHours, iDays);
  if (iDays > 0)
  {
    strSystemUptime.Format("%i %s, %i %s, %i %s",
      iDays,g_localizeStrings.Get(12393),
      iHours,g_localizeStrings.Get(12392),
      iMinutes, g_localizeStrings.Get(12391));
  }
  else if (iDays == 0 && iHours >= 1 )
  {
    strSystemUptime.Format("%i %s, %i %s",
      iHours,g_localizeStrings.Get(12392),
      iMinutes, g_localizeStrings.Get(12391));
  }
  else if (iDays == 0 && iHours == 0 &&  iMinutes >= 0)
  {
    strSystemUptime.Format("%i %s",
      iMinutes, g_localizeStrings.Get(12391));
  }
  return strSystemUptime;
}

CStdString CSysInfo::GetInternetState()
{
  // Internet connection state!
  XFILE::CFileCurl http;
  m_bInternetState = http.IsInternet();
  if (m_bInternetState)
    return g_localizeStrings.Get(13296);
  else if (http.IsInternet(false))
    return g_localizeStrings.Get(13274);
  else // NOT Connected to the Internet!
    return g_localizeStrings.Get(13297);
}

#if defined(_LINUX) && !defined(__APPLE__)
CStdString CSysInfo::GetLinuxDistro()
{
  CStdString result = "";
  
  FILE* pipe = popen("unset PYTHONHOME; unset PYTHONPATH; /usr/bin/lsb_release -d | cut -f2", "r");
  if (pipe)
  {
    char buffer[256] = {'\0'};
    if (fread(buffer, sizeof(char), sizeof(buffer), pipe) > 0 && !ferror(pipe))
      result = buffer;
    else
      CLog::Log(LOGWARNING, "Unable to determine Linux distribution");
    pclose(pipe);
  }
  
  return result.Trim();
}
#endif

#ifdef _LINUX
CStdString CSysInfo::GetUnameVersion()
{
  CStdString result = "";
  
  FILE* pipe = popen("uname -rs", "r");
  if (pipe)
  {
    char buffer[256] = {'\0'};
    if (fread(buffer, sizeof(char), sizeof(buffer), pipe) > 0 && !ferror(pipe))
      result = buffer;
    else
      CLog::Log(LOGWARNING, "Unable to determine Uname version");
    pclose(pipe);
  }
  
  return result.Trim();
}
#endif

CStdString CSysInfo::GetUserAgent()
{
  CStdString result;
  result = "XBMC/" + g_infoManager.GetLabel(SYSTEM_BUILD_VERSION) + " (";
#if defined(_WIN32)
  result += "Windows; ";
  result += GetKernelVersion();
#elif defined(__APPLE__)
  result += "Mac OS X; ";
  result += GetUnameVersion();
#elif defined(_LINUX)
  result += "Linux; ";
  CStdString distro = GetLinuxDistro();
  if (distro != "")
  {
    result += distro;
    result += "; ";
  }
  result += GetUnameVersion();
#endif
#ifdef SVN_REV
  CStdString strRevision; 
  strRevision.Format("; SVN r%s", SVN_REV);
  result += strRevision;
#endif
  result += "; http://www.xbmc.org)";
  
  return result;
}

bool CSysInfo::IsAppleTV()
{
  bool        result = false;
#if defined(__APPLE__)
  char        buffer[512];
  size_t      len = 512;
  std::string hw_model = "unknown";
  
  if (sysctlbyname("hw.model", &buffer, &len, NULL, 0) == 0)
    hw_model = buffer;
  
  if (hw_model.find("AppleTV") != std::string::npos)
    result = true;
#endif
  return result;
}


