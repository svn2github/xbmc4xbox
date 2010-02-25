#include "URL.h"
#include "FilterSelectionRule.h"
#include "VideoInfoTag.h"
#include "StreamDetails.h"
#include "GUISettings.h"
#include "utils/log.h"

CFilterSelectionRule::CFilterSelectionRule(TiXmlElement* pRule, const CStdString &nodeName)
{
  Initialize(pRule, nodeName);
}

CFilterSelectionRule::~CFilterSelectionRule()
{}

void CFilterSelectionRule::Initialize(TiXmlElement* pRule, const CStdString &nodeName)
{
  if (! pRule)
    return;

  m_name = pRule->Attribute("name");
  if (!m_name || m_name.IsEmpty())
    m_name = "un-named";

  CLog::Log(LOGDEBUG, "CFilterSelectionRule::Initialize: creating rule: %s", m_name.c_str());

  /*m_tInternetStream = GetTristate(pRule->Attribute("internetstream"));
  m_tAudio = GetTristate(pRule->Attribute("audio"));
  m_tVideo = GetTristate(pRule->Attribute("video"));

  m_tDVD = GetTristate(pRule->Attribute("dvd"));
  m_tDVDFile = GetTristate(pRule->Attribute("dvdfile"));
  m_tDVDImage = GetTristate(pRule->Attribute("dvdimage"));*/

  //m_protocols = pRule->Attribute("protocols");
  //m_fileTypes = pRule->Attribute("filetypes");
  m_dxva = GetTristate(pRule->Attribute("dxva"));

  m_mimeTypes = pRule->Attribute("mimetypes");
  m_fileName = pRule->Attribute("filename");

  m_audioCodec = pRule->Attribute("audiocodec");
  m_audioChannels = pRule->Attribute("audiochannels");
  m_videoCodec = pRule->Attribute("videocodec");
  m_videoResolution = pRule->Attribute("videoresolution");
  m_videoAspect = pRule->Attribute("videoaspect");

  m_bStreamDetails = m_audioCodec.length() > 0 || m_audioChannels.length() > 0 ||
    m_videoCodec.length() > 0 || m_videoResolution.length() > 0 || m_videoAspect.length() > 0;

  if (m_bStreamDetails && !g_guiSettings.GetBool("myvideos.extractflags"))
  {
      CLog::Log(LOGWARNING, "CFilterSelectionRule::Initialize: rule: %s needs media flagging, which is disabled", m_name.c_str());
  }

  m_filterName = pRule->Attribute("filter");

  TiXmlElement* pSubRule = pRule->FirstChildElement(nodeName);
  while (pSubRule) 
  {
    vecSubRules.push_back(new CFilterSelectionRule(pSubRule, nodeName));
    pSubRule = pSubRule->NextSiblingElement(nodeName);
  }
}

int CFilterSelectionRule::GetTristate(const char* szValue) const
{
  if (szValue)
  {
    if (stricmp(szValue, "true") == 0) return 1;
    if (stricmp(szValue, "false") == 0) return 0;
  }
  return -1;
}

bool CFilterSelectionRule::CompileRegExp(const CStdString& str, CRegExp& regExp) const
{
  return str.length() > 0 && regExp.RegComp(str.c_str());
}

bool CFilterSelectionRule::MatchesRegExp(const CStdString& str, CRegExp& regExp) const
{
  return regExp.RegFind(str, 0) == 0;
}

void CFilterSelectionRule::GetFilters(const CFileItem& item, std::vector<CStdString> &vecCores, bool dxva)
{
  CLog::Log(LOGDEBUG, "CFilterSelectionRule::GetFilters: considering rule: %s", m_name.c_str());

  if (m_bStreamDetails && !item.HasVideoInfoTag()) return;
  /*
  if (m_tAudio >= 0 && (m_tAudio > 0) != item.IsAudio()) return;
  if (m_tVideo >= 0 && (m_tVideo > 0) != item.IsVideo()) return;
  if (m_tInternetStream >= 0 && (m_tInternetStream > 0) != item.IsInternetStream()) return;

  if (m_tDVD >= 0 && (m_tDVD > 0) != item.IsDVD()) return;
  if (m_tDVDFile >= 0 && (m_tDVDFile > 0) != item.IsDVDFile()) return;
  if (m_tDVDImage >= 0 && (m_tDVDImage > 0) != item.IsDVDImage()) return;*/

  if (m_dxva >= 0 && (m_dxva > 0) != dxva) return;

  CRegExp regExp;

  if (m_bStreamDetails)
  {
    if (!item.GetVideoInfoTag()->HasStreamDetails())
    {
      CLog::Log(LOGDEBUG, "CFilterSelectionRule::GetFilters: cannot check rule: %s, no StreamDetails", m_name.c_str());
      return;
    }

    CStreamDetails streamDetails = item.GetVideoInfoTag()->m_streamDetails;

    if (CompileRegExp(m_audioCodec, regExp) && !MatchesRegExp(streamDetails.GetAudioCodec(), regExp)) return;

    if (CompileRegExp(m_videoCodec, regExp) && !MatchesRegExp(streamDetails.GetVideoCodec(), regExp)) return;

    if (CompileRegExp(m_videoResolution, regExp) &&
        !MatchesRegExp(CStreamDetails::VideoWidthToResolutionDescription(streamDetails.GetVideoWidth()), regExp)) return;

    if (CompileRegExp(m_videoAspect, regExp) &&
        !MatchesRegExp(CStreamDetails::VideoAspectToAspectDescription(streamDetails.GetVideoAspect()),  regExp)) return;
  }

  CURL url(item.m_strPath);

  //if (CompileRegExp(m_fileTypes, regExp) && !MatchesRegExp(url.GetFileType(), regExp)) return;
  
  //if (CompileRegExp(m_protocols, regExp) && !MatchesRegExp(url.GetProtocol(), regExp)) return;
  
  if (CompileRegExp(m_mimeTypes, regExp) && !MatchesRegExp(item.GetContentType(), regExp)) return;

  if (CompileRegExp(m_fileName, regExp) && !MatchesRegExp(item.m_strPath, regExp)) return;

  CLog::Log(LOGDEBUG, "CFilterSelectionRule::GetFilters: matches rule: %s", m_name.c_str());

  for (unsigned int i = 0; i < vecSubRules.size(); i++)
    vecSubRules[i]->GetFilters(item, vecCores, dxva);
  
  if (!m_filterName.empty())
  {
    CLog::Log(LOGDEBUG, "CFilterSelectionRule::GetFilters: adding filter: %s for rule: %s", m_filterName.c_str(), m_name.c_str());
    vecCores.push_back(m_filterName);
  }
}
