/* $Id: readconf.c,v 1.3 2002/04/01 14:44:39 dijkstra Exp $ */

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

#include "readconf.h"
#include "mon.h"
#include "xmalloc.h"
#include "lex.h"
#include "error.h"
#include "net.h"
#include "data.h"

__BEGIN_DECLS
void read_hub(struct lex *);
void read_stream(struct lex *);
__END_DECLS

extern struct muxlist muxlist;

/*
 * hub <host> (port|:|,| ) <number> } 
 */
void 
read_hub(struct lex *l)
{
    struct mux *m;

    if (! SLIST_EMPTY(&muxlist))
	fatal("%s:%d: only one hub statement allowed", 
	      l->filename, l->cline);
    
    lex_nexttoken(l);
    if (!lookup(l->token)) 
	fatal("%s:%d: could not resolve '%s'",
	      l->filename, l->cline, l->token );

    m = add_mux(&muxlist, lookup_address);
    m->ip = lookup_ip;

    lex_nexttoken(l);
    if (l->op == LXT_PORT || l->op == LXT_COLON || l->op == LXT_COMMA )
	lex_nexttoken(l);

    if (l->type != LXY_NUMBER)
	parse_error(l, "<number>");
    
    m->port = l->value;
}
/* stream { cpu() | mem | io() | if() } */
void read_stream(struct lex *l)
{
    struct stream *stream;
    struct mux *mux;

    char sn[_POSIX2_LINE_MAX];
    char sa[_POSIX2_LINE_MAX];
    int  st;

    /* Need a hub statement before we can add streams */
    if (SLIST_EMPTY(&muxlist))
	fatal("%s:%d: stream seen before hub was specified",
	      l->filename, l->cline);
    mux = SLIST_FIRST(&muxlist);

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
		if (l->op == LXT_CLOSE) parse_error(l, "<stream argument>");
		strncpy(&sa[0], l->token, _POSIX2_LINE_MAX);
		lex_nexttoken(l);
		if (l->op != LXT_CLOSE) parse_error(l, ")");
	    } else {
		lex_ungettoken(l);
		sa[0]='\0';
	    }

	    if ((stream = add_mux_stream(mux, st, sa)) == NULL) 
		fatal("%s:%d: stream %s(%s) redefined",
		      l->filename, l->cline, sn, sa);
	    
	    break; /* LXT_CPU/IF/IO/MEM */
	case LXT_COMMA:
	    break;
	default:
	    parse_error(l, "{cpu|mem|if|io}");
	    break;
	}
    }
}
/* Read mon.conf */
void 
read_config_file(const char *filename)
{
    struct lex *l;

    SLIST_INIT(&muxlist);

    l = open_lex(filename);
    
    while (lex_nexttoken(l)) {
    /* expecting keyword now */
	    switch (l->op) {
	    case LXT_HUB:
		read_hub(l);
		break;
	    case LXT_STREAM:
		read_stream(l);
		break;
	    default:
		parse_error(l, "source|hub" );
		break;
	    }
    }

    /* sanity checks */
    if (SLIST_EMPTY(&muxlist))
	fatal("%s: no hub section seen",
	      l->filename);
    else
	if (SLIST_EMPTY(&(SLIST_FIRST(&muxlist))->sl))
	    fatal("%s: no stream section seen",
		  l->filename);

    close_lex(l);
}
