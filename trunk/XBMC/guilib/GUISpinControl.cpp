#include "guispincontrol.h"
#include "guifontmanager.h"

#define SPIN_BUTTON_DOWN 0
#define SPIN_BUTTON_UP   1
CGUISpinControl::CGUISpinControl(DWORD dwParentID, DWORD dwControlId, DWORD dwPosX, DWORD dwPosY, DWORD dwWidth, DWORD dwHeight, const string& strUp, const string& strDown, const string& strUpFocus, const string& strDownFocus, const string& strFont, DWORD dwTextColor, int iType)
:CGUIControl(dwParentID, dwControlId, dwPosX, dwPosY, dwWidth, dwHeight)
,m_imgspinUp(dwParentID, dwControlId, dwPosX, dwPosY, dwWidth, dwHeight,strUp)
,m_imgspinDown(dwParentID, dwControlId, dwPosX, dwPosY, dwWidth, dwHeight,strDown)
,m_imgspinUpFocus(dwParentID, dwControlId, dwPosX, dwPosY, dwWidth, dwHeight,strUpFocus)
,m_imgspinDownFocus(dwParentID, dwControlId, dwPosX, dwPosY, dwWidth, dwHeight,strDownFocus)
{
  m_iStart=0;
  m_iEnd=100;
  m_fStart=0.0f;
  m_fEnd=1.0f;
  m_iValue=0;
  m_fValue=0.0;
  m_strFont=strFont;
  m_pFont=NULL;
  m_dwTextColor=dwTextColor;
  m_iType=iType;
  m_iSelect=SPIN_BUTTON_DOWN;

  AddLabel("label1");
  AddLabel("label2");
}

CGUISpinControl::~CGUISpinControl(void)
{
}


void CGUISpinControl::OnKey(const CKey& key)
{
  if (key.IsButton() )
  {
    if (key.GetButtonCode() == KEY_BUTTON_DPAD_LEFT)
    {
      if (m_iSelect==SPIN_BUTTON_UP) 
      {
        m_iSelect=SPIN_BUTTON_DOWN;
        return;
      }
    }
    if (key.GetButtonCode() == KEY_BUTTON_DPAD_RIGHT)
    {
      if (m_iSelect==SPIN_BUTTON_DOWN) 
      {
        m_iSelect=SPIN_BUTTON_UP;
        return;
      }
    }
    if (key.GetButtonCode()==KEY_BUTTON_A)
    {
      switch (m_iType)
      {
        case SPIN_CONTROL_TYPE_INT:
        {
          if (m_iSelect==SPIN_BUTTON_DOWN)
          {
            if (m_iValue-1 >= m_iStart)
              m_iValue--;
          }
          if (m_iSelect==SPIN_BUTTON_UP)
          {
            if (m_iValue+1 <= m_iEnd)
              m_iValue++;
          }
          CGUIMessage msg(GUI_MSG_CLICKED, GetID(), GetParentID());
          g_graphicsContext.SendMessage(msg);
          return;
        }
        break;
      
        case SPIN_CONTROL_TYPE_FLOAT:
        {
          if (m_iSelect==SPIN_BUTTON_DOWN)
          {
            if (m_fValue-0.1 >= m_fStart)
              m_fValue-=0.1f;
          }
          if (m_iSelect==SPIN_BUTTON_UP)
          {
            if (m_fValue+0.1 <= m_fEnd)
              m_fValue+=0.1f;
          }
          CGUIMessage msg(GUI_MSG_CLICKED, GetID(), GetParentID());
          g_graphicsContext.SendMessage(msg);
          return;
        }
        break;

        case SPIN_CONTROL_TYPE_TEXT:
        {
          if (m_iSelect==SPIN_BUTTON_DOWN)
          {
            if (m_iValue-1 >= 0)
              m_iValue--;
          }
          if (m_iSelect==SPIN_BUTTON_UP)
          {
            if (m_iValue+1 < (int)m_vecLabels.size() )
              m_iValue++;
          }
          CGUIMessage msg(GUI_MSG_CLICKED, GetID(), GetParentID());
          g_graphicsContext.SendMessage(msg);
          return;
        }
        break;
      }
    }
  }
  CGUIControl::OnKey(key);
}


bool CGUISpinControl::OnMessage(CGUIMessage& message)
{

	if (CGUIControl::OnMessage(message) ) 
  {
    if (!HasFocus()) m_iSelect=SPIN_BUTTON_DOWN;
    return true;
  }
  if (message.GetControlId() == GetID() )
  {
    switch (message.GetMessage())
    {
      case GUI_MSG_LABEL_ADD:
      {
        char *pLabel=(char*)message.GetLPVOID();
        AddLabel(pLabel);
        return true;
      }
      break;
    }
  }
	return false;
}


void CGUISpinControl::AllocResources()
{
	CGUIControl::AllocResources();
	m_imgspinUp.AllocResources();
	m_imgspinUpFocus.AllocResources();
	m_imgspinDown.AllocResources();
	m_imgspinDownFocus.AllocResources();

  m_pFont=g_fontManager.GetFont(m_strFont);
  SetPosition(m_dwPosX, m_dwPosY);
}

void CGUISpinControl::FreeResources()
{
	CGUIControl::FreeResources();
	m_imgspinUp.FreeResources();
	m_imgspinUpFocus.FreeResources();
	m_imgspinDown.FreeResources();
	m_imgspinDownFocus.FreeResources();
}



void CGUISpinControl::Render()
{
	if (!IsVisible()) return;
	if ( HasFocus() )
	{
		if (m_iSelect==SPIN_BUTTON_UP) 
      m_imgspinUpFocus.Render();
    else 
      m_imgspinUp.Render();

    if (m_iSelect==SPIN_BUTTON_DOWN) 
		  m_imgspinDownFocus.Render();
    else
      m_imgspinDown.Render();
	}
	else
	{
		m_imgspinUp.Render();
		m_imgspinDown.Render();
	}

	if (m_pFont)
	{
    WCHAR wszText[1024];
    if (m_iType == SPIN_CONTROL_TYPE_INT)
      swprintf(wszText,L"%i/%i",m_iValue, m_iEnd);
    else if (m_iType==SPIN_CONTROL_TYPE_FLOAT)
      swprintf(wszText,L"%02.2f/%02.2f",m_fValue, m_fEnd);
    else
    {
      swprintf(wszText,L"%S", m_vecLabels[m_iValue].c_str() );
    }

    float fWidth,fHeight;
    m_pFont->GetTextExtent( wszText, &fWidth,&fHeight);
    fHeight/=2;
    float fPosY = (float)m_dwHeight/2;
    fPosY-=fHeight;
    fPosY+=(float)m_dwPosY;

    
    m_pFont->DrawText((float)m_dwPosX-3, (float)fPosY,m_dwTextColor,wszText,XBFONT_RIGHT);
	}
}



void CGUISpinControl::SetRange(int iStart, int iEnd)
{
  m_iStart=iStart;
  m_iEnd=iEnd;
}


void CGUISpinControl::SetFloatRange(float fStart, float fEnd)
{
  m_fStart=fStart;
  m_fEnd=fEnd;
}

void CGUISpinControl::SetValue(int iValue)
{
  m_iValue=iValue;
}

void CGUISpinControl::SetFloatValue(float fValue)
{
  m_fValue=fValue;
}

int  CGUISpinControl::GetValue() const
{
  return m_iValue;
}

float CGUISpinControl::GetFloatValue() const
{
  return m_fValue;
}


void CGUISpinControl::AddLabel(const string& strLabel)
{
  m_vecLabels.push_back(strLabel);
}

const string CGUISpinControl::GetLabel() const
{
  return m_vecLabels[ m_iValue];
}

void CGUISpinControl::SetPosition(DWORD dwPosX, DWORD dwPosY)
{
  CGUIControl::SetPosition(dwPosX, dwPosY);

  m_imgspinDownFocus.SetPosition(dwPosX, dwPosY);
  m_imgspinDown.SetPosition(dwPosX, dwPosY);

  m_imgspinUp.SetPosition(m_dwPosX + m_imgspinDown.GetWidth(),m_dwPosY);
  m_imgspinUpFocus.SetPosition(m_dwPosX + m_imgspinDownFocus.GetWidth(),m_dwPosY);

}

DWORD CGUISpinControl::GetWidth() const
{
  return m_imgspinDown.GetWidth() * 2 ;
}

void CGUISpinControl::SetFocus(bool bOnOff)
{
  CGUIControl::SetFocus(bOnOff);
  m_iSelect=SPIN_BUTTON_DOWN;
}