/*
 * $Id: error.h,v 1.3 2002/03/09 16:18:18 dijkstra Exp $
 */
#ifndef _ERROR_H
#define _ERROR_H

extern char *__progname;

__BEGIN_DECLS
void fatal __P((char *, ...));
void warning __P((char *, ...));
__END_DECLS

#endif /* _ERROR_H */
