/*
 * Copyright (c) 2011 Willem Dijkstra
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

#include <sys/stat.h>
#include <sys/types.h>

#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <strings.h>

#include "error.h"
#include "platform.h"

/*
 * checkdisk(spath, dpath, maxlen)
 *
 * Determine that <spath> exists, and return the device name that was referenced
 * (/dev/sdx) when <spath> is a link in <dpath>, observing <dpath>'s <maxlen>.
 */
static size_t
checkdisk(const char *spath, char *dpath, size_t maxlen)
{
    char diskname[MAX_PATH_LEN];
    char *r;
    struct stat buffer;
    size_t result;

    bzero(diskname, MAX_PATH_LEN);
    bzero(&buffer, sizeof(struct stat));

    if (lstat(spath, &buffer)) {
        return 0;
    }

    if (S_ISLNK(buffer.st_mode)) {
        if ((r = realpath(spath, NULL))) {
            strlcpy(diskname, r, sizeof(diskname));
            free(r);
        }
        bzero(&buffer, sizeof(struct stat));
        if (lstat(diskname, &buffer)) {
            fatal("diskbyname: '%.200s' = '%.200s' cannot be examined: %.200s", spath, diskname, strerror(errno));
        }
    } else
        strlcpy(diskname, spath, sizeof(diskname));

    if (S_ISBLK(buffer.st_mode) && !S_ISLNK(buffer.st_mode)) {
        result = strlcpy(dpath, diskname, maxlen);
        return result;
    }

    return 0;
}

/*
 * diskbyname(spath, dpath, maxlen)
 *
 * Resolve a logical disk name <spath> to it's block device name
 * <dpath>. <dpath> is preallocated and can hold <maxlen> characters. <spath>
 * can refer to a disk via 1) an absolute path or 2) a diskname relative to
 * /dev, or 3) a diskname relative to the /dev/disk/by-* directories.
 */
size_t
diskbyname(const char *spath, char *dpath, size_t maxlen)
{
    char diskname[MAX_PATH_LEN];
    size_t size;
    char *l[] = {
        "/dev/%s",
        "/dev/disk/by-id/%s",
        "/dev/disk/by-label/%s",
        "/dev/disk/by-uuid/%s",
        "/dev/disk/by-path/%s",
        NULL
    };
    int i;

    if (spath == NULL)
        return 0;

    if (strchr(spath, '/'))
        return checkdisk(spath, dpath, maxlen);

    for (i = 0; l[i] != NULL; i++) {
        bzero(diskname, sizeof(diskname));
        snprintf(diskname, sizeof(diskname), l[i], spath);
        if ((size = checkdisk(diskname, dpath, maxlen)))
            return size;
    }

    return 0;
}
