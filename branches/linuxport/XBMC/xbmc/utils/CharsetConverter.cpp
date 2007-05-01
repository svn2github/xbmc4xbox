#include "../stdafx.h"

#define ICONV_PREPARE(iconv) iconv=(iconv_t)-1
#define ICONV_SAFE_CLOSE(iconv) if (iconv!=(iconv_t)-1) { iconv_close(iconv); iconv=(iconv_t)-1; }

CCharsetConverter g_charsetConverter;

CCharsetConverter::CCharsetConverter()
{
  m_vecCharsetNames.push_back("ISO-8859-1");
  m_vecCharsetLabels.push_back("Western Europe (ISO)");
  m_vecCharsetNames.push_back("ISO-8859-2");
  m_vecCharsetLabels.push_back("Central Europe (ISO)");
  m_vecCharsetNames.push_back("ISO-8859-3");
  m_vecCharsetLabels.push_back("South Europe (ISO)");
  m_vecCharsetNames.push_back("ISO-8859-4");
  m_vecCharsetLabels.push_back("Baltic (ISO)");
  m_vecCharsetNames.push_back("ISO-8859-5");
  m_vecCharsetLabels.push_back("Cyrillic (ISO)");
  m_vecCharsetNames.push_back("ISO-8859-6");
  m_vecCharsetLabels.push_back("Arabic (ISO)");
  m_vecCharsetNames.push_back("ISO-8859-7");
  m_vecCharsetLabels.push_back("Greek (ISO)");
  m_vecCharsetNames.push_back("ISO-8859-8");
  m_vecCharsetLabels.push_back("Hebrew (ISO)");
  m_vecCharsetNames.push_back("ISO-8859-9");
  m_vecCharsetLabels.push_back("Turkish (ISO)");

  m_vecCharsetNames.push_back("CP1250");
  m_vecCharsetLabels.push_back("Central Europe (Windows)");
  m_vecCharsetNames.push_back("CP1251");
  m_vecCharsetLabels.push_back("Cyrillic (Windows)");
  m_vecCharsetNames.push_back("CP1252");
  m_vecCharsetLabels.push_back("Western Europe (Windows)");
  m_vecCharsetNames.push_back("CP1253");
  m_vecCharsetLabels.push_back("Greek (Windows)");
  m_vecCharsetNames.push_back("CP1254");
  m_vecCharsetLabels.push_back("Turkish (Windows)");
  m_vecCharsetNames.push_back("CP1255");
  m_vecCharsetLabels.push_back("Hebrew (Windows)");
  m_vecCharsetNames.push_back("CP1256");
  m_vecCharsetLabels.push_back("Arabic (Windows)");
  m_vecCharsetNames.push_back("CP1257");
  m_vecCharsetLabels.push_back("Baltic (Windows)");
  m_vecCharsetNames.push_back("CP1258");
  m_vecCharsetLabels.push_back("Vietnamesse (Windows)");
  m_vecCharsetNames.push_back("CP874");
  m_vecCharsetLabels.push_back("Thai (Windows)");

  m_vecCharsetNames.push_back("BIG5");
  m_vecCharsetLabels.push_back("Chinese Traditional (Big5)");
  m_vecCharsetNames.push_back("GBK");
  m_vecCharsetLabels.push_back("Chinese Simplified (GBK)");
  m_vecCharsetNames.push_back("SHIFT_JIS");
  m_vecCharsetLabels.push_back("Japanese (Shift-JIS)");
  m_vecCharsetNames.push_back("CP949");
  m_vecCharsetLabels.push_back("Korean");
  m_vecCharsetNames.push_back("BIG5-HKSCS");
  m_vecCharsetLabels.push_back("Hong Kong (Big5-HKSCS)");

  m_vecBidiCharsetNames.push_back("ISO-8859-6");
  m_vecBidiCharsetNames.push_back("ISO-8859-8");
  m_vecBidiCharsetNames.push_back("CP1255");
  m_vecBidiCharsetNames.push_back("Windows-1255");
  m_vecBidiCharsetNames.push_back("CP1256");
  m_vecBidiCharsetNames.push_back("Windows-1256");
#ifndef _LINUX
  m_vecBidiCharsets.push_back(FRIBIDI_CHARSET_ISO8859_6);
  m_vecBidiCharsets.push_back(FRIBIDI_CHARSET_ISO8859_8);
  m_vecBidiCharsets.push_back(FRIBIDI_CHARSET_CP1255);
  m_vecBidiCharsets.push_back(FRIBIDI_CHARSET_CP1255);
  m_vecBidiCharsets.push_back(FRIBIDI_CHARSET_CP1256);
  m_vecBidiCharsets.push_back(FRIBIDI_CHARSET_CP1256);
#else
  m_vecBidiCharsets.push_back(FRIBIDI_CHAR_SET_ISO8859_6);
  m_vecBidiCharsets.push_back(FRIBIDI_CHAR_SET_ISO8859_8);
  m_vecBidiCharsets.push_back(FRIBIDI_CHAR_SET_CP1255);
  m_vecBidiCharsets.push_back(FRIBIDI_CHAR_SET_CP1255);
  m_vecBidiCharsets.push_back(FRIBIDI_CHAR_SET_CP1256);
  m_vecBidiCharsets.push_back(FRIBIDI_CHAR_SET_CP1256);
#endif

  ICONV_PREPARE(m_iconvStringCharsetToFontCharset);
  ICONV_PREPARE(m_iconvUtf8ToStringCharset);
  ICONV_PREPARE(m_iconvStringCharsetToUtf8);
  ICONV_PREPARE(m_iconvUcs2CharsetToStringCharset);
  ICONV_PREPARE(m_iconvSubtitleCharsetToUtf16);
  ICONV_PREPARE(m_iconvUtf16toUtf8);
  ICONV_PREPARE(m_iconvUtf16BEtoUtf8);
  ICONV_PREPARE(m_iconvUtf32ToStringCharset);
  ICONV_PREPARE(m_iconvUtf8toUtf16);
}

void CCharsetConverter::clear()
{
  m_vecBidiCharsetNames.clear();
  m_vecBidiCharsets.clear();
  m_vecCharsetNames.clear();
  m_vecCharsetLabels.clear();
}

vector<CStdString> CCharsetConverter::getCharsetLabels()
{
  return m_vecCharsetLabels;
}

CStdString& CCharsetConverter::getCharsetLabelByName(const CStdString& charsetName)
{
  for (unsigned int i = 0; i < m_vecCharsetNames.size(); i++)
  {
    if (m_vecCharsetNames[i] == charsetName)
    {
      return m_vecCharsetLabels[i];
    }
  }

  return EMPTY;
}

CStdString& CCharsetConverter::getCharsetNameByLabel(const CStdString& charsetLabel)
{
  for (unsigned int i = 0; i < m_vecCharsetLabels.size(); i++)
  {
    if (m_vecCharsetLabels[i] == charsetLabel)
    {
      return m_vecCharsetNames[i];
    }
  }

  return EMPTY;
}

bool CCharsetConverter::isBidiCharset(const CStdString& charset)
{
  for (unsigned int i = 0; i < m_vecBidiCharsetNames.size(); i++)
  {
    if (m_vecBidiCharsetNames[i].Equals(charset))
    {
      return true;
    }
  }

  return false;
}

void CCharsetConverter::reset(void)
{
  ICONV_SAFE_CLOSE(m_iconvStringCharsetToFontCharset);
  ICONV_SAFE_CLOSE(m_iconvUtf8ToStringCharset);
  ICONV_SAFE_CLOSE(m_iconvStringCharsetToUtf8);
  ICONV_SAFE_CLOSE(m_iconvUcs2CharsetToStringCharset);
  ICONV_SAFE_CLOSE(m_iconvSubtitleCharsetToUtf16);
  ICONV_SAFE_CLOSE(m_iconvUtf16toUtf8);
  ICONV_SAFE_CLOSE(m_iconvUtf16BEtoUtf8);
  ICONV_SAFE_CLOSE(m_iconvUtf32ToStringCharset);
  ICONV_SAFE_CLOSE(m_iconvUtf8toUtf16);

#ifndef _LINUX
  m_stringFribidiCharset = FRIBIDI_CHARSET_NOT_FOUND;
#else
  m_stringFribidiCharset = FRIBIDI_CHAR_SET_NOT_FOUND;
#endif

  CStdString strCharset=g_langInfo.GetGuiCharSet();

  for (unsigned int i = 0; i < m_vecBidiCharsetNames.size(); i++)
  {
    if (m_vecBidiCharsetNames[i] == strCharset)
    {
      m_stringFribidiCharset = m_vecBidiCharsets[i];
    }
  }
}

// The bVisualBiDiFlip forces a flip of characters for hebrew/arabic languages, only set to false if the flipping
// of the string is already made or the string is not displayed in the GUI
void CCharsetConverter::utf8ToUTF16(const CStdStringA& utf8String, CStdStringW &utf16String, bool bVisualBiDiFlip/*=true*/)
{
  CStdStringA strFlipped;
  const char* src;
  size_t inBytes;

  // Try to flip hebrew/arabic characters, if any
  if (bVisualBiDiFlip)
  {
#ifndef _LINUX
    logicalToVisualBiDi(utf8String, strFlipped, FRIBIDI_CHARSET_UTF8);
#else
    logicalToVisualBiDi(utf8String, strFlipped, FRIBIDI_CHAR_SET_UTF8);
#endif
    src = strFlipped.c_str();
    inBytes = strFlipped.length() + 1;
  }
  else
  {
    src = utf8String.c_str();
    inBytes = utf8String.length() + 1;
  }

  if (m_iconvUtf8toUtf16 == (iconv_t) - 1)
  {
    m_iconvUtf8toUtf16 = iconv_open("UTF-16LE", "UTF-8");
  }

  if (m_iconvUtf8toUtf16 != (iconv_t) - 1)
  {
    char *dst = new char[inBytes * 2];
    size_t outBytes = inBytes * 2;
    char *outdst = dst;
    if (iconv(m_iconvUtf8toUtf16, (char**) &src, &inBytes, &outdst, &outBytes))
    {
      // For some reason it failed (maybe wrong charset?). Nothing to do but
      // return the original..
      utf16String = utf8String;
    }
    else
    {
      utf16String = (WCHAR *)dst;
    }
    delete[] dst;
  }
}

void CCharsetConverter::subtitleCharsetToUTF16(const CStdStringA& strSource, CStdStringW& strDest)
{
  CStdStringA strFlipped;

  // No need to flip hebrew/arabic as mplayer does the flipping

  if (m_iconvSubtitleCharsetToUtf16 == (iconv_t) - 1)
  {
    CStdString strCharset=g_langInfo.GetSubtitleCharSet();
    m_iconvSubtitleCharsetToUtf16 = iconv_open("UTF-16LE", strCharset.c_str());
  }

  if (m_iconvSubtitleCharsetToUtf16 != (iconv_t) - 1)
  {
    const char* src = strSource.c_str();
    size_t inBytes = strSource.length() + 1;
    char *dst = (char*)strDest.GetBuffer(inBytes * 2);
    size_t outBytes = inBytes * 2;

    if (iconv(m_iconvSubtitleCharsetToUtf16, (char**) &src, &inBytes, &dst, &outBytes))
    {
      strDest.ReleaseBuffer();
      // For some reason it failed (maybe wrong charset?). Nothing to do but
      // return the original..
      strDest = strSource;
    }
    strDest.ReleaseBuffer();
  }
}

void CCharsetConverter::logicalToVisualBiDi(const CStdStringA& strSource, CStdStringA& strDest, CStdStringA& charset, FriBidiCharType base)
{
#ifndef _LINUX
  FriBidiCharSet fribidiCharset = FRIBIDI_CHARSET_UTF8;
#else
  FriBidiCharSet fribidiCharset = FRIBIDI_CHAR_SET_UTF8;
#endif

  for (unsigned int i = 0; i < m_vecBidiCharsetNames.size(); i++)
  {
    if (m_vecBidiCharsetNames[i].Equals(charset))
    {
		fribidiCharset = m_vecBidiCharsets[i];
		break;
    }
  }

  logicalToVisualBiDi(strSource, strDest, fribidiCharset, base);
}

void CCharsetConverter::logicalToVisualBiDi(const CStdStringA& strSource, CStdStringA& strDest, FriBidiCharSet fribidiCharset, FriBidiCharType base)
{
  int sourceLen = strlen(strSource.c_str());
  FriBidiChar* logical = (FriBidiChar*) malloc((sourceLen + 1) * sizeof(FriBidiChar));
  FriBidiChar* visual = (FriBidiChar*) malloc((sourceLen + 1) * sizeof(FriBidiChar));
  // Convert from the selected charset to Unicode
  int len = fribidi_charset_to_unicode(fribidiCharset, (char*) strSource.c_str(), sourceLen, logical);

  if (fribidi_log2vis(logical, len, &base, visual, NULL, NULL, NULL))
  {
    // Removes bidirectional marks
    //len = fribidi_remove_bidi_marks(visual, len, NULL, NULL, NULL);

    char *result = strDest.GetBuffer(sourceLen);

    // Convert back from Unicode to the charset
    fribidi_unicode_to_charset(fribidiCharset, visual, len, result);
    strDest.ReleaseBuffer();
  }

  free(logical);
  free(visual);
}

void CCharsetConverter::utf8ToStringCharset(const CStdStringA& strSource, CStdStringA& strDest)
{
  if (m_iconvUtf8ToStringCharset == (iconv_t) - 1)
  {
    CStdString strCharset=g_langInfo.GetGuiCharSet();
    m_iconvUtf8ToStringCharset = iconv_open(strCharset.c_str(), "UTF-8");
  }

  if (m_iconvUtf8ToStringCharset != (iconv_t) - 1)
  {
    const char* src = strSource.c_str();
    size_t inBytes = strSource.length() + 1;

    char *dst = strDest.GetBuffer(inBytes);
    size_t outBytes = inBytes - 1;

    if (iconv(m_iconvUtf8ToStringCharset, (char**) &src, &inBytes, &dst, &outBytes) == -1)
    {
      strDest.ReleaseBuffer();
      // For some reason it failed (maybe wrong charset?). Nothing to do but
      // return the original..
      strDest = strSource;
    }
    strDest.ReleaseBuffer();
  }
}

void CCharsetConverter::utf8ToStringCharset(CStdStringA& strSourceDest)
{
  CStdString strDest;
  utf8ToStringCharset(strSourceDest, strDest);
  strSourceDest=strDest;
}

void CCharsetConverter::stringCharsetToUtf8(const CStdStringA& strSource, CStdStringA& strDest)
{
  if (m_iconvStringCharsetToUtf8 == (iconv_t) - 1)
  {
    CStdString strCharset=g_langInfo.GetGuiCharSet();
    m_iconvStringCharsetToUtf8 = iconv_open("UTF-8", strCharset.c_str());
  }

  if (m_iconvStringCharsetToUtf8 != (iconv_t) - 1)
  {
    const char* src = strSource.c_str();
    size_t inBytes = strSource.length() + 1;

    size_t outBytes = (inBytes * 4) + 1;
    char *dst = strDest.GetBuffer(outBytes);

    if (iconv(m_iconvStringCharsetToUtf8, (char**) &src, &inBytes, &dst, &outBytes) == -1)
    {
      strDest.ReleaseBuffer();
      // For some reason it failed (maybe wrong charset?). Nothing to do but
      // return the original..
      strDest = strSource;
      return ;
    }

    strDest.ReleaseBuffer();
  }
}

void CCharsetConverter::stringCharsetToUtf8(CStdStringA& strSourceDest)
{
  CStdString strSource=strSourceDest;
  stringCharsetToUtf8(strSource, strSourceDest);
}

void CCharsetConverter::stringCharsetToUtf8(const CStdStringA& strSourceCharset, const CStdStringA& strSource, CStdStringA& strDest)
{
  iconv_t iconvString=iconv_open("UTF-8", strSourceCharset.c_str());

  if (iconvString != (iconv_t) - 1)
  {
    const char* src = strSource.c_str();
    size_t inBytes = strSource.length() + 1;

    size_t outBytes = (inBytes * 4) + 1;
    char *dst = strDest.GetBuffer(outBytes);

    if (iconv(iconvString, (char**) &src, &inBytes, &dst, &outBytes) == -1)
    {
      strDest.ReleaseBuffer();
      // For some reason it failed (maybe wrong charset?). Nothing to do but
      // return the original..
      strDest = strSource;
      return ;
    }

    strDest.ReleaseBuffer();

    iconv_close(iconvString);
  }
}

void CCharsetConverter::utf16toUTF8(const CStdStringW& strSource, CStdStringA &strDest)
{
  if (m_iconvUtf16toUtf8 == (iconv_t) - 1)
    m_iconvUtf16toUtf8 = iconv_open("UTF-8", "UTF-16LE");
  if (m_iconvUtf16toUtf8 != (iconv_t) - 1)
  {
    const char* src = (const char*) strSource.c_str();
    size_t inBytes = (strSource.length() + 1)*2;
    size_t outBytes = (inBytes + 1)*2;  // some free for UTF-8 (up to 4 bytes/char)
    char *dst = strDest.GetBuffer(outBytes);
    if (iconv(m_iconvUtf16toUtf8, (char**) &src, &inBytes, &dst, &outBytes))
    { // failed :(
      strDest.ReleaseBuffer();
      strDest = strSource;
      return;
    }
    strDest.ReleaseBuffer();
  }
}

void CCharsetConverter::utf16BEtoUTF8(const CStdStringW& strSource, CStdStringA &strDest)
{
  if (m_iconvUtf16BEtoUtf8 == (iconv_t) - 1)
    m_iconvUtf16BEtoUtf8 = iconv_open("UTF-8", "UTF-16BE");
  if (m_iconvUtf16BEtoUtf8 != (iconv_t) - 1)
  {
    const char* src = (const char*) strSource.c_str();
    size_t inBytes = (strSource.length() + 1)*2;
    size_t outBytes = (inBytes + 1)*2;  // UTF-8 is up to 4 bytes/character  
    char *dst = strDest.GetBuffer(outBytes);
    if (iconv(m_iconvUtf16BEtoUtf8, (char**) &src, &inBytes, &dst, &outBytes))
    { // failed :(
      strDest.ReleaseBuffer();
      strDest = strSource;
      return;
    }
    strDest.ReleaseBuffer();
  }
}

void CCharsetConverter::ucs2CharsetToStringCharset(const CStdStringW& strSource, CStdStringA& strDest, bool swap)
{
  if (m_iconvUcs2CharsetToStringCharset == (iconv_t) - 1)
  {
    CStdString strCharset=g_langInfo.GetGuiCharSet();
    m_iconvUcs2CharsetToStringCharset = iconv_open(strCharset.c_str(), "UTF-16LE");
  }

  if (m_iconvUcs2CharsetToStringCharset != (iconv_t) - 1)
  {
    const char* src = (const char*) strSource.c_str();
    size_t inBytes = (strSource.length() + 1) * 2;

    if (swap)
    {
      char* s = (char*) src;

      while (*s || *(s + 1))
      {
        char c = *s;
        *s = *(s + 1);
        *(s + 1) = c;

        s++;
        s++;
      }
    }

    char *dst = strDest.GetBuffer(inBytes);
    size_t outBytes = inBytes;

    if (iconv(m_iconvUcs2CharsetToStringCharset, (char**) &src, &inBytes, &dst, &outBytes))
    {
      strDest.ReleaseBuffer();
      // For some reason it failed (maybe wrong charset?). Nothing to do but
      // return the original..
      strDest = strSource;
    }
    strDest.ReleaseBuffer();
  }
}

void CCharsetConverter::utf32ToStringCharset(const unsigned long* strSource, CStdStringA& strDest)
{
  if (m_iconvUtf32ToStringCharset == (iconv_t) - 1)
  {
    CStdString strCharset=g_langInfo.GetGuiCharSet();
    m_iconvUtf32ToStringCharset = iconv_open(strCharset.c_str(), "UTF-32LE");
  }

  if (m_iconvUtf32ToStringCharset != (iconv_t) - 1)
  {
    const unsigned long* ptr=strSource;
    while (*ptr) ptr++;
    const char* src = (const char*) strSource;
    size_t inBytes = (ptr-strSource+1)*4;

    char *dst = strDest.GetBuffer(inBytes);
    size_t outBytes = inBytes;

    if (iconv(m_iconvUtf32ToStringCharset, (char**) &src, &inBytes, &dst, &outBytes))
    {
      strDest.ReleaseBuffer();
      // For some reason it failed (maybe wrong charset?). Nothing to do but
      // return the original..
      strDest = (const char *)strSource;
    }
    strDest.ReleaseBuffer();
  }
}

// Taken from RFC2640
bool CCharsetConverter::isValidUtf8(const char *buf, unsigned int len)
{
  const unsigned char *endbuf = (unsigned char*)buf + len;
  unsigned char byte2mask=0x00, c;
  int trailing=0; // trailing (continuation) bytes to follow

  while ((unsigned char*)buf != endbuf)
  {
    c = *buf++;
    if (trailing)
      if ((c & 0xc0) == 0x80) // does trailing byte follow UTF-8 format ?
      {
        if (byte2mask) // need to check 2nd byte for proper range
        {
          if (c & byte2mask) // are appropriate bits set ?
            byte2mask = 0x00;
          else
            return false;
        }
        trailing--;
      }
      else
        return 0;
    else
      if ((c & 0x80) == 0x00) continue; // valid 1-byte UTF-8
      else if ((c & 0xe0) == 0xc0)      // valid 2-byte UTF-8
        if (c & 0x1e)                   //is UTF-8 byte in proper range ?
          trailing = 1;
        else
          return false;
      else if ((c & 0xf0) == 0xe0)      // valid 3-byte UTF-8
       {
        if (!(c & 0x0f))                // is UTF-8 byte in proper range ?
          byte2mask = 0x20;             // if not set mask
        trailing = 2;                   // to check next byte
      }
      else if ((c & 0xf8) == 0xf0)      // valid 4-byte UTF-8
      {
        if (!(c & 0x07))                // is UTF-8 byte in proper range ?
          byte2mask = 0x30;             // if not set mask
        trailing = 3;                   // to check next byte
      }
      else if ((c & 0xfc) == 0xf8)      // valid 5-byte UTF-8
      {
        if (!(c & 0x03))                // is UTF-8 byte in proper range ?
          byte2mask = 0x38;             // if not set mask
        trailing = 4;                   // to check next byte
      }
      else if ((c & 0xfe) == 0xfc)      // valid 6-byte UTF-8
      {
        if (!(c & 0x01))                // is UTF-8 byte in proper range ?
          byte2mask = 0x3c;             // if not set mask
        trailing = 5;                   // to check next byte
      }
      else
        return false;
  }
  return trailing == 0;
}

bool CCharsetConverter::isValidUtf8(const CStdString& str)
{
  return isValidUtf8(str.c_str(), str.size());
}
