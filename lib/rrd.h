/*
* RRDTool 1.9.0+ has redefined it's implicit const arguments to
* explicit const arguments. Compiling for older rrdtool versions will
* now result in problems if we do not fix the api interface here.
*/

#ifndef _RRD_H
#define _RRD_H

int rrd_update(int, const char **);
void rrd_clear_error(void);
int rrd_test_error(void);
char *rrd_get_error(void);

#endif
