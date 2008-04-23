/*!
\file GUIFontManager.h
\brief 
*/

#ifndef GUILIB_FONTMANAGER_H
#define GUILIB_FONTMANAGER_H

#include "GraphicContext.h"

#pragma once

// Forward
class CGUIFont;
class CGUIFontTTF;
class TiXmlDocument;
class TiXmlNode;

struct OrigFontInfo
{
   int size;
   float aspect;
   CStdString fontFilePath;
   CStdString fileName;
};

/*!
 \ingroup textures
 \brief 
 */
class GUIFontManager
{
public:
  GUIFontManager(void);
  virtual ~GUIFontManager(void);
  void Unload(const CStdString& strFontName);
  void LoadFonts(const CStdString& strFontSet);
  CGUIFont* LoadTTF(const CStdString& strFontName, const CStdString& strFilename, DWORD textColor, DWORD shadowColor, const int iSize, const int iStyle, float lineSpacing = 1.0f, float aspect = 1.0f);
  CGUIFont* GetFont(const CStdString& strFontName, bool fallback = true);
  void Clear();
  void FreeFontFile(CGUIFontTTF *pFont);

  bool IsFontSetUnicode() { return m_fontsetUnicode; }
  bool IsFontSetUnicode(const CStdString& strFontSet);
  bool GetFirstFontSetUnicode(CStdString& strFontSet);
  
  void ReloadTTFFonts(void);

protected:
  void LoadFonts(const TiXmlNode* fontNode);
  CGUIFontTTF* GetFontFile(const CStdString& strFontFile);
  bool OpenFontFile(TiXmlDocument& xmlDoc);

  std::vector<CGUIFont*> m_vecFonts;
  std::vector<CGUIFontTTF*> m_vecFontFiles;
  std::vector<OrigFontInfo> m_vecFontInfo;
  bool m_fontsetUnicode;
  RESOLUTION m_skinResolution;
};

/*!
 \ingroup textures
 \brief 
 */
extern GUIFontManager g_fontManager;
#endif
