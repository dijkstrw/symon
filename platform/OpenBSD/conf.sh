case `grep -csq KERN_MBSTAT /usr/include/sys/sysctl.h` in
1)	echo "#define HAS_KERN_MBSTAT	1" ;;
0)	echo "#undef HAS_KERN_MBSTAT" ;;
esac;
if [ -f /usr/include/net/pfvar.h ]; then
    echo "#define HAS_PFVAR_H	1"
else
    echo "#undef HAS_PFVAR_H"
fi
