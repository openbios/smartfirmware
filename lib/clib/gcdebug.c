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
#include "stdlibx.h"
#include "gcmalloc.h"
#include "gcdefs.h"

void
gcdebug(FILE *fp)
{
	int i, g;
	struct pageinfo *freepage, *hitpage;
	struct freeelem *elem;

	if (fp == NULL)				/* so we can call this easily from a
								   debugger */
		fp = stderr;

	fprintf(fp, "gcdebug()...\n");
	fprintf(fp, "  gc.pages=%d  gc.pagecount=%d\n", gc.pages, gc.pagecount);
	fprintf(fp, "  regions=0x%X maxregions=%d numregions=%d...\n",
			gc.regions, gc.maxregions, gc.numregions);

	for (i = 0; i < gc.numregions; i++)
		if (gc.regions[i].start != NULL)
			fprintf(fp, "    regions[%d] start=0x%X end=0x%X\n",
					i, gc.regions[i].start, gc.regions[i].end);

	fprintf(fp, "  freepage...\n");
	freepage = gc.freepagelist;

	for (i = 1; freepage != NULL; i++, freepage = freepage->link)
		fprintf(fp, "    freepage[%d] = 0x%X, count=%d, history=0x%X\n", i,
				freepage->page, freepage->uf.count,
				freepage->history & HISTORY);

	fprintf(fp, "  hitpage...\n");
	hitpage = gc.hitpagelist;

	for (i = 1; hitpage != NULL; i++, hitpage = hitpage->link)
		fprintf(fp, "    hitpage[%d] = 0x%X, history=0x%X\n", i,
				hitpage->page, hitpage->history & HISTORY);


	fprintf(fp, "  simplefreelist = 0x%X...\n", gc.simplefreelist);
	elem = gc.simplefreelist;

	for (i = 1; elem != NULL; i++, elem = elem->link)
		fprintf(fp, "    simplefreelist[%d] = 0x%X, count=%d\n", i, elem,
				elem->count);

	fprintf(fp, "  allocated memory...\n");
	for (g = 0; g < MAXPAGEGROUPS; g++)
		if (gc.pagegroups[g] != NULL)
		{
			int p;
			struct pagegroupinfo *pgi = gc.pagegroups[g];

			for (p = 0; p < PAGESPERGROUP; p++)
			{
				void *base;
				struct pageinfo *pi = &pgi->pages[p];

				if (pi->blocksize <= 0 && pi->blocksize != -1)
					continue;

				base = (void *)((char *)PTRBASEADDR +
						(g * PAGESPERGROUP + p) * BYTESPERPAGE);

				fprintf(fp, "    base=0x%X g=%d p=%d blocksize=%d"
						" numblocks=%d link=0x%X history=0x%X\n",
						base, g, p, pi->blocksize, pi->numblocks, pi->link,
						pi->history & HISTORY);
				fprintf(fp, "      type=0x%X atomic=%s\n",
						pi->u.type, pi->atomic ? "TRUE" : "FALSE");
				fprintf(fp, "      freemap=0x%X refmap=0x%X\n",
						pi->uf.freemap, pi->ur.refmap);

				if (pi->blocksize > 0 && pi->numblocks > BITSPERLONG)
				{
					int w;
					int words = (pi->numblocks + BITSPERLONG - 1) / BITSPERLONG;

					for (w = 0; w < words; w++)
						fprintf(fp, "        %2d) freemap=0x%X refmap=0x%X\n",
								w, pi->uf.freemap[w], pi->ur.refmap[w]);
				}
			}
		}

	if (gc.snapshot)
		gcdumpsnapstack(fp);

	fflush(fp);
}
