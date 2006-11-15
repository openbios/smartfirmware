/*  Copyright (c) 1992-2005 CodeGen, Inc.  All rights reserved.
 *
 *  Redistribution and use in source and binary forms, with or without
 *  modification, are permitted provided that the following conditions
 *  are met:
 *  1. Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 *  2. Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 *  3. Redistributions in any form must be accompanied by information on
 *     how to obtain complete source code for the CodeGen software and any
 *     accompanying software that uses the CodeGen software.  The source code
 *     must either be included in the distribution or be available for no
 *     more than the cost of distribution plus a nominal fee, and must be
 *     freely redistributable under reasonable conditions.  For an
 *     executable file, complete source code means the source code for all
 *     modules it contains.  It does not include source code for modules or
 *     files that typically accompany the major components of the operating
 *     system on which the executable file runs.  It does not include
 *     source code generated as output by a CodeGen compiler.
 *
 *  THIS SOFTWARE IS PROVIDED BY CODEGEN AS IS AND ANY EXPRESS OR IMPLIED
 *  WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 *  OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 *  IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 *  INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 *  NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 *  DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 *  THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 *  (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 *  THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 *  (Commercial source/binary licensing and support is also available.
 *   Please contact CodeGen for details. http://www.codegen.com/)
 */


#include <stdlib.h>
#include <stdlibx.h>
#include <stdio.h>
#include <stdiox.h>
#include <string.h>
#include <stringx.h>
#include <hashtable.h>
#include <dynarray.h>
#include <bitset.h>

class Count
{
public:
    int _size;
    int _count;
    Count(int sz) { _size = sz; _count = 0; }
    Count(Count const &c) { _size = c._size; _count = c._count; }
    ~Count() { }
    operator int() const { return _size; }
    boolean operator==(int sz) const { return _size == sz; }
    boolean operator==(Count const &c) const { return _size == c._count; }
    void operator=(Count const &c) { _size = c._size; _count = c._count; }
};

unsigned long
counthash(int c)
{
    return c;
}

declare_hashtable(CountTable, CountIter, Count, int)
implement_hashtable(CountTable, CountIter, Count, int, counthash)

declare_array(IntVec, int, 4096)
implement_array(IntVec, int, 4096)

int
intcmp(const void *p1, const void *p2)
{
    int i1 = *(int *)p1;
    int i2 = *(int *)p2;
    return i1 - i2;
}

int
bucket(int size)
{
    if (size <= 1) return 0;
    if (size <= 2) return 3;
    if (size <= 3) return 6;
    if (size <= 4) return 9;
    if (size <= 5) return 10;
    if (size <= 6) return 12;
    if (size <= 7) return 13;
    if (size <= 9) return 14;
    if (size <= 10) return 15;
    if (size <= 12) return 16;
    if (size <= 14) return 17;
    if (size <= 16) return 18;
    if (size <= 18) return 19;
    if (size <= 22) return 20;
    if (size <= 25) return 21;
    if (size <= 29) return 22;
    if (size <= 34) return 23;
    if (size <= 40) return 24;
    if (size <= 46) return 25;
    if (size <= 54) return 26;
    if (size <= 63) return 27;
    if (size <= 74) return 28;
    if (size <= 86) return 29;
    if (size <= 100) return 30;
    if (size <= 117) return 31;
    if (size <= 136) return 32;
    if (size <= 158) return 33;
    if (size <= 185) return 34;
    if (size <= 215) return 35;
    if (size <= 251) return 36;
    if (size <= 293) return 37;
    if (size <= 341) return 38;
    if (size <= 398) return 39;
    if (size <= 464) return 40;
    if (size <= 541) return 41;
    if (size <= 631) return 42;
    if (size <= 736) return 43;
    if (size <= 858) return 44;
    if (size <= 1000) return 45;
    if (size <= 1170) return 46;
    if (size <= 1360) return 47;
    if (size <= 1580) return 48;
    if (size <= 1850) return 49;
    if (size <= 2150) return 50;
    if (size <= 2510) return 51;
    if (size <= 2930) return 52;
    if (size <= 3410) return 53;
    if (size <= 3980) return 54;
    if (size <= 4640) return 55;
    if (size <= 5410) return 56;
    if (size <= 6310) return 57;
    if (size <= 7360) return 58;
    if (size <= 8580) return 59;
    if (size <= 10000) return 60;
    if (size <= 11700) return 61;
    if (size <= 13600) return 62;
    return 63;
}

int
main(int argc, char **argv)
{
    char *str;
    IntVec used;
    CountTable hist;
    long allocs = 0;
    long frees = 0;
    long reallocs = 0;
    long memallocated = 0;
    long memfreed = 0;
    long meminuse = 0;
    long maxmeminuse = 0;
    FILE *fp = stdin;
    boolean dump = FALSE;
    int filearg = 1;

    if (argc > 1 && strcmp(argv[1], "-dump") == 0)
    {
	dump = TRUE;
	filearg = 2;
    }

    if (argc > filearg)
    {
    	fp = fopen(argv[filearg], "r");

	if (fp == NULL)
	{
	    fprintf(stderr, "%s: cannot open %s\n", argv[0], argv[1]);
	    exit(1);
	}
    }

    while ((str = xgets(fp)) != NULL)
    {
    	int argc;
	const char **argv = strsplit(str, " \t", TRUE, &argc);

	if (argc == 0)
	    continue;

	switch (argv[0][0])
	{
	case 'm':
	    {
		if (argc != 3)
		{
		    fprintf(stderr, "expected m id size\n");
		    break;
		}

		int id = atoi(argv[1]);
		int sz = atoi(argv[2]);

		allocs++;
		used[id] = sz;
		memallocated += sz;
		meminuse += sz;

		if (meminuse > maxmeminuse)
		    maxmeminuse = meminuse;

		hist[sz]._count++;
	    }

	    break;
	case 'f':
	    {
		if (argc != 2)
		{
		    fprintf(stderr, "expected f id\n");
		    break;
		}

		int id = atoi(argv[1]);
		int sz = used[id];

		frees++;
		memfreed += sz;
		meminuse -= sz;
	    }

	    break;
	case 'r':
	    {
		if (argc != 3)
		{
		    fprintf(stderr, "expected r id newsize\n");
		    break;
		}

		int id = atoi(argv[1]);
		int sz = used[id];
		int nsz = atoi(argv[2]);

		reallocs++;
		used[id] = nsz;
		memfreed += sz;
		memallocated += nsz;
		meminuse -= sz;
		meminuse += nsz;

		if (meminuse > maxmeminuse)
		    maxmeminuse = meminuse;

		hist[nsz]._count++;
	    }

	    break;
	case '#':
	    break;
	default:
	    fprintf(stderr, "unrecognized option \"%s\"\n", argv[0]);
	    break;
	}
    }

    printf("allocs = %ld\n", allocs);
    printf("frees = %ld\n", frees);
    printf("reallocs = %ld\n", reallocs);
    printf("memallocated = %ld\n", memallocated);
    printf("memfreed = %ld\n", memfreed);
    printf("meminuse = %ld\n", meminuse);
    printf("maxmeminuse = %ld\n", maxmeminuse);
    int numsizes = hist.size();
    int *sizes = new int[numsizes];
    int i = 0;
    int maxcount = 1;

    for (CountIter iter = hist; iter; iter++)
    {
    	int sz = iter()._size;
	int count = iter()._count;
	sizes[i++] = sz;

	if (count > maxcount)
	    maxcount = count;
    }

    qsort(sizes, numsizes, sizeof (int), intcmp);

    if (dump)
    {
	printf("numsizes = %d\n", numsizes);
	printf("maxsize = %d\n", sizes[numsizes - 1]);
    }
    else
    {
	printf("\n   size |  count | percent of blocks\n");
	printf("===============================================================================\n");
    }

    char *stars = "***************************************"
	"****************************";
    int starslen = strlen(stars);

    for (i = 0; i < numsizes; i++)
    {
	int sz = sizes[i];
	int nstars = (hist[sz]._count * 59) / maxcount;

	if (dump)
	    printf("%d %d\n", sz, hist[sz]._count);
	else
	    printf("%7d | %6d | %s\n", sz, hist[sz]._count, 
		    &stars[starslen - nstars]);
    }

    if (!dump)
	printf("\n\n");

    int hist2[64];

    for (i = 0; i < 64; i++)
    	hist2[i] = 0;

    for (i = 0; i < numsizes; i++)
    {
    	int sz = sizes[i];
	int count = hist[sz]._count;
	hist2[bucket(sz)] += count;
    }

    maxcount = 1;

    for (i = 0; i < 64; i++)
    	if (hist2[i] > maxcount)
	    maxcount = hist2[i];

    if (dump)
    {
	printf("numbuckets = %d\n", 64);
	printf("maxcount = %d\n", maxcount);
    }

    for (i = 0; i < 64; i++)
	if (dump)
	    printf("%d\n", hist2[i]);
	else
	    hist2[i] = (hist2[i] * 20) / maxcount;

    if (!dump)
    {
	for (int c = 20; c >= 1; c--)
	{
	    printf("\t|");

	    for (i = 0; i < 64; i++)
		if (hist2[i] >= c)
		    printf("*");
		else
		    printf(" ");

	    printf("\n");
	}

//	printf("--------+:--------:--------:--------:--------:--------:--------:--------:\n");
//	printf("         1        10      100      1000    10000     10e5     10e6     10e7\n");

	printf("--------+:--------------:--------------:--------------:--------------:---\n");
	printf("         1              10            100            1000          10000\n");
    }

    return 0;
}
