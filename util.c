#include <stdarg.h>
#include <stdio.h>
#include <unistd.h>
#include "util.h"

long stat_get_vsize()
{
	long s = -1;
	FILE *f = fopen("/proc/self/statm", "r");
	if (!f)
		return -1;
	if (fscanf(f, "%ld", &s)!=1)
		fprintf(stderr, "can not read statm\n");
	fclose (f);
	return s*getpagesize();
}
