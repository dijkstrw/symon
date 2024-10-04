/*
 * Copyright (c) 2009-2024 Willem Dijkstra
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
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/ataio.h>
#include <dev/ata/atavar.h>
#include <dev/ic/wdcreg.h>

#include <errno.h>
#include <fcntl.h>
#include <strings.h>
#include <string.h>
#include <stdlib.h>

#include "conf.h"
#include "data.h"
#include "error.h"
#include "xmalloc.h"
#include "smart.h"

/* per drive storage structure */
struct smart_device {
    char name[MAXPATHLEN];
    int fd;
    int type;
    int failed;
    struct atareq cmd;
    struct smart_values data;
};

static struct smart_device *smart_devs = NULL;
static int smart_cur = 0;

void
init_smart(struct stream *st)
{
    int i;
    char drivename[MAXPATHLEN];
    struct atareq *p;

    if (sizeof(struct smart_values) != DISK_BLOCK_LEN) {
        fatal("smart: internal error: smart values structure is broken");
    }

    if (st->arg == NULL) {
        fatal("smart: need a <device> argument");
    }

    bzero(drivename, MAXPATHLEN);
    snprintf(drivename, MAXPATHLEN, "/dev/%s", st->arg);

    /* look for drive in our global table */
    for (i = 0; i < smart_cur; i++) {
        if (strncmp(smart_devs[i].name, drivename, MAXPATHLEN) == 0) {
            st->parg.smart = i;
            return;
        }
    }

    /* this is a new drive; allocate the command and data block */
    if (smart_cur > SYMON_MAX_DOBJECTS) {
        fatal("%s:%d: dynamic object limit (%d) exceeded for smart data",
              __FILE__, __LINE__, SYMON_MAX_DOBJECTS);
    }

    smart_devs = xrealloc(smart_devs, (smart_cur + 1) * sizeof(struct smart_device));

    bzero(&smart_devs[smart_cur], sizeof(struct smart_device));

    /* rewire all bufferlocations, as our addresses may have changed */
    for (i = 0; i <= smart_cur; i++) {
        smart_devs[i].cmd.databuf = (caddr_t)&smart_devs[i].data;
    }

    /* store drivename in new block */
    snprintf(smart_devs[smart_cur].name, MAXPATHLEN, "%s", drivename);

    /* populate ata command header */
    p = &smart_devs[smart_cur].cmd;
    p->command = ATAPI_SMART;
    p->timeout = SMART_TIMEOUT;
    p->features = ATA_SMART_READ_VALUES;
    p->cylinder = htole16(SMART_CYLINDER);
    p->flags = ATACMD_READ;
    p->databuf = (caddr_t) &smart_devs[smart_cur].data;
    p->datalen = DISK_BLOCK_LEN;

    /* store filedescriptor to device */
    smart_devs[smart_cur].fd = open(drivename, O_RDWR | O_NONBLOCK);

    if (errno) {
        fatal("smart: could not open '%s' for read", drivename);
    }

    /* store smart dev entry in stream to facilitate quick get */
    st->parg.smart = smart_cur;

    smart_cur++;

    info("started module smart(%.200s)", st->arg);
}

void
gets_smart(void)
{
    int i;

    for (i = 0; i < smart_cur; i++) {
        if (ioctl(smart_devs[i].fd, ATAIOCCOMMAND, &smart_devs[i].cmd)) {
            warning("smart: ioctl for drive '%s' failed: %d",
                    &smart_devs[i].name, errno);
            smart_devs[i].failed = 1;
        }

        /* Some drives do not calculate the smart checksum correctly;
         * additional code that identifies these drives would increase our
         * footprint and the amount of datajuggling we need to do; we would
         * rather ignore the checksums.
         */

        smart_devs[i].failed = 0;
    }
    return;
}

int
get_smart(char *symon_buf, int maxlen, struct stream *st)
{
    struct smart_report sr;

    if ((st->parg.smart < smart_cur) &&
        (!smart_devs[st->parg.smart].failed))
    {
        smart_parse(&smart_devs[st->parg.smart].data, &sr);
        return snpack(symon_buf, maxlen, st->arg, MT_SMART,
                      sr.read_error_rate,
                      sr.reallocated_sectors,
                      sr.spin_retries,
                      sr.air_flow_temp,
                      sr.temperature,
                      sr.reallocations,
                      sr.current_pending,
                      sr.uncorrectables,
                      sr.soft_read_error_rate,
                      sr.g_sense_error_rate,
                      sr.temperature2,
                      sr.free_fall_protection);
    }

    return 0;
}
