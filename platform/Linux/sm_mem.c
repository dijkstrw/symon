/* $Id: sm_mem.c,v 1.1 2005/09/30 14:05:27 dijkstra Exp $ */

/*
 * Copyright (c) 2005 Harm Schotanus
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
 * Get current memory statistics in bytes; reports them back in symon_buf as
 *
 * real active : real total : free : [swap used : swap total]
 *
 * This code is not re-entrant.
 */

#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include "error.h"
#include "conf.h"
#include "symon.h"
#include "xmalloc.h"

#define ktob(size) ((size) << 10)

/* Globals for this module all start with me_ */
static void *me_buf = NULL;
static int me_size = 0;
static int me_maxsize = 0;
static long me_stats[5];

void
init_mem(char *s)
{
    if (me_buf == NULL) {
	me_maxsize = SYMON_MAX_OBJSIZE;
	me_buf = xmalloc(me_maxsize);
    }

    info("started module mem(%.200s)", s);
}
void
gets_mem()
{
   int fd;
   if ((fd = open("/proc/meminfo", O_RDONLY)) < 0) {
	warning("cannot access /proc/meminfo: %.200s", strerror(errno));
   }

   bzero(me_buf, me_maxsize);
   me_size = read(fd, me_buf, me_maxsize);
   close(fd);

   if (me_size == me_maxsize) {
	/* buffer is too small to hold all memory data */
	me_maxsize += SYMON_MAX_OBJSIZE;
	if (me_maxsize > SYMON_MAX_OBJSIZE * SYMON_MAX_DOBJECTS) {
	    fatal("%s:%d: dynamic object limit (%d) exceeded for cp data",
		__FILE__, __LINE__, SYMON_MAX_OBJSIZE * SYMON_MAX_DOBJECTS);
	}
	me_buf = xrealloc(me_buf, me_maxsize);
	gets_mem();
	return;
    }

   if (me_size == -1) {
       warning("could not read if statistics from /proc/meminfo: %.200s", strerror(errno));
   }
}

long
mem_getitem(char *name)
{
    long stat;
    char *line;

    if (me_size <= 0) {
	return 0;
    }

    if ((line = strstr(me_buf, name)) == NULL) {
	warning("could not find %s in /proc/meminfo", name);
	return 0;
    }

    line += strlen(name);
    if (1 < sscanf(line, ": %lu Kb", &stat)) {
	warning("could not parse memory statistics");
	return 0;
    } else {
	return stat;
    }
}

int
get_mem(char *symon_buf, int maxlen, char *s)
{
    me_stats[0] = ktob(mem_getitem("Active"));
    me_stats[1] = ktob(mem_getitem("MemTotal"));
    me_stats[2] = ktob(mem_getitem("MemFree"));
    me_stats[3] = ktob(mem_getitem("SwapFree"));
    me_stats[4] = ktob(mem_getitem("SwapTotal"));

    /* swap used = SwapTotal - SwapFree
     * real total = MemTotal + swap used - Active
     */

    me_stats[3] = me_stats[4] - me_stats[3];
    me_stats[1] += me_stats[3] - me_stats[0];

    return snpack(symon_buf, maxlen, s, MT_MEM,
		  me_stats[0], me_stats[1], me_stats[2],
		  me_stats[3], me_stats[4]);

}
