/*
 * $Id: lex.h,v 1.3 2001/09/20 19:26:33 dijkstra Exp $
 *
 * Simple lexical analyser.
 * Configuration of keywords and operators should be done here.
 */

#ifndef _LEX_H
#define _LEX_H

#include <stdio.h>

/* Tokens known to lex */
typedef enum {
    oBadToken,
    oSource, oHub, 
    oPort, 
    oAccept, oWrite, oIn,
    oCpu, oMem, oIf, oIo,
    oBegin, oEnd, oComma, oOpen, oClose
} OpCodes;

/* Lex structure */
struct lex {
    /* globals */
    FILE *fh;
    char *buffer;          /* current line(s) */
    int bsize;             /* size of buffer  */
    int curpos;            /* current position in buffer */
    int cline;             /* current lineno */
    int endpos;            /* current maxpos in buffer */
    int unget;             /* bool; token pushed back */
    const char *filename;
    /* current token */
    int tokpos;            /* current position in token buffer */
    char *token;           /* last token seen */
    long value;            /* value of last token seen, if num */
    OpCodes op;            /* opcode of token, if string */
    enum { tString, tNumber, tUnknown } type; 
};

/* prototypes */
struct lex *open_lex(const char *);
void close_lex(struct lex *);
int lex_nexttoken(struct lex *);
void lex_ungettoken(struct lex *);
const char* parse_opcode(const OpCodes);
OpCodes parse_token(const char *);
#endif                                       /* _LEX_H */
