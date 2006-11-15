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


#ifdef linux
#  define NO_FIX_MALLOC
#include <unistd.h>
#define PAGESIZE 4096
#endif

#include <stdlib.h>
#include <stdlibx.h>
#include <srchsort.h>

#ifdef cfree
#undef cfree
#endif

#ifdef _AUX_SOURCE
#include <sys/mmu.h>
inline int getpagesize() { return PAGESIZE; }
#endif

#ifdef macintosh
#include <memory.h>
#define PAGESIZE 4096
#define PAGEHUNK 16*PAGESIZE
inline int getpagesize() { return PAGESIZE; }
inline void *sbrk(size_t len) { return NewPtr(len); }

void *
operator new(size_t len)
{
    return malloc(len);
}

void
operator delete(void *p)
{
    if (p)
	free(p);
}
#endif

#ifndef PAGESIZE
#define PAGESIZE 4096
inline int getpagesize() { return PAGESIZE; }
extern "C" { void *sbrk(size_t len); }
#endif

struct block
{
    struct block *next;
    unsigned size;
};

#define MINPAGES	1
#define MINALIGN	(2 * sizeof (unsigned))
#define MINBLOCKSIZE	(2 * sizeof (struct block))
#define MAXTHUNKSIZE	(PAGESIZE / 4)
#define NUMTHUNKS	(MAXTHUNKSIZE / MINALIGN)

static unsigned thunksize = 96;
static struct block *thunks[NUMTHUNKS + 1];
static struct block *freelist = NULL;
static struct block *templist = NULL;
static int templistsize = 0;
static unsigned pagesize = 0;
static unsigned minblocksize = 0;
static unsigned freespace = 0;
static unsigned freeblocks = 0;
static unsigned allocspace = 0;
static unsigned allocblocks = 0;
static void *arenaend = NULL;
static unsigned arenasize = 0;

static void
init()
{
    pagesize = getpagesize();

    if (pagesize == 0)
	pagesize = 4096;

    minblocksize = pagesize * MINPAGES;
    char *e = getenv("MALLOC_THUNK");

    if (e != NULL && *e != '\0')
	thunksize = atoi(e);
    
    if (thunksize > MAXTHUNKSIZE)
	thunksize = MAXTHUNKSIZE;
}

inline void doinit() { if (pagesize == 0) init(); }

static int
expanddatasegment(unsigned blocksize)
{
    doinit();
    blocksize = ((blocksize + minblocksize - 1) / minblocksize) * minblocksize;

#ifdef macintosh
    if (blocksize < PAGEHUNK)
	blocksize = PAGEHUNK;
    else
	blocksize += PAGEHUNK;
#endif
    
    struct block *blk = (struct block *)sbrk(blocksize);

    if ((int)blk == -1 || blk == NULL)
	return 0;

    blk->next = NULL;
    blk->size = blocksize;
    struct block *end = (struct block *)freelist;

    while (end != NULL && end->next != NULL)
	end = end->next;

    if (end == NULL)
    {
	freelist = blk;
	freeblocks++;
    }
    else if ((struct block *)((char *)end + end->size) == blk)
	end->size += blocksize;
    else
    {
	end->next = blk;
	freeblocks++;
    }

    freespace += blocksize;
    arenasize += blocksize;
    arenaend = (void *)((char *)blk + blocksize);
    return 1;
}

void *
calloc(unsigned elems, unsigned elsize)
{
    unsigned size = elems * elsize;
    void *p = malloc(size);

    if (p == NULL)
	return NULL;

    memset(p, 0, size);
    return p;
}

void
cfree(void *p)
{
    free(p);
}

static void
free2(void *p)
{
    struct block *prev = NULL;
    struct block *us = (struct block *)((char *)p - MINALIGN);
    unsigned size = ((unsigned *)us)[0];
    us->size = size;

    for (struct block *blk = freelist; blk != NULL;
	    prev = blk, blk = blk->next)
	if (blk > us)
	    break;

    if (prev == NULL)
    {
	us->next = freelist;
	freelist = us;
	freeblocks++;
    }
    else if ((struct block *)((char *)prev + prev->size) == us)
    {
	prev->size += size;
	us = prev;
    }
    else
    {
	us->next = prev->next;
	prev->next = us;
	freeblocks++;
    }

    if ((struct block *)((char *)us + us->size) == us->next)
    {
	us->size += us->next->size;
	us->next = us->next->next;
	freeblocks--;
    }

    freespace += size;
    allocspace -= size;
    allocblocks--;
}

void
free(void *p)
{
#ifndef macintosh
    extern char end[];

    if ((char*)p <= end)
    {
	if (p == NULL)
	    crash("free(): attempt to free NULL pointer");

	crash("free(): attempt to free a pointer to code or static data");
    }
#endif

    struct block *us = (struct block *)((char *)p - MINALIGN);
    unsigned size = *(unsigned *)us;

    if (thunksize && size < thunksize)
    {
	size /= MINALIGN;
	us->next = thunks[size];
	thunks[size] = us;
	return;
    }

    us->size = size;
    us->next = templist;
    templist = us;
    templistsize++;
}

declare_searchsort(struct block *)
implement_searchsort(struct block *)

static int
blockaddrcmp(struct block * const *p1, struct block * const *p2)
{
    return *p1 - *p2;
}

static void
merge(boolean nomalloc = FALSE)
{
    static boolean locked = FALSE;

    if (templistsize == 0 || locked)
	return;

    // prevent any calls to malloc that we make from recursing infinitely
    locked = TRUE;

    // handle simple  case of only one element
    if (templistsize == 1)
    {
    	((unsigned *)templist)[0] = templist->size;
	free2((char *)templist + MINALIGN);
	templist = NULL;
	templistsize = 0;
	locked = FALSE;
	return;
    }

    // find largest block and its size
    unsigned maxsize = 0;
    struct block *maxb = NULL;
    int blocks = templistsize;
    unsigned minsize = blocks * sizeof (struct block *) + sizeof (struct block);
    struct block *b;

    for (b = templist; b != NULL; b = b->next)
    	if (b->size > maxsize)
	{
	    maxsize = b->size;
	    maxb = b;

	    if (maxsize >= minsize)
	    	break;
	}

    if (maxb == NULL)
    	crash("malloc:merge(): templist corrupt");

    int numptrs = (maxsize - sizeof (struct block)) / sizeof(struct block *);

    if (!nomalloc && numptrs * 3 < blocks)
    {
    	// free blocks are way too small allocate a block to hold things
	void *t = malloc((blocks + 1) * sizeof(struct block *));
	
	if (t != NULL)
	{
	    maxb = (struct block *)((char *)t - MINALIGN);
	    maxb->size = ((unsigned *)maxb)[0];
	    maxb->next = templist;
	    templist = maxb;
	    templistsize++;
	    blocks++;
	    numptrs = blocks;
	}
    }

    struct block **buf = (struct block **)((char *)maxb +
	    sizeof (struct block));

    // loop through the templist sorting groups of pointers and then linking
    // them into the freelist
    while (blocks)
    {
    	// numptrs should never be more then the number of blocks
	if (blocks < numptrs)
	    numptrs = blocks;

	// copy pointers from linked list into array of pointers
    	b = templist;
    	int i;

    	for (i = 0; i < numptrs; i++)
	{
	    buf[i] = b;
	    b = b->next;
	}

	blocks -= numptrs;
	templist = b;
	templistsize -= numptrs;

	// sort the list of blocks
//	qsort(buf, numptrs, sizeof (struct block *), blockaddrcmp);
	qsort(buf, numptrs, blockaddrcmp);
	b = freelist;
	struct block *prev = NULL;

	// insert blocks into the free list
	for (i = 0; i < numptrs; i++)
	{
	    while (b && buf[i] > b)
	    {
		prev = b;
		b = b->next;
	    }

	    struct block *t = buf[i];
	    int size = t->size;
	    freespace += size;
	    allocspace -= size;
	    allocblocks--;

	    if (prev == NULL)
	    {
	    	freelist = t;
		prev = t;
		t->next = b;
		freeblocks++;
	    }
	    else if ((char *)prev + prev->size == (char *)t)
		prev->size += size;
	    else
	    {
	    	prev->next = t;
		prev = t;
		t->next = b;
		freeblocks++;
	    }

	    if ((char *)prev + prev->size == (char *)b)
	    {
	    	prev->size += b->size;
	    	prev->next = b->next;
		b = b->next;
		freeblocks--;
	    }
	}
    }

    locked = FALSE;
}

void *
malloc(unsigned size)
{
    if (size % MINALIGN)
	size += MINALIGN - (size % MINALIGN);

    size += MINALIGN;

    if (size < MINBLOCKSIZE)
	size = MINBLOCKSIZE;

    if (thunksize)
    {
	doinit();

	if (size < thunksize)
	{
	    unsigned loc = size / MINALIGN;
	    struct block *b = thunks[loc];

	    if (b == NULL)
	    {
		unsigned numblocks = pagesize / size;
		unsigned savthunk = thunksize;
		thunksize = 0;
		thunks[loc] = b = (struct block *)malloc(size * numblocks--);
		thunksize = savthunk;
		char *p = (char*)b;

		if (b == NULL || (int)b == -1)
		    return NULL;

		unsigned i;

		for (i = 0; i < numblocks; i++)
		{
		    b = (struct block*)p;
		    b->next = (struct block*)(p += size);
		}

		((struct block*)p)->next = NULL;
		b = thunks[loc];
	    }

	    thunks[loc] = b->next;
	    *(unsigned *)b = size;
	    return (void *)((char *)b + MINALIGN);
	}
    }

    if (templistsize)
	merge();

    while (1)
    {
	struct block *prev = NULL;
	struct block *blk;

	for (blk = freelist; blk != NULL; prev = blk, blk = blk->next)
	    if (blk->size >= size)
		break;

	if (blk == NULL)
	{
	    if (!expanddatasegment(size))
		return NULL;

	    continue;
	}

	struct block *next;

	if (blk->size - size >= MINBLOCKSIZE)
	{
	    next = (struct block *)((char *)blk + size);
	    next->next = blk->next;
	    next->size = blk->size - size;
	}
	else
	{
	    size = blk->size;
	    next = blk->next;
	    freeblocks--;
	}

	if (prev == NULL)
	    freelist = next;
	else
	    prev->next = next;

	*(unsigned *)blk = size;
	freespace -= size;
	allocspace += size;
	allocblocks++;
	return (void *)((char *)blk + MINALIGN);
    }
}

void
meminfo(unsigned &allocspc, unsigned &freespc, unsigned &allocblks,
	unsigned &freeblks, void *&endarena, unsigned &arenasz)
{
    allocspc = allocspace;
    freespc = freespace;
    allocblks = allocblocks;
    freeblks = freeblocks;
    endarena = arenaend;
    arenasz = arenasize;
}

void *
realloc(void *p, unsigned newsize)
{
    if (newsize % MINALIGN)
	newsize += MINALIGN - (newsize % MINALIGN);

    if (newsize + MINALIGN < MINBLOCKSIZE)
	newsize = MINBLOCKSIZE - MINALIGN;

    unsigned *us = (unsigned *)((char *)p - MINALIGN);
    unsigned oldsize = *us - MINALIGN;

    if (newsize <= oldsize)
	return p;

    if (thunksize)
    {
	doinit();

	if (oldsize < thunksize)
	{
	    void *p2 = malloc(newsize);
	    memcpy(p2, p, oldsize);
	    free(p);
	    return p2;
	}
    }

    struct block *prev = NULL;
    struct block *blk;

    for (blk = freelist; blk != NULL; prev = blk, blk = blk->next)
	if (blk > (struct block *)p)
	    break;

    if (blk == NULL)
    {
	if (!expanddatasegment(newsize - oldsize))
	    return NULL;

	return realloc(p, newsize);
    }

    if ((char *)p + oldsize == (char *)blk && blk->size + oldsize >= newsize)
    {
	if (blk->size + oldsize - newsize >= MINBLOCKSIZE)
	{
	    struct block *newblk =
		    (struct block *)((char *)blk + newsize - oldsize);

	    newblk->size = blk->size - (newsize - oldsize);
	    newblk->next = blk->next;

	    if (prev != NULL)
		prev->next = newblk;
	    else
		freelist = newblk;
	}
	else
	{
	    if (prev != NULL)
		prev->next = blk->next;
	    else
		freelist = blk->next;

	    newsize = blk->size + oldsize;
	    freeblocks--;
	}

	freespace -= newsize - oldsize;
	allocspace += newsize - oldsize;
	*us = newsize + MINALIGN;
	return p;
    }

    void *p2 = malloc(newsize);
    memcpy(p2, p, oldsize);
    free(p);
    return p2;
}

extern "C" unsigned
blocksize(void *ptr)
{
    return ptr == NULL ? 0 : *(unsigned *)((char *)ptr - MINALIGN);
}
