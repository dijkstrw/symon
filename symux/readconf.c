/*
 * $Id: readconf.c,v 1.1 2001/08/20 14:40:12 dijkstra Exp $
 *
 * Parse monmux.conf style configuration files 
 * 
 */
#include <stdarg.h>
#include <stdio.h>
#include "lex.h"
#include "error.h"

void parse_error(struct lex *l,char *s, ...)
{
    va_list ap;

    va_start(ap, s);
    warning("%s:%d:%s ('%.5s')\n", l->filename, l->cline, s, l->token);
    va_end(ap);
}

/*
 * source <host> { accept ... | write ... }
 */
void read_source(struct lex *l){

    lex_nexttoken(l);
    lex_nexttoken(l);
    
    if (l->op != oBegin) {
	parse_error(l, "Expected a {");
	return;
    }
    
    while (lex_nexttoken(l)) {
	switch (l->op) {
	case oAccept:
	    break;
	case oWrite:
	    break;
	default:
	    parse_error(l, "Expected accept|write");
	    return;
	}
    }
}
void read_config_file(const char *filename)
{
    struct lex *l;

    l = open_lex(filename);
    
    while (lex_nexttoken(l)) {
	/* expecting keyword now */
	switch (l->op) {
	case oSource:
	    read_source(l);
	    break;
	default:
	    parse_error(l, "Expected a keyword");
	    break;
	}
    }
    
    close_lex(l);
    exit(1);
}
