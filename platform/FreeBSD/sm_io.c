/* $Id: sm_io.c,v 1.4 2006/06/27 18:53:58 dijkstra Exp $ */

/*
 * Copyright (c) 2005 J. Martin Petersen
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
 * nr of reads : nr of writes : seeks (zero) : read bytes : written bytes
 */

#include "conf.h"

#include <sys/types.h>
#include <sys/param.h>
#include <stdlib.h>
#include <ctype.h>

#include <errno.h>
#include <string.h>
#include <fcntl.h>

#ifdef HAS_RESOURCE_CPUSTATE
#include <sys/resource.h>
#else
#include <sys/dkstat.h>
#endif

#include <devstat.h>

#include "error.h"
#include "symon.h"

static struct statinfo io_stats;
static int io_numdevs = 0;

void
privinit_io()
{
    /* EMPTY */
}

void
init_io(struct stream *st)
{
    io_stats.dinfo = malloc(sizeof(struct devinfo));
    if (io_stats.dinfo == NULL) {
	fatal("io: could not allocate memory");
    }

    io_stats.dinfo->mem_ptr = NULL;
    info("started module io(%.200s)", st->arg);
}

void
gets_io()
{
#if DEVSTAT_USER_API_VER >= 5
    io_numdevs = devstat_getnumdevs(NULL);
#else
    io_numdevs = getnumdevs();
#endif

    if (io_numdevs == -1) {
	fatal("io: numdevs error");
    }

    if (io_stats.dinfo->mem_ptr != NULL) {
	free(io_stats.dinfo->mem_ptr);
    }

    /* clear the devinfo struct, as getdevs expects it to be all zeroes */
    bzero(io_stats.dinfo, sizeof(struct devinfo));

#if DEVSTAT_USER_API_VER >= 5
    devstat_getdevs(NULL, &io_stats);
#else
    getdevs(&io_stats);
#endif
}

int
get_io(char *symon_buf, int maxlen, struct stream *st)
{
    unsigned int i;
    struct devstat *ds;

    for (i=0; i < io_numdevs; i++) {
	ds = &io_stats.dinfo->devices[i];

	if (strncmp(ds->device_name, st->arg, strlen(ds->device_name)) == 0 &&
	    strlen(ds->device_name) < strlen(st->arg) &&
	    isdigit(st->arg[strlen(ds->device_name)]) &&
	    atoi(&st->arg[strlen(ds->device_name)]) == ds->unit_number) {
#if DEVSTAT_USER_API_VER >= 5
	    return snpack(symon_buf, maxlen, st->arg, MT_IO2,
			  ds->operations[DEVSTAT_READ],
			  ds->operations[DEVSTAT_WRITE],
			  (uint64_t) 0, /* don't know how to find #seeks */
			  ds->bytes[DEVSTAT_READ],
			  ds->bytes[DEVSTAT_WRITE]);

#else
	    return snpack(symon_buf, maxlen, st->arg, MT_IO2,
			  ds->num_reads,
			  ds->num_writes,
			  (uint64_t) 0, /* don't know how to find #seeks */
			  ds->bytes_read,
			  ds->bytes_written);
#endif
	}
    }

    return 0;
}
