#include "include.h"
#include "GUIListItem.h"
#include "GUIImage.h"
#include "GUIListContainer.h"

CGUIListItem::CGUIListItem(const CGUIListItem& item)
{
  *this = item;
  m_invalidated = true;
}

CGUIListItem::CGUIListItem(void)
{
  m_bIsFolder = false;
  m_strLabel2 = "";
  m_strLabel = "";
  m_pThumbnailImage = NULL;
  m_pIconImage = NULL;
  m_overlayImage = NULL;
  m_bSelected = false;
  m_strIcon = "";
  m_strThumbnailImage = "";
  m_overlayIcon = ICON_OVERLAY_NONE;
  m_layout = NULL;
  m_focusedLayout = NULL;
  m_invalidated = true;
}

CGUIListItem::CGUIListItem(const CStdString& strLabel)
{
  m_bIsFolder = false;
  m_strLabel2 = "";
  m_strLabel = strLabel;
  m_pThumbnailImage = NULL;
  m_pIconImage = NULL;
  m_overlayImage = NULL;
  m_bSelected = false;
  m_strIcon = "";
  m_strThumbnailImage = "";
  m_overlayIcon = ICON_OVERLAY_NONE;
  m_layout = NULL;
  m_focusedLayout = NULL;
  m_invalidated = true;
}

CGUIListItem::~CGUIListItem(void)
{
  FreeMemory();
}

void CGUIListItem::SetLabel(const CStdString& strLabel)
{
  m_strLabel = strLabel;
  m_invalidated = true;
}

const CStdString& CGUIListItem::GetLabel() const
{
  return m_strLabel;
}


void CGUIListItem::SetLabel2(const CStdString& strLabel2)
{
  m_strLabel2 = strLabel2;
  m_invalidated = true;
}

const CStdString& CGUIListItem::GetLabel2() const
{
  return m_strLabel2;
}

void CGUIListItem::SetThumbnailImage(const CStdString& strThumbnail)
{
  m_strThumbnailImage = strThumbnail;
  m_invalidated = true;
}

const CStdString& CGUIListItem::GetThumbnailImage() const
{
  return m_strThumbnailImage;
}

void CGUIListItem::SetIconImage(const CStdString& strIcon)
{
  m_strIcon = strIcon;
  m_invalidated = true;
}

const CStdString& CGUIListItem::GetIconImage() const
{
  return m_strIcon;
}

void CGUIListItem::SetOverlayImage(GUIIconOverlay icon, bool bOnOff)
{
  if (bOnOff)
    m_overlayIcon = GUIIconOverlay((int)(icon)+1);
  else
    m_overlayIcon = icon;
  m_invalidated = true;
}

CStdString CGUIListItem::GetOverlayImage() const
{
  switch (m_overlayIcon)
  {
  case ICON_OVERLAY_RAR:
    return "OverlayRAR.png";
  case ICON_OVERLAY_ZIP:
    return "OverlayZIP.png";
  case ICON_OVERLAY_TRAINED:
    return "OverlayTrained.png";
  case ICON_OVERLAY_HAS_TRAINER:
    return "OverlayHasTrainer.png";
  case ICON_OVERLAY_LOCKED:
    return "OverlayLocked.png";
  case ICON_OVERLAY_UNWATCHED:
    return "OverlayUnwatched.png";
  case ICON_OVERLAY_WATCHED:
    return "OverlayWatched.png";
  }
  return "";
}

void CGUIListItem::Select(bool bOnOff)
{
  m_bSelected = bOnOff;
}

bool CGUIListItem::HasIcon() const
{
  return (m_strIcon.size() != 0);
}


bool CGUIListItem::HasThumbnail() const
{
  return (m_strThumbnailImage.size() != 0);
}

bool CGUIListItem::HasOverlay() const
{
  return (m_overlayIcon != CGUIListItem::ICON_OVERLAY_NONE);
}

bool CGUIListItem::IsSelected() const
{
  return m_bSelected;
}

CGUIImage* CGUIListItem::GetThumbnail()
{
  return m_pThumbnailImage;
}

CGUIImage* CGUIListItem::GetIcon()
{
  return m_pIconImage;
}

CGUIImage* CGUIListItem::GetOverlay()
{
  return m_overlayImage;
}

void CGUIListItem::SetThumbnail(CGUIImage* pImage)
{
  if (m_pThumbnailImage)
  {
    m_pThumbnailImage->FreeResources();
    delete m_pThumbnailImage;
  }
  m_pThumbnailImage = pImage;
}

void CGUIListItem::SetIcon(CGUIImage* pImage)
{
  if (m_pIconImage)
  {
    m_pIconImage->FreeResources();
    delete m_pIconImage;
  }
  m_pIconImage = pImage;
}

void CGUIListItem::SetOverlay(CGUIImage *pImage)
{
  if (m_overlayImage)
  {
    m_overlayImage->FreeResources();
    delete m_overlayImage;
  }
  m_overlayImage = pImage;
}

const CGUIListItem& CGUIListItem::operator =(const CGUIListItem& item)
{
  if (&item == this) return * this;
  m_strLabel2 = item.m_strLabel2;
  m_strLabel = item.m_strLabel;
  m_pThumbnailImage = NULL;
  m_pIconImage = NULL;
  m_overlayImage = NULL;
  m_bSelected = item.m_bSelected;
  m_strIcon = item.m_strIcon;
  m_strThumbnailImage = item.m_strThumbnailImage;
  m_overlayIcon = item.m_overlayIcon;
  m_bIsFolder = item.m_bIsFolder;
  m_invalidated = true;
  return *this;
}

void CGUIListItem::FreeIcons()
{
  FreeMemory();
  m_strThumbnailImage = "";
  m_strIcon = "";
  m_invalidated = true;
}

void CGUIListItem::FreeMemory()
{
  if (m_pThumbnailImage)
  {
    m_pThumbnailImage->FreeResources();
    delete m_pThumbnailImage;
    m_pThumbnailImage = NULL;
  }
  if (m_pIconImage)
  {
    m_pIconImage->FreeResources();
    delete m_pIconImage;
    m_pIconImage = NULL;
  }
  if (m_overlayImage)
  {
    m_overlayImage->FreeResources();
    delete m_overlayImage;
    m_overlayImage = NULL;
  }
  if (m_layout)
  {
    delete m_layout;
    m_layout = NULL;
  }
  if (m_focusedLayout)
  {
    delete m_focusedLayout;
    m_focusedLayout = NULL;
  }
}

void CGUIListItem::SetLayout(CGUIListItemLayout *layout)
{
  if (m_layout)
    delete m_layout;
  m_layout = layout;
}

CGUIListItemLayout *CGUIListItem::GetLayout()
{
  return m_layout;
}

void CGUIListItem::SetFocusedLayout(CGUIListItemLayout *layout)
{
  if (m_focusedLayout)
    delete m_focusedLayout;
  m_focusedLayout = layout;
}

CGUIListItemLayout *CGUIListItem::GetFocusedLayout()
{
  return m_focusedLayout;
}