/* $Id: error.c,v 1.6 2002/04/01 14:44:15 dijkstra Exp $ */

/*
 * Copyright (c) 2001-2002 Willem Dijkstra
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
#include <syslog.h>

#include "error.h"

__BEGIN_DECLS
void output_message(int, char *, va_list); 
__END_DECLS

int flag_daemon = 0;
int flag_debug = 0;

enum { MON_LOG_FATAL, 
       MON_LOG_WARNING,
       MON_LOG_INFO,
       MON_LOG_DEBUG } loglevels;

struct {
    int type;
    int priority;
    char *errtxt;
    FILE *stream;
} logmapping[] = {
    {MON_LOG_FATAL,   LOG_ERR,     "fatal",   stderr},
    {MON_LOG_WARNING, LOG_WARNING, "warning", stderr},
    {MON_LOG_INFO,    LOG_INFO,    "",        stdout},
    {MON_LOG_DEBUG,   LOG_DEBUG,   "debug",   stdout},
    {-1,              0,           "",        NULL}
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

    if (level == MON_LOG_DEBUG && flag_debug == 0)
	return;

    vsnprintf(msgbuf, sizeof(msgbuf), fmt, args);
    
    for (loglevel = 0; logmapping[loglevel].type != -1; loglevel++) {
	if (logmapping[loglevel].type == level)
	    break;
    }

    if (logmapping[loglevel].type == -1)
	fatal("internal error: illegal loglevel encountered");

    if (flag_daemon) {
	syslog(logmapping[loglevel].priority, msgbuf);
    } else {
	if (strlen(logmapping[loglevel].errtxt) > 0) {
	    fprintf(logmapping[loglevel].stream, "%s: %s\n", 
		    logmapping[loglevel].errtxt, msgbuf);
	} else 
	    fprintf(logmapping[loglevel].stream, "%s\n", msgbuf);

	fflush(logmapping[loglevel].stream);
    }
}
/* Output error and exit */
__dead void 
fatal(char *fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    output_message(MON_LOG_FATAL, fmt, ap);
    va_end(ap);
        
    exit( 1 );
}
/* Warn and continue */
void 
warning(char *fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    output_message(MON_LOG_WARNING, fmt, ap);
    va_end(ap);
}
/* Inform and continue */
void 
info(char *fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    output_message(MON_LOG_INFO, fmt, ap);
    va_end(ap);
}
/* Debug statements only */
void
debug(char *fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    output_message(MON_LOG_DEBUG, fmt, ap);
    va_end(ap);
}

