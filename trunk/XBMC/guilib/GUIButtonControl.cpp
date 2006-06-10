#include "include.h"
#include "GUIButtonControl.h"
#include "GUIWindowManager.h"
#include "GUIDialog.h"
#include "../xbmc/utils/CharsetConverter.h"
#include "GUIFontManager.h"
#include "../xbmc/utils/GUIInfoManager.h"


CGUIButtonControl::CGUIButtonControl(DWORD dwParentID, DWORD dwControlId, int iPosX, int iPosY, DWORD dwWidth, DWORD dwHeight, const CStdString& strTextureFocus, const CStdString& strTextureNoFocus, const CLabelInfo& labelInfo)
    : CGUIControl(dwParentID, dwControlId, iPosX, iPosY, dwWidth, dwHeight)
    , m_imgFocus(dwParentID, dwControlId, iPosX, iPosY, dwWidth, dwHeight, strTextureFocus)
    , m_imgNoFocus(dwParentID, dwControlId, iPosX, iPosY, dwWidth, dwHeight, strTextureNoFocus)
{
  m_bSelected = false;
  m_bTabButton = false;
  m_dwFocusCounter = 0;
  m_dwFlickerCounter = 0;
  m_dwFrameCounter = 0;
  m_strLabel = "";
  m_strLabel2 = "";
  m_label = labelInfo;
  m_lHyperLinkWindowID = WINDOW_INVALID;
  ControlType = GUICONTROL_BUTTON;
}

CGUIButtonControl::~CGUIButtonControl(void)
{
}

void CGUIButtonControl::Render()
{
  if (!IsVisible()) return;

  m_dwFrameCounter++;

  if (m_bTabButton)
  {
    m_imgFocus.SetVisible(m_bSelected);
    m_imgNoFocus.SetVisible(!m_bSelected);
  }
  else if (HasFocus())
  {
    if (m_pulseOnSelect)
    {
      DWORD dwAlphaCounter = m_dwFocusCounter + 2;
      DWORD dwAlphaChannel;
      if ((dwAlphaCounter % 128) >= 64)
        dwAlphaChannel = dwAlphaCounter % 64;
      else
        dwAlphaChannel = 63 - (dwAlphaCounter % 64);

      dwAlphaChannel += 192;
      dwAlphaChannel = DWORD((float)m_dwAlpha * (float)dwAlphaChannel / 255.0f);
      m_imgFocus.SetAlpha(dwAlphaChannel);
    }
    m_imgFocus.SetVisible(true);
    m_imgNoFocus.SetVisible(false);
    m_dwFocusCounter++;
  }
  else
  {
    m_imgFocus.SetVisible(false);
    m_imgNoFocus.SetVisible(true);
  }
  // render both so the visibility settings cause the frame counter to resetcorrectly
  m_imgFocus.Render();
  m_imgNoFocus.Render();

  // if we're flickering then we may not need to render text
  bool bRenderText = (m_dwFlickerCounter > 0) ? (m_dwFrameCounter % 60 > 30) : true;
  m_dwFlickerCounter = (m_dwFlickerCounter > 0) ? (m_dwFlickerCounter - 1) : 0;

  CStdString renderLabel(g_infoManager.GetMultiLabel(m_multiInfo));

  if (renderLabel.size() > 0 && bRenderText && m_label.font)
  {
    float fPosX = (float)m_iPosX + m_label.offsetX;
    float fPosY = (float)m_iPosY + m_label.offsetY;

    if (m_label.align & XBFONT_RIGHT)
      fPosX = (float)m_iPosX + m_dwWidth - m_label.offsetX;

    if (m_label.align & XBFONT_CENTER_X)
      fPosX = (float)m_iPosX + m_dwWidth / 2;

    if (m_label.align & XBFONT_CENTER_Y)
      fPosY = (float)m_iPosY + m_dwHeight / 2;

    CStdStringW strLabelUnicode;
    g_charsetConverter.utf8ToUTF16(renderLabel, strLabelUnicode);

    m_label.font->Begin();
    if (IsDisabled())
      m_label.font->DrawText( fPosX, fPosY, m_label.angle, m_label.disabledColor, m_label.shadowColor, strLabelUnicode.c_str(), m_label.align, (float)m_label.width);
    else
      m_label.font->DrawText( fPosX, fPosY, m_label.angle, m_label.textColor, m_label.shadowColor, strLabelUnicode.c_str(), m_label.align, (float)m_label.width);

    // render the second label if it exists
    if (m_strLabel2.size() > 0)
    {
      float width = m_dwWidth - 2 * m_label.offsetX - m_label.font->GetTextWidth(strLabelUnicode.c_str()) - 5;
      if (width < 0) width = 0;
      fPosX = (float)m_iPosX + m_dwWidth - m_label.offsetX;
      DWORD dwAlign = XBFONT_RIGHT | (m_label.align & XBFONT_CENTER_Y);

      g_charsetConverter.utf8ToUTF16(m_strLabel2, strLabelUnicode);

      if (IsDisabled() )
        m_label.font->DrawText( fPosX, fPosY, m_label.angle, m_label.disabledColor, m_label.shadowColor, strLabelUnicode.c_str(), dwAlign, width);
      else
        m_label.font->DrawText( fPosX, fPosY, m_label.angle, m_label.textColor, m_label.shadowColor, strLabelUnicode.c_str(), dwAlign, width);
    }
    m_label.font->End();
  }
  CGUIControl::Render();
}

bool CGUIButtonControl::OnAction(const CAction &action)
{
  if (action.wID == ACTION_SELECT_ITEM)
  {
    OnClick();
    return true;
  }
  return CGUIControl::OnAction(action);
}

bool CGUIButtonControl::OnMessage(CGUIMessage& message)
{
  if (message.GetControlId() == GetID())
  {
    if (message.GetMessage() == GUI_MSG_LABEL_SET)
    {
      SetLabel(message.GetLabel());
      return true;
    }
  }

  return CGUIControl::OnMessage(message);
}

void CGUIButtonControl::PreAllocResources()
{
  CGUIControl::PreAllocResources();
  m_imgFocus.PreAllocResources();
  m_imgNoFocus.PreAllocResources();
}

void CGUIButtonControl::AllocResources()
{
  CGUIControl::AllocResources();
  m_dwFocusCounter = 0;
  m_imgFocus.AllocResources();
  m_imgNoFocus.AllocResources();
  m_dwWidth = m_imgFocus.GetWidth();
  m_dwHeight = m_imgFocus.GetHeight();
}

void CGUIButtonControl::FreeResources()
{
  CGUIControl::FreeResources();
  m_imgFocus.FreeResources();
  m_imgNoFocus.FreeResources();
}

void CGUIButtonControl::DynamicResourceAlloc(bool bOnOff)
{
  CGUIControl::DynamicResourceAlloc(bOnOff);
  m_imgFocus.DynamicResourceAlloc(bOnOff);
  m_imgNoFocus.DynamicResourceAlloc(bOnOff);
}

void CGUIButtonControl::SetLabel(const string &label)
{
  m_strLabel = label;
  g_infoManager.ParseLabel(label, m_multiInfo);
}

void CGUIButtonControl::SetLabel2(const string &label2)
{
  m_strLabel2 = label2;
}

void CGUIButtonControl::Update()
{
  CGUIControl::Update();

  m_imgFocus.SetWidth(m_dwWidth);
  m_imgFocus.SetHeight(m_dwHeight);

  m_imgNoFocus.SetWidth(m_dwWidth);
  m_imgNoFocus.SetHeight(m_dwHeight);
}

void CGUIButtonControl::SetPosition(int iPosX, int iPosY)
{
  CGUIControl::SetPosition(iPosX, iPosY);

  m_imgFocus.SetPosition(iPosX, iPosY);
  m_imgNoFocus.SetPosition(iPosX, iPosY);
}
void CGUIButtonControl::SetAlpha(DWORD dwAlpha)
{
  CGUIControl::SetAlpha(dwAlpha);

  m_imgFocus.SetAlpha(dwAlpha);
  m_imgNoFocus.SetAlpha(dwAlpha);
}

void CGUIButtonControl::SetColourDiffuse(D3DCOLOR colour)
{
  CGUIControl::SetColourDiffuse(colour);

  m_imgFocus.SetColourDiffuse(colour);
  m_imgNoFocus.SetColourDiffuse(colour);
}


void CGUIButtonControl::SetHyperLink(long dwWindowID)
{
  m_lHyperLinkWindowID = dwWindowID;
}

bool CGUIButtonControl::OnMouseClick(DWORD dwButton)
{
  if (dwButton == MOUSE_LEFT_BUTTON)
  {
    g_Mouse.SetState(MOUSE_STATE_CLICK);
    CAction action;
    action.wID = ACTION_SELECT_ITEM;
    OnAction(action);
    return true;
  }
  return false;
}

void CGUIButtonControl::Flicker(bool bFlicker)
{
  m_dwFlickerCounter = bFlicker ? 240 : 0;
}

CStdString CGUIButtonControl::GetDescription() const
{
  CStdString strLabel(g_infoManager.GetMultiLabel(m_multiInfo));
  return strLabel;
}

void CGUIButtonControl::PythonSetLabel(const CStdString &strFont, const string &strText, DWORD dwTextColor)
{
  m_label.font = g_fontManager.GetFont(strFont);
  m_label.textColor = dwTextColor;
  SetLabel(strText);
}

void CGUIButtonControl::PythonSetDisabledColor(DWORD dwDisabledColor)
{
  m_label.disabledColor = dwDisabledColor;
}

void CGUIButtonControl::RAMSetTextColor(DWORD dwTextColor)
{
  m_label.textColor = dwTextColor;
}

void CGUIButtonControl::SettingsCategorySetTextAlign(DWORD dwAlign)
{
  m_label.align = dwAlign;
}

void CGUIButtonControl::OnClick()
{
  // Save values, SEND_CLICK_MESSAGE may deactivate the window
  long lHyperLinkWindowID = m_lHyperLinkWindowID;
  vector<CStdString> clickActions = m_clickActions;
  DWORD dwControlID = GetID();
  DWORD dwParentID = GetParentID();

  // button selected, send a message
  SEND_CLICK_MESSAGE(dwControlID, dwParentID, 0);

  if (clickActions.size())
  {
    for (unsigned int i = 0; i < clickActions.size(); i++)
    {
      CGUIMessage message(GUI_MSG_EXECUTE, dwControlID, dwParentID);
      message.SetStringParam(clickActions[i]);
      g_graphicsContext.SendMessage(message);
    }
    return;
  }

  if (lHyperLinkWindowID != WINDOW_INVALID)
  {
    m_gWindowManager.ActivateWindow(lHyperLinkWindowID);
  }
}

void CGUIButtonControl::OnFocus()
{
  if (!m_focusAction.IsEmpty())
  { // send as a thread message so that the render can finish
    CGUIMessage message(GUI_MSG_EXECUTE, m_dwControlID, m_dwParentID);
    message.SetStringParam(m_focusAction);
    m_gWindowManager.SendThreadMessage(message);
  }
}