/* $Id: error.c,v 1.5 2002/03/31 14:27:46 dijkstra Exp $ */

/*
 * Copyright (c) 2001-2002 Willem Dijkstra
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 *    - Redistributions of source code must retain the above copyright
 *      notice, this list of conditions and the following disclaimer.
 *    - Redistributions in binary form must reproduce the above
 *      copyright notice, this list of conditions and the following
 *      disclaimer in the documentation and/or other materials provided
 *      with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
 * COPYRIGHT HOLDERS OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
 * ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *
 */
#include <sys/cdefs.h>

#include <stdarg.h>
#include <stdio.h>

#include "error.h"

/* Output error and exit */
__dead void 
fatal(char *str, ...)
{
    va_list ap;
    
/*    fprintf(stderr, "%s:", __progname); */
    va_start(ap, str);
    vfprintf(stderr, str, ap);
    va_end(ap);
        
    exit( 1 );
}
/* Warn and continue */
void 
warning(char *str, ...)
{
    va_list ap;
    
/*    fprintf(stderr, "%s:", __progname); */
    va_start(ap, str);
    vfprintf(stderr, str, ap);
    va_end(ap);
}
/* Inform and continue */
void 
inform(char *str, ...)
{
    va_list ap;
    
/*    fprintf(stdout, "%s:", __progname); */
    va_start(ap, str);
    vfprintf(stdout, str, ap);
    va_end(ap);
}


