/*
 * Copyright (c) 2010 Willem Dijkstra
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

#include <sys/types.h>
#include <strings.h>

#include "smart.h"

/*
 * smart_values are rewritten to a smart_report here. This function is reused
 * across all platforms, as smart_values is the form the disks report the data
 * back in.
 */
void
smart_parse(struct smart_values *ds, struct smart_report *sr)
{
    int i;

    bzero(sr, sizeof(struct smart_report));

    for (i = 0; i < MAX_SMART_ATTRIBUTES; i++) {
        switch (ds->attributes[i].id) {
        case ATA_ATTRIBUTE_END:
            /* end of attribute block reached */
            return;
            break;

        case ATA_ATTRIBUTE_READ_ERROR_RATE:
            sr->read_error_rate = ds->attributes[i].current;
            break;

        case ATA_ATTRIBUTE_REALLOCATED_SECTOR_COUNT:
            sr->reallocated_sectors = ds->attributes[i].current;
            break;

        case ATA_ATTRIBUTE_SPIN_RETRY_COUNT:
            sr->spin_retries = ds->attributes[i].current;
            break;

        case ATA_ATTRIBUTE_AIR_FLOW_TEMPERATURE:
            sr->air_flow_temp = ds->attributes[i].current;
            break;

        case ATA_ATTRIBUTE_TEMPERATURE:
            sr->temperature = ds->attributes[i].current;
            break;

        case ATA_ATTRIBUTE_REALLOCATION_EVENT_COUNT:
            sr->reallocations = ds->attributes[i].current;
            break;

        case ATA_ATTRIBUTE_CURRENT_PENDING_SECTOR_COUNT:
            sr->current_pending = ds->attributes[i].current;
            break;

        case ATA_ATTRIBUTE_UNCORRECTABLE_SECTOR_COUNT:
            sr->uncorrectables = ds->attributes[i].current;
            break;

        case ATA_ATTRIBUTE_SOFT_READ_ERROR_RATE:
            sr->soft_read_error_rate = ds->attributes[i].current;
            break;

        case ATA_ATTRIBUTE_G_SENSE_ERROR_RATE:
            sr->g_sense_error_rate = ds->attributes[i].current;
            break;

        case ATA_ATTRIBUTE_TEMPERATURE2:
            sr->temperature2 = ds->attributes[i].current;
            break;

        case ATA_ATTRIBUTE_FREE_FALL_PROTECTION:
            sr->free_fall_protection = ds->attributes[i].current;
            break;
        }
    }
}
/*
 * The return value of the smart read values is encoded in the ata
 * cylinder register. This function hides that magic and is used by
 * those operating systems that do not decode this data for us.
 */
int
smart_status(unsigned char low, unsigned char high)
{
    unsigned const char nlow = 0x4f;
    unsigned const char nhigh = 0xc2;
    unsigned const char flow = 0xf4;
    unsigned const char fhigh = 0x2c;

    /* Check for good values */
    if ((low == nlow) && (high == nhigh))
        return 0;

    /* Check for bad values */
    if ((low == flow) && (high == fhigh))
        return 1;

    /* Values do not make sense - signal to caller */
    return 2;
}
