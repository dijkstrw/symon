/* $Id: platform.h,v 1.6 2007/02/13 19:12:26 dijkstra Exp $ */

#ifndef _CONF_OPENBSD_H
#define _CONF_OPENBSD_H

#include "conf.h"

#include <sys/dkstat.h>
#include <sys/queue.h>
#include <sys/types.h>
#include <sys/sensors.h>
#include <sys/socket.h>
#include <net/if.h>

#include "sylimits.h"

#define SYMON_USER      "_symon"
#define SEM_ARGS        (SEM_A|SEM_R)
#define SA_LEN(x)       ((x)->sa_len)
#define SS_LEN(x)       ((x)->ss_len)

union stream_parg {
#ifdef HAS_KERN_CPTIME2
    struct {
        int64_t time[CPUSTATES];
        int64_t old[CPUSTATES];
        int64_t diff[CPUSTATES];
        int64_t states[CPUSTATES];
        int mib[3];
    } cp;
#else
    struct {
        long time[CPUSTATES];
        long old[CPUSTATES];
        long diff[CPUSTATES];
        int states[CPUSTATES];
    } cp;
#endif
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
};

#endif
