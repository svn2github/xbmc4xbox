#include "guilistcontrol.h"
#include "guifontmanager.h"

#define CONTROL_LIST		0
#define CONTROL_UPDOWN	1
CGUIListControl::CGUIListControl(DWORD dwParentID, DWORD dwControlId, DWORD dwPosX, DWORD dwPosY, DWORD dwWidth, DWORD dwHeight, 
                                 const CStdString& strFontName, 
                                 DWORD dwSpinWidth,DWORD dwSpinHeight,
                                 const CStdString& strUp, const CStdString& strDown, 
                                 const CStdString& strUpFocus, const CStdString& strDownFocus, 
                                 DWORD dwSpinColor,DWORD dwSpinX, DWORD dwSpinY,
                                 const CStdString& strFont, DWORD dwTextColor,DWORD dwSelectedColor,
                                 const CStdString& strButton, const CStdString& strButtonFocus)
:CGUIControl(dwParentID, dwControlId, dwPosX, dwPosY, dwWidth, dwHeight)
,m_upDown(dwControlId, 0, dwSpinX, dwSpinY, dwSpinWidth, dwSpinHeight, strUp, strDown, strUpFocus, strDownFocus, strFont, dwSpinColor, SPIN_CONTROL_TYPE_INT)
,m_imgButton(dwControlId, 0, dwPosX, dwPosY, dwWidth, dwHeight, strButtonFocus,strButton)
{
  m_dwSelectedColor=dwSelectedColor;
  m_iOffset=0;
  m_iItemsPerPage=10;
  m_iItemHeight=10;
  m_pFont=g_fontManager.GetFont(strFontName);
  m_pFont2=g_fontManager.GetFont(strFontName);
  m_iSelect=CONTROL_LIST;  
	m_iCursorY=0;
  m_dwTextColor=dwTextColor;
  m_strSuffix=L"|";
	m_iTextOffsetX=0;
	m_iTextOffsetY=0;
	m_iImageWidth=16;
	m_iImageHeight=16;
	m_iSpaceBetweenItems=4;
	m_iTextOffsetX2=0;
	m_iTextOffsetY2=0;
	
	m_dwTextColor2=dwTextColor;
	m_dwSelectedColor2=dwSelectedColor;
}

CGUIListControl::~CGUIListControl(void)
{
}

void CGUIListControl::Render()
{
  if (!m_pFont) return;
  if (!IsVisible()) return;
  CGUIControl::Render();
  WCHAR wszText[1024];
  DWORD dwPosY=m_dwPosY;
	
  for (int i=0; i < m_iItemsPerPage; i++)
  {
		DWORD dwPosX=m_dwPosX;
    if (i+m_iOffset < (int)m_vecItems.size() )
    {
      // render item
      CGUIListItem *pItem=m_vecItems[i+m_iOffset];
      if (i == m_iCursorY && HasFocus() && m_iSelect== CONTROL_LIST)
			{
				// render focused line
        m_imgButton.SetFocus(true);
			}
			else
			{
				// render no-focused line
        m_imgButton.SetFocus(false);
			}
      m_imgButton.SetPosition(m_dwPosX, dwPosY);	
      m_imgButton.Render();
      
			// render the icon
			if (pItem->HasIcon() )
      {
        // show icon
				CGUIImage* pImage=pItem->GetIcon();
				if (!pImage)
				{
					pImage=new CGUIImage(0,0,0,0,m_iImageWidth,m_iImageHeight,pItem->GetIconImage(),0xffffffff);
					pImage->AllocResources();
					pItem->SetIcon(pImage);
					
				}
				pImage->SetWidth(m_iImageWidth);
				pImage->SetHeight(m_iImageHeight);
				pImage->SetPosition(dwPosX+8, dwPosY+5);
				pImage->Render();
      }
			dwPosX+=(m_iImageWidth+10);

			// render the text
      DWORD dwColor=m_dwTextColor;
			if (pItem->IsSelected())
			{
        dwColor=m_dwSelectedColor;
			}
      
			dwPosX +=m_iTextOffsetX;
      bool bSelected(false);
      if (i == m_iCursorY && HasFocus() && m_iSelect== CONTROL_LIST)
			{
        bSelected=true;
			}

			CStdString strLabel2=pItem->GetLabel2();
      
			DWORD dMaxWidth=(m_dwWidth-m_iImageWidth-16);
      if ( strLabel2.size() > 0 )
      {
				float fTextHeight,fTextWidth;
        swprintf(wszText,L"%S", strLabel2.c_str() );
				m_pFont2->GetTextExtent( wszText, &fTextWidth,&fTextHeight);
				dMaxWidth -= (DWORD)(fTextWidth);
			}

			swprintf(wszText,L"%S", pItem->GetLabel().c_str() );
			RenderText((float)dwPosX, (float)dwPosY+2+m_iTextOffsetY, (FLOAT)dMaxWidth, dwColor, wszText,bSelected);
      
      if (strLabel2.size()>0)
      {
				dwColor=m_dwTextColor2;
				if (pItem->IsSelected())
				{
					dwColor=m_dwSelectedColor2;
				}
				if (!m_iTextOffsetX2)
					dwPosX=m_dwPosX+m_dwWidth-16;
				else
					dwPosX=m_dwPosX+m_iTextOffsetX2;

        swprintf(wszText,L"%S", strLabel2.c_str() );
        m_pFont2->DrawText((float)dwPosX, (float)dwPosY+2+m_iTextOffsetY2,dwColor,wszText,XBFONT_RIGHT); 
      }	
      dwPosY += (DWORD)(m_iItemHeight+m_iSpaceBetweenItems);
    }
  }
	m_upDown.Render();
}

void CGUIListControl::RenderText(float fPosX, float fPosY, float fMaxWidth,DWORD dwTextColor, WCHAR* wszText,bool bScroll )
{
	static int scroll_pos = 0;
	static int iScrollX=0;
	static int iLastItem=-1;
	static int iFrames=0;
	static int iStartFrame=0;

  float fTextHeight,fTextWidth;
  m_pFont->GetTextExtent( wszText, &fTextWidth,&fTextHeight);

	g_graphicsContext.SetViewPort(fPosX,fPosY,fMaxWidth-5.0f,60.0f);


  if (!bScroll || fTextWidth <= fMaxWidth)
  {
    m_pFont->DrawTextWidth(fPosX,fPosY,dwTextColor,wszText,fMaxWidth);
		g_graphicsContext.RestoreViewPort();
    return;
  }
  else
  {
    // scroll
    int iItem=m_iCursorY+m_iOffset;
    WCHAR wszOrgText[1024];
    wcscpy(wszOrgText, wszText);
    wcscat(wszOrgText, m_strSuffix.c_str());
    m_pFont->GetTextExtent( wszOrgText, &fTextWidth,&fTextHeight);

    if (fTextWidth > fMaxWidth)
    {
				fMaxWidth+=50.0f;
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
              m_pFont->DrawTextWidth(fPosX-iScrollX,fPosY,m_dwTextColor,szText,fMaxWidth);
						
					}
					else
					{
						iStartFrame++;
						if (fPosY >=0.0)
              m_pFont->DrawTextWidth(fPosX,fPosY,m_dwTextColor,wszText,fMaxWidth);
					}
    }
  }
	g_graphicsContext.RestoreViewPort();
}

void CGUIListControl::OnKey(const CKey& key)
{
  if (!key.IsButton() ) return;

  switch (key.GetButtonCode())
  {
    case KEY_REMOTE_REVERSE:
    case KEY_BUTTON_LEFT_TRIGGER:
      OnPageUp();
    break;

    case KEY_REMOTE_FORWARD:
    case KEY_BUTTON_RIGHT_TRIGGER:
      OnPageDown();
    break;

    case KEY_REMOTE_DOWN:
    case KEY_BUTTON_DPAD_DOWN:
    {
      OnDown();
    }
    break;
    
    case KEY_REMOTE_UP:
    case KEY_BUTTON_DPAD_UP:
    {
      OnUp();
    }
    break;

    case KEY_REMOTE_LEFT:
    case KEY_BUTTON_DPAD_LEFT:
    {
      OnLeft();
    }
    break;

    case KEY_REMOTE_RIGHT:
    case KEY_BUTTON_DPAD_RIGHT:
    {
      OnRight();
    }
    break;

    default:
    {
      if (m_iSelect==CONTROL_LIST)
      {
          CGUIMessage msg(GUI_MSG_CLICKED, GetID(), GetParentID(),key.GetButtonCode());
          g_graphicsContext.SendMessage(msg);
      }
      else
      {
        m_upDown.OnKey(key);
      }
    }
  }
}

bool CGUIListControl::OnMessage(CGUIMessage& message)
{
  if (message.GetControlId() == GetID() )
  {
    if (message.GetSenderId()==0)
    {
      if (message.GetMessage() == GUI_MSG_CLICKED)
      {
        m_iOffset=(m_upDown.GetValue()-1)*m_iItemsPerPage;
				while (m_iOffset+m_iCursorY >= (int)m_vecItems.size()) m_iCursorY--;
      }
    }
    if (message.GetMessage() == GUI_MSG_LOSTFOCUS ||
        message.GetMessage() == GUI_MSG_SETFOCUS)
    {
      m_iSelect=CONTROL_LIST;
    }
    if (message.GetMessage() == GUI_MSG_LABEL_ADD)
    {
      CGUIListItem* pItem=(CGUIListItem*)message.GetLPVOID();
      m_vecItems.push_back( pItem);
      int iPages=m_vecItems.size() / m_iItemsPerPage;
      if (m_vecItems.size() % m_iItemsPerPage) iPages++;
      m_upDown.SetRange(1,iPages);
      m_upDown.SetValue(1);
    }

    if (message.GetMessage() == GUI_MSG_LABEL_RESET)
    {
      m_iCursorY=0;
      m_iOffset=0;
      m_vecItems.erase(m_vecItems.begin(),m_vecItems.end());
      m_upDown.SetRange(1,1);
      m_upDown.SetValue(1);
    }

    if (message.GetMessage()==GUI_MSG_ITEM_SELECTED)
    {
      message.SetParam1(m_iCursorY+m_iOffset);
    }
		if (message.GetMessage()==GUI_MSG_ITEM_SELECT)
		{
			if (message.GetParam1() >=0 && message.GetParam1() < (int)m_vecItems.size())
			{
				int iPage=1;
				m_iOffset=0;
				m_iCursorY=message.GetParam1();
				while (m_iCursorY >= m_iItemsPerPage)
				{
					iPage++;
					m_iOffset+=m_iItemsPerPage;
					m_iCursorY -=m_iItemsPerPage;
				}
				m_upDown.SetValue(iPage);
			}
		}
  }

  if ( CGUIControl::OnMessage(message) ) return true;

  return false;
}

void CGUIListControl::AllocResources()
{
  if (!m_pFont) return;
  CGUIControl::AllocResources();
  m_upDown.AllocResources();
  
  m_imgButton.AllocResources();

  
  m_imgButton.SetWidth(m_dwWidth);
  m_imgButton.SetHeight(m_iItemHeight);

	float fHeight=(float)m_iItemHeight + (float)m_iSpaceBetweenItems;
  float fTotalHeight= (float)(m_dwHeight-m_upDown.GetHeight()-5);
  m_iItemsPerPage		= (int)(fTotalHeight / fHeight );

  int iPages=m_vecItems.size() / m_iItemsPerPage;
  if (m_vecItems.size() % m_iItemsPerPage) iPages++;
  m_upDown.SetRange(1,iPages);
  m_upDown.SetValue(1);

}

void CGUIListControl::FreeResources()
{
  CGUIControl::FreeResources();
  m_upDown.FreeResources();
  m_imgButton.FreeResources();
}

void CGUIListControl::OnRight()
{
  CKey key(true,KEY_BUTTON_DPAD_RIGHT);
  if (m_iSelect==CONTROL_LIST) 
  {
    m_iSelect=CONTROL_UPDOWN;
    m_upDown.SetFocus(true);
  }
  else
  {
    m_upDown.OnKey(key);
    if (!m_upDown.HasFocus()) 
    {
      CGUIControl::OnKey(key);
    }
  }
}

void CGUIListControl::OnLeft()
{
  CKey key(true,KEY_BUTTON_DPAD_LEFT);
  if (m_iSelect==CONTROL_LIST) 
  {
    CGUIControl::OnKey(key);
  }
  else
  {
    m_upDown.OnKey(key);
    if (!m_upDown.HasFocus()) 
    {
      m_iSelect=CONTROL_LIST;
    }
  }
}

void CGUIListControl::OnUp()
{
  CKey key(true,KEY_BUTTON_DPAD_UP);
  if (m_iSelect==CONTROL_LIST) 
  {
    if (m_iCursorY > 0) 
    {
      m_iCursorY--;
    }
    else if (m_iCursorY ==0 && m_iOffset)
    {
      m_iOffset--;
    }
    else
    {
      return CGUIControl::OnKey(key);
    }
  }
  else
  {
    m_upDown.OnKey(key);
    if (!m_upDown.HasFocus()) 
    {
      m_iSelect=CONTROL_LIST;
    }  
  }
}

void CGUIListControl::OnDown()
{
  CKey key(true,KEY_BUTTON_DPAD_DOWN);
  if (m_iSelect==CONTROL_LIST) 
  {
    if (m_iCursorY+1 < m_iItemsPerPage)
    {
      m_iCursorY++;
    }
    else 
    {
      m_upDown.SetFocus(true);
      m_iSelect = CONTROL_UPDOWN;
    }
  }
  else
  {
    m_upDown.OnKey(key);
    if (!m_upDown.HasFocus()) 
    {
      CGUIControl::OnKey(key);
    }  
  }
}

void CGUIListControl::SetScrollySuffix(const CStdString& wstrSuffix)
{
  WCHAR wsSuffix[128];
  swprintf(wsSuffix,L"%S", wstrSuffix.c_str());
  m_strSuffix=wsSuffix;
}


void CGUIListControl::OnPageUp()
{
  int iPage = m_upDown.GetValue();
  if (iPage > 1)
  {
    iPage--;
    m_upDown.SetValue(iPage);
    m_iOffset=(m_upDown.GetValue()-1)*m_iItemsPerPage;
  }
}

void CGUIListControl::OnPageDown()
{
  int iPages=m_vecItems.size() / m_iItemsPerPage;
  if (m_vecItems.size() % m_iItemsPerPage) iPages++;

  int iPage = m_upDown.GetValue();
  if (iPage+1 <= iPages)
  {
    iPage++;
    m_upDown.SetValue(iPage);
    m_iOffset=(m_upDown.GetValue()-1)*m_iItemsPerPage;
  }
  if (m_iOffset+m_iCursorY >= (int)m_vecItems.size() )
  {
    m_iCursorY = (m_vecItems.size()-m_iOffset)-1;
  }
}


void CGUIListControl::SetTextOffsets(int iXoffset, int iYOffset,int iXoffset2, int iYOffset2)
{
	m_iTextOffsetX = iXoffset;
	m_iTextOffsetY = iYOffset;
	m_iTextOffsetX2 = iXoffset2;
	m_iTextOffsetY2 = iYOffset2;
}

void CGUIListControl::SetImageDimensions(int iWidth, int iHeight)
{
	m_iImageWidth  = iWidth;
	m_iImageHeight = iHeight;
}

void CGUIListControl::SetItemHeight(int iHeight)
{
	m_iItemHeight=iHeight;
}
void CGUIListControl::SetSpace(int iHeight)
{
	m_iSpaceBetweenItems=iHeight;
}

void CGUIListControl::SetFont2(const CStdString& strFont)
{
	m_pFont2=g_fontManager.GetFont(strFont);
}
void CGUIListControl::SetColors2(DWORD dwTextColor, DWORD dwSelectedColor)
{
	m_dwTextColor2=dwTextColor;
	m_dwSelectedColor2=dwSelectedColor;
}