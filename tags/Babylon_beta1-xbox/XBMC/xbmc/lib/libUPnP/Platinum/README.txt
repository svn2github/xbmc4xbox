Platinum UPnP SDK
=================

This toolkit consists of 2 modules:
* Neptune : a C++ Runtime Library
* Platinum: a modular UPnP Framework [Platinum uses Neptune]

Unless you intend to use Neptune independently from Platinum, it is recommended that you build binaries directly from the "Build" tree of Platinum. All the dependent binaries will be rebuilt automatically (including Neptune).

---------------------------------------------
BUILDING SDK & SAMPLE APPLICATIONS

* Windows:
Open the Visual Studio 2005 solution located @ Build\Targets\x86-microsoft-win32-vs2005\Platinum.sln
Open the Visual Studio 2008 solution located @ Build\Targets\x86-microsoft-win32-vs2008\Platinum.sln

* XBox:
Open the Visual Studio .NET 2003 solution located @ Build\Targets\x86-microsoft-xbox-vs2003\Platinum.sln

* MacOSX:
Open the XCode project file located @ Build\Targets\universal-apple-macosx\PlatinumApps.xcodeproj

* Linux, Cygwin, MacOSX
Open a shell, go to the Platinum root directory and type 'scons' (http://scons.org). 
The output of the scons build will be found under Platinum/Build/Targets/{TARGET}/{Debug|Release}. 
Additionally, the output is copied under Platinum/Targets/{TARGET}/{Debug|Release} for convenience.

---------------------------------------------
RUNNING SAMPLE APPLICATIONS

* FileMediaServerTest
This is an example of a MediaServer. Given a path, it lets a ControlPoint browse the content of the directory and its sub-directories. Additionally, files can be streamed.

usage: FileMediaServerTest [-f <friendly_name>] <path>
    -f : optional upnp server friendly name
    <path> : local path to serve

Once started, type 'q' to quit.

* MediaRendererTest
This is an example shell of a MediaRenderer. It is to be contolled by a UPnP ControlPoint. This is just a SHELL, this won't play anything. You need to hook it up yourself for now.

usage: MediaRendererTest [-f <friendly_name>]
    -f : optional upnp server friendly name

Once started, type 'q' to quit.

* MediaCrawler
This is a combo MediaServer + ControlPoint. It browses content from other MediaServers it finds on the network and present them under one single aggregated view. This is useful for some devices that need to select one single MediaServer at boot time (i.e. Roku). Also since content is streamed through the crawler, it could be transcoded or even transcrypted.

Once started, type 'q' to quit.

* MicroMediaController
This is a ControlPoint (synchronous) that lets you browse a MediaServer using a shell-like interface. Once started, a command prompt lets you enter commands such as:
     quit    -   shutdown
     exit    -   same as quit
     setms   -   select a media server to become the active media server
     getms   -   print the friendly name of the active media server
     ls      -   list the contents of the current directory on the active 
                 media server
     cd      -   traverse down one level in the content tree on the active
                 media server
     cdup    -   traverse up one level in the content tree on the active
                 media server
     pwd     -   print the path from the root to your current position in the 
                 content tree on the active media server
                 
Experimental MediaRenderer commands (not yet full implemented):
     setmr   - * select a media renderer to become the active media renderer
     getmr   -   print the friendly name of the active media renderer
     open    -   set the uri on the active media renderer
     play    -   play the active uri on the active media renderer
     stop    -   stop the active uri on the active media renderer