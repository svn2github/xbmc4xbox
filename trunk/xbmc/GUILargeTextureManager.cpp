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

#include "GUILargeTextureManager.h"
#include "Picture.h"
#include "GUISettings.h"
#include "FileItem.h"
#include "Texture.h"
#include "utils/SingleLock.h"
#include "utils/TimeUtils.h"
#include "utils/JobManager.h"
#include "GraphicContext.h"
#include "Settings.h"
#include "AdvancedSettings.h"
#include "Util.h"

using namespace std;

CGUILargeTextureManager g_largeTextureManager;


CImageLoader::CImageLoader(const CStdString &path)
{
  m_path = path;
  m_texture = NULL;
}

CImageLoader::~CImageLoader()
{
  delete(m_texture);
}

bool CImageLoader::DoWork()
{
  CFileItem file(m_path, false);
  if (file.IsPicture() && !(file.IsZIP() || file.IsRAR() || file.IsCBR() || file.IsCBZ())) // ignore non-pictures
  { // check for filename only (i.e. lookup in skin/media/)
    CStdString loadPath(m_path);
    if ((size_t)m_path.FindOneOf("/\\") == CStdString::npos)
    {
      loadPath = g_TextureManager.GetTexturePath(m_path);
    }
    // check if this is a fanart image
    if (g_advancedSettings.m_useDDSFanart && file.IsType(".tbn"))
    {
      CStdString baseFolder1 = g_settings.GetMusicFanartFolder();
      CStdString baseFolder2 = g_settings.GetVideoFanartFolder();
      if (baseFolder1.Equals(m_path.Left(baseFolder1.GetLength())) ||
          baseFolder2.Equals(m_path.Left(baseFolder2.GetLength())))
      { // switch to dds
        CUtil::ReplaceExtension(m_path, ".dds", loadPath);
      }
    }
    m_texture = new CTexture();
    if (!m_texture->LoadFromFile(loadPath, min(g_graphicsContext.GetWidth(), 2048), min(g_graphicsContext.GetHeight(), 1080), g_guiSettings.GetBool("pictures.useexifrotation")))
    {
      delete m_texture;
      m_texture = NULL;
    }
  }

  return true;
}

CGUILargeTextureManager::CLargeTexture::CLargeTexture(const CStdString &path)
{
  m_path = path;
  m_refCount = 1;
  m_timeToDelete = 0;
};

CGUILargeTextureManager::CLargeTexture::~CLargeTexture()
{
  assert(m_refCount == 0);
  m_texture.Free();
}

void CGUILargeTextureManager::CLargeTexture::AddRef()
{
  m_refCount++;
}

bool CGUILargeTextureManager::CLargeTexture::DecrRef(bool deleteImmediately)
{
  assert(m_refCount);
  m_refCount--;
  if (m_refCount == 0)
  {
    if (deleteImmediately)
      delete this;
    else
      m_timeToDelete = CTimeUtils::GetFrameTime() + TIME_TO_DELETE;
    return true;
  }
  return false;
}

bool CGUILargeTextureManager::CLargeTexture::DeleteIfRequired()
{
  if (m_refCount == 0 && m_timeToDelete < CTimeUtils::GetFrameTime())
  {
    delete this;
    return true;
  }
  return false;
}

void CGUILargeTextureManager::CLargeTexture::SetTexture(CBaseTexture* texture)
{
  assert(!m_texture.size());
  if (texture)
    m_texture.Set(texture, texture->GetWidth(), texture->GetHeight());
}

CGUILargeTextureManager::CGUILargeTextureManager()
{
}

CGUILargeTextureManager::~CGUILargeTextureManager()
{
}

void CGUILargeTextureManager::CleanupUnusedImages()
{
  CSingleLock lock(m_listSection);
  // check for items to remove from allocated list, and remove
  listIterator it = m_allocated.begin();
  while (it != m_allocated.end())
  {
    CLargeTexture *image = *it;
    if (image->DeleteIfRequired())
      it = m_allocated.erase(it);
    else
      ++it;
  }
}

// if available, increment reference count, and return the image.
// else, add to the queue list if appropriate.
bool CGUILargeTextureManager::GetImage(const CStdString &path, CTextureArray &texture, bool firstRequest)
{
  // note: max size to load images: 2048x1024? (8MB)
  CSingleLock lock(m_listSection);
  for (listIterator it = m_allocated.begin(); it != m_allocated.end(); ++it)
  {
    CLargeTexture *image = *it;
    if (image->GetPath() == path)
    {
      if (firstRequest)
        image->AddRef();
      texture = image->GetTexture();
      return texture.size() > 0;
    }
  }

  if (firstRequest)
    QueueImage(path);

  return true;
}

void CGUILargeTextureManager::ReleaseImage(const CStdString &path, bool immediately)
{
  CSingleLock lock(m_listSection);
  for (listIterator it = m_allocated.begin(); it != m_allocated.end(); ++it)
  {
    CLargeTexture *image = *it;
    if (image->GetPath() == path)
    {
      if (image->DecrRef(immediately) && immediately)
        m_allocated.erase(it);
      return;
    }
  }
  for (queueIterator it = m_queued.begin(); it != m_queued.end(); ++it)
  {
    unsigned int id = it->first;
    CLargeTexture *image = it->second;
    if (image->GetPath() == path && image->DecrRef(true))
    {
      // cancel this job
      CJobManager::GetInstance().CancelJob(id);
      m_queued.erase(it);
      return;
    }
  }
}

// queue the image, and start the background loader if necessary
void CGUILargeTextureManager::QueueImage(const CStdString &path)
{
  CSingleLock lock(m_listSection);
  for (queueIterator it = m_queued.begin(); it != m_queued.end(); ++it)
  {
    CLargeTexture *image = it->second;
    if (image->GetPath() == path)
    {
      image->AddRef();
      return; // already queued
    }
  }

  // queue the item
  CLargeTexture *image = new CLargeTexture(path);
  unsigned int jobID = CJobManager::GetInstance().AddJob(new CImageLoader(path), this);
  m_queued.push_back(make_pair(jobID, image));
}

void CGUILargeTextureManager::OnJobComplete(unsigned int jobID, bool success, CJob *job)
{
  // see if we still have this job id
  CSingleLock lock(m_listSection);
  for (queueIterator it = m_queued.begin(); it != m_queued.end(); ++it)
  {
    if (it->first == jobID)
    { // found our job
      CImageLoader *loader = (CImageLoader *)job;
      CLargeTexture *image = it->second;
      image->SetTexture(loader->m_texture);
      loader->m_texture = NULL; // we want to keep the texture, and jobs are auto-deleted.
      m_queued.erase(it);
      m_allocated.push_back(image);
      return;
    }
  }
}



