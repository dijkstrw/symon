/*
 * $Id: symuxnet.h,v 1.1 2002/03/29 15:18:05 dijkstra Exp $
 *
 * Holds all network functions for monmux
 */

#ifndef _MUXNET_H
#define _MUXNET_H

#include "data.h"

/* prototypes */
__BEGIN_DECLS
int  getmuxsocket __P((struct mux *));
void wait_for_packet __P((int, struct sourcelist *, struct source **, struct monpacket *packet)); 
int  check_crc_packet __P((struct monpacket *));
__END_DECLS
#endif /* _MUXNET_H */
