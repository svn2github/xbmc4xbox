#include "PlayerCoreFactory.h"
#include "mplayer.h"
#include "cddaplayer.h"

CPlayerCoreFactory::CPlayerCoreFactory()
{
}
CPlayerCoreFactory::~CPlayerCoreFactory()
{
}

IPlayer* CPlayerCoreFactory::CreatePlayer(const CStdString& strCore,IPlayerCallback& callback) const
{
	CStdString strCoreLower=strCore;
	strCoreLower.ToLower();
	if (strCoreLower=="mplayer") return new CMPlayer(callback);
	if (strCoreLower=="cdda") return new CDDAPlayer(callback);
	//if (strCoreLower=="xine") return new CXinePlayer(callback);

	// default = mplayer
	return new CMPlayer(callback);
}


