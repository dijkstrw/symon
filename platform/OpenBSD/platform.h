#ifndef _CONF_OPENBSD_H
#define _CONF_OPENBSD_H

#include "conf.h"

#include <sys/dkstat.h>
#include <sys/queue.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/syslimits.h>
#include <net/if.h>

#include "sylimits.h"

#define SYMON_USER      "_symon"
#define SEM_ARGS        (SEM_A|SEM_R)
#define SA_LEN(x)       ((x)->sa_len)
#define SS_LEN(x)       ((x)->ss_len)

#define MAX_PATH_LEN PATH_MAX

union stream_parg {
    struct {
        long time1[CPUSTATES];
        int64_t time2[CPUSTATES];
        int64_t old[CPUSTATES];
        int64_t diff[CPUSTATES];
        int64_t states[CPUSTATES];
        int mib[3];
        int miblen;
    } cp;
    struct {
        char rawdev[SYMON_DFNAMESIZE];
    } df;
    struct ifreq ifr;
    struct {
#ifndef HAS_SENSORDEV
        int mib[3];
#else
        int mib[5];
#endif
    } sn;
    int smart;
};

#endif
