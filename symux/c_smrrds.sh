#!/bin/sh
#
# $Id: c_smrrds.sh,v 1.1 2001/08/20 14:40:12 dijkstra Exp $
#
# mon datafiles "make" file. Valid arguments:
#       all      Makes all known files
#       mem      Make memory file
#       cpu      Make cpu file
# Disks:
#       wd*      "winchester" disk drives
#       cd*      CD-ROMs
#       ccd*     concatenated disk drives
# Interfaces:
#       xl*      3Com Fast XL Etherlink cards
#       de*      DEC 21041 cards
#       wi*      Lucent Orinoco/Wavelan cards
#
# User configuration
INTERVAL=5
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
this=$0
for i 
do
case $i in

all)
    sh $this cpu mem
    sh $this wd0 wd1 wd2 ccd0 ccd1 cd0 cd1
    sh $this xl0 de0 wi0
    ;;

cpu)
    # Build cpu file
    if [ ! -f cpu.rrd ]; then
	rrdtool create cpu.rrd --step=$INTERVAL \
	    DS:user:GAUGE:5:0:100 \
	    DS:nice:GAUGE:5:0:100 \
	    DS:system:GAUGE:5:0:100 \
	    DS:interrupt:GAUGE:5:0:100 \
	    DS:idle:GAUGE:5:0:100 \
	    $RRA_SETUP
    else
	echo "cpu.rrd already exists; nop."
    fi
    ;;

mem)
    # Build memory file
    if [ ! -f mem.rrd ]; then
	rrdtool create mem.rrd --step=$INTERVAL \
	    DS:real_active:GAUGE:5:0:U \
	    DS:real_total:GAUGE:5:0:U \
	    DS:free:GAUGE:5:0:U \
	    DS:swap_used:GAUGE:5:0:U \
	    DS:swap_total:GAUGE:5:0:U \
	    $RRA_SETUP
    else
	echo "mem.rrd already exists; nop."
    fi
    ;;

xl[0-9]|de[0-9]|wi[0-9])
    # Build interface files
    if [ ! -f if_$i.rrd ]; then
        rrdtool create if_$i.rrd --step=$INTERVAL \
        DS:ipackets:COUNTER:5:U:U DS:opackets:COUNTER:5:U:U \
        DS:ibytes:COUNTER:5:U:U DS:obytes:COUNTER:5:U:U \
        DS:imcasts:COUNTER:5:U:U DS:omcasts:COUNTER:5:U:U \
        DS:ierrors:COUNTER:5:U:U DS:oerrors:COUNTER:5:U:U \
        DS:collisions:COUNTER:5:U:U DS:drops:COUNTER:5:U:U \
        $RRA_SETUP
    else
	echo "if_$i.rrd already exists; nop."
    fi
    ;;

wd[0-9]|sd[0-9]|cd[0-9]|ccd[0-9]|vnd[0-9]|raid[0-9]|rd[0-9])
    # Build disk files
    if [ ! -f if_$i.rrd ]; then
        rrdtool create disk_$i.rrd --step=$INTERVAL \
        DS:transfers:COUNTER:5:U:U \
	DS:seeks:COUNTER:5:U:U \
	DS:bytes:COUNTER:5:U:U \
        $RRA_SETUP
    else
	echo "if_$i.rrd already exists; nop."
    fi
esac
done

