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
#include <dynarray.h>
#include <srchsort.h>
#include <hashtable.h>
#include <backtrace.h>

#include "internals.h"

unsigned long
symboladdr(unsigned long x)
{
    char const *name;
    unsigned long offset;

    if (!symbollookup(x, name, offset))
	return x;

    return x - offset;
}

class CallerTable;

struct Caller
{
    unsigned long addr;
    CallerTable *callers;
    long memused;
    Caller(unsigned long a) { addr = a; callers = NULL; memused = 0; }
    ~Caller() { }
    boolean operator==(unsigned long a) { return addr == a; }
    operator unsigned long() { return addr; }
    Caller *addcaller(unsigned long a, long mem);
};

unsigned
ulhash(unsigned long x)
{
    return x >> 4;
}

declare_hashtable(CallerTable, CallerIter, Caller, unsigned long)
implement_hashtable(CallerTable, CallerIter, Caller, unsigned long, ulhash)
implement_hashtableiter(CallerTable, CallerIter, Caller, unsigned long, ulhash)

Caller *
Caller::addcaller(unsigned long a, long mem)
{
    if (callers == NULL)
    	callers = new CallerTable;

    struct Caller &c = (*callers)[a];
    c.memused += mem;
    return &c;
}

CallerTable basecallers;

void
addstack(Stack_elt &se, long mem)
{
    if (se.stack[0] == NULL)
	return;

    struct Caller *c = &basecallers[symboladdr(se.stack[0])];
    c->memused += mem;

    for (int i = 1; i < STACK_SIZE; i++)
    {
    	if (se.stack[i] == NULL)
	    return;

	c = c->addcaller(symboladdr(se.stack[i]), mem);
    }
}

declare_array(StackInfoPtrVec, Stack_elt *, 4)
implement_array(StackInfoPtrVec, Stack_elt *, 4)

declare_searchsort(Stack_elt *)
implement_searchsort(Stack_elt *)

struct Block_info
{
    int blocksize;
    long mem;
    int freecnt;
    int alloccnt;
    StackInfoPtrVec infovec;
    Block_info(int x) { blocksize = x; freecnt = 0; alloccnt = 0; mem = 0; }
    ~Block_info() { }
    boolean operator==(int x) { return x == blocksize; }
    operator int() { return blocksize; }
};

inline unsigned
inthash(int x)
{
    return x;
}

declare_hashtable(Block_table, Block_iter, Block_info, int)
implement_hashtable(Block_table, Block_iter, Block_info, int, inthash)
implement_hashtableiter(Block_table, Block_iter, Block_info, int, inthash)

declare_searchsort(Block_info *)
implement_searchsort(Block_info *)

int
blockinfocmp(Block_info * const *p1, Block_info * const *p2)
{
    Block_info &bi1 = **p1;
    Block_info &bi2 = **p2;
    long sz1 = (long)bi1.blocksize * (bi1.freecnt + bi1.alloccnt);
    long sz2 = (long)bi2.blocksize * (bi2.freecnt + bi2.alloccnt);

    if (sz1 != sz2)
    	return sz2 - sz1;

    sz1 = (long)bi1.blocksize * bi1.alloccnt;
    sz2 = (long)bi2.blocksize * bi2.alloccnt;

    if (sz1 != sz2)
    	return sz2 - sz1;

    return bi2.blocksize - bi1.blocksize;
}

int
blockinfoszcmp(Block_info * const *p1, Block_info * const *p2)
{
    Block_info &bi1 = **p1;
    Block_info &bi2 = **p2;
    return bi1.blocksize - bi2.blocksize;
}

int
stackcallscmp(Stack_elt * const *p1, Stack_elt * const *p2)
{
    return (*p2)->numcalls - (*p1)->numcalls;
}

int
stackmemcmp(Stack_elt * const *p1, Stack_elt * const *p2)
{
    return (*p2)->totmem - (*p1)->totmem;
}

typedef int (*cmpf)(Stack_elt * const *p1, Stack_elt * const *p2);

int
blkptrcmp(Stack_elt * const *p1, Stack_elt * const *p2)
{
    return (long)*p1 - (long)*p2;
}

boolean
uniq(Stack_elt **arr, int &len, cmpf blkptrcmp)
{
    if (len < 2)
	return TRUE;

    int j = 0;

    for (int i = 1; i < len; i++)
    {
    	int cmp = (*blkptrcmp)(&arr[j], &arr[i]);

	if (cmp > 0)
	    return FALSE;

	if (cmp < 0)
	{
	    j++;

	    if (j != i)
	    	arr[j] = arr[i];
	}
    }

    len = j + 1;
    return TRUE;
}

void
dumpstackelt(Stack_elt &se)
{
    printf("\ttype = 0x%X, file = %s, line = %d, numcalls = %d, totmem = %ld\n",
	    se.type, ((se.file == NULL) ? "<none>" : se.file), se.line,
	    se.numcalls, se.totmem);

    for (int j = STACK_SIZE - 1; j >= 0; j--)
	if (se.stack[j] != NULL)
	    printf("\tstack[%d] = %s\n", j, addrlabel(se.stack[j], TRUE));
}

declare_searchsort(Caller *)
implement_searchsort(Caller *)

int
callercmp(Caller * const *p1, Caller * const *p2)
{
    Caller &c1 = **p1;
    Caller &c2 = **p2;
    return c2.memused - c1.memused;
}

void
tab(int depth)
{
    while (depth)
    	if (depth >= 8)
	{
	    printf("\t");
	    depth -= 8;
	}
	else
	{
	    printf(" ");
	    depth--;
	}
}

void
dumpcallers(CallerTable &ctab, int level = 0)
{
    int num = ctab.size();
    Caller *carray[num];
    int i = 0;

    for (CallerIter ci = ctab; ci; ci++)
    	carray[i++] = &ci();

    qsort(carray, num, callercmp);

    for (i = 0; i < num; i++)
    {
    	Caller &c = *carray[i];
    	tab(level * 4);
	printf("%s = %ld\n", addrlabel(c.addr, TRUE), c.memused);

	if (c.callers != NULL)
	    dumpcallers(*c.callers, level + 1);
    }
}

char *prog = NULL;

int
main(int argc, char **argv)
{
    prog = argv[0];
    char *progname = NULL;
    char *filename = "mem.out";
    int pagesize;
    int numpagegroups;
    Stack_pool *stackpool;
    struct pagegroupinfo **pagegroups;

    if (argc > 1)
	programfilename(argv[1]);

    if (argc > 2)
	filename = argv[2];

    if (!gcread(filename, pagesize, numpagegroups, stackpool, pagegroups))
    {
    	fprintf(stderr, "ftest: cannot open %s\n", filename);
	exit(1);
    }

    if (stackpool != NULL)
    {
    	int i = 0;
	int numstacks = stackpool->size();
	Stack_elt *stacks[numstacks];

    	for (Stack_iter si = *stackpool; si; si++)
	    stacks[i++] = &si();

	qsort(stacks, numstacks, stackmemcmp);

	for (i = 0; i < numstacks; i++)
	{
	    printf("Stack #%d\n", i + 1);
	    dumpstackelt(*stacks[i]);
	    printf("\n");
	}
    }

    int ppg = pagesize / sizeof (struct pageinfo);
    Block_table blocktab;
    long totmem = 0;
    long totallocmem = 0;
    long totfreemem = 0;

    for (int g = 0; g < numpagegroups; g++)
    	if (pagegroups[g] != NULL)
	    for (int p = 0; p < ppg; p++)
	    {
	    	struct pageinfo &pi = pagegroups[g]->pages[p];

		if (pi.blocksize != 0 || pi.numblocks != UNUSED_PAGE)
		{
		    if (pi.blocksize == pagesize)
		    	printf("0x%X: 4096, nb=%d, t=0x%X, a=%d\n", pi.page,
				pi.numblocks, pi.type, pi.atomic);
		    else if (pi.blocksize > 0)
			printf("0x%X: %d\n", pi.page, pi.blocksize);
		    else if (pi.blocksize < 0)
			printf("0x%X: %d+\n", pi.page, pagesize);
		    else
		    	switch (pi.numblocks)
			{
			case FREE_PAGE:
			    printf("0x%X: FREE_PAGE\n", pi.page);
			    break;
			case HIT_PAGE:
			    printf("0x%X: HIT_PAGE\n", pi.page);
			    break;
			case SIMPLE_PAGE:
			    printf("0x%X: SIMPLE_PAGE\n", pi.page);
			    break;
			case HEADER_PAGE:
			    printf("0x%X: HEADER_PAGE\n", pi.page);
			    break;
			case DATA_PAGE:
			    printf("0x%X: DATA_PAGE\n", pi.page);
			    break;
			default:
			    printf("0x%X: ??? (%d)\n", pi.page, pi.numblocks);
			    break;
			}
		}

	    	if (pi.blocksize > 0)
		{
		    int bpp = pagesize / pi.blocksize;
		    int freeblocks = 0;

		    if (bpp > BITSPERLONG)
		    {
			int words = (bpp + BITSPERLONG - 1) / BITSPERLONG;

			for (int i = 0; i < words; i++)
			    freeblocks += bitcount(pi.freemap[i]);
		    }
		    else
			freeblocks = bitcount(pi.freebitmap);

		    int allocblocks = bpp - freeblocks;
		    Block_info &bi = blocktab[pi.blocksize];
		    bi.freecnt += freeblocks;
		    bi.alloccnt += allocblocks;
		    bi.mem += pagesize;
		    totmem += pagesize;
		    totallocmem += (long)pi.blocksize * allocblocks;
		    totfreemem += (long)pi.blocksize * freeblocks;

		    if (pi.snap != NULL)
		    {
			bi.infovec.end() = pi.snap;
			addstack(*pi.snap, allocblocks * pi.blocksize);
		    }
		}
		else if (pi.blocksize == -1)
		{
		    for (int numpages = 1; ; numpages++)
		    {
			int gg = g + (p + numpages) / ppg;
			int pp = (p + numpages) % ppg;

			if (pagegroups[gg] == NULL)
			    break;

			struct pageinfo &pi2 = pagegroups[gg]->pages[pp];

			if (pi2.blocksize != -numpages - 1)
			    break;
		    }

		    Block_info &bi = blocktab[numpages * pagesize];
		    bi.alloccnt++;
		    bi.mem += numpages * pagesize;
		    totmem += numpages * pagesize;
		    totallocmem += numpages * pagesize;

		    if (pi.snap != NULL)
		    {
			bi.infovec.end() = pi.snap;
			addstack(*pi.snap, numpages * pagesize);
		    }
		}
	    }

    int numblocks = blocktab.size();
    Block_info *blockarr[numblocks];
    int i = 0;

    for (Block_iter bi = blocktab; bi; bi++)
    	blockarr[i++] = &bi();

    qsort(blockarr, numblocks, blockinfoszcmp);

    double maxpercent = 0.001;

    for (i = 0; i < numblocks; i++)
    {
    	Block_info &bi = *blockarr[i];
	double percent = bi.mem * 100.0 / totmem;

	if (percent > maxpercent)
	    maxpercent = percent;
    }

    for (i = 0; i < numblocks; i++)
    {
    	Block_info &bi = *blockarr[i];
	double percent = bi.mem * 100.0 / totmem;
	printf("%6d: %4.1f ", bi.blocksize, percent);
	int stars = (int)(percent * 50 / maxpercent);

	while (stars)
	    if (stars > 8)
	    {
	    	printf("*******-");
		stars -= 8;
	    }
	    else
	    {
	    	printf("*");
		stars--;
	    }

	printf("\n");
/*
	int cnt = bi.alloccnt + bi.freecnt;
	long mem = (long)bi.blocksize * cnt;
	long freemem = (long)bi.blocksize * bi.freecnt;
	long allocmem = (long)bi.blocksize * bi.alloccnt;
	printf("blocksize = %d, mem = %ld/%d (%d%%), alloc = %ld/%d (%d%%), free = %ld/%d (%d%%)\n",
		bi.blocksize, mem, cnt, (mem * 100) / totmem,
		allocmem, bi.alloccnt, (allocmem * 100) / totallocmem,
		freemem, bi.freecnt, (freemem * 100) / totfreemem);
*/
/*
	Stack_elt **arr = bi.infovec.getarr();
	int len = bi.infovec.size();
	qsort(arr, len, blkptrcmp);
	uniq(arr, len, blkptrcmp);
	qsort(arr, len, stackmemcmp);

	for (int j = 0; j < len; j++)
	{
	    printf("    stack entry #%d\n", j);
	    dumpstackelt(*arr[j]);
	}

    	printf("\n");
*/
    }

    dumpcallers(basecallers);
}
