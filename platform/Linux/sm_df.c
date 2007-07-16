/* $Id: sm_df.c,v 1.2 2007/07/16 07:50:09 dijkstra Exp $ */

/*
 * Copyright (c) 2007 Martin van der Werff
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
 * Get current df statistics and return them in symon_buf as
 *
 *   blocks : bfree : bavail : files : ffree : 0 : 0
 *   syncwrites : asyncwrites are not available on linux
 */


#include <sys/types.h>
#include <stdio.h>
#include <mntent.h>
#include <string.h>
#include <sys/vfs.h>

#include "conf.h"
#include "error.h"
#include "symon.h"

void
init_df(struct stream *st)
{
    FILE * fp = setmntent("/etc/mtab", "r");
    struct mntent *mount;

    while ((mount = getmntent(fp))) {
	if (strncmp(mount->mnt_fsname, "/dev/", 5) == 0) {
            if (strcmp(mount->mnt_fsname + 5, st->arg) == 0) {
                strlcpy(st->parg.df.mountpath, mount->mnt_dir, sizeof(st->parg.df.mountpath));
                info("started module df(%.200s) --> %.200s", st->arg, st->parg.df.mountpath);
                endmntent(fp);
		return;
            }
	}
    }

    endmntent(fp);

    strlcpy(st->parg.df.mountpath, "/", sizeof(st->parg.df.mountpath));

    info("failed to find (%.200s) - started module df for /", st->arg);
}

void
gets_df()
{
}

/*
 * from src/bin/df.c:
 * Convert statfs returned filesystem size into BLOCKSIZE units.
 * Attempts to avoid overflow for large filesystems.
 */
#define fsbtoblk(num, fsbs, bs) \
        (((fsbs) != 0 && (fsbs) < (bs)) ? \
                (num) / ((bs) / (fsbs)) : (num) * ((fsbs) / (bs)))

int
get_df(char *symon_buf, int maxlen, struct stream *st)
{
    struct statfs buf;

    if (statfs(st->parg.df.mountpath, &buf) == 0 ) {
        return snpack(symon_buf, maxlen, st->arg, MT_DF,
                      (u_int64_t)fsbtoblk(buf.f_blocks, buf.f_bsize, SYMON_DFBLOCKSIZE),
                      (u_int64_t)fsbtoblk(buf.f_bfree, buf.f_bsize, SYMON_DFBLOCKSIZE),
                      (u_int64_t)fsbtoblk(buf.f_bavail, buf.f_bsize, SYMON_DFBLOCKSIZE),
                      (u_int64_t)buf.f_files,
                      (u_int64_t)buf.f_ffree,
                      0,
                      0);
    }

    warning("df(%.200s) failed", st->arg);
    return 0;
}