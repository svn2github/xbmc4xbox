#include "guilistitem.h"

CGUIListItem::CGUIListItem(void)
{
  m_strLabel2="";
  m_strLabel="";
  m_pThumbnailImage=NULL;
  m_pIconImage=NULL;
  m_bSelected=false;
  m_strIcon="";
  m_strThumbnailImage="";
}

CGUIListItem::CGUIListItem(const CStdString& strLabel)
{
  m_strLabel2="";
  m_strLabel=strLabel;
  m_pThumbnailImage=NULL;
  m_pIconImage=NULL;
  m_bSelected=false;
  m_strIcon="";
  m_strThumbnailImage="";
}

CGUIListItem::~CGUIListItem(void)
{
  if (m_pThumbnailImage) 
  {
    m_pThumbnailImage->FreeResources();
    delete m_pThumbnailImage;
    m_pThumbnailImage=NULL;
  }
}

void CGUIListItem::SetLabel(const CStdString& strLabel) 
{
  m_strLabel = strLabel;
}

const CStdString& CGUIListItem::GetLabel() const
{
  return m_strLabel;
}


void CGUIListItem::SetLabel2(const CStdString& strLabel2) 
{
  m_strLabel2 = strLabel2;
}

const CStdString& CGUIListItem::GetLabel2() const
{
  return m_strLabel2;
}

void CGUIListItem::SetThumbnailImage(const CStdString& strThumbnail)
{
	m_strThumbnailImage=strThumbnail;
}

const CStdString&	CGUIListItem::GetThumbnailImage() const
{
	return m_strThumbnailImage;
}

void CGUIListItem::SetIconImage(const CStdString& strIcon)
{
	m_strIcon=strIcon;
}

const CStdString& CGUIListItem::GetIconImage() const
{
	return m_strIcon;
}


void CGUIListItem::Select(bool bOnOff)
{
	m_bSelected=bOnOff;
}

bool CGUIListItem::HasIcon() const
{
	return (m_strIcon.size() != 0);
}


bool CGUIListItem::HasThumbnail() const
{
	return (m_strThumbnailImage.size() != 0);
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


void CGUIListItem::SetThumbnail(CGUIImage* pImage)
{
  m_pThumbnailImage=pImage;
}


void CGUIListItem::SetIcon(CGUIImage* pImage)
{
  m_pIconImage=pImage;
}