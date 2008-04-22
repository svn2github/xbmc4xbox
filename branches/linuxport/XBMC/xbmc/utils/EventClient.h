#ifndef __EVENT_CLIENT_H__
#define __EVENT_CLIENT_H__

#include "include.h"
#include "Thread.h"
#include "CriticalSection.h"
#include "Socket.h"
#include "EventPacket.h"
#include "GUISettings.h"
#include <map>
#include <queue>

namespace EVENTCLIENT
{

  class CEventButtonState
  {
  public:
    CEventButtonState()
    {
      m_iKeyCode   = 0;
      m_mapName    = "";
      m_buttonName = "";
      m_fAmount    = 0.0f;
      m_bRepeat    = false;
      m_bActive    = false;
      m_bAxis      = false;
      m_iControllerNumber = 0;
    }

    CEventButtonState(unsigned short iKeyCode,
                      std::string mapName,
                      std::string buttonName,
                      float fAmount,
                      bool bRepeat
      )
    {
      m_iKeyCode   = iKeyCode;
      m_buttonName = buttonName;
      m_mapName    = mapName;
      m_fAmount    = fAmount;
      m_bRepeat    = bRepeat;
      m_bActive    = true;
      m_bAxis      = false;
      m_iControllerNumber = 0;
      Load();
    }

    void Reset()     { m_bActive = false; }
    void SetActive() { m_bActive = true; }
    bool Active()    { return m_bActive; }
    bool Repeat()    { return m_bRepeat; }
    int  ControllerNumber()  { return m_iControllerNumber; }
    bool Axis()              { return m_bAxis; }
    unsigned short KeyCode()   { return m_iKeyCode; }
    float Amount()   { return m_fAmount; }
    void Load();
    const std::string& JoystickName() const { return m_joystickName; }

    // data
    unsigned short    m_iKeyCode;
    unsigned short    m_iControllerNumber;
    std::string       m_buttonName;
    std::string       m_mapName;
    std::string       m_joystickName;
    float             m_fAmount;
    bool              m_bRepeat;
    bool              m_bActive;
    bool              m_bAxis;
  };


  /**********************************************************************/
  /* UDP EventClient Class                                              */
  /**********************************************************************/
  // - clients timeout if they don't receive at least 1 ping in 1 minute
  // - sequence packets timeout after 5 seconds
  class CEventClient
  {
  public:
    CEventClient()
    {
      Initialize();
    }

    CEventClient(SOCKETS::CAddress& addr)
    {
      m_remoteAddr = addr;
      Initialize();
    }

    void Initialize()
    {
      m_bGreeted = false;
      m_iNextRepeat = 0;
      m_iMouseX = 0;
      m_iMouseY = 0;
      m_iCurrentSeqLen = 0;
      m_lastPing = 0;
      m_lastSeq = 0;
      m_iRemotePort = 0;
      m_bMouseMoved = false;
      RefreshSettings();
    }

    std::string Name()
    {
      return m_deviceName;
    }

    void RefreshSettings()
    {
      m_iRepeatDelay = g_guiSettings.GetInt("remoteevents.initialdelay");
      m_iRepeatSpeed = g_guiSettings.GetInt("remoteevents.continuousdelay");
    }

    SOCKETS::CAddress& Address()
    {
      return m_remoteAddr;
    }

    virtual ~CEventClient()
    {
      FreePacketQueues();
    }

    // add packet to queue
    bool AddPacket(EVENTPACKET::CEventPacket *packet);

    // return true if client received ping with the last 1 minute
    bool Alive();

    // process the packet queue
    bool ProcessQueue();

    // execute the queued up events (packets)
    void ExecuteEvents();

    // deallocate all packets in the queues
    void FreePacketQueues();

    // return event states
    unsigned short GetButtonCode(std::string& strMapName);

    // update mouse position
    bool GetMousePos(float& x, float& y);

  protected:
    bool ProcessPacket(EVENTPACKET::CEventPacket *packet);

    // packet handlers
    virtual bool OnPacketHELO(EVENTPACKET::CEventPacket *packet);
    virtual bool OnPacketBYE(EVENTPACKET::CEventPacket *packet);
    virtual bool OnPacketBUTTON(EVENTPACKET::CEventPacket *packet);
    virtual bool OnPacketMOUSE(EVENTPACKET::CEventPacket *packet);
    virtual bool OnPacketNOTIFICATION(EVENTPACKET::CEventPacket *packet);
    virtual bool OnPacketLOG(EVENTPACKET::CEventPacket *packet);
    bool CheckButtonRepeat();

    // returns true if the client has received the HELO packet
    bool Greeted() { return m_bGreeted; }

    // reset the timeout counter
    void ResetTimeout()
    {
      m_lastPing = time(NULL);
    }

    // helper functions

    // Parses a null terminated string from payload.
    // After parsing successfully:
    //   1. payload is incremented to end of string
    //   2. psize is decremented by length of string
    //   3. parsedVal contains the parsed string
    //   4. true is returned
    bool ParseString(unsigned char* &payload, int &psize, std::string& parsedVal);

    // Parses a single byte (same behavior as ParseString)
    bool ParseByte(unsigned char* &payload, int &psize, unsigned char& parsedVal);

    // Parse a single 32-bit integer (converts from network order to host order)
    bool ParseUInt32(unsigned char* &payload, int &psize, unsigned int& parsedVal);

    // Parse a single 16-bit integer (converts from network order to host order)
    bool ParseUInt16(unsigned char* &payload, int &psize, unsigned short& parsedVal);

    std::string       m_deviceName;
    int               m_iCurrentSeqLen;
    time_t            m_lastPing;
    time_t            m_lastSeq;
    int               m_iRemotePort;
    bool              m_bGreeted;
    unsigned int      m_iRepeatDelay;
    unsigned int      m_iRepeatSpeed;
    unsigned int      m_iNextRepeat;
    unsigned int      m_iMouseX;
    unsigned int      m_iMouseY;
    bool              m_bMouseMoved;

    SOCKETS::CAddress m_remoteAddr;

    EVENTPACKET::LogoType m_eLogoType;
    CCriticalSection  m_critSection;

    std::map <unsigned int, EVENTPACKET::CEventPacket*>  m_seqPackets;
    std::queue <EVENTPACKET::CEventPacket*> m_readyPackets;

    // button and mouse state
    std::queue<CEventButtonState*>  m_buttonQueue;
    CEventButtonState m_currentButton;
  };

} // EVENTCLIENT

#endif // __EVENT_CLIENT_H__
