/*
 * $Id: readconf.h,v 1.1 2002/03/17 13:37:31 dijkstra Exp $
 *
 */
#ifndef _READCONF_H
#define _READCONF_H

#include <sys/cdefs.h>

#include "lex.h"

/* Monitor subsystem structure */
struct monm {
    int type;
    void  (*init)(char *);
    char* (*get)(char *);
};

__BEGIN_DECLS
void read_config_file __P((const char *));
__END_DECLS
#endif /* _READCONF_H */


