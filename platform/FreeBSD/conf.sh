#!/bin/sh
if grep -q "m_drops" /usr/include/sys/mbuf.h; then
    echo "#define HAS_MBUF_MDROPS	1"
else
    echo "#undef HAS_MBUF_MDROPS"
fi
if grep -q "sf_allocfail" /usr/include/sys/mbuf.h; then
    echo "#define HAS_MBUF_SFALLOCFAIL	1"
else
    echo "#undef HAS_MBUF_SFALLOCFAIL"
fi
if ! grep -q "VM_TOTAL" /usr/include/vm/vm_param.h; then
    echo "#define VM_TOTAL VM_METER"
fi
if grep -q "struct xswdev" /usr/include/vm/vm_param.h; then
    echo "#define HAS_XSWDEV	1"
else
    echo "#undef HAS_XSWDEV"
fi
if [ -f /usr/include/net/pfvar.h ]; then
    echo "#define HAS_PFVAR_H	1"
else
    echo "#undef HAS_PFVAR_H"
fi
if grep -q "ki_paddr" /usr/include/sys/user.h; then
    echo "#define HAS_KI_PADDR	1"
else
    echo "#undef HAS_KI_PADDR"
fi
if grep -q "struct rusage_ext" /usr/include/sys/proc.h; then
    echo "#define HAS_RUSAGE_EXT	1"
else
    echo "#undef HAS_RUSAGE_EXT"
fi
if grep -q "CPUSTATES" /usr/include/sys/resource.h; then
    echo "#define HAS_RESOURCE_CPUSTATE	1"
else
    echo "#undef HAS_RESOURCE_CPUSTATE"
fi
if grep -q "IOCATAREQUEST" /usr/include/sys/ata.h; then
    echo "#define HAS_IOCATAREQUEST 1"
else
    echo "#undef HAS_IOCATAREQUEST"
fi
if grep -q "ATA_SMART_CMD" /usr/include/sys/ata.h; then
    echo "#define HAS_ATA_SMART_CMD 1"
else
    echo "#undef HAS_ATA_SMART_CMD"
fi
