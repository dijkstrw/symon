/* $Id: readconf.c,v 1.7 2002/07/25 09:51:43 dijkstra Exp $ */

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

#include <sys/queue.h>

#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <limits.h>
#include <fcntl.h>
#include <unistd.h>

#include "data.h"
#include "error.h"
#include "lex.h"
#include "mon.h"
#include "net.h"
#include "readconf.h"
#include "xmalloc.h"

__BEGIN_DECLS
int read_host_port(struct muxlist *, struct mux *, struct lex *);
int read_mon_args(struct mux *, struct lex *);
int read_monitor(struct muxlist *, struct lex *);
__END_DECLS

/* <hostname> (port|:|,| ) <number> */
int
read_host_port(struct muxlist *mul, struct mux *mux, struct lex *l)
{
    lex_nexttoken(l);
    if (!lookup(l->token)) {
	warning("%s:%d: could not resolve '%s'",
		l->filename, l->cline, l->token);
	return 0;
    }

    if (rename_mux(mul, mux, lookup_address) == NULL) {
	warning("%s:%d: monitored data for host '%s' has already been specified",
		l->filename, l->cline, lookup_address);
	return 0;
    }
	
    mux->ip = lookup_ip;

    /* check for port statement */
    if (!lex_nexttoken(l))
	return 1;

    if (l->op == LXT_PORT || l->op == LXT_COLON || l->op == LXT_COMMA)
	lex_nexttoken(l);
    else {
	if (l->type != LXY_NUMBER) {
	    lex_ungettoken(l);
	    mux->port = MONMUX_PORT;
	    return 1;
	}
    }

    if (l->type != LXY_NUMBER) {
	parse_error(l, "<number>");
	return 0;
    }

    mux->port = l->value;
    return 1;
}
/* parse "<cpu(arg)|mem|if(arg)|io(arg)>", end condition == "}" */
int 
read_mon_args(struct mux *mux, struct lex *l) 
{
    char sn[_POSIX2_LINE_MAX];
    char sa[_POSIX2_LINE_MAX];
    int  st;

    EXPECT(l, LXT_BEGIN)
    while (lex_nexttoken(l) && l->op != LXT_END) {
	switch (l->op) {
	case LXT_CPU:
	case LXT_IF:
	case LXT_IO:
	case LXT_MEM:
	    st = token2type(l->op);
	    strncpy(&sn[0], l->token, _POSIX2_LINE_MAX);

	    /* parse arg */
	    lex_nexttoken(l);
	    if (l->op == LXT_OPEN) {
		lex_nexttoken(l);
		if (l->op == LXT_CLOSE){
		    parse_error(l, "<stream argument>");
		    return 0;
		}
		strncpy(&sa[0], l->token, _POSIX2_LINE_MAX);
		lex_nexttoken(l);
		if (l->op != LXT_CLOSE){
		    parse_error(l, ")");
		    return 0;
		}
	    } else {
		lex_ungettoken(l);
		sa[0]='\0';
	    }

	    if ((add_mux_stream(mux, st, sa)) == NULL) {
		warning("%s:%d: stream %s(%s) redefined",
			l->filename, l->cline, sn, sa);
		return 0;
	    }
	    
	    break; /* LXT_CPU/IF/IO/MEM */
	case LXT_COMMA:
	    break;
	default:
	    parse_error(l, "cpu|mem|if|io|}");
	    return 0;
	    break;
	}
    }
    
    return 1;
}

/* parse monitor <args> stream [to] <host>:<port> */
int
read_monitor(struct muxlist *mul, struct lex *l)
{
    struct mux *mux;

    mux = add_mux(mul, "<unnamed host>");

    /* parse cpu|mem|if|io */
    if (!read_mon_args(mux, l))
	return 0;

    /* parse stream to */
    EXPECT(l, LXT_STREAM);
    lex_nexttoken(l);
    if (l->op != LXT_TO) 
	lex_ungettoken(l);

    /* parse host */
    if (!read_host_port(mul, mux, l))
	return 0;
    
    return 1;
}

/* Read mon.conf */
int
read_config_file(struct muxlist *muxlist, const char *filename)
{
    struct lex *l;
    struct mux *mux;

    SLIST_INIT(muxlist);

    if ((l = open_lex(filename)) == NULL)
	return 0;
    
    while (lex_nexttoken(l)) {
    /* expecting keyword now */
	switch (l->op) {
	case LXT_MONITOR:
	    if (!read_monitor(muxlist, l)) 
		return 0;
	    break;
	default:
	    parse_error(l, "monitor" );
	    return 0;
	    break;
	}
    }

    /* sanity checks */
    SLIST_FOREACH(mux, muxlist, muxes) {
	if (SLIST_EMPTY(&mux->sl)) {
	    warning("%s: no monitors selected for mux '%s'",
		    l->filename, mux->name);
	    return 0;
	}
    }

    close_lex(l);
    return 1;
}
