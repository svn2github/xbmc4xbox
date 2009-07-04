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

#include "stdafx.h"
#include <math.h>
#include "StreamDetails.h"

void CStreamDetail::Serialize(CArchive &ar)
{
  // there's nothing to do here, the type is stored externally and parent isn't stored
}

CStreamDetailVideo::CStreamDetailVideo() :
  CStreamDetail(CStreamDetail::VIDEO), m_iWidth(0), m_iHeight(0), m_fAspect(0.0)
{
}

void CStreamDetailVideo::Serialize(CArchive& ar)
{
  CStreamDetail::Serialize(ar);
  if (ar.IsStoring())
  {
    ar << m_strCodec;
    ar << m_fAspect;
    ar << m_iHeight;
    ar << m_iWidth;
  }
  else
  {
    ar >> m_strCodec;
    ar >> m_fAspect;
    ar >> m_iHeight;
    ar >> m_iWidth;
  }
}

bool CStreamDetailVideo::IsWorseThan(CStreamDetail *that)
{
  if (that->m_eType != CStreamDetail::VIDEO)
    return true;

  // Best video stream is that with the most pixels
  CStreamDetailVideo *sdv = (CStreamDetailVideo *)that;
  return (sdv->m_iWidth * sdv->m_iHeight) > (m_iWidth * m_iHeight);
}

CStreamDetailAudio::CStreamDetailAudio() :
  CStreamDetail(CStreamDetail::AUDIO), m_iChannels(-1)
{
}

void CStreamDetailAudio::Serialize(CArchive& ar)
{
  CStreamDetail::Serialize(ar);
  if (ar.IsStoring())
  {
    ar << m_strCodec;
    ar << m_strLanguage;
    ar << m_iChannels;
  }
  else
  {
    ar >> m_strCodec;
    ar >> m_strLanguage;
    ar >> m_iChannels;
  }
}

int CStreamDetailAudio::GetCodecPriority() const
{
  if (m_strCodec == "eac3")
    return 3;
  if (m_strCodec == "dca")
    return 2;
  if (m_strCodec == "ac3")
    return 1;
  return 0;
}

bool CStreamDetailAudio::IsWorseThan(CStreamDetail *that)
{
  if (that->m_eType != CStreamDetail::AUDIO)
    return true;

  CStreamDetailAudio *sda = (CStreamDetailAudio *)that;
  // First choice is the thing with the most channels
  if (sda->m_iChannels > m_iChannels)
    return true;
  if (m_iChannels > sda->m_iChannels)
    return false;

  // In case of a tie, eac3 > dts > ac3 > all else.
  return sda->GetCodecPriority() > GetCodecPriority();
}

CStreamDetailSubtitle::CStreamDetailSubtitle() :
  CStreamDetail(CStreamDetail::SUBTITLE)
{
}

void CStreamDetailSubtitle::Serialize(CArchive& ar)
{
  CStreamDetail::Serialize(ar);
  if (ar.IsStoring())
  {
    ar << m_strLanguage;
  }
  else
  {
    ar >> m_strLanguage;
  }
}

bool CStreamDetailSubtitle::IsWorseThan(CStreamDetail *that)
{
  if (that->m_eType != CStreamDetail::SUBTITLE)
    return true;

  // the preferred subtitle should be the one in the user's language
  if (m_pParent)
  {
    if (m_pParent->m_strLanguage == m_strLanguage)
      return false;  // already the best
    else
      return (m_pParent->m_strLanguage == ((CStreamDetailSubtitle *)that)->m_strLanguage);
  }
  return false;
}

CStreamDetails& CStreamDetails::operator=(const CStreamDetails &that)
{
  if (this != &that)
  {
    Reset();
    std::vector<CStreamDetail *>::const_iterator iter;
    for (iter = that.m_vecItems.begin(); iter != that.m_vecItems.end(); iter++)
    {
      switch ((*iter)->m_eType) 
      {
      case CStreamDetail::VIDEO:
        AddStream(new CStreamDetailVideo((const CStreamDetailVideo &)(**iter)));
        break;
      case CStreamDetail::AUDIO:
        AddStream(new CStreamDetailAudio((const CStreamDetailAudio &)(**iter)));
        break;
      case CStreamDetail::SUBTITLE:
        AddStream(new CStreamDetailSubtitle((const CStreamDetailSubtitle &)(**iter)));
        break;
      }
    }
  
    DetermineBestStreams();
  }  /* if this != that */

  return *this;
}

CStreamDetail *CStreamDetails::NewStream(CStreamDetail::StreamType type)
{
  CStreamDetail *retVal = NULL;
  switch (type)
  {
  case CStreamDetail::VIDEO:
    retVal = new CStreamDetailVideo();
    break;
  case CStreamDetail::AUDIO:
    retVal = new CStreamDetailAudio();
    break;
  case CStreamDetail::SUBTITLE:
    retVal = new CStreamDetailSubtitle();
    break;
  }

  if (retVal)
    AddStream(retVal);

  return retVal;
}

const int CStreamDetails::GetStreamCount(CStreamDetail::StreamType type) const
{
  int retVal = 0;
  std::vector<CStreamDetail *>::const_iterator iter;
  for (iter = m_vecItems.begin(); iter != m_vecItems.end(); iter++)
    if ((*iter)->m_eType == type)
      retVal++;
  return retVal;
}

const int CStreamDetails::GetVideoStreamCount(void) const
{
  return GetStreamCount(CStreamDetail::VIDEO);
}

const int CStreamDetails::GetAudioStreamCount(void) const
{
  return GetStreamCount(CStreamDetail::AUDIO);
}

const int CStreamDetails::GetSubtitleStreamCount(void) const
{
  return GetStreamCount(CStreamDetail::SUBTITLE);
}

CStreamDetails::CStreamDetails(const CStreamDetails &that)
{
  *this = that;
}

void CStreamDetails::AddStream(CStreamDetail *item)
{
  item->m_pParent = this;
  m_vecItems.push_back(item);
}

void CStreamDetails::Reset(void)
{
  m_pBestVideo = NULL;
  m_pBestAudio = NULL;
  m_pBestSubtitle = NULL;

  std::vector<CStreamDetail *>::iterator iter;
  for (iter = m_vecItems.begin(); iter != m_vecItems.end(); iter++)
    delete *iter;
  m_vecItems.clear();
}

const CStreamDetail* CStreamDetails::GetNthStream(CStreamDetail::StreamType type, int idx) const
{
  if (idx == 0)
  {
    switch (type)
    {
    case CStreamDetail::VIDEO:
      return m_pBestVideo; 
      break;
    case CStreamDetail::AUDIO:
      return m_pBestAudio; 
      break;
    case CStreamDetail::SUBTITLE:
      return m_pBestSubtitle; 
      break;
    default:
      return NULL;
      break;
    }
  }

  std::vector<CStreamDetail *>::const_iterator iter;
  for (iter = m_vecItems.begin(); iter != m_vecItems.end(); iter++)
    if ((*iter)->m_eType == type)
    {
      idx--;
      if (idx < 1)
        return *iter;
    }

  return NULL;
}

CStdString CStreamDetails::GetVideoCodec(int idx) const
{
  CStreamDetailVideo *item = (CStreamDetailVideo *)GetNthStream(CStreamDetail::VIDEO, idx);
  if (item)
    return item->m_strCodec;
  else
    return "";
}

float CStreamDetails::GetVideoAspect(int idx) const
{
  CStreamDetailVideo *item = (CStreamDetailVideo *)GetNthStream(CStreamDetail::VIDEO, idx);
  if (item)
    return item->m_fAspect;
  else
    return 0.0;
}

int CStreamDetails::GetVideoWidth(int idx) const
{
  CStreamDetailVideo *item = (CStreamDetailVideo *)GetNthStream(CStreamDetail::VIDEO, idx);
  if (item)
    return item->m_iWidth;
  else
    return 0;
}

int CStreamDetails::GetVideoHeight(int idx) const
{
  CStreamDetailVideo *item = (CStreamDetailVideo *)GetNthStream(CStreamDetail::VIDEO, idx);
  if (item)
    return item->m_iHeight;
  else
    return 0;
}

CStdString CStreamDetails::GetAudioCodec(int idx) const
{
  CStreamDetailAudio *item = (CStreamDetailAudio *)GetNthStream(CStreamDetail::AUDIO, idx);
  if (item)
    return item->m_strCodec;
  else
    return "";
}

CStdString CStreamDetails::GetAudioLanguage(int idx) const
{
  CStreamDetailAudio *item = (CStreamDetailAudio *)GetNthStream(CStreamDetail::AUDIO, idx);
  if (item)
    return item->m_strLanguage;
  else
    return "";
}

int CStreamDetails::GetAudioChannels(int idx) const
{
  CStreamDetailAudio *item = (CStreamDetailAudio *)GetNthStream(CStreamDetail::AUDIO, idx);
  if (item)
    return item->m_iChannels;
  else
    return -1;
}

CStdString CStreamDetails::GetSubtitleLanguage(int idx) const
{
  CStreamDetailSubtitle *item = (CStreamDetailSubtitle *)GetNthStream(CStreamDetail::SUBTITLE, idx);
  if (item)
    return item->m_strLanguage;
  else
    return "";
}

void CStreamDetails::Serialize(CArchive& ar)
{
  if (ar.IsStoring())
  {
    ar << (int)m_vecItems.size();

    std::vector<CStreamDetail *>::const_iterator iter;
    for (iter = m_vecItems.begin(); iter != m_vecItems.end(); iter++)
    {
      // the type goes before the actual item.  When loading we need
      // to know the type before we can construct an instance to serialize
      ar << (int)(*iter)->m_eType;
      ar << (**iter);
    }
  }
  else
  {
    int count;
    ar >> count;

    Reset();
    for (int i=0; i<count; i++)
    {
      int type;
      CStreamDetail *p = NULL;

      ar >> type;
      p = NewStream(CStreamDetail::StreamType(type));
      if (p)
        ar >> (*p);
    }

    DetermineBestStreams();
  }
}

void CStreamDetails::DetermineBestStreams(void)
{
  m_pBestVideo = NULL;
  m_pBestAudio = NULL;
  m_pBestSubtitle = NULL;

  std::vector<CStreamDetail *>::const_iterator iter;
  for (iter = m_vecItems.begin(); iter != m_vecItems.end(); iter++)
  {
    CStreamDetail **champion;
    switch ((*iter)->m_eType)
    {
    case CStreamDetail::VIDEO:
      champion = (CStreamDetail **)&m_pBestVideo; 
      break;
    case CStreamDetail::AUDIO:
      champion = (CStreamDetail **)&m_pBestAudio; 
      break;
    case CStreamDetail::SUBTITLE:
      champion = (CStreamDetail **)&m_pBestSubtitle; 
      break;
    default:
      champion = NULL;
    }  /* switch type */

    if (!champion)
      continue;

    if ((*champion == NULL) || (*champion)->IsWorseThan(*iter))
      *champion = *iter;
  }  /* for each */
}

const float VIDEOASPECT_EPSILON = 0.025f;

CStdString CStreamDetails::VideoWidthToResolutionDescription(int iWidth)
{
  if (iWidth == 0)
    return "";

  else if (iWidth < 721)
    return "480";
  // 960x540
  else if (iWidth < 961)
    return "540";
  // 1280x720
  else if (iWidth < 1281)
    return "720";
  // 1920x1080
  else 
    return "1080";
}

CStdString CStreamDetails::VideoAspectToAspectDescription(float fAspect)
{
  if (fAspect == 0.0f)
    return "";

  // With the epsilon method some of the ranges slightly overlap
  // so go in increasing size order to minimize the impact
  // of a growing tolerance value
  float fTolerance = (fAspect * VIDEOASPECT_EPSILON);

  // 4:3 video standard
  if (fabs(fAspect - 1.33f) < fTolerance)
    return "1.33";
  // 1.66:1 35mm European flat
  if (fabs(fAspect - 1.66f) < fTolerance)
    return "1.66";
  // 16:9 video widescreen 
  if (fabs(fAspect - 1.77f) < fTolerance)
    return "1.78";
  // 1.85:1 35mm US flat (theatrical widescreen)
  if (fabs(fAspect - 1.85f) < fTolerance)
    return "1.85";
  // 2.20:1 70m standard
  if (fabs(fAspect - 2.20f) < fTolerance)
    return "2.20";
  // 2.35:1 anamorphic wide - included are both true 2.35 (pre 1970s) and new
  // 2.39 as the industry convetion is to call the new standard 2.35 anyway
  if (fabs(fAspect - 2.35f) < fTolerance)
    return "2.35";
  if (fabs(fAspect - 2.39f) < fTolerance)
    return "2.35";

  return "";
}
