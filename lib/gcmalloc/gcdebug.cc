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
// $Header: /u/cgt/cvs/lib/gcmalloc/gcdebug.cc,v 1.8 1999/09/29 04:47:46 parag Exp $

#include <stddef.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdlibx.h>
#include "gcmalloc.h"
#include "internals.h"

void
gcdebug(FILE *fp)
{
    if (fp == NULL)	// so we can call this easily from a debugger
	fp = stderr;

    gcdump(fp);

    int i;
    fprintf(fp, "gcdebug()...\n");
    fprintf(fp, "  gc.pages=%d  gc.pagecount=%d\n", gc.pages, gc.pagecount);
    fprintf(fp, "  regions=%p maxregions=%d numregions=%d...\n",
	    gc.regions, gc.maxregions, gc.numregions);

    for (i = 0; i < gc.numregions; i++)
    	if (gc.regions[i].start != NULL)
    	    fprintf(fp, "    regions[%d] start=%p end=%p\n",
		    i, gc.regions[i].start, gc.regions[i].end);

    fprintf(fp, "  freepage...\n");
    struct pageinfo *freepage = gc.freepagelist;

    for (i = 1; freepage != NULL; i++, freepage = freepage->link)
	fprintf(fp, "    freepage[%d] = %p, count=%d, history=%#X\n", i,
		freepage->page, freepage->count, freepage->history & HISTORY);

    fprintf(fp, "  hitpage...\n");
    struct pageinfo *hitpage = gc.hitpagelist;

    for (i = 1; hitpage != NULL; i++, hitpage = hitpage->link)
	fprintf(fp, "    hitpage[%d] = %p, history=%#X\n", i,
		hitpage->page, hitpage->history & HISTORY);


    fprintf(fp, "  simplefreelist = %p...\n", gc.simplefreelist);
    struct freeelem *elem = gc.simplefreelist;

    for (i = 1; elem != NULL; i++, elem = elem->link)
	fprintf(fp, "    simplefreelist[%d] = %p, count=%d\n", i, elem,
		elem->count);

    fprintf(fp, "  allocated memory...\n");
    for (int g = 0; g < MAXPAGEGROUPS; g++)
    	if (gc.pagegroups[g] != NULL)
	{
	    struct pagegroupinfo *pgi = gc.pagegroups[g];

	    for (unsigned int p = 0; p < PAGESPERGROUP; p++)
	    {
	    	struct pageinfo *pi = &pgi->pages[p];

		if (pi->blocksize <= 0 && pi->blocksize != -1)
		    continue;

		void *base = (void *)((char *)PTRBASEADDR +
			(g * PAGESPERGROUP + p) * BYTESPERPAGE);

		fprintf(fp, "    base=%p g=%d p=%d blocksize=%d"
			" numblocks=%d link=%p history=%#X\n",
			base, g, p, pi->blocksize, pi->numblocks, pi->link,
			pi->history & HISTORY);
		fprintf(fp, "      type=%p atomic=%s\n",
			pi->type, pi->atomic ? "TRUE" : "FALSE");
		fprintf(fp, "      freemap=%p refmap=%p\n",
			pi->freemap, pi->refmap); 
		if (pi->blocksize > 0 && pi->numblocks > BITSPERLONG)
		{
		    int words = (pi->numblocks + BITSPERLONG - 1) / BITSPERLONG;

		    for (int w = 0; w < words; w++)
			fprintf(fp, "        %2d) freemap=%#lX refmap=%#lX\n",
				w, pi->freemap[w], pi->refmap[w]);
		}
	    }
	}

    if (gc.snapshot)
	gcdumpsnapstack(fp);

    fflush(fp);
}

void
gcdump(FILE *fp)
{
    if (fp == NULL)	// so we can call this easily from a debugger
	fp = stderr;

    int alloccount[PAGETHRESHHOLD + 1];
    int freecount[PAGETHRESHHOLD + 1];
    int largeblocks[PAGETHRESHHOLD + 1];
    int freepagecnt = 0;
    int hitpagecnt = 0;
    int simplepagecnt = 0;
    int headerpagecnt = 0;
    int datapagecnt = 0;
    int boguspagecnt = 0;
    int i;

    for (i = 0; i <= PAGETHRESHHOLD; i++)
    {
    	alloccount[i] = 0;
    	freecount[i] = 0;
	largeblocks[i] = 0;
    }

    for (int g = 0; g < MAXPAGEGROUPS; g++)
    	if (gc.pagegroups[g] != NULL)
	{
	    struct pagegroupinfo *pgi = gc.pagegroups[g];

	    for (unsigned int p = 0; p < PAGESPERGROUP; p++)
	    {
	    	struct pageinfo *pi = &pgi->pages[p];
		int sz = pi->blocksize;
		int nb = pi->numblocks;

		if (sz > 0)
		{
		    int words = (nb + BITSPERLONG - 1) / BITSPERLONG;
		    int freecnt = 0;

		    if (words == 1)
		    	freecnt = bitcount(pi->freebitmap);
		    else
		    	for (int j = 0; j < words; j++)
			    freecnt += bitcount(pi->freemap[j]);

		    freecount[sz] += freecnt;
		    alloccount[sz] += nb - freecnt;
		}
		else if (sz == -1)
		{
		    sz = pi->numblocks;

		    if (sz >= PAGETHRESHHOLD)
		    	sz = PAGETHRESHHOLD;

		    largeblocks[sz]++;
		}
		else if (sz == 0)
		{
		    switch (nb)
		    {
		    case FREE_PAGE:
			freepagecnt++;
		    	break;
		    case HIT_PAGE:
			hitpagecnt++;
		    	break;
		    case SIMPLE_PAGE:
		    	simplepagecnt++;
		    	break;
		    case HEADER_PAGE:
		    	headerpagecnt++;
		    	break;
		    case DATA_PAGE:
		    	datapagecnt++;
		    	break;
		    case UNUSED_PAGE:
		    	break;
		    default:
		    	boguspagecnt++;
		    	break;
		    }
		}
	    }
	}

    for (i = 0; i <= PAGETHRESHHOLD; i++)
    	if (alloccount[i] || freecount[i])
	    fprintf(fp, "%d: %d/%d\n", i, alloccount[i],
		    alloccount[i] + freecount[i]);

    for (i = 0; i <= PAGETHRESHHOLD; i++)
    	if (largeblocks[i])
	    if (i == PAGETHRESHHOLD)
		fprintf(fp, ">%d: %d\n", i * BYTESPERPAGE, largeblocks[i]);
	    else
		fprintf(fp, "%d: %d\n", i * BYTESPERPAGE, largeblocks[i]);

    fprintf(fp, "%d/%d/%d/%d/%d/%d\n", freepagecnt * BYTESPERPAGE,
	    hitpagecnt * BYTESPERPAGE, simplepagecnt * BYTESPERPAGE,
	    headerpagecnt * BYTESPERPAGE, datapagecnt * BYTESPERPAGE,
	    boguspagecnt * BYTESPERPAGE);

    fflush(fp);
}
