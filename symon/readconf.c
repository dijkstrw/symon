/* $Id: readconf.c,v 1.4 2002/04/01 20:15:59 dijkstra Exp $ */

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
int read_hub(struct muxlist *, struct lex *);
int read_stream(struct muxlist *,struct lex *);
__END_DECLS

/*
 * hub <host> (port|:|,| ) <number> } 
 */
int
read_hub(struct muxlist *muxlist, struct lex *l)
{
    struct mux *m;

    if (! SLIST_EMPTY(muxlist)) {
	warning("%s:%d: only one hub statement allowed", 
		l->filename, l->cline);
	return 0;
    }

    lex_nexttoken(l);
    if (!lookup(l->token)) {
	warning("%s:%d: could not resolve '%s'",
		l->filename, l->cline, l->token );
	return 0;
    }

    m = add_mux(muxlist, lookup_address);
    m->ip = lookup_ip;

    lex_nexttoken(l);
    if (l->op == LXT_PORT || l->op == LXT_COLON || l->op == LXT_COMMA )
	lex_nexttoken(l);

    if (l->type != LXY_NUMBER) {
	parse_error(l, "<number>");
	return 0;
    }

    m->port = l->value;
    return 1;
}
/* stream { cpu() | mem | io() | if() } */
int
read_stream(struct muxlist *muxlist, struct lex *l)
{
    struct stream *stream;
    struct mux *mux;

    char sn[_POSIX2_LINE_MAX];
    char sa[_POSIX2_LINE_MAX];
    int  st;

    /* Need a hub statement before we can add streams */
    if (SLIST_EMPTY(muxlist)) {
	warning("%s:%d: stream seen before hub was specified",
		l->filename, l->cline);
	return 0;
    }

    mux = SLIST_FIRST(muxlist);

    EXPECT(LXT_BEGIN);
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

	    if ((stream = add_mux_stream(mux, st, sa)) == NULL) {
		warning("%s:%d: stream %s(%s) redefined",
			l->filename, l->cline, sn, sa);
		return 0;
	    }
	    
	    break; /* LXT_CPU/IF/IO/MEM */
	case LXT_COMMA:
	    break;
	default:
	    parse_error(l, "{cpu|mem|if|io}");
	    return 0;
	    break;
	}
    }

    return 1;
}
/* Read mon.conf */
int
read_config_file(struct muxlist *muxlist, const char *filename)
{
    struct lex *l;

    SLIST_INIT(muxlist);

    if ((l = open_lex(filename)) == NULL)
	return 0;
    
    while (lex_nexttoken(l)) {
    /* expecting keyword now */
	    switch (l->op) {
	    case LXT_HUB:
		if (!read_hub(muxlist, l)) 
		    return 0;
		break;
	    case LXT_STREAM:
		if (!read_stream(muxlist, l))
		    return 0;
		break;
	    default:
		parse_error(l, "source|hub" );
		return 0;
		break;
	    }
    }

    /* sanity checks */
    if (SLIST_EMPTY(muxlist)) {
	warning("%s: no hub section seen",
		l->filename);
	return 0;
    }
    else
	if (SLIST_EMPTY(&(SLIST_FIRST(muxlist))->sl)) {
	    warning("%s: no stream section seen",
		    l->filename);
	    return 0;
	}

    close_lex(l);

    return 1;
}
