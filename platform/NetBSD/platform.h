#ifndef _CONF_NETBSD_H
#define _CONF_NETBSD_H

#define _NETBSD_SOURCE 1

#include <sys/queue.h>
#include <sys/sched.h>
#include <sys/types.h>
#include <sys/param.h>

#include <net/if.h>

#include <stdio.h>

#include "sylimits.h"

#define SYMON_USER      "_symon"
#define SEM_ARGS        (IPC_W|IPC_R)
#define SA_LEN(x)       ((x)->sa_len)
#define SS_LEN(x)       ((x)->ss_len)

union semun {
        int val;
};

#define MAX_PATH_LEN MAXPATHLEN

union stream_parg {
    struct {
        int64_t time[CPUSTATES];
        int64_t old[CPUSTATES];
        int64_t diff[CPUSTATES];
        int64_t states[CPUSTATES];
    } cp;
    struct {
        char rawdev[SYMON_DFNAMESIZE];
    } df;
    struct ifdatareq ifr;
    int sn;
    int smart;
};

#endif
