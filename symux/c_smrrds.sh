#!/bin/sh
# $Id: c_smrrds.sh,v 1.6 2002/07/25 09:51:44 dijkstra Exp $

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

# mon datafiles "make" file. Valid arguments:
#       all      Makes all files for active interfaces and disks
#       mem      Make memory file
#       cpu?     Make cpu file

# --- user configuration starts here
INTERVAL=`grep MON_INTERVAL ../mon/mon.h 2>/dev/null | cut -f3 -d\ `
INTERVAL=${INTERVAL:-5}
# RRA setup:
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
# --- user configuration ends here

# All interfaces and disks
INTERFACES="an|awi|bge|bridge|cnw|dc|de|ec|ef|eg|el|enc|ep|ex|faith|fea|fpa|fxp|gif|gre|ie|lc|le|le|lge|lmc|lo|ne|ne|nge|ray|rl|pflog|ppp|sf|sis|sk|skc|sl|sm|sppp|ste|stge|strip|ti|tl|tr|tun|tx|txp|vlan|vr|wb|we|wi|wx|xe|xl"
DISKS="sd|st|cd|ch|rd|raid|ss|uk|vnc|wd"

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
if [ X"$1" == "X" ]; then
    sh $this help
    exit 1;
fi

DISKS=`addsuffix $DISKS [0-9]`
INTERFACES=`addsuffix $INTERFACES [0-9]`

for i
do
# add if_*.rrd if it is an interface
if [ `echo $i | egrep -e "^($INTERFACES)$"` ]; then i=if_$i.rrd; fi
# add io_*.rrd if it is a disk
if [ `echo $i | egrep -e "^($DISKS)$"` ]; then i=io_$i.rrd; fi
# add .rrd if it is a cpu or mem
if [ `echo $i | egrep -e "^(cpu[0-9]|mem)$"` ]; then i=$i.rrd; fi

if [ -f $i ]; then
    echo "$i exists - ignoring"
    i="done"
fi

case $i in

all)
    echo "Creating rrd files for {cpu|mem|disks|interfaces}"
    sh $this cpu0 mem
    sh $this interfaces
    sh $this disks
    ;;

if|interfaces)
    # obtain all network cards
    sh $this `ifconfig -a| egrep -e "^($INTERFACES):" | cut -f1 -d\:  | sort -u`
    ;;

io|disks)
    # obtain all disks
    sh $this `df | grep dev | sed 's/^\/dev\/\(.*\)[a-z] .*$/\1/' | sort -u`
    ;;

cpu[0-9].rrd)
    # Build cpu file
    rrdtool create $i --step=$INTERVAL \
	DS:user:GAUGE:5:0:100 \
	DS:nice:GAUGE:5:0:100 \
	DS:system:GAUGE:5:0:100 \
	DS:interrupt:GAUGE:5:0:100 \
	DS:idle:GAUGE:5:0:100 \
	$RRA_SETUP
    echo "$i created"
    ;;

mem.rrd)
    # Build memory file
    rrdtool create $i --step=$INTERVAL \
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
    rrdtool create $i --step=$INTERVAL \
	DS:ipackets:COUNTER:5:U:U DS:opackets:COUNTER:5:U:U \
	DS:ibytes:COUNTER:5:U:U DS:obytes:COUNTER:5:U:U \
	DS:imcasts:COUNTER:5:U:U DS:omcasts:COUNTER:5:U:U \
	DS:ierrors:COUNTER:5:U:U DS:oerrors:COUNTER:5:U:U \
	DS:collisions:COUNTER:5:U:U DS:drops:COUNTER:5:U:U \
	$RRA_SETUP
    echo "$i created"
    ;;

io_*.rrd)
    # Build disk files
    rrdtool create $i --step=$INTERVAL \
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
    cat <<EOF
Usage: $0 all
       $0 cpu|mem|<if>|<io>

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
