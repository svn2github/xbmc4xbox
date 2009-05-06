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

#pragma once
#include "deque"

typedef struct htsmsg htsmsg_t;

namespace HTSP
{

template<typename T>
class const_circular_iter 
  : public std::iterator<std::bidirectional_iterator_tag, typename T::value_type>
{
  protected:
    typename T::iterator   begin;
    typename T::iterator   end;
    typename T::iterator   iter;
  public:
    const_circular_iter(T& t) : iter(t.begin()), begin(t.begin()), end(t.end()) {};
    const_circular_iter(T& t, typename T::iterator c) :begin(t.begin()), end(t.end()), iter(c) {};
    const_circular_iter(typename T::iterator b, typename T::iterator e) : begin(b), end(e), iter(b) {};
    const_circular_iter<T>& operator++()
    {
      if(begin == end)
        return(*this);
      ++iter;
      if (iter == end)
        iter = begin;
      return(*this);
    }

    const_circular_iter<T>& operator--()
    {
      if(begin == end)
        return(*this);
      if (iter == begin)
        iter = end;
      iter--;
      return(*this);
    }

    const typename T::value_type& operator*() const { return (*iter); }
    const typename T::value_type* operator->() const { return &(*iter); }
    bool operator==(const const_circular_iter<T>&            rhs) const { return (iter == rhs.iter); }
    bool operator==(const typename T::iterator&              rhs) const { return (iter == rhs); }
    bool operator!=(const const_circular_iter<T>&            rhs) const { return ! operator==(rhs); }
    bool operator!=(const typename T::iterator&              rhs) const { return ! operator==(rhs); }
};


struct STag
{
  int              id;
  std::string      name;
  std::string      icon;
  std::vector<int> channels;

  STag() { Clear(); }
  void Clear()
  {
    id    = 0;
    name.empty();
    icon.empty();
    channels.clear();
  }
};

struct SChannel
{
  int              id;
  std::string      name;
  std::string      icon;
  int              event;
  std::vector<int> tags;

  SChannel() { Clear(); }
  void Clear()
  {
    id    = 0;
    event = 0;
    name.empty();
    icon.empty();
    tags.clear();
  }
};

struct SEvent
{
  int         id;
  int         next;

  int         start;
  int         stop;
  std::string title;
  std::string descs;

  SEvent() { Clear(); }
  void Clear()
  {
    id    = 0;
    next  = 0;
    start = 0;
    stop  = 0;
    title.empty();
    descs.empty();
  }
};

typedef std::map<int, SChannel> SChannels;
typedef std::map<int, STag>     STags;
typedef std::map<int, SEvent>   SEvents;


class CHTSPSession
{
public:
  CHTSPSession();
  ~CHTSPSession();

  bool      Connect(const std::string& hostname, int port);
  void      Close();
  void      Abort();
  bool      Auth(const std::string& username, const std::string& password);

  htsmsg_t* ReadMessage();
  bool      SendMessage(htsmsg_t* m);

  htsmsg_t* ReadResult (htsmsg_t* m, bool sequence = true);
  bool      ReadSuccess(htsmsg_t* m, bool sequence = true, std::string action = "");

  bool      SendSubscribe  (int subscription, int channel);
  bool      SendUnsubscribe(int subscription);
  bool      SendEnableAsync();
  bool      GetEvent(SEvent& event, uint32_t id);

  int       GetProtocol() { return m_protocol; }
  unsigned  AddSequence() { return ++m_seq; }

  static bool ParseEvent        (htsmsg_t* msg, uint32_t id, SEvent &event);
  static void ParseChannelUpdate(htsmsg_t* msg, SChannels &channels);
  static void ParseChannelRemove(htsmsg_t* msg, SChannels &channels);
  static void ParseTagUpdate    (htsmsg_t* msg, STags &tags);
  static void ParseTagRemove    (htsmsg_t* msg, STags &tags);
  static bool ParseItem         (const SChannel& channel, int tag, const SEvent& event, CFileItem& item);

private:
  SOCKET      m_fd;
  unsigned    m_seq;
  void*       m_challenge;
  int         m_challenge_len;
  int         m_protocol;

  std::deque<htsmsg_t*> m_queue;
  const unsigned int    m_queue_size;
};

}
