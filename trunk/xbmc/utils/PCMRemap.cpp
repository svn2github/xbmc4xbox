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

#define __STDC_LIMIT_MACROS

#include <cstdlib>
#include <string.h>
#include <stdio.h>
#include <math.h>

#include "MathUtils.h"
#include "PCMRemap.h"
#include "utils/log.h"
#include "GUISettings.h"
#ifdef _WIN32
#include "../win32/PlatformDefs.h"
#endif

static enum PCMChannels PCMLayoutMap[PCM_MAX_LAYOUT][PCM_MAX_CH + 1] =
{
  /* 2.0 */ {PCM_FRONT_LEFT, PCM_FRONT_RIGHT, PCM_INVALID},
  /* 2.1 */ {PCM_FRONT_LEFT, PCM_FRONT_RIGHT, PCM_LOW_FREQUENCY, PCM_INVALID},
  /* 3.0 */ {PCM_FRONT_LEFT, PCM_FRONT_RIGHT, PCM_FRONT_CENTER, PCM_INVALID},
  /* 3.1 */ {PCM_FRONT_LEFT, PCM_FRONT_RIGHT, PCM_FRONT_CENTER, PCM_LOW_FREQUENCY, PCM_INVALID},
  /* 4.0 */ {PCM_FRONT_LEFT, PCM_FRONT_RIGHT, PCM_BACK_LEFT, PCM_BACK_RIGHT, PCM_INVALID},
  /* 4.1 */ {PCM_FRONT_LEFT, PCM_FRONT_RIGHT, PCM_FRONT_CENTER, PCM_LOW_FREQUENCY, PCM_INVALID},
  /* 5.0 */ {PCM_FRONT_LEFT, PCM_FRONT_RIGHT, PCM_FRONT_CENTER, PCM_BACK_LEFT, PCM_BACK_RIGHT, PCM_INVALID},
  /* 5.1 */ {PCM_FRONT_LEFT, PCM_FRONT_RIGHT, PCM_FRONT_CENTER, PCM_BACK_LEFT, PCM_BACK_RIGHT, PCM_LOW_FREQUENCY, PCM_INVALID},
  /* 7.0 */ {PCM_FRONT_LEFT, PCM_FRONT_RIGHT, PCM_FRONT_CENTER, PCM_SIDE_LEFT, PCM_SIDE_RIGHT, PCM_BACK_LEFT, PCM_BACK_RIGHT, PCM_INVALID},
  /* 7.1 */ {PCM_FRONT_LEFT, PCM_FRONT_RIGHT, PCM_FRONT_CENTER, PCM_SIDE_LEFT, PCM_SIDE_RIGHT, PCM_BACK_LEFT, PCM_BACK_RIGHT, PCM_LOW_FREQUENCY, PCM_INVALID}
};

static const char* PCMChannelName[PCM_MAX_CH] =
{
  "FL",
  "FR",
  "CE",
  "LFE",
  "BL",
  "BR",
  "FLOC",
  "FROC",
  "BC",
  "SL",
  "SR",
  "TFL",
  "TFR",
  "TFC",
  "TC",
  "TBL",
  "TBR",
  "TBC",
};

static const char* PCMLayoutName[PCM_MAX_LAYOUT] =
{
  "2.0",
  "2.1",
  "3.0",
  "3.1",
  "4.0",
  "4.1",
  "5.0",
  "5.1",
  "7.0",
  "7.1"
};

/*
  map missing output into channel @ volume level
  the order of this table is important, mix tables can not depend on channels that have not been defined yet
  eg, FC can only be mixed into FL, FR as they are the only channels that have been defined
*/
#define PCM_MAX_MIX 3
static struct PCMMapInfo PCMDownmixTable[PCM_MAX_CH][PCM_MAX_MIX] =
{
  /* PCM_FRONT_LEFT */
  {
    {PCM_INVALID}
  },
  /* PCM_FRONT_RIGHT */
  {
    {PCM_INVALID}
  },
  /* PCM_FRONT_CENTER */
  {
    {PCM_FRONT_LEFT_OF_CENTER , 1.0},
    {PCM_FRONT_RIGHT_OF_CENTER, 1.0},
    {PCM_INVALID}
  },
  /* PCM_LOW_FREQUENCY */
  {
    /*
      A/52B 7.8 paragraph 2 recomends +10db
      but due to horrible clipping when normalize
      is disabled we set this to 1.0
    */
    {PCM_FRONT_LEFT           , 1.0},//3.5}, 
    {PCM_FRONT_RIGHT          , 1.0},//3.5},
    {PCM_INVALID}
  },
  /* PCM_BACK_LEFT */
  {
    {PCM_FRONT_LEFT           , 1.0},
    {PCM_INVALID}
  },
  /* PCM_BACK_RIGHT */
  {
    {PCM_FRONT_RIGHT          , 1.0},
    {PCM_INVALID}
  },
  /* PCM_FRONT_LEFT_OF_CENTER */
  {
    {PCM_FRONT_LEFT           , 1.0},
    {PCM_FRONT_CENTER         , 1.0, true},
    {PCM_INVALID}
  },
  /* PCM_FRONT_RIGHT_OF_CENTER */
  {
    {PCM_FRONT_RIGHT          , 1.0},
    {PCM_FRONT_CENTER         , 1.0, true},
    {PCM_INVALID}
  },
  /* PCM_BACK_CENTER */
  {
    {PCM_BACK_LEFT            , 1.0},
    {PCM_BACK_RIGHT           , 1.0},
    {PCM_INVALID}
  },
  /* PCM_SIDE_LEFT */
  {
    {PCM_FRONT_LEFT           , 1.0},
    {PCM_BACK_LEFT            , 1.0},
    {PCM_INVALID}
  },
  /* PCM_SIDE_RIGHT */
  {
    {PCM_FRONT_RIGHT          , 1.0},
    {PCM_BACK_RIGHT           , 1.0},
    {PCM_INVALID}
  },
  /* PCM_TOP_FRONT_LEFT */
  {
    {PCM_FRONT_LEFT           , 1.0},
    {PCM_INVALID}
  },
  /* PCM_TOP_FRONT_RIGHT */
  {
    {PCM_FRONT_RIGHT          , 1.0},
    {PCM_INVALID}
  },
  /* PCM_TOP_FRONT_CENTER */
  {
    {PCM_TOP_FRONT_LEFT       , 1.0},
    {PCM_TOP_FRONT_RIGHT      , 1.0},
    {PCM_INVALID}
  },
  /* PCM_TOP_CENTER */
  {
    {PCM_TOP_FRONT_LEFT       , 1.0},
    {PCM_TOP_FRONT_RIGHT      , 1.0},
    {PCM_INVALID}
  },
  /* PCM_TOP_BACK_LEFT */
  {
    {PCM_BACK_LEFT            , 1.0},
    {PCM_INVALID}
  },
  /* PCM_TOP_BACK_RIGHT */
  {
    {PCM_BACK_LEFT            , 1.0},
    {PCM_INVALID}
  },
  /* PCM_TOP_BACK_CENTER */
  {
    {PCM_TOP_BACK_LEFT        , 1.0},
    {PCM_TOP_BACK_RIGHT       , 1.0},
    {PCM_INVALID}
  }
};

CPCMRemap::CPCMRemap() :
  m_inSet       (false),
  m_outSet      (false),
  m_inChannels  (0),
  m_outChannels (0),
  m_inSampleSize(0)
{
  Dispose();
}

CPCMRemap::~CPCMRemap()
{
  Dispose();
}

void CPCMRemap::Dispose()
{
}

/* resolves the channels recursively and returns the new index of tablePtr */
struct PCMMapInfo* CPCMRemap::ResolveChannel(enum PCMChannels channel, float level, bool ifExists, std::vector<enum PCMChannels> path, struct PCMMapInfo *tablePtr)
{
  if (channel == PCM_INVALID) return tablePtr;

  /* if its a 1 to 1 mapping, return */
  if (m_useable[channel])
  {
    tablePtr->channel = channel;
    tablePtr->level   = level;

    ++tablePtr;
    tablePtr->channel = PCM_INVALID;
    return tablePtr;
  } else
    if (ifExists)
      level /= 2;

  struct PCMMapInfo *info;
  std::vector<enum PCMChannels>::iterator itt;

  for(info = PCMDownmixTable[channel]; info->channel != PCM_INVALID; ++info)
  {
    /* make sure we are not about to recurse into ourself */
    bool found = false;
    for(itt = path.begin(); itt != path.end(); ++itt)
      if (*itt == info->channel)
      {
        found = true;
        break;
      }

    if (found)
      continue;

    path.push_back(channel);
    float  l = (info->level * (level / 100)) * 100;
    tablePtr = ResolveChannel(info->channel, l, info->ifExists, path, tablePtr);
    path.pop_back();
  }

  return tablePtr;
}

/*
  builds a lookup table to convert from the input mapping to the output
  mapping, this decreases the amount of work per sample to remap it.
*/
void CPCMRemap::BuildMap()
{
  if (!m_inSet || !m_outSet) return;

  unsigned int in_ch, out_ch;

  m_inStride  = m_inSampleSize * m_inChannels ;
  m_outStride = m_inSampleSize * m_outChannels;

  /* figure out what channels we have and can use */
  memset(m_useable, 0, sizeof(m_useable));
  for(enum PCMChannels *chan = PCMLayoutMap[m_channelLayout]; *chan != PCM_INVALID; ++chan)
  {
    for(out_ch = 0; out_ch < m_outChannels; ++out_ch)
      if (m_outMap[out_ch] == *chan)
      {
        m_useable[*chan] = true;
        break;
      }
  }

  /* see if we need to normalize the levels */
  bool dontnormalize = g_guiSettings.GetBool("audiooutput.dontnormalizelevels");
  CLog::Log(LOGDEBUG, "CPCMRemap: Downmix normalization is %s", (dontnormalize ? "disabled" : "enabled"));

  /* resolve all the channels */
  struct PCMMapInfo table[PCM_MAX_CH + 1], *info, *dst;
  std::vector<enum PCMChannels> path;
  int counts[PCM_MAX_CH];

  memset(m_lookupMap, PCM_INVALID, sizeof(m_lookupMap));
  memset(counts, 0, sizeof(counts));
  for(in_ch = 0; in_ch < m_inChannels; ++in_ch) {
    memset(table, PCM_INVALID, sizeof(table));
    ResolveChannel(m_inMap[in_ch], 1.0f, false, path, table);
    for(info = table; info->channel != PCM_INVALID; ++info)
    {
      /* find the end of the table */
      for(dst = m_lookupMap[info->channel]; dst->channel != PCM_INVALID; ++dst);

      /* append it to the table and set its input offset */
      dst->channel   = m_inMap[in_ch];
      dst->in_offset = in_ch * 2;
      dst->level     = info->level;
      counts[dst->channel]++;
    }
  }

  /* convert the levels into RMS values */
  for(out_ch = 0; out_ch < m_outChannels; ++out_ch)
  {
    float scale = 0;
    int count = 0;
    for(dst = m_lookupMap[m_outMap[out_ch]]; dst->channel != PCM_INVALID; ++dst)
    {
      dst->copy  = false;
      dst->level = dst->level / sqrt((float)counts[dst->channel]);
      scale     += dst->level;
      ++count;
    }

    /* if there is only 1 channel to mix, and the level is 1.0, then just copy the channel */
    dst = m_lookupMap[m_outMap[out_ch]];
    if (count == 1 && dst->level > 0.99 && dst->level < 1.01)
      dst->copy = true;
    
    /* normalize the levels if it is turned on */
    if (!dontnormalize)
      for(dst = m_lookupMap[m_outMap[out_ch]]; dst->channel != PCM_INVALID; ++dst)
        dst->level /= scale;
  }
  
  /* output the final map for debugging */
  for(out_ch = 0; out_ch < m_outChannels; ++out_ch)
  {
    CStdString s = "", f;
    for(dst = m_lookupMap[m_outMap[out_ch]]; dst->channel != PCM_INVALID; ++dst)
    {
      f.Format("%s(%f%s) ",  PCMChannelName[dst->channel], dst->level, dst->copy ? "*" : "");
      s += f;
    }
    CLog::Log(LOGDEBUG, "CPCMRemap: %s = %s\n", PCMChannelName[m_outMap[out_ch]], s.c_str());
  }
}

void CPCMRemap::DumpMap(CStdString info, unsigned int channels, enum PCMChannels *channelMap)
{
  if (channelMap == NULL)
  {
    CLog::Log(LOGINFO, "CPCMRemap: %s channel map: NULL", info.c_str());
    return;
  }

  CStdString mapping = "";
  for(unsigned int i = 0; i < channels; ++i)
    mapping += ((i == 0) ? "" : ",") + (CStdString)PCMChannelName[channelMap[i]];

  CLog::Log(LOGINFO, "CPCMRemap: %s channel map: %s\n", info.c_str(), mapping.c_str());
}

void CPCMRemap::Reset()
{
  m_inMap  = NULL;
  m_outMap = NULL;
  m_inSet  = false;
  m_outSet = false;
  Dispose();
}

/* sets the input format, and returns the requested channel layout */
enum PCMChannels *CPCMRemap::SetInputFormat(unsigned int channels, enum PCMChannels *channelMap, unsigned int sampleSize)
{
  m_inChannels   = channels;
  m_inMap        = channelMap;
  m_inSampleSize = sampleSize;
  m_inSet        = channelMap != NULL;

  /* fix me later */
  assert(sampleSize == 2);

  /* get the audio layout, and count the channels in it */
  m_channelLayout  = (enum PCMLayout)g_guiSettings.GetInt("audiooutput.channellayout");
  if (m_channelLayout >= PCM_MAX_LAYOUT) m_channelLayout = PCM_LAYOUT_2_0;

  CLog::Log(LOGINFO, "CPCMRemap: Channel Layout: %s\n", PCMLayoutName[m_channelLayout]);
  m_layoutMap      = PCMLayoutMap[m_channelLayout];

  DumpMap("I", channels, channelMap);
  BuildMap();

  return m_layoutMap;
}

/* sets the output format supported by the audio renderer */
void CPCMRemap::SetOutputFormat(unsigned int channels, enum PCMChannels *channelMap)
{
  m_outChannels   = channels;
  m_outMap        = channelMap;
  m_outSet        = channelMap != NULL;

  DumpMap("O", channels, channelMap);
  BuildMap();
}

/* remap the supplied data into out, which must be pre-allocated */
void CPCMRemap::Remap(void *data, void *out, unsigned int samples)
{
  unsigned int i, ch;
  uint8_t      *insample, *outsample;
  uint8_t      *src, *dst;

  insample  = (uint8_t*)data;
  outsample = (uint8_t*)out;

  /*
    the output may have channels the input does not have, so zero the data
    to stop random data being sent to them.
  */
  memset(out, 0, samples * (m_inSampleSize * m_outChannels));
  for(i = 0; i < samples; ++i)
  {
    for(ch = 0; ch < m_outChannels; ++ch)
    {
      struct PCMMapInfo *info;
      float value = 0;

      info = m_lookupMap[m_outMap[ch]];
      if (info->channel == PCM_INVALID) continue;

      /* if it is a 1-1 map, we just copy the data to avoid rounding errors */
      if (info->copy)
      {
        src = insample  + info->in_offset;
        dst = outsample + ch * m_inSampleSize;
        *(int16_t*)dst = *(int16_t*)src;
        continue;
      }

      for(; info->channel != PCM_INVALID; ++info)
      {
        src    = insample + info->in_offset;
        value += (float)(*(int16_t*)src) * info->level;
      }
      dst = outsample + ch * m_inSampleSize;

      //convert to signed int and clamp to 16 bit
      int outvalue = MathUtils::round_int(value);
      if (outvalue > INT16_MAX)
        outvalue = INT16_MAX;
      else if (outvalue < INT16_MIN)
        outvalue = INT16_MIN;

      *(int16_t*)dst = outvalue;
    }

    insample  += m_inStride;
    outsample += m_outStride;
  }
}

bool CPCMRemap::CanRemap()
{
  return (m_inSet && m_outSet);
}

int CPCMRemap::InBytesToFrames(int bytes)
{
  return bytes / m_inSampleSize / m_inChannels;
}

int CPCMRemap::FramesToOutBytes(int frames)
{
  return frames * m_inSampleSize * m_outChannels;
}

int CPCMRemap::FramesToInBytes(int frames)
{
  return frames * m_inSampleSize * m_inChannels;
}

