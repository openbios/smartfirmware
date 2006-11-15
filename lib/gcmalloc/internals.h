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

// Copyright (c) 1992 by TJ Merritt and Parag Patel.  All Rights Reserved.
// $Header: /u/cgt/cvs/lib/gcmalloc/internals.h,v 1.11 1998/02/17 00:37:24 tjm Exp $
#ifndef __INTERNALS_H_
#define __INTERNALS_H_

#ifndef __HASHTABLE_H_
#include <hashtable.h>
#endif

#ifdef _AUX_SOURCE
extern etext, edata, end;
#define DATASTART ((void*)(((long)&etext + 0x3FFFFF) & ~0x3FFFFF) \
		    + ((long)&etext & 0x1FFF))
#define DATAEND   ((void *)&end)
#define STACKSTART ((void *)&arg)
#define STACKEND   ((void *)environ)
//#define PTRBASEADDR ((void *)(((long)&edata + 0x1FFF) & ~0x1FFF))
#define PTRBASEADDR ((void *)(((long)&edata + (BYTESPERPAGE - 1)) & ~(BYTESPERPAGE - 1)))
#define PTRBUMP (sizeof (short))
#endif

#ifdef _DJGPP_SOURCE_
extern end;
#define PTRBASEADDR ((void *)0x400000)
#define DATASTART ((void *)0x400000)
#define DATAEND   ((void *)&end)
#define STACKSTART ((void *)&arg)
#define STACKEND   ((void *)0x80000000)
//#define PTRMASK (~3U)
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

// free page list keeps track of what blocks of pages are free.
struct freeelem
{
    struct freeelem *link;
    int count;
};

#define SIMPLEALIGN (sizeof (struct freeelem))
#define SIMPLEBLOCK (2 * SIMPLEALIGN)

// xxxx xxxx xxxx xxxx  xxxx xxxx xxxx xxxx
// xxgg gggg gggg pppp  pppp bbbb bbbb bbbb

// pageinfo keeps track of per page information and free block lists
// must be exactly 32 bytes in size for speed when decoding an address
struct pageinfo
{
    int blocksize;	// less than zero if multi page block, zero if free
    int numblocks;	// number of blocks in this page, or # pages in block
    void *page;		// pointer to actual page mapped by this header
    union
    {
    	struct Stack_elt *snap;	// stack snapshot
	void *type;		// caller specified type
    };
    union 
    {
	uLong *freemap;
	uLong freebitmap;
	int count;	// for freelist, goes here rather than with refmap so
			// refbitmap can be cleared on freepages in
			// garbagecollect
    };
    union
    {
	uLong *refmap;
	uLong refbitmap;
    };
    struct pageinfo *link; // to link pages together
    char history;	// free block mark history
    char atomic;	// flag to indicate atomic block
    char pad1;		// pad out to 32 bytes
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

// mainly for gcsnapstack()
#define STACK_SIZE 11

// element for a pool - size should divide into BYTESPERPAGE with link
struct Stack_elt
{
    void *type;		// caller specified type
    const char *file;
    int line;
    uLong stack[STACK_SIZE];
    long numcalls;
    long totmem;

    Stack_elt() { memset(this, '\0', sizeof *this); }
    Stack_elt(Stack_elt const &e) { memcpy(this, &e, sizeof *this); }
    boolean operator==(Stack_elt const &e)
	    { return file == e.file && line == e.line && type == e.type &&
		    !memcmp(stack, e.stack, sizeof stack); }
};

declare_hashtable(Stack_pool, Stack_iter, Stack_elt, Stack_elt)
extern Stack_pool *gcstackpool;

// garbage collection stuff
#define REGIONBUMP 16

struct region
{
    void *start;	// start of region
    void *end;		// end of region + 1
};

// gcglobals for memory allocator are in one chunk of memory to more easily
// avoid walking its own data when in gcmarkregion()
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
#ifdef SKIPINTERIOR
    boolean skipinterior;
#endif

    gcglobals() { }
    ~gcglobals();
};

extern struct gcglobals gc;

// break an address into its components
inline void
getpageinfo(void *p, int &group, unsigned int &page, unsigned int &offset)
{
    ptrdiff_t delta = (char *)p - (char *)PTRBASEADDR;
    group = delta >> GROUPSHIFT;
    page = (delta >> PAGESHIFT) & PAGEMASK;
    offset = delta & OFFSETMASK;
}

// return page group descriptor for a given address
inline struct pagegroupinfo *
getpagegroupinfo(void *p)
{
    return gc.pagegroups[((char *)p - (char *)PTRBASEADDR) >> GROUPSHIFT];
}

// return page descriptor for a given address
inline struct pageinfo *
getpageinfo(void *p)
{
    ptrdiff_t delta = (char *)p - (char *)PTRBASEADDR;
    return &gc.pagegroups[delta >> GROUPSHIFT]->pages[(delta >> PAGESHIFT) &
	    PAGEMASK];
}

inline void *
getpage(int group, int page)
{
    return (struct pageinfo *)((char *)PTRBASEADDR +
	    (group << GROUPSHIFT | page << PAGESHIFT));
}

inline unsigned int
blocknumber(unsigned int offset, int blocksize)
{
#ifdef _DJGPP_SOURCE_
    switch (blocksize)
    {
    case 1: return offset;
    case 2: return offset >> 1;
    case 4: return offset >> 2;
    case 8: return offset >> 3;
    case 16: return offset >> 4;
    case 32: return offset >> 5;
    case 64: return offset >> 6;
    case 128: return offset >> 7;
    case 256: return offset >> 8;
    case 512: return offset >> 9;
    case 1024: return offset >> 10;
    }
#endif
    return offset / (unsigned int)blocksize;
}

// gcmalloc.cc:
void *gcallocpages(int pages, boolean collect = FALSE);
void gcfreepages(void *ptr, int numpgs);
void *gcsimplemalloc(size_t size);
void *gcsimplerealloc(void *p, size_t nsz);
void gcsimplefree(void *ptr);
void gcabort(char const *fmt, ...);

// gcsnap.cc:
void *gcsnapstack(void *type, long mem, const char *file, int line);
extern void gcdumpsnapstack(FILE *fp = stdout);

// gcdebug.cc:
extern void gcdebug(FILE *fp = stdout);
extern void gcdump(FILE *fp = stdout);

// gcio.cc:
boolean gcwrite(char const *filename = NULL);
void gcreleaseheapinfo(Stack_pool *&stackpool, int pagesize, int numpagegroups,
	struct pagegroupinfo *&pagegroups, uLong **bitmaps = NULL);
boolean gcread(char const *filename, int &pagesize, int &numpagegroups,
	Stack_pool *&stackpool, struct pagegroupinfo **&pagegroups);

#endif // __INTERNALS_H_
