/*
 * Copyright (c) 2009-2013 Willem Dijkstra
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
#include <sys/ata.h>

#include <errno.h>
#include <fcntl.h>
#include <strings.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <camlib.h>
#include <cam/cam_ccb.h>

#include "conf.h"
#include "data.h"
#include "error.h"
#include "xmalloc.h"
#include "smart.h"
#include "diskname.h"

#ifdef HAS_IOCATAREQUEST
#ifndef HAS_ATA_SMART_CMD
#define ATA_SMART_CMD 0xb0
#endif

#define MSG_SIMPLE_Q_TAG 0x20

/* per drive storage structure */
struct smart_device {
    struct smart_values data;
    struct ata_ioc_request cmd;
    struct cam_device *cd;
    struct ccb_ataio ccb;
    char name[MAX_PATH_LEN];
    int fd;
    int type;
    int failed;
};

static struct smart_device *smart_devs = NULL;
static int smart_cur = 0;

void
init_smart(struct stream *st)
{
    struct disknamectx c;
    int fd;
    int i;
    char drivename[MAX_PATH_LEN];
    struct ata_ioc_request *pa;
    struct cam_device *cd;
    struct ccb_ataio *pc;

    if (sizeof(struct smart_values) != DISK_BLOCK_LEN) {
        fatal("smart: internal error: smart values structure is broken");
    }

    if (st->arg == NULL) {
        fatal("smart: need a <disk device|name> argument");
    }

    initdisknamectx(&c, st->arg, drivename, sizeof(drivename));

    fd = -1;
    cd = NULL;

    while (nextdiskname(&c)) {
        if ((cd = cam_open_device(drivename, O_RDWR)) != NULL)
            break;
        if ((fd = open(drivename, O_RDONLY, O_NONBLOCK)) != -1)
            break;
    }

    if ((fd < 0) && (cd == NULL))
        fatal("smart: cannot open '%.200s'", st->arg);

    /* look for drive in our global table */
    for (i = 0; i < smart_cur; i++) {
        if (strncmp(smart_devs[i].name, drivename, MAX_PATH_LEN) == 0) {
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
        smart_devs[i].cmd.data = (caddr_t)&smart_devs[i].data;
        smart_devs[i].ccb.data_ptr = (u_int8_t *)&smart_devs[i].data;
    }

    /* store drivename in new block */
    snprintf(smart_devs[smart_cur].name, MAX_PATH_LEN, "%s", drivename);

    /* store filedescriptor to device */
    if (fd > 0) {
        smart_devs[smart_cur].fd = fd;

        /* populate ata command header */
        pa = &smart_devs[smart_cur].cmd;
        pa->u.ata.command = ATA_SMART_CMD;
        pa->timeout = SMART_TIMEOUT;
        pa->u.ata.feature = ATA_SMART_READ_VALUES;
        pa->u.ata.lba = SMART_CYLINDER << 8;
        pa->flags = ATA_CMD_READ;
        pa->count = DISK_BLOCK_LEN;
    } else if (cd) {
        smart_devs[smart_cur].cd = cd;

        /* populate cam control block */
        pc = &smart_devs[smart_cur].ccb;
        cam_fill_ataio(pc,
                       0,                            /* retries */
                       NULL,                         /* completion callback */
                       CAM_DIR_IN,                   /* flags */
                       MSG_SIMPLE_Q_TAG,             /* tag_action */
                       (u_int8_t *)&smart_devs[smart_cur].data,
                       DISK_BLOCK_LEN,
                       SMART_TIMEOUT);               /* timeout (s) */

        /* disable queue freeze on error */
        pc->ccb_h.flags |= CAM_DEV_QFRZDIS;

        /* populate ata command header */
        pc->cmd.flags = CAM_ATAIO_NEEDRESULT;
        pc->cmd.command = ATA_SMART_CMD;
        pc->cmd.features = ATA_SMART_READ_VALUES;
        pc->cmd.lba_low = 0;
        pc->cmd.lba_mid = SMART_CYLINDER & 0xff;
        pc->cmd.lba_high = (SMART_CYLINDER >> 8) & 0xff;
        pc->cmd.lba_low_exp = 0;
        pc->cmd.lba_mid_exp = 0;
        pc->cmd.lba_high_exp = 0;
        pc->cmd.sector_count = 1;
        pc->cmd.sector_count_exp = 0;
    }

    /* store smart dev entry in stream to facilitate quick get */
    st->parg.smart = smart_cur;
    smart_cur++;

    info("started module smart(%.200s)", st->arg);
}

void
gets_smart()
{
    int i;

    for (i = 0; i < smart_cur; i++) {
        if (smart_devs[i].fd > 0) {
            if (ioctl(smart_devs[i].fd, IOCATAREQUEST, &smart_devs[i].cmd) || smart_devs[i].cmd.error) {
                warning("smart: ioctl for drive '%s' failed: %s",
                        &smart_devs[i].name, strerror(errno));
                smart_devs[i].failed = 1;
                continue;
            }

            /* Some drives do not calculate the smart checksum correctly;
             * additional code that identifies these drives would increase our
             * footprint and the amount of datajuggling we need to do; we would
             * rather ignore the checksums.
             */

            smart_devs[i].failed = 0;
        } else if (smart_devs[i].cd != NULL) {
            if ((cam_send_ccb(smart_devs[i].cd, (union ccb *)&smart_devs[i].ccb) < 0) || smart_devs[i].ccb.res.error) {
                warning("smart: ccb for drive '%s' failed: %s",
                        &smart_devs[i].name,
                        cam_error_string(smart_devs[i].cd, (union ccb *)&smart_devs[i].ccb,
                                         (char *)&smart_devs[i].data, DISK_BLOCK_LEN,
                                         CAM_ESF_ALL, CAM_EPF_ALL));
                smart_devs[i].failed = 1;
                continue;
            }

            smart_devs[i].failed = 0;
        }
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
        debug("%x %x %x %x %x %x %x %x %x %x %x %x",
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
#else
void
init_smart(struct stream *st)
{
    fatal("smart module not available");
}
void
gets_smart()
{
    fatal("smart module not available");
}
int
get_smart(char *symon_buf, int maxlen, struct stream *st)
{
    fatal("io module not available");

    /* NOT REACHED */
    return 0;
}
#endif
