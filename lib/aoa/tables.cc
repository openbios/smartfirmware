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
#include <stdio.h>

void
usage()
{
    fprintf(stderr, "Usage: tables pagesize alignment headersize\n");
    exit(2);
}

int
main(int argc, char **argv)
{
    int verbose = 0;

    if (strcmp(argv[1], "-v") == 0)
    {
    	verbose = 1;
	argc--;
	argv++;
    }

    if (argc != 4)
    	usage();

    long pagesize = atoi(argv[1]);
    long alignment = atoi(argv[2]);
    long header = atoi(argv[3]);

    long maxclasses = pagesize / alignment;
    long *blocksizes = new long[maxclasses];
    int blkclass = 0;

    long alignedheader = ((header + alignment - 1) / alignment) * alignment;
    long threshhold = (pagesize - alignedheader) / 2;
    threshhold -= threshhold % alignment;
    blocksizes[blkclass++] = threshhold;
    long blocksize;

    for (int bpp = 3; ; bpp++)
    {
    	blocksize = (pagesize - alignedheader) / bpp;
	blocksize -= blocksize % alignment;

	if (blocksize == blocksizes[blkclass - 1])
	    break;

	blocksizes[blkclass++] = blocksize;
    }

    for (blocksize -= alignment; blocksize > 0; blocksize -= alignment)
    	blocksizes[blkclass++] = blocksize;

    printf("\n#if PAGE_SIZE == %ld && ALIGNMENT == %ld && PAGE_OVERHEAD == %ld\n\n",
	    pagesize, alignment, alignedheader);
    printf("#define NUM_CLASSES %ld\n", blkclass);
    printf("#define THRESHHOLD %ld\n\n", threshhold);
    printf("static int g_blocksizes[NUM_CLASSES] =\n{\n");
    int col = 3;

    if (!verbose)
    	printf("   ");

    for (int i = blkclass - 1; i >= 0; i--)
    {
    	blocksize = blocksizes[i];
	long bpp = (pagesize - alignedheader) / blocksize;
	long waste = pagesize - alignedheader - (blocksize * bpp);
	int ovpercent = ((alignedheader + waste) * 10000) / pagesize;

	if (verbose)
	{
	    if (blocksize < 100)
		printf("    %d,\t\t", blocksize);
	    else
		printf("    %d,\t", blocksize);

	    printf("// bpp = %4ld, waste = %3ld, overhead = %d.%02d%%\n", bpp,
		    waste, ovpercent / 100, ovpercent % 100);
	}
	else
	{
	    int w;

	    if (blocksize < 10)
	    	w = 3;
	    else if (blocksize < 100)
	    	w = 4;
	    else if (blocksize < 1000)
	    	w = 5;
	    else if (blocksize < 10000)
	    	w = 6;
	    else
	    	w = 7;

	    if (col + w >= 75)
	    {
		printf(" %d,\n   ", blocksize);
		col = w + 3;
	    }
	    else
	    {
	    	printf(" %d,", blocksize);
		col += w;
	    }
	}
    }

    if (!verbose)
    	printf("\n");

    printf("}\n\n");
    printf("static int g_blockcount[NUM_CLASSES] =\n{\n");
    col = 3;

    if (!verbose)
    	printf("   ");

    for (i = blkclass - 1; i >= 0; i--)
    {
    	blocksize = blocksizes[i];
	long bpp = (pagesize - alignedheader) / blocksize;
	long waste = pagesize - alignedheader - (blocksize * bpp);
	int ovpercent = ((alignedheader + waste) * 10000) / pagesize;

	if (verbose)
	{
	    if (blocksize < 100)
		printf("    %d,\t\t", bpp);
	    else
		printf("    %d,\t", bpp);

	    printf("// bpp = %4ld, waste = %3ld, overhead = %d.%02d%%\n", bpp,
		    waste, ovpercent / 100, ovpercent % 100);
	}
	else
	{
	    int w;

	    if (bpp < 10)
	    	w = 3;
	    else if (bpp < 100)
	    	w = 4;
	    else if (bpp < 1000)
	    	w = 5;
	    else if (bpp < 10000)
	    	w = 6;
	    else
	    	w = 7;

	    if (col + w >= 75)
	    {
		printf(" %d,\n   ", bpp);
		col = w + 3;
	    }
	    else
	    {
	    	printf(" %d,", bpp);
		col += w;
	    }
	}
    }

    if (!verbose)
    	printf("\n");

    printf("}\n\n");

    char *type;

    if (blkclass <= 256)
    	type = "char";
    else if (blkclass <= 65536)
    	type = "short";
    else
    	type = "int";

    printf("static unsigned %s g_sizemap[THRESHHOLD + 1] =\n{\n", type);
    int ind = blkclass - 1;
    col = 3;

    if (!verbose)
    	printf("   ");

    for (long sz = 0; sz <= threshhold; sz++)
    {
    	if (verbose)
	    printf("    %d,\t\t// sz = %4ld, blksz = %4ld, waste = %3ld\n",
		    blkclass - ind - 1, sz, blocksizes[ind],
		    blocksizes[ind] - sz);
	else
	{
	    int t = blkclass - ind - 1;
	    int w;

	    if (t < 10)
	    	w = 3;
	    else if (t < 100)
	    	w = 4;
	    else if (t < 1000)
	    	w = 5;
	    else if (t < 10000)
	    	w = 6;
	    else
	    	w = 7;

	    if (col + w >= 75)
	    {
		printf(" %d,\n   ", t);
		col = w + 3;
	    }
	    else
	    {
	    	printf(" %d,", t);
		col += w;
	    }
	}

	if (blocksizes[ind] == sz)
	    ind--;
    }

    if (!verbose)
    	printf("\n");

    printf("}\n\n");
    printf("#endif // PAGE_SIZE == %ld && ALIGNMENT == %ld && PAGE_OVERHEAD == %ld\n\n",
	    pagesize, alignment, alignedheader);
}
