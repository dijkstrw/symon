/*
 * $Id: sm_io.c,v 1.1 2002/03/09 16:25:33 dijkstra Exp $
 *
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
#include <limits.h>
#include <sys/disk.h>
#include "mon.h"

struct disk di_disk;
size_t di_size_disk;
struct disklist_head di_dihead;
size_t di_size_dihead;
char di_name[_POSIX2_LINE_MAX];

void init_io(s) 
    char *s;
{
    di_size_disk = sizeof di_disk;
    di_size_dihead = sizeof di_dihead;
}
int get_io(mon_buf, maxlen, disk) 
    char *mon_buf;
    int maxlen;
    char *disk;
{
    u_long diskaddr;

    /*
     * Find the pointer to the first disk structure.  Replace
     * the pointer to the TAILQ_HEAD with the actual pointer
     * to the first list element.
     */
    kread(mon_nl[MON_DL].n_value, (char *) &di_dihead, di_size_dihead);
    diskaddr = (u_long) di_dihead.tqh_first;

    /* Traverse over disks */
    while (diskaddr) {
	kread(diskaddr, (char *) &di_disk, di_size_disk);
	diskaddr = (u_long) di_disk.dk_link.tqe_next;
	if (di_disk.dk_name != NULL) {
	    kread((u_long)di_disk.dk_name, (char *) &di_name, _POSIX2_LINE_MAX);
	    di_name[_POSIX2_LINE_MAX - 1] = '\0'; /* sanity */
	    if (disk != NULL && strcmp(di_name, disk) == 0) {
		return snpack(mon_buf, maxlen, disk, MT_IO,
			      di_disk.dk_xfer, di_disk.dk_seek,
			      di_disk.dk_bytes);
	    }
	}
    }
    return 0;
}

