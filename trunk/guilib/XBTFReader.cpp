/*
 *      Copyright (C) 2005-2009 Team XBMC
 *      http://www.xbmc.org
 *
 *  This Program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your option)
 *  any later version.
 *
 *  This Program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with XBMC; see the file COPYING.  If not, write to
 *  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 *  http://www.gnu.org/copyleft/gpl.html
 *
 */

#include <sys/stat.h>
#include "XBTFReader.h"
#include "EndianSwap.h"
#include "CharsetConverter.h"

#define READ_STR(str, size, file) fread(str, size, 1, file)
#define READ_U32(i, file) fread(&i, 4, 1, file); i = Endian_SwapLE32(i);
#define READ_U64(i, file) fread(&i, 8, 1, file); i = Endian_SwapLE64(i);

CXBTFReader::CXBTFReader()
{
  m_file = NULL;
}

bool CXBTFReader::IsOpen() const
{
  return m_file != NULL;
}

bool CXBTFReader::Open(const CStdString& fileName)
{
  m_fileName = fileName;
  
#ifndef _LINUX
  CStdStringW strPathW;
  g_charsetConverter.utf8ToW(_P(m_fileName), strPathW, false);
  m_file = _wfopen(strPathW.c_str(), L"rb");
#else
  m_file = fopen(m_fileName.c_str(), "rb");
#endif  
  if (m_file == NULL)
  {
    return false;
  }
  
  char magic[4];
  READ_STR(magic, 4, m_file);
  
  if (strncmp(magic, XBTF_MAGIC, sizeof(magic)) != 0)
  {
    return false;
  }
  
  char version[1];
  READ_STR(version, 1, m_file);
  
  if (strncmp(version, XBTF_VERSION, sizeof(version)) != 0)
  {
    return false;
  }
  
  unsigned int nofFiles;
  READ_U32(nofFiles, m_file);
  for (unsigned int i = 0; i < nofFiles; i++)
  {
    CXBTFFile file;
    unsigned int u32;
    unsigned long long u64;
    
    READ_STR(file.GetPath(), 256, m_file);
    READ_U32(u32, m_file);
    file.SetFormat(u32);
    READ_U32(u32, m_file);
    file.SetLoop(u32);
    
    unsigned int nofFrames;
    READ_U32(nofFrames, m_file);
    
    for (unsigned int j = 0; j < nofFrames; j++)
    {
      CXBTFFrame frame;
      
      READ_U32(u32, m_file);
      frame.SetWidth(u32);
      READ_U32(u32, m_file);
      frame.SetHeight(u32);
      READ_U32(u32, m_file);
      frame.SetX(u32);
      READ_U32(u32, m_file);
      frame.SetY(u32);
      READ_U64(u64, m_file);
      frame.SetPackedSize(u64);
      READ_U64(u64, m_file);
      frame.SetUnpackedSize(u64);
      READ_U32(u32, m_file);
      frame.SetDuration(u32);
      READ_U64(u64, m_file);
      frame.SetOffset(u64);
      
      file.GetFrames().push_back(frame);
    }
    
    m_xbtf.GetFiles().push_back(file);
    
    std::pair<CStdString, unsigned int> key = std::make_pair(file.GetPath(), file.GetFormat());
    m_filesMap[key] = file;    
  } 
  
  // Sanity check
  fpos_t pos;
  fgetpos(m_file, &pos);
  if ((unsigned int) pos != m_xbtf.GetHeaderSize())
  {
    printf("Expected header size (%llu) != actual size (%llu)\n", m_xbtf.GetHeaderSize(), pos);
    return false;
  }  
  
  return true;
}

void CXBTFReader::Close()
{
  if (m_file)
  {
    fclose(m_file);
    m_file = NULL;
  }
  
  m_xbtf.GetFiles().clear();
  m_filesMap.clear();
}

time_t CXBTFReader::GetLastModificationTimestamp()
{
  if (!m_file)
  {
    return 0;
  }
  
  struct stat fileStat;
  if (fstat(fileno(m_file), &fileStat) == -1)
  {
    return 0;
  }
  
  return fileStat.st_mtime;
}

bool CXBTFReader::Exists(const CStdString& name, int formatMask)
{
  return Find(name, formatMask) != NULL;
}

CXBTFFile* CXBTFReader::Find(const CStdString& name, int formatMask)
{
  std::pair<CStdString, unsigned int> key = std::make_pair(name, formatMask);
  
  std::map<std::pair<CStdString, unsigned int>, CXBTFFile>::iterator iter = m_filesMap.find(key);
  if (iter == m_filesMap.end())
  {
    return NULL;
  }
  
  return &(iter->second);
}

bool CXBTFReader::Load(const CXBTFFrame& frame, unsigned char* buffer)
{
  if (!m_file)
  {
    return false;
  }
  
  if (fseek(m_file, frame.GetOffset(), SEEK_SET) == -1)
  {
    return false;
  }
  
  if (fread(buffer, 1, frame.GetPackedSize(), m_file) != frame.GetPackedSize())
  {
    return false;
  }
  
  return true;
}

std::vector<CXBTFFile>& CXBTFReader::GetFiles()
{
  return m_xbtf.GetFiles();
}
