/* $Id: platform.h,v 1.3 2005/10/18 19:58:06 dijkstra Exp $ */

#ifndef _CONF_FREEBSD_H
#define _CONF_FREEBSD_H

#include <sys/queue.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/resource.h>

#include <net/if.h>
#include <net/if_mib.h>

#include "sylimits.h"

#define SYMON_USER      "_symon"
#define SEM_ARGS        (SEM_A|SEM_R)
#define SA_LEN(x)       ((x)->sa_len)
#define SS_LEN(x)       ((x)->ss_len)

union stream_parg {
    struct {
	long time[CPUSTATES];
	long old[CPUSTATES];
	long diff[CPUSTATES];
	int states[CPUSTATES];
    } cp;
    struct {
	char rawdev[SYMON_PS_ARGLEN + 6]; /* "/dev/xxx" */
    } df;
    struct ifreq ifr;
    int sn;
};

#endif
