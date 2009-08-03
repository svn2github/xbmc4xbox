/*!
\file GUIFont.h
\brief
*/

#ifndef CGUILIB_GUIFONTTTF_H
#define CGUILIB_GUIFONTTTF_H
#pragma once

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

// forward definition
struct FT_FaceRec_;
struct FT_LibraryRec_;
struct FT_GlyphSlotRec_;

typedef struct FT_FaceRec_ *FT_Face;
typedef struct FT_LibraryRec_ *FT_Library;
typedef struct FT_GlyphSlotRec_ *FT_GlyphSlot;

DWORD PadPow2(DWORD x);

#ifdef _LINUX
//#define max(a,b) ((a)>(b)?(a):(b))
//#define min(a,b) ((a)<(b)?(a):(b))
// NOTE: rintf is inaccurate - it appears to round to the nearest EVEN integer, rather than to the nearest integer
//       this is a useful reference for wintel platforms that may be useful:
//         http://ldesoras.free.fr/doc/articles/rounding_en.pdf
//       For now, we've dumped down to the simple (and slow?) floor(x + 0.5f)
//       Ideally we'd have a common round_int routine that does float -> float, as this is the case
//       we actually use (both here and in CGUIImage)
#define ROUND rintf
//#define RoundToPixel rintf
//#define ROUND(x) MathUtils::round_int(x)
//#define RoundToPixel(x) MathUtils::round_int(x)
//#define TRUNC_TO_PIXEL(x) MathUtils::truncate_int(x)
#else
#define ROUND(x) (float)(MathUtils::round_int(x))
#endif // _LINUX


/*!
 \ingroup textures
 \brief
 */

typedef struct _SVertex
{
  float u, v;
  unsigned char r, g, b, a;    
  float x, y, z;
} SVertex;


class CGUIFontTTFBase
{
  friend class CGUIFont;
  
public:

  CGUIFontTTFBase(const CStdString& strFileName);
  virtual ~CGUIFontTTFBase(void);

  void Clear();

  bool Load(const CStdString& strFilename, float height = 20.0f, float aspect = 1.0f, float lineSpacing = 1.0f);

  virtual float RoundToPixel(float x) = 0;
  virtual float TruncToPixel(float x) = 0;
  virtual void Begin() = 0;
  virtual void End() = 0;

  const CStdString& GetFileName() const { return m_strFileName; };
  void CopyReferenceCountFrom(CGUIFontTTFBase& ttf) { m_referenceCount = ttf.m_referenceCount; }

protected:
  struct Character
  {
    short offsetX, offsetY;
    float left, top, right, bottom;
    float advance;
    DWORD letterAndStyle;
  };
  void AddReference();
  void RemoveReference();

  float GetTextWidthInternal(std::vector<DWORD>::const_iterator start, std::vector<DWORD>::const_iterator end);
  float GetCharWidthInternal(DWORD ch);
  float GetTextHeight(float lineSpacing, int numLines) const;
  float GetLineHeight(float lineSpacing) const;

  void DrawTextInternal(float x, float y, const std::vector<DWORD> &colors, const std::vector<DWORD> &text,
                            DWORD alignment, float maxPixelWidth, bool scrolling);

  void DrawTextInternal(float x, float y, DWORD color, const std::vector<DWORD> &text,
                            DWORD alignment, float maxPixelWidth, bool scrolling)
  {
    std::vector<DWORD> colors;
    colors.push_back(color);
    DrawTextInternal(x, y, colors, text, alignment, maxPixelWidth, scrolling);
  }

  float m_height;
  CStdString m_strFilename;

  // Stuff for pre-rendering for speed
  inline Character *GetCharacter(DWORD letter);
  bool CacheCharacter(WCHAR letter, DWORD style, Character *ch);
  void RenderCharacter(float posX, float posY, const Character *ch, D3DCOLOR dwColor, bool roundX);
  void ClearCharacterCache();

  virtual XBMC::TexturePtr ReallocTexture(unsigned int& newHeight) = 0;
  virtual bool CopyCharToTexture(void* pGlyph, void* pCharacter) = 0;
  virtual void DeleteHardwareTexture() = 0;
  virtual void RenderInternal(SVertex* v) = 0;

  // modifying glyphs
  void EmboldenGlyph(FT_GlyphSlot slot);
  void ObliqueGlyph(FT_GlyphSlot slot);

  XBMC::TexturePtr m_texture;        // texture that holds our rendered characters (8bit alpha only)

  unsigned int m_textureWidth;       // width of our texture
  unsigned int m_textureHeight;      // heigth of our texture
  int m_posX;                        // current position in the texture
  int m_posY;

  DWORD m_dwColor;

  Character *m_char;                 // our characters
  Character *m_charquick[256*4];     // ascii chars (4 styles) here
  int m_maxChars;                    // size of character array (can be incremented)
  int m_numChars;                    // the current number of cached characters

  float m_ellipsesWidth;               // this is used every character (width of '.')

  unsigned int m_cellBaseLine;
  unsigned int m_cellHeight;

  DWORD m_dwNestedBeginCount;             // speedups

  // freetype stuff
  FT_Face    m_face;

  float m_originX;
  float m_originY;

  bool m_bTextureLoaded;
  unsigned int m_nTexture;

  SVertex* m_vertex;
  int      m_vertex_count;
  int      m_vertex_size;

  float    m_textureScaleX;
  float    m_textureScaleY;

  static int justification_word_weight;
  static unsigned int max_texture_size;

  CStdString m_strFileName;

private:
  int m_referenceCount;
};

#ifdef HAS_SDL_OPENGL
#include "GUIFontTTFGL.h"
#define CGUIFontTTF CGUIFontTTFGL
#elif defined(HAS_DX)
#include "GUIFontTTFDX.h"
#define CGUIFontTTF CGUIFontTTFDX
#endif

#endif
