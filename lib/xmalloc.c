/*
 * $Id: xmalloc.c,v 1.3 2002/03/09 16:18:19 dijkstra Exp $
 */
#include <err.h>
#include <strings.h>
#include <stdlib.h>

#include "xmalloc.h"

void *xmalloc(size)
    size_t size;
{
    void *ptr;
    
    if (size == 0)
	errx(1, "xmalloc: zero size");
    ptr = malloc(size);
    if (ptr == NULL)
	errx(1, "xmalloc: out of memory (allocating %lu bytes)", (u_long) size);
    return ptr;
}

void *xrealloc(ptr, new_size)
    void *ptr;
    size_t new_size;
{
    void *new_ptr;
    
    if (new_size == 0)
	errx(1, "xrealloc: zero size");
    if (ptr == NULL)
	new_ptr = malloc(new_size);
    else
	new_ptr = realloc(ptr, new_size);
    if (new_ptr == NULL)
	errx(1, "xrealloc: out of memory (new_size %lu bytes)", (u_long) new_size);
    return new_ptr;
}

void xfree(ptr)
    void *ptr;
{
    if (ptr == NULL)
	errx(1, "xfree: NULL pointer given as argument");
    free(ptr);
}

char *xstrdup(str)
    const char *str;
{
    size_t len = strlen(str) + 1;
    char *cp;

    if (len == 0)
	errx(1, "xstrdup: zero size");
    cp = xmalloc(len);
    strlcpy(cp, str, len);
    return cp;
}
