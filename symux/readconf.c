/* $Id: readconf.c,v 1.7 2002/03/31 14:27:50 dijkstra Exp $ */

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

#include "xmalloc.h"
#include "lex.h"
#include "error.h"
#include "net.h"
#include "data.h"

__BEGIN_DECLS
void read_hub(struct lex *);
void read_source(struct lex *);
__END_DECLS

extern struct muxlist muxlist;
extern struct sourcelist sourcelist;

/* hub <host> (port|:|,| ) <number> */
void 
read_hub(struct lex *l)
{
    struct mux *m;

    if (! SLIST_EMPTY(&muxlist))
	fatal("%s:%d: Only one hub statement allowed.\n", 
	      l->filename, l->cline);
    
    lex_nexttoken(l);
    if (!lookup(l->token)) 
	fatal("%s:%d: Could not resolve '%s'.\n",
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
/* source <host> { accept ... | write ... } */
void 
read_source(struct lex *l)
{
    struct source *source;
    struct stream *stream;
    char sn[_POSIX2_LINE_MAX];
    char sa[_POSIX2_LINE_MAX];
    int  st;

    /* get hostname */
    lex_nexttoken(l);
    if (!lookup(l->token))
	return; 
    source = add_source(&sourcelist, lookup_address);
    source->ip = lookup_ip;

    EXPECT(LXT_BEGIN);
    while (lex_nexttoken(l)) {
	switch (l->op) {
	    /* accept { cpu(x), ... } */
	case LXT_ACCEPT:
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

		    if ((stream = add_source_stream(source, st, sa)) == NULL) {
			fatal("%s:%d: stream %s(%s) redefined.\n", 
			      l->filename, l->cline, sn, sa);
		    }

		    break; /* LXT_CPU/LXT_IF/LXT_IO/LXT_MEM */
		case LXT_COMMA:
		    break;
		default:
		    parse_error(l, "{cpu|mem|if|io}");
		    break;
		}
	    }
	    break; /* LXT_ACCEPT */
	    /* write cpu(0) in "filename" */
	case LXT_WRITE:
	    lex_nexttoken(l);
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
		
		EXPECT(LXT_IN);

		lex_nexttoken(l);
		
		if ((stream = find_source_stream(source, st, sa)) == NULL) {
		    if ( strlen(sa) )
			fatal("%s:%d: stream %s(%s) is not accepted for %s.\n", 
			      l->filename, l->cline, sn, sa, source->name);
		    else
			fatal("%s:%d: stream %s is not accepted for %s.\n", 
			      l->filename, l->cline, sn, source->name);
		} else {
		    int fd;
		    /* try filename */
		    if ((fd = open(l->token, O_RDWR | O_NONBLOCK, 0)) == -1) {
			fatal("%s:%d: file '%s' cannot be opened.\n", 
			      l->filename, l->cline, l->token);
		    } else {
			close( fd );
			stream->file = xstrdup(l->token);
		    }
		}
		break; /* LXT_CPU/LXT_IF/LXT_IO/LXT_MEM */
	    default:
		parse_error(l, "{cpu|mem|if|io}");
		break;
	    }
	    break; /* LXT_WRITE */
	case LXT_END:
	    return;
	default:
	    parse_error(l, "accept|write");
	    return;
	}
    }
}
/* Read monmux.conf */
void 
read_config_file(const char *filename)
{
    struct lex *l;

    SLIST_INIT(&muxlist);
    SLIST_INIT(&sourcelist);

    l = open_lex(filename);
    
    while (lex_nexttoken(l)) {
    /* expecting keyword now */
	switch (l->op) {
	case LXT_HUB:
	    read_hub(l);
	    break;
	case LXT_SOURCE:
	    read_source(l);
	    break;
	default:
	    parse_error(l, "source|hub" );
	    break;
	}
    }
    
    /* sanity checks */
    if (SLIST_EMPTY(&muxlist))
	fatal("%s: No hub section seen",
	      l->filename);

    if (SLIST_EMPTY(&sourcelist))
	fatal("%s: No source section seen", 
	      l->filename);
    else
	if (SLIST_EMPTY(&(SLIST_FIRST(&sourcelist))->sl))
	    fatal("%s: No streams accepted", 
		  l->filename);
    
    close_lex(l);
}




