/*
 * $Id: lex.h,v 1.5 2002/03/09 16:18:18 dijkstra Exp $
 *
 * Simple lexical analyser.
 * Configuration of keywords and operators should be done here.
 */

#ifndef _LEX_H
#define _LEX_H

#include <sys/cdefs.h>
#include <stdio.h>

/* Tokens known to lex. */
#define LXT_BADTOKEN   0
#define LXT_MEM        1
#define LXT_IF         2
#define LXT_IO         3
#define LXT_CPU        4
#define LXT_SOURCE     5
#define LXT_HUB        6
#define LXT_STREAM     7
#define LXT_PORT       8
#define LXT_ACCEPT     9
#define LXT_WRITE     10
#define LXT_IN        11
#define LXT_BEGIN     12
#define LXT_END       13
#define LXT_COMMA     14
#define LXT_OPEN      15
#define LXT_CLOSE     16
#define LXT_COLON     17

struct lex {
    int curpos;            /* current position in buffer */
    int tokpos;            /* current position in token buffer */
    int endpos;            /* current maxpos in buffer */
    FILE *fh;
    char *buffer;          /* current line(s) */
    int bsize;             /* size of buffer  */
    int cline;             /* current lineno */
    int unget;             /* bool; token pushed back */
    char *token;           /* last token seen */
    long value;            /* value of last token seen, if num */
    int op;                /* opcode of token, if string */
    enum { LXY_STRING, LXY_NUMBER, LXY_UNKNOWN } 
        type; /* type of token in buffer */
    const char *filename;
};

__BEGIN_DECLS
struct lex *open_lex __P((const char *));
void        close_lex __P((struct lex *));
int         lex_nexttoken __P((struct lex *));
void        lex_ungettoken __P((struct lex *));
const char* parse_opcode __P((int));
int         parse_token __P((const char *));
void        parse_error __P((struct lex *, const char *));
__END_DECLS

/* expect(x) = next token in l must be x or return.  */
#define expect(x)    do {                         \
	lex_nexttoken(l);                         \
	if (l->op != (x)) {                       \
	    parse_error(l, parse_opcode((x)));    \
	    return;                               \
	}                                         \
    } while(0);

#endif                                       /* _LEX_H */

