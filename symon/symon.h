/*
 * $Id: symon.h,v 1.1 2001/04/28 16:06:44 dijkstra Exp $
 *
 * Mon - a minimal system monitor
 * 
 * General (global) definitions
 */

/* Log base 2 of 1024 is 10 (2^10=1024) */
#define LOG1024		10

/* Monitor subsystem structure */
struct monm {
  char* type;
  char* arg;
  char* file;
  void  (*init)(char *);
  char* (*get)(char *);
  int   interval;
  int   sleep;
};

/* cpu.c */
extern void init_cpu(char *);
extern char *get_cpu(char *);
/* mem.c */
extern void init_mem(char *);
extern char *get_mem(char *);
/* netstat.c */
extern void init_ns(char *);
extern char *get_ns(char *);
