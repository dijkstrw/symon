/*
 * Copyright (c) 2001-2024 Willem Dijkstra
 * Copyright (c) 2006/2007 Constantine A. Murenin
 *                         <cnst+symon@bugmail.mojo.ru>
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
 *
 */

#include "conf.h"

#include <sys/param.h>
#include <sys/sensors.h>
#include <sys/sysctl.h>

#include <errno.h>
#include <limits.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include "error.h"
#include "symon.h"
#include "xmalloc.h"

#ifndef MAXSENSORDEVICES
#define MAXSENSORDEVICES 1024
#endif

/* Globals for this module start with sn_ */
static struct sensor sn_sensor;

void
privinit_sensor()
{
    /* EMPTY */
}

void
init_sensor(struct stream *st)
{
    char *devname, *typename, *bufp, *bufpo;
    int dev, numt, i;
    enum sensor_type type;
    struct sensordev sensordev;
    size_t sdlen = sizeof(sensordev);

    st->parg.sn.mib[0] = CTL_HW;
    st->parg.sn.mib[1] = HW_SENSORS;

    bufpo = xstrdup(st->arg);
    bufp = bufpo;

    if ((devname = strsep(&bufp, ".")) == NULL)
        fatal("%s:%d: sensor(%.200s): incomplete specification",
              __FILE__, __LINE__, st->arg);

    /* convert sensor device string to an integer */
    for (dev = 0; dev < MAXSENSORDEVICES; dev++) {
        st->parg.sn.mib[2] = dev;
        if (sysctl(st->parg.sn.mib, 3, &sensordev, &sdlen, NULL, 0) == -1) {
            if (errno == ENOENT)
                break;
            if (errno == ENXIO)
                continue;
            fatal("%s:%d: sensor(%.200s): sensor %d unknown errno %d",
                  __FILE__, __LINE__, st->arg, dev, errno);
        }
        if (strcmp(devname, sensordev.xname) == 0)
            break;
    }
    if (strcmp(devname, sensordev.xname) != 0)
        fatal("sensor(%.200s): device not found: %.200s",
              st->arg, devname);

    /* convert sensor_type string to an integer */
    if ((typename = strsep(&bufp, ".")) == NULL)
        fatal("%s:%d: sensor(%.200s): incomplete specification",
              __FILE__, __LINE__, st->arg);
    numt = -1;
    for (i = 0; typename[i] != '\0'; i++)
        if (isdigit(typename[i])) {
            numt = atoi(&typename[i]);
            typename[i] = '\0';
            break;
        }
    for (type = 0; type < SENSOR_MAX_TYPES; type++)
        if (strcmp(typename, sensor_type_s[type]) == 0)
            break;
    if (type == SENSOR_MAX_TYPES)
        fatal("sensor(%.200s): sensor type not recognised: %.200s",
              st->arg, typename);
    if (sensordev.maxnumt[type] == 0)
        fatal("sensor(%.200s): no sensors of such type on this device: %.200s",
              st->arg, typename);
    st->parg.sn.mib[3] = type;

    if (numt == -1) {
        warning("sensor(%.200s): sensor number not specified, using 0",
                st->arg);
        numt = 0;
    }
    if (!(numt < sensordev.maxnumt[type]))
        fatal("sensor(%.200s): no such sensor attached to this device: %.200s%i",
              st->arg, typename, numt);
    st->parg.sn.mib[4] = numt;

    xfree(bufpo);

    info("started module sensor(%.200s)", st->arg);
}

int
get_sensor(char *symon_buf, int maxlen, struct stream *st)
{
    size_t len = sizeof(sn_sensor);
    double t;

    if (sysctl(st->parg.sn.mib,
               sizeof(st->parg.sn.mib)/sizeof(st->parg.sn.mib[0]),
               &sn_sensor, &len, NULL, 0) == -1) {
        if (errno != ENOENT)
            warning("%s:%d: sensor(%.200s): sysctl error: %.200s",
                    __FILE__, __LINE__, st->arg, strerror(errno));
        else
            warning("sensor(%.200s): sensor not found",
                    st->arg);

        return 0;
    } else {
        switch (sn_sensor.type) {
        case SENSOR_TEMP:                    /* temperature (µK) */
            sn_sensor.value -= (int64_t) (273.15 * 1000 * 1000);
            /* FALLTHROUGH */
        case SENSOR_VOLTS_DC:                /* voltage (µV DC) */
        case SENSOR_VOLTS_AC:                /* voltage (µV AC) */
        case SENSOR_WATTS:                   /* power (µW) */
        case SENSOR_AMPS:                    /* current (µA) */
        case SENSOR_WATTHOUR:                /* power capacity (µWh) */
        case SENSOR_AMPHOUR:                 /* power capacity (µAh) */
        case SENSOR_LUX:                     /* illuminance (µlx) */
        case SENSOR_FREQ:                    /* frequency (µHz) */
        case SENSOR_ANGLE:                   /* angle (µDegrees) */
        case SENSOR_DISTANCE:                /* distance (µMeter) */
        case SENSOR_ACCEL:                   /* acceleration (µm/s^2) */
        case SENSOR_VELOCITY:                /* velocity (µm/s) */
        case SENSOR_ENERGY:                  /* energy (µJ) */
            t = (double) (sn_sensor.value / (1000.0 * 1000.0));
            break;
        default:
            t = (double) sn_sensor.value;
        }

        return snpack(symon_buf, maxlen, st->arg, MT_SENSOR, t);
    }
}
