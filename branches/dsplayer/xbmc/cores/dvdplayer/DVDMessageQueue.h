#pragma once

/*
 *      Copyright (C) 2005-2008 Team XBMC
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

#include "DVDMessage.h"
#include <string>
#include <list>
#include "CriticalSection.h"

typedef struct stDVDMessageListItem
{
  CDVDMsg* pMsg;
  int priority;
}
DVDMessageListItem;

enum MsgQueueReturnCode
{
  MSGQ_OK               = 1,
  MSGQ_TIMEOUT          = 0,
  MSGQ_ABORT            = -1, // negative for legacy, not an error actually
  MSGQ_NOT_INITIALIZED  = -2,
  MSGQ_INVALID_MSG      = -3,
  MSGQ_OUT_OF_MEMORY    = -4
};

#define MSGQ_IS_ERROR(c)    (c < 0)

class CDVDMessageQueue
{
public:
  CDVDMessageQueue(const std::string &owner);
  virtual ~CDVDMessageQueue();

  void  Init();
  void  Flush(CDVDMsg::Message message = CDVDMsg::DEMUXER_PACKET);
  void  Abort();
  void  End();

  MsgQueueReturnCode Put(CDVDMsg* pMsg, int priority = 0);

  /**
   * msg,       message type from DVDMessage.h
   * timeout,   timeout in msec
   */
  MsgQueueReturnCode Get(CDVDMsg** pMsg, unsigned int iTimeoutInMilliSeconds, int priority = 0);


  int GetDataSize() const               { return m_iDataSize; }
  unsigned GetPacketCount(CDVDMsg::Message type);
  bool ReceivedAbortRequest()           { return m_bAbortRequest; }
  void WaitUntilEmpty();

  // non messagequeue related functions
  bool IsFull() const                   { return GetLevel() == 100; }
  int  GetLevel() const;

  void SetMaxDataSize(int iMaxDataSize) { m_iMaxDataSize = iMaxDataSize; }
  void SetMaxTimeSize(double sec)       { m_TimeSize  = 0.5 / std::max(1.0, sec); }
  int GetMaxDataSize() const            { return m_iMaxDataSize; }
  bool IsInited() const                 { return m_bInitialized; }

private:

  HANDLE m_hEvent;
  mutable CCriticalSection m_section;

  bool m_bAbortRequest;
  bool m_bInitialized;
  bool m_bCaching;

  int m_iDataSize;
  double m_TimeFront;
  double m_TimeBack;
  double m_TimeSize;

  int m_iMaxDataSize;
  bool m_bEmptied;
  std::string m_owner;

  typedef std::list<DVDMessageListItem> SList;
  SList m_list;
};

