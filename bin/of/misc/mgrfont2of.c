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

/*
 * Read an mgr font named on the command line or stdin and write out
 * a corresponding OpenFirmware font to stdout.
 * Only handle 16-bit wide fonts right now.
 *
 * Hacked from the MGR file "mgrfont2vga.c".
 */

#include <stdlib.h>
#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include "font.h"

int
main( argc, argv)
int argc;
char *argv[];
{
    int fd, size, pcount, count, row, uc, bwide, i;
    struct font_header fh;
    unsigned char *buf, zero;

    if (argc > 2)
    {
	fprintf(stderr, "usage: %s [mgrfontfilename]\n", argv[0]);
	exit(1);
    }

    if (argc < 2)
	fd = 0;
    else
    {
	char *filename = argv[1];

	if ((fd = open(filename, O_RDONLY)) < 0)
	{
	    fprintf(stderr, "%s: could not read file %s\n", argv[0],
		    filename);
	    exit(1);
	}
    }

    if (read(fd, &fh, sizeof (fh)) < sizeof (fh))
    {
	fprintf(stderr, "%s: no font header read\n", argv[0]);
	exit(1);
    }

    if (fh.type != FONT_A ||
	    (fh.wide != 8 && fh.wide != 16 && fh.wide != 24 && fh.wide != 32))
    {
	fprintf(stderr, "%s: not multiple of 8-bit fixed-width mgr font\n",
		argv[0]);
	exit(1);
    }

    if (fh.start > ' ' || fh.start + fh.count < '~')
    {
	fprintf(stderr, "%s: not enough printable ASCII chars\n", argv[0]);
	exit(1);
    }

    bwide = fh.wide / 8;
    count = (fh.count == 255 && fh.start == 0 && fh.wide < 32) ? 256 : fh.count;
    pcount = (count + 3) & ~3;
    buf = (unsigned char *)calloc(pcount * bwide, fh.high);
    size = fh.high * pcount * bwide;

    if (read(fd, buf, size) < size)
    {
	fprintf(stderr, "%s: font file short/corrupt\n", argv[0]);
	exit(1);
    }

    close(fd);

    if (count > pcount + fh.start)
	count = pcount + fh.start;

    fprintf(stderr,
	    "type=%d, wide=%d, high=%d, baseline=%d, count=%d, start=%d\n",
	    fh.type, fh.wide, fh.high, fh.baseline, fh.count, fh.start);
    fprintf(stderr, "pcount=%d size=%d\n", pcount, size);

    zero = 0;

    /* generate the font-file as a C array for #include by OF */
    for (uc = '!'; uc <= '~'; uc++)
    {
	printf("/* %c (%d) */ ", uc, uc);

	/* add a first row that is all zeros, as per the OF spec */
	for (i = 0; i < bwide; i++)
	    printf("0x00, ");

	for (row = 0; row < fh.high; row++)
	{
	    if (row)
		printf(", ");

	    for (i = 0; i < bwide - 1; i++)
		printf("0x%.2X, ",
		    buf[(row * pcount + (uc - fh.start)) * bwide + i]);

	    printf("0x%.2X", buf[(row * pcount + (uc - fh.start)) * bwide + i]);
	}

	/* OF assumes/adds a virtual last row that is all zeros */

	if (uc < '~')
	    printf(",");

	printf("\n");
    }

    return 0;
}
