/*!
\file GUIFontManager.h
\brief 
*/

#ifndef GUILIB_FONTMANAGER_H
#define GUILIB_FONTMANAGER_H

#pragma once

// Forward
class CGUIFont;
class CGUIFontBase;

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
  CGUIFont* LoadXPR(const CStdString& strFontName, const CStdString& strFilename, DWORD textColor, DWORD shadowColor);
  CGUIFont* LoadTTF(const CStdString& strFontName, const CStdString& strFilename, DWORD textColor, DWORD shadowColor, const int iSize, const int iStyle);
  CGUIFont* GetFont(const CStdString& strFontName, bool fallback = true);
  void Clear();
  void FreeFontFile(CGUIFontBase *pFont);

  bool IsFontSetUnicode() { return m_fontsetUnicode; }
  bool IsFontSetUnicode(const CStdString& strFontSet);
  bool GetFirstFontSetUnicode(CStdString& strFontSet);

protected:
  void LoadFonts(const TiXmlNode* fontNode);
  CGUIFontBase* GetFontFile(const CStdString& strFontFile);
  bool OpenFontFile(TiXmlDocument& xmlDoc);

  vector<CGUIFont*> m_vecFonts;
  vector<CGUIFontBase*> m_vecFontFiles;
  bool m_fontsetUnicode;
};

/*!
 \ingroup textures
 \brief 
 */
extern GUIFontManager g_fontManager;
#endif
