#include "stdafx.h"
#include "GUIToggleButtonControl.h"
#include "guifontmanager.h"
#include "guiWindowManager.h"


CGUIToggleButtonControl::CGUIToggleButtonControl(DWORD dwParentID, DWORD dwControlId, DWORD dwPosX, DWORD dwPosY, DWORD dwWidth, DWORD dwHeight, const CStdString& strTextureFocus,const CStdString& strTextureNoFocus, const CStdString& strAltTextureFocus,const CStdString& strAltTextureNoFocus)
:CGUIControl(dwParentID, dwControlId, dwPosX, dwPosY,dwWidth, dwHeight)
,m_imgFocus(dwParentID, dwControlId, dwPosX, dwPosY,dwWidth, dwHeight, strTextureFocus)
,m_imgNoFocus(dwParentID, dwControlId, dwPosX, dwPosY,dwWidth, dwHeight, strTextureNoFocus)
,m_imgAltFocus(dwParentID, dwControlId, dwPosX, dwPosY,dwWidth, dwHeight, strAltTextureFocus)
,m_imgAltNoFocus(dwParentID, dwControlId, dwPosX, dwPosY,dwWidth, dwHeight, strAltTextureNoFocus)
{
  m_bSelected=false;
  m_dwFrameCounter = 0;
	m_strLabel=L""; 
	m_dwTextColor	= 0xFFFFFFFF; 
	m_dwDisabledColor	= 0xFF606060; 
	m_pFont=NULL;
  m_lHyperLinkWindowID=WINDOW_INVALID;
  m_lTextOffsetX = 0;
  m_lTextOffsetY = 0;
	ControlType = GUICONTROL_TOGGLEBUTTON;
}

CGUIToggleButtonControl::~CGUIToggleButtonControl(void)
{
}

void CGUIToggleButtonControl::SetDisabledColor(D3DCOLOR color)
{
  m_dwDisabledColor=color;
}
void CGUIToggleButtonControl::Render()
{
  if (!IsVisible() ) return;

  if (HasFocus())
  {
    DWORD dwAlphaCounter = m_dwFrameCounter+2;
    DWORD dwAlphaChannel;
	  if ((dwAlphaCounter%128)>=64)
		  dwAlphaChannel = dwAlphaCounter%64;
	  else
		  dwAlphaChannel = 63-(dwAlphaCounter%64);

	  dwAlphaChannel += 192;
    SetAlpha(dwAlphaChannel );
    if (m_bSelected)
      m_imgFocus.Render();
    else
      m_imgAltFocus.Render();
    m_dwFrameCounter++;

  }
  else 
  {
		SetAlpha(0xff);
    if (m_bSelected)
      m_imgNoFocus.Render();
    else
      m_imgAltNoFocus.Render();  }
  

	if (m_strLabel.size() > 0 && m_pFont)
	{	
		float fTextX = (float)10+m_dwPosX+m_lTextOffsetX;
		float fTextY = (float) 2+m_dwPosY+m_lTextOffsetY;

		m_pFont->DrawText(fTextX, fTextY, 
			IsDisabled() ? m_dwDisabledColor : m_dwTextColor, m_strLabel.c_str() );
	}

}

void CGUIToggleButtonControl::OnAction(const CAction &action) 
{
	CGUIControl::OnAction(action);
	if (action.wID == ACTION_SELECT_ITEM)
    {
		m_bSelected=!m_bSelected;
		if (m_lHyperLinkWindowID != WINDOW_INVALID)
		{
			m_gWindowManager.ActivateWindow(m_lHyperLinkWindowID);
			return;
		}
		// button selected.
		// send a message
		CGUIMessage message(GUI_MSG_CLICKED,GetID(), GetParentID() );
		g_graphicsContext.SendMessage(message);
    }
}

bool CGUIToggleButtonControl::OnMessage(CGUIMessage& message)
{
  if ( message.GetControlId()==GetID() )
  {
    if (message.GetMessage() == GUI_MSG_LABEL_SET)
    {
	    m_strLabel=message.GetLabel() ;

      return true;
    }
  }
  if (CGUIControl::OnMessage(message)) return true;
  return false;
}

void CGUIToggleButtonControl::PreAllocResources()
{
	CGUIControl::PreAllocResources();
	m_dwFrameCounter=0;
	m_imgFocus.PreAllocResources();
	m_imgNoFocus.PreAllocResources();
	m_imgAltFocus.PreAllocResources();
	m_imgAltNoFocus.PreAllocResources();
}

void CGUIToggleButtonControl::AllocResources()
{
  CGUIControl::AllocResources();
  m_dwFrameCounter=0;
  m_imgFocus.AllocResources();
  m_imgNoFocus.AllocResources();
  m_imgAltFocus.AllocResources();
  m_imgAltNoFocus.AllocResources();
  m_dwWidth=m_imgFocus.GetWidth();
  m_dwHeight=m_imgFocus.GetHeight();
}

void CGUIToggleButtonControl::FreeResources()
{
  CGUIControl::FreeResources();
  m_imgFocus.FreeResources();
  m_imgNoFocus.FreeResources();
  m_imgAltFocus.FreeResources();
  m_imgAltNoFocus.FreeResources();
}


void CGUIToggleButtonControl::SetLabel(const CStdString& strFontName,const CStdString& strLabel,D3DCOLOR dwColor)
{
  WCHAR wszText[1024];
  swprintf(wszText,L"%S",strLabel.c_str());
	m_strLabel=wszText;
	m_dwTextColor=dwColor;
	m_pFont=g_fontManager.GetFont(strFontName);
}

void CGUIToggleButtonControl::SetLabel(const CStdString& strFontName,const wstring& strLabel,D3DCOLOR dwColor)
{
	m_strLabel=strLabel;
	m_dwTextColor=dwColor;
	m_pFont=g_fontManager.GetFont(strFontName);
}


void  CGUIToggleButtonControl::Update() 
{
  CGUIControl::Update();
  
  m_imgFocus.SetWidth(m_dwWidth);
  m_imgFocus.SetHeight(m_dwHeight);

  m_imgNoFocus.SetWidth(m_dwWidth);
  m_imgNoFocus.SetHeight(m_dwHeight);

  m_imgAltFocus.SetWidth(m_dwWidth);
  m_imgAltFocus.SetHeight(m_dwHeight);

  m_imgAltNoFocus.SetWidth(m_dwWidth);
  m_imgAltNoFocus.SetHeight(m_dwHeight);
}

void CGUIToggleButtonControl::SetPosition(DWORD dwPosX, DWORD dwPosY)
{
  CGUIControl::SetPosition(dwPosX, dwPosY);
  m_imgFocus.SetPosition(dwPosX, dwPosY);
  m_imgNoFocus.SetPosition(dwPosX, dwPosY);
  m_imgAltFocus.SetPosition(dwPosX, dwPosY);
  m_imgAltNoFocus.SetPosition(dwPosX, dwPosY);
}
void CGUIToggleButtonControl::SetAlpha(DWORD dwAlpha)
{
  CGUIControl::SetAlpha(dwAlpha);
  m_imgFocus.SetAlpha(dwAlpha);
  m_imgNoFocus.SetAlpha(dwAlpha);
  m_imgAltFocus.SetAlpha(dwAlpha);
  m_imgAltNoFocus.SetAlpha(dwAlpha);
}

void CGUIToggleButtonControl::SetColourDiffuse(D3DCOLOR colour)
{
  CGUIControl::SetColourDiffuse(colour);
  m_imgFocus.SetColourDiffuse(colour);
  m_imgNoFocus.SetColourDiffuse(colour);
  m_imgAltFocus.SetColourDiffuse(colour);
  m_imgAltNoFocus.SetColourDiffuse(colour);
}


void CGUIToggleButtonControl::SetHyperLink(long dwWindowID)
{
  m_lHyperLinkWindowID=dwWindowID;
}