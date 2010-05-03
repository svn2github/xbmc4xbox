/*
 *      Copyright (C) 2005-2009 Team XBMC
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
#include "ConsoleUPowerSyscall.h"
#include "utils/log.h"

#ifdef HAS_DBUS
#include "Application.h"
#include "LocalizeStrings.h"
#include "DBusUtil.h"

CConsoleUPowerSyscall::CConsoleUPowerSyscall()
{
  m_CanPowerdown = ConsoleKitMethodCall("CanStop");

  // If "the name org.freedesktop.UPower was not provided by any .service files",
  // GetVariant() would return NULL, and asBoolean() would crash.
  CVariant canSuspend = CDBusUtil::GetVariant("org.freedesktop.UPower", "/org/freedesktop/UPower",    "org.freedesktop.UPower", "can_suspend");

  if ( !canSuspend.isNull() )
    m_CanSuspend = canSuspend.asBoolean();
  else
    m_CanSuspend = false;

  CVariant canHibernate = CDBusUtil::GetVariant("org.freedesktop.UPower", "/org/freedesktop/UPower",    "org.freedesktop.UPower", "can_hibernate");

  if ( !canHibernate.isNull() )
    m_CanHibernate = canHibernate.asBoolean();
  else
    m_CanHibernate = false;

  m_CanReboot    = ConsoleKitMethodCall("CanRestart");
}

bool CConsoleUPowerSyscall::Powerdown()
{
  CDBusMessage message("org.freedesktop.ConsoleKit", "/org/freedesktop/ConsoleKit/Manager", "org.freedesktop.ConsoleKit.Manager", "Stop");
  return message.SendSystem() != NULL;
}

bool CConsoleUPowerSyscall::Suspend()
{
  CPowerSyscallWithoutEvents::Suspend();

  CDBusMessage message("org.freedesktop.UPower", "/org/freedesktop/UPower", "org.freedesktop.UPower", "Suspend");
  return message.SendSystem() != NULL;
}

bool CConsoleUPowerSyscall::Hibernate()
{
  CPowerSyscallWithoutEvents::Hibernate();

  CDBusMessage message("org.freedesktop.UPower", "/org/freedesktop/UPower", "org.freedesktop.UPower", "Hibernate");
  return message.SendSystem() != NULL;
}

bool CConsoleUPowerSyscall::Reboot()
{
  CDBusMessage message("org.freedesktop.ConsoleKit", "/org/freedesktop/ConsoleKit/Manager", "org.freedesktop.ConsoleKit.Manager", "Restart");
  return message.SendSystem() != NULL;
}

bool CConsoleUPowerSyscall::CanPowerdown()
{
  return m_CanPowerdown;
}
bool CConsoleUPowerSyscall::CanSuspend()
{
  return m_CanSuspend;
}
bool CConsoleUPowerSyscall::CanHibernate()
{
  return m_CanHibernate;
}
bool CConsoleUPowerSyscall::CanReboot()
{
  return m_CanReboot;
}

bool CConsoleUPowerSyscall::HasDeviceConsoleKit()
{
  bool hasConsoleKitManager = false;
  CDBusMessage consoleKitMessage("org.freedesktop.ConsoleKit", "/org/freedesktop/ConsoleKit/Manager", "org.freedesktop.ConsoleKit.Manager", "CanStop");

  DBusError error;
  dbus_error_init (&error);
  DBusConnection *con = dbus_bus_get(DBUS_BUS_SYSTEM, &error);

  consoleKitMessage.Send(con, &error);

  if (!dbus_error_is_set(&error))
    hasConsoleKitManager = true;
  else
    CLog::Log(LOGDEBUG, "ConsoleKit.Manager: %s - %s", error.name, error.message);

  dbus_error_free (&error);

  bool hasUPower = false;
  CDBusMessage deviceKitMessage("org.freedesktop.UDisks", "/org/freedesktop/UDisks", "org.freedesktop.UDisks", "EnumerateDevices");

  deviceKitMessage.Send(con, &error);

  if (!dbus_error_is_set(&error))
    hasUPower = true;
  else
    CLog::Log(LOGDEBUG, "UPower: %s - %s", error.name, error.message);

  dbus_error_free (&error);
  dbus_connection_unref(con);

  return hasUPower && hasConsoleKitManager;
}

bool CConsoleUPowerSyscall::ConsoleKitMethodCall(const char *method)
{
  CDBusMessage message("org.freedesktop.ConsoleKit", "/org/freedesktop/ConsoleKit/Manager", "org.freedesktop.ConsoleKit.Manager", method);
  DBusMessage *reply = message.SendSystem();
  if (reply)
  {
    dbus_bool_t boolean = FALSE;

    if (dbus_message_get_args (reply, NULL, DBUS_TYPE_BOOLEAN, &boolean, DBUS_TYPE_INVALID))
      return boolean;
  }

  return false;
}
#endif
