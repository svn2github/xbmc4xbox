#include "stdafx.h"
#include "GUIListControlEx.h"
#include "guifontmanager.h"
#include "../xbmc/utils/log.h"

#define CONTROL_LIST		0
#define CONTROL_UPDOWN	1
CGUIListControlEx::CGUIListControlEx(DWORD dwParentID, DWORD dwControlId, DWORD dwPosX, DWORD dwPosY, DWORD dwWidth, DWORD dwHeight, 
                                 const CStdString& strFontName, 
                                 DWORD dwSpinWidth,DWORD dwSpinHeight,
                                 const CStdString& strUp, const CStdString& strDown, 
                                 const CStdString& strUpFocus, const CStdString& strDownFocus, 
                                 DWORD dwSpinColor,DWORD dwSpinX, DWORD dwSpinY,
                                 const CStdString& strFont, DWORD dwTextColor,DWORD dwSelectedColor,
                                 const CStdString& strButton, const CStdString& strButtonFocus,
								 DWORD dwItemTextOffsetX, DWORD dwItemTextOffsetY)
:CGUIControl(dwParentID, dwControlId, dwPosX, dwPosY, dwWidth, dwHeight)
,m_upDown(dwControlId, 0, dwSpinX, dwSpinY, dwSpinWidth, dwSpinHeight, strUp, strDown, strUpFocus, strDownFocus, strFont, dwSpinColor, SPIN_CONTROL_TYPE_INT)
,m_imgButton(dwControlId, 0, dwPosX, dwPosY, dwWidth, dwHeight, strButtonFocus,strButton, dwItemTextOffsetX, dwItemTextOffsetY)
{
	m_pList = NULL;
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
	m_iImageWidth=16;
	m_iImageHeight=16;
	m_iSpaceBetweenItems=4;
	m_bUpDownVisible = true;	// show the spin control by default
	
	m_dwTextColor2=dwTextColor;
	m_dwSelectedColor2=dwSelectedColor;
	m_dwTextOffsetX = dwItemTextOffsetX;
	m_dwTextOffsetY = dwItemTextOffsetY;
}

CGUIListControlEx::~CGUIListControlEx(void)
{
	DeleteCriticalSection(&m_critical_section);
}

void CGUIListControlEx::Render()
{
	if ( (!IsVisible()) || (!m_pList) || (!m_pFont) )
	{
		return;
	}

	CGUIControl::Render();
	
	DWORD dwPosY=m_dwPosY;	
	CGUIList::GUILISTITEMS& list = m_pList->Lock();

	for (int i=0; i < m_iItemsPerPage; i++)
	{
		DWORD dwPosX=m_dwPosX;
		if (i+m_iOffset < (int)list.size() )
		{
			CGUIItem* pItem = list[i+m_iOffset];

			// create a list item rendering context
			CGUIListExItem::RenderContext context;
			context.m_dwPositionX			= dwPosX;
			context.m_dwPositionY			= dwPosY;
			context.m_bFocused				= (i == m_iCursorY && HasFocus() && m_iSelect== CONTROL_LIST);
			context.m_pButton				= &m_imgButton;
			context.m_pFont					= m_pFont;
			context.m_dwTextNormalColour	= m_dwTextColor;
			context.m_dwTextSelectedColour	= m_dwSelectedColor;

			// render the list item
			pItem->OnPaint(&context);

			dwPosY += (DWORD)(m_iItemHeight+m_iSpaceBetweenItems);
		}
	}
	
	if (m_bUpDownVisible)
	{
		int iPages = list.size() / m_iItemsPerPage;
		
		if (list.size() % m_iItemsPerPage) 
		{
			iPages++;
		}
		m_upDown.SetRange(1,iPages);
		m_upDown.SetValue(1);
		m_upDown.Render();
	}

	m_pList->Release();
}


void CGUIListControlEx::OnAction(const CAction &action)
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

    case ACTION_SELECT_ITEM:
    {
		CGUIMessage msg(GUI_MSG_CLICKED, GetID(), GetParentID(), action.wID);
		g_graphicsContext.SendMessage(msg);
		break;
    }
  }
}

bool CGUIListControlEx::OnMessage(CGUIMessage& message)
{
	if (!m_pList)
	{
		if ( (message.GetControlId() == GetID()) &&
			 (message.GetMessage() == GUI_MSG_LABEL_BIND) )
		{
			m_pList = (CGUIList*) message.GetLPVOID();
		}

		return CGUIControl::OnMessage(message);
	}

	if (message.GetControlId() == GetID() )
	{
		if (message.GetSenderId()==0)
		{
			if (message.GetMessage() == GUI_MSG_CLICKED)
			{
				CGUIList::GUILISTITEMS& list = m_pList->Lock();

				m_iOffset=(m_upDown.GetValue()-1)*m_iItemsPerPage;
				while (m_iOffset+m_iCursorY >= (int)list.size()) m_iCursorY--;

				m_pList->Release();
			}
		}
    
		if (message.GetMessage() == GUI_MSG_LOSTFOCUS ||
			message.GetMessage() == GUI_MSG_SETFOCUS)
		{
			m_iSelect=CONTROL_LIST;
		}
    
		if (message.GetMessage() == GUI_MSG_LABEL_BIND)
		{
			CGUIList* pNewList = (CGUIList*) message.GetLPVOID();
			CGUIList* pOldList = m_pList;
			pOldList->Lock();
			m_pList = pNewList;
			pOldList->Release();
		}

		if (message.GetMessage()==GUI_MSG_ITEM_SELECTED)
		{
			CGUIList::GUILISTITEMS& list = m_pList->Lock();

			if ((int)list.size()>m_iCursorY+m_iOffset)
			{
				message.SetParam1(m_iCursorY+m_iOffset);
				message.SetLPVOID(list[m_iCursorY+m_iOffset]);
			}
			else
			{
				m_iCursorY = m_iOffset = 0;
			}

			m_pList->Release();
		}
		
		if (message.GetMessage()==GUI_MSG_ITEM_SELECT)
		{
			CGUIList::GUILISTITEMS& list = m_pList->Lock();

			if (message.GetParam1() >=0 && message.GetParam1() < (int)list.size())
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

			m_pList->Release();
		}
	}

	if ( CGUIControl::OnMessage(message) )
	{
		return true;
	}

	return false;
}

void CGUIListControlEx::PreAllocResources()
{
	if (!m_pFont) return;
	CGUIControl::PreAllocResources();
	m_upDown.PreAllocResources();
	m_imgButton.PreAllocResources();
}

void CGUIListControlEx::AllocResources()
{
	if (!m_pFont)
	{
		return;
	}
  
	CGUIControl::AllocResources();
	m_upDown.AllocResources();
  
	m_imgButton.AllocResources(); 
	m_imgButton.SetWidth(m_dwWidth);
	m_imgButton.SetHeight(m_iItemHeight);

	float fActualItemHeight=(float)m_iItemHeight + (float)m_iSpaceBetweenItems;
	float fTotalHeight= (float)(m_dwHeight-m_upDown.GetHeight()-5);
	m_iItemsPerPage		= (int)(fTotalHeight / fActualItemHeight );
	m_upDown.SetRange(1,1);
	m_upDown.SetValue(1);
}

void CGUIListControlEx::FreeResources()
{
	CGUIControl::FreeResources();
	m_upDown.FreeResources();
	m_imgButton.FreeResources();
}

void CGUIListControlEx::OnRight()
{
  CKey key(KEY_BUTTON_DPAD_RIGHT);
  CAction action;
  action.wID = ACTION_MOVE_RIGHT;
  if (m_iSelect==CONTROL_LIST) 
  {
    if (m_upDown.GetMaximum() > 1)
    {
      m_iSelect=CONTROL_UPDOWN;
      m_upDown.SetFocus(true);
      if (!m_upDown.HasFocus()) 
      {
        m_iSelect=CONTROL_LIST;
      }
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

void CGUIListControlEx::OnLeft()
{
  CKey key(KEY_BUTTON_DPAD_LEFT);
  CAction action;
  action.wID = ACTION_MOVE_LEFT;
  if (m_iSelect==CONTROL_LIST) 
  {
    CGUIControl::OnAction(action);
    if (!m_upDown.HasFocus()) 
    {
      m_iSelect=CONTROL_LIST;
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

void CGUIListControlEx::OnUp()
{

  CKey key(KEY_BUTTON_DPAD_UP);
  CAction action;
  action.wID = ACTION_MOVE_UP;
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
    else if (m_pList)
    {
		// move 2 last item in list
		CGUIList::GUILISTITEMS& list = m_pList->Lock();
		CGUIMessage msg(GUI_MSG_ITEM_SELECT, GetID(), GetID(), list.size() -1); 
		OnMessage(msg);
		m_pList->Release();
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

void CGUIListControlEx::OnDown()
{
	CKey key(KEY_BUTTON_DPAD_DOWN);
	CAction action;
	action.wID = ACTION_MOVE_DOWN;

	if ((m_iSelect==CONTROL_LIST) && (m_pList))
	{
		CGUIList::GUILISTITEMS& list = m_pList->Lock();

		if (m_iCursorY+1 < m_iItemsPerPage)
		{
			if (m_iOffset+1+m_iCursorY <  (int)list.size())
			{
				m_iCursorY++;
			}
			else
			{
				// move first item in list
				CGUIMessage msg(GUI_MSG_ITEM_SELECT, GetID(), GetID(), 0); 
				OnMessage(msg);
			}
		}
	    else 
		{
			if (m_iOffset+1+m_iCursorY <  (int)list.size())
			{
				m_iOffset++;

				int iPage=1;
				int iSel=m_iOffset+m_iCursorY;
				while (iSel >= m_iItemsPerPage)
				{
					iPage++;
					iSel -= m_iItemsPerPage;
				}
				m_upDown.SetValue(iPage);
			}
			else
			{
				// move first item in list
				CGUIMessage msg(GUI_MSG_ITEM_SELECT, GetID(), GetID(), 0); 
				OnMessage(msg);
			}
		}
		m_pList->Release();
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

void CGUIListControlEx::SetScrollySuffix(const CStdString& wstrSuffix)
{
  WCHAR wsSuffix[128];
  swprintf(wsSuffix,L"%S", wstrSuffix.c_str());
  m_strSuffix=wsSuffix;
}


void CGUIListControlEx::OnPageUp()
{
  int iPage = m_upDown.GetValue();
  if (iPage > 1)
  {
    iPage--;
    m_upDown.SetValue(iPage);
    m_iOffset=(m_upDown.GetValue()-1)*m_iItemsPerPage;
  }
	else 
	{
		// already on page 1, then select the 1st item
		m_iCursorY=0;
	}
}

void CGUIListControlEx::OnPageDown()
{
	if (!m_pList)
	{
		return;
	}

	CGUIList::GUILISTITEMS& list = m_pList->Lock();

	int iPages=list.size() / m_iItemsPerPage;
	if (list.size() % m_iItemsPerPage)
		iPages++;

	int iPage = m_upDown.GetValue();
	if (iPage+1 <= iPages)
	{
		iPage++;
		m_upDown.SetValue(iPage);
		m_iOffset=(m_upDown.GetValue()-1)*m_iItemsPerPage;
	}
	else
	{
		// already on last page, move 2 last item in list
		CGUIMessage msg(GUI_MSG_ITEM_SELECT, GetID(), GetID(), list.size() -1); 
		OnMessage(msg);
	}
	
	if (m_iOffset+m_iCursorY >= (int)list.size() )
	{
		m_iCursorY = (list.size()-m_iOffset)-1;
	}
	
	m_pList->Release();
}




void CGUIListControlEx::SetImageDimensions(int iWidth, int iHeight)
{
	m_iImageWidth  = iWidth;
	m_iImageHeight = iHeight;
}

void CGUIListControlEx::SetItemHeight(int iHeight)
{
	m_iItemHeight=iHeight;
}
void CGUIListControlEx::SetSpace(int iHeight)
{
	m_iSpaceBetweenItems=iHeight;
}

void CGUIListControlEx::SetFont2(const CStdString& strFont)
{
	if (strFont != "")
	{
		m_pFont2=g_fontManager.GetFont(strFont);
	}
}
void CGUIListControlEx::SetColors2(DWORD dwTextColor, DWORD dwSelectedColor)
{
	m_dwTextColor2=dwTextColor;
	m_dwSelectedColor2=dwSelectedColor;
}

int CGUIListControlEx::GetSelectedItem(CStdString& strLabel)
{
	if(!m_pList)
	{
		return -1;
	}

	CGUIList::GUILISTITEMS& list = m_pList->Lock();

	strLabel="";
	int iItem = m_iCursorY+m_iOffset;
	if (iItem >=0 && iItem < (int)list.size())
	{
		CGUIItem* pItem = list[iItem];	
		strLabel = pItem->GetName();
	}

	m_pList->Release();
	
	return iItem;
}

void CGUIListControlEx::SetPageControlVisible(bool bVisible)
{
	m_bUpDownVisible = bVisible;
	return;
}