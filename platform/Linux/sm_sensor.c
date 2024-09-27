/*
 * Copyright (c) 2009-2024 Willem Dijkstra
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
 * Get sensor data from the kernel via sysfs
 *
 * num : value
 *
 */

#include "conf.h"

#include <sys/types.h>
#include <sys/stat.h>

#include <ctype.h>
#include <errno.h>
#include <inttypes.h>
#include <limits.h>
#include <stdlib.h>
#include <string.h>

#include "error.h"
#include "symon.h"
#include "xmalloc.h"

void
privinit_sensor()
{
    /* EMPTY */
}

void
init_sensor(struct stream *st)
{
    char buf[SYMON_MAX_OBJSIZE];
    struct stat pathinfo;
    char *name, *p;
    int32_t n;

    /* sensors can be identified as using
     *
     * - a relative path; /sys/class/hwmon[/hwmon0][/device]/${arg}_input is
     *   then assumed: hwmon0/fan1 or fan1
     *
     * - the full path, e.g. /sys/class/hwmon/hwmon0/fan1 or
     *   /sys/class/hwmon/hwmon0/device/fan1
     *
     * - the full path outside of sysfs, this may be necessary for hwmon probes
     *   that need to some calculation before returning usable results,
     *   e.g. /symon/fan1
     *
     * Note that _input is always appended to the sensor argument.
     */

    if (strlen(st->arg) < 1)
        fatal("sensor(): no valid argument");

    /* Determine the sensor type */
    if ((name = strrchr(st->arg, '/')))
        name += 1;
    else
        name = st->arg;

    if (sscanf(name, "fan%" SCNd32, &n) == 1)
        st->parg.sn.type = SENSOR_FAN;
    else if (sscanf(name, "in%" SCNd32, &n) == 1)
        st->parg.sn.type = SENSOR_IN;
    else if (sscanf(name, "temp%" SCNd32, &n) == 1)
        st->parg.sn.type = SENSOR_TEMP;
    else
        fatal("sensor(%.200s): '%s' is a unknown sensor type; expected fan/in/temp",
              st->arg, name);

    /* Find the sensor in sysfs */
    p = &st->parg.sn.path[0];
    if (st->arg[0] == '/') {
        snprintf(p, MAX_PATH_LEN - 1, "%s_input", st->arg);
        p[MAX_PATH_LEN - 1] = '\0';

        if (stat(p, &pathinfo) < 0)
            fatal("sensor(%.200s): could not find sensor at '%.200s'",
                  st->arg, p);
    } else {
        snprintf(p, MAX_PATH_LEN - 1, "/sys/class/hwmon/hwmon0/%s_input", st->arg);
        p[MAX_PATH_LEN - 1] = '\0';

        if (stat(p, &pathinfo) < 0) {
            snprintf(p, MAX_PATH_LEN - 1, "/sys/class/hwmon/hwmon0/device/%s_input", st->arg);
            p[MAX_PATH_LEN - 1] = '\0';

            if (stat(p, &pathinfo) < 0)
                fatal("sensor(%.200s): could not be found in /sys/class/hwmon/hwmon0[/device]/%s_input",
                      st->arg, st->arg);
        }
    }

    get_sensor(buf, sizeof(buf), st);

    info("started module sensor(%.200s)", st->arg);
}

int
get_sensor(char *symon_buf, int maxlen, struct stream *st)
{
    FILE *f;
    double t;

    if ((f = fopen(st->parg.sn.path, "r")) == NULL)
        fatal("sensor(%s): cannot access %.200s: %.200s",
              st->arg, st->parg.sn.path, strerror(errno));

    if (fscanf(f, "%lf", &t) < 0) {
        warning("sensor(%s): cannot read sensor value",
                st->arg);
        return 0;
    }

    if (fclose(f) < 0)
        warning("sensor(%s): cannot close '%.200s'",
                st->arg, st->parg.sn.path);

    switch (st->parg.sn.type) {
    case SENSOR_TEMP:
    case SENSOR_IN:
        t /= 1000.0;
        break;
    }

    return snpack(symon_buf, maxlen, st->arg, MT_SENSOR, t);
}
