/*
 * $Id: error.h,v 1.2 2001/09/02 19:01:49 dijkstra Exp $
 */
#ifndef _ERROR_H
#define _ERROR_H
extern char *__progname;

void fatal(char *, ...);
void warning(char *, ...);

#endif /* _ERROR_H */
