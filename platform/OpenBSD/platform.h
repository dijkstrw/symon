/* $Id: platform.h,v 1.1 2004/08/09 06:28:28 dijkstra Exp $ */

#ifndef _CONF_OPENBSD_H
#define _CONF_OPENBSD_H

#include <sys/queue.h>

#define SYMON_USER      "_symon"
#define SEM_ARGS        (SEM_A|SEM_R)
#define SA_LEN(x)       ((x)->sa_len)
#define SS_LEN(x)       ((x)->ss_len)

#endif
