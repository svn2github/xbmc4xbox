#!/bin/bash 


#-------------------------------------------------------------------------------
# build libSDL/libSDLmain
#depends_libs: libX11.6

wget http://www.libsdl.org/release/SDL-1.2.13.tar.gz
tar -xzf SDL-1.2.13.tar.gz
cd SDL-1.2.13
patch -p1 <../SDL-1.2.13-xbmc.patch
./configure MACOSX_DEPLOYMENT_TARGET=10.4 CFLAGS="-O2 -isysroot /Developer/SDKs/MacOSX10.4u.sdk -mmacosx-version-min=10.4" --disable-nasm 
make
cp build/.libs/libSDL.a ../libSDL-osx.a
cp build/libSDLmain.a ../libSDLmain-osx.a
cd ..
rm -rf SDL-1.2.13 SDL-1.2.13.tar.gz


#-------------------------------------------------------------------------------
# build libSDL_image
#depends_libs: libsdl, libpng, libjpeg, libtiff, zlib

wget http://www.libsdl.org/projects/SDL_image/release/SDL_image-1.2.6.tar.gz
tar -xzf SDL_image-1.2.6.tar.gz
cd SDL_image-1.2.6
./configure MACOSX_DEPLOYMENT_TARGET=10.4 CFLAGS="-O2 -isysroot /Developer/SDKs/MacOSX10.4u.sdk -mmacosx-version-min=10.4" --enable-static --disable-shared --disable-sdltest 
make
cp .libs/libSDL_image.a ../libSDL_image-osx.a
cd ..
rm -rf SDL_image-1.2.6 SDL_image-1.2.6.tar.gz


#-------------------------------------------------------------------------------
# build libSDL_mixer
#depends_libs: libsdl, libvorbis, libogg

wget http://www.libsdl.org/projects/SDL_mixer/release/SDL_mixer-1.2.8.tar.gz
tar -xzf SDL_mixer-1.2.8.tar.gz
cd SDL_mixer-1.2.8
./configure MACOSX_DEPLOYMENT_TARGET=10.4 CFLAGS="-O2 -isysroot /Developer/SDKs/MacOSX10.4u.sdk -mmacosx-version-min=10.4" --enable-static --disable-shared --disable-music-ogg --disable-music-mp3 --disable-music-mod --disable-music-midi --disable-sdltest
make
cp build/.libs/libSDL_mixer.a ../libSDL_mixer-osx.a
cd ..
rm -rf SDL_mixer-1.2.8 SDL_mixer-1.2.8.tar.gz

