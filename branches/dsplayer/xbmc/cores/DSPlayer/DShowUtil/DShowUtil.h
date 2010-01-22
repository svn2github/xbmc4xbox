#pragma once

#include "streams.h"
#include <dshow.h>

#include <strsafe.h>
#include <assert.h>
#include "ObjBase.h"
//#include "HdmvClipInfo.h"
#include "H264Nalu.h"

#include "MediaTypeEx.h"
#include "moreuuids.h"
#include "vd.h"
//#include "text.h" Removed until i changed the catllist
#include <vector>
#include <list>

#ifndef ASSERT
#define ASSERT assert
#endif

#ifndef SAFE_RELEASE
#define SAFE_RELEASE(p)      { if(p) { (p)->Release(); (p)=NULL; } }
#endif

#ifndef CHECK_HR
#define CHECK_HR(hr) if (FAILED(hr)) { goto done; }
#endif

#include "DshowCommon.h"

#ifndef dsmax
#define dsmax(a,b)            (((a) > (b)) ? (a) : (b))
#endif

#ifndef dsmin
#define dsmin(a,b)            (((a) < (b)) ? (a) : (b))
#endif
#ifdef _DEBUG
  #define DNew new
#else
  #define DNew new
#endif

#define LCID_NOSUBTITLES      -1

typedef enum {CDROM_NotFound, CDROM_Audio, CDROM_VideoCD, CDROM_DVDVideo, CDROM_Unknown} cdrom_t;

static const GUID CLSID_NullRenderer =
  { 0xC1F400A4, 0x3F08, 0x11D3, { 0x9F, 0x0B, 0x00, 0x60, 0x08, 0x03, 0x9E, 0x37 } };

typedef std::list<GUID> GuidList;
typedef std::list<GUID>::iterator GuidListIter;

class  DShowUtil
{
public:
  static bool GuidVectItterCompare(GuidListIter it,const std::vector<GUID>::const_reference vect);
  static bool GuidItteratorIsNull(GuidListIter it);
  static bool GuidVectIsNull(const std::vector<GUID>::const_reference vect);
  static HRESULT IsPinConnected(IPin* pPin);
  static long MFTimeToMsec(const LONGLONG& time);
  static CStdString GetFilterPath(CStdString pClsid);
  static CStdStringW AnsiToUTF16(const CStdString strFrom);
  static int CountPins(IBaseFilter* pBF, int& nIn, int& nOut, int& nInC, int& nOutC);
  static bool IsSplitter(IBaseFilter* pBF, bool fCountConnectedOnly = false);
  static bool IsMultiplexer(IBaseFilter* pBF, bool fCountConnectedOnly = false);
  static bool IsStreamStart(IBaseFilter* pBF);
  static bool IsStreamEnd(IBaseFilter* pBF);
  static bool IsVideoRenderer(IBaseFilter* pBF);
  static bool IsAudioWaveRenderer(IBaseFilter* pBF);
  static std::vector<IMoniker*> GetAudioRenderersGuid();
  static HRESULT RemoveUnconnectedFilters(IGraphBuilder *pGraph);
  static IBaseFilter* GetUpStreamFilter(IBaseFilter* pBF, IPin* pInputPin = NULL);
  static IPin* GetUpStreamPin(IBaseFilter* pBF, IPin* pInputPin = NULL);
  static IPin* GetFirstPin(IBaseFilter* pBF, PIN_DIRECTION dir = PINDIR_INPUT);
  static IPin* GetFirstDisconnectedPin(IBaseFilter* pBF, PIN_DIRECTION dir);
  static void NukeDownstream(IBaseFilter* pBF, IFilterGraph* pFG);
  static void NukeDownstream(IPin* pPin, IFilterGraph* pFG);
  static IBaseFilter* FindFilter(LPCWSTR clsid, IFilterGraph* pFG);
  static IBaseFilter* FindFilter(const CLSID& clsid, IFilterGraph* pFG);
  static IPin* FindPin(IBaseFilter* pBF, PIN_DIRECTION direction, const AM_MEDIA_TYPE* pRequestedMT);
  static CStdStringW GetFilterName(IBaseFilter* pBF);
  static CStdStringW GetPinName(IPin* pPin);
  static IFilterGraph* GetGraphFromFilter(IBaseFilter* pBF);
  static IBaseFilter* GetFilterFromPin(IPin* pPin);
  static IPin* AppendFilter(IPin* pPin, CStdString DisplayName, IGraphBuilder* pGB);
  static IPin* InsertFilter(IPin* pPin, CStdString DisplayName, IGraphBuilder* pGB);
  static void ExtractMediaTypes(IPin* pPin, std::vector<GUID>& types);
  static void ExtractMediaTypes(IPin* pPin, std::list<CMediaType>& mts);
  static CLSID GetCLSID(IBaseFilter* pBF);
  static CLSID GetCLSID(IPin* pPin);
  static bool IsCLSIDRegistered(LPCTSTR clsid);
  static bool IsCLSIDRegistered(const CLSID& clsid);
  static void CStringToBin(CStdString str, std::vector<BYTE>& data);
  static CStdString BinToCString(BYTE* ptr, int len);
  typedef enum {CDROM_NotFound, CDROM_Audio, CDROM_VideoCD, CDROM_DVDVideo, CDROM_Unknown} cdrom_t;
  static CStdString GetDriveLabel(TCHAR drive);
  static DVD_HMSF_TIMECODE RT2HMSF(REFERENCE_TIME rt, double fps = 0);
  static REFERENCE_TIME HMSF2RT(DVD_HMSF_TIMECODE hmsf, double fps = 0);
  static void memsetd(void* dst, unsigned int c, int nbytes);
  static bool ExtractBIH(const AM_MEDIA_TYPE* pmt, BITMAPINFOHEADER* bih);
  static bool ExtractBIH(IMediaSample* pMS, BITMAPINFOHEADER* bih);
  static bool ExtractAvgTimePerFrame(const AM_MEDIA_TYPE* pmt, REFERENCE_TIME& rtAvgTimePerFrame);
  static bool ExtractInterlaced(const AM_MEDIA_TYPE* pmt);
  static bool ExtractDim(const AM_MEDIA_TYPE* pmt, int& w, int& h, int& arx, int& ary);
  static bool MakeMPEG2MediaType(CMediaType& mt, BYTE* seqhdr, DWORD len, int w, int h);
  static unsigned __int64 GetFileVersion(LPCTSTR fn);
  static bool CreateFilter(CStdStringW DisplayName, IBaseFilter** ppBF, CStdStringW& FriendlyName);
  static IBaseFilter* AppendFilter(IPin* pPin, IMoniker* pMoniker, IGraphBuilder* pGB);
  static CStdStringW GetFriendlyName(CStdStringW DisplayName);
  static HRESULT LoadExternalObject(LPCTSTR path, REFCLSID clsid, REFIID iid, void** ppv);
  static HRESULT LoadExternalFilter(LPCTSTR path, REFCLSID clsid, IBaseFilter** ppBF);
  static HRESULT LoadExternalPropertyPage(IPersist* pP, REFCLSID clsid, IPropertyPage** ppPP);
  static void UnloadExternalObjects();
  //Useless anyway
  //static CStdString MakeFullPath(LPCTSTR path);
  static CStdString GetMediaTypeName(const GUID& guid);
  static GUID GUIDFromCString(CStdString str);
  static HRESULT GUIDFromCString(CStdString str, GUID& guid);
  static CStdString CStringFromGUID(const GUID& guid);
  static CStdStringW UTF8To16(LPCSTR utf8);
  static CStdStringA UTF16To8(LPCWSTR utf16);
  static CStdString ISO6391ToLanguage(LPCSTR code);
  static CStdString ISO6392ToLanguage(LPCSTR code);
  static LCID    ISO6391ToLcid(LPCSTR code);
  static LCID    ISO6392ToLcid(LPCSTR code);
  static CStdString ISO6391To6392(LPCSTR code);
  static CStdString ISO6392To6391(LPCSTR code);
  static CStdString LanguageToISO6392(LPCTSTR lang);
  static int MakeAACInitData(BYTE* pData, int profile, int freq, int channels);
  //Need afx for the CFileStatus
  //static BOOL CFileGetStatus(LPCTSTR lpszFileName, CFileStatus& status);
  static bool DeleteRegKey(LPCTSTR pszKey, LPCTSTR pszSubkey);
  static bool SetRegKeyValue(LPCTSTR pszKey, LPCTSTR pszSubkey, LPCTSTR pszValueName, LPCTSTR pszValue);
  static bool SetRegKeyValue(LPCTSTR pszKey, LPCTSTR pszSubkey, LPCTSTR pszValue);
  static void RegisterSourceFilter(const CLSID& clsid, const GUID& subtype2, LPCTSTR chkbytes, LPCTSTR ext = NULL, ...);
  static void RegisterSourceFilter(const CLSID& clsid, const GUID& subtype2, const std::list<CStdString>& chkbytes, LPCTSTR ext = NULL, ...);
  static void UnRegisterSourceFilter(const GUID& subtype);
  static LPCTSTR GetDXVAMode(const GUID* guidDecoder);
  static void DumpBuffer(BYTE* pBuffer, int nSize);
  static CStdString ReftimeToString(const REFERENCE_TIME& rtVal);
REFERENCE_TIME StringToReftime(LPCTSTR strVal);
  static COLORREF YCrCbToRGB_Rec601(BYTE Y, BYTE Cr, BYTE Cb);
  static COLORREF YCrCbToRGB_Rec709(BYTE Y, BYTE Cr, BYTE Cb);
  static DWORD  YCrCbToRGB_Rec601(BYTE A, BYTE Y, BYTE Cr, BYTE Cb);
  static DWORD  YCrCbToRGB_Rec709(BYTE A, BYTE Y, BYTE Cr, BYTE Cb);
};

class DShowVideoInfo
{
public:
  static CStdString GetInfoOnMajorType(IGraphBuilder *pGraphBuilder, const GUID& clsidMajorType);
};

class CPinInfo : public PIN_INFO
{
public:
  CPinInfo() {pFilter = NULL;}
  ~CPinInfo() {if(pFilter) pFilter->Release();}
};

class CFilterInfo : public FILTER_INFO
{
public:
  CFilterInfo() {pGraph = NULL;}
  ~CFilterInfo() {if(pGraph) pGraph->Release();}
};

#define BeginEnumFilters(pFilterGraph, pEnumFilters, pBaseFilter) \
  {IEnumFilters* pEnumFilters; \
  if(pFilterGraph && SUCCEEDED(pFilterGraph->EnumFilters(&pEnumFilters))) \
  { \
    for(IBaseFilter* pBaseFilter; S_OK == pEnumFilters->Next(1, &pBaseFilter, 0); pBaseFilter = NULL) \
    { \

#define EndEnumFilters }}}

#define BeginEnumCachedFilters(pGraphConfig, pEnumFilters, pBaseFilter) \
  {IEnumFilters* pEnumFilters; \
  if(pGraphConfig && SUCCEEDED(pGraphConfig->EnumCacheFilter(&pEnumFilters))) \
  { \
    for(IBaseFilter* pBaseFilter; S_OK == pEnumFilters->Next(1, &pBaseFilter, 0); pBaseFilter = NULL) \
    { \

#define EndEnumCachedFilters }}}
//error in enumpins
#define BeginEnumPins(pBaseFilter, pEnumPins, pPin) \
  {IEnumPins* pEnumPins; \
  if(pBaseFilter && SUCCEEDED(pBaseFilter->EnumPins(&pEnumPins))) \
  { \
    for(IPin* pPin; S_OK == pEnumPins->Next(1, &pPin, 0); pPin = NULL) \
    { \

#define EndEnumPins }}}

#define BeginEnumMediaTypes(pPin, pEnumMediaTypes, pMediaType) \
  {IEnumMediaTypes* pEnumMediaTypes; \
  if(pPin && SUCCEEDED(pPin->EnumMediaTypes(&pEnumMediaTypes))) \
  { \
    AM_MEDIA_TYPE* pMediaType = NULL; \
    for(; S_OK == pEnumMediaTypes->Next(1, &pMediaType, NULL); DeleteMediaType(pMediaType), pMediaType = NULL) \
    { \

#define EndEnumMediaTypes(pMediaType) } if(pMediaType) DeleteMediaType(pMediaType); }}

#define BeginEnumSysDev(clsid, pMoniker) \
  {ICreateDevEnum* pDevEnum4$##clsid; \
  CoCreateInstance(CLSID_SystemDeviceEnum,NULL,CLSCTX_ALL,__uuidof(pDevEnum4$##clsid),(void**) &pDevEnum4$##clsid); \
  IEnumMoniker* pClassEnum4$##clsid; \
  if(SUCCEEDED(pDevEnum4$##clsid->CreateClassEnumerator(clsid, &pClassEnum4$##clsid, 0)) \
  && pClassEnum4$##clsid) \
  { \
    for(IMoniker* pMoniker; pClassEnum4$##clsid->Next(1, &pMoniker, 0) == S_OK; pMoniker = NULL) \
    { \

#define EndEnumSysDev }}}

#define QI(i) (riid == __uuidof(i)) ? GetInterface((i*)this, ppv) :
#define QI2(i) (riid == IID_##i) ? GetInterface((i*)this, ppv) :

template <typename T> __inline void INITDDSTRUCT(T& dd)
{
    ZeroMemory(&dd, sizeof(dd));
    dd.dwSize = sizeof(dd);
}

#define countof(array) (sizeof(array)/sizeof(array[0]))

template <class T>
static CUnknown* WINAPI CreateInstance(LPUNKNOWN lpunk, HRESULT* phr)
{
  *phr = S_OK;
    CUnknown* punk = DNew T(lpunk, phr);
    if(punk == NULL) *phr = E_OUTOFMEMORY;
  return punk;
}