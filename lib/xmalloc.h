/*
 * $Id:
 */

#ifndef _XMALLOC_H
#define _XMALLOC_H

#include <sys/types.h>
/* Like malloc and friends, but these call errx if something breaks */
void   *xmalloc(size_t);
void   *xrealloc(void *, size_t);
void    xfree(void *);
char   *xstrdup(const char *);

#endif /* _XMALLOC_H */
