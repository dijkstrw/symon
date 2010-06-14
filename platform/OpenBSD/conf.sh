case `grep -csq KERN_MBSTAT /usr/include/sys/sysctl.h` in
1)	echo "#define HAS_KERN_MBSTAT	1" ;;
0)	echo "#undef HAS_KERN_MBSTAT" ;;
esac;
case `grep -csq KERN_CPTIME2 /usr/include/sys/sysctl.h` in
1)	echo "#define HAS_KERN_CPTIME2	1" ;;
0)	echo "#undef HAS_KERN_CPTIME2" ;;
esac;
case `grep -csq KERN_PROC2 /usr/include/sys/sysctl.h` in
1)	echo "#define HAS_KERN_PROC2	1" ;;
0)	echo "#undef HAS_KERN_PROC2" ;;
esac;
case `grep -csq "struct sensordev" /usr/include/sys/sensors.h` in
1)	echo "#define HAS_SENSORDEV	1" ;;
0)	echo "#undef HAS_SENSORDEV" ;;
esac;
case `grep -csq "ds_rxfer" /usr/include/sys/disk.h` in
1)	echo "#define HAS_IO2	1" ;;
0)	echo "#undef HAS_IO2" ;;
esac;
if [ -f /usr/include/net/pfvar.h ]; then
    echo "#define HAS_PFVAR_H	1"
else
    echo "#undef HAS_PFVAR_H"
fi
