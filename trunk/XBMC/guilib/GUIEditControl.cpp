#include "include.h"
#include "GUIEditControl.h"
#include "../xbmc/util.h"


CGUIEditControl::CGUIEditControl(DWORD dwParentID, DWORD dwControlId,
                                 float posX, float posY, float width, float height,
                                 const CLabelInfo& labelInfo, const string& strLabel)
    : CGUILabelControl(dwParentID, dwControlId, posX, posY, width, height, strLabel, labelInfo, false)
{
  ControlType = GUICONTROL_EDIT;
  m_pObserver = NULL;
  m_originalPosX = posX;
  ShowCursor(true);
}

CGUIEditControl::~CGUIEditControl(void)
{}

void CGUIEditControl::SetObserver(IEditControlObserver* aObserver)
{
  m_pObserver = aObserver;
}

void CGUIEditControl::OnKeyPress(WORD wKeyId)
{
  if (wKeyId >= KEY_VKEY && wKeyId < KEY_ASCII)
  {
    // input from the keyboard (vkey, not ascii)
    BYTE b = wKeyId & 0xFF;
    if (b == 0x25 && m_iCursorPos > 0)
    {
      // left
      m_iCursorPos--;
    }
    if (b == 0x27 && m_iCursorPos < (int)m_strLabel.length())
    {
      // right
      m_iCursorPos++;
    }
  }
  else if (wKeyId >= KEY_ASCII)
  {
    // input from the keyboard
    char ch = wKeyId & 0xFF;
    switch (ch)
    {
    case 27:
      { // escape
        m_strLabel.clear();
        m_iCursorPos = 0;
        break;
      }
    case 10:
      {
        // enter
        if (m_pObserver)
        {
          CStdString strLineOfText = m_strLabel;
          m_strLabel.clear();
          m_iCursorPos = 0;
          m_pObserver->OnEditTextComplete(strLineOfText);
        }
        break;
      }
    case 8:
      {
        // backspace or delete??
        if (m_iCursorPos > 0)
        {
          m_strLabel.erase(m_iCursorPos - 1, 1);
          m_iCursorPos--;
        }
        break;
      }
    default:
      {
        // use character input
        m_strLabel.insert( m_strLabel.begin() + m_iCursorPos, ch);
        m_iCursorPos++;
        break;
      }
    }
  }

  RecalcLabelPosition();
}

void CGUIEditControl::RecalcLabelPosition()
{
  float maxWidth = m_width - 8;

  FLOAT fTextWidth, fTextHeight;

  CStdStringW strTempLabel = m_strLabel;
  CStdStringW strTempPart = strTempLabel.Mid(0, m_iCursorPos);

  m_label.font->GetTextExtent( strTempPart.c_str(), &fTextWidth, &fTextHeight );

  // if skinner forgot to set height :p
  if (m_height == 0)
  {
    // store font height
    m_height = fTextHeight;
  }

  // if text accumulated is greater than width allowed
  if (fTextWidth > maxWidth)
  {
    // move the position of the label to the left (outside of the viewport)
    m_posX = (m_originalPosX + maxWidth) - fTextWidth;
  }
  else
  {
    // otherwise use original position
    m_posX = m_originalPosX;
  }
}

void CGUIEditControl::Render()
{
  if (!IsVisible()) return;

  // we can only perform view port operations if we have an area to display
  if (m_height > 0 && m_width > 0)
  {
    if (g_graphicsContext.SetViewPort(m_originalPosX, m_posY, m_width, m_height))
    {
      CGUILabelControl::Render();
      g_graphicsContext.RestoreViewPort();
    }
  }
  else
  {
    // use default rendering until we have recalculated label position
    CGUILabelControl::Render();
  }
}
