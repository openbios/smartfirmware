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

/* Copyright 1994 by Parag Patel.  All Rights Reserved. */
/* $Header: /u/cgt/cvs/lib/aoa/gcms.c,v 1.8 1999/09/29 04:10:01 parag Exp $ */

#include <stddef.h>
#include <stdlib.h>
#include <stdio.h>
#include <memory.h>
#include <string.h>
#include <stdarg.h>
#include <setjmp.h>
#include <unistd.h>
#include "aoa.h"
#include "gcext.h"
#include "gcbase.h"


/* this may safely be cast to a gc_pageinfo or a Pageinfo */
struct gcms_pageinfo
{
    struct gc_pageinfo g;
    uLong *refbits;		/* array of bits for mark/sweep */
    unsigned long unused2;
};

struct gc_root
{
    char *start;
    char *end;
    struct gc_root *next;
};

static struct gc_root *gc_root = NULL;
static const int *g_blocksizes = NULL;
static const int *g_blockcounts = NULL;
static boolean g_ingc = FALSE;
static size_t g_totfree = 0;
static void *g_heaplow = NULL;
static void *g_heaphigh = NULL;


#define BLOCKNUM(pinfo, p)    \
    ((((uLong)((char*)(p) - (char*)(pinfo))) - GC_PAGE_OVERHEAD) \
	/ g_blocksizes[(pinfo)->g.p.blkclass])


static void
gc_ms_init_page(struct gc_interface *self, struct gc_pageinfo *page,
    void *vtype)
{
    struct gcms_pageinfo *g = (struct gcms_pageinfo *)page;
    struct gc_type_info *type = (struct gc_type_info *)vtype;
    int bcount;

    if (g == NULL || type == NULL)
	gc_abort("gc_initpage(): NULL gcms_pageinfo or gc_type_info");

    if (g_blocksizes == NULL)
    {
	g_blocksizes = gc_base_blocksizes();
	g_blockcounts = gc_base_blockcounts();
    }

    bcount = g_blockcounts[page->p.blkclass];
    g->g.interface = self;
    g->g.type = type;
    g->refbits = (uLong *)calloc(sizeof (uLong), bcount / BITSPERLONG + 1);

    if (g->refbits == NULL)
	gc_abort("gc_initpage(): cannot allocate refbits");
}

static void
gc_ms_destroy_page(struct gc_interface *self, struct gc_pageinfo *page)
{
    struct gcms_pageinfo *g = (struct gcms_pageinfo *)page;

    if (g->refbits)
    {
	free(g->refbits);
	g->refbits = NULL;
    }

    g->g.type = NULL;
    g->g.interface = NULL;
}

static void
gc_ms_delroot(struct gc_interface *self, char *ptr)
{
    struct gc_root *p = NULL;
    struct gc_root *r = gc_root;

    for (; r != NULL && r->start != ptr; (p = r), r = r->next)
	;

    if (r != NULL)
    {
	if (p == NULL)
	    gc_root = r->next;
	else
	    p->next = r->next;

	free(r);
    }
}

static void
gc_ms_addroot(struct gc_interface *self, char *start, char *end)
{
    struct gc_root *r = malloc(sizeof *r);

    if (r == NULL)
	gc_abort("gc_ms_addroot(): cannot allocate gc_root");

    gc_ms_delroot(self, start);

    if (start < end)
    {
	r->start = start;
	r->end = end;
    }
    else
    {
	r->start = end;
	r->end = start;
    }

    r->next = gc_root;
    gc_root = r;
}

static void
gc_mark_region(struct gc_interface *self, void *start, void *end)
{
#define STACK 96
    void *startstk[STACK];
    void *endstk[STACK];
    char *low = (char *)g_heaplow;
    char *high = (char *)g_heaphigh;
    int tos = 1;
    int i;

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
	    struct gcms_pageinfo *pi;
	    void *pp;
	    size_t bb;

	    if (p < low || p > high)
		continue;

	    pi = (struct gcms_pageinfo *)GETPAGEINFO(p);

	    if (!gc_is_gc_page(p) || pi->g.interface != self ||
		    pi->g.type->atomic == ATOMIC)
		continue;

	    if (pi->g.p.blkclass == MULTIPAGE)    /* big block */
	    {
		if (pi->refbits[0])    /* already marked and walked */
		    continue;

		pi->refbits[0] = 1;
		bb = pi->g.p.count * PAGESIZE;
		num = 1;
		pp = (char *)pi + bb - GC_PAGE_OVERHEAD;
	    }
	    else		/* regular block */
	    {
		bb = g_blocksizes[pi->g.p.blkclass];
		num = (p - (char *)pi - GC_PAGE_OVERHEAD) / bb;
		pp = (char *)pi + num * bb + GC_PAGE_OVERHEAD;

		if (pp != p || num > g_blockcounts[pi->g.p.blkclass])
		    continue;

		if (GETBIT(pi->refbits, num))
				/* already marked and walked */
		    continue;

		SETBIT(pi->refbits, num);
	    }

	    if ((char *)pp < low || (char *)pp > high)
		gc_abort("gc_mark_region(): block pointer pp not in heap");

	    if ((char *)pp + bb < low || (char *)pp + bb > (char *)g_heaphigh)
		gc_abort("gc_mark_region(): end of block ptr pp not in heap");

	    /* anything could be a pointer in here, so mark the whole
	       block */
	    if (pi->g.type->atomic == UNKNOWN_ATOMIC)
	    {
		if (tos >= STACK)
		    gc_mark_region(self, pp, (char *)pp + bb);
		else
		{
		    startstk[tos] = pp;
		    endstk[tos] = (char *)pp + bb;
		    tos++;
		}

		continue;
	    }

	    /* walk only the specific offsets that are pointers */
	    for (i = 0; i < pi->g.type->num_offsets; i++)
	    {
		pp = (char *)p + pi->g.type->offsets[i];

		if (tos >= STACK)
		    gc_mark_region(self, pp, (char *)pp + sizeof (char *));
		else
		{
		    startstk[tos] = pp;
		    endstk[tos] = (char *)pp + sizeof (char *);
		    tos++;
		}
	    }
	}
    }
}

static boolean
gc_ms_need_collect(struct gc_interface *self)
{
    return TRUE;
#ifdef COMMENT
    size_t allocspc;
    size_t freespc;
    size_t allocblks;
    size_t freeblks;
    void *endarena = NULL;
    size_t arenasize;
    int expandcnt;

    meminfo(&allocspc, &freespc, &allocblks,
	    &freeblks, &endarena, &arenasize);
    expandcnt = arenasize / PAGESIZE * 10;    /* g.percent */
    expandcnt /= 100;

    /* if last garbage collection didn't yield a page then allow
       collection */
    if (g_totfree == 0)
	return TRUE;

    /* if garbage collection yielded less than g.percent of the heap */
    /* then give the heap some extra growing room. */
    expandcnt -= (g_totfree + freespc) / PAGESIZE;
    expandcnt -= 10;	    /* g.numfreepages */
    return expandcnt <= 0 ? TRUE : FALSE;
#endif
}

/* garbage collect using mark & sweep */
static void
gc_ms_collect(struct gc_interface *self)
{
    long stacktop;		/* to get the top of stack */
    int i, numpgs;
    struct gcms_pageinfo *page;
    jmp_buf jmp;
    struct gc_root *root;

    if (g_ingc)
	return;

    g_blocksizes = gc_base_blocksizes();
    g_blockcounts = gc_base_blockcounts();
    g_heaplow = aoa_get_heaplow();
    g_heaphigh = aoa_get_heaphigh();
    g_ingc = TRUE;
    g_totfree = 0;

    /* ref bits should already be cleared since we clear them upon return
       to avoid doing an extra pass over all of memory */

    /* this makes sure that registers are searched (depending on setjmp) 
    */
    memset(jmp, 0, sizeof jmp);
    setjmp(jmp);

    /* set ref bits of referenced blocks */
    gc_mark_region(self, (char *)&jmp, (char *)&jmp + sizeof jmp);
    gc_mark_region(self, STACKSTART, STACKEND);

    for (root = gc_root; root; root = root->next)
	gc_mark_region(self, root->start, root->end);

    numpgs = ((char *)g_heaphigh - (char *)g_heaplow) / PAGESIZE;
    page = (struct gcms_pageinfo *)g_heaplow;

    /* sweep and release unused memory */
    for (i = 0; i < numpgs; (i++),
	    page = (struct gcms_pageinfo *)((char *)page + PAGESIZE))
    {
	if (!gc_is_gc_page(page) || page->g.interface != self)
	    continue;

	if (page->g.p.blkclass == MULTIPAGE)
	{
	    if (page->refbits[0])
		page->refbits[0] = 0;
	    else
	    {
		g_totfree += PAGESIZE * page->g.p.count - GC_PAGE_OVERHEAD;

		if (page->g.type->destroy)
		    page->g.type->destroy((char *)page + GC_PAGE_OVERHEAD);

		gc_base_free(self, (char *)page + GC_PAGE_OVERHEAD);
	    }
	}
	else		    /* small blocks */
	{
	    int j;
	    int freed = 0;
	    int numblocks = g_blockcounts[page->g.p.blkclass];
	    int blocksize = g_blocksizes[page->g.p.blkclass];
	    struct Freeblock *e;

	    /* mark all the free blocks on this page so we do not try to
	       free them */
	    for (e = page->g.p.block; e != NULL; e = e->link)
	    {
		int num = ((char *)e - (char *)page - GC_PAGE_OVERHEAD) /
		    blocksize;
		SETBIT(page->refbits, num);
		freed++;
	    }

	    for (j = 0; j < numblocks; j++)
	    {
		if (!GETBIT(page->refbits, j))
		{
		    e = (struct Freeblock *)
			((char *)page + GC_PAGE_OVERHEAD + blocksize * j);

		    if (page->g.type->destroy)
			page->g.type->destroy(e);

		    gc_base_free(self, e);
		    freed++;
		}
	    }

	    /* if we freed everything on this page, the page has also been
	       freed so everything it pointed to is now gone */
	    if (freed != numblocks)
		memset(page->refbits, 0,
			sizeof (uLong)*(numblocks / BITSPERLONG + 1));

	    g_totfree += freed * blocksize;
	}
    }

    g_ingc = FALSE;
}

struct gc_interface gc_codegen_ms_interface =
{
    "CodeGen mark-and-sweep collector",
    gc_ms_init_page,
    gc_ms_destroy_page,
    gc_ms_addroot,
    gc_ms_delroot,
    gc_ms_need_collect,
    gc_ms_collect,
    NULL,    /* no init_block */
    NULL,    /* no destroy_block */
    NULL,    /* private - must be NULL */
};
