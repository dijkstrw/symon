if [ -f /usr/include/net/pfvar.h ]; then
    echo "#define HAS_PFVAR_H	1"
else
    echo "#undef HAS_PFVAR_H"
fi
echo "#define HAS_UNVEIL	1"
