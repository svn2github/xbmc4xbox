#include "stdafx.h"
#include "guithumbnailpanel.h"
#include "guifontmanager.h"

#define CONTROL_LIST		0
#define CONTROL_UPDOWN	1

/*
total width/height for an item including space between items:
  <itemWidth>128</itemWidth>
	<itemHeight>128</itemHeight>
  
width/height of folder icon:
  <textureWidth>80</textureWidth>
  <textureHeight>80</textureHeight> 

width/height of thumbnail
  <thumbWidth>128</thumbWidth>
  <thumbHeight>100</thumbHeight>

relative position of thumbnail in the folder icon
  <thumbPosX>4</thumbPosX>
  <thumbPosY>16</thumbPosY>


*/
CGUIThumbnailPanel::CGUIThumbnailPanel(DWORD dwParentID, DWORD dwControlId, DWORD dwPosX, DWORD dwPosY, DWORD dwWidth, DWORD dwHeight, 
                                 const CStdString& strFontName, 
                                 const CStdString& strImageIcon,
                                 const CStdString& strImageIconFocus,
                                 DWORD dwitemWidth, DWORD dwitemHeight,
                                 DWORD dwSpinWidth,DWORD dwSpinHeight,
                                 const CStdString& strUp, const CStdString& strDown, 
                                 const CStdString& strUpFocus, const CStdString& strDownFocus, 
                                 DWORD dwSpinColor,DWORD dwSpinX, DWORD dwSpinY,
                                 const CStdString& strFont, DWORD dwTextColor, DWORD dwSelectedColor)
:CGUIControl(dwParentID, dwControlId, dwPosX, dwPosY, dwWidth, dwHeight)
,m_imgFolder(dwParentID, dwControlId, dwPosX, dwPosY, dwitemWidth,dwitemHeight,strImageIcon)
,m_imgFolderFocus(dwParentID, dwControlId, dwPosX, dwPosY, dwitemWidth,dwitemHeight,strImageIconFocus)
,m_upDown(dwControlId, 0, dwSpinX, dwSpinY, dwSpinWidth, dwSpinHeight, strUp, strDown, strUpFocus, strDownFocus, strFont, dwSpinColor, SPIN_CONTROL_TYPE_INT)
{
  m_iItemWidth=dwitemWidth;
  m_iItemHeight=dwitemHeight;
  m_iOffset    = 0; 
  m_dwSelectedColor=dwSelectedColor;
  m_pFont      = g_fontManager.GetFont(strFontName);
  m_iSelect    = CONTROL_LIST;  
	m_iCursorY   = 0;
  m_iCursorX   = 0;
  m_dwTextColor= dwTextColor;
  m_strSuffix=L"|";  
	m_bScrollUp=false;
	m_bScrollDown=false;
  m_bShowTexture=true;
	m_iScrollCounter=0;
	m_iLastItem=-1;
	m_iTextureWidth=80;
	m_iTextureHeight=80;
  m_iThumbWidth=64;
  m_iThumbHeight=64;
  m_iThumbXPos=8;
  m_iThumbYPos=8;
	ControlType = GUICONTROL_THUMBNAIL;
}

CGUIThumbnailPanel::~CGUIThumbnailPanel(void)
{
}

void CGUIThumbnailPanel::RenderItem(bool bFocus,DWORD dwPosX, DWORD dwPosY, CGUIListItem* pItem)
{

  WCHAR wszText[1024];
  float fTextPosY =(float)dwPosY+ (float)m_iTextureHeight;
	swprintf(wszText,L"%S", pItem->GetLabel().c_str() );

  DWORD dwColor=m_dwTextColor;
  DWORD dwCenteredPosX = dwPosX + (m_iItemWidth-m_iTextureWidth)/2;
	if (pItem->IsSelected()) dwColor=m_dwSelectedColor;
  if (bFocus && HasFocus()&&m_iSelect==CONTROL_LIST )
  {
    m_imgFolderFocus.SetPosition(dwCenteredPosX, dwPosY);
		if (m_bShowTexture) m_imgFolderFocus.Render();
    
    RenderText((float)dwPosX,(float)fTextPosY,dwColor,wszText,true);
  }
  else
  {
 	m_imgFolder.SetPosition(dwCenteredPosX, dwPosY);
    if (m_bShowTexture) m_imgFolder.Render();
    
    RenderText((float)dwPosX,(float)fTextPosY,dwColor,wszText,false);
  
  }
	if (pItem->HasThumbnail() )
  {
		CGUIImage *pImage=pItem->GetThumbnail();
		if (!pImage )
    {
			pImage=new CGUIImage(0,0,m_iThumbXPos+dwCenteredPosX,m_iThumbYPos+dwPosY,m_iThumbWidth,m_iThumbHeight,pItem->GetThumbnailImage(),0x0);
      pImage->SetKeepAspectRatio(true);
      pImage->AllocResources();
			pItem->SetThumbnail(pImage);
      int xOff=(m_iThumbWidth-pImage->GetRenderWidth())/2;
      int yOff=(m_iThumbHeight-pImage->GetRenderHeight())/2;
      pImage->SetPosition(m_iThumbXPos+dwCenteredPosX+xOff,m_iThumbYPos+dwPosY+yOff);
    }
    else
    {
      pImage->SetWidth(m_iThumbWidth);
      pImage->SetHeight(m_iThumbHeight);
      int xOff=(m_iThumbWidth-pImage->GetRenderWidth())/2;
      int yOff=(m_iThumbHeight-pImage->GetRenderHeight())/2;
      pImage->SetPosition(m_iThumbXPos+dwCenteredPosX+xOff,m_iThumbYPos+dwPosY+yOff);
      pImage->Render();
    }
  }
}

void CGUIThumbnailPanel::Render()
{
  if (!m_pFont) return;
  if (!IsVisible()) return;

  if (!ValidItem(m_iCursorX,m_iCursorY) )
  {
      m_iCursorX=0;
      m_iCursorY=0;
  }
  CGUIControl::Render();

	int iScrollYOffset=0;
	if (m_bScrollDown)
	{
		iScrollYOffset=-(m_iItemHeight-m_iScrollCounter);
	}
	if (m_bScrollUp)
	{
		iScrollYOffset=m_iItemHeight-m_iScrollCounter;
	}


	g_graphicsContext.SetViewPort( (float)m_dwPosX, (float)m_dwPosY, (float)m_iColumns*m_iItemWidth, (float)m_iRows*m_iItemHeight);

  int iStartItem=30000;
  int iEndItem=-1;
	if (m_bScrollUp)
	{
		// render item on top
		DWORD dwPosY=m_dwPosY -m_iItemHeight + iScrollYOffset;
    m_iOffset-=m_iColumns;
    for (int iCol=0; iCol < m_iColumns; iCol++)
    {
			DWORD dwPosX = m_dwPosX + iCol*m_iItemWidth;
      int iItem = iCol+m_iOffset;
      if (iItem>0 && iItem < (int)m_vecItems.size())
      {
        CGUIListItem *pItem=m_vecItems[iItem];
				RenderItem(false,dwPosX,dwPosY,pItem);
        if (iItem<iStartItem) iStartItem=iItem;
        if (iItem>iEndItem) iEndItem=iItem;
      }
    }
    m_iOffset+=m_iColumns;
	}

	// render main panel
  for (int iRow=0; iRow < m_iRows; iRow++)
  {
    DWORD dwPosY=m_dwPosY + iRow*m_iItemHeight + iScrollYOffset;
    for (int iCol=0; iCol < m_iColumns; iCol++)
    {
			DWORD dwPosX = m_dwPosX + iCol*m_iItemWidth;
	    int iItem = iRow*m_iColumns+iCol+m_iOffset;
      if (iItem < (int)m_vecItems.size())
      {
        CGUIListItem *pItem=m_vecItems[iItem];
				bool bFocus=(m_iCursorX==iCol && m_iCursorY==iRow );
				RenderItem(bFocus,dwPosX,dwPosY,pItem);
        if (iItem<iStartItem) iStartItem=iItem;
        if (iItem>iEndItem) iEndItem=iItem;
      }
    }
  }

	if (m_bScrollDown)
	{
		// render item on bottom
		DWORD dwPosY=m_dwPosY + m_iRows*m_iItemHeight + iScrollYOffset;
    for (int iCol=0; iCol < m_iColumns; iCol++)
    {
			DWORD dwPosX = m_dwPosX + iCol*m_iItemWidth;
	    int iItem = m_iRows*m_iColumns+iCol+m_iOffset;
      if (iItem < (int)m_vecItems.size())
      {
        CGUIListItem *pItem=m_vecItems[iItem];
				RenderItem(false,dwPosX,dwPosY,pItem);
        if (iItem<iStartItem) iStartItem=iItem;
        if (iItem>iEndItem) iEndItem=iItem;
      }
    }
	}

	g_graphicsContext.RestoreViewPort();
  

	//
  int iFrames=12;
  int iStep=m_iItemHeight/iFrames;
  if (!iStep) iStep=1;
	if (m_bScrollDown)
	{
		m_iScrollCounter-=iStep;
		if (m_iScrollCounter<=0)
		{
			m_bScrollDown=false;
			m_iOffset+=m_iColumns;
			// Check if we need to update our position
			if (!ValidItem(m_iCursorX, m_iCursorY))
			{	// select the last item available
				int iPos = m_vecItems.size()-1-m_iOffset;
				m_iCursorY = iPos / m_iColumns;
				m_iCursorX = iPos % m_iColumns;
			}
			// Update the page counter
			int iPage = m_iOffset/(m_iRows*m_iColumns);
			m_upDown.SetValue(iPage+1);
		}
	}
	if (m_bScrollUp)
	{
		m_iScrollCounter-=iStep;
		if (m_iScrollCounter<=0)
		{
			m_bScrollUp=false;
      m_iOffset -= m_iColumns;
			int iPage = m_iOffset/(m_iRows*m_iColumns);
			m_upDown.SetValue(iPage+1);
		}
	}
  //free memory
  if (iStartItem < 30000)
  {
    for (int i=0; i < iStartItem;++i)
    {
      CGUIListItem *pItem=m_vecItems[i];
      if (pItem)
      {
        pItem->FreeMemory();
      }
    }
  }

  for (int i=iEndItem+1; i < (int)m_vecItems.size(); ++i) 
  {
    CGUIListItem *pItem=m_vecItems[i];
    if (pItem)
    {
      pItem->FreeMemory();
    }
  }
  m_upDown.Render();
}

void CGUIThumbnailPanel::OnAction(const CAction &action)
{
  switch (action.wID)
  {
	case ACTION_PAGE_UP:
      OnPageUp();
    break;

    case ACTION_PAGE_DOWN:
      OnPageDown();
    break;


    case ACTION_MOVE_DOWN:
    {
      OnDown();
    }
    break;
    
    case ACTION_MOVE_UP:
    {
      OnUp();
    }
    break;

    case ACTION_MOVE_LEFT:
    {
      OnLeft();
    }
    break;

    case ACTION_MOVE_RIGHT:
    {
      OnRight();
    }
    break;

    default:
    {
      if (m_iSelect==CONTROL_LIST)
      {
          CGUIMessage msg(GUI_MSG_CLICKED, GetID(), GetParentID(), action.wID);
          g_graphicsContext.SendMessage(msg);
      }
      else
      {
        m_upDown.OnAction(action);
      }
    }
  }
}

bool CGUIThumbnailPanel::OnMessage(CGUIMessage& message)
{
  if (message.GetControlId() == GetID() )
  {
    if (message.GetSenderId()==0)
    {
      if (message.GetMessage() == GUI_MSG_CLICKED)
      {
        m_iOffset=(m_upDown.GetValue()-1)*(m_iRows*m_iColumns);
      }
    }
    if (message.GetMessage() == GUI_MSG_LOSTFOCUS ||
        message.GetMessage() == GUI_MSG_SETFOCUS)
    {
      m_iSelect=CONTROL_LIST;
    }
    if (message.GetMessage() == GUI_MSG_LABEL_ADD)
    {
      m_vecItems.push_back( (CGUIListItem*) message.GetLPVOID() );
      int iItemsPerPage=m_iRows*m_iColumns;
      int iPages=m_vecItems.size() / iItemsPerPage;
      if (m_vecItems.size() % iItemsPerPage) iPages++;
      m_upDown.SetRange(1,iPages);
      m_upDown.SetValue(1);
    }

    if (message.GetMessage() == GUI_MSG_LABEL_RESET)
    {
      m_vecItems.erase(m_vecItems.begin(),m_vecItems.end());
      m_upDown.SetRange(1,1);
      m_upDown.SetValue(1);
      m_iCursorX=m_iCursorY=m_iOffset=0;

    }
    if (message.GetMessage()==GUI_MSG_ITEM_SELECTED)
    {
      message.SetParam1(m_iOffset + m_iCursorY*m_iColumns+m_iCursorX);
    }
		if (message.GetMessage()==GUI_MSG_ITEM_SELECT)
		{
			int iItem=message.GetParam1();
			if (iItem >=0 && iItem < (int)m_vecItems.size())
			{
				int iPage=1;
				m_iCursorX=0;
				m_iCursorY=0;
				m_iOffset=0;
				while (iItem >= (m_iRows*m_iColumns) )
				{
					m_iOffset +=(m_iRows*m_iColumns);
					iItem -= (m_iRows*m_iColumns);
					iPage++;
				}
				while (iItem >= m_iColumns)
				{
					m_iCursorY++;
					iItem -= m_iColumns;
				}
				m_upDown.SetValue(iPage);
				m_iCursorX=iItem;
			}
		}
  }

  if ( CGUIControl::OnMessage(message) ) return true;

  return false;

}

void CGUIThumbnailPanel::PreAllocResources()
{
	if (!m_pFont) return;
	CGUIControl::PreAllocResources();
	m_upDown.PreAllocResources();
	m_imgFolder.PreAllocResources();
	m_imgFolderFocus.PreAllocResources();
}

void CGUIThumbnailPanel::Calculate()
{
	m_imgFolder.SetWidth(m_iTextureWidth);
	m_imgFolder.SetHeight(m_iTextureHeight);
	m_imgFolderFocus.SetWidth(m_iTextureWidth);
	m_imgFolderFocus.SetHeight(m_iTextureHeight);
	m_iLastItem=-1;
  float fWidth,fHeight;
  
  // height of 1 item = folder image height + text row height + space in between
  m_pFont->GetTextExtent( L"y", &fWidth,&fHeight);
  
  
  fWidth =(float)m_iItemWidth;
  fHeight=(float)m_iItemHeight;
  float fTotalHeight= (float)(m_dwHeight -5);
  m_iRows		        = (int)(fTotalHeight / fHeight);
    
  
  m_iColumns = (int) (m_dwWidth / fWidth );

  int iItemsPerPage=m_iRows*m_iColumns;
  int iPages=m_vecItems.size() / iItemsPerPage;
  if (m_vecItems.size() % iItemsPerPage) iPages++;
  m_upDown.SetRange(1,iPages);
  m_upDown.SetValue(1);
}

void CGUIThumbnailPanel::AllocResources()
{
  if (!m_pFont) return;
  CGUIControl::AllocResources();
  m_upDown.AllocResources();
  m_imgFolder.AllocResources();
  m_imgFolderFocus.AllocResources();
  Calculate();

}

void CGUIThumbnailPanel::FreeResources()
{
  CGUIControl::FreeResources();
  m_upDown.FreeResources();
  m_imgFolder.FreeResources();
  m_imgFolderFocus.FreeResources();
}
bool CGUIThumbnailPanel::ValidItem(int iX, int iY)
{
  if (iX >= m_iColumns) return false;
  if (iY >= m_iRows) return false;
  if (m_iOffset + iY*m_iColumns+iX < (int)m_vecItems.size() ) return true;
  return false;
}
void CGUIThumbnailPanel::OnRight()
{
  CKey key(KEY_BUTTON_DPAD_RIGHT);
  CAction action;
  action.wID = ACTION_MOVE_RIGHT;
  if (m_iSelect==CONTROL_LIST) 
  {
    if (m_iCursorX+1 < m_iColumns && ValidItem(m_iCursorX+1,m_iCursorY) )
    {
      m_iCursorX++;
      return;
    }

    if (m_upDown.GetMaximum() > 1)
    {
      m_iSelect=CONTROL_UPDOWN;
      m_upDown.SetFocus(true);
    }
  }
  else
  {
    m_upDown.OnAction(action);
    if (!m_upDown.HasFocus()) 
    {
      CGUIControl::OnAction(action);
    }
  }
}

void CGUIThumbnailPanel::OnLeft()
{
  CKey key(KEY_BUTTON_DPAD_LEFT);
  CAction action;
  action.wID = ACTION_MOVE_LEFT;
  if (m_iSelect==CONTROL_LIST) 
  {
    if (m_iCursorX > 0) 
    {
      m_iCursorX--;
      return;
    }
    CGUIControl::OnAction(action);
  }
  else
  {
    m_upDown.OnAction(action);
    if (!m_upDown.HasFocus()) 
    {
      m_iSelect=CONTROL_LIST;
    }
  }
}

void CGUIThumbnailPanel::OnUp()
{
  CKey key(KEY_BUTTON_DPAD_UP);
  CAction action;
  action.wID = ACTION_MOVE_UP;
  if (m_iSelect==CONTROL_LIST) 
  {
		if (m_bScrollUp)
		{
			m_iScrollCounter=0;
			m_bScrollUp=false;
			m_iOffset -= m_iColumns;
			int iPage = m_iOffset/(m_iRows*m_iColumns);
			m_upDown.SetValue(iPage+1);
		}

    if (m_iCursorY > 0) 
    {
      m_iCursorY--; 
    }
    else if (m_iCursorY ==0 && m_iOffset)
    {
			m_iScrollCounter=m_iItemHeight;
			m_bScrollUp=true;
     // m_iOffset-=m_iColumns;
    }
    else
    {
      return CGUIControl::OnAction(action);
    }
  }
  else
  {
    m_upDown.OnAction(action);
    if (!m_upDown.HasFocus()) 
    {
      m_iSelect=CONTROL_LIST;
    }  
  }
}

void CGUIThumbnailPanel::OnDown()
{
  CKey key(KEY_BUTTON_DPAD_DOWN);
  CAction action;
  action.wID = ACTION_MOVE_DOWN;
  if (m_iSelect==CONTROL_LIST) 
  {
		if (m_bScrollDown)
		{
			m_bScrollDown=false;
			m_iOffset+=m_iColumns;
			// Check if we need to update our position
			if (!ValidItem(m_iCursorX, m_iCursorY))
			{	// select the last item available
				int iPos = m_vecItems.size()-1-m_iOffset;
				m_iCursorY = iPos / m_iColumns;
				m_iCursorX = iPos % m_iColumns;
			}
			int iPage = m_iOffset/(m_iRows*m_iColumns);
			m_upDown.SetValue(iPage+1);
		}

    if (m_iCursorY+1==m_iRows)
    {
		m_iOffset+= m_iColumns;
		// Check if we can scroll down.
		// This will be if we have an item underneath us, or if we have a row underneath us
		if ( ValidItem(m_iCursorX,m_iCursorY) || (unsigned)(m_iOffset+m_iColumns*m_iCursorY) < m_vecItems.size()) 
		{	// Yes, we can scroll down
			m_iScrollCounter=m_iItemHeight;
			m_bScrollDown=true;
		}
		m_iOffset-= m_iColumns;
		return;
	}
    else
    {
      if ( ValidItem(m_iCursorX,m_iCursorY+1) )
      {
        m_iCursorY++;
      }
      else 
      {
        m_upDown.SetFocus(true);
        m_iSelect = CONTROL_UPDOWN;
      }
    }
  }
  else
  {
    m_upDown.OnAction(action);
    if (!m_upDown.HasFocus()) 
    {
      CGUIControl::OnAction(action);
    }  
  }
}

void CGUIThumbnailPanel::RenderText(float fPosX, float fPosY, DWORD dwTextColor, WCHAR* wszText,bool bScroll )
{
	static int scroll_pos = 0;
	static int iScrollX=0;
	static int iLastItem=-1;
	static int iFrames=0;
	static int iStartFrame=0;

  float fTextHeight,fTextWidth;
  m_pFont->GetTextExtent( wszText, &fTextWidth,&fTextHeight);
	float fMaxWidth=(float)m_iItemWidth*0.9f;
	fPosX += ((float)m_iItemWidth-fMaxWidth)/2.0f;
  if (!bScroll)
  {
	// Center text to make it look nicer...
	if (fTextWidth <= fMaxWidth) fPosX+=(fMaxWidth-fTextWidth)/2;
    m_pFont->DrawTextWidth(fPosX,fPosY,dwTextColor,wszText,fMaxWidth);
    return;
  }
  else
	{
		if (fTextWidth <= fMaxWidth)
		{	// Center text to make it look nicer...
			fPosX+=(fMaxWidth-fTextWidth)/2;
			m_pFont->DrawTextWidth(fPosX,fPosY,dwTextColor,wszText,fMaxWidth);
			iLastItem = -1; // reset scroller
			return;
		}
		float fPosCX=fPosX;
		float fPosCY=fPosY;
		g_graphicsContext.Correct(fPosCX, fPosCY);
		if (fPosCX <0) fPosCX=0.0f;
		if (fPosCY <0) fPosCY=0.0f;
		if (fPosCY >g_graphicsContext.GetHeight()) fPosCY=(float)g_graphicsContext.GetHeight();
		float fHeight=60.0f;
		if (fHeight+fPosCY >= g_graphicsContext.GetHeight() )
			fHeight = g_graphicsContext.GetHeight() - fPosCY -1;
		if (fHeight <= 0) return ;

//		float fwidth=fMaxWidth-5.0f;

		D3DVIEWPORT8 newviewport,oldviewport;
		g_graphicsContext.Get3DDevice()->GetViewport(&oldviewport);
		newviewport.X      = (DWORD)fPosCX;
		newviewport.Y			 = (DWORD)fPosCY;
		newviewport.Width  = (DWORD)(fMaxWidth);
		newviewport.Height = (DWORD)(fHeight);
		newviewport.MinZ   = 0.0f;
		newviewport.MaxZ   = 1.0f;
		g_graphicsContext.Get3DDevice()->SetViewport(&newviewport);

    // scroll
    WCHAR wszOrgText[1024];
    wcscpy(wszOrgText, wszText);
		wcscat(wszOrgText, L" ");
    wcscat(wszOrgText,m_strSuffix.c_str());
    m_pFont->GetTextExtent( wszOrgText, &fTextWidth,&fTextHeight);

    int iItem=m_iCursorX+m_iCursorY*m_iColumns+m_iOffset;
    if (fTextWidth > fMaxWidth)
    {
				fMaxWidth+=50;
        WCHAR szText[1024];
				if (iLastItem != iItem)
				{
					scroll_pos=0;
					iLastItem=iItem;
					iStartFrame=0;
					iScrollX=1;
				}
        if (iStartFrame > 25)
					{
						WCHAR wTmp[3];
						if (scroll_pos >= (int)wcslen(wszOrgText) )
							wTmp[0]=L' ';
						else
							wTmp[0]=wszOrgText[scroll_pos];
						wTmp[1]=0;
            float fWidth,fHeight;
						m_pFont->GetTextExtent(wTmp,&fWidth,&fHeight);
						if ( iScrollX >= fWidth)
						{
							++scroll_pos;
							if (scroll_pos > (int)wcslen(wszOrgText) )
								scroll_pos = 0;
							iFrames=0;
							iScrollX=1;
						}
						else iScrollX++;
					
						int ipos=0;
						for (int i=0; i < (int)wcslen(wszOrgText); i++)
						{
							if (i+scroll_pos < (int)wcslen(wszOrgText))
								szText[i]=wszOrgText[i+scroll_pos];
							else
							{
								if (ipos==0) szText[i]=L' ';
								else szText[i]=wszOrgText[ipos-1];
								ipos++;
							}
							szText[i+1]=0;
						}
						if (fPosY >=0.0)
              m_pFont->DrawTextWidth(fPosX-iScrollX,fPosY,dwTextColor,szText,fMaxWidth);
						
					}
					else
					{
						iStartFrame++;
						if (fPosY >=0.0)
              m_pFont->DrawTextWidth(fPosX,fPosY,dwTextColor,wszText,fMaxWidth);
					}
    }
		
		g_graphicsContext.Get3DDevice()->SetViewport(&oldviewport);

  }
}

void CGUIThumbnailPanel::SetScrollySuffix(CStdString wstrSuffix)
{
  WCHAR wsSuffix[128];
  swprintf(wsSuffix,L"%S", wstrSuffix.c_str());
  m_strSuffix=wsSuffix;
}

void CGUIThumbnailPanel::OnPageUp()
{
  int iPage = m_upDown.GetValue();
  if (iPage > 1)
  {
    iPage--;
    m_upDown.SetValue(iPage);
    m_iOffset=(m_upDown.GetValue()-1)* m_iColumns * m_iRows;
  }
}

void CGUIThumbnailPanel::OnPageDown()
{
  int iItemsPerPage=m_iRows*m_iColumns;
  int iPages=m_vecItems.size() / iItemsPerPage;
  if (m_vecItems.size() % iItemsPerPage) iPages++;

  int iPage = m_upDown.GetValue();
  if (iPage+1 <= iPages)
  {
    iPage++;
    m_upDown.SetValue(iPage);
    m_iOffset=(m_upDown.GetValue()-1)*iItemsPerPage;
  }
  while  (m_iCursorX > 0 && m_iOffset + m_iCursorY*m_iColumns+m_iCursorX >= (int) m_vecItems.size() )
  {
    m_iCursorX--;
  }
  while  (m_iCursorY > 0 && m_iOffset + m_iCursorY*m_iColumns+m_iCursorX >= (int) m_vecItems.size() )
  {
    m_iCursorY--;
  }

}
void CGUIThumbnailPanel::SetTextureDimensions(int iWidth, int iHeight)
{
	m_iTextureWidth=iWidth;
	m_iTextureHeight=iHeight;

  
  m_imgFolder.SetHeight(m_iTextureHeight);
  m_imgFolderFocus.SetHeight(m_iTextureHeight);

  m_imgFolder.SetWidth(m_iTextureWidth);
  m_imgFolderFocus.SetWidth(m_iTextureWidth);
}


void CGUIThumbnailPanel::SetThumbDimensions(int iXpos, int iYpos,int iWidth, int iHeight)
{
  m_iThumbWidth=iWidth;
  m_iThumbHeight=iHeight;
  m_iThumbXPos=iXpos;
  m_iThumbYPos=iYpos;
}
void CGUIThumbnailPanel::GetThumbDimensions(int& iXpos, int& iYpos,int& iWidth, int& iHeight)
{
  iWidth=m_iThumbWidth;
  iHeight=m_iThumbHeight;
  iXpos=m_iThumbXPos;
  iYpos=m_iThumbYPos;
}

void CGUIThumbnailPanel::SetItemWidth(DWORD dwWidth)
{
  m_iItemWidth=dwWidth;
  FreeResources();
  AllocResources();
}
void CGUIThumbnailPanel::SetItemHeight(DWORD dwHeight)
{
  m_iItemHeight=dwHeight;
  FreeResources();
  AllocResources();
}
void CGUIThumbnailPanel::ShowTexture(bool bOnoff)
{
  m_bShowTexture=bOnoff;
}

int CGUIThumbnailPanel::GetSelectedItem(CStdString& strLabel)
{
  strLabel="";
  int iItem=m_iOffset + m_iCursorY*m_iColumns+m_iCursorX;
  if (iItem >=0 && iItem < (int)m_vecItems.size())
  {
   CGUIListItem *pItem=m_vecItems[iItem];
   strLabel=pItem->GetLabel();
   if (pItem->m_bIsFolder)
   {
     strLabel.Format("[%s]", pItem->GetLabel().c_str());
   }
  }
  return iItem;
}

void CGUIThumbnailPanel::ShowBigIcons(bool bOnOff)
{
  if (bOnOff)
  {
    m_iItemWidth=m_iItemWidthBig;
    m_iItemHeight=m_iItemHeightBig;
	  m_iTextureWidth=m_iTextureWidthBig;
	  m_iTextureHeight=m_iTextureHeightBig;
    SetThumbDimensions(m_iThumbXPosBig,m_iThumbYPosBig,m_iThumbWidthBig,m_iThumbHeightBig);
  }
  else
  {
    m_iItemWidth=m_iItemWidthLow;
    m_iItemHeight=m_iItemHeightLow;
	  m_iTextureWidth=m_iTextureWidthLow;
	  m_iTextureHeight=m_iTextureHeightLow;
    SetThumbDimensions(m_iThumbXPosLow,m_iThumbYPosLow,m_iThumbWidthLow,m_iThumbHeightLow);
  }
  Calculate();
}

void CGUIThumbnailPanel::GetThumbDimensionsBig(int& iXpos, int& iYpos,int& iWidth, int& iHeight)
{
  iXpos=m_iThumbXPosBig;
  iYpos=m_iThumbYPosBig;
  iWidth=m_iThumbWidthBig;
  iHeight=m_iThumbHeightBig;
}

void CGUIThumbnailPanel::GetThumbDimensionsLow(int& iXpos, int& iYpos,int& iWidth, int& iHeight)
{
  iXpos=m_iThumbXPosLow;
  iYpos=m_iThumbYPosLow;
  iWidth=m_iThumbWidthLow;
  iHeight=m_iThumbHeightLow;
}