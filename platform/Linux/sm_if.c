/*
 * Copyright (c) 2001-2016 Willem Dijkstra
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
 * Get current interface statistics from kernel and return them in symon_buf as
 *
 * ipackets : opackets : ibytes : obytes : imcasts : omcasts : ierrors :
 * oerrors : colls : drops
 */

#include <sys/types.h>
#include <sys/stat.h>

#include <errno.h>
#include <fcntl.h>
#include <inttypes.h>
#include <string.h>
#include <unistd.h>

#include "conf.h"
#include "xmalloc.h"
#include "error.h"
#include "symon.h"

/* Globals for this module start with if_ */
static void *if_buf = NULL;
static int if_size = 0;
static int if_maxsize = 0;
struct if_device_stats
{
    u_int64_t rx_packets;             /* total packets received       */
    u_int64_t tx_packets;             /* total packets transmitted    */
    u_int64_t rx_bytes;               /* total bytes received         */
    u_int64_t tx_bytes;               /* total bytes transmitted      */
    u_int64_t rx_errors;              /* bad packets received         */
    u_int64_t tx_errors;              /* packet transmit problems     */
    u_int64_t rx_dropped;             /* no space in linux buffers    */
    u_int64_t tx_dropped;             /* no space available in linux  */
    u_int64_t multicast;              /* multicast packets received   */
    u_int64_t collisions;
    u_int64_t rx_frame_errors;        /* recv'd frame alignment error */
    u_int64_t rx_fifo_errors;         /* recv'r fifo overrun          */
    u_int64_t tx_carrier_errors;
    u_int64_t tx_fifo_errors;
    u_int64_t rx_compressed;
    u_int64_t tx_compressed;
    /* aggregates */
    u_int64_t errors_in;
    u_int64_t errors_out;
    u_int64_t drops;
};

void
init_if(struct stream *st)
{
    if (if_buf == NULL) {
        if_maxsize = SYMON_MAX_OBJSIZE;
        if_buf = xmalloc(if_maxsize);
    }

    snprintf(st->parg.ifname, sizeof(st->parg.ifname), "%s:", st->arg);

    info("started module if(%.200s)", st->arg);
}

void
gets_if()
{
    int fd;

    if ((fd = open("/proc/net/dev", O_RDONLY)) < 0) {
        warning("cannot access /proc/net/dev: %.200s", strerror(errno));
        return;
    }

    bzero(if_buf, if_maxsize);
    if_size = read(fd, if_buf, if_maxsize);
    close(fd);

    if (if_size == if_maxsize) {
        /* buffer is too small to hold all interface data */
        if_maxsize += SYMON_MAX_OBJSIZE;
        if (if_maxsize > SYMON_MAX_OBJSIZE * SYMON_MAX_DOBJECTS) {
            fatal("%s:%d: dynamic object limit (%d) exceeded for if data",
                  __FILE__, __LINE__, SYMON_MAX_OBJSIZE * SYMON_MAX_DOBJECTS);
        }
        if_buf = xrealloc(if_buf, if_maxsize);
        gets_if();
        return;
    }

    if (if_size == -1) {
        warning("could not read if statistics from /proc/net/dev: %.200s", strerror(errno));
    }
}

int
get_if(char *symon_buf, int maxlen, struct stream *st)
{
    char *line;
    struct if_device_stats stats;

    if (if_size <= 0) {
        return 0;
    }

    if ((line = strstr(if_buf, st->parg.ifname)) == NULL) {
        warning("could not find interface %s", st->arg);
        return 0;
    }

    line += strlen(st->parg.ifname);
    bzero(&stats, sizeof(struct if_device_stats));

    /* Inter-|   Receive                                                |  Transmit
     *  face |bytes    packets errs drop fifo frame compressed multicast|bytes    packets errs drop fifo colls carrier compressed
     */
    if (16 > sscanf(line, "%" SCNu64 " %" SCNu64 " %" SCNu64 " %" SCNu64 " %" SCNu64 " %"
                               SCNu64 " %" SCNu64 " %" SCNu64 " %"
                               SCNu64 " %" SCNu64 " %" SCNu64 " %" SCNu64 " %" SCNu64 " %"
                               SCNu64 " %" SCNu64 " %" SCNu64 "\n",
                    &stats.rx_bytes, &stats.rx_packets, &stats.rx_errors, &stats.rx_dropped, &stats.rx_fifo_errors,
                    &stats.rx_frame_errors, &stats.rx_compressed, &stats.multicast,
                    &stats.tx_bytes, &stats.tx_packets, &stats.tx_errors, &stats.tx_dropped, &stats.tx_fifo_errors,
                    &stats.collisions, &stats.tx_carrier_errors, &stats.tx_compressed)) {
        warning("could not parse interface statistics for %.200s", st->arg);
        return 0;
    }

    stats.errors_in = (stats.rx_errors + stats.rx_fifo_errors + stats.rx_frame_errors);
    stats.errors_out = (stats.tx_errors + stats.tx_fifo_errors + stats.tx_carrier_errors);
    stats.drops = (stats.rx_dropped + stats.tx_dropped);

    return snpack(symon_buf, maxlen, st->arg, MT_IF2,
                  (u_int64_t) stats.rx_packets,
                  (u_int64_t) stats.tx_packets,
                  (u_int64_t) stats.rx_bytes,
                  (u_int64_t) stats.tx_bytes,
                  (u_int64_t) stats.multicast,
                  (u_int64_t) 0,
                  (u_int64_t) stats.errors_in,
                  (u_int64_t) stats.errors_out,
                  (u_int64_t) stats.collisions,
                  (u_int64_t) stats.drops);
}
