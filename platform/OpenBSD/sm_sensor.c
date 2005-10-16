/* $Id: sm_sensor.c,v 1.6 2005/10/16 15:26:59 dijkstra Exp $ */

/*
 * Copyright (c) 2001-2005 Willem Dijkstra
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
#include <sys/sysctl.h>

#include <errno.h>
#include <limits.h>
#include <stdlib.h>
#include <string.h>

#include "error.h"
#include "symon.h"

#ifndef HAS_SENSORS_H
void
privinit_sensor()
{
    fatal("sensor support not available");
}
void
init_sensor(struct stream *st)
{
    fatal("sensor support not available");
}
int
get_sensor(char *symon_buf, int maxlen, struct stream *st)
{
    fatal("sensor support not available");
    return 0;
}

#else

#include <sys/sensors.h>

/* Globals for this module start with sn_ */
static int sn_mib[] = {CTL_HW, HW_SENSORS, 0};
static struct sensor sn_sensor;

/* Prepare if module for first use */
void
privinit_sensor()
{
}
void
init_sensor(struct stream *st)
{
    long l = strtol(st->arg, NULL, 10);
    st->parg.sn = (int) (l & SYMON_SENSORMASK);

    info("started module sensors(%.200s)", st->arg);
}
/* Get sensor statistics */
int
get_sensor(char *symon_buf, int maxlen, struct stream *st)
{
    size_t len;
    double t;

    bzero((void *) &sn_sensor, sizeof(sn_sensor));
    sn_mib[2] = st->parg.sn;

    len = sizeof(sn_sensor);

    if (sysctl(sn_mib, 3, &sn_sensor, &len, NULL, 0) == -1) {
	warning("%s:%d: sensor can't get sensor %.200s -- %.200s",
		__FILE__, __LINE__, st->arg, strerror(errno));

	return 0;
    } else {
	switch (sn_sensor.type) {
	case SENSOR_TEMP:
	    t = (double) (sn_sensor.value / 1000.0 / 1000.0) - 273.16;
	    break;
	case SENSOR_FANRPM:
	    t = (double) sn_sensor.value;
	    break;
	case SENSOR_VOLTS_DC:
	    t = (double) (sn_sensor.value / 1000.0 / 1000.0);
	    break;
	default:
	    t = (double) sn_sensor.value;
	}

	return snpack(symon_buf, maxlen, st->arg, MT_SENSOR, t);
    }
}
#endif /* HAS_SENSORS_H */
