/*
 * DAAP Support for XBox Media Center
 * Copyright (c) 2004 Forza (Chris Barnett)
 * Portions Copyright (c) by the authors of libOpenDAAP
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
*/

#include "stdafx.h"
#include "DAAPDirectory.h"
#include "../url.h"
#include "../util.h"
#include "../sectionloader.h"
#include "directorycache.h"
// using the smb library for UTF-8 conversion
#include "../lib/libsmb/xbLibSmb.h"
#include "../application.h"

CDAAPDirectory::CDAAPDirectory(void)
{
	CSectionLoader::Load("LIBXDAAP");

	// m_currLevel holds where we are in the playlist/artist/album/songs hierarchy (0,1,2,3)
	m_currLevel = -1;
	m_thisHost = NULL;
	m_thisClient = NULL;
	m_artisthead = NULL;
	m_currentSongItems = NULL;
	m_currentSongItemCount = 0;
}

CDAAPDirectory::~CDAAPDirectory(void)
{
	//if (m_thisClient) DAAP_Client_Release(m_thisClient);

	free_artists();

	m_thisHost = NULL;
	m_thisClient = NULL;
	m_artisthead = NULL;

	m_currentSongItems = NULL;
	CSectionLoader::Unload("LIBXDAAP");
}

void  CDAAPDirectory::CloseDAAP(void)
{
	if (g_application.m_DAAPPtr)
	{
		CSectionLoader::Load("LIBXDAAP");
		m_thisClient = (DAAP_SClient *) g_application.m_DAAPPtr;
		DAAP_Client_Release(m_thisClient);
		m_thisClient = NULL;
		CSectionLoader::Unload("LIBXDAAP");
	}
	g_application.m_DAAPPtr = NULL;
}

bool  CDAAPDirectory::GetDirectory(const CStdString& strPath, VECFILEITEMS &items)
{
	int c;
	wchar_t wStrFile[1024]; // buffer for converting strings

	CURL url(strPath);
  
	CStdString strRoot=strPath;
	if (!CUtil::HasSlashAtEnd(strPath)) strRoot+="/";

	// Clear out any cached entries for this path
	g_directoryCache.ClearDirectory(strPath);

	// if we are accessing a different host then close any current host connection
	if (strcmp(g_application.m_CurrDAAPHost, (char *) url.GetHostName().c_str()) != 0)
	{
		CloseDAAP();
	}

	if (g_application.m_DAAPPtr)
	{
		m_thisClient = (DAAP_SClient *) g_application.m_DAAPPtr;
		m_thisHost = m_thisClient->hosts;
	}
	else
	{
		// Create a client object if we don't already have one
		if (!m_thisClient) m_thisClient = DAAP_Client_Create(NULL, NULL);
		g_application.m_DAAPPtr = m_thisClient;
	}

	// Add the defined host to the client object if we don't already have one
	if (!m_thisHost)
	{
		m_thisHost = DAAP_Client_AddHost(m_thisClient, (char *) url.GetHostName().c_str(), "A", "A");

		// If no host object returned then the connection failed
		if (!m_thisHost)
		{
			CloseDAAP();
			return false;
		}

		if (DAAP_ClientHost_Connect(m_thisHost) < 0)
		{
			CloseDAAP();
			return false;
		}

		DAAP_Client_GetDatabases(m_thisHost);
		g_application.m_DAAPDBID =  m_thisHost->databases[0].id;
		strcpy(g_application.m_CurrDAAPHost, m_thisHost->host);
	}

	// if we have at least one database we should show it's contents
	if (m_thisHost->nDatabases)
	{
		// Get the songs from the database if we haven't already
		if (!m_artisthead)
		{
			m_currentSongItems = m_thisHost->dbitems[0].items;
			m_currentSongItemCount = m_thisHost->dbitems[0].nItems;

			// Add each artist and album to an array
			int c;
			for (c = 0; c < m_currentSongItemCount; c++)
			{
				AddToArtistAlbum(m_currentSongItems[c].songartist, m_currentSongItems[c].songalbum);
			}
		}

		// find out where we are in the folder hierarchy
		m_currLevel = GetCurrLevel(strRoot);

		if (m_currLevel < 0)	// root, so show playlists
		{
			for (c = 0; c < m_thisHost->dbplaylists->nPlaylists; c++)
			{
				CStdString strThisRoot = m_thisHost->dbplaylists->playlists[c].itemname;
				CStdString strFile;
				size_t strLen;

				// convert from UTF8 to wide string
				strLen = convert_string(CH_UTF8, CH_UCS2, strThisRoot, strThisRoot.length(), wStrFile, 1024, false);
				wStrFile[(strLen/2)] = 0;
				CUtil::Unicode2Ansi(wStrFile, strFile);

				// Add item to directory list
				CFileItem* pItem = new CFileItem(strFile);
				pItem->m_strPath = strRoot + m_thisHost->dbplaylists->playlists[c].itemname;
				pItem->m_bIsFolder = true;
				items.push_back(new CFileItem(*pItem));
			}
		}
		else if (m_currLevel == 0)	// playlists, so show albums
		{
			CStdString strFolder;
			bool bFoundMatch;

			// find the playlist
			bFoundMatch = false;
			for (c = 0; c < m_thisHost->dbplaylists->nPlaylists; c++)
			{
				if (m_thisHost->dbplaylists->playlists[c].itemname == m_selectedPlaylist)
				{
					bFoundMatch = true;
					break;
				}
			}

			// if we found it show the songs contained ...
			if (bFoundMatch)
			{
				
				// if selected playlist name == d/b name then show in artist/album/song formation
				if (strcmp(m_thisHost->databases[0].name, m_thisHost->dbplaylists->playlists[c].itemname) == 0)
				{
					artistPTR *cur = m_artisthead;
					while (cur)
					{
						CFileItem* pItem = new CFileItem(cur->artist);
						pItem->m_strPath = strRoot + cur->artist;
						pItem->m_bIsFolder = true;
						items.push_back(new CFileItem(*pItem));
						cur = cur->next;
					}
				}
				else
				{
					// no support for playlist items in libOpenDAAP yet? - there is now :)

					int j;
					for (j = 0; j < m_thisHost->dbplaylists->playlists[c].count; j++)
					{
						// the playlist id is the song id, these do not directly match the array
						// position so we've no choice but to cycle through all the songs until
						// we find the one we want.
						int i, idx;
						idx = -1;
						for (i = 0; i < m_currentSongItemCount; i ++)
						{
							if (m_currentSongItems[i].id == m_thisHost->dbplaylists->playlists[c].items[j].id)
							{
								idx = i;
								break;
							}
						}
						
						if (idx > -1)
						{
							CFileItem* pItem = new CFileItem(m_currentSongItems[idx].itemname);
							pItem->m_strPath.Format("daap://%s/%d.%s", m_thisHost->host, m_currentSongItems[idx].id,
								m_currentSongItems[idx].songformat);
							pItem->m_bIsFolder = false;
							pItem->m_dwSize = m_currentSongItems[idx].songsize;

							pItem->m_musicInfoTag.SetURL(pItem->m_strPath);
							pItem->m_musicInfoTag.SetTitle(m_currentSongItems[idx].itemname);
							pItem->m_musicInfoTag.SetArtist(m_currentSongItems[idx].songartist);
							pItem->m_musicInfoTag.SetAlbum(m_currentSongItems[idx].songalbum);
							//pItem->m_musicInfoTag.SetTrackNumber(m_currentSongItems[idx].songtracknumber);
							pItem->m_musicInfoTag.SetTrackNumber(m_thisHost->dbplaylists->playlists[c].items[j].songid);
							//pItem->m_musicInfoTag.SetTrackNumber(j+1);
							pItem->m_musicInfoTag.SetDuration((int) (m_currentSongItems[idx].songtime / 1000));
							pItem->m_musicInfoTag.SetLoaded(true);
							
							items.push_back(new CFileItem(*pItem));
						}
					}
				}
			}
		}
		else if (m_currLevel == 1)	// artists, so show albums
		{
			// Find the artist ...
			artistPTR *cur = m_artisthead;
			while (cur)
			{
				if (cur->artist == m_selectedArtist) break;
				cur = cur->next;
			}

			// if we find it, then show albums for this artist
			if (cur)
			{
				albumPTR *curAlbum = cur->albumhead;
				while (curAlbum)
				{
					CFileItem* pItem = new CFileItem(curAlbum->album);
					pItem->m_strPath = strRoot + curAlbum->album;
					pItem->m_bIsFolder = true;
					items.push_back(new CFileItem(*pItem));
					curAlbum = curAlbum->next;
				}
			}
		}
		else if (m_currLevel == 2)	// albums, so show songs
		{
			int c;
			for (c = 0; c < m_currentSongItemCount; c++)
			{
				// if this song is for the current artist & album add it to the file list
				if (m_currentSongItems[c].songartist == m_selectedArtist &&
					m_currentSongItems[c].songalbum == m_selectedAlbum)
				{
					CFileItem* pItem = new CFileItem(m_currentSongItems[c].itemname);
					pItem->m_strPath.Format("daap://%s/%d.%s", m_thisHost->host, m_currentSongItems[c].id,
						m_currentSongItems[c].songformat);
					pItem->m_bIsFolder = false;
					pItem->m_dwSize = m_currentSongItems[c].songsize;

					pItem->m_musicInfoTag.SetURL(pItem->m_strPath);
					pItem->m_musicInfoTag.SetTitle(m_currentSongItems[c].itemname);
					pItem->m_musicInfoTag.SetArtist(m_selectedArtist);
					pItem->m_musicInfoTag.SetAlbum(m_selectedAlbum);
					pItem->m_musicInfoTag.SetTrackNumber(m_currentSongItems[c].songtracknumber);
					pItem->m_musicInfoTag.SetDuration((int) (m_currentSongItems[c].songtime / 1000));
					pItem->m_musicInfoTag.SetLoaded(true);
					
					items.push_back(new CFileItem(*pItem));
				}
			}
		}
	}
	return true;
}

void CDAAPDirectory::free_albums(albumPTR *alb)
{
    albumPTR *cur = alb;
    while (cur)
    {
        albumPTR *next = cur->next;
        if (cur->album) free(cur->album);
        free(cur);
        cur = next;
    }
}

void CDAAPDirectory::free_artists()
{
    artistPTR *cur = m_artisthead;
    while (cur)
    {
        artistPTR *next = cur->next;
        if (cur->artist) free(cur->artist);
        if (cur->albumhead) free_albums(cur->albumhead);
        free(cur);
        cur = next;
    }
    m_artisthead = NULL;
}

void CDAAPDirectory::AddToArtistAlbum(char *artist_s, char *album_s)
{
    artistPTR *cur_artist = m_artisthead;
    albumPTR *cur_album = NULL;
    while (cur_artist)
    {
		if (strcmp(cur_artist->artist, artist_s) == 0)
            break;
        cur_artist = cur_artist->next;
    }
    if (!cur_artist)
    {
        artistPTR *newartist = (artistPTR *) malloc(sizeof(artistPTR));

        newartist->artist = (char *) malloc(strlen(artist_s) +1);
        strcpy(newartist->artist, artist_s);

        newartist->albumhead = NULL;

        newartist->next = m_artisthead;
        m_artisthead = newartist;

        cur_artist = newartist;
    }
    cur_album = cur_artist->albumhead;
    while (cur_album)
    {
        if (strcmp(cur_album->album, album_s) == 0)
            break;
        cur_album = cur_album->next;
    }
    if (!cur_album)
    {
        albumPTR *newalbum = (albumPTR *) malloc(sizeof(albumPTR));

        newalbum->album = (char *) malloc(strlen(album_s) + 1);
        strcpy(newalbum->album, album_s);

        newalbum->next = cur_artist->albumhead;
        cur_artist->albumhead = newalbum;

        cur_album = newalbum;
    }
}

int CDAAPDirectory::GetCurrLevel(CStdString strPath)
{
	int intSPos;
	int intEPos;
	int intLevel;
	int intCnt;
	CStdString strJustPath;

	intSPos = strPath.Find("://");
	if (intSPos > -1)
		strJustPath = strPath.Right(strPath.size()-(intSPos+3));
	else
		strJustPath = strPath;

	if (CUtil::HasSlashAtEnd(strJustPath))
		strJustPath = strJustPath.Left(strJustPath.size()-1);

	intLevel = -1;
	intSPos = strPath.length();
	while (intSPos > -1)
	{
		intSPos = strJustPath.ReverseFind("/", intSPos);
		if (intSPos > -1) intLevel ++;
		intSPos -= 2;
	}

	m_selectedPlaylist = "";
	m_selectedArtist = "";
	m_selectedAlbum = "";
	intCnt = intLevel;
	intEPos = (strJustPath.length() - 1);
	while(intCnt >= 0)
	{
		intSPos = strJustPath.ReverseFind("/", intEPos);
		if (intSPos > -1)
		{
			if (intCnt == 2)		// album
			{
				m_selectedAlbum = strJustPath.substr(intSPos + 1, (intEPos-intSPos));
			}
			else if (intCnt == 1)	// artist
			{
				m_selectedArtist = strJustPath.substr(intSPos + 1, (intEPos-intSPos));
			}
			else if (intCnt == 0)	// playlist
			{
				m_selectedPlaylist = strJustPath.substr(intSPos + 1, (intEPos-intSPos));
			}

			intEPos = (intSPos - 1);
			intCnt --;
		}
	}

	return intLevel;
}