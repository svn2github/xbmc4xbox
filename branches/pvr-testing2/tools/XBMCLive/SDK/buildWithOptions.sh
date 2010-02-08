#!/bin/sh

# Using apt-cacher(-ng) to speed up apt-get downloads

SDK_BUILDHOOKS=""

# getopt-parse.bash

TEMP=$(getopt -o snp:ulkgi --long xbmc-svn,nvidia-only,proxy:,usb-image,live-only,keep-workarea,grub2,intel-only -- "$@")
eval set -- "$TEMP"

while true
do
	case $1 in
	-s|--xbmc-svn)
		echo "Enable option: Use XBMC SVN PPA"
		export SDK_BUILDHOOKS="$SDK_BUILDHOOKS ./xbmcSvnHook.sh"
		shift
		;;
	-n|--nvidia-only)
		echo "Enable option: NVIDIA support only"
		export SDK_BUILDHOOKS="$SDK_BUILDHOOKS ./nvidiaOnlyHook.sh"
		shift
		;;
	-u|--usb-image)
		echo "Enable option: Generate USBHDD disk image"
		export SDK_BUILDHOOKS="$SDK_BUILDHOOKS ./usbhddImageHook.sh"
		shift
		;;
	-l|--live-only)
		echo "Enable option: Do not include Debian Installer"
		export SDK_BUILDHOOKS="$SDK_BUILDHOOKS ./liveOnlyHook.sh"
		shift
		;;
	-k|--keep-workarea)
		echo "Enable option: Do not delete temporary workarea"
		export KEEP_WORKAREA=1
		shift
		;;
	-g|--grub2)
		echo "Enable option: Use grub2"
		export SDK_BUILDHOOKS="$SDK_BUILDHOOKS ./grub2Hook.sh"
		shift
		;;
	-i|--intel-only)
		echo "Enable option: Intel support only"
		export SDK_BUILDHOOKS="$SDK_BUILDHOOKS ./intelOnlyHook.sh"
		shift
		;;
	-p|--proxy)
		echo "Enable option: Use APT proxy"
		case "$2" in
			"") echo "No proxy URL provided, exiting"; exit ;;
			*)  PROXY_URL=$2;;
		esac

		export APT_HTTP_PROXY=$PROXY_URL
		export APT_FTP_PROXY=$PROXY_URL

		# We use apt-cacher when retrieving d-i udebs, too
		export http_proxy=$PROXY_URL
		export ftp_proxy=$PROXY_URL
		shift 2
		;;
	--) shift ; break ;;
	esac
done

./build.sh
