/* $Id: readconf.c,v 1.13 2002/09/14 15:54:55 dijkstra Exp $ */

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
int read_mux(struct muxlist *mul, struct lex *);
int read_source(struct sourcelist *sol, struct lex *);
__END_DECLS

/* mux <host> (port|:|,| ) <number> */
int 
read_mux(struct muxlist *mul, struct lex *l)
{
    struct mux *m;

    if (! SLIST_EMPTY(mul)) {
	warning("%s:%d: only one mux statement allowed", 
		l->filename, l->cline);
	return 0;
    }
    
    lex_nexttoken(l);
    if (!lookup(l->token)) {
	warning("%s:%d: could not resolve '%s'",
		l->filename, l->cline, l->token );
	return 0;
    }

    m = add_mux(mul, lookup_address);
    m->ip = lookup_ip;

    /* check for port statement */
    lex_nexttoken(l);
    if (l->op == LXT_PORT || l->op == LXT_COLON || l->op == LXT_COMMA)
	lex_nexttoken(l);
    else {
	if (l->type != LXY_NUMBER) {
	    lex_ungettoken(l);
	    m->port = SYMUX_PORT;
	    return 1;
	}
    }

    if (l->type != LXY_NUMBER) {
	parse_error(l, "<number>");
	return 0;
    }

    m->port = l->value;

    return 1;
}
/* source <host> { accept ... | write ... } */
int 
read_source(struct sourcelist *sol, struct lex *l)
{
    struct source *source;
    struct stream *stream;
    char sn[_POSIX2_LINE_MAX];
    char sa[_POSIX2_LINE_MAX];
    int  st;

    /* get hostname */
    lex_nexttoken(l);
    if (!lookup(l->token)) {
	warning("%s:%d: could not resolve '%s'",
		l->filename, l->cline, l->token );
	return 0; 
    }

    source = add_source(sol, lookup_address);
    source->ip = lookup_ip;
    
    EXPECT(l, LXT_BEGIN);
    while (lex_nexttoken(l)) {
	switch (l->op) {
	    /* accept { cpu(x), ... } */
	case LXT_ACCEPT:
	    EXPECT(l, LXT_BEGIN);
	    while (lex_nexttoken(l) && l->op != LXT_END) {
		switch (l->op) {
		case LXT_CPU:
		case LXT_IF:
		case LXT_IO:
		case LXT_MEM:
		case LXT_PF:
		    st = token2type(l->op);
		    strncpy(&sn[0], l->token, _POSIX2_LINE_MAX);

		    /* parse arg */
		    lex_nexttoken(l);
		    if (l->op == LXT_OPEN) {
			lex_nexttoken(l);
			if (l->op == LXT_CLOSE) {
			    parse_error(l, "<stream argument>");
			    return 0;
			}

			strncpy(&sa[0], l->token, _POSIX2_LINE_MAX);
			lex_nexttoken(l);

			if (l->op != LXT_CLOSE) {
			    parse_error(l, ")");
			    return 0;
			}
		    } else {
			lex_ungettoken(l);
			sa[0]='\0';
		    }

		    if ((stream = add_source_stream(source, st, sa)) == NULL) {
			warning("%s:%d: stream %s(%s) redefined", 
				l->filename, l->cline, sn, sa);
			return 0;
		    }

		    break; /* LXT_CPU/LXT_IF/LXT_IO/LXT_MEM/LXT_PF */
		case LXT_COMMA:
		    break;
		default:
		    parse_error(l, "{cpu|mem|if|io|pf}");
		    return 0;

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
	    case LXT_PF:
		st = token2type(l->op);
		strncpy(&sn[0], l->token, _POSIX2_LINE_MAX);
		
		/* parse arg */
		lex_nexttoken(l);
		if (l->op == LXT_OPEN) {
		    lex_nexttoken(l);
		    if (l->op == LXT_CLOSE) {
			parse_error(l, "<stream argument>");
			return 0;
		    }

		    strncpy(&sa[0], l->token, _POSIX2_LINE_MAX);
		    lex_nexttoken(l);
		    if (l->op != LXT_CLOSE) {
			parse_error(l, ")");
			return 0;
		    }
		} else {
		    lex_ungettoken(l);
		    sa[0]='\0';
		}
		
		EXPECT(l, LXT_IN);

		lex_nexttoken(l);
		
		if ((stream = find_source_stream(source, st, sa)) == NULL) {
		    if (strlen(sa)) {
			warning("%s:%d: stream %s(%s) is not accepted for %s", 
				l->filename, l->cline, sn, sa, source->name);
			return 0;
		    } else {
			warning("%s:%d: stream %s is not accepted for %s", 
				l->filename, l->cline, sn, source->name);
			return 0;
		    }
		} else {
		    int fd;
		    /* try filename */
		    if ((fd = open(l->token, O_RDWR | O_NONBLOCK, 0)) == -1) {
			warning("%s:%d: file '%s' cannot be opened", 
				l->filename, l->cline, l->token);
			return 0;
		    } else {
			close(fd);
			stream->file = xstrdup(l->token);
		    }
		}
		break; /* LXT_CPU/LXT_IF/LXT_IO/LXT_MEM/LXT_PF */
	    default:
		parse_error(l, "{cpu|mem|if|io}");
		return 0;
		break;
	    }
	    break; /* LXT_WRITE */
	case LXT_END:
	    return 1;
	default:
	    parse_error(l, "accept|write");
	    return 0;
	}
    }

    warning("%s:%d: missing close brace on source statement", 
	    l->filename, l->cline);

    return 0;
}
/* Read symux.conf */
int  
read_config_file(struct muxlist *mul, 
		 struct sourcelist *sol, 
		 const char *filename)
{
    struct lex *l;
    struct source *source;
    struct stream *stream;

    SLIST_INIT(mul);
    SLIST_INIT(sol);

    if ((l = open_lex(filename)) == NULL)
	return 0;
    
    info("reading configfile '%s'", filename);

    while (lex_nexttoken(l)) {
    /* expecting keyword now */
	switch (l->op) {
	case LXT_MUX:
	    if (!read_mux(mul, l))
		return 0;
	    break;
	case LXT_SOURCE:
	    if (!read_source(sol, l))
		return 0;
	    break;
	default:
	    parse_error(l, "mux|source" );
	    return 0;
	    break;
	}
    }
    
    /* sanity checks */
    if (SLIST_EMPTY(mul)) {
	warning("%s: no mux statement seen",
		l->filename);
	return 0;
    }

    if (SLIST_EMPTY(sol)) {
	warning("%s: no source section seen", 
		l->filename);
	return 0;
    } else {
	SLIST_FOREACH(source, sol, sources) {
	    if (SLIST_EMPTY(&source->sl)) {
		warning("%s: no streams accepted for source '%s'", 
			l->filename, source->name);
		return 0;
	    } else {
		SLIST_FOREACH(stream, &source->sl, streams) {
		    if (stream->file == NULL) {
			warning("%s: no filename specified for stream '%s(%s)' in source '%s'",
				l->filename, type2str(stream->type), stream->args, source->name);
			return 0;
		    }
		}
	    }
	}
    }
    
    close_lex(l);
    
    return 1;
}
