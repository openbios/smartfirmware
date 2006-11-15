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
/* $Header: /u/cgt/cvs/lib/aoa/gcbase.c,v 1.6 1999/09/29 04:10:00 parag Exp $ */

#include <stddef.h>
#include <stdlib.h>
#include <stdio.h>
#include <memory.h>
#include <string.h>
#include <stdarg.h>
#include <setjmp.h>
#include <unistd.h>
#include "aoa.h"
#include "gcbase.h"


#if PAGESIZE != 4096 || ALIGNMENT != 16 || BASESIZE != 4 || \
    GC_PAGE_OVERHEAD != 32
#  error GC tables need to be rebuilt
#endif

#define NUMCLASSES 31
#define THRESHHOLD 2032

static const int g_blocksizes[NUMCLASSES] =
{
    4, 8, 16, 32, 48, 64, 80, 96, 112, 128, 144, 160, 176, 192, 208, 224, 240,
    256, 288, 304, 336, 368, 400, 448, 496, 576, 672, 800, 1008, 1344,
    2032,
};

static const int g_blockcount[NUMCLASSES] =
{
    1016, 508, 254, 127, 84, 63, 50, 42, 36, 31, 28, 25, 23, 21, 19, 18, 16,
    15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2,
};

static const unsigned char g_sizemap[THRESHHOLD + 1] =
{
    0, 0, 0, 0, 0, 1, 1, 1, 1, 2, 2, 2, 2, 2, 2, 2, 2, 3, 3, 3, 3, 3, 3, 3,
    3, 3, 3, 3, 3, 3, 3, 3, 3, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4,
    4, 4, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 6, 6, 6, 6, 6,
    6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7,
    7, 7, 7, 7, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 9, 9, 9,
    9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 10, 10, 10, 10, 10, 10, 10, 10,
    10, 10, 10, 10, 10, 10, 10, 10, 11, 11, 11, 11, 11, 11, 11, 11, 11,
    11, 11, 11, 11, 11, 11, 11, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12,
    12, 12, 12, 12, 12, 12, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13,
    13, 13, 13, 13, 13, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14,
    14, 14, 14, 14, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15,
    15, 15, 15, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16,
    16, 16, 17, 17, 17, 17, 17, 17, 17, 17, 17, 17, 17, 17, 17, 17, 17,
    17, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18,
    18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 19,
    19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 20, 20,
    20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20,
    20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 21, 21, 21, 21,
    21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21,
    21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 22, 22, 22, 22, 22, 22,
    22, 22, 22, 22, 22, 22, 22, 22, 22, 22, 22, 22, 22, 22, 22, 22, 22,
    22, 22, 22, 22, 22, 22, 22, 22, 22, 23, 23, 23, 23, 23, 23, 23, 23,
    23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23,
    23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23,
    23, 23, 23, 23, 23, 23, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24,
    24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24,
    24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24,
    24, 24, 24, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25,
    25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25,
    25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25,
    25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25,
    25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 26, 26,
    26, 26, 26, 26, 26, 26, 26, 26, 26, 26, 26, 26, 26, 26, 26, 26, 26,
    26, 26, 26, 26, 26, 26, 26, 26, 26, 26, 26, 26, 26, 26, 26, 26, 26,
    26, 26, 26, 26, 26, 26, 26, 26, 26, 26, 26, 26, 26, 26, 26, 26, 26,
    26, 26, 26, 26, 26, 26, 26, 26, 26, 26, 26, 26, 26, 26, 26, 26, 26,
    26, 26, 26, 26, 26, 26, 26, 26, 26, 26, 26, 26, 26, 26, 26, 26, 26,
    26, 26, 26, 26, 26, 26, 26, 26, 26, 27, 27, 27, 27, 27, 27, 27, 27,
    27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27,
    27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27,
    27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27,
    27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27,
    27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27,
    27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27,
    27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27,
    27, 28, 28, 28, 28, 28, 28, 28, 28, 28, 28, 28, 28, 28, 28, 28, 28,
    28, 28, 28, 28, 28, 28, 28, 28, 28, 28, 28, 28, 28, 28, 28, 28, 28,
    28, 28, 28, 28, 28, 28, 28, 28, 28, 28, 28, 28, 28, 28, 28, 28, 28,
    28, 28, 28, 28, 28, 28, 28, 28, 28, 28, 28, 28, 28, 28, 28, 28, 28,
    28, 28, 28, 28, 28, 28, 28, 28, 28, 28, 28, 28, 28, 28, 28, 28, 28,
    28, 28, 28, 28, 28, 28, 28, 28, 28, 28, 28, 28, 28, 28, 28, 28, 28,
    28, 28, 28, 28, 28, 28, 28, 28, 28, 28, 28, 28, 28, 28, 28, 28, 28,
    28, 28, 28, 28, 28, 28, 28, 28, 28, 28, 28, 28, 28, 28, 28, 28, 28,
    28, 28, 28, 28, 28, 28, 28, 28, 28, 28, 28, 28, 28, 28, 28, 28, 28,
    28, 28, 28, 28, 28, 28, 28, 28, 28, 28, 28, 28, 28, 28, 28, 28, 28,
    28, 28, 28, 28, 28, 28, 28, 28, 28, 28, 28, 28, 28, 28, 28, 28, 28,
    28, 28, 28, 28, 28, 28, 28, 28, 28, 28, 28, 28, 28, 28, 28, 28, 28,
    28, 28, 28, 28, 28, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29,
    29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29,
    29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29,
    29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29,
    29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29,
    29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29,
    29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29,
    29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29,
    29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29,
    29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29,
    29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29,
    29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29,
    29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29,
    29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29,
    29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29,
    29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29,
    29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29,
    29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29,
    29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29,
    29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29,
    29, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30,
    30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30,
    30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30,
    30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30,
    30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30,
    30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30,
    30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30,
    30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30,
    30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30,
    30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30,
    30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30,
    30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30,
    30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30,
    30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30,
    30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30,
    30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30,
    30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30,
    30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30,
    30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30,
    30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30,
    30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30,
    30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30,
    30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30,
    30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30,
    30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30,
    30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30,
    30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30,
    30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30,
    30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30,
    30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30,
    30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30,
    30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30,
    30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30,
    30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30,
    30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30,
    30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30,
    30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30,
    30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30,
    30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30,
    30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30,
    30, 30, 30, 30, 30, 30, 30, 30, 30,
};

/* PAGESIZE == 4096 && ALIGNMENT == 16 && BASESIZE == 4 && PAGEOVERHEAD == 32 */


static struct gc_pageinfo *g_pageheaders[NUMCLASSES];

#ifdef MTSAFE
static struct Lockinfo g_gc_locks[NUMCLASSES];
static struct Lockinfo g_gc_markpagelock;
#endif /* MTSAFE*/

/* global list of bits to mark GC vs non-GC pages (default non-GC) */
static uLong *gc_pages = NULL;
static int gc_max_pages = 0;    /* number of pages in above list */

/* linked list of gc interfaces available in random order */
static struct gc_interface *g_interfaces = NULL;

static void *g_heaplow = NULL;

boolean
gc_is_gc_page(void *p)
{
    ptrdiff_t page;

    if (g_heaplow == NULL)
	g_heaplow = aoa_get_heaplow();

    page = ((char *)p - (char *)g_heaplow) / PAGESIZE;

    if (page >= 0 && page < gc_max_pages)
	if (GETBIT(gc_pages, page))
	    return TRUE;

    return FALSE;
}

const int *
gc_base_blocksizes()
{
    return g_blocksizes;
}

const int *
gc_base_blockcounts()
{
    return g_blockcount;
}

static void
mark_gc_page(void *p, boolean mark)
{
    ptrdiff_t page;

    if (g_heaplow == NULL)
	g_heaplow = aoa_get_heaplow();

    page = ((char *)p - (char *)g_heaplow) / PAGESIZE;
#ifdef MTSAFE
    ACQUIRE_LOCK(g_gc_markpagelock);
#endif

    if (page >= gc_max_pages)
    {
	int newmax = page + BITSPERLONG * 2;
	uLong *newmap = calloc(sizeof (uLong), newmax / BITSPERLONG + 1);

	if (newmap == NULL)
	    gc_abort("mark_gc_page(): cannot allocate space to grow gc pages");

	if (gc_pages)
	{
	    memcpy(newmap, gc_pages,
		    (gc_max_pages / BITSPERLONG + 1) * sizeof (uLong));
	    free(gc_pages);
	}

	gc_pages = newmap;
	gc_max_pages = newmax;
    }

    if (mark)
	SETBIT(gc_pages, page);
    else
	CLEARBIT(gc_pages, page);

#ifdef MTSAFE
    RELEASE_LOCK(g_gc_markpagelock);
#endif
}

void *
gc_base_alloc(struct gc_interface *gc, size_t size, void *arg)
{
    int i;
    struct gc_pageinfo *pi;
    void *base;
    struct Freeblock *f;

    if (gc == NULL)
	return NULL;

    if (gc->next == NULL && g_interfaces != NULL)
    {
	gc->next = g_interfaces;
	g_interfaces = gc;
    }

    if (size <= THRESHHOLD)
    {
	int blkclass = g_sizemap[size];
	struct gc_pageinfo *ppi = NULL;
#ifdef MTSAFE
	ACQUIRE_LOCK(g_gc_locks[blkclass]);
#endif

	/* look for a block of the right size and interface */
	for (pi = g_pageheaders[blkclass]; pi != NULL;
		(ppi = pi), pi = (struct gc_pageinfo *)pi->p.link)
	if (pi->interface == gc)
	    break;

	if (g_heaplow == NULL)
	    g_heaplow = aoa_get_heaplow();

	/* no such page - try scavenging some space */
	if (pi == NULL && g_heaplow && gc->need_collect(gc))
	{
	#ifdef MTSAFE
	    RELEASE_LOCK(g_gc_locks[blkclass]);
	    ACQUIRE_LOCK(gc->lock);
	#endif

	    gc->collect(gc);

	#ifdef MTSAFE
	    RELEASE_LOCK(gc->lock);
	    ACQUIRE_LOCK(g_gc_locks[blkclass]);
	#endif

	    ppi = NULL;

	    for (pi = g_pageheaders[blkclass]; pi != NULL;
		    (ppi = pi), pi = (struct gc_pageinfo *)pi->p.link)
	    if (pi->interface == gc)
		break;
	}

	/* no such page - allocate one and initialize it */
	if (pi == NULL)
	{
	    base = aoa_allocpages(1);

	    if (base == NULL)
		return NULL;

	    mark_gc_page(base, TRUE);
	    pi = (struct gc_pageinfo *)base;
	    pi->p.blkclass = blkclass;
	    pi->p.count = g_blockcount[blkclass];
	    pi->p.link = (struct Pageinfo *)g_pageheaders[blkclass];
	    g_pageheaders[blkclass] = pi;
	    ppi = NULL;

	    /* make a list of all free blocks on the page */
	    f = (struct Freeblock *)((char *)base + GC_PAGE_OVERHEAD);
	    pi->p.block = NULL;

	    for (i = 0; i < pi->p.count; i++)
	    {
		f->link = pi->p.block;
		pi->p.block = f;
		f = (struct Freeblock *)((char *)f + g_blocksizes[blkclass]);
	    }

	    pi->interface = gc;

	    if (gc->init_page)
		gc->init_page(gc, pi, arg);
	}

	f = pi->p.block;
	pi->p.block = f->link;
	pi->p.count--;

	if (pi->p.count == 0)    /* unlink page from free list */
	{
	    if (ppi)
		ppi->p.link = pi->p.link;
	    else
		g_pageheaders[blkclass] = (struct gc_pageinfo *)pi->p.link;

	    pi->p.link = NULL;
	}

#ifdef MTSAFE
	RELEASE_LOCK(g_gc_locks[blkclass]);
#endif

	if (gc->init_block)
	    gc->init_block(gc, pi, f);

	return (void *)f;
    }
    else			/* really big block */
    {
	int pgs = (size + PAGEOVERHEAD + PAGESIZE - 1) / PAGESIZE;
	base = aoa_allocpages(pgs);

	if (base == NULL)
	    return NULL;

	mark_gc_page(base, TRUE);
	pi = (struct gc_pageinfo *)base;
	pi->p.blkclass = MULTIPAGE;
	pi->p.count = pgs;
	pi->p.link = NULL;
	pi->interface = gc;

	if (gc->init_page)
	    gc->init_page(gc, pi, arg);

	if (gc->init_block)
	    gc->init_block(gc, pi, (char *)base + PAGEOVERHEAD);

	return (char *)base + PAGEOVERHEAD;
    }
}

void *
gc_base_realloc(struct gc_interface *gc, void *obj, size_t nsize, void *arg)
{
    /* not implemented yet */
    return NULL;
}

void
gc_base_free(struct gc_interface *gc, void *obj)
{
    struct gc_pageinfo *pi;
    struct Freeblock *f;
#ifdef MTSAFE
    int blkclass;
#endif

    if (obj == NULL)
	return;

    pi = (struct gc_pageinfo *)GETPAGEINFO(obj);

    if (!gc_is_gc_page(pi))
	gc_abort("gc_base_free(): attempt to free non-GC pointer");

    if (pi->interface != gc)
	gc_abort("gc_base_free(): attempt to free `%s' GC pointer from `%s'",
		pi->interface->name, gc->name);

    if (gc->destroy_block)
	gc->destroy_block(gc, pi, obj);

    /* really big block */
    if (pi->p.blkclass == MULTIPAGE)
    {
	if (gc->destroy_page)
	    gc->destroy_page(gc, pi);

	pi->interface = NULL;
	mark_gc_page(pi, FALSE);

    #ifdef MTSAFE
	aoa_freepages(pi, pi->p.count, FALSE);
    #else
	aoa_freepages(pi, pi->p.count);
    #endif
	return;
    }

    /* otherwise this is a bunch of small blocks */
#ifdef MTSAFE
    blkclass = pi->p.blkclass;
    ACQUIRE_LOCK(g_gc_locks[blkclass]);
#endif

    /* link the page into the global free list if it is not already there */
    if (pi->p.count == 0)
    {
	pi->p.link = (struct Pageinfo *)g_pageheaders[pi->p.blkclass];
	g_pageheaders[pi->p.blkclass] = pi;
    }

    /* link this object into the free list of blocks for this page */
    pi->p.count++;
    f = (struct Freeblock *)obj;
    f->link = pi->p.block;
    pi->p.block = f;

    /* free the page if all the blocks on this page are free */
    if (pi->p.count == g_blockcount[pi->p.blkclass])
    {
	struct gc_pageinfo *ppi = NULL;
	struct gc_pageinfo *p;

	/* unlink it from the global free block list */
	for (p = g_pageheaders[pi->p.blkclass]; p != NULL && p != pi;
		(ppi = p), p = (struct gc_pageinfo *)p->p.link)
	    ;

	if (ppi == NULL)
	    g_pageheaders[pi->p.blkclass] = (struct gc_pageinfo *)pi->p.link;
	else
	    ppi->p.link = pi->p.link;

	if (gc->destroy_page)
	    gc->destroy_page(gc, pi);

	pi->interface = NULL;
	mark_gc_page(pi, FALSE);

    #ifdef MTSAFE
	aoa_freepages(pi, 1, FALSE);
    #else
	aoa_freepages(pi, 1);
    #endif
    }

#ifdef MTSAFE
    RELEASE_LOCK(g_gc_locks[blkclass]);
#endif
}

void
gc_abort(char const *fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    vfprintf(stderr, fmt, ap);
    va_end(ap);
    fprintf(stderr, "\r\n");
    abort();
    exit(-1);
}

void
gc_dumpheap()
{
    ptrdiff_t page;
    struct gc_pageinfo *pi;
    int i;

    fflush(stdout);
    fprintf(stderr, "\nDumping heap...\n");

    for (i = 0; i < NUMCLASSES; i++)
	if (g_pageheaders[i])
	{
	    fprintf(stderr,
		    "block class %d, size=%d, count=%d, pages=%p\n",
		    i, g_blocksizes[i], g_blockcount[i], g_pageheaders[i]);

	    for (pi = g_pageheaders[i]; pi != NULL;
		    pi = (struct gc_pageinfo *)pi->p.link)
		fprintf(stderr, "\tlink=%p\n", pi->p.link);
	}

    if (g_heaplow == NULL)
	g_heaplow = aoa_get_heaplow();

    for (page = 0; page < gc_max_pages; page++)
	if (GETBIT(gc_pages, page))
	{
	    struct Freeblock *f;

	    pi = (struct gc_pageinfo *)((char *)g_heaplow + page * PAGESIZE);
	    fprintf(stderr, "page %d: pageaddr=%p interface=`%s'\n",
		    page, page * PAGESIZE + (char *)g_heaplow,
		    pi->interface ? pi->interface->name : "<none>");
	    fprintf(stderr,
		    "\tblock class %d, size=%d count=%d free=0%d\n",
		    pi->p.blkclass, g_blocksizes[pi->p.blkclass],
		    g_blockcount[pi->p.blkclass], pi->p.count);

	    for (f = pi->p.block; f != NULL; f = f->link)
		fprintf(stderr, "\t\tfree=%p\n", f);
	}

    fprintf(stderr, "end\n\n");
}
