/*
 * $Id: sourcetypes.h,v 1.1 2001/09/30 15:04:42 dijkstra Exp $
 *
 * Descriptions of the sources in mon
 */

#ifndef _SOURCETYPES_H
#define _SOURCETYPES_H

/* disk */
total nr of transfers : total seeks : total bytes transferred

/* cpu */
user : nice : system : interrupt : idle

/* mem */
real active : real total : free : [swap used : swap total]

/* ifstat */
ipackets : opackets : ibytes : obytes : imcasts : omcasts : ierrors : oerrors : colls : drops

#endif /*  _SOURCETYPES_H */
