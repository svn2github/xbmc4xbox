#pragma once
/*
 *      Copyright (C) 2005-2011 Team XBMC
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

#if (defined HAVE_CONFIG_H) && (!defined WIN32)
  #include "config.h"
#endif
#include "DynamicDll.h"
#include "DllAvUtil.h"

extern "C" {
#ifndef HAVE_MMX
#define HAVE_MMX
#endif
#ifndef __STDC_CONSTANT_MACROS
#define __STDC_CONSTANT_MACROS
#endif
#ifndef __STDC_LIMIT_MACROS
#define __STDC_LIMIT_MACROS
#endif

#ifndef __GNUC__
#pragma warning(disable:4244)
#endif

#include "libavutil/avutil.h"
#include "libavutil/crc.h"
#include "libavutil/opt.h"
#include "libavutil/mem.h"
#include "libavutil/fifo.h"
#include "libavutil/samplefmt.h"

}

#include "settings.h"

// callback used for logging
void ff_avutil_log(void* ptr, int level, const char* format, va_list va);

class DllAvUtilInterface
{
public:
  virtual ~DllAvUtilInterface() {}
  virtual void av_log_set_callback(void (*)(void*, int, const char*, va_list))=0;
  virtual void *av_malloc(unsigned int size)=0;
  virtual void *av_mallocz(unsigned int size)=0;
  virtual void *av_realloc(void *ptr, unsigned int size)=0;
  virtual void av_free(void *ptr)=0;
  virtual void av_freep(void *ptr)=0;
  virtual int64_t av_rescale_rnd(int64_t a, int64_t b, int64_t c, enum AVRounding)=0;
  virtual const AVCRC* av_crc_get_table(AVCRCId crc_id)=0;
  virtual uint32_t av_crc(const AVCRC *ctx, uint32_t crc, const uint8_t *buffer, size_t length)=0;
  virtual int av_set_string3(void *obj, const char *name, const char *val, int alloc, const AVOption **o_out)=0;
  virtual char *av_strdup(const char *s)=0;
  virtual int av_get_bits_per_sample_fmt(enum AVSampleFormat p1) = 0;
};

#if (defined USE_EXTERNAL_FFMPEG)

// Use direct layer
class DllAvUtilBase : public DllDynamic, DllAvUtilInterface
{
public:
  
  virtual ~DllAvUtilBase() {}
   virtual void av_log_set_callback(void (*foo)(void*, int, const char*, va_list)) { ::av_log_set_callback(foo); }
   virtual void *av_malloc(unsigned int size) { return ::av_malloc(size); }
   virtual void *av_mallocz(unsigned int size) { return ::av_mallocz(size); }
   virtual void *av_realloc(void *ptr, unsigned int size) { return ::av_realloc(ptr, size); } 
   virtual void av_free(void *ptr) { ::av_free(ptr); }
   virtual void av_freep(void *ptr) { ::av_freep(ptr); }
   virtual int64_t av_rescale_rnd(int64_t a, int64_t b, int64_t c, enum AVRounding d) { return ::av_rescale_rnd(a, b, c, d); }
   virtual const AVCRC* av_crc_get_table(AVCRCId crc_id) { return ::av_crc_get_table(crc_id); }
   virtual uint32_t av_crc(const AVCRC *ctx, uint32_t crc, const uint8_t *buffer, size_t length) { return ::av_crc(ctx, crc, buffer); }
#if LIBAVCODEC_VERSION_INT >= AV_VERSION_INT(52,7,0)
   // API added on: 2008-12-16
   virtual int av_set_string3(void *obj, const char *name, const char *val, int alloc, const AVOption **o_out) { return ::av_set_string3(obj, name, val, alloc, o_out); }
#else
   virtual int av_set_string3(void *obj, const char *name, const char *val, int alloc, const AVOption **o_out) { return A
#endif
  virtual int av_get_bits_per_sample_fmt(enum AVSampleFormat p1)
    { return ::av_get_bits_per_sample_fmt(p1); }

   // DLL faking.
   virtual bool ResolveExports() { return true; }
   virtual bool Load() {
     CLog::Log(LOGDEBUG, "DllAvUtilBase: Using libavutil system library");
     return true;
   }
   virtual void Unload() {}
};

#else

class DllAvUtilBase : public DllDynamic, DllAvUtilInterface
{
public:
  DllAvUtilBase() : DllDynamic( g_settings.GetFFmpegDllFolder() + "avutil-51.dll") {}

  LOAD_SYMBOLS()

  DEFINE_METHOD1(void, av_log_set_callback, (void (*p1)(void*, int, const char*, va_list)))
  DEFINE_METHOD1(void*, av_malloc, (unsigned int p1))
  DEFINE_METHOD1(void*, av_mallocz, (unsigned int p1))
  DEFINE_METHOD2(void*, av_realloc, (void *p1, unsigned int p2))
  DEFINE_METHOD1(void, av_free, (void *p1))
  DEFINE_METHOD1(void, av_freep, (void *p1))
  DEFINE_METHOD4(int64_t, av_rescale_rnd, (int64_t p1, int64_t p2, int64_t p3, enum AVRounding p4));
  DEFINE_METHOD1(const AVCRC*, av_crc_get_table, (AVCRCId p1))
  DEFINE_METHOD4(uint32_t, av_crc, (const AVCRC *p1, uint32_t p2, const uint8_t *p3, size_t p4));
  DEFINE_METHOD5(int, av_set_string3, (void *p1, const char *p2, const char *p3, int p4, const AVOption **p5));
  DEFINE_METHOD1(char*, av_strdup, (const char *p1))
  DEFINE_METHOD1(int, av_get_bits_per_sample_fmt, (enum AVSampleFormat p1))

  public:
  BEGIN_METHOD_RESOLVE()
    RESOLVE_METHOD(av_log_set_callback)
    RESOLVE_METHOD(av_malloc)
    RESOLVE_METHOD(av_mallocz)
    RESOLVE_METHOD(av_realloc)
    RESOLVE_METHOD(av_free)
    RESOLVE_METHOD(av_freep)
    RESOLVE_METHOD(av_rescale_rnd)
    RESOLVE_METHOD(av_crc_get_table)
    RESOLVE_METHOD(av_crc)
    RESOLVE_METHOD(av_set_string3)
    RESOLVE_METHOD(av_strdup)
    RESOLVE_METHOD(av_get_bits_per_sample_fmt)
  END_METHOD_RESOLVE()
};

#endif

class DllAvUtil : public DllAvUtilBase
{
public:
  virtual bool Load()
  {
    if( DllAvUtilBase::Load() )
    {
      DllAvUtilBase::av_log_set_callback(ff_avutil_log);
      return true;
    }
    return false;
  }
};
