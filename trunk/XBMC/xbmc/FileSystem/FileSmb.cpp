// FileSmb.cpp: implementation of the CFileSMB class.

//

//////////////////////////////////////////////////////////////////////

#include "FileSmb.h"
#include "../sectionloader.h"
#include "../settings.h"
using namespace XFILE;

void xb_smbc_auth(const char *srv, const char *shr, char *wg, int wglen, 
			char *un, int unlen,  char *pw, int pwlen)
{
	return;
}

CSMB::CSMB()
{
	InitializeCriticalSection(&m_critSection);
	binitialized = false;
}

CSMB::~CSMB()
{
	DeleteCriticalSection(&m_critSection);
}

void CSMB::Init()
{
	if(!binitialized)
	{
		// set ip and subnet
		set_xbox_interface(g_stSettings.m_strLocalIPAdres, g_stSettings.m_strLocalNetmask);

		if(!smbc_init(xb_smbc_auth, 1/*Debug Level*/))
		{
			// set wins nameserver
			lp_do_parameter((-1), "wins server", g_stSettings.m_strNameServer);
			binitialized = true;
		}
	}
}

void CSMB::Lock()
{
	::EnterCriticalSection(&m_critSection);
}

void CSMB::Unlock()
{
	::LeaveCriticalSection(&m_critSection);
}

CSMB smb;

CFileSMB::CFileSMB()
{
	smb.Init();
	m_fd = -1;
}

CFileSMB::~CFileSMB()
{
	Close();
}

offset_t CFileSMB::GetPosition()
{
	if (m_fd == -1) return 0;
	smb.Lock();
	offset_t pos = smbc_lseek(m_fd, 0, SEEK_CUR);
	smb.Unlock();
	if( pos < 0 )
		return 0;
	return pos;
}

offset_t CFileSMB::GetLength()
{
	if (m_fd == -1) return 0;
	return m_fileSize;
}

bool CFileSMB::Open(const char* strUserName, const char* strPassword,const char *strHostName, const char *strFileName,int iport, bool bBinary)
{
	char szFileName[1024];
	m_bBinary = bBinary;

	// since the syntax of the new smb is a little different, the workgroup is now specified
	// as workgroup;username:pass@pc/share (strUserName contains workgroup;username)
	// instead of username:pass@workgroup/pc/share
	// this means that if no password and username is provided szFileName doesn't have the workgroup.
	// should be fixed.

	if (strPassword && strUserName)
		sprintf(szFileName,"smb://%s:%s@%s/%s", strUserName, strPassword, strHostName, strFileName);
	else
		sprintf(szFileName,"smb://%s/%s", strHostName, strFileName);

	Close();

	// convert from string to UTF8
	char strUtfFileName[1024];
	int strLen = convert_string(CH_DOS, CH_UTF8, szFileName, (size_t)strlen(szFileName), strUtfFileName, 1024);
	strUtfFileName[strLen] = 0;

	smb.Lock();

	m_fd = smbc_open(strUtfFileName, O_RDONLY, 0);

	if(m_fd == -1)
	{
		smb.Unlock();
		return false;
	}
	UINT64 ret = smbc_lseek(m_fd, 0, SEEK_END);

	if( ret < 0 )
	{
		smbc_close(m_fd);
		m_fd = -1;
		smb.Unlock();
		return false;
	}

	m_fileSize = ret;
	ret = smbc_lseek(m_fd, 0, SEEK_SET);
	if( ret < 0 )
	{
		smbc_close(m_fd);
		m_fd = -1;
		smb.Unlock();
		return false;
	}
	// We've successfully opened the file!
	smb.Unlock();
	return true;
}

unsigned int CFileSMB::Read(void *lpBuf, offset_t uiBufSize)
{
	if (m_fd == -1) return 0;
	smb.Lock();
	int bytesRead = smbc_read(m_fd, lpBuf, (int)uiBufSize);
	smb.Unlock();
	if( bytesRead <= 0 )
	{
		char szTmp[128];
		sprintf(szTmp,"SMB returned %i errno:%i\n",bytesRead,errno);
		OutputDebugString(szTmp);
		return 0;
	}
	return (unsigned int)bytesRead;
}

bool CFileSMB::ReadString(char *szLine, int iLineLength)
{
	if (m_fd == -1) return false;
	offset_t iFilePos=GetPosition();

	smb.Lock();	
	int iBytesRead = smbc_read(m_fd, (unsigned char*)szLine, iLineLength);
	smb.Unlock();
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

offset_t CFileSMB::Seek(offset_t iFilePosition, int iWhence)
{
	if (m_fd == -1) return 0;

	smb.Lock();	
	INT64 pos = smbc_lseek(m_fd, iFilePosition, iWhence);
	smb.Unlock();

	if( pos < 0 )	return 0;

	return (offset_t)pos;
}

void CFileSMB::Close()
{
	if (m_fd != -1)
	{
		smb.Lock();
		smbc_close(m_fd);
		smb.Unlock();
	}
	m_fd = -1;
}

int CFileSMB::Write(const void* lpBuf, offset_t uiBufSize)
{
	if (m_fd == -1) return -1;

	DWORD dwNumberOfBytesWritten=0;

	// lpBuf can be safely casted to void* since xmbc_write will only read from it.
	smb.Lock();
	dwNumberOfBytesWritten = smbc_write(m_fd, (void*)lpBuf, (DWORD)uiBufSize);
	smb.Unlock();

	return (int)dwNumberOfBytesWritten;
}
