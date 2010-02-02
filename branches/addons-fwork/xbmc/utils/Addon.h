#pragma once
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

#include "IAddon.h"
#include "tinyXML/tinyxml.h"
#include "Util.h"
#include "URL.h"
#include "LocalizeStrings.h"
#include <ostream>

class CURL;
class TiXmlElement;

namespace ADDON
{

// utils
const CStdString    TranslateContent(const CONTENT_TYPE &content, bool pretty=false);
const CONTENT_TYPE  TranslateContent(const CStdString &string);
const CStdString    TranslateType(const TYPE &type, bool pretty=false);
const TYPE          TranslateType(const CStdString &string);

struct AddonVersion
{
public:
  AddonVersion(CStdString &version) : str(version) {}
  bool operator==(const AddonVersion &rhs) const;
  bool operator!=(const AddonVersion &rhs) const;
  bool operator>(const AddonVersion &rhs) const;
  bool operator>=(const AddonVersion &rhs) const;
  bool operator<(const AddonVersion &rhs) const;
  bool operator<=(const AddonVersion &rhs) const;
  std::ostream& operator<<(std::ostream& out) const;
  const CStdString str;
};

struct AddonProps
{
public:
  AddonProps(CStdString &uuid, TYPE type, CStdString &versionstr)
    : uuid(uuid)
    , type(type)
    , version(versionstr)
  {}

  AddonProps(const AddonPtr &addon)
    : uuid(addon->UUID())
    , type(addon->Type())
    , version(addon->Version())
  { if(addon->Parent()) parent = addon->Parent()->UUID(); }

  bool operator=(const AddonProps &rhs)
  { return (*this).uuid == rhs.uuid
    && (*this).type == rhs.type
    && (*this).version == rhs.version; }
  const CStdString uuid;
  const TYPE type;
  const AddonVersion version;
  CStdString name;
  CStdString parent;
  CStdString license;
  CStdString summary;
  CStdString description;
  CStdString path;
  CStdString libname;
  CStdString author;
  CStdString source;
  CStdString icon;
  CStdString disclaimer;
  std::set<CONTENT_TYPE> contents;
  int        stars;
};
typedef std::vector<struct AddonProps> VECADDONPROPS;

class CAddon : public IAddon
{
public:
  CAddon(const AddonProps &addonprops);
  virtual ~CAddon() {}
  virtual AddonPtr Clone(const AddonPtr& parent) const;

  // settings & language
  virtual bool HasSettings();
  virtual bool LoadSettings();
  virtual void SaveSettings();
  virtual void SaveFromDefault();
  virtual void UpdateSetting(const CStdString& key, const CStdString& value, const CStdString &type = "");
  virtual CStdString GetSetting(const CStdString& key) const;
  TiXmlElement* GetSettingsXML();
  virtual CStdString GetString(uint32_t id) const;

  const TYPE Type() const { return m_props.type; }
  AddonProps Props() const { return m_props; }
  const CStdString UUID() const { return m_props.uuid; }
  const AddonPtr Parent() const { return m_parent; }
  const CStdString Name() const { return m_props.name; }
  bool Disabled() const { return m_disabled; }
  const AddonVersion Version();
  const CStdString Summary() const { return m_props.summary; }
  const CStdString Description() const { return m_props.description; }
  const CStdString Path() const { return m_props.path; }
  const CStdString Profile() const { return m_strProfile; }
  const CStdString LibName() const { return m_props.libname; }
  const CStdString Author() const { return m_props.author; }
  const CStdString Icon() const { return m_props.icon; }
  const int Stars() const { return m_props.stars; }
  const CStdString Disclaimer() const { return m_props.disclaimer; }
  bool Supports(const CONTENT_TYPE &content) const { return (m_props.contents.count(content) == 1); }

protected:
  CAddon(const CAddon&); // protected as all copying is handled by Clone()
  CAddon(const CAddon&, const AddonPtr&);
  bool LoadUserSettings();
  TiXmlDocument     m_addonXmlDoc;
  TiXmlDocument     m_userXmlDoc;
  CStdString        m_userSettingsPath;

private:
  friend class AddonMgr;
  AddonProps m_props;
  const AddonPtr    m_parent;
  CStdString GetProfilePath();
  CStdString GetUserSettingsPath();

  void Enable() { LoadStrings(); m_disabled = false; }
  void Disable() { m_disabled = true; ClearStrings();}

  virtual bool LoadStrings();
  virtual void ClearStrings();

  void SetDeps(ADDONDEPS& deps) { m_dependencies = deps; }
  ADDONDEPS GetDeps() { return m_dependencies; }
  ADDONDEPS m_dependencies;

  void BuildLibName();
  CStdString  m_strProfile;
  CStdString  m_strLibName;
  bool        m_disabled;
  CLocalizeStrings  m_strings;
};

}; /* namespace ADDON */

