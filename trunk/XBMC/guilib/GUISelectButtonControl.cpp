#include "guiselectbuttoncontrol.h"
#include "guifontmanager.h"


CGUISelectButtonControl::CGUISelectButtonControl(DWORD dwParentID, DWORD dwControlId, 
																								 DWORD dwPosX, DWORD dwPosY, 
																								 DWORD dwWidth, DWORD dwHeight, 
																								 const CStdString& strButtonFocus,
																								 const CStdString& strButton,
																								 const CStdString& strSelectBackground,
																								 const CStdString& strSelectArrowLeft,
																								 const CStdString& strSelectArrowRight)
:CGUIButtonControl(dwParentID, dwControlId, dwPosX, dwPosY,dwWidth, dwHeight, strButtonFocus, strButton)
,m_imgBackground(dwParentID, dwControlId, dwPosX, dwPosY, dwWidth, dwHeight,strSelectBackground)
,m_imgLeft(dwParentID, dwControlId, dwPosX, dwPosY, 16, 16,strSelectArrowLeft)
,m_imgRight(dwParentID, dwControlId, dwPosX, dwPosY, 16, 16,strSelectArrowRight)
{
	m_bShowSelect=false;
	m_iCurrentItem=-1;
}

CGUISelectButtonControl::~CGUISelectButtonControl(void)
{
}

void CGUISelectButtonControl::Render()
{
	if (!IsVisible() ) return;

	//	Are we in selection mode
	if (!m_bShowSelect)
	{
		//	No, render a normal button
		CGUIButtonControl::Render();
	}
	else
	{
		//	Yes, render the select control

		//	Render background, left and right arrow
		m_imgBackground.Render();
		m_imgLeft.Render();
		m_imgRight.Render();

		//	Render text if a current item is available
		if (m_iCurrentItem>=0 && m_pFont)
			m_pFont->DrawText((float)m_dwPosX+16+15, (float)2+m_dwPosY, m_dwTextColor, m_vecItems[m_iCurrentItem].c_str());
	}

}

bool CGUISelectButtonControl::OnMessage(CGUIMessage& message)
{
	if ( message.GetControlId()==GetID() )
	{
		if (message.GetMessage() == GUI_MSG_LABEL_ADD)
		{
			m_vecItems.push_back(message.GetLabel());
		}
		else if (message.GetMessage() == GUI_MSG_LABEL_RESET)
		{
			m_vecItems.erase(m_vecItems.begin(), m_vecItems.end());
			m_iCurrentItem=-1;
		}
		else if (message.GetMessage()==GUI_MSG_ITEM_SELECTED)
		{
			message.SetParam1(m_iCurrentItem);
		}
		else if (message.GetMessage()==GUI_MSG_ITEM_SELECT)
		{
			m_iCurrentItem=message.GetParam1();
		}
	}

	return CGUIButtonControl::OnMessage(message);
}

void CGUISelectButtonControl::OnAction(const CAction &action) 
{
	if (!m_bShowSelect)
	{
		if (action.wID == ACTION_SELECT_ITEM)
		{
			//	Enter selection mode
			m_bShowSelect=true;
		}
		else
			CGUIButtonControl::OnAction(action);
	}
	else
	{
		if (action.wID == ACTION_SELECT_ITEM)
		{
			//	User has selected an item, disable select mode
			m_bShowSelect=false;
			// and send a message.
			CGUIMessage message(GUI_MSG_CLICKED,GetID(), GetParentID() );
			g_graphicsContext.SendMessage(message);
			return;
		}
		else if (action.wID == ACTION_MOVE_LEFT)
		{
			//	Switch to previous item
			if (m_vecItems.size()>0)
			{
				m_iCurrentItem--;
				if (m_iCurrentItem<0)
					m_iCurrentItem=m_vecItems.size()-1;
			}
			return;
		}
		else if (action.wID == ACTION_MOVE_RIGHT)
		{
			//	Switch to next item
			if (m_vecItems.size()>0)
			{
				m_iCurrentItem++;
				if (m_iCurrentItem>=(int)m_vecItems.size())
					m_iCurrentItem=0;
			}
			return;
		}
		if (action.wID == ACTION_MOVE_UP || action.wID == ACTION_MOVE_DOWN )
		{
			//	Disable selection mode when moving up or down
			m_bShowSelect=false;
		}

		CGUIButtonControl::OnAction(action);
	}
}

void CGUISelectButtonControl::FreeResources()
{
	CGUIButtonControl::FreeResources();

	m_imgBackground.FreeResources();
	m_imgLeft.FreeResources();
	m_imgRight.FreeResources();

	m_bShowSelect=false;
}

void CGUISelectButtonControl::AllocResources()
{
	CGUIButtonControl::AllocResources();

	m_imgBackground.AllocResources();
	m_imgLeft.AllocResources();
	m_imgRight.AllocResources();

	//	Position right arrow
	DWORD dwPosX=(m_dwPosX+m_dwWidth-8) - 16;
  DWORD dwPosY=m_dwPosY+(m_dwHeight-16)/2;
  m_imgRight.SetPosition(dwPosX,dwPosY);

	//	Position left arrow
	dwPosX=m_dwPosX+8;
	m_imgLeft.SetPosition(dwPosX, dwPosY);
}

void CGUISelectButtonControl::Update()
{
	CGUIButtonControl::Update();

  m_imgBackground.SetWidth(m_dwWidth);
  m_imgBackground.SetHeight(m_dwHeight);
}
