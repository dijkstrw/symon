/*
 * Copyright (c) 2012-2025 Willem Dijkstra
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

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>

#include "diskname.h"
#include "error.h"
#include "platform.h"

void
initdisknamectx(struct disknamectx *c,
                const char *spath,
                char *dpath,
                size_t dmaxlen)
{
    bzero(c, sizeof(struct disknamectx));
    c->spath = spath;
    c->dpath = dpath;
    c->maxlen = dmaxlen;

    if (c->dpath != NULL)
        bzero(c->dpath, c->maxlen);
}

/*
 * diskbyname(spath, dpath, maxlen)
 *
 * Resolve a logical disk name <spath> to it's block device name
 * <dpath>. <dpath> is preallocated and can hold <maxlen> characters. <spath>
 * can refer to a disk via 1) an absolute path or 2) a diskname relative to
 * /dev in various forms.
 */
char *
nextdiskname(struct disknamectx *c)
{
    char *r;
    struct stat buffer;
#ifdef DISK_PATHS
    char *l[] = DISK_PATHS;
#else
    char *l[] = { "/dev/%s", NULL };
#endif

    if ((c->spath == NULL) || (c->dpath == NULL) || (l[c->i] == NULL))
        return NULL;

    bzero(c->dpath, c->maxlen);

    /* do not play with absolute paths, just return it once */
    if (c->spath[0] == '/') {
        if (c->i == 0) {
            snprintf(c->dpath, c->maxlen, "%s", c->spath);
            c->i++;
            return c->dpath;
        }
        return NULL;
    }

    /* prepare the next pathname */
    snprintf(c->dpath, c->maxlen, l[c->i], c->spath);

    if ((c->link == 0) && (lstat(c->dpath, &buffer) == 0)) {
        /* return the real path of a link, but only dereference once */
        if (S_ISLNK(buffer.st_mode)) {
            if ((r = realpath(c->dpath, NULL))) {
                snprintf(c->dpath, c->maxlen, "%s", r);
                free(r);
                c->link = 1;

                return c->dpath;
            }
        }
    }

    c->link = 0;
    c->i++;

    return c->dpath;
}
