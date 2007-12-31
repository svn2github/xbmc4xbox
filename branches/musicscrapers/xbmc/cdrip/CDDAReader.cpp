#include "stdafx.h"
#include "CDDAReader.h"


#define SECTOR_COUNT 55

CCDDAReader::CCDDAReader()
{
  m_sRipBuffer[0].pbtStream = NULL;
  m_sRipBuffer[1].pbtStream = NULL;

  m_iCurrentBuffer = 0;

  m_hReadEvent = CreateEvent(NULL, false, false, NULL);
  m_hDataReadyEvent = CreateEvent(NULL, false, false, NULL);
  m_hStopEvent = CreateEvent(NULL, false, false, NULL);
}

CCDDAReader::~CCDDAReader()
{
  m_bStop = true;
  PulseEvent(m_hStopEvent);
  StopThread();

  CloseHandle(m_hReadEvent);
  CloseHandle(m_hDataReadyEvent);
  CloseHandle(m_hStopEvent);

  if (m_sRipBuffer[0].pbtStream) delete []m_sRipBuffer[0].pbtStream;
  if (m_sRipBuffer[1].pbtStream) delete []m_sRipBuffer[1].pbtStream;
}

bool CCDDAReader::Init(const char* strFileName)
{
  // Open the Ripper session
  if (!m_fileCdda.Open(strFileName))
    return false;

  CLog::Log(LOGINFO, "%s, Sectors %d", strFileName, m_fileCdda.GetLength() / CDIO_CD_FRAMESIZE_RAW);

  // allocate 2 buffers
  // read around 128k per chunk. This makes the cd reading less noisy.
  m_sRipBuffer[0].pbtStream = new BYTE[CDIO_CD_FRAMESIZE_RAW * SECTOR_COUNT];
  m_sRipBuffer[1].pbtStream = new BYTE[CDIO_CD_FRAMESIZE_RAW * SECTOR_COUNT];

  Create(false, THREAD_MINSTACKSIZE);

  return true;
}

bool CCDDAReader::DeInit()
{
  m_bStop = true;
  SetEvent(m_hStopEvent);
  StopThread();

  m_iCurrentBuffer = 0;

  // free buffers
  if (m_sRipBuffer[0].pbtStream) delete []m_sRipBuffer[0].pbtStream;
  m_sRipBuffer[0].pbtStream = NULL;
  if (m_sRipBuffer[1].pbtStream) delete []m_sRipBuffer[1].pbtStream;
  m_sRipBuffer[1].pbtStream = NULL;

  // Close the Ripper session
  m_fileCdda.Close();

  return true;
}

int CCDDAReader::GetPercent()
{
  return (int)((m_fileCdda.GetPosition()*100) / m_fileCdda.GetLength());
}

int CCDDAReader::ReadChunk()
{
  // Read data
  DWORD dwBytesRead = m_fileCdda.Read(m_sRipBuffer[m_iCurrentBuffer].pbtStream, SECTOR_COUNT * CDIO_CD_FRAMESIZE_RAW);
  if (dwBytesRead == 0)
  {
    m_sRipBuffer[m_iCurrentBuffer].lBytesRead = 0;
    return CDDARIP_ERR;
  }

  m_sRipBuffer[m_iCurrentBuffer].lBytesRead = dwBytesRead;

  if (m_fileCdda.GetPosition() == m_fileCdda.GetLength()) return CDDARIP_DONE;

  return CDDARIP_OK;
}

void CCDDAReader::Process()
{
  // fill first buffer
  m_iCurrentBuffer = 0;
  HANDLE hHandles[2] = { m_hReadEvent, m_hStopEvent };

  m_sRipBuffer[0].iRipError = ReadChunk();
  SetEvent(m_hDataReadyEvent);

  while (!m_bStop)
  {
    // wait until someone called GetData()
    if (WaitForMultipleObjects(2, hHandles, false, INFINITE) == WAIT_OBJECT_0)
    {
      // event generated by m_hReadEvent, continue
      // switch buffer and start reading in this one
      m_iCurrentBuffer = m_iCurrentBuffer ? 0 : 1;
      m_sRipBuffer[m_iCurrentBuffer].iRipError = ReadChunk();

      SetEvent(m_hDataReadyEvent);
    }
  }
}

int CCDDAReader::GetData(BYTE** stream, long& lBytes)
{
  // wait until we are sure we have a buffer that is filled
  WaitForSingleObject(m_hDataReadyEvent, INFINITE);

  int iError = m_sRipBuffer[m_iCurrentBuffer].iRipError;
  *stream = m_sRipBuffer[m_iCurrentBuffer].pbtStream;
  lBytes = m_sRipBuffer[m_iCurrentBuffer].lBytesRead;

  // got data buffer, signal thread so it can start filling the other buffer
  SetEvent(m_hReadEvent);
  return iError;
}
