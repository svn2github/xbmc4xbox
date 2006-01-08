#include "include.h"
#include "GUIRadioButtonControl.h"


CGUIRadioButtonControl::CGUIRadioButtonControl(DWORD dwParentID, DWORD dwControlId, int iPosX, int iPosY, DWORD dwWidth, DWORD dwHeight,
    const CStdString& strTextureFocus, const CStdString& strTextureNoFocus,
    const CLabelInfo& labelInfo,
    const CStdString& strRadioFocus, const CStdString& strRadioNoFocus
                                              )
    : CGUIButtonControl(dwParentID, dwControlId, iPosX, iPosY, dwWidth, dwHeight, strTextureFocus, strTextureNoFocus, labelInfo)
    , m_imgRadioFocus(dwParentID, dwControlId, iPosX, iPosY, 16, 16, strRadioFocus)
    , m_imgRadioNoFocus(dwParentID, dwControlId, iPosX, iPosY, 16, 16, strRadioNoFocus)
{
  ControlType = GUICONTROL_RADIO;
}

CGUIRadioButtonControl::~CGUIRadioButtonControl(void)
{}


void CGUIRadioButtonControl::Render()
{
  if (!UpdateEffectState()) return ;
  CGUIButtonControl::Render();
  if ( IsSelected() && !IsDisabled() )
  {
    //    if ( HasFocus() )
    //   {
    m_imgRadioFocus.Render();
    //   }
    //   else
    //   {
    //    m_imgRadioNoFocus.Render();
    //   }
  }
  else
  {
    m_imgRadioNoFocus.Render();
  }
}

bool CGUIRadioButtonControl::OnAction(const CAction &action)
{
  if (action.wID == ACTION_SELECT_ITEM)
  {
    m_bSelected = !m_bSelected;
  }
  return CGUIButtonControl::OnAction(action);
}

bool CGUIRadioButtonControl::OnMessage(CGUIMessage& message)
{
  return CGUIButtonControl::OnMessage(message);
}

void CGUIRadioButtonControl::PreAllocResources()
{
  CGUIButtonControl::PreAllocResources();
  m_imgRadioFocus.PreAllocResources();
  m_imgRadioNoFocus.PreAllocResources();
}

void CGUIRadioButtonControl::AllocResources()
{
  CGUIButtonControl::AllocResources();
  m_imgRadioFocus.AllocResources();
  m_imgRadioNoFocus.AllocResources();

  SetPosition(m_iPosX, m_iPosY);
}

void CGUIRadioButtonControl::FreeResources()
{
  CGUIButtonControl::FreeResources();
  m_imgRadioFocus.FreeResources();
  m_imgRadioNoFocus.FreeResources();
}

void CGUIRadioButtonControl::DynamicResourceAlloc(bool bOnOff)
{
  CGUIControl::DynamicResourceAlloc(bOnOff);
  m_imgRadioFocus.DynamicResourceAlloc(bOnOff);
  m_imgRadioNoFocus.DynamicResourceAlloc(bOnOff);
}

void CGUIRadioButtonControl::Update()
{
  CGUIButtonControl::Update();

}

void CGUIRadioButtonControl::SetPosition(int iPosX, int iPosY)
{
  CGUIButtonControl::SetPosition(iPosX, iPosY);
  int iRadioPosX = (m_iPosX + (int)m_dwWidth - 8) - 16;
  int iRadioPosY = m_iPosY + ((int)m_dwHeight - 16) / 2;
  m_imgRadioFocus.SetPosition(iRadioPosX, iRadioPosY);
  m_imgRadioNoFocus.SetPosition(iRadioPosX, iRadioPosY);
}

void CGUIRadioButtonControl::SetWidth(int iWidth)
{
  CGUIButtonControl::SetWidth(iWidth);
  SetPosition(GetXPosition(), GetYPosition());
}

void CGUIRadioButtonControl::SetHeight(int iHeight)
{
  CGUIButtonControl::SetHeight(iHeight);
  SetPosition(GetXPosition(), GetYPosition());
}

CStdString CGUIRadioButtonControl::GetDescription() const
{
  CStdString strLabel = CGUIButtonControl::GetDescription();
  if (m_bSelected)
    strLabel += " (*)";
  else
    strLabel += " ( )";
  return strLabel;
}
