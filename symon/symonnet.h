/*
 * $Id: symonnet.h,v 1.1 2002/03/17 13:37:31 dijkstra Exp $
 *
 * Holds all network functions for mon
 */

#ifndef _MONNET_H
#define _MONNET_H

#include "data.h"

/* prototypes */
__BEGIN_DECLS
void connect2mux __P((struct mux *));
void senddata __P((struct mux *));
__END_DECLS
#endif /* _MONNET_H */
