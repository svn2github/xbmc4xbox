#pragma once

#include "StdString.h"
#include <vector>

class CFileItem;  // forward

class CLabelFormatter
{
public:
  CLabelFormatter(const CStdString &mask, const CStdString &mask2);

  void FormatLabel(CFileItem *item);
  void FormatLabel2(CFileItem *item);
  void FormatLabels(CFileItem *item) // convenient shorthand
  {
    FormatLabel(item);
    FormatLabel2(item);
  }

private:
  class CMaskString
  {
  public:
    CMaskString(const CStdString &prefix, char content, const CStdString &postfix)
    {
      m_prefix = prefix; m_content = content; m_postfix = postfix;
    };
    CStdString m_prefix;
    CStdString m_postfix;
    char m_content;
  };

  // functions for assembling the mask vectors
  void AssembleMask(unsigned int label, const CStdString &mask);
  void SplitMask(unsigned int label, const CStdString &mask);

  // functions for retrieving content based on our mask vectors
  CStdString GetContent(unsigned int label, const CFileItem *item);
  CStdString GetMaskContent(const CMaskString &mask, const CFileItem *item);

  vector<CStdString>   m_staticContent[2];
  vector<CMaskString>  m_dynamicContent[2];
  bool                 m_hideFileExtensions;
};
