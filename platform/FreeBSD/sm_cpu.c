/*
 * Copyright (c) 2014-2024 Willem Dijkstra
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
 * This code is not re-entrant.
 *
 * This module uses the sysctl interface and can run as any user.
 */

#include "conf.h"

#include <sys/param.h>
#include <sys/dkstat.h>
#include <sys/sysctl.h>

#include <errno.h>
#include <stdlib.h>
#include <limits.h>

#include "error.h"
#include "percentages.h"
#include "symon.h"
#include "sylimits.h"
#include "xmalloc.h"

/* Globals for this module all start with cp_ */
#ifdef HAS_CP_TIMES
static char cp_time_mib_str[] = "kern.cp_times";
#else
static char cp_time_mib_str[] = "kern.cp_time";
#endif
static int cp_time_mib[CTL_MAXNAME];
static size_t cp_time_len = 0;
static void *cp_buf;
static size_t cp_size = 0;

void
init_cpu(struct stream *st)
{
    char buf[SYMON_MAX_OBJSIZE];
    const char *errstr;

    if (cp_time_len == 0) {
        cp_time_len = CTL_MAXNAME;
        if (sysctlnametomib(cp_time_mib_str, cp_time_mib, &cp_time_len) < 0) {
            warning("sysctlnametomib for cpu failed");
            cp_time_len = 0;
        }

        if ((sysctl(cp_time_mib, cp_time_len, NULL, &cp_size, NULL, 0) != -1) &&
            (errno == ENOMEM))
            fatal("cpu(%.200s): failed to determine sysctl buffer len", st->arg);

        if (cp_size > SYMON_MAX_OBJSIZE)
            fatal("cpu(%.200s): sysctl buffer too large", st->arg);

        cp_buf = xmalloc(cp_size);
        gets_cpu();
    }

    st->parg.cp.id = strtonum(st->arg, 0, SYMON_MAXCPUID, &errstr);
    if (errstr != NULL)
        fatal("cpu(%.200s) is invalid: %.200s", st->arg, errstr);

    get_cpu(buf, sizeof(buf), st);

    info("started module cpu(%.200s)", st->arg);
}

void
gets_cpu()
{
    if (sysctl(cp_time_mib, cp_time_len, cp_buf, &cp_size, NULL, 0) < 0) {
        warning("%s:%d: sysctl failed", __FILE__, __LINE__);
        return;
    }
}

int
get_cpu(char *symon_buf, int maxlen, struct stream *st)
{
    int i;
    long *time1;

    if (!cp_time_len)
        return 0;

    time1 = (long *)(cp_buf + (sizeof(long) * CPUSTATES * st->parg.cp.id));

    if (((void *)time1 - cp_buf) > cp_size)
        warning("cpu(%d): not in sysctl buffer", st->parg.cp.id);

    /* convert cp_time counts to percentages */
    for (i = 0; i < CPUSTATES; i++)
        st->parg.cp.time2[i] = (int64_t) time1[i];

    (void)percentages(CPUSTATES, st->parg.cp.states, st->parg.cp.time2, st->parg.cp.old, st->parg.cp.diff);

    return snpack(symon_buf, maxlen, st->arg, MT_CPU,
                  (double) (st->parg.cp.states[CP_USER] / 10.0),
                  (double) (st->parg.cp.states[CP_NICE] / 10.0),
                  (double) (st->parg.cp.states[CP_SYS] / 10.0),
                  (double) (st->parg.cp.states[CP_INTR] / 10.0),
                  (double) (st->parg.cp.states[CP_IDLE] / 10.0));
}
