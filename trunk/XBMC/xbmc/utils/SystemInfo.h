#pragma once

#include "md5.h"
#include "../xbox/xkeeprom.h"
#include "InfoLoader.h"
#include "GUIInfoManager.h"

#define KB  (1024)          // 1 KiloByte (1KB)   1024 Byte (2^10 Byte)
#define MB  (1024*KB)       // 1 MegaByte (1MB)   1024 KB (2^10 KB)
#define GB  (1024*MB)       // 1 GigaByte (1GB)   1024 MB (2^10 MB)
#define TB  (1024*GB)       // 1 TerraByte (1TB)  1024 GB (2^10 GB)

#define SMARTXX_LED_OFF        0 // SmartXX ModCHIP LED Controll
#define SMARTXX_LED_BLUE       1
#define SMARTXX_LED_RED        2
#define SMARTXX_LED_BLUE_RED   3
#define SMARTXX_LED_CYCLE      4

#define MAX_KNOWN_ATTRIBUTES  46
struct Bios
{
 char Name[50];
 char Signature[33];
};
class CBackgroundSystemInfoLoader : public CBackgroundLoader
{
public:
  CBackgroundSystemInfoLoader(CInfoLoader *pCallback) : CBackgroundLoader(pCallback) {};

protected:
  virtual void GetInformation();
};
class CSysInfo : public CInfoLoader
{
  public:
    CSysInfo(void);
    virtual ~CSysInfo();

    char MD5_Sign[32 + 1];

    bool GetDVDInfo(CStdString& strDVDModel, CStdString& strDVDFirmware);
    bool GetHDDInfo(CStdString& strHDDModel, CStdString& strHDDSerial,CStdString& strHDDFirmware,CStdString& strHDDpw,CStdString& strHDDLockState);
    bool GetRefurbInfo(CStdString& rfi_FirstBootTime, CStdString& rfi_PowerCycleCount);

    bool CreateBiosBackup();
    bool CreateEEPROMBackup();
    void WriteTXTInfoFile();

    CStdString SmartXXModCHIP();
    CStdString GetAVPackInfo();
    CStdString GetVideoEncoder();
    CStdString GetMPlayerVersion();
    CStdString GetKernelVersion();
    CStdString GetSystemUpTime(bool bTotalUptime);
    CStdString GetCPUFreqInfo();
    CStdString GetXBVerInfo();
    CStdString GetUnits(int iFrontPort);
    CStdString GetMACAddress();
    CStdString GetXBOXSerial(bool bLabel);
    CStdString GetXBProduceInfo();
    CStdString GetVideoXBERegion();
    CStdString GetDVDZone();
    CStdString GetXBLiveKey();
    CStdString GetHDDKey();
    CStdString GetModChipInfo();
    CStdString GetBIOSInfo();
    CStdString GetInternetState();
    CStdString GetTrayState();
    bool GetDiskSpace(const CStdString drive,int& iTotal, int& iTotalFree, int& iTotalUsed, int& iPercentFree, int& iPercentUsed);
    CStdString GetHddSpaceInfo(int drive, bool shortText=false);

    CStdString m_XboxBios;
    CStdString m_XboxModChip;
    CStdString m_mplayerversion;
    CStdString m_kernelversion;
    CStdString m_cpufrequency;
    CStdString m_xboxversion;
    CStdString m_avpackinfo;
    CStdString m_videoencoder;
    CStdString m_xboxserial;
    CStdString m_hddlockkey;
    CStdString m_hddbootdate;
    CStdString m_hddcyclecount;
    CStdString m_macadress;
    CStdString m_videoxberegion;
    CStdString m_videodvdzone;
    CStdString m_produceinfo;
    CStdString m_InternetState;
    CStdString m_systemtotaluptime;
    CStdString m_systemuptime;

    CStdString m_HDDModel, m_HDDSerial,m_HDDFirmware,m_HDDpw,m_HDDLockState;
    CStdString m_DVDModel, m_DVDFirmware;

    bool m_bInternetState;
    bool m_bRequestDone;
    bool m_bSmartSupported;
    bool m_bSmartEnabled;

    bool m_hddRequest;
    bool m_dvdRequest;

    signed char byHddTemp;

  private:
    #define XBOX_BIOS_ID_INI_FILE "Q:\\System\\SystemInfo\\BiosIDs.ini"
    #define XBOX_BIOS_BACKUP_FILE "Q:\\System\\SystemInfo\\BIOSBackup.bin"
    #define XBOX_EEPROM_BIN_BACKUP_FILE "Q:\\System\\SystemInfo\\EEPROMBackup.bin"
    #define XBOX_EEPROM_CFG_BACKUP_FILE "Q:\\System\\SystemInfo\\EEPROMBackup.cfg"
    #define XBOX_XBMC_TXT_INFOFILE "Q:\\System\\SystemInfo\\XBMCSystemInfo.txt"
    #define SYSINFO_TMP_SIZE 256
    #define XDEVICE_TYPE_IR_REMOTE  (&XDEVICE_TYPE_IR_REMOTE_TABLE)
    #define DEBUG_KEYBOARD
    #define DEBUG_MOUSE

    CStdString m_temp;
    XKEEPROM* m_XKEEPROM;
    XBOX_VERSION  m_XBOXVersion;

    double GetCPUFrequency();
    double RDTSC(void);
    bool SystemUpTime(int iInputMinutes, int &iMinutes, int &iHours, int &iDays);
    bool GetXBOXVersionDetected(CStdString& strXboxVer);
    CStdString GetModCHIPDetected();

    struct Bios * LoadBiosSigns();
    bool CheckBios(CStdString& strDetBiosNa);
    char* ReturnBiosName(char *buffer, char *str);
    char* ReturnBiosSign(char *buffer, char *str);
    char* CheckMD5 (struct Bios *Listone, char *Sign);
    char* MD5Buffer(char *filename,long PosizioneInizio,int KBytes);
    CStdString MD5BufferNew(char *filename,long PosizioneInizio,int KBytes);

    void Reset();

protected:
    virtual const char *TranslateInfo(DWORD dwInfo);
    virtual DWORD TimeToNextRefreshInMs();
};

extern CSysInfo g_sysinfo;