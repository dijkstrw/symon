/*
 * $Id: symux.h,v 1.2 2001/09/20 19:26:33 dijkstra Exp $
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
struct hub {
    char *name;
    u_int32_t ip;
    int port;
};

extern struct hub *hub;
extern struct source *sources;

#endif /* _MONMUX_H */
