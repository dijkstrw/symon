/*
 * $Id: sm_mem.c,v 1.7 2001/07/01 12:50:24 dijkstra Exp $
 *
 * Get current memory statistics in bytes; reports them back in mon_buf as
 *
 * real active : real total : free : [swap used : swap total]
 * 
 * -DMEM_SWAP controls whether the swap statistics are generated.  */

#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <limits.h>
#include <sys/param.h>
#include <sys/sysctl.h>
#include <syslog.h>
#include <varargs.h>
#include "mon.h"
#include <sys/swap.h>

#define pagetob(size) ((size) << me_pageshift)

/*
 * globals for this module all start with me_
 */
static int me_pageshift;
static long me_stats[4];
static int me_vm_mib[] = {CTL_VM, VM_METER};
static struct vmtotal me_vmtotal;
static size_t me_vmsize;
static int me_pagesize;
static int me_nswap;
struct swapent *me_swdev = NULL;

void init_mem(s) 
    char *s;
{
    me_pagesize = sysconf( _SC_PAGESIZE );
    me_pageshift = 0;
    while (me_pagesize > 1) {
	me_pageshift++;
	me_pagesize >>= 1;
    }

    /* we only need the amount of log(2)1024 for our conversion */
    /*  me_pageshift -= LOG1024;*/

    /* get total -- systemwide main memory usage structure */
    me_vmsize = sizeof(me_vmtotal);

    /* determine number of swap entries */
    me_nswap = swapctl(SWAP_NSWAP, 0, 0);
  
    if (me_swdev) free(me_swdev);  
    me_swdev = malloc(me_nswap * sizeof(*me_swdev));
    if (me_swdev == NULL && me_nswap != 0) me_nswap=0; 
}
char *get_mem(s)
    char *s;
{
    int i,rnswap;

    if (sysctl(me_vm_mib, 2, &me_vmtotal, &me_vmsize, NULL, 0) < 0) {
	syslog(LOG_WARNING,"mem.c:%d: sysctl failed",__LINE__);
	bzero(&me_vmtotal, sizeof(me_vmtotal));
    }
    /* convert memory stats to Kbytes */
    me_stats[0] = pagetob(me_vmtotal.t_arm);
    me_stats[1] = pagetob(me_vmtotal.t_rm);
    me_stats[2] = pagetob(me_vmtotal.t_free);

    rnswap = swapctl(SWAP_STATS, me_swdev, me_nswap);
    if (rnswap == -1) { 
	/* A swap device may have been added; increase and retry */
	init_mem(NULL); 
	rnswap = swapctl(SWAP_STATS, me_swdev, me_nswap);
    }

    me_stats[3] = me_stats[4] = 0;
    if (rnswap == me_nswap) { /* Read swap succesfully */
	/* Total things up */
	for (i = 0; i < me_nswap; i++) {
	    if (me_swdev[i].se_flags & SWF_ENABLE) {
		me_stats[3] += (me_swdev[i].se_inuse * DEV_BSIZE);
		me_stats[4] += (me_swdev[i].se_nblks * DEV_BSIZE);
	    }
	}
    }

    /* real active, real total, free, swap used, swap total */
    snprintf(&mon_buf[0], _POSIX2_LINE_MAX, "N:%lu:%lu:%lu:%lu:%lu", 
	     me_stats[0], me_stats[1], me_stats[2], 
	     me_stats[3], me_stats[4]);

    return &mon_buf[0];
}
