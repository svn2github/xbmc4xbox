#!/bin/bash 

set -e

xbmc_clean ()
{
  echo "Cleaning ..."
  [ -d .libs ] && rm -r .libs
  make distclean 2>/dev/null || true
}

# $1 = additional configure parameters
xbmc_configure ()
{
  echo "Configuring ..."
  CFLAGS="-D_XBOX -fno-common -mtune=pentium3 -msse -mfpmath=sse -pipe"
  PARAMS=" \
  --cpu=pentium3 \
  --enable-gpl \
  --enable-shared \
  --disable-static \
  --enable-w32threads \
  --enable-memalign-hack \
  --enable-small \
  --enable-zlib \
  --disable-debug \
  \
  --disable-doc \
  --disable-ffmpeg \
  --disable-ffplay \
  --disable-ffprobe \
  --disable-ffserver \
  \
  --disable-muxers \
  --enable-muxer=spdif \
  --disable-encoders \
  --disable-devices \
  --disable-bsfs \
  \
  --enable-postproc \
  \
  --disable-filters \
  --enable-filter=buffer \
  \
  --disable-protocols \
  --enable-protocol=concat,file,pipe,gopher,mmst,rtp,tcp,udp,http \
  \
  --disable-vdpau \
  --disable-vaapi \
  --disable-dxva2 \
  \
  --disable-altivec \
  --disable-amd3dnow \
  --disable-amd3dnowext \
  --disable-ssse3 \
  --disable-armv5te \
  --disable-armv6 \
  --disable-armv6t2 \
  --disable-armvfp \
  --disable-iwmmxt \
  --disable-mmi \
  --disable-neon \
  --disable-vis"
  echo "--extra-cflags=\"$CFLAGS\" $PARAMS $1"
  ./configure --extra-cflags="$CFLAGS" $PARAMS $1
}

# $1 = destination folder
xbmc_make ()
{
  set -e
  echo "Making ..."
  make -j2
  [ ! -d .libs ] && mkdir .libs
  cp lib*/*.dll .libs/
  mv .libs/swscale-0.dll .libs/swscale-0.6.1.dll
  if [ "$1" != "" ]; then
    echo "Copying libraries to $1 ..."
    [ ! -d "$1" ] && mkdir -p "$1"
    cp .libs/avcodec-52.dll "$1"
    cp .libs/avformat-52.dll "$1"
    cp .libs/avutil-50.dll "$1"
    cp .libs/postproc-51.dll "$1"
    cp .libs/swscale-0.6.1.dll "$1"
  fi
}

xbmc_all ()
{
  xbmc_clean
  xbmc_configure "\
    --disable-decoders \
    --enable-decoder=mpeg4,msmpeg4v1,msmpeg4v2,msmpeg4v3 \
    --enable-decoder=vp6,vp6a,vp6f \
    --enable-decoder=mp1,mp2,mp3,mpegvideo,mpeg1video,mpeg2video \
	--enable-decoder=mjpeg,mjpegb \
    --enable-decoder=wmav1,wmav2,wmapro,wmv1,wmv2,wmv3 \
    --enable-decoder=aac,ac3,dca,dvbsub,dvdsub,flv,h263,h264,rtp,vorbis \
    \
    --disable-demuxers \
    --enable-demuxer=mp1,mp2,mp3,mpegps,mpegts,mpegtsraw,mpegvideo \
    --enable-demuxer=aac,ac3,dts,asf,avi,flv,h263,h264,ogg,matroska,mov \
    --enable-demuxer=nuv,sdp,rtsp \
	"
	xbmc_make ../../../../../system/players/dvdplayer/
	xbmc_clean
	xbmc_configure
	xbmc_make ../../../../../system/players/dvdplayer/full
}

case "$1"
in
  clean)
    xbmc_clean
  ;;
  configure)
    xbmc_configure "$2"
  ;;
  make)
    xbmc_make "$2"
  ;;
  all)
    xbmc_all "$2"
  ;;
  *)
    echo "$0 clean|configure [additional parameters]|make [install dir]|all"
  ;;
esac

