//  GUI messages outside GuiLib
//
#pragma once
#include "guimessage.h"

//  DVD Drive Messages
#define GUI_MSG_DVDDRIVE_EJECTED_CD     GUI_MSG_USER + 1
#define GUI_MSG_DVDDRIVE_CHANGED_CD     GUI_MSG_USER + 2

//  General playlist items changed
#define GUI_MSG_PLAYLIST_CHANGED        GUI_MSG_USER + 3

//  Start Slideshow in my pictures lpVoid = CStdString
//  Param lpVoid: CStdString* that points to the Directory
//  to start the slideshow in.
#define GUI_MSG_START_SLIDESHOW         GUI_MSG_USER + 4

#define GUI_MSG_PLAYBACK_STARTED        GUI_MSG_USER + 5
#define GUI_MSG_PLAYBACK_ENDED          GUI_MSG_USER + 6

//  Playback stopped by user
#define GUI_MSG_PLAYBACK_STOPPED        GUI_MSG_USER + 7

//  Message is send by the playlistplayer when it starts a playlist
//  Parameter:
//  dwParam1 = Current Playlist, can be PLAYLIST_MUSIC, PLAYLIST_TEMP_MUSIC, PLAYLIST_VIDEO or PLAYLIST_TEMP_VIDEO
//  dwParam2 = Item started in the playlist
//  lpVoid = Playlistitem started playing
#define GUI_MSG_PLAYLISTPLAYER_STARTED  GUI_MSG_USER + 8

//  Message is send by playlistplayer when next/previous item is started
//  Parameter:
//  dwParam1 = Current Playlist, can be PLAYLIST_MUSIC, PLAYLIST_TEMP_MUSIC, PLAYLIST_VIDEO or PLAYLIST_TEMP_VIDEO
//  dwParam2 = LOWORD Position of the current playlistitem
//             HIWORD Position of the previous playlistitem
//  lpVoid = Current Playlistitem
#define GUI_MSG_PLAYLISTPLAYER_CHANGED  GUI_MSG_USER + 9

//  Message is send by the playlistplayer when the last item to play ended
//  Parameter:
//  dwParam1 = Current Playlist, can be PLAYLIST_MUSIC, PLAYLIST_TEMP_MUSIC, PLAYLIST_VIDEO or PLAYLIST_TEMP_VIDEO
//  dwParam2 = Playlistitem played when stopping
#define GUI_MSG_PLAYLISTPLAYER_STOPPED  GUI_MSG_USER + 10

#define GUI_MSG_LOAD_SKIN               GUI_MSG_USER + 11

//  Message is send by the dialog scan music
//  Parameter:
//  StringParam = Directory last scanned
#define GUI_MSG_DIRECTORY_SCANNED       GUI_MSG_USER + 12

#define GUI_MSG_SCAN_FINISHED           GUI_MSG_USER + 13

//  Mute activated by the user
#define GUI_MSG_MUTE_ON                 GUI_MSG_USER + 14
#define GUI_MSG_MUTE_OFF                GUI_MSG_USER + 15

