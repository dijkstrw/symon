/*
 * $Id: error.h,v 1.1 2001/08/20 14:40:12 dijkstra Exp $
 */
#ifndef ERROR_H
#define ERROR_H
extern char *__progname;

void fatal(char *, ...);
void warning(char *, ...);
#endif /* ERROR_H */
