/*
 * Copyright (c) 2004      Matthew Gream
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 *    - Redistributions of source code must retain the above copyright
 *      notice, this list of conditions and the following disclaimer.
 *    - Redistributions in binary form must reproduce the above
 *      copyright notice, this list of conditions and the following
 *      disclaimer in the documentation and/or other materials provided
 *      with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
 * COPYRIGHT HOLDERS OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
 * ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *
 */

/*
 * Get sensor data from the kernel and return in symon_buf as
 *
 * num : value
 */

#include <errno.h>
#include <limits.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>

#include <sys/envsys.h>
#include <sys/ioctl.h>

#include "error.h"
#include "symon.h"

/* Globals for this module start with sn_ */
static int sn_dev = -1;
void
privinit_sensor(void)
{
    if (sn_dev == -1 && (sn_dev = open("/dev/sysmon", O_RDONLY)) == -1) {
        warning("could not open \"/dev/sysmon\", %.200s", strerror(errno));
    }
}

void
init_sensor(struct stream *st)
{
    if (sn_dev == -1) {
        privinit_sensor();
    }

    st->parg.sn = (strtol(st->arg, NULL, 10) & SYMON_SENSORMASK);

    info("started module sensors(%.200s)", st->arg);
}

int
get_sensor(char *symon_buf, int maxlen, struct stream *st)
{
    double t;

    envsys_basic_info_t e_info;
    envsys_tre_data_t e_data;

    e_info.sensor = st->parg.sn;
    if (ioctl(sn_dev, ENVSYS_GTREINFO, &e_info) == -1) {
        warning("%s:%d: sensor can't get sensor info %.200s -- %.200s",
                __FILE__, __LINE__, st->arg, strerror(errno));
        return 0;
    }

    if (!(e_info.validflags & ENVSYS_FVALID)) {
        warning("%s:%d: sensor info is invalid %.200s -- %.200s",
                __FILE__, __LINE__, st->arg, strerror(errno));
        return 0;
    }

    e_data.sensor = st->parg.sn;
    if (ioctl(sn_dev, ENVSYS_GTREDATA, &e_data) == -1) {
        warning("%s:%d: sensor can't get sensor data %.200s -- %.200s",
                __FILE__, __LINE__, st->arg, strerror(errno));
        return 0;
    }

    if (!(e_data.validflags & ENVSYS_FCURVALID)) {
        warning("%s:%d: sensor data is invalid %.200s -- %.200s",
                __FILE__, __LINE__, st->arg, strerror(errno));
        return 0;
    }

    switch (e_data.units) {
        case ENVSYS_INDICATOR:
            t = (double) (e_data.cur.data_us ? 1.0 : 0.0);
            break;
        case ENVSYS_INTEGER:
            t = (double) (e_data.cur.data_s);
            break;
        case ENVSYS_STEMP:
            t = (double) (e_data.cur.data_s / 1000.0 / 1000.0) - 273.16;
            break;
        case ENVSYS_SFANRPM:
            t = (double) (e_data.cur.data_us);
            break;
        default: /* generic - includes volts/etc */
            t = (double) (e_data.cur.data_s / 1000.0 / 1000.0);
            break;
    }

    return snpack(symon_buf, maxlen, st->arg, MT_SENSOR, t);
}
