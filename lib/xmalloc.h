/*
 * $Id:
 */

#ifndef _XMALLOC_H
#define _XMALLOC_H

#include <sys/cdefs.h>
#include <sys/types.h>

/* Like malloc and friends, but these call errx if something breaks */
__BEGIN_DECLS
void   *xmalloc __P((size_t));
void   *xrealloc __P((void *, size_t));
void    xfree __P((void *));
char   *xstrdup __P((const char *));
__END_DECLS
#endif /* _XMALLOC_H */
