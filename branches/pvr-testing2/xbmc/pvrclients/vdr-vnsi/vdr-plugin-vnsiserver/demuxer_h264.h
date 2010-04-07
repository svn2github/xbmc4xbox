/*
 *      Copyright (C) 2010 Alwin Esch (Team XBMC)
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

#ifndef VNSIDEMUXER_H264_H
#define VNSIDEMUXER_H264_H

#include "demuxer.h"

class cBitstream;

// --- cParserH264 -------------------------------------------------

class cParserH264 : public cParser
{
private:
  typedef struct h264_private
  {
    struct
    {
      int frame_duration;
      int cbpsize;
    } sps[256];

    struct
    {
      int sps;
    } pps[256];

  } h264_private_t;

  typedef struct mpeg_rational_s {
    int num;
    int den;
  } mpeg_rational_t;

  enum
  {
    NAL_SLH     = 0x01, // Slice Header
    NAL_SEI     = 0x06, // Supplemental Enhancement Information
    NAL_SPS     = 0x07, // Sequence Parameter Set
    NAL_PPS     = 0x08, // Picture Parameter Set
    NAL_AUD     = 0x09, // Access Unit Delimiter
    NAL_END_SEQ = 0x0A  // End of Sequence
  };

  cTSDemuxer     *m_demuxer;
  uint8_t        *m_pictureBuffer;
  int             m_pictureBufferSize;
  int             m_pictureBufferPtr;
  uint32_t        m_StartCond;
  uint32_t        m_StartCode;
  int             m_StartCodeOffset;
  int             m_Height;
  int             m_Width;
  mpeg_rational_t m_PixelAspect;
  int64_t         m_PrevDTS;
  int             m_FrameDuration;
  sStreamPacket   m_StreamPacket;
  bool            m_FoundFrame;
  h264_private    m_streamData;
  int             m_vbvDelay;       /* -1 if CBR */
  int             m_vbvSize;        /* Video buffer size (in bytes) */
  bool            m_firstPUSIseen;

  bool Parse_H264(size_t len, uint32_t next_startcode, int sc_offset);
  bool Parse_PPS(uint8_t *buf, int len);
  bool Parse_SLH(uint8_t *buf, int len, int *pkttype);
  bool Parse_SPS(uint8_t *buf, int len);
  int nalUnescape(uint8_t *dst, const uint8_t *src, int len);

public:
  cParserH264(cTSDemuxer *demuxer, cLiveStreamer *streamer, int streamID);
  virtual ~cParserH264();

  virtual void Parse(unsigned char *data, int size, bool pusi);
};


#endif /* VNSIDEMUXER_H264_H */
