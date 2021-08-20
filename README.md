## zseal
**open source client for FICS Timeseal v2 for Linux/Unix systems**

:copyright:  2021-2016 Felipe Bergo (Pulga @ FICS), fbergo at gmail_dot_com
https://github.com/fbergo/zseal

Timeseal is a protocol for network lag reporting and compensation used by the
[FICS chess server](https://www.freechess.org/). Version 2 of the protocol
provides better delay stats as the server actively pings the client
periodically. FICS's FTP repository was taken down a few years 
ago in one of the server host changes or hard disk crashes (I forget which)
and was never put back on. Since then, there has been nowhere to download a
trustable timeseal client for Linux systems.

The protocol was deduced from Marcello Mamino's openseal (an implementation of
timeseal v1) and the [java timeseal v2 implementation in Raptor](https://github.com/Raptor-Fics-Interface/Raptor) by cday - 

[zseal](https://github.com/fbergo/zseal) is released under the [MIT](mit.md)
license: you can use this software as you please; the authors are not liable
for anything the software does to you or your stuff; derived works must retain the copyright notice

#### To compile:

run  `./configure`  (to generate config.h), then run `make` to compile.

The zseal binary will be generated in the current directory

#### Using/Installing:

To run from the command line:

  `zseal [host [port]]`

  * If host is omitted, the default "freechess.org" is used
  * If host is given but port is omitted, the default 5000 is used

  `zseal -v` or `zseal --version` will print the version and exit.

#### To use with eboard:

   local installation for a single user:
   `cp zseal ~/.eboard/timeseal.Linux`
   
   On non-Linux systems, eboard expects the executable to be called `timeseal.SYSTEM`,
   SYSTEM can be obtained with the command `uname -s`

   for a system-wide installation, copy zseal to `/usr/share/eboard/timeseal.SYSTEM` or any
   of the other directories eboard searches.
 see: http://www.bergo.eng.br/eboard/doc.php?d=3  

### To use with xboard:

  `xboard -ics -icshost freechess.org -icsport 5000 -icshelper ./zseal`

  (replace `./zseal` with wherever you want to leave `zseal` in)
