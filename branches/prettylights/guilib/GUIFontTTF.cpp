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

#include "GUIFont.h"
#include "GUIFontTTF.h"
#include "GUIFontManager.h"
#include "Texture.h"
#include "GraphicContext.h"
#include "FileSystem/SpecialProtocol.h"
#include "MathUtils.h"
#include "utils/log.h"

#include <math.h>

// stuff for freetype
#ifndef _LINUX
#include "ft2build.h"
#else
#include <ft2build.h>
#endif
#include FT_FREETYPE_H
#include FT_GLYPH_H
#include FT_OUTLINE_H

#define USE_RELEASE_LIBS

using namespace std;



// our free type library (debug)
#if defined(_DEBUG) && !defined(USE_RELEASE_LIBS)
  #pragma comment (lib,"../../guilib/freetype2/freetype239ST_D.lib")
#elif !defined(__GNUC__)
  #pragma comment (lib,"../../guilib/freetype2/freetype239ST.lib")
#endif


#define CHARS_PER_TEXTURE_LINE 20 // number of characters to cache per texture line
#define CHAR_CHUNK    64      // 64 chars allocated at a time (1024 bytes)

int CGUIFontTTFBase::justification_word_weight = 6;   // weight of word spacing over letter spacing when justifying.
                                                  // A larger number means more of the "dead space" is placed between
                                                  // words rather than between letters.

unsigned int CGUIFontTTFBase::max_texture_size = 2048;         // max texture size - 2048 for GMA965

class CFreeTypeLibrary
{
public:
  CFreeTypeLibrary()
  {
    m_library = NULL;
  }

  virtual ~CFreeTypeLibrary()
  {
    if (m_library)
      FT_Done_FreeType(m_library);
  }

  FT_Face GetFont(const CStdString &filename, float size, float aspect)
  {
    // don't have it yet - create it
    if (!m_library)
      FT_Init_FreeType(&m_library);
    if (!m_library)
    {
      CLog::Log(LOGERROR, "Unable to initialize freetype library");
      return NULL;
    }

    FT_Face face;

    // ok, now load the font face
    if (FT_New_Face( m_library, _P(filename).c_str(), 0, &face ))
      return NULL;

    unsigned int ydpi = GetDPI();
    unsigned int xdpi = (unsigned int)MathUtils::round_int(ydpi * aspect);

    // we set our screen res currently to 96dpi in both directions (windows default)
    // we cache our characters (for rendering speed) so it's probably
    // not a good idea to allow free scaling of fonts - rather, just
    // scaling to pixel ratio on screen perhaps?
    if (FT_Set_Char_Size( face, 0, (int)(size*64 + 0.5f), xdpi, ydpi ))
    {
      FT_Done_Face(face);
      return NULL;
    }

    return face;
  };

  void ReleaseFont(FT_Face face)
  {
    assert(face);
    FT_Done_Face(face);
  };

  unsigned int GetDPI() const
  {
    return 72; // default dpi, matches what XPR fonts used to use for sizing
  };

private:
  FT_Library   m_library;
};

CFreeTypeLibrary g_freeTypeLibrary; // our freetype library

CGUIFontTTFBase::CGUIFontTTFBase(const CStdString& strFileName)
{
  m_texture = NULL;
  m_char = NULL;
  m_maxChars = 0;
  m_dwNestedBeginCount = 0;

  m_bTextureLoaded = false;
  m_vertex_size   = 4*1024;
  m_vertex        = (SVertex*)malloc(m_vertex_size * sizeof(SVertex));

  m_face = NULL;
  memset(m_charquick, 0, sizeof(m_charquick));
  m_strFileName = strFileName;
  m_referenceCount = 0;
  m_originX = m_originY = 0.0f;
  m_cellBaseLine = m_cellHeight = 0;
  m_numChars = 0;
  m_posX = m_posY = 0;
  m_textureHeight = m_textureWidth = 0;
  m_textureScaleX = m_textureScaleY = 0.0;
  m_ellipsesWidth = m_height = 0.0f;
  m_dwColor = 0;
  m_vertex_count = 0;
  m_nTexture = 0;
}

CGUIFontTTFBase::~CGUIFontTTFBase(void)
{
  Clear();
}

void CGUIFontTTFBase::AddReference()
{
  m_referenceCount++;
}

void CGUIFontTTFBase::RemoveReference()
{
  // delete this object when it's reference count hits zero
  m_referenceCount--;
  if (!m_referenceCount)
    g_fontManager.FreeFontFile(this);
}


void CGUIFontTTFBase::ClearCharacterCache()
{
  if (m_texture)
  {
    delete(m_texture);
  }

  DeleteHardwareTexture();

  m_texture = NULL;
  if (m_char)
    delete[] m_char;
  m_char = new Character[CHAR_CHUNK];
  memset(m_charquick, 0, sizeof(m_charquick));
  m_numChars = 0;
  m_maxChars = CHAR_CHUNK;
  // set the posX and posY so that our texture will be created on first character write.
  m_posX = m_textureWidth;
  m_posY = -(int)m_cellHeight;
}

void CGUIFontTTFBase::Clear()
{
  if (m_texture)
    delete(m_texture);

  m_texture = NULL;
  if (m_char)
    delete[] m_char;
  memset(m_charquick, 0, sizeof(m_charquick));
  m_char = NULL;
  m_maxChars = 0;
  m_numChars = 0;
  m_posX = 0;
  m_posY = 0;
  m_dwNestedBeginCount = 0;

  if (m_face)
    g_freeTypeLibrary.ReleaseFont(m_face);
  m_face = NULL;

  free(m_vertex);
  m_vertex = NULL;
  m_vertex_count = 0;
}

bool CGUIFontTTFBase::Load(const CStdString& strFilename, float height, float aspect, float lineSpacing)
{
  // we now know that this object is unique - only the GUIFont objects are non-unique, so no need
  // for reference tracking these fonts
  m_face = g_freeTypeLibrary.GetFont(strFilename, height, aspect);

  if (!m_face)
    return false;

  // grab the maximum cell height and width
  unsigned int m_cellWidth = m_face->bbox.xMax - m_face->bbox.xMin;
  m_cellHeight = m_face->bbox.yMax - m_face->bbox.yMin;
  m_cellBaseLine = m_face->bbox.yMax;

  unsigned int ydpi = g_freeTypeLibrary.GetDPI();
  unsigned int xdpi = (unsigned int)MathUtils::round_int(ydpi * aspect);

  m_cellWidth *= (unsigned int)(height * xdpi);
  m_cellWidth /= (72 * m_face->units_per_EM);

  m_cellHeight *= (unsigned int)(height * ydpi);
  m_cellHeight /= (72 * m_face->units_per_EM);

  m_cellBaseLine *= (unsigned int)(height * ydpi);
  m_cellBaseLine /= (72 * m_face->units_per_EM);

  // increment for good measure to give space in our texture
  m_cellWidth++;
  m_cellHeight+=2;
  m_cellBaseLine++;

//  CLog::Log(LOGDEBUG, "%s Scaled size of font %s (%f): width = %i, height = %i, lineheight = %li",
//    __FUNCTION__, strFilename.c_str(), height, m_cellWidth, m_cellHeight, m_face->size->metrics.height / 64);

  m_height = height;

  if (m_texture)
    delete(m_texture);

  m_texture = NULL;
  if (m_char)
    delete[] m_char;
  m_char = NULL;

  m_maxChars = 0;
  m_numChars = 0;

  m_strFilename = strFilename;

  m_textureHeight = 0;
  m_textureWidth = ((m_cellHeight * CHARS_PER_TEXTURE_LINE) & ~63) + 64;

  m_textureWidth = CBaseTexture::PadPow2(m_textureWidth);

  if (m_textureWidth > max_texture_size) m_textureWidth = max_texture_size;

  // set the posX and posY so that our texture will be created on first character write.
  m_posX = m_textureWidth;
  m_posY = -(int)m_cellHeight;

  // cache the ellipses width
  Character *ellipse = GetCharacter(L'.');
  if (ellipse) m_ellipsesWidth = ellipse->advance;

  return true;
}

void CGUIFontTTFBase::DrawTextInternal(float x, float y, const vector<DWORD> &colors, const vector<DWORD> &text, DWORD alignment, float maxPixelWidth, bool scrolling)
{
  Begin();

  // save the origin, which is scaled separately
  m_originX = x;
  m_originY = y;

  // Check if we will really need to truncate or justify the text
  if ( alignment & XBFONT_TRUNCATED )
  {
    if ( maxPixelWidth <= 0.0f || GetTextWidthInternal(text.begin(), text.end()) <= maxPixelWidth)
      alignment &= ~XBFONT_TRUNCATED;
  }
  else if ( alignment & XBFONT_JUSTIFIED )
  {
    if ( maxPixelWidth <= 0.0f )
      alignment &= ~XBFONT_JUSTIFIED;
  }

  // calculate sizing information
  float startX = 0;
  float startY = (alignment & XBFONT_CENTER_Y) ? -0.5f*(m_cellHeight-2) : 0;  // vertical centering

  if ( alignment & (XBFONT_RIGHT | XBFONT_CENTER_X) )
  {
    // Get the extent of this line
    float w = GetTextWidthInternal( text.begin(), text.end() );

    if ( alignment & XBFONT_TRUNCATED && w > maxPixelWidth )
      w = maxPixelWidth;

    if ( alignment & XBFONT_CENTER_X)
      w *= 0.5f;
    // Offset this line's starting position
    startX -= w;
  }

  float spacePerLetter = 0; // for justification effects
  if ( alignment & XBFONT_JUSTIFIED )
  {
    // first compute the size of the text to render in both characters and pixels
    unsigned int lineChars = 0;
    float linePixels = 0;
    for (vector<DWORD>::const_iterator pos = text.begin(); pos != text.end(); pos++)
    {
      Character *ch = GetCharacter(*pos);
      if (ch)
      { // spaces have multiple times the justification spacing of normal letters
        lineChars += ((*pos & 0xffff) == L' ') ? justification_word_weight : 1;
        linePixels += ch->advance;
      }
    }
    if (lineChars > 1)
      spacePerLetter = (maxPixelWidth - linePixels) / (lineChars - 1);
  }
  float cursorX = 0; // current position along the line

  for (vector<DWORD>::const_iterator pos = text.begin(); pos != text.end(); pos++)
  {
    // If starting text on a new line, determine justification effects
    // Get the current letter in the CStdString
    DWORD color = (*pos & 0xff0000) >> 16;
    if (color >= colors.size())
      color = 0;
    color = colors[color];

    // grab the next character
    Character *ch = GetCharacter(*pos);
    if (!ch) continue;

    if ( alignment & XBFONT_TRUNCATED )
    {
      // Check if we will be exceeded the max allowed width
      if ( cursorX + ch->advance + 3 * m_ellipsesWidth > maxPixelWidth )
      {
        // Yup. Let's draw the ellipses, then bail
        // Perhaps we should really bail to the next line in this case??
        Character *period = GetCharacter(L'.');
        if (!period)
          break;

        for (int i = 0; i < 3; i++)
        {
          RenderCharacter(startX + cursorX, startY, period, color, !scrolling);
          cursorX += period->advance;
        }
        break;
      }
    }
    else if (maxPixelWidth > 0 && cursorX > maxPixelWidth)
      break;  // exceeded max allowed width - stop rendering

    RenderCharacter(startX + cursorX, startY, ch, color, !scrolling);
    if ( alignment & XBFONT_JUSTIFIED )
    {
      if ((*pos & 0xffff) == L' ')
        cursorX += ch->advance + spacePerLetter * justification_word_weight;
      else
        cursorX += ch->advance + spacePerLetter;
    }
    else
      cursorX += ch->advance;
  }

  End();
}

// this routine assumes a single line (i.e. it was called from GUITextLayout)
float CGUIFontTTFBase::GetTextWidthInternal(vector<DWORD>::const_iterator start, vector<DWORD>::const_iterator end)
{
  float width = 0;
  while (start != end)
  {
    Character *c = GetCharacter(*start++);
    if (c) width += c->advance;
  }
  return width;
}

float CGUIFontTTFBase::GetCharWidthInternal(DWORD ch)
{
  Character *c = GetCharacter(ch);
  if (c) return c->advance;
  return 0;
}

float CGUIFontTTFBase::GetTextHeight(float lineSpacing, int numLines) const
{
  return (float)(numLines - 1) * GetLineHeight(lineSpacing) + (m_cellHeight - 2); // -2 as we increment this for space in our texture
}

float CGUIFontTTFBase::GetLineHeight(float lineSpacing) const
{
  if (m_face)
    return lineSpacing * m_face->size->metrics.height / 64.0f;
  return 0.0f;
}

CGUIFontTTFBase::Character* CGUIFontTTFBase::GetCharacter(DWORD chr)
{
  WCHAR letter = (WCHAR)(chr & 0xffff);
  DWORD style = (chr & 0x3000000) >> 24;

  // ignore linebreaks
  if (letter == L'\r')
    return NULL;

  // quick access to ascii chars
  if (letter < 255)
  {
    DWORD ch = (style << 8) | letter;
    if (m_charquick[ch])
      return m_charquick[ch];
  }

  // letters are stored based on style and letter
  DWORD ch = (style << 16) | letter;

  int low = 0;
  int high = m_numChars - 1;
  int mid;
  while (low <= high)
  {
    mid = (low + high) >> 1;
    if (ch > m_char[mid].letterAndStyle)
      low = mid + 1;
    else if (ch < m_char[mid].letterAndStyle)
      high = mid - 1;
    else
      return &m_char[mid];
  }
  // if we get to here, then low is where we should insert the new character

  // increase the size of the buffer if we need it
  if (m_numChars >= m_maxChars)
  { // need to increase the size of the buffer
    Character *newTable = new Character[m_maxChars + CHAR_CHUNK];
    if (m_char)
    {
      memcpy(newTable, m_char, low * sizeof(Character));
      memcpy(newTable + low + 1, m_char + low, (m_numChars - low) * sizeof(Character));
      delete[] m_char;
    }
    m_char = newTable;
    m_maxChars += CHAR_CHUNK;

  }
  else
  { // just move the data along as necessary
    memmove(m_char + low + 1, m_char + low, (m_numChars - low) * sizeof(Character));
  }
  // render the character to our texture
  // must End() as we can't render text to our texture during a Begin(), End() block
  DWORD dwNestedBeginCount = m_dwNestedBeginCount;
  m_dwNestedBeginCount = 1;
  if (dwNestedBeginCount) End();
  if (!CacheCharacter(letter, style, m_char + low))
  { // unable to cache character - try clearing them all out and starting over
    CLog::Log(LOGDEBUG, "GUIFontTTF::GetCharacter: Unable to cache character.  Clearing character cache of %i characters", m_numChars);
    ClearCharacterCache();
    low = 0;
    if (!CacheCharacter(letter, style, m_char + low))
    {
      CLog::Log(LOGERROR, "GUIFontTTF::GetCharacter: Unable to cache character (out of memory?)");
      if (dwNestedBeginCount) Begin();
      m_dwNestedBeginCount = dwNestedBeginCount;
      return NULL;
    }
  }
  if (dwNestedBeginCount) Begin();
  m_dwNestedBeginCount = dwNestedBeginCount;

  // fixup quick access
  memset(m_charquick, 0, sizeof(m_charquick));
  for(int i=0;i<m_numChars;i++)
  {
    if ((m_char[i].letterAndStyle & 0xffff) < 255)
    {
      DWORD ch = ((m_char[i].letterAndStyle & 0xffff0000) >> 8) | (m_char[i].letterAndStyle & 0xff);
      m_charquick[ch] = m_char+i;
    }
  }

  return m_char + low;
}

bool CGUIFontTTFBase::CacheCharacter(WCHAR letter, DWORD style, Character *ch)
{
  int glyph_index = FT_Get_Char_Index( m_face, letter );

  FT_Glyph glyph = NULL;
  if (FT_Load_Glyph( m_face, glyph_index, FT_LOAD_TARGET_LIGHT ))
  {
    CLog::Log(LOGDEBUG, "%s Failed to load glyph %x", __FUNCTION__, letter);
    return false;
  }
  // make bold if applicable
  if (style & FONT_STYLE_BOLD)
    EmboldenGlyph(m_face->glyph);
  // and italics if applicable
  if (style & FONT_STYLE_ITALICS)
    ObliqueGlyph(m_face->glyph);
  // grab the glyph
  if (FT_Get_Glyph(m_face->glyph, &glyph))
  {
    CLog::Log(LOGDEBUG, "%s Failed to get glyph %x", __FUNCTION__, letter);
    return false;
  }
  // render the glyph
  if (FT_Glyph_To_Bitmap(&glyph, FT_RENDER_MODE_NORMAL, NULL, 1))
  {
    CLog::Log(LOGDEBUG, "%s Failed to render glyph %x to a bitmap", __FUNCTION__, letter);
    return false;
  }
  FT_BitmapGlyph bitGlyph = (FT_BitmapGlyph)glyph;
  FT_Bitmap bitmap = bitGlyph->bitmap;
  if (bitGlyph->left < 0)
    m_posX += -bitGlyph->left;

  // check we have enough room for the character
  if (m_posX + bitGlyph->left + bitmap.width > (int)m_textureWidth)
  { // no space - gotta drop to the next line (which means creating a new texture and copying it across)
    m_posX = 0;
    m_posY += m_cellHeight;
    if (bitGlyph->left < 0)
      m_posX += -bitGlyph->left;

    if(m_posY + m_cellHeight >= m_textureHeight)
    {
      // create the new larger texture
      unsigned int newHeight = m_posY + m_cellHeight;
      // check for max height (can't be more than max_texture_size texels
      if (newHeight > max_texture_size)
      {
        CLog::Log(LOGDEBUG, "GUIFontTTF::CacheCharacter: New cache texture is too large (%u > %u pixels long)", newHeight, max_texture_size);
        FT_Done_Glyph(glyph);
        return false;
      }

      CBaseTexture* newTexture = NULL;
      newTexture = ReallocTexture(newHeight);
      if(newTexture == NULL)
      {
        FT_Done_Glyph(glyph);
        return false;
      }
      m_texture = newTexture;
    }
  }

  // set the character in our table
  ch->letterAndStyle = (style << 16) | letter;
  ch->offsetX = (short)bitGlyph->left;
  ch->offsetY = (short)max((short)m_cellBaseLine - bitGlyph->top, 0);
  ch->left = (float)m_posX + ch->offsetX;
  ch->top = (float)m_posY + ch->offsetY;
  ch->right = ch->left + bitmap.width;
  ch->bottom = ch->top + bitmap.rows;
  ch->advance = (float)MathUtils::round_int( (float)m_face->glyph->advance.x / 64 );

  // we need only render if we actually have some pixels
  if (bitmap.width * bitmap.rows)
  {
    CopyCharToTexture(bitGlyph, ch);
  }
  m_posX += (unsigned short)max(ch->right - ch->left + ch->offsetX, ch->advance + 1);
  m_numChars++;

  m_textureScaleX = 1.0f / m_textureWidth;
  m_textureScaleY = 1.0f / m_textureHeight;

  // free the glyph
  FT_Done_Glyph(glyph);

  return true;
}

void CGUIFontTTFBase::RenderCharacter(float posX, float posY, const Character *ch, D3DCOLOR dwColor, bool roundX)
{
  // actual image width isn't same as the character width as that is
  // just baseline width and height should include the descent
  const float width = ch->right - ch->left;
  const float height = ch->bottom - ch->top;

  // posX and posY are relative to our origin, and the textcell is offset
  // from our (posX, posY).  Plus, these are unscaled quantities compared to the underlying GUI resolution
  CRect vertex((posX + ch->offsetX) * g_graphicsContext.GetGUIScaleX(),
               (posY + ch->offsetY) * g_graphicsContext.GetGUIScaleY(),
               (posX + ch->offsetX + width) * g_graphicsContext.GetGUIScaleX(),
               (posY + ch->offsetY + height) * g_graphicsContext.GetGUIScaleY());
  vertex += CPoint(m_originX, m_originY);
  CRect texture(ch->left, ch->top, ch->right, ch->bottom);
  g_graphicsContext.ClipRect(vertex, texture);

  // transform our positions - note, no scaling due to GUI calibration/resolution occurs
  float x[4];

  x[0] = g_graphicsContext.ScaleFinalXCoord(vertex.x1, vertex.y1);
  x[1] = g_graphicsContext.ScaleFinalXCoord(vertex.x2, vertex.y1);
  x[2] = g_graphicsContext.ScaleFinalXCoord(vertex.x2, vertex.y2);
  x[3] = g_graphicsContext.ScaleFinalXCoord(vertex.x1, vertex.y2);

  if (roundX)
  {
    // We only round the "left" side of the character, and then use the direction of rounding to
    // move the "right" side of the character.  This ensures that a constant width is kept when rendering
    // the same letter at the same size at different places of the screen, avoiding the problem
    // of the "left" side rounding one way while the "right" side rounds the other way, thus getting
    // altering the width of thin characters substantially.  This only really works for positive
    // coordinates (due to the direction of truncation for negatives) but this is the only case that
    // really interests us anyway.
    float rx0 = (float)MathUtils::round_int(x[0]);
    float rx3 = (float)MathUtils::round_int(x[3]);
    x[1] = (float)MathUtils::truncate_int(x[1]);
    x[2] = (float)MathUtils::truncate_int(x[2]);
    if (rx0 > x[0])
      x[1] += 1;
    if (rx3 > x[3])
      x[2] += 1;
    x[0] = rx0;
    x[3] = rx3;
  }

  float y1 = (float)MathUtils::round_int(g_graphicsContext.ScaleFinalYCoord(vertex.x1, vertex.y1));
  float y2 = (float)MathUtils::round_int(g_graphicsContext.ScaleFinalYCoord(vertex.x2, vertex.y1));
  float y3 = (float)MathUtils::round_int(g_graphicsContext.ScaleFinalYCoord(vertex.x2, vertex.y2));
  float y4 = (float)MathUtils::round_int(g_graphicsContext.ScaleFinalYCoord(vertex.x1, vertex.y2));

  float z1 = (float)MathUtils::round_int(g_graphicsContext.ScaleFinalZCoord(vertex.x1, vertex.y1));
  float z2 = (float)MathUtils::round_int(g_graphicsContext.ScaleFinalZCoord(vertex.x2, vertex.y1));
  float z3 = (float)MathUtils::round_int(g_graphicsContext.ScaleFinalZCoord(vertex.x2, vertex.y2));
  float z4 = (float)MathUtils::round_int(g_graphicsContext.ScaleFinalZCoord(vertex.x1, vertex.y2));

  // tex coords converted to 0..1 range
  float tl = texture.x1 * m_textureScaleX;
  float tr = texture.x2 * m_textureScaleX;
  float tt = texture.y1 * m_textureScaleY;
  float tb = texture.y2 * m_textureScaleY;

  // grow the vertex buffer if required
  if(m_vertex_count >= m_vertex_size)
  {
    m_vertex_size *= 2;
    m_vertex       = (SVertex*)realloc(m_vertex, m_vertex_size * sizeof(SVertex));
  }

  m_dwColor = dwColor;
  SVertex* v = m_vertex + m_vertex_count;

  for(int i = 0; i < 4; i++)
  {
    v[i].r = GET_R(dwColor);
    v[i].g = GET_G(dwColor);
    v[i].b = GET_B(dwColor);
    v[i].a = GET_A(dwColor);
  }

  v[0].u = tl;
  v[0].v = tt;
  v[0].x = x[0];
  v[0].y = y1;
  v[0].z = z1;

  v[1].u = tr;
  v[1].v = tt;
  v[1].x = x[1];
  v[1].y = y2;
  v[1].z = z2;

  v[2].u = tr;
  v[2].v = tb;
  v[2].x = x[2];
  v[2].y = y3;
  v[2].z = z3;

  v[3].u = tl;
  v[3].v = tb;
  v[3].x = x[3];
  v[3].y = y4;
  v[3].z = z4;

  RenderInternal(v);

  m_vertex_count+=4;
}

// Oblique code - original taken from freetype2 (ftsynth.c)
void CGUIFontTTFBase::ObliqueGlyph(FT_GlyphSlot slot)
{
  /* only oblique outline glyphs */
  if ( slot->format != FT_GLYPH_FORMAT_OUTLINE )
    return;

  /* we don't touch the advance width */

  /* For italic, simply apply a shear transform, with an angle */
  /* of about 12 degrees.                                      */

  FT_Matrix    transform;
  transform.xx = 0x10000L;
  transform.yx = 0x00000L;

  transform.xy = 0x06000L;
  transform.yy = 0x10000L;

  FT_Outline_Transform( &slot->outline, &transform );
}


// Embolden code - original taken from freetype2 (ftsynth.c)
void CGUIFontTTFBase::EmboldenGlyph(FT_GlyphSlot slot)
{
  if ( slot->format != FT_GLYPH_FORMAT_OUTLINE )
    return;

  /* some reasonable strength */
  FT_Pos strength = FT_MulFix( m_face->units_per_EM,
                    m_face->size->metrics.y_scale ) / 24;

  FT_BBox bbox_before, bbox_after;
  FT_Outline_Get_CBox( &slot->outline, &bbox_before );
  FT_Outline_Embolden( &slot->outline, strength );  // ignore error
  FT_Outline_Get_CBox( &slot->outline, &bbox_after );

  FT_Pos dx = bbox_after.xMax - bbox_before.xMax;
  FT_Pos dy = bbox_after.yMax - bbox_before.yMax;

  if ( slot->advance.x )
    slot->advance.x += dx;

  if ( slot->advance.y )
    slot->advance.y += dy;

  slot->metrics.width        += dx;
  slot->metrics.height       += dy;
  slot->metrics.horiBearingY += dy;
  slot->metrics.horiAdvance  += dx;
  slot->metrics.vertBearingX -= dx / 2;
  slot->metrics.vertBearingY += dy;
  slot->metrics.vertAdvance  += dy;
}


