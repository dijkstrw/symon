/*
 * Copyright (c) 2010 Willem Dijkstra
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
 * Get measurement data from a flukso (arduino) board tied to the serial port.
 *
 * num : value
 *
 */

#include "conf.h"

#include <sys/types.h>
#include <sys/stat.h>

#include <string.h>
#include <inttypes.h>
#include <fcntl.h>
#include <termios.h>
#include <unistd.h>
#include <ctype.h>
#include <strings.h>

#include "error.h"
#include "symon.h"
#include "xmalloc.h"

#define FLUKSO_MAXSENSORS 3
#define FLUKSO_IDLEN 32
#define SCN(x) #x

static int flukso_fd = -1;
static void *flukso_buf = NULL;
static int flukso_size = 0;
static int flukso_maxsize = 0;
static int flukso_nrsensors = 0;

struct flukso_sensor {
    char id[FLUKSO_IDLEN];
    uint64_t value;
    uint64_t n;
};
struct flukso_sensor flukso_sensor[FLUKSO_MAXSENSORS];

void
init_flukso(struct stream *st)
{
    struct termios tio;

    bzero(flukso_sensor, sizeof(struct flukso_sensor) * FLUKSO_MAXSENSORS);

    if (flukso_buf == NULL) {
        flukso_maxsize = SYMON_MAX_OBJSIZE;
        flukso_buf = xmalloc(flukso_maxsize);
    }

    if (flukso_fd == -1) {
        bzero(&tio, sizeof(tio));
        tio.c_iflag = tio.c_oflag = tio.c_lflag = 0;
        tio.c_cflag = CS8 | CREAD | CLOCAL;
        tio.c_cc[VMIN] = 1;
        tio.c_cc[VTIME] = 5;
        if ((flukso_fd = open("/dev/ttyS0", O_RDONLY | O_NONBLOCK)) < 0)
            fatal("flukso: could not open '%s' for read", "/dev/ttyS0");
        cfsetispeed(&tio, B4800);
        tcsetattr(flukso_fd, TCSANOW, &tio);
    }

    if (flukso_size == 0)
        gets_flukso();

    if (st->arg != NULL &&
        FLUKSO_IDLEN == strspn(st->arg, "0123456789abcdef")) {
        bcopy(st->arg, st->parg.flukso, FLUKSO_IDLEN);
        info("started module flukso(%.200s)", st->arg);
    } else {
        fatal("flukso: could not parse sensor name %.200s", st->arg);
    }
}

void
gets_flukso()
{
    int len = 0;
    int p = 0;
    int i;
    void *nl;
    uint32_t value;
    char id[FLUKSO_IDLEN];

    if ((len = read(flukso_fd, flukso_buf + flukso_size, flukso_maxsize - flukso_size)) < 0) {
        warning("flukso: cannot read data");
        return;
    }

    flukso_size += len;

    /* We read the pwr messages rather than the pls pulse messages as we are
     * interested in the current load only */
    while ((p < flukso_size) &&
           ((nl = memchr(flukso_buf + p, '\n', flukso_size - p)) != NULL)) {
        len = nl - (flukso_buf + p) + 1;

        if (2 > sscanf(flukso_buf + p, "pwr %32s:%" SCNu32 "\n", &id[0], &value)) {
            p += len;
            continue;
        }

        for (i = 0; i < flukso_nrsensors; i++) {
            if (strncmp(flukso_sensor[i].id, id, FLUKSO_IDLEN) == 0) {
                flukso_sensor[i].value += value;
                flukso_sensor[i].n++;
                break;
            }
        }

        if ((i == flukso_nrsensors) && (flukso_nrsensors < FLUKSO_MAXSENSORS)) {
            bcopy(id, flukso_sensor[i].id, FLUKSO_IDLEN);
            flukso_sensor[i].value = value;
            flukso_sensor[i].n = 1;
            flukso_nrsensors++;
        }

        p += len;
    }

    if (p < flukso_size) {
        bcopy(flukso_buf + p, flukso_buf, flukso_size - p);
        flukso_size -= p;
    } else {
        flukso_size = 0;
    }
}

int
get_flukso(char *symon_buf, int maxlen, struct stream *st)
{
    int i;
    double avgwatts;

    for (i = 0; i < flukso_nrsensors; i++) {
        if (strncmp(st->parg.flukso, flukso_sensor[i].id, FLUKSO_IDLEN) == 0) {
            avgwatts = (double) flukso_sensor[i].value / (double) flukso_sensor[i].n;
            flukso_sensor[i].value = 0;
            flukso_sensor[i].n = 0;

            return snpack(symon_buf, maxlen, st->arg, MT_FLUKSO, avgwatts);
        }
    }

    return 0;
}
