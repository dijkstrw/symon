#!/bin/sh
# $Id: c_smrrds.sh,v 1.20 2003/10/10 15:20:03 dijkstra Exp $

#
# Copyright (c) 2001-2002 Willem Dijkstra
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

# symon datafiles "make" file. Valid arguments:
#       all      Makes all files for active interfaces and disks
#       mem      Make memory file
#       cpu?     Make cpu file
#       pf       Make pf file
#       mbuf     Make mbuf file

# --- user configuration starts here
INTERVAL=`grep SYMON_INTERVAL ../symon/symon.h 2>/dev/null | cut -f3 -d\ `
INTERVAL=${INTERVAL:-5}
RRD_ARGS="--step=$INTERVAL --start=0"
# --- user configuration ends here

# get arguments
for i
do
case $i in
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
    if [ X"$child" == "X" ]; then 
	echo "RRDs will only contain a single day of data"
    fi
    ;;
*)
    args="$args $i"
    ;;
esac
done

if [ X"$RRA_SETUP" == "X" ]; then
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
INTERFACES="an|awi|be|bge|bm|cnw|dc|de|ec|ef|eg|el|em|ep|ex|fea|fpa|fxp|gem|gm|gre|hme|ie|kue|lc|le|lge|lmc|lo|ne|ne|nge|ray|rl|qe|qec|sf|sis|sk|skc|sl|sm|siop|ste|stge|ti|tl|tr|tx|txp|vme|vr|wb|we|wi|wx|xe|xl"
VIRTUALINTERFACES="bridge|enc|faith|gif|ppp|sppp|strip|tun|vlan";
DISKS="sd|cd|ch|rd|raid|ss|uk|vnc|wd"

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

this=$0
if [ X"$1$2$3$4$5$6$7$8$9" == "X" ]; then
    sh $this help
    exit 1;
fi

DISKS=`addsuffix $DISKS [0-9]`
INTERFACES=`addsuffix $INTERFACES [0-9]`
VIRTUALINTERFACES=`addsuffix $VIRTUALINTERFACES \\.\\*`

for i in $args
do
# add if_*.rrd if it is an interface
if [ `echo $i | egrep -e "^($INTERFACES)$"` ]; then i=if_$i.rrd; fi
if [ `echo $i | egrep -e "^($VIRTUALINTERFACES)$"` ]; then i=if_$i.rrd; fi
# add io_*.rrd if it is a disk
if [ `echo $i | egrep -e "^($DISKS)$"` ]; then i=io_$i.rrd; fi
# add .rrd if it is a cpu, etc.
if [ `echo $i | egrep -e "^(cpu[0-9]$|mem$|pf$|mbuf$|debug$|proc_|sensor[0-9]$|sensor[0-9][0-9]$)"` ]; then i=$i.rrd; fi

if [ -f $i ]; then
    echo "$i exists - ignoring"
    i="done"
fi

case $i in

all)
    echo "Creating rrd files for {cpu0|mem|disks|interfaces|pf|mbuf}"
    sh $this child $config cpu0 mem
    sh $this child $config interfaces
    sh $this child $config disks
    sh $this child $config pf
    sh $this child $config mbuf
    ;;

if|interfaces)
    # obtain all network cards
    sh $this child $config `ifconfig -a| egrep -e "^($INTERFACES):" | cut -f1 -d\:  | sort -u`
    ;;

io|disks)
    # obtain all disks
    sh $this child $config `df | grep dev | sed 's/^\/dev\/\(.*\)[a-z] .*$/\1/' | sort -u`
    ;;

cpu[0-9].rrd)
    # Build cpu file
    rrdtool create $i $RRD_ARGS \
	DS:user:GAUGE:5:0:100 \
	DS:nice:GAUGE:5:0:100 \
	DS:system:GAUGE:5:0:100 \
	DS:interrupt:GAUGE:5:0:100 \
	DS:idle:GAUGE:5:0:100 \
	$RRA_SETUP
    echo "$i created"
    ;;

sensor*.rrd)
    # Build sensor file
    rrdtool create $i $RRD_ARGS \
	DS:value:GAUGE:5:-U:U \
	$RRA_SETUP
    echo "$i created"
    ;;

mem.rrd)
    # Build memory file
    rrdtool create $i $RRD_ARGS \
	DS:real_active:GAUGE:5:0:U \
	DS:real_total:GAUGE:5:0:U \
	DS:free:GAUGE:5:0:U \
	DS:swap_used:GAUGE:5:0:U \
	DS:swap_total:GAUGE:5:0:U \
	$RRA_SETUP
    echo "$i created"
    ;;

if_*.rrd)
    # Build interface files
    rrdtool create $i $RRD_ARGS \
	DS:ipackets:COUNTER:5:U:U DS:opackets:COUNTER:5:U:U \
	DS:ibytes:COUNTER:5:U:U DS:obytes:COUNTER:5:U:U \
	DS:imcasts:COUNTER:5:U:U DS:omcasts:COUNTER:5:U:U \
	DS:ierrors:COUNTER:5:U:U DS:oerrors:COUNTER:5:U:U \
	DS:collisions:COUNTER:5:U:U DS:drops:COUNTER:5:U:U \
	$RRA_SETUP
    echo "$i created"
    ;;

debug.rrd)
    # Build debug file
    rrdtool create $i $RRD_ARGS \
	DS:debug0:GAUGE:5:U:U DS:debug1:GAUGE:5:U:U \
	DS:debug2:GAUGE:5:U:U DS:debug3:GAUGE:5:U:U \
	DS:debug4:GAUGE:5:U:U DS:debug5:GAUGE:5:U:U \
	DS:debug6:GAUGE:5:U:U DS:debug7:GAUGE:5:U:U \
	DS:debug8:GAUGE:5:U:U DS:debug9:GAUGE:5:U:U \
	DS:debug10:GAUGE:5:U:U DS:debug11:GAUGE:5:U:U \
	DS:debug12:GAUGE:5:U:U DS:debug13:GAUGE:5:U:U \
	DS:debug14:GAUGE:5:U:U DS:debug15:GAUGE:5:U:U \
	DS:debug16:GAUGE:5:U:U DS:debug17:GAUGE:5:U:U \
	DS:debug18:GAUGE:5:U:U DS:debug19:GAUGE:5:U:U \
	$RRA_SETUP
    echo "$i created"
    ;;
proc_*.rrd)
    # Build proc file
    rrdtool create $i $RRD_ARGS \
	DS:number:GAUGE:5:0:U DS:uticks:GAUGE:5:0:U \
	DS:sticks:GAUGE:5:0:U DS:iticks:GAUGE:5:0:U \
	DS:cpusec:GAUGE:5:0:U DS:cpupct:GAUGE:5:0:100 \
        DS:procsz:GAUGE:5:0:U DS:rsssz:GAUGE:5:0:U \
	$RRA_SETUP
    echo "$i created"
    ;;
    
pf.rrd)
    # Build pf file
    rrdtool create $i $RRD_ARGS \
	DS:bytes_v4_in:DERIVE:5:0:U DS:bytes_v4_out:DERIVE:5:0:U \
	DS:bytes_v6_in:DERIVE:5:0:U DS:bytes_v6_out:DERIVE:5:0:U \
	DS:packets_v4_in_pass:DERIVE:5:0:U DS:packets_v4_in_drop:DERIVE:5:0:U \
	DS:packets_v4_out_pass:DERIVE:5:0:U DS:packets_v4_out_drop:DERIVE:5:0:U \
	DS:packets_v6_in_pass:DERIVE:5:0:U DS:packets_v6_in_drop:DERIVE:5:0:U \
	DS:packets_v6_out_pass:DERIVE:5:0:U DS:packets_v6_out_drop:DERIVE:5:0:U \
	DS:states_entries:ABSOLUTE:5:0:U \
	DS:states_searches:DERIVE:5:0:U \
	DS:states_inserts:DERIVE:5:0:U \
	DS:states_removals:DERIVE:5:0:U \
	DS:counters_match:DERIVE:5:0:U \
	DS:counters_badoffset:DERIVE:5:0:U \
	DS:counters_fragment:DERIVE:5:0:U \
	DS:counters_short:DERIVE:5:0:U \
	DS:counters_normalize:DERIVE:5:0:U \
	DS:counters_memory:DERIVE:5:0:U \
	$RRA_SETUP
    echo "$i created"
    ;;

mbuf.rrd)
    # Build mbuf file
    rrdtool create $i $RRD_ARGS \
	DS:totmbufs:GAUGE:5:0:U DS:mt_data:GAUGE:5:0:U \
	DS:mt_oobdata:GAUGE:5:0:U DS:mt_control:GAUGE:5:0:U \
	DS:mt_header:GAUGE:5:0:U DS:mt_ftable:GAUGE:5:0:U \
	DS:mt_soname:GAUGE:5:0:U DS:mt_soopts:GAUGE:5:0:U \
	DS:pgused:GAUGE:5:0:U DS:pgtotal:GAUGE:5:0:U \
	DS:totmem:GAUGE:5:0:U DS:totpct:GAUGE:5:0:100 \
	DS:m_drops:COUNTER:5:0:U DS:m_wait:COUNTER:5:0:U \
	DS:m_drain:COUNTER:5:0:U \
	$RRA_SETUP
    echo "$i created"
    ;;

io_*.rrd)
    # Build disk files
    rrdtool create $i $RRD_ARGS \
	DS:transfers:COUNTER:5:U:U \
	DS:seeks:COUNTER:5:U:U \
	DS:bytes:COUNTER:5:U:U \
	$RRA_SETUP
    echo "$i created"
    ;;

"done")
    # ignore
    ;;
*)
    # Default match
    echo $i - unknown
    cat <<EOF
Usage: $0 [oneday] all
       $0 [oneday] cpu0|mem|pf|mbuf|debug|proc|<if>|<io>|sensor[0-25]

Where:
if=	`echo $INTERFACES|
    awk 'BEGIN  {FS="|"}
		{for (i=1; i<=NF; i++) { 
		    printf("%s|",$i); 
		    if ((i%6)==0) {
			printf("%s","\n	")
		    }
		} 
		print " ";}'`
io=	`echo $DISKS|
	awk 'BEGIN  {FS="|"}
		{for (i=1; i<=NF; i++) { 
		    printf("%s|",$i); 
		    if ((i%6)==0) {
			printf("%s","\n	")
		    }
		} 
		print " ";}'`

EOF
    ;;
esac
done
