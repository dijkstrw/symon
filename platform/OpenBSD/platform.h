/* $Id: platform.h,v 1.3 2005/10/21 14:58:46 dijkstra Exp $ */

#ifndef _CONF_OPENBSD_H
#define _CONF_OPENBSD_H

#include <sys/dkstat.h>
#include <sys/queue.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <net/if.h>

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
	char rawdev[SYMON_DFNAMESIZE];
    } df;
    struct ifreq ifr;
    int sn;
};

#endif
