#!/bin/sh
# $Id: c_config.sh,v 1.1 2002/10/18 12:29:48 dijkstra Exp $
#
# Create an example configuration file for symon on a host and print to stdout

# exit on errors, use a sane path and install prefix
#
set -e
PATH=/bin:/usr/bin:/sbin:/usr/sbin

# verify proper execution
#
if [ $# -ge 3 ]; then
    echo "usage: $0 [host] [port]" >&2
    exit 1
fi

interfaces=`netstat -ni | sed '1,1d;s/^\([a-z]*[0-9]\).*$/\1/g' | uniq`
for i in $interfaces; do
case $i in
bridge*|enc*|gif*|gre*|lo*|pflog*|ppp*|sl*|tun*|vlan*)
	# ignore this interface
	;;
*)
	if="if($i), $if"
	;;
esac
done
io=`mount | sed '1,1d;s/^\/dev\/\([a-z]*[0-9]\).*$/io(\1), /g' | uniq | tr -d \\\n`
host=${1:-127.0.0.1}
port=${2:-2100}
cat <<EOF
#
# symon configuration generated by  
# `basename $0` $1 $2 
#
monitor { $if 
          $io 
          cpu(0), mem } stream to $host:$port
EOF

