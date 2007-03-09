/*
* XBoxMediaPlayer
* Copyright (c) 2002 d7o3g4q and RUNTiME
* Portions Copyright (c) by the authors of ffmpeg and xvid
*
* This program is free software; you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation; either version 2 of the License, or
* (at your option) any later version.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with this program; if not, write to the Free Software
* Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
*/

// IoSupport.cpp: implementation of the CIoSupport class.
//
//////////////////////////////////////////////////////////////////////

#include "../stdafx.h"
#include "IoSupport.h"
#ifdef HAS_UNDOCUMENTED
#ifdef _XBOX
#include "Undocumented.h"
#include "../xbox/XKExports.h"
#else
#include "ntddcdrm.h"
#endif
#endif

#ifdef _XBOX
#define CTLCODE(DeviceType, Function, Method, Access) ( ((DeviceType) << 16) | ((Access) << 14) | ((Function) << 2) | (Method)  )

#define FSCTL_DISMOUNT_VOLUME  CTLCODE( FILE_DEVICE_FILE_SYSTEM, 0x08, METHOD_BUFFERED, FILE_ANY_ACCESS )
#endif

typedef struct
{
  char szDriveLetter;
  char* szDevice;
  int iPartition;
}
stDriveMapping;

#ifdef _XBOX
stDriveMapping driveMapping[] =
  {
    { 'C', "Harddisk0\\Partition2", 2},
    { 'D', "Cdrom0", -1},
    { 'E', "Harddisk0\\Partition1", 1},
    { 'F', "Harddisk0\\Partition6", 6},
    { 'X', "Harddisk0\\Partition3", 3},
    { 'Y', "Harddisk0\\Partition4", 4},
    { 'Z', "Harddisk0\\Partition5", 5},
    { 'G', "Harddisk0\\Partition7", 7},
  };
#else
stDriveMapping driveMapping[] =
  {
    { 'C', "C:", 2},
    { 'D', "D:", -1},
    { 'E', "E:", 1},
    { 'F', "F:", 6},
    { 'X', "X:", 3},
    { 'Y', "Y:", 4},
    { 'Z', "Z:", 5},
    { 'G', "G:", 7},
  };

#include "../../Tools/Win32/XBMC_PC.h"
#endif
#define NUM_OF_DRIVES ( sizeof( driveMapping) / sizeof( driveMapping[0] ) )


#ifdef _XBOX
PARTITION_TABLE *CIoSupport::m_partitionTable = NULL;

unsigned int CIoSupport::read_active_partition_table(PARTITION_TABLE *p_table)
{
  ANSI_STRING a_file;
  OBJECT_ATTRIBUTES obj_attr;
  IO_STATUS_BLOCK io_stat_block;
  HANDLE handle;
  unsigned int stat;
  unsigned int ioctl_cmd_in_buf[100];
  unsigned int ioctl_cmd_out_buf[100];
  unsigned int partition_table_addr;

  memset(p_table, 0, sizeof(PARTITION_TABLE));

  RtlInitAnsiString(&a_file, "\\Device\\Harddisk0\\partition0");
  obj_attr.RootDirectory = 0;
  obj_attr.ObjectName = &a_file;
  obj_attr.Attributes = OBJ_CASE_INSENSITIVE;

  stat = NtOpenFile(&handle, (GENERIC_READ | 0x00100000), &obj_attr, &io_stat_block, (FILE_SHARE_READ | FILE_SHARE_WRITE), 0x10);

  if (stat != STATUS_SUCCESS)
  {
    return stat;
  }

  memset(ioctl_cmd_out_buf, 0, sizeof(ioctl_cmd_out_buf));
  memset(ioctl_cmd_in_buf, 0, sizeof(ioctl_cmd_in_buf));
  ioctl_cmd_in_buf[0] = IOCTL_SUBCMD_GET_INFO;


  stat = NtDeviceIoControlFile(handle, 0, 0, 0, &io_stat_block,
                               IOCTL_CMD_LBA48_ACCESS,
                               ioctl_cmd_in_buf, sizeof(ioctl_cmd_in_buf),
                               ioctl_cmd_out_buf, sizeof(ioctl_cmd_out_buf));

  NtClose(handle);
  if (stat != STATUS_SUCCESS)
  {
    return stat;
  }

  if ((ioctl_cmd_out_buf[LBA48_GET_INFO_MAGIC1_IDX] != LBA48_GET_INFO_MAGIC1_VAL) ||
      (ioctl_cmd_out_buf[LBA48_GET_INFO_MAGIC2_IDX] != LBA48_GET_INFO_MAGIC2_VAL))
  {

    return STATUS_UNSUCCESSFUL;
  }

  partition_table_addr = ioctl_cmd_out_buf[LBA48_GET_INFO_LOWCODE_BASE_IDX];
  partition_table_addr += ioctl_cmd_out_buf[LBA48_GET_INFO_PART_TABLE_OFS_IDX];

  memcpy(p_table, (void *)partition_table_addr, sizeof(PARTITION_TABLE));

  return STATUS_SUCCESS;
}
#endif

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CIoSupport::CIoSupport()
{
  m_gmXferBuffer = GlobalAlloc(GPTR, RAW_SECTOR_SIZE);
  m_rawXferBuffer = NULL;
  if ( m_gmXferBuffer )
    m_rawXferBuffer = GlobalLock(m_gmXferBuffer);
}

CIoSupport::CIoSupport(CIoSupport& other)
{
  m_gmXferBuffer = GlobalAlloc(GPTR, RAW_SECTOR_SIZE);
  m_rawXferBuffer = NULL;
  if ( m_gmXferBuffer )
    m_rawXferBuffer = GlobalLock(m_gmXferBuffer);
}

CIoSupport::~CIoSupport()
{
  if ( m_gmXferBuffer )
  {
    GlobalUnlock(m_gmXferBuffer);
    GlobalFree(m_gmXferBuffer);
  }
}

// szDrive e.g. "D:"
// szDevice e.g. "Cdrom0" or "Harddisk0\Partition6"

HRESULT CIoSupport::Mount(const char* szDrive, char* szDevice)
{
#ifdef _XBOX
  char szSourceDevice[256];
  char szDestinationDrive[16];

  if (!PartitionExists(szDevice) ) return S_OK;

  sprintf(szSourceDevice, "\\Device\\%s", szDevice);
  sprintf(szDestinationDrive, "\\??\\%s", szDrive);

  STRING DeviceName =
    {
      strlen(szSourceDevice),
      strlen(szSourceDevice) + 1,
      szSourceDevice
    };

  STRING LinkName =
    {
      strlen(szDestinationDrive),
      strlen(szDestinationDrive) + 1,
      szDestinationDrive
    };

//  CLog::Log(LOGERROR, "Calling IoCreateSymbolicLink()");
  IoCreateSymbolicLink(&LinkName, &DeviceName);
//  CLog::Log(LOGERROR, "IoCreateSymbolicLink() done");
#else
  if ((strnicmp(szDevice, "Harddisk0", 9) == 0) ||
      (strnicmp(szDevice, "CDrom", 5) == 0))
    return S_OK;

  /*
  // ok, do the actual mounting then
  CStdString command;
  command.Format("subst %s %s", szDrive, szDevice);
  WinExec(command.c_str(), SW_HIDE);
  Sleep(500); // HACK: Give it time to run
  */
#endif
  return S_OK;
}



// szDrive e.g. "D:"

HRESULT CIoSupport::Unmount(const char* szDrive)
{
#ifdef _XBOX
  char szDestinationDrive[16];
  sprintf(szDestinationDrive, "\\??\\%s", szDrive);
  if (!DriveExists(szDrive) ) return S_OK;

  STRING LinkName =
    {
      strlen(szDestinationDrive),
      strlen(szDestinationDrive) + 1,
      szDestinationDrive
    };

  IoDeleteSymbolicLink(&LinkName);
#else
  /*if (szDrive[0] == 'Q' || szDrive[0] == 'P' || szDrive[0] == 'Q' || szDrive[0] == 'T')
  {
    CStdString command;
    command.Format("subst %s /D", szDrive);
    WinExec(command.c_str(), SW_HIDE);
    Sleep(500); // HACK! give it time to run
  }*/
#endif
  return S_OK;
}





HRESULT CIoSupport::Remount(LPCSTR szDrive, LPSTR szDevice)
{
#ifdef _XBOX
  if (!PartitionExists(szDevice) ) return S_OK;

  char szSourceDevice[48];
  sprintf(szSourceDevice, "\\Device\\%s", szDevice);

  Unmount(szDrive);

  ANSI_STRING filename;
  OBJECT_ATTRIBUTES attributes;
  IO_STATUS_BLOCK status;
  HANDLE hDevice;
  NTSTATUS error;
  DWORD dummy;

  RtlInitAnsiString(&filename, szSourceDevice);
  InitializeObjectAttributes(&attributes, &filename, OBJ_CASE_INSENSITIVE, NULL);

  if (NT_SUCCESS(error = NtCreateFile(&hDevice, GENERIC_READ |
                                      SYNCHRONIZE | FILE_READ_ATTRIBUTES, &attributes, &status, NULL, 0,
                                      FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, FILE_OPEN,
                                      FILE_NON_DIRECTORY_FILE | FILE_SYNCHRONOUS_IO_NONALERT)))
  {

    if (!DeviceIoControl(hDevice, FSCTL_DISMOUNT_VOLUME, NULL, 0, NULL, 0, &dummy, NULL))
    {
      CloseHandle(hDevice);
      return E_FAIL;
    }

    CloseHandle(hDevice);
  }

  Mount(szDrive, szDevice);
#endif
  return S_OK;
}

HRESULT CIoSupport::Remap(char* szMapping)
{
#ifdef _XBOX
  char szMap[32];
  strcpy(szMap, szMapping );

  char* pComma = strstr(szMap, ",");
  if (pComma)
  {
    *pComma = 0;
    if (!PartitionExists(&pComma[1]) ) return S_OK;

    // map device to drive letter
    Unmount(szMap);
    Mount(szMap, &pComma[1]);
    return S_OK;
  }
#endif
  return E_FAIL;
}


HRESULT CIoSupport::EjectTray()
{
#ifdef _XBOX
  HalWriteSMBusValue(0x20, 0x0C, FALSE, 0);  // eject tray
#endif
  return S_OK;
}

HRESULT CIoSupport::CloseTray()
{
#ifdef _XBOX
  HalWriteSMBusValue(0x20, 0x0C, FALSE, 1);  // close tray
#endif
  return S_OK;
}

DWORD CIoSupport::GetTrayState()
{
#ifdef _XBOX
  DWORD dwTrayState, dwTrayCount;
  if (g_advancedSettings.m_usePCDVDROM)
  {
    dwTrayState = TRAY_CLOSED_MEDIA_PRESENT;
  }
  else
  {
    HalReadSMCTrayState(&dwTrayState, &dwTrayCount);
  }

  return dwTrayState;
#endif
  return DRIVE_NOT_READY;
}

HRESULT CIoSupport::Shutdown()
{
#ifdef _XBOX
  // fails assertion on debug bios (symptom lockup unless running dr watson)
  // so you can continue past the failed assertion).
  if (IsDebug())
    return E_FAIL;
  KeRaiseIrqlToDpcLevel();
  HalInitiateShutdown();
#endif
  return S_OK;
}


VOID CIoSupport::RemountDrive(LPCSTR szDrive)
{
  // ugly, but it works ;-)
  for (int i = 0; i < NUM_OF_DRIVES; i++)
  {
    if (szDrive[0] == driveMapping[i].szDriveLetter)
    {
      Remount(szDrive, driveMapping[i].szDevice);
    }
  }
}

VOID CIoSupport::GetPartition(LPCSTR strFilename, LPSTR strPartition)
{
  strcpy(strPartition, "");
  for (int i = 0; i < NUM_OF_DRIVES; i++)
  {
    if ( toupper(strFilename[0]) == driveMapping[i].szDriveLetter)
    {
      strcpy(strPartition, driveMapping[i].szDevice);
      return ;
    }
  }
}

string CIoSupport::GetDrive(const string &szPartition)
{
  static string strDrive = "E:";
  for (int i = 0; i < NUM_OF_DRIVES; i++)
  {
    CStdString strMap = driveMapping[i].szDevice;
    CStdString strSearch = szPartition;
    CStdString strSearchLeft = strSearch.Left( strMap.size() );
    strMap.ToLower();
    strSearchLeft.ToLower();
    if ( strMap == strSearchLeft )
    {
      char szDrive[3];
      szDrive[0] = driveMapping[i].szDriveLetter;
      szDrive[1] = 0;
      strDrive = szDrive;
      return strDrive;
    }
  }
  return strDrive;
}

HANDLE CIoSupport::OpenCDROM()
{
  HANDLE hDevice;

  Remount("D:", "Cdrom0");

  if ( !m_rawXferBuffer )
    return NULL;

#ifdef _XBOX
  NTSTATUS error;
  IO_STATUS_BLOCK status;
  ANSI_STRING filename;
  OBJECT_ATTRIBUTES attributes;
  RtlInitAnsiString(&filename, "\\Device\\Cdrom0");
  InitializeObjectAttributes(&attributes, &filename, OBJ_CASE_INSENSITIVE, NULL);
  if (!NT_SUCCESS(error = NtCreateFile(&hDevice,
                                       GENERIC_READ | SYNCHRONIZE | FILE_READ_ATTRIBUTES,
                                       &attributes,
                                       &status,
                                       NULL,
                                       0,
                                       FILE_SHARE_READ,
                                       FILE_OPEN,
                                       FILE_NON_DIRECTORY_FILE | FILE_SYNCHRONOUS_IO_NONALERT)))
  {
    return NULL;
  }
#else

  hDevice = CreateFile("\\\\.\\Cdrom0", GENERIC_READ, FILE_SHARE_READ,
                       NULL, OPEN_EXISTING,
                       FILE_FLAG_RANDOM_ACCESS, NULL );

#endif
  return hDevice;
}

INT CIoSupport::ReadSector(HANDLE hDevice, DWORD dwSector, LPSTR lpczBuffer)

{
  DWORD dwRead;
  DWORD dwSectorSize = 2048;
  LARGE_INTEGER Displacement;

  Displacement.QuadPart = ((INT64)dwSector) * dwSectorSize;

  for (int i = 0; i < 5; i++)
  {
    SetFilePointer(hDevice, Displacement.LowPart, &Displacement.HighPart, FILE_BEGIN);

    if (ReadFile(hDevice, m_rawXferBuffer, dwSectorSize, &dwRead, NULL))
    {
      memcpy(lpczBuffer, m_rawXferBuffer, dwSectorSize);
      return dwRead;
    }
  }

  OutputDebugString("CD Read error\n");
  return -1;
}


INT CIoSupport::ReadSectorMode2(HANDLE hDevice, DWORD dwSector, LPSTR lpczBuffer)
{
#ifdef HAS_DVD_DRIVE
  DWORD dwBytesReturned;
  RAW_READ_INFO rawRead = {0};

  // Oddly enough, DiskOffset uses the Red Book sector size
  rawRead.DiskOffset.QuadPart = 2048 * dwSector;
  rawRead.SectorCount = 1;
  rawRead.TrackMode = XAForm2;

  for (int i = 0; i < 5; i++)
  {
    if ( DeviceIoControl( hDevice,
                          IOCTL_CDROM_RAW_READ,
                          &rawRead,
                          sizeof(RAW_READ_INFO),
                          m_rawXferBuffer,
                          RAW_SECTOR_SIZE,
                          &dwBytesReturned,
                          NULL ) != 0 )
    {
      memcpy(lpczBuffer, (byte*)m_rawXferBuffer + MODE2_DATA_START, MODE2_DATA_SIZE);
      return MODE2_DATA_SIZE;
    }
    else
    {
      int iErr = GetLastError();
      //   printf("%i\n", iErr);
    }
  }
#endif
  return -1;
}

INT CIoSupport::ReadSectorCDDA(HANDLE hDevice, DWORD dwSector, LPSTR lpczBuffer)
{
#ifdef HAS_DVD_DRIVE
  DWORD dwBytesReturned;
  RAW_READ_INFO rawRead;

  // Oddly enough, DiskOffset uses the Red Book sector size
  rawRead.DiskOffset.QuadPart = 2048 * dwSector;
  rawRead.SectorCount = 1;
  rawRead.TrackMode = CDDA;

  for (int i = 0; i < 5; i++)
  {
    if ( DeviceIoControl( hDevice,
                          IOCTL_CDROM_RAW_READ,
                          &rawRead,
                          sizeof(RAW_READ_INFO),
                          m_rawXferBuffer,
                          sizeof(RAW_SECTOR_SIZE),
                          &dwBytesReturned,
                          NULL ) != 0 )
    {
      memcpy(lpczBuffer, m_rawXferBuffer, RAW_SECTOR_SIZE);
      return RAW_SECTOR_SIZE;
    }
  }
#endif
  return -1;
}

VOID CIoSupport::CloseCDROM(HANDLE hDevice)
{
  CloseHandle(hDevice);
}

VOID CIoSupport::UpdateDvdrom()
{
#ifdef _XBOX
  OutputDebugString("Starting Dvdrom update.\n");
  BOOL bClosingTray = false;
  BOOL bShouldHaveClosed = false;

  // if the tray is open, close it
  DWORD dwCurrentState;
  do
  {
    dwCurrentState = GetTrayState();
    switch (dwCurrentState)
    {
    case DRIVE_OPEN:

      // drive is open
      if (!bClosingTray)
      {
        bClosingTray = true;

        OutputDebugString("Drive open, closing tray...\n");
        CloseTray();
      }
      else if (bShouldHaveClosed)
      {
        // the operation failed, we cannot stay in this loop
        OutputDebugString("Dvdrom ended (failed to retract tray).\n");
        return ;
      }
      break;
    case DRIVE_NOT_READY:
      // drive is not ready (closing, opening)
      OutputDebugString("Drive transition.\n");
      bShouldHaveClosed = bClosingTray;
      Sleep(6000);
      break;
    case DRIVE_READY:
      // drive is ready
      OutputDebugString("Drive ready.\n");
      break;
    case DRIVE_CLOSED_NO_MEDIA:
      // nothing in there...
      OutputDebugString("Drive closed no media.\n");
      break;
    case DRIVE_CLOSED_MEDIA_PRESENT:
      // drive has been closed and is ready
      OutputDebugString("Drive closed media present, remounting...\n");
      Remount("D:", "Cdrom0");
      break;
    }

  }
  while (dwCurrentState < DRIVE_READY);

  OutputDebugString("Dvdrom updated.\n");
#endif
}


// returns true if this is a debug machine
BOOL CIoSupport::IsDebug()
{
#ifdef _XBOX
  return (XboxKrnlVersion->Qfe & 0x8000) || ((DWORD)XboxHardwareInfo & 0x10);
#else
  return FALSE;
#endif
}


VOID CIoSupport::GetXbePath(char* szDest)
{
#ifdef _XBOX
  //Function to get the XBE Path like:
  //E:\DevKit\xbplayer\xbplayer.xbe

  PANSI_STRING pImageFileName = (PANSI_STRING)XeImageFileName;

  char szDevicePath[1024 + 1];
  char szHomeDrive[3];
  char szPartition[50];
  memset(szDevicePath, 0, sizeof(szDevicePath));
  memset(szHomeDrive, 0, sizeof(szHomeDrive));
  memset(szPartition, 0, sizeof(szPartition));
  strncpy(szDevicePath,
          &pImageFileName->Buffer[8],
          pImageFileName->Length - 8 ); //get xbepath (without \Device\ prefix)


  strcpy(szHomeDrive, GetDrive(szDevicePath).c_str()); //get driveletter of xbepath
  if (szHomeDrive[0])
  {
    //OutputDebugString("homedrive:");OutputDebugString(szHomeDrive);OutputDebugString("\n");

    GetPartition(szHomeDrive, szPartition);  //get patition (ie: Hardisk0\Partition1)
    //  OutputDebugString("partition:");OutputDebugString(szPartition);OutputDebugString("\n");
    strcpy(szDest, szHomeDrive);      //copy drive letter
    strcat(szDest, ":");          //copy drive letter
    strncat(szDest,              //concat the path
            &szDevicePath[strlen(szPartition)],
            strlen(szDevicePath) - strlen(szPartition));


  }
  else
  {
    OutputDebugString("cant get xbe path\n");
    szDest[0] = 0; //somehow we cant get the xbepath :(
  }
#else
//  extern CXBMC_PC *g_xbmcPC;
//  GetModuleFileName(g_xbmcPC->GetInstance(), szDest, XBMC_MAX_PATH);
  GetCurrentDirectory(XBMC_MAX_PATH, szDest);
  strcat(szDest, "\\XBMC_PC.exe");
#endif
}

bool CIoSupport::DriveExists(const char* szDrive)
{
  if (szDrive[0] == 'Q' || szDrive[0] == 'P' || szDrive[0] == 'T') return true;
  for (int i = 0; i < (int)NUM_OF_DRIVES; ++i)
  {
    if ( driveMapping[i].szDriveLetter == szDrive[0])
    {
      int iPartition = driveMapping[i].iPartition;
      if (iPartition < 0) return true;
      //if ( m_partitionTable->pt_entries[iPartition].pe_flags & PE_PARTFLAGS_IN_USE)
      //{
      return true;
      //}
      //return false;
    }
  }
  return false;
}
bool CIoSupport::PartitionExists(const char* szPartition)
{
  for (int i = 0; i < (int)NUM_OF_DRIVES; ++i)
  {
    if ( !strncmp(driveMapping[i].szDevice, szPartition, strlen(driveMapping[i].szDevice) ) )
    {
      int iPartition = driveMapping[i].iPartition;
      if (iPartition < 0) return true;
      //if ( m_partitionTable->pt_entries[iPartition].pe_flags & PE_PARTFLAGS_IN_USE)
      //{
      return true;
      //}
      //return false;
    }
  }
  return false;
}

// stolen from DVD2XBOX :P
bool CIoSupport::IsDrivePresent( const char* cDrive )
{
  bool bReturn = false;
  ULARGE_INTEGER uFree1, uTotal1, uTotal2;

  CIoSupport io;
  WIN32_FIND_DATA wfd;
  HANDLE hFind;
  char path[5];

  strcpy(path, cDrive);
  strcat(path, "\\*");

  bool bMapped = false;
  for (int i = 0; i < NUM_OF_DRIVES; i++)
  {
    if (driveMapping[i].szDriveLetter == cDrive[0])
    {
      char temp[32];
      sprintf(temp, "%s,%s", cDrive, driveMapping[i].szDevice);
      Remap(temp);
      bMapped = true;
    }
  }

  if (!bMapped)
  {
    return false;
  }

  hFind = FindFirstFile( path, &wfd );

  if ( INVALID_HANDLE_VALUE != hFind)
  {
    bReturn = true;
    FindClose( hFind );
  }
  else if ( GetDiskFreeSpaceEx( cDrive, &uFree1, &uTotal1, &uTotal2 ) )
  {
    bReturn = true;
  }
  else
  {
    // Should do a final check....
    char szVolume[256];
    char szFileSys[32];
    DWORD dwVolSer, dwMaxCompLen, dwFileSysFlags;
    if ( GetVolumeInformation( cDrive, szVolume, 255, &dwVolSer, &dwMaxCompLen, &dwFileSysFlags, szFileSys, 32 ) )
    {
      bReturn = true;
    }
  }
  Unmount(cDrive);
  return bReturn;
}
