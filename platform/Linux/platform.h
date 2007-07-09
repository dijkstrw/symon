/* $Id: platform.h,v 1.5 2007/07/09 11:16:37 dijkstra Exp $ */

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

#define CPUSTATES    8
#define CP_USER      0
#define CP_NICE      1
#define CP_SYS       2
#define CP_IDLE      3
#define CP_IOWAIT    4
#define CP_HARDIRQ   5
#define CP_SOFTIRQ   6
#define CP_STEAL     7

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
