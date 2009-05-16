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

#include "include.h"
#include "GUIMultiImage.h"
#include "TextureManager.h"
#include "FileSystem/Directory.h"
#include "Util.h"
#include "FileItem.h"

using namespace std;
using namespace DIRECTORY;

CGUIMultiImage::CGUIMultiImage(DWORD dwParentID, DWORD dwControlId, float posX, float posY, float width, float height, const CTextureInfo& texture, DWORD timePerImage, DWORD fadeTime, bool randomized, bool loop, DWORD timeToPauseAtEnd)
    : CGUIControl(dwParentID, dwControlId, posX, posY, width, height)
{
  m_textureInfo = texture;
  m_currentImage = 0;
  m_timePerImage = timePerImage;
  m_timeToPauseAtEnd = timeToPauseAtEnd;
  m_fadeTime = fadeTime;
  m_randomized = randomized;
  m_loop = loop;
  m_aspect = CAspectRatio::AR_STRETCH;
  ControlType = GUICONTROL_MULTI_IMAGE;
  m_bDynamicResourceAlloc=false;
  m_directoryLoaded = false;
}

CGUIMultiImage::CGUIMultiImage(const CGUIMultiImage &from)
: CGUIControl(from)
{
  m_textureInfo = from.m_textureInfo;
  m_texturePath = from.m_texturePath;
  m_timePerImage = from.m_timePerImage;
  m_timeToPauseAtEnd = from.m_timeToPauseAtEnd;
  m_fadeTime = from.m_fadeTime;
  m_randomized = from.m_randomized;
  m_loop = from.m_loop;
  m_aspect = from.m_aspect;
  m_directoryLoaded = false;
  if (m_texturePath.IsConstant())
    m_currentPath = m_texturePath.GetLabel(WINDOW_INVALID);
  m_currentImage = 0;
  ControlType = GUICONTROL_MULTI_IMAGE;
}

CGUIMultiImage::~CGUIMultiImage(void)
{
}

void CGUIMultiImage::UpdateVisibility(const CGUIListItem *item)
{
  CGUIControl::UpdateVisibility(item);

  // check if we're hidden, and deallocate if so
  if (!IsVisible() && m_visible != DELAYED)
  {
    if (m_bDynamicResourceAlloc && m_bAllocated)
      FreeResources();
    return;
  }

  // we are either delayed or visible, so we can allocate our resources

  // check for conditional information before we
  // alloc as this can free our resources
  if (!m_texturePath.IsConstant())
  {
    CStdString texturePath(m_texturePath.GetLabel(m_dwParentID));
    if (texturePath != m_currentPath && !texturePath.IsEmpty())
    {
      m_currentPath = texturePath;
      FreeResources();
      LoadDirectory();
    }
  }

  // and allocate our resources
  if (!m_bAllocated)
    AllocResources();
}

void CGUIMultiImage::Render()
{
  if (!m_images.empty())
  {
    // Set a viewport so that we don't render outside the defined area
    g_graphicsContext.SetClipRegion(m_posX, m_posY, m_width, m_height);

    unsigned int nextImage = m_currentImage + 1;
    if (nextImage >= m_images.size())
      nextImage = m_loop ? 0 : m_currentImage;  // stay on the last image if <loop>no</loop>

    bool renderNext = false;
    if (nextImage != m_currentImage)
    {
      // check if we should be loading a new image yet
      DWORD timeToShow = m_timePerImage;
      if (0 == nextImage) // last image should be paused for a bit longer if that's what the skinner wishes.
        timeToShow += m_timeToPauseAtEnd;
      if (m_imageTimer.IsRunning() && m_imageTimer.GetElapsedMilliseconds() > timeToShow)
      {
        m_imageTimer.Stop();
        // grab a new image
        LoadImage(nextImage);
        // start the fade timer
        m_fadeTimer.StartZero();
      }

      // check if we are still fading
      if (m_fadeTimer.IsRunning())
      {
        // check if the fade timer has run out
        float timeFading = m_fadeTimer.GetElapsedMilliseconds();
        if (timeFading >= m_fadeTime)
        {
          m_fadeTimer.Stop();
          // swap images
          m_images[m_currentImage]->FreeResources();
          m_images[nextImage]->SetAlpha(255);
          m_currentImage = nextImage;
          // start the load timer
          m_imageTimer.StartZero();
        }
        else
        { // perform the fade in of next image
          float fadeAmount = timeFading / m_fadeTime;
          float alpha = (float)(m_diffuseColor >> 24) / 255.0f;
          if (alpha < 1 && alpha > 0)
          { // we have a semi-transparent image, so we need to use a more complicated
            // fade technique.  Assuming a black background (not generally true, but still...)
            // we have
            // b(t) = [a - b(1-t)*a] / a*(1-b(1-t)*a),
            // where a = alpha, and b(t):[0,1] -> [0,1] is the blend function.
            // solving, we get
            // b(t) = [1 - (1-a)^t] / a
            float blendIn = (1 - pow(1-alpha, fadeAmount)) / alpha;
            m_images[nextImage]->SetAlpha((unsigned char)(255 * blendIn));
            float blendOut = (1 - blendIn) / (1 - blendIn*alpha); // no need to use pow() again here
            m_images[m_currentImage]->SetAlpha((unsigned char)(255 * blendOut));
          }
          else
          { // simple case, just fade in the second image
            m_images[m_currentImage]->SetAlpha(255);
            m_images[nextImage]->SetAlpha((unsigned char)(255*fadeAmount));
          }
        }
        renderNext = true;
      }
    }
    m_images[m_currentImage]->SetDiffuseColor(m_diffuseColor);
    m_images[m_currentImage]->Render();
    if (renderNext)
    {
      m_images[nextImage]->SetDiffuseColor(m_diffuseColor);
      m_images[nextImage]->Render();
    }
    g_graphicsContext.RestoreClipRegion();
  }
  CGUIControl::Render();
}

bool CGUIMultiImage::OnAction(const CAction &action)
{
  return false;
}

bool CGUIMultiImage::OnMessage(CGUIMessage &message)
{
  if (message.GetMessage() == GUI_MSG_REFRESH_THUMBS)
  {
    if (!m_texturePath.IsConstant())
      FreeResources();
    return true;
  }
  return CGUIControl::OnMessage(message);
}

void CGUIMultiImage::PreAllocResources()
{
  FreeResources();
}

void CGUIMultiImage::AllocResources()
{
  FreeResources();
  CGUIControl::AllocResources();

  if (!m_directoryLoaded)
    LoadDirectory();

  // Randomize or sort our images if necessary
  if (m_randomized)
    random_shuffle(m_files.begin(), m_files.end());

  for (unsigned int i=0; i < m_files.size(); i++)
  {
    CTextureInfo info(m_textureInfo);
    info.filename = m_files[i];
    CGUITexture *pImage = new CGUITexture(m_posX, m_posY, m_width, m_height, info);
    if (pImage)
      m_images.push_back(pImage);
  }
  // Load in the current image, and reset our timer
  m_imageTimer.StartZero();
  m_fadeTimer.Stop();
  m_currentImage = 0;
  if (m_images.empty())
    return;

  LoadImage(m_currentImage);
}

void CGUIMultiImage::LoadImage(int image)
{
  if (image < 0 || image >= (int)m_images.size())
    return;

  m_images[image]->AllocResources();

  // Scale image so that it will fill our render area
  if (m_aspect.ratio != CAspectRatio::AR_STRETCH)
  {
    // to get the pixel ratio, we must use the SCALED output sizes
    float pixelRatio = g_graphicsContext.GetScalingPixelRatio();

    float sourceAspectRatio = m_images[image]->GetTextureWidth() / m_images[image]->GetTextureHeight();
    float aspectRatio = sourceAspectRatio / pixelRatio;

    float newWidth = m_width;
    float newHeight = newWidth / aspectRatio;
    if ((m_aspect.ratio == CAspectRatio::AR_SCALE && newHeight < m_height) ||
      (m_aspect.ratio == CAspectRatio::AR_KEEP && newHeight > m_height))
    {
      newHeight = m_height;
      newWidth = newHeight * aspectRatio;
    }
    m_images[image]->SetPosition(m_posX - (newWidth - m_width)*0.5f, m_posY - (newHeight - m_height)*0.5f);
    m_images[image]->SetWidth(newWidth);
    m_images[image]->SetHeight(newHeight);
  }
}

void CGUIMultiImage::FreeResources()
{
  for (unsigned int i = 0; i < m_images.size(); ++i)
  {
    m_images[i]->FreeResources();
    delete m_images[i];
  }

  m_images.clear();
  m_currentImage = 0;
  CGUIControl::FreeResources();
}

void CGUIMultiImage::DynamicResourceAlloc(bool bOnOff)
{
  CGUIControl::DynamicResourceAlloc(bOnOff);
  m_bDynamicResourceAlloc=bOnOff;
}

bool CGUIMultiImage::CanFocus() const
{
  return false;
}

void CGUIMultiImage::SetAspectRatio(const CAspectRatio &ratio)
{
  if (m_aspect != ratio)
  {
    m_aspect = ratio;
    m_bInvalidated = true;
  }
}

void CGUIMultiImage::LoadDirectory()
{
  // Load any images from our texture bundle first
  m_files.clear();

  // don't load any images if our path is empty
  if (m_currentPath.IsEmpty()) return;

  // check to see if we have a single image or a folder of images
  CFileItem item(m_currentPath, false);
  if (item.IsPicture())
  {
    m_files.push_back(g_TextureManager.GetTexturePath(m_currentPath));
  }
  else
  { // folder of images
    g_TextureManager.GetBundledTexturesFromPath(m_currentPath, m_files);

    // Load in our images from the directory specified
    // m_currentPath is relative (as are all skin paths)
    CStdString realPath = g_TextureManager.GetTexturePath(m_currentPath, true);
    if (realPath.IsEmpty())
      return;

    CUtil::AddSlashAtEnd(realPath);
    CFileItemList items;
    CDirectory::GetDirectory(realPath, items);
    for (int i=0; i < items.Size(); i++)
    {
      CFileItemPtr pItem = items[i];
      if (pItem->IsPicture())
        m_files.push_back(pItem->m_strPath);
    }
  }

  // sort our images - they'll be randomized in AllocResources() if necessary
  sort(m_files.begin(), m_files.end());

  // flag as loaded - no point in constantly reloading them
  m_directoryLoaded = true;
}

void CGUIMultiImage::SetInfo(const CGUIInfoLabel &info)
{
  m_texturePath = info;
  if (m_texturePath.IsConstant())
    m_currentPath = m_texturePath.GetLabel(WINDOW_INVALID);
}