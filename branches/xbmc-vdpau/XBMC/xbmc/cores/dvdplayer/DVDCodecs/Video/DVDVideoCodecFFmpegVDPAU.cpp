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
#ifdef HAVE_LIBVDPAU
#include "DVDVideoCodecFFmpegVDPAU.h"
#include "Surface.h"
using namespace Surface;
extern bool usingVDPAU;
#include "vdpau.h"
#include "TextureManager.h"                         //DAVID-CHECKNEEDED
#include "cores/VideoRenderers/RenderManager.h"
#include "DVDVideoCodecFFmpeg.h"
#include "Settings.h"

#define ARSIZE(x) (sizeof(x) / sizeof((x)[0]))

Desc decoder_profiles[] = {
{"MPEG1",        VDP_DECODER_PROFILE_MPEG1},
{"MPEG2_SIMPLE", VDP_DECODER_PROFILE_MPEG2_SIMPLE},
{"MPEG2_MAIN",   VDP_DECODER_PROFILE_MPEG2_MAIN},
{"H264_BASELINE",VDP_DECODER_PROFILE_H264_BASELINE},
{"H264_MAIN",    VDP_DECODER_PROFILE_H264_MAIN},
{"H264_HIGH",    VDP_DECODER_PROFILE_H264_HIGH},
{"VC1_SIMPLE",   VDP_DECODER_PROFILE_VC1_SIMPLE},
{"VC1_MAIN",     VDP_DECODER_PROFILE_VC1_MAIN},
{"VC1_ADVANCED", VDP_DECODER_PROFILE_VC1_ADVANCED},
};
const size_t decoder_profile_count = sizeof(decoder_profiles)/sizeof(Desc);

CDVDVideoCodecVDPAU::CDVDVideoCodecVDPAU()
{
  // Point the singleton to myself so we can use it to access our
  // instance variables from our static callbacks
  surfaceNum = 0;
  picAge.b_age = picAge.ip_age[0] = picAge.ip_age[1] = 256*256*256*64;
  vdpauConfigured = false;
  m_Surface = new CSurface(g_graphicsContext.getScreenSurface());
  m_Display = g_graphicsContext.getScreenSurface()->GetDisplay();
  InitVDPAUProcs();
  recover = false;
  outputSurface = 0;
/*  noiseReduction = g_stSettings.m_currentVideoSettings.m_NoiseReduction;
  sharpness = g_stSettings.m_currentVideoSettings.m_Sharpness;
  inverseTelecine = g_stSettings.m_currentVideoSettings.m_InverseTelecine;
*/  
  tmpBrightness = 0;
  tmpContrast = 0;
  InitCSCMatrix();
  lastSwapTime = frameLagTime = frameLagTimeRunning = previousTime = frameCounter = 0;
  frameLagAverage = 0;
  interlaced = false;
  m_avctx = NULL;
  videoSurfaces = NULL;
}

CDVDVideoCodecVDPAU::~CDVDVideoCodecVDPAU()
{
  FiniVDPAUOutput();
  FiniVDPAUProcs();
  usingVDPAU = false;
  if (m_Surface) 
  {
    CLog::Log(LOGNOTICE,"Deleting m_Surface in CDVDVideoCodecVDPAU");
    delete m_Surface;
    m_Surface = NULL;
  }
}

void CDVDVideoCodecVDPAU::CheckRecover()
{
  if (recover)
  {
    recover = false;
    XLockDisplay( m_Display );
    CLog::Log(LOGNOTICE,"Attempting recovery");
    FiniVDPAUOutput();
    FiniVDPAUProcs();
    InitVDPAUProcs();
    ConfigVDPAU(m_avctx);
    XUnlockDisplay( m_Display );
  }
}

bool CDVDVideoCodecVDPAU::IsVDPAUFormat(uint32_t format)
{
  if ((format >= PIX_FMT_VDPAU_H264) && (format <= PIX_FMT_VDPAU_VC1)) return true;
  else return false;
}

void CDVDVideoCodecVDPAU::CheckFeatures()
{
  if (tmpBrightness != g_stSettings.m_currentVideoSettings.m_Brightness)
  {
    SetColor();
    tmpBrightness = g_stSettings.m_currentVideoSettings.m_Brightness;
  }
  if (tmpContrast != g_stSettings.m_currentVideoSettings.m_Contrast)
  {
    SetColor();
    tmpContrast = g_stSettings.m_currentVideoSettings.m_Contrast;
  }
  if (tmpInverseTelecine != g_stSettings.m_currentVideoSettings.m_InverseTelecine)
  {
    tmpInverseTelecine = g_stSettings.m_currentVideoSettings.m_InverseTelecine;
    SetTelecine();
  }
  if (tmpNoiseReduction != g_stSettings.m_currentVideoSettings.m_NoiseReduction)
  {
    tmpNoiseReduction = g_stSettings.m_currentVideoSettings.m_NoiseReduction;
    SetNoiseReduction();
  }
  if (tmpSharpness != g_stSettings.m_currentVideoSettings.m_Sharpness)
  {
    tmpSharpness = g_stSettings.m_currentVideoSettings.m_Sharpness;
    SetSharpness();
  }
  if (tmpDeint != g_stSettings.m_currentVideoSettings.m_InterlaceMethod)
  {
    tmpDeint = g_stSettings.m_currentVideoSettings.m_InterlaceMethod;
    SetDeinterlacing();
  }
}

void CDVDVideoCodecVDPAU::SetColor()
{
  VdpStatus vdp_st;
  if (tmpBrightness != g_stSettings.m_currentVideoSettings.m_Brightness)
    m_Procamp.brightness = (float)((g_stSettings.m_currentVideoSettings.m_Brightness)-50) / 100;
  if (tmpContrast != g_stSettings.m_currentVideoSettings.m_Contrast)
    m_Procamp.contrast = (float)((g_stSettings.m_currentVideoSettings.m_Contrast)+50) / 100;
  vdp_st = vdp_generate_csc_matrix(&m_Procamp, 
                                   VDP_COLOR_STANDARD_ITUR_BT_709,
                                   &m_CSCMatrix);
  VdpVideoMixerAttribute attributes[] = { VDP_VIDEO_MIXER_ATTRIBUTE_CSC_MATRIX };
  void const * pm_CSCMatix[] = { &m_CSCMatrix };
  vdp_st = vdp_video_mixer_set_attribute_values(videoMixer, 1, attributes, pm_CSCMatix);
  CHECK_ST
}

void CDVDVideoCodecVDPAU::SetTelecine()
{
  VdpBool enabled[1];
  VdpVideoMixerFeature feature = VDP_VIDEO_MIXER_FEATURE_INVERSE_TELECINE;
  VdpStatus vdp_st;

  if (interlaced && g_stSettings.m_currentVideoSettings.m_InverseTelecine)
    enabled[0] = true;
  else
    enabled[0] = false;
  vdp_st = vdp_video_mixer_set_feature_enables(videoMixer, 1, &feature, enabled);
  CHECK_ST
}

void CDVDVideoCodecVDPAU::SetNoiseReduction()
{
  VdpVideoMixerFeature feature[] = { VDP_VIDEO_MIXER_FEATURE_NOISE_REDUCTION };
  VdpVideoMixerAttribute attributes[] = { VDP_VIDEO_MIXER_ATTRIBUTE_NOISE_REDUCTION_LEVEL };
  VdpStatus vdp_st;

  if (!g_stSettings.m_currentVideoSettings.m_NoiseReduction) {
    VdpBool enabled[]= {0};
    vdp_st = vdp_video_mixer_set_feature_enables(videoMixer, 1, feature, enabled);
    CHECK_ST
    return;
  }
  VdpBool enabled[]={1};
  vdp_st = vdp_video_mixer_set_feature_enables(videoMixer, 1, feature, enabled);
  CHECK_ST
  void* nr[] = { &g_stSettings.m_currentVideoSettings.m_NoiseReduction };
  CLog::Log(LOGNOTICE,"Setting Noise Reduction to %f",g_stSettings.m_currentVideoSettings.m_NoiseReduction);
  vdp_st = vdp_video_mixer_set_attribute_values(videoMixer, 1, attributes, nr);
  CHECK_ST
}

void CDVDVideoCodecVDPAU::SetSharpness()
{
  VdpVideoMixerFeature feature[] = { VDP_VIDEO_MIXER_FEATURE_SHARPNESS };
  VdpVideoMixerAttribute attributes[] = { VDP_VIDEO_MIXER_ATTRIBUTE_SHARPNESS_LEVEL };
  VdpStatus vdp_st;

  if (!g_stSettings.m_currentVideoSettings.m_Sharpness) {
    VdpBool enabled[]={0};
    vdp_st = vdp_video_mixer_set_feature_enables(videoMixer, 1, feature, enabled);
    CHECK_ST
    return;
  }
  VdpBool enabled[]={1};
  vdp_st = vdp_video_mixer_set_feature_enables(videoMixer, 1, feature, enabled);
  CHECK_ST
  void* sh[] = { &g_stSettings.m_currentVideoSettings.m_Sharpness };
  CLog::Log(LOGNOTICE,"Setting Sharpness to %f",g_stSettings.m_currentVideoSettings.m_Sharpness);
  vdp_st = vdp_video_mixer_set_attribute_values(videoMixer, 1, attributes, sh);
  CHECK_ST
}


void CDVDVideoCodecVDPAU::SetDeinterlacing()
{
  VdpVideoMixerFeature feature[] = { VDP_VIDEO_MIXER_FEATURE_DEINTERLACE_TEMPORAL,
                                     VDP_VIDEO_MIXER_FEATURE_DEINTERLACE_TEMPORAL_SPATIAL };
  VdpStatus vdp_st;

  if (!g_stSettings.m_currentVideoSettings.m_InterlaceMethod) {
    VdpBool enabled[]={0,0};
    vdp_st = vdp_video_mixer_set_feature_enables(videoMixer, 1, feature, enabled);
    CHECK_ST
    return;
  }
  else if (g_stSettings.m_currentVideoSettings.m_InterlaceMethod == 1) {
    VdpBool enabled[]={1,0};
    vdp_st = vdp_video_mixer_set_feature_enables(videoMixer, 1, feature, enabled);
    CHECK_ST
  }
  else if (g_stSettings.m_currentVideoSettings.m_InterlaceMethod == 2) {
    VdpBool enabled[]={1,1};
    vdp_st = vdp_video_mixer_set_feature_enables(videoMixer, 1, feature, enabled);
    CHECK_ST
  }
}

void CDVDVideoCodecVDPAU::InitVDPAUProcs()
{
  int mScreen = DefaultScreen(m_Display);
  VdpStatus vdp_st;

  // Create Device
  vdp_st = vdp_device_create_x11(m_Display, //x_display,
                                 mScreen, //x_screen,
                                 &vdp_device,
                                 &vdp_get_proc_address);
  CHECK_ST


  vdp_st = vdp_get_proc_address(vdp_device,
                                VDP_FUNC_ID_DEVICE_DESTROY,
                                (void **)&vdp_device_destroy);
  CHECK_ST

  vdp_st = vdp_get_proc_address(vdp_device,
                                VDP_FUNC_ID_VIDEO_SURFACE_CREATE,
                                (void **)&vdp_video_surface_create);
  CHECK_ST

  vdp_st = vdp_get_proc_address(
                                vdp_device,
                                VDP_FUNC_ID_VIDEO_SURFACE_DESTROY,
                                (void **)&vdp_video_surface_destroy
                                );
  CHECK_ST

  vdp_st = vdp_get_proc_address(
                                vdp_device,
                                VDP_FUNC_ID_VIDEO_SURFACE_PUT_BITS_Y_CB_CR,
                                (void **)&vdp_video_surface_put_bits_y_cb_cr
                                );
  CHECK_ST

  vdp_st = vdp_get_proc_address(
                                vdp_device,
                                VDP_FUNC_ID_VIDEO_SURFACE_GET_BITS_Y_CB_CR,
                                (void **)&vdp_video_surface_get_bits_y_cb_cr
                                );
  CHECK_ST

  vdp_st = vdp_get_proc_address(
                                vdp_device,
                                VDP_FUNC_ID_OUTPUT_SURFACE_PUT_BITS_Y_CB_CR,
                                (void **)&vdp_output_surface_put_bits_y_cb_cr
                                );
  CHECK_ST

  vdp_st = vdp_get_proc_address(
                                vdp_device,
                                VDP_FUNC_ID_OUTPUT_SURFACE_PUT_BITS_NATIVE,
                                (void **)&vdp_output_surface_put_bits_native
                                );
  CHECK_ST

  vdp_st = vdp_get_proc_address(
                                vdp_device,
                                VDP_FUNC_ID_OUTPUT_SURFACE_CREATE,
                                (void **)&vdp_output_surface_create
                                );
  CHECK_ST

  vdp_st = vdp_get_proc_address(
                                vdp_device,
                                VDP_FUNC_ID_OUTPUT_SURFACE_DESTROY,
                                (void **)&vdp_output_surface_destroy
                                );
  CHECK_ST

  vdp_st = vdp_get_proc_address(
                                vdp_device,
                                VDP_FUNC_ID_OUTPUT_SURFACE_GET_BITS_NATIVE,
                                (void **)&vdp_output_surface_get_bits_native
                                );
  CHECK_ST

  vdp_st = vdp_get_proc_address(
                                vdp_device,
                                VDP_FUNC_ID_VIDEO_MIXER_CREATE,
                                (void **)&vdp_video_mixer_create
                                );
  CHECK_ST

  vdp_st = vdp_get_proc_address(
                                vdp_device,
                                VDP_FUNC_ID_VIDEO_MIXER_SET_FEATURE_ENABLES,
                                (void **)&vdp_video_mixer_set_feature_enables
                                );
  CHECK_ST

  vdp_st = vdp_get_proc_address(
                                vdp_device,
                                VDP_FUNC_ID_VIDEO_MIXER_DESTROY,
                                (void **)&vdp_video_mixer_destroy
                                );
  CHECK_ST

  vdp_st = vdp_get_proc_address(
                                vdp_device,
                                VDP_FUNC_ID_VIDEO_MIXER_RENDER,
                                (void **)&vdp_video_mixer_render
                                );
  CHECK_ST

  vdp_st = vdp_get_proc_address(
                                vdp_device,
                                VDP_FUNC_ID_GENERATE_CSC_MATRIX,
                                (void **)&vdp_generate_csc_matrix
                                );
  CHECK_ST

  vdp_st = vdp_get_proc_address(
                                vdp_device,
                                VDP_FUNC_ID_VIDEO_MIXER_SET_ATTRIBUTE_VALUES,
                                (void **)&vdp_video_mixer_set_attribute_values
                                );
  CHECK_ST

  vdp_st = vdp_get_proc_address(
                                vdp_device,
                                VDP_FUNC_ID_VIDEO_MIXER_QUERY_PARAMETER_SUPPORT,
                                (void **)&vdp_video_mixer_query_parameter_support
                                );
  CHECK_ST

  vdp_st = vdp_get_proc_address(
                                vdp_device,
                                VDP_FUNC_ID_PRESENTATION_QUEUE_TARGET_DESTROY,
                                (void **)&vdp_presentation_queue_target_destroy
                                );
  CHECK_ST

  vdp_st = vdp_get_proc_address(
                                vdp_device,
                                VDP_FUNC_ID_PRESENTATION_QUEUE_CREATE,
                                (void **)&vdp_presentation_queue_create
                                );
  CHECK_ST

  vdp_st = vdp_get_proc_address(
                                vdp_device,
                                VDP_FUNC_ID_PRESENTATION_QUEUE_DESTROY,
                                (void **)&vdp_presentation_queue_destroy
                                );
  CHECK_ST

  vdp_st = vdp_get_proc_address(
                                vdp_device,
                                VDP_FUNC_ID_PRESENTATION_QUEUE_DISPLAY,
                                (void **)&vdp_presentation_queue_display
                                );
  CHECK_ST

  vdp_st = vdp_get_proc_address(
                                vdp_device,
                                VDP_FUNC_ID_PRESENTATION_QUEUE_BLOCK_UNTIL_SURFACE_IDLE,
                                (void **)&vdp_presentation_queue_block_until_surface_idle
                                );
  CHECK_ST

  vdp_st = vdp_get_proc_address(
                                vdp_device,
                                VDP_FUNC_ID_PRESENTATION_QUEUE_TARGET_CREATE_X11,
                                (void **)&vdp_presentation_queue_target_create_x11
                                );
  CHECK_ST

  vdp_st = vdp_get_proc_address(
                                vdp_device,
                                VDP_FUNC_ID_DECODER_CREATE,
                                (void **)&vdp_decoder_create
                                );
  CHECK_ST

  vdp_st = vdp_get_proc_address(
                                vdp_device,
                                VDP_FUNC_ID_DECODER_DESTROY,
                                (void **)&vdp_decoder_destroy
                                );
  CHECK_ST

  vdp_st = vdp_get_proc_address(
                                vdp_device,
                                VDP_FUNC_ID_DECODER_RENDER,
                                (void **)&vdp_decoder_render
                                );
  CHECK_ST

  vdp_st = vdp_get_proc_address(
                                vdp_device,
                                VDP_FUNC_ID_DECODER_QUERY_CAPABILITIES,
                                (void **)&vdp_decoder_query_caps
                                );
  CHECK_ST

  vdp_st = vdp_get_proc_address(
                                vdp_device,
                                VDP_FUNC_ID_PRESENTATION_QUEUE_QUERY_SURFACE_STATUS,
                                (void **)&vdp_presentation_queue_query_surface_status
                                );
  CHECK_ST

  vdp_st = vdp_get_proc_address(
                                vdp_device,
                                VDP_FUNC_ID_PRESENTATION_QUEUE_GET_TIME,
                                (void **)&vdp_presentation_queue_get_time
                                );
  CHECK_ST

  // Added for draw_osd.
  vdp_st = vdp_get_proc_address(
                                vdp_device,
                                VDP_FUNC_ID_OUTPUT_SURFACE_RENDER_OUTPUT_SURFACE,
                                (void **)&vdp_output_surface_render_output_surface
                                );
  CHECK_ST

  vdp_st = vdp_get_proc_address(
                                vdp_device,
                                VDP_FUNC_ID_OUTPUT_SURFACE_PUT_BITS_INDEXED,
                                (void **)&vdp_output_surface_put_bits_indexed
                                );
  CHECK_ST

  vdp_st = vdp_get_proc_address(
                                vdp_device,
                                VDP_FUNC_ID_PREEMPTION_CALLBACK_REGISTER,
                                (void **)&vdp_preemption_callback_register
                                );
  CHECK_ST

  vdp_st = vdp_preemption_callback_register(vdp_device,
                                   &VDPPreemptionCallbackFunction,
                                   (void*)this);
  CHECK_ST
}

VdpStatus CDVDVideoCodecVDPAU::FiniVDPAUProcs()
{
  VdpStatus vdp_st;

  vdp_st = vdp_device_destroy(vdp_device);
  CHECK_ST

  vdpauConfigured = false;

  return VDP_STATUS_OK;
}

void CDVDVideoCodecVDPAU::InitVDPAUOutput()
{
  m_Surface->MakePixmap(vid_width,vid_height);

  VdpStatus vdp_st;
  vdp_st = vdp_presentation_queue_target_create_x11(vdp_device,
                                                    m_Surface->GetXPixmap(), //x_window,
                                                    &vdp_flip_target);
  CHECK_ST

  vdp_st = vdp_presentation_queue_create(vdp_device,
                                         vdp_flip_target,
                                         &vdp_flip_queue);
  CHECK_ST
}

void CDVDVideoCodecVDPAU::InitCSCMatrix()
{
  VdpStatus vdp_st;
  m_Procamp.struct_version = VDP_PROCAMP_VERSION;
  m_Procamp.brightness = 0.0;
  m_Procamp.contrast = 1.0;
  m_Procamp.saturation = 1.0;
  m_Procamp.hue = 0;
  vdp_st = vdp_generate_csc_matrix(&m_Procamp,
                                   VDP_COLOR_STANDARD_ITUR_BT_709,
                                   &m_CSCMatrix);
  CHECK_ST
}

VdpStatus CDVDVideoCodecVDPAU::FiniVDPAUOutput()
{
  VdpStatus vdp_st;

  vdp_st = vdp_presentation_queue_destroy(vdp_flip_queue);
  CHECK_ST

  vdp_st = vdp_presentation_queue_target_destroy(vdp_flip_target);
  CHECK_ST

  free(videoSurfaces);
  videoSurfaces=NULL;

  return VDP_STATUS_OK;
}


int CDVDVideoCodecVDPAU::ConfigVDPAU(AVCodecContext* avctx)
{
  if (vdpauConfigured) return 1;
  VdpStatus vdp_st;
  int i;
  VdpDecoderProfile vdp_decoder_profile;
  VdpChromaType vdp_chroma_type;
  uint32_t max_references;
  vid_width = avctx->width;
  vid_height = avctx->height;
  image_format = avctx->pix_fmt;

  past[1] = past[0] = current = future = VDP_INVALID_HANDLE;
  CLog::Log(LOGNOTICE, "screenWidth:%i widWidth:%i",g_graphicsContext.GetWidth(),vid_width);
  // FIXME: Are higher profiles able to decode all lower profile streams?
  switch (image_format) {
    case PIX_FMT_VDPAU_MPEG1:
      vdp_decoder_profile = VDP_DECODER_PROFILE_MPEG1;
      vdp_chroma_type = VDP_CHROMA_TYPE_420;
      num_video_surfaces = NUM_VIDEO_SURFACES_MPEG2;
      break;
    case PIX_FMT_VDPAU_MPEG2:
      vdp_decoder_profile = VDP_DECODER_PROFILE_MPEG2_MAIN;
      vdp_chroma_type = VDP_CHROMA_TYPE_420;
      num_video_surfaces = NUM_VIDEO_SURFACES_MPEG2;
      break;
    case PIX_FMT_VDPAU_H264:
      vdp_decoder_profile = VDP_DECODER_PROFILE_H264_HIGH;
      vdp_chroma_type = VDP_CHROMA_TYPE_420;
      // Theoretically, "num_reference_surfaces+1" is correct.
      // However, to work around invalid/corrupt streams,
      // and/or ffmpeg DPB management issues,
      // we allocate more than we should need to allow problematic
      // streams to play.
      //num_video_surfaces = num_reference_surfaces + 1;
      num_video_surfaces = NUM_VIDEO_SURFACES_H264;
      break;
    case PIX_FMT_VDPAU_WMV3:
      vdp_decoder_profile = VDP_DECODER_PROFILE_VC1_MAIN;
      vdp_chroma_type = VDP_CHROMA_TYPE_420;
      num_video_surfaces = NUM_VIDEO_SURFACES_VC1;
      break;
    case PIX_FMT_VDPAU_VC1:
      vdp_decoder_profile = VDP_DECODER_PROFILE_VC1_ADVANCED;
      vdp_chroma_type = VDP_CHROMA_TYPE_420;
      num_video_surfaces = NUM_VIDEO_SURFACES_VC1;
      break;

      /* Non VDPAU specific formats.
       * No HW acceleration. VdpDecoder will not be created and
       * there will be no call for VdpDecoderRender.
       */
      /*case PIX_FMT_YV12:
       vdp_chroma_type = VDP_CHROMA_TYPE_420;
       num_video_surfaces = NUM_VIDEO_SURFACES_NON_ACCEL_YUV;
       break;*/
    case PIX_FMT_BGRA:
      // No need for videoSurfaces, directly renders to outputSurface.
      num_video_surfaces = NUM_VIDEO_SURFACES_NON_ACCEL_RGB;
      break;
    default:
      assert(0);
      return 1;
  }

  switch (image_format) {
    case PIX_FMT_VDPAU_H264:
   {
     max_references = avctx->ref_frames;
     if (max_references > 16) max_references = 16;
     if (max_references < 5) max_references=5;
     num_video_surfaces = max_references+6;
   }
      break;
    default:
      max_references = 2;
      break;
  }

  if (num_video_surfaces) {
    videoSurfaces = (VdpVideoSurface *)malloc(sizeof(VdpVideoSurface)*num_video_surfaces);
  } else {
    videoSurfaces = NULL;
  }

  if (IsVDPAUFormat(image_format)) {
    vdp_st = vdp_decoder_create(vdp_device,
                                vdp_decoder_profile,
                                vid_width,
                                vid_height,
                                max_references,
                                &decoder);
    CHECK_ST
  }

  // Creation of VideoSurfaces
  for (i = 0; i < num_video_surfaces; i++) {
    vdp_st = vdp_video_surface_create(vdp_device,
                                      vdp_chroma_type,
                                      vid_width,
                                      vid_height,
                                      &videoSurfaces[i]);
    CHECK_ST
  }

  if (num_video_surfaces) {
    surface_render = (vdpau_render_state*)malloc(num_video_surfaces*sizeof(vdpau_render_state));
    memset(surface_render,0,num_video_surfaces*sizeof(vdpau_render_state));

    for (i = 0; i < num_video_surfaces; i++) {
      //surface_render[i].magic = FF_VDPAU_RENDER_MAGIC;
      surface_render[i].state = FF_VDPAU_STATE_USED_FOR_RENDER;
      surface_render[i].surface = videoSurfaces[i];
    }

    // Creation of VideoMixer.
    VdpVideoMixerParameter parameters[] = {
      VDP_VIDEO_MIXER_PARAMETER_VIDEO_SURFACE_WIDTH,
      VDP_VIDEO_MIXER_PARAMETER_VIDEO_SURFACE_HEIGHT,
      VDP_VIDEO_MIXER_PARAMETER_CHROMA_TYPE
    };

    void const * parameter_values[] = {
      &vid_width,
      &vid_height,
      &vdp_chroma_type
    };

    VdpVideoMixerFeature features[] = {
      VDP_VIDEO_MIXER_FEATURE_NOISE_REDUCTION,
      VDP_VIDEO_MIXER_FEATURE_SHARPNESS,
      VDP_VIDEO_MIXER_FEATURE_DEINTERLACE_TEMPORAL,
      VDP_VIDEO_MIXER_FEATURE_DEINTERLACE_TEMPORAL_SPATIAL,
      VDP_VIDEO_MIXER_FEATURE_INVERSE_TELECINE
    };

    vdp_st = vdp_video_mixer_create(vdp_device,
                                    5,
                                    features,
                                    ARSIZE(parameters),
                                    parameters,
                                    parameter_values,
                                    &videoMixer);
    CHECK_ST

  } else {
    surface_render = NULL;
  }

  // Creation of outputSurfaces
  for (i = 0; i < NUM_OUTPUT_SURFACES; i++) {
    vdp_st = vdp_output_surface_create(vdp_device,
                                       VDP_RGBA_FORMAT_B8G8R8A8,
                                       vid_width,
                                       vid_height,
                                       &outputSurfaces[i]);
    CHECK_ST
  }
  surfaceNum = 0;
  outputSurface = outputSurfaces[surfaceNum];

  videoSurface = videoSurfaces[0];

  /*vdp_st = vdp_video_mixer_render(videoMixer,
                                  VDP_INVALID_HANDLE,
                                  0,
                                  VDP_VIDEO_MIXER_PICTURE_STRUCTURE_FRAME,
                                  0,
                                  NULL,
                                  videoSurface,
                                  0,
                                  NULL,
                                  NULL,
                                  outputSurface,
                                  &outRect,
                                  &outRectVid,
                                  0,
                                  NULL);
  CHECK_ST */

  InitVDPAUOutput();

  SpewHardwareAvailable();
  vdpauConfigured = true;
  return 0;
}

void CDVDVideoCodecVDPAU::SpewHardwareAvailable()  //Copyright (c) 2008 Wladimir J. van der Laan  -- VDPInfo
{
  VdpStatus rv;
  CLog::Log(LOGNOTICE,"VDPAU Decoder capabilities:");
  CLog::Log(LOGNOTICE,"name          level macbs width height");
  CLog::Log(LOGNOTICE,"------------------------------------");
  for(int x=0; x<decoder_profile_count; ++x)
  {
    VdpBool is_supported = false;
    uint32_t max_level, max_macroblocks, max_width, max_height;
    rv = vdp_decoder_query_caps(vdp_device, decoder_profiles[x].id,
                                &is_supported, &max_level, &max_macroblocks, &max_width, &max_height);
    if(rv == VDP_STATUS_OK && is_supported)
    {
      CLog::Log(LOGNOTICE,"%-16s %2i %5i %5i %5i\n", decoder_profiles[x].name,
                max_level, max_macroblocks, max_width, max_height);
    }
  }
}

enum PixelFormat CDVDVideoCodecVDPAU::FFGetFormat(struct AVCodecContext * avctx,
                                                     const PixelFormat * fmt)
{
  CDVDVideoCodecFFmpeg* ctx        = (CDVDVideoCodecFFmpeg*)avctx->opaque;
  CDVDVideoCodecVDPAU*  pSingleton = ctx->GetContextVDPAU();

  //CLog::Log(LOGNOTICE,"%s",__FUNCTION__);
  if(usingVDPAU){
    avctx->get_buffer      = FFGetBuffer;
    avctx->release_buffer  = FFReleaseBuffer;
    avctx->draw_horiz_band = FFDrawSlice;
    avctx->slice_flags=SLICE_FLAG_CODED_ORDER|SLICE_FLAG_ALLOW_FIELD;
  }
  pSingleton->ConfigVDPAU(avctx);
  return fmt[0];
}

vdpau_render_state * CDVDVideoCodecVDPAU::FindFreeSurface()
{
  int i;
  for (i = 0 ; i < num_video_surfaces; i++)
  {
    //CLog::Log(LOGDEBUG,"find_free_surface(%i):0x%08x @ 0x%08x",i,pSingleton->surface_render[i].state, &(pSingleton->surface_render[i]));
    if (!(surface_render[i].state & FF_VDPAU_STATE_USED_FOR_REFERENCE)) {
      return &(surface_render[i]);
    }
  }
  return NULL;
}

int CDVDVideoCodecVDPAU::FFGetBuffer(AVCodecContext *avctx, AVFrame *pic)
{
  //CLog::Log(LOGNOTICE,"%s",__FUNCTION__);
  CDVDVideoCodecFFmpeg* ctx        = (CDVDVideoCodecFFmpeg*)avctx->opaque;
  CDVDVideoCodecVDPAU*  pSingleton = ctx->GetContextVDPAU();
  struct pictureAge*    pA         = &pSingleton->picAge;

  pSingleton->m_avctx = avctx;
  pSingleton->ConfigVDPAU(avctx); //->width,avctx->height,avctx->pix_fmt);
  vdpau_render_state * render;

  render = pSingleton->FindFreeSurface();
  //assert(render->magic == FF_VDPAU_RENDER_MAGIC);
  render->state = 0;

  pic->data[0]= (uint8_t*)render;
  pic->data[1]= (uint8_t*)render;
  pic->data[2]= (uint8_t*)render;

  /* Note, some (many) codecs in libavcodec must have stride1==stride2 && no changes between frames
   * lavc will check that and die with an error message, if its not true
   */
  pic->linesize[0]= 0;
  pic->linesize[1]= 0;
  pic->linesize[2]= 0;

/*  double *pts= (double*)malloc(sizeof(double));
  *pts= ((CDVDVideoCodecFFmpeg*)avctx->opaque)->m_pts;
  pic->opaque= pts;
*/
  if(pic->reference)
  {   //I or P frame
    pic->age = pA->ip_age[0];
    pA->ip_age[0]= pA->ip_age[1]+1;
    pA->ip_age[1]= 1;
    pA->b_age++;
  }
  else
  {   //B frame
    pic->age = pA->b_age;
    pA->ip_age[0]++;
    pA->ip_age[1]++;
    pA->b_age = 1;
  }
  pic->type= FF_BUFFER_TYPE_USER;

  assert(render != NULL);
  //assert(render->magic == FF_VDPAU_RENDER_MAGIC);
  render->state |= FF_VDPAU_STATE_USED_FOR_REFERENCE;
  pic->reordered_opaque= avctx->reordered_opaque;
  return 0;
}

void CDVDVideoCodecVDPAU::FFReleaseBuffer(AVCodecContext *avctx, AVFrame *pic)
{
  //CLog::Log(LOGNOTICE,"%s",__FUNCTION__);
  vdpau_render_state * render;
  int i;

  // Mark the surface as not required for prediction
  render=(vdpau_render_state*)pic->data[2];
  assert(render != NULL);
  //assert(render->magic == FF_VDPAU_RENDER_MAGIC);
  render->state &= ~FF_VDPAU_STATE_USED_FOR_REFERENCE;
  for(i=0; i<4; i++){
    pic->data[i]= NULL;
  }
}


void CDVDVideoCodecVDPAU::FFDrawSlice(struct AVCodecContext *s,
                                           const AVFrame *src, int offset[4],
                                           int y, int type, int height)
{
  //CLog::Log(LOGNOTICE,"%s",__FUNCTION__);
  CDVDVideoCodecFFmpeg* ctx        = (CDVDVideoCodecFFmpeg*)s->opaque;
  CDVDVideoCodecVDPAU*  pSingleton = ctx->GetContextVDPAU();

  assert(src->linesize[0]==0 && src->linesize[1]==0 && src->linesize[2]==0);
  assert(offset[0]==0 && offset[1]==0 && offset[2]==0);

  VdpStatus vdp_st;
  vdpau_render_state * render;

  render = (vdpau_render_state*)src->data[2]; // this is a copy of private
  assert( render != NULL );
  //assert(render->magic == FF_VDPAU_RENDER_MAGIC);

  /* VdpDecoderRender is called with decoding order. Decoded images are store in
   * videoSurface like rndr->surface. VdpVideoMixerRender put this videoSurface
   * to outputSurface which is displayable.
   */
  pSingleton->CheckRecover();
  vdp_st = pSingleton->vdp_decoder_render(pSingleton->decoder,
                                          render->surface,
                                          (VdpPictureInfo const *)&(render->info),
                                          render->bitstream_buffers_used,
                                          render->bitstream_buffers);
  CHECK_ST
}

void CDVDVideoCodecVDPAU::PrePresent(AVCodecContext *avctx, AVFrame *pFrame)
{
  //CLog::Log(LOGNOTICE,"%s",__FUNCTION__);

  vdpau_render_state * render = (vdpau_render_state*)pFrame->data[2];
  VdpVideoMixerPictureStructure structure;
  VdpTime dummy;
  VdpStatus vdp_st;

  CheckFeatures();

  ConfigVDPAU(avctx); //->width,avctx->height,avctx->pix_fmt);
  outputSurface = outputSurfaces[surfaceNum];
  //  usleep(2000);
  past[1] = past[0];
  past[0] = current;
  current = future;
  future = render->surface;
  //if (pSingleton->past[1] == VDP_INVALID_HANDLE)
    //return;
  interlaced = pFrame->interlaced_frame;

  if (interlaced)
    structure = pFrame->top_field_first ? VDP_VIDEO_MIXER_PICTURE_STRUCTURE_TOP_FIELD :
                                          VDP_VIDEO_MIXER_PICTURE_STRUCTURE_BOTTOM_FIELD;
  else structure = VDP_VIDEO_MIXER_PICTURE_STRUCTURE_TOP_FIELD; //VDP_VIDEO_MIXER_PICTURE_STRUCTURE_FRAME;
  past[1] = past[0];
  past[0] = current;
  current = future;
  future = render->surface;

  CheckRecover();
  vdp_st = vdp_presentation_queue_block_until_surface_idle(
                                              vdp_flip_queue,
                                              outputSurface,
                                              &dummy);

  if (( outRect.x1 != outWidth ) ||
      ( outRect.y1 != outHeight ))
  {
    outRectVid.x0 = 0;
    outRectVid.y0 = 0;
    outRectVid.x1 = vid_width;
    outRectVid.y1 = vid_height;

    if(g_graphicsContext.GetViewWindow().right < vid_width)
      outWidth = vid_width;
    else
      outWidth = g_graphicsContext.GetViewWindow().right;
    if(g_graphicsContext.GetViewWindow().bottom < vid_height)
      outHeight = vid_height;
    else
      outHeight = g_graphicsContext.GetViewWindow().bottom;

    outRect.x0 = 0;
    outRect.y0 = 0;
    outRect.x1 = outWidth;
    outRect.y1 = outHeight;
  }

  CheckRecover();
  vdp_st = vdp_video_mixer_render(videoMixer,
                                              VDP_INVALID_HANDLE,
                                              0,
                                              structure,
                                              2,
                                              past,
                                              current,
                                              1,
                                              &(future),
                                              NULL,
                                              outputSurface,
                                              &(outRect),
                                              &(outRectVid),
                                              0,
                                              NULL);
  CHECK_ST
}

void CDVDVideoCodecVDPAU::Present()
{
  //CLog::Log(LOGNOTICE,"%s",__FUNCTION__);
  VdpStatus vdp_st;
  VdpTime time;

  vdp_st = vdp_presentation_queue_get_time(vdp_flip_queue, &time);
  previousTime = time;
  if (frameLagAverage > 5000000)
    time = time + 10000000;
  else
    time = time + (frameLagAverage *2);
//  time = time + (20000000);


  //CLog::Log(LOGNOTICE,"predicted %Li",
  CheckRecover();
  vdp_st = vdp_presentation_queue_display(vdp_flip_queue,
                                          outputSurface,
                                          0,
                                          0,
                                          0);  //time);
  CHECK_ST
  surfaceNum = surfaceNum ^ 1;
}

void CDVDVideoCodecVDPAU::VDPPreemptionCallbackFunction(VdpDevice device, void* context)
{
  CLog::Log(LOGERROR,"VDPAU Device Preempted - attempting recovery");
  CDVDVideoCodecVDPAU* pCtx = (CDVDVideoCodecVDPAU*)context;
  pCtx->recover = true;
}

bool CDVDVideoCodecVDPAU::CheckDeviceCaps(uint32_t Param)
{
  VdpStatus vdp_st;
  VdpBool supported = false;
  uint32_t max_level, max_macroblocks, max_width, max_height;
  vdp_st = vdp_decoder_query_caps(vdp_device, Param,
                              &supported, &max_level, &max_macroblocks, &max_width, &max_height);
  CHECK_ST
  return supported;
}

void CDVDVideoCodecVDPAU::NotifySwap()
{
  VdpStatus vdp_st;
  vdp_st = vdp_presentation_queue_get_time(vdp_flip_queue, &(lastSwapTime));
  CHECK_ST
  if (previousTime) {
    frameCounter++;
    frameLagTime = lastSwapTime - previousTime;
    frameLagTimeRunning += frameLagTime;
    frameLagAverage = frameLagTimeRunning / frameCounter;
  }
}

#endif
