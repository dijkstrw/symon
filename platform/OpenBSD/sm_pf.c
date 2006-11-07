/* $Id: sm_pf.c,v 1.12 2006/11/07 08:00:20 dijkstra Exp $ */

/*
 * Copyright (c) 2002 Daniel Hartmeier
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
 * Get current pf statistics and return them in symon_buf as
 *
 *   bytes_v4_in : bytes_v4_out : bytes_v6_in : bytes_v6_out :
 *   packets_v4_in_pass : * packets_v4_in_drop : packets_v4_out_pass :
 *   packets_v4_out_drop : * packets_v6_in_pass : packets_v6_in_drop :
 *   packets_v6_out_pass : * packets_v6_out_drop : states_entries :
 *   states_searches : states_inserts : * states_removals : counters_match :
 *   counters_badoffset : counters_fragment : * counters_short :
 *   counters_normalize : counters_memory
 *
 */

#include "conf.h"

#include <sys/types.h>
#include <sys/ioctl.h>
#include <sys/socket.h>

#include <netinet/in.h>
#include <net/if.h>
#ifdef HAS_PFVAR_H
#include <net/pfvar.h>
#endif
#include <errno.h>
#include <fcntl.h>
#include <string.h>

#include "error.h"
#include "symon.h"

#ifndef HAS_PFVAR_H
void
privinit_pf()
{
}
void
init_pf(struct stream *st)
{
    fatal("pf support not available");
}
int
get_pf(char *symon_buf, int maxlen, struct stream *st)
{
    fatal("pf support not available");
    return 0;
}
void
gets_pf()
{
    fatal("pf support not available");
}

#else

/* Globals for this module start with pf_ */
int pf_dev = -1;
struct pf_status pf_stat;

void
privinit_pf()
{
    if ((pf_dev = open("/dev/pf", O_RDONLY)) == -1) {
	warning("could not open \"/dev/pf\", %.200s", strerror(errno));
    }
}

void
init_pf(struct stream *st)
{
    if (pf_dev == -1) {
	privinit_pf();
    }

    info("started module pf()");
}

void
gets_pf()
{
    if (pf_dev == -1) {
	warning("could not get pf stats (dev == -1)");
	pf_stat.running = 0;
	return;
    }

    if (ioctl(pf_dev, DIOCGETSTATUS, &pf_stat)) {
	warning("could not get pf stats (ioctl error)");
	pf_stat.running = 0;
	return;
    }
}

int
get_pf(char *symon_buf, int maxlen, struct stream *st)
{
    u_int64_t n;

    if (!pf_stat.running) {
	return 0;
    }

    n = pf_stat.states;
    return snpack(symon_buf, maxlen, st->arg, MT_PF,
		  pf_stat.bcounters[0][0],
		  pf_stat.bcounters[0][1],
		  pf_stat.bcounters[1][0],
		  pf_stat.bcounters[1][1],
		  pf_stat.pcounters[0][0][PF_PASS],
		  pf_stat.pcounters[0][0][PF_DROP],
		  pf_stat.pcounters[0][1][PF_PASS],
		  pf_stat.pcounters[0][1][PF_DROP],
		  pf_stat.pcounters[1][0][PF_PASS],
		  pf_stat.pcounters[1][0][PF_DROP],
		  pf_stat.pcounters[1][1][PF_PASS],
		  pf_stat.pcounters[1][1][PF_DROP],
		  n,
		  pf_stat.fcounters[0],
		  pf_stat.fcounters[1],
		  pf_stat.fcounters[2],
		  pf_stat.counters[0],
		  pf_stat.counters[1],
		  pf_stat.counters[2],
		  pf_stat.counters[3],
		  pf_stat.counters[4],
		  pf_stat.counters[5]
	);
}
#endif /* HAS_PFVAR_H */
