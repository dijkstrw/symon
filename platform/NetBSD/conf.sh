if [ -f /usr/include/net/pfvar.h ]; then
    echo "#define HAS_PFVAR_H	1"
else
    echo "#undef HAS_PFVAR_H"
fi
if grep -q "priq_basic_class_stats" /usr/include/altq/altq_priq.h; then
    echo "#define HAS_ALTQ_BASIC_STATS	1"
else
    echo "#undef HAS_ALTQ_BASIC_STATS"
fi
if grep -q HW_IOSTATS /usr/include/sys/sysctl.h; then
    echo "#define HAS_HW_IOSTATS	1"
else
    echo "#undef HAS_HW_IOSTATS"
fi
