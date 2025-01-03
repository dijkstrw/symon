symon
=====

**symon** is a lightweight system monitor that measures cpu, load,
filesystem, interface, disk, memory, pf, pf queues, mbuf, proc and
sensor statistics every 5 seconds. This information is then spooled to
**symux(8)** for further processing.

**symon** has been designed to inflict minimal performance and security
impact on the system it monitors.  **symux(8)** has performance impact
proportional to the amount of streams it needs to manage. Ideally
symux should live on a different system and collect data from several
**symon** instances in a LAN.

By default, **symon** will drop privileges and **chroot(2)** to home of
the `symon` user.  This behaviour is not strictly needed for the cpu,
mem, mbuf, disk debug and interface probes as these will work even
when **symon** is started as `nobody`.

**symon** has been ported to FreeBSD, NetBSD, OpenBSD and Linux.

Rationale
---------
There are many system monitoring tools; what is unique to **symon**:
- lightweight, aim is to get the data at minimum cpu cost
- measure every 5 seconds, this high measurement speed makes
  e.g. network burst throttling visible
- can be used to feed real-time dashboards that show health of all
  hosts in a deployment

Suite
-----
Currently the *suite* consists of these parts:
- **symon** - lightweight system monitor. Can be run with privleges
  equivalent to nobody on the monitored host. Offers no functionality
  but monitoring and forwarding of measured data.
- **symux** - persists data. Incoming **symon** streams are stored on
  disk in `rrd` files. **symux** offers systems statistics as they
  come in to 3rd party clients.
- **SymuxClient.pm** - generic perl **symux** client. Could, for
  instance, be used to get the hourly amount of data that was
  transmitted on a particular interface.
- **syweb** - draws `rrdtool` pictures of the stored data. **syweb**
  is a php script that can deal with chrooted apaches. It can show all
  systems that are monitored in one go, or be configured to only show
  a set of graphs. *(separate package)*
- **sylcd** - **symux** client that drives CrystalFontz and HD44780
  lcds. **sylcd** shows current network load on a specific
  host. *(separate package)*

Building & Installing
=====================

Privileges
----------
**symux** needs read and write access to its `rrdfiles`.

**symon** needs to interface with your kernel. Depending on your host system this
leads to different privilege requirements:

OpenBSD:
- no privs: cpu, debug, df, if, io, mbuf, mem, proc, sensor
- rw on `/dev/pf` for pf

NetBSD:
- no privs: cpu, debug, df, if, io, mbuf, proc
- r on `/dev/sysmon` for sensor

FreeBSD:
- no privs: all
- non-chroot on FreeBSD 5.x for CPU ticks in proc
- rw on `/dev/pf` for pf and pfq

Linux:
- r on `/proc/net/dev`: if
- r on `/proc/stat`: cpu, cpuiow
- r on `/proc/meminfo`: mem

all:
- r on `chroot/etc/localtime` for proper timezone logging

Real quick on OpenBSD
---------------------
Install binary packages using:

    pkg_add symon   # on all monitored hosts
    pkg_add symux   # on the loghost
    pkg_add syweb   # also on the loghost

Or build from source:

    /usr/ports/net/rrdtool && make install) &&
    	make &&
    	make install &&
    	vi /etc/symux.conf /etc/symon.conf &&
    	~symon/symux/c_smrrds.sh all &&
    	/usr/local/libexec/symux &&
    	useradd -d /var/empty -L daemon -c 'symon Account' -s /sbin/nologin _symon
    	/usr/local/libexec/symon

Note that the **syweb** package to show the data stored in the `rrd`
files is a separate package.

Less quick, but all OSes
========================
- Install `rrdtool` on the host that will also run your **symux** gatherer.
  BSDs: `cd /usr/ports/net/rrdtool && make install`

- Check `Makefile.inc` for settings. Things to watch out for are:

     + `PREFIX` = Where does the installation tree start. Defaults to
                `/usr/local`.

     + `BINDIR` = Where should the daemons be put, relative to
                `$PREFIX`. Defaults to `libexec`.

     + `MANDIR` = Where should the manuals be installed, relative to
                `$PREFIX`. Defaults to `man`.

     + `SHRDIR` = Where are the example configurations to be
                installed. Defaults to `share/symon`.

     + `RRDDIR` = `$RRDDIR/include` should yield `rrd.h`. Define
                `SYMON_ONLY` in the environment or on the make command
                line to render this mute.

     + `INSTALLUSER` / `GROUPDIR` / `GROUPFILE` = user and groups that
                install should use.

     + you can define `SYMON_ONLY` if you do not want to compile **symux** / do not
       have `rrdtool` installed.

     + `symon/platform/os/Makefile.inc` is read before `Makefile.inc`; define
       your vars in the environment, or in `Makefile.inc` with `!=` to force
       overwriting the defaults.

BSDs: Run `make && make install`

Linux: Run `pmake && pmake install || bmake && bmake install`

- Create an `/etc/symon.conf` for each monitored host and one `symux.conf` for
  the gatherer host. See the manual pages on how to specify alternative
  configuration file locations, if necessary. Note that there are example
  configurations for both in `$PREFIX/$SHRDIR`.

- Create the `rrd` files where the incoming **symon** data is to be
  stored. `$PREFIX/$SHRDIR/c_smrrds.sh` and `symux -l` are your friends. Note that
  **syweb** expects an `.../machine/*.rrd` style directory structure somewhere
  under `/var/www`.

- Ensure that `/etc/localtime` is accessible by
  **symon**/**symux**. Failing this you will get log messages in
  GMT. Note that `etc` is `chroot/etc` when **symon** chroots.

- Both **symon** and **symux** will daemonize if started normally. Start them with
  debugging on initially to iron out any configuration problems:

      $PREFIX/$BINDIR/symux -d &
      $PREFIX/$BINDIR/symon -d

- Remove `-d` flags and check system logs for any failures.

- Only if you need the web interface: download and install **syweb**.

Getting measurements without the web
------------------------------------
The client directory contains a perl module **SymuxClient.pm** that can be used
to read measurements as they come in at the symux host. A sample Perl program
called `getsymonitem.pl` shows how to use the module.

Example:

    nexus$ cat /var/run/symux.fifo | getsymonitem.pl 127.0.0.1 2100 127.0.0.1 "cpu(0)" "idle"
    93.40

Historical data can be gathered using `rrdfetch(1)` from symux's `rrd`
files.

Portability
===========
This package was originally built as an OpenBSD application. It now
has support for FreeBSD, NetBSD and Linux.

My mental rules for extensions
------------------------------
- Cpu cost matters more than storage.

- Structures are not overloaded in functionality even if that would be possible
  memberwise. Overloading = code can be understood in more than one way and
  that leads to more maintenance and bugs.

- Defined values are not overloaded in functionality if possible. Same reason
  as above.

- Error messages carry `__FILE__`, `__LINE__` if they are failed assertions and
  plain English if they can be attributed to the end-user.

- Marshalling is hard. Alignment is carried out by bcopy.

- Support improvements of kernel structures by adding a new stream
  type. Goal is to support both the old and the new, so that a
  deployment can slowly move towards newer installations without
  losing metrics.

    + add a new stream id with a postfix (i.e. `IO2`)
    + make old stream accessable via lex with previous postfix (i.e. `io1`)
