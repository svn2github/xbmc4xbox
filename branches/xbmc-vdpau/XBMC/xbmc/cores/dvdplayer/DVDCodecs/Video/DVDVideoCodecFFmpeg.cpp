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
#include "DVDVideoCodecFFmpeg.h"
#include "DVDDemuxers/DVDDemux.h"
#include "DVDStreamInfo.h"
#include "DVDClock.h"
#include "DVDCodecs/DVDCodecs.h"
#include "../../../../utils/Win32Exception.h"
#if defined(_LINUX) || defined(_WIN32PC)
#include "utils/CPUInfo.h"
#endif
#include "Settings.h"

#ifndef _LINUX
#define RINT(x) ((x) >= 0 ? ((int)((x) + 0.5)) : ((int)((x) - 0.5)))
#else
#include <math.h>
#define RINT lrint
#endif

#include "Surface.h"
using namespace Surface;
bool usingVDPAU;
#ifdef HAVE_LIBVDPAU
extern CDVDVideoCodecVDPAU* m_VDPAU;
#endif
#include "cores/VideoRenderers/RenderManager.h"

#define ARSIZE(x) (sizeof(x) / sizeof((x)[0]))

int my_get_buffer(struct AVCodecContext *c, AVFrame *pic){
    if (c->pix_fmt == PIX_FMT_NONE)
      return -1;
    int ret= ((CDVDVideoCodecFFmpeg*)c->opaque)->m_dllAvCodec.avcodec_default_get_buffer(c, pic);
    double *pts= (double*)malloc(sizeof(double));
    *pts= ((CDVDVideoCodecFFmpeg*)c->opaque)->m_pts;
    pic->opaque= pts;
    return ret;
}

void my_release_buffer(struct AVCodecContext *c, AVFrame *pic){
    if(pic)
    {
      free(pic->opaque);
      pic->opaque = NULL;
    }
    ((CDVDVideoCodecFFmpeg*)c->opaque)->m_dllAvCodec.avcodec_default_release_buffer(c, pic);
}

CDVDVideoCodecFFmpeg::CDVDVideoCodecFFmpeg() : CDVDVideoCodec()
{
  CLog::Log(LOGNOTICE,"Constructing CDVDVideoCodecFFmpeg");
  m_pCodecContext = NULL;
  m_pConvertFrame = NULL;
  m_pFrame = NULL;

  m_iPictureWidth = 0;
  m_iPictureHeight = 0;
#ifdef HAVE_LIBVDPAU
  usingVDPAU = false;
#endif
  m_iScreenWidth = 0;
  m_iScreenHeight = 0;
}

CDVDVideoCodecFFmpeg::~CDVDVideoCodecFFmpeg()
{
#ifdef HAVE_LIBVDPAU
  if (m_VDPAU) {
    delete m_VDPAU;
    m_VDPAU = NULL;
  }
#endif
  if (m_Surface) {
    CLog::Log(LOGNOTICE,"Deleting m_Surface in CDVDVideoCodecFFmpeg");
    delete m_Surface;
    m_Surface = NULL;
  }
  Dispose();
}

bool CDVDVideoCodecFFmpeg::Open(CDVDStreamInfo &hints, CDVDCodecOptions &options)
{
  CLog::Log(LOGNOTICE,"CDVDVideoCodecFFmpeg::Open");
  AVCodec* pCodec;
  int requestedMethod = g_guiSettings.GetInt("videoplayer.rendermethod");

  if (!m_dllAvUtil.Load() || !m_dllAvCodec.Load() || !m_dllSwScale.Load()) return false;
  
  m_dllSwScale.sws_rgb2rgb_init(SWS_CPU_CAPS_MMX2);

  m_pCodecContext = m_dllAvCodec.avcodec_alloc_context();
  // avcodec_get_context_defaults(m_pCodecContext);
#ifdef HAVE_LIBVDPAU
  if ((requestedMethod == 0) || (requestedMethod==3))
    pCodec = m_dllAvCodec.avcodec_find_vdpau_decoder(hints.codec);
  else
#endif
  pCodec = m_dllAvCodec.avcodec_find_decoder(hints.codec);
  if (!pCodec)
  {
    CLog::Log(LOGDEBUG,"CDVDVideoCodecFFmpeg::Open() Unable to find codec %d", hints.codec);
    return false;
  }
  CLog::Log(LOGNOTICE,"Using codec: %s",pCodec->long_name);

  if(pCodec->capabilities & CODEC_CAP_HWACCEL_VDPAU){
    if (!m_Surface) m_Surface = new CSurface(g_graphicsContext.getScreenSurface());
    m_Surface->MakePixmap(hints.width,hints.height);
    Pixmap px = m_Surface->GetXPixmap();
    m_VDPAU = new CDVDVideoCodecVDPAU(g_graphicsContext.getScreenSurface()->GetDisplay(), px);
    m_pCodecContext->get_format= CDVDVideoCodecVDPAU::VDPAUGetFormat;
    m_pCodecContext->get_buffer= CDVDVideoCodecVDPAU::VDPAUGetBuffer;
    m_pCodecContext->release_buffer= CDVDVideoCodecVDPAU::VDPAUReleaseBuffer;
    m_pCodecContext->draw_horiz_band = CDVDVideoCodecVDPAU::VDPAURenderFrame;
    usingVDPAU = true;
  }
  else {
    m_pCodecContext->opaque = (void*)this;
    m_pCodecContext->get_buffer = my_get_buffer;
    m_pCodecContext->release_buffer = my_release_buffer;
  }

  m_pCodecContext->slice_flags=SLICE_FLAG_CODED_ORDER|SLICE_FLAG_ALLOW_FIELD;
  m_pCodecContext->debug_mv = 0;
  m_pCodecContext->debug = 0;
  m_pCodecContext->workaround_bugs = FF_BUG_AUTODETECT;
  /* some decoders (eg. dv) do not know the pix_fmt until they decode the
   * first frame. setting to -1 avoid enabling DR1 for them.
   */
  m_pCodecContext->pix_fmt = (PixelFormat) - 1;  
  if (pCodec->id != CODEC_ID_H264 && pCodec->capabilities & CODEC_CAP_DR1)
    m_pCodecContext->flags |= CODEC_FLAG_EMU_EDGE;

  // Hack to correct wrong frame rates that seem to be generated by some
  // codecs
  if (m_pCodecContext->time_base.den > 1000 && m_pCodecContext->time_base.num == 1)
    m_pCodecContext->time_base.num = 1000;

  // if we don't do this, then some codecs seem to fail.
  m_pCodecContext->coded_height = hints.height;
  m_pCodecContext->coded_width = hints.width;

  if( hints.extradata && hints.extrasize > 0 )
  {
    m_pCodecContext->extradata_size = hints.extrasize;
    m_pCodecContext->extradata = (uint8_t*)m_dllAvUtil.av_mallocz(hints.extrasize + FF_INPUT_BUFFER_PADDING_SIZE);
    memcpy(m_pCodecContext->extradata, hints.extradata, hints.extrasize);
  }

  // set acceleration
  m_pCodecContext->dsp_mask = FF_MM_FORCE | FF_MM_MMX | FF_MM_MMXEXT | FF_MM_SSE;
  
  // This doesn't seem to help with the H.264 MT patch. "Basically, the code decodes 
  // up to 128 macroblocks in one thread while doing prediction+idct+deblock of the 
  // previously decoded 128 blocks in another thread." This could explain the lack of
  // much difference.
  //
  //m_pCodecContext->skip_loop_filter = AVDISCARD_BIDIR;

  // advanced setting override for skip loop filter (see avcodec.h for valid options)
  // TODO: allow per video setting?
  if (g_advancedSettings.m_iSkipLoopFilter != 0)
  {
    m_pCodecContext->skip_loop_filter = (AVDiscard)g_advancedSettings.m_iSkipLoopFilter;
  }

  // set any special options
  for(CDVDCodecOptions::iterator it = options.begin(); it != options.end(); it++)
  {
    m_dllAvCodec.av_set_string(m_pCodecContext, it->m_name.c_str(), it->m_value.c_str());
  }

  if (m_dllAvCodec.avcodec_open(m_pCodecContext, pCodec) < 0)
  {
    CLog::Log(LOGDEBUG,"CDVDVideoCodecFFmpeg::Open() Unable to open codec");
    return false;
  }
  m_pFrame = m_dllAvCodec.avcodec_alloc_frame();
  if (!m_pFrame) {
	  CLog::Log(LOGERROR,"CDVDVideoCodecFFmpeg::Open() Failed to allocate frames");
	  return false;
  }
  return true;
}

void CDVDVideoCodecFFmpeg::Dispose()
{
  if (m_pFrame) m_dllAvUtil.av_free(m_pFrame);
  m_pFrame = NULL;

  if (m_pConvertFrame)
  {
    delete[] m_pConvertFrame->data[0];
    if(m_pConvertFrame->opaque)
      free(m_pConvertFrame->opaque);
    m_dllAvUtil.av_free(m_pConvertFrame);
  }
  m_pConvertFrame = NULL;

  if (m_pCodecContext)
  {
    if (m_pCodecContext->codec) m_dllAvCodec.avcodec_close(m_pCodecContext);
    if (m_pCodecContext->extradata)
    {
      m_dllAvUtil.av_free(m_pCodecContext->extradata);
      m_pCodecContext->extradata = NULL;
      m_pCodecContext->extradata_size = 0;
    }
    m_dllAvUtil.av_free(m_pCodecContext);
    m_pCodecContext = NULL;
  }
  
  m_dllAvCodec.Unload();
  m_dllAvUtil.Unload();
}

void CDVDVideoCodecFFmpeg::SetDropState(bool bDrop)
{
  if( m_pCodecContext )
  {
    // i don't know exactly how high this should be set
    // couldn't find any good docs on it. think it varies
    // from codec to codec on what it does

    //  2 seem to be to high.. it causes video to be ruined on following images
    if( bDrop )
    {
      // TODO: 'hurry_up' has been deprecated in favor of the skip_* variables
      // Use those instead.

      m_pCodecContext->hurry_up = 1;
      //m_pCodecContext->skip_frame = AVDISCARD_NONREF;
      //m_pCodecContext->skip_idct = AVDISCARD_NONREF;
      //m_pCodecContext->skip_loop_filter = AVDISCARD_NONREF;
    }
    else
    {
      m_pCodecContext->hurry_up = 0;
      //m_pCodecContext->skip_frame = AVDISCARD_DEFAULT;
      //m_pCodecContext->skip_idct = AVDISCARD_DEFAULT;
      //m_pCodecContext->skip_loop_filter = AVDISCARD_DEFAULT;
    }
  }
}

int CDVDVideoCodecFFmpeg::Decode(BYTE* pData, int iSize, double pts)
{
  int iGotPicture = 0, len = 0;
  if (!m_pCodecContext) 
    return VC_ERROR;

  // store pts, it will be used to set
  // the pts of pictures decoded
  m_pts = pts;
#ifdef HAVE_LIBVDPAU
  if (usingVDPAU)
    m_pCodecContext->opaque = (void*)&(m_VDPAU->picAge);
#endif
  try
  {
    len = m_dllAvCodec.avcodec_decode_video(m_pCodecContext, m_pFrame, &iGotPicture, pData, iSize);
  }
  catch (win32_exception e)
  {
    CLog::Log(LOGERROR, "%s::avcodec_decode_video", __FUNCTION__);
    return VC_ERROR;
  }

  if (len < 0)
  {
    CLog::Log(LOGERROR, "%s - avcodec_decode_video returned failure", __FUNCTION__);
    return VC_ERROR;
  }

  if (len != iSize && !m_pCodecContext->hurry_up)
    CLog::Log(LOGWARNING, "%s - avcodec_decode_video didn't consume the full packet. size: %d, consumed: %d", __FUNCTION__, iSize, len);

  if (!iGotPicture)
    return VC_BUFFER;

  if (m_pCodecContext->pix_fmt != PIX_FMT_YUV420P
      && m_pCodecContext->pix_fmt != PIX_FMT_YUVJ420P
#ifdef HAVE_LIBVDPAU
      && !usingVDPAU
#endif
      )
  {
    CLog::Log(LOGERROR, "%s - unsupported format - attempting to convert pix_fmt %i", __FUNCTION__,m_pCodecContext->pix_fmt);
    if (!m_dllSwScale.IsLoaded())
    {
      if(!m_dllSwScale.Load())
        return VC_ERROR;

      m_dllSwScale.sws_rgb2rgb_init(SWS_CPU_CAPS_MMX2);    
    }

    if (!m_pConvertFrame)
    {
      // Allocate an AVFrame structure
      m_pConvertFrame =  m_dllAvCodec.avcodec_alloc_frame();

      // Determine required buffer size and allocate buffer
      int numBytes =  m_dllAvCodec.avpicture_get_size(PIX_FMT_YUV420P, m_pCodecContext->width, m_pCodecContext->height);
      BYTE* buffer = new BYTE[numBytes];

      // Assign appropriate parts of buffer to image planes in pFrameRGB
      m_dllAvCodec.avpicture_fill((AVPicture *)m_pConvertFrame, buffer, PIX_FMT_YUV420P, m_pCodecContext->width, m_pCodecContext->height);

      m_pConvertFrame->opaque= malloc(sizeof(double));
      *(double*)m_pConvertFrame->opaque = DVD_NOPTS_VALUE;
    }

    // convert the picture
    struct SwsContext *context = m_dllSwScale.sws_getContext(m_pCodecContext->width, m_pCodecContext->height, 
			m_pCodecContext->pix_fmt, m_pCodecContext->width, m_pCodecContext->height, 
			PIX_FMT_YUV420P, SWS_FAST_BILINEAR, NULL, NULL, NULL);

    uint8_t *src[] = { m_pFrame->data[0], m_pFrame->data[1], m_pFrame->data[2] };
    int     srcStride[] = { m_pFrame->linesize[0], m_pFrame->linesize[1], m_pFrame->linesize[2] };
    uint8_t *dst[] = { m_pConvertFrame->data[0], m_pConvertFrame->data[1], m_pConvertFrame->data[2] };
    int     dstStride[] = { m_pConvertFrame->linesize[0], m_pConvertFrame->linesize[1], m_pConvertFrame->linesize[2] };
    m_dllSwScale.sws_scale(context, src, srcStride, 0, m_pCodecContext->height, dst, dstStride);

    m_dllSwScale.sws_freeContext(context); 

    m_pConvertFrame->coded_picture_number = m_pFrame->coded_picture_number;
    m_pConvertFrame->interlaced_frame = m_pFrame->interlaced_frame;
    m_pConvertFrame->repeat_pict = m_pFrame->repeat_pict;
    m_pConvertFrame->top_field_first = m_pFrame->top_field_first;
    if(m_pConvertFrame->opaque)
    {
      if(m_pFrame->opaque)
        *(double*)m_pConvertFrame->opaque = *(double*)m_pFrame->opaque;
      else
        *(double*)m_pConvertFrame->opaque = DVD_NOPTS_VALUE;
    }
  }
  else
  {
    // no need to convert, just free any existing convert buffers
    if (m_pConvertFrame)
    {
      delete[] m_pConvertFrame->data[0];
      m_dllAvUtil.av_free(m_pConvertFrame);
      m_pConvertFrame = NULL;
    }
  }
#ifdef HAVE_LIBVDPAU
if (usingVDPAU)
  m_VDPAU->VDPAUPrePresent(m_pCodecContext,m_pFrame);
#endif
  return VC_PICTURE | VC_BUFFER;
}

void CDVDVideoCodecFFmpeg::Reset()
{
  try {

  m_dllAvCodec.avcodec_flush_buffers(m_pCodecContext);

  if (m_pConvertFrame)
  {
    delete[] m_pConvertFrame->data[0];
    m_dllAvUtil.av_free(m_pConvertFrame);
    m_pConvertFrame = NULL;
  }
  m_pts = DVD_NOPTS_VALUE;

  } catch (win32_exception e) {
    e.writelog(__FUNCTION__);
  }
}

bool CDVDVideoCodecFFmpeg::GetPicture(DVDVideoPicture* pDvdVideoPicture)
{
  GetVideoAspect(m_pCodecContext, pDvdVideoPicture->iDisplayWidth, pDvdVideoPicture->iDisplayHeight);
  pDvdVideoPicture->iWidth = m_pCodecContext->width;
  pDvdVideoPicture->iHeight = m_pCodecContext->height;
  pDvdVideoPicture->pts = DVD_NOPTS_VALUE;

  AVFrame *frame = m_pConvertFrame ? m_pConvertFrame : m_pFrame;
  if (!frame)
    return false;

  for (int i = 0; i < 4; i++) pDvdVideoPicture->data[i] = frame->data[i];
  for (int i = 0; i < 4; i++) pDvdVideoPicture->iLineSize[i] = frame->linesize[i];
  pDvdVideoPicture->iRepeatPicture = frame->repeat_pict;
  pDvdVideoPicture->iFlags = DVP_FLAG_ALLOCATED;
  pDvdVideoPicture->iFlags |= frame->interlaced_frame ? DVP_FLAG_INTERLACED : 0;
  pDvdVideoPicture->iFlags |= frame->top_field_first ? DVP_FLAG_TOP_FIELD_FIRST: 0;
  pDvdVideoPicture->iFlags |= frame->data[0] ? 0 : DVP_FLAG_DROPPED;
  if (m_pCodecContext->pix_fmt == PIX_FMT_YUVJ420P)
    pDvdVideoPicture->color_range = 1;
  return true;
}

/*
 * Calculate the height and width this video should be displayed in
 */
void CDVDVideoCodecFFmpeg::GetVideoAspect(AVCodecContext* pCodecContext, unsigned int& iWidth, unsigned int& iHeight)
{
  double aspect_ratio;

  /* XXX: use variable in the frame */
  if (pCodecContext->sample_aspect_ratio.num == 0) aspect_ratio = 0;
  else aspect_ratio = av_q2d(pCodecContext->sample_aspect_ratio) * pCodecContext->width / pCodecContext->height;

  if (aspect_ratio <= 0.0) aspect_ratio = (float)pCodecContext->width / (float)pCodecContext->height;

  /* XXX: we suppose the screen has a 1.0 pixel ratio */ // CDVDVideo will compensate it.
  iHeight = pCodecContext->height;
  iWidth = ((int)RINT(pCodecContext->height * aspect_ratio)) & -3;
  if (iWidth > (unsigned int)pCodecContext->width)
  {
    iWidth = pCodecContext->width;
    iHeight = ((int)RINT(pCodecContext->width / aspect_ratio)) & -3;
  }
}
