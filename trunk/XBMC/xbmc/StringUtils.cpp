//-----------------------------------------------------------------------
//
//  File:      StringUtils.cpp
//
//  Purpose:   ATL split string utility
//  Author:    Paul J. Weiss
//
//  Modified to use J O'Leary's CStdString class by kraqh3d
//
//------------------------------------------------------------------------


#include "stdafx.h"
#include "StringUtils.h"

// Splits the string input into pieces delimited by delimiter.
// if 2 delimiters are in a row, it will include the empty string between them.
int StringUtils::SplitString(const CStdString& input, const CStdString& delimiter, CStdStringArray &results)
{
  int iPos = 0;
  int newPos = -1;
  int sizeS2 = delimiter.GetLength();
  int isize = input.GetLength();

  //CArray positions;
  vector<unsigned int> positions;

  newPos = input.Find (delimiter, 0);

  if ( newPos < 0 )
  {
    results.push_back(input);
    return 1;
  }

  int numFound = 1;

  while ( newPos > iPos )
  {
    numFound++;
    positions.push_back(newPos);
    iPos = newPos;
    newPos = input.Find (delimiter, iPos + sizeS2);
  }

  for ( unsigned int i = 0; i <= positions.size(); i++ )
  {
    CStdString s;
    if ( i == 0 )
	{
		if (i == positions.size())
			s = input;
		else
		s = input.Mid( i, positions[i] );
	}
    else
    {
      int offset = positions[i - 1] + sizeS2;
      if ( offset < isize )
      {
        if ( i == positions.size() )
          s = input.Mid(offset);
        else if ( i > 0 )
          s = input.Mid( positions[i - 1] + sizeS2,
                         positions[i] - positions[i - 1] - sizeS2 );
      }
    }
    results.push_back(s);
  }
  return numFound;
}

// returns the number of occurences of strFind in strInput.
int StringUtils::FindNumber(const CStdString& strInput, const CStdString &strFind)
{
  int pos = strInput.Find(strFind, 0);
  int numfound = 0;
  while (pos > 0)
  {
    numfound++;
    pos = strInput.Find(strFind, pos + 1);
  }
  return numfound;
}

// Compares separately the numeric and alphabetic parts of a string.
// returns negative if left < right, positive if left > right
// and 0 if they are identical (essentially calculates left - right)
int StringUtils::AlphaNumericCompare(const char *left, const char *right)
{
  unsigned char *l = (unsigned char *)left;
  unsigned char *r = (unsigned char *)right;
  unsigned char *ld, *rd;
  unsigned char lc, rc;
  unsigned int lnum, rnum;
  while (*l != 0 && *r != 0)
  {
    // check if we have a numerical value
    if (*l >= '0' && *l <= '9' && *r >= '0' && *r <= '9')
    {
      ld = l;
      lnum = 0;
      while (*ld >= '0' && *ld <= '9')
      {
        lnum *= 10;
        lnum += *ld++ - '0';
      }
      rd = r;
      rnum = 0;
      while (*rd >= '0' && *rd <= '9')
      {
        rnum *= 10;
        rnum += *rd++ - '0';
      }
      // do we have numbers?
      if (lnum != rnum)
      { // yes - and they're different!
        return lnum - rnum;
      }
      l = ld;
      r = rd;
      continue;
    }
    // do case less comparison
    lc = *l;
    if (lc >= 'A' && lc <= 'Z')
      lc += 'a'-'A';
    rc = *r;
    if (rc >= 'A' && rc <= 'Z')
      rc += 'a'-'A';
    // ok, do a normal comparison.  Add special case stuff (eg '(' characters)) in here later
    if (lc  != rc)
    {
      return lc - rc;
    }
    l++; r++;
  }
  if (*r)
  { // r is longer
    return -1;
  }
  else if (*l)
  { // l is longer
    return 1;
  }
  return 0; // files are the same
}

long StringUtils::TimeStringToSeconds(const CStdString &timeString)
{
  CStdStringArray secs;
  StringUtils::SplitString(timeString, ":", secs);
  int timeInSecs = 0;
  for (unsigned int i = 0; i < secs.size(); i++)
  {
    timeInSecs *= 60;
    timeInSecs += atoi(secs[i]);
  }
  return timeInSecs;
}

void StringUtils::SecondsToTimeString(long lSeconds, CStdString& strHMS, bool bMustUseHHMMSS)
{
  int hh = lSeconds / 3600;
  lSeconds = lSeconds % 3600;
  int mm = lSeconds / 60;
  int ss = lSeconds % 60;

  if (hh >= 1 || bMustUseHHMMSS)
    strHMS.Format("%2.2i:%02.2i:%02.2i", hh, mm, ss);
  else
    strHMS.Format("%i:%02.2i", mm, ss);
}

CStdString StringUtils::SystemTimeToString(const SYSTEMTIME &dateTime)
{
  CStdString time;
  time.Format("%04.2i-%02.2i-%02.2i %02.2i:%02.2i:%02.2i", 
          dateTime.wYear, dateTime.wMonth, dateTime.wDay,
          dateTime.wHour, dateTime.wMinute, dateTime.wSecond);
  return time;
}

bool StringUtils::IsNaturalNumber(const CStdString& str)
{
  if (0 == (int)str.size())
    return false;
  for (int i = 0; i < (int)str.size(); i++)
  {
    if ((str[i] < '0') || (str[i] > '9')) return false;
  }
  return true;
}

void StringUtils::RemoveCRLF(CStdString& strLine)
{
  while ( strLine.size() && (strLine.Right(1) == "\n" || strLine.Right(1) == "\r") )
  {
    strLine = strLine.Left((int)strLine.size() - 1);
  }
}

CStdString StringUtils::SizeToString(__int64 size)
{
  CStdString strLabel;
  // file < 1 kbyte?
  if (size < 1024)
  {
    //  substract the integer part of the float value
    float fRemainder = (((float)size) / 1024.0f) - floor(((float)size) / 1024.0f);
    float fToAdd = 0.0f;
    if (fRemainder < 0.01f)
      fToAdd = 0.1f;
    strLabel.Format("%2.1f KB", (((float)size) / 1024.0f) + fToAdd);
    return strLabel;
  }
  const __int64 iOneMeg = 1024 * 1024;

  // file < 1 megabyte?
  if (size < iOneMeg)
  {
    strLabel.Format("%02.1f KB", ((float)size) / 1024.0f);
    return strLabel;
  }

  // file < 1 GByte?
  __int64 iOneGigabyte = iOneMeg;
  iOneGigabyte *= (__int64)1000;
  if (size < iOneGigabyte)
  {
    strLabel.Format("%02.1f MB", ((float)size) / ((float)iOneMeg));
    return strLabel;
  }
  //file > 1 GByte
  int iGigs = 0;
  __int64 dwFileSize = size;
  while (dwFileSize >= iOneGigabyte)
  {
    dwFileSize -= iOneGigabyte;
    iGigs++;
  }
  float fMegs = ((float)dwFileSize) / ((float)iOneMeg);
  fMegs /= 1000.0f;
  fMegs += iGigs;
  strLabel.Format("%02.1f GB", fMegs);

  return strLabel;
}