/* $Id: sm_debug.c,v 1.5 2007/02/11 20:07:32 dijkstra Exp $ */

/*
 * Copyright (c) 2004      Matthew Gream
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
 * Get current debug statistics from kernel and return them in symon_buf as
 *
 * debug0 : debug1 : ... : debug19
 */

#include <sys/param.h>
#include <sys/sysctl.h>

#include <string.h>

#include "conf.h"
#include "error.h"
#include "symon.h"

/* Globals for this module start with db_ */
static int db_mib[] = { CTL_DEBUG, 0, CTL_DEBUG_VALUE };
static int db_v[SYMON_MAXDEBUGID];

void
init_debug(struct stream *st)
{
    info("started module debug(%.200s)", st->arg);
}

int
get_debug(char *symon_buf, int maxlen, struct stream *st)
{
    size_t len;
    int i;

    bzero((void *) db_v, sizeof(db_v));
    len = sizeof(int);

    for (i = 0; i < SYMON_MAXDEBUGID; i++) {
        db_mib[1] = i;

        sysctl(db_mib, sizeof(db_mib)/sizeof(int), &db_v[i], &len, NULL, 0);
    }

    return snpack(symon_buf, maxlen, st->arg, MT_DEBUG,
                  db_v[0], db_v[1], db_v[2], db_v[3], db_v[4], db_v[5], db_v[6],
                  db_v[7], db_v[8], db_v[9], db_v[10], db_v[11], db_v[12], db_v[13],
                  db_v[14], db_v[15], db_v[16], db_v[17], db_v[18], db_v[19]);

}
