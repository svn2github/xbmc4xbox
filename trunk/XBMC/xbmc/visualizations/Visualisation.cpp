#include "../stdafx.h" 
// Visualisation.cpp: implementation of the CVisualisation class.
//
//////////////////////////////////////////////////////////////////////

#include "../application.h"
#include "Visualisation.h" 
//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CVisualisation::CVisualisation(struct Visualisation* pVisz, DllLoader* pLoader, const CStdString& strVisualisationName)
    : m_pLoader(pLoader)
    , m_pVisz(pVisz)
    , m_strVisualisationName(strVisualisationName)
{}

CVisualisation::~CVisualisation()
{
  g_application.m_CdgParser.FreeGraphics();
}

void CVisualisation::Create(int posx, int posy, int width, int height)
{
  // allow vis. to create internal things needed
  // pass it the location,width,height
  // and the name of the visualisation.
  char szTmp[129];
  sprintf(szTmp, "create:%ix%i at %ix%i %s\n", width, height, posx, posy, m_strVisualisationName.c_str());
  OutputDebugString(szTmp);
  m_pVisz->Create (g_graphicsContext.Get3DDevice(), posx, posy, width, height, m_strVisualisationName.c_str());
  if (g_guiSettings.GetBool("Karaoke.Enabled"))
    g_application.m_CdgParser.AllocGraphics();
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
  if (g_guiSettings.GetBool("Karaoke.Enabled"))
    g_application.m_CdgParser.Render();
  // CLog::Log(LOGERROR, "Test");
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

bool CVisualisation::OnAction(long flags) 
{
  // see if vis wants to handle the input
  // returns false if vis doesnt want the input
  // returns true if vis handled the input
  if (m_pVisz->OnAction)
    return m_pVisz->OnAction(flags);
  return false;
}