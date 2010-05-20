if [ -f /proc/diskstats ]; then
    echo "#define HAS_PROC_DISKSTATS 1"
else
    echo "#undef HAS_PROC_DISKSTATS"
fi
if [ -f /proc/partitions ]; then
    FIELDS=`head -1 /proc/partitions | wc -w`
    if [ $FIELDS -eq 15 ]; then
	echo "#define HAS_PROC_PARTITIONS 1"
    else
	echo "#undef HAS_PROC_PARTITIONS"
    fi
fi
if grep -q "hd_drive_cmd_hdr" /usr/include/linux/hdreg.h; then
    echo "#define HAS_HDDRIVECMDHDR 1"
else
    echo "#undef HAS_HDDRIVECMDHDR"
fi

