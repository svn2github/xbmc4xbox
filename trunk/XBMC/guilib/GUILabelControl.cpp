#include "include.h"
#include "GUILabelControl.h"
#include "GUIFontManager.h"
#include "../xbmc/utils/CharsetConverter.h"
#include "../xbmc/utils/GUIInfoManager.h"

CGUILabelControl::CGUILabelControl(DWORD dwParentID, DWORD dwControlId, int iPosX, int iPosY, DWORD dwWidth, DWORD dwHeight, const CStdString& strFont, const wstring& strLabel, DWORD dwTextColor, DWORD dwDisabledColor, DWORD dwTextAlign, bool bHasPath)
    : CGUIControl(dwParentID, dwControlId, iPosX, iPosY, dwWidth, dwHeight)
{
  SetLabel(strLabel);
  m_pFont = g_fontManager.GetFont(strFont);
  m_dwTextColor = dwTextColor;
  m_dwTextAlign = dwTextAlign;
  m_bHasPath = bHasPath;
  m_dwDisabledColor = dwDisabledColor;
  m_bShowCursor = false;
  m_iCursorPos = 0;
  m_dwCounter = 0;
  ControlType = GUICONTROL_LABEL;
  m_ScrollInsteadOfTruncate = false;
}

CGUILabelControl::~CGUILabelControl(void)
{}

void CGUILabelControl::ShowCursor(bool bShow)
{
  m_bShowCursor = bShow;
}

void CGUILabelControl::SetCursorPos(int iPos)
{
  if (iPos > (int)m_strLabel.length()) iPos = m_strLabel.length();
  if (iPos < 0) iPos = 0;
  m_iCursorPos = iPos;
}

void CGUILabelControl::SetInfo(const vector<int> &vecInfo)
{
  m_vecInfo = vecInfo;
  if (m_vecInfo.size() == 0)
    return; // no info at all
}

void CGUILabelControl::Render()
{
  if (!UpdateEffectState())
  {
    return;
  }

 
	WCHAR szLabel[1024];
	swprintf(szLabel, L"%s", m_strLabel.c_str() );
	CStdString strRenderLabel = szLabel;

	if (m_vecInfo.size())
	{ 
		strRenderLabel = g_infoManager.GetLabel(m_vecInfo[0]);
	}
	else
	{
		strRenderLabel = ParseLabel(strRenderLabel);
	}


  if (m_pFont)
  {
    CStdStringW strLabelUnicode;
    g_charsetConverter.stringCharsetToFontCharset(strRenderLabel, strLabelUnicode);

    // check for scrolling
    bool bNormalDraw = true;
    if (m_ScrollInsteadOfTruncate && m_dwWidth > 0 && !IsDisabled())
    { // ignore align center - just use align left/right
      float width, height;
      m_pFont->GetTextExtent(strLabelUnicode.c_str(), &width, &height);
      if (width > m_dwWidth)
      { // need to scroll - set the viewport.  Should be set just using the height of the text
        bNormalDraw = false;
        float fPosX = (float)m_iPosX;
        if (m_dwTextAlign & XBFONT_RIGHT)
          fPosX -= (float)m_dwWidth;

        m_pFont->DrawScrollingText(fPosX, (float)m_iPosY, &m_dwTextColor, 1, strLabelUnicode, (float)m_dwWidth, m_ScrollInfo);
      }
    }
    if (bNormalDraw)
    {
      float fPosX = (float)m_iPosX;
      if (m_dwTextAlign & XBFONT_CENTER_X)
        fPosX += (float)m_dwWidth / 2;

      float fPosY = (float)m_iPosY;
      if (m_dwTextAlign & XBFONT_CENTER_Y)
        fPosY += (float)m_dwHeight / 2;

      if (IsDisabled())
      {
        m_pFont->DrawText(fPosX, fPosY, m_dwDisabledColor, strLabelUnicode.c_str(), m_dwTextAlign | XBFONT_TRUNCATED, (float)m_dwWidth);
      }
      else
      {
        if (m_bShowCursor)
        { // show the cursor...
          strLabelUnicode.Insert(m_iCursorPos, L"|");
          if (m_dwTextAlign & XBFONT_CENTER_X)
            fPosX -= m_pFont->GetTextWidth(strLabelUnicode.c_str()) * 0.5f;
          BYTE *palette = new BYTE[strLabelUnicode.size()];
          for (unsigned int i=0; i < strLabelUnicode.size(); i++)
            palette[i] = 0;
          palette[m_iCursorPos] = 1;
          DWORD color[2];
          color[0] = m_dwDisabledColor;
          if ((++m_dwCounter % 50) > 25)
            color[1] = m_dwTextColor;
          else
            color[1] = 0; // transparent black
          m_pFont->DrawColourTextWidth(fPosX, fPosY, color, 2, strLabelUnicode.c_str(), palette, (float)m_dwWidth);
        }
        else
          m_pFont->DrawText(fPosX, fPosY, m_dwTextColor, strLabelUnicode.c_str(), m_dwTextAlign | XBFONT_TRUNCATED, (float)m_dwWidth);
      }
    }
  }
  CGUIControl::Render();
}


bool CGUILabelControl::CanFocus() const
{
  return false;
}

void CGUILabelControl::SetAlpha(DWORD dwAlpha)
{
  CGUIControl::SetAlpha(dwAlpha);
  m_dwTextColor = (dwAlpha << 24) | (m_dwTextColor & 0xFFFFFF);
  m_dwDisabledColor = (dwAlpha << 24) | (m_dwDisabledColor & 0xFFFFFF);
}

void CGUILabelControl::SetLabel(const wstring &strLabel)
{
  if (m_strLabel.compare(strLabel) == 0)
    return;
  m_strLabel = strLabel;
  SetWidthControl(m_ScrollInsteadOfTruncate);
}

void CGUILabelControl::SetWidthControl(bool bScroll)
{
  m_ScrollInsteadOfTruncate = bScroll;
  m_ScrollInfo.Reset();
}

void CGUILabelControl::SetText(CStdString aLabel)
{
  WCHAR wszText[1024];
  swprintf(wszText, L"%S", aLabel.c_str());
  SetLabel(wszText);
}

bool CGUILabelControl::OnMessage(CGUIMessage& message)
{
  if ( message.GetControlId() == GetID() )
  {
    if (message.GetMessage() == GUI_MSG_LABEL_SET)
    {
      SetLabel(message.GetLabel());

      if ( m_bHasPath )
      {
        ShortenPath();
      }
      return true;
    }
  }

  return CGUIControl::OnMessage(message);
}

void CGUILabelControl::ShortenPath()
{
  if (!m_pFont)
    return ;
  if ( m_dwWidth <= 0 )
    return ;
  if ( m_strLabel.size() <= 0 )
    return ;

  float fTextHeight, fTextWidth;
  char cDelim = '\0';
  int nGreaterDelim, nPos;

  nPos = m_strLabel.find_last_of( '\\' );
  if ( nPos >= 0 )
    cDelim = '\\';
  else
  {
    nPos = m_strLabel.find_last_of( '/' );
    if ( nPos >= 0 )
      cDelim = '/';
  }
  if ( cDelim == '\0' )
    return ;

  // remove trailing slashes
  if (nPos == m_strLabel.size() - 1)
  {
    m_strLabel.erase(m_strLabel.size() - 1);
    nPos = m_strLabel.find_last_of( cDelim );
  }

  CStdStringW strLabelUnicode;
  g_charsetConverter.stringCharsetToFontCharset(m_strLabel, strLabelUnicode);

  m_pFont->GetTextExtent( strLabelUnicode.c_str(), &fTextWidth, &fTextHeight);

  if ( fTextWidth <= (m_dwWidth) )
    return ;

  while ( fTextWidth > m_dwWidth )
  {
    nPos = m_strLabel.find_last_of( cDelim, nPos );
    nGreaterDelim = nPos;
    if ( nPos >= 0 )
      nPos = m_strLabel.find_last_of( cDelim, nPos - 1 );
    else
      break;

    if ( nPos < 0 )
      break;

    if ( nGreaterDelim > nPos )
    {
      m_strLabel.replace( nPos + 1, nGreaterDelim - nPos - 1, L"..." );
      g_charsetConverter.stringCharsetToFontCharset(m_strLabel, strLabelUnicode);
    }

    m_pFont->GetTextExtent( strLabelUnicode.c_str(), &fTextWidth, &fTextHeight );
  }
}
