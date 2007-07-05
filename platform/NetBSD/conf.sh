if [ -f /usr/include/net/pfvar.h ]; then
    echo "#define HAS_PFVAR_H	1"
else
    echo "#undef HAS_PFVAR_H"
fi
case `grep -csq HW_IOSTATS /usr/include/sys/sysctl.h` in
1)	echo "#define HAS_HW_IOSTATS	1" ;;
0)	echo "#undef HAS_HW_IOSTATS" ;;
esac;
