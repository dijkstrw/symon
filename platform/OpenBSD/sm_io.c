/* $Id: sm_io.c,v 1.3 2002/04/01 20:15:59 dijkstra Exp $ */

/*
 * Copyright (c) 2001-2002 Willem Dijkstra
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
 * Get current disk transfer statistics from kernel and return them in mon_buf as
 * 
 * total nr of transfers : total seeks : total bytes transferred
 *
 * This module uses the kvm interface to read kernel values directly. It needs
 * a valid kvm handle and will only work if this has been obtained by someone
 * belonging to group kmem. Note that these priviledges can be dropped right
 * after the handle has been obtained. (sgid kmem on executable) 
 *
 * Re-entrant code: globals are used to speedup calculations for all disks.
 * 
 * TODO: di_name is too large
 */
#include <sys/disk.h>

#include <limits.h>
#include <string.h>

#include "error.h"
#include "mon.h"

/* Globals for this module start with io_ */
struct disk io_disk;
size_t io_size_disk;
struct disklist_head io_dihead;
size_t io_size_dihead;
char io_name[_POSIX2_LINE_MAX];
/* Prepare io module for first use */
void 
init_io(char *s) 
{
    io_size_disk = sizeof io_disk;
    io_size_dihead = sizeof io_dihead;

    info("started module io(%s)", s);
}
/* Get new io statistics */
int 
get_io(char *mon_buf, int maxlen, char *disk) 
{
    u_long diskptr;

    diskptr = mon_nl[MON_DL].n_value;
    if (diskptr == 0)
	fatal("%s:%d: dl = symbol not defined", __FILE__, __LINE__);

    /* obtain first disk structure in kernel memory */
    kread(mon_nl[MON_DL].n_value, (char *) &io_dihead, io_size_dihead);
    diskptr = (u_long) io_dihead.tqh_first;

    while (diskptr) {
	kread(diskptr, (char *) &io_disk, io_size_disk);
	diskptr = (u_long) io_disk.dk_link.tqe_next;

	if (io_disk.dk_name != NULL) {
	    kread((u_long)io_disk.dk_name, (char *) &io_name, _POSIX2_LINE_MAX);
	    io_name[_POSIX2_LINE_MAX - 1] = '\0';

	    if (disk != NULL && strncmp(io_name, disk, _POSIX2_LINE_MAX) == 0) {
		return snpack(mon_buf, maxlen, disk, MT_IO,
			      io_disk.dk_xfer, io_disk.dk_seek,
			      io_disk.dk_bytes);
	    }
	}
    }

    return 0;
}

