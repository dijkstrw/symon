/* 
 * $Id: error.c,v 1.1 2001/08/20 14:40:12 dijkstra Exp $
 *
 * Routines that show errors
 */
#include <stdarg.h>
#include <stdio.h>
#include "error.h"

void fatal(char *str, ...)
{
    va_list ap;
    
    fprintf(stderr, "%s:", __progname);
    va_start(ap, str);
    vfprintf(stderr, str, ap);
    va_end(ap);
        
    exit( 1 );
}
void warning(char *str, ...)
{
    va_list ap;
    
    fprintf(stderr, "%s:", __progname);
    va_start(ap, str);
    vfprintf(stderr, str, ap);
    va_end(ap);
}
