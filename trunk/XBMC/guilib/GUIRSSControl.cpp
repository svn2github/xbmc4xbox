#include "stdafx.h"
#include "guiRSSControl.h"
#include "guifontmanager.h"
#include "guiWindowManager.h"
#include "..\xbmc\Application.h"

extern CApplication g_application;

CGUIRSSControl::CGUIRSSControl(DWORD dwParentID, DWORD dwControlId, DWORD dwPosX, DWORD dwPosY, DWORD dwWidth, DWORD dwHeight, const CStdString& strFontName, D3DCOLOR dwChannelColor, D3DCOLOR dwHeadlineColor, D3DCOLOR dwNormalColor, CStdString& strUrl)
:CGUIControl(dwParentID, dwControlId, dwPosX, dwPosY,dwWidth, dwHeight)
{
	m_strUrl			= strUrl;
	m_dwChannelColor	= dwChannelColor;
	m_dwHeadlineColor	= dwHeadlineColor; 
	m_dwTextColor		= dwNormalColor; 
	m_pFont				= g_fontManager.GetFont(strFontName);

	WCHAR wTmp[2];
	wTmp[0]=L' ';
	wTmp[1]=0;
    float fWidth,fHeight;
	m_pFont->GetTextExtent(wTmp,&fWidth,&fHeight);
	m_iLeadingSpaces	= (int) (dwWidth / fWidth);
	m_iCharIndex		= 0;
	m_iStartFrame		= 0;
	m_iScrollX			= 1;
	m_pdwPalette		= new DWORD[3];
	m_pdwPalette[0]		= dwNormalColor;
	m_pdwPalette[1]		= dwHeadlineColor;
	m_pdwPalette[2]		= dwChannelColor;

	m_pReader	= NULL;
	m_pwzText	= NULL;
	m_pwzBuffer = NULL;
	m_pbBuffer	= NULL;
	m_pbColors	= NULL;
	ControlType = GUICONTROL_RSS;
}

CGUIRSSControl::~CGUIRSSControl(void)
{
}

void CGUIRSSControl::Render()
{
	if ( (!IsVisible()) || g_application.IsPlayingVideo() ) 
	{
		return;
	}

	if (m_pReader==NULL)
	{
		// Create RSS background/worker thread
		m_pReader = new CRssReader();
		m_pReader->Create(this,m_strUrl,m_iLeadingSpaces);
	}

	if (m_pFont && m_pwzText)
	{
		RenderText();
	}
}

void CGUIRSSControl::OnFeedUpdate(CStdString& aFeed, LPBYTE aColorArray)
{
	int nStringLength = aFeed.GetLength()+1;

	m_pwzText	= new WCHAR[nStringLength];
	m_pwzBuffer	= new WCHAR[nStringLength];
	swprintf(m_pwzText,L"%S", aFeed.c_str() );

	m_pbColors	= aColorArray;
	m_pbBuffer	= new BYTE[nStringLength];
}

void CGUIRSSControl::RenderText()
{
	float fPosX = (float) m_dwPosX;
	float fPosY = (float) m_dwPosY;
	float fMaxWidth = (float)m_dwWidth;

	float fTextHeight,fTextWidth;
	m_pFont->GetTextExtent( m_pwzText, &fTextWidth, &fTextHeight);

	float fPosCX=fPosX;
	float fPosCY=fPosY;
	g_graphicsContext.Correct(fPosCX, fPosCY);

	if (fPosCX <0)
	{
		fPosCX=0.0f;
	}

	if (fPosCY <0)
	{
		fPosCY=0.0f;
	}

	if (fPosCY > g_graphicsContext.GetHeight())
	{
		fPosCY=(float)g_graphicsContext.GetHeight();
	}

	float fHeight=60.0f;
	
	if (fHeight+fPosCY >= g_graphicsContext.GetHeight())
	{
		fHeight = g_graphicsContext.GetHeight() - fPosCY -1;
	}
	
	if (fHeight <= 0)
	{
		return ;
	}

	float fwidth = fMaxWidth - 5.0f;

	D3DVIEWPORT8 newviewport,oldviewport;
	g_graphicsContext.Get3DDevice()->GetViewport(&oldviewport);
	newviewport.X		= (DWORD)fPosCX;
	newviewport.Y		= (DWORD)fPosCY;
	newviewport.Width	= (DWORD)(fwidth);
	newviewport.Height	= (DWORD)(fHeight);
	newviewport.MinZ	= 0.0f;
	newviewport.MaxZ	= 1.0f;
	g_graphicsContext.Get3DDevice()->SetViewport(&newviewport);

	// if size of text is bigger than can be drawn onscreen
    if (fTextWidth > fMaxWidth)
    {
		fMaxWidth+=50.0f;

		// if start frame is greater than 25? (why?)
		if (m_iStartFrame > 25)
		{
			WCHAR wTmp[3];
			int iTextLenght = (int)wcslen(m_pwzText);

			// if scroll char index is bigger or equal to the text length
			if (m_iCharIndex >= iTextLenght )
			{
				// use a space character
				wTmp[0]=L' ';
			}
			else
			{
				// use the next character
				wTmp[0]=m_pwzText[m_iCharIndex];
			}

			// determine the width of the character
			wTmp[1]=0;
            float fWidth,fHeight;
			m_pFont->GetTextExtent(wTmp,&fWidth,&fHeight);
			// if the scroll offset is the same or as big as the next character
			if ( m_iScrollX >= fWidth)
			{
				// increase the character offset
				++m_iCharIndex;

				// if we've reached the end of the text
				if (m_iCharIndex > iTextLenght )
				{
					// start from the begning
					m_iCharIndex = 0;
				}

				// reset the scroll offset
				m_iScrollX=1;
			}
			else
			{
				// increase the scroll offset
				m_iScrollX++;
			}
					
			int ipos=0;
			// truncate the scroll text starting from the character index and wrap
			// the text.
			for (int i=0; i < iTextLenght; i++)
			{
				if (i+m_iCharIndex < iTextLenght)
				{
					m_pwzBuffer[i]=m_pwzText[i+m_iCharIndex];
					m_pbBuffer[i]=m_pbColors[i+m_iCharIndex];
				}
				else
				{
					if (ipos==0)
					{
						m_pwzBuffer[i]=L' ';
						m_pbBuffer[i]=0;
					}
					else
					{
						m_pwzBuffer[i]=m_pwzText[ipos-1];
						m_pbBuffer[i]=m_pbColors[ipos-1];
					}
					ipos++;
				}
				m_pwzBuffer[i+1]=0;
			}
			
			if (fPosY >=0.0)
			{
				m_pFont->DrawColourTextWidth(fPosX-m_iScrollX,fPosY,m_pdwPalette,m_pwzBuffer,m_pbBuffer,fMaxWidth);
			}					
		}
		else
		{
			m_iStartFrame++;
			if (fPosY >=0.0)
			{
				m_pFont->DrawColourTextWidth(fPosX,fPosY,m_pdwPalette,m_pwzText,m_pbColors,fMaxWidth);
			}
		}
	}
	g_graphicsContext.Get3DDevice()->SetViewport(&oldviewport);
}

