#ifndef _CONF_FREEBSD_H
#define _CONF_FREEBSD_H

#include <sys/queue.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <sys/dkstat.h>
#include <sys/param.h>
#include <sys/resource.h>

#include <net/if.h>
#include <net/if_mib.h>

#include "sylimits.h"

#define SYMON_USER      "_symon"
#define SEM_ARGS        (SEM_A|SEM_R)
#define SA_LEN(x)       ((x)->sa_len)
#define SS_LEN(x)       ((x)->ss_len)

#define DISK_PATHS { "/dev/%s", "/dev/ufs/%s", "/dev/ufsid/%s", "/dev/gpt/%s", NULL }
#define MAX_PATH_LEN MAXPATHLEN

union stream_parg {
    struct {
        long time1[CPUSTATES];
        int64_t time2[CPUSTATES];
        int64_t old[CPUSTATES];
        int64_t diff[CPUSTATES];
        int64_t states[CPUSTATES];
    } cp;
    struct {
        char rawdev[SYMON_DFNAMESIZE];
    } df;
    struct ifreq ifr;
    int sn;
    int smart;
};

#endif
