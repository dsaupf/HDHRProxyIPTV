# HDHRProxyIPTV (Enhanced and Fixed)

HDHRProxyIPTV is an application for Windows that simulates the behavior of the server HDHomeRun© device
in relation to clients, that will act as proxy to receive the flow of channels from different sources to
a real tuner, as they could be multicast IPTV sources. The access protocol that has been implemented to 
receive these flows is the HTTP protocol. These flows are sent on demand to customers in the protocol 
they expect. You can consider this tool as one HDHomeRun Emulator.


Last release: 1.0.3n (2019-03-29)
https://github.com/dsaupf/HDHRProxyIPTV/raw/master/Release/HDHRProxyIPTV.exe

Notes:

* The HDHRProxyIPTV project uses the original library of HDHomeRun (`libhdhomerun_.lib`), 
  (http://www.silicondust.com/support/downloads/linux/ -> get libhdhomerun source), 
  specifically the version 20100213.
  (Current versions include the library headers and the .lib binary inside the "\HDHRProxyIPTV\libs" subdirectory)

* The project can be compiled with Visual Studio 2013, 2015 & 2017 Community Edition.

* You can found one precompiled Windows binary inside the "\Release" subdirectory.

* Edit the file "HDHRProxyIPTV_MappingList.ini" for your environment.

* As input only the HTTP protocol is supported at time (UDP not enabled). 

* You can execute this program in a Linux environment using Wine (even as daemon using Xpra).

* Inputs can be _any_ Transport Stream, however they need to be compatible with DTV standards.

* Tested programs to feed this proxy:
  - minisatip: http://github.com/catalinii/minisatip (_recommended_)
  - udpxy: http://sourceforge.net/projects/udpxy/
  - UDP-to-HTTP proxy (Windows): http://borpas.info/utils
  - TVHeadEnd: http://tvheadend.org/
