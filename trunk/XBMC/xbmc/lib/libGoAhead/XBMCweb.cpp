#include "stdafx.h"
/*
 * XBMCweb.c -- Active Server Page Support
 *
 */

/******************************** Description *********************************/

/*
 *	This module provides the links between the web server and XBox Media Center
 */

/********************************* Includes ***********************************/

#pragma once

#include "xbmcweb.h"
#include "..\..\Application.h"
#include "..\..\util.h"
#include "..\..\playlistplayer.h"

#pragma code_seg("WEB_TEXT")
#pragma data_seg("WEB_DATA")
#pragma bss_seg("WEB_BSS")
#pragma const_seg("WEB_RD")

#define XML_MAX_INNERTEXT_SIZE 256
#define NO_EID -1

/***************************** Definitions ***************************/
/* Define common commands */

#define XBMC_NONE			T("none")
#define XBMC_ID				T("id")
#define XBMC_TYPE			T("type")

#define XBMC_REMOTE_MENU		T("menu")	
#define XBMC_REMOTE_BACK		T("back")
#define XBMC_REMOTE_SELECT	T("select")
#define XBMC_REMOTE_DISPLAY	T("display")
#define XBMC_REMOTE_TITLE		T("title")
#define XBMC_REMOTE_INFO		T("info")
#define XBMC_REMOTE_UP			T("up")
#define XBMC_REMOTE_DOWN		T("down")
#define XBMC_REMOTE_LEFT		T("left")
#define XBMC_REMOTE_RIGHT		T("right")
#define XBMC_REMOTE_PLAY		T("play")
#define XBMC_REMOTE_FORWARD	T("forward")
#define XBMC_REMOTE_REVERSE	T("reverse")
#define XBMC_REMOTE_PAUSE		T("pause")
#define XBMC_REMOTE_STOP		T("stop")
#define XBMC_REMOTE_SKIP_PLUS	T("skip+")
#define XBMC_REMOTE_SKIP_MINUS	T("skip-")
#define XBMC_REMOTE_1		T("1")
#define XBMC_REMOTE_2		T("2")
#define XBMC_REMOTE_3		T("3")
#define XBMC_REMOTE_4		T("4")
#define XBMC_REMOTE_5		T("5")
#define XBMC_REMOTE_6		T("6")
#define XBMC_REMOTE_7		T("7")
#define XBMC_REMOTE_8		T("8")
#define XBMC_REMOTE_9		T("9")
#define XBMC_REMOTE_0		T("0")

#define WEB_VIDEOS		T("videos")
#define WEB_MUSIC			T("music")
#define WEB_PICTURES	T("pictures")
#define WEB_PROGRAMS	T("programs")
#define WEB_FILES			T("files")
#define WEB_PLAYLIST	T("playlist")

#define XBMC_CAT_NAME			T("name")
#define XBMC_CAT_SELECT		T("select")
#define XBMC_CAT_QUE			T("que")
#define XBMC_CAT_UNQUE		T("unque")
#define XBMC_CAT_TYPE			T("type")
#define XBMC_CAT_PREVIOUS	T("previous")
#define XBMC_CAT_ITEMS		T("items")
#define XBMC_CAT_FIRST		T("first")
#define XBMC_CAT_NEXT			T("next")

#define XBMC_CMD_URL					T("/xbmcCmds/xbmcForm")
#define XBMC_CMD_DIRECTORY		T("directory")
#define XBMC_CMD_VIDEO				T("video")
#define XBMC_CMD_MUSIC				T("music")
#define XBMC_CMD_PICTURE			T("picture")
#define XBMC_CMD_APPLICATION	T("application")

struct SSortWebFilesByName
{
	bool operator()(CFileItem* pStart, CFileItem* pEnd)
	{
    CFileItem& rpStart=*pStart;
    CFileItem& rpEnd=*pEnd;
		if (rpStart.GetLabel()=="..") return true;
    if (rpEnd.GetLabel()=="..") return false;
		bool bGreater=true;
		if (g_stSettings.m_bMyFilesSortAscending) bGreater=false;
    if ( rpStart.m_bIsFolder   == rpEnd.m_bIsFolder)
		{
			char szfilename1[1024];
			char szfilename2[1024];

			switch ( g_stSettings.m_iMyFilesSortMethod ) 
			{
				case 0:	//	Sort by Filename
					strcpy(szfilename1, rpStart.GetLabel().c_str());
					strcpy(szfilename2, rpEnd.GetLabel().c_str());
					break;
				case 1: // Sort by Date
          if ( rpStart.m_stTime.wYear > rpEnd.m_stTime.wYear ) return bGreater;
					if ( rpStart.m_stTime.wYear < rpEnd.m_stTime.wYear ) return !bGreater;
					
					if ( rpStart.m_stTime.wMonth > rpEnd.m_stTime.wMonth ) return bGreater;
					if ( rpStart.m_stTime.wMonth < rpEnd.m_stTime.wMonth ) return !bGreater;
					
					if ( rpStart.m_stTime.wDay > rpEnd.m_stTime.wDay ) return bGreater;
					if ( rpStart.m_stTime.wDay < rpEnd.m_stTime.wDay ) return !bGreater;

					if ( rpStart.m_stTime.wHour > rpEnd.m_stTime.wHour ) return bGreater;
					if ( rpStart.m_stTime.wHour < rpEnd.m_stTime.wHour ) return !bGreater;

					if ( rpStart.m_stTime.wMinute > rpEnd.m_stTime.wMinute ) return bGreater;
					if ( rpStart.m_stTime.wMinute < rpEnd.m_stTime.wMinute ) return !bGreater;

					if ( rpStart.m_stTime.wSecond > rpEnd.m_stTime.wSecond ) return bGreater;
					if ( rpStart.m_stTime.wSecond < rpEnd.m_stTime.wSecond ) return !bGreater;
					return true;
				break;

        case 2:
          if ( rpStart.m_dwSize > rpEnd.m_dwSize) return bGreater;
					if ( rpStart.m_dwSize < rpEnd.m_dwSize) return !bGreater;
					return true;
        break;

				default:	//	Sort by Filename by default
          strcpy(szfilename1, rpStart.GetLabel().c_str());
					strcpy(szfilename2, rpEnd.GetLabel().c_str());
					break;
			}


			for (int i=0; i < (int)strlen(szfilename1); i++)
			{
				szfilename1[i]=tolower((unsigned char)szfilename1[i]);
			}

			for (int i=0; i < (int)strlen(szfilename2); i++)
			{
				szfilename2[i]=tolower((unsigned char)szfilename2[i]);
			}
			//return (rpStart.strPath.compare( rpEnd.strPath )<0);

			if (g_stSettings.m_bMyFilesSortAscending)
				return (strcmp(szfilename1,szfilename2)<0);
			else
				return (strcmp(szfilename1,szfilename2)>=0);
		}
    if (!rpStart.m_bIsFolder) return false;
		return true;
	}
};

CXbmcWeb::CXbmcWeb()
{
	navigatorState = 0;
	directory = NULL;
	strCurrentMediaFile = "";
}

CXbmcWeb::~CXbmcWeb()
{
	if (directory) delete directory;
}

char* CXbmcWeb::GetCurrentDir()
{
	return currentDir;
}

void CXbmcWeb::SetCurrentDir(const char* newDir)
{
	strcpy(currentDir, newDir);
}

int CXbmcWeb::GetCurrentItem()
{
	return currentItem;
}

void CXbmcWeb::SetCurrentItem(int item)
{
	currentItem = item;
}

DWORD CXbmcWeb::GetNavigatorState()
{
	return navigatorState;
}

void CXbmcWeb::SetNavigatorState(DWORD state)
{
	navigatorState = state;
}

void CXbmcWeb::AddItemToPlayList(const CFileItem* pItem)
{
	if (pItem->m_bIsFolder)
	{
		// recursive
		if (pItem->GetLabel() == "..") return;
		CStdString strDirectory=pItem->m_strPath;
		VECFILEITEMS items;
		directory->GetDirectory(strDirectory, items);
		for (int i=0; i < (int) items.size(); ++i)
		{
			AddItemToPlayList(items[i]);
			delete items[i];
		}
	}
	else
	{
		//selected item is a file, add it to playlist
		PLAYLIST::CPlayList::CPlayListItem playlistItem;
		playlistItem.SetFileName(pItem->m_strPath);
		playlistItem.SetDescription(pItem->GetLabel());
		playlistItem.SetDuration(pItem->m_musicInfoTag.GetDuration());

		switch(GetNavigatorState())
		{
			case WEB_NAV_VIDEOS:
				g_playlistPlayer.GetPlaylist(PLAYLIST_VIDEO).Add(playlistItem);
			break;
			
			case WEB_NAV_MUSIC:
				g_playlistPlayer.GetPlaylist(PLAYLIST_MUSIC).Add(playlistItem);
			break;
		}
	}
}

/************************************* Code ***********************************/

/* Handle rempte control requests */
int CXbmcWeb::xbmcRemoteControl( int eid, webs_t wp, char_t *parameter)
{
	/***********************************
	 *  Remote control command structure
	 *  This structure is formatted as follows:
	 *		command string, XBMC IR command
	 */

	xbmcRemoteControlHandlerType	xbmcRemoteControls[ ] = {
		XBMC_REMOTE_MENU,		XINPUT_IR_REMOTE_MENU,
		XBMC_REMOTE_BACK,		XINPUT_IR_REMOTE_BACK,
		XBMC_REMOTE_SELECT,		XINPUT_IR_REMOTE_SELECT,
		XBMC_REMOTE_DISPLAY,	XINPUT_IR_REMOTE_DISPLAY,
		XBMC_REMOTE_TITLE,		XINPUT_IR_REMOTE_TITLE,
		XBMC_REMOTE_INFO,		XINPUT_IR_REMOTE_INFO,
		XBMC_REMOTE_UP,			XINPUT_IR_REMOTE_UP,
		XBMC_REMOTE_DOWN,		XINPUT_IR_REMOTE_DOWN,
		XBMC_REMOTE_LEFT,		XINPUT_IR_REMOTE_LEFT,
		XBMC_REMOTE_RIGHT,		XINPUT_IR_REMOTE_RIGHT,
		XBMC_REMOTE_PLAY,		XINPUT_IR_REMOTE_PLAY,
		XBMC_REMOTE_FORWARD,	XINPUT_IR_REMOTE_FORWARD,
		XBMC_REMOTE_REVERSE,	XINPUT_IR_REMOTE_REVERSE,
		XBMC_REMOTE_PAUSE,		XINPUT_IR_REMOTE_PAUSE,
		XBMC_REMOTE_STOP,		XINPUT_IR_REMOTE_STOP,
		XBMC_REMOTE_SKIP_PLUS,	XINPUT_IR_REMOTE_SKIP_MINUS,
		XBMC_REMOTE_SKIP_MINUS, XINPUT_IR_REMOTE_SKIP_PLUS,
		XBMC_REMOTE_1,			XINPUT_IR_REMOTE_1,
		XBMC_REMOTE_2,			XINPUT_IR_REMOTE_2,
		XBMC_REMOTE_3,			XINPUT_IR_REMOTE_3,
		XBMC_REMOTE_4,			XINPUT_IR_REMOTE_4,
		XBMC_REMOTE_5,			XINPUT_IR_REMOTE_5,
		XBMC_REMOTE_6,			XINPUT_IR_REMOTE_6,
		XBMC_REMOTE_7,			XINPUT_IR_REMOTE_7,
		XBMC_REMOTE_8,			XINPUT_IR_REMOTE_8,
		XBMC_REMOTE_9,			XINPUT_IR_REMOTE_9,
		XBMC_REMOTE_0,			XINPUT_IR_REMOTE_0,
		"", -1};

	int cmd = 0;
	// look through the xbmcCommandStructure
	while( xbmcRemoteControls[ cmd].xbmcIRCode != -1) {
		// if we have a match
		if( stricmp( parameter,xbmcRemoteControls[ cmd].xbmcRemoteParameter) == 0) {
			g_application.m_DefaultIR_Remote.wButtons = xbmcRemoteControls[ cmd].xbmcIRCode;
			// send it to the application
			g_application.FrameMove();
			// return the number of characters written
			return 0;
		}
		cmd++;
	}
	return 0;
}

/* Handle navagate requests */
int CXbmcWeb::xbmcNavigate( int eid, webs_t wp, char_t *parameter)
{

	/***********************************
	 *  Navagation command structure
	 *  This structure is formatted as follows:
	 *		command string, XBMC application state
	 */
	xbmcNavigationHandlerType	xbmcNavigator[ ] = {
		WEB_VIDEOS, WEB_NAV_VIDEOS,
		WEB_MUSIC, WEB_NAV_MUSIC,
		WEB_PICTURES, WEB_NAV_PICTURES,
		WEB_PROGRAMS, WEB_NAV_PROGRAMS,
		WEB_FILES, WEB_NAV_FILES,
		//WEB_PLAYLIST, WEB_NAV_PLAYLIST,
		"", -1};

	if (!stricmp(WEB_PLAYLIST, parameter))
	{
		//we are navigating to a playlist
		SetNavigatorState(WEB_NAV_PLAYLIST);
		catalogItemCounter = 0;

		//delete old directory
		if (directory) {
			delete directory;
			directory = NULL;
		}
	}

	int cmd = 0;
	// look through the xbmcCommandStructure
	while( xbmcNavigator[ cmd].xbmcAppStateCode != -1) {
		// if we have a match
		if( stricmp( parameter,xbmcNavigator[ cmd].xbmcNavigateParameter) == 0) {

			SetNavigatorState(xbmcNavigator[cmd].xbmcAppStateCode);
			catalogItemCounter = 0;

			//delete old directory
			if (directory) {
				delete directory;
				directory = NULL;
			}
			
			//make a new directory and set the nessecary shares
			directory = new CVirtualDirectory();

			VECSHARES *shares;
			CStdString strDirectory;

			//get shares and extensions
			if (!strcmp(parameter, WEB_VIDEOS))
			{
				g_playlistPlayer.SetCurrentPlaylist(PLAYLIST_VIDEO);
				strDirectory = g_stSettings.m_szDefaultVideos;
				shares = &g_settings.m_vecMyVideoShares;
				directory->SetMask(g_stSettings.m_szMyVideoExtensions);
			}
			else if (!strcmp(parameter, WEB_MUSIC))
			{
				g_playlistPlayer.SetCurrentPlaylist(PLAYLIST_MUSIC);
				strDirectory = g_stSettings.m_szDefaultMusic;
				shares = &g_settings.m_vecMyMusicShares;
				directory->SetMask(g_stSettings.m_szMyMusicExtensions);
			}
			else if (!strcmp(parameter, WEB_PICTURES))
			{
				strDirectory = g_stSettings.m_szDefaultPictures;
				shares = &g_settings.m_vecMyPictureShares;
				directory->SetMask(g_stSettings.m_szMyPicturesExtensions);
			}
			else if (!strcmp(parameter, WEB_PROGRAMS))
			{
				strDirectory = g_stSettings.m_szDefaultFiles;
				shares = &g_settings.m_vecMyFilesShares;
				directory->SetMask("xbe|cut");
			}
			else if (!strcmp(parameter, WEB_FILES))
			{
				strDirectory = g_stSettings.m_szDefaultFiles;
				shares = &g_settings.m_vecMyFilesShares;
				directory->SetMask("*");
			}

			directory->SetShares(*shares);
			directory->GetDirectory("",webDirItems);

			//sort items
			sort(webDirItems.begin(), webDirItems.end(), SSortWebFilesByName());

			return 0;
		}
		cmd++;
	}
	return 0;
}

int catalogNumber( char_t *parameter)
{
	int itemNumber = 0;
	char_t *comma = NULL;

	if(( comma = strchr( parameter, ',')) != NULL) {
		itemNumber = atoi( comma + 1);
	}
	return itemNumber;
}

/* Deal with catalog requests */
int CXbmcWeb::xbmcCatalog( int eid, webs_t wp, char_t *parameter)
{
	int iterationCounter = 0;
	int selectionNumber = 0;
	int	iItemCount = 0;  // for test purposes
	int cnt = 0;
	char buffer[XML_MAX_INNERTEXT_SIZE];

	// by default teh answer to any question is 0
	if( eid != NO_EID) {
		ejSetResult( eid, "0");
	}

	// if we are in an interface that supports media catalogs
	DWORD state = GetNavigatorState();
	if((state == WEB_NAV_VIDEOS) ||
			(state == WEB_NAV_MUSIC) ||
			(state == WEB_NAV_PICTURES) ||
			(state == WEB_NAV_PROGRAMS) ||
			(state == WEB_NAV_FILES))
	{
		CHAR *output = "error";

		iItemCount = webDirItems.size();

		// have we requested a catalog item name?
		if( strstr( parameter, XBMC_CAT_NAME) != NULL)
		{
			selectionNumber = catalogNumber( parameter);
			if (navigatorState == WEB_NAV_PLAYLIST)
			{
				if (selectionNumber <= g_playlistPlayer.GetPlaylist( PLAYLIST_MUSIC ).size())
				{
					strcpy(buffer, g_playlistPlayer.GetPlaylist( PLAYLIST_MUSIC )[selectionNumber].GetDescription());
					output = buffer;
				}
			}
			else
			{
				if( selectionNumber <= iItemCount)
				{
					CFileItem *itm = webDirItems[selectionNumber];
					strcpy(buffer, itm->m_strPath);
					output = buffer;
				}
			}
			websHeader(wp); wroteHeader = TRUE;
			cnt = websWrite(wp, output);
			websFooter(wp); wroteFooter = TRUE;
			return cnt;
		}

		// have we requested a catalog item type?
		if( strstr( parameter, XBMC_CAT_TYPE) != NULL) {

			selectionNumber = catalogNumber( parameter);
			if (navigatorState == WEB_NAV_PLAYLIST)
			{
				output = XBMC_CMD_MUSIC;
			}
			else
			{
				if( selectionNumber <= iItemCount) {
					CFileItem *itm = webDirItems[selectionNumber];
					if (itm->m_bIsFolder)
					{
						output = XBMC_CMD_DIRECTORY;
					}
					else
					{
						switch(GetNavigatorState())
						{
						case WEB_NAV_VIDEOS: 
							output = XBMC_CMD_VIDEO;
							break;
						case WEB_NAV_MUSIC:
							output = XBMC_CMD_MUSIC;
							break;
						case WEB_NAV_PICTURES:
							output = XBMC_CMD_PICTURE;
							break;
						default:
							output = XBMC_CMD_APPLICATION;
							break;
						}
					}
				} 
			}
			if( eid != NO_EID) {
				ejSetResult( eid, output);
			} else {
				cnt = websWrite(wp, output);
			}
			return cnt;
		}

		// have we requested the number of catalog items?
		if( stricmp( parameter, XBMC_CAT_ITEMS) == 0) {
			int items;
			if (navigatorState == WEB_NAV_PLAYLIST)
			{
				items = g_playlistPlayer.GetPlaylist( PLAYLIST_MUSIC ).size();
			}
			else
			{
				items = iItemCount;
			}
			if( eid != NO_EID) {
				itoa(items, buffer, 10);
				ejSetResult( eid, buffer);
			} else {
				cnt = websWrite(wp, "%i", items);
			}
			return cnt;
		}

		// have we requested the first catalog item?
		if( stricmp( parameter, XBMC_CAT_FIRST) == 0) {
			catalogItemCounter = 0;
			CStdString name;
			if (navigatorState == WEB_NAV_PLAYLIST)
			{
				if(catalogItemCounter < g_playlistPlayer.GetPlaylist( PLAYLIST_MUSIC ).size()) {
					name = g_playlistPlayer.GetPlaylist( PLAYLIST_MUSIC )[catalogItemCounter].GetDescription();
				}
			}
			else
			{
				if(catalogItemCounter < iItemCount) {
					name = webDirItems[catalogItemCounter]->GetLabel();
					if (name == "") name = "..";
				}
			}
			if( eid != NO_EID) {
				ejSetResult( eid, ( char_t *)name.c_str());
			} else {
				cnt = websWrite(wp, (char_t *)name.c_str());
			}
			return cnt;
		}

		// have we requested the next catalog item?
		if( stricmp( parameter, XBMC_CAT_NEXT) == 0) {
			// are there items left to see
			CStdString name;
			if (navigatorState == WEB_NAV_PLAYLIST)
			{
				if((catalogItemCounter + 1) < g_playlistPlayer.GetPlaylist( PLAYLIST_MUSIC ).size())
				{
					++catalogItemCounter;
					name = g_playlistPlayer.GetPlaylist( PLAYLIST_MUSIC )[catalogItemCounter].GetDescription();
					if( eid != NO_EID) {
						ejSetResult( eid, (char_t *)name.c_str());
					} else {
						cnt = websWrite(wp, (char_t *)name.c_str());
					}
				}
				else
				{
					if( eid != NO_EID) {
						ejSetResult( eid, XBMC_NONE);
					} else {
						cnt = websWrite(wp, XBMC_NONE);
					}
				}
			}
			else
			{
				if((catalogItemCounter + 1) < iItemCount) {
					++catalogItemCounter;

					name = webDirItems[catalogItemCounter]->GetLabel();
					if (name == "") name = "..";
					if( eid != NO_EID) {
						ejSetResult( eid, (char_t *)name.c_str());
					} else {
						cnt = websWrite(wp, (char_t *)name.c_str());
					}
				} else {
					if( eid != NO_EID) {
						ejSetResult( eid, XBMC_NONE);
					} else {
						cnt = websWrite(wp, XBMC_NONE);
					}
				}
			}
			return cnt;
		}

		// have we selected a catalog item name?
		if(( strstr( parameter, XBMC_CAT_SELECT) != NULL) ||
			  ( strstr( parameter, XBMC_CAT_QUE) != NULL))
		{
			selectionNumber = catalogNumber( parameter);
			if( selectionNumber <= iItemCount)
			{
				if(strstr( parameter, XBMC_CAT_QUE) != NULL)
				{
					// attempt to enque the selected directory or file

					CFileItem *itm = webDirItems[selectionNumber];
					AddItemToPlayList(itm);
					g_playlistPlayer.HasChanged();
				}
				else if (strstr( parameter, XBMC_CAT_UNQUE) != NULL)
				{
					// attemt to unque item from playlist.
					//g_playlistPlayer.GetPlaylist(PLAYLIST_MUSIC).Remove(g_playlistPlayer.GetPlaylist( PLAYLIST_MUSIC )[selectionNumber].GetFileName());
				}
				else
				{
					CFileItem *itm = webDirItems[selectionNumber];
					if (!itm) return 0;

					if (itm->m_bIsFolder) // enter the directory
					{
						CStdString strDirectory = itm->m_strPath;
						CStdString strParentPath;
						webDirItems.clear();

						//set new current directory for webserver
						SetCurrentDir(strDirectory.c_str());

						bool bParentExists=CUtil::GetParentPath(strDirectory, strParentPath);

						// check if current directory is a root share
						if ( !directory->IsShare(strDirectory) )
						{
							// no, do we got a parent dir?
							if ( bParentExists )
							{
								// yes
								CFileItem *pItem = new CFileItem("..");
								pItem->m_strPath=strParentPath;
								pItem->m_bIsFolder=true;
								pItem->m_bIsShareOrDrive=false;
								webDirItems.push_back(pItem);
							}
						}
						else
						{
							// yes, this is the root of a share
							// add parent path to the virtual directory
							CFileItem *pItem = new CFileItem("..");
							pItem->m_strPath="";
							pItem->m_bIsShareOrDrive=false;
							pItem->m_bIsFolder=true;
							webDirItems.push_back(pItem);
						}
						directory->GetDirectory(strDirectory, webDirItems);
						sort(webDirItems.begin(), webDirItems.end(), SSortWebFilesByName());
					}
					else
					{
						//no directory, execute file
						SetCurrentItem(selectionNumber);
						CFileItem *itm = webDirItems[selectionNumber];
						if (GetNavigatorState() == WEB_NAV_VIDEOS ||
								GetNavigatorState() == WEB_NAV_MUSIC)
						{
							strCurrentMediaFile = itm->m_strPath;
							g_applicationMessenger.MediaPlay(itm->m_strPath);
						}
						else
						{
							if (GetNavigatorState() == WEB_NAV_PICTURES)
							{
								g_applicationMessenger.PictureShow(itm->m_strPath);
							}
						}
					}
				}
			}
			return 0;
		}
	}
	return 0;
}


/* Play */
int CXbmcWeb::xbmcPlayerPlay( int eid, webs_t wp, char_t *parameter)
{
	if (strCurrentMediaFile.size() > 0)
	{
		g_applicationMessenger.MediaPlay(strCurrentMediaFile);
	}
	else
	{
		g_applicationMessenger.PlayListPlayerPlay(0);
	}
	return 0;
}

/*
 * Play next file in active playlist
 * Active playlist = MUSIC or VIDEOS
 */
int CXbmcWeb::xbmcPlayerNext( int eid, webs_t wp, char_t *parameter)
{
	OutputDebugString("next");
	g_applicationMessenger.PlayListPlayerNext();
	return 0;
}

/*
 * Play previous file in active playlist
 * Active playlist = MUSIC or VIDEOS
 */
int CXbmcWeb::xbmcPlayerPrevious( int eid, webs_t wp, char_t *parameter)
{
	OutputDebugString("previous");
	g_applicationMessenger.PlayListPlayerPrevious();
	return 0;
}

/* Turn on subtitles */
int CXbmcWeb::xbmcSubtitles( int eid, webs_t wp, char_t *parameter)
{
	if (g_application.m_pPlayer) g_application.m_pPlayer->ToggleSubtitles();
	return 0;
}

/* Parse an XBMC command */
int CXbmcWeb::xbmcProcessCommand( int eid, webs_t wp, char_t *command, char_t *parameter)
{
	if (!strcmp(command, "play"))								return xbmcPlayerPlay(eid, wp, parameter);
	else if (!strcmp(command, "stop"))					g_applicationMessenger.MediaStop();
	else if (!strcmp(command, "pause"))					g_applicationMessenger.MediaPause();
	else if (!strcmp(command, "shutdown"))			g_applicationMessenger.Shutdown();
	else if (!strcmp(command, "restart"))				g_applicationMessenger.Restart();
	else if (!strcmp(command, "exit"))					g_applicationMessenger.RebootToDashBoard();
	else if (!strcmp(command, "show_time"))			return 0;
	else if (!strcmp(command, "remote"))				return xbmcRemoteControl(eid, wp, parameter);			// remote control functions
	else if (!strcmp(command, "navigate"))			return xbmcNavigate(eid, wp, parameter);	// Navigate to a particular interface
	else if (!strcmp(command, "catalog"))				return xbmcCatalog(eid, wp, parameter);	// interface to teh media catalog

	else if (!strcmp(command, "ff"))						return 0;
	else if (!strcmp(command, "rw"))						return 0;
	else if (!strcmp(command, "next"))					return xbmcPlayerNext(eid, wp, parameter);
	else if (!strcmp(command, "previous"))			return xbmcPlayerPrevious(eid, wp, parameter);
	return 0;
}

/* XBMC Javascript binding for ASP. This will be invoked when "XBMCCommand" is
 *  embedded in an ASP page.
 */
int CXbmcWeb::xbmcCommand( int eid, webs_t wp, int argc, char_t **argv)
{
	char_t	*command, *parameter;

	if (ejArgs(argc, argv, T("%s %s"), &command, &parameter) < 2) {
		websError(wp, 400, T("Insufficient args\n"));
		return -1;
	}

	return xbmcProcessCommand( eid, wp, command, parameter);
}

/* XBMC form for posted data (in-memory CGI). This will be called when the
 * form in /xbmc/xbmcForm is invoked. Set browser to "localhost/xbmc/xbmcForm?command=test&parameter=videos" to test.
 * Parameters:
 *		command:	The command that will be invoked
 *		parameter:	The parameter for this command
 *		next_page:	The next page to display, use to invoke another page following this call
 */
void CXbmcWeb::xbmcForm(webs_t wp, char_t *path, char_t *query)
{
	char_t	*command, *parameter, *next_page;

	command = websGetVar(wp, WEB_COMMAND, XBMC_NONE); 
	parameter = websGetVar(wp, WEB_PARAMETER, XBMC_NONE);

	// do the command
	wroteHeader = false;
	wroteFooter = false;
	xbmcProcessCommand( NO_EID, wp, command, parameter);

	// if we do want to redirect
	if( websTestVar(wp, WEB_NEXT_PAGE)) {
		next_page = websGetVar(wp, WEB_NEXT_PAGE, XBMC_NONE); 
		// redirect to another web page
		websRedirect(wp, next_page);
		return;
	}
	websDone(wp, 200);
}
