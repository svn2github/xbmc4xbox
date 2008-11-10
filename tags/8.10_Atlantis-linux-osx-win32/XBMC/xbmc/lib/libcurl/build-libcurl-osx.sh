#!/bin/bash 

# curl-7.18.2 configure settings cloned from MacPorts
# build against 10.4 sdk for 10.5/10.4 compatibility

# build libcurl
#depends_libs: libz

wget http://curl.sourceforge.net/download/curl-7.18.2.tar.gz
tar -xzf curl-7.18.2.tar.gz
cd curl-7.18.2

sh configure MACOSX_DEPLOYMENT_TARGET=10.4 CFLAGS="-O2 -isysroot /Developer/SDKs/MacOSX10.4u.sdk -mmacosx-version-min=10.4 -fno-common" \
    --disable-ipv6 \
    --without-libidn \
    --without-libssh2 \
    --without-ssl \
    --disable-ldap

make
gcc -bundle -flat_namespace -undefined suppress -fPIC -shared -o ../../../../system/libcurl-osx.so lib/.libs/*.o
chmod +x ../../../../system/libcurl-osx.so

cd ..
rm -rf curl-7.18.2 curl-7.18.2.tar.gz
