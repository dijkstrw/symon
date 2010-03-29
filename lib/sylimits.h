#ifndef _LIB_LIMITS_H
#define _LIB_LIMITS_H

#define SYMON_PS_ARGLENV1      16       /* maximum argument length in a v1 packet */
#define SYMON_PS_ARGLENV2      64       /* maximum argument length in a v2 packet */
#define SYMON_PS_ARGLENSTRV2   "63"     /* maximum number of chars in an v2 argument, as str */
#define SYMON_WARN_SENDERR     50       /* warn every x errors */
#define SYMON_MAX_DOBJECTS     10000    /* max dynamic allocation; local limit per
                                         * measurement module */
#define SYMON_MAX_OBJSIZE      (_POSIX2_LINE_MAX)
#define SYMON_SENSORMASK       0xFF     /* sensors 0-255 are allowed */
#define SYMON_MAXDEBUGID       20       /* = CTL_DEBUG_MAXID; depends lib/data.h */
#define SYMON_MAXCPUID         16       /* cpu0 - cpu15 */
#define SYMON_DFBLOCKSIZE      512
#define SYMON_DFNAMESIZE       64
#define SYMON_MAXPACKET        65515    /* udp packet max payload 65Kb - 20 byte header */

#define SYMON_MAXLEXNUM        65535    /* maximum numeric argument while lexing */
#endif
