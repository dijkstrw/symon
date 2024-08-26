/*
 * Copyright (c) 2001-2024 Willem Dijkstra
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

#include <sys/socket.h>
#include <sys/types.h>

#include <errno.h>
#include <fcntl.h>
#include <netdb.h>
#include <string.h>
#include <unistd.h>

#include "conf.h"
#include "data.h"
#include "error.h"
#include "net.h"
#include "symux.h"
#include "symuxnet.h"
#include "xmalloc.h"

int check_crc_packet(struct symonpacket *);

/* Obtain sockets for incoming symon traffic */
int
get_symon_sockets(struct mux *mux)
{
    struct source *source;
    struct sockaddr_storage sockaddr;
    int family, nsocks, one = 1;
    nsocks = 0;

    /* generate the udp listen socket specified in the mux statement */
    get_mux_sockaddr(mux, SOCK_DGRAM);

    /* iterate over our sources to determine what types of sockets we need */
    SLIST_FOREACH (source, &mux->sol, sources) {
        if (!get_source_sockaddr(source, AF_INET)) {
            if (!get_source_sockaddr(source, AF_INET6)) {
                warning("cannot determine socket family for source %.200s",
                    source->addr);
            }
        }

        family = source->sockaddr.ss_family;
        /* do we have a socket for this type of family */
        if (mux->symonsocket[family] <= 0) {
            if ((mux->symonsocket[family] = socket(family, SOCK_DGRAM, 0))
                != -1) {
                /* attempt to set reuse, ignore errors */
                if (setsockopt(mux->symonsocket[family], SOL_SOCKET,
                        SO_REUSEADDR, &one, sizeof(one))
                    == -1) {
                    warning(
                        "could set socket options: %.200s", strerror(errno));
                }

                /*
                 * does the mux statement specify a specific destination
                 * address
                 */
                if (mux->sockaddr.ss_family == family) {
                    cpysock((struct sockaddr *)&mux->sockaddr, &sockaddr);
                } else {
                    get_sockaddr(&sockaddr, family, SOCK_DGRAM, AI_PASSIVE,
                        NULL, mux->port);
                }

                if (bind(mux->symonsocket[family],
                        (struct sockaddr *)&sockaddr, SS_LEN(&sockaddr))
                    == -1) {
                    switch (errno) {
                    case EADDRNOTAVAIL:
                        warning("mux address %.200s is not a local address",
                            mux->addr);
                        break;
                    case EADDRINUSE:
                        warning("mux address %.200s %.200s already in use",
                            mux->addr, mux->port);
                        break;
                    case EACCES:
                        warning(
                            "mux port %.200s is restricted from current user",
                            mux->port);
                        break;
                    default:
                        warning("mux port %.200s bind failed", mux->port);
                        break;
                    }
                    close(mux->symonsocket[family]);
                    mux->symonsocket[family] = 0;

                } else {
                    if (get_numeric_name(&sockaddr)) {
                        info("getnameinfo error - cannot determine numeric "
                             "hostname and service");
                        info("listening for incoming symon traffic for "
                             "family %d",
                            family);
                    } else
                        info("listening for incoming symon traffic on udp "
                             "%.200s %.200s",
                            res_host, res_service);

                    nsocks++;
                }
            }
        }
    }
    return nsocks;
}

/*
 * Wait for traffic (symon reports from a source in sourcelist)
 * Returns the <source> and <packet>
 */
void
wait_for_traffic(struct mux *mux, struct source **source)
{
    fd_set readset;
    int i;
    int socksactive;
    int maxsock;

    for (;;) { /* FOREVER - until a valid symon packet is
                * received */
        FD_ZERO(&readset);

        maxsock = 0;
        for (i = 0; i < AF_MAX; i++) {
            if (mux->symonsocket[i] > 0) {
                FD_SET(mux->symonsocket[i], &readset);
                maxsock
                    = ((maxsock < mux->symonsocket[i]) ? mux->symonsocket[i]
                                                       : maxsock);
            }
        }

        maxsock++;
        socksactive = select(maxsock, &readset, NULL, NULL, NULL);

        if (socksactive != -1) {
            for (i = 0; i < AF_MAX; i++)
                if (FD_ISSET(mux->symonsocket[i], &readset)) {
                    if (recv_symon_packet(mux, i, source))
                        return;
                }
        } else {
            if (errno == EINTR)
                return; /* signal received while waiting, bail out */
        }
    }
}
/* Receive a symon packet for mux. Checks if the source is allowed and returns
 * the source found. return 0 if no valid packet found
 */
int
recv_symon_packet(struct mux *mux, int socknr, struct source **source)
{
    struct sockaddr_storage sind;
    socklen_t sl;
    int size, tries;
    unsigned int received;
    u_int32_t crc;

    received = 0;
    tries = 0;

    do {
        sl = sizeof(sind);

        size = recvfrom(mux->symonsocket[socknr],
            (mux->packet.data + received), (mux->packet.size - received), 0,
            (struct sockaddr *)&sind, &sl);
        if (size > 0)
            received += size;

        tries++;
    } while ((size == -1) && (errno == EAGAIN || errno == EINTR)
        && (tries < SYMUX_MAXREADTRIES) && (received < mux->packet.size));

    if ((size == -1) && errno) {
        warning("recvfrom failed: %.200s", strerror(errno));
        return 0;
    }

    *source = find_source_sockaddr(&mux->sol, (struct sockaddr *)&sind);

    get_numeric_name(&sind);

    if (*source == NULL) {
        debug("ignored data from %.200s:%.200s", res_host, res_service);
        return 0;
    } else {
        /* get header stream */
        mux->packet.offset = getheader(mux->packet.data, &mux->packet.header);
        /* check crc */
        crc = mux->packet.header.crc;
        mux->packet.header.crc = 0;
        setheader(mux->packet.data, &mux->packet.header);
        crc ^= crc32(mux->packet.data, received);
        if (crc != 0) {
            if (mux->packet.header.length > mux->packet.size)
                warning("ignored oversized packet from %.200s:%.200s; client "
                        "and server have different stream configurations",
                    res_host, res_service);
            else
                warning("ignored packet with bad crc from %.200s:%.200s",
                    res_host, res_service);
            return 0;
        }
        /* check packet version */
        if (mux->packet.header.symon_version > SYMON_PACKET_VER) {
            warning("ignored packet with unsupported version %d from "
                    "%.200s:%.200s",
                mux->packet.header.symon_version, res_host, res_service);
            return 0;
        } else {
            if (flag_debug) {
                debug("good data received from %.200s:%.200s", res_host,
                    res_service);
            }
            return 1; /* good packet received */
        }
    }
}
