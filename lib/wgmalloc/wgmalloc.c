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

/* Copyright (c) 1993,1998 by TJ Merritt and Parag Patel.
   All Rights Reserved.
 */

#include <stddef.h>
#include <stdlib.h>
#include <stdio.h>
#include "stdlibx.h"

/*#define DEBUG*/	/* turn this on to enable verbose debug output */
#define DO_GC	/* comment this out if you do not want GC enabled */

#ifdef DEBUG
#	define FPRINTF(args) fprintf args
#else
#	define FPRINTF(args)
#endif

struct freeblock
{
    struct freeblock *link;
};

/* PTRBASEADDR may need to be changed for some machines, but it is unlikely */
#define PTRBASEADDR 0UL
#define BYTESPERPAGE 4096
#define PAGETHRESHHOLD (BYTESPERPAGE / 2)

/* make MINALIGN a power of 2 for speed */
#define MINALIGN (2 * sizeof (struct freeblock))
#ifdef DO_GC
#  define MINBLOCK (MINALIGN*2)
#else
#  define MINBLOCK MINALIGN
#endif
#define MINPAGES 4

#ifdef DO_GC
#include <setjmp.h>

#ifdef __FreeBSD__
extern int etext, edata, end;
extern const char **environ;
#define DATASTART ((void*)&etext)
#define DATAEND   ((void *)&end)
#define STACKSTART ((void *)&stacktop)	/* variable declared on stack */
#define STACKEND   ((void *)environ)
#define GCBASEADDR ((void *)&edata)
#define PTRBUMP (sizeof (char*))
#define MAXPAGES 4096	/* maximum addressable number of pages - 16Mb */
#endif

#ifdef _AUX_SOURCE
extern int etext, edata, end;
extern const char **environ;
#define DATASTART ((void*)(((long)&etext + 0x3FFFFF) & ~0x3FFFFF) \
		    + ((long)&etext & 0x1FFF))
#define DATAEND   ((void *)&end)
#define STACKSTART ((void *)&stacktop)
#define STACKEND   ((void *)environ)
#define GCBASEADDR ((void *)(((long)&edata + 0x1FFF) & ~0x1FFF))
#define PTRBUMP (sizeof (short))
#define MAXPAGES 4096	/* maximum addressable number of pages */
#endif

#ifdef _DJGPP_SOURCE_
extern int end;
#define GCBASEADDR ((void *)0x400000)
#define DATASTART ((void *)0x400000)
#define DATAEND   ((void *)&end)
#define STACKSTART ((void *)&stacktop)
#define STACKEND   ((void *)0x80000000)
/*#define PTRMASK (~3U) */
#define PTRBUMP (sizeof (char*))
#endif

#ifndef GCBASEADDR
#define GCBASEADDR ((void *)0x400000)
#define DATASTART ((void *)0x400000)
#define DATAEND   ((void *)0x402000)
#define STACKSTART ((void *)0x400000)
#define STACKEND   ((void *)0x402000)
#define PTRMASK (~3U)
#define PTRBUMP (sizeof (char*))
#endif

#ifndef MAXPAGES
#define MAXPAGES 1024	/* maximum addressable number of pages */
#endif
#define GC_WORDS (BYTESPERPAGE / MINBLOCK / BITSPERLONG)
#define MAXREGIONS 32

struct region
{
    void *start;	/* start of region */
    void *end;		/* end of region + 1 */
};
#endif

/* size should be in MINALIGN chunks to preserve alignment of following mem */
struct pageinfo
{
    short blocksize;	/* -1 if multi-page block, 0 if free */
    short numblocks;	/* total # of blocks on page, 0 for multi-page block */
    int count;		/* # of allocated blocks in page, total # pages */
    struct pageinfo *link;	/* to link pages together */
    struct freeblock *block;	/* 1st free block on this page, if any */
#ifdef DO_GC
    /* this is big enough for the largest number of small blocks but
       wastes space for bigger blocks - still, it is much easier to code */
    uLong refbits[GC_WORDS];	/* garbage-collect reference bits */
#endif
};

/* all malloc globals are kept in one chunk of memory. */
static struct globals
{
    struct pageinfo *blockmap[PAGETHRESHHOLD / MINBLOCK + 1];
    short sizemap[PAGETHRESHHOLD];
    struct pageinfo *freepagelist;
    int numfreepages;
    void *heaplow;
    void *heaphigh;
    int minpages;
    boolean clearblocks, clearfree;
#ifdef DO_GC
    boolean gc, ignorefree, skipinterior;
    boolean ingc;
    uLong ourpages[MAXPAGES / BITSPERLONG];
    struct region regions[MAXREGIONS];
    int numregions;
    int pages, pagecount, percent;
#endif
} g;

/* return pointer to beginning of page (descriptor) for a given address */
#define GETPAGEINFO(p)	\
    ((struct pageinfo*)(((uLong)((char *)(p) - (char *)PTRBASEADDR) & \
	    ~(BYTESPERPAGE - 1)) + (char*)PTRBASEADDR))

#ifdef DO_GC
#  define GETBIT(bmap, bit) \
	((bmap)[(bit) / BITSPERLONG] & (1UL << ((bit) & (BITSPERLONG - 1))))
#  define SETBIT(bmap, bit) \
	((bmap)[(bit) / BITSPERLONG] |= (1UL << ((bit) & (BITSPERLONG - 1))))
#  define CLEARBIT(bmap, bit) \
	((bmap)[(bit) / BITSPERLONG] &= ~(1UL << ((bit) & (BITSPERLONG - 1))))
#  define CLEARBITMAP(bmap) (memset((bmap), 0, sizeof (bmap)))

/* given a pointer to a page, return page number relative to the 1st page */
#  define PAGENUM(p) \
	(((uLong)((char *)(p) - (char *)GCBASEADDR)) / BYTESPERPAGE)

/* given a pointer to a block and a pointer to its pageinfo, figure out
   its block number on that page relative to the 1st block on that page */
#  define BLOCKNUM(pinfo, p) \
	((((uLong)((char*)(p) - (char*)(pinfo))) - sizeof (struct pageinfo)) \
		/ ((pinfo)->blocksize))

/* forward declarations */
static void gcaddregion(void *start, void *end);
static void gcremoveregion(void *start, void *end);
void garbagecollect();

#endif /* DO_GC */

static void
freepages(void *ptr, int numpgs)
{
	struct pageinfo *fpi;
	void *ptrend;
	struct pageinfo *prev = NULL;
	struct pageinfo *freepage = g.freepagelist;

	FPRINTF((stderr, "freepages(ptr=%p, numpgs=%d)\n", ptr, numpgs));

	/* insert pages into the free list in sorted order by address. */
	fpi = GETPAGEINFO(ptr);

	if (fpi != ptr)
		crash("freepages(): fpi != ptr");

	if (g.clearfree)
		memset(ptr, 0, numpgs * BYTESPERPAGE);

#ifdef DO_GC
	{
		int i;
		int pn = PAGENUM(ptr);
		struct pageinfo *p = fpi;

		for (i = 0; i < numpgs; i++, pn++)
		{
			if (pn >= MAXPAGES)
				crash("freepages(): out of space for freepages");

			SETBIT(g.ourpages, pn);
			p->blocksize = 0;
			p = (struct pageinfo *)((char *)p + BYTESPERPAGE);
		}
	}
#endif

	g.numfreepages += numpgs;
	ptrend = ((char *)ptr) + numpgs * BYTESPERPAGE;

	for (; freepage != NULL; (prev = freepage), freepage = freepage->link)
	{
		if ((char *)freepage + freepage->count * BYTESPERPAGE == (char *)ptr)
		{
			struct pageinfo *nextpage = freepage->link;

			freepage->count += numpgs;

			if (nextpage && (char *)nextpage ==
					(char *)freepage + freepage->count * BYTESPERPAGE)
			{
				freepage->count += nextpage->count;
				freepage->link = nextpage->link;
			}

		return;
		}

		if (ptrend == freepage)
		{
			fpi->link = freepage->link;
			fpi->count = numpgs + freepage->count;

			if (prev == NULL)
				g.freepagelist = fpi;
			else
				prev->link = fpi;

			return;
		}

		if ((char *)ptr < (char *)freepage)
			break;
	}

	fpi->link = freepage;
	fpi->count = numpgs;
	fpi->blocksize = 0;

	if (prev == NULL)
		g.freepagelist = fpi;
	else
		prev->link = fpi;

	return;
}

static boolean
expandarena(int numpgs)
{
	void *base;
	int offset;
	void *page;
	void *high;
	void *sbrk(int);

	if (numpgs < g.minpages)
		numpgs = g.minpages;

	/* get base address for sbrk'able memory */
	base = sbrk(0);

	if (base == NULL || (int)base == -1)
	{
		FPRINTF((stderr, "expandarena(%d), sbrk(0) failed\n", numpgs));
		return FALSE;
	}

	/* determine offset from start of page the sbrk will return */
	offset = ((uLong)base - PTRBASEADDR) % BYTESPERPAGE;

	if (offset)
	{
		/* if not zero then allocate space to page align things */
		base = (void *)sbrk(BYTESPERPAGE - offset);

		if (base == NULL || (int)base == -1)
		{
			FPRINTF((stderr, "expandarena(%d), offset sbrk(%d) failed\n",
					numpgs, BYTESPERPAGE - offset));
			return FALSE;
		}

	/* get revised base pointer */
	base = (void *)sbrk(0);

		if (base == NULL || (int)base == -1)
		{
			FPRINTF((stderr, "expandarena(%d), offset sbrk(0) failed\n",
					numpgs));
			return FALSE;
		}

	/* make sure that offset is now zero */
		if ((((uLong)base - PTRBASEADDR) % BYTESPERPAGE) != 0)
		{
			FPRINTF((stderr, "expandarena(%d), offset didn't work\n", numpgs));
			return FALSE;
		}
	}

	/* allocate the pages */
	page = (void *)sbrk(numpgs * BYTESPERPAGE);

	if (page == NULL || (int)page == -1)
	{
		FPRINTF((stderr, "expandarena(%d), sbrk(%d) failed\n", numpgs,
				numpgs *BYTESPERPAGE));
		return FALSE;
	}

	/* make sure that bases agree */
	if (page != base)
	{
		FPRINTF((stderr, "expandarena(%d), base pointer mismatch\n", numpgs));
		return FALSE;
	}

	g.pagecount -= numpgs;

	/* update heap low and high water marks */
	if (g.heaplow == NULL || (char *)page < (char *)g.heaplow)
		g.heaplow = (char *)page;

	high = (void *)((char *)page + numpgs * BYTESPERPAGE);

	if (g.heaphigh == NULL || (char *)high > (char *)g.heaphigh)
		g.heaphigh = high;

	/* add pages to free list */
	freepages(page, numpgs);
	return TRUE;
}

static void *
allocpages(int numpgs)
{
	struct pageinfo *prev = NULL;
	struct pageinfo *freepage = g.freepagelist;
	void *ptr;

	FPRINTF((stderr, "allocpages(%d)\n", numpgs));

	/* search for pages */
	for (; freepage != NULL; (prev = freepage), freepage = freepage->link)
		if (freepage->count >= numpgs)
			break;

#ifdef DO_GC
	if (freepage == NULL && g.gc && !g.ingc && g.pagecount <= 0)
	{
		garbagecollect();

		for (freepage = g.freepagelist; freepage != NULL;
				(prev = freepage), freepage = freepage->link)
			if (freepage->count >= numpgs)
				break;
	}
#endif

	/* check if any pages were found */
	if (freepage == NULL)
	{
		if (!expandarena(numpgs))
		{
			FPRINTF((stderr, "allocpages(%d), cannot expand data area\n",
					numpgs));
			return NULL;
		}

		for ((prev = NULL), freepage = g.freepagelist; freepage != NULL;
				(prev = freepage), freepage = freepage->link)
			if (freepage->count >= numpgs)
				break;

		/* everything failed! */
		if (freepage == NULL)
		{
			FPRINTF((stderr,
					"allocpages(%d), no block large enough after expand\n",
					numpgs));
			return NULL;
		}
	}

	/* remove pages from freepage list */
	if (freepage->count == numpgs)
	{
		if (prev == NULL)
			g.freepagelist = freepage->link;
		else
			prev->link = freepage->link;

		ptr = freepage;
	}
	else
	{
#ifdef ALLOCENDPAGES
		freepage->count -= numpgs;
		ptr = (char *)freepage + freepage->count * BYTESPERPAGE;
#else
		void *npage;
		int count;
		struct pageinfo *link;

		ptr = freepage;
		npage = (char *)ptr + numpgs * BYTESPERPAGE;
		count = freepage->count - numpgs;
		link = freepage->link;
		freepage = (struct pageinfo *)npage;
		freepage->link = link;
		freepage->count = count;

		if (prev == NULL)
			g.freepagelist = freepage;
		else
			prev->link = freepage;
#endif
	}

	g.numfreepages -= numpgs;

	FPRINTF((stderr, "allocpages(%d), page = %p\n", numpgs, ptr));
	return ptr;
}

static boolean
getmoreblocks(int sz)
{
	void *page;
	int bpp;
	int ind;
	struct freeblock *e;
	struct pageinfo *pi;
	int i;

	FPRINTF((stderr, "getmoreblocks(%d)\n", sz));

	if (sz < 1 || sz > PAGETHRESHHOLD)
		return FALSE;

	FPRINTF((stderr, "getmoreblocks(%d), size is valid\n", sz));
	page = allocpages(1);		/* get a page */

	if (page == NULL)
		return FALSE;

	FPRINTF((stderr, "getmoreblocks(%d), got new page\n", sz));

	/* fill out pageinfo for this page */
	bpp = (BYTESPERPAGE - sizeof (struct pageinfo)) / sz;
	ind = sz / MINBLOCK;
	e = (struct freeblock *)((char *)page + sizeof (struct pageinfo));
	pi = (struct pageinfo *)page;
	pi->blocksize = sz;
	pi->numblocks = bpp;
	pi->block = e;
	pi->link = g.blockmap[ind];
	g.blockmap[ind] = pi;
	bpp--;

	for (i = 0; i < bpp; i++, e = e->link)
		e->link = (struct freeblock *)((char *)e + sz);

	e->link = NULL;

#ifdef DO_GC
	CLEARBITMAP(pi->refbits);
	ind = PAGENUM(pi);

	if (ind >= MAXPAGES)
		crash("getmoreblocks(): out of space for new pages");

	SETBIT(g.ourpages, ind);
#endif

	return TRUE;
}

static void
freeblockspage(struct pageinfo *page)
{
	int ind = page->blocksize / MINBLOCK;
	struct pageinfo *ppi = NULL;
	struct pageinfo *p = g.blockmap[ind];

	for (; p != NULL && p != page; (ppi = p), p = p->link)
		;

	if (p == NULL)
		crash("free(): cannot find page to be freed from blockmap");

	if (ppi == NULL)
		g.blockmap[ind] = page->link;
	else
		ppi->link = page->link;

	freepages(page, 1);
}

static boolean initialized = FALSE;

static void
init()
{
	char *e;
	int minalign;
	int maxalign;
	int i;

	g.freepagelist = NULL;
	g.heaplow = NULL;
	g.heaphigh = NULL;
	g.numfreepages = 0;

	e = getenv("MALLOC_MIN_PAGES");

	if (e != NULL && *e != '\0')
		g.minpages = atoi(e);
	else
		g.minpages = MINPAGES;

	if (g.minpages < 1)
		g.minpages = MINPAGES;

	e = getenv("MALLOC_MIN_ALIGN");

	if (e != NULL && *e != '\0')
		minalign = atoi(e);
	else
		minalign = MINALIGN;

	if (minalign < MINALIGN)
		minalign = MINALIGN;

	if (minalign < MINBLOCK)
		minalign = MINBLOCK;

	e = getenv("MALLOC_MAX_ALIGN");

	if (e != NULL && *e != '\0')
		maxalign = atoi(e);
	else
		maxalign = MINALIGN * 4;

	if (maxalign < minalign)
		maxalign = minalign;
	else if (maxalign >= PAGETHRESHHOLD / MINBLOCK)
		maxalign = MINALIGN * 4;

	e = getenv("MALLOC_CLEAR_BLOCKS");

	if (e != NULL && *e != '\0')
		g.clearblocks = atoi(e) ? TRUE : FALSE;
	else
		g.clearblocks = FALSE;

	e = getenv("MALLOC_CLEAR_FREE");

	if (e != NULL && *e != '\0')
		g.clearfree = atoi(e) ? TRUE : FALSE;
	else
		g.clearfree = g.clearblocks;

	/* initialize g.sizemap */
	g.sizemap[0] = minalign;

	for (i = 1; i < maxalign; i++)
		g.sizemap[i] = ((i + minalign - 1) / minalign) * minalign;

	for (; i < PAGETHRESHHOLD; i++)
		g.sizemap[i] = ((i + maxalign - 1) / maxalign) * maxalign;

#ifdef DO_GC
	e = getenv("MALLOC_DO_GC");

	if (e != NULL && *e != '\0')
		g.gc = atoi(e) ? TRUE : FALSE;
	else
		g.gc = FALSE;

	e = getenv("MALLOC_SKIP_INTERIOR");

	if (e != NULL && *e != '\0')
		g.skipinterior = atoi(e) ? TRUE : FALSE;
	else
		g.skipinterior = FALSE;

	e = getenv("MALLOC_IGNORE_FREE");

	if (e != NULL && *e != '\0')
		g.ignorefree = atoi(e) ? TRUE : FALSE;
	else
		g.ignorefree = g.gc;

	e = getenv("MALLOC_PERCENT");

	if (e != NULL && *e != '\0')
		g.percent = atoi(e);
	else
		g.percent = 20;

	e = getenv("MALLOC_AFTER_PAGES");

	if (e != NULL && *e != '\0')
		g.pages = atoi(e);
	else
		g.pages = 16;

	if (g.pages < 0)
		g.pages = 0;

	g.pagecount = g.pages;
	g.ingc = FALSE;
	g.numregions = 0;

	/* add the data regions to be searched for roots, but exclude our own 
	*/
	gcaddregion(DATASTART, DATAEND);
	gcremoveregion(&g, (char *)&g + sizeof g);
#endif

	/* this is for things calling malloc in the following statements... */
	initialized = TRUE;
}

void *
malloc(size_t sz)
{
	int nsz;
	int ind, i;
	struct pageinfo *ppi;
	struct pageinfo *pi;
	void *ptr;

	if (!initialized)
		init();

	if (sz >= PAGETHRESHHOLD - sizeof (struct pageinfo))
	{
		/* block is bigger than threshhold so allocate whole pages */
		int pgs = (sz + sizeof (struct pageinfo)+BYTESPERPAGE - 1) /
			BYTESPERPAGE;
		void *base = allocpages(pgs);

		FPRINTF((stderr, "malloc(%d), large block\n", sz));

		if (base == NULL)
			return NULL;

		pi = (struct pageinfo *)base;
		pi->blocksize = -1;
		pi->numblocks = 0;
		pi->count = pgs;
		pi->link = NULL;
		base = (char *)base + sizeof (struct pageinfo);

		if (g.clearblocks)
			memset(base, 0, pgs * BYTESPERPAGE - sizeof (struct pageinfo));

#ifdef DO_GC
		CLEARBITMAP(pi->refbits);
		ind = PAGENUM(pi);

		if (ind + pgs >= MAXPAGES)
			crash("malloc(): out of space for new pages");

		SETBIT(g.ourpages, ind);

		/* all following pages are marked as part of a big block for GC */
		for (ind++, i = 1; i < pgs; i++, ind++)
			CLEARBIT(g.ourpages, ind);
#endif

		return base;
	}

/* search page list for a free block of our size */
nsz = g.sizemap[sz];

	if (nsz < sz)
		nsz = sz;

	FPRINTF((stderr, "malloc(%d), small block, sz = %d\n", sz, nsz));
	ind = nsz / MINBLOCK;
	ppi = NULL;
	pi = g.blockmap[ind];

	/* search pages with blocks of the given size */
	for (; pi != NULL && pi->block == NULL; pi = pi->link)
		ppi = pi;

	if (pi == NULL)
	{
		if (!getmoreblocks(nsz))
		{
			FPRINTF((stderr, "malloc(%d), no more small blocks\n", sz));
			return NULL;
		}

		/* this works since getmoreblocks always sticks new pages */
		/* at the front of the list */
		pi = g.blockmap[ind];
		ppi = NULL;
	}

	if (ppi != NULL)
	{
		/* move page with free blocks to the head of the list so we save
		   time on the next call for this block size. */
		ppi->link = pi->link;
		pi->link = g.blockmap[ind];
		g.blockmap[ind] = pi;
	}

	ptr = pi->block;
	pi->block = pi->block->link;
	pi->count++;

	if (g.clearblocks)
		memset(ptr, 0, nsz);

	FPRINTF((stderr, "malloc(%d), nsz=%d small block=%p\n", sz, nsz, ptr));
	return (void *)ptr;
}

void
free(void *ptr)
{
	extern int end;
	struct pageinfo *pi;
	int sz;

	if ((char *)ptr <= (char *)&end)
	{
		if (ptr == NULL)
			crash("free(): attempt to free NULL pointer");

		crash("free(): attempt to free a pointer to code or static data");
	}

pi = GETPAGEINFO(ptr);
	sz = pi->blocksize;

#ifdef DO_GC
	if (g.ignorefree)
	{
		if (g.clearfree)
			memset(ptr, 0, sz);

		return;
	}
#endif

	if (sz < 1)
	{
		if (sz != -1)
			crash("free(): invalid pointer - not a large multipage block");

		freepages(pi, pi->count);
		return;
	}

	FPRINTF((stderr, "free(%p) pi=%p blocksize=%d numblocks=%d\n",
			ptr, pi, pi->blocksize, pi->numblocks));

	if (g.clearfree)
		memset(ptr, 0, sz);

	pi->count--;

	/* freed up all blocks in this page, so put it back in the freepage
	   pool */
	if (pi->count == 0)
		freeblockspage(pi);
	else
	{
		struct freeblock *e = (struct freeblock *)ptr;

		e->link = pi->block;
		pi->block = e;
	}

	return;
}

void *
realloc(void *p, size_t nsz)
{
	struct pageinfo *pi;
	size_t sz;
	void *np;

	if (p == NULL)
		return malloc(nsz);

	pi = GETPAGEINFO(p);
	sz = pi->blocksize < 0 ?
		pi->count * BYTESPERPAGE - sizeof (struct pageinfo):
		pi->blocksize;

	if (nsz && nsz <= sz)
		return p;

	np = malloc(nsz);

	if (np == NULL)
		return NULL;

	memcpy(np, p, sz);
	free(p);
	return np;
}

void *
calloc(size_t elems, size_t elsize)
{
	size_t size = elems * elsize;
	void *p = malloc(size);
	struct pageinfo *pi;

	if (g.clearblocks || p == NULL)
		return p;

	pi = GETPAGEINFO(p);

	if (pi->blocksize > 0)
		memset(p, 0, pi->blocksize);
	else if (pi->blocksize == -1)
		memset(p, 0, pi->count * BYTESPERPAGE - sizeof (struct pageinfo));
	else
		crash("calloc(): malloc returned a bogus block");

	return p;
}

void
cfree(void *p)
{
	free(p);
}

void
meminfo(size_t *allocspc, size_t *freespc, size_t *allocblks,
	size_t *freeblks, void **endarena, size_t *arenasz)
{
	struct pageinfo *fp;
	int pg;

	*allocspc = 0;
	*freespc = 0;
	*allocblks = 0;
	*freeblks = 0;
	*endarena = g.heaphigh;
	*arenasz = (char *)g.heaphigh - (char *)g.heaplow;

	for (fp = g.freepagelist; fp != NULL; fp = fp->link)
	{
		*freeblks += 1;
		*freespc += fp->count * BYTESPERPAGE;
	}

	for (pg = 0; pg < sizeof g.blockmap / sizeof g.blockmap[0]; pg++)
		if (g.blockmap[pg] != NULL)
		{
			struct pageinfo *pi = g.blockmap[pg];

			if (pi->blocksize > 0)
			{
				*freeblks += pi->numblocks - pi->count;
				*allocblks += pi->count;
				*allocspc += pi->count * pi->blocksize;
			}
			else if (pi->blocksize == -1)
			{
				*allocblks += 1;
				*allocspc += pi->count * BYTESPERPAGE -
					sizeof (struct pageinfo);
			}
		}
}

unsigned
blocksize(void *ptr)
{
	struct pageinfo *pi;

	if (ptr == NULL)
		return 0;

	pi = GETPAGEINFO(ptr);

	if (pi->blocksize > 0)
		return pi->blocksize;
	else if (pi->blocksize == -1)
		return pi->count * BYTESPERPAGE - sizeof (struct pageinfo);

	return 0;
}

#ifdef DO_GC

#define GCINREGION(p, start, end)	\
    ((char*)(start) <= (char*)(p) && (char*)(p) < (char*)(end))

#define GCREGIONSOVERLAP(start1, end1, start2, end2)	\
    (GCINREGION(start1, start2, end2) || GCINREGION(end1, start2, end2)	\
	|| GCINREGION(start2, start1, end1) || GCINREGION(end2, start1, end1))

static void
gcaddregion(void *start, void *end)
{
	int empty = -1;
	int i;

	if (start == NULL || end == NULL)
		return;

	/* search region table to see if this region is next to an existing
	   region */
	for (i = 0; i < g.numregions; i++)
		if (g.regions[i].start == NULL)
			empty = i;
		else if (GCREGIONSOVERLAP(start, end,
					g.regions[i].start, g.regions[i].end))
			break;

	if (i < g.numregions)
	{
		void *rstart = g.regions[i].start;
		void *rend = g.regions[i].end;

		if ((char *)start < (char *)rstart)
			rstart = start;

		if ((char *)end > (char *)rend)
			rend = end;

	g.regions[i].start = g.regions[i].end = NULL;
		gcaddregion(rstart, rend);
		return;
	}

	if (empty == -1)
	{
		if (g.numregions >= MAXREGIONS)
			crash("gcaddregion(): too many regions - bump MAXREGIONS");

		empty = g.numregions++;
	}

	g.regions[empty].start = start;
	g.regions[empty].end = end;
}

static void
gcremoveregion(void *start, void *end)
{
	int i;
	void *rstart, *rend;

	for (i = 0; i < g.numregions; i++)
		if (GCREGIONSOVERLAP(start, end,
					g.regions[i].start, g.regions[i].end))
			break;

	if (i >= g.numregions)
		return;

	rstart = g.regions[i].start;
	rend = g.regions[i].end;

	if ((char *)start > (char *)rstart)
	{
		g.regions[i].end = start;

		if ((char *)end < (char *)rend)
			gcaddregion(end, rend);
		else if ((char *)end > (char *)rend)
			gcremoveregion(rend, end);
	}
	else if ((char *)end < (char *)rend)
	{
		g.regions[i].start = end;
		g.regions[i].end = rend;

		if ((char *)start < (char *)rstart)
			gcremoveregion(start, rstart);
	}
	else
	{
		g.regions[i].start = g.regions[i].end = NULL;

		if ((char *)start < (char *)rstart)
			gcremoveregion(start, rstart);

		if ((char *)end > (char *)rend)
			gcremoveregion(rend, end);
	}
}

static void
gcmarkregion(void *start, void *end)
{
#define STACK 128
	void *startstk[STACK];
	void *endstk[STACK];
	char *low = (char *)g.heaplow;
	char *high = (char *)g.heaphigh - sizeof (char *);
	int tos = 1;

	startstk[0] = start;
	endstk[0] = end;

	while (tos-- > 0)
	{
#ifdef PTRMASK
		char *st = (char *)(((uLong)startstk[tos]) & PTRMASK);
		char *e = (char *)(((uLong)endstk[tos] - sizeof (char *)) & PTRMASK);
#else
		char *st = (char *)startstk[tos];
		char *e = (char *)endstk[tos] - sizeof (char *);
#endif
		char *s;

		for (s = st; s <= e; s += PTRBUMP)
		{
			char *p = *(char **)s;
			int num;
			struct pageinfo *pi;
			void *pp;
			size_t bb;
			boolean big = FALSE;

			if (p < low || p > high)
				continue;

			pi = GETPAGEINFO(p);
			num = PAGENUM(pi);

			if (num >= MAXPAGES)/* bogus block? */
				continue;

			while (!GETBIT(g.ourpages, num) && num >= 0)
			{
				num--;
				pi = (struct pageinfo *)((char *)pi - BYTESPERPAGE);
				big = TRUE;
			}

			if (num < 0)		/* bogus block? */
				continue;

			if (pi->blocksize == 0)	/* hit in free block */
			{
				/* pi->history |= 1; */		/* flag the hit */
				continue;
			}

			if (pi->blocksize == -1)	/* big block */
			{
				pp = (char *)pi + sizeof (struct pageinfo);

				if (g.skipinterior && pp != p)
					continue;

				if (pi->refbits[0])	/* already marked and walked */
					continue;

				pi->refbits[0] = 1;
				bb = pi->count * BYTESPERPAGE - sizeof (struct pageinfo);
				FPRINTF((stdout, "gcmarkregion: st=%p e=%p s=%p"
						" p=%p pp=%p bb=%d\n",
						st, e, s, p, pp, bb));
			}
			else if (big)		/* bogus block */
				continue;
			else				/* regular block */
			{
				num = BLOCKNUM(pi, p);

				/* if the last byte of this block is outside a page, it is
				   bogus */
				if (num >= pi->numblocks)
				{
					/* pi->history |= 1; */		/* flag the hit */
					continue;
				}

				bb = pi->blocksize;
				pp = (char *)pi + num * bb + sizeof (struct pageinfo);

				if (g.skipinterior && pp != p)
					continue;

				if (GETBIT(pi->refbits, num))
								/* already marked and walked */
					continue;

				SETBIT(pi->refbits, num);
			}

			if ((char *)pp < low || (char *)pp > high)
				crash("gcmarkregion(): block pointer pp not in heap");

			if ((char *)pp + bb < low || (char *)pp + bb > (char *)g.heaphigh)
				crash("gcmarkregion(): end of block pointer pp not in heap");

			if (tos >= STACK)
				gcmarkregion(pp, pp + bb);
			else
			{
				startstk[tos] = pp;
				endstk[tos] = pp + bb;
				tos++;
			}
		}
	}
}

/* garbage collect using mark & sweep */
void
garbagecollect()
{
	static jmp_buf jmp;
	long stacktop;				/* to get the top of stack */
	int i, numpgs;
	struct pageinfo *page;
	size_t totfree;
	int arenasz, expandcnt;

	if (g.ingc || g.heaphigh == NULL)
		return;

	g.ingc = TRUE;

	/* ref bits should already be cleared since we clear them upon return
	   to avoid doing an extra pass over all of memory */

	/* this does not go in our "gc" struct as we really do want it
	   searched */
	memset(jmp, 0, sizeof jmp);
	setjmp(jmp);
	totfree = 0;

	/* set ref bits of referenced blocks */
	gcmarkregion(STACKSTART, STACKEND);

	for (i = 0; i < g.numregions; i++)
	{
		if (g.regions[i].start == NULL)
			continue;

		gcmarkregion(g.regions[i].start, g.regions[i].end);
	}

	numpgs = PAGENUM(g.heaphigh);
	page = g.heaplow;

	/* release unused memory */
	for (i = PAGENUM(page); i < numpgs; i++,
			page = (struct pageinfo *)((char *)page + BYTESPERPAGE))
	{
		if (i >= MAXPAGES)
			crash("garbagecollect(): out of space for new pages");

		if (!GETBIT(g.ourpages, i))
			continue;

		if (page->blocksize == -1)
		{
			if (page->refbits[0])
				page->refbits[0] = 0;
			else
				freepages(page, page->count);
		}
		else if (page->blocksize > 0)
		{
			int j;
			int freed = 0;
			page->block = NULL;

			for (j = 0; j < page->numblocks; j++)
				if (!GETBIT(page->refbits, j))
				{
					struct freeblock *e = (struct freeblock *)
						((char *)page + sizeof (struct pageinfo)+
							page->blocksize * j);

					if (g.clearfree)
						memset(e, 0, page->blocksize);

					e->link = page->block;
					page->block = e;
					freed++;
				}

			CLEARBITMAP(page->refbits);

			if (freed == page->numblocks)
				freeblockspage(page);
			else
			{
				page->count = page->numblocks - freed;
				totfree += freed * page->blocksize;
			}
		}
	}

	/* rebuildfreelist(); */
	g.ingc = FALSE;

	totfree += g.numfreepages * BYTESPERPAGE;
	arenasz = ((char *)g.heaphigh - (char *)g.heaplow) / BYTESPERPAGE;
	expandcnt = arenasz * g.percent;
	expandcnt /= 100;

	if (g.freepagelist == NULL)
	{
		/* if garbage collection didn't yield a page then allow */
		/* heap to grow by g.page or g.percent which ever is larger. */
		g.pagecount = (g.pages > expandcnt ? g.pages : expandcnt);
	}
	else
	{
		/* if garbage collection yielded less than g.percent of the heap */
		/* then give the heap some extra growing room. */
		expandcnt -= totfree / BYTESPERPAGE;
		expandcnt -= g.numfreepages;

		if (g.pagecount < expandcnt)
			g.pagecount = expandcnt;

		if (g.pagecount <= 0)
			g.pagecount = 16;
	}

	return;
}

void
gcdebug(FILE *fp)
{
	int i, numpgs;
	struct pageinfo *freepage, *page;

	if (fp == NULL)				/* so we can call this easily from a
								   debugger */
		fp = stderr;

	fprintf(fp, "gcdebug()...\n");
	fprintf(fp, "  g.pages=%d  g.pagecount=%d  g.numregions=%d\n",
			g.pages, g.pagecount, g.numregions);

	for (i = 0; i < g.numregions; i++)
		if (g.regions[i].start != NULL)
			fprintf(fp, "    regions[%d] start=%p end=%p\n",
					i, g.regions[i].start, g.regions[i].end);

	fprintf(fp, "  g.heaphigh=%p  g.heaplow=%p  g.numfreepages=%d\n",
			g.heaphigh, g.heaplow, g.numfreepages);
	fprintf(fp, "  freepage...\n");
	freepage = g.freepagelist;

	for (i = 1; freepage != NULL; i++, freepage = freepage->link)
		fprintf(fp, "    freepage[%d] = %p, count=%d\n", i,
				freepage, freepage->count);

	fprintf(fp, "  allocated memory...\n");
	numpgs = PAGENUM(g.heaphigh);
	page = (struct pageinfo *)g.heaplow;

	/* release unused memory */
	for (i = PAGENUM(page); i < numpgs; i++,
			page = (struct pageinfo *)((char *)page + BYTESPERPAGE))
	{
		if (i >= MAXPAGES)
			crash("gcdebug(): out of space for new pages");

		if (!GETBIT(g.ourpages, i))
			continue;

		if (page->blocksize == -1)
		{
			fprintf(fp, "    block=%p numpages=%d refbits[0]=%ld\n",
					page, page->count, page->refbits[0]);
		}
		else if (page->blocksize > 0)
		{
			int j;

			fprintf(fp, "    page=%p blocksize=%d numblocks=%d used=%d\n",
					page, page->blocksize, page->numblocks, page->count);
			fprintf(fp, "        block=%p link=%p\n",
					page->block, page->link);
			fprintf(fp, "        refbits=%p", page->refbits);

			for (j = 0; j < GC_WORDS / 2; j++)
				fprintf(fp, "0x%08lX ", page->refbits[j]);

			fprintf(fp, "\n                ");

			for (; j < GC_WORDS; j++)
				fprintf(fp, "0x%08lX ", page->refbits[j]);

			fprintf(fp, "\n");
		}
	}

/****
    if (g.snapshot)
		gcdumpsnapstack(fp);
****/

	fflush(fp);
}

#endif /* !GC */
