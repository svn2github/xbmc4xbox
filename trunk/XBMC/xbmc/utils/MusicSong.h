#pragma once
#include "stdstring.h"
namespace MUSIC_GRABBER
{
	class CMusicSong
	{
	public:
		CMusicSong(void);
		CMusicSong(int iTrack, const CStdString& strName, int iDuration);
		virtual ~CMusicSong(void);

		const CStdString& GetSongName() const;
		int								GetTrack() const;
		int								GetDuration() const;
		bool							Parse(const CStdString& strHTML);
	protected:
		int					m_iTrack;
		CStdString	m_strSongName;
		int					m_iDuration;
	};
};