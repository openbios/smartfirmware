#include <stdio.h>

static char buf[256];

void
func(int pcol)
{
    if (buf != NULL)
	fprintf(stderr, "\n");

    if (buf[pcol] == '\t')
	fprintf(stderr, "\n");
}
