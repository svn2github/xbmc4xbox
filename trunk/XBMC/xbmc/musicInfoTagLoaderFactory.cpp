#pragma once

#include "musicInfoTagLoaderFactory.h"
#include "MusicInfoTagLoaderMP3.h"
#include "MusicInfoTagLoaderOgg.h"
#include "MusicInfoTagLoaderWMA.h"
#include "util.h"

using namespace MUSIC_INFO;
CMusicInfoTagLoaderFactory::CMusicInfoTagLoaderFactory()
{
}

CMusicInfoTagLoaderFactory::~CMusicInfoTagLoaderFactory()
{
}

IMusicInfoTagLoader* CMusicInfoTagLoaderFactory::CreateLoader(const CStdString& strFileName)
{
	CStdString strExtension;
	CUtil::GetExtension( strFileName, strExtension);
	strExtension.ToLower();
	if (strExtension==".mp3")
	{
		CMusicInfoTagLoaderMP3 *pTagLoader= new CMusicInfoTagLoaderMP3();
		return (IMusicInfoTagLoader*)pTagLoader;
	}
	if (strExtension==".ogg")
	{
		CMusicInfoTagLoaderOgg *pTagLoader= new CMusicInfoTagLoaderOgg();
		return (IMusicInfoTagLoader*)pTagLoader;
	}
	if (strExtension==".wma")
	{
		CMusicInfoTagLoaderWMA *pTagLoader= new CMusicInfoTagLoaderWMA();
		return (IMusicInfoTagLoader*)pTagLoader;
	}
	return NULL;
}
