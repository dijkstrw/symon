case `grep -csq "ds_rxfer" /usr/include/sys/disk.h` in
1)	echo "#define HAS_IO2	1" ;;
0)	echo "#undef HAS_IO2" ;;
esac;
if [ -f /usr/include/net/pfvar.h ]; then
    echo "#define HAS_PFVAR_H	1"
else
    echo "#undef HAS_PFVAR_H"
fi
