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

#include "stdafx.h"
#include "SliceQueue.h"


// CSliceQueue
//////////////////////////////////////////////////////////////////////////////////////
CSliceQueue::CSliceQueue() :
  m_TotalBytes(0),
  m_pPartialSlice(NULL),
  m_RemainderSize(0)
{

}

CSliceQueue::~CSliceQueue()
{
  Clear();
}

void CSliceQueue::Push(audio_slice* pSlice) 
{
  if (pSlice)
  {
    m_Slices.push(pSlice);
    m_TotalBytes += pSlice->header.data_len;
  }
}

audio_slice* CSliceQueue::Pop()
{
  audio_slice* pSlice = NULL;
  if (!m_Slices.empty())
  {
    pSlice = m_Slices.front();
    m_Slices.pop();
    m_TotalBytes -= pSlice->header.data_len;
  }
  return pSlice;
}

audio_slice* CSliceQueue::GetSlice(size_t alignSize, size_t maxSize)
{
  if (GetTotalBytes() < alignSize) // Need to have enough data to give
    return NULL;
  
  size_t bytesUsed = 0;
  size_t remainder = 0;
  audio_slice* pNext = NULL;
  size_t sliceSize = std::min<size_t>(GetTotalBytes(),maxSize); // Set the size of the slice to be created, limit it to maxSize
  
  // If we have no remainder and the first whole slice can satisfy the requirements, pass it along as-is.
  // This reduces allocs and writes
  if (!m_RemainderSize) 
  {
    size_t headLen = m_Slices.front()->header.data_len;
    if (!(headLen % alignSize) && headLen <= maxSize)
    {
      pNext = Pop();
      return pNext;
    }
  }
  
  sliceSize -= sliceSize % alignSize; // Make sure we are properly aligned.
  audio_slice* pOutSlice = m_Allocator.NewSlice(sliceSize); // Create a new slice
  
  // See if we can fill the new slice out of our partial slice (if there is one)
  if (m_RemainderSize >= sliceSize)
  {
    memcpy(pOutSlice->get_data(), m_pPartialSlice->get_data() + m_pPartialSlice->header.data_len - m_RemainderSize , sliceSize);
    m_RemainderSize -= sliceSize;
  }
  else // Pull what we can from the partial slice and get the rest from complete slices
  {
    // Take what we can from the partial slice (if there is one)
    if (m_RemainderSize)
    {
      memcpy(pOutSlice->get_data(), m_pPartialSlice->get_data() + m_pPartialSlice->header.data_len - m_RemainderSize , m_RemainderSize);
      bytesUsed += m_RemainderSize;
      m_RemainderSize = 0;
    }
    
    // Pull slices from the fifo until we have enough data
    do // TODO: The efficiency of this loop can be improved (a lot I imagine)
    {
      pNext = Pop();
      size_t nextLen = pNext->header.data_len;
      if (bytesUsed + nextLen > sliceSize) // Check for a partial slice
        remainder = nextLen - (sliceSize - bytesUsed);
      memcpy(pOutSlice->get_data() + bytesUsed, pNext->get_data(), nextLen - remainder);
      bytesUsed += nextLen; // Increment output size (remainder will be captured separately)
      if (!remainder)
        m_Allocator.FreeSlice(pNext); // Free the copied slice
    } while (bytesUsed < sliceSize);
  }
  
  // Clean up the previous partial slice
  if (!m_RemainderSize)
  {
    delete m_pPartialSlice;
    m_pPartialSlice = NULL;
  }
  
  // Save off a new partial slice (if there is one)
  if (remainder)
  {
    m_pPartialSlice = pNext;
    m_RemainderSize = remainder;
  }
  
  return pOutSlice;  
}

size_t CSliceQueue::AddData(void* pBuf, size_t bufLen)
{
  if (pBuf && bufLen)
  {
  
    audio_slice* pSlice = m_Allocator.NewSlice(bufLen);
    if (pSlice)
    {
      memcpy(pSlice->get_data(), pBuf, bufLen);
      Push(pSlice);
      return bufLen;
    }
  }
  return 0;
}

size_t CSliceQueue::GetData(void* pBuf, size_t bufLen)
{
  if (GetTotalBytes() < bufLen || !pBuf || !bufLen)
    return NULL;

  size_t bytesUsed = 0;
  size_t remainder = 0;
  audio_slice* pNext = NULL;
  
  // See if we can fill the request out of our partial slice (if there is one)
  if (m_RemainderSize >= bufLen)
  {
    memcpy(pBuf, m_pPartialSlice->get_data() + m_pPartialSlice->header.data_len - m_RemainderSize , bufLen);
    m_RemainderSize -= bufLen;
  }
  else // Pull what we can from the partial slice and get the rest from complete slices
  {
    // Take what we can from the partial slice (if there is one)
    if (m_RemainderSize)
    {
      memcpy(pBuf, m_pPartialSlice->get_data() + m_pPartialSlice->header.data_len - m_RemainderSize , m_RemainderSize);
      bytesUsed += m_RemainderSize;
      m_RemainderSize = 0;
    }

    // Pull slices from the fifo until we have enough data
    do // TODO: The efficiency of this loop can be improved (a lot I imagine)
    {
      pNext = Pop();
      size_t nextLen = pNext->header.data_len;
      if (bytesUsed + nextLen > bufLen) // Check for a partial slice
        remainder = nextLen - (bufLen - bytesUsed);
      memcpy((BYTE*)pBuf + bytesUsed, pNext->get_data(), nextLen - remainder);
      bytesUsed += nextLen; // Increment output size (remainder will be captured separately)
      if (!remainder)
        m_Allocator.FreeSlice(pNext); // Free the copied slice
    } while (bytesUsed < bufLen);
  }

  // Clean up the previous partial slice
  if (!m_RemainderSize && m_pPartialSlice)
  {
    m_Allocator.FreeSlice(m_pPartialSlice);
    m_pPartialSlice = NULL;
  }

  // Save off a new partial slice (if there is one)
  if (remainder)
  {
    m_pPartialSlice = pNext;
    m_RemainderSize = remainder;
  }

  return bufLen;
}

void CSliceQueue::Clear()
{
  while (!m_Slices.empty())
     m_Allocator.FreeSlice(Pop());

  m_Allocator.FreeSlice(m_pPartialSlice);
  m_pPartialSlice = NULL;
  m_RemainderSize = 0;
}
