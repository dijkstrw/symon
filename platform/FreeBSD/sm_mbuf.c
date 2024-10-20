/*
 * Copyright (c) 2024 Willem Dijkstra
 * Copyright (c) 2004 Matthew Gream
 * Copyright (c) 2003 Daniel Hartmeier
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

#include <sys/param.h>
#include <sys/mbuf.h>
#include <sys/protosw.h>
#include <sys/sf_buf.h>
#include <sys/socket.h>
#include <sys/socketvar.h>
#include <sys/sysctl.h>
#include <errno.h>

#include <memstat.h>
#include <string.h>
#include <unistd.h>

#include "error.h"
#include "symon.h"

void
init_mbuf(struct stream *st)
{
    info("started module mbuf(%.200s)", st->arg);
}

int
get_mbuf(char *symon_buf, int maxlen, struct stream *st)
{
    u_int32_t stats[15];
    struct memory_type_list *mtlp;
    struct memory_type *mtp;
    uintmax_t mbuf_count, mbuf_bytes, mbuf_free, mbuf_failures, mbuf_size;
    uintmax_t mbuf_sleeps;
    uintmax_t cluster_count, cluster_free, cluster_size;
    uintmax_t packet_count, packet_bytes, packet_free, packet_failures;
    uintmax_t packet_sleeps;
    uintmax_t tag_bytes;
    uintmax_t jumbop_count, jumbop_free, jumbop_size;
    uintmax_t jumbo9_count, jumbo9_free, jumbo9_size;
    uintmax_t jumbo16_count, jumbo16_free, jumbo16_size;
    uintmax_t bytes_inuse, bytes_incache, bytes_total;

    bzero(&stats, sizeof(stats));

    mtlp = memstat_mtl_alloc();
    if (mtlp == NULL) {
        warning("mbuf() failed memstat_mtl_alloc");
        return (0);
    }

    if (memstat_sysctl_all(mtlp, 0) < 0) {
        warning("mbuf() failed memstat_sysctl_all: %.200s",
                memstat_strerror(memstat_mtl_geterror(mtlp)));
        goto out;
    }

    mtp = memstat_mtl_find(mtlp, ALLOCATOR_UMA, MBUF_MEM_NAME);
    if (mtp == NULL) {
        warning("mbuf() failed memstat_mtl_find: zone %.200s not found",
                MBUF_MEM_NAME);
        goto out;
    }
    mbuf_count = memstat_get_count(mtp);
    mbuf_bytes = memstat_get_bytes(mtp);
    mbuf_free = memstat_get_free(mtp);
    mbuf_failures = memstat_get_failures(mtp);
    mbuf_sleeps = memstat_get_sleeps(mtp);
    mbuf_size = memstat_get_size(mtp);

    mtp = memstat_mtl_find(mtlp, ALLOCATOR_UMA, MBUF_PACKET_MEM_NAME);
    if (mtp == NULL) {
        warning("mbuf() failed memstat_mtl_find: zone %.200s not found",
                MBUF_PACKET_MEM_NAME);
        goto out;
    }
    packet_count = memstat_get_count(mtp);
    packet_bytes = memstat_get_bytes(mtp);
    packet_free = memstat_get_free(mtp);
    packet_sleeps = memstat_get_sleeps(mtp);
    packet_failures = memstat_get_failures(mtp);

    mtp = memstat_mtl_find(mtlp, ALLOCATOR_UMA, MBUF_CLUSTER_MEM_NAME);
    if (mtp == NULL) {
        warning("mbuf() failed memstat_mtl_find: zone %.200s not found",
                MBUF_CLUSTER_MEM_NAME);
        goto out;
    }
    cluster_count = memstat_get_count(mtp);
    cluster_free = memstat_get_free(mtp);
    cluster_size = memstat_get_size(mtp);

    mtp = memstat_mtl_find(mtlp, ALLOCATOR_MALLOC, MBUF_TAG_MEM_NAME);
    if (mtp == NULL) {
        warning("mbuf() failed memstat_mtl_find: zone %.200s not found",
                MBUF_TAG_MEM_NAME);
        goto out;
    }
    tag_bytes = memstat_get_bytes(mtp);

    mtp = memstat_mtl_find(mtlp, ALLOCATOR_UMA, MBUF_JUMBOP_MEM_NAME);
    if (mtp == NULL) {
        warning("mbuf() failed memstat_mtl_find: zone %.200s not found",
                MBUF_JUMBOP_MEM_NAME);
        goto out;
    }
    jumbop_count = memstat_get_count(mtp);
    jumbop_free = memstat_get_free(mtp);
    jumbop_size = memstat_get_size(mtp);

    mtp = memstat_mtl_find(mtlp, ALLOCATOR_UMA, MBUF_JUMBO9_MEM_NAME);
    if (mtp == NULL) {
        warning("mbuf() failed memstat_mtl_find: zone %.200s not found",
                MBUF_JUMBO9_MEM_NAME);
        goto out;
    }
    jumbo9_count = memstat_get_count(mtp);
    jumbo9_free = memstat_get_free(mtp);
    jumbo9_size = memstat_get_size(mtp);

    mtp = memstat_mtl_find(mtlp, ALLOCATOR_UMA, MBUF_JUMBO16_MEM_NAME);
    if (mtp == NULL) {
        warning("mbuf() failed memstat_mtl_find: zone %.200s not found",
                MBUF_JUMBO16_MEM_NAME);
        goto out;
    }
    jumbo16_count = memstat_get_count(mtp);
    jumbo16_free = memstat_get_free(mtp);
    jumbo16_size = memstat_get_size(mtp);

    /*-
     * Calculate in-use bytes as:
     * - straight mbuf memory
     * - mbuf memory in packets
     * - the clusters attached to packets
     * - and the rest of the non-packet-attached clusters.
     * - m_tag memory
     * This avoids counting the clusters attached to packets in the cache.
     * This currently excludes sf_buf space.
     */
    bytes_inuse =
      mbuf_bytes +                      /* straight mbuf memory */
      packet_bytes +                    /* mbufs in packets */
      (packet_count * cluster_size) +   /* clusters in packets */
      /* other clusters */
      ((cluster_count - packet_count - packet_free) * cluster_size) +
      tag_bytes +
      (jumbop_count * jumbop_size) +    /* jumbo clusters */
      (jumbo9_count * jumbo9_size) +
      (jumbo16_count * jumbo16_size);

    /*
     * Calculate in-cache bytes as:
     * - cached straught mbufs
     * - cached packet mbufs
     * - cached packet clusters
     * - cached straight clusters
     * This currently excludes sf_buf space.
     */
    bytes_incache =
      (mbuf_free * mbuf_size) +         /* straight free mbufs */
      (packet_free * mbuf_size) +               /* mbufs in free packets */
      (packet_free * cluster_size) +    /* clusters in free packets */
      (cluster_free * cluster_size) +   /* free clusters */
      (jumbop_free * jumbop_size) +     /* jumbo clusters */
      (jumbo9_free * jumbo9_size) +
      (jumbo16_free * jumbo16_size);

    /*
     * Total is bytes in use + bytes in cache.  This doesn't take into
     * account various other misc data structures, overhead, etc, but
     * gives the user something useful despite that.
     */
    bytes_total = bytes_inuse + bytes_incache;

    stats[0] = mbuf_count + packet_count;
    stats[1] = 0; /*mbstat.m_mtypes[MT_DATA];*/
    stats[2] = 0; /*mbstat.m_mtypes[MT_OOBDATA];*/
    stats[3] = 0; /*mbstat.m_mtypes[MT_CONTROL];*/
    stats[4] = 0; /*mbstat.m_mtypes[MT_HEADER];*/
    stats[5] = 0; /*mbstat.m_mtypes[MT_FTABLE];*/
    stats[6] = 0; /*mbstat.m_mtypes[MT_SONAME];*/
    stats[7] = 0; /*mbstat.m_mtypes[MT_SOOPTS];*/
    stats[8] = 0;
    stats[9] = 0;
    stats[10] = bytes_total;
    stats[11] = (bytes_total == 0) ? 0 : ((bytes_inuse * 100) / bytes_total);
    stats[12] = mbuf_failures + packet_failures;
    stats[13] = mbuf_sleeps + packet_sleeps;
    stats[14] = 0;

out:
    memstat_mtl_free(mtlp);

    return snpack(symon_buf, maxlen, st->arg, MT_MBUF,
                  stats[0],
                  stats[1],
                  stats[2],
                  stats[3],
                  stats[4],
                  stats[5],
                  stats[6],
                  stats[7],
                  stats[8],
                  stats[9],
                  stats[10],
                  stats[11],
                  stats[12],
                  stats[13],
                  stats[14]);
}
