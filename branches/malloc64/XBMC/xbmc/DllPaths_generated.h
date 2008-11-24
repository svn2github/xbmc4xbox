#ifndef DLL_PATHS_GENERATED_H_
#define DLL_PATHS_GENERATED_H_

#ifdef __APPLE__
#define ARCH "osx"
#endif // __APPLE__

/* libraries */
#define DLL_PATH_IMAGELIB      "Q:\\system\\ImageLib-" ARCH ".so"
#define DLL_PATH_LIBEXIF       "Q:\\system\\libexif-" ARCH ".so"
#define DLL_PATH_LIBID3TAG     "Q:\\system\\libid3tag-" ARCH ".so"
#define DLL_PATH_LIBHDHOMERUN  "Q:\\system\\hdhomerun-" ARCH ".so"
#define DLL_PATH_LIBCMYTH      "xbmc.so"

#ifndef DLL_PATH_LIBCURL
#ifdef __APPLE__
#define DLL_PATH_LIBCURL       "Q:\\system\\libcurl-" ARCH ".so"
#else
#define DLL_PATH_LIBCURL       "/usr/lib/libcurl.so"
#endif
#endif

/* paplayer */
#define DLL_PATH_AAC_CODEC     "Q:\\system\\players\\paplayer\\AACCodec-" ARCH ".so"
#define DLL_PATH_AC3_CODEC     "Q:\\system\\players\\paplayer\\ac3codec-" ARCH ".so"
#define DLL_PATH_ADPCM_CODEC   "Q:\\system\\players\\paplayer\\adpcm-" ARCH ".so"
#define DLL_PATH_ADPLUG_CODEC  "Q:\\system\\players\\paplayer\\adplug-" ARCH ".so"
#define DLL_PATH_APE_CODEC     "Q:\\system\\players\\paplayer\\MACDll-" ARCH ".so"
#define DLL_PATH_ASAP_CODEC    "Q:\\system\\players\\paplayer\\xbmc_asap-" ARCH ".so"
#define DLL_PATH_DCA_CODEC     "Q:\\system\\players\\paplayer\\dcacodec-" ARCH ".so"
#define DLL_PATH_FLAC_CODEC    "Q:\\system\\players\\paplayer\\libFLAC-" ARCH ".so"
#define DLL_PATH_GYM_CODEC     "Q:\\system\\players\\paplayer\\gensapu-" ARCH ".so"
#define DLL_PATH_MAD_CODEC     "Q:\\system\\players\\paplayer\\MADCodec-" ARCH ".so"
#define DLL_PATH_MID_CODEC     "Q:\\system\\players\\paplayer\\timidity-" ARCH ".so"
#define DLL_PATH_MODULE_CODEC  "Q:\\system\\players\\paplayer\\dumb-" ARCH ".so"
#define DLL_PATH_MPC_CODEC     "Q:\\system\\players\\paplayer\\libmpcdec-" ARCH ".so"
#define DLL_PATH_NSF_CODEC     "Q:\\system\\players\\paplayer\\nosefart-" ARCH ".so"
#define DLL_PATH_OGG_CODEC     "Q:\\system\\players\\paplayer\\vorbisfile-" ARCH ".so"
#define DLL_PATH_SID_CODEC     "Q:\\system\\players\\paplayer\\libsidplay2-" ARCH ".so"
#define DLL_PATH_SPC_CODEC     "Q:\\system\\players\\paplayer\\SNESAPU-" ARCH ".so"
#define DLL_PATH_VGM_CODEC     "Q:\\system\\players\\paplayer\\vgmstream-" ARCH ".so"
#define DLL_PATH_WAVPACK_CODEC "Q:\\system\\players\\paplayer\\wavpack-" ARCH ".so"
#define DLL_PATH_WMA_CODEC     "Q:\\system\\players\\paplayer\\wma-" ARCH ".so"
#define DLL_PATH_YM_CODEC      "Q:\\system\\players\\paplayer\\stsoundlibrary-" ARCH ".so"
#define DLL_PATH_SHN_CODEC     "Q:\\system\\players\\paplayer\\libshnplay-" ARCH ".so"

/* dvdplayer */
#define DLL_PATH_LIBASS        "Q:\\system\\players\\dvdplayer\\libass-" ARCH ".so"
#define DLL_PATH_LIBA52        "Q:\\system\\players\\dvdplayer\\liba52-" ARCH ".so"
#define DLL_PATH_LIBDTS        "Q:\\system\\players\\dvdplayer\\libdts-" ARCH ".so"
#define DLL_PATH_LIBFAAD       "Q:\\system\\players\\dvdplayer\\libfaad-" ARCH ".so"
#define DLL_PATH_LIBMAD        "Q:\\system\\players\\dvdplayer\\libmad-" ARCH ".so"
#define DLL_PATH_LIBMPEG2      "Q:\\system\\players\\dvdplayer\\libmpeg2-" ARCH ".so"
#define DLL_PATH_LIBDVDNAV     "Q:\\system\\players\\dvdplayer\\libdvdnav-" ARCH ".so"

/* ffmpeg */
#define DLL_PATH_LIBAVCODEC    "Q:\\system\\players\\dvdplayer\\avcodec-51-" ARCH ".so"
#define DLL_PATH_LIBAVFORMAT   "Q:\\system\\players\\dvdplayer\\avformat-51-" ARCH ".so"
#define DLL_PATH_LIBAVUTIL     "Q:\\system\\players\\dvdplayer\\avutil-51-" ARCH ".so"
#define DLL_PATH_LIBPOSTPROC   "Q:\\system\\players\\dvdplayer\\postproc-51-" ARCH ".so"
#define DLL_PATH_LIBSWSCALE    "Q:\\system\\players\\dvdplayer\\swscale-51-" ARCH ".so"

/* cdrip */
#define DLL_PATH_LAME_ENC      "Q:\\system\\cdrip\\lame_enc-" ARCH ".so"
#define DLL_PATH_OGG           "Q:\\system\\cdrip\\ogg-" ARCH ".so"
#define DLL_PATH_VORBIS_ENC    "Q:\\system\\cdrip\\vorbisenc-" ARCH ".so"
#define DLL_PATH_VORBIS        "Q:\\system\\cdrip\\vorbis-" ARCH ".so"

#endif

