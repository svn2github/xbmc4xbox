
#pragma once

#include "DVDSubtitleParser.h"
#include "DVDSubtitleLineCollection.h"

class CDVDSubtitleParserSubrip : public CDVDSubtitleParser
{
public:
  CDVDSubtitleParserSubrip(CDVDSubtitleStream* pStream, const std::string& strFile);
  virtual ~CDVDSubtitleParserSubrip();
  
  virtual bool Open(CDVDStreamInfo &hints);
  virtual void Dispose();
  virtual void Reset();
  
  virtual CDVDOverlay* Parse(double iPts);

private:
  int ParseFile();
  
  CDVDSubtitleLineCollection m_collection;
};
