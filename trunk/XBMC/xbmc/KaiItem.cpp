
#include "stdafx.h"
#include "Settings.h"
#include "KaiItem.h"
#include "Utils/KaiClient.h"
#include "Utils/Http.h"
#include "Util.h"
#include "Crc32.h"
#include "Picture.h"

CKaiItem::CKaiItem(CStdString& strLabel) : CGUIListExItem(strLabel)
{
	m_pAvatar			= NULL;

	if (IsAvatarCached())
	{
		UseCachedAvatar();
	}
}

CKaiItem::~CKaiItem(void)
{
	if (m_pAvatar)
	{
		OutputDebugString("End of life for this Kai's avatar\r\n");
		m_pAvatar->FreeResources();
		delete m_pAvatar;
		m_pAvatar = NULL;
		OutputDebugString("Bye!!\r\n");
	}
}

void CKaiItem::SetAvatar(CStdString& aAvatarUrl)
{
//	OutputDebugString("Setting Kai avatar\r\n");

	// Get file extension from url
	CStdString strExtension;
	CUtil::GetExtension(aAvatarUrl,strExtension);	
	if (!strExtension.IsEmpty())
	{
		if (IsAvatarCached())
		{
			CStdString strAvatarFilePath;
			GetAvatarFilePath(strAvatarFilePath);

			DWORD dwAvatarWidth	 = 50;
			DWORD dwAvatarHeight = 50;
			m_pAvatar = new CGUIImage(0,0,0,0,dwAvatarWidth,dwAvatarHeight,strAvatarFilePath);
		}
		else
		{
			g_DownloadManager.RequestFile(aAvatarUrl,this);
		}
	}
}

void CKaiItem::UseCachedAvatar()
{
	CStdString strAvatarFilePath;
	GetAvatarFilePath(strAvatarFilePath);

	DWORD dwAvatarWidth	 = 50;
	DWORD dwAvatarHeight = 50;

	m_pAvatar = new CGUIImage(0,0,0,0,dwAvatarWidth,dwAvatarHeight,strAvatarFilePath);
}

bool CKaiItem::IsAvatarCached()
{
	// Compute cached filename
	CStdString strFilePath;
	GetAvatarFilePath(strFilePath);
	    
	// Check to see if the file is already cached
	return CUtil::FileExists(strFilePath);
}

void CKaiItem::GetAvatarFilePath(CStdString& aFilePath)
{
	// Generate a unique identifier from player name    
	Crc32 crc;
	crc.Compute(m_strName.c_str(),m_strName.length());
	aFilePath.Format("Q:\\thumbs\\kai\\avatar-%x.jpg",(DWORD)crc);
}

void CKaiItem::OnFileComplete(TICKET aTicket, CStdString& aFilePath, INT aByteRxCount, Result aResult)
{
	if (aResult == IDownloadQueueObserver::Succeeded && aByteRxCount>=100) 
	{
		//OutputDebugString("Downloaded avatar.\r\n");

		CStdString strAvatarFilePath;
		GetAvatarFilePath(strAvatarFilePath);

		try
		{
			CPicture picture;
			picture.Convert(aFilePath,strAvatarFilePath);

			DWORD dwAvatarWidth	 = 50;
			DWORD dwAvatarHeight = 50;
			m_pAvatar = new CGUIImage(0,0,0,0,dwAvatarWidth,dwAvatarHeight,strAvatarFilePath);
		}
		catch(...)
		{
			::DeleteFile(strAvatarFilePath.c_str());
		}
	}
	else
	{
		//OutputDebugString("Unable to download avatar.\r\n");
	}
}

