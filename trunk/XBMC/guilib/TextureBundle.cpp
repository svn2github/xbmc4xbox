#include <xtl.h>
#include <XGraphics.h>
#include ".\texturebundle.h"
#include "GraphicContext.h"
#include "../xbmc/utils/log.h"
#include "../xbmc/lib/liblzo/LZO1X.H"

#pragma comment(lib,"xbmc/lib/liblzo/lzo.lib")

enum XPR_FLAGS
{
	XPRFLAG_PALETTE = 0x00000001,
	XPRFLAG_ANIM =    0x00000002,
};

class CAutoBuffer
{
	BYTE* p;
public:
	CAutoBuffer() { p = 0; }
	explicit CAutoBuffer(size_t s) { p = (BYTE*)malloc(s); }
	~CAutoBuffer() { if (p) free(p); }
	operator BYTE*() { return p; }
	void Set(BYTE* buf) { if (p) free(p); p = buf; }
	bool Resize(size_t s);
	void Release() { p = 0; }
};

bool CAutoBuffer::Resize(size_t s)
{
	if (s == 0) 
	{
		if (!p)
			return false;
		free(p);
		p = 0;
		return true;
	}
	void* q = realloc(p, s);
	if (q)
	{
		p = (BYTE*)q;
		return true;
	}
	return false;
}

// as above but for texture allocation (do not change from XPhysicalAlloc!)
class CAutoTexBuffer
{
	BYTE* p;
public:
	CAutoTexBuffer() { p = 0; }
	explicit CAutoTexBuffer(size_t s) { p = (BYTE*)XPhysicalAlloc(s, MAXULONG_PTR, 128, PAGE_READWRITE); }
	~CAutoTexBuffer() { if (p) XPhysicalFree(p); }
	operator BYTE*() { return p; }
	BYTE* Set(BYTE* buf) { if (p) XPhysicalFree(p); return p = buf; }
	void Release() { p = 0; }
};

CTextureBundle::CTextureBundle(void)
{
	m_hFile = INVALID_HANDLE_VALUE;
	m_CurFileHeader[0] = m_FileHeaders.end();
	m_CurFileHeader[1] = m_FileHeaders.end();
	m_PreLoadBuffer[0] = 0;
	m_PreLoadBuffer[1] = 0;
	m_Ovl[0].hEvent = CreateEvent(0, TRUE, TRUE, 0);
	m_Ovl[1].hEvent = CreateEvent(0, TRUE, TRUE, 0);
}

CTextureBundle::~CTextureBundle(void)
{
	if (m_hFile != INVALID_HANDLE_VALUE)
		CloseHandle(m_hFile);
	if (m_PreLoadBuffer[0])
		free(m_PreLoadBuffer[0]);
	if (m_PreLoadBuffer[1])
		free(m_PreLoadBuffer[1]);
	CloseHandle(m_Ovl[0].hEvent);
	CloseHandle(m_Ovl[1].hEvent);
}

bool CTextureBundle::OpenBundle()
{
	if (m_hFile != INVALID_HANDLE_VALUE)
		Cleanup();

	CStdString strPath=g_graphicsContext.GetMediaDir();
	strPath += "\\media\\Textures.xpr";

	m_hFile = CreateFile(strPath.c_str(), GENERIC_READ, FILE_SHARE_READ, 0, OPEN_EXISTING, FILE_FLAG_NO_BUFFERING|FILE_FLAG_OVERLAPPED, 0);
	if (m_hFile == INVALID_HANDLE_VALUE)
		return false;

	CAutoBuffer HeaderBuf(512);
	DWORD n;

	m_Ovl[0].Offset = 0;
	m_Ovl[0].OffsetHigh = 0;
	if (!ReadFile(m_hFile, HeaderBuf, 512, &n, &m_Ovl[0]) && GetLastError() != ERROR_IO_PENDING)
		goto LoadError;
	if (!GetOverlappedResult(m_hFile, &m_Ovl[0], &n, TRUE) || n < 512)
		goto LoadError;

	XPR_HEADER* pXPRHeader = (XPR_HEADER*)(BYTE*)HeaderBuf;
	int Version = (pXPRHeader->dwMagic >> 24) - '0';
	pXPRHeader->dwMagic -= Version << 24;

	if (pXPRHeader->dwMagic != XPR_MAGIC_VALUE || Version < 2)
		goto LoadError;

	DWORD HeaderSize = pXPRHeader->dwHeaderSize;
	HeaderSize = (HeaderSize - 1) & ~511; // align to sector, but remove the first sector
	HeaderBuf.Resize(HeaderSize + 512);

	m_Ovl[0].Offset = 512;
	if (!ReadFile(m_hFile, HeaderBuf + 512, HeaderSize, &n, &m_Ovl[0]) && GetLastError() != ERROR_IO_PENDING)
		goto LoadError;
	if (!GetOverlappedResult(m_hFile, &m_Ovl[0], &n, TRUE) || n < HeaderSize)
		goto LoadError;

#ifdef CACHE_WHOLE_BUNDLE
	// Do this here before creating map so it runs in background during the create
	m_CacheOffset = HeaderSize + 512;
	DWORD DataSize = ((pXPRHeader->dwTotalSize - m_CacheOffset) + 511) & ~511;
	m_BundleCache = (BYTE*)VirtualAlloc(0, DataSize, MEM_COMMIT, PAGE_READWRITE);
	if (m_BundleCache)
	{
		m_Ovl[0].Offset = m_CacheOffset;
		if (!ReadFile(m_hFile, m_BundleCache, DataSize, &n, &m_Ovl[0]) && GetLastError() != ERROR_IO_PENDING)
		{
			VirtualFree(m_BundleCache, 0, MEM_RELEASE);
			m_BundleCache = 0;
		}
	}
#endif

	struct DiskFileHeader_t
	{
		char Name[52];
		DWORD Offset;
		DWORD UnpackedSize;
		DWORD PackedSize;
	} *FileHeader;
	FileHeader = (DiskFileHeader_t*)(HeaderBuf + sizeof(XPR_HEADER));

	n = (pXPRHeader->dwHeaderSize - sizeof(XPR_HEADER)) / sizeof(DiskFileHeader_t);
	for (unsigned i = 0; i < n; ++i)
	{
		std::pair<CStdString, FileHeader_t> entry;
		entry.first = FileHeader[i].Name;
		entry.first.Normalize();
		entry.second.Offset = FileHeader[i].Offset;
		entry.second.UnpackedSize = FileHeader[i].UnpackedSize;
		entry.second.PackedSize = FileHeader[i].PackedSize;
		m_FileHeaders.insert(entry);
	}
	m_CurFileHeader[0] = m_FileHeaders.end();
	m_CurFileHeader[1] = m_FileHeaders.end();
	m_PreloadIdx = m_LoadIdx = 0;

	lzo_init();

	return true;

LoadError:
	CloseHandle(m_hFile); m_hFile = INVALID_HANDLE_VALUE;
	return false;
}

void CTextureBundle::Cleanup()
{
	if (m_hFile != INVALID_HANDLE_VALUE)
		CloseHandle(m_hFile);
	m_hFile = INVALID_HANDLE_VALUE;
	
	m_FileHeaders.clear();
	
	m_CurFileHeader[0] = m_FileHeaders.end();
	m_CurFileHeader[1] = m_FileHeaders.end();
	
	if (m_PreLoadBuffer[0])
		free(m_PreLoadBuffer[0]);
	m_PreLoadBuffer[0] = 0;
	if (m_PreLoadBuffer[1])
		free(m_PreLoadBuffer[1]);
	m_PreLoadBuffer[1] = 0;

#ifdef CACHE_WHOLE_BUNDLE
	if (m_BundleCache)
		VirtualFree(m_BundleCache, 0, MEM_RELEASE);
	m_BundleCache = 0;
#endif
}

bool CTextureBundle::HasFile(const CStdString& Filename)
{
	if (m_hFile == INVALID_HANDLE_VALUE && !OpenBundle())
		return false;

	CStdString name(Filename);
	name.Normalize();
	return m_FileHeaders.find(name) != m_FileHeaders.end();
}

bool CTextureBundle::PreloadFile(const CStdString& Filename)
{
	CStdString name(Filename);
	name.Normalize();

#ifdef CACHE_WHOLE_BUNDLE
	if (m_BundleCache)
	{
		if (m_Ovl[0].Offset)
		{
			DWORD n;
			if (!GetOverlappedResult(m_hFile, &m_Ovl[0], &n, TRUE))
			{
				VirtualFree(m_BundleCache, 0, MEM_RELEASE);
				m_BundleCache = 0;
			}
			m_Ovl[0].Offset = 0;
		}

		if (m_BundleCache)
		{
			m_CurFileHeader[m_PreloadIdx] = m_FileHeaders.find(name);
			if (m_CurFileHeader[m_PreloadIdx] != m_FileHeaders.end())
			{
				m_PreLoadBuffer[m_PreloadIdx] = m_BundleCache + (m_CurFileHeader[m_PreloadIdx]->second.Offset - m_CacheOffset);
				m_PreloadIdx = 1 - m_PreloadIdx;
				return true;
			}
			return false;
		}
	}
#endif

	if (m_PreLoadBuffer[m_PreloadIdx])
		free(m_PreLoadBuffer[m_PreloadIdx]);
	m_PreLoadBuffer[m_PreloadIdx] = 0;

	m_CurFileHeader[m_PreloadIdx] = m_FileHeaders.find(name);
	if (m_CurFileHeader[m_PreloadIdx] != m_FileHeaders.end())
	{
		if (!HasOverlappedIoCompleted(&m_Ovl[m_PreloadIdx]))
		{
			bool FlushBuf = !HasOverlappedIoCompleted(&m_Ovl[1-m_PreloadIdx]);
			CancelIo(m_hFile);
			if (FlushBuf)
			{
				free(m_PreLoadBuffer[1-m_PreloadIdx]);
				m_PreLoadBuffer[1-m_PreloadIdx] = 0;
				m_CurFileHeader[1-m_PreloadIdx] = m_FileHeaders.end();
			}
		}

		// preload texture
		DWORD ReadSize = (m_CurFileHeader[m_PreloadIdx]->second.PackedSize + 511) & ~511;
		m_PreLoadBuffer[m_PreloadIdx] = (BYTE*)malloc(ReadSize);

		if (m_PreLoadBuffer[m_PreloadIdx])
		{
			m_Ovl[m_PreloadIdx].Offset = m_CurFileHeader[m_PreloadIdx]->second.Offset;
			m_Ovl[m_PreloadIdx].OffsetHigh = 0;

			DWORD n;
			if (!ReadFile(m_hFile, m_PreLoadBuffer[m_PreloadIdx], ReadSize, &n, &m_Ovl[m_PreloadIdx]) && GetLastError() != ERROR_IO_PENDING)
			{
				free(m_PreLoadBuffer[m_PreloadIdx]);
				m_PreLoadBuffer[m_PreloadIdx] = 0;
				m_CurFileHeader[m_PreloadIdx] = m_FileHeaders.end();
				return false;
			}

			m_PreloadIdx = 1 - m_PreloadIdx;
			return true;
		}
	}
	return false;
}

HRESULT CTextureBundle::LoadFile(const CStdString& Filename, CAutoTexBuffer& UnpackedBuf)
{
	CStdString name(Filename);
	name.Normalize();
	if (m_CurFileHeader[0] != m_FileHeaders.end() && m_CurFileHeader[0]->first == name)
		m_LoadIdx = 0;
	else if (m_CurFileHeader[1] != m_FileHeaders.end() && m_CurFileHeader[1]->first == name)
		m_LoadIdx = 1;
	else
	{
		m_LoadIdx = m_PreloadIdx;
		if (!PreloadFile(Filename))
			return E_FAIL;
	}

	if (!m_PreLoadBuffer[m_LoadIdx])
		return E_OUTOFMEMORY;
	if (!UnpackedBuf.Set((BYTE*)XPhysicalAlloc(m_CurFileHeader[m_LoadIdx]->second.UnpackedSize, MAXULONG_PTR, 128, PAGE_READWRITE)))
		return E_OUTOFMEMORY;

#ifdef CACHE_WHOLE_BUNDLE
	if (!m_BundleCache)
#endif
	{
		DWORD n;
		if (!GetOverlappedResult(m_hFile, &m_Ovl[m_LoadIdx], &n, TRUE) || n < m_CurFileHeader[m_LoadIdx]->second.PackedSize)
			return E_FAIL;
	}

	lzo_uint s = m_CurFileHeader[m_LoadIdx]->second.UnpackedSize;
	HRESULT hr = S_OK;
	if (lzo1x_decompress(m_PreLoadBuffer[m_LoadIdx], m_CurFileHeader[m_LoadIdx]->second.PackedSize, UnpackedBuf, &s, NULL) != LZO_E_OK ||
		s != m_CurFileHeader[m_LoadIdx]->second.UnpackedSize)
		hr = E_FAIL;

#ifdef CACHE_WHOLE_BUNDLE
	if (!m_BundleCache)
#endif
	{
		free(m_PreLoadBuffer[m_LoadIdx]);
		m_PreLoadBuffer[m_LoadIdx] = 0;
	}

	// switch on writecombine on memory and flush the cache for the gpu
	// it's about 3 times faster to load in cached ram then do this than to load in wc ram. :)
	if (hr == S_OK)
	{
		XPhysicalProtect(UnpackedBuf, m_CurFileHeader[m_LoadIdx]->second.UnpackedSize, PAGE_READWRITE | PAGE_WRITECOMBINE);
		__asm {
			wbinvd
		}
	}

	return hr;
}

HRESULT CTextureBundle::LoadTexture(LPDIRECT3DDEVICE8 pDevice, const CStdString& Filename, D3DXIMAGE_INFO* pInfo, LPDIRECT3DTEXTURE8* ppTexture,
																		LPDIRECT3DPALETTE8* ppPalette)
{
	*ppTexture = NULL; *ppPalette = NULL;

	CAutoTexBuffer UnpackedBuf;
	HRESULT r = LoadFile(Filename, UnpackedBuf);
	if (r != S_OK)
		return r;

	D3DTexture* pTex = (D3DTexture*)(new char[sizeof(D3DTexture) + sizeof(DWORD)]);
	D3DPalette* pPal = 0;
	void* ResData = 0;

	WORD RealSize[2];

	enum XPR_FLAGS
	{
		XPRFLAG_PALETTE = 0x00000001,
		XPRFLAG_ANIM =    0x00000002,
	};

	BYTE* Next = UnpackedBuf;

	DWORD flags = *(DWORD*)Next;
	Next += sizeof(DWORD);
	if (flags & XPRFLAG_ANIM || (flags >> 16) > 1)
		goto PackedLoadError;

	if (flags & XPRFLAG_PALETTE)
	{
		pPal = new D3DPalette;
		memcpy(pPal, Next, sizeof(D3DPalette));
		Next += sizeof(D3DPalette);
	}

	memcpy(pTex, Next, sizeof(D3DTexture));
	Next += sizeof(D3DTexture);

	memcpy(RealSize, Next, 4);
	Next += 4;

	DWORD ResDataOffset = ((Next - UnpackedBuf) + 127) & ~127;
	ResData = UnpackedBuf + ResDataOffset;
	UnpackedBuf.Release();

	if ((pTex->Common & D3DCOMMON_TYPE_MASK) != D3DCOMMON_TYPE_TEXTURE)
		goto PackedLoadError;

	*ppTexture = (LPDIRECT3DTEXTURE8)pTex;
	(*ppTexture)->Register(ResData);
	*(DWORD*)(pTex + 1) = (DWORD)(BYTE*)UnpackedBuf;
	if (pPal)
	{
		*ppPalette = (LPDIRECT3DPALETTE8)pPal;
		(*ppPalette)->Register(ResData);
	}

	pInfo->Width = RealSize[0];
	pInfo->Height = RealSize[1];
	pInfo->Depth = 0;
	pInfo->MipLevels = 1;
	D3DSURFACE_DESC desc;
	(*ppTexture)->GetLevelDesc(0, &desc);
	pInfo->Format = desc.Format;

	return S_OK;

PackedLoadError:
	delete [] pTex;
	if (pPal) delete pPal;
	return E_FAIL;
}


int CTextureBundle::LoadAnim(LPDIRECT3DDEVICE8 pDevice, const CStdString& Filename, D3DXIMAGE_INFO* pInfo, LPDIRECT3DTEXTURE8** ppTextures,
														 LPDIRECT3DPALETTE8* ppPalette, int& nLoops, int** ppDelays)
{
	*ppTextures = NULL; *ppPalette = NULL; *ppDelays = NULL;

	CAutoTexBuffer UnpackedBuf;
	HRESULT r = LoadFile(Filename, UnpackedBuf);
	if (r != S_OK)
		return 0;

	struct AnimInfo_t {
		DWORD nLoops;
		WORD RealSize[2];
	} *pAnimInfo;

	D3DTexture** ppTex = 0;
	D3DPalette* pPal = 0;
	void* ResData = 0;

	BYTE* Next = UnpackedBuf;

	DWORD flags = *(DWORD*)Next;
	Next += sizeof(DWORD);
	if (!(flags & XPRFLAG_ANIM))
		goto PackedAnimError;

	pAnimInfo = (AnimInfo_t*)Next;
	Next += sizeof(AnimInfo_t);
	nLoops = pAnimInfo->nLoops;

	if (flags & XPRFLAG_PALETTE)
	{
		pPal = new D3DPalette;
		memcpy(pPal, Next, sizeof(D3DPalette));
		Next += sizeof(D3DPalette);
	}

	int nTextures = flags >> 16;
	ppTex = new D3DTexture*[nTextures];
	*ppDelays = new int[nTextures];
	for (int i = 0; i < nTextures; ++i)
	{
		ppTex[i] = (D3DTexture*)(new char[sizeof(D3DTexture) + sizeof(DWORD)]);
		memcpy(ppTex[i], Next, sizeof(D3DTexture));
		Next += sizeof(D3DTexture);

		(*ppDelays)[i] = *(int*)Next;
		Next += sizeof(int);
	}

	DWORD ResDataOffset = ((Next - UnpackedBuf) + 127) & ~127;
	ResData = UnpackedBuf + ResDataOffset;
	UnpackedBuf.Release();

	*ppTextures = new LPDIRECT3DTEXTURE8[nTextures];
	for (int i = 0; i < nTextures; ++i)
	{
		if ((ppTex[i]->Common & D3DCOMMON_TYPE_MASK) != D3DCOMMON_TYPE_TEXTURE)
			goto PackedAnimError;

		(*ppTextures)[i] = (LPDIRECT3DTEXTURE8)ppTex[i];
		(*ppTextures)[i]->Register(ResData);
		*(DWORD*)(ppTex[i] + 1) = 0;
	}
	*(DWORD*)(ppTex[0] + 1) = (DWORD)(BYTE*)UnpackedBuf;

	delete [] ppTex;
	ppTex = 0;

	if (pPal)
	{
		*ppPalette = (LPDIRECT3DPALETTE8)pPal;
		(*ppPalette)->Register(ResData);
	}

	pInfo->Width = pAnimInfo->RealSize[0];
	pInfo->Height = pAnimInfo->RealSize[1];
	pInfo->Depth = 0;
	pInfo->MipLevels = 1;
	pInfo->Format = D3DFMT_UNKNOWN;

	return nTextures;

PackedAnimError:
	if (ppTex)
	{
		for (int i = 0; i < nTextures; ++i)
			delete [] ppTex[i];
		delete [] ppTex;
	}
	if (pPal) delete pPal;
	if (*ppDelays) delete [] *ppDelays;
	return 0;
}
