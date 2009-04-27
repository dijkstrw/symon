/*
 * Copyright (c) 2005 Marc Balmer
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
 *   blocks : bfree : bavail : files : ffree : \
 *   syncwrites : asyncwrites
 */

#include "conf.h"

#include <sys/types.h>
#include <sys/param.h>
#include <sys/mount.h>
#include <ufs/ufs/dinode.h>
#include <ufs/ffs/fs.h>

#include <errno.h>
#include <fcntl.h>
#include <fstab.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "error.h"
#include "symon.h"

/* Globals for this module start with df_ */
static struct statfs *df_stats = NULL;
static int df_parts = 0;

void
init_df(struct stream *st)
{
    strlcpy(st->parg.df.rawdev, "/dev/", sizeof(st->parg.df.rawdev));
    strlcat(st->parg.df.rawdev, st->arg, sizeof(st->parg.df.rawdev));

    info("started module df(%.200s)", st->arg);
}

void
gets_df()
{
    if ((df_parts = getmntinfo(&df_stats, MNT_NOWAIT)) == 0) {
        warning("df failed");
    }
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
    int n;

    for (n = 0; n < df_parts; n++) {
        if (!strncmp(df_stats[n].f_mntfromname, st->parg.df.rawdev, SYMON_DFNAMESIZE)) {
            return snpack(symon_buf, maxlen, st->arg, MT_DF,
                          (u_int64_t)fsbtoblk(df_stats[n].f_blocks, df_stats[n].f_bsize, SYMON_DFBLOCKSIZE),
                          (u_int64_t)fsbtoblk(df_stats[n].f_bfree, df_stats[n].f_bsize, SYMON_DFBLOCKSIZE),
                          (u_int64_t)fsbtoblk(df_stats[n].f_bavail, df_stats[n].f_bsize, SYMON_DFBLOCKSIZE),
                          (u_int64_t)df_stats[n].f_files,
                          (u_int64_t)df_stats[n].f_ffree,
                          (u_int64_t)df_stats[n].f_syncwrites,
                          (u_int64_t)df_stats[n].f_asyncwrites);
        }
    }

    warning("df(%.200s) failed (no such device)", st->arg);
    return 0;
}
