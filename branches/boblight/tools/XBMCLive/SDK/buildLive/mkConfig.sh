
makeConfig()
{
	CATEGORIES="main restricted universe multiverse"

	configString=""
	configString="$configString --distribution karmic"
	configString="$configString --mirror-chroot-security http://security.ubuntu.com/ubuntu/"
	configString="$configString --mirror-binary-security http://security.ubuntu.com/ubuntu/"
	configString="$configString --binary-images iso"
	configString="$configString --binary-filesystem fat32"
	configString="$configString --hostname XBMCLive"
	configString="$configString --iso-application XBMC_Live"
	configString="$configString --iso-volume XBMC_Live"
	configString="$configString --iso-publisher http://xbmc.org"

	configString="$configString --bootloader grub"

	configString="$configString --debian-installer live"
        configString="$configString --debian-installer-preseedfile preseed.cfg"
	configString="$configString --debian-installer-gui disabled"
	configString="$configString --win32-loader disabled"

	configString="$configString --initramfs live-initramfs"

	if [ -n "$APT_HTTP_PROXY" ]; then
		configString="$configString --apt-http-proxy $APT_HTTP_PROXY"
	fi
	if [ -n "$APT_FTP_PROXY" ]; then
		configString="$configString --apt-ftp-proxy $APT_FTP_PROXY"
	fi

	configString="$configString --mirror-bootstrap http://archive.ubuntu.com/ubuntu/"
	configString="$configString --mirror-binary http://archive.ubuntu.com/ubuntu/"
	configString="$configString --mirror-chroot http://archive.ubuntu.com/ubuntu/"

	lh config --mode ubuntu --archive-areas "$CATEGORIES" $configString

	# Get TVHeadend public key
	wget --no-proxy -q http://www.lonelycoder.com/public.key
	if [ "$?" -ne "0" ]; then
		echo "Needed public key not found, exiting..."
		exit 1
	fi

	cp public.key "$THISDIR"/Files/chroot_sources/tvheadend.binary.gpg
	cp public.key "$THISDIR"/Files/chroot_sources/tvheadend.chroot.gpg
	rm public.key 

	# Copy files for chroot
	cp -R "$THISDIR"/Files/chroot_* config

	# Copy files for ISO
	cp -R "$THISDIR"/Files/binary_* config
}
