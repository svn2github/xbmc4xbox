#pragma once
#include "IStorageProvider.h"
#include "HALProvider.h"
#include "DeviceKitDisksProvider.h"
#include "PosixMountProvider.h"

class CLinuxStorageProvider : public IStorageProvider
{
public:
  CLinuxStorageProvider()
  {
#ifdef HAS_HAL
    m_instance = new CHALProvider();
#else
#ifdef HAS_DBUS
    if (CDeviceKitDisksProvider::HasDeviceKitDisks())
      m_instance = new CDeviceKitDisksProvider();
    else
#endif
      m_instance = new CPosixMountProvider();
#endif
  }

  virtual ~CLinuxStorageProvider()
  {
    delete m_instance;
  }

  virtual void GetLocalDrives(VECSOURCES &localDrives)
  {
    // Home directory
    CMediaSource share;
    share.strPath = getenv("HOME");
    share.strName = g_localizeStrings.Get(21440);
    share.m_ignore = true;
    share.m_iDriveType = CMediaSource::SOURCE_TYPE_LOCAL;
    localDrives.push_back(share);

    m_instance->GetLocalDrives(localDrives);
  }

  virtual void GetRemovableDrives(VECSOURCES &removableDrives)
  {
    m_instance->GetRemovableDrives(removableDrives);
  }

  virtual std::vector<CStdString> GetDiskUsage()
  {
    return m_instance->GetDiskUsage();
  }

  virtual bool PumpDriveChangeEvents()
  {
    return m_instance->PumpDriveChangeEvents();
  }

private:
  IStorageProvider *m_instance;
};
