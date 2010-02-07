/*
 *      Copyright (C) 2005-2010 Team XBMC
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

#include <vector>
#include <stdio.h>

using namespace std;

#include "epg.h"
#include "utils.h"
#include "client.h"

//Copied from PVREpg.h:
//subtypes derived from English strings.xml and cPVREPGInfoTag::ConvertGenreIdToString
//TODO: Finish me... This list is not yet complete
#define EVCONTENTMASK_MOVIEDRAMA                     0x10
//Subtypes MOVIE/DRAMA
#define DETECTIVE_THRILLER                           0x01
#define ADVENTURE_WESTERN_WAR                        0x02
#define SF_FANTASY_HORROR                            0x03
#define COMEDY                                       0x04
#define SOAP_MELODRAMA_FOLKLORIC                     0x05
#define ROMANCE                                      0x06
#define SERIOUS_CLASSICAL_RELIGIOUS_HISTORICAL_DRAMA 0x07
#define ADULTMOVIE_DRAMA                             0x08

#define EVCONTENTMASK_NEWSCURRENTAFFAIRS             0x20
//subtypes:
#define NEWS_WEATHER_REPORT                          0x01
#define NEWS_MAGAZINE                                0x02
#define DOCUMENTARY                                  0x03
#define DISCUSSION_INTERVIEW_DEBATE                  0x04

#define EVCONTENTMASK_SHOW                           0x30
//subtypes:
#define GAMESHOW_QUIZ_CONTEST                        0x01
#define VARIETY_SHOW                                 0x02
#define TALK_SHOW                                    0x03

#define EVCONTENTMASK_SPORTS                         0x40

#define EVCONTENTMASK_CHILDRENYOUTH                  0x50
//subtypes
#define PRESCHOOL_CHILD_PROGRAM                      0x01
#define ENTERTAINMENT_6TO14                          0x02
#define ENTERTAINMENT_10TO16                         0x03
#define INFO_EDUC_SCHOOL_PROGRAM                     0x04
#define CARTOONS_PUPPETS                             0x05

#define EVCONTENTMASK_MUSICBALLETDANCE               0x60
#define EVCONTENTMASK_ARTSCULTURE                    0x70
//subtypes
#define PERFORMING_ARTS                              0x01
#define FINE_ARTS                                    0x02
#define RELIGION                                     0x03
#define POP_CULTURE_TRAD_ARTS                        0x04
#define LITERATURE                                   0x05
#define FILM_CINEMA                                  0x06
#define EXP_FILM_VIDEO                               0x07
#define BROADCASTING_PRESS                           0x08
#define NEW_MEDIA                                    0x09
#define ARTS_CULTURE_MAGAZINES                       0x10
#define FASHION                                      0x11

#define EVCONTENTMASK_SOCIALPOLITICALECONOMICS       0x80
//subtype
#define MAGAZINES_REPORTS_DOCUMENTARY                0x01
#define ECONOMICS_SOCIAL_ADVISORY                    0x02
#define REMARKABLE_PEOPLE                            0x03

#define EVCONTENTMASK_EDUCATIONALSCIENCE             0x90
//subtypes
#define NATURE_ANIMALS_ENVIRONMENT                   0x01
#define TECHNOLOGY_NATURAL_SCIENCES                  0x02
#define MEDICINE_PHYSIOLOGY_PSYCHOLOGY               0x03
#define FOREIGN_COUNTRIES_EXPEDITIONS                0x04
#define SOCIAL_SPIRITUAL_SCIENCES                    0x05
#define FURTHER_EDUCATION                            0x06
#define LANGUAGES                                    0x07

#define EVCONTENTMASK_LEISUREHOBBIES                 0xA0
#define EVCONTENTMASK_SPECIAL                        0xB0
#define EVCONTENTMASK_USERDEFINED                    0xF0

cEpg::cEpg()
{
  m_uid             = 0;
  m_StartTime       = 0;
  m_EndTime         = 0;
  m_Duration        = 0;
  m_genre_type      = 0;
  m_genre_subtype   = 0;
  m_UTCdiff = GetUTCdifftime();
}

cEpg::~cEpg()
{
}

void cEpg::Reset()
{
  m_genre.clear();
  m_title.clear();
  m_shortText.clear();
  m_description.clear();

  m_StartTime       = 0;
  m_EndTime         = 0;
  m_Duration        = 0;
  m_genre_type      = 0;
  m_genre_subtype   = 0;
  m_uid             = 0;
}

bool cEpg::ParseLine(string& data)
{
  struct tm timeinfo;
  int year, month ,day;
  int hour, minute, second;
  int count;

  try
  {
    vector<string> epgfields;
    
    Tokenize(data, epgfields, "|");

    if( epgfields.size() == 5 )
    {
      XBMC_log(LOG_DEBUG, "%s: %s", epgfields[0].c_str(), epgfields[2].c_str());
      // field 0 = start date + time
      // field 1 = end   date + time
      // field 2 = title
      // field 3 = description
      // field 4 = genre string

      count = sscanf(epgfields[0].c_str(), "%d-%d-%d %d:%d:%d", &year, &month, &day, &hour, &minute, &second);

      if( count != 6)
        return false;

      timeinfo.tm_hour = hour;
      timeinfo.tm_min = minute;
      timeinfo.tm_sec = second;
      timeinfo.tm_year = year - 1900;
      timeinfo.tm_mon = month - 1;
      timeinfo.tm_mday = day;
      // Make the other fields empty:
      timeinfo.tm_isdst = 0;
      timeinfo.tm_wday = 0;
      timeinfo.tm_yday = 0;

      m_StartTime = mktime (&timeinfo);// + m_UTCdiff; //m_StartTime should be localtime, MP TV returns UTC

      if(m_StartTime < 0)
      {
        XBMC_log(LOG_ERROR, "cEpg::ParseLine: Unable to convert start time '%s' into date+time", epgfields[0].c_str());
        return false;
      }

      count = sscanf(epgfields[1].c_str(), "%d-%d-%d %d:%d:%d", &year, &month, &day, &hour, &minute, &second);

      if(count != 6)
        return false;

      timeinfo.tm_hour = hour;
      timeinfo.tm_min = minute;
      timeinfo.tm_sec = second;
      timeinfo.tm_year = year - 1900;
      timeinfo.tm_mon = month - 1;
      timeinfo.tm_mday = day;
      // Make the other fields empty:
      timeinfo.tm_isdst = 0;
      timeinfo.tm_wday = 0;
      timeinfo.tm_yday = 0;

      m_EndTime = mktime (&timeinfo);// + m_UTCdiff; //m_EndTime should be localtime, MP TV returns UTC

      if( m_EndTime < 0)
      {
        XBMC_log(LOG_ERROR, "cEpg::ParseLine: Unable to convert end time '%s' into date+time", epgfields[0].c_str());
        return false;
      }

      m_Duration  = m_EndTime - m_StartTime;
      m_uid       = 0;
      
      m_title = epgfields[2];
      m_description = epgfields[3];
      m_shortText = epgfields[2];
      SetGenre(epgfields[4], 0, 0);

      return true;
    }
  }
  catch(std::exception &e)
  {
    XBMC_log(LOG_ERROR, "Exception '%s' during parse EPG data string.", e.what());
  }

  return false;
}

void cEpg::SetGenre(string& Genre, int genreType, int genreSubType)
{
  //TODO: The xmltv plugin may return genre strings in local language
  //      The only way to solve this at the XMBC side is to transfer the
  //      genre string to XBMC or to let this plugin (or the TVServerXBMC
  //      plugin) translate it into XBMC compatible (numbered) genre types
  m_genre = Genre;
  m_genre_subtype = 0;

  if(m_bReadGenre && m_genre.length() > 0) {

    if(m_genre.compare("news/current affairs (general)") == 0) {
      m_genre_type = EVCONTENTMASK_NEWSCURRENTAFFAIRS;
    } else if (m_genre.compare("magazines/reports/documentary") == 0) {
      m_genre_type = EVCONTENTMASK_SOCIALPOLITICALECONOMICS;
      m_genre_subtype = MAGAZINES_REPORTS_DOCUMENTARY;
    } else if (m_genre.compare("sports (general)") == 0) {
      m_genre_type = EVCONTENTMASK_SPORTS;
    } else if (m_genre.compare("arts/culture (without music, general)") == 0) {
      m_genre_type = EVCONTENTMASK_ARTSCULTURE;
    } else if (m_genre.compare("childrens's/youth program (general)") == 0) {
      m_genre_type = EVCONTENTMASK_CHILDRENYOUTH;
    } else if (m_genre.compare("show/game show (general)") == 0) {
      m_genre_type = EVCONTENTMASK_SHOW;
    } else if (m_genre.compare("detective/thriller") == 0) {
      m_genre_type = EVCONTENTMASK_MOVIEDRAMA;
      m_genre_subtype = DETECTIVE_THRILLER;
    } else if (m_genre.compare("religion") == 0) {
      m_genre_type = EVCONTENTMASK_ARTSCULTURE;
      m_genre_subtype = RELIGION; 
    } else if (m_genre.compare("documentary") == 0) {
      m_genre_type = EVCONTENTMASK_NEWSCURRENTAFFAIRS;
      m_genre_subtype = DOCUMENTARY;
    } else if (m_genre.compare("education/science/factual topics (general)") == 0) {
      m_genre_type = EVCONTENTMASK_EDUCATIONALSCIENCE;
    } else if (m_genre.compare("comedy") == 0) {
      m_genre_type = EVCONTENTMASK_MOVIEDRAMA;
      m_genre_subtype = COMEDY;
    } else if (m_genre.compare("soap/melodram/folkloric") == 0) {
      m_genre_type = EVCONTENTMASK_MOVIEDRAMA;
      m_genre_subtype = SOAP_MELODRAMA_FOLKLORIC;
    } else if (m_genre.compare("cartoon/puppets") == 0) {
      m_genre_type = EVCONTENTMASK_CHILDRENYOUTH;
      m_genre_subtype = CARTOONS_PUPPETS;
    } else if (m_genre.compare("movie/drama (general)") == 0) {
      m_genre_type = EVCONTENTMASK_MOVIEDRAMA;
    } else if (m_genre.compare("nature/animals/environment") == 0) {
      m_genre_type = EVCONTENTMASK_EDUCATIONALSCIENCE;
      m_genre_subtype = NATURE_ANIMALS_ENVIRONMENT;
    } else if (m_genre.compare("adult movie/drama") == 0) {
      m_genre_type = EVCONTENTMASK_MOVIEDRAMA;
      m_genre_subtype = ADULTMOVIE_DRAMA;
    } else if (m_genre.compare("music/ballet/dance (general)") == 0) {
      m_genre_type = EVCONTENTMASK_MUSICBALLETDANCE;
    } else {
      //XBMC_log(LOG_DEBUG, "epg::setgenre: TODO mapping of MPTV's '%s' genre.", Genre.c_str());
      m_genre_type     = genreType;
      m_genre_subtype  = genreSubType;
    }
  } else {
    m_genre_type = 0;
  }
}
