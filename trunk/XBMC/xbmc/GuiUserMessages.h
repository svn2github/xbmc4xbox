//	GUI messages outside GuiLib
//
#pragma once
#include "guimessage.h"

//	DVD Drive Messages
#define GUI_MSG_DVDDRIVE_EJECTED_CD			GUI_MSG_USER + 1
#define GUI_MSG_DVDDRIVE_CHANGED_CD			GUI_MSG_USER + 2

//	General playlist items changed
#define GUI_MSG_PLAYLIST_CHANGED				GUI_MSG_USER + 3

//	Start Slideshow in my pictures lpVoid = CStdString
//	Param lpVoid: CStdString* that points to the Directory 
//	to start the slideshow in.
#define GUI_MSG_START_SLIDESHOW					GUI_MSG_USER + 4

#define GUI_MSG_PLAYBACK_STARTED				GUI_MSG_USER + 5
#define GUI_MSG_PLAYBACK_ENDED					GUI_MSG_USER + 6

//	Playback stopped by user
#define GUI_MSG_PLAYBACK_STOPPED				GUI_MSG_USER + 7

//	Message is send by playlistplayer when starts playing or next/previous item is started
//	Parameter:
//	dwParam1 = Current Playlist, can be PLAYLIST_MUSIC, PLAYLIST_TEMP_MUSIC, PLAYLIST_VIDEO or PLAYLIST_TEMP_VIDEO
//	dwParam2 = LOWORD Position of the current playlistitem
//						 HIWORD Position of the previous playlistitem
//	lpVoid = Current Playlistitem
#define GUI_MSG_PLAYLIST_PLAY_NEXT_PREV	GUI_MSG_USER + 8

#define GUI_MSG_LOAD_SKIN	GUI_MSG_USER + 9
