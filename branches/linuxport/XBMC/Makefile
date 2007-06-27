DIRS=guilib xbmc xbmc/FileSystem xbmc/FileSystem/MusicDatabaseDirectory xbmc/FileSystem/VideoDatabaseDirectory xbmc/cores xbmc/cores/paplayer xbmc/cores/DllLoader xbmc/cores/DllLoader/exports xbmc/cores/DllLoader/exports/util xbmc/xbox xbmc/linux xbmc/visualizations xbmc/utils guilib/common guilib/tinyXML xbmc/lib/sqlLite xbmc/lib/libPython xbmc/lib/libPython/xbmcmodule xbmc/lib/libscrobbler xbmc/lib/UnrarXLib xbmc/lib/libGoAhead xbmc/lib/libUPnP xbmc/cores/dvdplayer xbmc/cores/dvdplayer/DVDSubtitles xbmc/cores/dvdplayer/DVDInputStreams xbmc/cores/dvdplayer/DVDCodecs xbmc/cores/dvdplayer/DVDCodecs/Audio xbmc/cores/dvdplayer/DVDCodecs/Video xbmc/cores/dvdplayer/DVDCodecs/Overlay xbmc/cores/dvdplayer/DVDCodecs/Overlay/libspucc xbmc/cores/dvdplayer/DVDDemuxers xbmc/cores/VideoRenderers

all : compile 
	$(MAKE) XboxMediaCenter

.PHONY : guilib xbmc filesystem musicdatabase videodatabase cores paplayer dllloader exports xbox linux visualizations utils common tinyxml sqllite libscrobbler unrarxlib libpython libgoahead compile dvdplayer libupnp

sdl_2d:
	$(MAKE) SDL_DEFINES=-DHAS_SDL_2D all 

guilib: 
	$(MAKE) -C guilib
xbmc: 
	$(MAKE) -C xbmc
filesystem: 
	$(MAKE) -C xbmc/FileSystem
musicdatabase: 
	$(MAKE) -C xbmc/FileSystem/MusicDatabaseDirectory
videodatabase: 
	$(MAKE) -C xbmc/FileSystem/VideoDatabaseDirectory
cores: 
	$(MAKE) -C xbmc/cores
paplayer: 
	$(MAKE) -C xbmc/cores/paplayer
dllloader: 
	$(MAKE) -C xbmc/cores/DllLoader
exports: 
	$(MAKE) -C xbmc/cores/DllLoader/exports
	$(MAKE) -C xbmc/cores/DllLoader/exports/util
xbox: 
	$(MAKE) -C xbmc/xbox
linux: 
	$(MAKE) -C xbmc/linux
visualizations: 
	$(MAKE) -C xbmc/visualizations
utils: 
	$(MAKE) -C xbmc/utils
common: 
	$(MAKE) -C guilib/common
tinyxml: 
	$(MAKE) -C guilib/tinyXML
sqllite: 
	$(MAKE) -C xbmc/lib/sqlLite
libscrobbler: 
	$(MAKE) -C xbmc/lib/libscrobbler
unrarxlib: 
	$(MAKE) -C xbmc/lib/UnrarXLib
libpython: 
	$(MAKE) -C xbmc/lib/libPython
	$(MAKE) -C xbmc/lib/libPython/xbmcmodule
	$(MAKE) -C xbmc/lib/libPython/linux
libgoahead: 
	$(MAKE) -C xbmc/lib/libGoAhead
libupnp: 
	$(MAKE) -C xbmc/lib/libUPnP
dvdplayer:
	$(MAKE) -C xbmc/cores/dvdplayer
	$(MAKE) -C xbmc/cores/dvdplayer/DVDSubtitles
	$(MAKE) -C xbmc/cores/dvdplayer/DVDInputStreams
	$(MAKE) -C xbmc/cores/dvdplayer/DVDCodecs
	$(MAKE) -C xbmc/cores/dvdplayer/DVDCodecs/Audio
	$(MAKE) -C xbmc/cores/dvdplayer/DVDCodecs/Video
	$(MAKE) -C xbmc/cores/dvdplayer/DVDCodecs/Overlay
	$(MAKE) -C xbmc/cores/dvdplayer/DVDDemuxers
	$(MAKE) -C xbmc/cores/VideoRenderers 
	
compile: guilib xbmc filesystem musicdatabase videodatabase cores paplayer dllloader exports xbox linux visualizations utils common tinyxml sqllite libscrobbler libgoahead unrarxlib libpython dvdplayer libupnp

XboxMediaCenter: $(wildcard xbmc/*.o xbmc/settings/*.o guilib/*.o guilib/tinyXML/*.o guilib/common/*.o xbmc/FileSystem/*.o xbmc/FileSystem/VideoDatabaseDirectory/*.o xbmc/FileSystem/MusicDatabaseDirectory/*.o xbmc/visualizations/*.o xbmc/cores/*.o xbmc/cores/paplayer/*.o xbmc/linux/*.o xbmc/lib/sqlLite/*.o xbmc/lib/libscrobbler/*.o xbmc/lib/libPython/*.o xbmc/lib/libPython/xbmcmodule/*.o xbmc/xbox/*.o xbmc/cores/DllLoader/*.o xbmc/cores/DllLoader/exports/*.o xbmc/cores/DllLoader/exports/util/*.o xbmc/utils/*.o xbmc/lib/UnrarXLib/*.o xbmc/lib/libGoAhead/*.o xbmc/cores/dvdplayer/*.o xbmc/cores/dvdplayer/DVDSubtitles/*.o xbmc/cores/dvdplayer/DVDInputStreams/*.o xbmc/cores/dvdplayer/DVDCodecs/*.o xbmc/cores/dvdplayer/DVDCodecs/Audio/*.o xbmc/cores/dvdplayer/DVDCodecs/Video/*.o xbmc/cores/dvdplayer/DVDCodecs/Overlay/*.o xbmc/cores/dvdplayer/DVDDemuxers/*.o xbmc/cores/dvdplayer/DVDCodecs/Overlay/libspucc/*.o xbmc/cores/VideoRenderers/*.o) xbmc/lib/libXBMS/libxbms-i486-linux.a xbmc/lib/libUPnP/libupnp-i486-linux.a 
	g++-4.1 -o XboxMediaCenter xbmc/*.o xbmc/settings/*.o guilib/*.o guilib/tinyXML/*.o guilib/common/*.o xbmc/FileSystem/*.o xbmc/FileSystem/VideoDatabaseDirectory/*.o xbmc/FileSystem/MusicDatabaseDirectory/*.o xbmc/visualizations/*.o xbmc/cores/*.o xbmc/cores/paplayer/*.o xbmc/linux/*.o xbmc/lib/sqlLite/*.o xbmc/lib/libscrobbler/*.o xbmc/lib/libPython/*.o xbmc/lib/libPython/xbmcmodule/*.o xbmc/xbox/*.o xbmc/cores/DllLoader/*.o xbmc/cores/DllLoader/exports/*.o xbmc/cores/DllLoader/exports/util/*.o xbmc/utils/*.o xbmc/lib/UnrarXLib/*.o xbmc/lib/libGoAhead/*.o xbmc/cores/dvdplayer/*.o xbmc/cores/dvdplayer/DVDSubtitles/*.o xbmc/cores/dvdplayer/DVDInputStreams/*.o xbmc/cores/dvdplayer/DVDCodecs/*.o xbmc/cores/dvdplayer/DVDCodecs/Audio/*.o xbmc/cores/dvdplayer/DVDCodecs/Video/*.o xbmc/cores/dvdplayer/DVDCodecs/Overlay/*.o xbmc/cores/dvdplayer/DVDDemuxers/*.o xbmc/cores/dvdplayer/DVDCodecs/Overlay/libspucc/*.o xbmc/cores/VideoRenderers/*.o xbmc/lib/libGoAhead/libGoAhead-i486-linux.a xbmc/lib/libXBMS/libxbms-i486-linux.a xbmc/lib/libUPnP/libupnp-i486-linux.a xbmc/lib/libshout/libshout-i486-linux.a -lSDL_image -lSDL_gfx -lSDL_mixer -lSDL -llzo -lfreetype -lcdio -lsqlite3 -lfribidi -lGL -lGLU -lGLEW -lsmbclient -ldl -rdynamic

include Makefile.include
