// FileSmb.h: interface for the CFileSMB class.

//

//////////////////////////////////////////////////////////////////////



#if !defined(AFX_FILESMB_H__2C4AB5BC_0742_458D_95EA_E9C77BA5663D__INCLUDED_)

#define AFX_FILESMB_H__2C4AB5BC_0742_458D_95EA_E9C77BA5663D__INCLUDED_



#if _MSC_VER > 1000

#pragma once

#endif // _MSC_VER > 1000



#include "../lib/libsmb/smb++.h"
#include "IFile.h"

using namespace XFILE;

namespace XFILE
{
	class CFileSMB : public IFile  
	{
	public:
		CFileSMB();
		virtual ~CFileSMB();
		virtual void					Close();
		virtual offset_t			Seek(offset_t iFilePosition, int iWhence=SEEK_SET);
		virtual bool					ReadString(char *szLine, int iLineLength);
		virtual unsigned int	Read(void* lpBuf, offset_t uiBufSize);
		virtual bool					Open(const char* strUserName, const char* strPassword,const char* strHostName, const char* strFileName,int iport, bool bBinary=true);
		virtual offset_t			GetLength();
		virtual offset_t			GetPosition();
		void									Lock();
		void									Unlock();
		void									SetLogin(const char* strLogin, const char* strPassword);
		SMB*									GetSMB();
	protected:
		static SMB*			m_pSMB;
		static int			m_iReferences;
		offset_t	m_fileSize;
		bool			m_bBinary;
		int				m_fd;
	};
};

#endif // !defined(AFX_FILESMB_H__2C4AB5BC_0742_458D_95EA_E9C77BA5663D__INCLUDED_)
