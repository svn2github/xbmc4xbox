#pragma once

#include <xtl.h>
#include "xbox/undocumented.h"
#include "settings.h"

class XBVideoConfig
{
public:
	XBVideoConfig();
	~XBVideoConfig();

	bool HasPAL() const;
	bool HasPAL60() const;
	bool HasWidescreen() const;
	bool Has480p() const;
	bool Has720p() const;
	bool Has1080i() const;
	void GetModes(LPDIRECT3D8 pD3D);
	RESOLUTION GetSafeMode() const;

private:
	bool bHasPAL;
	bool bHasNTSC;
	DWORD m_dwVideoFlags;
};

extern XBVideoConfig g_videoConfig;