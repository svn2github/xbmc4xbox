/*
 *      Copyright (C) 2005-2008 Team XBMC
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
// Visualisation.cpp: implementation of the CVisualisation class.
//
//////////////////////////////////////////////////////////////////////

#include "Visualisation.h"
#include "visualizations/VisualisationTypes.h"
#include "MusicInfoTag.h"
#include "Settings.h"

using namespace std;
using namespace MUSIC_INFO;

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CVisualisation::CVisualisation(struct Visualisation* pVisz, DllVisualisation* pDll, const CStdString& strVisualisationName)
    : m_pVisz(pVisz)
    , m_pDll(pDll)
    , m_strVisualisationName(strVisualisationName)
{}

CVisualisation::~CVisualisation()
{
}

void CVisualisation::Create(int posx, int posy, int width, int height)
{
  // allow vis. to create internal things needed
  // pass it the location,width,height
  // and the name of the visualisation.
  char szTmp[129];
  sprintf(szTmp, "create:%ix%i at %ix%i %s\n", width, height, posx, posy, m_strVisualisationName.c_str());
  OutputDebugString(szTmp);
  
  float pixelRatio = g_settings.m_ResInfo[g_graphicsContext.GetVideoResolution()].fPixelRatio;
  m_pVisz->Create (g_graphicsContext.Get3DDevice(), posx, posy, width, height, m_strVisualisationName.c_str(), pixelRatio);

}

void CVisualisation::Start(int iChannels, int iSamplesPerSec, int iBitsPerSample, const CStdString strSongName)
{
  // notify visz. that new song has been started
  // pass it the nr of audio channels, sample rate, bits/sample and offcourse the songname
  m_pVisz->Start(iChannels, iSamplesPerSec, iBitsPerSample, strSongName.c_str());
}

void CVisualisation::AudioData(const short* pAudioData, int iAudioDataLength, float *pFreqData, int iFreqDataLength)
{
  // pass audio data to visz.
  // audio data: is short audiodata [channel][iAudioDataLength] containing the raw audio data
  // iAudioDataLength = length of audiodata array
  // pFreqData = fft-ed audio data
  // iFreqDataLength = length of pFreqData
  m_pVisz->AudioData(const_cast<short*>(pAudioData), iAudioDataLength, pFreqData, iFreqDataLength);
}

void CVisualisation::Render()
{
  // ask visz. to render itself
  m_pVisz->Render();
}

void CVisualisation::Stop()
{
  // ask visz. to cleanup
  m_pVisz->Stop();
}


void CVisualisation::GetInfo(VIS_INFO *info)
{
  // get info from vis
  m_pVisz->GetInfo(info);
}

bool CVisualisation::OnAction(VIS_ACTION action, void *param)
{
  // see if vis wants to handle the input
  // returns false if vis doesnt want the input
  // returns true if vis handled the input
  if (action != VIS_ACTION_NONE && m_pVisz->OnAction)
  {
    // if this is a VIS_ACTION_UPDATE_TRACK action, copy relevant
    // tags from CMusicInfoTag to VisTag
    if ( action == VIS_ACTION_UPDATE_TRACK && param )
    {
      const CMusicInfoTag* tag = (const CMusicInfoTag*)param;
      VisTrack track;

      track.title       = tag->GetTitle().c_str();
      track.artist      = tag->GetArtist().c_str();
      track.album       = tag->GetAlbum().c_str();
      track.albumArtist = tag->GetAlbumArtist().c_str();
      track.genre       = tag->GetGenre().c_str();
      track.comment     = tag->GetComment().c_str();
      track.lyrics      = tag->GetLyrics().c_str();
      track.trackNumber = tag->GetTrackNumber();
      track.discNumber  = tag->GetDiscNumber();
      track.duration    = tag->GetDuration();
      track.year        = tag->GetYear();
      track.rating      = tag->GetRating();

      return m_pVisz->OnAction((int)action, (void*)(&track));
    }
    return m_pVisz->OnAction((int)action, param);
  }
  return false;
}

void CVisualisation::GetSettings(vector<VisSetting> **vecSettings)
{
  if (vecSettings) *vecSettings = NULL;
  if (m_pVisz->GetSettings)
    m_pVisz->GetSettings(vecSettings);
}

void CVisualisation::UpdateSetting(int num)
{
  if (m_pVisz->UpdateSetting)
    m_pVisz->UpdateSetting(num);
}

void CVisualisation::GetPresets(char ***pPresets, int *currentPreset, int *numPresets, bool *locked)
{
  if (m_pVisz->GetPresets)
    m_pVisz->GetPresets(pPresets, currentPreset, numPresets, locked);
}

void CVisualisation::GetCurrentPreset(char **pPreset, bool *locked)
{
  if (pPreset && locked && m_pVisz->GetPresets)
  {
    char **presets = NULL;
    int currentPreset = 0;
    int numPresets = 0;
    *locked = false;
    m_pVisz->GetPresets(&presets, &currentPreset, &numPresets, locked);
    if (presets && currentPreset < numPresets)
      *pPreset = presets[currentPreset];
  }
}

bool CVisualisation::IsLocked()
{
  char *preset;
  bool locked = false;
  GetCurrentPreset(&preset, &locked);
  return locked;
}

char *CVisualisation::GetPreset()
{
  char *preset = NULL;
  bool locked = false;
  GetCurrentPreset(&preset, &locked);
  return preset;
}
