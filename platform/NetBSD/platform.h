/* $Id: platform.h,v 1.3 2005/10/18 19:58:09 dijkstra Exp $ */

#ifndef _CONF_NETBSD_H
#define _CONF_NETBSD_H

#define _NETBSD_SOURCE 1

#include <sys/queue.h>
#include <sys/sched.h>

#include <net/if.h>

#include "sylimits.h"

#define SYMON_USER      "_symon"
#define SEM_ARGS        (IPC_W|IPC_R)
#define SA_LEN(x)       ((x)->sa_len)
#define SS_LEN(x)       ((x)->ss_len)

union semun {
	int val;
};

union stream_parg {
    struct {
	u_int64_t time[CPUSTATES];
	u_int64_t old[CPUSTATES];
	u_int64_t diff[CPUSTATES];
	int states[CPUSTATES];
    } cp;
    struct {
	char rawdev[SYMON_PS_ARGLEN + 6]; /* "/dev/xxx" */
    } df;
    struct ifdatareq ifr;
    int sn;
};

#endif
