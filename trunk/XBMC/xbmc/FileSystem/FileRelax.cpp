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
#include "FileRelax.h"
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>


//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CFileRelax::CFileRelax()
{
	m_filePos=0;
	m_fileSize=0;
	m_bOpened=false;
}

CFileRelax::~CFileRelax()
{
	Close();
}
//*********************************************************************************************
bool CFileRelax::Open(const char* strUserName, const char* strPassword,const char* strHostName, const char* strFileName,int iport, bool bBinary)
{
	if (m_bOpened) Close();
	m_bOpened=false;
	m_filePos=0;
	m_fileSize=0;
	//mp_msg(0,0,"Opening %s:%s\n", strHostName,strFileName);

	char cmd[1024];
	char result[32];

	sockaddr_in service;

	m_socket = socket(AF_INET,SOCK_STREAM,IPPROTO_TCP);

	service.sin_family = AF_INET;
	service.sin_addr.s_addr = inet_addr( strHostName );
	service.sin_port = htons(iport);
		
	// attempt to connection
	if (connect(m_socket,(sockaddr*) &service,sizeof(struct sockaddr)) == SOCKET_ERROR)
	{
		//mp_msg(0,MSGL_WARN, "Unable to establish a connection with server.\n" );
		m_socket=NULL;
		return false;
	}
	char optval=1;
	setsockopt(m_socket,IPPROTO_TCP,TCP_NODELAY,&optval,1);

	int ioptval=0;
	setsockopt(m_socket,SOL_SOCKET,SO_SNDBUF,&optval,sizeof(int)); 

	// validate greeting
	memset(cmd,0,256);
	if (!Send((unsigned char*)"HELLO XSTREAM 6.0\r\n",19))
	{
		closesocket(m_socket);
		m_socket=NULL;
		return false;
	}
	if (!Recv((unsigned char*)cmd,11))
	{		
		closesocket(m_socket);
		m_socket=NULL;
		return false;
	}
	cmd[12]=0;

	if (strcmp(cmd,"HELLO XBOX!")!=0)
	{
		closesocket(m_socket);
		m_socket=NULL;
		//mp_msg(0,MSGL_WARN,  "Unable to negotiate protocol with server\n" );
		return false;
	}

	// attempt open file		
	// ???
//	if (strFileName[0]=='/')
//		sprintf(cmd,"OPEN,%s",&strFileName[1]);
//	else
		sprintf(cmd,"OPEN,%s",strFileName);
	if (!Send((unsigned char*)cmd,strlen(cmd) ) )
	{
		closesocket(m_socket);
		m_socket=NULL;
		return false;
	}

	if (!Recv((unsigned char*)result,32))
	{
		closesocket(m_socket);
		m_socket=NULL;
		return false;
	}
	INT64 sending = _atoi64(result);
	if (sending<=0)
	{
		closesocket(m_socket);
		m_socket=NULL;
		//mp_msg(0,MSGL_WARN, "Server unable to fulfill request.\n" );
		return false;
	}

	m_filePos=0;
	m_fileSize = sending;
	//mp_msg(0,0,"opened socket:%x filesize:%x\n", m_socket,m_fileSize);
	m_bOpened=true;
	return true;
}

//*********************************************************************************************
unsigned int CFileRelax::Read(void *lpBuf, offset_t uiBufSize)
{
	char cmd[1024];
	char result[1024];
	char szOffset[32];
	char* pBuffer=(char* )lpBuf;

	// TODO: Frodo, this function is of the type (unsigned int).
	// So, -1, shouldn't that be 0?
	if (!m_bOpened)
		return -1;

	if (m_filePos>=m_fileSize)
	{
		//mp_msg(0,MSGL_WARN,"try 2 read past end of file %x/%x\n", m_filePos, m_fileSize);
		return 0;
	}
	// send request
	_i64toa(m_filePos,szOffset,10);
	sprintf(cmd,"READ,%s,%u",szOffset,uiBufSize);
	if (!Send((unsigned char*)cmd,strlen(cmd)))
	{
		return 0;
	}

	
	if (!Recv((unsigned char*)result,32))
	{
		return 0;
	}
	long sending = atol(result);
	if (sending<=0)
	{
		result[20]=0;
		//mp_msg(0,0, "CFileRelax::Read()  '%s' returned %s.\n" ,cmd,result);

		return 0;
	}

	// read

	if (!Recv((unsigned char*)pBuffer,sending))
	{
		return 0;
	}


	m_filePos += sending;
	//mp_msg(0,0,"file pos:%x", m_filePos);
	return sending;
}

//*********************************************************************************************
void CFileRelax::Close()
{
	//mp_msg(0,0,"relax close\n");
	if (m_bOpened && m_socket) 
	{
		char result[32];

		Send((byte*)"CLSE",4);
		Recv((byte*)result,32);
		closesocket(m_socket);	
	}
	m_socket=NULL;
	m_bOpened=false;
	m_filePos=0;
	m_fileSize=0;
}

//*********************************************************************************************
offset_t CFileRelax::Seek(offset_t iFilePosition, int iWhence)
{
	if (!m_bOpened) return 0;
	switch(iWhence) 
	{
		case SEEK_SET:
			// cur = pos
			m_filePos = iFilePosition;
			break;
		case SEEK_CUR:
			// cur += pos
			m_filePos += iFilePosition;
			break;
		case SEEK_END:
			// end -= pos
			m_filePos = m_fileSize - iFilePosition;
			break;
	}
	// Return offset from beginning
	if (m_filePos < 0) m_filePos =0;
	if (m_filePos > m_fileSize) m_filePos=m_fileSize;
	return m_filePos;
}

//*********************************************************************************************
offset_t CFileRelax::GetLength()
{
	if (!m_bOpened) return 0;
	return m_fileSize;
}

//*********************************************************************************************
offset_t CFileRelax::GetPosition()
{
	if (!m_bOpened) return 0;
	return m_filePos;
}


//*********************************************************************************************
bool CFileRelax::ReadString(char *szLine, int iLineLength)
{
	if (!m_bOpened) return false;
	offset_t iFilePos=GetPosition();

	int iBytesRead=Read( (unsigned char*)szLine, iLineLength);
	if (iBytesRead <= 0)
	{
		return false;
	}

	szLine[iBytesRead]=0;

	for (int i=0; i < iBytesRead; i++)
	{
		if ('\n' == szLine[i])
		{
			if ('\r' == szLine[i+1])
			{
				szLine[i+2]=0;
				Seek(iFilePos+i+2,SEEK_SET);
			}
			else
			{
				// end of line
				szLine[i+1]=0;
				Seek(iFilePos+i+1,SEEK_SET);
			}
			break;
		}
		else if ('\r'==szLine[i])
		{
			if ('\n' == szLine[i+1])
			{
				szLine[i+2]=0;
				Seek(iFilePos+i+2,SEEK_SET);
			}
			else
			{
				// end of line
				szLine[i+1]=0;
				Seek(iFilePos+i+1,SEEK_SET);
			}
			break;
		}
	}
	if (iBytesRead>0) 
	{
		return true;
	}
	return false;
}

//*********************************************************************************************
bool CFileRelax::Send(byte* pBuffer, int iLen)
{
	int iPos=0;
	int iOrgLen=iLen;
	while (iLen>0)
	{
		int iErr=send(m_socket,(const char*)&pBuffer[iPos],iLen,0);
		if (iErr>0)
		{
			iPos+=iLen;
			iLen-=iPos;
		}
		else
		{
			iErr=WSAGetLastError();
			if (iErr != WSAEINPROGRESS) 
			{
				//mp_msg(0,MSGL_WARN,"Send %i bytes failed:%i\n", iOrgLen,iErr);
				return false;
			}
		}
	}
	return true;
}

//*********************************************************************************************
bool CFileRelax::Recv(byte* pBuffer, int iLen)
{
	int iLenOrg=iLen;
	long bytesRead = 0;
	while (iLen>0)
	{
		long lenRead=recv(m_socket,(char*)&pBuffer[bytesRead],iLen,0);
		if (lenRead > 0)
		{
			bytesRead+=lenRead;
			iLen-=lenRead;
		}
		else
		{
			//mp_msg(0,MSGL_WARN,"failed 2 read %i/%i bytes result %i\n", bytesRead,iLenOrg,WSAGetLastError());
			return false;
		}
	}
	return true;
	
}