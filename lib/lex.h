/*
 * $Id: lex.h,v 1.1 2001/08/20 14:40:12 dijkstra Exp $
 *
 * Simple lexical analyser.
 * Configuration of keywords and operators should be done here.
 */

#ifndef LEX_H
#define LEX_H

/* Tokens known to lex */
typedef enum {
    oBadToken,
    oSource, 
    oAccept, oWrite, oIn,
    oCpu, oMem, oIf, oIo,
    oBegin, oEnd, oComma, oOpen, oClose
} OpCodes;
static struct {
    const char *name;
    OpCodes opcode;
} keywords[] = {
    { "source", oSource },
    { "accept", oAccept },
    { "write", oWrite },
    { "in", oIn },
    { "cpu", oCpu },
    { "mem", oMem },
    { "if", oIf },
    { "io", oIo },
    { "{", oBegin },
    { "}", oEnd },
    { ",", oComma },
    { "(", oOpen },
    { ")", oClose },
    { NULL, 0 }
};
#define KW_OPS "{},()"

/* Lex structure */
struct lex {
    /* globals */
    FILE *fh;
    char *buffer;          /* current line(s) */
    int bsize;             /* size of buffer  */
    int curpos;            /* current position in buffer */
    int cline;             /* current lineno */
    int endpos;            /* current maxpos in buffer */
    char *filename;
    /* current token */
    int tokpos;            /* current position in token buffer */
    char *token;           /* last token seen */
    long value;            /* value of last token seen, if num */
    OpCodes op;            /* opcode of token, if string */
    enum { tString, tNumber } type; 
};

/* prototypes */
struct lex *open_lex(char *);
void close_lex(struct lex *);
int lex_nexttoken(struct lex *);

#endif                                       /* LEX_H */
