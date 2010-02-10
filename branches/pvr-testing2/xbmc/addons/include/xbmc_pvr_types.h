/*
 *      Copyright (C) 2005-2009 Team XBMC
 *      http://www.xbmc.org
 *
 *  This Program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your option)
 *  any later version.
 *
 *  This Program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with XBMC; see the file COPYING.  If not, write to
 *  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 *  http://www.gnu.org/copyleft/gpl.html
 *
 */

/*
 * Common data structures shared between XBMC and PVR clients
 */

#ifndef __PVRCLIENT_TYPES_H__
#define __PVRCLIENT_TYPES_H__

#define XBMC_PVRDLL_API 1

#ifdef HAS_XBOX_HARDWARE
#include <xtl.h>
#else
#ifndef _LINUX
#include <windows.h>
#else
#undef __cdecl
#define __cdecl
#undef __declspec
#define __declspec(x)
#include <time.h>
#endif
#endif

#include <string.h>
#include "xbmc_addon_dll.h"

#undef ATTRIBUTE_PACKED
#undef PRAGMA_PACK_BEGIN
#undef PRAGMA_PACK_END

#if defined(__GNUC__)
#if __GNUC__ > 2 || (__GNUC__ == 2 && __GNUC_MINOR__ >= 95)
#define ATTRIBUTE_PACKED __attribute__ ((packed))
#define PRAGMA_PACK 0
#endif
#endif

#if !defined(ATTRIBUTE_PACKED)
#define ATTRIBUTE_PACKED
#define PRAGMA_PACK 1
#endif

#define EVCONTENTMASK_MOVIEDRAMA               0x10
#define EVCONTENTMASK_NEWSCURRENTAFFAIRS       0x20
#define EVCONTENTMASK_SHOW                     0x30
#define EVCONTENTMASK_SPORTS                   0x40
#define EVCONTENTMASK_CHILDRENYOUTH            0x50
#define EVCONTENTMASK_MUSICBALLETDANCE         0x60
#define EVCONTENTMASK_ARTSCULTURE              0x70
#define EVCONTENTMASK_SOCIALPOLITICALECONOMICS 0x80
#define EVCONTENTMASK_EDUCATIONALSCIENCE       0x90
#define EVCONTENTMASK_LEISUREHOBBIES           0xA0
#define EVCONTENTMASK_SPECIAL                  0xB0
#define EVCONTENTMASK_USERDEFINED              0xF0

#ifdef __cplusplus
extern "C" {
#endif

  /**
  * PVR Client Return Data handle
  */
  struct PVRHANDLE_STRUCT
  {
    void* DATA_ADDRESS;
    int   DATA_IDENTIFIER;
  };
  typedef PVRHANDLE_STRUCT* PVRHANDLE;

  /**
  * PVR Client startup properties
  */
  struct PVR_PROPS
  {
    int clientID;
    ADDON_HANDLE hdl;
    const char *userpath;
    const char *clientpath;
  };

  /**
  * PVR Client Error Codes
  */
  typedef enum {
    PVR_ERROR_NO_ERROR             = 0,
    PVR_ERROR_UNKOWN               = -1,
    PVR_ERROR_NOT_IMPLEMENTED      = -2,
    PVR_ERROR_SERVER_ERROR         = -3,
    PVR_ERROR_SERVER_TIMEOUT       = -4,
    PVR_ERROR_NOT_SYNC             = -5,
    PVR_ERROR_NOT_DELETED          = -6,
    PVR_ERROR_NOT_SAVED            = -7,
    PVR_ERROR_RECORDING_RUNNING    = -8,
    PVR_ERROR_ALREADY_PRESENT      = -9
  } PVR_ERROR;

  /**
  * PVR Client Event Codes
  * Sent via PVRManager callback
  */
  typedef enum {
    PVR_EVENT_UNKNOWN              = 0,
    PVR_EVENT_CLOSE                = 1,
    PVR_EVENT_RECORDINGS_CHANGE    = 2,
    PVR_EVENT_CHANNELS_CHANGE      = 3,
    PVR_EVENT_TIMERS_CHANGE        = 4,
    PVR_EVENT_MSG_STATUS           = 5,
    PVR_EVENT_MSG_INFO             = 6,
    PVR_EVENT_MSG_WARNING          = 7,
    PVR_EVENT_MSG_ERROR            = 8,
  } PVR_EVENT;

#if PRAGMA_PACK
#pragma pack(1)
#endif

  /**
  * PVR Client Properties
  * Returned on client initialization
  */
  typedef struct PVR_SERVERPROPS {
    bool SupportChannelLogo;            /* Client support transfer of channel logos */
    bool SupportChannelSettings;        /* Client support changing channels on backend */
    bool SupportTimeShift;              /* Client handle Live TV Timeshift, otherwise it is handled by XBMC */
    bool SupportEPG;                    /* Client provide EPG information */
    bool SupportTV;                     /* Client provide TV Channels, is false for Radio only clients */
    bool SupportRadio;                  /* Client provide also Radio Channels */
    bool SupportRecordings;             /* Client support playback of recordings stored on the backend */
    bool SupportTimers;                 /* Client support creation and editing of timers */
    bool SupportDirector;               /* Client provide information about multifeed channels, like Sky Select */
    bool SupportBouquets;               /* Client support Bouqets */
    bool HandleInputStream;             /* Input stream is handled by the client if set, can be false for http */
    bool HandleDemuxing;                /* Demux of stream is handled by the client, as example TVFrontend (htsp protocol) */
  } ATTRIBUTE_PACKED PVR_SERVERPROPS;

  /**
  * PVR Stream Properties
  * Returned on request
  */
  typedef struct PVR_STREAMPROPS {
#define PVR_STREAM_MAX_STREAMS 16
    int nstreams;
    struct PVR_STREAM {
      int id;
      int physid;
      unsigned int codec_type;
      unsigned int codec_id;
    } stream[PVR_STREAM_MAX_STREAMS];
  } ATTRIBUTE_PACKED PVR_STREAMPROPS;

  /**
  * EPG Channel Definition
  * Is used by the TransferChannelEntry function to inform XBMC that this
  * channel is present.
  */
  typedef struct PVR_CHANNEL {
    int             uid;                /* Unique identifier for this channel */
    int             number;             /* The backend channel number */

    const char     *name;               /* Channel name provided by the Broadcast */
    const char     *callsign;           /* Channel name provided by the user (if present) */
    const char     *iconpath;           /* Path to the channel icon (if present) */

    int             encryption;         /* This is a encrypted channel and have a CA Id */
    bool            radio;              /* This is a radio channel */
    bool            hide;               /* This channel is hidden by the user */
    bool            recording;          /* This channel is currently recording */

    int             bouquet;            /* Bouquet ID this channel have (if supported) */

    bool            multifeed;          /* This is a multifeed channel */
    int             multifeed_master;   /* The Master multifeed channel, multifeed_master==number for master itself */
    int             multifeed_number;   /* The own number inside multifeed channel list */

    /* The Stream URL to access this channel, it can be all types of protocol and types
     * are supported by XBMC or in case the client read the stream leave it empty
     * as URL.
     */
    const char     *stream_url;
  } ATTRIBUTE_PACKED PVR_CHANNEL;

  /**
  * EPG Bouquet Definition
  */
  typedef struct PVR_BOUQUET {
    char*  Name;
    char*  Category;
    int    Number;
  } ATTRIBUTE_PACKED PVR_BOUQUET;

  /**
  * EPG Programme Definition
  * Used to signify an individual broadcast, whether it is also a recording, timer etc.
  */
  typedef struct PVR_PROGINFO {
//    int           bouquet;
//    int           recording;
//    int           rec_status;
//    int           event_flags;

    unsigned int  uid;
    int           channum;
    const char   *title;
    const char   *subtitle;
    const char   *description;
    time_t        starttime;
    time_t        endtime;
    int           genre_type;
    int           genre_sub_type;
    int           parental_rating;
  } ATTRIBUTE_PACKED PVR_PROGINFO;

  /**
   * TV Timer Definition
   */
  typedef struct PVR_TIMERINFO {
    int           index;
    int           active;
    const char   *title;
    const char   *directory;
    int           channelNum;
    time_t        starttime;
    time_t        endtime;
    time_t        firstday;
    int           recording;
    int           priority;
    int           lifetime;
    int           repeat;
    int           repeatflags;
  } ATTRIBUTE_PACKED PVR_TIMERINFO;

  /**
   * TV Recording Definition
   */
  typedef struct PVR_RECORDINGINFO {
    int           index;
    const char   *directory;
    const char   *title;
    const char   *subtitle;
    const char   *description;
    const char   *channel_name;
    time_t        recording_time;
    int           duration;
    int           priority;
    int           lifetime;
    /* The Stream URL to access this channel, it can be all types of protocol and types
     * are supported by XBMC or in case the client read the stream leave it empty
     * as URL.
     */
    const char    *stream_url;
  } ATTRIBUTE_PACKED PVR_RECORDINGINFO;

  /**
   * TV Stream Signal Quality Information
   */
  typedef struct PVR_SIGNALQUALITY {
    char          frontend_name[1024];
    char          frontend_status[1024];
    int           snr;
    int           signal;
    long          ber;
    long          unc;
    double        video_bitrate;
    double        audio_bitrate;
    double        dolby_bitrate;
  } ATTRIBUTE_PACKED PVR_SIGNALQUALITY;

#if PRAGMA_PACK
#pragma pack()
#endif

  /**
   * Structure to transfer the PVR functions to XBMC
   */
  typedef struct PVRClient
  {
    /** PVR General Functions **/
    PVR_ERROR (__cdecl* GetProperties)(PVR_SERVERPROPS *props);
    PVR_ERROR (__cdecl* GetStreamProperties)(PVR_STREAMPROPS *props);
    const char* (__cdecl* GetBackendName)();
    const char* (__cdecl* GetBackendVersion)();
    const char* (__cdecl* GetConnectionString)();
    PVR_ERROR (__cdecl* GetDriveSpace)(long long *total, long long *used);
    PVR_ERROR (__cdecl* GetBackendTime)(time_t *localTime, int *gmtOffset);

    /** PVR EPG Functions **/
    PVR_ERROR (__cdecl* RequestEPGForChannel)(PVRHANDLE handle, const PVR_CHANNEL &channel, time_t start, time_t end);

    /** PVR Bouquets Functions **/
    int (__cdecl* GetNumBouquets)();

    /** PVR Channel Functions **/
    int (__cdecl* GetNumChannels)();
    PVR_ERROR (__cdecl* RequestChannelList)(PVRHANDLE handle, int radio);
//    PVR_ERROR (__cdecl* GetChannelSettings)(cPVRChannelInfoTag *result);
//    PVR_ERROR (__cdecl* UpdateChannelSettings)(const cPVRChannelInfoTag &chaninfo);
//    PVR_ERROR (__cdecl* AddChannel)(const PVR_CHANNEL &info);
//    PVR_ERROR (__cdecl* DeleteChannel)(unsigned int number);
//    PVR_ERROR (__cdecl* RenameChannel)(unsigned int number, CStdString &newname);
//    PVR_ERROR (__cdecl* MoveChannel)(unsigned int number, unsigned int newnumber);

    /** PVR Recording Functions **/
    int (__cdecl* GetNumRecordings)();
    PVR_ERROR (__cdecl* RequestRecordingsList)(PVRHANDLE handle);
    PVR_ERROR (__cdecl* DeleteRecording)(const PVR_RECORDINGINFO &recinfo);
    PVR_ERROR (__cdecl* RenameRecording)(const PVR_RECORDINGINFO &recinfo, const char *newname);

    /** PVR Timer Functions **/
    int (__cdecl* GetNumTimers)();
    PVR_ERROR (__cdecl* RequestTimerList)(PVRHANDLE handle);
    PVR_ERROR (__cdecl* AddTimer)(const PVR_TIMERINFO &timerinfo);
    PVR_ERROR (__cdecl* DeleteTimer)(const PVR_TIMERINFO &timerinfo, bool force);
    PVR_ERROR (__cdecl* RenameTimer)(const PVR_TIMERINFO &timerinfo, const char *newname);
    PVR_ERROR (__cdecl* UpdateTimer)(const PVR_TIMERINFO &timerinfo);

    /** PVR Live Stream Functions **/
    bool (__cdecl* OpenLiveStream)(const PVR_CHANNEL &channelinfo);
    void (__cdecl* CloseLiveStream)();
    int (__cdecl* ReadLiveStream)(unsigned char* buf, int buf_size);
    long long (__cdecl* SeekLiveStream)(long long pos, int whence);
    long long (__cdecl* LengthLiveStream)(void);
    int (__cdecl* GetCurrentClientChannel)();
    bool (__cdecl* SwitchChannel)(const PVR_CHANNEL &channelinfo);
    PVR_ERROR (__cdecl* SignalQuality)(PVR_SIGNALQUALITY &qualityinfo);
    /** PVR Live Stream Function for the MediaPortal PVR addon **/
    const char* (__cdecl* GetLiveStreamURL)(const PVR_CHANNEL &channelinfo);

    /** PVR Recording Stream Functions **/
    bool (__cdecl* OpenRecordedStream)(const PVR_RECORDINGINFO &recinfo);
    void (__cdecl* CloseRecordedStream)(void);
    int (__cdecl* ReadRecordedStream)(unsigned char* buf, int buf_size);
    long long (__cdecl* SeekRecordedStream)(long long pos, int whence);
    long long (__cdecl* LengthRecordedStream)(void);
  } PVRClient;

#ifdef __cplusplus
}
#endif

#endif //__PVRCLIENT_TYPES_H__
