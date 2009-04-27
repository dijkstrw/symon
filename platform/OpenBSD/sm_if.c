/*
 * Copyright (c) 2001-2007 Willem Dijkstra
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
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <sys/sockio.h>

#include <net/if.h>
#include <net/if_dl.h>
#include <net/if_types.h>
#include <netinet/in.h>
#include <netinet/in_var.h>

#include <errno.h>
#include <limits.h>
#include <string.h>

#include "error.h"
#include "symon.h"

/* Globals for this module start with if_ */
static int if_s = -1;

void
init_if(struct stream *st)
{
    if (if_s == -1) {
        if ((if_s = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
            fatal("%s:%d: socket failed, %.200",
                  __FILE__, __LINE__, strerror(errno));
        }
    }

    strncpy(st->parg.ifr.ifr_name, st->arg, IFNAMSIZ - 1);
    st->parg.ifr.ifr_name[IFNAMSIZ - 1] = '\0';

    info("started module if(%.200s)", st->arg);
}

void
gets_if()
{
    /* EMPTY */
}

int
get_if(char *symon_buf, int maxlen, struct stream *st)
{
    struct if_data ifdata;

    st->parg.ifr.ifr_data = (caddr_t) &ifdata;

    if (ioctl(if_s, SIOCGIFDATA, &st->parg.ifr)) {
        warning("if(%.200s) failed (ioctl error)", st->arg);
        return 0;
    }

    return snpack(symon_buf, maxlen, st->arg, MT_IF2,
                  (u_int64_t) ifdata.ifi_ipackets,
                  (u_int64_t) ifdata.ifi_opackets,
                  (u_int64_t) ifdata.ifi_ibytes,
                  (u_int64_t) ifdata.ifi_obytes,
                  (u_int64_t) ifdata.ifi_imcasts,
                  (u_int64_t) ifdata.ifi_omcasts,
                  (u_int64_t) ifdata.ifi_ierrors,
                  (u_int64_t) ifdata.ifi_oerrors,
                  (u_int64_t) ifdata.ifi_collisions,
                  (u_int64_t) ifdata.ifi_iqdrops);
}
