/*
 * $Id: symux.h,v 1.3 2002/03/09 16:25:36 dijkstra Exp $
 *
 * Structures that are used for datastorage
 */
#ifndef _MONMUX_H
#define _MONMUX_H

#include <sys/types.h>
#include "lex.h"

/* a source is a collection of streams indexed by their host */
struct source {
    char *name;
    struct stream {
	OpCodes type;
	char *args;
	char *file;
	char *lastdata;
	struct stream *next;
    } *streams;
    struct source *next;
};
extern struct source *sources;

#endif /* _MONMUX_H */
