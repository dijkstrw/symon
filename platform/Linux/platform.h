#ifndef _CONF_LINUX_H
#define _CONF_LINUX_H

#include <stdint.h>
#include <grp.h>

/* uclibc snprintf is redefined between stdio.h and string.h */
#include <features.h>
#ifdef __UCLIBC_MAJOR__
#undef __USE_BSD
#endif

#include "queue.h"
#include "sylimits.h"

#define SYMON_USER      "symon"
#define SYMUX_USER      "symux"
#define SA_LEN(x)       (((x)->sa_family == AF_INET6) ? sizeof(struct sockaddr_in6) : sizeof(struct sockaddr_in))
#define SS_LEN(x)       (((x)->ss_family == AF_INET6) ? sizeof(struct sockaddr_in6) : sizeof(struct sockaddr_in))

#ifdef LONG_LONG_MAX
#define QUAD_MAX LONG_LONG_MAX
#endif
#ifndef QUAD_MAX
#define QUAD_MAX     (0x7fffffffffffffffLL)
#endif

#define CPUSTATES    8
#define CP_USER      0
#define CP_NICE      1
#define CP_SYS       2
#define CP_IDLE      3
#define CP_IOWAIT    4
#define CP_HARDIRQ   5
#define CP_SOFTIRQ   6
#define CP_STEAL     7

#define MAX_PATH_LEN       1024

#define DISK_PATHS   { "%s", "/dev/%s", "/dev/disk/by-id/%s", "/dev/disk/by-id/%s-part1", "/dev/disk/by-label/%s", "/dev/disk/by-uuid/%s", "/dev/disk/by-path/%s", "/dev/disk/by-path/%s-part1", "/dev/mapper/%s", NULL }

#define DISK_BLOCK_LEN     512

#define SENSOR_FAN       0
#define SENSOR_IN        1
#define SENSOR_TEMP      2

union stream_parg {
    struct {
        int64_t time[CPUSTATES];
        int64_t old[CPUSTATES];
        int64_t diff[CPUSTATES];
        int64_t states[CPUSTATES];
        char name[6];
    } cp;
    struct {
        int64_t time[CPUSTATES];
        int64_t old[CPUSTATES];
        int64_t diff[CPUSTATES];
        int64_t states[CPUSTATES];
        char name[6];
    } cpw;
    struct {
        char mountpath[MAX_PATH_LEN];
    } df;
    struct {
        int type;
        char path[MAX_PATH_LEN];
    } sn;
    int smart;
    char ifname[MAX_PATH_LEN];
    char flukso[MAX_PATH_LEN];
    char io[MAX_PATH_LEN];
};

#endif
