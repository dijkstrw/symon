/* $Id: platform.h,v 1.2 2005/10/16 15:26:54 dijkstra Exp $ */

#ifndef _CONF_FREEBSD_H
#define _CONF_FREEBSD_H

#include <sys/queue.h>

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
