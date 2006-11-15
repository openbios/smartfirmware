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

/* Copyright (c) 1992 by Thomas J. Merritt and Parag Patel.
   All Rights Reserved. */

#include <stddef.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <setjmp.h>
#include "stdlibx.h"
#include "bits.h"
#include "stkback.h"
#include "gcmalloc.h"
#include "gcdefs.h"

/* our global data structures */
struct gcglobals gc;

void
gcabort(char const *fmt, ...)
{
	va_list ap;
	va_start(ap, fmt);
	vfprintf(stderr, fmt, ap);
	va_end(ap);
	fprintf(stderr, "\r\n");
	backtrace(TRUE);
	abort();
	exit(-1);
}

void
gcfreepages(void *ptr, int numpgs)
{
	/* remove any back pointers and bitmaps from pages */
	void *p = ptr;
	int i;
	struct pageinfo *fpi, *prev, *freepage;
	void *ptrend;

	for (i = 0; i < numpgs; i++, p = (char *)p + BYTESPERPAGE)
	{
		struct pageinfo *pi;

		if (getpagegroupinfo(p) == NULL)
			gcabort("gcfreepages: pages not completely mapped, #%d, 0x%X",
					i, p);

		pi = getpageinfo(p);
		pi->blocksize = 0;
		pi->numblocks = FREE_PAGE;
		pi->uf.freemap = NULL;
		pi->ur.refmap = NULL;
		pi->link = NULL;
	}

	/* insert pages into the free list in sorted order by address. */
	fpi = getpageinfo(ptr);
	ptrend = ((char *)ptr) + numpgs * BYTESPERPAGE;
	prev = NULL;
	freepage = gc.freepagelist;

	for (; freepage != NULL; prev = freepage, freepage = freepage->link)
	{
		if ((char *)freepage->page + freepage->uf.count * BYTESPERPAGE
				== (char *)ptr)
		{
			struct pageinfo *nextpage;

			freepage->uf.count += numpgs;
			nextpage = freepage->link;

			if (nextpage && (char *)nextpage->page ==
					(char *)freepage->page + freepage->uf.count * BYTESPERPAGE)
			{
				freepage->uf.count += nextpage->uf.count;
				freepage->link = nextpage->link;
			}

		return;
		}

		if (ptrend == freepage->page)
		{
			fpi->link = freepage->link;
			fpi->uf.count = numpgs + freepage->uf.count;

			if (prev == NULL)
				gc.freepagelist = fpi;
			else
				prev->link = fpi;

			return;
		}

		if ((char *)ptr < (char *)freepage->page)
			break;
	}

	fpi->link = freepage;
	fpi->uf.count = numpgs;

	if (prev == NULL)
		gc.freepagelist = fpi;
	else
		prev->link = fpi;
}

extern void *sbrk(size_t size);

static boolean
expandarena(int numpgs)
{
	void *base, *page, *high;
	int offset, pgs, firstgroup, lastgroup, group, addpgs, i;
	struct pagegroupinfo *extra;

	if (numpgs < gc.minpages)
		numpgs = gc.minpages;

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
offset = (uLong)base % BYTESPERPAGE;

	if (offset)
	{
		/* if not zero then allocate space to page align things */
		base = sbrk(BYTESPERPAGE - offset);

		if (base == NULL || (int)base == -1)
		{
#ifdef DEBUGX
			fprintf(stderr, "expandarena(%d), offset sbrk(%d) failed\n", numpgs,
					BYTESPERPAGE - offset);
#endif
			return FALSE;
		}

	/* get revised base pointer */
	base = sbrk(0);

		if (base == NULL || (int)base == -1)
		{
#ifdef DEBUGX
			fprintf(stderr, "expandarena(%d), offset sbrk(0) failed\n", numpgs);
#endif
			return FALSE;
		}

	/* make sure that offset is now zero */
		if (((uLong)base % BYTESPERPAGE) != 0)
		{
#ifdef DEBUGX
			fprintf(stderr, "expandarena(%d), offset didn't work\n", numpgs);
#endif
			return FALSE;
		}
	}

	pgs = numpgs;
	firstgroup = ((char *)base - (char *)PTRBASEADDR) / BYTESPERPAGEGROUP;
	lastgroup = ((char *)base + pgs * BYTESPERPAGE - (char *)PTRBASEADDR) /
		BYTESPERPAGEGROUP;

	/* count the number of page groups that are new */
	for (group = firstgroup; group <= lastgroup; group++)
	{
		if (group < 0 || group >= MAXPAGEGROUPS)
		{
#ifdef DEBUGX
			fprintf(stderr, "expandarena(%d), group out of range, %d, %d..%d\n",
					numpgs, group, 0, MAXPAGEGROUPS - 1);
#endif
			return FALSE;
		}

		if (gc.pagegroups[group] == NULL)
		{
			pgs++;
			lastgroup = ((char *)base + pgs * BYTESPERPAGE -
					(char *)PTRBASEADDR) / BYTESPERPAGEGROUP;
		}
	}

	/* allocate the pages */
	page = sbrk(pgs * BYTESPERPAGE);

	if (page == NULL || (int)page == -1)
	{
#ifdef DEBUGX
		fprintf(stderr, "expandarena(%d), sbrk(%d) failed\n", numpgs,
			pgs *BYTESPERPAGE);
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

	/* update the page counter */
	gc.pagecount -= pgs;

	/* update heap low and high water marks */
	if (gc.heaplow == NULL || (char *)page < (char *)gc.heaplow)
		gc.heaplow = (char *)page;

	high = (void *)((char *)page + pgs * BYTESPERPAGE);

	if (gc.heaphigh == NULL || (char *)high > (char *)gc.heaphigh)
		gc.heaphigh = high;

extra = (struct pagegroupinfo *)((char *)page + numpgs * BYTESPERPAGE);

	/* fill in new page groups */
	for (group = firstgroup; group <= lastgroup; group++)
		if (gc.pagegroups[group] == NULL)
		{
			struct pageinfo *pi;
			int j;

			/* fill in new page group */
			gc.pagegroups[group] = extra++;
			pi = gc.pagegroups[group]->pages;

			for (j = 0; j < PAGESPERGROUP; j++, pi++)
			{
				pi->blocksize = 0;
				pi->numblocks = UNUSED_PAGE;
				pi->page = getpage(group, j);
				pi->uf.freemap = NULL;
				pi->ur.refmap = NULL;
				pi->link = NULL;
				pi->history = 0;
				pi->atomic = FALSE;
			}
		}

	/* mark header pages as header pages */
	extra = (struct pagegroupinfo *)((char *)page + numpgs * BYTESPERPAGE);
	addpgs = pgs - numpgs;

	for (i = 0; i < addpgs; i++, extra++)
	{
		struct pageinfo *pi = getpageinfo((void *)extra);
		pi->numblocks = HEADER_PAGE;
	}

	/* add pages to free list */
	gcfreepages(page, numpgs);
	return TRUE;
}

void *
gcallocpages(int numpgs, boolean collect)
{
	struct pageinfo *prev = NULL;
	struct pageinfo *freepage;
	void *ptr;
	int i;

#ifdef DEBUGX
	fprintf(stderr, "gcallocpages(%d)\n", numpgs);
#endif

	/* search for pages */
	for (freepage = gc.freepagelist; freepage != NULL;
			prev = freepage, freepage = freepage->link)
		if (freepage->uf.count >= numpgs)
			break;

	if (freepage == NULL && numpgs == 1 && gc.hitpagelist != NULL)
	{
		/* take a block off the hit page list */
		freepage = gc.hitpagelist;
		gc.hitpagelist = freepage->link;

		/* and place it on the freepagelist */
		freepage->link = gc.freepagelist;
		freepage->uf.count = 1;
		gc.freepagelist = freepage;
		prev = NULL;
	}

	/* check if any pages were found */
	if (freepage == NULL)
	{
		/* try garbage-collecting here to pick up used big pages */
		if (gc.pagecount <= 0 && collect && gc.pages && !gc.ingc)
		{
			garbagecollect();

			for (prev = NULL, freepage = gc.freepagelist; freepage != NULL;
					prev = freepage, freepage = freepage->link)
				if (freepage->uf.count >= numpgs)
					break;
		}

		/* try expanding our heap if all else fails */
		if (freepage == NULL)
		{
			if (!expandarena(numpgs))
			{
#ifdef DEBUGX
				fprintf(stderr, "gcallocpages(%d), cannot expand data area\n",
						numpgs);
#endif
				return NULL;
			}

			for (prev = NULL, freepage = gc.freepagelist; freepage != NULL;
					prev = freepage, freepage = freepage->link)
				if (freepage->uf.count >= numpgs)
					break;

			/* everything failed! */
			if (freepage == NULL)
			{
#ifdef DEBUGX
				fprintf(stderr,
						"gcallocpages(%d), no block large enough after expand\n",
						numpgs);
#endif
				return NULL;
			}
		}
	}

	/* remove pages from freepage list */
	if (freepage->uf.count == numpgs)
	{
		if (prev == NULL)
			gc.freepagelist = freepage->link;
		else
			prev->link = freepage->link;

		ptr = freepage->page;
	}
	else
	{
		void *npage;
		int count;
		struct pageinfo *link;

#ifdef ALLOCENDPAGES
		freepage->uf.count -= numpgs;
		ptr = (void *)((char *)freepage->page +
				freepage->uf.count * BYTESPERPAGE);
#else
		ptr = freepage->page;
		npage = (void *)((char *)ptr + numpgs * BYTESPERPAGE);
		count = freepage->uf.count - numpgs;
		link = freepage->link;
		freepage = getpageinfo(npage);
		freepage->link = link;
		freepage->uf.count = count;

		if (prev == NULL)
			gc.freepagelist = freepage;
		else
			prev->link = freepage;
#endif
	}

	/* point pages to first page info block */
	for (i = 0; i < numpgs; i++)
	{
		void *p = (((char *)ptr) + i * BYTESPERPAGE);
		struct pageinfo *pi;

		if (getpagegroupinfo(p) == NULL)
			gcabort("gcallocpages: free list not completely mapped, #%d, 0x%X",
					i, p);

		pi = getpageinfo(p);
		pi->blocksize = -i - 1;
		pi->numblocks = numpgs;
	}

#ifdef DEBUGX
	fprintf(stderr, "gcallocpages(%d), page = 0x%X\n", numpgs, ptr);
#endif
	return ptr;
}

/* gcsimplefree() and gcsimplemalloc() implement a traditional malloc */
/* for low-level use within this gc allocator code.  These blocks will */
/* never be walked for garbage-collection. */

void
gcsimplefree(void *ptr)
{
	/* get the size of the block from within the block - see
	   gcsimplemalloc() */
	struct freeelem *prev = NULL;
	struct freeelem *us = (struct freeelem *)((char *)ptr - SIMPLEALIGN);
	size_t size = *(size_t *)us;
	struct freeelem *blk;

	us->count = size;

	if (size == 0)
		gcabort("gcsimplefree: ptr=0x%X size should never be zero", ptr);

#ifdef DEBUGX
	fprintf(stderr, "gcsimplefree(ptr=0x%X) size=0x%X(%d) us=0x%X\n",
			ptr, size, size, us);
#endif

	for (blk = gc.simplefreelist; blk != NULL; prev = blk, blk = blk->link)
		if (blk > us)
			break;

	if (prev == NULL)
	{
		/* this block goes before the 1st block in the chain */
		us->link = gc.simplefreelist;
		gc.simplefreelist = us;
	}
	else if ((struct freeelem *)((char *)prev + prev->count) == us)
	{
		/* this block is appended to the end of the previous block */
		prev->count += size;
		us = prev;
	}
	else
	{
		us->link = prev->link;
		prev->link = us;
	}

	if ((struct freeelem *)((char *)us + us->count) == us->link)
	{
		/* append the following block to the end of this block */
		us->count += us->link->count;
		us->link = us->link->link;
	}
}

void *
gcsimplemalloc(size_t size)
{
	struct freeelem *prev = NULL;
	struct freeelem *blk, *next;

#ifdef DEBUGX
	fprintf(stderr, "gcsimplemalloc(size=0x%X(%d))\n", size, size);
#endif

	if (size % SIMPLEALIGN)		/* adjust alignment */
		size += SIMPLEALIGN - (size % SIMPLEALIGN);

	size += SIMPLEALIGN;		/* to store the size of this block */

	if (size < SIMPLEBLOCK)		/* smallest block size allowed */
		size = SIMPLEBLOCK;

	while (1)
	{
		int pgs, i;
		void *p;

		prev = NULL;

		for (blk = gc.simplefreelist; blk != NULL; prev = blk, blk = blk->link)
			if ((size_t)blk->count >= size)
				break;

		if (blk != NULL)
			break;

		/* we could not find a block of the right size, so we need more
		   pages */
		pgs = (size + BYTESPERPAGE - 1) / BYTESPERPAGE;
		blk = (struct freeelem *)gcallocpages(pgs, FALSE);

		if (blk == NULL)
			return NULL;

		/* mark our pages so free() calls gcsimplefree() for our blocks */
		p = blk;

		for (i = 0; i < pgs; i++)
		{
			struct pageinfo *pi = getpageinfo(p);

			pi->blocksize = 0;	/* mark this as a free page */
			pi->numblocks = SIMPLE_PAGE;	/* but used for simple blocks */
			p = (char *)p + BYTESPERPAGE;
		}

		/* this will add the new pages to our free list at the proper spot
		   */
		*(size_t *)blk = pgs * BYTESPERPAGE;
		gcsimplefree((char *)blk + SIMPLEALIGN);
	}

	if (blk->count - size >= SIMPLEBLOCK)
	{
		/* there is room left over in this block, so don't lose it */
		next = (struct freeelem *)((char *)blk + size);
		next->link = blk->link;
		next->count = blk->count - size;
	}
	else
	{
		size = blk->count;
		next = blk->link;
	}

	if (prev == NULL)
		gc.simplefreelist = next;
	else
		prev->link = next;

#ifdef DEBUGX
	fprintf(stderr, "gcsimplemalloc(size=0x%X(%d)) blk=0x%X return=0x%X\n",
			size, size, blk, (char *)blk + SIMPLEALIGN);
#endif

	/* store the size of the block inside the block */
	*(size_t *)blk = size;
	return (char *)blk + SIMPLEALIGN;
}

void *
gcsimplerealloc(void *p, size_t nsz)
{
	size_t sz;
	void *np;

	if (p == NULL)
		return gcsimplemalloc(nsz);

	sz = *(size_t *)((char *)p - SIMPLEALIGN);

	if (nsz <= sz)
		return p;

	np = gcsimplemalloc(nsz);
	memcpy(np, p, sz);
	gcsimplefree(p);
	return np;
}

static boolean
getmoreblocks(int sz, boolean atomic, void *type, boolean collect)
{
	void *page;
	struct pageinfo *pi;
	int bpp;

#ifdef DEBUGX
	fprintf(stderr, "getmoreblocks(%d, 0x%X, %d)\n", sz, type, atomic);
#endif
	if (sz < 1 || sz > PAGETHRESHHOLD)
		return FALSE;

#ifdef DEBUGX
	fprintf(stderr, "getmoreblocks(%d, 0x%X, %d), size is valid\n",
			sz, type, atomic);
#endif

	page = gcallocpages(1, collect);	/* get a page */

	if (page == NULL)
		return FALSE;

#ifdef DEBUGX
	fprintf(stderr, "getmoreblocks(%d, 0x%X, %d), got new page\n",
			sz, type, atomic);
#endif

	/* fill out pageinfo for this page */
	pi = getpageinfo(page);
	bpp = BYTESPERPAGE / sz;

	if (bpp > BITSPERLONG)
	{
		int words = (bpp + BITSPERLONG - 1) / BITSPERLONG;
		int i;

		pi->uf.freemap = (uLong *)gcsimplemalloc(words * 2 *
				sizeof *pi->uf.freemap);
		pi->ur.refmap = pi->uf.freemap + words;

		/* set all our free and referenced bits to 1 */
		for (i = 0; i < words; i++)
		{
			pi->uf.freemap[i] = ~0UL;
			pi->ur.refmap[i] = ~0UL;
		}

		/* and clear the unused bits */
		if (words * BITSPERLONG != bpp)
		{
			i--;
			pi->uf.freemap[i] = (1UL << (bpp % BITSPERLONG)) - 1;
			pi->ur.refmap[i] = (1UL << (bpp % BITSPERLONG)) - 1;
		}
	}
	else						/* bpp <= BITSPERLONG */
	{
		uLong bits = (bpp == BITSPERLONG) ? ~0UL : (1UL << bpp) - 1;

		pi->uf.freebitmap = bits;
		pi->ur.refbitmap = bits;
	}

	pi->blocksize = sz;
	pi->numblocks = bpp;
	pi->u.type = type;
	pi->atomic = atomic;
	pi->link = gc.blockmap[sz];
	gc.blockmap[sz] = pi;
	return TRUE;
}

static boolean initialized = FALSE;

void
init_gcmalloc(void)
{
	char *e;
	int i;

	if (initialized)
		return;

	if (sizeof (struct pageinfo)!=1 << PAGEINFOBITS)
		gcabort("pageinfo struct is not padded correctly");

	gc.ingc = FALSE;
	gc.inalloc = FALSE;
	gc.freepagelist = NULL;
	gc.hitpagelist = NULL;
	gc.simplefreelist = NULL;
	gc.heaplow = NULL;
	gc.heaphigh = NULL;

	gc.regions = NULL;
	gc.numregions = gc.maxregions = 0;

	e = getenv("GC_AFTER_PAGES");

	if (e != NULL && *e != '\0')
		gc.pages = atoi(e);
	else
		gc.pages = 0;

	if (gc.pages < 0)
		gc.pages = 0;

	gc.pagecount = gc.pages;

	e = getenv("GC_MIN_PAGES");

	if (e != NULL && *e != '\0')
		gc.minpages = atoi(e);
	else
		gc.minpages = 4;

	if (gc.minpages < 1)
		gc.minpages = 4;

	e = getenv("GC_MIN_ALIGN");

	if (e != NULL && *e != '\0')
		gc.minalign = atoi(e);
	else
		gc.minalign = 8;

	if (gc.minalign < 1)
		gc.minalign = 8;

	e = getenv("GC_MAX_ALIGN");

	if (e != NULL && *e != '\0')
		gc.maxalign = atoi(e);
	else
		gc.maxalign = 32;

	if (gc.maxalign < gc.minalign)
		gc.maxalign = gc.minalign;

	e = getenv("GC_IGNORE_FREE");

	if (e != NULL && *e != '\0')
		gc.ignorefree = atoi(e) ? TRUE : FALSE;
	else
		gc.ignorefree = gc.pages ? TRUE : FALSE;

	e = getenv("GC_PERCENT");

	if (e != NULL && *e != '\0')
		gc.percent = atoi(e);
	else
		gc.percent = 20;

	e = getenv("GC_CLEAR_BLOCKS");

	if (e != NULL && *e != '\0')
		gc.clearblocks = atoi(e) ? TRUE : FALSE;
	else
		gc.clearblocks = FALSE;

	e = getenv("GC_SNAPSHOT");

	if (e != NULL && *e != '\0')
		gc.snapshot = atoi(e) ? TRUE : FALSE;
	else
		gc.snapshot = FALSE;

	e = getenv("GC_DUMPFILE");

	if (e != NULL && *e != '\0')
		gc.dumpfile = e;
	else
		gc.dumpfile = NULL;

#ifdef SKIPINTERIOR
	e = getenv("GC_SKIP_INTERIOR");

	if (e != NULL && *e != '\0')
		gc.skipinterior = atoi(e) ? TRUE : FALSE;
	else
		gc.skipinterior = FALSE;
#endif

	/* initialize gc.sizemap */
	gc.sizemap[0] = gc.minalign;

	for (i = 1; i < PAGETHRESHHOLD; i++)
		if (i < gc.maxalign)
			gc.sizemap[i] = ((i + gc.minalign - 1) / gc.minalign) * gc.minalign;
		else
			gc.sizemap[i] = ((i + gc.maxalign - 1) / gc.maxalign) * gc.maxalign;

	/* this is for things calling malloc in the following statements... */
	initialized = TRUE;

	/* add the data gc.regions to be searched for roots, but exclude our
	   own */
	gcaddregion(DATASTART, DATAEND);
	gcremoveregion(&gc, (char *)&gc + sizeof gc);
}

void *
gcblockalloc(size_t sz, boolean atomic, void *type, const char *file, int line)
{
	int nsz;
	struct pageinfo *ppi;
	struct pageinfo *pi;
	uLong *freemap;
	int i, b;
	unsigned int bn;
	void *ptr;

	if (!initialized)
		init_gcmalloc();

#ifdef DEBUGX
	if (file)
		fprintf(stderr, "file=%s line=%d ", file, line);
#endif

	/* we are already within our own gc*() code, so use our simple
	   allocator */
	if (gc.inalloc)
		return gcsimplemalloc(sz);

	if (sz >= PAGETHRESHHOLD)
	{
		/* block is bigger than threshhold so allocate whole pages */
		int pgs = (sz + BYTESPERPAGE - 1) / BYTESPERPAGE;
		void *base = gcallocpages(pgs, TRUE);
		struct pageinfo *pi;

#ifdef DEBUGX
		fprintf(stderr, "malloc(%d), large block\n", sz);
#endif

		if (base == NULL)
			return NULL;

		pi = getpageinfo(base);

		/* this lets us see who is calling us as a plain vanilla malloc() 
		*/
		if (gc.snapshot)
			pi->u.type = gcsnapstack(type, pgs * BYTESPERPAGE, file, line);
		else
			pi->u.type = type;

		pi->atomic = atomic;

		if (!atomic && gc.clearblocks)
			memset(base, '\0', pgs * BYTESPERPAGE);

		return base;
	}

	/* search page list for a free block of our size */
	nsz = gc.sizemap[sz];

	if (nsz < sz)
		nsz = sz;

	/* this only lets us see who is calling us as a plain vanilla malloc()
	   */
	if (gc.snapshot)
		type = gcsnapstack(type, nsz, file, line);

#ifdef DEBUGX
	fprintf(stderr, "malloc(%d), small block, sz = %d\n", sz, nsz);
#endif

	ppi = NULL;
	pi = gc.blockmap[nsz];
	freemap = NULL;

	/* search pages with blocks of the given size */
	for (; pi != NULL; ppi = pi, pi = pi->link)
	{
		int bpp;
		int freemapwords;

		if (pi->u.type != type || pi->atomic != atomic)
			continue;

		bpp = pi->numblocks;
		freemapwords = (bpp + BITSPERLONG - 1) / BITSPERLONG;
		freemap = bpp > BITSPERLONG ? pi->uf.freemap : &pi->uf.freebitmap;

		/* search for a free block in the page */
		for (i = 0; i < freemapwords; i++, freemap++)
			if (*freemap)
				break;

		if (i < freemapwords)
			break;
	}

	if (pi == NULL)
	{
		boolean collect = TRUE;

		/* garbage collect and try again, if that fails then add more
		   blocks */
		if (gc.freepagelist == NULL && gc.hitpagelist == NULL &&
				gc.pagecount <= 0 && gc.pages && !gc.ingc)
		{
			garbagecollect();
			collect = FALSE;

			/* repeat search */
			pi = gc.blockmap[nsz];

			for (ppi = NULL; pi != NULL; ppi = pi, pi = pi->link)
			{
				int bpp;
				int freemapwords;

				if (pi->u.type != type || pi->atomic != atomic)
					continue;

				bpp = pi->numblocks;
				freemapwords = (bpp + BITSPERLONG - 1) / BITSPERLONG;
				freemap = bpp > BITSPERLONG ? pi->uf.freemap :
					&pi->uf.freebitmap;

				/* search for a free block in the page */
				for (i = 0; i < freemapwords; i++, freemap++)
					if (*freemap)
						break;

				if (i < freemapwords)
					break;
			}
		}

		/* no blocks found, try to add some more */
		if (pi == NULL)
		{
			if (!getmoreblocks(nsz, atomic, type, collect))
			{
#ifdef DEBUGX
				fprintf(stderr, "malloc(%d), no more small blocks\n", sz);
#endif
				return NULL;
			}

			/* this works since getmoreblocks always sticks new pages */
			/* at the front of the list */
			pi = gc.blockmap[nsz];
			ppi = NULL;
			freemap = pi->numblocks > BITSPERLONG ? pi->uf.freemap :
				&pi->uf.freebitmap;
			i = 0;
		}
	}

	if (ppi != NULL)
	{
		/* move page with free blocks to the head of the list so we save 
		*/
		/* time on the next call. */
		ppi->link = pi->link;
		pi->link = gc.blockmap[nsz];
		gc.blockmap[nsz] = pi;
	}

	b = findlowestbit(*freemap);
	bn = i * BITSPERLONG + b;
	*freemap &= ~(1UL << b);

	ptr = (char *)pi->page + bn * pi->blocksize;

	if (!atomic && gc.clearblocks)
		memset(ptr, '\0', pi->blocksize);

#ifdef DEBUGX
	fprintf(stderr, "malloc(%d), small block=0x%X b=%d i=%d bn=%d\n",
			sz, ptr, b, i, bn);
#endif
	return ptr;
}

void *
(malloc)(size_t sz)
{
	return gcblockalloc(sz, FALSE, NULL, NULL, 0);
}

void *
gcblockpointer(void *ptr)
{
	/* return a pointer to the begining of the block that ptr is contained
	   */
	/* in.  return NULL if ptr does not point into any known block return.
	   */
	int group, page, offset, blocksize;
	struct pagegroupinfo *pgi;
	struct pageinfo *pi;
	unsigned int bn;

	getpagecomps(ptr, &group, &page, &offset);

	if (group < 0 || group >= MAXPAGEGROUPS)
		return NULL;

	pgi = gc.pagegroups[group];

	if (pgi == NULL)			/* this will only happen if ptr is invalid
								   */
		return NULL;

	pi = &pgi->pages[page];
	blocksize = pi->blocksize;

	if (blocksize == 0)
		return NULL;

	if (blocksize < 0)
		return (void *)((char *)ptr + (blocksize + 1) * BYTESPERPAGE - offset);

	bn = blocknumber(offset, blocksize);

	if (pi->numblocks > BITSPERLONG)
	{
		unsigned int i = bn / BITSPERLONG;
		unsigned int b = bn % BITSPERLONG;

		/* this will only happen if free block */
		if (pi->uf.freemap[i] & (1UL << b))
			return NULL;
	}
	else if (pi->uf.freebitmap & (1UL << bn))
		return NULL;

	return (void *)((char *)ptr - offset + bn * blocksize);
}

size_t
gcblocksize(void *ptr)
{
	/* return the size of the block pointed to by an allocated ptr. */
	/* return 0 if ptr does not point into any known block. */
	int group, page, offset;
	struct pagegroupinfo *pgi;
	struct pageinfo *pi;
	unsigned int bn;

	getpagecomps(ptr, &group, &page, &offset);

	if (group < 0 || group >= MAXPAGEGROUPS)
		return 0;

	pgi = gc.pagegroups[group];

	if (pgi == NULL)			/* this will only happen if ptr is invalid
								   */
		return 0;

	pi = &pgi->pages[page];

	if (pi->blocksize < 0)
	{
		if (pi->blocksize == -1)
			return pi->numblocks * BYTESPERPAGE;

		return 0;				/* bogus block */
	}

	bn = blocknumber(offset, pi->blocksize);

	if (bn >= pi->numblocks)
		return 0;				/* bogus pointer */

	if (pi->numblocks > BITSPERLONG)
	{
		unsigned int b = bn % BITSPERLONG;
		unsigned int i = bn / BITSPERLONG;

		/* this will only happen if free block */
		if (pi->uf.freemap[i] & (1UL << b))
			return 0;
	}
	else if (pi->uf.freebitmap & (1UL << bn))
		return 0;

	return pi->blocksize;
}

void *
gcblocktype(void *ptr)
{
	if (gcblockpointer(ptr) != ptr)
		return NULL;

	if (gc.snapshot)
		return getpageinfo(ptr)->u.snap->type;

	return getpageinfo(ptr)->u.type;
}

boolean
gcblockatomic(void *ptr)
{
	if (gcblockpointer(ptr) != ptr)
		return NULL;

	return getpageinfo(ptr)->atomic;
}

void
free(void *ptr)
{
	int group, page, offset, sz;
	struct pagegroupinfo *pgi;
	struct pageinfo *pi;
	unsigned int bn, bit;

	if (gc.ignorefree)
		return;

	getpagecomps(ptr, &group, &page, &offset);

	if (group < 0 || group >= MAXPAGEGROUPS)
	{
		gcabort("free: invalid pointer 0x%X, group=0x%X", ptr, group);
		return;
	}

	pgi = gc.pagegroups[group];

	if (pgi == NULL)			/* this will only happen if ptr is invalid
								   */
	{
		gcabort("free: invalid pointer 0x%X, pgi==NULL", ptr);
		return;
	}

	pi = &pgi->pages[page];
	sz = pi->blocksize;

	if (sz < 1)
	{
		/* if this is from our low-level allocator, handle it there */
		if (sz == 0 && pi->numblocks == SIMPLE_PAGE)
		{
#ifdef DEBUGX
			fprintf(stderr, "free(0x%X) pi=0x%X blocksize=%d numblocks=%d\n",
					ptr, pi, pi->blocksize, pi->numblocks);
#endif
			gcsimplefree(ptr);
			return;
		}

		if (sz != -1 || offset != 0)
		{
			gcabort("free: invalid pointer 0x%X, sz=0x%X offset=0x%X", ptr, sz,
					offset);
			return;
		}

		gcfreepages(ptr, pi->numblocks);
		return;
	}

	bn = blocknumber(offset, sz);

	if (offset != bn * sz || bn >= pi->numblocks)
	{
		gcabort("free: invalid pointer 0x%X, offset=0x%X bn=0x%X sz=0x%X", ptr,
				offset, bn, sz);
		return;
	}

	bit = 1UL << (bn % BITSPERLONG);

	/* this will only happen if free block */
	if (pi->numblocks > BITSPERLONG)
	{
		unsigned int i = bn / BITSPERLONG;

		if (pi->uf.freemap[i] & bit)
		{
			gcabort("free: invalid pointer 0x%X, free block, sz=%d", ptr, sz);
			return;
		}

		/* mark block as free */
		pi->uf.freemap[i] |= bit;
	}
	else if (pi->uf.freebitmap & bit)
	{
		gcabort("free: invalid pointer 0x%X, free block, sz=%d", ptr, sz);
		return;
	}
	else
		/* mark block as free */
		pi->uf.freebitmap |= bit;
}

void *
realloc(void *p, size_t nsz)
{
	void *base, *np;
	size_t sz;
	struct pageinfo *pi;

	if (p == NULL)
		return malloc(nsz);

	base = gcblockpointer(p);

	if (base != p)
	{
		gcabort("realloc: invalid pointer 0x%X", p);
		return NULL;
	}

	sz = gcblocksize(p);

	if (nsz <= sz)
		return p;

	pi = getpageinfo(p);
	np = gcblockalloc(nsz, pi->atomic, pi->u.type, NULL, 0);
	memcpy(np, p, sz);
	free(p);
	return np;
}

void *
calloc(size_t elems, size_t elsize)
{
	struct pageinfo *pi;
	size_t size = elems * elsize;
	void *p = gcblockalloc(size, TRUE, NULL, NULL, 0);

	if (p == NULL)
		return NULL;

	pi = getpageinfo(p);

	if (pi->blocksize > 0)
		memset(p, '\0', pi->blocksize);
	else if (pi->blocksize == -1)
		memset(p, '\0', pi->numblocks * BYTESPERPAGE);
	else
		gcabort("calloc: gcblockalloc returned a bogus block");

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
	int g;

	*allocspc = 0;
	*freespc = 0;
	*allocblks = 0;
	*freeblks = 0;
	*endarena = gc.heaphigh;
	*arenasz = (char *)gc.heaphigh - (char *)gc.heaplow;

	for (fp = gc.freepagelist; fp != NULL; fp = fp->link)
	{
		(*freeblks)++;
		*freespc += fp->uf.count * BYTESPERPAGE;
	}

	for (fp = gc.hitpagelist; fp != NULL; fp = fp->link)
	{
		(*freeblks)++;
		*freespc += BYTESPERPAGE;
	}

	for (g = 0; g < MAXPAGEGROUPS; g++)
		if (gc.pagegroups[g] != NULL)
		{
			int p;
			struct pagegroupinfo *pgi = gc.pagegroups[g];

			for (p = 0; p < PAGESPERGROUP; p++)
			{
				struct pageinfo *pi = &pgi->pages[p];

				if (pi->blocksize > 0)
				{
					int b;
					int bpp = pi->numblocks;
					uLong *freemap = bpp > BITSPERLONG ? pi->uf.freemap :
					      &pi->uf.freebitmap;

					for (b = 0; b < bpp; b++, freemap++)
						if (freemap[b / BITSPERLONG] &
								(1UL << (b % BITSPERLONG)))
						{
							(*freeblks)++;
							*freespc += pi->blocksize;
						}
						else
						{
							(*allocblks)++;
							*allocspc += pi->blocksize;
						}
				}
				else if (pi->blocksize == -1)
				{
					(*allocblks)++;
					*allocspc += pi->numblocks * BYTESPERPAGE;
				}
			}
		}
}

#define gcinregion(p, start, end)	\
	((char*)(start) <= (char*)(p) && (char*)(p) < (char*)(end))

#define gcregionsoverlap(start1, end1, start2, end2)	\
	(gcinregion(start1, start2, end2) || gcinregion(end1, start2, end2)	\
		|| gcinregion(start2, start1, end1) || gcinregion(end2, start1, end1))

void
gcaddregion(void *start, void *end)
{
	int empty = -1;
	int i;

	if (start == NULL || end == NULL)
		return;

	/* search region table to see if this region is next to an existing
	   region */
	for (i = 0; i < gc.numregions; i++)
		if (gc.regions[i].start == NULL)
			empty = i;
		else if (gcregionsoverlap(start, end,
					gc.regions[i].start, gc.regions[i].end))
			break;

	if (i < gc.numregions)
	{
		void *rstart = gc.regions[i].start;
		void *rend = gc.regions[i].end;

		if ((char *)start < (char *)rstart)
			rstart = start;

		if ((char *)end > (char *)rend)
			rend = end;

	gc.regions[i].start = gc.regions[i].end = NULL;
		gcaddregion(rstart, rend);
		return;
	}

	if (empty == -1)
	{
		if (gc.numregions >= gc.maxregions)
		{
			int newlen = gc.maxregions + REGIONBUMP;
			int i;

			gc.regions = (struct region *)gcsimplerealloc(gc.regions,
				newlen *sizeof *gc.regions);

			if (gc.regions == NULL)
				return;

			for (i = gc.maxregions; i < newlen; i++)
			{
				gc.regions[i].start = NULL;
				gc.regions[i].end = NULL;
			}

			gc.maxregions = newlen;
		}

		empty = gc.numregions++;
	}

	gc.regions[empty].start = start;
	gc.regions[empty].end = end;
}

void
gcremoveregion(void *start, void *end)
{
	int i;
	void *rstart, *rend;

	for (i = 0; i < gc.numregions; i++)
		if (gcregionsoverlap(start, end,
					gc.regions[i].start, gc.regions[i].end))
			break;

	if (i >= gc.numregions)
		return;

	rstart = gc.regions[i].start;
	rend = gc.regions[i].end;

	if ((char *)start > (char *)rstart)
	{
		gc.regions[i].end = start;

		if ((char *)end < (char *)rend)
			gcaddregion(end, rend);
22: Unmatched else
		else if ((char *)end > (char *)rend)
			gcremoveregion(rend, end);
	}
	else if ((char *)end < (char *)rend)
	{
		gc.regions[i].start = end;
		gc.regions[i].end = rend;

		if ((char *)start < (char *)rstart)
			gcremoveregion(start, rstart);
	}
	else
	{
		gc.regions[i].start = gc.regions[i].end = NULL;

		if ((char *)start < (char *)rstart)
			gcremoveregion(start, rstart);

		if ((char *)end > (char *)rend)
			gcremoveregion(rend, end);
	}
}

static void
gcdischargepage(struct pageinfo *p)
{
	int sz = p->blocksize;
	struct pageinfo *pi;
	struct pageinfo *prev;

	if (sz <= 0)
	{
		gcabort("gcdischargepage: block size is not positive, blocksize=%d",
				p->blocksize);
		return;
	}

	pi = gc.blockmap[sz];
	prev = NULL;

	for (; pi != NULL; prev = pi, pi = pi->link)
		if (pi == p)
			break;

	if (pi == NULL)
	{
		gcabort("gcdischargepage: page is not on block map, 0x%X", p);
		return;
	}

	if (prev == NULL)
		gc.blockmap[sz] = p->link;
	else
		prev->link = p->link;

	if (p->numblocks > BITSPERLONG)
		gcsimplefree(p->uf.freemap);

	gcfreepages(p->page, 1);
}

static void
gcmarkregion(void *start, void *end)
{
#define STACK 128
	void *startstk[STACK];
	void *endstk[STACK];
	char *low = (char *)gc.heaplow;
	char *high = (char *)gc.heaphigh - sizeof (char *);
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
			ptrdiff_t delta;
			int group;
			struct pagegroupinfo *pgi;
			struct pageinfo *pi;
			void *pp;
			size_t bb;

			if (p < low || p > high)
				continue;

			delta = (char *)p - (char *)PTRBASEADDR;
			group = delta >> GROUPSHIFT;

			if (group < 0 || group >= MAXPAGEGROUPS)
				continue;

			pgi = gc.pagegroups[group];

			if (pgi == NULL)
				continue;

			pi = &pgi->pages[(delta >> PAGESHIFT) & PAGEMASK];

			if (pi->blocksize == 0)
			{
				pi->history |= 1;	/* flag the hit */
				continue;
			}

			if (pi->blocksize < 0)
			{
				pp = (char *)p + (pi->blocksize + 1) * BYTESPERPAGE -
					(delta & OFFSETMASK);

#ifdef SKIPINTERIOR
				if (gc.skipinterior && pp != p)
					continue;
#endif

				pi = getpageinfo(pp);

				if (pi->ur.refbitmap)
					continue;

				pi->ur.refbitmap = 1;
				bb = pi->numblocks * BYTESPERPAGE;
#ifdef DEBUG
				printf("gcmarkregion: st=0x%X e=0x%X s=0x%X p=0x%X pp=0x%X bb=%d\n",
						st, e, s, p, pp, bb);
#endif
			}
			else
			{
				unsigned int bn;

				bb = pi->blocksize;
				bn = blocknumber(delta & OFFSETMASK, bb);

				/* if the last byte of this block is outside a page, it is
				   bogus */
				if (bn >= pi->numblocks)
				{
					pi->history |= 1;	/* flag the hit */
					continue;
				}

				if (!pi->atomic)
				{
					pp = (char *)p - (delta & OFFSETMASK) + bn * bb;

#ifdef SKIPINTERIOR
					if (gc.skipinterior && pp != p)
						continue;
#endif
				}

				if (pi->numblocks > BITSPERLONG)
				{
					unsigned int i = bn / BITSPERLONG;
					uLong bit = 1UL << (unsigned int)(bn % BITSPERLONG);

					if (pi->uf.freemap[i] & bit)
					{
						pi->history |= 1;	/* flag the hit */
						continue;
					}

					if (pi->ur.refmap[i] & bit)
						continue;

					pi->ur.refmap[i] |= bit;
#ifdef DEBUG
					printf("gcmarkregion: st=0x%X e=0x%X s=0x%X p=0x%X pp=0x%X bb=%d nb=%d bn=%d\n",
							st, e, s, p, pp, bb, pi->numblocks, bn);
#endif
				}
				else			/* pi->numblocks <= BITSPERLONG */
				{
					uLong bit = 1UL << bn;

					if (pi->uf.freebitmap & bit)
					{
						pi->history |= 1;	/* flag the hit */
						continue;
					}

					if (pi->ur.refbitmap & bit)
						continue;

					pi->ur.refbitmap |= bit;
#ifdef DEBUG
					printf("gcmarkregion: st=0x%X e=0x%X s=0x%X p=0x%X pp=0x%X bb=%d nb=%d bn=%d\n",
							st, e, s, p, pp, bb, pi->numblocks, bn);
#endif
				}
			}

			if (!pi->atomic)
			{
				if (tos >= STACK)
					gcmarkregion(pp, (char *)pp + bb);
				else
				{
					startstk[tos] = pp;
					endstk[tos] = (char *)pp + bb;
					tos++;
				}
			}
		}
	}
}

void
rebuildfreelist(void)
{
	/* select pages for freepagelist and hitpagelist based upon history */
	struct pageinfo *freepage = gc.freepagelist;
	struct pageinfo *hitpage = gc.hitpagelist;
	struct pageinfo *lastfreepage = NULL;
	struct pageinfo *lasthitpage = NULL;

	gc.freepagelist = NULL;
	gc.hitpagelist = NULL;

	while (freepage != NULL)
	{
		struct pageinfo *link = freepage->link;
		void *firstpage = freepage->page;
		struct pageinfo *firstpi = getpageinfo(firstpage);
		void *page = firstpage;
		int numpages = freepage->uf.count;
		int nohitcount = 0;
		int i;

		for (i = 0; i < numpages; i++, page = (char *)page + BYTESPERPAGE)
		{
			struct pageinfo *pi = getpageinfo(page);

			if (pi->history & HISTORY)
			{
				if (nohitcount)
				{
					if (lastfreepage == NULL)
						gc.freepagelist = firstpi;
					else
						lastfreepage->link = firstpi;

					firstpi->link = NULL;
					firstpi->uf.count = nohitcount;
					lastfreepage = firstpi;
				}

				if (lasthitpage == NULL)
					gc.hitpagelist = pi;
				else
					lasthitpage->link = pi;

				lasthitpage = pi;
				pi->numblocks = HIT_PAGE;
				firstpage = (char *)page + BYTESPERPAGE;
				firstpi = getpageinfo(firstpage);
				nohitcount = 0;
			}
			else
				nohitcount++;
		}

		if (nohitcount)
		{
			if (lastfreepage == NULL)
				gc.freepagelist = firstpi;
			else
				lastfreepage->link = firstpi;

			firstpi->link = NULL;
			firstpi->uf.count = nohitcount;
			lastfreepage = firstpi;
		}

		freepage = link;
	}

	while (hitpage != NULL)
	{
		struct pageinfo *link = hitpage->link;

		if (hitpage->history & HISTORY)
		{
			if (lasthitpage == NULL)
				gc.hitpagelist = hitpage;
			else
				lasthitpage->link = hitpage;

			hitpage->link = NULL;
			lasthitpage = hitpage;
		}
		else
			gcfreepages(hitpage->page, 1);

		hitpage = link;
	}
}

/* garbage collect using mark & sweep */
void
garbagecollect(void)
{
	long arg;					/* to get the top of stack */
	int g, i;
	size_t totfree;
	int arenasz, expandcnt;
	static jmp_buf jmp;

	if (gc.ingc)
		return;

	gc.ingc = TRUE;
#ifdef DEBUG
	gcdebug();
#endif

	/* clear ref bits */
	for (g = 0; g < MAXPAGEGROUPS; g++)
		if (gc.pagegroups[g] != NULL)
		{
			struct pagegroupinfo *pgi = gc.pagegroups[g];
			int p;

			for (p = 0; p < PAGESPERGROUP; p++)
			{
				struct pageinfo *pi = &pgi->pages[p];

				pi->history <<= 1;

				if (pi->blocksize > 0)
				{
					int bpp = pi->numblocks;

					if (bpp > BITSPERLONG)
					{
						int words = (bpp + BITSPERLONG - 1) / BITSPERLONG;
						int w;

						for (w = 0; w < words; w++)
							pi->ur.refmap[w] = 0;
					}
					else
						pi->ur.refbitmap = 0;
				}
				else
					pi->ur.refbitmap = 0;
			}
		}

	/* this does not go in our "gc" struct as we really do want it
	   searched */
	memset(jmp, 0, sizeof jmp);
	setjmp(jmp);

	/* set ref bits of referenced blocks */
	gcmarkregion(STACKSTART, STACKEND);

	for (i = 0; i < gc.numregions; i++)
	{
		if (gc.regions[i].start == NULL)
			continue;

		gcmarkregion(gc.regions[i].start, gc.regions[i].end);
	}

	totfree = 0;

	/* release unused memory */
	for (g = 0; g < MAXPAGEGROUPS; g++)
		if (gc.pagegroups[g] != NULL)
		{
			struct pagegroupinfo *pgi = gc.pagegroups[g];
			int p;

			for (p = 0; p < PAGESPERGROUP; p++)
			{
				struct pageinfo *pi = &pgi->pages[p];

				if (pi->blocksize > 0)
				{
					int bpp = pi->numblocks;

					if (bpp > BITSPERLONG)
					{
						int words = (bpp + BITSPERLONG - 1) / BITSPERLONG;
						boolean used = FALSE;
						size_t f = totfree;
						int w;

						for (w = 0; w < words; w++)
						{
							pi->uf.freemap[w] |= ~pi->ur.refmap[w];
							f += bitcount(pi->uf.freemap[w]) * pi->blocksize;

							if (pi->ur.refmap[w])
								used = TRUE;
						}

						if (used)
						{
							if (bpp % BITSPERLONG)
								pi->uf.freemap[words - 1] &=
									((1UL << (bpp % BITSPERLONG)) - 1);
							totfree = f;
						}
						else
							gcdischargepage(pi);
					}
					else
					{
						if (pi->ur.refbitmap)
						{
							pi->uf.freebitmap |= ~pi->ur.refbitmap;
							totfree += bitcount(pi->uf.freebitmap) *
								pi->blocksize;

							if (bpp != BITSPERLONG)
								pi->uf.freebitmap &= (1UL << bpp) - 1;
						}
						else
							gcdischargepage(pi);
					}

					continue;
				}

				if (pi->blocksize == -1 && pi->ur.refbitmap == 0)
				{
#ifdef DEBUG
					printf("garbagecollect: page=0x%X blocksize=%d numblocks=%d\n",
							pi->page, pi->blocksize, pi->numblocks);
#endif
					gcfreepages(pi->page, pi->numblocks);
				}
			}
		}

	rebuildfreelist();
	gc.ingc = FALSE;

	arenasz = ((char *)gc.heaphigh - (char *)gc.heaplow) / BYTESPERPAGE;
	expandcnt = arenasz * gc.percent;
	expandcnt /= 100;

	if (gc.freepagelist == NULL)
	{
		/* if garbage collection didn't yield a page then allow */
		/* heap to grow by gc.page or 20% which ever is larger. */
		gc.pagecount = (gc.pages > expandcnt ? gc.pages : expandcnt);
	}
	else
	{
		/* if garbage collection yielded less than 20% of the heap */
		/* then give the heap some extra growing room. */
		struct pageinfo *fp;

		expandcnt -= totfree / BYTESPERPAGE;

		for (fp = gc.freepagelist; fp != NULL; fp = fp->link)
			expandcnt -= fp->uf.count;

		if (gc.pagecount < expandcnt)
			gc.pagecount = expandcnt;
	}

#ifdef DEBUG
	gcdebug();
#endif
}

void
fini_gcmalloc(void)
{
	FILE *fp;

	if (!initialized)
		return;

	if (gc.dumpfile == NULL)
		return;

	fp = fopen(gc.dumpfile, "w");

	if (fp == NULL)
		fp = stderr;

	gcdebug(fp);
	fclose(fp);
}
