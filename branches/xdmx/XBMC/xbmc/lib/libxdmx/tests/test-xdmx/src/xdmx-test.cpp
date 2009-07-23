
#include "xdmx.h"
#include "tsdemux.h"
#include <iostream>
#include <sstream>
#include <fstream>
#include <map>

class CFileStream : public IXdmxInputStream
{
public:
  CFileStream();
  virtual ~CFileStream();
  virtual unsigned int Read(unsigned char* buf, unsigned int len);
  virtual __int64 Seek(__int64 offset, int whence);
  virtual __int64 GetLength();
  virtual bool IsEOF();
  bool Open(const char* pFilePath);
protected:
  std::string m_FileName;
  unsigned int m_FileLen;
  std::fstream m_InputFile;
};

CFileStream::CFileStream() :
  m_FileName(""),
  m_FileLen(0)
{
    
}

CFileStream::~CFileStream()
{
  m_InputFile.close();
}

unsigned int CFileStream::Read(unsigned char* buf, unsigned int len)
{
  if (IsEOF())
    return 0;
  
  m_InputFile.read((char*)buf, len);
  if (!m_InputFile.fail())
    return len;
  return 0;
}

__int64 CFileStream::Seek(__int64 offset, int whence)
{
  return m_FileLen;
}

__int64 CFileStream::GetLength()
{
  return 0;
}

bool CFileStream::IsEOF()
{
  if (m_InputFile.is_open())
    return m_InputFile.eof();
  return true;
}

bool CFileStream::Open(const char* pFilePath)
{
  if (m_InputFile.is_open())
    m_InputFile.close();
  m_InputFile.open(pFilePath, std::ios::in | std::ios::binary);
  if (!m_InputFile.fail())
  {
    m_FileName = pFilePath;
    return true;
  }
  return false;
}

//////////////////////////////////////////////////////////////////////////////////////////

typedef struct DemuxPacket
{
  unsigned char* pData;   // data
  int iSize;     // data size
  int iStreamId; // integer representing the stream index
  int iGroupId;  // the group this data belongs to, used to group data from different streams together

  double pts; // pts in DVD_TIME_BASE
  double dts; // dts in DVD_TIME_BASE
  double duration; // duration in DVD_TIME_BASE if available
} DemuxPacket;

////////////////////////////////////////////////////////////////////////////////////

std::map<unsigned short, unsigned int> m_StreamMap;
double m_StartTime = -1.0;
ITransportStreamDemux* m_pDemux;
CFileStream* m_pInput; 
int main() 
{
  m_pInput = new CFileStream();
  if (!m_pInput->Open("Z:\\video\\HD Movies\\Casino Royale (BluRay).m2ts"))
  {
    return 1;
  }
  m_pDemux = CreateTSDemux();
  if (!m_pDemux->Open(dynamic_cast<IXdmxInputStream*>(m_pInput)))
  {
    return 2;
  }
  // Get the last Program in the Transport Stream
  CTransportStream* pTransportStream = m_pDemux->GetTransportStream();
  CTSProgram* pProgram = pTransportStream->GetProgram(pTransportStream->GetProgramCount() - 1); 

  // Map Stream Id's to Indexes
  for (unsigned int s = 0; s < pProgram->GetStreamCount(); s++)
  {
    CElementaryStream* pStream = pProgram->GetStream(s);
    m_StreamMap[pStream->GetId()] = s;
  }

  for (CParserPayload* pPayload = m_pDemux->GetPayload(); pPayload != NULL; pPayload = m_pDemux->GetPayload())
  {
    DemuxPacket dmx;
    dmx.iSize = pPayload->GetSize();
    dmx.pData = pPayload->Detach();
    dmx.iStreamId = m_StreamMap[pPayload->GetStream()->GetId()];
    dmx.pts = pPayload->GetPts();
    dmx.dts = pPayload->GetDts();
    if (m_StartTime == -1.0)
    {
      m_StartTime = dmx.dts;
    }
    dmx.dts -= m_StartTime;
    dmx.pts -= m_StartTime;
    printf("*** Got One *** stream: %d, size: %lu, pts: %0.06f, dts: %0.06f\n", dmx.iStreamId, dmx.iSize, dmx.pts, dmx.dts);
    delete pPayload;
    delete[] dmx.pData;
  }

	return 0;
}
