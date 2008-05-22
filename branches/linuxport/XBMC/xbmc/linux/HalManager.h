#ifdef HAS_HAL
#ifndef HALMANAGER_H
#define HALMANAGER_H

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

#include "../../guilib/system.h"
#include <string.h>
#include <stdio.h>
#include <dbus/dbus.h>
#include <libhal.h>
#include <vector>

#define BYTE char
#include "../utils/log.h"
#include "../utils/CriticalSection.h"

class CHalDevice
{
public:
  CStdString UDI;
  CStdString FriendlyName;
  CHalDevice(const char *udi) { UDI = udi; }
};

class CStorageDevice : public CHalDevice
{
public:
#ifdef HAL_HANDLEMOUNT
  CStorageDevice(const char *udi) : CHalDevice(udi) { HotPlugged = false; Mounted = false; Approved = false; MountedByXBMC = false; }
  bool MountedByXBMC;
#else
  CStorageDevice(const char *udi) : CHalDevice(udi) { HotPlugged = false; Mounted = false; Approved = false; }
#endif
  bool Mounted;
  bool Approved;
  bool HotPlugged;
  CStdString MountPoint;
  CStdString Label;
  CStdString UUID;
  CStdString DevID;
  int  Type;
  CStdString FileSystem;

  CStdString toString()
  { // Not the prettiest but it's better than having to reproduce it elsewere in the code...
    CStdString rtn, tmp1, tmp2, tmp3, tmp4;
    if (UUID.size() > 0)
      tmp1.Format("UUID %s | ", UUID.c_str());
    if (FileSystem.size() > 0)
      tmp2.Format("FileSystem %s | ", FileSystem.c_str());
    if (MountPoint.size() > 0)
      tmp3.Format("Mounted on %s | ", MountPoint.c_str());
    if (HotPlugged)
      tmp4.Format("HotPlugged YES | ");
    else
      tmp4.Format("HotPlugged NO  | ");

    if (Approved)
      rtn.Format("%s%s%s%sApproved YES ", tmp1.c_str(), tmp2.c_str(), tmp3.c_str(), tmp4.c_str());
    else
      rtn.Format("%s%s%s%sApproved NO  ", tmp1.c_str(), tmp2.c_str(), tmp3.c_str(), tmp4.c_str());

    return  rtn;
  }
};


class CHalManager
{
public:
static const char *StorageTypeToString(int DeviceType);
static int StorageTypeFromString(const char *DeviceString);
bool Update();

void Initialize();
CHalManager();
~CHalManager();
std::vector<CStorageDevice> GetVolumeDevices();
std::vector<CHalDevice> GetMTPDevices();

protected:
DBusConnection *m_DBusConnection;
LibHalContext  *m_Context;
static DBusError m_Error;
static bool NewMessage;

void ParseDevice(const char *udi);
bool RemoveDevice(const char *udi);

private:
LibHalContext *InitializeHal();
bool InitializeDBus();
void GenerateGDL();

static bool DeviceFromVolumeUdi(const char *udi, CStorageDevice *device);
static std::vector<CStorageDevice> DeviceFromDriveUdi(const char *udi);
static CCriticalSection m_lock;

//Callbacks HAL
static void DeviceRemoved(LibHalContext *ctx, const char *udi);
static void DeviceNewCapability(LibHalContext *ctx, const char *udi, const char *capability);
static void DeviceLostCapability(LibHalContext *ctx, const char *udi, const char *capability);
static void DevicePropertyModified(LibHalContext *ctx, const char *udi, const char *key, dbus_bool_t is_removed, dbus_bool_t is_added);
static void DeviceCondition(LibHalContext *ctx, const char *udi, const char *condition_name, const char *condition_details);
static void DeviceAdded(LibHalContext *ctx, const char *udi);

//Remembered Devices
std::vector<CStorageDevice> m_Volumes;
};

extern CHalManager g_HalManager;
#endif
#endif // HAS_HAL
