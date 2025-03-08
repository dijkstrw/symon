/*
 * Author: Tatu Ylonen <ylo@cs.hut.fi>
 * Copyright (c) 1995 Tatu Ylonen <ylo@cs.hut.fi>, Espoo, Finland
 *                    All rights reserved
 * Versions of malloc and friends that check their results, and never return
 * failure (they call fatal if they encounter an error).
 *
 * As far as I am concerned, the code I have written for this software
 * can be used freely for any purpose.  Any derived versions of this
 * software must be clearly marked as such, and if the derived work is
 * incompatible with the protocol description in the RFC file, it must be
 * called by a name other than "ssh" or "Secure Shell".
 */

#ifndef _SYMON_LIB_XMALLOC_H
#define _SYMON_LIB_XMALLOC_H

#include <sys/types.h>

/* Like malloc and friends, but these call fatal if something breaks */
void *xmalloc(size_t);
void *xrealloc(void *, size_t);
void xfree(void *);
char *xstrdup(const char *);
#endif                          /* _SYMON_LIB_XMALLOC_H */
