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
#ifndef __GCDEFS_H_
#define __GCDEFS_H_

#include <hash.h>

#ifdef bsdi
extern etext, edata, end;
extern const char **environ;
#define DATASTART ((void*)&etext)
#define DATAEND   ((void *)&end)
#define STACKSTART ((void *)&arg)	/* variable declared on stack */
#define STACKEND   ((void *)environ)
#define PTRBASEADDR ((void *)&edata)
#define PTRBUMP (sizeof (char*))
#endif

#ifdef _AUX_SOURCE
extern etext, edata, end;
extern const char **environ;
#define DATASTART ((void*)(((long)&etext + 0x3FFFFF) & ~0x3FFFFF) \
		    + ((long)&etext & 0x1FFF))
#define DATAEND   ((void *)&end)
#define STACKSTART ((void *)&arg)
#define STACKEND   ((void *)environ)
#define PTRBASEADDR ((void *)(((long)&edata + 0x1FFF) & ~0x1FFF))
#define PTRBUMP (sizeof (short))
#endif

#ifdef _DJGPP_SOURCE_
extern end;
#define PTRBASEADDR ((void *)0x400000)
#define DATASTART ((void *)0x400000)
#define DATAEND   ((void *)&end)
#define STACKSTART ((void *)&arg)
#define STACKEND   ((void *)0x80000000)
/*#define PTRMASK (~3U) */
#define PTRBUMP (sizeof (char*))
#endif

#ifndef PTRBASEADDR
#define PTRBASEADDR ((void *)0x400000)
#define DATASTART ((void *)0x400000)
#define DATAEND   ((void *)0x402000)
#define STACKSTART ((void *)0x400000)
#define STACKEND   ((void *)0x402000)
#define PTRMASK (~3U)
#define PTRBUMP (sizeof (char*))
#endif

enum pagetype
{
	UNUSED_PAGE,
	FREE_PAGE,
	HIT_PAGE,
	SIMPLE_PAGE,
	HEADER_PAGE,
	DATA_PAGE,
};

/* free page list keeps track of what blocks of pages are free. */
struct freeelem
{
	struct freeelem *link;
	int count;
};

#define SIMPLEALIGN (sizeof (struct freeelem))
#define SIMPLEBLOCK (2 * SIMPLEALIGN)

/* xxxx xxxx xxxx xxxx  xxxx xxxx xxxx xxxx */
/* xxgg gggg gggg pppp  pppp bbbb bbbb bbbb */

/* pageinfo keeps track of per page information and free block lists */
/* must be exactly 32 bytes in size for speed when decoding an address */
struct pageinfo
{
	int blocksize;	/* less than zero if multi page block, zero if free */
	int numblocks;	/* number of blocks in this page, or # pages in block */
	void *page;		/* pointer to actual page mapped by this header */
	union
	{
		struct gcstackelt *snap;	/* stack snapshot */
		void *type;		/* caller specified type */
	} u;
	union 
	{
		uLong *freemap;
		uLong freebitmap;
		int count;	/* for freelist, goes here rather than with refmap so */
					/* refbitmap can be cleared on freepages in */
					/* garbagecollect */
	} uf;
	union
	{
		uLong *refmap;
		uLong refbitmap;
	} ur;
	struct pageinfo *link; /* to link pages together */
	char history;	/* free block mark history */
	char atomic;	/* flag to indicate atomic block */
	char pad1;		/* pad out to 32 bytes */
	char pad2;
};

#define PAGEINFOBITS 5
#define BYTESPERPAGE 4096
#define PAGESHIFT 12
#define HISTORY 0x07	/* amount of history to check for hit page list */
#define PAGEMASK ((1UL << (PAGESHIFT - PAGEINFOBITS)) - 1)
#define GROUPSHIFT (PAGESHIFT + PAGESHIFT - PAGEINFOBITS)
#define OFFSETMASK ((1UL << PAGESHIFT) - 1)

#define PAGESPERGROUP (BYTESPERPAGE / sizeof (struct pageinfo))
#define BYTESPERPAGEGROUP (PAGESPERGROUP * BYTESPERPAGE)

struct pagegroupinfo
{
	struct pageinfo pages[PAGESPERGROUP];
};

#define MAXPAGEGROUPS 64
#define MAXALLOCSIZE (MAXPAGEGROUPS * PAGESPERGROUP * BYTESPERPAGE)
#define PAGETHRESHHOLD (BYTESPERPAGE / 2)

/* for gcsnapstack() and gcstackelt */
#define STACK_SIZE 11

/* element for a pool - size should divide into BYTESPERPAGE with link */
typedef struct gcstackelt
{
	void *type;					/* caller specified type */
	const char *file;
	int line;
	uLong stack[STACK_SIZE];
	long numcalls;
	long totmem;
} gcstackelt;

declare_hashtable(gcstackpool, gcstackiter, gcstackelt, gcstackelt*);

/* garbage collection stuff */
#define REGIONBUMP 16

struct region
{
	void *start;				/* start of region */
	void *end;					/* end of region + 1 */
};

/* globals for memory allocator are in one chunk of memory to */
/* avoid walking its own data when in gcmarkregion() */
struct gcglobals
{
	struct pagegroupinfo *pagegroups[MAXPAGEGROUPS];
	struct pageinfo *blockmap[PAGETHRESHHOLD + 1];
	int sizemap[PAGETHRESHHOLD];
	struct pageinfo *freepagelist;
	struct pageinfo *hitpagelist;
	struct freeelem *simplefreelist;
	void *heaplow;
	void *heaphigh;
	int minpages;
	int minalign;
	int maxalign;
	int pages;
	int pagecount;
	int percent;
	struct region *regions;
	int numregions;
	int maxregions;
	boolean ingc;
	boolean inalloc;
	boolean ignorefree;
	boolean clearblocks;
	boolean snapshot;
	const char *dumpfile;
	ptrdiff_t delta;			/* temp var for macros below */
#ifdef SKIPINTERIOR
	boolean skipinterior;
#endif
	gcstackpool *stackpool;
};

extern struct gcglobals gc;
extern void init_gcmalloc(void);
extern void fini_gcmalloc(void);

/* break an address into its components */
#define getpagecomps(p, group, page, offset)	\
	(void)((gc.delta = (char *)(p) - (char *)PTRBASEADDR),	\
	(*(group) = gc.delta >> GROUPSHIFT),	\
	(*(page) = (gc.delta >> PAGESHIFT) & PAGEMASK),	\
	(*(offset) = gc.delta & OFFSETMASK))

/* return page group descriptor for a given address */
#define getpagegroupinfo(p)	\
    (gc.pagegroups[((char *)(p) - (char *)PTRBASEADDR) >> GROUPSHIFT])

/* return page descriptor for a given address */
#define getpageinfo(p)	\
	((gc.delta = (char *)(p) - (char *)PTRBASEADDR),	\
	(&gc.pagegroups[gc.delta >> GROUPSHIFT]->pages[(gc.delta >>	\
		PAGESHIFT) & PAGEMASK]))

/* return pointer to specified page */
#define getpage(group, page) 	\
	(void *)((char *)PTRBASEADDR +	\
		((group) << GROUPSHIFT | (page) << PAGESHIFT))

#define blocknumber(offset, blocksize)	\
    ((offset) / (unsigned int)(blocksize))

/* gcmalloc.c: */
void *gcallocpages(int pages, boolean collect);
void gcfreepages(void *ptr, int numpgs);
void *gcsimplemalloc(size_t size);
void *gcsimplerealloc(void *p, size_t nsz);
void gcsimplefree(void *ptr);
void gcabort(char const *fmt, ...);

/* gcsnap.c: */
void *gcsnapstack(void *type, long mem, const char *file, int line);
extern void gcdumpsnapstack(FILE *fp);

/* gcdebug.c: */
extern void gcdebug(FILE *fp);

#endif /* __GCDEFS_H_ */
