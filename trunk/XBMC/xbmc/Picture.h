#pragma once
#include "DllImageLib.h"

class CPicture
{
public:
  CPicture(void);
  virtual ~CPicture(void);
  IDirect3DTexture8* Load(const CStdString& strFilename, int iMaxWidth = 128, int iMaxHeight = 128);

  bool CreateThumbnailFromMemory(const BYTE* pBuffer, int nBufSize, const CStdString& strExtension, const CStdString& strThumbFileName);
  bool CreateThumbnailFromSurface(BYTE* pBuffer, int width, int height, int stride, const CStdString &strThumbFileName);
  int ConvertFile(const CStdString& srcFile, const CStdString& destFile, float rotateDegrees, int width, int height, unsigned int quality);

  ImageInfo GetInfo() const { return m_info; };
  unsigned int GetWidth() const { return m_info.width; };
  unsigned int GetHeight() const { return m_info.height; };
  unsigned int GetOriginalWidth() const { return m_info.originalwidth; };
  unsigned int GetOriginalHeight() const { return m_info.originalheight; };
  long GetExifOrientation() const { return m_info.rotation; };

  void CreateFolderThumb(const CStdString *strThumbs, const CStdString &folderThumbnail);
  bool DoCreateThumbnail(const CStdString& strFileName, const CStdString& strThumbFileName, bool checkExistence = false);

protected:
  
private:
  struct VERTEX
  {
    D3DXVECTOR4 p;
    D3DCOLOR col;
    FLOAT tu, tv;
  };
  static const DWORD FVF_VERTEX = D3DFVF_XYZRHW | D3DFVF_DIFFUSE | D3DFVF_TEX1;

  DllImageLib m_dll;

  ImageInfo m_info;
};
