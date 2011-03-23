/*
 * Copyright (c) 2008-2011 Willem Dijkstra
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

#include <sys/ioctl.h>
#include <sys/stat.h>
#include <sys/types.h>

#include <errno.h>
#include <fcntl.h>
#include <strings.h>
#include <string.h>
#include <stdlib.h>
#include <linux/hdreg.h>

#include "conf.h"
#include "data.h"
#include "error.h"
#include "xmalloc.h"
#include "smart.h"

#ifndef HAS_HDDRIVECMDHDR
typedef unsigned char task_ioreg_t;

struct hd_drive_cmd_hdr {
    task_ioreg_t command;
    task_ioreg_t sector_number;
    task_ioreg_t feature;
    task_ioreg_t sector_count;
};
#endif

/* Ata command register set for requesting smart values */
static struct hd_drive_cmd_hdr smart_cmd = {
    WIN_SMART, /* command code */
    0, /* sector number */
    SMART_READ_VALUES, /* feature */
    1 /* sector count */
};

/* Per drive storage structure; the ata cmd is followed by the data buffer that
 * is filled by the ioctl. There can be no room between the two; hence the
 * pragma for byte alignment.
 */
#pragma pack(1)
struct smart_device {
    struct hd_drive_cmd_hdr cmd;
    struct smart_values data;
    char name[MAX_PATH_LEN];
    int fd;
    int type;
    int failed;
};
#pragma pack()

static struct smart_device *smart_devs = NULL;
static int smart_cur = 0;

void
init_smart(struct stream *st)
{
    int i;
    char drivename[MAX_PATH_LEN];

    if (sizeof(struct smart_values) != DISK_BLOCK_LEN) {
        fatal("smart: internal error: smart values structure is broken");
    }

    if (st->arg == NULL)
        fatal("smart: need a <disk device|name> argument");

    if ((diskbyname(st->arg, drivename, sizeof(drivename)) == 0))
        fatal("smart: '%.200s' is not a disk device", st->arg);

    /* look for drive in our global table */
    for (i = 0; i < smart_cur; i++) {
        if (strncmp(smart_devs[i].name, drivename, sizeof(drivename)) == 0) {
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

    /* store drivename in new block */
    strlcpy(smart_devs[smart_cur].name, drivename, sizeof(smart_devs[0].name));

    /* store filedescriptor to device */
    if ((smart_devs[smart_cur].fd = open(drivename, O_RDONLY | O_NONBLOCK)) < 0)
        fatal("smart: could not open '%.200s' for read: %.200s", drivename, strerror(errno));

    /* store smart dev entry in stream to facilitate quick get */
    st->parg.smart = smart_cur;

    smart_cur++;

    info("started module smart(%.200s = %.200s)", st->arg, smart_devs[st->parg.smart].name);
}

void
gets_smart()
{
  int i;

    for (i = 0; i < smart_cur; i++) {
        /* populate ata command header */
        memcpy(&smart_devs[i].cmd, (void *) &smart_cmd, sizeof(struct hd_drive_cmd_hdr));
        if (ioctl(smart_devs[i].fd, HDIO_DRIVE_CMD, &smart_devs[i].cmd)) {
            warning("smart: ioctl for drive '%s' failed: %d",
                    &smart_devs[i].name, errno);
            smart_devs[i].failed = 1;
        }

        /* Linux does not allow checking the smart return code using the
         * HDIO_DRIVE_CMD */

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
