
#include "stdafx.h"
/*
 * XBoxMediaPlayer
 * Copyright (c) 2002 Frodo
 * Portions Copyright (c) by the authors of ffmpeg and xvid
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
*/
#include "FileISO.h"
#include "../sectionLoader.h"

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////
//*********************************************************************************************
CFileISO::CFileISO()
{
	m_bOpened=false;
}

//*********************************************************************************************
CFileISO::~CFileISO()
{
	if (m_bOpened)
	{
		Close();
	}
}
//*********************************************************************************************
bool CFileISO::Open(const char* strUserName, const char* strPassword,const char* strHostName, const char* strFileName, int iport,bool bBinary)
{
	
	string strFName="\\";
	strFName+=strFileName;
	for (int i=0; i < (int)strFName.size(); ++i )
	{
		if (strFName[i]=='/') strFName[i]='\\';
	}
  m_hFile=m_isoReader.OpenFile((char*)strFName.c_str());
  if (m_hFile == INVALID_HANDLE_VALUE)
	{
    m_bOpened=false;
    return false;
	}

  m_bOpened=true;
	// cache text files...
	if (!bBinary)
	{
		m_cache.Create(10000);
	}
	return true;
}

//*********************************************************************************************
unsigned int CFileISO::Read(void *lpBuf, __int64 uiBufSize)
{
	if (!m_bOpened) return 0;
	if (m_cache.Size() > 0)
	{
		long lTotalBytesRead=0;
		while (uiBufSize> 0)
		{
			if (m_cache.GetMaxReadSize() )
			{
				long lBytes2Read=m_cache.GetMaxReadSize();
				if (lBytes2Read>uiBufSize) lBytes2Read = (long)uiBufSize;
				m_cache.ReadBinary((char*)lpBuf,lBytes2Read );
				uiBufSize-=lBytes2Read ;
				lTotalBytesRead+=lBytes2Read ;
			}

			if (m_cache.GetMaxWriteSize() > 5000)
			{
				byte buffer[5000];
        long lBytesRead=m_isoReader.ReadFile( m_hFile,buffer,sizeof(buffer));
				if (lBytesRead > 0)
				{
					m_cache.WriteBinary((char*)buffer,lBytesRead);
				}
				else
				{
					break;
				}
			}
		}
		return lTotalBytesRead;
	}
  return m_isoReader.ReadFile( m_hFile,(byte*)lpBuf,(long)uiBufSize);
}

//*********************************************************************************************
void CFileISO::Close()
{
	if (!m_bOpened) return ;
  m_isoReader.CloseFile( m_hFile);
}

//*********************************************************************************************
__int64 CFileISO::Seek(__int64 iFilePosition, int iWhence)
{
	if (!m_bOpened) return -1;
  __int64 lNewPos=m_isoReader.Seek(m_hFile,iFilePosition,iWhence);
	m_cache.Clear();
	return lNewPos;
}

//*********************************************************************************************
__int64 CFileISO::GetLength()
{
	if (!m_bOpened) return -1;
  return m_isoReader.GetFileSize(m_hFile);
}

//*********************************************************************************************
__int64 CFileISO::GetPosition()
{
	if (!m_bOpened) return -1;
  return m_isoReader.GetFilePosition(m_hFile);
}


//*********************************************************************************************
bool CFileISO::ReadString(char *szLine, int iLineLength)
{
	if (!m_bOpened) return false;
	int iBytesRead=0;
	__int64 iPrevFilePos = GetPosition();
	strcpy(szLine,"");
	while (1)
	{
		char chKar;
		int iRead=Read((unsigned char*)&chKar, 1);
		if (iRead <= 0)
		{
			return false;
		}

		szLine[iBytesRead]=chKar;

		if ('\n' == szLine[iBytesRead])
		{
			szLine[iBytesRead]=0;
			m_cache.PeakBinary(&chKar,1);
			if (chKar== '\r')
			{
				Read((unsigned char*)&chKar, 1);
			}
			return true;
		}

		if ('\r' == szLine[iBytesRead])
		{
			szLine[iBytesRead]=0;
			m_cache.PeakBinary(&chKar,1);
			if(chKar== '\n')
			{
				Read((unsigned char*)&chKar, 1);
			}
			return true;
		}
		iBytesRead++;
		if (iBytesRead>=iLineLength)
		{
			Seek(iPrevFilePos);
			return false;
		}
	}
	return false;
}

bool CFileISO::Exists(const char* strUserName, const char* strPassword,const char* strHostName, const char* strFileName,int iport)
{
	string strFName="\\";
	strFName+=strFileName;
	for (int i=0; i < (int)strFName.size(); ++i )
	{
		if (strFName[i]=='/') strFName[i]='\\';
	}
	m_hFile=m_isoReader.OpenFile((char*)strFName.c_str());
	if (m_hFile == INVALID_HANDLE_VALUE)
		return false;

  m_isoReader.CloseFile(m_hFile);
	return true;
}
