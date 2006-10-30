#!/bin/sh
# $Id: c_smrrds.sh,v 1.34 2006/10/30 08:40:07 dijkstra Exp $

#
# Copyright (c) 2001-2005 Willem Dijkstra
# All rights reserved.
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions
# are met:
#
#    - Redistributions of source code must retain the above copyright
#      notice, this list of conditions and the following disclaimer.
#    - Redistributions in binary form must reproduce the above
#      copyright notice, this list of conditions and the following
#      disclaimer in the documentation and/or other materials provided
#      with the distribution.
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
# "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
# LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
# FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
# COPYRIGHT HOLDERS OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
# INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
# BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
# LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
# CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
# LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
# ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
# POSSIBILITY OF SUCH DAMAGE.

# --- user configuration starts here
INTERVAL=${INTERVAL:-5}
# --- user configuration ends here
OS=`uname -s`
# addsuffix adds a suffix to each entry of a list (item|item|...)
addsuffix() {
    list=$1'|'
    suffix=$2
    while [ `echo $list | grep '|'` ]; do
	newlist=$newlist'|'`echo $list | cut -f1 -d\|`$suffix
	list=`echo $list | cut -f2- -d\|`
    done
    echo $newlist | cut -b2-
}

create_rrd() {
    file=$1
    shift
    rrdtool create $file $RRD_ARGS $* $RRA_SETUP
    if [ "$?" = "0" -a -f $file ]; then
	echo "$file created"
    else
	echo "could not create $file"
    fi
}

# get arguments
select_interval=""
for i
do
case $i in
interval)
    select_interval="yes"
    ;;
child)
    child=1
    ;;
oneday)
    config=$i
# today only RRA setup:
# - 1   day  of  5 second  samples = 17280 x 5 second samples
    RRA_SETUP=" RRA:AVERAGE:0.5:1:17280
	    RRA:MAX:0.5:1:17280
	    RRA:MIN:0.5:1:17280"
    if [ X"$child" = "X" ]; then
	echo "RRDs will only contain a single day of data"
    fi
    ;;
*)
    if [ -n "$select_interval" ]; then
	INTERVAL=$i
	select_interval=""
    else
	args="$args $i"
    fi
    ;;
esac
done

if [ X"$RRA_SETUP" = "X" ]; then
# default RRA setup:
# - 2   days of  5 second  samples = 34560 x 5 second samples
# - 14  days of 30 minutes samples = 672 x 360 x 5 second samples
# - 50  days of  2 hour    samples = 600 x 1440 x 5 second samples
# - 600 days of  1 day     samples = 600 x 17280 x 5 second samples
RRA_SETUP=" RRA:AVERAGE:0.5:1:34560
	    RRA:AVERAGE:0.5:360:672
	    RRA:AVERAGE:0.5:1440:600
	    RRA:AVERAGE:0.5:17280:600
	    RRA:MAX:0.5:1:34560
	    RRA:MAX:0.5:360:672
	    RRA:MAX:0.5:1440:600
	    RRA:MAX:0.5:17280:600
	    RRA:MIN:0.5:1:34560
	    RRA:MIN:0.5:360:672
	    RRA:MIN:0.5:1440:600
	    RRA:MIN:0.5:17280:600"
fi

# All interfaces and disks
case ${OS} in
FreeBSD)
	DISKS="ad|acd|afd|ast|sa|da|ar|cd|ch|md"
	INTERFACES="an|ar|ath|aue|awi|axe|bfe|bge|cm|cnw|cs|cue|dc|de|ed|el|em|ep|ex|fe|fwe|fxp|gem|gx|hme|ie|kue|lge|lnc|my|nge|pcn|ray|re|rl|rue|sf|sis|sk|sn|snc|ste|ti|tl|tx|txp|vge|vr|vx|wb|wi|xe|xl";
	VIRTUALINTERFACES="bridge|carp|enc|faith|gif|ppp|sl|sppp|strip|tun|vlan";
	diskcmd="mount | sed -n '/^\/dev/ s,/dev/\([a-z]*[0-9]\).*,\1,p' | sort -u"
	interfacecmd="ifconfig -l | sed 's/lo0//'"
	;;
Linux)
	DISKS="hda|hdb|hdc|hdd|sda|sdb|sdc|sdd"
	INTERFACES="eth"
	VIRTUALINTERFACES="sit"
	diskcmd="mount | sed -n '/^\/dev/ s,/dev/\([a-z]*[0-9]\).*,\1,p' | sort -u"
	interfacecmd="ifconfig -a| egrep -e \"^(\$INTERFACES) \" | cut -f1 -d\  | sort -u"
	;;
OpenBSD)
	DISKS="sd|cd|ch|rd|raid|ss|uk|vnc|wd"
	INTERFACES="an|ath|awi|axe|be|bge|bm|cnw|dc|de|ec|ef|eg|el|em|ep|ex|fea|fpa|fxp|gem|gm|gre|hme|ie|kue|lc|le|lge|lmc|lo|ne|nge|ray|rl|qe|sf|sis|sk|sl|sm|ste|stge|ti|tl|tr|tx|txp|vme|vr|wb|we|wi|wx|xe|xl"
	VIRTUALINTERFACES="bridge|carp|enc|faith|gif|ppp|sppp|strip|tun|vlan";
	diskcmd="mount | sed -n '/^\/dev/ s,/dev/\([a-z]*[0-9]\).*,\1,p' | sort -u"
	partcmd="grep ffs /etc/fstab | sed -n '/^\/dev/ s,/dev/\([a-z]*[0-9]*[a-z]*\).*,\1,p' | sort -u"
	interfacecmd="ifconfig -a| egrep -e \"^(\$INTERFACES):\" | cut -f1 -d\:  | sort -u"
	;;
NetBSD)
	DISKS="sd|cd|ch|rd|raid|ss|uk|vnc|wd"
	INTERFACES="ai|an|ate|ath|atw|awi|bce|bge|bicc|cnw|com|cs|depca|ec|ef|eg|el|elmc|en|ep|epic|esh|ex|fea|fmv|fpa|fxp|gsip|hme|ipw|iwi|ix|iy|lc|le|lmc|mbe|mhzc|mtd|ne|nele|ntwoc|pcn|ray|re|rtk|sf|sip|sk|skc|sm|ste|stge|ti|tl|tlp|tr|vge|vr|we|wi|wm|xi|xirc"
	VIRTUALINTERFACES="bridge|carp|enc|faith|gif|ppp|sppp|sl|strip|tun|vlan";
	diskcmd="mount | sed -n '/^\/dev/ s,/dev/\([a-z]*[0-9]\).*,\1,p' | sort -u"
	interfacecmd="ifconfig -l | sed 's/lo0//'"
	;;
esac
DISKS=`addsuffix $DISKS [0-9]`
INTERFACES=`addsuffix $INTERFACES [0-9]`
VIRTUALINTERFACES=`addsuffix $VIRTUALINTERFACES \\.\\*`

this=$0
if [ X"$1$2$3$4$5$6$7$8$9" = "X" ]; then
    cat <<EOF
Create rrd files for symux.

Usage: `basename $0` [oneday] [interval <seconds>] all | cpu0 | mem |
		   pf | pfq_<queue> | mbuf | debug | proc_<process> |
		   <if> | <io> | sensor[0-25]

Where:
oneday  = modify rrds to only contain one day of information
seconds = modify rrds for non standard monitoring interval
process = the name of a process as specified in sy{mon,mux}.conf
	  e.g. proc(httpd) -> proc_httpd
queue = the name of a queue as specified in sy{mon,mux}.conf
	  e.g. pfq(root) -> pfq_root

if=	`echo $INTERFACES|
   awk 'BEGIN  {FS="|"}
	   {for (i=1; i<=NF; i++) {
	       printf("%s|",$i);
	       if ((i%6)==0) {
		  printf("%s","\n\t")
	       }
	   }
	   print " ";}'`
io=	`echo $DISKS|
   awk 'BEGIN  {FS="|"}
		{for (i=1; i<=NF; i++) {
		    printf("%s|",$i);
		    if ((i%6)==0) {
			printf("%s","\n\t")
		    }
		}
		print " ";}'`

OpenBSD pre-3.5 disk statistics are available via the io1_<disk> argument.
EOF
    exit 1;
fi

for i in $args
do
# add if_*.rrd if it is an interface
if echo $i | egrep -qe "^($INTERFACES)$"; then i=if_$i.rrd; fi
if echo $i | egrep -qe "^($VIRTUALINTERFACES)$"; then i=if_$i.rrd; fi
# add io_*.rrd if it is a disk
if echo $i | egrep -qe "^($DISKS)$"; then i=io_$i.rrd; fi
# add io_*.rrd if it is a disk
if echo $i | egrep -qe "^($DISKS)[a-z]$"; then i=df_$i.rrd; fi
# add .rrd if it is a cpu, etc. don't duplicate the '.rrd' at the end of proc_, pfq_ and io1_
if echo $i | egrep -qe "^(cpu[0-9]$|mem$|pf$|pfq_|mbuf$|debug$|proc_|sensor[0-9]$|sensor[0-9][0-9]$|io1_)"; then i=${i%.rrd}.rrd; fi

if [ -f $i ]; then
    echo "$i exists - ignoring"
    i="done"
fi

RRD_ARGS="--step=$INTERVAL --start=0"

case $i in

all)
    echo "Creating rrd files for {cpu0|df|mem|disks|interfaces|pf|mbuf}"
    sh $this interval $INTERVAL child $config cpu0 mem
    sh $this interval $INTERVAL child $config interfaces
    sh $this interval $INTERVAL child $config disks
    sh $this interval $INTERVAL child $config pf
    sh $this interval $INTERVAL child $config mbuf
    sh $this interval $INTERVAL child $config df
    ;;

if|interfaces)
    # obtain all network cards
    sh $this child $config `eval $interfacecmd`
    ;;

io|disks)
    # obtain all disks
    sh $this child $config `eval $diskcmd`
    ;;

df)
    # obtain all ffs partitions
    sh $this child $config `eval $partcmd`
    ;;

cpu[0-9].rrd)
    # Build cpu file
    create_rrd $i \
	DS:user:GAUGE:$INTERVAL:0:100 \
	DS:nice:GAUGE:$INTERVAL:0:100 \
	DS:system:GAUGE:$INTERVAL:0:100 \
	DS:interrupt:GAUGE:$INTERVAL:0:100 \
	DS:idle:GAUGE:$INTERVAL:0:100
    ;;

df_*.rrd)
    # Build df file
    create_rrd $i \
	DS:blocks:GAUGE:$INTERVAL:0:U \
	DS:bfree:GAUGE:$INTERVAL:0:U \
	DS:bavail:GAUGE:$INTERVAL:0:U \
	DS:files:GAUGE:$INTERVAL:0:U \
	DS:ffree:GAUGE:$INTERVAL:0:U \
	DS:syncwrites:COUNTER:$INTERVAL:U:U \
	DS:asyncwrites:COUNTER:$INTERVAL:U:U
    ;;

sensor*.rrd)
    # Build sensor file
    create_rrd $i \
	DS:value:GAUGE:$INTERVAL:-U:U
    ;;

mem.rrd)
    # Build memory file
    create_rrd $i \
	DS:real_active:GAUGE:$INTERVAL:0:U \
	DS:real_total:GAUGE:$INTERVAL:0:U \
	DS:free:GAUGE:$INTERVAL:0:U \
	DS:swap_used:GAUGE:$INTERVAL:0:U \
	DS:swap_total:GAUGE:$INTERVAL:0:U
    ;;

if_*.rrd)
    # Build interface files
    create_rrd $i \
	DS:ipackets:COUNTER:$INTERVAL:U:U DS:opackets:COUNTER:$INTERVAL:U:U \
	DS:ibytes:COUNTER:$INTERVAL:U:U DS:obytes:COUNTER:$INTERVAL:U:U \
	DS:imcasts:COUNTER:$INTERVAL:U:U DS:omcasts:COUNTER:$INTERVAL:U:U \
	DS:ierrors:COUNTER:$INTERVAL:U:U DS:oerrors:COUNTER:$INTERVAL:U:U \
	DS:collisions:COUNTER:$INTERVAL:U:U DS:drops:COUNTER:$INTERVAL:U:U
    ;;

debug.rrd)
    # Build debug file
    create_rrd $i \
	DS:debug0:GAUGE:$INTERVAL:U:U DS:debug1:GAUGE:$INTERVAL:U:U \
	DS:debug2:GAUGE:$INTERVAL:U:U DS:debug3:GAUGE:$INTERVAL:U:U \
	DS:debug4:GAUGE:$INTERVAL:U:U DS:debug5:GAUGE:$INTERVAL:U:U \
	DS:debug6:GAUGE:$INTERVAL:U:U DS:debug7:GAUGE:$INTERVAL:U:U \
	DS:debug8:GAUGE:$INTERVAL:U:U DS:debug9:GAUGE:$INTERVAL:U:U \
	DS:debug10:GAUGE:$INTERVAL:U:U DS:debug11:GAUGE:$INTERVAL:U:U \
	DS:debug12:GAUGE:$INTERVAL:U:U DS:debug13:GAUGE:$INTERVAL:U:U \
	DS:debug14:GAUGE:$INTERVAL:U:U DS:debug15:GAUGE:$INTERVAL:U:U \
	DS:debug16:GAUGE:$INTERVAL:U:U DS:debug17:GAUGE:$INTERVAL:U:U \
	DS:debug18:GAUGE:$INTERVAL:U:U DS:debug19:GAUGE:$INTERVAL:U:U
    ;;
proc_*.rrd)
    # Build proc file
    create_rrd $i \
	DS:number:GAUGE:$INTERVAL:0:U DS:uticks:COUNTER:$INTERVAL:0:U \
	DS:sticks:COUNTER:$INTERVAL:0:U DS:iticks:COUNTER:$INTERVAL:0:U \
	DS:cpusec:GAUGE:$INTERVAL:0:U DS:cpupct:GAUGE:$INTERVAL:0:100 \
	DS:procsz:GAUGE:$INTERVAL:0:U DS:rsssz:GAUGE:$INTERVAL:0:U
    ;;

pf.rrd)
    # Build pf file
    create_rrd $i \
	DS:bytes_v4_in:DERIVE:$INTERVAL:0:U DS:bytes_v4_out:DERIVE:$INTERVAL:0:U \
	DS:bytes_v6_in:DERIVE:$INTERVAL:0:U DS:bytes_v6_out:DERIVE:$INTERVAL:0:U \
	DS:packets_v4_in_pass:DERIVE:$INTERVAL:0:U DS:packets_v4_in_drop:DERIVE:$INTERVAL:0:U \
	DS:packets_v4_out_pass:DERIVE:$INTERVAL:0:U DS:packets_v4_out_drop:DERIVE:$INTERVAL:0:U \
	DS:packets_v6_in_pass:DERIVE:$INTERVAL:0:U DS:packets_v6_in_drop:DERIVE:$INTERVAL:0:U \
	DS:packets_v6_out_pass:DERIVE:$INTERVAL:0:U DS:packets_v6_out_drop:DERIVE:$INTERVAL:0:U \
	DS:states_entries:GAUGE:$INTERVAL:0:U \
	DS:states_searches:DERIVE:$INTERVAL:0:U \
	DS:states_inserts:DERIVE:$INTERVAL:0:U \
	DS:states_removals:DERIVE:$INTERVAL:0:U \
	DS:counters_match:DERIVE:$INTERVAL:0:U \
	DS:counters_badoffset:DERIVE:$INTERVAL:0:U \
	DS:counters_fragment:DERIVE:$INTERVAL:0:U \
	DS:counters_short:DERIVE:$INTERVAL:0:U \
	DS:counters_normalize:DERIVE:$INTERVAL:0:U \
	DS:counters_memory:DERIVE:$INTERVAL:0:U
    ;;

pfq_*.rrd)
	# Build pfq file
	create_rrd $i \
	    DS:sent_bytes:COUNTER:$INTERVAL:0:U \
	    DS:sent_packets:COUNTER:$INTERVAL:0:U \
	    DS:drop_bytes:COUNTER:$INTERVAL:0:U \
	    DS:drop_packets:COUNTER:$INTERVAL:0:U
	;;

mbuf.rrd)
    # Build mbuf file
    create_rrd $i \
	DS:totmbufs:GAUGE:$INTERVAL:0:U DS:mt_data:GAUGE:$INTERVAL:0:U \
	DS:mt_oobdata:GAUGE:$INTERVAL:0:U DS:mt_control:GAUGE:$INTERVAL:0:U \
	DS:mt_header:GAUGE:$INTERVAL:0:U DS:mt_ftable:GAUGE:$INTERVAL:0:U \
	DS:mt_soname:GAUGE:$INTERVAL:0:U DS:mt_soopts:GAUGE:$INTERVAL:0:U \
	DS:pgused:GAUGE:$INTERVAL:0:U DS:pgtotal:GAUGE:$INTERVAL:0:U \
	DS:totmem:GAUGE:$INTERVAL:0:U DS:totpct:GAUGE:$INTERVAL:0:100 \
	DS:m_drops:COUNTER:$INTERVAL:0:U DS:m_wait:COUNTER:$INTERVAL:0:U \
	DS:m_drain:COUNTER:$INTERVAL:0:U
    ;;

io_*.rrd)
    # Build disk files
    create_rrd $i \
	DS:rxfer:COUNTER:$INTERVAL:U:U \
	DS:wxfer:COUNTER:$INTERVAL:U:U \
	DS:seeks:COUNTER:$INTERVAL:U:U \
	DS:rbytes:COUNTER:$INTERVAL:U:U \
	DS:wbytes:COUNTER:$INTERVAL:U:U
    ;;

io1_*.rrd)
    # Build disk files
    create_rrd $i \
	DS:transfers:COUNTER:$INTERVAL:U:U \
	DS:seeks:COUNTER:$INTERVAL:U:U \
	DS:bytes:COUNTER:$INTERVAL:U:U
    ;;

"done")
    # ignore
    ;;
*)
    # Default match
    echo $i - unknown
    ;;
esac
done
