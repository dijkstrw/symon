/* $Id: sm_pf.c,v 1.2 2002/08/31 15:02:10 dijkstra Exp $ */

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
 * Get current pf statistics and return them in mon_buf as 
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
#include <sys/types.h>
#include <sys/ioctl.h>
#include <sys/socket.h>

#include <netinet/in.h>
#include <net/if.h>
#include <net/pfvar.h>
#include <errno.h>
#include <fcntl.h>
#include <string.h>

#include "error.h"
#include "mon.h"

/* Globals for this module start with pf_ */
static int pf_dev = -1;

/* Prepare if module for first use */
void
init_pf(char *s)
{
    if (pf_dev == -1)
	if ((pf_dev = open("/dev/pf", O_RDWR)) == -1)
	    fatal("%s:%d: open(\"/dev/pf\") failed, %.200s", 
		  __FILE__, __LINE__, strerror(errno));

    info("started module pf(%s)", s);
}
/* Get pf statistics */
int
get_pf(char *mon_buf, int maxlen, char *arg)
{
    struct pf_status s;
    u_int64_t n;
    
    if (pf_dev == -1) {
	warning("pf(%s) failed (dev == -1)", arg);
	return 0;
    }
    
    if (ioctl(pf_dev, DIOCGETSTATUS, &s)) {
	warning("pf(%s) failed (ioctl error)", arg);
	return 0;
    }
    
    if (!s.running)
	return 0;
    
    n = s.states;
    return snpack(mon_buf, maxlen, arg, MT_PF,
		  s.bcounters[0][PF_IN],
		  s.bcounters[0][PF_OUT],
		  s.bcounters[1][PF_IN],
		  s.bcounters[1][PF_OUT],
		  s.pcounters[0][PF_IN][PF_PASS],
		  s.pcounters[0][PF_IN][PF_DROP],
		  s.pcounters[0][PF_OUT][PF_PASS],
		  s.pcounters[0][PF_OUT][PF_DROP],
		  s.pcounters[1][PF_IN][PF_PASS],
		  s.pcounters[1][PF_IN][PF_DROP],
		  s.pcounters[1][PF_OUT][PF_PASS],
		  s.pcounters[1][PF_OUT][PF_DROP],
		  n,
		  s.fcounters[0],
		  s.fcounters[1],
		  s.fcounters[2],
		  s.counters[0],
		  s.counters[1],
		  s.counters[2],
		  s.counters[3],
		  s.counters[4],
		  s.counters[5]
	);
}