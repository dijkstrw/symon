case `grep -c "m_drops" /usr/include/sys/mbuf.h` in
1)	echo "#define HAS_MBUF_MDROPS	1" ;;
0)	echo "#undef HAS_MBUF_MDROPS" ;;
esac;
case `grep -c "sf_allocfail" /usr/include/sys/mbuf.h` in
1)	echo "#define HAS_MBUF_SFALLOCFAIL	1" ;;
0)	echo "#undef HAS_MBUF_SFALLOCFAIL" ;;
esac;
if [ -f /usr/include/net/pfvar.h ]; then
    echo "#define HAS_PFVAR_H	1"
else
    echo "#undef HAS_PFVAR_H"
fi
