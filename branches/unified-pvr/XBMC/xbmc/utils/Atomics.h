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

#ifndef __ATOMICS_H__
#define __ATOMICS_H__

// 32-bit atomic compare-and-swap
// Returns previous value of *pAddr
#ifdef __ppc__ // PowerPC
static inline long cas(volatile long *pAddr, long expectedVal, long swapVal)
{
  unsigned int prev;
  
  __asm__ __volatile__ (
                        "  loop:   lwarx   %0,0,%2  \n" /* Load the current value of *pAddr,  (%2) into prev (%0) and lock pAddr,  */
                        "          cmpw    0,%0,%3  \n" /* Verify that the current value (%2) == old value (%3) */
                        "          bne     exit     \n" /* Bail if the two values are not equal [not as expected] */
                        "          stwcx.  %4,0,%2  \n" /* Attempt to store swapVal (%4) value into *pAddr (%2) [p must still be reserved] */
                        "          bne-    loop     \n" /* Loop if p was no longer reserved */
                        "          sync             \n" /* Reconcile multiple processors [if present] */
                        "  exit:                    \n"
                        : "=&r" (prev), "=m" (*pAddr)   /* Outputs [prev, *pAddr] */
                        : "r" (pAddr), "r" (expectedVal), "r" (swapVal), "m" (*pAddr) /* Inputs [pAddr, expectedVal, swapVal, *pAddr] */
                        : "cc", "memory");             /* Clobbers */
  
  return prev;
}

#elif defined(WIN32)

static inline long cas(volatile long* pAddr,long expectedVal, long swapVal)
{
  long prev;
  
  __asm
  {
    // Load parameters
    mov eax, expectedVal ;
    mov ebx, pAddr ;
    mov ecx, swapVal ;
    
    // Do Swap
    lock cmpxchg [ebx], ecx ;
    
    // Store the return value
    mov prev, eax;
  }
  
  return prev;
}

#else // Linux / OSX (GCC)

static inline long cas(volatile long* pAddr,long expectedVal, long swapVal)
{
  long prev;
  
  __asm__ __volatile__ (
                        "lock cmpxchg %1, %2"
                        : "=a" (prev)
                        : "r" (swapVal), "m" (*pAddr), "0" (expectedVal)
                        : "memory" );
  return prev;
  
}

#endif

class CAtomicLock
{
public:
  CAtomicLock(long& lock) : m_Lock(lock)
  {
    while (cas(&m_Lock, 0, 1) != 0); // Lock
  }
  ~CAtomicLock()
  {
    m_Lock = 0; // Unlock
  }
private:
  long& m_Lock;
};

#endif // __ATOMICS_H__
