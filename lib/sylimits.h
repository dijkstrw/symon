/* $Id: sylimits.h,v 1.2 2005/10/18 19:58:05 dijkstra Exp $ */

#ifndef _LIB_LIMITS_H
#define _LIB_LIMITS_H

#define SYMON_PS_ARGLEN        16       /* maximum argument length */
#define SYMON_PS_ARGLENSTR     "15"     /* maximum number of chars in an argument, as str */
#define SYMON_WARN_SENDERR     50       /* warn every x errors */
#define SYMON_MAX_DOBJECTS     10000    /* max dynamic allocation; local limit per
					 * measurement module */
#define SYMON_MAX_OBJSIZE      (_POSIX2_LINE_MAX)
#define SYMON_SENSORMASK       0xFF     /* sensors 0-255 are allowed */
#define SYMON_MAXCPUS          4
#define SYMON_MAXDEBUGID       20       /* = CTL_DEBUG_MAXID; depends lib/data.h */

#endif
