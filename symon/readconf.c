/*
 * $Id: readconf.c,v 1.1 2002/03/17 13:37:31 dijkstra Exp $
 *
 * Parse mon.conf style configuration files 
 * 
 */
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <limits.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/queue.h>

#include "readconf.h"
#include "mon.h"
#include "xmalloc.h"
#include "lex.h"
#include "error.h"
#include "net.h"
#include "data.h"

extern struct muxlist muxlist;

/*
 * hub <host> (port|:|,| ) <number> } 
 */
void read_hub(l)
    struct lex *l;
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
/*
 * stream { cpu() | mem | io() | if() }
 */
void read_stream(l)
    struct lex *l;
{
    struct stream *stream;
    struct mux *mux;

    char sn[_POSIX2_LINE_MAX];
    char sa[_POSIX2_LINE_MAX];
    int  st;

    /* Need a hub statement before we can add streams */
    if (SLIST_EMPTY(&muxlist))
	fatal("%s:%d: Stream seen before hub was specified.\n",
	      l->filename, l->cline);
    mux = SLIST_FIRST(&muxlist);

    expect(LXT_BEGIN);
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
		fatal("%s:%d: stream %s(%s) redefined.\n",
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
void read_config_file(const char *filename)
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
    if (SLIST_EMPTY(&muxlist)) {
	fatal("%s: No hub section seen\n",
	      l->filename);
    } else {
	if (SLIST_EMPTY(&(SLIST_FIRST(&muxlist))->sl))
	    fatal("%s: No stream section seen\n",
		  l->filename);
    }

    close_lex(l);
}
