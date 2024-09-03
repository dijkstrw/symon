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
 * Get current altq statistics from pf and return them in symon_buf as
 * sent_bytes : sent_packets : drop_bytes : drop_packets
 */

#include "conf.h"

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <net/if.h>
#include <err.h>
#include <stdio.h>
#include <stdlib.h>

#ifdef HAS_PFVAR_H
#include <net/pfvar.h>
#include <altq/altq.h>
#include <altq/altq_cbq.h>
#include <altq/altq_priq.h>
#include <altq/altq_hfsc.h>
#endif

#include <errno.h>
#include <string.h>
#include <fcntl.h>

#include "error.h"
#include "symon.h"
#include "xmalloc.h"

#ifndef HAS_PFVAR_H
void
privinit_pfq(void)
{
    fatal("pf support not available");
}

void
init_pfq(struct stream *st)
{
    fatal("pf support not available");
}

void
gets_pfq(void)
{
    fatal("pf support not available");
}

int
get_pfq(char *b, int l, struct stream *st)
{
  fatal("pf support not available");
  return 0;
}

#else

union class_stats {
    class_stats_t cbq;
    struct priq_classstats priq;
    struct hfsc_classstats hfsc;
};

/*
 * We do not use the data structures from altq/altq_{cbq|hfsc|priq}.h as they
 * are overly complex. For now we only grab the interesting stuff.
 */

struct altq_stats {
    char qname[PF_QNAME_SIZE + IFNAMSIZ + 1];
    u_int64_t sent_bytes;
    u_int64_t sent_packets;
    u_int64_t drop_bytes;
    u_int64_t drop_packets;
};

static struct altq_stats *pfq_stats = NULL;
static int pfq_cur = 0;
static int pfq_max = 0;
int pfq_dev = -1;

void
privinit_pfq(void)
{
    if ((pfq_dev = open("/dev/pf", O_RDONLY)) == -1) {
        warning("pfq: could not open \"/dev/pf\", %.200s", strerror(errno));
    }
}

void
init_pfq(struct stream *st)
{
    if (pfq_dev == -1) {
        privinit_pfq();
    }

    info("started module pfq(%.200s)", st->arg);
}

void
gets_pfq(void)
{
    struct pfioc_altq qs;
    struct pfioc_qstats stats;
    union class_stats q;
    unsigned int nqs;
    unsigned int i;

    bzero(&qs, sizeof(qs));
    bzero(&stats, sizeof(stats));
    bzero(&q, sizeof(q));

    if (ioctl(pfq_dev, DIOCGETALTQS, &qs)) {
        fatal("pfq: DIOCGETALTQS failed");
    }
    nqs = qs.nr;

    /* Allocate memory for info for the nqs queues */
    if (nqs > pfq_max) {
        if (pfq_stats) {
            xfree(pfq_stats);
        }

        pfq_max = 2 * nqs;

        if (pfq_max > SYMON_MAX_DOBJECTS) {
            fatal("%s:%d: dynamic object limit (%d) exceeded for pf queue structures",
                  __FILE__, __LINE__, SYMON_MAX_DOBJECTS);
        }

        pfq_stats = xmalloc(pfq_max * sizeof(struct altq_stats));
    }

    pfq_cur = 0;

    /* Loop through the queues, copy info */
    for (i = 0; i < nqs; i++) {
        qs.nr = i;
        if (ioctl(pfq_dev, DIOCGETALTQ, &qs)) {
            fatal("pfq: DIOCGETALTQ failed");
        }

        /* only process the non-empty queues */
        if (qs.altq.qid > 0) {
            stats.nr = qs.nr;
            stats.ticket = qs.ticket;
            stats.buf = &q;
            stats.nbytes = sizeof(q);

            if (ioctl(pfq_dev, DIOCGETQSTATS, &stats)) {
                fatal("pfq: DIOCGETQSTATS failed");
            }

            /* We're now ready to copy the data we want. */
            snprintf(pfq_stats[pfq_cur].qname, sizeof(pfq_stats[0].qname),
                     "%s/%s", qs.altq.ifname, qs.altq.qname);

            switch (qs.altq.scheduler) {
            case ALTQT_CBQ:
                pfq_stats[pfq_cur].sent_bytes = q.cbq.xmit_cnt.bytes;
                pfq_stats[pfq_cur].sent_packets = q.cbq.xmit_cnt.packets;
                pfq_stats[pfq_cur].drop_bytes = q.cbq.drop_cnt.bytes;
                pfq_stats[pfq_cur].drop_packets = q.cbq.drop_cnt.packets;
                break;

            case ALTQT_PRIQ:
                pfq_stats[pfq_cur].sent_bytes = q.priq.xmitcnt.bytes;
                pfq_stats[pfq_cur].sent_packets = q.priq.xmitcnt.packets;
                pfq_stats[pfq_cur].drop_bytes = q.priq.dropcnt.bytes;
                pfq_stats[pfq_cur].drop_packets = q.priq.dropcnt.packets;
                break;

            case ALTQT_HFSC:
                pfq_stats[pfq_cur].sent_bytes = q.hfsc.xmit_cnt.bytes;
                pfq_stats[pfq_cur].sent_packets = q.hfsc.xmit_cnt.packets;
                pfq_stats[pfq_cur].drop_bytes = q.hfsc.drop_cnt.bytes;
                pfq_stats[pfq_cur].drop_packets = q.hfsc.drop_cnt.packets;
                break;

            default:
                warning("pfq: unknown altq scheduler type encountered");
                break;
            }
            pfq_cur++;
        }
    }
}

int
get_pfq(char *symon_buf, int maxlen, struct stream *st)
{
    unsigned int i;

    for (i = 0; i < pfq_cur; i++) {
        if (strncmp(pfq_stats[i].qname, st->arg, sizeof(pfq_stats[0].qname)) == 0) {
            return snpack(symon_buf, maxlen, st->arg, MT_PFQ,
                          pfq_stats[i].sent_bytes,
                          pfq_stats[i].sent_packets,
                          pfq_stats[i].drop_bytes,
                          pfq_stats[i].drop_packets
                );
        }
    }

    return 0;
}
#endif
