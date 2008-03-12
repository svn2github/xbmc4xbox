#ifndef __EVENT_PACKET_H__
#define __EVENT_PACKET_H__

#include "include.h"

#ifdef _LINUX

namespace EVENTPACKET
{
  const int PACKET_SIZE   = 1024;
  const int HEADER_SIZE   = 32;
  const char HEADER_SIG[] = "XBMC";
  
  
  /************************************************************************
     The payload format for each packet type is decribed below each
     packet type.

     Legend:
            %s - null terminated ASCII string (strlen + '\0' bytes)
                 (empty string is represented as a single byte NULL '\0')
            %c - single byte
            %i - network byte ordered short unsigned integer (2 bytes)
            %d - network byte ordered long unsigned integer  (4 bytes)
            XX - binary data prefixed with %d size 
                 (can span multiple packets with <raw>)
           raw - raw binary data
   ************************************************************************/

  enum LogoType
  {
    LT_NONE = 0x00,
    LT_JPEG = 0x01,
    LT_PNG  = 0x02,
    LT_GIF  = 0x03
  };

  enum PacketType
  {
    PT_HELO          = 0x01,
    /************************************************************************/
    /* Payload format                                                       */
    /* %s -  device name (max 128 chars)                                    */
    /* %c -  logotype ( 0=>NOLOGO, 1=>JPEG , 2=>PNG , 3=>GIF )              */
    /* %d -  reserved ( 0 )                                                 */
    /* XX -  imagedata ( can span multiple packets )                        */
    /************************************************************************/

    PT_BYE           = 0x02,
    /************************************************************************/
    /* no payload                                                           */
    /************************************************************************/

    PT_BUTTON        = 0x03,
    /************************************************************************/
    /* Payload format                                                       */
    /* %s -  device map                                                     */
    /*       "KB", "MS", "RM1" or valid SDL joystick map name               */
    /* %i - button code                                                     */
    /* %i - flags 0x01 => use button name instead of code                   */
    /*            0x02 => btn down                                          */
    /*            0x04 => btn up                                            */
    /*            0x08 => use amount                                        */
    /*            0x10 => queue event                                       */
    /* %i - amount ( 0 => 65k maps to -1 => 1 )                             */
    /* %s - button name                                                     */
    /************************************************************************/

    PT_MOUSE         = 0x04,
    /************************************************************************/
    /* Payload format                                                       */
    /* %c - type ( 1=>absolute, 2=>relative )                               */
    /* %i - mousex                                                          */
    /* %i - mousey                                                          */
    /************************************************************************/

    PT_PING          = 0x05,
    /************************************************************************/
    /* no payload                                                           */
    /************************************************************************/

    PT_BROADCAST     = 0x06,
    /************************************************************************/
    /* Payload format: TODO                                                 */
    /************************************************************************/

    PT_NOTIFICATION  = 0x07,
    /************************************************************************/
    /* Payload format: TODO                                                 */
    /************************************************************************/

    PT_BLOB          = 0x08,
    /************************************************************************/
    /* Payload format                                                       */
    /* raw - raw binary data                                                */
    /************************************************************************/
    
    PT_DEBUG         = 0xFF,
    /************************************************************************/
    /* Payload format: TODO                                                 */
    /************************************************************************/

    PT_LAST // NO-OP
  };

  /************************************************************************/
  /*                                                                      */
  /* - Generic packet structure (maximum 1024 bytes per packet)           */
  /* - Header is 32 bytes long, so 992 bytes available for payload        */
  /* - large payloads can be split into multiple packets using H4 and H5  */
  /*   H5 should contain total no. of packets in such a case              */
  /* - H6 contains length of P1, which is limited to 992 bytes            */
  /* - if H5 is 0 or 1, then H4 will be ignored (single packet msg)       */
  /* - H7 must be set to zeros for now                                    */
  /*                                                                      */
  /*     -----------------------------                                    */
  /*     | -H1 Signature ("XBMC")    | - 4  x CHAR                4B      */
  /*     | -H2 Version (eg. 2.0)     | - 2  x UNSIGNED CHAR       2B      */
  /*     | -H3 PacketType            | - 1  x UNSIGNED SHORT      2B      */
  /*     | -H4 Sequence number       | - 1  x UNSIGNED LONG       4B      */
  /*     | -H5 No. of packets in msg | - 1  x UNSIGNED LONG       4B      */
  /*     | -H6 Payload size          | - 1  x UNSIGNED SHORT      2B      */
  /*     | -H7 Reserved              | - 14 x UNSIGNED CHAR      14B      */
  /*     |---------------------------|                                    */
  /*     | -P1 payload               | -                                  */
  /*     -----------------------------                                    */
  /************************************************************************/

  class CEventPacket
  {
  public:
    CEventPacket() 
    {
      m_bValid = false; 
      m_pPayload = NULL;
    }

    CEventPacket(int datasize, const void* data) 
    { 
      m_bValid = false; 
      m_pPayload = NULL;
      Parse(datasize, data);
    }

    virtual      ~CEventPacket() { if (m_pPayload) free(m_pPayload); }
    virtual bool Parse(int datasize, const void *data);
    bool         IsValid() { return m_bValid; }
    PacketType   Type() { return m_eType; }
    unsigned int Size() { return m_iTotalPackets; }
    unsigned int Sequence() { return m_iSeq; }
    void*        Payload() { return m_pPayload; }
    unsigned int PayloadSize() { return m_sPayloadSize; }

  protected:
    bool           m_bValid;
    unsigned int   m_iSeq;
    unsigned int   m_iTotalPackets;
    unsigned char  m_header[32];
    void*          m_pPayload;
    unsigned short m_sPayloadSize;
    unsigned char  m_cMajVer;
    unsigned char  m_cMinVer;
    PacketType     m_eType;
  };

}

#endif // _LINUX
#endif // __EVENT_PACKET_H__
