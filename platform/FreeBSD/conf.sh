case `grep -cq "m_drops" /usr/include/sys/mbuf.h` in
1)	echo "#define HAS_MBUF_MDROPS	1" ;;
0)	echo "#undef HAS_MBUF_MDROPS" ;;
esac;
case `grep -cq "sf_allocfail" /usr/include/sys/mbuf.h` in
1)	echo "#define HAS_MBUF_SFALLOCFAIL	1" ;;
0)	echo "#undef HAS_MBUF_SFALLOCFAIL" ;;
esac;
case `grep -cq "VM_TOTAL" /usr/include/vm/vm_param.h` in
0)      echo "#define VM_TOTAL VM_METER" ;;
esac;
sysctl -N vm.nswapdev 1>/dev/null 2>&1
case $? in
1)      echo "#undef HAS_VM_NSWAPDEV" ;;
0)      echo "#define HAS_VM_NSWAPDEV	1" ;;
esac;
if [ -f /usr/include/net/pfvar.h ]; then
    echo "#define HAS_PFVAR_H	1"
else
    echo "#undef HAS_PFVAR_H"
fi;
case `grep -cq "ki_paddr" /usr/include/sys/user.h` in
1)      echo "#define HAS_KI_PADDR	1" ;;
0)      echo "#undef HAS_KI_PADDR" ;;
esac;