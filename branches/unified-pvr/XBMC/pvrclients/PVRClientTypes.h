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
Common data structures shared between XBMC and PVR clients
*/

#ifndef __PVRCLIENT_TYPES_H__
#define __PVRCLIENT_TYPES_H__

#define MIN_XBMC_PVRDLL_API 1

#include <vector>
#include <string.h>
#include <time.h>

extern "C"
{
  /**
  * XBMC logging levels
  */ 
  enum PVR_LOG {
    LOG_DEBUG,
    LOG_INFO,
    LOG_ERROR
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
    PVR_ERROR_NOT_SAVED            = -7
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
    PVR_EVENT_TIMERS_CHANGE        = 4
  } PVR_EVENT;

  /**
  * PVR Client Properties
  * Returned on client initialization
  */ 
  typedef struct PVR_SERVERPROPS {
    const char* Name;
    const char* DefaultHostname;
    int   DefaultPort;
    const char* DefaultUser;
    const char* DefaultPassword;
    bool SupportChannelLogo;
    bool SupportChannelSettings;
    bool SupportTimeShift;
    bool SupportEPG;
    bool SupportRadio;
    bool SupportRecordings;
    bool SupportTimers;
    bool SupportTeletext;
    bool SupportDirector;
    bool SupportBouquets;
  } PVR_SERVERPROPS;

  /**
  * EPG Channel Definition
  */
  typedef struct PVR_CHANNEL {
    int         uid;
    const char  *name;
    const char  *callsign;
    const char  *iconpath;
    int         number;
    int         bouquet;
    bool        encrypted;
    bool        radio;
    bool        hide;
    bool        recording;
  } PVR_CHANNEL;
  typedef struct PVR_CHANLIST {
    PVR_CHANNEL* channel;
    int          length;
  } PVR_CHANLIST;

  /**
  * EPG Bouquet Definition
  */
  typedef struct PVR_BOUQUET {
    char*  Name;
    char*  Category;
    int    Number;
  } PVR_BOUQUET;
  typedef struct PVR_BOUQUETLIST {
    PVR_BOUQUET* bouquet;
    int          length;
  } PVR_BOUQUETLIST;

  /**
  * EPG Programme Definition
  * Used to signify an individual broadcast, whether it is also a recording, timer etc.
  */
  typedef struct PVR_PROGINFO {
    int           uid; // unique identifier, if supported will be used for 
    int           channum;
    int           bouquet;
    const char   *title;
    const char   *subtitle;
    const char   *description;
    time_t        starttime;
    time_t        endtime;
    const char   *episodeid;
    const char   *seriesid;
    const char   *category;
    int           recording;
    int           rec_status;
    int           event_flags;
  } PVR_PROGINFO;
  typedef struct PVR_PROGLIST {
    PVR_PROGINFO* progInfo;
    int           length;
  } PVR_PROGLIST;

  /**
  * XBMC callbacks
  */ 
  typedef void (*PVREventCallback)(void *userData, const PVR_EVENT, const char*);
  typedef void (*PVRLogCallback)(void *userData, const PVR_LOG loglevel, const char *format, ... );

  typedef struct PVRCallbacks
  {
    PVREventCallback Event;
    PVRLogCallback   Log;
    void *userData;
  } PVRCallbacks;

  /**
  * The PVRSetting class for client's GUI settings.
  */
  class PVRSetting
  {
  public:
    enum SETTING_TYPE { NONE=0, CHECK, SPIN };

    PVRSetting(SETTING_TYPE t, const char *label)
    {
      name = NULL;
      if (label)
      {
        name = new char[strlen(label)+1];
        strcpy(name, label);
      }
      current = 0;
      type = t;
    }

    PVRSetting(const PVRSetting &rhs) // copy constructor
    {
      name = NULL;
      if (rhs.name)
      {
        name = new char[strlen(rhs.name)+1];
        strcpy(name, rhs.name);
      }
      current = rhs.current;
      type = rhs.type;
      for (unsigned int i = 0; i < rhs.entry.size(); i++)
      {
        char *lab = new char[strlen(rhs.entry[i]) + 1];
        strcpy(lab, rhs.entry[i]);
        entry.push_back(lab);
      }
    }

    ~PVRSetting()
    {
      if (name)
        delete[] name;
      for (unsigned int i=0; i < entry.size(); i++)
        delete[] entry[i];
    }

    void AddEntry(const char *label)
    {
      if (!label || type != SPIN) return;
      char *lab = new char[strlen(label) + 1];
      strcpy(lab, label);
      entry.push_back(lab);
    }

    // data members
    SETTING_TYPE type;
    char*        name;
    int          current;
    std::vector<const char *> entry;
  };

  // Structure to transfer the above functions to XBMC
  struct PVRClient
  {
    void (__cdecl* GetSettings)(std::vector<PVRSetting> **vecSettings);
    void (__cdecl* UpdateSetting)(int num);
    PVR_ERROR (__cdecl* Create)(PVRCallbacks *callbacks);
    long (__cdecl* GetID)();
    PVR_ERROR (__cdecl* GetProperties)(PVR_SERVERPROPS *props);
    PVR_ERROR (__cdecl* Connect)();
    void (__cdecl* Disconnect)();
    bool (__cdecl* IsUp)();
    const char* (__cdecl* GetBackendName)();
    const char* (__cdecl* GetBackendVersion)();
    PVR_ERROR (__cdecl* GetDriveSpace)(long long *total, long long *used);
    int (__cdecl* GetNumBouquets)();
    PVR_ERROR (__cdecl* GetBouquetInfo)(const unsigned number, PVR_BOUQUET *info);
    int (__cdecl* GetNumChannels)();
    PVR_ERROR (__cdecl* GetChannelList)(PVR_CHANLIST *channels);
    PVR_ERROR (__cdecl* GetEPGForChannel)(const unsigned channel, PVR_PROGLIST *epg, time_t start, time_t end);
    PVR_ERROR (__cdecl* GetEPGNowInfo)(const unsigned channel, PVR_PROGINFO *result);
    PVR_ERROR (__cdecl* GetEPGNextInfo)(const unsigned channel, PVR_PROGINFO *result);
    PVR_ERROR (__cdecl* GetEPGDataEnd)(time_t *end);
  };

}

#endif //__PVRCLIENT_TYPES_H__
