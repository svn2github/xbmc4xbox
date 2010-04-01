#!/bin/bash

#      Copyright (C) 2005-2008 Team XBMC
#      http://www.xbmc.org
#
#  This Program is free software; you can redistribute it and/or modify
#  it under the terms of the GNU General Public License as published by
#  the Free Software Foundation; either version 2, or (at your option)
#  any later version.
#
#  This Program is distributed in the hope that it will be useful,
#  but WITHOUT ANY WARRANTY; without even the implied warranty of
#  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
#  GNU General Public License for more details.
#
#  You should have received a copy of the GNU General Public License
#  along with XBMC; see the file COPYING.  If not, write to
#  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
#  http://www.gnu.org/copyleft/gpl.html


THISDIR=$(pwd)

if ! ls live-initramfs_*.udeb > /dev/null 2>&1 ; then
	echo "Making live-initramfs..."
	if [ ! -f live-initramfs.tar ]; then
		git clone git://live.debian.net/git/live-initramfs.git
		if [ "$?" -ne "0" ]; then
			exit 1
		fi

		# Saved, to avoid cloning for multiple builds
		tar cvf live-initramfs.tar live-initramfs  > /dev/null 2>&1
	else
		tar xvf live-initramfs.tar  > /dev/null 2>&1
	fi

	#
	# (Ugly) Patch to allow FAT boot disk to be mounted RW
	#	discussions is in progress with upstream developer
	#
	sed -i -e "/\"\${devname}\" \${mountpoint}/s/-o ro,noatime /\$([ "\$fstype" = \"vfat\" ] \&\& echo \"-o rw,noatime,umask=000\" \|\| echo \"-o ro,noatime\") /" $THISDIR/live-initramfs/scripts/live

	cd $THISDIR/live-initramfs
	dpkg-buildpackage -rfakeroot -b -uc -us 
	cd $THISDIR
fi
