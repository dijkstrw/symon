/* $Id: xmalloc.c,v 1.7 2004/08/07 12:21:36 dijkstra Exp $ */

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

#include <string.h>
#include <stdlib.h>

#include "conf.h"
#include "xmalloc.h"
#include "error.h"

void *
xmalloc(size_t size)
{
    void *ptr;

    if (size == 0)
	fatal("xmalloc: zero size");

    ptr = malloc(size);

    if (ptr == NULL)
	fatal("xmalloc: out of memory (allocating %lu bytes)", (u_long) size);

    return ptr;
}

void *
xrealloc(void *ptr, size_t new_size)
{
    void *new_ptr;

    if (new_size == 0)
	fatal("xrealloc: zero size");

    if (ptr == NULL)
	new_ptr = malloc(new_size);
    else
	new_ptr = realloc(ptr, new_size);

    if (new_ptr == NULL)
	fatal("xrealloc: out of memory (new_size %lu bytes)", (u_long) new_size);

    return new_ptr;
}

void
xfree(void *ptr)
{
    if (ptr == NULL)
	fatal("xfree: NULL pointer given as argument");

    free(ptr);
}

char
*xstrdup(const char *str)
{
    size_t len = strlen(str) + 1;
    char *cp;

    if (len == 0)
	fatal("xstrdup: zero size");

    cp = xmalloc(len);
    strlcpy(cp, str, len);

    return cp;
}
