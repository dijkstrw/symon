/* $Id: platform.h,v 1.1 2004/08/07 12:21:36 dijkstra Exp $ */

#ifndef _CONF_LINUX_H
#define _CONF_LINUX_H

#include <stdio.h>
#include <grp.h>
#include "queue.h"

#define SYMON_USER      "symon"
#define SEM_ARGS        (S_IWUSR|S_IRUSR|IPC_CREAT|IPC_EXCL)
#define SA_LEN(x)       (((x)->sa_family == AF_INET6) ? sizeof(struct sockaddr_in6) : sizeof(struct sockaddr_in))
#define SS_LEN(x)       (((x)->ss_family == AF_INET6) ? sizeof(struct sockaddr_in6) : sizeof(struct sockaddr_in))

union semun {
	int val;
};

#define strlcpy(x,y,z)  snprintf((x),(z),"%s", (y))

#endif
