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
 * Get current cpu statistics in percentages (total of all counts = 100.0)
 * and returns them in symon_buf as
 *
 * user : nice : system : interrupt : idle
 *
 * This code is not re-entrant and UP only.
 *
 * This module uses the sysctl interface and can run as any user.
 */

#include <sys/dkstat.h>
#include <sys/param.h>
#include <sys/sysctl.h>
#include <sys/sched.h>

#include "error.h"
#include "percentages.h"
#include "symon.h"

/* Globals for this module all start with cp_ */
static int cp_time_mib[] = {CTL_KERN, KERN_CP_TIME};
static size_t cp_size;
void
init_cpu(struct stream *st)
{
    char buf[SYMON_MAX_OBJSIZE];

    cp_size = sizeof(st->parg.cp.time);
    get_cpu(buf, sizeof(buf), st);

    info("started module cpu(%.200s)", st->arg);
}

void
gets_cpu(void)
{
    /* EMPTY */
}

int
get_cpu(char *symon_buf, int maxlen, struct stream *st)
{
    if (sysctl(cp_time_mib, 2, &st->parg.cp.time, &cp_size, NULL, 0) < 0) {
        warning("%s:%d: sysctl kern.cp_time failed", __FILE__, __LINE__);
        return 0;
    }

    /* convert cp_time counts to percentages */
    (void)percentages(CPUSTATES, st->parg.cp.states, st->parg.cp.time, st->parg.cp.old, st->parg.cp.diff);

    return snpack(symon_buf, maxlen, st->arg, MT_CPU,
                  (double) (st->parg.cp.states[CP_USER] / 10.0),
                  (double) (st->parg.cp.states[CP_NICE] / 10.0),
                  (double) (st->parg.cp.states[CP_SYS] / 10.0),
                  (double) (st->parg.cp.states[CP_INTR] / 10.0),
                  (double) (st->parg.cp.states[CP_IDLE] / 10.0));
}
