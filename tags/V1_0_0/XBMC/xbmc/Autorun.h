//  CAutorun		 -  Autorun for different Cd Media
//									like DVD Movies or XBOX Games
//
//	by Bobbin007 in 2003
//	
//	
//

#pragma once
#include "filesystem/factoryDirectory.h"

#include <memory>
using namespace std;

namespace MEDIA_DETECT 
{
	class CAutorun
	{
	public:
		CAutorun();
		virtual			~CAutorun();
		void				HandleAutorun();
		void				Enable();
		void				Disable();
		bool				IsEnabled();
	protected:
		void				ExecuteAutorun();
		void				RunXboxCd();
		void				RunCdda();
		void				RunISOMedia();
		bool				RunDisc(CDirectory* pDir, const CStdString& strDrive, int& nAddedToPlaylist, bool bRoot);

		bool				m_bEnable;
	};
}