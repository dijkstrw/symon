/* $Id: platform.h,v 1.1 2004/08/07 12:21:36 dijkstra Exp $ */

#ifndef _CONF_FREEBSD_H
#define _CONF_FREEBSD_H

#include <sys/queue.h>

#define SYMON_USER      "_symon"
#define SEM_ARGS        (SEM_A|SEM_R)
#define SA_LEN(x)       ((x)->sa_len)
#define SS_LEN(x)       ((x)->ss_len)

#endif
