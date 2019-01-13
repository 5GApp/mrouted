Simple Multicast Routing for UNIX
=================================
[![License Badge][]][License] [![Travis Status][]][Travis] [![Coverity Status][]][Coverity Scan]

Table of Contents
-----------------

* [Introduction](#introduction)
* [Build & Install](#build--install)
* [Building from GIT](#building-from-git)
* [Running](#running)
* [Configuration](#configuration)
* [Contributing](#contributing)
* [Origin & References](#origin--references)


Introduction
------------

mrouted is a [3-clause BSD][BSD License] licensed implementation of the
DVMRP multicast routing protocol.  It can run on any UNIX based system,
from embedded Linux systems to workstations, turning them into multicast
routers with (built-in) [IP-in-IP] tunneling support.  GRE can of course
also be used.

DVMRP is a distance vector based protocol, derived from RIP, suitable
for closely located multicast users in smaller networks.  It uses the
"flood and prune" method, where multicast is flooded until neighboring
routers opt out from unwanted multicast groups.  For a more thorough
explanation of the protocol see [RFC 1075][].

This version of mrouted has [RSRR][] support for running RSVP.

mrouted is primarily developed on Linux and should work as-is out of the
box on all major distributions.  Other UNIX variants should also work,
but are not as thoroughly tested.


Build & Install
---------------

The configure script and Makefile supports de facto standard settings
and environment variables such as `--prefix=PATH` and `DESTDIR=` for the
install process.  E.g., to install pimd to `/usr` instead of the default
`/usr/local`, but redirect install to a package directory in `/tmp`:

    ./configure --prefix=/usr --sysconfdir=/etc --localstatedir=/var --enable-rsrr
    make
    make DESTDIR=/tmp/mrouted-4.0-1 install-strip


Building from GIT
-----------------

If you want to contribute, or simply just try out the latest but
unreleased features, then you need to know a few things about the
[GNU build system][buildsystem]:

- `configure.ac` and a per-directory `Makefile.am` are key files
- `configure` and `Makefile.in` are generated from `autogen.sh`
- `Makefile` is generated by `configure` script

To build from GIT you first need to clone the repository and run the
`autogen.sh` script.  This requires `automake` and `autoconf` to be
installed on your system.

    git clone https://github.com/troglobit/mrouted.git
    cd mrouted/
    ./autogen.sh
    ./configure && make

GIT sources are a moving target and are not recommended for production
systems, unless you know what you are doing!


Running
-------

mrouted must run as root.

For the native mrouted tunnel to work in Linux based systems, you need
to have the "ipip" kernel module loaded or as built-in:

    modprobe ipip

Alternatively, you may of course also set up GRE tunnels between your
multicast capable routers.


Configuration
-------------

mrouted reads its configuration file from `/etc/mrouted.conf`.  You can
override the default by specifying an alternate file when invoking
mrouted:

    mrouted -c /path/file.conf

mrouted can be reconfigured at runtime like any regular UNIX daemon,
simply send it a `SIGHUP` to activate changes to the configuration file.
The PID is saved automatically to the file `/var/run/mrouted.pid` for
your scripting needs.

By default, mrouted configures itself to act as a multicast router on
all multicast capable interfaces, excluding loopback.  Therefore, you do
not need to explicitly configure mrouted, unless you need to setup
tunnel links, change the default operating parameters, disable multicast
routing over a specific physical interfaces, or have dynamic interfaces.

For more help, see the man page.


Contributing
------------

The basic functionality has been tested thoroughly over the years, but
that does not mean mrouted is bug free.  Please report bugs, feature
requests, patches and pull requests at [GitHub][].


Origin & References
-------------------

The mrouted routing daemon was developed by David Waitzman, Craig
Partridge, Steve Deering, Ajit Thyagarajan, Bill Fenner, David Thaler
and Daniel Zappala.  With contributions by many others.

The last release by Mr. Fenner was 3.9-beta3 on April 26 1999 and
mrouted has been in "beta" status since then.  Several prominent UNIX
operating systems, such as AIX, Solaris, HP-UX, BSD/OS, NetBSD, FreeBSD,
OpenBSD as well as most GNU/Linux based distributions have used that
beta as a de facto stable release, with (mostly) minor patches for
system adaptations.  Over time however many dropped support, but Debian
and OpenBSD kept it under their wings.

In March 2003 [OpenBSD](http://www.openbsd.org/), led by the fearless
Theo de Raadt, managed to convince Stanford to release mrouted under a
[fully free license][License], the [3-clause BSD license][BSD License].
Unfortunately, and despite the license issue being corrected by OpenBSD,
in February 2005 [Debian dropped mrouted][1] as an "obsolete protocol".

For a long time the OpenBSD team remained the sole guardian of this
project.  In 2010 [Joachim Nilsson](http://troglobit.com) revived
mrouted on [GitHub][].  The 3.9.x stable series represent the first
releases in over a decade.  Patches from all over the Internet,
including OpenBSD, have been merged.

[1]:               http://bugs.debian.org/cgi-bin/bugreport.cgi?bug=288112
[License]:         http://www.openbsd.org/cgi-bin/cvsweb/src/usr.sbin/mrouted/LICENSE
[License Badge]:   https://img.shields.io/badge/License-BSD%203--Clause-blue.svg
[BSD License]:     http://en.wikipedia.org/wiki/BSD_licenses
[RFC 1075]:        http://tools.ietf.org/html/rfc1075
[IP-in-IP]:        https://en.wikipedia.org/wiki/IP_in_IP
[RSRR]:            docs/RSRR.md
[buildsystem]:     https://airs.com/ian/configure/
[GitHub]:          http://github.com/troglobit/mrouted/
[Travis]:          https://travis-ci.org/troglobit/mrouted
[Travis Status]:   https://travis-ci.org/troglobit/mrouted.png?branch=master
[Coverity Scan]:   https://scan.coverity.com/projects/3320
[Coverity Status]: https://scan.coverity.com/projects/3320/badge.svg
