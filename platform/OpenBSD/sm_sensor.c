/* $Id: sm_sensor.c,v 1.2 2003/12/20 16:30:44 dijkstra Exp $ */

/*
 * Copyright (c) 2001-2003 Willem Dijkstra
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
 * This code is not re-entrant. It uses sysctl and can be run as any user.
 */
#include <conf.h>

#include <sys/param.h>
#include <sys/sysctl.h>

#ifdef HAS_SENSORS_H
#include <sys/sensors.h>
#endif HAS_SENSORS_H

#include <errno.h>
#include <limits.h>
#include <stdlib.h>
#include <string.h>

#include "error.h"
#include "symon.h"

#ifndef HAS_SENSORS_H
void
init_sensor(char *s)
{
    fatal("sensor support not available");
}
int
get_sensor(char *symon_buf, int maxlen, char *s)
{
    fatal("sensor support not available");
}
#else
/* Globals for this module start with sn_ */
static int sn_mib[] = {CTL_HW, HW_SENSORS, 0};
static struct sensor sn_sensor;

/* Prepare if module for first use */
void
init_sensor(char *s)
{
    info("started module sensors(%.200s)", s);
}
/* Get sensor statistics */
int
get_sensor(char *symon_buf, int maxlen, char *s)
{
    size_t len;
    long l;
    int i;
    double t;

    bzero((void *) &sn_sensor, sizeof(sn_sensor));
    l = strtol(s, NULL, 10);
    i = (int) (l & SYMON_SENSORMASK);
    sn_mib[2] = i;

    len = sizeof(sn_sensor);

    if (sysctl(sn_mib, 3, &sn_sensor, &len, NULL, 0) == -1) {
	warning("%s:%d: sensor can't get sensor %.200s -- %.200s",
		__FILE__, __LINE__, s, strerror(errno));

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

	return snpack(symon_buf, maxlen, s, MT_SENSOR, t);
    }
}
#endif /* HAS_SENSORS_H */