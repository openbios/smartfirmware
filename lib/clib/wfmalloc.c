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

/* Copyright (c) 1993 by TJ Merritt and Parag Patel.  All Rights Reserved. */

#include <stddef.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#ifdef unix
#  include <unistd.h>
#endif
#include "stdlibx.h"

struct freeblock
{
    struct freeblock *link;
};

/* size should be in MINALIGN chunks to preserve alignment of following mem */
struct pageinfo
{
	short blocksize;	/* -1 if multi-page block, 0 if free */
	short numblocks;	/* total # of blocks on page, 0 for multi-page block */
	int count;		/* # of allocated blocks in page, total # pages */
	struct pageinfo *link;	/* to link pages together */
	struct freeblock *block;	/* 1st free block on this page, if any */
};

/* PTRBASEADDR may need to be changed for some machines, but it is unlikely */
#define PTRBASEADDR 0UL
#define BYTESPERPAGE 4096
#define PAGETHRESHHOLD (BYTESPERPAGE / 2)
/* make MINALIGN a power of 2 for speed */
#define MINALIGN (2 * sizeof (struct freeblock))
#define MINBLOCK MINALIGN
#define MINPAGES 4

/* all malloc globals are kept in one chunk of memory. */
static struct globals
{
	struct pageinfo *blockmap[PAGETHRESHHOLD / MINBLOCK + 1];
	short sizemap[PAGETHRESHHOLD];
	struct pageinfo *freepagelist;
	void *heaplow;
	void *heaphigh;
	int minpages;
	boolean clearblocks;
} g;

/* return pointer to beginning of page (descriptor) for a given address */
#define getpageinfo(p)	\
    ((struct pageinfo*)(((uLong)((char *)(p) - (char *)PTRBASEADDR) & \
	    ~(BYTESPERPAGE - 1)) + (char*)PTRBASEADDR))

static void
freepages(void *ptr, int numpgs)
{
	struct pageinfo *fpi;
	void *ptrend;
	struct pageinfo *prev = NULL;
	struct pageinfo *freepage = g.freepagelist;

#ifdef DEBUGX
	fprintf(stderr, "freepages(ptr=0x%X, numpgs=%d)\n", ptr, numpgs);
#endif

	/* insert pages into the free list in sorted order by address. */
	fpi = getpageinfo(ptr);

	if (fpi != ptr)
		crash("freepages(): fpi != ptr");

	ptrend = ((char *)ptr) + numpgs * BYTESPERPAGE;

	for (; freepage != NULL; prev = freepage, freepage = freepage->link)
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
}

static boolean
expandarena(int numpgs)
{
	void *base;
	int offset;
	void *page;
	void *high;

	if (numpgs < g.minpages)
		numpgs = g.minpages;

	/* get base address for sbrk'able memory */
	base = (void *)sbrk(0);

	if (base == NULL || (int)base == -1)
	{
#ifdef DEBUGX
		fprintf(stderr, "expandarena(%d), sbrk(0) failed\n", numpgs);
#endif
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
#ifdef DEBUGX
			fprintf(stderr, "expandarena(%d), offset sbrk(%d) failed\n", numpgs,
					BYTESPERPAGE - offset);
#endif
			return FALSE;
		}

	/* get revised base pointer */
	base = (void *)sbrk(0);

		if (base == NULL || (int)base == -1)
		{
#ifdef DEBUGX
			fprintf(stderr, "expandarena(%d), offset sbrk(0) failed\n", numpgs);
#endif
			return FALSE;
		}

	/* make sure that offset is now zero */
		if ((((uLong)base - PTRBASEADDR) % BYTESPERPAGE) != 0)
		{
#ifdef DEBUGX
			fprintf(stderr, "expandarena(%d), offset didn't work\n", numpgs);
#endif
			return FALSE;
		}
	}

	/* allocate the pages */
	page = (void *)sbrk(numpgs * BYTESPERPAGE);

	if (page == NULL || (int)page == -1)
	{
#ifdef DEBUGX
		fprintf(stderr, "expandarena(%d), sbrk(%d) failed\n", numpgs,
			numpgs *BYTESPERPAGE);
#endif
		return FALSE;
	}

/* make sure that bases agree */
	if (page != base)
	{
#ifdef DEBUGX
		fprintf(stderr, "expandarena(%d), base pointer mismatch\n", numpgs);
#endif
		return FALSE;
	}

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

#ifdef DEBUGX
	fprintf(stderr, "allocpages(%d)\n", numpgs);
#endif

	/* search for pages */
	for (; freepage != NULL; prev = freepage, freepage = freepage->link)
		if (freepage->count >= numpgs)
			break;

	/* check if any pages were found */
	if (freepage == NULL)
	{
		if (!expandarena(numpgs))
		{
#ifdef DEBUGX
			fprintf(stderr, "allocpages(%d), cannot expand data area\n",
					numpgs);
#endif
			return NULL;
		}

		for (prev = NULL, freepage = g.freepagelist; freepage != NULL;
				prev = freepage, freepage = freepage->link)
			if (freepage->count >= numpgs)
				break;

		/* everything failed! */
		if (freepage == NULL)
		{
#ifdef DEBUGX
			fprintf(stderr,
					"allocpages(%d), no block large enough after expand\n",
					numpgs);
#endif
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

#ifdef DEBUGX
	fprintf(stderr, "allocpages(%d), page = 0x%X\n", numpgs, ptr);
#endif
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

#ifdef DEBUGX
	fprintf(stderr, "getmoreblocks(%d)\n", sz);
#endif
	if (sz < 1 || sz > PAGETHRESHHOLD)
		return FALSE;

#ifdef DEBUGX
	fprintf(stderr, "getmoreblocks(%d), size is valid\n", sz);
#endif

	page = allocpages(1);		/* get a page */

	if (page == NULL)
		return FALSE;

#ifdef DEBUGX
	fprintf(stderr, "getmoreblocks(%d), got new page\n", sz);
#endif

	/* fill out pageinfo for this page */
	bpp = (BYTESPERPAGE - sizeof (struct pageinfo)) / sz;
	ind = sz / MINBLOCK;
	e = (struct freeblock *)((char *)page + sizeof (struct pageinfo));
	pi = (struct pageinfo *)page;
	pi->blocksize = sz;
	pi->numblocks = 0;
	pi->block = e;
	pi->link = g.blockmap[ind];
	g.blockmap[ind] = pi;
	bpp--;

	for (i = 0; i < bpp; i++, e = e->link)
		e->link = (struct freeblock *)((char *)e + sz);

	e->link = NULL;
	return TRUE;
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

	e = getenv("GC_MIN_PAGES");

	if (e != NULL && *e != '\0')
		g.minpages = atoi(e);
	else
		g.minpages = MINPAGES;

	if (g.minpages < 1)
		g.minpages = MINPAGES;

	e = getenv("GC_MIN_ALIGN");

	if (e != NULL && *e != '\0')
		minalign = atoi(e);
	else
		minalign = MINALIGN;

	if (minalign < MINALIGN)
		minalign = MINALIGN;

	e = getenv("GC_MAX_ALIGN");

	if (e != NULL && *e != '\0')
		maxalign = atoi(e);
	else
		maxalign = MINALIGN * 4;

	if (maxalign < minalign)
		maxalign = minalign;
	else if (maxalign >= PAGETHRESHHOLD / MINBLOCK)
		maxalign = MINALIGN * 4;

	e = getenv("GC_CLEAR_BLOCKS");

	if (e != NULL && *e != '\0')
		g.clearblocks = atoi(e) ? TRUE : FALSE;
	else
		g.clearblocks = FALSE;

	/* initialize g.sizemap */
	g.sizemap[0] = minalign;

	for (i = 1; i < maxalign; i++)
		g.sizemap[i] = ((i + minalign - 1) / minalign) * minalign;

	for (; i < PAGETHRESHHOLD; i++)
		g.sizemap[i] = ((i + maxalign - 1) / maxalign) * maxalign;

	/* this is for things calling malloc in the following statements... */
	initialized = TRUE;
}

void *
malloc(size_t sz)
{
	int nsz;
	int ind;
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

#ifdef DEBUGX
		fprintf(stderr, "malloc(%d), large block\n", sz);
#endif

		if (base == NULL)
			return NULL;

		pi = (struct pageinfo *)base;
		pi->blocksize = -1;
		pi->numblocks = 0;
		pi->count = pgs;
		pi->link = NULL;
		base = (char *)base + sizeof (struct pageinfo);

		if (g.clearblocks)
			memset(base, '\0', pgs * BYTESPERPAGE);

		return base;
	}

/* search page list for a free block of our size */
nsz = g.sizemap[sz];

	if (nsz < sz)
		nsz = sz;

#ifdef DEBUGX
	fprintf(stderr, "malloc(%d), small block, sz = %d\n", sz, nsz);
#endif

	ind = nsz / MINBLOCK;
	ppi = NULL;
	pi = g.blockmap[ind];

	/* search pages with blocks of the given size */
	for (; pi != NULL && pi->block == NULL; ppi = pi, pi = pi->link)
		;

	if (pi == NULL)
	{
		if (!getmoreblocks(nsz))
		{
#ifdef DEBUGX
			fprintf(stderr, "malloc(%d), no more small blocks\n", sz);
#endif
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
		*/
		/* time on the next call for this size. */
		ppi->link = pi->link;
		pi->link = g.blockmap[ind];
		g.blockmap[ind] = pi;
	}

	ptr = pi->block;
	pi->block = pi->block->link;
	pi->count++;

	if (g.clearblocks)
		memset(ptr, '\0', nsz);

#ifdef DEBUGX
	fprintf(stderr, "malloc(%d), nsz=%d small block=0x%X\n", sz, nsz, ptr);
#endif
	return (void *)ptr;
}

void
free(void *ptr)
{
	extern end;
	struct pageinfo *pi;
	int sz;

	if ((char *)ptr <= (char *)&end)
	{
		if (ptr == NULL)
			crash("free(): attempt to free NULL pointer");

		crash("free(): attempt to free a pointer to code or static data");
	}

pi = getpageinfo(ptr);
	sz = pi->blocksize;

	if (sz < 1)
	{
		if (sz != -1)
			crash("free(): invalid pointer - not a large multipage block");

		freepages(pi, pi->count);
		return;
	}

#ifdef DEBUGX
	fprintf(stderr, "free(0x%X) pi=0x%X blocksize=%d numblocks=%d\n",
			ptr, pi, pi->blocksize, pi->numblocks);
#endif

	pi->count--;

	/* freed up all blocks in this page, so put it back in the freepage
	   pool */
	if (pi->count == 0)
	{
		int ind = sz / MINBLOCK;
		struct pageinfo *ppi = NULL;
		struct pageinfo *p = g.blockmap[ind];

		for (; p != NULL && p != pi; ppi = p, p = p->link)
			;

		if (p == NULL)
			crash("free(): cannot find page to be freed from blockmap");

		if (ppi == NULL)
			g.blockmap[ind] = pi->link;
		else
			ppi->link = pi->link;

		freepages(pi, 1);
	}
	else
	{
		struct freeblock *e = (struct freeblock *)ptr;

		e->link = pi->block;
		pi->block = e;
	}
}

void *
realloc(void *p, size_t nsz)
{
	struct pageinfo *pi;
	size_t sz;
	void *np;

	if (p == NULL)
		return malloc(nsz);

	pi = getpageinfo(p);
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

	if (g.clearblocks)
		return p;

	if (p == NULL)
		return NULL;

	pi = getpageinfo(p);

	if (pi->blocksize > 0)
		memset(p, '\0', pi->blocksize);
	else if (pi->blocksize == -1)
		memset(p, '\0', pi->count * BYTESPERPAGE -
				sizeof (struct pageinfo));
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

	pi = getpageinfo(ptr);

	if (pi->blocksize > 0)
		return pi->blocksize;
	else if (pi->blocksize == -1)
		return pi->count * BYTESPERPAGE - sizeof (struct pageinfo);

	return 0;
}
