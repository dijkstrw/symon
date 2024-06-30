#ifndef _CONF_OPENBSD_H
#define _CONF_OPENBSD_H

#include "conf.h"

#include <sys/queue.h>
#include <sys/types.h>
#include <sys/select.h> /* fd_set */
#include <sys/socket.h>
#include <sys/sched.h>
#include <sys/syslimits.h>
#include <time.h>
#include <net/if.h>

#include "sylimits.h"

#define SYMON_USER      "_symon"
#define SYMUX_USER      "_symux"
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
        int mib[5];
    } sn;
    int smart;
    struct {
	char full[IFNAMSIZ + 1 + SYMON_WGPEERDESC];
	char *peerdesc;
    } wg;
};

#endif
