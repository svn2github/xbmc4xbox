#include "factorydirectory.h"
#include "HDDirectory.h"
#include "ISO9660Directory.h"
#include "XNSDirectory.h"
#include "SMBDirectory.h"
#include "XBMSDirectory.h"
#include "CDDADirectory.h"
#include "RTVDirectory.h"
#include "../url.h"

CFactoryDirectory::CFactoryDirectory(void)
{
}

CFactoryDirectory::~CFactoryDirectory(void)
{
}

/*!
	\brief Create a CDirectory object of the share type specified in \e strPath .
	\param strPath Specifies the share type to access, can be a share or share with path.
	\return CDirectory object to access the directories on the share.
	\sa CDirectory
	*/
CDirectory* CFactoryDirectory::Create(const CStdString& strPath)
{
	CURL url(strPath);
	CStdString strProtocol=url.GetProtocol();
	if (strProtocol=="iso9660")
	{
		return (CDirectory*)new CISO9660Directory();
	}
	
	if (strProtocol=="xns")
	{
		return (CDirectory*)new CXNSDirectory();
	}

	if (strProtocol=="smb")
	{
		return (CDirectory*)new CSMBDirectory();
	}

	if (strProtocol=="xbms")
	{
		return (CDirectory*)new CXBMSDirectory();
	}
	if (strProtocol=="cdda")
	{
		return (CDirectory*)new CCDDADirectory();
	}
	if (strProtocol=="rtv")
	{
		return (CDirectory*)new CRTVDirectory();
	}

  return (CDirectory*)new CHDDirectory();
}
