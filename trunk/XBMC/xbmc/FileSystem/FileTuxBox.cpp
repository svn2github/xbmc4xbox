#include "../stdafx.h"
#include "FileTuxBox.h"
#include "../utils/Http.h"

//Reserved for TuxBox Recording!

CFileTuxBox::CFileTuxBox()
{}

CFileTuxBox::~CFileTuxBox()
{
}

__int64 CFileTuxBox::GetPosition()
{
  return 0;
}

__int64 CFileTuxBox::GetLength()
{
  return 0;
}

bool CFileTuxBox::Open(const CURL& url, bool bBinary)
{
  return true;
}

unsigned int CFileTuxBox::Read(void* lpBuf, __int64 uiBufSize)
{
  return 0;
}

__int64 CFileTuxBox::Seek(__int64 iFilePosition, int iWhence)
{
  return 0;
}

void CFileTuxBox::Close()
{
}