/* $Id: platform.h,v 1.3 2005/10/21 14:58:44 dijkstra Exp $ */

#ifndef _CONF_LINUX_H
#define _CONF_LINUX_H

#include <stdio.h>
#include <grp.h>

#include "queue.h"
#include "sylimits.h"

#define SYMON_USER      "symon"
#define SEM_ARGS        (S_IWUSR|S_IRUSR|IPC_CREAT|IPC_EXCL)
#define SA_LEN(x)       (((x)->sa_family == AF_INET6) ? sizeof(struct sockaddr_in6) : sizeof(struct sockaddr_in))
#define SS_LEN(x)       (((x)->ss_family == AF_INET6) ? sizeof(struct sockaddr_in6) : sizeof(struct sockaddr_in))
#define strlcpy(x,y,z)  snprintf((x),(z),"%s", (y))

union semun {
	int val;
};

#define CPUSTATES 4
#define CP_USER 0
#define CP_NICE 1
#define CP_SYS  2
#define CP_IDLE 3

union stream_parg {
    struct {
	long time[CPUSTATES];
	long old[CPUSTATES];
	long diff[CPUSTATES];
	int states[CPUSTATES];
	char name[6];
    } cp;
};

#endif
