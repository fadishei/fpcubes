#ifndef UTIL_H
#define UTIL_H

#include <math.h>

#define SUP(s, n)	((long)ceil((s)*(n)))

#if DBGLEVEL == 1
#define DEBUG(fmt, ...) fprintf(stderr, "DEBUG: "fmt, ##__VA_ARGS__)
#else
#define DEBUG(fmt, ...) do {} while (0)
#endif

extern int verbosity;
long stat_get_vsize();

#endif