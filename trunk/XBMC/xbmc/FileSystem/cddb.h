

#include "stdstring.h"
#include "../util.h"
#include <map>
#include <sstream>
#include <iostream>
#include <strstream>
#include "../DetectDVDType.h"

namespace CDDB
{

	#define	IN_PROGRESS											-1
	#define	OK							 								0
	#define	E_FAILED					 							1
	#define	E_TOC_INCORRECT				 	  			2
	#define	E_NETWORK_ERROR_OPEN_SOCKET	 	  3
	#define	E_NETWORK_ERROR_SEND		 				4
	#define	E_WAIT_FOR_INPUT				  			5
	#define	E_PARAMETER_WRONG				  			6
	#define	QUERRY_OK					 							7

	#define	E_NO_MATCH_FOUND							 202
	#define	E_INEXACT_MATCH_FOUND					 211
			
	#define	W_CDDB_already_shook_hands      402
	#define	E_CDDB_Handshake_not_successful	431
	#define	E_CDDB_permission_denied				432
	#define	E_CDDB_max_users_reached				433
	#define	E_CDDB_system_load_too_high	    434

	#define CDDB_PORT 8880;

	using namespace std;


	struct toc {
		int	min;
		int	sec;
		int	frame;
	};


	class Xcddb
	{
		public:
			Xcddb();
			virtual ~Xcddb();
			void							setCDDBIpAdress(const CStdString& ip_adress);
			void 							setCacheDir(const CStdString&  pCacheDir );

			int								queryCDinfo(int real_track_count, toc cdtoc[]);
			int 							queryCDinfo(int inexact_list_select);
			int								queryCDinfo(CCdInfo* pInfo);
			int 							getLastError() const;
			const char *			getLastErrorText() const;
			const CStdString&	getYear() const;
			const CStdString&	getGenre() const;
			const CStdString& getTrackArtist(int track) const;
			const CStdString& getTrackTitle(int track) const;
			void 							getDiskArtist(CStdString& strdisk_artist) const;
			void 							getDiskTitle(CStdString& strdisk_title) const;
			const CStdString& getTrackExtended(int track)  const;
			unsigned long			calc_disc_id(int nr_of_tracks, toc cdtoc[]);
			const CStdString& getInexactArtist(int select) const;
			const CStdString& getInexactTitle(int select) const;
			bool 							queryCache( unsigned long discid );
			bool 							writeCacheFile( const char* pBuffer, unsigned long discid );
			bool 							isCDCached( int nr_of_tracks, toc cdtoc[] );
			bool							isCDCached( CCdInfo* pInfo );

	protected:
		CStdString						m_strNull;
		SOCKET								m_cddb_socket;  
		const static int			recv_buffer=4096;
		int										m_lastError;
		map<int,CStdString> 	m_mapTitles;
		map<int,CStdString> 	m_mapArtists;
		map<int,CStdString> 	m_mapExtended_track;
		
		map<int,CStdString> 	m_mapInexact_cddb_command_list;
		map<int,CStdString> 	m_mapInexact_artist_list;
		map<int,CStdString> 	m_mapInexact_title_list;


		CStdString 						m_strDisk_artist;
		CStdString 						m_strDisk_title;
		CStdString 						m_strYear;
		CStdString 						m_strGenre;

		void 									addTitle(const char *buffer);
		void 									addExtended(const char *buffer);
		void 									parseData(const char *buffer);
		bool									Send( const void *buffer, int bytes );
		bool 									Send( const char *buffer);
		string								Recv(bool wait4point);
		bool									openSocket();
		bool 									closeSocket();
		struct toc						cdtoc[100];
		int										cddb_sum(int n);
		void									addInexactList(const char *list);
		void									addInexactListLine(int line_cnt, const char *line, int len);
		const CStdString&			getInexactCommand(int select) const;
		CStdString						cddb_ip_adress;
		CStdString    				cCacheDir;
	};
};