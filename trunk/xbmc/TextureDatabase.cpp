/*
 *      Copyright (C) 2005-2010 Team XBMC
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

#include "TextureDatabase.h"
#include "utils/log.h"
#include "Crc32.h"

CTextureDatabase::CTextureDatabase()
{
}

CTextureDatabase::~CTextureDatabase()
{
}

bool CTextureDatabase::Open()
{
  return CDatabase::Open();
}

bool CTextureDatabase::CreateTables()
{
  try
  {
    CDatabase::CreateTables();

    CLog::Log(LOGINFO, "create texture table");
    m_pDS->exec("CREATE TABLE texture (id integer primary key, urlhash integer, url text, cachedurl text, ddsurl text, usecount integer, lastusetime text, imagehash text)\n");

    CLog::Log(LOGINFO, "create textures index");
    m_pDS->exec("CREATE INDEX idxTexture ON texture(urlhash)");
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "%s unable to create tables", __FUNCTION__);
    return false;
  }

  return true;
}

bool CTextureDatabase::UpdateOldVersion(int version)
{
  return true;
}

bool CTextureDatabase::GetCachedTexture(const CStdString &url, CStdString &cacheFile, CStdString &ddsFile)
{
  try
  {
    if (NULL == m_pDB.get()) return false;
    if (NULL == m_pDS.get()) return false;

    unsigned int hash = GetURLHash(url);

    CStdString sql = FormatSQL("select id, cachedurl, ddsurl from texture where urlhash=%u", hash);
    m_pDS->query(sql.c_str());

    if (!m_pDS->eof())
    { // have some information
      int textureID = m_pDS->fv(0).get_asInt();
      cacheFile = m_pDS->fv(1).get_asString();
      ddsFile = m_pDS->fv(2).get_asString();
      m_pDS->close();
      // update the use count
      sql = FormatSQL("update texture set usecount=usecount+1, lastusetime=CURRENT_TIMESTAMP where id=%u", textureID);
      m_pDS->exec(sql.c_str());
      return true;
    }
    m_pDS->close();
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "%s, failed on url '%s'", __FUNCTION__, url.c_str());
  }
  return false;
}

bool CTextureDatabase::AddCachedTexture(const CStdString &url, const CStdString &cacheFile, const CStdString &ddsFile, const CStdString &imageHash)
{
  try
  {
    if (NULL == m_pDB.get()) return false;
    if (NULL == m_pDS.get()) return false;

    unsigned int hash = GetURLHash(url);

    CStdString sql = FormatSQL("select id from texture where urlhash=%u", hash);
    m_pDS->query(sql.c_str());
    if (!m_pDS->eof())
    { // update
      int textureID = m_pDS->fv(0).get_asInt();
      m_pDS->close();
      if (!imageHash.IsEmpty())
        sql = FormatSQL("update texture set cachedurl='%s', ddsurl='%s', usecount=1, lastusetime=CURRENT_TIMESTAMP, imagehash='%s' where id=%u", cacheFile.c_str(), ddsFile.c_str(), imageHash.c_str(), textureID);
      else
        sql = FormatSQL("update texture set cachedurl='%s', ddsurl='%s', usecount=1, lastusetime=CURRENT_TIMESTAMP where id=%u", cacheFile.c_str(), ddsFile.c_str(), textureID);        
      m_pDS->exec(sql.c_str());
    }
    else
    { // add the texture
      m_pDS->close();
      sql = FormatSQL("insert into texture (id, urlhash, url, cachedurl, ddsurl, usecount, lastusetime, imagehash) values(NULL, %u, '%s', '%s', '%s', 1, CURRENT_TIMESTAMP, '%s')", hash, url.c_str(), cacheFile.c_str(), ddsFile.c_str(), imageHash.c_str());
      m_pDS->exec(sql.c_str());
    }
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "%s failed on url '%s'", __FUNCTION__, url.c_str());
  }
  return true;
}

bool CTextureDatabase::ClearCachedTexture(const CStdString &url, CStdString &cacheFile, CStdString &ddsFile)
{
  try
  {
    if (NULL == m_pDB.get()) return false;
    if (NULL == m_pDS.get()) return false;

    unsigned int hash = GetURLHash(url);

    CStdString sql = FormatSQL("select id, cachedurl, ddsurl from texture where urlhash=%u", hash);
    m_pDS->query(sql.c_str());

    if (!m_pDS->eof())
    { // have some information
      int textureID = m_pDS->fv(0).get_asInt();
      cacheFile = m_pDS->fv(1).get_asString();
      ddsFile = m_pDS->fv(2).get_asString();
      m_pDS->close();
      // remove it
      sql = FormatSQL("delete from texture where id=%u", textureID);
      m_pDS->exec(sql.c_str());
      return true;
    }
    m_pDS->close();
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "%s, failed on url '%s'", __FUNCTION__, url.c_str());
  }
  return false;
}

unsigned int CTextureDatabase::GetURLHash(const CStdString &url) const
{
  Crc32 crc;
  crc.ComputeFromLowerCase(url);
  return (unsigned int)crc;
}
