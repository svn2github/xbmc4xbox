#include "include.h"
#include "GUITextBox.h"
#include "../xbmc/utils/CharsetConverter.h"
#include "../xbmc/StringUtils.h"
#include "GUILabelControl.h"

#define CONTROL_LIST  0
#define CONTROL_UPDOWN 9998
CGUITextBox::CGUITextBox(DWORD dwParentID, DWORD dwControlId, int iPosX, int iPosY, DWORD dwWidth, DWORD dwHeight,
                         DWORD dwSpinWidth, DWORD dwSpinHeight,
                         const CStdString& strUp, const CStdString& strDown,
                         const CStdString& strUpFocus, const CStdString& strDownFocus,
                         const CLabelInfo& spinInfo, int iSpinX, int iSpinY,
                         const CLabelInfo& labelInfo)
    : CGUIControl(dwParentID, dwControlId, iPosX, iPosY, dwWidth, dwHeight)
    , m_upDown(dwControlId, CONTROL_UPDOWN, 0, 0, dwSpinWidth, dwSpinHeight, strUp, strDown, strUpFocus, strDownFocus, spinInfo, SPIN_CONTROL_TYPE_INT)
{
  m_upDown.SetSpinAlign(XBFONT_CENTER_Y | XBFONT_RIGHT, 0);
  m_iOffset = 0;
  m_label = labelInfo;
  m_iItemsPerPage = 10;
  m_iItemHeight = 10;
  m_iSpinPosX = iSpinX;
  m_iSpinPosY = iSpinY;
  m_iMaxPages = 50;
  m_upDown.SetShowRange(true); // show the range by default
  ControlType = GUICONTROL_TEXTBOX;
  m_wrapText = false;
}

CGUITextBox::~CGUITextBox(void)
{
}

void CGUITextBox::Render()
{
  if (!IsVisible()) return;

  if (m_bInvalidated)
  { 
    // first correct any sizing we need to do
    float fWidth, fHeight;
    m_label.font->GetTextExtent( L"y", &fWidth, &fHeight);
    m_iItemHeight = (int)fHeight;
    float fTotalHeight = (float)(m_dwHeight - m_upDown.GetHeight() - 5);
    m_iItemsPerPage = (int)(fTotalHeight / fHeight);

    // we have all the sizing correct so do any wordwrapping
    m_vecItems.erase(m_vecItems.begin(), m_vecItems.end());
    CStdString text(m_strText);
    CGUILabelControl::WrapText(text, m_label.font, (float)m_dwWidth);
    // convert to line by lines
    CStdStringArray lines;
    StringUtils::SplitString(text, "\n", lines);
    for (unsigned int i = 0; i < lines.size(); i++)
    {
      CGUIListItem item(lines[i]);
      m_vecItems.push_back(item);
    }
    UpdatePageControl();
  }

  int iPosY = m_iPosY;

  if (m_label.font)
  {
    m_label.font->Begin();
    for (int i = 0; i < m_iItemsPerPage; i++)
    {
      int iPosX = m_iPosX;
      if (i + m_iOffset < (int)m_vecItems.size() )
      {
        // render item
        CGUIListItem& item = m_vecItems[i + m_iOffset];
        CStdString strLabel1 = item.GetLabel();
        CStdString strLabel2 = item.GetLabel2();

        CStdStringW strText1Unicode;
        g_charsetConverter.utf8ToUTF16(strLabel1, strText1Unicode);

        DWORD dMaxWidth = m_dwWidth + 16;
        if (strLabel2.size())
        {
          CStdStringW strText2Unicode;
          g_charsetConverter.utf8ToUTF16(strLabel2, strText2Unicode);
          float fTextWidth, fTextHeight;
          m_label.font->GetTextExtent( strText2Unicode.c_str(), &fTextWidth, &fTextHeight);
          dMaxWidth -= (DWORD)(fTextWidth);

          m_label.font->DrawTextWidth((float)iPosX + dMaxWidth, (float)iPosY + 2, m_label.textColor, m_label.shadowColor, strText2Unicode.c_str(), (float)fTextWidth);
        }
        m_label.font->DrawTextWidth((float)iPosX, (float)iPosY + 2, m_label.textColor, m_label.shadowColor, strText1Unicode.c_str(), (float)dMaxWidth);
        iPosY += (DWORD)m_iItemHeight;
      }
    }
    m_label.font->End();
  }
  m_upDown.SetPosition(m_iPosX + m_iSpinPosX, m_iPosY + m_iSpinPosY);
  m_upDown.Render();
  CGUIControl::Render();
}

bool CGUITextBox::OnAction(const CAction &action)
{
  switch (action.wID)
  {
  case ACTION_PAGE_UP:
    OnPageUp();
    return true;
    break;

  case ACTION_PAGE_DOWN:
    OnPageDown();
    return true;
    break;

  case ACTION_MOVE_UP:
  case ACTION_MOVE_DOWN:
  case ACTION_MOVE_LEFT:
  case ACTION_MOVE_RIGHT:
    return CGUIControl::OnAction(action);
    break;

  default:
    return m_upDown.OnAction(action);
  }
}

bool CGUITextBox::OnMessage(CGUIMessage& message)
{
  if (message.GetControlId() == GetID() )
  {
    if (message.GetSenderId() == CONTROL_UPDOWN)
    {
      if (message.GetMessage() == GUI_MSG_CLICKED)
      {
        m_iOffset = (m_upDown.GetValue() - 1) * m_iItemsPerPage;
      }
    }
    if (message.GetMessage() == GUI_MSG_LABEL2_SET)
    {
      int iItem = message.GetParam1();
      if (iItem >= 0 && iItem < (int)m_vecItems.size())
      {
        CGUIListItem& item = m_vecItems[iItem];
        item.SetLabel2( message.GetLabel() );
      }
    }
    if (message.GetMessage() == GUI_MSG_LABEL_BIND)
    { // send parameter is a link to a vector of CGUIListItem's
      vector<CGUIListItem> *items = (vector<CGUIListItem> *)message.GetLPVOID();
      if (items)
      {
        m_vecItems.clear();
        m_vecItems.assign(items->begin(), items->end());
        m_wrapText = false;
        UpdatePageControl();
      }
    }
    if (message.GetMessage() == GUI_MSG_LABEL_SET)
    {
      m_iOffset = 0;
      m_vecItems.erase(m_vecItems.begin(), m_vecItems.end());
      m_upDown.SetRange(1, 1);
      m_upDown.SetValue(1);

      // set max pages (param1)
      if (message.GetParam1() > 0) m_iMaxPages = message.GetParam1();
      SetText( message.GetLabel() );
    }

    if (message.GetMessage() == GUI_MSG_LABEL_RESET)
    {
      m_iOffset = 0;
      m_vecItems.erase(m_vecItems.begin(), m_vecItems.end());
      m_upDown.SetRange(1, 1);
      m_upDown.SetValue(1);
    }

    if (message.GetMessage() == GUI_MSG_SETFOCUS)
    {
      m_upDown.SetFocus(true);
    }
    if (message.GetMessage() == GUI_MSG_LOSTFOCUS)
    {
      m_upDown.SetFocus(false);
    }
  }

  if ( CGUIControl::OnMessage(message) ) return true;

  return false;
}

void CGUITextBox::PreAllocResources()
{
  if (!m_label.font) return;
  CGUIControl::PreAllocResources();
  m_upDown.PreAllocResources();
}

void CGUITextBox::AllocResources()
{
  if (!m_label.font) return;
  CGUIControl::AllocResources();
  m_upDown.AllocResources();

  SetHeight(m_dwHeight);
}

void CGUITextBox::FreeResources()
{
  CGUIControl::FreeResources();
  m_upDown.FreeResources();
}

void CGUITextBox::DynamicResourceAlloc(bool bOnOff)
{
  CGUIControl::DynamicResourceAlloc(bOnOff);
  m_upDown.DynamicResourceAlloc(bOnOff);
}

void CGUITextBox::OnRight()
{
  if (!m_upDown.IsFocusedOnUp())
    m_upDown.OnRight();
  else
    CGUIControl::OnRight();
}

void CGUITextBox::OnLeft()
{
  if (m_upDown.IsFocusedOnUp())
    m_upDown.OnLeft();
  else
    CGUIControl::OnLeft();
}

void CGUITextBox::OnPageUp()
{
  int iPage = m_upDown.GetValue();
  if (iPage > 1)
  {
    iPage--;
    m_upDown.SetValue(iPage);
    m_iOffset = (m_upDown.GetValue() - 1) * m_iItemsPerPage;
  }
}

void CGUITextBox::OnPageDown()
{
  int iPages = m_vecItems.size() / m_iItemsPerPage;
  if (m_vecItems.size() % m_iItemsPerPage) iPages++;

  int iPage = m_upDown.GetValue();
  if (iPage + 1 <= iPages)
  {
    iPage++;
    m_upDown.SetValue(iPage);
    m_iOffset = (m_upDown.GetValue() - 1) * m_iItemsPerPage;
  }
}
void CGUITextBox::SetText(const string &strText)
{
  m_wrapText = true;
  m_strText = strText;
  m_bInvalidated = true;
}

void CGUITextBox::UpdatePageControl()
{
  // and update our page control
  int iPages = m_vecItems.size() / m_iItemsPerPage;
  if (m_vecItems.size() % m_iItemsPerPage || !iPages) iPages++;
  m_upDown.SetRange(1, iPages);
  m_upDown.SetValue(1);
}

bool CGUITextBox::HitTest(int iPosX, int iPosY) const
{
  if (m_upDown.HitTest(iPosX, iPosY)) return true;
  return CGUIControl::HitTest(iPosX, iPosY);
}

bool CGUITextBox::OnMouseOver()
{
  if (m_upDown.HitTest(g_Mouse.iPosX, g_Mouse.iPosY))
    m_upDown.OnMouseOver();
  return CGUIControl::OnMouseOver();
}

bool CGUITextBox::OnMouseClick(DWORD dwButton)
{
  if (m_upDown.HitTest(g_Mouse.iPosX, g_Mouse.iPosY))
    return m_upDown.OnMouseClick(dwButton);
  return false;
}

bool CGUITextBox::OnMouseWheel()
{
  if (m_upDown.HitTest(g_Mouse.iPosX, g_Mouse.iPosY))
  {
    return m_upDown.OnMouseWheel();
  }
  else
  { // increase or decrease our offset by the appropriate amount.
    m_iOffset -= g_Mouse.cWheel;
    // check that we are within the correct bounds.
    if (m_iOffset + m_iItemsPerPage > (int)m_vecItems.size())
      m_iOffset = m_vecItems.size() - m_iItemsPerPage;
    if (m_iOffset < 0) m_iOffset = 0;
    // update the page control...
    int iPage = m_iOffset / m_iItemsPerPage + 1;
    // last page??
    if (m_iOffset + m_iItemsPerPage == (int)m_vecItems.size())
      iPage = m_upDown.GetMaximum();
    m_upDown.SetValue(iPage);
  }
  return true;
}

void CGUITextBox::SetPosition(int iPosX, int iPosY)
{
  // offset our spin control by the appropriate amount
  int iSpinOffsetX = m_upDown.GetXPosition() - GetXPosition();
  int iSpinOffsetY = m_upDown.GetYPosition() - GetYPosition();
  CGUIControl::SetPosition(iPosX, iPosY);
  m_upDown.SetPosition(GetXPosition() + iSpinOffsetX, GetYPosition() + iSpinOffsetY);
}

void CGUITextBox::SetWidth(int iWidth)
{
  int iSpinOffsetX = m_upDown.GetXPosition() - GetXPosition() - GetWidth();
  CGUIControl::SetWidth(iWidth);
  m_upDown.SetPosition(GetXPosition() + GetWidth() + iSpinOffsetX, m_upDown.GetYPosition());
}

void CGUITextBox::SetHeight(int iHeight)
{
  int iSpinOffsetY = m_upDown.GetYPosition() - GetYPosition() - GetHeight();
  CGUIControl::SetHeight(iHeight);
  m_upDown.SetPosition(m_upDown.GetXPosition(), GetYPosition() + GetHeight() + iSpinOffsetY);
}

void CGUITextBox::SetPulseOnSelect(bool pulse)
{
  m_upDown.SetPulseOnSelect(pulse);
  CGUIControl::SetPulseOnSelect(pulse);
}

void CGUITextBox::SetNavigation(DWORD up, DWORD down, DWORD left, DWORD right)
{
  CGUIControl::SetNavigation(up, down, left, right);
  m_upDown.SetNavigation(up, down, left, right);
}