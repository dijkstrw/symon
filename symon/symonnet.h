/*
 * $Id: symonnet.h,v 1.2 2002/03/29 15:17:08 dijkstra Exp $
 *
 * Holds all network functions for mon
 */

#ifndef _MONNET_H
#define _MONNET_H

#include "data.h"

/* prototypes */
__BEGIN_DECLS
void connect2mux __P((struct mux *));
void send_packet __P((struct mux *));
void prepare_packet __P((struct mux *));
void stream_in_packet __P((struct stream *, struct mux *));
void finish_packet __P((struct mux *));
__END_DECLS
#endif /* _MONNET_H */
