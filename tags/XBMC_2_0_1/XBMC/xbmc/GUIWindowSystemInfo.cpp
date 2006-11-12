/*
******************************************
**    XBOX System/Hardware Info         **
**     01.03.2005 GeminiServer          **
******************************************
Todo/BUG: GeminiServer 10.05.2005

- TODO: Create EEPROM Backup [CreateEEPROMBackup], like ConfigMagic also cfg and TXT [50% done cur_on pending]
- TODO: Need Better routine that checks if the XBOX is Connected to the Internet!

- Exlude from System Info: May Fix Later!   
BUG: HDD Password is show Wrong!! Print HDD Password... [SomeThing Goes Wrong! Need analizing & Fixing!]
BUG: The XBE Region detection Is wrong! Need to Decyrpt the EEPROM!

*/
#include "stdafx.h"
#include "guiwindowsysteminfo.h"
#include "xbox/xkeeprom.h"
#include "utils/GUIInfoManager.h"
#include "utils/SystemInfo.h"
#include "xbox/network.h"
#include "application.h"
#include "utils/HddSmart.h"

#define DEBUG_KEYBOARD
#define DEBUG_MOUSE

CStdString strMplayerVersion;
extern "C" XPP_DEVICE_TYPE XDEVICE_TYPE_IR_REMOTE_TABLE;
#define     XDEVICE_TYPE_IR_REMOTE  (&XDEVICE_TYPE_IR_REMOTE_TABLE)

XKEEPROM* m_pXKEEPROM;
EEPROMDATA  m_EEPROMData;
BOOL  m_EnryptedRegionValid;
BOOL  m_XBOX_EEPROM_Current;
XBOX_VERSION  m_XBOX_Version;
DWORD m_dwlastTime;

char* cTempEEPROMBackUPPath = "Q:\\System\\SystemInfo\\";

CGUIWindowSystemInfo::CGUIWindowSystemInfo(void)
:CGUIWindow(WINDOW_SYSTEM_INFORMATION, "SettingsSystemInfo.xml")
{

}

CGUIWindowSystemInfo::~CGUIWindowSystemInfo(void)
{

}

bool CGUIWindowSystemInfo::GetMPlayerVersion(CStdString& strVersion)
{
  DllLoader* mplayerDll;
  const char* (__cdecl* pMplayerGetVersion)();
  const char* (__cdecl* pMplayerGetCompileDate)();
  const char* (__cdecl* pMplayerGetCompileTime)();

  const char *version = NULL;
  const char *date = NULL;
  const char *btime = NULL;

  mplayerDll = new DllLoader("Q:\\system\\players\\mplayer\\mplayer.dll",true);

  if( mplayerDll->Parse() )
  {   
    if (mplayerDll->ResolveExport("mplayer_getversion", (void**)&pMplayerGetVersion))
      version = pMplayerGetVersion();
    if (mplayerDll->ResolveExport("mplayer_getcompiledate", (void**)&pMplayerGetCompileDate))
      date = pMplayerGetCompileDate();
    if (mplayerDll->ResolveExport("mplayer_getcompiletime", (void**)&pMplayerGetCompileTime))
      btime = pMplayerGetCompileTime();
    if (version && date && btime)
    {
      strVersion.Format("%s (%s - %s)",version, date, btime);
    }
    else if (version)
    {
      strVersion.Format("%s",version);
    }
  }
  delete mplayerDll;
  mplayerDll=NULL;
  return true;
}
void CGUIWindowSystemInfo::BytesToHexStr(LPBYTE SrcBytes, DWORD byteCount, LPSTR DstString, UCHAR Seperator)
{
  USHORT Inc = (Seperator == 0x00)?2:3;
  for (ULONG i=0; i < byteCount; i++)
  {
    if ((UCHAR)*(SrcBytes+i) > 0x0F)
    { itoa((UCHAR)*(SrcBytes+i), DstString+(i*Inc), 16);  }
    else
    { *(DstString+i*Inc) = '0'; itoa((UCHAR)*(SrcBytes+i), DstString+(i*Inc+1), 16);    }
  }

  if (Seperator != 0x00)
  {
    for (ULONG i=1; i < byteCount; i++)
      *(DstString+i*Inc-1) = Seperator;
  }
}

void CGUIWindowSystemInfo::BytesToHexStr(LPBYTE SrcBytes, DWORD byteCount, LPSTR DstString)
{ 
  BytesToHexStr(SrcBytes, byteCount, DstString, 0x00); 
}

bool CGUIWindowSystemInfo::OnAction(const CAction &action)
{
  if (action.wID == ACTION_PREVIOUS_MENU)
  {
    m_gWindowManager.PreviousWindow();
    return true;
  }
  return CGUIWindow::OnAction(action);
}

bool CGUIWindowSystemInfo::OnMessage(CGUIMessage& message)
{
  switch ( message.GetMessage() )
  {
  case GUI_MSG_WINDOW_INIT:
    { 
      m_wszMPlayerVersion[0] = 0;     
      //Get SystemInformations on Init
      CGUIWindow::OnMessage(message);

      //Open Progress Dialog
      CGUIDialogProgress&  pDlgProgress= *((CGUIDialogProgress*)m_gWindowManager.GetWindow(WINDOW_DIALOG_PROGRESS));
      pDlgProgress.SetHeading(g_localizeStrings.Get(10007));
      pDlgProgress.SetLine(0, g_localizeStrings.Get(20185));
      pDlgProgress.SetLine(1, "");
      pDlgProgress.SetLine(2, g_localizeStrings.Get(20186));
      pDlgProgress.SetPercentage(0);
      pDlgProgress.Progress();
      pDlgProgress.StartModal();
      pDlgProgress.ShowProgressBar(true);

      // Get Values from EEPROM 
      pDlgProgress.SetLine(1, g_localizeStrings.Get(20187));
      pDlgProgress.SetPercentage(50);
      pDlgProgress.Progress();
      if ( XKUtils::ReadEEPROMFromXBOX((LPBYTE)&m_EEPROMData, 0, 255))
      {
        CreateEEPROMBackup("System");
        pDlgProgress.SetLine(1, g_localizeStrings.Get(20188));
        pDlgProgress.SetPercentage(55);
        pDlgProgress.Progress();
      }
      m_dwlastTime=0;
      GetMPlayerVersion(strMplayerVersion);
      pDlgProgress.SetLine(1, g_localizeStrings.Get(20189));
      pDlgProgress.SetPercentage(100);
      pDlgProgress.Progress();
      pDlgProgress.Close();
      SetLabelDummy();
      b_IsHome = TRUE;
      return true;
    }
    break;

  case GUI_MSG_CLICKED:
    {
      unsigned int iControl=message.GetSenderId();
      bool b_playing= false;
      if(iControl == CONTROL_BT_HDD)
      { 
        // Pause the Current Playing Media, to prevent corruption during info request
        if (g_application.IsPlaying())
        {
           b_playing= true;
           g_application.m_pPlayer->Pause();
        }

        b_IsHome = FALSE;
        SetLabelDummy();
        SET_CONTROL_LABEL(40,g_localizeStrings.Get(20156));

        CGUIDialogProgress&  pDlgProgress= *((CGUIDialogProgress*)m_gWindowManager.GetWindow(WINDOW_DIALOG_PROGRESS));
		    pDlgProgress.SetHeading(g_localizeStrings.Get(20156));
        pDlgProgress.SetLine(0, g_localizeStrings.Get(20190));
        pDlgProgress.SetLine(1, "");
        pDlgProgress.SetLine(2, g_localizeStrings.Get(20186));
        pDlgProgress.SetPercentage(20);
        pDlgProgress.Progress();
        pDlgProgress.StartModal();
        pDlgProgress.ShowProgressBar(true);

        //Label 2-6; HDD Values
        pDlgProgress.SetLine(1, g_localizeStrings.Get(20191));
        pDlgProgress.SetPercentage(60);
        pDlgProgress.Progress();
        GetATAValues(2, 3, 4, 5, 0); //0=6 is excluded.. todo: Fix the HDD PW detection!

        //Label 7: HDD Lock/UnLock key
        pDlgProgress.SetLine(1, g_localizeStrings.Get(20192));
        pDlgProgress.SetPercentage(80);
        pDlgProgress.Progress();
        CStdString strhddlockey;
        GetHDDKey(strhddlockey);
        //SET_CONTROL_LABEL(6,strhddlockey); //// is exclded.. todo: Fix the HDD key detection!

        //Label 8: HDD Temperature
        pDlgProgress.SetLine(1, g_localizeStrings.Get(20193));
        pDlgProgress.SetPercentage(100);
        pDlgProgress.Progress();
        CStdString strItemhdd;
        GetHDDTemp(strItemhdd);
        SET_CONTROL_LABEL(6,strItemhdd);

        pDlgProgress.Close();
        if(b_playing) g_application.m_pPlayer->Pause();

        CGUIWindow::Render();
      }
      else if(iControl == CONTROL_BT_DVD)
      {
        // Pause the Current Playing Media, to prevent corruption during info request
        if (g_application.IsPlaying())
        {
           b_playing= true;
           g_application.m_pPlayer->Pause();
        }

        b_IsHome = FALSE;
        //Todo: Get DVD-ROM Supportted Disc's
        SetLabelDummy();
        SET_CONTROL_LABEL(40,g_localizeStrings.Get(20157));

        CGUIDialogProgress&  pDlgProgress= *((CGUIDialogProgress*)m_gWindowManager.GetWindow(WINDOW_DIALOG_PROGRESS));
        pDlgProgress.SetHeading(g_localizeStrings.Get(20157));
        pDlgProgress.SetLine(0, g_localizeStrings.Get(20194));
        pDlgProgress.SetLine(1, "");
        pDlgProgress.SetLine(2, g_localizeStrings.Get(20186));
        pDlgProgress.SetPercentage(20);
        pDlgProgress.Progress();
        pDlgProgress.StartModal();
        pDlgProgress.ShowProgressBar(true);

        //Label 2-3: DVD-ROM Values
        pDlgProgress.SetLine(1, g_localizeStrings.Get(20195));
        pDlgProgress.SetPercentage(100);
        pDlgProgress.Progress();
        GetATAPIValues(2, 3);

        pDlgProgress.Close();
        
        if(b_playing) g_application.m_pPlayer->Pause();
        CGUIWindow::Render();
      }
      else if(iControl == CONTROL_BT_STORAGE)
      {
        b_IsHome = FALSE;
        SetLabelDummy();
        SET_CONTROL_LABEL(40,g_localizeStrings.Get(20155));

        // Label 2-10: Storage Values
        GetStorage(2, 3, 4, 5, 6, 7, 8, 9, 10, 11);

        CGUIWindow::Render();
      }
      else if(iControl == CONTROL_BT_DEFAULT)
      { 
        SetLabelDummy();
        b_IsHome = TRUE;
        Render();
      }
      else if(iControl == CONTROL_BT_NETWORK)
      {
        b_IsHome = FALSE;
        SetLabelDummy();
        SET_CONTROL_LABEL(40,g_localizeStrings.Get(20158));

        CGUIDialogProgress&  pDlgProgress= *((CGUIDialogProgress*)m_gWindowManager.GetWindow(WINDOW_DIALOG_PROGRESS));
        pDlgProgress.SetHeading(g_localizeStrings.Get(20158));
        pDlgProgress.SetLine(0, g_localizeStrings.Get(20196));
        pDlgProgress.SetLine(1, "");
        pDlgProgress.SetLine(2, g_localizeStrings.Get(20186));
        pDlgProgress.SetPercentage(40);
        pDlgProgress.Progress();
        pDlgProgress.StartModal();
        pDlgProgress.ShowProgressBar(true);

        pDlgProgress.SetPercentage(20);
        pDlgProgress.Progress();
        GetNetwork(2,5,3,6,7,8,9);  // Label 2-7

        // Label 8: Mac Adress
        pDlgProgress.SetLine(1, g_localizeStrings.Get(20197));
        pDlgProgress.SetPercentage(20);
        pDlgProgress.Progress();
        CStdString strMacAdress;
        if (GetMACAdress(strMacAdress))
          SET_CONTROL_LABEL(4, strMacAdress);

        // Label 9: Online State
        pDlgProgress.SetLine(1, g_localizeStrings.Get(20198));
        pDlgProgress.SetPercentage(20);
        pDlgProgress.Progress();
        CStdString strInetCon;
        GetINetState(strInetCon);
        SET_CONTROL_LABEL(10,strInetCon);
        pDlgProgress.Close();

        CGUIWindow::Render();
      }
      else if(iControl == CONTROL_BT_VIDEO)
      {
        b_IsHome = FALSE;
        SetLabelDummy();
        SET_CONTROL_LABEL(40,g_localizeStrings.Get(20159));

        // Label 2: Video Encoder
        CStdString strVideoEnc;
        GetVideoEncInfo(strVideoEnc);
        SET_CONTROL_LABEL(2,strVideoEnc);

        // Label 3: Resolution 
        CStdString strResol;
        GetResolution(strResol);
        SET_CONTROL_LABEL(3,strResol);

        // Label 4: AV Pack Info
        CStdString stravpack;
        GetAVPackInfo(stravpack);
        SET_CONTROL_LABEL(4,stravpack);

        // Label 5: XBE Video Region
        //CStdString strVideoXBERegion;
        //GetVideoXBERegion(strVideoXBERegion);
        //SET_CONTROL_LABEL(5,GetVideoXBERegion()); 

        // Label 6: DVD Zone
        CStdString strdvdzone;
        GetDVDZone(strdvdzone);
        SET_CONTROL_LABEL(5,strdvdzone);

        CGUIWindow::Render();
      }
      else if(iControl == CONTROL_BT_HARDWARE)
      {
        b_IsHome = FALSE;

        SetLabelDummy();
        SET_CONTROL_LABEL(40,g_localizeStrings.Get(20160));

        CGUIDialogProgress&  pDlgProgress= *((CGUIDialogProgress*)m_gWindowManager.GetWindow(WINDOW_DIALOG_PROGRESS));
        pDlgProgress.SetHeading(g_localizeStrings.Get(20160));
        pDlgProgress.SetLine(0, g_localizeStrings.Get(20300));
        pDlgProgress.SetLine(1, g_localizeStrings.Get(20199));
        pDlgProgress.SetLine(2, g_localizeStrings.Get(20186));
        pDlgProgress.SetPercentage(0);
        pDlgProgress.Progress();
        pDlgProgress.StartModal();
        pDlgProgress.ShowProgressBar(true);

        // Label 2: XBOX Version
        CStdString strXBoxVer;
        GetXBVerInfo(strXBoxVer);
        SET_CONTROL_LABEL(2,strXBoxVer);

        // Label 3: XBOX Serial
        CStdString strXBSerial, strXBOXSerial;
        CStdString strlblXBSerial = g_localizeStrings.Get(13289).c_str();
        if (GetXBOXSerial(strXBSerial))
          strXBOXSerial.Format("%s %s",strlblXBSerial.c_str(),strXBSerial.c_str());
        SET_CONTROL_LABEL(3,strXBOXSerial);

        // Label 4: ModChip ID!
        pDlgProgress.SetLine(1, g_localizeStrings.Get(20301));
        pDlgProgress.SetPercentage(20);
        pDlgProgress.Progress();
        CStdString strModChip;
        if(GetModChipInfo(strModChip))
          SET_CONTROL_LABEL(4,strModChip);    

        // Label 5: Detested BiosName
        pDlgProgress.SetLine(1, g_localizeStrings.Get(20302));
        pDlgProgress.SetPercentage(20);
        pDlgProgress.Progress();
        CStdString strBiosName;
        if (GetBIOSInfo(strBiosName))
          SET_CONTROL_LABEL(5,strBiosName);

        // Label 6: CPU Speed!
        pDlgProgress.SetLine(1, g_localizeStrings.Get(20303));
        pDlgProgress.SetPercentage(20);
        pDlgProgress.Progress();
        CStdString strCPUFreq;
        GetCPUFreqInfo(strCPUFreq); 
        SET_CONTROL_LABEL(6,strCPUFreq);

        // Label 7: XBOX Live Key
        CStdString strXBLiveKey;
        GetXBLiveKey(strXBLiveKey);
        SET_CONTROL_LABEL(7,strXBLiveKey);

        // Label 8: XBOX ProducutionDate Info
        pDlgProgress.SetLine(1, g_localizeStrings.Get(20304));
        CStdString strXBProDate;
        if (GetXBProduceInfo(strXBProDate))
          SET_CONTROL_LABEL(8,strXBProDate);  
        pDlgProgress.SetPercentage(20);
        pDlgProgress.Progress();

        // Label 9,10: Attached Units!
        pDlgProgress.SetLine(1, g_localizeStrings.Get(20305));
        GetUnits(9, 10, 11);
        pDlgProgress.SetPercentage(20);
        pDlgProgress.Progress();
        pDlgProgress.Close();

        CGUIWindow::Render();
      }
    }
    break;
  }
  return CGUIWindow::OnMessage(message);
}

void CGUIWindowSystemInfo::Render()
{
  if (b_IsHome) 
  {
    // Default Values
    SET_CONTROL_LABEL(40,g_localizeStrings.Get(20154));
    SET_CONTROL_LABEL(2, g_infoManager.GetSystemHeatInfo("cpu")); // CPU Temperature
    SET_CONTROL_LABEL(3, g_infoManager.GetSystemHeatInfo("gpu")); // GPU Temperature
    SET_CONTROL_LABEL(4, g_infoManager.GetSystemHeatInfo("fan")); // Fan Speed

    // Label 5: Set FreeMemeory Info 
    CStdString strFreeMem;
    GetFreeMemory(strFreeMem);
    SET_CONTROL_LABEL(5, strFreeMem);

    //Label 6: XBMC IP Adress
    if (g_infoManager.GetLabel(NETWORK_IP_ADDRESS)=="")
    {
      SET_CONTROL_LABEL(6,g_localizeStrings.Get(416));
    }
    else
    {
      SET_CONTROL_LABEL(6, g_infoManager.GetLabel(NETWORK_IP_ADDRESS));
    }
    
    // Label 7: Set Resolution Info
    CStdString strResol;
    GetResolution(strResol);
    SET_CONTROL_LABEL(7,strResol);

    // Label 8: Get Kernel Info
    CStdString strGetKernel;
    GetKernelVersion(strGetKernel);
    SET_CONTROL_LABEL(8,strGetKernel);

    // Label 9: Get System Uptime
    CStdString strSystemUptime;
    GetSystemUpTime(strSystemUptime);
    SET_CONTROL_LABEL(9,strSystemUptime);

    // Label 10: Get System Total Uptime
    CStdString strSystemTotalUptime;
    GetSystemTotalUpTime(strSystemTotalUptime);
    SET_CONTROL_LABEL(10,strSystemTotalUptime);

    /*
    // Label 11: Get System Total Uptime
    CStdString strSmartHDDTemp;
    strSmartHDDTemp.Format("%s %s",g_localizeStrings.Get(13151).c_str(),g_infoManager.GetHDDTemp(17).c_str());
    SET_CONTROL_LABEL(11,strSmartHDDTemp);
    */
    
  }

  // Label 50: Get Current Time
  SET_CONTROL_LABEL(50, g_infoManager.GetTime(true) + " | " + g_infoManager.GetDate());
  // Set XBMC Build Time
  GetBuildTime(51, 52, 53); // Laber 51, 52, 53
  CGUIWindow::Render();
}

void CGUIWindowSystemInfo::SetLabelDummy()
{
  // Set Label Dummy Entry! ""
  for (int i=2; i<12; i++ )
  {
    SET_CONTROL_LABEL(i,"");
  }
}

bool CGUIWindowSystemInfo::GetKernelVersion(CStdString& strKernel)
{
  CStdString lblKernel=  g_localizeStrings.Get(13283).c_str();
  strKernel.Format("%s %d.%d.%d.%d",lblKernel.c_str(),*((USHORT*)XboxKrnlVersion),*((USHORT*)XboxKrnlVersion+1),*((USHORT*)XboxKrnlVersion+2),*((USHORT*)XboxKrnlVersion+3));
  return true;
}

bool CGUIWindowSystemInfo::GetCPUFreqInfo(CStdString& strCPUFreq)
{
  // XBOX CPU Frequence Detection
  double CPUFreq;
  CStdString lblCPUSpeed  = g_localizeStrings.Get(13284).c_str();
  CPUFreq         = g_sysinfo.GetCPUFrequence();
  
  strCPUFreq.Format("%s %4.2f Mhz.",lblCPUSpeed.c_str(), CPUFreq);
  return true;
}

bool CGUIWindowSystemInfo::GetMACAdress(CStdString& strMacAdress)
{
  //Print MAC Address..
  char TempString1[100];
  ZeroMemory(TempString1, 100);
  CStdString lbl1 = g_localizeStrings.Get(149);
  if ( XKUtils::ReadEEPROMFromXBOX((LPBYTE)&m_EEPROMData, 0, 255))
  {
    strncpy(TempString1, (LPSTR)&m_EEPROMData.SerialNumber, MACADDRESS_SIZE);
    BytesToHexStr((LPBYTE)&m_EEPROMData.MACAddress, MACADDRESS_SIZE, TempString1, 0x3a); // "-"=0x2d or ":"=0x3a
    strMacAdress.Format("%s: %s",lbl1.c_str(), TempString1);
    return true;
  }
  else return false;  
}

bool CGUIWindowSystemInfo::GetBIOSInfo(CStdString& strBiosName)
{
  // Get XBOX Bios Informations, BiosDetector!
  CStdString cBIOSName;
  CStdString strlblBios = g_localizeStrings.Get(13285).c_str();
  if(g_sysinfo.CheckBios(cBIOSName))
  {
    strBiosName.Format("%s %s", strlblBios.c_str(),cBIOSName.c_str());
    return true;
  }
  else 
  {
    strBiosName.Format("%s %s", strlblBios.c_str(),"File: BiosIDs.ini Not Found!");
    return true;
  }
}

bool CGUIWindowSystemInfo::GetVideoEncInfo(CStdString& strItemVideoENC)
{
  // XBOX Video Encoder Detection
  CStdString lblVideoEnc  = g_localizeStrings.Get(13286).c_str();
  CStdString VideoEncoder = g_sysinfo.GetVideoEncoder();
  strItemVideoENC.Format("%s %s", lblVideoEnc.c_str(),VideoEncoder.c_str());
  return true;
}

bool CGUIWindowSystemInfo::GetResolution(CStdString& strResol)
{
  // Set Screen Resolution Info
  CStdString lblResInf  = g_localizeStrings.Get(13287).c_str();
  strResol.Format("%s %ix%i %s %02.2f Hz.",lblResInf.c_str(),
    g_settings.m_ResInfo[g_guiSettings.m_LookAndFeelResolution].iWidth,
    g_settings.m_ResInfo[g_guiSettings.m_LookAndFeelResolution].iHeight,
    g_settings.m_ResInfo[g_guiSettings.m_LookAndFeelResolution].strMode,
    g_infoManager.GetFPS()
    );
  return true;
}

bool CGUIWindowSystemInfo::GetXBVerInfo(CStdString& strXBoxVer)
{
  // XBOX Version Detection
  CStdString strXBOXVersion;
  CStdString lblXBver   =  g_localizeStrings.Get(13288).c_str();
  if (g_sysinfo.GetXBOXVersionDetected(strXBOXVersion))
  {
    strXBoxVer.Format("%s %s", lblXBver.c_str(),strXBOXVersion.c_str());
    CLog::Log(LOGDEBUG,"XBOX Version: %s",strXBOXVersion.c_str());
    return true;
  }
  else return false;
}

bool CGUIWindowSystemInfo::GetXBOXSerial(CStdString& strXBOXSerial)
{
  //Detect XBOX Serial Number
  CHAR TempString1[100];
  ZeroMemory(TempString1, 100);
  //CStdString strlblXBSerial = g_localizeStrings.Get(13289).c_str();
  if ( XKUtils::ReadEEPROMFromXBOX((LPBYTE)&m_EEPROMData, 0, 255))
  {
    strncpy(TempString1, (LPSTR)&m_EEPROMData.SerialNumber, SERIALNUMBER_SIZE);
    //strXBOXSerial.Format("%s %s",strlblXBSerial.c_str(),TempString1);
    strXBOXSerial.Format("%s",TempString1);
    return true;
  }
  else return false;
}

bool CGUIWindowSystemInfo::GetXBProduceInfo(CStdString& strXBProDate) 
{
  // Print XBOX Production Place and Date 
  CStdString lbl = g_localizeStrings.Get(13290).c_str();
  CStdString lblYear = g_localizeStrings.Get(201).c_str();
  if ( XKUtils::ReadEEPROMFromXBOX((LPBYTE)&m_EEPROMData, 0, 255))
  {
    char *info = (LPSTR)&m_EEPROMData.SerialNumber;
    char *country;
    switch (atoi(&info[11])) 
    {
    case 2:
      country = "Mexico";
      break;
    case 3:
      country = "Hungary";
      break;
    case 5: 
      country = "China";
      break;
    case 6:
      country = "Taiwan";
      break;
    default:
      country = "Unknown";
      break;
    }
    CLog::Log(LOGDEBUG, "- XBOX became produced: Country: %s, LineNumber: %c, Week %c%c, Year 200%c", country, info[0x00], info[0x08], info[0x09],info[0x07]);
    strXBProDate.Format("%s %s, %s 200%c, "+g_localizeStrings.Get(20169)+": %c%c "+g_localizeStrings.Get(20170)+": %c",lbl.c_str(), country, lblYear.c_str(),info[0x07], info[0x08],info[0x09], info[0x00]);
    return true;
  }
  else return false;
}

bool CGUIWindowSystemInfo::GetModChipInfo(CStdString& strModChip)
{
  // XBOX ModCHIP Type Detection GeminiServer
  CStdString ModChip    = g_sysinfo.GetModCHIPDetected().c_str();
  CStdString lblModChip = g_localizeStrings.Get(13291).c_str();
   // Chech if it is a SmartXX
  CStdString strIsSmartXX = g_sysinfo.SmartXXModCHIP();
  if (!strIsSmartXX.Equals("None"))
  { 
    strModChip.Format("%s %s", lblModChip.c_str(),strIsSmartXX.c_str());
    CLog::Log(LOGDEBUG, "- Detected ModChip: %s",strIsSmartXX.c_str());
    return true;
  }
  else
  { 
    //CStdString strXBOXVersion;
    //g_sysinfo.GetXBOXVersionDetected(strXBOXVersion);
    if ( ModChip.Equals("Unknown/Onboard TSOP (protected)"))
    {
      strModChip.Format("%s %s", lblModChip.c_str(),g_localizeStrings.Get(20311));
    }
    else
    {
      strModChip.Format("%s %s", lblModChip.c_str(),ModChip.c_str()); 
    }
    return true;
  }
  return false; 
}

void CGUIWindowSystemInfo::GetAVPackInfo(CStdString& stravpack)
{
  //AV-[Cable]Pack Detection 
  CStdString DetectedAVpack = g_sysinfo.GetAVPackInfo();
  CStdString lblAVpack    = g_localizeStrings.Get(13292).c_str();
  stravpack.Format("%s %s",lblAVpack.c_str(), DetectedAVpack.c_str());
  return;
}

bool CGUIWindowSystemInfo::GetVideoXBERegion(CStdString& strVideoXBERegion)
{
  //Print Game!Video Standard & XBE Region... // Todo:Decrpyring Required!!!
  DWORD xberegion;
  DWORD VideoStandard;
  CHAR TempString1[100];
  ZeroMemory(TempString1, 100);
  CStdString lblVXBE = g_localizeStrings.Get(13293).c_str();
  if ( XKUtils::ReadEEPROMFromXBOX((LPBYTE)&m_EEPROMData, 0, 255))
  {
    xberegion = XBE_REGION (m_EEPROMData.XBERegion[0]);
    VideoStandard = (VIDEO_STANDARD) *((LPDWORD)&m_EEPROMData.VideoStandard);
    //DWORD xberegion = m_pXKEEPROM->GetXBERegionVal();
    //DWORD VideoStandard = m_pXKEEPROM->GetVideoStandardVal();

    switch (VideoStandard)
    {
    case (XKEEPROM::NTSC_M):
      {
        sprintf(TempString1, "NTSC, Region %d", xberegion / 40);
        break;
      }
    case (XKEEPROM::PAL_I):
      {
        sprintf(TempString1, "PAL, Region %d", xberegion / 40);
        break;
      }
    default:
      {
        sprintf(TempString1, "UNKNOWN, Region %d", xberegion / 40);
        break;
      }
    }
    strVideoXBERegion.Format("%s %s",lblVXBE.c_str(), TempString1);

    return true;
  }
  else return false;
}

bool CGUIWindowSystemInfo::GetDVDZone(CStdString& strdvdzone)
{
  //Print DVD [Region] Zone ..
  CStdString lblDVDZone =  g_localizeStrings.Get(13294).c_str();
  DVD_ZONE retVal;
  CHAR TempString1[100];
  ZeroMemory(TempString1, 100);
  if ( XKUtils::ReadEEPROMFromXBOX((LPBYTE)&m_EEPROMData, 0, 255))
  {
    retVal = DVD_ZONE (m_EEPROMData.DVDPlaybackKitZone[0]);
    strdvdzone.Format("%s %d",lblDVDZone.c_str(), retVal);
    return true;
  }
  else return false;
}

bool CGUIWindowSystemInfo::GetINetState(CStdString& strInetCon)
{
  CHTTP http;
  CStdString lbl2 = g_localizeStrings.Get(13295);
  CStdString lbl3 = g_localizeStrings.Get(13296);
  CStdString lbl4 = g_localizeStrings.Get(13297);

  if (http.IsInternet()) 
  { // Connected to the Internet!
    strInetCon.Format("%s %s",lbl2.c_str(), lbl3.c_str());
    return true;
  }
  else if (http.IsInternet(false))
  { // connected, but no DNS
    strInetCon.Format("%s %s",lbl2.c_str(), g_localizeStrings.Get(13274).c_str());
    return true;
  }
  // NOT Connected to the Internet!
  strInetCon.Format("%s %s",lbl2.c_str(), lbl4.c_str());
  return true;
}

bool CGUIWindowSystemInfo::GetXBLiveKey(CStdString& strXBLiveKey)
{
  //Print XBLIVE Online Key..
  CStdString lbl3 = g_localizeStrings.Get(13298).c_str();
  CHAR TempString1[100];
  ZeroMemory(TempString1, 100);
  if ( XKUtils::ReadEEPROMFromXBOX((LPBYTE)&m_EEPROMData, 0, 255))
  {
    BytesToHexStr((LPBYTE)&m_EEPROMData.OnlineKey, ONLINEKEY_SIZE, TempString1);
    strXBLiveKey.Format("%s %s",lbl3.c_str(), TempString1);
    return true;
  }else return false;
}

bool CGUIWindowSystemInfo::GetHDDKey(CStdString& strhddlockey)
{
  //Print HDD Key...
  CStdString lbl5 = g_localizeStrings.Get(13150).c_str();
  CHAR TempString1[100];
  ZeroMemory(TempString1, 100);
  if ( XKUtils::ReadEEPROMFromXBOX((LPBYTE)&m_EEPROMData, 0, 255))
  {
    BytesToHexStr((LPBYTE)&m_EEPROMData.HDDKkey, HDDKEY_SIZE, TempString1);
    strhddlockey.Format("%s %s",lbl5.c_str(), TempString1);
    return true;
  }
  else return false;
}

bool CGUIWindowSystemInfo::GetHDDTemp(CStdString& strItemhdd)
{
  // Set HDD Temp
  CStdString lblhdd = g_localizeStrings.Get(13151).c_str();
  /*
  if(!g_hddsmart.IsRunning())
    g_hddsmart.Start();
  g_hddsmart.SmartREQ = 17;
  */
  
  BYTE bTemp= g_hddsmart.GetSmartValues(17);
  //CTemperature temp= CTemperature::CreateFromCelsius((double)g_hddsmart.m_HddSmarValue);
  CTemperature temp= CTemperature::CreateFromCelsius((double)bTemp);
  if (bTemp ==0 )
    temp.SetState(CTemperature::invalid);

  strItemhdd.Format("%s %s", lblhdd.c_str(), temp.ToString().c_str()); 
  return true;
}

void CGUIWindowSystemInfo::GetFreeMemory(CStdString& strFreeMem)
{
  // Set FreeMemory Info
  MEMORYSTATUS stat;
  GlobalMemoryStatus(&stat);
  CStdString lblFreeMem = g_localizeStrings.Get(158).c_str();
  strFreeMem.Format("%s %i/%iMB",lblFreeMem.c_str(),stat.dwAvailPhys/MB, stat.dwTotalPhys/MB);
}

bool CGUIWindowSystemInfo::GetATAPIValues(int i_lblp1, int i_lblp2)
{
  CStdString strDVDModel, strDVDFirmware;
  CStdString lblDVDModel    = g_localizeStrings.Get(13152).c_str();
  CStdString lblDVDFirmware = g_localizeStrings.Get(13153).c_str();
  if(g_sysinfo.GetDVDInfo(strDVDModel, strDVDFirmware))
  {
    CStdString strDVDModelA;
    strDVDModelA.Format("%s %s",lblDVDModel.c_str(), strDVDModel.c_str());
    SET_CONTROL_LABEL(i_lblp1, strDVDModelA);

    CStdString lblDVDFirmwareA;
    lblDVDFirmwareA.Format("%s %s",lblDVDFirmware.c_str(), strDVDFirmware.c_str());
    SET_CONTROL_LABEL(i_lblp2, lblDVDFirmwareA);
    return true;
  }
  else return false;
}

bool CGUIWindowSystemInfo::GetATAValues(int i_lblp1, int i_lblp2, int i_lblp3, int i_lblp4, int i_lblp5)
{ 
  /*
  //Get IDE_ATA_COMMAND_READ_SECTORS Data for HDD
  ZeroMemory(&hddcommand, sizeof(XKHDD::ATA_COMMAND_OBJ));
  hddcommand.DATA_BUFFSIZE    = 0;
  hddcommand.IPReg.bDriveHeadReg  = IDE_DEVICE_MASTER;
  //hddcommand.IPReg.bCommandReg  = IDE_ATA_COMMAND_READ_SECTORS;
  hddcommand.IPReg.bCommandReg  = IDE_ATA_IDENTIFY


  if (XKHDD::SendATACommand(IDE_PRIMARY_PORT, &hddcommand, IDE_COMMAND_READ) == TRUE)
  {
  // now compute the head, cylinder, and sector
  short head;
  short sector;
  short cylinder;

  short num_Cylinders     = XKHDD::GetATABefehle(hddcommand.DATA_BUFFER, IDE_INDENTIFY_NUM_CYLINDERS);
  short num_Heads       = XKHDD::GetATABefehle(hddcommand.DATA_BUFFER, IDE_INDENTIFY_NUM_HEADS);
  short num_SectorsPerTrack = XKHDD::GetATABefehle(hddcommand.DATA_BUFFER, IDE_INDENTIFY_NUM_SECTORS_TRACK);
  short num_BytesPerSector  = XKHDD::GetATABefehle(hddcommand.DATA_BUFFER, IDE_INDENTIFY_NUM_BYTES_SECTOR);

  // Return the number of logical blocks for a particular drive.
  int blockNum        = ( num_Heads * num_SectorsPerTrack * num_Cylinders );

  // Read a block at the logical block number indicated.
  // now compute the head, cylinder, and sector
  sector   = blockNum % num_SectorsPerTrack + 1;
  cylinder = blockNum / (num_Heads * num_SectorsPerTrack);
  head     = (blockNum / num_SectorsPerTrack) % num_Heads;

  CLog::Log(LOGDEBUG, "request to read block %d\n", blockNum);
  CLog::Log(LOGDEBUG, "    head %d\n", head);
  CLog::Log(LOGDEBUG, "    cylinder %d\n", cylinder);
  CLog::Log(LOGDEBUG, "    sector %d\n", sector);

  CLog::Log(LOGDEBUG, "    %d cylinders, %d heads, %d sectors/tack, %d bytes/sector\n", num_Cylinders, num_Heads, num_SectorsPerTrack, num_BytesPerSector);

  long long int bytes = ((long long int )blockNum) * 512;
  CLog::Log(LOGDEBUG, "    Disk has %d blocks (%l bytes)\n", blockNum, bytes);
  }

  */
  CStdString strHDDModel, strHDDSerial,strHDDFirmware,strHDDpw,strHDDLockState;
  if (g_sysinfo.GetHDDInfo(strHDDModel, strHDDSerial,strHDDFirmware,strHDDpw,strHDDLockState))
  {
    CStdString strHDDModelA, strHDDSerialA, strHDDFirmwareA, strHDDpwA, strHDDLockStateA;

    CStdString lblhddm  = g_localizeStrings.Get(13154); //"HDD Model";
    CStdString lblhdds  = g_localizeStrings.Get(13155); //"HDD Serial";
    CStdString lblhddf  = g_localizeStrings.Get(13156); //"HDD Firmware";
    CStdString lblhddpw = g_localizeStrings.Get(13157); //"HDD Password";
    CStdString lblhddlk = g_localizeStrings.Get(13158); //"HDD Lock State";

    //HDD Model
    strHDDModelA.Format("%s %s",lblhddm.c_str(),strHDDModel.c_str());
    SET_CONTROL_LABEL(i_lblp1, strHDDModelA);
    //CLog::Log(LOGDEBUG, "HDD Model: %s",strHDDModelA);

    //HDD Serial
    strHDDSerialA.Format("%s %s",lblhdds.c_str(),strHDDSerial.c_str());
    SET_CONTROL_LABEL(i_lblp2, strHDDSerialA);
    //CLog::Log(LOGDEBUG, "HDD Serial: %s",strHDDSerialA);

    //HDD Firmware
    strHDDFirmwareA.Format("%s %s",lblhddf.c_str(),strHDDFirmware.c_str());
    SET_CONTROL_LABEL(i_lblp3, strHDDFirmwareA);
    //CLog::Log(LOGDEBUG, "HDD Firmware: %s",strHDDFirmwareA);

    //HDD Lock State
    strHDDLockStateA.Format("%s %s",lblhddlk.c_str(),strHDDLockState.c_str());
    SET_CONTROL_LABEL(i_lblp4, strHDDLockStateA);
    //CLog::Log(LOGDEBUG, "HDD LockState: %s",strHDDLockStateA);

    //HDD Password
    strHDDpwA.Format("%s %s",lblhddpw.c_str(),strHDDpw.c_str());
    //SET_CONTROL_LABEL(i_lblp5, strHDDpwA);
    //CLog::Log(LOGDEBUG, "HDD Password: %s",strHDDpwA);

    return true;
  }
  return false;
}

bool CGUIWindowSystemInfo::GetNetwork(int i_lblp1, int i_lblp2, int i_lblp3, int i_lblp4, int i_lblp5, int i_lblp6, int i_lblp7)
{
  // Set Network Informations
  XNADDR net_stat;
  CStdString ip;

  // Set IP Type [DHCP/Fixed]
  if(XNetGetTitleXnAddr(&net_stat) & XNET_GET_XNADDR_DHCP)  
    ip.Format("%s %s", g_localizeStrings.Get(146).c_str(), g_localizeStrings.Get(148).c_str());
  else
    ip.Format("%s %s", g_localizeStrings.Get(146).c_str(), g_localizeStrings.Get(147).c_str());

  SET_CONTROL_LABEL(i_lblp1,ip);

  // Set Ethernet Link State
  DWORD dwnetstatus = XNetGetEthernetLinkStatus();
  CStdString linkStatus = g_localizeStrings.Get(151);
  linkStatus += " ";
  if (dwnetstatus & XNET_ETHERNET_LINK_ACTIVE)
  {
    if (dwnetstatus & XNET_ETHERNET_LINK_100MBPS)
      linkStatus += "100mbps ";
    if (dwnetstatus & XNET_ETHERNET_LINK_10MBPS)
      linkStatus += "10mbps ";
    if (dwnetstatus & XNET_ETHERNET_LINK_FULL_DUPLEX)
      linkStatus += g_localizeStrings.Get(153);
    if (dwnetstatus & XNET_ETHERNET_LINK_HALF_DUPLEX)
      linkStatus += g_localizeStrings.Get(152); 
  } 
  else
    linkStatus += g_localizeStrings.Get(159);

  SET_CONTROL_LABEL(i_lblp3,linkStatus);

  // Get IP/Subnet/Gateway/DHCP Server/DNS1/DNS2
  const char* pszIP=g_localizeStrings.Get(150).c_str();

  CStdString strlblSubnet   = g_localizeStrings.Get(13159); //"Subnet:";      
  CStdString strlblGateway  = g_localizeStrings.Get(13160); //"Gateway:";
  CStdString strlblDNS    = g_localizeStrings.Get(13161); //"DNS:";
  CStdString strlblDNS2    = g_localizeStrings.Get(20307);
  CStdString strlblDHCPServer = g_localizeStrings.Get(20308);
    
  CStdString strItem1, strItem2, strItem3, strItem4;

  ip.Format("%s: %s",pszIP, g_network.m_networkinfo.ip);  // IP
  strItem1.Format("%s %s", strlblSubnet.c_str(), g_network.m_networkinfo.subnet); // Subnetmask
  strItem2.Format("%s %s", strlblGateway.c_str(), g_network.m_networkinfo.gateway); //Gateway (Router IP)
  //strItem3.Format("%s %s", strlblDHCPServer.c_str(), g_network.m_networkinfo.dhcpserver); // DHCP-Server IP
  
  strItem3.Format("%s: %s", strlblDNS.c_str(), g_network.m_networkinfo.DNS1 ); // DNS1
  strItem4.Format("%s: %s", strlblDNS2.c_str(), g_network.m_networkinfo.DNS2 ); // DNS2
  
  SET_CONTROL_LABEL(i_lblp2,ip);
  SET_CONTROL_LABEL(i_lblp4,strItem1);
  SET_CONTROL_LABEL(i_lblp5,strItem2);
  SET_CONTROL_LABEL(i_lblp6,strItem3);
  SET_CONTROL_LABEL(i_lblp7,strItem4);

  return true;
}

bool CGUIWindowSystemInfo::GetStorage(int i_lblp1, int i_lblp2, int i_lblp3, int i_lblp4, int i_lblp5, int i_lblp6, int i_lblp7, int i_lblp8, int i_lblp9, int i_lblp10)
{
  // Set HDD Space Informations 
  ULARGE_INTEGER lTotalFreeBytesC;  ULARGE_INTEGER lTotalNumberOfBytesC;
  ULARGE_INTEGER lTotalFreeBytesE;  ULARGE_INTEGER lTotalNumberOfBytesE;
  ULARGE_INTEGER lTotalFreeBytesF;  ULARGE_INTEGER lTotalNumberOfBytesF;
  ULARGE_INTEGER lTotalFreeBytesG;  ULARGE_INTEGER lTotalNumberOfBytesG;
  ULARGE_INTEGER lTotalFreeBytesX;  ULARGE_INTEGER lTotalNumberOfBytesX;
  ULARGE_INTEGER lTotalFreeBytesY;  ULARGE_INTEGER lTotalNumberOfBytesY;
  ULARGE_INTEGER lTotalFreeBytesZ;  ULARGE_INTEGER lTotalNumberOfBytesZ;

  // Set DVD Drive State! [TrayOpen, NotReady....]
  CStdString trayState = "D: ";
  const char* pszStatus1;

  CIoSupport m_pIOhelp;
  switch (m_pIOhelp.GetTrayState())
  {
  case TRAY_OPEN:
    pszStatus1=g_localizeStrings.Get(162).c_str();
    break;
  case DRIVE_NOT_READY:
    pszStatus1=g_localizeStrings.Get(163).c_str();
    break;
  case TRAY_CLOSED_NO_MEDIA:
    pszStatus1=g_localizeStrings.Get(164).c_str();
    break;
  case TRAY_CLOSED_MEDIA_PRESENT:
    pszStatus1=g_localizeStrings.Get(165).c_str();
    break;
  }
  trayState += pszStatus1;
  SET_CONTROL_LABEL(i_lblp2, trayState);
 
  //For C and E
  CStdString hdC, hdE;
  GetDiskSpace("C", lTotalNumberOfBytesC, lTotalFreeBytesC, hdC);
  GetDiskSpace("E", lTotalNumberOfBytesE, lTotalFreeBytesE, hdE);
  SET_CONTROL_LABEL(i_lblp1,hdC);
  SET_CONTROL_LABEL(i_lblp3,hdE);

  //For X, Y, Z
  CStdString hdX, hdY, hdZ;
  GetDiskSpace("X", lTotalNumberOfBytesX, lTotalFreeBytesX, hdX);
  GetDiskSpace("Y", lTotalNumberOfBytesY, lTotalFreeBytesY, hdY);
  GetDiskSpace("Z", lTotalNumberOfBytesZ, lTotalFreeBytesZ, hdZ);

  // Total Free Size: Generate from Drives#
  ULARGE_INTEGER lTotalDiscSpace;
  lTotalDiscSpace.QuadPart = ( 
    lTotalNumberOfBytesC.QuadPart + 
    lTotalNumberOfBytesE.QuadPart + 
    lTotalNumberOfBytesX.QuadPart + 
    lTotalNumberOfBytesY.QuadPart + 
    lTotalNumberOfBytesZ.QuadPart );

  // Total Free Size: Generate from Drives#
  ULARGE_INTEGER lTotalDiscFree;
  lTotalDiscFree.QuadPart = ( 
    lTotalFreeBytesC.QuadPart + 
    lTotalFreeBytesE.QuadPart + 
    lTotalFreeBytesX.QuadPart + 
    lTotalFreeBytesY.QuadPart + 
    lTotalFreeBytesZ.QuadPart );

  //For F and G
  CStdString hdF,hdG;
  bool bUseDriveF = GetDiskSpace("F", lTotalNumberOfBytesF, lTotalFreeBytesF, hdF);
  bool bUseDriveG = GetDiskSpace("G", lTotalNumberOfBytesG, lTotalFreeBytesG, hdG);

  if (bUseDriveF) {
    lTotalDiscSpace.QuadPart = lTotalDiscSpace.QuadPart + lTotalNumberOfBytesF.QuadPart;
    lTotalDiscFree.QuadPart = lTotalDiscFree.QuadPart + lTotalFreeBytesF.QuadPart;
  }
  if (bUseDriveG) {
    lTotalDiscSpace.QuadPart = lTotalDiscSpace.QuadPart + lTotalNumberOfBytesG.QuadPart;
    lTotalDiscFree.QuadPart = lTotalDiscFree.QuadPart + lTotalFreeBytesG.QuadPart;
  }
  // Total USED Size: Generate from Drives#
  ULARGE_INTEGER lTotalDiscUsed;
  ULARGE_INTEGER lTotalDiscPercent;

  lTotalDiscUsed.QuadPart   = lTotalDiscSpace.QuadPart - lTotalDiscFree.QuadPart;
  lTotalDiscPercent.QuadPart  = lTotalDiscSpace.QuadPart/100;  // => 1%   

  CStdString hdTotalSize, hdTotalUsedPercent, t1,t2,t3;
  t1.Format("%u",lTotalDiscSpace.QuadPart/MB);
  t2.Format("%u",lTotalDiscUsed.QuadPart/MB);
  t3.Format("%u",lTotalDiscFree.QuadPart/MB);
  hdTotalSize.Format(g_localizeStrings.Get(20161), t1.c_str(), t2.c_str(), t3.c_str());  //Total Free To make it MB
  //hdTotalSize.Format("Total: %u MB, Used: %u MB, Free: %u MB ", lTotalDiscSpace.QuadPart/MB, lTotalDiscUsed.QuadPart/MB, lTotalDiscFree.QuadPart/MB );  //Total Free To make it MB

  int percentUsed = (int)(100.0f * lTotalDiscUsed.QuadPart/lTotalDiscSpace.QuadPart + 0.5f);
  hdTotalUsedPercent.Format(g_localizeStrings.Get(20162), percentUsed, 100 - percentUsed); //Total Free %

  CLog::Log(LOGDEBUG, "------------- HDD Space Info: -------------------");
  CLog::Log(LOGDEBUG, "HDD Total Size: %u MB", lTotalDiscSpace.QuadPart/MB);
  CLog::Log(LOGDEBUG, "HDD Used Size: %u MB", lTotalDiscUsed.QuadPart/MB);
  CLog::Log(LOGDEBUG, "HDD Free Size: %u MB", lTotalDiscFree.QuadPart/MB);
  CLog::Log(LOGDEBUG, "--------------HDD Percent Info: -----------------");
  CLog::Log(LOGDEBUG, "HDD Used Percent: %u%%", lTotalDiscUsed.QuadPart/lTotalDiscPercent.QuadPart );
  CLog::Log(LOGDEBUG, "HDD Free Percent: %u%%", lTotalDiscFree.QuadPart/lTotalDiscPercent.QuadPart );
  CLog::Log(LOGDEBUG, "-------------------------------------------------");

  // Detect which to show!! 
  if(bUseDriveF)  // Show if Drive F is availible
  {
    SET_CONTROL_LABEL(i_lblp4,hdF);
    if(bUseDriveG)
    {
      SET_CONTROL_LABEL(i_lblp5,hdG);
      SET_CONTROL_LABEL(i_lblp6,hdX);
      SET_CONTROL_LABEL(i_lblp7,hdY);
      SET_CONTROL_LABEL(i_lblp8,hdZ);
      SET_CONTROL_LABEL(i_lblp9,hdTotalSize);
      SET_CONTROL_LABEL(i_lblp10,hdTotalUsedPercent);
    }
    else
    {
      SET_CONTROL_LABEL(i_lblp5,hdX);
      SET_CONTROL_LABEL(i_lblp6,hdY);
      SET_CONTROL_LABEL(i_lblp7,hdZ);
      SET_CONTROL_LABEL(i_lblp8,hdTotalSize);
      SET_CONTROL_LABEL(i_lblp9,hdTotalUsedPercent);
    }

  } 
  else  // F and G not available
  {
    SET_CONTROL_LABEL(i_lblp4,hdX);
    SET_CONTROL_LABEL(i_lblp5,hdY);
    SET_CONTROL_LABEL(i_lblp6,hdZ);
    SET_CONTROL_LABEL(i_lblp7,hdTotalSize);
    SET_CONTROL_LABEL(i_lblp8,hdTotalUsedPercent);
  }

#ifdef _DEBUG
  //Only DebugOutput!
  MEMORYSTATUS stat;
  CHAR strOut[1024], *pstrOut;
  // Get the memory status.
  GlobalMemoryStatus( &stat );
  // Setup the output string.
  pstrOut = strOut;
  AddStr( "%4d total MB of virtual memory.\n", stat.dwTotalVirtual / MB );
  AddStr( "%4d  free MB of virtual memory.\n", stat.dwAvailVirtual / MB );
  AddStr( "%4d total MB of physical memory.\n", stat.dwTotalPhys / MB );
  AddStr( "%4d  free MB of physical memory.\n", stat.dwAvailPhys / MB );
  AddStr( "%4d total MB of paging file.\n", stat.dwTotalPageFile / MB );
  AddStr( "%4d  free MB of paging file.\n", stat.dwAvailPageFile / MB );
  AddStr( "%4d  percent of memory is in use.\n", stat.dwMemoryLoad );
  OutputDebugString( strOut );
#endif
  return true;
}

bool CGUIWindowSystemInfo::GetDiskSpace(const CStdString &drive, ULARGE_INTEGER &total, ULARGE_INTEGER& totalFree, CStdString &string)
{
  CStdString driveName = drive + ":\\";
  CStdString t1,t2;
  BOOL ret;
  if ((ret = GetDiskFreeSpaceEx( driveName.c_str(), NULL, &total, &totalFree)))
  {
    t1.Format("%u",totalFree.QuadPart/MB);
    t2.Format("%u",total.QuadPart/MB);
    string.Format(g_localizeStrings.Get(20163), drive.c_str(),t1.c_str(),t2.c_str());
    //string.Format("%s: %u MB of %u MB %s", drive.c_str(), (totalFree.QuadPart/MB), (total.QuadPart/MB), g_localizeStrings.Get(160).c_str());
  }
  else
  {
    string.Format("%s %s: %s", g_localizeStrings.Get(155).c_str(), drive.c_str(), g_localizeStrings.Get(161).c_str());
    total.QuadPart = 0;
    totalFree.QuadPart = 0;
  }
  return ret == TRUE;
}

bool CGUIWindowSystemInfo::GetBuildTime(int label1, int label2, int label3)
{
  CStdString version, buildDate, mplayerVersion;
  version.Format("%s %s", g_localizeStrings.Get(144).c_str(), g_infoManager.GetVersion().c_str());
  buildDate.Format("XBMC %s (Compiled :%s)", version, g_infoManager.GetBuild().c_str());
  mplayerVersion.Format("%s",strMplayerVersion.c_str());
  //SET_CONTROL_LABEL(label1, version);
  SET_CONTROL_LABEL(label2, buildDate);
  SET_CONTROL_LABEL(label3, mplayerVersion);
  return true;
}

bool CGUIWindowSystemInfo::GetUnits(int i_lblp1, int i_lblp2, int i_lblp3 )
{
  // Get the Connected Units on the Front USB Ports!
  DWORD dwDeviceGamePad   = XGetDevices(XDEVICE_TYPE_GAMEPAD);      char* sclDeviceVle;       
  DWORD dwDeviceKeyboard    = XGetDevices(XDEVICE_TYPE_DEBUG_KEYBOARD);   char* sclDeviceKeyb;      
  DWORD dwDeviceMouse     = XGetDevices(XDEVICE_TYPE_DEBUG_MOUSE);    char* sclDeviceMouse;     
  DWORD dwDeviceHeadPhone   = XGetDevices(XDEVICE_TYPE_VOICE_HEADPHONE);  char* sclDeviceHeadPhone;   
  DWORD dwDeviceMicroPhone  = XGetDevices(XDEVICE_TYPE_VOICE_MICROPHONE); char* sclDeviceMicroPhone;    
  DWORD dwDeviceMemory    = XGetDevices(XDEVICE_TYPE_MEMORY_UNIT);    char* sclDeviceMemory;
  DWORD dwDeviceIRRemote    = XGetDevices(XDEVICE_TYPE_IR_REMOTE);      char* sclDeviceIRRemote;

  CStdString strlblGamePads = g_localizeStrings.Get(13163); //"GamePads On Port:";      
  CStdString strlblKeyboard = g_localizeStrings.Get(13164); //"Keyboard On Port:";
  CStdString strlblMouse    = g_localizeStrings.Get(13165); //"Mouse On Port:";
  CStdString strlblHeadMicro  = g_localizeStrings.Get(13166); //"Head/MicroPhone On Ports:";
  CStdString strlblMemoryStk  = g_localizeStrings.Get(13167); //"MemoryStick On Port:";
  CStdString strlblIRRemote = g_localizeStrings.Get(13168); //"IR-Remote On Port:";

  if (dwDeviceGamePad > 0)
  {
    switch (dwDeviceGamePad)
    {
    case 1:   // Values 1 ->  on Port 1
      sclDeviceVle = "1";
      break;
    case 2:   // Values 2 ->  on Port 2 
      sclDeviceVle = "2";
      break;
    case 4:   // Values 4 ->  on Port 3
      sclDeviceVle = "3";
      break;
    case 8:   // Values 8 ->  on Port 4 
      sclDeviceVle = "4";
      break;
    case 3:   // Values 3 ->  on Port 1&2 
      sclDeviceVle = "1, 2";
      break;
    case 5:   // Values 5 ->  on Port 1&3 
      sclDeviceVle = "1, 3";
      break;
    case 6:   // Values 6 ->  on Port 2&3 
      sclDeviceVle = "2, 3";
      break;
    case 7:   // Values 7 ->  on Port 1&2&3
      sclDeviceVle = "1, 2, 3";
      break;
    case 9:   // Values 9  -> on Port 1&4 
      sclDeviceVle = "1, 4";
      break;
    case 10:  // Values 10 -> on Port 2&4 
      sclDeviceVle = "2, 4";
      break;
    case 11:  // Values 11 -> on Port 1&2&4
      sclDeviceVle = "1, 2, 4";
      break;
    case 12:  // Values 12 -> on Port 3&4
      sclDeviceVle = "3, 4";
      break;
    case 13:  // Values 13 -> on Port 1&3&4
      sclDeviceVle = "1, 3, 4";
      break;
    case 14:  // Values 14 -> on Port 2&3&4
      sclDeviceVle = "2, 3, 4";
      break;
    case 15:  // Values 15 -> on Port 1&2&3&4
      sclDeviceVle = "1, 2, 3, 4";
      break;
    default:
      sclDeviceVle = "-";
    }
  }
  else sclDeviceVle = "0";
  if (dwDeviceKeyboard > 0)
  {
    switch (dwDeviceKeyboard)
    { 
    case 1:   // Values 1 ->  on Port 1
      sclDeviceKeyb = "1";
      break;
    case 2:   // Values 2 ->  on Port 2 
      sclDeviceKeyb = "2";
      break;
    case 4:   // Values 4 ->  on Port 3 
      sclDeviceKeyb = "3";
      break;
    case 8:   // Values 8 ->  on Port 4
      sclDeviceKeyb = "4";
      break;
    default:
      sclDeviceKeyb = "-";
    }
  }
  else sclDeviceKeyb = "0";
  if (dwDeviceMouse > 0)
  {
    switch (dwDeviceMouse)
    {
    case 1:   // Values 1 ->  on Port 1 
      sclDeviceMouse = "1";
      break;
    case 2:   // Values 2 ->  on Port 2 
      sclDeviceMouse = "2";
      break;
    case 4:   // Values 4 ->  on Port 3 
      sclDeviceMouse = "3";
      break;
    case 8:   // Values 8 ->  on Port 4
      sclDeviceMouse = "4";
      break;
    default:
      sclDeviceMouse = "-";
    }
  }
  else sclDeviceMouse = "0";
  if (dwDeviceHeadPhone > 0)
  {
    switch (dwDeviceHeadPhone)
    {
    case 1:   // Values 1 ->  on Port 1
      sclDeviceHeadPhone = "1";
      break;
    case 2:   // Values 2 ->  on Port 2 
      sclDeviceHeadPhone = "2";
      break;
    case 4:   // Values 4 ->  on Port 3 
      sclDeviceHeadPhone = "3";
      break;
    case 8:   // Values 8 ->  on Port 4
      sclDeviceHeadPhone = "4";
      break;
    default:
      sclDeviceHeadPhone = "-";
    }
  }
  else sclDeviceHeadPhone = "0";
  if (dwDeviceMicroPhone > 0)
  {
    switch (dwDeviceMicroPhone)
    {
    case 1:   // Values 1 ->  on Port 1 
      sclDeviceMicroPhone = "1";
      break;
    case 2:   // Values 2 ->  on Port 2 
      sclDeviceMicroPhone = "2";
      break;
    case 4:   // Values 4 ->  on Port 3 
      sclDeviceMicroPhone = "3";
      break;
    case 8:   // Values 8 ->  on Port 4
      sclDeviceMicroPhone = "4";
      break;
    default:
      sclDeviceMicroPhone = "-";
    }
  }
  else sclDeviceMicroPhone ="0";
  if (dwDeviceMemory > 0)
  {
    switch (dwDeviceMemory)
    {
    case 1:   // Values 1 ->  on Port 1 
      sclDeviceMemory = "1";
      break;
    case 2:   // Values 2 ->  on Port 2
      sclDeviceMemory = "2";
      break;
    case 4:   // Values 4 ->  on Port 3 
      sclDeviceMemory = "3";
      break;
    case 8:   // Values 8 ->  on Port 4
      sclDeviceMemory = "4";
      break;
    case 3:   // Values 3 ->  on Port 1&2 
      sclDeviceMemory = "1, 2";
      break;
    case 5:   // Values 5 ->  on Port 1&3 
      sclDeviceMemory = "1, 3";
      break;
    case 6:   // Values 6 ->  on Port 2&3 
      sclDeviceMemory = "2, 3";
      break;
    case 7:   // Values 7 ->  on Port 1&2&3
      sclDeviceMemory = "1, 2, 3";
      break;
    case 9:   // Values 9  -> on Port 1&4 
      sclDeviceMemory = "1, 4";
      break;
    case 10:  // Values 10 -> on Port 2&4 
      sclDeviceMemory = "2, 4";
      break;
    case 11:  // Values 11 -> on Port 1&2&4
      sclDeviceMemory = "1, 2, 4";
      break;
    case 12:  // Values 12 -> on Port 3&4
      sclDeviceMemory = "3, 4";
      break;
    case 13:  // Values 13 -> on Port 1&3&4
      sclDeviceMemory = "1, 3, 4";
      break;
    case 14:  // Values 14 -> on Port 2&3&4
      sclDeviceMemory = "2, 3, 4";
      break;
    case 15:  // Values 15 -> on Port 1&2&3&4
      sclDeviceMemory = "1, 2, 3, 4";
      break;
    default:
      sclDeviceMemory = "-";
    }
  }
  else sclDeviceMemory = "0";
  if(dwDeviceIRRemote > 0)
  {
    switch (dwDeviceIRRemote)
    {
    case 1:   // Values 1 ->  on Port 1 
      sclDeviceIRRemote = "1";
      break;
    case 2:   // Values 2 ->  on Port 2 
      sclDeviceIRRemote = "2";
      break;
    case 4:   // Values 4 ->  on Port 3 
      sclDeviceIRRemote = "3";
      break;
    case 8:   // Values 8 ->  on Port 4
      sclDeviceIRRemote = "4";
      break;
    default:
      sclDeviceIRRemote = "-";
    }
  }
  else sclDeviceIRRemote = "0";

  CStdString strItem1, strItem2, strItem3, strItem4;
  CStdString strItem5, strItem6, strItem7, strItem8;

  strItem1.Format("%s %s", strlblGamePads.c_str(), sclDeviceVle);
  strItem2.Format("%s %s", strlblKeyboard.c_str(), sclDeviceKeyb);
  strItem3.Format("%s %s", strlblMouse.c_str(), sclDeviceMouse);
  strItem4.Format("%s %s %s", strItem1.c_str(), strItem2.c_str(), strItem3.c_str());

  strItem5.Format("%s %s",  strlblIRRemote.c_str(), sclDeviceIRRemote);
  //strItem6.Format("%s %s-%s", strlblHeadMicro.c_str(), sclDeviceHeadPhone, sclDeviceMicroPhone );
  strItem6.Format("%s %s", strlblHeadMicro.c_str(), sclDeviceHeadPhone, sclDeviceMicroPhone );  // Head and Micro are normly on the same port!
  strItem7.Format("%s %s", strItem5.c_str(), strItem6.c_str());

  // !? Show Memory stick, because it only shows with USB->MemoryStick adapter!
  strItem8.Format("%s %s", strlblMemoryStk.c_str(), sclDeviceMemory);
  SET_CONTROL_LABEL(i_lblp3, strItem8); // MemoryStick

  CLog::Log(LOGDEBUG,"- GamePads are Connected on Port:   %s (%d)", sclDeviceVle, dwDeviceGamePad );
  CLog::Log(LOGDEBUG,"- Keyboard is Connected on Port:    %s (%d)", sclDeviceKeyb, dwDeviceKeyboard);
  CLog::Log(LOGDEBUG,"- Mouse is Connected on Port:       %s (%d)", sclDeviceMouse, dwDeviceMouse);
  CLog::Log(LOGDEBUG,"- IR-Remote is Connected on Port:   %s (%d)", sclDeviceIRRemote, dwDeviceIRRemote);
  CLog::Log(LOGDEBUG,"- Head Phone is Connected on Port:  %s (%d)", sclDeviceHeadPhone, dwDeviceHeadPhone);
  CLog::Log(LOGDEBUG,"- Micro Phone is Connected on Port: %s (%d)", sclDeviceMicroPhone, dwDeviceMicroPhone);
  CLog::Log(LOGDEBUG,"- MemoryStick is Connected on Port: %d (%d)", dwDeviceMemory, dwDeviceMemory);

  SET_CONTROL_LABEL(i_lblp1, strItem4); // GamePad, Keyboard, Mouse
  SET_CONTROL_LABEL(i_lblp2, strItem7); // Remote, Headset & MicroPhone
  return true;
}

void CGUIWindowSystemInfo::CreateEEPROMBackup(LPCSTR BackupFilePrefix)
{
  //m_pXKEEPROM->ReadFromXBOX();

  //save current eeprom context..
  CHAR tmpFileName[FILENAME_MAX];
  ZeroMemory(tmpFileName, FILENAME_MAX);

  strcat(tmpFileName, cTempEEPROMBackUPPath );
  strcat(tmpFileName, BackupFilePrefix);
  strcat(tmpFileName, "_eeprom.bin");

  DWORD dwBytesWrote = 0;
  HANDLE hf = CreateFile(tmpFileName, GENERIC_WRITE, FILE_SHARE_WRITE, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
  if (hf !=  INVALID_HANDLE_VALUE)
  {
    //Write EEPROM File
    WriteFile(hf , &m_EEPROMData, EEPROM_SIZE, &dwBytesWrote, NULL);
  }
  CloseHandle(hf); 


  //Create Current EEPROM CFG File
  CHAR tmpData[256];
  ZeroMemory(tmpData, 256);
  DWORD tmpSize = 256;

  ZeroMemory(tmpFileName, FILENAME_MAX);
  strcat(tmpFileName, cTempEEPROMBackUPPath );
  strcat(tmpFileName, BackupFilePrefix);
  strcat(tmpFileName, "_eeprom.cfg");

  HANDLE hfa = CreateFile(tmpFileName, GENERIC_WRITE, FILE_SHARE_WRITE, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
  if (hf !=  INVALID_HANDLE_VALUE)
  {
    //Write CFG File Header..
    LPSTR fHeaderInfo = "#Please note ALL fields and Values are Case Sensitive !!\r\n\r\n[EEPROMDATA]\r\n";
    WriteFile(hfa, fHeaderInfo, (DWORD)strlen(fHeaderInfo), &dwBytesWrote, NULL);

    //Write Serial Number
    fHeaderInfo = "XBOXSerial\t= \"";
    WriteFile(hfa, fHeaderInfo, (DWORD)strlen(fHeaderInfo), &dwBytesWrote, NULL);
    WriteFile(hfa, m_EEPROMData.SerialNumber, SERIALNUMBER_SIZE, &dwBytesWrote, NULL);
    fHeaderInfo = "\"\r\n";
    WriteFile(hfa, fHeaderInfo, (DWORD)strlen(fHeaderInfo), &dwBytesWrote, NULL);

    //Write MAC Address..
    fHeaderInfo = "XBOXMAC\t\t= \"";
    WriteFile(hfa, fHeaderInfo, (DWORD)strlen(fHeaderInfo), &dwBytesWrote, NULL);
    ZeroMemory(tmpData, tmpSize);
    XKGeneral::BytesToHexStr(m_EEPROMData.MACAddress, MACADDRESS_SIZE, tmpData, ':');
    strupr(tmpData);
    WriteFile(hfa, tmpData, (DWORD)strlen(tmpData), &dwBytesWrote, NULL);
    fHeaderInfo = "\"\r\n";
    WriteFile(hfa, fHeaderInfo, (DWORD)strlen(fHeaderInfo), &dwBytesWrote, NULL);

    //Write Online Key ..
    fHeaderInfo = "\r\nOnlineKey\t= \"";
    WriteFile(hfa, fHeaderInfo, (DWORD)strlen(fHeaderInfo), &dwBytesWrote, NULL);
    ZeroMemory(tmpData, tmpSize);
    XKGeneral::BytesToHexStr(m_EEPROMData.OnlineKey, ONLINEKEY_SIZE, tmpData, ':');
    strupr(tmpData);
    WriteFile(hfa, tmpData, (DWORD)strlen(tmpData), &dwBytesWrote, NULL);
    fHeaderInfo = "\"\r\n";
    WriteFile(hfa, fHeaderInfo, (DWORD)strlen(fHeaderInfo), &dwBytesWrote, NULL);

    //Write VideoMode ..
    fHeaderInfo = "\r\n#ONLY Use NTSC or PAL for VideoMode\r\n";
    WriteFile(hfa, fHeaderInfo, (DWORD)strlen(fHeaderInfo), &dwBytesWrote, NULL);
    fHeaderInfo = "VideoMode\t= \"";
    WriteFile(hfa, fHeaderInfo, (DWORD)strlen(fHeaderInfo), &dwBytesWrote, NULL);

    VIDEO_STANDARD vdo = (VIDEO_STANDARD) *((LPDWORD)&m_EEPROMData.VideoStandard);
    if (vdo == m_pXKEEPROM->PAL_I)
      fHeaderInfo = "PAL";
    else
      fHeaderInfo = "NTSC";
    WriteFile(hfa, fHeaderInfo, (DWORD)strlen(fHeaderInfo), &dwBytesWrote, NULL);
    fHeaderInfo = "\"\r\n";
    WriteFile(hfa, fHeaderInfo, (DWORD)strlen(fHeaderInfo), &dwBytesWrote, NULL);

    //Write XBE Region..
    fHeaderInfo = "\r\n#ONLY Use 01, 02 or 04 for XBE Region\r\n";
    WriteFile(hfa, fHeaderInfo, (DWORD)strlen(fHeaderInfo), &dwBytesWrote, NULL);
    fHeaderInfo = "XBERegion\t= \"";
    WriteFile(hfa, fHeaderInfo, (DWORD)strlen(fHeaderInfo), &dwBytesWrote, NULL);
    ZeroMemory(tmpData, tmpSize);
    XKGeneral::BytesToHexStr(m_EEPROMData.XBERegion, XBEREGION_SIZE, tmpData, 0x00);
    strupr(tmpData);
    WriteFile(hfa, tmpData, (DWORD)strlen(tmpData), &dwBytesWrote, NULL);
    fHeaderInfo = "\"\r\n";
    WriteFile(hfa, fHeaderInfo, (DWORD)strlen(fHeaderInfo), &dwBytesWrote, NULL);

    //Write HDDKey..
    fHeaderInfo = "\r\nHDDKey\t\t= \"";
    WriteFile(hfa, fHeaderInfo, (DWORD)strlen(fHeaderInfo), &dwBytesWrote, NULL);
    ZeroMemory(tmpData, tmpSize);
    XKGeneral::BytesToHexStr(m_EEPROMData.HDDKkey, HDDKEY_SIZE, tmpData, ':');
    strupr(tmpData);
    WriteFile(hfa, tmpData, (DWORD)strlen(tmpData), &dwBytesWrote, NULL);
    fHeaderInfo = "\"\r\n";
    WriteFile(hfa, fHeaderInfo, (DWORD)strlen(fHeaderInfo), &dwBytesWrote, NULL);

    //Write Confounder..
    fHeaderInfo = "Confounder\t= \"";
    WriteFile(hfa, fHeaderInfo, (DWORD)strlen(fHeaderInfo), &dwBytesWrote, NULL);
    ZeroMemory(tmpData, tmpSize);
    XKGeneral::BytesToHexStr(m_EEPROMData.Confounder, CONFOUNDER_SIZE, tmpData, ':');
    strupr(tmpData);
    WriteFile(hfa, tmpData, (DWORD)strlen(tmpData), &dwBytesWrote, NULL);
    fHeaderInfo = "\"\r\n";
    WriteFile(hfa, fHeaderInfo, (DWORD)strlen(fHeaderInfo), &dwBytesWrote, NULL);
  }
  CloseHandle(hfa); 

  //Create Full path for TXT File..
  //ZeroMemory(tmpFileName, FILENAME_MAX  );
  //strcat(tmpFileName, (LPCSTR)cTempEEPROMBackUPPath);
  //strcat(tmpFileName, BackupFilePrefix);
  //strcat(tmpFileName, ".TXT");

  //Write XBOX Information into .TXT file...
  //WriteTXTInfoFile(tmpFileName);


  //switch eeprom context Back to previous
  //if (EncryptedState)
  //  m_pXKEEPROM->SetEncryptedEEPROMData(&currentEEPROM);
  //else 
  //  m_pXKEEPROM->SetDecryptedEEPROMData(m_XBOX_Version, &currentEEPROM);
}

void CGUIWindowSystemInfo::WriteTXTInfoFile(LPCSTR strFilename)
{
  BOOL retVal = FALSE;
  DWORD dwBytesWrote = 0;
  CHAR tmpData[256];
  LPSTR tmpFileStr = new CHAR[2048];
  DWORD tmpSize = 256;
  ZeroMemory(tmpData, tmpSize);
  ZeroMemory(tmpFileStr, 2048);

  HANDLE hf = CreateFile(strFilename, GENERIC_WRITE, FILE_SHARE_WRITE, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
  if (hf !=  INVALID_HANDLE_VALUE)
  {
    //Write File Header..
    strcat(tmpFileStr, "*******  XBOXMEDIACENTER [XBMC] INFORMATION FILE  *******\r\n");
    if (m_XBOX_Version== m_pXKEEPROM->V1_0)
      strcat(tmpFileStr, "\r\nXBOX Version = \t\tV1.0");
    else if (m_XBOX_Version == m_pXKEEPROM->V1_1)
      strcat(tmpFileStr, "\r\nXBOX Version = \t\tV1.1");
    else if (m_XBOX_Version == m_pXKEEPROM->V1_6) 
      strcat(tmpFileStr,  "\r\nXBOX Version = \t\tV1.6");
    //Get Kernel Version
    tmpSize = 256;ZeroMemory(tmpData, tmpSize);
    sprintf(tmpData, "\r\nKernel Version: \t%d.%d.%d.%d", *((USHORT*)XboxKrnlVersion),*((USHORT*)XboxKrnlVersion+1),*((USHORT*)XboxKrnlVersion+2),*((USHORT*)XboxKrnlVersion+3));
    strcat(tmpFileStr, tmpData);

    //Get Memory Status
    strcat(tmpFileStr, "\r\nXBOX RAM = \t\t");
    tmpSize = 256;ZeroMemory(tmpData, tmpSize);
    MEMORYSTATUS stat;
    GlobalMemoryStatus( &stat );
    ltoa(stat.dwTotalPhys/1024/1024, tmpData, 10);
    strcat(tmpFileStr, tmpData);
    strcat(tmpFileStr, " MBytes");

    //Write Serial Number..
    strcat(tmpFileStr, "\r\n\r\nXBOX Serial Number = \t");
    tmpSize = 256;
    ZeroMemory(tmpData, tmpSize);
    strncpy(tmpData ,(LPSTR)&m_EEPROMData.SerialNumber, SERIALNUMBER_SIZE);
    strcat(tmpFileStr, tmpData);

    //Write MAC Address..
    strcat(tmpFileStr, "\r\nXBOX MAC Address = \t");
    tmpSize = 256;ZeroMemory(tmpData, tmpSize);
    strncpy(tmpData ,(LPSTR)&m_EEPROMData.MACAddress, MACADDRESS_SIZE);
    strcat(tmpFileStr, tmpData);

    /*
    //Write Online Key ..
    strcat(tmpFileStr, "\r\nXBOX Online Key = \t");
    tmpSize = 256;ZeroMemory(tmpData, tmpSize);
    strncpy(tmpData ,(LPSTR)&m_EEPROMData.OnlineKey, ONLINEKEY_SIZE);
    strcat(tmpFileStr, tmpData);

    //Write VideoMode ..
    strcat(tmpFileStr, "\r\nXBOX Video Mode = \t");
    VIDEO_STANDARD vdo = (VIDEO_STANDARD) *((LPDWORD)&m_EEPROMData.VideoStandard);
    if (vdo == XKEEPROM::VIDEO_STANDARD::PAL_I)
    strcat(tmpFileStr, "PAL");
    else
    strcat(tmpFileStr, "NTSC");

    //Write XBE Region..
    strcat(tmpFileStr, "\r\nXBOX XBE Region = \t");
    tmpSize = 256;ZeroMemory(tmpData, tmpSize);
    //m_pXKEEPROM->GetXBERegionString(tmpData, &tmpSize);
    LPSTR XBERegion;
    //BytesToHexStr((LPBYTE)&m_EEPROMData.XBERegion, XBEREGION_SIZE, XBERegion);
    //strcat(tmpFileStr, XBERegion);

    //Write HDDKey..
    strcat(tmpFileStr, "\r\nXBOX HDD Key = \t\t");
    tmpSize = 256;ZeroMemory(tmpData, tmpSize);
    //m_pXKEEPROM->GetHDDKeyString(tmpData, &tmpSize);
    //strcat(tmpFileStr, tmpData);

    //Write Confounder..
    strcat(tmpFileStr, "\r\nXBOX Confounder = \t");
    tmpSize = 256;ZeroMemory(tmpData, tmpSize);
    //m_pXKEEPROM->GetConfounderString(tmpData, &tmpSize);
    //strcat(tmpFileStr, tmpData);

    //GET HDD Info...
    //Query ATA IDENTIFY
    XKHDD::ATA_COMMAND_OBJ cmdObj;
    ZeroMemory(&cmdObj, sizeof(XKHDD::ATA_COMMAND_OBJ));
    cmdObj.IPReg.bCommandReg = IDE_ATA_IDENTIFY;
    cmdObj.IPReg.bDriveHeadReg = IDE_DEVICE_MASTER;
    XKHDD::SendATACommand(IDE_PRIMARY_PORT, &cmdObj, IDE_COMMAND_READ);

    //Write HDD Model
    strcat(tmpFileStr, "\r\n\r\nXBOX HDD Model = \t");
    tmpSize = 256;ZeroMemory(tmpData, tmpSize);
    XKHDD::GetIDEModel(cmdObj.DATA_BUFFER, (LPSTR)tmpData, &tmpSize);
    strcat(tmpFileStr, tmpData);

    //Write HDD Serial..
    strcat(tmpFileStr, "\r\nXBOX HDD Serial = \t");
    tmpSize = 256;ZeroMemory(tmpData, tmpSize);
    XKHDD::GetIDESerial(cmdObj.DATA_BUFFER, (LPSTR)tmpData, &tmpSize);
    strcat(tmpFileStr, tmpData);

    //Write HDD Password..
    tmpSize = 256;ZeroMemory(tmpData, tmpSize);
    strcat(tmpFileStr, "\r\n\r\nXBOX HDD Password = \t");

    //Need decrypted HDD Key to calculate password !!
    //BOOL OldState = m_pXKEEPROM->IsEncrypted();
    //if (OldState)
    //  m_pXKEEPROM->Decrypt();
    XKEEPROM::EEPROMDATA tmptEEP;
    //m_pXKEEPROM->GetEEPROMData(&tmptEEP);
    tmpSize = 256;ZeroMemory(tmpData, tmpSize);
    BYTE HDDpwd[20];ZeroMemory(HDDpwd, 20);
    XKHDD::GenerateHDDPwd((UCHAR*)&tmptEEP.HDDKkey, cmdObj.DATA_BUFFER, (UCHAR*)&HDDpwd);
    XKGeneral::BytesToHexStr(HDDpwd, 20, tmpData);
    strcat(tmpFileStr, tmpData);
    //if (OldState)
    //m_pXKEEPROM->EncryptAndCalculateCRC(m_XBOX_Version);

    //Query ATAPI IDENTIFY
    ZeroMemory(&cmdObj, sizeof(XKHDD::ATA_COMMAND_OBJ));
    cmdObj.IPReg.bCommandReg = IDE_ATAPI_IDENTIFY;
    cmdObj.IPReg.bDriveHeadReg = IDE_DEVICE_SLAVE;
    XKHDD::SendATACommand(IDE_PRIMARY_PORT, &cmdObj, IDE_COMMAND_READ);

    //Write DVD Model
    strcat(tmpFileStr, "\r\n\r\nXBOX DVD Model = \t");
    tmpSize = 256;ZeroMemory(tmpData, tmpSize);
    XKHDD::GetIDEModel(cmdObj.DATA_BUFFER, (LPSTR)tmpData, &tmpSize);
    strcat(tmpFileStr, tmpData);
    */
    strupr(tmpFileStr);
    WriteFile(hf, tmpFileStr, (DWORD)strlen(tmpFileStr), &dwBytesWrote, NULL);
  }
  delete[] tmpFileStr;
  CloseHandle(hf);
}
bool CGUIWindowSystemInfo::GetSystemUpTime(CStdString& strSystemUptime)
{
  CStdString lbl1 = g_localizeStrings.Get(12390);
  CStdString lblMin = g_localizeStrings.Get(12391);
  CStdString lblHou = g_localizeStrings.Get(12392);
  CStdString lblDay = g_localizeStrings.Get(12393);

  int iInputMinutes, iMinutes,iHours,iDays;
  iInputMinutes = (int)(timeGetTime() / 60000);
  g_sysinfo.SystemUpTime(iInputMinutes,iMinutes, iHours, iDays);
  // Will Display Autodetected Values!
  if (iDays > 0) strSystemUptime.Format("%s: %i %s, %i %s, %i %s",lbl1.c_str(), iDays,lblDay.c_str(), iHours,lblHou.c_str(), iMinutes,lblMin.c_str());
  else if (iDays == 0 && iHours >= 1 ) strSystemUptime.Format("%s: %i %s, %i %s",lbl1.c_str(), iHours,lblHou.c_str(), iMinutes,lblMin.c_str());
  else if (iDays == 0 && iHours == 0 &&  iMinutes >= 0) strSystemUptime.Format("%s: %i %s",lbl1, iMinutes,lblMin.c_str());

  return true;
}
bool CGUIWindowSystemInfo::GetSystemTotalUpTime(CStdString& strSystemUptime)
{
  CStdString lbl1 = g_localizeStrings.Get(12394);
  CStdString lblMin = g_localizeStrings.Get(12391);
  CStdString lblHou = g_localizeStrings.Get(12392);
  CStdString lblDay = g_localizeStrings.Get(12393);

  int iInputMinutes, iMinutes,iHours,iDays;
  iInputMinutes = g_stSettings.m_iSystemTimeTotalUp + ((int)(timeGetTime() / 60000));
  g_sysinfo.SystemUpTime(iInputMinutes,iMinutes, iHours, iDays);
  // Will Display Autodetected Values!
  if (iDays > 0) strSystemUptime.Format("%s: %i %s, %i %s, %i %s",lbl1.c_str(), iDays,lblDay.c_str(), iHours,lblHou.c_str(), iMinutes,lblMin.c_str());
  else if (iDays == 0 && iHours >= 1 ) strSystemUptime.Format("%s: %i %s, %i %s",lbl1.c_str(), iHours,lblHou.c_str(), iMinutes,lblMin.c_str());
  else if (iDays == 0 && iHours == 0 &&  iMinutes >= 0) strSystemUptime.Format("%s: %i %s",lbl1.c_str(), iMinutes,lblMin.c_str());

  return true;
}