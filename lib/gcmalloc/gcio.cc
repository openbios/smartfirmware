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


#include <stdlib.h>
#include <stdlibx.h>
#include <stdio.h>
#include <hashtable.h>
#include <srchsort.h>
#include <unistd.h>

#include "gcmalloc.h"
#include "internals.h"

#define MEM_MAGIC 0xC7434DC4
#define GCBUFSIZ 8192

struct header
{
    uInt magic;
    uInt pagesize;
    uInt pagebaseaddr;
    uInt stringchars;
    uInt stringoffset;
    uInt snapcount;
    uInt snapsize;
    uInt snapoffset;
    uInt bitmapwords;
    uInt bitmapwordsize;
    uInt bitmapoffset;
    uInt hdrpages;
    uInt hdrsize;
    uInt hdrpageptrsoffset;
};

struct pagehdr
{
    uInt blocksize;
    uInt numblocks;
    uInt snapoffset;
    uInt atomic;
    union
    {
	uLong freemap;
	uInt freemapoffset;
    };
};

struct ptrmap
{
    void *ptr;
    long offset;
};

declare_searchsort(ptrmap)
implement_searchsort(ptrmap)

int
ptrmapptrcmp(struct ptrmap const *p1, struct ptrmap const *p2)
{
    return (int)((char *)p1->ptr - (char *)p2->ptr);
}

int
ptrmapoffsetcmp(struct ptrmap const *p1, struct ptrmap const *p2)
{
    return (int)(p1->offset - p2->offset);
}

inline long
getoffset(struct ptrmap *table, int size, void const *ptr)
{
    struct ptrmap key;
    key.ptr = (void *)ptr;
    int index = bsearch(key, table, size, ptrmapptrcmp);

    if (index < 0)
    	return 0;

    return table[index].offset;
}

inline void *
getpointer(struct ptrmap *table, int size, long offset)
{
    struct ptrmap key;
    key.offset = offset;
    int index = bsearch(key, table, size, ptrmapoffsetcmp);

    if (index < 0)
    	return 0;

    return table[index].ptr;
}

#define FILEALIGN 16

boolean
fpad(FILE *fp, long &offset)
{
    if ((offset % FILEALIGN) == 0)
    	return TRUE;

    size_t fill = FILEALIGN - (offset % FILEALIGN);
    char buf[FILEALIGN];
    memset(buf, '\0', FILEALIGN);

    if (fwrite(buf, sizeof (char), fill, fp) != fill)
    	return FALSE;

    offset += fill;
    return TRUE;
}

boolean
gcwrite(char const *filename)
{
    gc.inalloc = TRUE;

    if (filename == NULL)
	filename = "mem.out";

    FILE *fp = fopen(filename, "wb+");

    if (fp == NULL)
    {
    	gc.inalloc = FALSE;
    	return FALSE;
    }

    char buf[GCBUFSIZ];
    setvbuf(fp, buf, _IOFBF, GCBUFSIZ);		// keep stdio from mallocing mem

    long offset = 0;
    struct header hdr;
    hdr.magic = MEM_MAGIC;
    hdr.pagesize = BYTESPERPAGE;
    hdr.pagebaseaddr = (uInt)PTRBASEADDR;
    hdr.stringchars = 0;
    hdr.stringoffset = 0;
    hdr.snapcount = 0;
    hdr.snapsize = sizeof (Stack_elt);
    hdr.snapoffset = 0;
    hdr.bitmapwords = 0;
    hdr.bitmapwordsize = sizeof (uLong);
    hdr.bitmapoffset = 0;
    hdr.hdrpages = MAXPAGEGROUPS;
    hdr.hdrsize = sizeof (struct pageinfo);
    hdr.hdrpageptrsoffset = 0;
    fprintf(stderr, "gcwrite: about to write initial header\n");

    if (fwrite(&hdr, sizeof hdr, 1, fp) != 1)
    {
    	fclose(fp);
    	gc.inalloc = FALSE;
	return FALSE;
    }

    offset += sizeof hdr;

    // write out filename strings
    int stringcount = 0;

    if (gc.snapshot && gcstackpool != NULL)
    	for (Stack_iter si = *gcstackpool; si; si++)
	    if (si().file != NULL)
		stringcount++;

    struct ptrmap stringtable[stringcount];

    if (gc.snapshot && gcstackpool != NULL)
    {
    	if (!fpad(fp, offset))
	{
	    fclose(fp);
	    gc.inalloc = FALSE;
	    return FALSE;
	}

	hdr.stringoffset = offset;
	int strnum = 0;
	fprintf(stderr, "gcwrite: about to write filename strings\n");

	// store the file names
    	for (Stack_iter si = *gcstackpool; si; si++)
	{
	    Stack_elt &se = si();

	    if (se.file != NULL)
	    {
		if (strnum < stringcount)
		{
		    stringtable[strnum].ptr = (void *)se.file;
		    stringtable[strnum].offset = offset;
		}

		size_t len = strlen(se.file) + 1;

		if (fwrite(se.file, sizeof (char), len, fp) != len)
		{
		    fclose(fp);
		    gc.inalloc = FALSE;
		    return FALSE;
		}

		offset += len;
		hdr.stringchars += len;
		strnum++;
	    }
	}

	if (strnum != stringcount)
	    gcabort("gcwrite: string count calculation errors");
    }

    qsort(stringtable, stringcount, ptrmapptrcmp);

    // write snapshot info
    if (gc.snapshot && gcstackpool != NULL)
    {
    	if (!fpad(fp, offset))
	{
	    fclose(fp);
	    gc.inalloc = FALSE;
	    return FALSE;
	}

	hdr.snapoffset = offset;
	hdr.snapcount = gcstackpool->size();
    }

    struct ptrmap snaptable[hdr.snapcount];

    if (gc.snapshot && gcstackpool != NULL)
    {
	fprintf(stderr, "gcwrite: about to write stack snapshots\n");
	Stack_iter si = *gcstackpool;
	unsigned int i = 0;

	for (; si; si++, i++)
	{
	    Stack_elt *p = &si();

	    if (i < hdr.snapcount)
	    {
		snaptable[i].ptr = p;
		snaptable[i].offset = offset;
	    }

	    char const *filename = p->file;
	    long fileoffset = getoffset(stringtable, stringcount, filename);
	    p->file = (char const *)fileoffset;

	    if (fwrite(p, sizeof *p, 1, fp) != 1)
	    {
		p->file = filename;
	    	fclose(fp);
		gc.inalloc = FALSE;
		return FALSE;
	    }

	    p->file = filename;
	    offset += sizeof *p;
	}

	if (i != hdr.snapcount)
	    gcabort("gcwrite: iterator/table size mismatch");

	qsort(snaptable, i, ptrmapptrcmp);
    }

    // count number of large bit maps
    int bitmapcount = 0;
    int g;

    for (g = 0; g < MAXPAGEGROUPS; g++)
    	if (gc.pagegroups[g] != NULL)
	{
	    struct pagegroupinfo &pgi = *gc.pagegroups[g];

	    for (unsigned int p = 0; p < PAGESPERGROUP; p++)
	    {
		struct pageinfo &pi = pgi.pages[p];

		if (pi.blocksize > 0 &&
			pi.blocksize < (BYTESPERPAGE / BITSPERLONG))
		    bitmapcount++;
	    }
	}

    // write out large bit maps
    struct ptrmap bitmaptable[bitmapcount];
    int n = 0;
    fprintf(stderr, "gcwrite: about to write bitmaps\n");

    if (bitmapcount > 0)
    {
    	if (!fpad(fp, offset))
	{
	    fclose(fp);
	    gc.inalloc = FALSE;
	    return FALSE;
	}

	hdr.bitmapwords = 0;
	hdr.bitmapoffset = offset;

	for (g = 0; g < MAXPAGEGROUPS; g++)
	    if (gc.pagegroups[g] != NULL)
	    {
		struct pagegroupinfo &pgi = *gc.pagegroups[g];

		for (unsigned int p = 0; p < PAGESPERGROUP; p++)
		{
		    struct pageinfo &pi = pgi.pages[p];
		    int sz = pi.blocksize;

		    if (sz > 0 && sz < (BYTESPERPAGE / BITSPERLONG))
		    {
			int bpp = BYTESPERPAGE / sz;
			unsigned int words = (bpp + BITSPERLONG - 1) / BITSPERLONG;

			if (n < bitmapcount)
			{
			    bitmaptable[n].ptr = pi.freemap;
			    bitmaptable[n].offset = offset;
			}

			if (fwrite(pi.freemap, sizeof (uLong), words, fp) !=
				words)
			{
			    fclose(fp);
			    gc.inalloc = FALSE;
			    return FALSE;
			}

			offset += sizeof (uLong) * words;
			hdr.bitmapwords += words;
			n++;
		    }
		}
	    }

	if (n != bitmapcount)
	    gcabort("gcwrite: bitmap count calculation errors");

	qsort(bitmaptable, n, ptrmapptrcmp);
    }

    // write out header index
    if (!fpad(fp, offset))
    {
	fclose(fp);
	gc.inalloc = FALSE;
	return FALSE;
    }

    hdr.hdrpageptrsoffset = offset;
    uInt pageoffsets[MAXPAGEGROUPS];
    offset += sizeof (uInt) * MAXPAGEGROUPS;

    for (g = 0; g < MAXPAGEGROUPS; g++)
    	if (gc.pagegroups[g] != NULL)
	{
	    pageoffsets[g] = offset;
	    offset += sizeof (pagehdr) * PAGESPERGROUP;
	}
	else
	    pageoffsets[g] = 0;

    fprintf(stderr, "gcwrite: about to write page indexes and headers\n");

    if (fwrite(pageoffsets, sizeof (uInt), MAXPAGEGROUPS, fp) != MAXPAGEGROUPS)
    {
    	fclose(fp);
	gc.inalloc = FALSE;
	return FALSE;
    }

    // write out page headers
    for (g = 0; g < MAXPAGEGROUPS; g++)
    	if (gc.pagegroups[g] != NULL)
	{
	    struct pagehdr hdrs[PAGESPERGROUP];
	    struct pagegroupinfo &pgi = *gc.pagegroups[g];

	    for (unsigned int p = 0; p < PAGESPERGROUP; p++)
	    {
		struct pageinfo &pi = pgi.pages[p];
		int sz = pi.blocksize;
		struct pagehdr &ph = hdrs[p];
		ph.blocksize = sz;
		ph.numblocks = pi.numblocks;
		ph.atomic = pi.atomic;
		ph.snapoffset = 0;

		if (gc.snapshot)
		    ph.snapoffset = getoffset(snaptable, hdr.snapcount,
			    pi.type);

		if (sz > 0 && sz < (BYTESPERPAGE / BITSPERLONG))
		    ph.freemapoffset = getoffset(bitmaptable, n, pi.freemap);
		else
		    ph.freemap = pi.freebitmap;
	    }

	    fprintf(stderr, "gcwrite: about to write page headers for group %d\n", g);

	    if (fwrite(hdrs, sizeof (struct pagehdr), PAGESPERGROUP, fp) !=
		    PAGESPERGROUP)
	    {
	    	fclose(fp);
		gc.inalloc = FALSE;
		fprintf(stderr, "gcwrite: about to write of page header failed\n");
		return FALSE;
	    }
	}

    fprintf(stderr, "gcwrite: about to rewrite header\n");
//    fclose(fp);
//    fp = fopen(filename, "rb+");

    // rewrite header
    if (fseek(fp, 0L, SEEK_SET) != 0)
    {
    	fclose(fp);
	gc.inalloc = FALSE;
	return FALSE;
    }

    if (fwrite(&hdr, sizeof hdr, 1, fp) != 1)
    {
    	fclose(fp);
	gc.inalloc = FALSE;
	return FALSE;
    }

    fclose(fp);
    gc.inalloc = FALSE;
    return TRUE;
}

static boolean
fseekread(void *buf, long offset, size_t blocksz, size_t blockcnt, FILE *fp)
{
    if (fseek(fp, offset, SEEK_SET) != 0)
    	return FALSE;

    if (fread(buf, blocksz, blockcnt, fp) != blockcnt)
	return FALSE;

    return TRUE;
}

void
gcreleaseheapinfo(Stack_pool *&stackpool, int pagesize, int numpagegroups,
	struct pagegroupinfo **&pagegroups, uLong **bitmaps = NULL,
	char **strings = NULL)
{
    char *stringptr = NULL;
    uLong *bitmapptr = NULL;

    if (strings == NULL)
	strings = &stringptr;

    if (bitmaps == NULL)
    	bitmaps = &bitmapptr;

    int pagespergroup = pagesize / sizeof (struct pageinfo);
    int blocksizelimit = pagesize / BITSPERLONG;

    // find begining of strings
    if (stackpool != NULL && *strings == NULL)
    {	
	// These curly's must be here so that Stack_iter dies at the end of 
	// this block

    	for (Stack_iter si = *stackpool; si; si++)
	{
	    Stack_elt &se = si();

	    if (se.file != NULL && (*strings == NULL || se.file < *strings))
		*strings = (char *)se.file;
	}
    }

    // search for the first bitmap
    if (pagegroups != NULL && *bitmaps == NULL)
	for (int g = 0; *bitmaps == NULL && g < numpagegroups; g++)
	    if (pagegroups[g] != NULL)
	    {
	    	struct pagegroupinfo &pagegroup = *pagegroups[g];

	    	for (int p = 0; p < pagespergroup; p++)
		{
		    struct pageinfo &pi = pagegroup.pages[p];

		    if (pi.blocksize > 0 && pi.blocksize < blocksizelimit &&
			    pi.freemap != NULL)
		    {
		    	*bitmaps = pagegroup.pages[p].freemap;
			break;
		    }
		}
	    }

    delete *strings;
    delete *bitmaps;
    delete stackpool;

    if (pagegroups != NULL)
    {
	int g;

	for (g = 0; g < numpagegroups; g++)
	    delete[/*pagespergroup*/] (struct pageinfo *)pagegroups[g];

	delete[/*numpagegroups*/] pagegroups;
    }

    *strings = NULL;
    *bitmaps = NULL;
    stackpool = NULL;
    pagegroups = NULL;
}

#ifdef _DJGPP_SOURCE_
// see comment below
void
dummy(uLong bs1, uLong nb1, int bs2, int nb2)
{
}
#endif

boolean
gcread(char const *filename, int &pagesize, int &numpagegroups,
	Stack_pool *&stackpool, struct pagegroupinfo **&pagegroups)
{
    if (filename == NULL)
	filename = "mem.out";

    FILE *fp = fopen(filename, "rb");

    if (fp == NULL)
    	return FALSE;

    struct header hdr;

    if (fread(&hdr, sizeof hdr, 1, fp) != 1)
    {
	fclose(fp);
	return FALSE;
    }

    if (hdr.magic != MEM_MAGIC ||
	    hdr.snapsize != sizeof (Stack_elt) ||
	    hdr.bitmapwordsize != sizeof (uLong) ||
	    hdr.hdrsize != sizeof (struct pageinfo) ||
	    hdr.stringchars > 10000000 ||
	    hdr.snapcount > 1000000 ||
	    hdr.bitmapwords > 1000000 ||
	    hdr.hdrpages > 10000)
    {
    	fclose(fp);
    	return FALSE;
    }

    pagesize = hdr.pagesize;
    numpagegroups = hdr.hdrpages;
    stackpool = NULL;
    pagegroups = NULL;
    struct ptrmap snaptable[hdr.snapcount];
    char *strings = NULL;

    if (hdr.stringchars)
    {
	strings = new char[hdr.stringchars];

	if (!fseekread(strings, hdr.stringoffset, sizeof (char),
		hdr.stringchars, fp))
	{
	    fclose(fp);
	    gcreleaseheapinfo(stackpool, pagesize, numpagegroups, pagegroups,
		    NULL, &strings);
	    return FALSE;
	}
    }

    // read stack pool
    if (hdr.snapcount)
    {
	Stack_elt stackitems[hdr.snapcount];

	if (!fseekread(stackitems, hdr.snapoffset, sizeof (Stack_elt),
		hdr.snapcount, fp))
	{
	    fclose(fp);
	    gcreleaseheapinfo(stackpool, pagesize, numpagegroups, pagegroups,
		    NULL, &strings);
	    return FALSE;
	}

	stackpool = new Stack_pool;

	for (unsigned int i = 0; i < hdr.snapcount; i++)
	{
	    long fileoffset = (long)stackitems[i].file;

	    if (fileoffset != (long)NULL)
		stackitems[i].file = &strings[fileoffset - hdr.stringoffset];
	    else
		stackitems[i].file = NULL;

	    snaptable[i].ptr = &((*stackpool)[stackitems[i]]);
	    snaptable[i].offset = hdr.snapoffset + sizeof (Stack_elt) * i;
	}

	qsort(snaptable, hdr.snapcount, ptrmapoffsetcmp);
    }

    // read bitmaps
    uLong *bitmaps = NULL;

    if (hdr.bitmapwords)
    {
    	bitmaps = new uLong[hdr.bitmapwords];

	if (!fseekread(bitmaps, hdr.bitmapoffset, sizeof (uLong),
		hdr.bitmapwords, fp))
	{
	    fclose(fp);
	    gcreleaseheapinfo(stackpool, pagesize, numpagegroups, pagegroups,
		    &bitmaps);
	    return FALSE;
	}
    }

    // read page index
    uInt pageoffsets[numpagegroups];

    if (!fseekread(pageoffsets, hdr.hdrpageptrsoffset, sizeof (uInt),
	    numpagegroups, fp))
    {
	fclose(fp);
	gcreleaseheapinfo(stackpool, pagesize, numpagegroups, pagegroups,
		&bitmaps);
	return FALSE;
    }

    pagegroups = new pagegroupinfo *[numpagegroups];
    int pagespergroup = hdr.pagesize / sizeof (struct pageinfo);
    struct pagehdr pageheaders[pagespergroup];
    int g;

    for (g = 0; g < numpagegroups; g++)
	pagegroups[g] = NULL;

    for (g = 0; g < numpagegroups; g++)
    	if (pageoffsets[g] != 0)
	{
	    // read page headers
	    if (!fseekread(pageheaders, pageoffsets[g], sizeof (struct pagehdr),
		    pagespergroup, fp))
	    {
		fclose(fp);
		gcreleaseheapinfo(stackpool, pagesize, numpagegroups,
			pagegroups, &bitmaps);
		return FALSE;
	    }
		
	    struct pageinfo *pagegroup = new pageinfo[pagespergroup];
	    pagegroups[g] = (struct pagegroupinfo *)pagegroup;

	    for (int p = 0; p < pagespergroup; p++)
	    {
		struct pagehdr &ph = pageheaders[p];
	    	struct pageinfo &pi = pagegroup[p];
#ifdef _DJGPP_SOURCE_
		// this is a workaround for BIZARRE problem on
		// my Everex Tempo Carrier 386/20!  The problem is
		// that ph.blocksize is read incorrectly for the
		// assignment to pi.blocksize!  Adding the call to
		// dummy makes the assignment work correctly!  Why
		// this fixes the problem is your guess!
		dummy(ph.blocksize, ph.numblocks, pi.blocksize, pi.numblocks);
#endif
		pi.blocksize = ph.blocksize;
		pi.numblocks = ph.numblocks;
		pi.page = getpage(g, p);
		pi.link = NULL;
		pi.type = NULL;
		pi.history = 0;
		pi.atomic = ph.atomic;

		if (pi.blocksize > 0 &&
			pi.blocksize < (signed)(hdr.pagesize / BITSPERLONG))
		{
		    if (ph.freemapoffset != 0)
		    {
			int wordoffset = (ph.freemapoffset - hdr.bitmapoffset) /
				sizeof (uLong);
			pi.freemap = &bitmaps[wordoffset];
		    }
		    else
			pi.freemap = NULL;

		    pi.refmap = NULL;
		}
		else
		{
		    pi.freebitmap = ph.freemap;
		    pi.refbitmap = 0;
		}

		if (ph.snapoffset != 0)
		    pi.type = getpointer(snaptable, hdr.snapcount,
			    ph.snapoffset);
	    }
	}

    fclose(fp);
    return TRUE;
}
