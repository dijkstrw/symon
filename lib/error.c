/* $Id: error.c,v 1.13 2004/08/07 12:21:36 dijkstra Exp $ */

/*
 * Copyright (c) 2001-2004 Willem Dijkstra
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 *    - Redistributions of source code must retain the above copyright
 *      notice, this list of conditions and the following disclaimer.
 *    - Redistributions in binary form must reproduce the above
 *      copyright notice, this list of conditions and the following
 *      disclaimer in the documentation and/or other materials provided
 *      with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
 * COPYRIGHT HOLDERS OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
 * ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *
 */
#include <sys/cdefs.h>

#include <limits.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <syslog.h>
#include <stdlib.h>

#include "error.h"

__BEGIN_DECLS
void output_message(int, char *, va_list);
__END_DECLS

int flag_daemon = 0;
int flag_debug = 0;

enum {
    SYMON_LOG_FATAL,
    SYMON_LOG_WARNING,
    SYMON_LOG_INFO,
    SYMON_LOG_DEBUG
}    loglevels;
enum {
    SYMON_OUT_STDERR,
    SYMON_OUT_STDOUT
}    outstreams;

struct {
    int type;
    int priority;
    char *errtxt;
    int stream;
}      logmapping[] = {
    { SYMON_LOG_FATAL, LOG_ERR, "fatal", SYMON_OUT_STDERR },
    { SYMON_LOG_WARNING, LOG_WARNING, "warning", SYMON_OUT_STDERR },
    { SYMON_LOG_INFO, LOG_INFO, "", SYMON_OUT_STDOUT },
    { SYMON_LOG_DEBUG, LOG_DEBUG, "debug", SYMON_OUT_STDOUT },
    { -1, 0, "", 0 }
};
/*
 * Internal helper that actually outputs every
 * (fatal|warning|info|debug) message
 */
void
output_message(int level, char *fmt, va_list args)
{
    char msgbuf[_POSIX2_LINE_MAX];
    int loglevel;

    if (level == SYMON_LOG_DEBUG && flag_debug == 0)
	return;

    vsnprintf(msgbuf, sizeof(msgbuf), fmt, args);

    for (loglevel = 0; logmapping[loglevel].type != -1; loglevel++) {
	if (logmapping[loglevel].type == level)
	    break;
    }

    if (logmapping[loglevel].type == -1)
	fatal("%s:%d:internal error: illegal loglevel encountered",
	      __FILE__, __LINE__);

    if (flag_daemon) {
	syslog(logmapping[loglevel].priority, msgbuf);
    } else {
	if (strlen(logmapping[loglevel].errtxt) > 0) {
	    fprintf(logmapping[loglevel].stream == SYMON_OUT_STDERR ? stderr : stdout, "%.200s: %.200s\n",
		    logmapping[loglevel].errtxt, msgbuf);
	} else
	    fprintf(logmapping[loglevel].stream == SYMON_OUT_STDERR ? stderr : stdout, "%.200s\n", msgbuf);

	fflush(logmapping[loglevel].stream == SYMON_OUT_STDERR ? stderr : stdout);
    }
}
/* Output error and exit */
void
fatal(char *fmt,...)
{
    va_list ap;
    va_start(ap, fmt);
    output_message(SYMON_LOG_FATAL, fmt, ap);
    va_end(ap);

    exit(1);
}
/* Warn and continue */
void
warning(char *fmt,...)
{
    va_list ap;
    va_start(ap, fmt);
    output_message(SYMON_LOG_WARNING, fmt, ap);
    va_end(ap);
}
/* Inform and continue */
void
info(char *fmt,...)
{
    va_list ap;
    va_start(ap, fmt);
    output_message(SYMON_LOG_INFO, fmt, ap);
    va_end(ap);
}
/* Debug statements only */
void
debug(char *fmt,...)
{
    va_list ap;
    va_start(ap, fmt);
    output_message(SYMON_LOG_DEBUG, fmt, ap);
    va_end(ap);
}
