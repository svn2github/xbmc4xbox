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

#include "stdafx.h"
#include "SystemInfo.h"
#ifndef _LINUX
#include <conio.h>
#else
#include <sys/utsname.h>
#endif
#include "utils/HTTP.h"
#include "utils/GUIInfoManager.h"
#include "Network.h"
#include "Application.h"
CSysInfo g_sysinfo;

void CBackgroundSystemInfoLoader::GetInformation()
{
  CSysInfo *callback = (CSysInfo *)m_callback;
  //Request always
  callback->m_systemuptime = callback->GetSystemUpTime(false);
  callback->m_systemtotaluptime = callback->GetSystemUpTime(true);
  callback->m_InternetState = callback->GetInternetState();
#ifdef _LINUX
  callback->m_videoencoder    = callback->GetVideoEncoder();
  callback->m_xboxversion     = callback->GetXBVerInfo();
  callback->m_cpufrequency    = callback->GetCPUFreqInfo();
  callback->m_kernelversion   = callback->GetKernelVersion();
  callback->m_macadress       = callback->GetMACAddress();
  callback->m_bRequestDone = true;
#endif
}

const char *CSysInfo::TranslateInfo(DWORD dwInfo)
{
  switch(dwInfo)
  {
  case SYSTEM_VIDEO_ENCODER_INFO:
    if (m_bRequestDone) return m_videoencoder;
    else return CInfoLoader::BusyInfo(dwInfo);
    break;
  case NETWORK_MAC_ADDRESS:
    if (m_bRequestDone) return m_macadress;
    else return CInfoLoader::BusyInfo(dwInfo);
    break;
  case SYSTEM_KERNEL_VERSION:
    if (m_bRequestDone) return m_kernelversion;
    else return CInfoLoader::BusyInfo(dwInfo);
    break;
  case SYSTEM_CPUFREQUENCY:
    if (m_bRequestDone) return m_cpufrequency;
    else return CInfoLoader::BusyInfo(dwInfo);
    break;
  case SYSTEM_UPTIME:
    if (!m_systemuptime.IsEmpty()) return m_systemuptime;
    else return CInfoLoader::BusyInfo(dwInfo);
  case SYSTEM_TOTALUPTIME:
     if (!m_systemtotaluptime.IsEmpty()) return m_systemtotaluptime;
    else return CInfoLoader::BusyInfo(dwInfo);
  case SYSTEM_INTERNET_STATE:
    if (!m_InternetState.IsEmpty())return m_InternetState;
    else return g_localizeStrings.Get(503); //Busy text

  default:
    return g_localizeStrings.Get(503); //Busy text
  }
}
DWORD CSysInfo::TimeToNextRefreshInMs()
{
  // request every 15 seconds
  return 15000;
}
void CSysInfo::Reset()
{
  m_bInternetState = false;
  m_InternetState = "";
}

CSysInfo::CSysInfo(void) : CInfoLoader("sysinfo")
{
}

CSysInfo::~CSysInfo()
{
}

bool CSysInfo::GetDiskSpace(const CStdString drive,int& iTotal, int& iTotalFree, int& iTotalUsed, int& iPercentFree, int& iPercentUsed)
{
  CStdString driveName = drive + ":\\";
  ULARGE_INTEGER total, totalFree, totalUsed;

  if (drive.IsEmpty() || drive.Equals("*")) //All Drives
  {
    ULARGE_INTEGER totalC, totalFreeC;
    ULARGE_INTEGER totalE, totalFreeE;
    ULARGE_INTEGER totalF, totalFreeF;
    ULARGE_INTEGER totalG, totalFreeG;
    ULARGE_INTEGER totalX, totalFreeX;
    ULARGE_INTEGER totalY, totalFreeY;
    ULARGE_INTEGER totalZ, totalFreeZ;

    BOOL bC = GetDiskFreeSpaceEx("C:\\", NULL, &totalC, &totalFreeC);
    BOOL bE = GetDiskFreeSpaceEx("E:\\", NULL, &totalE, &totalFreeE);
    BOOL bF = GetDiskFreeSpaceEx("F:\\", NULL, &totalF, &totalFreeF);
    BOOL bG = GetDiskFreeSpaceEx("G:\\", NULL, &totalG, &totalFreeG);
    BOOL bX = GetDiskFreeSpaceEx("X:\\", NULL, &totalX, &totalFreeX);
    BOOL bY = GetDiskFreeSpaceEx("Y:\\", NULL, &totalY, &totalFreeY);
    BOOL bZ = GetDiskFreeSpaceEx("Z:\\", NULL, &totalZ, &totalFreeZ);

    total.QuadPart = (bC?totalC.QuadPart:0)+
      (bE?totalE.QuadPart:0)+
      (bF?totalF.QuadPart:0)+
      (bG?totalG.QuadPart:0)+
      (bX?totalX.QuadPart:0)+
      (bY?totalY.QuadPart:0)+
      (bZ?totalZ.QuadPart:0);
    totalFree.QuadPart = (bC?totalFreeC.QuadPart:0)+
      (bE?totalFreeE.QuadPart:0)+
      (bF?totalFreeF.QuadPart:0)+
      (bG?totalFreeG.QuadPart:0)+
      (bX?totalFreeX.QuadPart:0)+
      (bY?totalFreeY.QuadPart:0)+
      (bZ?totalFreeZ.QuadPart:0);

    iTotal = (int)(total.QuadPart/MB);
    iTotalFree = (int)(totalFree.QuadPart/MB);
    iTotalUsed = (int)((total.QuadPart - totalFree.QuadPart)/MB);

    totalUsed.QuadPart = total.QuadPart - totalFree.QuadPart;
    iPercentUsed = (int)(100.0f * totalUsed.QuadPart/total.QuadPart + 0.5f);
    iPercentFree = 100 - iPercentUsed;
    return true;
  }
  else if ( GetDiskFreeSpaceEx(driveName.c_str(), NULL, &total, &totalFree))
  {
    iTotal = (int)(total.QuadPart/MB);
    iTotalFree = (int)(totalFree.QuadPart/MB);
    iTotalUsed = (int)((total.QuadPart - totalFree.QuadPart)/MB);

    totalUsed.QuadPart = total.QuadPart - totalFree.QuadPart;
    iPercentUsed = (int)(100.0f * totalUsed.QuadPart/total.QuadPart + 0.5f);
    iPercentFree = 100 - iPercentUsed;
    return true;
  }
  return false;
}

double CSysInfo::GetCPUFrequency()
{
#if defined (_LINUX)
  return double (g_cpuInfo.getCPUFrequency());
#else
  return 0;
#endif
}

CStdString CSysInfo::GetMACAddress()
{
  CStdString strMacAddress;

#if defined(HAS_LINUX_NETWORK)
  CNetworkInterface* iface = g_application.getNetwork().GetFirstConnectedInterface();
  if (iface)
    strMacAddress.Format("%s: %s", g_localizeStrings.Get(149), iface->GetMacAddress());
#endif
  return strMacAddress;
}

CStdString CSysInfo::GetVideoEncoder()
{
  return "GPU: " + g_graphicsContext.getScreenSurface()->GetGLRenderer();
}

CStdString CSysInfo::GetXBVerInfo()
{
  return "CPU: " + g_cpuInfo.getCPUModel();
}

CStdString CSysInfo::GetKernelVersion()
{
#if defined (_LINUX)
  struct utsname un;
  if (uname(&un)==0)
  {
    CStdString strKernel;
    strKernel.Format("%s %s %s %s %s",g_localizeStrings.Get(13283), un.sysname, un.release, un.version, un.machine);
    return strKernel;
  }

  return "";
#else
  return "";
#endif
}

CStdString CSysInfo::GetCPUFreqInfo()
{
  CStdString strCPUFreq;
  double CPUFreq = GetCPUFrequency();
  strCPUFreq.Format("%s %4.2fMHz", g_localizeStrings.Get(13284), CPUFreq);
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
      strDrive = g_localizeStrings.Get(20161);
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
        strRet.Format("%s: %i MB %s", strDrive, totalUsed, g_localizeStrings.Get(20162));
        break;
      case SYSTEM_TOTAL_SPACE:
      case SYSTEM_TOTAL_SPACE_C:
      case SYSTEM_TOTAL_SPACE_E:
      case SYSTEM_TOTAL_SPACE_F:
      case SYSTEM_TOTAL_SPACE_G:
        strRet.Format("%s: %i MB %s", strDrive, total, g_localizeStrings.Get(20161));
        break;
      case SYSTEM_FREE_SPACE_PERCENT:
      case SYSTEM_FREE_SPACE_PERCENT_C:
      case SYSTEM_FREE_SPACE_PERCENT_E:
      case SYSTEM_FREE_SPACE_PERCENT_F:
      case SYSTEM_FREE_SPACE_PERCENT_G:
        strRet.Format("%s: %i%% %s", strDrive, percentFree, g_localizeStrings.Get(160));
        break;
      case SYSTEM_USED_SPACE_PERCENT:
      case SYSTEM_USED_SPACE_PERCENT_C:
      case SYSTEM_USED_SPACE_PERCENT_E:
      case SYSTEM_USED_SPACE_PERCENT_F:
      case SYSTEM_USED_SPACE_PERCENT_G:
        strRet.Format("%s: %i%% %s", strDrive, percentused, g_localizeStrings.Get(20162));
        break;
      }
    }
  }
  else
  {
    if (shortText)
      strRet = "N/A";
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
  CStdString strLabel;
  int iInputMinutes, iMinutes,iHours,iDays;

  if(bTotalUptime)
  {
    //Total Uptime
    strLabel = g_localizeStrings.Get(12394);
    iInputMinutes = g_stSettings.m_iSystemTimeTotalUp + ((int)(timeGetTime() / 60000));
  }
  else
  {
    //Current UpTime
    strLabel = g_localizeStrings.Get(12390);
    iInputMinutes = (int)(timeGetTime() / 60000);
  }

  SystemUpTime(iInputMinutes,iMinutes, iHours, iDays);
  if (iDays > 0)
  {
    strSystemUptime.Format("%s: %i %s, %i %s, %i %s",
      strLabel,
      iDays,g_localizeStrings.Get(12393),
      iHours,g_localizeStrings.Get(12392),
      iMinutes, g_localizeStrings.Get(12391));
  }
  else if (iDays == 0 && iHours >= 1 )
  {
    strSystemUptime.Format("%s: %i %s, %i %s",
      strLabel,
      iHours,g_localizeStrings.Get(12392),
      iMinutes, g_localizeStrings.Get(12391));
  }
  else if (iDays == 0 && iHours == 0 &&  iMinutes >= 0)
  {
    strSystemUptime.Format("%s: %i %s",
      strLabel,
      iMinutes, g_localizeStrings.Get(12391));
  }
  return strSystemUptime;
}

CStdString CSysInfo::GetInternetState()
{
  // Internet connection state!
  CHTTP http;
  CStdString strInetCon;
  m_bInternetState = http.IsInternet();
  if (m_bInternetState)
    strInetCon.Format("%s %s",g_localizeStrings.Get(13295), g_localizeStrings.Get(13296));
  else if (http.IsInternet(false))
    strInetCon.Format("%s %s",g_localizeStrings.Get(13295), g_localizeStrings.Get(13274));
  else // NOT Connected to the Internet!
    strInetCon.Format("%s %s",g_localizeStrings.Get(13295), g_localizeStrings.Get(13297));
  return strInetCon;
}
